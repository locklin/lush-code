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
 * $Id: vector.h,v 1.1 2005-02-10 14:49:11 leonb Exp $
 **********************************************************************/

#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus__
extern "C" { 
#if 0
}
#endif
#endif


/* ------------------------------------- */
/* SIMPLE VECTORS */


typedef struct lasvm_vector_s {
  int size;
  double data[1];
} lasvm_vector_t;

lasvm_vector_t *lasvm_vector_create(int size);

void lasvm_vector_destroy(lasvm_vector_t *v);

double lasvm_vector_dot_product(lasvm_vector_t *v1, lasvm_vector_t *v2);


/* ------------------------------------- */
/* SPARSE VECTORS */


typedef struct lasvm_sparsevector_pair_s {
  struct lasvm_sparsevector_pair_s *next;
  int    index;
  double data;
} lasvm_sparsevector_pair_t;

typedef struct lasvm_sparsevector_s {
  int size;
  int npairs;
  lasvm_sparsevector_pair_t *pairs;
  lasvm_sparsevector_pair_t **last;
} lasvm_sparsevector_t;

lasvm_sparsevector_t *lasvm_sparsevector_create(void);

void lasvm_sparsevector_destroy(lasvm_sparsevector_t *v);

void lasvm_sparsevector_set(lasvm_sparsevector_t *v, int index, double data);

double lasvm_sparsevector_get(lasvm_sparsevector_t *v, int index);

lasvm_sparsevector_t *lasvm_sparsevector_combine(lasvm_sparsevector_t *v1, double coeff1,
						 lasvm_sparsevector_t *v2, double coeff2);

double lasvm_sparsevector_dot_product(lasvm_sparsevector_t *v1, 
				      lasvm_sparsevector_t *v2);

#ifdef __cplusplus__
}
#endif
#endif
