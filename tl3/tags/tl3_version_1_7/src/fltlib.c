/*   TL3 Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/***********************************************************************
	TL3: (C) LYB YLC 1988
	fltlib.c
	definition of numeric type  'flt'
	$Id: fltlib.c,v 1.1.1.1 2002-04-16 17:37:38 leonb Exp $
********************************************************************** */

#include "header.h"



flt 
Fsqr(flt x)
{
  return Fmul(x, x);
}


#ifndef HAVE_TANH

/* LIBRARY ROUTINES OFTEN USE MATH COPROCESSOR */

flt 
Ftanh(flt x)
{
  register flt y;

  y = Fexp(Fmul(x, Ftwo));
  return Fdiv(Fsub(y, Fone), Fadd(y, Fone));
}

flt 
Fcosh(flt x)
{
  register flt y;

  y = Fexp(x);
  y = Fadd(y, Fdiv(Fone, y));
  return Fmul(y, Fhalf);
}

flt 
Fsinh(flt x)
{
  register flt y;

  y = Fexp(x);
  y = Fsub(y, Fdiv(Fone, y));
  return Fmul(y, Fhalf);
}

#endif



/* Fseed(x):    gives a seed for random generators */
/* Frand: 	uniform random generator in [0,1]	 */
/* Fgauss:	normal random generator. mean 0, sdev 1 */


/*
 * references KNUTH D.E. 1981 'Seminumerical Algorithms' 2nd ed.,vol 2 'The
 * Art of Computer Programming' chapter 3.2, 3.3  (substractive method).
 * 
 * PRESS W.H & al. 1988 'Numerical Recipes in C' Chapter 7 (ran3) - Cambridge
 * University Press
 */

#define MMASK  0x7fffffffL
#define MSEED  161803398L	/* Any large MSEED < MBIG */
#define FAC    (1.0/(1.0+MMASK))


static int inext, inextp;
static int ma[56];		/* Should not be modified */

void 
Fseed(int x)
{
  int mj, mk;
  int i, ii;

  mj = MSEED - (x < 0 ? -x : x);
  mj &= MMASK;
  ma[55] = mj;
  mk = 1;
  for (i = 1; i <= 54; i++) {
    ii = (21 * i) % 55;
    ma[ii] = mk;
    mk = (mj - mk) & MMASK;
    mj = ma[ii];
  }
  for (ii = 1; ii <= 4; ii++)
    for (i = 1; i < 55; i++) {
      ma[i] -= ma[1 + (i + 30) % 55];
      ma[i] &= MMASK;
    }
  inext = 0;
  inextp = 31;			/* Special constant */
}

flt 
Frand(void)
{
  register int mj;

  if (++inext == 56)
    inext = 1;
  if (++inextp == 56)
    inextp = 1;
  mj = ((ma[inext] - ma[inextp]) * 84589 + 45989) & MMASK;
  ma[inext] = mj;
  return dtoF(mj * FAC);
}


flt 
Fgauss(void)
{
  /*
   * Now a quick and dirty way to build
   * a quasi-normal random number.
   */
#define FAC2   (1.0/0x01000000L)
  register int i;
  register int mj, sum;
  sum = 0;
  for (i = 12; i; i--) {
    if (++inext == 56)
      inext = 1;
    if (++inextp == 56)
      inextp = 1;
    mj = (ma[inext] - ma[inextp]) & MMASK;
    ma[inext] = mj;
    if (mj & 0x00800000L)
      mj |= 0xff000000L;
    else
      mj &= 0x00ffffffL;
    sum += mj;
  }
  ma[inext] = (mj * 84589 + 45989) & MMASK;
  return dtoF(sum * FAC2);
}





