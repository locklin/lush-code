/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann LeCun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as 
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *  MA  02110-1301  USA
 *
 ***********************************************************************/

#include "header.h"


static at *cref_bool_selfeval(at *p)
{
   return NEW_BOOL(*(bool *)Gptr(p));
}

static void cref_bool_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);

   bool *dest = Gptr(self);
   
   if (val == NIL)
      *dest = false;
   else if (val == at_t)
      *dest = true;
   else
      error(NIL, "type mismatch in assignment", val);
}

class_t *cref_bool_class;


#define DEF_NUMBER_CREF(type)                     \
   static at *cref_##type##_selfeval(at *p)       \
   {                                              \
      return NEW_NUMBER(*((type *)Gptr(p)));      \
   }                                              \
                                                  \
   static void cref_##type##_setslot(at *self, at *slot, at *val)       \
   {                                                                    \
      if (slot)                                                         \
         error(NIL, "object does not accept scope syntax", self);       \
      ifn (NUMBERP(val) && (Number(val) == (type)(Number(val))))        \
         error(NIL, "type mismatch in assignment (not a " #type ")", val); \
      void *p = Gptr(self);                                             \
      *((type *)p) = (type)Number(val);                                 \
   }                                                                    \
                                                                        \
   class_t *cref_##type##_class

DEF_NUMBER_CREF(char);
DEF_NUMBER_CREF(uchar);
DEF_NUMBER_CREF(short);
DEF_NUMBER_CREF(int);
DEF_NUMBER_CREF(float);
DEF_NUMBER_CREF(double);


/* pointer to managed string */

static at *cref_str_selfeval(at *p)
{
   return NEW_STRING(mm_strdup(*((const char **)Gptr(p))));
}

static void cref_str_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (STRINGP(val))
      error(NIL, "type mismatch in assignment", val);

   const char **dest = Gptr(self);
   *dest = String(val);
}

class_t *cref_str_class;


static at *cref_index_selfeval(at *p)
{
   return (*((index_t **)Gptr(p)))->backptr;
}

static void cref_index_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (INDEXP(val))
      error(NIL, "type mismatch in assignment", val);
   
   index_t **dest = Gptr(self);
   if (IND_STTYPE(*dest) != IND_STTYPE((index_t *)Gptr(val)))
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   *dest = Gptr(val);
}

class_t *cref_index_class;


/* this one is for indexes of fixed rank  */

static at *cref_idx_selfeval(at *p)
{
   return (*((index_t **)Gptr(p)))->backptr;
}

static void cref_idx_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (INDEXP(val))
      error(NIL, "type mismatch in assignment", val);
   
   index_t **dest = Gptr(self);
   if (*dest && (IND_STTYPE(*dest) != IND_STTYPE((index_t *)Gptr(val))))
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   if (*dest && (IND_NDIMS(*dest) != IND_NDIMS((index_t *)Gptr(val))))
      error(NIL, "type mismatch in assignment (wrong rank)", val);
   *dest = Gptr(val);
}

class_t *cref_idx_class;



static at *cref_storage_selfeval(at *p)
{
   return (*((storage_t **)Gptr(p)))->backptr;
}

static void cref_storage_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (STORAGEP(val))
      error(NIL, "type mismatch in assignment", val);
   
   storage_t **dest = Gptr(self);
   if (*dest && ((*dest)->type != ((storage_t *)Gptr(val))->type))
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   *dest = Gptr(val);
}

class_t *cref_storage_class;


static at *cref_gptr_selfeval(at *p)
{
   return NEW_GPTR(*(gptr *)Gptr(p));
}

static void cref_gptr_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (GPTRP(val))
      error(NIL, "type mismatch in assignment", val);

   void **dest = Gptr(self);
   *dest = Gptr(val);
}

class_t *cref_gptr_class;


static at *cref_mptr_selfeval(at *p)
{
   return NEW_MPTR(*(gptr *)Gptr(p));
}

static void cref_mptr_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   ifn (MPTRP(val))
      error(NIL, "type mismatch in assignment", val);

   void **dest = Gptr(self);
   *dest = Mptr(val);
}

class_t *cref_mptr_class;


static at *cref_object_selfeval(at *p)
{
   struct CClass_object *cobj = *((struct CClass_object **)Gptr(p));
   return object_from_cobject(cobj)->backptr;
}

static void cref_object_setslot(at *self, at *slot, at *val)
{
   if (slot)
      error(NIL, "object does not accept scope syntax", self);
   
   /* this is not what we want, but we don't know the type */
   void **dest = Gptr(self);
   if (*dest){
      at *_self = cref_object_selfeval(self);
      ifn (isa(val, Class(_self)))
         error(NIL, "type mismatch in assignment", val);
   }
   struct CClass_object *cobj = ((object_t *)Mptr(val))->cptr;
   ifn (cobj)
      error(NIL, "object has no compiled part", val);

   *dest = (void *)cobj;
}

class_t *cref_object_class;


at *new_cref(int dht, void *p) 
{
   const char *errmsg_nonmanaged = "storage does not hold a managed address";

   if (!p)
      RAISEF("not an address", p);
   
   class_t *cl = NULL;
   
   switch (dht) {
   case DHT_BOOL  : return new_at(cref_bool_class, p);
   case DHT_CHAR  : return new_at(cref_char_class, p);
   case DHT_UCHAR : return new_at(cref_uchar_class, p);
   case DHT_SHORT : return new_at(cref_short_class, p);
   case DHT_INT   : return new_at(cref_int_class, p);
   case DHT_FLOAT : return new_at(cref_float_class, p);
   case DHT_DOUBLE: return new_at(cref_double_class, p);
   case DHT_GPTR  : return new_at(cref_gptr_class, p);
      
   case DHT_MPTR  : cl = cref_mptr_class; break;
   case DHT_STR   : cl = cref_str_class; break;
   case DHT_INDEX : cl = cref_index_class; break;
   case DHT_IDX   : cl = cref_idx_class; break;
   case DHT_STORAGE: cl = cref_storage_class; break;
   case DHT_OBJ   : cl = cref_object_class; break;
   default:
      RAISEF("unsupported type", NEW_NUMBER(dht));
   }
   if (cl) {
      /*
       * NULL must be allowed as crefs are created during object
       * construction, before the class constructor is called
       */
      void **dest = p;
      ifn (*dest==NULL || mm_ismanaged(*dest))
         error(NIL, errmsg_nonmanaged, p);
      return new_at(cl, p);
   } else
      abort();
}

DX(xnew_cref)
{
   ARG_NUMBER(2);
   return new_cref(dht_from_cname(ASYMBOL(1)), AGPTR(2));
}

at *assign(at *p, at *q)
{
   ifn (CREFP(p))
      RAISEF("not a cref", p);

   class_t *cl = classof(p);
   cl->setslot(p, NIL, q);
   return q;
} 

DX(xassign)
{
   ARG_NUMBER(2);
   return assign(APOINTER(1), APOINTER(2));
}


/*
 *  Casting functions
 */

/* defined in module.c */
struct module;
extern void *dynlink_symbol(struct module *, const char *, int, int);

DX(xto_gptr)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   
   if (!p || p==at_NULL)
      return NEW_GPTR(Mptr(at_NULL));
   
   else if (CREFP(p)) {
      void *ptr = Gptr(p);
      class_t *cl = Class(p);
      if (cl==cref_bool_class   ||
          cl==cref_char_class   ||
          cl==cref_uchar_class  ||
          cl==cref_short_class  ||
          cl==cref_int_class    ||
          cl==cref_float_class  ||
          cl==cref_double_class )  return NEW_GPTR(ptr);
      
      void **dest = (void **)ptr;
      if (cl==cref_gptr_class   ||
          cl==cref_mptr_class   ||
          cl==cref_str_class    ||
          cl==cref_index_class  ||
          cl==cref_idx_class    ||
          cl==cref_storage_class||
          cl==cref_object_class )  return NEW_GPTR(*dest);
      
   } else if (NUMBERP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (GPTRP(p)) {
      return p;
      
   } else if (MPTRP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (INDEXP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (OBJECTP(p)) {
      object_t *obj = Mptr(p);
      if (obj->cptr)
         return NEW_GPTR(obj->cptr);

   } else if (STORAGEP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (STRINGP(p)) {
      char *cp = (char *)String(p);
      return NEW_GPTR((void *)cp);

   } else if (p && (Class(p) == dh_class)) {
      struct cfunction *cfunc = Gptr(p);
      if (CONSP(cfunc->name))
         check_primitive(cfunc->name, cfunc->info);
      
      assert(MODULEP(Car(cfunc->name)));
      struct module *m = Gptr(Car(cfunc->name));
      dhdoc_t *dhdoc;
      if (( dhdoc = (dhdoc_t*)(cfunc->info) )) {
         void *q = dynlink_symbol(m, dhdoc->lispdata.c_name, 1, 1);
         ifn (q)
            RAISEF("could not find function pointer", p);
         return NEW_GPTR(q);
      }
   }
   error(NIL, "cannot cast to C object", p);
}

DX(xto_mptr)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   
   if (!p || p==at_NULL) {
      return at_NULL;
   
   } else if (CREFP(p)) {
      void *ptr = Gptr(p);
      class_t *cl = Class(p);
      if (cl==cref_bool_class   ||
          cl==cref_char_class   ||
          cl==cref_uchar_class  ||
          cl==cref_short_class  ||
          cl==cref_int_class    ||
          cl==cref_float_class  ||
          cl==cref_double_class ) {
         if (mm_ismanaged(ptr))
            return NEW_MPTR(ptr);
      }
      void **dest = (void **)ptr;
      if (cl==cref_gptr_class) {
         if (mm_ismanaged(*dest))
            return NEW_MPTR(*dest);
      }
      if (cl==cref_mptr_class   ||
          cl==cref_str_class    ||
          cl==cref_index_class  ||
          cl==cref_idx_class    ||
          cl==cref_storage_class||
          cl==cref_object_class )
         return NEW_MPTR(*dest);
      
      RAISEF("not a managed address", NEW_GPTR(p));

   } else if (GPTRP(p)) {
      if (mm_ismanaged(Gptr(p)))
         return NEW_MPTR(Mptr(p));
      else
         RAISEF("not a managed address", NEW_GPTR(Gptr(p)));
      
   } else if (NUMBERP(p)) {
      return NEW_MPTR(Mptr(p));
      
   } else if (MPTRP(p)) {
      return p;
      
   } else if (INDEXP(p)) {
      return NEW_MPTR(Mptr(p));
      
   } else if (OBJECTP(p)) {
      object_t *obj = Mptr(p);
      if (obj->cptr)
         return NEW_MPTR(obj->cptr);
      
   } else if (STORAGEP(p)) {
      return NEW_MPTR(Mptr(p));
      
   } else if (STRINGP(p)) {
      char *cp = (char *)String(p);
      return NEW_MPTR((void *)cp);

   } else if (p && (Class(p) == dh_class)) {
      RAISEF("DH-functions are not in managed memory", p);
   }
   error(NIL, "cannot cast to C object", p);
}

DX(xto_int)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(AINTEGER(1));
}

DX(xto_char)
{
   ARG_NUMBER(1);
   return NEW_NUMBER((char)AINTEGER(1));
}

DX(xto_uchar)
{
   ARG_NUMBER(1);
   return NEW_NUMBER((uchar)AINTEGER(1));
}

DX(xto_float)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(Ftor(AFLOAT(1)));
}

DX(xto_double)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(ADOUBLE(1));
}

DX(xto_bool)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   if (!p)
      return NIL;
   /* C semantics */
   if (NUMBERP(p) && Number(p)==0)
      return NIL;
   return t();
}

DX(xto_obj)
{
   at *p = NIL;
   class_t *cl = NULL;
   
   switch (arg_number) {
   case 1:
      p = APOINTER(1);
      break;

   case 2:
      p = APOINTER(1);
      ifn (CLASSP(p))
         error(NIL,"not a class", p);
      cl = Mptr(p);
      p = APOINTER(2);
      break;

   default:
      ARG_NUMBER(-1);
   }

   if (!p) {
      return p;

   }  else if (OBJECTP(p)) {
      /* nothing to do */

   } else if (GPTRP(p) || MPTRP(p)) {
      struct CClass_object *obj = Gptr(p);
      ifn (obj && mm_ismanaged(obj))
         error(NIL, "not pointer to an object", p);
      p = object_from_cobject(obj)->backptr;

   } else
      error(NIL, "not a GPTR or object", p);
   
   /* check class */
   if (cl) {
      const class_t *clm = Class(p);
      while (clm && (clm != cl))
         clm = clm->super;
      if (clm != cl)
         error(NIL, "GPTR does not point to an object of this class", cl->backptr);
   }
   return p;
}

DX(xto_index)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (GPTRP(p) || MPTRP(p))
      RAISEFX("not a pointer", p);

   index_t * ind = Mptr(p);
   ifn (mm_ismanaged(ind) || !mm_ismanaged(ind->backptr))
      RAISEFX("not pointer to a lush object", p);
   
   p = ind->backptr;
   ifn (isa(p, index_class))
      RAISEFX("not pointer to an index", APOINTER(1));

   return p;
}

DX(xto_str)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
  
   if (STRINGP(p)) {
      return p;

   } else if (GPTRP(p) || MPTRP(p)) {
      const char *s = String(p);
      if (s && mm_ismanaged(s)) // && mm_typeof(s)==mt_blob)
         return NEW_STRING(s);
      else
         RAISEF("not a managed address", p);
   } else
      RAISEF("not a pointer or string", p);

   return NIL;
}

class_t *cref_class;

#define INIT_CREF(type)                                           \
   cref_##type##_class = new_builtin_class(cref_class);           \
   cref_##type##_class->selfeval = cref_##type##_selfeval;        \
   cref_##type##_class->setslot = cref_##type##_setslot;          \
   cref_##type##_class->managed = false;                          \
   class_define("cref<"#type">", cref_##type##_class);

void init_cref(void)
{
   cref_class = new_builtin_class(NIL);

   INIT_CREF(bool);
   INIT_CREF(char);
   INIT_CREF(uchar);
   INIT_CREF(short);
   INIT_CREF(int);
   INIT_CREF(float);
   INIT_CREF(double);
   INIT_CREF(str);
   INIT_CREF(index);
   INIT_CREF(idx);
   INIT_CREF(storage);
   INIT_CREF(gptr);
   INIT_CREF(mptr);
   INIT_CREF(object);

   dx_define("new-cref", xnew_cref);
   dx_define("assign", xassign);

   dx_define("to-bool", xto_bool);
   dx_define("to-char", xto_char);
   dx_define("to-uchar", xto_uchar);
   dx_define("to-int", xto_int);
   dx_define("to-float", xto_float);
   dx_define("to-double", xto_double);
   dx_define("to-number", xto_double);
   dx_define("to-str", xto_str);
   dx_define("to-gptr", xto_gptr);
   dx_define("to-mptr", xto_mptr);
   dx_define("to-obj", xto_obj);
   dx_define("to-index", xto_index);
}
   

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
