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
 * $Id: kcache.h,v 1.3 2004-11-02 19:00:54 leonb Exp $
 **********************************************************************/

#ifndef KCACHE_H
#define KCACHE_H

#ifdef __cplusplus__
extern "C" { 
#if 0
}
#endif
#endif


/* ------------------------------------- */
/* GENERIC KERNEL TYPE */


/* --- mysvm_kernel_t
   This is the type for user defined symmetric kernel functions.
   It returns the Gram matrix element at position <i>,<j>. 
   Argument <closure> represents arbitrary additional information.
*/
#ifndef MYSVM_KERNEL_T_DEFINED
#define MYSVM_KERNEL_T_DEFINED
typedef double (*mysvm_kernel_t)(int i, int j, void* closure);
#endif



/* ------------------------------------- */
/* CACHE FOR KERNEL VALUES */


/* --- mysvm_kcache_t
   This is the opaque data structure for a kernel cache.
*/
typedef struct mysvm_kcache_s mysvm_kcache_t;

/* --- mysvm_kcache_create
   Returns a cache object for kernel <kernelfun>.
   The cache handles a Gram matrix of size <n>x<n> at most.
   Argument <closure> is passed to the kernel function <kernelfun>.
 */
mysvm_kcache_t* mysvm_kcache_create(mysvm_kernel_t kernelfunc, void *closure);

/* --- mysvm_kcache_destroy
   Deallocates a kernel cache object.
*/
void mysvm_kcache_destroy(mysvm_kcache_t *self);

/* --- mysvm_kcache_set_maximum_size
   Sets the maximum memory size used by the cache.
   Argument <entries> indicates the maximum cache memory.
   The default size is 256Mb.
*/
void mysvm_kcache_set_maximum_size(mysvm_kcache_t *self, int entries);

/* --- mysvm_kcache_set_maximum_size
   Returns the maximum cache memory.
 */
int mysvm_kcache_get_maximum_size(mysvm_kcache_t *self);

/* --- mysvm_kcache_set_maximum_size
   Returns the currently used cache memory.
   This can slighly exceed the value specified by 
   <mysvm_kcache_set_maximum_size>.
 */
int mysvm_kcache_get_current_size(mysvm_kcache_t *self);

/* --- mysvm_kcache_query
   Returns the possibly cached value of the Gram matrix element (<i>,<j>).
   This function will not modify the cache geometry.
 */
double mysvm_kcache_query(mysvm_kcache_t *self, int i, int j);

/* --- mysvm_kcache_query_row
   Returns the <len> first elements of row <i> of the Gram matrix.
   The cache user can modify the order of the row elements
   using the mysvm_kcache_swap() functions.  Functions mysvm_kcache_i2r() 
   and mysvm_kcache_r2i() convert from example index to row position 
   and vice-versa.
*/

float *mysvm_kcache_query_row(mysvm_kcache_t *self, int i, int len);

/* --- mysvm_kcache_i2r
   --- mysvm_kcache_r2i
   Return an array of integer of length at least <n> containing
   the conversion table from example index to row position and vice-versa. 
*/

int *mysvm_kcache_i2r(mysvm_kcache_t *self, int n);
int *mysvm_kcache_r2i(mysvm_kcache_t *self, int n);


/* --- mysvm_kcache_swap_rr
   --- mysvm_kcache_swap_ii
   --- mysvm_kcache_swap_ri
   Swaps examples in the row ordering table.
   Examples can be specified by indicating their row position (<r1>, <r2>)
   or by indicating the example number (<i1>, <i2>).
*/

void mysvm_kcache_swap_rr(mysvm_kcache_t *self, int r1, int r2);
void mysvm_kcache_swap_ii(mysvm_kcache_t *self, int i1, int i2);
void mysvm_kcache_swap_ri(mysvm_kcache_t *self, int r1, int i2);


#ifdef __cplusplus__
}
#endif
#endif
