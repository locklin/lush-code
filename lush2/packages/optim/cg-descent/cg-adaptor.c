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

double cg_value_adaptor(double *x, int n)
{
   static at *call = NIL;
   static int nx = -1;
   static storage_t *st = NULL;
   static at *(*listeval)(at *, at *) = NULL;

   if (n == -1) {
      /* initialize */
      at *x0 = var_get(named("x0"));
      at *vargs = var_get(named("vargs"));
      at *f = var_get(named("f"));
      ifn (x0)
         error(NIL, "x0 not found", NIL);
      ifn (INDEXP(x0) && IND_STTYPE((index_t *)Mptr(x0)))
         error(NIL, "x0 not a double index", x0);
      ifn (f)
         error(NIL, "f not found", NIL);

      listeval = Class(f)->listeval;
      index_t *ind = Mptr(x0);
      nx = storage_nelems(IND_ST(ind));
      st = new_storage(ST_DOUBLE);
      st->kind = STS_FOREIGN;
      st->size = nx;
      st->data = (char *)-1;

      call = new_cons(f, new_cons(NEW_INDEX(st, IND_SHAPE(ind)), vargs));
      return NAN;
         
   } else {
      if (n != nx)
         error(NIL, "vector of different size expected", NEW_NUMBER(n));
      st->data = x;
      return Number(listeval(Car(call), call));
   }
}

void cg_grad_adaptor(double *g, double *x, int n)
{
   static at *call = NIL;
   static int nx = -1;
   static storage_t *stx = NULL;
   static storage_t *stg = NULL;
   static at *(*listeval)(at *, at *) = NULL;

   if (n == -1) {
      /* initialize */
      at *x0 = var_get(named("x0"));
      at *vargs = var_get(named("vargs"));
      at *g = var_get(named("g"));
      ifn (x0)
         error(NIL, "x0 not found", NIL);
      ifn (INDEXP(x0) && IND_STTYPE((index_t *)Mptr(x0)))
         error(NIL, "x0 not a double index", x0);
      ifn (g)
         error(NIL, "g not found", NIL);
      
      listeval = Class(g)->listeval;
      index_t *ind = Mptr(x0);
      nx = storage_nelems(IND_ST(ind));
      stx = new_storage(ST_DOUBLE);
      stx->kind = STS_FOREIGN;
      stx->size = nx;
      stx->data = (char *)-1;

      stg = new_storage(ST_DOUBLE);
      stg->kind = STS_FOREIGN;
      stg->size = nx;
      stg->data = (char *)-1;
      call = new_cons(g, 
                      new_cons(NEW_INDEX(stg, IND_SHAPE(ind)),
                               new_cons(NEW_INDEX(stx, IND_SHAPE(ind)),
                                        vargs)));
   } else {
      if (n != nx)
         error(NIL, "vector of different size expected", NEW_NUMBER(n));
      stx->data = x;
      stg->data = g;
      listeval(Car(call), call);
   }
}

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
