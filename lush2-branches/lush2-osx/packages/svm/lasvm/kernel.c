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

/***********************************************************************
 * $Id: kernel.c,v 1.1 2005/02/10 14:49:11 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "messages.h"
#include "kernel.h"

#ifdef __cplusplus__
#define this  mythis
#define or    myor
#define and   myand
#endif


double 
lasvm_vectorproblem_lin_kernel(int i, int j, void *problem)
{
  lasvm_vectorproblem_t *p = (lasvm_vectorproblem_t*)problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  return lasvm_vector_dot_product(p->x[i], p->x[j]);
}


double 
lasvm_vectorproblem_rbf_kernel(int i, int j, void *problem)
{
  double d;
  lasvm_vectorproblem_t *p = (lasvm_vectorproblem_t*)problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  d = lasvm_vector_dot_product(p->x[i], p->x[j]);
  return exp( - p->rbfgamma * ( p->xnorm[i] + p->xnorm[j] - 2 * d ));
}



double 
lasvm_sparsevectorproblem_lin_kernel(int i, int j, void *problem)
{
  lasvm_sparsevectorproblem_t *p = (lasvm_sparsevectorproblem_t*)problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  return lasvm_sparsevector_dot_product(p->x[i], p->x[j]);
}

double 
lasvm_sparsevectorproblem_rbf_kernel(int i, int j, void *problem)
{
  double d;
  lasvm_sparsevectorproblem_t *p = (lasvm_sparsevectorproblem_t*)problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  d = lasvm_sparsevector_dot_product(p->x[i], p->x[j]);
  return exp( - p->rbfgamma * ( p->xnorm[i] + p->xnorm[j] - 2 * d ));
}

