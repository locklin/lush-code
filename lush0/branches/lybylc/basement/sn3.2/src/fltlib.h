/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/
  

#ifndef DEFINE_H
#include "define.h"
#endif

#ifndef FLTLIB_H
#define FLTLIB_H

/**
 ** THIS IS A BUNCH OF MACROS 
 ** FOR FAST SINGLE PRECISION ARITHMETIC.
 ** IMPLIES '-FSINGLE'
 **/

/* 'real' is the natural floating point type */
#define real double

/* 'flt' is a fast, single precision, floating point */
#define flt  float


/* Constants */

#define Fzero 	((flt)0.0)
#define Fone	((flt)1.0)
#define Ftwo	((flt)2.0)
#define Fminus	((flt)-1.0)
#define Fhalf	((flt)0.5)

#define Dzero 	((double)0.0)
#define Done	((double)1.0)
#define Dtwo	((double)2.0)
#define Dminus	((double)-1.0)
#define Dhalf	((double)0.5)

/* Standard Math routines */

#define Fsqrt(x)	((flt)sqrt((double)x))
#define Finv(x)		((flt)(1/(double)x))
#define Fsin(x)		((flt)sin((double)x))
#define Fcos(x)		((flt)cos((double)x))
#define Ftan(x)		((flt)tan((double)x))
#define Fasin(x)	((flt)asin((double)x))
#define Facos(x)	((flt)acos((double)x))
#define Fatan(x)	((flt)atan((double)x))
#define Fexp(x)         ((flt)exp((double)x))
#define Fexpm1(x)       ((flt)expm1((double)x))
#define Flog(x)         ((flt)log((double)x))
#define Flog1p(x)       ((flt)log1p((double)x))
#define Fsinh(x)        ((flt)sinh((double)x))
#define Fcosh(x)        ((flt)cosh((double)x))
#define Ftanh(x)        ((flt)tanh((double)x))
#define Fatanh(x)       ((flt)atanh((double)x))

#define Dsqrt(x)	((double)sqrt((double)x))
#define Dinv(x)		((double)(1/(double)x))
#define Dsin(x)		((double)sin((double)x))
#define Dcos(x)		((double)cos((double)x))
#define Dtan(x)		((double)tan((double)x))
#define Dasin(x)	((double)asin((double)x))
#define Dacos(x)	((double)acos((double)x))
#define Datan(x)	((double)atan((double)x))
#define Dexp(x)         ((double)exp((double)x))
#define Dexpm1(x)       ((double)expm1((double)x))
#define Dlog(x)         ((double)log((double)x))
#define Dlog1p(x)       ((double)log1p((double)x))
#define Dsinh(x)        ((double)sinh((double)x))
#define Dcosh(x)        ((double)cosh((double)x))
#define Dtanh(x)        ((double)tanh((double)x))
#define Datanh(x)       ((double)atanh((double)x))


/* Piecewise functions */

#define Ffloor(x)       ((flt)floor((double)x))
#define Fabs(x)         (((x)<Fzero)?-(x):(x))
#define Fsgn(x)         (((x)<Fzero)?Fminus:Fone)
#define Fpiece(x)       (((x)<Fminus)?Fminus:(((x)<Fone)?x:Fone))
#define Frect(x)        ((((x)>Fminus)&&((x)<Fone))?Fone:Fzero)

#define Dfloor(x)       ((double)floor((double)x))
#define Dabs(x)         (((x)<Dzero)?-(x):(x))
#define Dsgn(x)         (((x)<Dzero)?Dminus:Done)
#define Dpiece(x)       (((x)<Dminus)?Dminus:(((x)<Done)?x:Done))
#define Drect(x)        ((((x)>Dminus)&&((x)<Done))?Done:Dzero)


/* Rational approximation of TANH, SIGMOIDS */
/* Declarations for EXP(-|X|), EXP(-X2)     */

extern double FQtanh();
extern double FQDtanh();
extern double FQstdsigmoid();
extern double FQDstdsigmoid();
extern double FQexpmx();
extern double FQDexpmx();
extern double FQexpmx2();
extern double FQDexpmx2();

/* Spline interpolation */

extern void Fsplinit();
extern double Fspline();
extern double Fdspline();

/* Random functions */

extern void Fseed();
extern double Frand();
extern double Fgauss();


#endif
