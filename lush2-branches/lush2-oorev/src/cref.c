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
#include "dh.h"

typedef unsigned char   uchar;

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
      ifn (NUMBERP(val))                                                \
         error(NIL, "type mismatch in assignment", val);                \
      void *p = Gptr(self);                                             \
      *((type *)p) = Number(val);                                       \
   }                                                                    \
                                                                        \
   class_t *cref_##type##_class

DEF_NUMBER_CREF(char);
DEF_NUMBER_CREF(uchar);
DEF_NUMBER_CREF(short);
DEF_NUMBER_CREF(int);
DEF_NUMBER_CREF(float);
DEF_NUMBER_CREF(double);

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
   /* what to do when *dest==NULL ? */
   if (mm_ismanaged(*dest))
      *dest = String(val);
   else {
      fprintf(stderr, "*** Warning: possibly leaked memory at %p\n", *dest);
      *dest = strdup(String(val));
   }
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
   if (IND_STTYPE(*dest) != IND_STTYPE((index_t *)Mptr(val)))
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   *dest = Mptr(val);
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
   if (IND_STTYPE(*dest) != IND_STTYPE((index_t *)Mptr(val)))
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   if (IND_NDIMS(*dest) != IND_NDIMS((index_t *)Mptr(val)))
      error(NIL, "type mismatch in assignment (wrong rank)", val);
   *dest = Mptr(val);
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
   if ((*dest)->type != ((storage_t *)Mptr(val))->type)
      error(NIL, "type mismatch in assignment (wrong storage type)", val);
   *dest = Mptr(val);
}

class_t *cref_storage_class;



at *new_cref(int dht, void *p) 
{
   switch (dht) {
   case DHT_CHAR  : return new_at(cref_char_class, p);
   case DHT_UCHAR : return new_at(cref_uchar_class, p);
   case DHT_SHORT : return new_at(cref_short_class, p);
   case DHT_INT   : return new_at(cref_int_class, p);
   case DHT_FLOAT : return new_at(cref_float_class, p);
   case DHT_DOUBLE: return new_at(cref_double_class, p);
   case DHT_STR   : return new_at(cref_str_class, p);
   case DHT_INDEX :
   {
      void **dest = p;
      ifn (mm_ismanaged(*dest))
         error(NIL, "storage does not hold managed address", p);
      return new_at(cref_index_class, p);
   }

   case DHT_IDX   :
   {
      void **dest = p;
      ifn (mm_ismanaged(*dest))
         error(NIL, "storage does not hold managed address", p);
      return new_at(cref_idx_class, p);
   }
   case DHT_STORAGE:
   {
      void **dest = p;
      ifn (mm_ismanaged(*dest))
         error(NIL, "storage does not hold managed address", p);
      return new_at(cref_storage_class, p);
   }
   default:
      error(NIL, "unsupported type", NEW_NUMBER(dht));
   }
}

DX(xnew_cref)
{
   ARG_NUMBER(2);
   return new_cref(dht_from_cname(ASYMBOL(1)), AGPTR(2));
}

DX(xassign)
{
   ARG_NUMBER(2);
   ifn (CREFP(APOINTER(1)))
      RAISEFX("not a cref", APOINTER(1));

   class_t *cl = classof(APOINTER(1));
   cl->setslot(APOINTER(1), NIL, APOINTER(2));
   return APOINTER(2);
}


/* defined in module.c */
struct module;
extern void *dynlink_symbol(struct module *, const char *, int, int);

DX(xto_gptr)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   
   if (p==NIL)
      return NIL;
   
   else if (CREFP(p)) {
      return NEW_GPTR(Gptr(p));
      
   } else if (NUMBERP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (GPTRP(p)) {
      //LOCK(p);
      return p;
      
   } else if (MPTRP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (INDEXP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (OBJECTP(p)) {
      error(NIL, "not supported", NIL);
      //return NEW_GPTR(n->citem);
      
   } else if (STORAGEP(p)) {
      return NEW_GPTR(Mptr(p));
      
   } else if (p && (Class(p) == dh_class)) {
      struct cfunction *cfunc = Mptr(p);
      if (CONSP(cfunc->name))
         check_primitive(cfunc->name, cfunc->info);
      
      assert(MODULEP(Car(cfunc->name)));
      struct module *m = Mptr(Car(cfunc->name));
      dhdoc_t *dhdoc;
      if (( dhdoc = (dhdoc_t*)(cfunc->info) )) {
         void *q = dynlink_symbol(m, dhdoc->lispdata.c_name, 1, 1);
         ifn (q)
            RAISEF("could not find function pointer\n", p);
         return NEW_GPTR(q);
      }
   }
   error(NIL,"Cannot make a compiled version of this lisp object",p);
}


class_t *abstract_cref_class;

#define INIT_NUMBER_CREF(type)                                    \
   cref_##type##_class = new_builtin_class(abstract_cref_class);  \
   cref_##type##_class->selfeval = cref_##type##_selfeval;        \
   cref_##type##_class->setslot = cref_##type##_setslot;          \
   cref_##type##_class->managed = false;                          \
   class_define("cref<"#type">", cref_##type##_class);

void init_cref(void)
{
   abstract_cref_class = new_builtin_class(NIL);

   INIT_NUMBER_CREF(char);
   INIT_NUMBER_CREF(uchar);
   INIT_NUMBER_CREF(short);
   INIT_NUMBER_CREF(int);
   INIT_NUMBER_CREF(float);
   INIT_NUMBER_CREF(double);

   cref_str_class = new_builtin_class(abstract_cref_class);
   cref_str_class->selfeval = cref_str_selfeval;
   cref_str_class->setslot = cref_str_setslot;
   cref_str_class->managed = false;
   class_define("cref<str>", cref_str_class);

   cref_index_class = new_builtin_class(abstract_cref_class);
   cref_index_class->selfeval = cref_index_selfeval;
   cref_index_class->setslot = cref_index_setslot;
   class_define("cref<index>", cref_index_class);

   cref_idx_class = new_builtin_class(abstract_cref_class);
   cref_idx_class->selfeval = cref_idx_selfeval;
   cref_idx_class->setslot = cref_idx_setslot;
   class_define("cref<idx>", cref_idx_class);

   cref_storage_class = new_builtin_class(abstract_cref_class);
   cref_storage_class->selfeval = cref_storage_selfeval;
   cref_storage_class->setslot = cref_storage_setslot;
   class_define("cref<storage>", cref_storage_class);

   dx_define("new-cref", xnew_cref);
   dx_define("to-gptr", xto_gptr);
   dx_define("assign", xassign);
}
   

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
