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

/* moved here from arith.c */

#include "header.h"

/* --------- RANDOM FUNCTIONS --------- */

DX(xseed)
{
   ARG_NUMBER(1);
   Dseed((int)AREAL(1));
   return NIL;
}

static index_t *array_rand2(index_t *_lo, index_t *_hi)
{
   index_t *lo = NULL;
   index_t *hi = NULL;
   shape_t *shp = index_broadcast2(_lo, _hi, &lo, &hi);

   index_t *res = make_array(ST_DOUBLE, shp, NIL);
   double *lop = IND_BASE_TYPED(lo, double);
   double *hip = IND_BASE_TYPED(hi, double);
   double *resp = IND_BASE_TYPED(res, double);
   begin_idx_aloop3(hi, lo, res, j, k, l) {
      double r = Drand();
      resp[l] = (hip[j] - lop[k])*r + lop[k];
   } end_idx_aloop3(hi, lo, res, j, k, l);

   return res;
}

static index_t *array_rand1(index_t *ind)
{
   index_t *res = clone_array(ind);
   double *argp = IND_BASE_TYPED(ind, double);
   double *resp = IND_BASE_TYPED(res, double);
   begin_idx_aloop2(ind, res, j, k) {
      double r = Drand();
      resp[k] = 2*argp[j]*r - argp[j];
   } end_idx_aloop2(ind, res, j, k);

   return res;
}

DX(xrand)
{
   double lo, hi;
   if (arg_number == 0) {
      lo = 0.0;
      hi = 1.0;
   } else if (arg_number == 1) {
      if (ISINDEX(1)) 
         return array_rand1(as_double_array(APOINTER(1)))->backptr;
      hi = AREAL(1);
      lo = -hi;
   } else {
      ARG_NUMBER(2);
      if (ISINDEX(1) || ISINDEX(2)) {
         index_t *ind1 = as_double_array(APOINTER(1));
         index_t *ind2 = as_double_array(APOINTER(2));
         return array_rand2(ind1, ind2)->backptr;
      }
      lo = AREAL(1);
      hi = AREAL(2);
   }
   real rand = Drand();
   return NEW_NUMBER((hi - lo) * rand + lo);
}


static index_t *array_gauss2(index_t *_mean, index_t *_std)
{
   index_t *mean = NULL;
   index_t *std = NULL;
   shape_t *shp = index_broadcast2(_mean, _std, &mean, &std);

   index_t *res = make_array(ST_DOUBLE, shp, NIL);
   double *meanp = IND_BASE_TYPED(mean, double);
   double *stdp = IND_BASE_TYPED(std, double);
   double *resp = IND_BASE_TYPED(res, double);
   begin_idx_aloop3(mean, std, res, j, k, l) {
      double r = Dgauss();
      resp[l] = stdp[k]*r + meanp[j];
   } end_idx_aloop3(mean, std, res, j, k, l);

   return res;
}

static index_t *array_gauss1(index_t *ind)
{
   index_t *res = clone_array(ind);
   double *argp = IND_BASE_TYPED(ind, double);
   double *resp = IND_BASE_TYPED(res, double);
   begin_idx_aloop2(ind, res, j, k) {
      resp[k] = argp[j]*Dgauss();
   } end_idx_aloop2(ind, res, j, k);

   return res;
}

DX(xgauss)
{
   real mean, sdev;
   if (arg_number == 0) {
      mean = 0.0;
      sdev = 1.0;
   } else if (arg_number == 1) {
      mean = 0.0;
      if (ISINDEX(1))
         return array_gauss1(as_double_array(APOINTER(1)))->backptr;
      sdev = AREAL(1);
   } else {
      ARG_NUMBER(2);
      if (ISINDEX(1) || ISINDEX(2)) {
         index_t *ind1 = as_double_array(APOINTER(1));
         index_t *ind2 = as_double_array(APOINTER(2));
         return array_gauss2(ind1, ind2)->backptr;
      }
      mean = AREAL(1);
      sdev = AREAL(2);
   }
   
   real rand = Dgauss();
   return NEW_NUMBER(sdev * rand + mean);
}

void init_lushrng(void) /* to avoid name clash with GSL's 'init_rng' */
{
   dx_define("seed", xseed);
   dx_define("rand", xrand);
   dx_define("gauss", xgauss);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
