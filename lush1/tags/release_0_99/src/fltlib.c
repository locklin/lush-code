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

/***********************************************************************
 * $Id: fltlib.c,v 1.5 2003-10-17 18:06:24 leonb Exp $
 **********************************************************************/

#include "header.h"

/* 
 * FQtanh(x):   A very fast but inaccurate TANH 
 * FQDtanh(x):  A very fast but inaccurate derivative of TANH
 *
 * Formula:               d
 *                    P(x)  - 1 
 * for x>0, TH(x) =  -----------
 *                        d
 *                    P(x)  + 1
 *
 *                    n        i
 *                   ---   (2x)
 * with      P(x) =  >    ------
 *                   ---    i
 *                   i=0   d  i!
 *
 *
 * Interesting values of d and n:
 *   5 mult/adds:  n=3 , d=4,   epsilon=0.00287964,     maxtemp=2.39355e+06
 *   6 mult/adds:  n=3 , d=8,   epsilon=0.000501412,    maxtemp=5.26381e+07
 *** 7 mult/adds:  n=3 , d=16,  epsilon=7.5464e-05,     maxtemp=2.59865e+08
 *   8 mult/adds:  n=4 , d=16,  epsilon=4.08997e-06,    maxtemp=4.18983e+08
 *   9 mult/adds:  n=5 , d=16,  epsilon=2.22735e-07,    maxtemp=4.71092e+08
 */


#define A0   ((flt)1.0)
#define A1   ((flt)0.125)
#define A2   ((flt)0.0078125)
#define A3   ((flt)0.000325520833333)

flt
FQtanh(flt x)
{
  register flt y;

  if (x >= Fzero) 
    if (x < (flt)13) 
      y = A0+x*(A1+x*(A2+x*(A3)));
    else 
      return Fone;
  else
    if (x > -(flt)13) 
      y = A0-x*(A1-x*(A2-x*(A3)));
    else 
      return Fminus;
  
  y *= y;
  y *= y;
  y *= y;
  y *= y;
  return (y-Fone)/(y+Fone);
} 

flt
FQDtanh(flt x)
{
  if (x < Fzero)
    x = -x;
  if (x < (flt)13) 
    {
      register flt y;
      y = A0+x*(A1+x*(A2+x*(A3)));
      y *= y;
      y *= y;
      y *= y;
      y *= y;
      y = (y-Fone)/(y+Fone);
      return Fone-y*y;
    }
  else
    return Fzero;
}

#undef A0
#undef A1
#undef A2
#undef A3



#define A0   ((real)1.0)
#define A1   ((real)0.125)
#define A2   ((real)0.0078125)
#define A3   ((real)0.000325520833333)

real
DQtanh(real x)
{
  register real y;

  if (x >= Dzero) 
    if (x < (real)13) 
      y = A0+x*(A1+x*(A2+x*(A3)));
    else 
      return Done;
  else
    if (x > -(real)13) 
      y = A0-x*(A1-x*(A2-x*(A3)));
    else 
      return Dminus;
  
  y *= y;
  y *= y;
  y *= y;
  y *= y;
  return (y-Done)/(y+Done);
} 

real
DQDtanh(real x)
{
  if (x < Dzero)
    x = -x;
  if (x < (real)13) 
    {
      register real y;
      y = A0+x*(A1+x*(A2+x*(A3)));
      y *= y;
      y *= y;
      y *= y;
      y *= y;
      y = (y-Done)/(y+Done);
      return Done-y*y;
    }
  else
    return Dzero;
}

#undef A0
#undef A1
#undef A2
#undef A3


/* 
 * FQstdsigmoid(x)
 * FQDstdsigmoid(x)
 * The same formulas for computing 1.71593428 tanh (0.66666666 x)
 */

#define PR  ((flt)0.66666666)
#define PO  ((flt)1.71593428)
#define A0   ((flt)(1.0))
#define A1   ((flt)(0.125*PR))
#define A2   ((flt)(0.0078125*PR*PR))
#define A3   ((flt)(0.000325520833333*PR*PR*PR))

flt
FQstdsigmoid(flt x)
{
  register flt y;

  if (x >= Fzero) 
    if (x < (flt)13) 
      y = A0+x*(A1+x*(A2+x*(A3)));
    else 
      return PO;
  else
    if (x > -(flt)13) 
      y = A0-x*(A1-x*(A2-x*(A3)));
    else 
      return -PO;
  
  y *= y;
  y *= y;
  y *= y;
  y *= y;
  return (x > Fzero) ? PO*(y-Fone)/(y+Fone) : PO*(Fone-y)/(y+Fone);
}


flt
FQDstdsigmoid(flt x)
{
  if (x < Fzero)
    x = -x;
  if (x < (flt)13) 
    {
      register flt y;
      y = A0+x*(A1+x*(A2+x*(A3)));
      y *= y;
      y *= y;
      y *= y;
      y *= y;
      y = (y-Fone)/(y+Fone);
      return PR*PO - PR*PO*y*y;
    }
  else
    return Fzero;
}

#undef PR
#undef PO
#undef A0
#undef A1
#undef A2
#undef A3


/* 
 * DQstdsigmoid(x)
 * DQDstdsigmoid(x)
 * The same formulas for computing 1.71593428 tanh (0.66666666 x)
 */

#define PR  ((real)0.66666666)
#define PO  ((real)1.71593428)
#define A0   ((real)(1.0))
#define A1   ((real)(0.125*PR))
#define A2   ((real)(0.0078125*PR*PR))
#define A3   ((real)(0.000325520833333*PR*PR*PR))

real
DQstdsigmoid(real x)
{
  register real y;

  if (x >= Dzero) 
    if (x < (real)13) 
      y = A0+x*(A1+x*(A2+x*(A3)));
    else 
      return PO;
  else
    if (x > -(real)13) 
      y = A0-x*(A1-x*(A2-x*(A3)));
    else 
      return -PO;
  
  y *= y;
  y *= y;
  y *= y;
  y *= y;
  return (x > Dzero) ? PO*(y-Done)/(y+Done) : PO*(Done-y)/(y+Done);
}


real
DQDstdsigmoid(real x)
{
  if (x < Dzero)
    x = -x;
  if (x < (real)13) 
    {
      register real y;
      y = A0+x*(A1+x*(A2+x*(A3)));
      y *= y;
      y *= y;
      y *= y;
      y *= y;
      y = (y-Done)/(y+Done);
      return PR*PO - PR*PO*y*y;
    }
  else
    return Dzero;
}

#undef PR
#undef PO
#undef A0
#undef A1
#undef A2
#undef A3




/* 
 * FQexpmx(x):   A faster but inaccurate EXP(-|X|)
 * FQDexpmx(x):  Its derivative...
 * FQexpmx2(x):   A faster but inaccurate EXP(-X^2)
 * FQDexpmx2(x): Its derivative...
 *
 * Formula:              
 *                        1 
 * for x>0, EXP(-x) =  -------
 *                          d
 *                      P(x)
 *
 *                    n       i
 *                   ---   (x)
 * with      P(x) =  >    ------
 *                   ---    i
 *                   i=0   d  i!
 *
 *FOR EXPMX
 * (05 multiply/adds) n= 2, d= 8, diff=0.00274766
 * (06 multiply/adds) n= 3, d= 8, diff=0.000262787
 **(07 multiply/adds) n= 4, d= 8, diff=2.62182e-05
 * (08 multiply/adds) n= 4, d=16, diff=2.07879e-06
 * (09 multiply/adds) n= 5, d=16, diff=1.12114e-07
 *
 *FOR EXPMX2
 * (05 multiply/adds) n= 3, d= 4, diff=0.00152103
 * (06 multiply/adds) n= 3, d= 8, diff=0.000262747
 **(07 multiply/adds) n= 4, d= 8, diff=2.61688e-05
 * (08 multiply/adds) n= 4, d=16, diff=2.07761e-06
 * (09 multiply/adds) n= 5, d=16, diff=1.12073e-07
 */

#define A0   ((flt)1.0)
#define A1   ((flt)0.125)
#define A2   ((flt)0.0078125)
#define A3   ((flt)0.00032552083)
#define A4   ((flt)1.0172526e-5) 

flt
FQexpmx(flt x)
{
  x = Fabs(x);
  if (x < (flt)13) 
    {
      register flt y;
      y = A0+x*(A1+x*(A2+x*(A3+x*A4)));
      y *= y;
      y *= y;
      y *= y;
      y = 1/y;
      return y;
    }
  else
    return Fzero;
}

flt
FQDexpmx(flt x)
{
  register flt y;
  y = Fabs(x);
  if (y < (flt)13) 
    {
      y = A0+y*(A1+y*(A2+y*(A3+y*A4)));
      y *= y;
      y *= y;
      y *= y;
      y = 1/y;
      return (x<Fzero)?(-y):(y);
    }
  else
    return Fzero;
}

flt
FQexpmx2(flt x)
{
  return FQexpmx(x*x);
}

flt
FQDexpmx2(flt x)
{
  return 2*x*FQexpmx(x*x);
}

#undef A0
#undef A1
#undef A2
#undef A3
#undef A4

/* real/double version */
 
#define A0   ((real)1.0)
#define A1   ((real)0.125)
#define A2   ((real)0.0078125)
#define A3   ((real)0.00032552083)
#define A4   ((real)1.0172526e-5) 

real
DQexpmx(real x)
{
  x = Dabs(x);
  if (x < (real)13) 
    {
      register real y;
      y = A0+x*(A1+x*(A2+x*(A3+x*A4)));
      y *= y;
      y *= y;
      y *= y;
      y = 1/y;
      return y;
    }
  else
    return Dzero;
}

real
DQDexpmx(real x)
{
  register real y;
  y = Dabs(x);
  if (y < (real)13) 
    {
      y = A0+y*(A1+y*(A2+y*(A3+y*A4)));
      y *= y;
      y *= y;
      y *= y;
      y = 1/y;
      return (x<Dzero)?(-y):(y);
    }
  else
    return Dzero;
}

real
DQexpmx2(real x)
{
  return DQexpmx(x*x);
}

real
DQDexpmx2(real x)
{
  return 2*x*DQexpmx(x*x);
}

#undef A0
#undef A1
#undef A2
#undef A3
#undef A4


/*
 * Fsplinit(n,flt[3n]):   initialize a cubic spline interpolation.
 * Fspline(x,n,flt[3n]):  interpolate in x.
 * Fdspline(x,n,flt[3n]): derivative of the interpolation.
 */

void
Fsplinit(int size, flt *parm)
{
  flt *x  = parm;
  flt *y  = parm+size;
  flt *y2 = parm+size+size;
  flt *k  = (flt*)alloca(sizeof(flt)*size);
  flt sig,p,q;
  int i;
  
  y2[0] = y2[size-1] = k[0] = Fzero;
  
#ifndef NOLISP
  for(i=1;i<size;i++)
    if (x[i-1] >= x[i])
      error(NIL,"Unordered X values in the spline",NIL);
#endif
  
  for( i=1; i < size-1; i++) 
    {
      sig = (x[i]-x[i-1]) / (x[i+1]-x[i-1]);
      p = sig*y2[i-1] + Ftwo;
      q = (y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
      y2[i] = (sig-1) / p;
      k[i] = ( (flt)(6.0)*q/(x[i+1]-x[i-1]) - sig*k[i-1] ) / p;
    }
  for( i=size-2; i>=0; i-- ) 
    {
      y2[i] = y2[i]*y2[i+1]+k[i];
    }
}


flt
Fspline(flt xx, int size, flt *parm)
{
  flt *x  = parm;
  flt *y  = parm+size;
  flt *y2 = parm+size+size;
  
  int k,klo,khi;
  flt a,b,h;

  klo = 0;
  khi = size-1;

  while( khi-klo > 1 ) {
    k = (khi+klo)>>1;
    if (x[k]>xx)
      khi = k;
    else
      klo = k;
  }

  h = x[khi]-x[klo];
	
  if (klo==0 && xx<x[klo]) 
    {
      flt d = (y[khi]-y[klo])/h - h/(flt)(6.0)*(2*y2[klo]+y2[khi]);
      return y[klo] + (xx-x[klo])*d;
    }
  else if (khi==size-1 && xx>x[khi])
    {
      flt d = (y[khi]-y[klo])/h + h/(flt)(6.0)*(y2[klo]+2*y2[khi]);
      return y[khi] + (xx-x[khi])*d;
    }
  else
    {
      a = (x[khi]-xx)/h;
      b = (xx-x[klo])/h;
      h = h*h/(flt)(6.0);
      return a*y[klo] + b*y[khi] + h*( (a*a*a-a)*y2[klo] + (b*b*b-b)*y2[khi] );
    }
}


flt
Fdspline(flt xx, int size, flt *parm)
{
  flt *x  = parm;
  flt *y  = parm+size;
  flt *y2 = parm+size+size;
  
  int k,klo,khi;
  flt a,b,h;

  klo = 0;
  khi = size-1;

  while( khi-klo > 1 ) {
    k = (khi+klo)>>1;
    if (x[k]>xx)
      khi = k;
    else
      klo = k;
  }

  h = x[khi]-x[klo];


  if (klo==0 && xx<x[klo]) 
    {
      return (y[khi]-y[klo])/h - h/(flt)(6.0)*(2*y2[klo]+y2[khi]);
    }
  else if (khi==size-1 && xx>x[khi])
    {
      return (y[khi]-y[klo])/h + h/(flt)(6.0)*(y2[klo]+2*y2[khi]);  
    }
  else
    {
      a = (x[khi]-xx)/h;
      a = (flt)(3.0)*a*a - Fone;
      b = (xx-x[klo])/h;
      b = (flt)(3.0)*b*b - Fone;
      return (y[khi]-y[klo])/h + (b*y2[khi]-a*y2[klo])*h/(flt)(6.0);
    }
}

/*
 * Same but for double precision (real)
 * Dsplinit(n,real[3n]):   initialize a cubic spline interpolation.
 * Dspline(x,n,real[3n]):  interpolate in x.
 * Ddspline(x,n,real[3n]): derivative of the interpolation.
 */

void
Dsplinit(int size, real *parm)
{
  real *x  = parm;
  real *y  = parm+size;
  real *y2 = parm+size+size;
  real *k  = (real*)alloca(sizeof(real)*size);
  real sig,p,q;
  int i;
  
  y2[0] = y2[size-1] = k[0] = Dzero;
  
#ifndef NOLISP
  for(i=1;i<size;i++)
    if (x[i-1] >= x[i])
      error(NIL,"Unordered X values in the spline",NIL);
#endif
  
  for( i=1; i < size-1; i++) 
    {
      sig = (x[i]-x[i-1]) / (x[i+1]-x[i-1]);
      p = sig*y2[i-1] + Dtwo;
      q = (y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
      y2[i] = (sig-1) / p;
      k[i] = ( (real)(6.0)*q/(x[i+1]-x[i-1]) - sig*k[i-1] ) / p;
    }
  for( i=size-2; i>=0; i-- ) 
    {
      y2[i] = y2[i]*y2[i+1]+k[i];
    }
}


real
Dspline(real xx, int size, real *parm)
{
  real *x  = parm;
  real *y  = parm+size;
  real *y2 = parm+size+size;
  
  int k,klo,khi;
  real a,b,h;

  klo = 0;
  khi = size-1;

  while( khi-klo > 1 ) {
    k = (khi+klo)>>1;
    if (x[k]>xx)
      khi = k;
    else
      klo = k;
  }

  h = x[khi]-x[klo];
	
  if (klo==0 && xx<x[klo]) 
    {
      real d = (y[khi]-y[klo])/h - h/(real)(6.0)*(2*y2[klo]+y2[khi]);
      return y[klo] + (xx-x[klo])*d;
    }
  else if (khi==size-1 && xx>x[khi])
    {
      real d = (y[khi]-y[klo])/h + h/(real)(6.0)*(y2[klo]+2*y2[khi]);
      return y[khi] + (xx-x[khi])*d;
    }
  else
    {
      a = (x[khi]-xx)/h;
      b = (xx-x[klo])/h;
      h = h*h/(real)(6.0);
      return a*y[klo] + b*y[khi] + h*( (a*a*a-a)*y2[klo] + (b*b*b-b)*y2[khi] );
    }
}


real
Ddspline(real xx, int size, real *parm)
{
  real *x  = parm;
  real *y  = parm+size;
  real *y2 = parm+size+size;
  
  int k,klo,khi;
  real a,b,h;

  klo = 0;
  khi = size-1;

  while( khi-klo > 1 ) {
    k = (khi+klo)>>1;
    if (x[k]>xx)
      khi = k;
    else
      klo = k;
  }

  h = x[khi]-x[klo];


  if (klo==0 && xx<x[klo]) 
    {
      return (y[khi]-y[klo])/h - h/(real)(6.0)*(2*y2[klo]+y2[khi]);
    }
  else if (khi==size-1 && xx>x[khi])
    {
      return (y[khi]-y[klo])/h + h/(real)(6.0)*(y2[klo]+2*y2[khi]);  
    }
  else
    {
      a = (x[khi]-xx)/h;
      a = (real)(3.0)*a*a - Done;
      b = (xx-x[klo])/h;
      b = (real)(3.0)*b*b - Done;
      return (y[khi]-y[klo])/h + (b*y2[khi]-a*y2[klo])*h/(real)(6.0);
    }
}



/*
 * Fseed(x):    gives a seed for random generators
 * Frand(): 	uniform random generator in [0,1]	
 * Fgauss():	normal random generator. mean 0, sdev 1 
 * 
 * References 
 * 
 * -  KNUTH D.E. 1981 'Seminumerical Algorithms' 2nd ed.,vol 2 'The
 * Art of Computer Programming' chapter 3.2, 3.3  (substractive method).
 * -  PRESS W.H & al. 1988 'Numerical Recipes in C' Chapter 7 (ran3) -
 * Cambridge University Press
 */

#define MMASK  0x7fffffffL
#define MSEED  161803398L
#define FAC    ((flt)(1.0/(1.0+MMASK)))
#define FAC2   ((flt)(1.0/0x01000000L))

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
  return (flt)(mj * FAC);
}


flt 
Fgauss(void)
{
  /*
   * Now a quick and dirty way to build
   * a quasi-normal random number.
   */
  register int i;
  register int mj, sum;
  mj = 0;
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
  return (flt)(sum * FAC2);
}


/* should probably be rewritten to fill up all the bits */
void Dseed(int x) { Fseed(x); }

real 
Drand(void)
{
  register int mj;

  if (++inext == 56)
    inext = 1;
  if (++inextp == 56)
    inextp = 1;
  mj = ((ma[inext] - ma[inextp]) * 84589 + 45989) & MMASK;
  ma[inext] = mj;
  return (real)(mj * FAC);
}


real 
Dgauss(void)
{
  /*
   * Now a quick and dirty way to build
   * a quasi-normal random number.
   */
  register int i;
  register int mj, sum;
  mj = 0;
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
  return (real)(sum * FAC2);
}

#undef MMASK
#undef MSEED
#undef FAC
#undef FAC2

