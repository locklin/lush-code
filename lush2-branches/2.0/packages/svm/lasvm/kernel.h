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
 * $Id: kernel.h,v 1.1 2005/02/10 14:49:11 leonb Exp $
 **********************************************************************/

#ifndef KERNEL_H
#define KERNEL_H

#include <stdlib.h>
#include <stdio.h>

#include "vector.h"

#ifdef __cplusplus__
extern "C" { 
#if 0
}
#endif
#endif



/* ------------------------------------- */
/* GENERIC KERNEL TYPE */


/* --- lasvm_kernel_t
   This is the type for user defined symmetric kernel functions.
   It returns the Gram matrix element at position <i>,<j>. 
   Argument <closure> represents arbitrary additional information.
*/
#ifndef LASVM_KERNEL_T_DEFINED
#define LASVM_KERNEL_T_DEFINED
typedef double (*lasvm_kernel_t)(int i, int j, void* closure);
#endif



/* ------------------------------------- */
/* USEFUL KERNELS */


typedef struct lasvm_vectorproblem_s 
{
  int l;			/* number of examples */
  int n;			/* dimension of examples */
  lasvm_vector_t **x;		/* x[0]...x[l-1] */
  double *y;			/* category */
  double *xnorm;		/* for rbf kernel: l2-norm of the x[i] */
  double rbfgamma;		/* for rbf kernel: gamma */
} lasvm_vectorproblem_t;


double lasvm_vectorproblem_lin_kernel(int i, int j, void *problem);

double lasvm_vectorproblem_rbf_kernel(int i, int j, void *problem);


/* ------------------------------------- */
/* MORE USEFUL KERNELS */


typedef struct lasvm_sparsevectorproblem_s 
{
  int l;			/* number of examples */
  int n;			/* dimension of examples */
  lasvm_sparsevector_t **x;	/* x[0]...x[l-1] */
  double *y;			/* category */
  double *xnorm;		/* for rbf kernel: l2-norm of the x[i] */
  double rbfgamma;		/* for rbf kernel: gamma */
} lasvm_sparsevectorproblem_t;


double lasvm_sparsevectorproblem_lin_kernel(int i, int j, void *problem);

double lasvm_sparsevectorproblem_rbf_kernel(int i, int j, void *problem);







#ifdef __cplusplus__
}
#endif
#endif
