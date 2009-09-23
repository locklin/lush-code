/***********************************************************************
 * 
 *  LASVM -- online svm.
 *  Copyright (C) 2005 NEC Labs America
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
 * $Id: lasvm.h,v 1.5 2005/11/16 20:33:20 leonb Exp $
 **********************************************************************/

#ifndef LASVM_H
#define LASVM_H

#include "kcache.h"

/* --- lasvm_t
   Opaque type representing the LASVM state. 
*/
typedef struct lasvm_s lasvm_t;

/* --- lasvm_create
   Creates a lasvm object.  You must first create and configure 
   a kernel cache <cache> object for your chosen kernel. 
   Never associate a same cache object with several lasvm objects.
   Argument <sumflag> indicates whether the equality constraint
   should be honored (i.e. whether the svm has a bias term).
   Setting <sumflag> to zero should be considered experimental.
   Arguments <cp> and <cn> are the C values for positive and
   negative examples.
*/
lasvm_t *lasvm_create( lasvm_kcache_t *cache,
                       int sumflag, double cp, double cn );

/* --- lasvm_destroy
   Deallocates a lasvm object. 
   The associated kernel cache must be deallocated separately. 
*/
void lasvm_destroy( lasvm_t *self );

/* --- lasvm_get_l
   Returns the number of support vectors.
*/
int lasvm_get_l( lasvm_t *self );

/* --- lasvm_process
   Perform the PROCESS operation on example with index <xi>.
   Argument <y> is the example label and must be +1 or -1.
   Returns the number of SVs if a change has been made.
   Otherwise returns zero.
*/

int lasvm_process( lasvm_t *self, int xi, double y );

/* --- lasvm_reprocess
   Performs the REPROCESS operation with a tolerance <epsgr>
   on the gradients. Returns the number of SVs if a change 
   has been made. Otherwise returns zero. 
*/
int lasvm_reprocess(lasvm_t *self, double epsgr);

/* --- lasvm_finish
   Specialized version of REPROCESS used for the finishing step.
   Calling <lasvm_finish> is essentially similar to 
   calling <lasvm_reprocess> several times. However it runs
   faster because it performs the shrinking optimization.
   Returns the number of iterations.
*/
int lasvm_finish(lasvm_t *self, double epsgr);

/* -- lasvm_get_cp, lasvm_get_cn
   Returns the values of parameter C for positive 
   and negative examples.
*/
double lasvm_get_cp( lasvm_t *self );
double lasvm_get_cn( lasvm_t *self );

/* --- lasvm_get_delta
   Returns the maximal gradient exploitable by a subsequent
   REPROCESS operation. Calling REPROCESS with <epsgr>
   smaller than delta returns immediately.
*/
double lasvm_get_delta(lasvm_t *self);

/* --- lasvm_get_alpha
   Copies the support vector coefficients into array <alpha>.
   Coefficients are positive for positive example
   and negative for negative examples.
*/
int lasvm_get_alpha(lasvm_t *self, double *alpha);

/* --- lasvm_get_alpha
   Returns the support vector indices into array <sv>.
   Similar information can be obtained using <lasvm_kcache_r2i>.
*/
int lasvm_get_sv(lasvm_t *self, int *sv);

/* --- lasvm_get_g
   Returns the gradient of the dual objective function
   with respect to each support vector coefficient.
*/
int lasvm_get_g(lasvm_t *self, double *g);

/* --- lasvm_get_b
   Returns the value of the bias term. 
*/
double lasvm_get_b(lasvm_t *self);

/* --- lasvm_get_w2
   Returns the value of the dual objective function.
*/
double lasvm_get_w2(lasvm_t *self);

/* --- lasvm_predict
   Computes the kernel expansion on example with index <xi>
   and returns the result.
*/
double lasvm_predict(lasvm_t *self, int xi);

/* --- lasvm_predict_nocache
   Same as <lasvm_predict> but does not mark the corresponding
   kernel values as recently used. 
*/
double lasvm_predict_nocache(lasvm_t *self, int xi);

/* --- lasvm_init
   Resets the state of lasvm to known values.
   The gradients <g> are optional. 
   Passing a null pointer will cause them
   to be recomputed on the fly.

   KNOWN BUG: this function uses the sign of the
   coefficient <alpha> to determine the class
   of each example. That does not work if <alpha>
   is zero. Such support vectors are eliminated...
*/
void lasvm_init( lasvm_t *self, int l, 
                 const int *sv, 
                 const double *alpha, 
                 const double *g );

#endif
