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

/**********************************************
 **
 **  A fast generalized portrait: innerloop
 **
 **  CF VAPNIK PP353-...
 **/


#include "header.h"

/* #define DEBUG */


/* =========================================== */
/* =========== STRUCTURE DEFINITION ========== */
/* =========================================== */


#define GP2NUMARGS 8
#define GP2NAMELEN 32

struct gp2 {

  flt **dpt;			/* The dot product table */
  flt  *dpt_buffer;		/* The buffer for the dot product table */
  int   dpt_flag;		/* Is the table is up to date ? */
  
  flt **set;			/* The examples table */
  flt  *set_buffer;		/* The examples buffer */
  
  int  *index;			/* The index of the examples in the set */
  flt  *alpha;			/* The lagrange coefficients */
  int  *polarity;		/* The class of the example +1 or -1 */
  
  flt (*func)();		/* The dot product function */
  char  funcname[GP2NAMELEN];	/* The name of the function */
  flt   funcarg[GP2NUMARGS];	/* The function arguments */
  
  int   dim;			/* Dimension of the examples */
  int   maxsize;		/* The maximum number of examples */
  int   size;			/* The current number of examples */
  flt   lasttwow;		/* Last error rate */
};


extern struct gp2_ftable {
    char *name;
    flt (*func)();
} gp2_ftable[];

extern char *gp2_error;



/* =========================================== */
/* ================ C LOOPS ================== */
/* =========================================== */


/* Comment that out to remove 
   full debugging informations */
/* #define DEBUG */

/* Comment that out to remove
   some debugging informations */
/* #define DEBUG2 */

/* This is a small number, used for replacing zero */
#define SMALL_FLT (1.0E-15)

/* This is a large number, 
   used for preventing overflows */
#define LARGE_FLT (1.0E15)

/* This is the maximal allowed step, 
   also used for preventing overflows */
#define QUITELARGE_FLT (1.0E6)


/* ======================================================= */

#ifdef DEBUG

#define DEBUG2

void vdump(f,n)
flt *f;
int n;
{
  printf("[ ");
  while (n-->0)
    printf("%5.2f ",*f++);
  printf("]\n");
}
#endif

/* ======================================================= */

/* Stuff for (re)creating, growing, shrinking 
   the dot-product table (dpt) */

char *gp2_error = NULL;

void gp2_free(gp2)
struct gp2 *gp2;
{
#ifdef DEBUG
  printf("Freeing %x\n",gp2);
#endif
  
  if (gp2->dpt)
    free(gp2->dpt);
  if (gp2->dpt_buffer)
    free(gp2->dpt_buffer);
  
  if (gp2->set)
    free(gp2->set);
  if (gp2->set_buffer)
    free(gp2->set_buffer);
  
  if (gp2->index)
    free(gp2->index);
  if (gp2->alpha)
    free(gp2->alpha);
  if (gp2->polarity)
    free(gp2->polarity);

  gp2->maxsize = 0;
  gp2->dpt_flag = 0;
}



void gp2_alloc(gp2,size,dim,func)
struct gp2 *gp2;
int size, dim;
flt (*func)();
{
  int i;
  flt *f;
  
#ifdef DEBUG
  printf("Creating %x\n",gp2);
#endif
  
  gp2->set = (flt**)malloc( sizeof(flt*)*size );
  gp2->set_buffer = (flt*)malloc( sizeof(flt)*size*dim );
  f = gp2->set_buffer;
  for (i=0;i<size;i++) {
    gp2->set[i] = f;
    f += dim;
  }

  gp2->index = (int*)malloc( sizeof(int)*size );
  gp2->alpha = (flt*)malloc( sizeof(flt)*size );
  gp2->polarity = (int*)malloc( sizeof(int)*size );
  
  gp2_error = NULL;
  ifn (gp2->set && gp2->set_buffer &&
       gp2->index && gp2->alpha && gp2->polarity ) {
    gp2_error = "gp2_alloc: Not enough memory";
    return;
  }
  
  gp2->dpt = NULL;
  gp2->dpt_buffer = NULL;
  gp2->dpt_flag = 0;
  gp2->func = func;
  gp2->dim = dim;
  gp2->size = 0;
  gp2->maxsize = size;
  gp2->lasttwow = -LARGE_FLT;
}


void gp2_check_dpt(gp2)
struct gp2 *gp2;
{

  ifn (gp2->dpt) {
    
    ifn (gp2->dpt) 
      { /* allocate dpt table */
	
	flt *f;
	int i, size;
	
#ifdef DEBUG
	printf("Allocating dpt table in %x\n",gp2);
#endif
	size = gp2->maxsize;
	gp2->dpt_buffer = (flt*)malloc( sizeof(flt)*size*size );
	gp2->dpt = (flt**)malloc( sizeof(flt*)*size );
	f = gp2->dpt_buffer;
	for (i=0;i<size;i++) {
	  gp2->dpt[i] = f;
	  f += size;
	}
	ifn (gp2->dpt && gp2->dpt_buffer) {
	  if (gp2->dpt)
	    free(gp2->dpt);
	gp2->dpt = NULL;
	  gp2_error = "Not enough memory";
	  return;
	}
      }
    
    
    { /* compute dpt table */

      flt **d, **set;
      int i,j, size;
      flt (*func)();
      
      
#ifdef DEBUG
      printf("Computing dpt table in %x\n",gp2);
#endif
      func = gp2->func;
      size = gp2->size;
      d = gp2->dpt;
      set = gp2->set;

      for (i=0; i<size; i++)
	for(j=0;j<=i;j++)
	  d[i][j] = d[j][i] =  (*func)( set[i], set[j], gp2 );
      gp2->dpt_flag = 1;
    }
  }
}




void gp2_grow(gp2, pattern, polarity, initial_alpha, index)
struct gp2 *gp2;
flt *pattern;
int polarity;
flt initial_alpha;
int index;
{
  int size, dim;
  flt **set;
  int i;
  
#ifdef DEBUG
  printf("Growing %x\n",gp2);
#endif
  
  gp2_error = NULL;
  
  size = gp2->size;
  dim = gp2->dim;
  set = gp2->set;
  
  if (size+1 > gp2->maxsize) {
    gp2_error = "gp2_grow: Too many examples";
    return;
  }
  
  {
    int i;
    for(i=0; i<dim; i++)
      set[size][i] = pattern[i];
    gp2->index[size] = index;
    gp2->polarity[size] = polarity;
    gp2->alpha[size] = initial_alpha;
  }
  
  
  if (gp2->dpt_flag) {
    int i;
    flt **f = gp2->dpt;
    flt (*func)() = gp2->func;
    for (i=0; i<=size; i++)
      f[i][size] = f[size][i] =  (*func)( set[i], set[size], gp2 );
  }
  
  gp2->size += 1;
}





void gp2_shrink(gp2, n)
struct gp2 *gp2;
int n;
{
  int size;
  int last;
  int i;
  flt **f;
  flt *tmp;
  int tmp_index;
  int tmp_polarity;
  

#ifdef DEBUG
  printf("Shrinking %x, %d\n",gp2, n);
#endif
  
  gp2_error = NULL;  
  
  size = gp2->size;
  if (n<0 || n>=size) {
    gp2_error = "gp2_shrink: bad index to remove";
    return;
  }
  
  last = size-1;
  if (last != n) {
    
    if (gp2->dpt_flag) {
      /* contract dot product */
      f = gp2->dpt;
      for (i=0; i<size; i++)
	f[n][i] = f[last][i];
      for (i=0; i<size; i++)
	f[i][n] = f[i][last];
    }
    
    /* permute set pointers !!! */
    f = gp2->set;
    tmp=f[n]; f[n]=f[last]; f[last]=tmp;
    
    /* contract polarity, alpha and index */
    tmp_polarity = gp2->polarity[n];
    gp2->polarity[n] = gp2->polarity[last];
    gp2->polarity[last] = tmp_polarity;
    gp2->alpha[n] = gp2->alpha[last];
    tmp_index = gp2->index[n];
    gp2->index[n] = gp2->index[last];
    gp2->index[last] = tmp_index;
  }
  gp2->size = last;
}


/* ======================================================= */

/* Performs a recognition run */

flt gp2_run(gp2,x)
struct gp2 *gp2;
flt *x;
{
  flt ans;
  flt inc;
  int i;
  int dim = gp2->dim;
  flt (*f)() = gp2->func;
  flt **y = gp2->set;
  int *p = gp2->polarity;
  flt  *a = gp2->alpha;
  
  gp2_error = NULL;
  
  ans = 0;
  for (i=0; i<gp2->size; i++) {
    inc = *a  * (*f)(x,*y,gp2);
    switch (*p) {
    case 0:
      ans += inc;
      break;
    case 1:
      ans -= inc;
      break;
    default:
      gp2_error = "gp2_run: strange polarity";
      return;
    }
    a++;
    p++;
    y++;
  }
  return ans;
}


/* ======================================================= */



/* Perform the elementary conjugate gradient loop */

/* Returns number of support points or
 *   -1 if failure because too many iterations,
 *   -2 if failure because W is too large,
 *   -3 if failure because Hessian is non negative
 */


int gp2_conjloop(gp2, k0, k1, rho, eps)
struct gp2 *gp2;
flt k0, k1;
flt rho;
flt eps;
{
  flt lastgradnorm = 0;
  flt gradnorm;
  flt twow;
  flt step;
  int iters=0;
  int size = gp2->size;
  
  flt *conjdir = (flt*)alloca( sizeof(flt)*size);
  flt *grad = (flt*)alloca( sizeof(flt)*size);
  flt *alpha = gp2->alpha;
  int *polar = gp2->polarity;
  flt **dpt;
  
  flt kp[2];
  static flt pp[2][2] = { 1, -1, -1, 1};
  
  gp2_error = NULL;
  gp2_check_dpt(gp2);
  dpt = gp2->dpt;
  if (gp2_error)
    return;

  /* Compute kp */
  kp[0] = k0;
  kp[1] = -k1;
  
  /* Adjust rho and eps */
  rho = rho*rho / (k0-k1) / (k0-k1);
  eps = eps*eps;
  
  
  ifn (conjdir && grad) {
    gp2_error = "gp2_conjloop: alloca failed";
    return -1;
  }
#ifdef DEBUG
  printf("loop called with %d equations and eps of %f\n",size,eps);
#endif
  
  forever {
    /* CONJUGATE GRADIENT LOOP */
    
    if (iters++ > 2*size)
      {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: too many iterations\n");
#endif
	return -1;
      }
    
#ifdef DEBUG
    printf("ITERATION %d\n",iters);
    printf("alpha is ");vdump(alpha,size);
#endif
    
    
    { /* compute gradient */
      
      int i,j;
      flt sum;
      flt *ppi;
      
      for  (i=0; i<size; i++) {
	sum = kp[polar[i]];
	ppi = pp[polar[i]];
	for (j=0; j<size; j++)
	  sum -= alpha[j] * ppi[polar[j]] * dpt[i][j];
	grad[i] = sum;
      }
    }
    
#ifdef DEBUG
    printf("gradient is");vdump(grad,size);
#endif
    
    {   /* compute W */ 
      int i;
      twow = 0;
      for (i=0; i<size; i++)
	twow += ( grad[i] + kp[polar[i]] ) * alpha[i];
      
#ifdef DEBUG2
      if (gp2->lasttwow > twow + SMALL_FLT)
	printf("gp2->lasttwow = %f > twow=%f\n",gp2->lasttwow ,twow);
#endif
      gp2->lasttwow = twow;
      
#ifdef DEBUG    
      printf("2W is %g\n",twow);
#endif
      
      /* Test for resolvability */
      
#ifdef DEBUG
      printf("resolve or not %f times  %f is %f \n",twow,rho,twow*rho);
#endif
      if (twow*rho > 1 ) {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: too large W\n");
#endif
	return -2;
      }
    }
    
    
    {   /* Make the gradient positive
	   Compute its norm
	   Check its norm against epsilon */
      
      int i;
      gradnorm = 0;
      for (i=0;i<size;i++)
	if (alpha[i]!=0 || grad[i]>0)
	  gradnorm += grad[i]*grad[i];
	else
	  grad[i] = 0;
#ifdef DEBUG
      printf("positive gradient norm is %g\n",gradnorm);
#endif
      if (gradnorm<eps)
	{
#ifdef DEBUG2
	  printf("number of iterations was %d\n",iters);
#endif
	  break;
	}
    }
    
    
    {   /* Compute conjugate gradient */
      if (lastgradnorm!=0) {
	int i;
	flt ratio;
	ratio = gradnorm/lastgradnorm;
	for (i=0;i<size;i++)
	  conjdir[i] = grad[i] + ratio*conjdir[i];
      } else {
	int i;
	for (i=0;i<size;i++) 
	  conjdir[i] = grad[i];
      }
      lastgradnorm = gradnorm;
    }
    
#ifdef DEBUG	
    printf("conjugate direction is ");vdump(conjdir,size);
#endif
    
    {   /* Compute step size */
      
      int i,j;
      flt zaz,ostep;
      flt ppalphaj[2];
      
      /* Compute the clipping constraints */
      
      step = LARGE_FLT;
      for (i=0;i<size;i++) {
	if (conjdir[i]<0) {
	  ostep = -alpha[i]/conjdir[i];
	  if (ostep < step)
	    step = ostep;
	}
      }
      
      /* Compute standard conjugate gradient step size */
      
      zaz = 0;		/* Compute zAz */
      for (j=0; j<size; j++) {
	zaz += 0.5 * conjdir[j]*conjdir[j]*dpt[j][j];
	ppalphaj[0] =  pp[polar[j]][0] * conjdir[j];
	ppalphaj[1] =  pp[polar[j]][1] * conjdir[j];
	for (i=0; i<j; i++)
	  zaz += conjdir[i]*ppalphaj[polar[i]]*dpt[j][i];
      }
      zaz *= 2;
      
#ifdef DEBUG	
      printf("zAz is %g\n",zaz);
#endif
      
      if (zaz<SMALL_FLT) {	
	/* if zAz is zero (or <zero), and no clip, it fails */
	if (step==LARGE_FLT) {
#ifdef DEBUG2
	  printf( "gp2_conjloop: failure: Hessian is singular\n");
#endif
	  return -3;
	}
      } else {		
	ostep=0;
	for(i=0;i<size;i++)
	  ostep+=conjdir[i]*grad[i];
	ostep/=zaz;
#ifdef DEBUG	
	printf("conjugate step is %g\n",ostep);
	if (step!=LARGE_FLT) printf("best clip step is %g\n",step);
#endif
	if (ostep<step) 
	  step = ostep;	/* if optimal jump ok, accept it */
      }

      if (step*gradnorm>QUITELARGE_FLT) {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: Hessian looks non negative\n");
#endif
	return -3;
      }
    }
    
#ifdef DEBUG  
    printf("step is %g\n",step);
#endif
    
    {   /* update alpha */
      int i;
      for (i=0;i<size;i++)
	alpha[i] += conjdir[i]*step;
    }
    
    
    {    /* Remove those points whose alpha 
	    is smaller than SMALL_FLT */
      int i;
      for (i=size-1;i>=0;i--)
	if (alpha[i]<SMALL_FLT) {
#ifdef DEBUG
	  printf("CLIPPING dimension %d\n",i);
#endif
	  size -= 1;
	  gp2_shrink(gp2,i);
	  lastgradnorm = 0;
	  iters = 0;
	}
    }
    
  } /* forever */
  
  return size;
}




/* ======================================================= */

/* Perform the elementary optimal hyperplane loop */


/* Returns number of support points or
 *   -1 if failure because too many iterations,
 *   -2 if failure because W is too large,
 *   -3 if failure because Hessian non negative
 */

int gp2_ophyperloop(gp2, rho, eps)
struct gp2 *gp2;
flt rho;
flt eps;
{
  flt k0 = 1;
  flt k1 = -1;
  flt lastgradnorm = 0;
  flt gradnorm;
  flt twow;
  flt step;
  flt corr;
  int iters=0;
  int size = gp2->size;
  
  flt *conjdir = (flt*)alloca( sizeof(flt)*size);
  flt *grad = (flt*)alloca( sizeof(flt)*size);
  flt *alpha = gp2->alpha;
  int *polar = gp2->polarity;
  flt **dpt;
  
  flt kp[2];
  static flt pp[2][2] = { 1, -1, -1, 1};
  static flt p[2]= { 1, -1};
  
  gp2_error = NULL;
  gp2_check_dpt(gp2);
  dpt = gp2->dpt;
  if (gp2_error)
    return;
  
  /* Compute kp */
  kp[0] = k0;
  kp[1] = -k1;
  
  /* Adjust rho and eps */
  rho = rho * rho / (k0-k1) / (k0-k1);
  eps = eps * eps;
  
  
#ifdef DEBUG
  printf("rho %f\n",rho);
#endif
  
  ifn (conjdir && grad) {
    gp2_error = "gp2_conjloop: alloca failed";
    return -1;
  }


  forever {
    /* OPTIMAL HYPERPLANE LOOP */
    
    if (iters++ > 2*size)
      {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: too many iterations\n");
#endif
	return -1;
      }
    
#ifdef DEBUG
    printf("ITERATION %d\n",iters);
    printf("alpha is ");vdump(alpha,size);
#endif
    
#ifdef DEBUG
    { /*check the constraint*/
      flt sum = 0;
      int i;
      flt alpha_sum = 0;
      
      for (i=0; i< gp2->size; i++)
	{
	  sum +=  p[polar[i]]* alpha[i];
	  alpha_sum +=  alpha[i];
	}
      alpha_sum/=gp2->size;
      if ((sum<=-alpha_sum) || (sum >= alpha_sum)) 
	printf("sum is %f and alpha-mean is %g\n", sum, alpha_sum);
    }
#endif
    
    
    { /* compute gradient */
      
      int i,j;
      flt sum;
      flt *ppi;
      
      for  (i=0; i<size; i++) {
	sum = kp[polar[i]];
	ppi = pp[polar[i]];
	for (j=0; j<size; j++)
	  sum -= alpha[j] * ppi[polar[j]] * dpt[i][j];
	grad[i] = sum;
      }
    }
    
    
#ifdef DEBUG
    printf("gradient is");vdump(grad,size);
#endif
    
    {   /* compute W */ 
      int i;
      twow = 0;
      for (i=0; i<size; i++)
	twow += ( grad[i] + kp[polar[i]] ) * alpha[i];
      
#ifdef DEBUG2
      if (gp2->lasttwow > twow + SMALL_FLT)
	printf("gp2->lasttwow = %f > twow=%f\n",gp2->lasttwow ,twow);
#endif
      
      gp2->lasttwow = twow;
      
#ifdef DEBUG    
      printf("2W is %g\n",twow);
#endif
      
      /* test for resolvability */
      if (twow*rho > 1 ) {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: too large W\n");
#endif
	return -2;
      }
    }
    
    
    {   /* Make the gradient positive 
	   and insure that sum-alpha = sum-beta */
      
      int i;
      int hits;			/* number of blocked gradients */
      int lasthits;		/* number of blocked last time */
      flt tcorr;		/* total correction for projecting */
      flt corr;			/* average correction for projecting */
      flt ch;			/* the suggested change of the gradient */

      hits = 0;
      do {
	
	tcorr = 0;		/* compute the remaining correction */
	for (i = 0;i<size;i++)
	  tcorr += p[polar[i]]*grad[i];
	corr = tcorr / (size-hits);
	
	lasthits = hits;	/* apply it... */
	hits = 0;
	for (i=0; i<size; i++) {
	  ch = p[polar[i]]*corr;
	  if (alpha[i]==0 && grad[i]<ch) {
	    grad[i] = 0;
	    hits += 1;
	  } else {
	    grad[i] -= ch;
	  }
	}
	
      } while (lasthits < hits);
    }
    
    {   /* Compute its norm
	   Check its norm against epsilon */
      
      int i;
      gradnorm = 0;
      for (i=0;i<size;i++)
	gradnorm += grad[i]*grad[i];
      
#ifdef DEBUG
      printf("constrained gradient norm is %g\n",gradnorm);
#endif
      if (gradnorm<eps) {
#ifdef DEBUG2
	printf("number of iterations was %d\n",iters);
#endif
	break;
      }
    }
    
    
    {   /* Compute conjugate gradient */
      if (lastgradnorm!=0) {
	int i;
	flt ratio;
	ratio = gradnorm/lastgradnorm;
	for (i=0;i<size;i++)
	  conjdir[i] = grad[i] + ratio*conjdir[i];
      } else {
	int i;
	for (i=0;i<size;i++) 
	  conjdir[i] = grad[i];
      }
      lastgradnorm = gradnorm;
    }
    
    
#ifdef DEBUG	
    printf("conjugate direction is ");vdump(conjdir,size);
#endif
    
    {   /* Compute step size */
      
      int i,j;
      flt zaz,ostep;
      flt ppalphaj[2];
      
      /* Compute the clipping constraints */
      
      step = LARGE_FLT;
      for (i=0;i<size;i++) {
	if (conjdir[i]<0) {
	  ostep = -alpha[i]/conjdir[i];
	  if (ostep < step)
	    step = ostep;
	}
      }
      
      /* Compute standard conjugate gradient step size */
      
      zaz = 0;		/* Compute zAz */
      for (j=0; j<size; j++) {
	zaz += 0.5 * conjdir[j]*conjdir[j]*dpt[j][j];
	ppalphaj[0] =  pp[polar[j]][0] * conjdir[j];
	ppalphaj[1] =  pp[polar[j]][1] * conjdir[j];
	for (i=0; i<j; i++)
	  zaz += conjdir[i]*ppalphaj[polar[i]]*dpt[j][i];
      }
      zaz *= 2;
      
#ifdef DEBUG	
      printf("zAz is %g\n",zaz);
#endif
      
      if (zaz<SMALL_FLT) {	
	/* if zAz is zero (or <zero), and no clip, it fails */
	if (step>QUITELARGE_FLT) {
#ifdef DEBUG2
	  printf( "gp2_conjloop: failure: Hessian is non negative\n");
#endif
	  return -3;
	}
      } else {		
	ostep=0;
	for(i=0;i<size;i++)
	  ostep+=conjdir[i]*grad[i];
	ostep/=zaz;
#ifdef DEBUG	
	printf("conjugate step is %g\n",ostep);
	if (step!=LARGE_FLT) 
	  printf("best clip step is %g\n",step);
#endif
	if (ostep<step) 
	  step = ostep;	/* if optimal jump ok, accept it */
      }

      if (step*gradnorm>QUITELARGE_FLT) {
#ifdef DEBUG2
	printf( "gp2_conjloop: failure: Hessian looks non negative\n");
#endif
	return -3;
      }
    }
    
#ifdef DEBUG  
    printf("step is %g\n",step);
#endif
    
    {   /* update alpha */
      int i;
      for (i=0;i<size;i++)
	alpha[i] += conjdir[i]*step;
    }
    
    
    {    /* Remove those points whose alpha 
	    is smaller than SMALL_FLT */
      int i;
      for (i=size-1;i>=0;i--)
	if (alpha[i]<SMALL_FLT) {
#ifdef DEBUG
	  printf("CLIPPING dimension %d\n",i);
#endif
	  size -= 1;
	  gp2_shrink(gp2,i);
	  lastgradnorm = 0;
	  iters = 0;
	}
    }
    
  } /* forever */
  
  return size;
}


/* ======================================================= */

/* Compute the threshold */

flt gp2_threshold(gp2)
struct gp2 *gp2;
{
  int *pol;
  flt **set;
  int i0, i1, n;
  
  n = gp2->size;
  pol = gp2->polarity;
  set = gp2->set;
  
  i0=0;
  while (pol[i0]!=0)
    if (i0++>=n) {
      gp2_error = "No positive examples in working set";
      return 0;
    }
  i1=0;
  while (pol[i1]!=1)
    if (i1++>=n) {
      gp2_error = "No negative examples in working set";
      return 0;
    }
  return (gp2_run(gp2,set[i0]) + gp2_run(gp2,set[i1])) / 2;
}



/* ======================================================= */

/* Finding a function by its name */

struct gp2_ftable *gp2_find_product(name)
char *name;
{
  struct gp2_ftable *tb;
  tb = gp2_ftable;
  while (tb->name) {
    ifn (strcmp(name,tb->name))
      return tb;
    tb++;
  }
  return NULL;
}




/* Saving and loading gp2 objects... */

#define GP2_MAGIC    (long)0x1f3d4c51L


void gp2_write(gp2,f)
struct gp2 *gp2;
FILE *f;
{
  int i;
  int dim = gp2->dim;
  int size = gp2->size;
  int magic = GP2_MAGIC;
  
  if (   (fwrite(&magic,sizeof(int),1,f) != 1)
      || (fwrite(gp2,sizeof(struct gp2),1,f) != 1)
      || (fwrite(gp2->index,sizeof(int),size,f) != size)
      || (fwrite(gp2->alpha,sizeof(flt),size,f) != size)
      || (fwrite(gp2->polarity,sizeof(int),size,f) != size) ) {
    gp2_error = "Cannot write";
    return;
  }
  for (i=0;i<size;i++)
    if (fwrite(gp2->set[i],sizeof(flt),dim,f) != dim) {
      gp2_error = "Cannot write";
      return;
    }
  gp2_error = NULL;
}



void gp2_read(gp2,f,minsize)
struct gp2 *gp2;
FILE *f;
int minsize;
{
  struct gp2 gp;
  struct gp2_ftable *tb;
  int magic;
  int i,dim,size;
  flt (*func)();

  gp2_error = NULL;
  
  ifn (fread(&magic,sizeof(int),1,f) == 1) {
    gp2_error = "Cant read magic number";
    return;
  }
  ifn (magic == GP2_MAGIC) {
    gp2_error = "Bad magic number";
    return;
  }
  ifn (fread(&gp,sizeof(struct gp2),1,f) == 1) {
    gp2_error = "Cant read header";
    return;
  }
  tb = gp2_find_product(gp.funcname);
  ifn (tb) {
    sprintf(string_buffer,"Unknown inner product: %s",gp.funcname);
    gp2_error = string_buffer;
    return;
  }
  func = tb->func;
  size = gp.size;
  dim = gp.dim;
  
  gp2_free(gp2);
  gp2_alloc(gp2,((size>minsize)?size:minsize),dim,func);
  
  strncpy(gp2->funcname,gp.funcname,GP2NAMELEN);
  gp2->funcname[GP2NAMELEN-1] = 0;
  for(i=0;i<GP2NUMARGS;i++)
    gp2->funcarg[i] = gp.funcarg[i];
  
  if (   (fread(gp2->index,sizeof(int),size,f) != size)
      || (fread(gp2->alpha,sizeof(flt),size,f) != size)
      || (fread(gp2->polarity,sizeof(int),size,f) != size) ) {
    gp2_error = "Cant read data";
    return;
  }
  for (i=0; i<size;i++)
    if (fread(gp2->set[i],sizeof(int),dim,f) != dim) {
      gp2_error = "Cant read data";
      return;
    }
  
  gp2->lasttwow = -1e38;
  gp2->dpt_flag = 0;
  gp2->size = size;
}



  


/* ================================================ */
/* =============== DOT PRODUCTS =================== */
/* ================================================ */



flt gp2_dot(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
  int n = gp2->dim;
  int i;
  flt sum=0;
  for(i=0;i<n;i++)
    sum += *x++ * *y++;
  return sum;
}

flt gp2_dot1(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=1;
    for(i=0;i<n;i++)
      sum += *x++ * *y++;
    return sum;
}

flt gp2_dot2(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=0;
    for(i=0;i<n;i++)
      sum += *x++ * *y++;
    return sum*sum;
}

flt gp2_dot21(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=1;
    for(i=0;i<n;i++)
      sum += *x++ * *y++;
    return sum*sum;
}

flt gp2_dot3(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=0;
    for(i=0;i<n;i++)
      sum += *x++ * *y++;
    return sum*sum*sum;
}

flt gp2_dot31(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=1;
    for(i=0;i<n;i++)
      sum += *x++ * *y++;
    return sum*sum*sum;
}




flt gp2_poly(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
  int n = gp2->dim;
  int i;
  flt sum;
  flt prod;

  /* compute XY+1 */
  sum = 1;
  for(i=0;i<n;i++)
    sum += *x++ * *y++;

  /* raise it to the nth power */
  n = gp2->funcarg[0];
  prod = sum;
  for (i=1;i<n;i++)
    prod *= sum;

  return prod;
}


flt gp2_rbf(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt sum=0;
    flt dif;

    for(i=0;i<n;i++) {
      dif = *x++ - *y++;
      sum += dif*dif;
    }
    return exp( (double)(-sum/gp2->funcarg[0]) );
}

flt gp2_poly_rbf(x,y,gp2)
flt *x;
flt *y;
struct gp2 *gp2;
{
    int n = gp2->dim;
    int i;
    flt dst=0;
    flt dot=1;
    flt dif;

    /* compute distance and dot-product+1 */
    for(i=0;i<n;i++) {
      dif = *x - *y;
      dst += dif*dif;
      dot += *x * *y;
      x++;
      y++;
    }

    /* take the exponential */
    dst = exp( (double)(-dst/gp2->funcarg[1]) );
    
    /* raise it to the nth power */
    n = gp2->funcarg[0];
    for (i=0;i<n;i++)
      dst *= dot;
    return dst;
}


/* =========================================== */
/* =========== INNER PRODUCT NAMES =========== */
/* =========================================== */



struct gp2_ftable gp2_ftable[] = {

    "dot", 		gp2_dot,
    "dot1",		gp2_dot1,
    "dot2",		gp2_dot2,
    "dot21",		gp2_dot21,
    "dot3",		gp2_dot3,
    "dot31",		gp2_dot31,
    "poly",             gp2_poly,
    "rbf",              gp2_rbf,
    "poly-rbf",         gp2_poly_rbf,

    "poly(0)",          gp2_poly,    	/* Historical alias name. */
    NULL, NULL,				/* Mandatory last entry.  */
};









/* =========================================== */
/* ============ LISP INTERFACE =============== */
/* =========================================== */





/* an new kind of object: GP2 */

void gp2_dispose(p)
at *p;
{
  struct gp2 *gp2 = p->Object;
  gp2_free(gp2);
  free(gp2);
}

class gp2_class = {
  gp2_dispose,
  generic_action,
  generic_name,
  generic_eval,
  generic_listeval,
  XT_GP2,
};



/* 
 * (new-gp2 <maxsize> <dim> <funcname> <..args..>) 
 * Create a GP2 
 */


DX(xnew_gp2)
{
  struct gp2 *gp2;
  struct gp2_ftable *tb;
  
  char *name;
  int maxsize, dim, i;
  
  
  if (arg_number<3)
    ARG_NUMBER(3);
  if (arg_number>=3+GP2NUMARGS)
    ARG_NUMBER(-1);

  ALL_ARGS_EVAL;
  
  maxsize = AINTEGER(1);
  dim = AINTEGER(2);
  name = ASTRING(3);
  
  /* find the function */
  tb = gp2_find_product(name);
  ifn (tb)
    error(NIL,"Function not found",APOINTER(3));
  
  /* check args */
  if (dim<1 || maxsize<1)
    error(NIL,"illegal args",NIL);
  
  /* allocate and go */
  gp2 = (struct gp2*)malloc( sizeof(struct gp2) );
  ifn (gp2)
    error(NIL,"malloc failed",NIL);
  gp2_alloc(gp2,maxsize,dim,tb->func);
  if (gp2_error)
    error(NIL,gp2_error,NIL);

  /* clear and set the args */
  for (i=0; i<GP2NUMARGS; i++)
    gp2->funcarg[i] = 0;
  for (i=4; i<=arg_number; i++)
    gp2->funcarg[i-4] = AFLT(i);
  
  /* sets the name */
  strncpy(gp2->funcname,tb->name,GP2NAMELEN);
  gp2->funcname[GP2NAMELEN-1] = 0;

  return new_extern(&gp2_class,gp2,XT_GP2);
}


/*********************************************/


#define AGP2(n) (gp2_intf_agp2(arg_array,n))

struct gp2 *gp2_intf_agp2(arg_array,n)
at **arg_array;
int n;
{
  at *p;
  p = arg_array[n];
  ifn (p && p->ctype==XT_GP2)
    error(NIL,"Not a GP2 object",p);
  return p->Object;
}



/*********************************************/

DX(xgp2_write)
{
  struct gp2 *gp2;
  at *p;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  p = APOINTER(2);
  ifn (p && p->ctype ==XT_FILE_WO)
    error(NIL,"Not a write-only file",p);
  gp2_write(gp2,p->Object);
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NIL;
}

DX(xgp2_read)
{
  struct gp2 *gp2;
  at *p;

  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  p = APOINTER(2);
  ifn (p && p->ctype ==XT_FILE_RO)
    error(NIL,"Not a read-only file",p);
  gp2_read(gp2,p->Object,AINTEGER(3));
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NIL;
}


/*********************************************/


DX(xgp2_get_w)
{
  struct gp2 *gp2;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  return NEW_NUMBER(gp2->lasttwow/2);
}

/* (gp2-get-size <gp2>)
 * Returns the size of <gp2>
 */

DX(xgp2_get_size)
{
  struct gp2 *gp2;

  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  return NEW_NUMBER(gp2->size);
}


/* (gp2-reset <gp2)
 * Remove all elements of <gp2>.
 */


DX(xgp2_reset)
{
  struct gp2 *gp2;

  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  gp2->size = 0;
  gp2->lasttwow = -1e38;
  return NIL;
}


/* 
 * (gp2-get-index <gp2>)
 * Returns a matrix the indexes 
 */


DX(xgp2_get_index)
{
  struct gp2 *gp2;
  at *p;
  struct index *pa;
  int i,size;
  flt *pt;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  size = gp2->size;
  
  p = F_matrix(1,&size);
  pa = p->Object;

  pt = (flt*)(pa->st->srg.data) + pa->offset;
  for (i=0; i<size; i++) {
    *pt = gp2->index[i];
    pt += pa->mod[0];
  }
  return p;
}


/* 
 * (gp2-get-alpha <gp2>)
 * Returns a matrix the alphas
 */

DX(xgp2_get_alpha)
{
  struct gp2 *gp2;
  at *p;
  struct index *pa;
  int i,size;
  flt *pt;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  size = gp2->size;
  
  p = F_matrix(1,&size);
  pa = p->Object;

  pt = (flt*)(pa->st->srg.data) + pa->offset;
  for (i=0; i<size; i++) {
    *pt = gp2->alpha[i];
    pt += pa->mod[0];
  }
  return p;
}


/* 
 * (gp2-get-alpha <gp2>)
 * Returns a matrix the alphas
 */

DX(xgp2_get_polarity)
{
  struct gp2 *gp2;
  at *p;
  struct index *pa;
  int i,size;
  flt *pt;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  size = gp2->size;
  
  p = F_matrix(1,&size);
  pa = p->Object;

  pt = (flt*)(pa->st->srg.data) + pa->offset;
  for (i=0; i<size; i++) {
    *pt = gp2->polarity[i];
    pt += pa->mod[0];
  }
  return p;
}

/* 
 * (gp2-get-set <gp2>)
 * Returns a matrix of the active examples
 */

DX(xgp2_get_set)
{
  struct gp2 *gp2;
  at *p;
  struct index *pa;
  int i,j,size[2];
  flt *pt0, *pt1;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  size[0] = gp2->size;
  size[1] = gp2->dim;
  
  p = F_matrix(2,size);
  pa = p->Object;

  pt0 = (flt*)(pa->st->srg.data) + pa->offset;
  for (i=0; i<size[0]; i++) {
    pt1 = pt0;
    for (j=0; j<size[1]; j++) {
      *pt1 = gp2->set[i][j];
      pt1 += pa->mod[1];
    }
    pt0 += pa->mod[0];
  }
  return p;
}


/* 
 * (gp2-get-dpt <gp2>)
 * Returns a matrix of the dotproducts for the example
 */

DX(xgp2_get_dpt)
{
  struct gp2 *gp2;
  at *p;
  struct index *pa;
  int i,j,size[2];
  flt *pt0, *pt1;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  
  gp2_check_dpt(gp2);
  if (gp2_error)
    error(NIL,gp2_error,NIL);

  size[0] = gp2->size;
  size[1] = gp2->size;
  
  p = F_matrix(2,size);
  pa = p->Object;

  pt0 = (flt*)(pa->st->srg.data) + pa->offset;
  for (i=0; i<size[0]; i++) {
    pt1 = pt0;
    for (j=0; j<size[1]; j++) {
      *pt1 = gp2->dpt[i][j];
      pt1 += pa->mod[1];
    }
    pt0 += pa->mod[0];
  }
  return p;
}



/*********************************************/

/* 
 * (gp2-grow <gp2> <pat> <pol> <alpha> <index>)
 * Add an example to a gp2
 */

DX(xgp2_grow)
{
  struct gp2 *gp2;
  struct index *ex;
  flt *pt;
  
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  ex = AINDEX(2);
  
  if (ex->ndim!=1 || ex->dim[0]!=gp2->dim)
    error(NIL,"Matrix dimensions are wrong",APOINTER(2));

  if (ex->mod[0]!=1)
      error(NIL,"need a contiguous vector",APOINTER(2));

  pt = (flt*)(ex->st->srg.data) + ex->offset;
  gp2_grow(gp2, pt, AINTEGER(3), AFLT(4), AINTEGER(5) );
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(gp2->size);
  
}

/*********************************************/

/* 
 * (gp2-shrink <gp2> <n>)
 * Remove example number <n> from <gp2>
 */

DX(xgp2_shrink)
{
  struct gp2 *gp2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  gp2 = AGP2(1);
  gp2_shrink(gp2,AINTEGER(2));
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(gp2->size);
}



/*********************************************/


/*
 * (gp2-run <gp2> <x>)
 */

DX(xgp2_run)
{
  struct gp2 *gp2;
  struct index *ex;
  flt *pt;
  flt result;


  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  
  gp2 = AGP2(1);
  ex = AINDEX(2);

  if (ex->ndim!=1 || ex->dim[0]!=gp2->dim)
    error(NIL,"Matrix dimensions are wrong",APOINTER(2));

  if (ex->mod[0]!=1)
      error(NIL,"need a contiguous vector",APOINTER(2));

  pt = (flt*)(ex->st->srg.data) + ex->offset;
  result = gp2_run(gp2,pt);
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(result);
}



/*********************************************/



/* Returns number of support points or
 *   -1 if failure because too many iterations,
 *   -2 if failure because W is too large,
 *   -3 if failure because Hessian is singular
 */

DX(xgp2_conjloop)
{
  int result;
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  result = gp2_conjloop(AGP2(1),AFLT(2),AFLT(3),AFLT(4),AFLT(5));
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(result);
}



/*********************************************/

/* Returns number of support points or
 *   -1 if failure because too many iterations,
 *   -2 if failure because W is too large,
 *   -3 if failure because Hessian is singular
 */

DX(xgp2_ophyperloop)
{
  int result;
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  result = gp2_ophyperloop(AGP2(1),AFLT(2),AFLT(3));
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(result);
}


/*********************************************/

/* Return the threshold for classification */

DX(xgp2_threshold)
{
  flt result;
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  result = gp2_threshold(AGP2(1));
  if (gp2_error)
    error(NIL,gp2_error,NIL);
  return NEW_NUMBER(result);
}


/*********************************************/


void init_gp2()
{
  class_define("GP2",&gp2_class);
  dx_define("new-gp2",xnew_gp2);
  dx_define("gp2-read",xgp2_read);
  dx_define("gp2-write",xgp2_write);

  dx_define("gp2-reset",xgp2_reset);

  dx_define("gp2_get_w",xgp2_get_w);
  dx_define("gp2-get-size",xgp2_get_size);
  dx_define("gp2-get-index",xgp2_get_index);
  dx_define("gp2-get-alpha",xgp2_get_alpha);
  dx_define("gp2-get-polarity",xgp2_get_polarity);
  dx_define("gp2-get-set",xgp2_get_set);
  dx_define("gp2-get-dpt",xgp2_get_dpt);

  dx_define("gp2-grow",xgp2_grow);
  dx_define("gp2-shrink",xgp2_shrink);

  dx_define("gp2-run",xgp2_run);

  dx_define("gp2-conjloop",xgp2_conjloop);
  dx_define("gp2-ophyperloop",xgp2_ophyperloop);

  dx_define("gp2-threshold",xgp2_threshold);
}




int majver_gp2 = 3;
int minver_gp2 = 1;


