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
 * $Id: kernel.c,v 1.2 2004-08-30 23:24:30 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "messages.h"
#include "kernel.h"





double 
mysvm_vectorproblem_lin_kernel(int i, int j, void *problem)
{
  mysvm_vectorproblem_t *p = problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  return mysvm_vector_dot_product(p->x[i], p->x[j]);
}


double 
mysvm_vectorproblem_rbf_kernel(int i, int j, void *problem)
{
  double d;
  mysvm_vectorproblem_t *p = problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  d = mysvm_vector_dot_product(p->x[i], p->x[j]);
  return exp( - p->rbfgamma * ( p->xnorm[i] + p->xnorm[j] - 2 * d ));
}



double 
mysvm_sparsevectorproblem_lin_kernel(int i, int j, void *problem)
{
  mysvm_sparsevectorproblem_t *p = problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  return mysvm_sparsevector_dot_product(p->x[i], p->x[j]);
}

double 
mysvm_sparsevectorproblem_rbf_kernel(int i, int j, void *problem)
{
  double d;
  mysvm_sparsevectorproblem_t *p = problem;
  ASSERT(i>=0 && i<p->l);
  ASSERT(j>=0 && j<p->l);
  d = mysvm_sparsevector_dot_product(p->x[i], p->x[j]);
  return exp( - p->rbfgamma * ( p->xnorm[i] + p->xnorm[j] - 2 * d ));
}

