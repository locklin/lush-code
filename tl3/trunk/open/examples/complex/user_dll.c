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
 
/************************************************************/
/************************************************************/
/**                                                        **/
/**   TL/OPEN EXAMPLE: COMPLEX                             **/
/**      Please refer to the TL/Open manual for            **/
/**      additional information                            **/
/**                                                        **/
/************************************************************/
/************************************************************/



/* --- HEADER SECTION --- 
 *
 * This section loads the appropriate header files
 * and make sure that the correct libraries will be loaded.
 * (WIN32 details are specific to Visual C++ 4.x.)
 */

#include "tlopen.h"








/* --- DECLARATIONS --
 * 
 * This section contains type declarations
 * that pertain to all the C file.
 */



/* COMPLEX -- internal representation of a complex number */

typedef struct { 
  real re; 
  real im;
} COMPLEX;



/* EPSILON -- small double */

#define EPSILON 1e-15




/* complex_alloc -- fast allocation of complex structures */

/* ADVANCED TLOPEN:
 * TLisp provides a very fast memory allocator for
 * managing a large number of objects with the same size.
 * Both functions <allocate()> and <deallocate()> take
 * a pointer to an <alloc_root> structure as argument.
 * This structure specifies the size of the objects and
 * how many objects will be preallocated from the
 * system.
 */

static struct alloc_root complex_alloc = 
{
  NULL,
  NULL,
  sizeof(COMPLEX),      /* objects have this size ... and  */
  200                   /* are allocated by chunks of 200. */
};



/* -- forward declaration of utility functions */

static void get_complex(at*, COMPLEX*);
static at *new_complex(real, real);







/* --- CLASS DEFINITION ---
 *
 * This section contains the functions that define
 * the class of the new user defined type COMPLEX.
 * See TL/Open manual for more details.
 */



/* complex_dispose -- destroys object */

static void
complex_dispose(at *p)
{
  /* return memory to the allocator */
  deallocate(&complex_alloc, p->Object);
}



/* complex_action -- follows <at> pointers 
 *
 * Since there are no <at> pointers in struct <COMPLEX>,
 * we will use the default function <generic_action>
 */



/* complex_name -- text representation of object */

static char *
complex_name(at *p)
{
  COMPLEX *cp = (COMPLEX*)(p->Object);
  static char buffer[80];
  /* ADVANCED TLOPEN:
   * Function <str_number> returns a compact textual representation
   * of a <real> that do not requires a fixed format (like <sprintf>)
   */
  strcpy(buffer,"#{");
  strcat(buffer, str_number(cp->re));
  strcat(buffer,",");
  strcat(buffer, str_number(cp->im));
  strcat(buffer,"}");
  return buffer;
}



/* complex_eval -- evaluation of object
 *
 * Numbers (real or complex) evaluate as themselves.
 * We can use the default function <generic_eval>.
 */



/* complex_listeval -- evaluation of list starting with object
 *
 * Lists starting with numbers (real or complex) cannot be evaluated.
 * We can use the default function <generic_listeval>.
 */



/* complex_serialize -- convert object to/from byte stream */

static void
complex_serialize(at **pp, int code)
{
  COMPLEX *cp;
  /* create complex if needed */
  if (code==SRZ_READ)
    *pp = new_complex(1.0, 1.0);  /* Danger: args must be non zero */
  /* access complex */
  cp = (COMPLEX*)((*pp)->Object);
  /* serialize component (assume real==double) */
  serialize_double(&cp->re, code);
  serialize_double(&cp->im, code);
}



/* complex_compare -- compare complex numbers */

static int 
complex_compare(at *p, at *q, int order)
{
  COMPLEX *cp, *cq;
  if (order)
    error(NIL,"Cannot rank complex numbers",NIL);
  cp = (COMPLEX*)(p->Object);
  cq = (COMPLEX*)(q->Object);
  if (cp->re == cq->re)
    if (cp->im == cq->im)
      return 0;
  return 1;
}



/* complex_hash -- hash complex number */

static unsigned long 
complex_hash(at *p)
{
  int i;
  unsigned long x = 0;
  unsigned long *cl;

  /* Just mix up the bits of the real numbers */
  cl = (unsigned long*) (COMPLEX*) (p->Object);
  for ( i=0; i<sizeof(COMPLEX)/sizeof(unsigned long); i++ )
  {
    x = x ^ cl[i];
    x = (x<<6) | ((x&0xfc000000)>>26);
  }
  return x;
}



/* complex_class -- class definition for complex objects */

static class complex_class = 
{
  complex_dispose,
  generic_action,
  complex_name,
  generic_eval,
  generic_listeval,
  complex_serialize,
  (void*)complex_compare,  /* We cast to (void*) because      */
  (void*)complex_hash      /*    VC++4.1 warns for no reason. */
};





/* --- UTILITIES ---
 *
 * This section contains a utility functions that
 * are useful for implementing the primitives.
 */



/* new_complex -- creates a complex number <at> */

static at *
new_complex(real re, real im)
{
  if (im == 0.0)
  {
    /* Always return real when possible */
    return NEW_NUMBER(re);
  }
  else
  {
    /* Allocate a complex */
    COMPLEX *cp = allocate(&complex_alloc);
    cp->re = re;
    cp->im = im;
    return new_extern(&complex_class, cp);
  }
}



/* get_complex -- converts an <at> into a <COMPLEX> */

static void
get_complex (at *p, COMPLEX *cp)
{
  if (NUMBERP(p))
  {
    cp->re = p->Number;
    cp->im = 0;
  }
  else if (EXTERNP(p, &complex_class))
  {
    cp->re = ((COMPLEX*)(p->Object))->re;
    cp->im = ((COMPLEX*)(p->Object))->im;
  }
  else
  {
    error(NIL,"Not a number (real or complex)", p);
  }
}



/* cartesian_to_polar -- converts complex representation */

static void 
cartesian_to_polar(real re, real im, real *r, real *theta)
{
  real n,t;
  /* Compute norm (beware overflow) */
  if (re==0 && im==0) {
    n = 0;
  } else if (fabs(re) > fabs(im)) {
    n = im / re;
    n = fabs(re) * sqrt(1 + n*n);
  } else {
    n = re / im;
    n = fabs(im) * sqrt(1 + n*n);
  }
  /* Compute argument */
  t = 0;
  if (n > 0) {
    t = acos(re / n);
    if (im < 0)
      t = -t;
  }
  /* Return */
  *r = n;
  *theta = t;
}


/* polar_to_cartesian -- convert complex representation */

static void
polar_to_cartesian(real r, real theta, real *re, real *im)
{
  real c = cos(theta);
  real s = sin(theta);
  real sa = fabs(s);
  real ca = fabs(c);
  /* Remove meaningless decimals */
  if (sa<EPSILON && sa*2>fabs(theta))
    s = 0;
  if (ca < EPSILON)
    c = 0;
  /* Return */
  *re = r * c;
  *im = r * s;
}




/* --- PRIMITIVE SECTION --- 
 *
 * This section contains your new TL3 primitives.
 * You may also choose to implement them in auxilliary files.
 */



/* (complex [<re>] <im>) -- create complex number */

DX(xcomplex)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  return new_complex(AREAL(1),AREAL(2));
}



/* (re <complex>) -- real part of complex number */

DX(xre)
{
  COMPLEX a;
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  get_complex(APOINTER(1),&a);
  return NEW_NUMBER(a.re);
}



/* (im <complex>) -- imaginary part of complex number */

DX(xim)
{
  COMPLEX a;
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  get_complex(APOINTER(1),&a);
  return NEW_NUMBER(a.im);
}




/* (complex+ ...) -- addition of complex numbers */

DX(xcomplexadd)
{
  int i = 0;
  COMPLEX sum = {0,0};
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
  {
    COMPLEX tmp;
    get_complex(APOINTER(i), &tmp);
    sum.re += tmp.re;
    sum.im += tmp.im;
  }
  return new_complex(sum.re, sum.im);
}



/* (complex* ...) -- multiplication of complex numbers */

DX(xcomplexmul)
{
  int i = 0;
  COMPLEX prod = {1,0};
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
  {
    COMPLEX a,b;
    a = prod;
    get_complex(APOINTER(i), &b);
    prod.re = a.re * b.re - a.im * b.im;
    prod.im = a.re * b.im + a.im * b.re;
  }
  return new_complex(prod.re, prod.im);
}

  

/* (complex- ...) -- subtraction of complex numbers */

DX(xcomplexsub)
{
  COMPLEX a1, a2;
  if (arg_number == 1) 
  {
    ARG_EVAL(1);
    get_complex(APOINTER(1), &a1);
    return new_complex(-a1.re, -a1.im);
  } 
  else 
  {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    get_complex(APOINTER(1), &a1);
    get_complex(APOINTER(2), &a2);
    return new_complex(a1.re-a2.re, a1.im-a2.im);
  }
}



/* (complex/ ...) -- division of complex numbers */

DX(xcomplexdiv)
{
  real norm;
  COMPLEX a1, a2;

  if (arg_number == 1) 
  {
    ARG_EVAL(1);
    get_complex(APOINTER(1), &a1);
    norm = a1.re * a1.re + a1.im * a1.im;
    if (norm == 0)
      error(NIL,"Division by zero", NIL);
    return new_complex(a1.re/norm, -a1.im/norm);
  } 
  else 
  {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    get_complex(APOINTER(1), &a1);
    get_complex(APOINTER(2), &a2);
    /* This code may lead to overflows */
    norm = a2.re * a2.re + a2.im * a2.im;
    if (norm == 0)
      error(NIL,"Division by zero", NIL);
    return new_complex((a1.re*a2.re + a1.im*a2.im)/norm,
                       (a1.im*a2.re - a1.re*a2.im)/norm );
  }
}



/* (complex1+ <a>) -- add one to complex number */

DX(xcomplexadd1)
{
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  return new_complex(a.re + 1.0, a.im);
}



/* (complex1- <a>) -- subtract one to complex number */

DX(xcomplexsub1)
{
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  return new_complex(a.re - 1.0, a.im);
}



/* (complex2* <a>) -- multiply complex number by two */

DX(xcomplexmul2)
{
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  return new_complex(a.re * 2.0, a.im * 2.0);
}



/* (complex2/ <a>) -- divide complex number by two */

DX(xcomplexdiv2)
{
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  return new_complex(a.re / 2.0, a.im / 2.0);
}



/* (complex-to-polar <a>) -- returns polar coordinates */

DX(xcomplex2polar)
{
  COMPLEX a;
  real r, theta;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  cartesian_to_polar(a.re, a.im, &r, &theta);
  return cons(NEW_NUMBER(r), cons(NEW_NUMBER(theta), NIL));
}



/* (polar-to-complex <r> <theta>) -- return complex */

DX(xpolar2complex)
{
  real r, theta, re, im;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  r = AREAL(1);
  theta = AREAL(2);
  polar_to_cartesian(r,theta,&re,&im);
  return new_complex(re,im);
}


/* (cnorm <a>) -- norm of complex number */

DX(xcnorm)
{
  real n;
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  if (a.re==0 && a.im==0) {
    n = 0;
  } else if (fabs(a.re) > fabs(a.im)) {
    n = a.im / a.re;
    n = fabs(a.re) * sqrt(1 + n*n);
  } else {
    n = a.re / a.im;
    n = fabs(a.im) * sqrt(1 + n*n);
  }
  return NEW_NUMBER(n);
}


/* (csqrt <a>) -- complex square root */

DX(xcsqrt)
{
  COMPLEX a;
  real n, re, im;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  /* Case of real numbers */
  if (a.im==0) {
    if (a.re >= 0)
      return new_complex(sqrt(a.re), 0.0);
    else
      return new_complex(0.0, sqrt(-a.re));
  }
  /* Case of complex numbers (overflow risk) */
  n = sqrt(a.re*a.re + a.im*a.im);
  re = sqrt( (n + a.re) / 2.0 );
  im = a.im / re / 2.0;
  return new_complex(re,im);
}


/* (cexp <a>) -- complex exponential */

DX(xcexp)
{
  COMPLEX a;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  polar_to_cartesian(exp(a.re),a.im,&a.re,&a.im);
  return new_complex(a.re, a.im);
}


/* (clog <a>) -- complex logarithm */

DX(xclog)
{
  COMPLEX a;
  real r, theta;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  get_complex(APOINTER(1), &a);
  cartesian_to_polar(a.re, a.im, &r, &theta);
  if (r <= 0)
    error(NIL,"Illegal argument to math function",APOINTER(1));
  return new_complex(log(r), theta);
}





/* --- INITIALISATION SECTION --- 
 * 
 * Function 'init_user_dll' is called when you load 
 * the TLOpen extension. You may :
 * -- perform your initializations here
 * -- declare your primitives here
 */


int init_user_dll(int major, int minor)
{
  /* Check version */
  if (major!=TLOPEN_MAJOR || minor<TLOPEN_MINOR)
    return -2;
  /* Declare primitives */
  class_define("COMPLEX", &complex_class);
  dx_define("complex", xcomplex);
  dx_define("re", xre);
  dx_define("im", xim);
  dx_define("complex+", xcomplexadd);
  dx_define("complex*", xcomplexmul);
  dx_define("complex-", xcomplexsub);
  dx_define("complex/", xcomplexdiv);
  dx_define("complex1+", xcomplexadd1);
  dx_define("complex1-", xcomplexsub1);
  dx_define("complex2*", xcomplexmul2);
  dx_define("complex2/", xcomplexdiv2);
  dx_define("complex-to-polar", xcomplex2polar);
  dx_define("polar-to-complex", xpolar2complex);
  dx_define("cnorm", xcnorm);
  dx_define("csqrt", xcsqrt);
  dx_define("cexp", xcexp);
  dx_define("clog", xclog);
  return 0;
}
