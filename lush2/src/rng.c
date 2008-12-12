/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

/* moved here from arith.c */

#include "header.h"

/* --------- RANDOM FUNCTIONS --------- */

DX(xseed)
{
   ARG_NUMBER(1);
   Dseed((int)AREAL(1));
   return NIL;
}

DX(xrand)
{
   real lo, hi;
   if (arg_number == 0) {
      lo = 0.0;
      hi = 1.0;
   } else if (arg_number == 1) {
      hi = AREAL(1);
      lo = -hi;
   } else {
      ARG_NUMBER(2);
      lo = AREAL(1);
      hi = AREAL(2);
   }
   
   real rand = Drand();
   return NEW_NUMBER((hi - lo) * rand + lo);
}


DX(xgauss)
{
   real mean, sdev;
   if (arg_number == 0) {
      mean = 0.0;
      sdev = 1.0;
   } else if (arg_number == 1) {
      mean = 0.0;
      sdev = AREAL(1);
   } else {
      ARG_NUMBER(2);
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
