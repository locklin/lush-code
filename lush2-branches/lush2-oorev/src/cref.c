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

struct cref {
   void *mobj;           /* non-NULL when cobj points into managed object */
   void *cobj;           /* reference to C object                         */
};

typedef struct cref cref_t;

static void clear_cref(cref_t *cr, size_t _)
{
   cr->mobj = NULL;
}

static void mark_cref(cref_t *cr, size_t _)
{
   MM_MARK(cr->mobj);
}

mt_t mt_cref = mt_undefined;


/* DHT_DOUBLE */

static at *cref_double_selfeval(at *p)
{
   cref_t *cr = Mptr(p);
   return NEW_NUMBER(*((double *)cr->cobj));
}

static void cref_double_setslot(at *p, at *slot, at *val)
{
   cref_t *cr = Mptr(p);
   if (slot)
      error(NIL, "object does not accept scope syntax", p);
   ifn (NUMBERP(val))
      error(NIL, "invalid value for assignment", val);
   *((double *)cr->cobj) = Number(val);
}

class_t *cref_double_class;




at *new_cref(int dht, void *p) 
{
   cref_t *cr = mm_alloc(mt_cref);
   cr->cobj = p;

   switch (dht) {
   case DHT_REAL: return new_at(cref_double_class, cr);

   default:
      error(NIL, "unsupported type", NEW_NUMBER(dht));
   }
}



void init_cref(void)
{
   mt_cref = MM_REGTYPE("cref", sizeof(cref_t), clear_cref, mark_cref, 0);
   
   cref_double_class = new_builtin_class(NIL);
   cref_double_class->selfeval = cref_double_selfeval;
   cref_double_class->setslot = cref_double_setslot;
   class_define("cref<double>", cref_double_class);

}
   

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
