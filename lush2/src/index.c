/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA
 * 
 ***********************************************************************/

/******************************************************************************
 *
 *	INDEX.C.  Accessing storages as matrices...
 *
 ******************************************************************************/

/*
 * An index is a way to access a storage as a multi-dimensional object.
 * There are two kind of indexes:
 * 
 * 1) <struct index> are index on an arbitrary storage. They describe the
 *    starting offset, the dimensions, and their modulos.  
 * 
 *    The easiest way to access an index element consists in computing
 *    its offset, and using the <(*st->getat)> and <(*st->setat)> routines 
 *    of the related storage. This SLOW task is easily performed with the
 *    <easy_index_{check,set,get}()> routines.
 * 
 *    The second way consists in creating a <struct idx> for reading or
 *    for writing, with the <index_{read,write}_idx()> functions. After
 *    having used these idxs, a call to <index_rls_idx()> function releases
 *    them. 
 * 
 * 2) <struct X_idx> are "light" indexes that always refer to a piece of 
 *    memory. They describe the type of the objects, the starting address,
 *    the dimensions and the modulos, as well as two vectors setf and getf
 *    for accessing the data independently of the type.
 */

#include "header.h"
#include <inttypes.h>

typedef at* atp_t;

void clear_index(index_t *ind, size_t _)
{
   ind->st = NULL;
   ind->backptr = NULL;
}

void mark_index(index_t *ind)
{
   MM_MARK(ind->st);
   MM_MARK(ind->backptr);
}

static index_t *index_dispose(index_t *);

bool finalize_index(index_t *ind)
{
   /* XXX may go away when lisp_c is obsolete */
   index_dispose(ind);
   return true;
}

static mt_t mt_index = mt_undefined;

/* ------- THE CLASS FUNCTIONS ------ */

static subscript_t *parse_subscript(at *atss)
{
   static char errmsg_not_a_subscript[] = "not a valid subscript";
   static char errmsg_dimensions[] = "too many dimensions";
   
   subscript_t *ss = mm_blob(sizeof(struct subscript));

   if (INDEXP(atss) && index_numericp(Mptr(atss))) {
      index_t *ind = Mptr(atss);
      ifn (IND_NDIMS(ind)==1)
         error(NIL, errmsg_not_a_subscript, atss);
      ss->ndims = IND_DIM(ind,0);
      ifn (ss->ndims<MAXDIMS)
         error(NIL, errmsg_dimensions, atss);
      
      switch (IND_STTYPE(ind)) {
      case ST_I32: {
         int *b = IND_BASE_TYPED(ind, int);
         for (int i=0; i<ss->ndims; i++, b+=ind->mod[0])
            if (*b<0) 
               error(NIL, errmsg_not_a_subscript, atss);
            else
               ss->dim[i] = *b;
         break;
      }
      case ST_D: {
         double *b = IND_BASE_TYPED(ind, double);
         for (int i=0; i<ss->ndims; i++, b+=ind->mod[0])
            if (*b<0) 
               error(NIL, errmsg_not_a_subscript, atss);
            else
               ss->dim[i] = *b;
         break;
      }
      default: {
         gptr *b = IND_BASE(ind);
         double (*getr)(gptr,size_t) = storage_getr[IND_STTYPE(ind)];
         for (int i=0; i<ss->ndims; i++) {
            ss->dim[i] = (int)(*getr)(b, i*ind->mod[0]);
            if (ss->dim[i]<0) 
               error(NIL, errmsg_not_a_subscript, atss);
         }
      }
      }

   } else if (CONSP(atss)) {
      at *l = atss;
      int nd = 0;
      while(l) {
         ifn (CONSP(l) && NUMBERP(Car(l)))
            error(NIL, errmsg_not_a_subscript, atss);
         ifn (nd<MAXDIMS)
            error(NIL, errmsg_dimensions, atss);
      
         ss->dim[nd++] = (int)Number(Car(l));
         l = Cdr(l);
      }
      ss->ndims = nd;

   } else if (atss == NIL) {
      ss->ndims = 0;

   } else
      error(NIL, errmsg_not_a_subscript, atss);

   return ss;
}

static at *index_set(struct index*,at**,at*,int);
static at *index_ref(struct index*,at**);
static shape_t *parse_shape(at *, shape_t *);

static index_t *index_dispose(index_t *ind)
{
   if (ind->cptr)
      lside_destroy_item(ind->cptr);
   zombify(ind->backptr);
   return NULL;
}

static const char *index_name(at *p)
{
   index_t *ind = Mptr(p);
   char *s = string_buffer;
   
   if (IND_UNSIZEDP(ind)) {
      sprintf(s, "::%s:<unsized>", NAMEOF(Class(p)->classname));

   } else {
      sprintf(s, "::%s:<", NAMEOF(Class(p)->classname));
      while (*s)
         s++;
      for (int d=0; d<ind->ndim; d++) {
         sprintf(s, "%"PRIdPTR"x", ind->dim[d]);
         while (*s)
            s++;
      }
      if (s[-1]=='x')
         s[-1] = '>';
   }
   return mm_strdup(string_buffer);
}

static at *broadcast_and_put(index_t *ind, index_t *ss, index_t *vals)
{
   int d = IND_NDIMS(ind);
   if (IND_DIM(ss, IND_NDIMS(ss)-1) == d) {
      /* -------  Mode 1 -------- */
      ifn (IND_STTYPE(ind) == IND_STTYPE(vals))
         error(NIL, "element type of first and third array must match", NIL);
      IND_NDIMS(ss) -= 1;
      vals = index_broadcast1(vals, ss); 
      IND_NDIMS(ss) += 1;
      return array_put(ind, ss, vals)->backptr;

   } else {
      /* -------  Mode 2 -------- */
      index_t *inds = index_selectS(ind, parse_subscript(ss->backptr));
      vals = index_broadcast1(vals, inds); 
      array_copy(vals, inds);
      return ind->backptr;
   }
}

extern at **dx_sp; /* in function.c */

static at *index_listeval(at *p, at *q)
{
   index_t *ind = Mptr(p);
   
   if (IND_UNSIZEDP(ind))
      error(NIL, "unsized index", p);

   /* There are two subscript modes:
    * 1. The array-take/put-style subscription
    * 2. select*-style subscription with a single, partial subscript
    * 3. Single element subscription with full number of subscript indices
    */

   int d = IND_NDIMS(ind);
   at **args = eval_arglist_dx(Cdr(q));
   int n = (dx_sp - args);
   dx_sp = args;
   args++;

   if (n > IND_NDIMS(ind)+1) {
     error(NIL, "too many subscripts in subscript expression", q);

   } else if (n && INDEXP(args[0])) {
      if (IND_NDIMS(ind) == 0) {
         if (IND_STTYPE(ind) == ST_AT)
            goto mode_3;
         else
            error(NIL, "take-style subscription not valid for scalars", q);
      }
      /* -------  Mode 1 -------- */
      index_t *ss = Mptr(args[0]);
      if (IND_NDIMS(ss)<1)
         error(NIL, "subscript array must not be scalar", ss->backptr);

      if (IND_DIM(ss, IND_NDIMS(ss)-1) > d)
         error(NIL, "too many subscripts in subscript vector", ss->backptr);

      ss = as_int_array(ss->backptr);
      if (n == 1) {
         if (IND_NDIMS(ss)==1 && IND_DIM(ss, 0)<d)
            /* -------  Mode 2 -------- */
            return index_selectS(ind, parse_subscript(ss->backptr))->backptr;
         else
            return array_take2(ind, ss)->backptr;

      } else if (n == 2) {
         index_t *vals = NIL;
         if (IND_STTYPE(ind) == ST_DOUBLE) {
            vals = as_double_array(args[1]);
            return broadcast_and_put(ind, ss, vals);

         } else if (NUMBERP(args[1])) {
            vals = make_array(IND_STTYPE(ind), SHAPE0D, args[1]);
            return broadcast_and_put(ind, ss, vals);

         } else if (INDEXP(args[1])) {
            vals = Mptr(args[1]);
            return broadcast_and_put(ind, ss, vals);
            
         } else {
            error(NIL, "invalid argument for array update", args[1]);
            return NIL;
         }
         
      } else
         error(NIL, "not a valid subscript expression", q);

   } else if (n == (size_t)d) {
      /* ------- Mode 3 -------- */ 
      extern at *index_ref(index_t*, at**);
      return index_ref(ind, args);

   } else if (n == (size_t)(d+1)) {
      /* ------- Mode 3 -------- */ 
   mode_3: ;
      extern at *index_set(index_t*, at**, at*, int);
      index_set(ind, args, args[d], 1);
      return p;

   } else
      error(NIL, "not a valid subscript expression (enough subscripts?)", q);

   return NULL;
}   

static void index_serialize(at **pp, int code)
{
   at *atst = NIL;
   index_t *ind = NIL;
   if (code != SRZ_READ) {
      ind = Mptr(*pp);
      atst = IND_ATST(ind);
   }
   serialize_atstar(&atst, code);
   if (code == SRZ_READ) {
      ind = new_index((storage_t *)Mptr(atst), NIL);
      *pp = ind->backptr;
   }
   serialize_offset(&ind->offset, code);
   serialize_int(&IND_NDIMS(ind), code);
   
   for (int i=0; i<IND_NDIMS(ind); i++)
      serialize_size(&IND_DIM(ind, i), code);
   for (int i=0; i<IND_NDIMS(ind); i++)
      serialize_offset(&ind->mod[i], code);
}


static int index_compare(at *p, at *q, int order) 
{ 
   index_t *ind1 = Mptr(p);
   index_t *ind2 = Mptr(q);
   
   /* simple */
   if (order)
      error(NIL, "cannot rank matrices or arrays", NIL);
   if (IND_NDIMS(ind1) != IND_NDIMS(ind2))
      return 1;

   for (int i=0; i<IND_NDIMS(ind1); i++)
      if (IND_DIM(ind1, i) != IND_DIM(ind2, i))
         return 1;
   storage_type_t type1 = IND_STTYPE(ind1);
   storage_type_t type2 = IND_STTYPE(ind2);
   if (type1 != type2)
      if (type1 == ST_AT || type1 == ST_GPTR || 
          type2 == ST_AT || type2 == ST_GPTR )
         return 1;

   /* iterate */
   struct idx id1, id2;
   index_read_idx(ind1,&id1);
   index_read_idx(ind2,&id2);
   gptr *base1 = IDX_BASE(&id1);
   gptr *base2 = IDX_BASE(&id2);
   int ret = 1;

   switch (type1) {
   case ST_AT: {
      begin_idx_aloop2(&id1, &id2, off1, off2) {
         ifn (eq_test( ((at**)base1)[off1], ((at**)base2)[off2]))
            goto fail;
      } end_idx_aloop2(&id1, &id2, off1, off2); 
      ret = 0;
   }
   case ST_GPTR: {
      begin_idx_aloop2(&id1, &id2, off1, off2) {
         ifn (((gptr*)base1)[off1]==((gptr*)base2)[off2])
            goto fail;
      } end_idx_aloop2(&id1, &id2, off1, off2); 
      ret = 0;
   }
   default: {
      real (*get1)(gptr,size_t) = *storage_getr[type1];
      real (*get2)(gptr,size_t) = *storage_getr[type2];
      begin_idx_aloop2(&id1, &id2, off1, off2) {
         real r1 = (*get1)(base1, off1);
         real r2 = (*get2)(base2, off2);
         if (r1 != r2)
            goto fail;
#if defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)
         else {
            float delta = (float)(r1 - r2);
            if (*(long*)&delta)  // Has to do with NaNs
               goto fail;
         }
#endif
      } end_idx_aloop2(&id1, &id2, off1, off2); 
      ret = 0;
   }
   }
fail:
   index_rls_idx(ind1,&id1);
   index_rls_idx(ind2,&id2);
   return ret; 
}


static unsigned long index_hash(at *p)
{ 
   index_t *ind = Mptr(p);
   struct idx idx;
   index_read_idx(ind,&idx);
   gptr *base = IDX_BASE(&idx);
   unsigned long x = 0;
   
   switch IND_STTYPE(ind) {
   case ST_AT: {
      begin_idx_aloop1(&idx, off) {
         x = (x<<1) | ((long)x<0 ? 0:1);
         x ^= hash_value( ((at**)base)[off] );
      } end_idx_aloop1(&idx, off);
      break;
   }          
   case ST_GPTR: {
      begin_idx_aloop1(&idx, off) {
         x = (x<<1) | ((long)x<0 ? 0:1);
         x ^= (unsigned long) ( ((gptr*)base)[off] );
      } end_idx_aloop1(&idx, off);
      break;
   } 
   default: {
      union { real r; long l[2]; } u;
      real (*getr)(gptr,size_t) = *storage_getr[IND_STTYPE(ind)];
      
      begin_idx_aloop1(&idx, off) {
         x = (x<<1) | ((long)x<0 ? 0:1);
         u.r = (*getr)(base, off);
         x ^= u.l[0];
         if (sizeof(real) >= 2*sizeof(unsigned long))
            x ^= u.l[1];
      } end_idx_aloop1(&idx, off);      
      break;
   }
   }
   index_rls_idx(ind,&idx);
   return x; 
}


/* Scoping functions 
 * for accessing arrays and matrix using the following syntax:
 *
 *  :m:(i j)
 *  (setq :m:(i j) 3)
 */

static at *index_getslot(at *obj, at *prop)
{ 
   index_t *ind = Mptr(obj);
   
   /* Checks */
   at *p = Car(prop);
   ifn (LISTP(p))
      error(NIL, "subscript(s) expected with this object", obj);

   for (int i=0; i<IND_NDIMS(ind); i++) {
      ifn (CONSP(p))
         error(NIL, "not enough subscripts for array access", obj);
      p = Cdr(p);
   }
   if (p)
      error(NIL,"Too many subscripts for array access",obj);

  /* Access */
   at *arg = new_cons(obj, Cdr(prop));
   at *ans = Class(obj)->listeval(obj, arg);
   if (Cdr(prop)) {
      p = ans;
      ans = getslot(p, Cdr(prop));
   }
   return ans;
}

static void index_setslot(at *obj, at *prop, at *val)
{
   struct index *arr = Mptr(obj);

   /* Build listeval argument */
   at *p = Car(prop);
   if (!LISTP(p))
      error(NIL,"Subscript(s) expected with this object",obj);
   at *arg = new_cons(obj,NIL);
   at **where = &Cdr(arg);
   for (int i=0; i<arr->ndim; i++) {
      if (!CONSP(p)) 
         error(NIL,"not enough subscripts for array access",obj);
      *where = new_cons(Car(p), NIL);
      where = &Cdr(*where);
      p = Cdr(p);
   }
   if (p)
      error(NIL,"too many subscripts for array access",obj);
   /* Access */
   if (Cdr(prop)) {
      p = Class(obj)->listeval(obj, arg);
      setslot(&p, Cdr(prop), val);

   } else {
      *where = new_cons(val,NIL);
      Class(obj)->listeval(obj, arg);
   }
}

/* ---------- Argument checking ----------- */

/* chk-* functions check for an index property and return an      */
/* informative error message if the property does not hold.       */
/* The typical use is with RAISEF/RAISEFU:                        */
/* RAISEFU(chk_index_consistent(ind) index2at(ind));              */


static char *chk_index_consistent(index_t *ind)
{
   static char *msg_offset_negative = "offset is negative";
   static char *msg_extent_negative = "index is negative";
   static char *msg_storage_size =  "index references elements outside storage";
   
   if (ind->offset<0)
      return msg_offset_negative;

   ptrdiff_t size_min = ind->offset;
   ptrdiff_t size_max = ind->offset;
   for(int j=0; j<IND_NDIMS(ind); j++) {
      if (IND_DIM(ind, j)<0)
         return msg_extent_negative;
      if (IND_DIM(ind, j)>0) {
         if (IND_MOD(ind, j)<0)
            size_min += (IND_DIM(ind, j) - 1) * IND_MOD(ind, j);
         else
            size_max += (IND_DIM(ind, j) - 1) * IND_MOD(ind, j);
      }
   }
   if (size_min<0 || size_max>=IND_STNELEMS(ind))
      return msg_storage_size;
   
   return NULL;
}

static char *chk_contiguous(index_t *ind)
{
   static char *msg_not_contiguous = "index is not contiguous";
   ifn (index_contiguousp(ind))
      return msg_not_contiguous;
   else
      return NULL;
}

static char *chk_nonempty(index_t *ind)
{
   static char *msg_nonempty = "index is empty";
   if (index_emptyp(ind))
      return msg_nonempty;
   else
      return NULL;
}

/* static inline char * */
/* chk_sized(index_t *ind) */
/* { */
/*   static char *msg_unsized = "index is unsized"; */
/*   if (IND_UNSIZEDP(ind)) */
/*     return msg_unsized; */
/*   else */
/*     return NIL; */
/* } */

/* raise error if dimension d is invalid          */
/* yield equivalent non-negative value otherwise  */
static int validate_dimension(index_t *ind, int d)
{
   d = d<0 ? IND_NDIMS(ind)+d : d;
   if (d<0 || d>=IND_NDIMS(ind))
      RAISEF("invalid dimension", NEW_NUMBER(d));
   return d;
}

/* raise error if subscript s for dimension d is invalid */
/* yield equivalent non-negative value otherwise         */
/* NOTE: validate_subscript does not validate d          */
static size_t validate_subscript(index_t *ind, int d, ptrdiff_t s)
{
   s = s<0 ? IND_DIM(ind, d)+s : s;
   if (s<0 || s>=IND_DIM(ind, d))
      RAISEF("invalid subscript", NEW_NUMBER(s));
   return s;
}


/* -------- Shape auxiliaries  -------- */

shape_t *shape_set(shape_t *shp, int ndims, size_t d1, size_t d2, size_t d3, size_t d4) 
{
   static shape_t shape;
   
   if (shp==NIL)
      shp = &shape;
   shp->ndims = ndims;
   
   if (ndims<0 || ndims>4)
      RAISEF("number of dimensions invalid", NEW_NUMBER(ndims));
   if (ndims>0) shp->dim[0] = d1;
   if (ndims>1) shp->dim[1] = d2;
   if (ndims>2) shp->dim[2] = d3;
   if (ndims>3) shp->dim[3] = d4;
   
   return shp;
}

size_t shape_nelems(shape_t *shp) 
{
   size_t nelems = shp->ndims<0 ? 0 : 1;
   ifn (shp->ndims<MAXDIMS)
      RAISEF("too many dimensions", NEW_NUMBER(shp->ndims));
   
  for (int i=0; i<shp->ndims; i++)
     nelems *= shp->dim[i];
  
  return nelems;
}

shape_t *shape_copy(shape_t *shp_from, shape_t *shp_to) 
{
   shp_to->ndims = shp_from->ndims;
   for (int i=0; i<shp_to->ndims; i++)
      shp_to->dim[i] = shp_from->dim[i];
   
   return shp_to;
}

/*
static void shape_swap(shape_t *shp1, shape_t *shp2) 
{
   for (int i=0; i<MAXDIMS; i++) {
      size_t e = shp1->dim[i];
      shp1->dim[i] = shp2->dim[i];
      shp2->dim[i] = e;
   }
   int d = shp1->ndims;
   shp1->ndims = shp2->ndims;
   shp2->ndims = d;
}
*/
bool shape_equalp(shape_t *shp1, shape_t *shp2)
{
   if (shp1->ndims != shp2->ndims)
      return false;
   for (int i=0; i<shp1->ndims; i++)
      if (shp1->dim[i] != shp2->dim[i])
         return false;
   return true; 
}

bool same_shape_p(index_t *ind1, index_t *ind2)
{
   return shape_equalp((shape_t *)ind1, (shape_t *)ind2);
}

/* true if shapes are compatible (can be broadcasted) */

bool index_broadcastable_p(index_t *a, index_t *b) 
{
   int da = IND_NDIMS(a)-1;
   int db = IND_NDIMS(b)-1;
   
   for (; (da>=0) && (db>=0); da--, db--) {
      if (IND_DIM(a, da)==IND_DIM(b, db)) continue;
      if (IND_DIM(a, da)==1 || IND_DIM(b, db)==1) continue;
      return false;
   }
   return true;
}

DX(xidx_broadcastable_p) 
{
   ARG_NUMBER(2);
   if (index_broadcastable_p(AINDEX(1), AINDEX(2)))
      return t();
   else 
      return NIL;
}


/* shape_t * */
/* shape_broadcast(shape_t *shp1, shape_t *shp2) { */

/*   static shape_t shape, *shp = &shape; */

/*   if (shp1->ndims > shp2->ndims) { */
/*     if (shp2->ndims==0) return shp1; */
/*     shape_copy(shp1, shp); */

/*   }  else { */
/*     if (shp1->ndims==0) return shp2; */
/*     shape_copy(shp2, shp); */
/*     shp2 = shp1; */
/*   } */
/*   int i=shp->ndims; */
/*   int i2=shp2->ndims; */
/*   for (; i>=0; i--, i2--) { */
/*     intg di = shp->dim[i]; */
/*     intg di2 = shp2->dim[i2]; */
/*     if (di2==1) continue; */
/*     if (di2==di) continue; */
/*     if (di==1) shp->dim[i]=di2; */
/*     if (i2==0) */
/*       break; */
/*     return NIL; */
/*   } */
/*   return shp; */
/* } */


/* broadcast index blank to the shape of ref if possible or
 * return an error
 */
index_t *index_broadcast1(index_t *blank, index_t *ref)
{
   ifn (IND_NDIMS(blank) <= IND_NDIMS(ref))
      goto cannot_broadcast_error;
   
   index_t *b = copy_index(blank);
   if (IND_NDIMS(b) < IND_NDIMS(ref)) {
      for (int d = IND_NDIMS(ref)-1; d >= 0; d--) {
         IND_NDIMS(b)--;
         IND_DIM(b, d) = IND_NDIMS(b)>=0 ? IND_DIM(b, IND_NDIMS(b)) : 1;
         IND_MOD(b, d) = IND_NDIMS(b)>=0 ? IND_MOD(b, IND_NDIMS(b)) : 0;
      }
      IND_NDIMS(b) = IND_NDIMS(ref);
   }

   /* expand b */
   for (int d = IND_NDIMS(ref)-1; d >= 0; d--) {
      if (IND_DIM(b, d) == IND_DIM(ref, d)) {
         continue;
      } else if (IND_DIM(b, d) == 1) {
         IND_DIM(b, d) = IND_DIM(ref, d);
         IND_MOD(b, d) = 0;
      } else 
         goto cannot_broadcast_error;
   }
   return b;

cannot_broadcast_error:
   error(NIL, "cannot broadcast index to desired shape", blank->backptr);
   return b;
}

DX(xidx_broadcast1)
{
   ARG_NUMBER(2);
   return index_broadcast1(AINDEX(1), AINDEX(2))->backptr;
}

/* Broadcast two indexes and return the resulting shape; raise an error if
 * indexes are not compatible.
 * The broadcasted indices are returned through ba and bb, which must be
 * addresses of pointers to index_ts.
 */

shape_t *index_broadcast2(index_t *a, index_t *b, index_t **ba, index_t **bb) 
{
   ifn (index_broadcastable_p(a, b))
      RAISEF("indexes not broadcastable", NIL);
  
   ifn (*ba == NULL)
      RAISEF("*ba must be zero", NIL);

   ifn (*bb == NULL)
      RAISEF("*bb must be zero", NIL);

   a = *ba = copy_index(a);
   b = *bb = copy_index(b);

   if (IND_NDIMS(a) < IND_NDIMS(b)) {
      /* reshape a in-place */
      for (int d = IND_NDIMS(b)-1; d >= 0; d--) {
         IND_NDIMS(a)--;
         IND_DIM(a, d) = IND_NDIMS(a)>=0 ? IND_DIM(a, IND_NDIMS(a)) : 1;
         IND_MOD(a, d) = IND_NDIMS(a)>=0 ? IND_MOD(a, IND_NDIMS(a)) : 0;
      }
      IND_NDIMS(a) = IND_NDIMS(b);

   } else if (IND_NDIMS(b) < IND_NDIMS(a)) {
      /* reshape b in-place */
      for (int d = IND_NDIMS(a)-1; d >= 0; d--) {
         IND_NDIMS(b)--;
         IND_DIM(b, d) = IND_NDIMS(b)>=0 ? IND_DIM(b, IND_NDIMS(b)) : 1;
         IND_MOD(b, d) = IND_NDIMS(b)>=0 ? IND_MOD(b, IND_NDIMS(b)) : 0;
      }
      IND_NDIMS(b) = IND_NDIMS(a);
   }
   
   /* expand a or b */
   for (int d = IND_NDIMS(a)-1; d >= 0; d--)
      if (IND_DIM(a, d)==IND_DIM(b, d)) {
         continue;
      } else if (IND_DIM(a, d)==1) {
         IND_DIM(a, d) = IND_DIM(b, d);
         IND_MOD(a, d) = 0;
      } else if (IND_DIM(b, d)==1) {
         IND_DIM(b, d) = IND_DIM(a, d);
         IND_MOD(b, d) = 0;
      } else
         RAISEF("internal error (should never get here)", NIL);
  
   return IND_SHAPE(a);
}


DX(xidx_broadcast2)
{
   ARG_NUMBER(2);
   index_t *ba = NULL;
   index_t *bb = NULL;
   index_t *a = (NUMBERP(APOINTER(1))) ? make_array(ST_DOUBLE, SHAPE0D, APOINTER(1)) : AINDEX(1);
   index_t *b = (NUMBERP(APOINTER(2))) ? make_array(ST_DOUBLE, SHAPE0D, APOINTER(2)) : AINDEX(2);
   index_broadcast2(a, b, &ba, &bb);
   return bb->backptr;
}

/* -------- SIMPLE LISP FUNCTIONS ------- */

DX(xindexp)
{
   ARG_NUMBER(1);
   if (INDEXP(APOINTER(1)))
      return t();
   else
      return NIL;
}

bool index_emptyp(const index_t *ind)
{ 
   return index_nelems(ind)==0;
}

bool index_readonlyp(const index_t *ind)
{
   return storage_readonlyp(IND_ST(ind));
}

bool index_numericp(const index_t *ind)
{
   return !((IND_STTYPE(ind)==ST_AT) || (IND_STTYPE(ind)==ST_GPTR));
}

DX(xidx_numericp)
{
   ARG_NUMBER(1);
   if (index_numericp(AINDEX(1)))
      return t();
   else
      return NIL;
}

bool index_contiguousp(const index_t *ind)
{
   if (IND_UNSIZEDP(ind))
      RAISEF("unsized index", NIL);
   ptrdiff_t size = 1;
   for (int i=IND_NDIMS(ind)-1; i>=0; i--) {
      if (size != IND_MOD(ind, i))
         return false;
      size *= (ptrdiff_t)IND_DIM(ind, i);
   }
   return true;
}

DX(xidx_contiguousp)
{
   ARG_NUMBER(1);
   if (index_contiguousp(AINDEX(1)))
      return t();
   else
      return NIL;
}

DX(xidx_storage)
{
   ARG_NUMBER(1);
   return IND_ATST(AINDEX(1));
}

DX(xidx_dc)
{
   ARG_NUMBER(1);
   index_t *ind = AINDEX(1);
   RAISEF(chk_nonempty(ind), APOINTER(1));
   while (IND_NDIMS(ind)>0)
      ind = index_select(ind, 0, 0);
   return ind->backptr;
}

DX(xarray_dc)
{
   at *init = NIL;
   if (arg_number == 2) {
      init = APOINTER(2);

   } else if (arg_number > 2)
      ARG_NUMBER(-1);

   if (init) {
      return make_array(IND_STTYPE(AINDEX(1)), SHAPE0D, init)->backptr;

   } else {
      index_t *ind = AINDEX(1);
      RAISEF(chk_nonempty(ind), APOINTER(1));
      while (IND_NDIMS(ind)>0)
         ind = index_select(ind, 0, 0);
      return copy_array(ind)->backptr;
   }
}

size_t index_nelems(const index_t *ind) 
{
   size_t nelems = (IND_UNSIZEDP(ind)) ? 0 : 1;
   for (int i=0; i<IND_NDIMS(ind); i++) {
      if (nelems==0) break;
      nelems *= IND_DIM(ind, i);
   }
   return nelems;
}

DX(xidx_nelems)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(index_nelems(AINDEX(1)));
}


DX(xidx_rank)
{
   ARG_NUMBER(1);
   index_t *ind = AINDEX(1);
   if (IND_UNSIZEDP(ind))
      return NIL;
   else
      return NEW_NUMBER(IND_NDIMS(ind));
}

DX(xidx_offset)
{
   ARG_NUMBER(1);
   index_t *ind = AINDEX(1);  
   return NEW_NUMBER(ind->offset);
}


DX(xidx_shape)
{
   if (arg_number<1 || arg_number>2)
      ARG_NUMBER(-1);
   index_t *ind = AINDEX(1);
  
   if (arg_number==1) {
      at *p = NIL;
      for (int n=IND_NDIMS(ind)-1; n>=0; n--)
         p = new_cons(NEW_NUMBER(IND_DIM(ind, n)), p);
      return p;
      /*
        index_t *indres = make_array(ST_ID, SHAPE1D(IND_NDIMS(ind)), NIL);
        id_t *dres = IND_BASE_TYPED(indres, id_t);
        int n;
        for (n=0; n<IND_NDIMS(ind); n++)
        dres[n] = IND_DIM(ind, n);
        return index2at(indres);
      */
   } else /* arg_number==2 */ {
      int n = AINTEGER(2);
      if (n<0)
         n += IND_NDIMS(ind);
      if (n<0 || n>=IND_NDIMS(ind))
         RAISEF("illegal dimension", APOINTER(2));
      return NEW_NUMBER(IND_DIM(ind, n));
   }
}

index_t *index_shape(const index_t *ind)
{
   index_t *shape = make_array(ST_INT, SHAPE1D(IND_NDIMS(ind)), NIL);
   int *sp = IND_BASE(shape);
   for (int i=0; i<IND_NDIMS(ind); i++)
      sp[i] = (int)IND_DIM(ind, i);
   return shape;
}

DX(xindex_shape)
{
   index_t *ind = NULL;
   if (arg_number == 2) {
      ind = AINDEX(1);
      int d = validate_dimension(ind, AINTEGER(2));
      return NEW_NUMBER(IND_DIM(ind, d));

   } else if (arg_number == 1) {
      return index_shape(AINDEX(1))->backptr; 

   } else
      ARG_NUMBER(-1);

   return NIL;
}

DX(xindex_reshape)
{
   ARG_NUMBER(2);
   shape_t *shp = parse_shape(APOINTER(2), NULL);
   index_t *ind = index_reshape(AINDEX(1), shp);
   return ind->backptr;
}


DX(xidx_modulo)
{
   if (arg_number<1 || arg_number>2) 
      ARG_NUMBER(-1);
   
   index_t *ind = AINDEX(1);
   
   if (arg_number==1) {
      at *p = NIL;
      for (int n=IND_NDIMS(ind)-1; n>=0; n--)
         p = new_cons(NEW_NUMBER(ind->mod[n]), p);
      return p;
      /*
        index_t *indres = make_array(ST_ID, SHAPE1D(IND_NDIMS(ind)), NIL);
        id_t *dres = IND_BASE_TYPED(indres, id_t);
        int n;
        for (n=0; n<IND_NDIMS(ind); n++)
        dres[n] = ind->mod[n];
        return index2at(indres);
      */
   } else /* arg_number==2 */ {
      int n = AINTEGER(2);
      if (n<0)
         n += IND_NDIMS(ind);
      if (n<0 || n>=IND_NDIMS(ind))
         RAISEF("illegal dimension", APOINTER(2));
      return NEW_NUMBER(ind->mod[n]);
   }
}


DX(xidx_base)
{
   ARG_NUMBER(1);
   index_t *ind = AINDEX(1);
   return NEW_GPTR(IND_BASE(ind));
}


/* ========== CREATION FUNCTIONS ========== */

static shape_t *parse_shape(at *atshp, shape_t *shp)
{
   static char errmsg_not_a_shape[] = "not a shape";
   static char errmsg_dimensions[] = "too many dimensions";
   static shape_t shape;
  
   if (shp==NIL)
      shp = &shape;
   
   if (INDEXP(atshp) && index_numericp(Mptr(atshp))) {
      index_t *ind = Mptr(atshp);
      ifn (IND_NDIMS(ind)==1)
         error(NIL, errmsg_not_a_shape, atshp);
      shp->ndims = IND_DIM(ind, 0);
      ifn (shp->ndims<MAXDIMS)
         error(NIL, errmsg_dimensions, atshp);

      switch (IND_STTYPE(ind)) {
      case ST_I32: {
         int *b = IND_BASE_TYPED(ind, int);
         for (int i=0; i<shp->ndims; i++, b+=ind->mod[0])
            if (*b<0) 
               error(NIL, errmsg_not_a_shape, atshp);
            else
               shp->dim[i] = *b;
         break;
      }
      case ST_F: {
         flt *b = IND_BASE_TYPED(ind, flt);
         for (int i=0; i<shp->ndims; i++, b+=ind->mod[0])
            if (*b<0) 
               error(NIL, errmsg_not_a_shape, atshp);
            else
               shp->dim[i] = (int)*b;
         break;
      }
      default: {
         gptr *b = IND_BASE(ind);
         real (*getr)(gptr,size_t) = storage_getr[IND_STTYPE(ind)];
         for (int i=0; i<shp->ndims; i++) {
            shp->dim[i] = (int)(*getr)(b, i*ind->mod[0]);
            if (shp->dim[i]<0) 
               error(NIL, errmsg_not_a_shape, atshp);
         }
      }
      }
   } else if (LISTP(atshp)) {
      at *l = atshp;
      int nd = 0;
      while(l) {
         ifn (CONSP(l) && NUMBERP(Car(l)) )
            error(NIL, errmsg_not_a_shape, atshp);
         ifn (nd<MAXDIMS)
            error(NIL, errmsg_dimensions, atshp);
         
         shp->dim[nd++] = (int)Number(Car(l));
         l = Cdr(l);
      }
      shp->ndims = nd;
   } else
      error(NIL, errmsg_not_a_shape, atshp);
   
   return shp;
}

/* ---------- Index C-API ----------- */

/* create new index for storage st */ 
index_t *new_index(storage_t *st, shape_t *shp)
{
   size_t stnelems = st->size;
   size_t shpnelems = shp ? shape_nelems(shp) : stnelems;
   if (shp==NIL)
      shp = SHAPE1D(stnelems);
   
   else if (shpnelems>stnelems)
      RAISEF("storage too small for new index", NEW_NUMBER(stnelems));
   
   
   index_t *ind = mm_alloc(mt_index);
   if (st->data==NULL) {
      assert(stnelems==0);
      IND_NDIMS(ind) = -1;
   } else {
      IND_NDIMS(ind) = 1;
      IND_DIM(ind,0) = shpnelems;
      IND_MOD(ind,0) = 1;
   }
   IND_ST(ind) = st;
   ind->offset = 0;
   ind->cptr = NULL;
   ind->backptr = new_extern(&index_class, ind);
   if (shp->ndims != 1) 
      ind = index_reshapeD(ind, shp);
   return ind;
}

DX(xnew_index)
{
   if (arg_number<1 || arg_number>2)
      ARG_NUMBER(-1);
   storage_t *st = ASTORAGE(1);
   
   if (arg_number==2) {
      shape_t *shp = parse_shape(APOINTER(2), NIL);
      return NEW_INDEX(st, shp);
   } else
      return NEW_INDEX(st, NIL);
}

/*
 * Create an index referencing contiguous data
 */

index_t *new_index_for_cdata(storage_type_t type, shape_t *shp, void *data)
{
   storage_t *st = new_storage(type);
   st->data = data;
   st->flags = STS_MALLOC;
   st->size = shape_nelems(shp);
   
   return new_index(st, shp);
}

index_t *make_array(storage_type_t type, shape_t *shp, at *init)
{
   /* create a storage of the right size */
   size_t nelems = shape_nelems(shp);
   nelems = (nelems<MINSTORAGE) ? MINSTORAGE : nelems;
   storage_t *st = make_storage(type, nelems, init);
   index_t *res = new_index(st, shp);
   return res;
}

DX(xmake_array)
{
   ARG_NUMBER(3);
   class_t *cl = ACLASS(1);
   shape_t *shp   = parse_shape(APOINTER(2), NIL);
   storage_type_t type;
   for (type = ST_AT; type < ST_LAST; type++)
      if (&storage_class[type] == cl)
         break;
   if (type == ST_LAST)
      RAISEF("not a storage class", APOINTER(1));

   return make_array(type, shp, APOINTER(3))->backptr;
}


/* make new array from prototype */

index_t *clone_array(index_t *ind)  
{ 
   return make_array(IND_STTYPE(ind), IND_SHAPE(ind), NIL);
}

/* ---- argument conversion ----- */

index_t *as_contiguous_array(index_t *ind) 
{
   if (index_contiguousp(ind))
      return copy_index(ind);
   else
      return copy_array(ind);
}


index_t *as_double_array(at *arg) 
{
   if (NUMBERP(arg))
      return  make_array(ST_DOUBLE, SHAPE0D, arg);

   else if (LISTP(arg)) {
      index_t *ind = make_array(ST_DOUBLE, SHAPE1D(length(arg)), NIL);
      at *myp[1] = { NIL };
      index_set(ind, myp, arg, 1);
      return ind;

   } else if (INDEXP(arg)) {
      index_t *ind = Mptr(arg);
      if (IND_STTYPE(ind)==ST_DOUBLE)
         return copy_index(ind);
      else
         return array_copy(ind, make_array(ST_DOUBLE, IND_SHAPE(ind), NIL));

   } else
      RAISEF("not a number, list, or index", arg);

   return NULL;  
}      


DX(xas_double_array) 
{
   ARG_NUMBER(1);
   index_t *ind = as_double_array(APOINTER(1));
   return ind->backptr;
}

index_t *as_int_array(at *arg) 
{
   if (NUMBERP(arg))
      return  make_array(ST_INT, SHAPE0D, arg);

   else if (LISTP(arg)) {
      index_t *ind = make_array(ST_INT, SHAPE1D(length(arg)), NIL);
      at *myp[1] = { NIL };
      index_set(ind, myp, arg, 1);
      return ind;

   } else if (INDEXP(arg)) {
      index_t *ind = Mptr(arg);
      if (IND_STTYPE(ind)==ST_INT)
         return copy_index(ind);
      else
         return array_copy(ind, make_array(ST_INT, IND_SHAPE(ind), NIL));

   } else
      RAISEF("not a number, list, or index", arg);
   
   return NULL;  
}

DX(xas_int_array)
{
   ARG_NUMBER(1);
   return as_int_array(APOINTER(1))->backptr;
}


index_t *as_ubyte_array(at *arg) 
{
   if (NUMBERP(arg))
      return  make_array(ST_U8, SHAPE0D, arg);

   else if (LISTP(arg)) {
      index_t *ind = make_array(ST_U8, SHAPE1D(length(arg)), NIL);
      at *myp[1] = { NIL };
      index_set(ind, myp, arg, 1);
      return ind;
      
   } else if (INDEXP(arg)) {
      index_t *ind = Mptr(arg);
      if (IND_STTYPE(ind)==ST_U8)
         return copy_index(ind);
      else
         return array_copy(ind, make_array(ST_U8, IND_SHAPE(ind), NIL));
      
   } else
      RAISEF("not a number, list, or index", arg);
   
   return NULL;  
}

DX(xas_ubyte_array)
{
   ARG_NUMBER(1);
   return as_ubyte_array(APOINTER(1))->backptr;
}

/* -------- THE REF/SET FUNCTIONS ------- */


/*
 * Loop to access the index elements
 */

static at *index_ref(index_t *ind, at *p[])
{
   at *myp[MAXDIMS];

   /* 1: numeric arguments only */
   
   storage_t *st = ind->st;
   ptrdiff_t offs = ind->offset;
   int i = 0;
   at *q;
   for (; i<ind->ndim; i++) {
      q = p[i];
      if (NUMBERP(q)) {
         size_t k = validate_subscript(ind, i, Number(q));
         offs += k*IND_MOD(ind, i);
      } else
         goto list_ref;
   }
   return storage_getat[st->type](st, offs);


   /* 2: found a non numeric argument */

list_ref:

   for (int j = 0; j < ind->ndim; j++)
      myp[j] = p[j];

   int start, end;
   if (!q) {
      start = 0;
      end = ind->dim[i] - 1;
   } else if (LISTP(q) && CONSP(Cdr(q)) && !Cddr(q) &&
              NUMBERP(Car(q)) && NUMBERP(Cadr(q))) {
      start = Number(Car(q));
      end = Number(Cadr(q));
   } else
      error(NIL, "illegal subscript", NIL);

   at *ans = NIL;
   at **where = &ans;
   myp[i] = NEW_NUMBER(0);
   for (int j = start; j <= end; j++) {
      Number(myp[i]) = j;
      *where = new_cons(index_ref(ind, myp), NIL);
      where = &Cdr(*where);
      CHECK_MACHINE("on");
   }
   return ans;
}


/*
 * Loop to modify index elements.
 */

static at *index_set(struct index *ind, at *p[], at *value, int mode)
{				
   /* MODE: 0 means 'value is constant'	       */
   /*       1 means 'get values from sublists'  */
   /*       2 means 'get values in sequence'    */

   int start, end;
   storage_t *st;
   at *myp[MAXDIMS];

   /* 1: numeric arguments only */

   at *q = NIL;
   ptrdiff_t offs = ind->offset;
   int i = 0;
   for (; i < ind->ndim; i++) {
      q = p[i];
      if (NUMBERP(q)) {
         size_t k = validate_subscript(ind, i, Number(q));
         offs += k*IND_MOD(ind, i);
      } else
         goto list_set;
   }
   
   if (mode == 2) {
      q = Car(value);
      value = Cdr(value);
   } else {
      q = value;
      value = NIL;
   }

   st = ind->st;
   (storage_setat[st->type])(st, offs, q);
   return value;
  
   /* 2 found a non numeric argument */
   
 list_set:
  
   for (int j = 0; j < ind->ndim; j++)
      myp[j] = p[j];
   
   if (!q) {
      start = 0;
      end = ind->dim[i] - 1;
   } else if (LISTP(q) && CONSP(Cdr(q)) && !Cddr(q) &&
              NUMBERP(Car(q)) &&  NUMBERP(Cadr(q)) ) {
      start = Number(Car(q));
      end = Number(Cadr(q));
   } else
      error(NIL, "illegal subscript", NIL);
      
   if (mode == 1)
      if (length(value) > end - start + 1)
         mode = 2;
   
   myp[i] = NEW_NUMBER(0);
   for (int j = start; j <= end; j++) {
      Number(myp[i]) = j;
      ifn(CONSP(value))
         mode = 0;
      switch (mode) {
      case 0:
         index_set(ind, myp, value, mode);
         break;
      case 1:
         index_set(ind, myp, Car(value), mode);
         value = Cdr(value);
         break;
      case 2:
         value = index_set(ind, myp, value, mode);
         break;
      }
      CHECK_MACHINE("on");
   }
   return value;
}


/* ----------- THE EASY INDEX ACCESS ----------- */

/*
 * easy_index_check(p,ndim,dim)
 *
 * Check that index <p> has <ndim> dimensions.
 * If dim[n] contains a non-zero value, <ind> must have the 
 * specified <n>th dimension. 
 * If dim[n] contains a zero value, the <n>th dimension of 
 * <ind> is returned in dim[i].
 *
 * Raise error if <ind> is an unsized index.
 */

void easy_index_check(index_t *ind, shape_t *shp)
{
   static char message[40];
   char *msg = message;
   
   if (IND_UNSIZEDP(ind))
      RAISEF("index is unsized", NIL);

   if (shp->ndims != IND_NDIMS(ind)) {
      sprintf(msg,"%dD index expected",shp->ndims);
      RAISEF(msg, NIL);
   }

   for (int i=0; i<shp->ndims; i++)
      if (shp->dim[i]) {
         if (shp->dim[i] != IND_DIM(ind, i)) {
            sprintf(msg,"illegal dimension #%d",i);
            RAISEF(msg, NIL);
         }
      } else
         shp->dim[i] = IND_DIM(ind, i);
}

/*
 * index_read_idx()
 * index_write_idx()
 * index_rls_idx()
 *
 * Given the index, these function return a
 * idx structure pointing to the data area;
 */


static void index_to_idx(index_t *ind, struct idx *idx)
{
   idx->ndim = ind->ndim;
   idx->dim = ind->dim;
   idx->mod = ind->mod;
   idx->offset = ind->offset;
   idx->srg = (struct srg *)ind->st;
}

static void idx_to_index(struct idx *idx, index_t *ind)
{
   ind->ndim = idx->ndim;
   for(int i=0;i<idx->ndim;i++) {
      ind->mod[i] = idx->mod[i];
      ind->dim[i] = idx->dim[i];
   }
   ind->offset = idx->offset;
}

void index_read_idx(index_t *ind, struct idx *idx)
{
    /* A read idx CANNOT be unsized!  Don't forget that redimensioning
       occasionally cause a reallocation.  You certainly cannot do that 
       on a read only object.  Most (all?)  automatic redimensioning 
       function write in the matrix they redimension. */
   if (IND_UNSIZEDP(ind))
      error(NIL, "unsized index",NIL);
   index_to_idx(ind, idx);
}

void index_write_idx(index_t *ind, struct idx *idx)
{
   index_to_idx(ind, idx);
}

void index_rls_idx(struct index *ind, struct idx *idx)
{
   idx_to_index(idx, ind);
}


extern void get_write_permit(storage_t *);  /* in storage.c */

/* copy array contents from ind1 to ind2, return ind2 */

index_t *array_copy(index_t *ind1, index_t *ind2)
{
   if (index_nelems(ind1) != index_nelems(ind2))
      RAISEF("arrays have different number of elements",NIL);
  
   get_write_permit(IND_ST(ind2));

   if (IND_STTYPE(ind1) != IND_STTYPE(ind2))
      goto default_copy;
   
   switch (IND_STTYPE(ind1)) {
      
#define GenericCopy(Prefix,Type) 		  \
  case name2(ST_,Prefix): 			  \
    { 						  \
      Type *d1 = IND_BASE_TYPED(ind1, Type); 	  \
      Type *d2 = IND_BASE_TYPED(ind2, Type);	  \
      begin_idx_aloop2(ind1,ind2,p1,p2) { 	  \
         d2[p2] = d1[p1]; 			  \
      } end_idx_aloop2(ind1,ind2,p1,p2); 	  \
    } 						  \
    break;

      GenericCopy(F, flt);
      GenericCopy(D, real);
      GenericCopy(I32, int);
      GenericCopy(I16, short);
      GenericCopy(I8, char);
      GenericCopy(U8, unsigned char);
      GenericCopy(GPTR, gptr);
      GenericCopy(AT, atp_t);

#undef GenericCopy

   default:
   default_copy: {
      flt (*getf)(gptr,size_t) = storage_getf[IND_STTYPE(ind1)];
      void (*setf)(gptr,size_t,flt) = storage_setf[IND_STTYPE(ind2)];
      begin_idx_aloop2(ind1,ind2,p1,p2) {
         (*setf)(IND_BASE(ind2), p2, (*getf)(IND_BASE(ind1), p1) );
      } end_idx_aloop2(ind1,ind2,p1,p2);
   }
   break;

   } // switch
   return ind2;
}

DX(xarray_copy)
{
   ARG_NUMBER(2);
   array_copy(AINDEX(1), AINDEX(2));
   return APOINTER(2);
}

/* copy array contents from ind1 to ind2, and vice versa, return NIL */

void array_swap(index_t *ind1, index_t *ind2)
{
   if (index_nelems(ind1) != index_nelems(ind2))
      RAISEF("arrays have different number of elements", NIL);
   
   if (IND_STTYPE(ind1) != IND_STTYPE(ind2))
      RAISEF("arrays have different element-type", NIL);
    
   get_write_permit(IND_ST(ind1));
   get_write_permit(IND_ST(ind2));

   switch (IND_STTYPE(ind1)) {
    
#define GenericSwap(Prefix,Type) 			     \
  case name2(ST_,Prefix): 				     \
    { 							     \
      Type *d1 = IND_BASE_TYPED(ind1, Type); 		     \
      Type *d2 = IND_BASE_TYPED(ind2, Type);		     \
      begin_idx_aloop2(ind1,ind2,p1,p2) { 		     \
         Type t = d2[p2];                                    \
         d2[p2] = d1[p1];                                    \
         d1[p1] = t; 					     \
      } end_idx_aloop2(ind1,ind2,p1,p2); 		     \
    } 							     \
    break;

      GenericSwap(F, flt);
      GenericSwap(D, real);
      GenericSwap(I32, int);
      GenericSwap(I16, short);
      GenericSwap(I8, char);
      GenericSwap(U8, unsigned char);
      GenericSwap(GPTR, gptr);
      GenericSwap(AT, atp_t);

#undef GenericCopy

   default:
      RAISEF("unknown element-type", NIL);
   }
}

DX(xarray_swap)
{
   ARG_NUMBER(2);
   array_swap(AINDEX(1), AINDEX(2));
   return NIL;
}


#define begin_idx_dloop(idx,ptr,dims) {                                      \
  ptrdiff_t dims[MAXDIMS]; 						     \
  ptrdiff_t ptr = 0;							     \
  int _j_;                                                                   \
  bool emptyp = false;                                                       \
  for (_j_=0;_j_<(idx)->ndim; _j_++ ) 					     \
    { dims[_j_]=0; emptyp = emptyp || (idx)->dim[_j_] == 0; }                \
  _j_ = emptyp ? -1 : (idx)->ndim;					     \
  while (_j_>=0) {

#define end_idx_dloop(idx,ptr,dims)                                          \
    _j_--; 								     \
    do { 								     \
      if (_j_<0) break; 						     \
      if (++dims[_j_] < (idx)->dim[_j_]) {				     \
	ptr+=(idx)->mod[_j_];						     \
	_j_++;								     \
      } else { 								     \
	ptr -= (idx)->dim[_j_]*(idx)->mod[_j_]; 			     \
	dims[_j_--] = -1; 						     \
      } 								     \
    } while (_j_<(idx)->ndim);  					     \
  } 									     \
}

/* create array of subscripts of all nonzero elements in ind */
index_t *array_where_nonzero(index_t *ind)
{
   storage_t *srg = make_storage(ST_INT, 64, NIL);
   int n = 0;
   int r = IND_NDIMS(ind);

   switch (IND_STTYPE(ind)) {

#define GenericWhere(Prefix, Type)                                      \
   case name2(ST_,Prefix): {                                            \
      Type *elp = IND_BASE_TYPED(ind, Type);                            \
      begin_idx_dloop(ind, p, ds) {                                     \
         if (elp[p]) {                                                  \
            if (srg->size < n+r)                                        \
               storage_realloc(srg, srg->size*2, NIL);                  \
            for (int i = 0; i<r; i++)                                   \
               ((int *)srg->data)[n+i] = ds[i];                         \
            n += r;                                                     \
         }                                                              \
      } end_idx_dloop(ind, p, ds);                                      \
   } break;

   GenericWhere(F, float);
   GenericWhere(D, double);
   GenericWhere(I32, int);
   GenericWhere(I16, short);
   GenericWhere(I8, char);
   GenericWhere(U8, unsigned char);
   GenericWhere(GPTR, gptr);
   GenericWhere(AT, atp_t);
   
#undef GenericWhere

   default:
      RAISEF("unknown element-type", NIL);
   }
      
   return new_index(srg, SHAPE2D(n/r, r));
}

DX(xarray_where_nonzero)
{
   ARG_NUMBER(1);
   return array_where_nonzero(AINDEX(1))->backptr;
}


index_t *index_copy(index_t *src, index_t *dest)
{
   memcpy(dest, src, sizeof(index_t));
   dest->backptr = new_extern(&index_class, dest);
   dest->cptr = NULL;
   return dest; 
}

index_t *copy_index(index_t *ind)
{
   index_t *newind = mm_alloc(mt_index);
   return index_copy(ind, newind);
}

DX(xcopy_index)
{
   ARG_NUMBER(1);
   return copy_index(AINDEX(1))->backptr;
}

index_t *copy_array(index_t *ind)
{
   index_t *res = make_array(IND_STTYPE(ind), IND_SHAPE(ind), NIL);
   array_copy(ind, res);
   return res;
}

DX(xcopy_array)
{
   ARG_NUMBER(1);
   return copy_array(AINDEX(1))->backptr;
}



/* -------- MATRIX FILE FUNCTIONS ----------- */


#define BINARY_MATRIX    (0x1e3d4c51)
#define PACKED_MATRIX	 (0x1e3d4c52)
#define DOUBLE_MATRIX	 (0x1e3d4c53)
#define INTEGER_MATRIX	 (0x1e3d4c54)
#define BYTE_MATRIX	 (0x1e3d4c55)
#define SHORT_MATRIX	 (0x1e3d4c56)
#define SHORT8_MATRIX	 (0x1e3d4c57)
#define ASCII_MATRIX	 (0x2e4d4154)	/* '.MAT' */

#define SWAP(x) ((int)(((x&0xff)<<24)|((x&0xff00)<<8)|\
		      ((x&0xff0000)>>8)|((x&0xff000000)>>24))) 


/* --------- Utilities */

/* Check that datatypes are ``standard'' */
static void compatible_p(void)
{
   static int warned = 0;
   if (warned)
      return;
   warned = 1;
   if (sizeof(int) != 4) {
      fprintf(stderr, "+++ Warning:\n");
      fprintf(stderr, "+++ Integers are not four bytes wide on this cpu\n");
      fprintf(stderr, "+++ Matrix files may be non portable!\n");

   } else  {
      union { int i; flt b; } u;
      u.b = 2.125;
      if (sizeof(flt)!=4 || u.i!=0x40080000) {
         fprintf(stderr, "+++ Warning:\n");
         fprintf(stderr, "+++ Flt are not IEEE single precision numbers\n");
         fprintf(stderr, "+++ Matrix files may be non portable!\n");
      }
   }
}

static void mode_check(index_t *ind, size_t *size_p, size_t *elem_p)
{
   *elem_p = storage_sizeof[IND_STTYPE(ind)];
   *size_p = shape_nelems(IND_SHAPE(ind));
}

static void swap_buffer(char *b, int n, int m)
{
   char buffer[16];
   for (int i=0; i<n; i++) {
      for (int j=0; j<m; j++)
         buffer[j] = b[m-j-1];
      for (int j=0; j<m; j++)
         *b++ = buffer[j];
   }
}

/* -------- Saving a Matrix -------- */


/* save_matrix_len(p)
 * returns the number of bytes written by a save_matrix!
 */

int save_matrix_len(index_t *ind)
{
   size_t size, elsize;
   mode_check(ind, &size, &elsize);
   int ndim = ind->ndim;
   if (IND_UNSIZEDP(ind))
      return 2 * sizeof(int);
   else if (ndim==1 || ndim==2)
      return elsize * size + 5 * sizeof(int);
   else
      return elsize * size + (ndim+2) * sizeof(int);
}


/*
 * save_matrix(p,f) 
 * save_ascii_matrix(p,f)
 */

static void format_save_matrix(index_t *ind, FILE *f, bool with_header)
{
   /* validation */  
   compatible_p();
   storage_type_t st = IND_STTYPE(ind);

   /* magic */
   int magic;
   switch(st) {
   case ST_F:    magic=BINARY_MATRIX  ; break;
   case ST_D:    magic=DOUBLE_MATRIX  ; break;
   case ST_I32:  magic=INTEGER_MATRIX ; break;
   case ST_I16:  magic=SHORT_MATRIX   ; break;
   case ST_I8:   magic=SHORT8_MATRIX  ; break;
   case ST_U8:   magic=BYTE_MATRIX    ; break;
   default:      
      error(NIL, "cannot save an index this storage",IND_ATST(ind));
   }

   /* header */
   FMODE_BINARY(f);
   if (with_header) {
      /* magic, ndim, dimensions */
      int j, ndim = IND_UNSIZEDP(ind) ? -1 : IND_NDIMS(ind);
      write4(f, magic);
      write4(f, ndim);
      for (j = 0; j < ndim; j++)
	write4(f, ind->dim[j]);
      /* sn1 compatibility */
      if (ndim==1 || ndim==2)
         while (j++ < 3)
            write4(f, 1);
    }
   /* iterate */
   ifn (IND_UNSIZEDP(ind)) {
      struct idx id;
      index_read_idx(ind,&id);
      begin_idx_aloop1(&id, off) {
         char *p = (char*)(id.srg->data) + storage_sizeof[st]*(id.offset + off);
         fwrite(p, storage_sizeof[st], 1, f);
      } end_idx_aloop1(&id, off);
      index_rls_idx(ind,&id);
   }
   test_file_error(f);
}

void save_matrix(index_t *ind, FILE *f)
{
   format_save_matrix(ind, f, true);
}

void export_matrix(index_t *ind, FILE *f)
{
   format_save_matrix(ind, f, false);
}

DX(xsave_matrix)
{
   at *p, *ans;
   ARG_NUMBER(2);
   if (ISSTRING(2)) {
      p = OPEN_WRITE(ASTRING(2), "mat");
      ans = make_string(file_name);
   } else {
      p = APOINTER(2);
      ifn (WFILEP(p))
         RAISEFX("not a string or write descriptor", p);
      ans = p;
   }
   save_matrix(AINDEX(1), Mptr(p));
   return ans;
}

DX(xexport_raw_matrix)
{
   at *p, *ans;
   ARG_NUMBER(2);
   if (ISSTRING(2)) {
      p = OPEN_WRITE(ASTRING(2), NULL);
      ans = make_string(file_name);
   } else {
      p = APOINTER(2);
      ifn (WFILEP(p))
         RAISEFX("not a string or write descriptor", p);
      ans = p;
   }
   export_matrix(AINDEX(1), Mptr(p));
   return ans;
}


static void format_save_ascii_matrix(index_t *ind, FILE *f, int mode)
{
   /* validation */  
   compatible_p();
   if (IND_UNSIZEDP(ind))
      error(NIL,"ascii matrix format does not support unsized matrices", ind->backptr);
   storage_type_t st = IND_STTYPE(ind);
   flt (*getf)(gptr,size_t) = storage_getf[st];

   /* header */
   FMODE_TEXT(f);
   if (mode==1) {
      /* ascii matrix with header */
      fprintf(f, ".MAT %d", ind->ndim); 
      for (int j=0; j < ind->ndim; j++)
         fprintf(f, " %"PRIdPTR, ind->dim[j]);
      fprintf(f, "\n");
   }
   
   /* iterate */
   if (mode<2) {
      struct idx id;
      index_read_idx(ind,&id);
      gptr base = IDX_DATA_PTR(&id);
      begin_idx_aloop1(&id, off) {
         flt x = (*getf)(base, off);
         fprintf(f, "%10.5f\n", x);
      } end_idx_aloop1(&id, off);
      index_rls_idx(ind,&id);
      FMODE_BINARY(f);

   } else if (mode==2) {
      int d = IND_NDIMS(ind);
      struct idx id;
      index_read_idx(ind,&id);
      gptr base = IDX_DATA_PTR(&id);
      begin_idx_aloop1(&id, off) {
         flt x = (*getf)(base, off);
         fprintf(f, "%10.5f ", x);
         for (int i=0; i<d-1; i++)
            if ((off+1)%IND_MOD(ind, i)==0)
               fprintf(f, "\n");
      } end_idx_aloop1(&id, off);
      index_rls_idx(ind,&id);
      FMODE_BINARY(f);
   }
   test_file_error(f);
}

void save_ascii_matrix(index_t *ind, FILE *f)
{
   format_save_ascii_matrix(ind, f, (int)1);
}

void export_ascii_matrix(index_t *ind, FILE *f)
{
   format_save_ascii_matrix(ind, f, (int)0);
}

DX(xsave_ascii_matrix)
{
   at *p, *ans;
   
   ARG_NUMBER(2);
   if (ISSTRING(2)) {
      p = OPEN_WRITE(ASTRING(2), NULL);
      ans = make_string(file_name);
   } else if ((p = APOINTER(2)) && WFILEP(p)) {
      ans = p;
   } else {
      error(NIL, "not a string or write descriptor", p);
  }
   save_ascii_matrix(AINDEX(1), Mptr(p));
   return ans;
}

DX(xexport_text_matrix)
{
   at *p, *ans;
   
   ARG_NUMBER(2);
   if (ISSTRING(2)) {
      p = OPEN_WRITE(ASTRING(2), "mat");
      ans = make_string(file_name);
   } else {
      p = APOINTER(2);
      ifn (p && WFILEP(p))
         error(NIL, "not a string or write descriptor", p);
      ans = p;
   }
   export_ascii_matrix(AINDEX(1), Mptr(p));
   return ans;
}

DX(xarray_export_tabular)
{
   at *p, *ans;
   
   ARG_NUMBER(2);
   if (ISSTRING(2)) {
      p = OPEN_WRITE(ASTRING(2), "mat");
      ans = make_string(file_name);
   } else {
      p = APOINTER(2);
      ifn (p && WFILEP(p))
         error(NIL, "not a string or write descriptor", p);
      ans = p;
   }
   format_save_ascii_matrix(AINDEX(1), Mptr(p), 2);
   return ans;
}


/* -------- Loading a Matrix ------ */

/*
 * import_raw_matrix(p,f,n) 
 * import_text_matrix(p,f)
 */

void import_raw_matrix(index_t *ind, FILE *f, size_t offset)
{
   struct idx id;
   size_t size, elsize, rsize;
   char *pntr;
   int contiguous;

   /* validate */
   mode_check(ind, &size, &elsize);
   contiguous = index_contiguousp(ind);
   if (IND_UNSIZEDP(ind))
      return;
   if (IND_STTYPE(ind) == ST_AT || IND_STTYPE(ind) == ST_GPTR )
      error(NIL,"cannot read data for this storage type", IND_ATST(ind));
   
   /* skip */
   if (offset != 0) {
#if HAVE_FSEEKO
      if (fseeko(f, (off_t)offset, SEEK_CUR) < 0)
#else
         if (fseek(f, offset, SEEK_CUR) < 0)
#endif
            test_file_error(NIL);
   }
   
   /* read */
   index_write_idx(ind, &id);
   pntr = IDX_DATA_PTR(&id);
   if (contiguous) {
      /* fast read of contiguous matrices */
      rsize = fread(pntr, elsize, size, f);
      index_rls_idx(ind,&id);
      if (rsize < 0)
         test_file_error(NIL);
      else if (rsize < size)
         error(NIL, "file is too short", NIL); 

   } else {
      /* must loop on each element */
      rsize = 1;
      begin_idx_aloop1(&id, off) {
         if (rsize == 1)
            rsize = fread(pntr + (off * elsize), elsize, 1, f);
      } end_idx_aloop1(&id, off);
      index_rls_idx(ind,&id);
      if (rsize < 0)
         test_file_error(NIL);
      else if (rsize < 1)
         error(NIL, "file too short", NIL); 
    }
}

DX(ximport_raw_matrix)
{
   size_t offset = 0;
   at *p;
   if (arg_number==3)
      offset = AINTEGER(3);
   else 
      ARG_NUMBER(2);

   if (ISSTRING(2)) {
      p = OPEN_READ(ASTRING(2), NULL);
   } else {
      p = APOINTER(2);
      ifn (p && RFILEP(p))
         error(NIL, "not a string or read file descriptor", p);
   }
   import_raw_matrix(AINDEX(1),Mptr(p),offset);
   lush_delete(p); /* close file */
   return APOINTER(1);
}


void import_text_matrix(index_t *ind, FILE *f)
{
   struct idx id;
   size_t size, elsize;
   char *pntr;
   real x;
   /* validate */
   mode_check(ind, &size, &elsize);
   ifn (index_contiguousp(ind))
      error(NIL, "index not contiguous", NIL);
   storage_type_t type = IND_STTYPE(ind);
   void (*setr)(gptr,size_t,real) = storage_setr[type];

   if (IND_UNSIZEDP(ind))
      return;
   if (type == ST_AT || type == ST_GPTR )
      error(NIL, "cannot read data for this storage type",IND_ATST(ind));
   
   /* load */
   index_write_idx(ind, &id);
   pntr = IDX_DATA_PTR(&id);
   begin_idx_aloop1(&id, off) {
      if (fscanf(f, " %lf ", &x) != 1) {
         index_rls_idx(ind,&id);
         error(NIL,"Cannot read a number",NIL);
      }
      (*setr)(pntr, off, x);
   } end_idx_aloop1(&id, off);
   index_rls_idx(ind,&id);
}


DX(ximport_text_matrix)
{
   ARG_NUMBER(2);
   at *p;
   if (ISSTRING(2)) {
      p = OPEN_READ(ASTRING(2), NULL);
   } else {
      p = APOINTER(2);
      ifn (p && RFILEP(p))
         error(NIL, "not a string or read descriptor", p);
   }
   import_text_matrix(AINDEX(1),Mptr(p));
   lush_delete(p); /* close file */
   return APOINTER(1);
}

static void load_matrix_header(FILE *f, int *magic_p, int *swap_p, shape_t *shp)
{
   int swapflag = 0;
   int magic, ndim, i;
   switch (magic = read4(f)) {
   case SWAP( BINARY_MATRIX ):
   case SWAP( PACKED_MATRIX ):
   case SWAP( DOUBLE_MATRIX ):
   case SWAP( SHORT_MATRIX ):
   case SWAP( SHORT8_MATRIX ):
   case SWAP( INTEGER_MATRIX ):
   case SWAP( BYTE_MATRIX ):
      magic = SWAP(magic);
      swapflag = 1;
      /* no break */
   case BINARY_MATRIX:
   case PACKED_MATRIX:
   case DOUBLE_MATRIX:
   case SHORT_MATRIX:
   case SHORT8_MATRIX:
   case INTEGER_MATRIX:
   case BYTE_MATRIX:
      shp->ndims = read4(f);
      if (swapflag) 
         shp->ndims = SWAP(shp->ndims);
      if (shp->ndims == -1)
         break;
      if (shp->ndims < 0 || shp->ndims > MAXDIMS)
         goto trouble;
      for (i = 0; i < shp->ndims; i++) {
         shp->dim[i] = read4(f);
         if (swapflag)
            shp->dim[i] = SWAP(shp->dim[i]);
/*         if (shp->dim[i] < 1) */
/*           goto trouble; */
      }
      /* sn1 compatibility */
      if (shp->ndims == 1 || shp->ndims == 2)
         while (i++ < 3) {
            shp->dim[i] = read4(f);
            if (swapflag)
               shp->dim[i] = SWAP(shp->dim[i]);
            if (shp->dim[i] != 1)
               goto trouble;
         }
      break;
   case SWAP(ASCII_MATRIX):
      magic = SWAP(magic);
      /* no break */
   case ASCII_MATRIX:
      if (fscanf(f, " %d", &ndim) != 1) 
         goto trouble;
      if ((ndim<0) || (ndim>MAXDIMS)) 
         goto trouble;
      for (i = 0; i < ndim; i++)
         if (fscanf(f, " %"PRIdPTR" ", &(shp->dim[i])) != 1) 
            goto trouble;
      shp->ndims = ndim;
      break;
   default:
      /* handle pascal vincent idx-io types */
#ifndef NO_SUPPORT_FOR_PASCAL_IDX_FILES
      swapflag = 1;
      swapflag = (int)(*(char*)&swapflag);
      if (swapflag) 
         magic = SWAP(magic);
      if (! (magic & ~0xF03) ) {
         shp->ndims = magic & 0x3;
         switch ((magic & 0xF00) >> 8) {
         case 0x8: magic = BYTE_MATRIX; break;
         case 0x9: magic = SHORT8_MATRIX; break;
         case 0xB: magic = SHORT_MATRIX; break;
         case 0xC: magic = INTEGER_MATRIX; break;
         case 0xD: magic = BINARY_MATRIX; break;
         case 0xE: magic = DOUBLE_MATRIX; break;
         default: magic = 0;
         }
         if (magic && shp->ndims>=1 && shp->ndims<=3) {
            for (i=0; i<shp->ndims; i++)
               shp->dim[i] = read4(f);
            if (swapflag)
               for (i=0; i<shp->ndims; i++)
                  shp->dim[i] = SWAP(shp->dim[i]);
            break;
         }
      }
#endif
      error(NIL, "not a recognized matrix file", NIL);
   trouble:
      test_file_error(f);
      error(NIL, "corrupted matrix file",NIL);
   }
  
   *magic_p = magic;
   *swap_p = swapflag;
   return;
}



at *load_matrix(FILE *f)
{
   int magic;
   shape_t shape;
   int swapflag = 0;
   index_t *ind;

   /* Header */
   load_matrix_header(f, &magic, &swapflag, &shape);
   /* Create */
   switch (magic) {
   case BINARY_MATRIX:
      ind = make_array(ST_F, &shape, NIL);
      break;
   case ASCII_MATRIX:
   case DOUBLE_MATRIX:
      ind = make_array(ST_D, &shape, NIL);;
      break;
   case INTEGER_MATRIX:
      ind = make_array(ST_I32, &shape, NIL);;
      break;
   case SHORT_MATRIX:
      ind = make_array(ST_I16, &shape, NIL);
      break;
   case SHORT8_MATRIX:
      ind = make_array(ST_I8, &shape, NIL);
      break;
   case BYTE_MATRIX:
      ind = make_array(ST_U8, &shape, NIL);
      break;
   default:
      RAISEF("unknown format", NIL);
   }

   /* Import */
   if (shape.ndims >= 0) {
      if (magic==ASCII_MATRIX)
         import_text_matrix(ind,f);
      else
         import_raw_matrix(ind,f,0);
      /* Swap when needed */
      if (swapflag) {
         struct idx id;
         size_t size, elsize;
         char *pntr;
         mode_check(ind, &size, &elsize);
         if (elsize > 1) {
            index_write_idx(ind, &id);
            pntr = IDX_DATA_PTR(&id);
            swap_buffer(pntr, size, elsize);
            index_rls_idx(ind,&id);
         }
      }
   }
   return ind->backptr;
}

DX(xload_matrix)
{
   at *p, *ans;
   if (arg_number == 2) {
      ASYMBOL(1);
      if (ISSTRING(2)) {
         p = OPEN_READ(ASTRING(2), "mat");
      } else {
      p = APOINTER(2);
      ifn (p && RFILEP(p))
         error(NIL, "not a string or read descriptor", p);
      }
      ans = load_matrix(Mptr(p));
      var_set(APOINTER(1), ans);

   } else {
      ARG_NUMBER(1);
      if (ISSTRING(1)) {
         p = OPEN_READ(ASTRING(1), "mat");
      } else {
         p = APOINTER(1);
         ifn (p && RFILEP(p))
            error(NIL, "not a string or read descriptor", p);
      }
      ans = load_matrix(Mptr(p));
   }
   lush_delete(p); /* close file */
   return ans;
}



/* ------------- Mapping a matrix */


#ifdef HAVE_MMAP

at *map_matrix(FILE *f)
{
   int magic, swapflag;
#ifdef HAVE_FTELLO
   off_t pos;
#else
   int pos;
#endif

   /* Header */
   shape_t shape;
   load_matrix_header(f, &magic, &swapflag, &shape);
   if (swapflag && magic!=BYTE_MATRIX && magic!=SHORT8_MATRIX)
      error(NIL,"cannot map this byteswapped matrix",NIL);
   /* Create storage */
   storage_t *st;
   switch(magic) {
   case BINARY_MATRIX:  st = new_storage(ST_F);   break;
   case DOUBLE_MATRIX:  st = new_storage(ST_D);   break;
   case INTEGER_MATRIX: st = new_storage(ST_I32); break;
   case SHORT_MATRIX:   st = new_storage(ST_I16); break;
   case SHORT8_MATRIX:  st = new_storage(ST_I8);  break;
   case BYTE_MATRIX:    st = new_storage(ST_U8);  break;
   default:
      error(NIL, "cannot map ascii matrix files", NIL);
   }
   /* Map storage */
#ifdef HAVE_FTELLO
   if ((pos = ftello(f)) < 0)
#else
   if ((pos = ftell(f)) < 0)
#endif
      error(NIL, "cannot seek through this file", NIL);
   storage_mmap(st, f, pos);
   return NEW_INDEX(st, &shape);
}

DX(xmap_matrix)
{
   at *p, *ans;
   
   if (arg_number == 2) {
      ASYMBOL(1);
      if (ISSTRING(2)) {
         p = OPEN_READ(ASTRING(2), "mat");
      } else {
         p = APOINTER(2);
         ifn (p && RFILEP(p))
            error(NIL, "not a string or read descriptor", p);
      }
      ans = map_matrix(Mptr(p));
      var_set(APOINTER(1), ans);
   } else {
      ARG_NUMBER(1);
      if (ISSTRING(1)) {
         p = OPEN_READ(ASTRING(1), "mat");
      } else {
         p = APOINTER(1);
         ifn (p && RFILEP(p))
            error(NIL, "not a string or read descriptor", p);
      }
      ans = map_matrix(Mptr(p));
   }
   return ans;
}

#endif

/* ------ INDEX STRUCTURE OPERATIONS ------ */



/* reshape contiguous index */

index_t *index_reshapeD(index_t *ind, shape_t *shp)
{
   RAISEF(chk_contiguous(ind), NIL);
   if (shape_nelems(shp)!=index_nelems(ind)) 
      RAISEF("number of elements must be the same with new shape", 
             NEW_NUMBER(index_nelems(ind)));
  
   IND_NDIMS(ind) = shp->ndims;
   ptrdiff_t size = 1;
   for (int i=shp->ndims-1; i>=0; i--) {
      IND_DIM(ind,i) = shp->dim[i];
      IND_MOD(ind,i) = size;
      size *= shp->dim[i];
   }
   return ind;
}

index_t *index_reshape(index_t *ind, shape_t *shp)
{
   return index_reshapeD(copy_index(ind), shp);
}

DX(xidx_reshapeD)
{
   ARG_NUMBER(2);
   index_reshapeD(AINDEX(1), parse_shape(APOINTER(2), NIL));
   return APOINTER(1);
}

DX(xidx_reshape)
{
   ARG_NUMBER(2);
   index_t *ind = index_reshape(AINDEX(1), parse_shape(APOINTER(2), NIL));
   return ind->backptr;
}

index_t *index_flatten(index_t *ind, int n)
{
   RAISEF(chk_contiguous(ind), NIL);

   if (n > 0) {
      ifn (n < IND_NDIMS(ind))
         RAISEF("too many dimensions to collapse", NEW_NUMBER(n));
      
      shape_t shape, *shp = &shape;
      shp->ndims = IND_NDIMS(ind)-n;
      IND_DIM(shp, 0) = IND_DIM(ind, 0);
      for (int i=1; i<n+1; i++)
         IND_DIM(shp, 0) *= IND_DIM(ind, i);
      for (int i=n+1; i<IND_NDIMS(ind); i++)
         IND_DIM(shp, i-n) = IND_DIM(ind, i);

      ind = index_reshape(ind, shp);

   } else if (n < 0) {
      n = -n;
      ifn (n < IND_NDIMS(ind))
         RAISEF("too many dimensions to collapse", NEW_NUMBER(n));
      
      shape_t shape, *shp = &shape;
      int r = IND_NDIMS(ind)-n;
      shp->ndims = r; 
      IND_DIM(shp, r-1) = IND_DIM(ind, IND_NDIMS(ind)-1);
      for (int i=IND_NDIMS(ind)-2; i>=r-1; i--)
         IND_DIM(shp, r-1) *= IND_DIM(ind, i);
      for (int i=r-2; i>=0; i--)
         IND_DIM(shp, i-r+2) = IND_DIM(ind, i);

      ind = index_reshape(ind, shp);

   } else
      ind = copy_index(ind);

   return ind;
}

DX(xidx_flatten)
{
   index_t *ind = NULL;
   if (arg_number == 2)
      ind = index_flatten(AINDEX(1), AINTEGER(2));
   else if (arg_number == 1)
      ind = index_flatten(AINDEX(1), IND_NDIMS(AINDEX(1))-1);
   else
      ARG_NUMBER(-1);

   return ind->backptr;
}

index_t *index_nickD(index_t *ind, int d)
{
   if (IND_NDIMS(ind)==MAXDIMS)
      RAISEF("cannot nick, array already has maximum number of dimensions", NIL);
   
   d = validate_dimension(ind, d);
   for (int i=IND_NDIMS(ind); i>d; i--) {
      IND_DIM(ind, i) = IND_DIM(ind, i-1);
      IND_MOD(ind, i) = IND_MOD(ind, i-1);
   }
   IND_DIM(ind, d) = 1;
   IND_NDIMS(ind) += 1;
   return ind;
}

index_t *index_nick(index_t *ind, int d)
{
   return index_nickD(copy_index(ind), d);
}

DX(xidx_nickD)
{
   ARG_NUMBER(2);
   index_nickD(AINDEX(1), AINTEGER(2));
   return APOINTER(1);
}

DX(xidx_nick)
{
   ARG_NUMBER(2);
   return index_nick(AINDEX(1), AINTEGER(2))->backptr;
}

index_t *index_ravelD(index_t *ind)
{
   ifn (index_contiguousp(ind))
      RAISEF("cannot ravel in-place, index not contiguous", NIL);
   index_reshapeD(ind, SHAPE1D(index_nelems(ind)));
   return ind;
}

index_t *index_ravel(index_t *ind) 
{
   return index_ravelD(as_contiguous_array(ind));
}


/* extend index in first dimension if possible */

index_t *index_extendD(index_t *ind, int d, ptrdiff_t ne)
{
   if (IND_NDIMS(ind)==0)
      RAISEF("cannot extend a scalar", NIL);
   d = validate_dimension(ind, d);

   IND_DIM(ind, d) += ne;
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      IND_DIM(ind, d) -= ne;
      RAISEF(errmsg, NIL);
   }
   return ind;
}

index_t *index_extend(index_t *ind, int d, ptrdiff_t ne)
{
   return index_extendD(copy_index(ind), d, ne);
}

index_t *index_extendSD(index_t *ind, subscript_t *des)
{
   if (IND_NDIMS(ind)!=des->ndims)
      RAISEF("wrong number of dimensions", NIL);
  
   for (int d=0; d<des->ndims; d++)
      IND_DIM(ind, d) += des->dim[d];
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      for (int d=0; d<des->ndims; d++)
         IND_DIM(ind, d) -= des->dim[d];
      RAISEF(errmsg, NIL);
   }
   return ind;
}

index_t *index_extendS(index_t *ind, subscript_t *des)
{
   return index_extendSD(copy_index(ind), des);
}

DX(xidx_extendD)
{
   ARG_NUMBER(3);
   index_extendD(AINDEX(1), AINTEGER(2), AINTEGER(3));
   return APOINTER(1);
}

DX(xidx_extend)
{
   ARG_NUMBER(3);
   return index_extend(AINDEX(1), AINTEGER(2), AINTEGER(3))->backptr;
}

index_t *array_extend(index_t *ind, int d, ptrdiff_t n, at *init)
{
   d = validate_dimension(ind, d);
   shape_t shape, *shp = &shape;
   shape_copy(IND_SHAPE(ind), shp);
   shp->dim[d] += n;
   index_t *arr = make_array(IND_STTYPE(ind), shp, init);

   if (n>0) {
      index_t *arr2 = index_extend(arr, d, -n);
      array_copy(ind, arr2);
   } else {
      index_t *ind2 = index_extend(ind, d, n);
      array_copy(ind2, arr);
   }
   return arr;
}

DX(xarray_extend)
{
   if (arg_number<3 || arg_number>4)
      ARG_NUMBER(-1);
   at *init = arg_number==4 ? APOINTER(4) : NEW_NUMBER(0);
   return array_extend(AINDEX(1), AINTEGER(2), AINTEGER(3), init)->backptr;
}

/* change offset of nth dimension */

index_t *index_shiftD(index_t *ind, int d, ptrdiff_t ne)
{
   d = validate_dimension(ind, d);
   ind->offset += IND_MOD(ind, d)*ne;
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      ind->offset -= IND_MOD(ind, d)*ne;
      RAISEF(errmsg, NIL);
   }
   return ind;
}

index_t *index_shift(index_t *ind, int d, ptrdiff_t ne)
{
   return index_shiftD(copy_index(ind), d, ne);
}

DX(xidx_shiftD)
{
   ARG_NUMBER(3);
   index_shiftD(AINDEX(1), AINTEGER(2), AINTEGER(3));
   return APOINTER(1);
}

DX(xidx_shift)
{
   ARG_NUMBER(3);
   return index_shift(AINDEX(1), AINTEGER(2), AINTEGER(3))->backptr;
}

index_t *index_shiftSD(index_t *ind, subscript_t *des)
{
   if (IND_NDIMS(ind)!=des->ndims)
      RAISEF("wrong number of dimensions", NIL);
   for (int d=0; d<des->ndims; d++)
      ind->offset += IND_MOD(ind, d)*des->dim[d];
  
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      for (int d=0; d<des->ndims; d++)
         ind->offset -= IND_MOD(ind, d)*des->dim[d];
      RAISEF(errmsg, NIL);
   }
   return ind;
}


index_t *index_shiftS(index_t *ind, subscript_t *des)
{
   return index_shiftSD(copy_index(ind), des);
}

/* DX(xidx_shiftSD) */
/* { */
/*   ARG_NUMBER(2); */
/*   subscript_t dextents, *des = parse_subscript(APOINTER(2), &dextents); */
/*   index_shiftSD(AINDEX(1), des); */
/*   LOCK(APOINTER(1)); */
/*   return APOINTER(1); */
/* } */

/* DX(xidx_shiftS) */
/* { */
/*   ARG_NUMBER(2); */
/*   subscript_t dextents, *des = parse_subscript(APOINTER(2), &dextents); */
/*   return index2at(index_shiftS(AINDEX(1), des)); */
/* } */

  

/* xidx_unfold prepares a matrix for a convolution */

DX(xidx_unfoldD)
{
   ARG_NUMBER(4);
   index_t *ind = AINDEX(1);
   int d = AINTEGER(2);
   int sz = AINTEGER(3);
   int st = AINTEGER(4);
   
   if (d<0 || d>=IND_NDIMS(ind) || sz<1 || st<1)
      RAISEF("illegal parameters",NIL);

   int s = 1+ (IND_DIM(ind, d)-sz)/st;
   if (s<=0 || ind->dim[d]!=st*(s-1)+sz)
      error(NIL,"Index dimension does not match size and step", 
            NEW_NUMBER(ind->dim[d]));
   
   int n = ind->ndim;
   if (n+1 >= MAXDIMS)
      error(NIL,"Dimension value exceeds maximum number of dimensions", 
            NEW_NUMBER(MAXDIMS));
  
   ind->ndim = n+1;
   ind->dim[n] = sz;
   ind->mod[n] = ind->mod[d];
   ind->dim[d] = s;
   ind->mod[d] *= st;
   
   return NIL;
}


/* index_expand a dimension*/

index_t *index_expandD(index_t *ind, int d, size_t ne)
{
   d = validate_dimension(ind ,d);
   if (IND_DIM(ind, d)==ne)
      return ind;
   ifn (IND_DIM(ind, d)==1)
      RAISEF("can only expand singleton dimensions", NIL);

   /* create output */
   IND_MOD(ind, d) = 0;
   IND_DIM(ind, d) = ne;
   // todo: flag resulting index as non-writeable?
   return ind;
}

index_t *index_expand(index_t *ind, int d, size_t ne)
{
   return index_expandD(copy_index(ind), d, ne);
}

DX(xidx_expandD)
{
   ARG_NUMBER(3);
   index_expandD(AINDEX(1), AINTEGER(2), AINTEGER(3));
   return APOINTER(1);
}

DX(xidx_expand)
{
   ARG_NUMBER(3);
   return index_expand(AINDEX(1), AINTEGER(2), AINTEGER(3))->backptr;
}

/* index_lift  dimensions */

index_t *index_liftD(index_t *ind, shape_t *shp)
{
   if (shp->ndims==0)
      return ind;
   
   int d = shp->ndims;
   if (d+IND_NDIMS(ind)>MAXDIMS)
      RAISEF("too many dimensions", NIL);
  
   for (int i=IND_NDIMS(ind)+d-1; i>=d; i--) {
      IND_MOD(ind, i) = IND_MOD(ind, i-d);
      IND_DIM(ind, i) = IND_DIM(ind, i-d);
   }
   for (int i=0; i<d; i++) {
      IND_DIM(ind, i) = IND_DIM(shp, i);
      IND_MOD(ind, i) = IND_DIM(ind,i) == 1 ? 1 : 0;
   }
   IND_NDIMS(ind) += d;
   
   return ind;
}

index_t *index_lift(index_t *ind, shape_t *shp)
{
   return index_liftD(copy_index(ind), shp);
}

DX(xidx_liftD)
{
   if (arg_number<1)
      ARG_NUMBER(-1);

   shape_t shape, *shp = &shape;
   for (int i=2; i<=arg_number; i++)
      shp->dim[i-2] = AINTEGER(i);
   shp->ndims = arg_number-1;
   index_liftD(AINDEX(1), shp);
   return APOINTER(1);
}

DX(xidx_lift)
{
   if (arg_number<1)
      ARG_NUMBER(-1);

   index_t *ind = AINDEX(1);
   shape_t shape, *shp = &shape;
   for (int i=2; i<=arg_number; i++)
      shp->dim[i-2] = AINTEGER(i);
   shp->ndims = arg_number-1;
   ind = index_lift(AINDEX(1), shp);
   return ind->backptr;
}


/* index_sink dimensions */

index_t *index_sinkD(index_t *ind, shape_t *shp)
{
   int d = shp->ndims+IND_NDIMS(ind);
   if (d>MAXDIMS)
      RAISEF("too many dimensions", NIL);
  
   for (int i=IND_NDIMS(ind); i<d; i++) {
      IND_DIM(ind, i) = shp->dim[i-IND_NDIMS(ind)];
      IND_MOD(ind, i) = IND_DIM(ind, i) == 1 ? 1 : 0;
   }
   IND_NDIMS(ind) = d;
   
   return ind;
}

index_t *index_sink(index_t *ind, shape_t *shp)
{
   return index_sinkD(copy_index(ind), shp);
}

DX(xidx_sinkD)
{
   if (arg_number<1)
      ARG_NUMBER(-1);

   shape_t shape, *shp = &shape;
   for (int i=2; i<=arg_number; i++)
      shp->dim[i-2] = AINTEGER(i);
   shp->ndims = arg_number-1;
   index_sinkD(AINDEX(1), shp);
   return APOINTER(1);
}

DX(xidx_sink)
{
   if (arg_number<1)
      ARG_NUMBER(-1);

   index_t *ind = AINDEX(1);
   shape_t shape, *shp = &shape;
   for (int i=2; i<=arg_number; i++)
      shp->dim[i-2] = AINTEGER(i);
   shp->ndims = arg_number-1;
   ind = index_sink(AINDEX(1), shp);
   return ind->backptr;
}


/* index_trim restrict a dimension */

index_t *index_trimD(index_t *ind, int d, ptrdiff_t o, size_t sz)
{
   d = validate_dimension(ind, d);
   if (d<0 || d>=IND_NDIMS(ind) || sz<0 || o<0 || o>IND_DIM(ind, d))
      RAISEF("illegal parameters", NIL);
   if (o+sz > ind->dim[d])
      RAISEF("specified interval is too large", NIL);

   ind->dim[d] = sz;
   ind->offset += o * ind->mod[d];
   return ind;
}

index_t *index_trim(index_t *ind, int d, ptrdiff_t o, size_t sz)
{
   return index_trimD(copy_index(ind), d, o, sz);
}

DX(xidx_trimD)
{
   ifn (arg_number==3 || arg_number==4)
      ARG_NUMBER(-1);
  
   /* calc parameters to index_trimD */
   index_t *ind = AINDEX(1);
   int d = validate_dimension(ind, AINTEGER(2));
   ptrdiff_t o = AINTEGER(3);
   size_t sz = 0;
   if (arg_number==4) {
      double dsz = AREAL(4);
      if (dsz<0)
         RAISEF("new extent must be non-negative",APOINTER(4));
      sz = (size_t)dsz;
   } else
      sz = IND_DIM(ind, d) - abs(o);

   if (o<0)
      o = IND_DIM(ind, d) + o - sz;

   index_trimD(ind, d, o, sz);
   return APOINTER(1);
}

DX(xidx_trim)
{
   ifn (arg_number==3 || arg_number==4)
      ARG_NUMBER(-1);
  
   /* calc parameters to index_trim */
   index_t *ind = AINDEX(1);
   int d = validate_dimension(ind, AINTEGER(2));
   ptrdiff_t o = AINTEGER(3);
   size_t sz = 0;
   if (arg_number==4) {
      double dsz = AREAL(4);
      if (dsz<0)
         RAISEF("new extent must be non-negative", APOINTER(4));
      sz = (size_t)dsz;
   } else
      sz = IND_DIM(ind, d) - abs(o);
   if (o<0)
      o = IND_DIM(ind, d) + o - sz;
   
   return index_trim(ind, d, o, sz)->backptr;
}


DX(xidx_narrowD)
{
   ARG_NUMBER(4);
   index_t *ind = AINDEX(1);
   int d = AINTEGER(2);
   size_t sz = AINTEGER(3);
   size_t st = AINTEGER(4);
   if (d<0 || d>=ind->ndim || sz<1 || st<0)
      error(NIL, "illegal parameters", NIL);
   if (st+sz > ind->dim[d])
      error(NIL, "specified interval is too large", NIL);
   
   ind->dim[d] = sz;
   ind->offset += st * ind->mod[d];
   return NIL;
}

/* index_select a slice in a dimension */ 

index_t *index_select(index_t *ind, int d, ptrdiff_t s)
{
   d = validate_dimension(ind, d);
   s = validate_subscript(ind, d, s);

   /* create output */
   ind = copy_index(ind);
   ind->offset += s*IND_MOD(ind, d);
   IND_NDIMS(ind)--;
   while (d<IND_NDIMS(ind)) {
      IND_DIM(ind, d) = IND_DIM(ind, d+1);
      IND_MOD(ind, d) = IND_MOD(ind, d+1);
      d++;
   }
   return ind;
}

DX(xidx_select)
{
   ARG_NUMBER(3);
   return index_select(AINDEX(1), AINTEGER(2), AINTEGER(3))->backptr;
}


index_t *index_selectS(index_t *ind, subscript_t *ss)
{
   if (ss->ndims > IND_NDIMS(ind))
      RAISEF("too many subscripts", NIL);

   /* create output */
   ind = copy_index(ind);

   for (int d=0; d<ss->ndims; d++) {
      ss->dim[d] = validate_subscript(ind, d, ss->dim[d]);
      ind->offset += IND_MOD(ind, d)*ss->dim[d];
   }
   int i = 0;
   int d = ss->ndims;
   while (d<IND_NDIMS(ind)) {
      IND_DIM(ind, i) = IND_DIM(ind, d);
      IND_MOD(ind, i) = IND_MOD(ind, d);
      i++;
      d++;
   }
   IND_NDIMS(ind) -= ss->ndims;
   return ind;
}

DY(yidx_selectS)
{
   at *p = ARG_LIST;
   ifn (CONSP(p))
      RAISEF("at least one argument expected", NIL);
   
   p = eval_arglist(p);
   ifn (INDEXP(Car(p)))
      RAISEF("not an array", Car(ARG_LIST));

   index_t *ind = Mptr(Car(p));
   subscript_t *ss = parse_subscript(Cdr(p));
   return index_selectS(ind, ss)->backptr;
}


DX(xidx_select_all)
{
   if (arg_number<1 || arg_number>2)
      ARG_NUMBER(-1);
   
   index_t *ind = AINDEX(1);
   int d = (arg_number==1) ? 0 : validate_dimension(ind, AINTEGER(2));
   
   /* create a list of all slices in dimension d */
   index_t *slice = index_select(ind, d, -1);
   at *res = new_cons(slice->backptr, NIL);
   for (int i=0; i<IND_DIM(ind, d)-1; i++) {
      slice = copy_index(slice);
      slice->offset -= IND_MOD(ind, d);
      res = new_cons(slice->backptr, res);
   }
   return res;
}

index_t *array_select(index_t *ind, int d, ptrdiff_t x)
{
   ind = index_select(ind, d, x);
   index_t *arr = copy_array(ind);
   return arr;
}


index_t *array_take2(index_t *ind, index_t *ss)
{
   static char *errmsg_out_of_range = "subscript out of range";       
   char *errmsg = NULL;
   
   int r = IND_NDIMS(ind);
   int d = IND_NDIMS(ss)-1;

   /* check & normalize arguments */
   ifn (IND_STTYPE(ss) == ST_INT)
      RAISEF("subscript array must be integer", NIL);
   ifn (d >= 0)
      RAISEF("subscript array must not be scalar", NIL);
   ifn (IND_DIM(ss, d) == r)
      RAISEF("final extent of subscript array must match rank of first argument", NIL);
   
   /* create result array */
   shape_t shape, *shp = shape_copy((shape_t *)ss, &shape);
   shp->ndims -= 1;
   index_t *res = make_array(IND_STTYPE(ind), shp, NIL);

   if (index_emptyp(res))
      goto clean_up_and_return2;

   /* copy selected array contents */
   ifn (IND_MOD(ss, d) == 1)  // when not contiguous in last dimension, make it so
      ss = copy_array(ss);
   
   switch (IND_STTYPE(res)) {
      
#define GenericTake(Prefix,Type) 				     \
  case name2(ST_,Prefix): {					     \
     								     \
    index_t *ss0 = index_select(ss, d, 0);                           \
    Type *pi = IND_BASE_TYPED(ind, Type);			     \
    Type *pr = IND_BASE_TYPED(res, Type);                            \
    int *ps = IND_BASE_TYPED(ss, int);                               \
    begin_idx_aloop2(res, ss0, resp, ss0p) {                         \
       /* validate dimension */                                      \
       ptrdiff_t offs = 0;                                           \
       for (int i = 0; i < r; i++) {                                 \
          int ii = ps[ss0p+i];                                       \
          if (ii < 0) ii += IND_DIM(ind, i);                         \
          if (ii<0 || ii>=IND_DIM(ind, i)) {                         \
             errmsg = errmsg_out_of_range;                           \
             goto clean_up_and_return2;                              \
          }                                                          \
          offs += IND_MOD(ind, i)*ii;                                \
       }                                                             \
       pr[resp] = pi[offs];                                          \
    } end_idx_aloop2(res, ss0, resp, ss0p);                          \
  } 								     \
  break;

      GenericTake(FLOAT, float);
      GenericTake(DOUBLE, real);
      GenericTake(INT, int);
      GenericTake(SHORT, short);
      GenericTake(BYTE, char);
      GenericTake(UBYTE, unsigned char);
      GenericTake(GPTR, gptr);

#undef GenericTake
      
   default:
      errmsg = "element-type not supported";
      break;
   }

clean_up_and_return2:
   /* report any errors or return result */
   RAISEF(errmsg, NIL);
   return res;
}


index_t *array_take3(index_t *ind, int d, index_t *ss)
{
   static char *errmsg_out_of_range = "subscript out of range";       
   char *errmsg = NULL;
   
   /* check & normalize arguments */
   if (IND_NDIMS(ss)+IND_NDIMS(ind)>MAXDIMS)
      RAISEF("Rank of output exceeds possible number of dimensions",
             NEW_NUMBER(IND_NDIMS(ss)+IND_NDIMS(ind)-1));
   d = validate_dimension(ind, d);
   ifn (IND_STTYPE(ss)==ST_INT)
      RAISEF("subscript array must be integer", NIL);
   index_t *iss = index_ravel(ss); 
  
   /* create result array */
   size_t td = IND_DIM(ind, d); 
   IND_DIM(ind, d) = index_nelems(iss);
   index_t *res = clone_array(ind);
   shape_t shape, *shp = shape_copy((shape_t *)res, &shape);
   index_t *islice = NIL;
   index_t *rslice = NIL;
   IND_DIM(ind, d) = td;

   /* create result shape */
   shp->ndims = IND_NDIMS(ss) + IND_NDIMS(ind) - 1;
   for (int i=0; i<d; i++)
      shp->dim[i] = IND_DIM(ind, i);
   for (int i=0; i<IND_NDIMS(ss); i++)
      shp->dim[d+i] = IND_DIM(ss, i);
   for (int i=d; i<IND_NDIMS(ind); i++)
      shp->dim[i+IND_NDIMS(ss)] = IND_DIM(ind, i+1);

   if (index_emptyp(res))
      goto clean_up_and_return3;

   /* copy selected array contents */
   ptrdiff_t ii = 0;
   ptrdiff_t ir = 0;
   islice = index_select(ind, d, 0);
   rslice = index_select(res, d, 0);
   
   switch (IND_STTYPE(res)) {
      
#define GenericTake(Prefix,Type) 				     \
  case name2(ST_,Prefix): {					     \
     								     \
    size_t indext = IND_DIM(ind, d);                                 \
    ptrdiff_t indmod = IND_MOD(ind, d);                              \
    ptrdiff_t resmod = IND_MOD(res, d);                              \
    int  *pss   = IND_BASE_TYPED(iss, int);			     \
    begin_idx_aloop1(iss, oss) { 			             \
       /* validate dimension */                                      \
       ii = pss[oss];                                                \
       ii = ii<0 ? ii + indext : ii;                                 \
       if (ii<0 || ii>=indext) {                                     \
           errmsg = errmsg_out_of_range;                             \
           goto clean_up_and_return3;                                \
       }                                                             \
       islice->offset = ind->offset + indmod*ii;                     \
       rslice->offset = res->offset + resmod*ir;                     \
       array_copy(islice, rslice);                                   \
       ir++;                                                         \
    } end_idx_aloop1(iss,oss);   			             \
  } 								     \
  break;

      GenericTake(FLOAT, float);
      GenericTake(DOUBLE, real);
      GenericTake(INT, int);
      GenericTake(SHORT, short);
      GenericTake(BYTE, char);
      GenericTake(UBYTE, unsigned char);
      GenericTake(GPTR, gptr);

#undef GenericTake
      
   default:
      errmsg = "element-type not supported";
      break;
   }

clean_up_and_return3:

   /* report any errors or return result */
   RAISEF(errmsg, NIL);
   return index_reshapeD(res, shp);
}


DX(xarray_take)
{
   switch (arg_number) {
   case 2: {
      index_t *iss = as_int_array(APOINTER(2));
      return array_take2(AINDEX(1), iss)->backptr;
   }
   case 3: {
      index_t *iss = as_int_array(APOINTER(3));
      return array_take3(AINDEX(1), AINTEGER(2), iss)->backptr;
   }
   default:
      ARG_NUMBER(-1);
      return NIL;
   }
}

index_t *array_put(index_t *ind, index_t *ss, index_t *vals)
{
   static char *errmsg_out_of_range = "subscript out of range";       
   static char *errmsg_not_supported = "element-type not supported";
   char *errmsg = NULL;
   
   int r = IND_NDIMS(ind);
   int d = IND_NDIMS(ss)-1;

   /* check & normalize arguments */
   ifn (IND_STTYPE(ss) == ST_INT)
      RAISEF("subscript array must be integer", NIL);
   ifn (IND_STTYPE(ind) == IND_STTYPE(vals))
      RAISEF("first and third array must have same element type", NIL);
   ifn (d >= 0)
      RAISEF("subscript array must not be scalar", NIL);
   ifn (IND_DIM(ss, d) == r)
      RAISEF("final extent of subscript array must match rank of first argument", NIL);
   shape_t shape, *shp = shape_copy((shape_t *)ss, &shape);
   shp->ndims -= 1;
   ifn (shape_equalp(shp, (shape_t *)vals))
      RAISEF("subscript array and value array must match in shape", NIL);
   
   if (index_emptyp(vals))
      goto clean_up_and_return2;

   /* copy selected array contents */
   get_write_permit(IND_ST(ind));
   ifn (IND_MOD(ss, d) == 1)  // when not contiguous in last dimension, make it so
      ss = copy_array(ss);
   
   switch (IND_STTYPE(ind)) {
      
#define GenericPut(Prefix,Type) 				     \
  case name2(ST_,Prefix): {					     \
     								     \
    index_t *ss0 = index_select(ss, d, 0);                           \
    Type *pi = IND_BASE_TYPED(ind, Type);			     \
    Type *pv = IND_BASE_TYPED(vals, Type);                           \
    int *ps = IND_BASE_TYPED(ss, int);                               \
    begin_idx_aloop3(ind, ss0, vals, indp, ss0p, valp) {             \
       /* validate dimension */                                      \
       ptrdiff_t offs = 0;                                           \
       for (int i = 0; i < r; i++) {                                 \
          int ii = ps[ss0p+i];                                       \
          if (ii < 0) ii += IND_DIM(ind, i);                         \
          if (ii<0 || ii>=IND_DIM(ind, i)) {                         \
             errmsg = errmsg_out_of_range;                           \
             goto clean_up_and_return2;                              \
          }                                                          \
          offs += IND_MOD(ind, i)*ii;                                \
       }                                                             \
       pi[offs] = pv[valp];                                          \
    } end_idx_aloop3(ind, ss0, vals, indp, ss0p, valp);              \
  } 								     \
  break;

      GenericPut(FLOAT, float);
      GenericPut(DOUBLE, real);
      GenericPut(INT, int);
      GenericPut(SHORT, short);
      GenericPut(BYTE, char);
      GenericPut(UBYTE, unsigned char);
      GenericPut(GPTR, gptr);

#undef GenericTake
      
   default:
      errmsg = errmsg_not_supported;
      break;
   }

clean_up_and_return2:

   /* report any errors or return result */
   RAISEF(errmsg, NIL);
   return ind;
}

DX(xarray_put)
{
   ARG_NUMBER(3);
   index_t *iss = as_int_array(APOINTER(2));
   return array_put(AINDEX(1), iss, AINDEX(3))->backptr;
}


/* index_transpose */

index_t *index_transpose(index_t *ind, shape_t *perm) {

   if (perm->ndims!=IND_NDIMS(ind))
      RAISEF("permutation list and shape must be of same length", NIL);

   /* check permutation list */
   bool tmp[MAXDIMS];
   for (int i=0; i<perm->ndims; i++) {
      tmp[i] = false;
      perm->dim[i] = validate_dimension(ind, perm->dim[i]);
   }
   for (int i=0; i<perm->ndims; i++)
      if (perm->dim[i]<0 || tmp[perm->dim[i]]) {
         RAISEF("Invalid permutation list", NIL);
      } else
         tmp[perm->dim[i]] = true;
  
   /* create transposed index */
   index_t *ind2 = copy_index(ind);
   for (int i=0; i<perm->ndims; i++) {
      ind2->dim[i] = ind->dim[perm->dim[i]];
      ind2->mod[i] = ind->mod[perm->dim[i]];
   }
   return ind2;
}

DX(xidx_transpose)
{
   ARG_NUMBER(2);
   shape_t shape, *perm = parse_shape(APOINTER(2), &shape);
   return index_transpose(AINDEX(1), perm)->backptr;
}

/* reverse index along the specified dimension */

index_t *index_reverseD(index_t *ind, int d) 
{
   d = validate_dimension(ind, d);
   ind->offset += IND_MOD(ind, d)*(IND_DIM(ind, d) - 1);
   IND_MOD(ind, d) = -IND_MOD(ind, d);
   return ind;
}

index_t *index_reverse(index_t *ind, int d)
{
   return index_reverseD(copy_index(ind), d);
}

DX(xidx_reverseD)
{
   ARG_NUMBER(2);
   index_reverseD(AINDEX(1), AINTEGER(2));
   return APOINTER(1);
}

DX(xidx_reverse)
{
   ARG_NUMBER(2);
   return index_reverse(AINDEX(1), AINTEGER(2))->backptr;
}

/* patch the dim, mod and offset members of an index */

DX(xidx_set_dim)
{
   ARG_NUMBER(3);
   index_t *ind = AINDEX(1);
   int d = validate_dimension(ind, AINTEGER(2)); 
   size_t s = AINTEGER(3);
   size_t olds = IND_DIM(ind, d);
   
   s = s<0 ? IND_DIM(ind, d)+s : s;
   if (s<=0)
      error(NIL, "new dimension value is negative", NEW_NUMBER(s));
   IND_DIM(ind, d) = s;
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      IND_DIM(ind, d) = olds;
      RAISEF(errmsg, NIL);
   }
   return NIL;
}

DX(xidx_set_mod)
{
   ARG_NUMBER(3);
   index_t *ind = AINDEX(1);
   int d = validate_dimension(ind, AINTEGER(2));
   ptrdiff_t oldmod = IND_MOD(ind, d);
   
   IND_MOD(ind, d) = (ptrdiff_t) AINTEGER(3);
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      IND_MOD(ind, d) = oldmod;
      RAISEF(errmsg, NIL);
   }
   return NIL;
}

DX(xidx_set_offset)
{
   ARG_NUMBER(2);
   index_t *ind = AINDEX(1);
   ptrdiff_t oldoff = ind->offset;

   ind->offset = (ptrdiff_t) AINTEGER(2);
   char *errmsg = chk_index_consistent(ind);
   if (errmsg) {
      ind->offset = oldoff;
      RAISEF(errmsg, NIL);
   }
   return NIL;
}

/* ----------------- THE LOOPS ---------------- */

#define MAXEBLOOP 8


static int ebloop_args(at *p, at **syms, at **iats, 
                       struct index **inds, at **last_index)
{
   at *q, *r, *s;
   int n;
   
   ifn ( CONSP(p) && CONSP(Car(p)) )
      error(NIL, "syntax error", NIL);
  
   n = 0;
   r = NIL;
   for (q = Car(p); CONSP(q); q = Cdr(q)) {
      p = Car(q);
      ifn (CONSP(p) && CONSP(Cdr(p)) && !Cddr(p))
         error(NIL, "syntax error in variable declaration", p);
      ifn ((s = Car(p)) && SYMBOLP(s))
         error(NIL, "symbol expected", s);
      r = eval(Cadr(p));
      ifn (INDEXP(r))
         RAISE(NIL, "not an index", r);
    
      if (n >= MAXEBLOOP)
         error(NIL,"Looping on too many indexes",NIL);
      syms[n] = s;
      iats[n] = s = COPY_INDEX(Mptr(r));
      inds[n] = Mptr(s);
      n++;
   }
   *last_index = r;
   if (q)
      error(NIL, "syntax error in variable declaration", q);
   return n;
}


DY(yeloop)
{
   at *ans, *last_index;
   int d, n;
   index_t *ind;
   
   at *syms[MAXEBLOOP];
   at *iats[MAXEBLOOP];
   struct index *inds[MAXEBLOOP];
   
   n = ebloop_args(ARG_LIST, syms, iats, inds, &last_index);
   d = inds[0]->dim[ inds[0]->ndim - 1 ];
   for (int i=0; i<n; i++) {
      ind = inds[i];
      int j = --(ind->ndim);		/* remove last dimension */
      if (d != ind->dim[j])
         error(NIL,"looping dimension are different", Car(ARG_LIST));
      SYMBOL_PUSH(syms[i], iats[i]);
   }

   /* loop */
   for (int j=0; j<d; j++) {
      ans = progn(Cdr(ARG_LIST));
      for (int i=0; i<n; i++)
         if (INDEXP(iats[i])) {
            /* check not deleted! */
            ind = inds[i];
            ind->offset += ind->mod[ ind->ndim ];
         }
   }
   for (int i=0; i<n; i++) {
      SYMBOL_POP(syms[i]);
   }
   return last_index;
}



DY(ybloop)
{
   at *syms[MAXEBLOOP];
   at *iats[MAXEBLOOP];
   index_t *inds[MAXEBLOOP];
  
   at *last_index;
   int n = ebloop_args(ARG_LIST, syms, iats, inds, &last_index);
   int d = inds[0]->dim[ 0 ];
   index_t *ind;
   for (int i=0; i<n; i++) {
      ind = inds[i];
      if (d != ind->dim[0])
         error(NIL,"looping dimension are different", Car(ARG_LIST));
      --(ind->ndim);		/* remove one dimension */
      int m = ind->mod[0];		/* reorganize the dim and mod arrays */
      for (int j=0; j<ind->ndim; j++) {
         ind->dim[j] = ind->dim[j+1];
         ind->mod[j] = ind->mod[j+1];
      }
      ind->mod[ind->ndim] = m;	/* put the stride at the end! */
      SYMBOL_PUSH(syms[i], iats[i]);
   }
   
   at *ans = NIL;			/* loop */
   for (int j=0; j<d; j++) {
      ans = progn(Cdr(ARG_LIST));
      for (int i=0; i<n; i++)
         if (INDEXP(iats[i])) {
            /* check not deleted! */
            ind = inds[i];
            ind->offset += ind->mod[ind->ndim];
         }
   }
   for (int i=0; i<n; i++) {
      SYMBOL_POP(syms[i]);
   }
   return last_index;
}



class_t index_class;

void init_index(void)
{
   mt_index = MM_REGTYPE("index", sizeof(index_t),
                         clear_index, mark_index, finalize_index);

   /* setting up index_class */
   class_init(&index_class, false);
   index_class.dispose = (dispose_func_t *)index_dispose;
   index_class.name = index_name;
   index_class.listeval = index_listeval;
   index_class.serialize = index_serialize;
   index_class.compare = index_compare;
   index_class.hash = index_hash;
   index_class.getslot = index_getslot;
   index_class.setslot = index_setslot;
   class_define("INDEX",&index_class);

   /* info */
   dx_define("indexp", xindexp);
   dx_define("idx-numericp", xidx_numericp);
   dx_define("idx-contiguousp", xidx_contiguousp);
   dx_define("idx-rank", xidx_rank); 
   dx_define("idx-shape", xidx_shape);
   dx_define("$", xindex_shape);
   dx_define("idx-nelems", xidx_nelems);
   dx_define("idx-storage", xidx_storage);
   dx_define("idx-offset", xidx_offset);
   dx_define("idx-modulo", xidx_modulo);
   dx_define("idx-base", xidx_base);
   dx_define("idx-dc", xidx_dc);
   dx_define("idx-broadcastable-p", xidx_broadcastable_p);
   
   /* array and index creation */
   dx_define("new-index", xnew_index);
   dx_define("make-array", xmake_array);
   dx_define("copy-index", xcopy_index);
   dx_define("copy-array", xcopy_array);
   
   /* argument processing */
   dx_define("as-double-array", xas_double_array);
   dx_define("as-int-array", xas_int_array);
   dx_define("as-ubyte-array", xas_ubyte_array);

   /* matrix files */
   dx_define("save-matrix", xsave_matrix);
   dx_define("save-ascii-matrix", xsave_ascii_matrix);
   dx_define("export-raw-matrix", xexport_raw_matrix);
   dx_define("export-text-matrix", xexport_text_matrix);
   dx_define("import-raw-matrix", ximport_raw_matrix);
   dx_define("import-text-matrix", ximport_text_matrix);
   dx_define("load-matrix", xload_matrix);
   dx_define("array-export-tabular", xarray_export_tabular);
#ifdef HAVE_MMAP
   dx_define("map-matrix", xmap_matrix);
#endif


   /* index manipulation */
   dx_define("idx-reshape", xidx_reshape);
   dx_define("$$",xindex_reshape);
   dx_define("idx-flatten", xidx_flatten);
   dx_define("idx-nick", xidx_nick);
   dx_define("idx-extend", xidx_extend);
   dx_define("idx-lift", xidx_lift);
   dx_define("idx-sink", xidx_sink);
   dx_define("idx-expand", xidx_expand);
   dx_define("idx-broadcast1", xidx_broadcast1);
   dx_define("idx-broadcast2", xidx_broadcast2);
   dx_define("idx-shift", xidx_shift);
   dx_define("idx-trim", xidx_trim);
   dx_define("idx-select", xidx_select);
   dy_define("idx-select*", yidx_selectS);
   dx_define("idx-select-all", xidx_select_all);
   dx_define("idx-transpose", xidx_transpose);
   dx_define("idx-reverse", xidx_reverse);
   
   /* in-place functions */
   dx_define("idx-reshape!", xidx_reshapeD);
   dx_define("idx-nick!", xidx_nickD);
   dx_define("idx-unfold!", xidx_unfoldD);
   dx_define("idx-extend!", xidx_extendD);
   dx_define("idx-lift!", xidx_liftD);
   dx_define("idx-sink!", xidx_sinkD);
   dx_define("idx-expand!", xidx_expandD);
   dx_define("idx-shift!", xidx_shiftD);
   dx_define("idx-trim!", xidx_trimD);
   dx_define("idx-narrow!", xidx_narrowD);
   dx_define("idx-reverse!", xidx_reverseD);
   dx_define("idx-set-dim", xidx_set_dim);
   dx_define("idx-set-mod", xidx_set_mod);
   dx_define("idx-set-offset", xidx_set_offset);

   /* other array functions */
   dx_define("array-dc", xarray_dc);
   dx_define("array-extend", xarray_extend);
   dx_define("array-copy", xarray_copy);
   dx_define("array-swap", xarray_swap);
   dx_define("array-take", xarray_take);
   dx_define("array-put", xarray_put);
   dx_define("array-where-nonzero", xarray_where_nonzero);

   /* loops */
   dy_define("idx-eloop", yeloop);
   dy_define("idx-bloop", ybloop);
   
   at* p = var_define("+MAXDIMS+");
   var_set(p, NEW_NUMBER(MAXDIMS));
   var_lock(p);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
