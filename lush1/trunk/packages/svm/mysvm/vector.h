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
 * $Id: vector.h,v 1.1 2004-08-30 22:07:28 leonb Exp $
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


typedef struct mysvm_vector_s {
  int size;
  double data[1];
} mysvm_vector_t;

mysvm_vector_t *mysvm_vector_create(int size);

void mysvm_vector_destroy(mysvm_vector_t *v);

double mysvm_vector_dot_product(mysvm_vector_t *v1, mysvm_vector_t *v2);

double mysvm_vector_l2_distance(mysvm_vector_t *v1, mysvm_vector_t *v2);


/* ------------------------------------- */
/* SPARSE VECTORS */


typedef struct mysvm_sparsevector_pair_s {
  struct mysvm_sparsevector_pair_s *next;
  int    index;
  double data;
} mysvm_sparsevector_pair_t;

typedef struct mysvm_sparsevector_s {
  int size;
  int npairs;
  mysvm_sparsevector_pair_t *pairs;
  mysvm_sparsevector_pair_t **last;
} mysvm_sparsevector_t;

mysvm_sparsevector_t *mysvm_sparsevector_create(void);

void mysvm_sparsevector_destroy(mysvm_sparsevector_t *v);

void mysvm_sparsevector_set(mysvm_sparsevector_t *v, int index, double data);

double mysvm_sparsevector_get(mysvm_sparsevector_t *v, int index);

mysvm_sparsevector_t *mysvm_sparsevector_combine(mysvm_sparsevector_t *v1, double coeff1,
						 mysvm_sparsevector_t *v2, double coeff2);

double mysvm_sparsevector_dot_product(mysvm_sparsevector_t *v1, 
				      mysvm_sparsevector_t *v2);

double mysvm_sparsevector_l2_distance(mysvm_sparsevector_t *v1, 
				      mysvm_sparsevector_t *v2);


#ifdef __cplusplus__
}
#endif
#endif
