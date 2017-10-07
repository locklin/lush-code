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
	TL3:	(C) LYB YLC  1988
	fltlib.h
	definition of numeric type  'flt'
	$Id: fltlib.h,v 1.1.1.1 2002-04-16 17:37:32 leonb Exp $
********************************************************************** */

#ifndef DEFINE_H
#include "define.h"
#endif

#ifndef FLTLIB_H
#define FLTLIB_H

/* PLEASE, See FLTLIB.C */

#ifdef FIXED
#define real double
#define flt  long
#define Fadd(x,y) ((x)+(y))
#define Fsub(x,y) ((x)-(y))
#define Fmul(x,y) (((x)>>8)*((y)>>8))
#define Fdiv(x,y) (rtoF( (float)Ftor(x) / (float)Ftor(y) ))
#define Fzero 	(0x00000000L)
#define Fone	(0x00010000L)
#define Ftwo	(0x00020000L)
#define Fminus	(0xffff0000L)
#define Fhalf	(0x00008000L)
#define rtoF(x) ((long)((x)*65536))
#define dtoF(x) ((long)((x)*65536))
#define Ftor(x) ((real)(x) /65536)
#define Ftod(x) ((double)(x)/65536)
#define Fsgn(x)	((x)==0 ? 0 : ((x)>0 ? 1 : -1 ) )
#else
#define real double
#define flt  float
#define Fadd(x,y) (flt)((x)+(y))
#define Fsub(x,y) (flt)((x)-(y))
#define Fmul(x,y) (flt)((x)*(y))
#define Fdiv(x,y) (flt)((x)/(y))
#define Fzero 	((flt)0.0)
#define Fone	((flt)1.0)
#define Ftwo	((flt)2.0)
#define Fminus	((flt)-1.0)
#define Fhalf	((flt)0.5)
#define rtoF(x) ((flt)(x))
#define dtoF(x) ((flt)(x))
#define Ftor(x) ((real)(x))
#define Ftod(x) ((double)(x))
#define Fsgn(x)	((x)==0 ? 0 : ((x)>0 ? 1 : -1 ) )
#endif


#define Fsqrt(x)	(rtoF(sqrt( (double)Ftor(x) )))
#define Fsin(x)		(rtoF(sin( (double)Ftor(x) )))
#define Fcos(x)		(rtoF(cos( (double)Ftor(x) )))
#define Ftan(x)		(rtoF(tan( (double)Ftor(x) )))
#define Fatn(x)		(rtoF(atn( (double)Ftor(x) )))
#ifdef HAVE_TANH
#define Fsinh(x)	(rtoF(sinh( (double)Ftor(x) )))
#define Fcosh(x)	(rtoF(cosh( (double)Ftor(x) )))
#define Ftanh(x)	(rtoF(tanh( (double)Ftor(x) )))
#endif
#define Flog(x)		(rtoF(log( (double)Ftor(x) )))
#define Fexp(x)		(rtoF(exp( (double)Ftor(x) )))

/* ------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif
TLAPI flt Fsqr(flt x);
TLAPI flt Frand(void);
TLAPI flt Fgauss(void);
TLAPI void Fseed(int x);
#ifndef HAVE_TANH
TLAPI flt Fsinh(flt);
TLAPI flt Fcosh(flt);
TLAPI flt Ftanh(flt);
#endif
#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------- */
#endif
