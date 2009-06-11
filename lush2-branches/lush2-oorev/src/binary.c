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

#include "header.h"

typedef unsigned char   uchar;

#define BINARYSTART     (0x9f)

enum binarytokens {
   TOK_NULL,
   TOK_REF,
   TOK_DEF,
   TOK_NIL,
   TOK_CONS,
   TOK_NUMBER,
   TOK_STRING,
   TOK_SYMBOL,
   TOK_OBJECT,
   TOK_CLASS,
   TOK_MATRIX,
   TOK_ARRAY,
   TOK_DE,
   TOK_DF,
   TOK_DM,
   TOK_CFUNC,
   TOK_CCLASS,
   TOK_COBJECT,
};


/* flags */

#define MARK_BIT          2
#define MULTIPLE_BIT      4

#define CLEAR(p)  ((void *)((uintptr_t)(p)&~(MARK_BIT + MULTIPLE_BIT)))


/*** GLOBAL VARIABLES ****/

int          in_bwrite = 0;
static int   opt_bwrite = 0;
static FILE *fin;
static FILE *fout;


/*** THE RELOCATION STRUCTURE **********/

static int relocm = 0;
static int relocn = 0;
static gptr *relocp = 0;
static char *relocf = 0;

#define R_EMPTY   0
#define R_REFD    1
#define R_DEFD    2


static void check_reloc_size(int m)
{
   if (relocm >= m) 
      return;
   
   char *f = NULL;
   gptr *p = NULL;
   if (relocm == 0) {
      m = 1 + (m | 0x3ff);
      f = malloc(sizeof(char)*m);
      p = malloc(sizeof(gptr)*m);
   } else {
      m = 1 + (m | 0x3ff);
      f = realloc(relocf,sizeof(char)*m);
      p = realloc(relocp,sizeof(gptr)*m);
   }
   if (f && p) {
      relocf = f;
      relocp = p;
      relocm = m;
   } else  {
      if (f) free(f);
      if (p) free(p);
      relocf = 0;
      relocp = 0;
      relocm = 0;
      error(NIL, "not enough memory", NIL);
   }
}


static void clear_reloc(int n)
{
   check_reloc_size(n);
   for (int i=0; i<n; i++) {
      relocp[i] = 0;
      relocf[i] = 0;
   }
   relocn = n;
}


static void insert_reloc(void *p)
{
   check_reloc_size(relocn+1);
   int i = relocn;
   relocn += 1;
   relocp[i] = 0;
   relocf[i] = 0;

   while (i>0 && p<=relocp[i-1]) {
      relocp[i] = relocp[i-1];
      relocf[i] = relocf[i-1];
      i--;
   }
   if (relocp[i]==p)
      error(NIL, "internal error: relocation requested twice",NIL);
   relocp[i] = p;
   relocf[i] = R_EMPTY;
}


static int search_reloc(void *p)
{
   int lo = 0;
   int hi = relocn - 1;
   int k;
   while (hi>=lo) {
      k = (lo+hi)/2;
      if (p>relocp[k])
         lo = k+1;
      else if (p<relocp[k])
         hi = k-1;
      else
         return k;
   }
   error(NIL, "internal error: relocation search failed", NIL);
}


static void define_refd_reloc(int k, at *p)
{
   at **h = relocp[k];
   while (h) {
      gptr f = *h;
      *h = p;
      h = f;
   }
}


static void forbid_refd_reloc(void)
{
   int flag = 0;
   for (int i=0; i<relocn; i++) {
      if (relocf[i]!= R_DEFD) 
         flag = 1;
      if (relocf[i]== R_REFD)
         define_refd_reloc(i,NIL);
   }
   if (flag)
      error(NIL, "corrupted binary file (unresolved relocs)", NIL);
}


/*** SWEEP OVER THE ELEMENTS TO SAVE ***/

static int cond_set_flags(at *p);
static int cond_clear_flags(at *p);
static int local_write(at *p);
static int local_bread(at **pp, int opt);

/* This function recursively performs an action on
 * all objects under p. If action returns a non 0
 * result, recursion is stopped.
 */ 

static void sweep(at *p, int code)
{
  
again:
  
   switch (code) {
   case SRZ_SETFL:
      if (cond_set_flags(p))
         return;
      break;

   case SRZ_CLRFL:
      if (cond_clear_flags(p))
         return;
      break;

   case SRZ_WRITE:
      if (local_write(p))
         return;
      break;
   default:
      error(NIL, "internal error (unknown serialization action)", NIL);
   }
   
   if (!p)
      return;

   const class_t *cl = Class(p);

   if (cl == cons_class) {
      sweep(Car(p),code);
      p = Cdr(p);
      goto again;
      
   } 
   cl = CLEAR(cl);

   if (cl==number_class || cl==gptr_class) {
      return;

   } else if (cl->dispose == object_class->dispose) {
      object_t *c = Mptr(p);
      if (opt_bwrite)
         sweep(cl->backptr, code);
      else
         sweep(cl->classname, code);
      for(int i=0; i<c->size; i++) {
	sweep(c->slots[i].symb, code);
	sweep(c->slots[i].val, code);
      }
   } else if (cl == class_class) {
      class_t *clcl = Mptr(p);
      sweep(clcl->priminame, code);
      if (!builtin_class_p(clcl) && !clcl->classdoc) {
         sweep(clcl->atsuper, code);
         sweep(clcl->keylist, code);
         sweep(clcl->defaults, code);
      }
      sweep(clcl->methods, code);
   } else if (cl == index_class && !opt_bwrite) {
      index_t *ind = Mptr(p);
      if (IND_STTYPE(ind) == ST_AT)
         if (!index_emptyp(ind)) {
            at** data;
            struct idx id;
            index_read_idx(ind, &id);
            data = IDX_DATA_PTR(&id);
            begin_idx_aloop1(&id, off) {
               sweep(data[off], code);
            } end_idx_aloop1(&id, off);
            index_rls_idx(ind, &id);
         }
   } else if (cl == de_class || cl == df_class || cl == dm_class ) {
      lfunction_t *f = Mptr(p);
      sweep(f->formal_args, code);
      sweep(f->body, code);
      
   } else if (cl == dx_class || cl == dy_class || cl == dh_class ) {
      cfunction_t *f = Mptr(p);
      sweep(f->name, code);
      
   } else if (cl->serialize) {
      sweep(cl->classname, code);
      (cl->serialize) (&p, code);
   }
}



/*** MARKING, FLAGGING ***/

/* This functions computes all objects
 * which must be relocated when saving object p 
 */

static int cond_set_flags(at *p)
{
   if (p) {
      uintptr_t bs = PTRBITS(p->head.cl);
      if (bs & MARK_BIT) {
         if (! (bs & MULTIPLE_BIT))
            insert_reloc(p);
         SET_PTRBIT(p->head.cl, MULTIPLE_BIT);
         return 1;  /* stop recursion */
      }
      SET_PTRBIT(p->head.cl, MARK_BIT);
      return 0;
   }
   return 1;
}


static void set_flags(at *p)
{
   sweep(p, SRZ_SETFL);
}

/* This function clears all flags 
 * leaving Lush ready for another run
 */

static int cond_clear_flags(at *p)
{
   if (p && ((uintptr_t)(p->head.cl) & MARK_BIT)) {
      p->head.cl = CLEAR(p->head.cl);
      return 0;
   } else
      return 1; /* stop recursion if already cleared */
}

static void clear_flags(at *p)
{
   sweep(p, SRZ_CLRFL);
}



/*** LOW LEVEL CHECK/READ/WRITE ***/

static void check(FILE *f)
{
   if (feof(f))
      error(NIL, "end of file during bread", NIL);
   if (ferror(f))
      test_file_error(NULL);
}

/* read */

static int read_card8(void)
{
   uchar c[1];
   if (fread(c, sizeof(char), 1, fin) != 1)
      check(fin);
   return c[0];
}

static int read_card16(void)
{
   uchar c[2];
   if (fread(c, sizeof(char), 2, fin) != 2)
      check(fin);
   return (c[0]<<8)+c[1];
}

static int read_card24(void)
{
   uchar c[3];
   if (fread(c, sizeof(char), 3, fin) != 3)
      check(fin);
   return (((c[0]<<8)+c[1])<<8)+c[2];
}

static int read_card32(void)
{
   uchar c[4];
   if (fread(c, sizeof(char), 4, fin) != 4)
      check(fin);
   return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}

static inline void read_buffer(void *s, int n)
{
   if (fread(s, sizeof(char), (size_t)n, fin) != (size_t)n)
      check(fin);
}


/* write */

static void write_card8(int x)
{
   char c[1];
   in_bwrite += 1;
   c[0] = x;
   if (fwrite(&c, sizeof(char), 1, fout) != 1)
      check(fout);
}

static void write_card16(int x)
{
   char c[2];
   in_bwrite += 2;
   c[0] = x>>8;
   c[1] = x;
   if (fwrite(&c, sizeof(char), 2, fout) != 2)
      check(fout);
}

static void write_card24(int x)
{
   char c[3];
   in_bwrite += 3;
   c[0] = x>>16;
   c[1] = x>>8;
   c[2] = x;
   if (fwrite(&c, sizeof(char), 3, fout) != 3)
     check(fout);
}

static void write_card32(int x)
{
   char c[4];
   in_bwrite += 4;
   c[0] = x>>24;
   c[1] = x>>16;
   c[2] = x>>8;
   c[3] = x;
   if (fwrite(&c, sizeof(char), 4, fout) != 4)
      check(fout);
}

static void write_buffer(const void *s, int n)
{
   in_bwrite += n;
   if (fwrite(s, sizeof(char), (size_t)n, fout) != (size_t)n)
      check(fout);
}


/*** The swap stuff ***/

static int swapflag;

static void  set_swapflag(void)
{
   union { int i; char c; } s;
   s.i = 1;
   swapflag = s.c;
}

static void swap_buffer(void *bb, int n, int m)
{
   char *b = bb;
   char buffer[16];
   for (int i=0; i<n; i++) {
      for (int j=0; j<m; j++)
         buffer[j] = b[m-j-1];
      for (int j=0; j<m; j++)
         *b++ = buffer[j];
   }
}


/*** The serialization stuff ***/

FILE *serialization_file_descriptor(int code)
{
   switch (code) {
   case SRZ_READ:
      return fin;

   case SRZ_WRITE:
      return fout;

   default:
      return NULL;
   }
}

void serialize_char(char *data, int code)
{
   switch (code) {
   case SRZ_READ:
      *data = read_card8();
      return;

   case SRZ_WRITE:
      write_card8(*data);
      return;
   }
}

void serialize_short(short int *data, int code)
{
   switch(code) {
   case SRZ_READ:
      *data = read_card16();
      return;
      
   case SRZ_WRITE:
      write_card16((int)*data);
      return;
   }
}

void serialize_int(int *data, int code)
{
   switch(code) {
   case SRZ_READ:
      *data = read_card32();
      return;
   case SRZ_WRITE:
      write_card32((int)*data);
      return;
   }
}


void serialize_size(size_t *data, int code)
{
   switch(code) {
   case SRZ_READ:
      read_buffer(data, sizeof(size_t));
      return;

   case SRZ_WRITE:
      write_buffer(data, sizeof(size_t));
      return;
   }
}

void serialize_offset(ptrdiff_t *data, int code)
{
   switch(code) {
   case SRZ_READ:
      read_buffer(data, sizeof(ptrdiff_t));
      return;

   case SRZ_WRITE:
      write_buffer(data, sizeof(ptrdiff_t));
      return;
   }
}

/* in READ mode with maxlen==-1, serialize_string returns a managed string */
void serialize_string(char **data, int code, int maxlen)
{
   switch (code) {
      int  l;
      char *buffer;

   case SRZ_READ:
      if (read_card8() != 's')
         error(NIL,"serialization error (string expected)",NIL);
      l = read_card24();
      if (maxlen == -1) {
         /* automatic mallocation */
         buffer = mm_blob(l+1);

      } else {
         /* user provided buffer */
         buffer = *data;
         if (l+1 >= maxlen)
            error(NIL, "serialization error (string too large)", NIL);
      }
      read_buffer(buffer, l);
      buffer[l] = 0;
      *data = buffer;
      return;
      
   case SRZ_WRITE:
      write_card8('s');
      l = strlen(*data);
      write_card24(l);
      write_buffer(*data, l);
      return;
   }
}

void serialize_chars(void **data, int code, int thelen)
{
   switch (code) {
      int  l;
      char *buffer;

   case SRZ_READ:
      if (read_card8() != 's')
         error(NIL, "serialization error (string expected)",NIL);
      l = read_card32();
      if (thelen == -1) {
         /* automatic mallocation */
         ifn ((buffer = malloc(l))) {
            error(NIL, "out of memory", NIL);
         }
      } else {
         /* user provided buffer */
         buffer = *data;
         if (l != thelen)
            error(NIL,"serialization error (lengths mismatch)",NIL);
      }
      read_buffer(buffer,l);
      *data = buffer;
      return;
      
   case SRZ_WRITE:
      write_card8('s');
      write_card32(thelen);
      write_buffer(*data,thelen);
      return;
   }
}

void serialize_flt(flt *data, int code)
{
   switch (code) {
      flt x;

   case SRZ_READ:
      read_buffer(&x, sizeof(flt));
      if (swapflag)
         swap_buffer(&x,1,sizeof(flt));
      *data = x;
      return;

   case SRZ_WRITE:
      x = *data;
      if (swapflag)
         swap_buffer(&x,1,sizeof(flt));
      write_buffer(&x, sizeof(flt));
      return;
   }
}

void serialize_float(float *data, int code)
{
   switch (code) {
      float x;

   case SRZ_READ:
      read_buffer(&x, sizeof(float));
      if (swapflag)
         swap_buffer(&x,1,sizeof(float));
      *data = x;
      return;

   case SRZ_WRITE:
      x = *data;
      if (swapflag)
         swap_buffer(&x,1,sizeof(float));
      write_buffer(&x, sizeof(float));
      return;
   }
}

void serialize_real(real *data, int code)
{
   switch (code) {
      real x;

   case SRZ_READ:
      read_buffer(&x, sizeof(real));
      if (swapflag)
         swap_buffer(&x,1,sizeof(real));
      *data = x;
      return;

   case SRZ_WRITE:
      x = *data;
      if (swapflag)
         swap_buffer(&x,1,sizeof(real));
      write_buffer(&x, sizeof(real));
      return;
   }
}

void serialize_double(double *data, int code)
{
   switch (code) {
      double x;

   case SRZ_READ:
      read_buffer(&x, sizeof(double));
      if (swapflag)
         swap_buffer(&x,1,sizeof(double));
      *data = x;
      return;
      
   case SRZ_WRITE:
      x = *data;
      if (swapflag)
         swap_buffer(&x,1,sizeof(double));
      write_buffer(&x, sizeof(double));
      return;
   }
}

int serialize_atstar(at **data, int code)
{
   switch (code) {

   case SRZ_CLRFL:
   case SRZ_SETFL:
      sweep(*data, code);
      return 0;

   case SRZ_READ:
      if (read_card8() != 'p')
         error(NIL, "serialization error (pointer expected)", NIL);
      return local_bread(data, NIL);

   case SRZ_WRITE:
      write_card8('p');
      sweep(*data, code);
      return 0;
   }
   error(NIL,"binary internal: wrong serialization mode",NIL);
}


/*** BINARY WRITE ***/

/* This local writing routine is called from sweep */

static int local_write(at *p)
{
   if (!p || ZOMBIEP(p)) {
      write_card8(TOK_NIL);
      return 1;
   }
  
   if ((uintptr_t)(p->head.cl) & MULTIPLE_BIT) {
      int k = search_reloc(p);
      if (relocf[k]) {
         write_card8(TOK_REF);
         write_card24(k);
         return 1;

      } else {
         write_card8(TOK_DEF);
         write_card24(k);
         relocf[k] = R_DEFD;
         /* continue */
      }
   }
  
   const class_t *cl = Class(p);
   if (cl == cons_class) {
      write_card8(TOK_CONS);
      return 0;
   }
   cl = CLEAR(cl);
  
   if (cl == number_class) {
      double x = Number(p);
      write_card8(TOK_NUMBER);
      if (swapflag)
         swap_buffer(&x,1,sizeof(real));
      write_buffer( &x, sizeof(real) );
      return 1;
   }
  
   if (cl == string_class) {
      const char *s = String(p);
      int l = strlen(s);
      write_card8(TOK_STRING);
      write_card24(l);
      write_buffer(s,l);
      return 1;
   }
  
   if (cl == symbol_class) {
      const char *s = nameof(Symbol(p));
      int l = strlen(s);
      write_card8(TOK_SYMBOL);
      write_card24(l);
      write_buffer(s,l);
      return 1;
   }
  
   if (cl->dispose == object_class->dispose) {
      object_t *o = Mptr(p);
      write_card8(TOK_OBJECT);
      write_card24(o->size);
      return 0;
   }
  
   if (cl == class_class) {
      class_t *c = Mptr(p);
      if (!builtin_class_p(c) && !c->classdoc)
         write_card8(TOK_CLASS);	
      else
         write_card8(TOK_CCLASS);
      return 0;
   }
  
   if (cl == index_class && !opt_bwrite) {
      index_t *arr = Mptr(p);
      if (arr->st->type == ST_AT) {
         int ndim = arr->ndim;
         write_card8(TOK_ARRAY);
         write_card24(ndim);
         for (int i=0; i<ndim; i++)
            write_card32(arr->dim[i]);
         return 0;

      } else {
         write_card8(TOK_MATRIX);
         in_bwrite += save_matrix_len(arr);
         if (arr->st->type == ST_GPTR)
            error(NIL,"cannot save a gptr array",p);
         save_matrix(arr, fout);
         return 1;
      }
   }
  
   if (cl == de_class) {
      write_card8(TOK_DE);
      return 0;
   }
  
   if (cl == df_class) {
      write_card8(TOK_DF);
      return 0;
   }
  
   if (cl == dm_class) {
      write_card8(TOK_DM);
      return 0;
   }
  
   if (cl == dx_class || cl == dy_class || cl == dh_class) {
      write_card8(TOK_CFUNC);
      return 0;
   }
  
   if (cl->serialize) {
      write_card8(TOK_COBJECT);
      return 0;
   }

   error(NIL, "cannot save this object", p);
}


/* Writes object p on file f.
 * Returns the number of bytes
 */

int bwrite(at *p, FILE *f, int opt)
{
   if (in_bwrite!=0)
      error(NIL,"Recursive binary read/write are forbidden",NIL);
   opt_bwrite = opt;
   
   fout = f;
   in_bwrite = 0;
   clear_reloc(0);
   set_flags(p);
   write_card8(BINARYSTART);
   write_card24(relocn);
   sweep(p, SRZ_WRITE);
   clear_flags(p);
   
   int count = in_bwrite;
   in_bwrite = 0;
   return count;
}

DX(xbwrite)
{
   int count = 0;
   for (int i=1; i<=arg_number; i++) 
      count += bwrite( APOINTER(i), context->output_file, false );
   return NEW_NUMBER(count);
}

DX(xbwrite_exact)
{
   int count = 0;
   for (int i=1; i<=arg_number; i++) 
      count += bwrite( APOINTER(i), context->output_file, true );
   return NEW_NUMBER(count);
}


/*** BINARY READ ***/

/* Special routines for reading array and objects */

static void local_bread_cobject(at **pp)
{
   at *cname;
   if (local_bread(&cname, NIL))
      error(NIL,"Corrupted file (unresolved class)",NIL);
   at *name = cname;
   at *cptr = NIL;
   if (CONSP(cname))
      cptr = find_primitive(Car(cname), (name = Cdr(cname)));
   if (!cptr)
      cptr = find_primitive(NIL, name);
   if (!SYMBOLP(name))
      error(NIL, "corrupted file (expecting class name)", NIL);
   if (!CLASSP(cptr))
      error(NIL, "cannot find primitive class", name);
  
   class_t *cl = Mptr(cptr);
   if (! cl->serialize)
      error(NIL, "serialization error (undefined serialization)", name);
   *pp = 0;
   (*cl->serialize)(pp, SRZ_READ);
}

static void local_bread_object(at **pp,  int opt)
{
   int size = read_card24();
   at *cptr;
   if (local_bread(&cptr, NIL))
      error(NIL,"Corrupted file (unresolved class)",NIL);
   at *cname = cptr;
   if (SYMBOLP(cname))
      cptr = var_get(cname);
   if (! CLASSP(cptr))
      error(NIL,"corrupted binary file (class expected)",cname);    
   if (cname == cptr) {
      cname = ((class_t *)Mptr(cptr))->classname;
   }
  
   class_t *cl = Mptr(cptr);
   *pp = new_object(cl);     /* create structure */
   object_t *s = Mptr(*pp);
   if (size > s->size) 
      error(NIL, "class definition has less slots than expected",cname);
   else  if ( (size < s->size) && opt)
      error(NIL,"class definition has more slots than expected",cname);  
   
   at *name;
   for(int i=0; i<size; i++) {
      if (local_bread(&name, NIL))
         error(NIL,"corrupted file (accessing slot name)",cname);
      ifn (SYMBOLP(name))
         error(NIL,"corrupted binary file (slot name expected)",cname);
      int j = i;
      if (name!=s->slots[j].symb) {
         for (j=(i+1) % size; j!=i; j++)
            if (name==s->slots[j].symb) break;
         if (j==i) 
            error(NIL, "incompatible class definition",cname);
      }
      s->slots[j].val = NIL;
      local_bread(&(s->slots[j].val), NIL);
   }
}

static void local_bread_class(at **pp)
{
   at *name, *super, *key, *def;
   if (local_bread(&name, NIL) || 
       local_bread(&super, NIL) || 
       local_bread(&key, NIL) || 
       local_bread(&def, NIL) )
      error(NIL, "corrupted file (unresolved critical class component)", NIL);
   *pp = new_ooclass(name,super,key,def);
   class_t *cl = Mptr(*pp);
   local_bread(&cl->methods, NIL);
   cl->hashok = false;
}


static void local_bread_array(at **pp)
{
   int ndims = read_card24();
   if (ndims == 0xFFFFFF) 
      /* THIS IS AN UNSIZED ARRAY */
      *pp = NEW_INDEX(new_storage(ST_AT), NIL);
   
   else if (ndims < MAXDIMS) {
      /* THIS IS A NORMAL ARRAY */
      shape_t shape = {0, {}};  
      size_t size = 1;
      for (int i=0; i<ndims; i++) 
         size *= ( shape.dim[i] = read_card32() );
      shape.ndims = ndims;
      *pp = MAKE_ARRAY(ST_AT, &shape, NIL);
      index_t *ind = Mptr(*pp);
      struct idx id;
      index_write_idx(ind, &id);
      pp = IDX_DATA_PTR(&id);
      for (int i=0; i<size; i++)
         local_bread(pp++, NIL);
      index_rls_idx(ind,&id);

   } else
      error(NIL, "corrupted binary file", NIL);
}


static void local_bread_lfunction(at **pp, at *(*new) (at*, at*))
{
   *pp = (*new)(NIL,NIL);
   lfunction_t *f = Mptr(*pp);
   local_bread(&f->formal_args, NIL);
   local_bread(&f->body, NIL);
}


static void local_bread_primitive(at **pp)
{
   at *p, *q;
   if (local_bread(&q, NIL))
      error(NIL, "corrupted file (unresolved symbol!)", NIL);
   if (CONSP(q)) {
      p = find_primitive(Car(q), Cdr(q));
      if (! p)
         p = find_primitive(NIL, Cdr(q));      
   } else 
      p = find_primitive(NIL,q);

   if (! p)
      error(NIL, "cannot find primitive", q);
   *pp = p;
}


/* This is the basic reading routine */

static int local_bread(at **pp, int opt)
{
   int tok, ret = 1;
  
again:
   tok = read_card8();

   switch (tok) {
   case TOK_DEF:
   {
      int xdef = read_card24();
      if (local_bread(pp, opt))
         error(NIL, "corrupted binary file (unresolved ref follows def)", NIL);
      if (relocf[xdef] == R_DEFD)
         error(NIL, "corrupted binary file (duplicate reloc definition)", NIL);
      if (relocf[xdef] == R_REFD)
         define_refd_reloc(xdef,*pp);
      relocf[xdef] = R_DEFD;
      relocp[xdef] = *pp;
      return 0;
   }
   
   case TOK_REF:
   {
      int xdef = read_card24();
      if (xdef<0 || xdef>=relocn)
         error(NIL, "corrupted binary file (illegal ref)", NIL);
      if (relocf[xdef] == R_DEFD) {
         *pp = relocp[xdef];
         return 0;

      } else {
         *pp = relocp[xdef];
         relocp[xdef] = pp;
         relocf[xdef] = R_REFD;
         return ret;
      }
   }
      
   case TOK_NIL:
   {
      *pp = NIL;
      return 0;
   }
      
   case TOK_CONS:
   {
      *pp = new_cons(NIL,NIL);
      //local_bread(&Car(*pp), opt);
      local_bread((at **)&((*pp)->head.cl), opt);
      SET_PTRBIT((*pp)->head.cl, CONS_BIT);
      pp = &Cdr(*pp);
      ret = 0;
      goto again;
   }
      
   case TOK_NUMBER:
   {
      *pp = new_number(0.0);
      read_buffer(&Number(*pp), sizeof(real));
      if (swapflag)
         swap_buffer(&Number(*pp), 1, sizeof(real));
      return 0;
   }
   
   case TOK_STRING:
   {
      int l = read_card24();
      *pp = make_string_of_length(l);
      read_buffer(Mptr(*pp), l);
      return 0;
   }
   
   case TOK_SYMBOL:
   {
      int l = read_card24();
      read_buffer(string_buffer,l);
      string_buffer[l] = 0;
      *pp = named(string_buffer);
      return 0;
   }
      
   case TOK_OBJECT:
   {
      local_bread_object(pp, opt);
      return 0;
   }
      
   case TOK_CLASS:
   {
      local_bread_class(pp);
      return 0;
   }
      
   case TOK_ARRAY:
   {
      local_bread_array(pp);
      return 0;
   }
   
   case TOK_MATRIX:
   {
      *pp = load_matrix(fin);
      return 0;
   }
   
   case TOK_DE:
   {
      local_bread_lfunction(pp,new_de);
      return 0;
   }
   
   case TOK_DF:
   {
      local_bread_lfunction(pp,new_df);
      return 0;
   }
      
   case TOK_DM:
   {
      local_bread_lfunction(pp,new_dm);
      return 0;
   }
   
   case TOK_CFUNC:
   {
      local_bread_primitive(pp);
      return 0;
   }
      
   case TOK_CCLASS:
   {
      local_bread_primitive(pp);
      class_t *cl = Mptr(*pp);
      local_bread(&cl->methods, opt);
      cl->hashok = 0;
      return 0;
   }
   
   case TOK_COBJECT:
   {
      local_bread_cobject(pp);
      return 0;
   }
   
   default:
      error(NIL, "corrupted binary file (illegal token in file)", NEW_NUMBER(tok));
   }
}



at *bread(FILE *f, int opt)
{
   if (in_bwrite!=0)
      error(NIL, "recursive binary read/write are forbidden", NIL);
   
   fin = f;
   in_bwrite = -1;
   int tok = read_card8();
   if (tok != BINARYSTART)
      error(NIL, "corrupted binary file (cannot find BINARYSTART)", NIL);
   tok = read_card24();
   if (tok<0 || tok>10000000)
      error(NIL,"corrupted binary file (illegal reloc number)",NEW_NUMBER(tok));    
   clear_reloc(tok);

   at *ans = NIL;
   local_bread(&ans, opt);
   forbid_refd_reloc();
   in_bwrite = 0;
   return ans;
}

DX(xbread)
{
   ARG_NUMBER(0);
   return bread(context->input_file, false);
}

DX(xbread_exact)
{
   ARG_NUMBER(0);
   return bread(context->input_file, true);
}

/*** INITIALISATION ***/

void init_binary(void)
{
   assert(sizeof(short)==2); /* fix serialize_short */
   assert(sizeof(int)==4);   /* fix serialize_int   */

   set_swapflag();
   dx_define("bwrite",xbwrite);
   dx_define("bwrite-exact",xbwrite_exact);
   dx_define("bread",xbread);
   dx_define("bread-exact",xbread_exact);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
