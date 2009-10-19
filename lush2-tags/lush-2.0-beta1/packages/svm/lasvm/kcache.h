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
 * $Id: kcache.h,v 1.8 2007/01/25 22:42:09 leonb Exp $
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
/* CACHE FOR KERNEL VALUES */


/* --- lasvm_kcache_t
   This is the opaque data structure for a kernel cache.
*/
typedef struct lasvm_kcache_s lasvm_kcache_t;

/* --- lasvm_kcache_create
   Returns a cache object for kernel <kernelfun>.
   The cache handles a Gram matrix of size <n>x<n> at most.
   Argument <closure> is passed to the kernel function <kernelfun>.
 */
lasvm_kcache_t* lasvm_kcache_create(lasvm_kernel_t kernelfunc, void *closure);

/* --- lasvm_kcache_destroy
   Deallocates a kernel cache object.
*/
void lasvm_kcache_destroy(lasvm_kcache_t *self);

/* --- lasvm_kcache_set_maximum_size
   Sets the maximum memory size used by the cache.
   Argument <entries> indicates the maximum cache memory in bytes
   The default size is 256Mb.
*/
void lasvm_kcache_set_maximum_size(lasvm_kcache_t *self, long entries);

/* --- lasvm_kcache_get_maximum_size
   Returns the maximum cache memory.
 */
long lasvm_kcache_get_maximum_size(lasvm_kcache_t *self);

/* --- lasvm_kcache_get_current_size
   Returns the currently used cache memory.
   This can slighly exceed the value specified by 
   <lasvm_kcache_set_maximum_size>.
 */
long lasvm_kcache_get_current_size(lasvm_kcache_t *self);

/* --- lasvm_kcache_query
   Returns the possibly cached value of the Gram matrix element (<i>,<j>).
   This function will not modify the cache geometry.
 */
double lasvm_kcache_query(lasvm_kcache_t *self, int i, int j);

/* --- lasvm_kcache_query_row
   Returns the <len> first elements of row <i> of the Gram matrix.
   The cache user can modify the order of the row elements
   using the lasvm_kcache_swap() functions.  Functions lasvm_kcache_i2r() 
   and lasvm_kcache_r2i() convert from example index to row position 
   and vice-versa.
*/

float *lasvm_kcache_query_row(lasvm_kcache_t *self, int i, int len);

/* --- lasvm_kcache_status_row
   Returns the number of cached entries for row i.
*/

int lasvm_kcache_status_row(lasvm_kcache_t *self, int i);

/* --- lasvm_kcache_discard_row
   Indicates that we wont need row i in the near future.
*/

void lasvm_kcache_discard_row(lasvm_kcache_t *self, int i);


/* --- lasvm_kcache_i2r
   --- lasvm_kcache_r2i
   Return an array of integer of length at least <n> containing
   the conversion table from example index to row position and vice-versa. 
*/

int *lasvm_kcache_i2r(lasvm_kcache_t *self, int n);
int *lasvm_kcache_r2i(lasvm_kcache_t *self, int n);


/* --- lasvm_kcache_swap_rr
   --- lasvm_kcache_swap_ii
   --- lasvm_kcache_swap_ri
   Swaps examples in the row ordering table.
   Examples can be specified by indicating their row position (<r1>, <r2>)
   or by indicating the example number (<i1>, <i2>).
*/

void lasvm_kcache_swap_rr(lasvm_kcache_t *self, int r1, int r2);
void lasvm_kcache_swap_ii(lasvm_kcache_t *self, int i1, int i2);
void lasvm_kcache_swap_ri(lasvm_kcache_t *self, int r1, int i2);


/* --- lasvm_kcache_shuffle
   Swaps the row ordering table in order to make sure that the 
   first elements are exactly those found in <ilist[0]...ilist[ilen-1]>.
   The elements are always kept in increasing order and the duplicates are eliminated. 
   This function returns the number <n> of distinct elements.
   Array <ilist> is reordered in the process.
*/

int lasvm_kcache_shuffle(lasvm_kcache_t *self, int *ilist, int ilen);



/* --- lasvm_kcache_set_buddy
   This function is called to indicate that the caches <self> and <buddy>
   implement the same kernel function. When a buddy experiences a cache
   miss, it can try querying its buddies instead of calling the 
   kernel function.  Buddy relationship is transitive. */

void lasvm_kcache_set_buddy(lasvm_kcache_t *self, lasvm_kcache_t *buddy);


#ifdef __cplusplus__
}
#endif
#endif
