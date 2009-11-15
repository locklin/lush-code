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
 *  This file is derived from SN-2.8
 *    Copyright (C) 1987-1999 Neuristique s.a.
 ***********************************************************************/

/***********************************************************************
 * $Id: nlf.c,v 1.2 2005/06/03 04:10:09 leonb Exp $
 **********************************************************************/



#include "defn.h"


#define Parms u.parms
#define Splin u.splin

#define TYPE_STD 1
#define TYPE_SPLINE 2

struct nlf *nlf,*dnlf,*ddnlf;

/* ------ NLF ALLOCATION STRUCTURE ------- */

struct alloc_root nlf_alloc =
{	NULL,
	NULL,
	sizeof(struct nlf),
	10
};


/* --------- NLF CLASS FUNCTIONS -------------- */


static void 
nlf_dispose(at *p)
{
	struct nlf *n;

	n = Mptr(p);
	if (n->type == TYPE_SPLINE)
	  free(n->Splin.x);
	deallocate( &nlf_alloc, (void*)n );
}
	
static void 
nlf_action(at *p, void (*action)(at*))
{
	struct nlf *n;
	n = p->Object;
	(*action)(n->atf);
}

static at*
nlf_listeval(at *p, at *q)
{
  struct nlf *n;
  at *ans;

  n = p->Object;
  q = q->Cdr;
  ifn (CONSP(q) && !Cdr(q))
    error(NIL,"One argument only",q);
  q = eval(Car(q));
  ifn (q && NUMBERP(q))
    error(NIL,"Not a number",q);

  ans = NEW_NUMBER(Ftor((*(n->f))(n,rtoF(Number(q)))));
  return ans;
}

static flt f_0(struct nlf *n, float x);
static flt f_lisp(struct nlf *n, float x);
static flt f_bell(struct nlf *n, float x);
static flt df_bell(struct nlf *n, float x);
static flt f_tanh(struct nlf *n, float x);
static flt df_tanh(struct nlf *n, float x);
static flt f_lin(struct nlf *n, float x);
static flt f_piecewise(struct nlf *n, float x);
static flt f_threshold(struct nlf *n, float x);
static flt f_spline(struct nlf *n, float xx);
static flt df_spline(struct nlf *n, float xx);
static flt ddf_spline(struct nlf *n, float xx);
static flt df_all(struct nlf *n, float xx);
static flt ddf_all(struct nlf *n, float xx);

void
nlf_serialize(at **pp, int code)
{
  static flt (*ftable[])(struct nlf*, flt) =  { 
    f_0, f_lisp, f_bell, df_bell, f_tanh, df_tanh, 
    f_lin, f_piecewise, f_threshold, 
    f_spline, df_spline, ddf_spline, 
    df_all, ddf_all, 0L 
  };
  
  struct nlf *n;
  short func;
  
  /* OBJECT CREATION */
  if (code == SRZ_READ) {
    n = allocate( &nlf_alloc );
  } else {
    n = Mptr(*pp);
    for (func=0; ftable[func]!=n->f ; func++)
      if (!ftable[func])
	error(NIL,"Internal error (corrupted NLF)",NIL);      
  }
  /* FIELD "F" */
  serialize_short(&func, code);
  if (func<0 || func >= sizeof(ftable)/sizeof(ftable[0]))
    error(NIL,"Serialization error (func id expected in NLF)",NIL);
  n->f = ftable[func];
  
  /* FIELD "ATF" */
  serialize_atstar(&n->atf, code);
  
  /* FIELD "TYPE" */
  serialize_short(&n->type, code);
  
  /* TYPE DEPENDEND FIELDS */
  if (n->type == TYPE_STD)
    {
      serialize_float(&n->Parms.A, code);
      serialize_float(&n->Parms.B, code);
      serialize_float(&n->Parms.C, code);
      serialize_float(&n->Parms.D, code);
      serialize_float(&n->Parms.P1, code);
      serialize_float(&n->Parms.P2, code);
    }
  else if (n->type == TYPE_SPLINE)
    {
      int i, size;
      
      /* FIELD "SIZE" */
      serialize_int(&n->Splin.size, code);
      size = n->Splin.size;
      
      /* FIELDS "X,Y,Y2" */
      if (code == SRZ_READ) {
	ifn (n->Splin.x = calloc( 3*size, sizeof(flt) ) )
	  error(NIL,"no room for a new spline",NIL);
	n->Splin.y  = n->Splin.x + size;
	n->Splin.y2 = n->Splin.y + size;
      }
      for (i=0; i<size; i++)
	serialize_float(&n->Splin.x[i], code);
      for (i=0; i<size; i++)
	serialize_float(&n->Splin.y[i], code);
      for (i=0; i<size; i++)
	serialize_float(&n->Splin.y2[i], code);
    }
  else
    error(NIL,"Serialization error (bad type field in NLF)",NIL);

  /* FINISH CREATION */
  if (code == SRZ_READ)
    *pp = new_extern(&nlf_class,n);
}




class nlf_class =
{	
  nlf_dispose,
  nlf_action,
  generic_name,
  generic_eval,
  nlf_listeval,
  nlf_serialize,
};



/* ------- TRIVIAL NLF FUNCS ----------- */


/* f_0
 * Returns always zero		Base function is F(x) = 0
 */
static flt 
f_0(struct nlf *n, float x)
{
  return Fzero;
}

DX(xnlf_f_0)
{
  struct nlf *n;
  ARG_NUMBER(0);
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->atf = NIL;
  n->f = f_0;
  return new_extern(&nlf_class,n);
}



/* ------- LISP NLF FUNCS ----------- */


/* f_lisp
 *  calls a lisp function
 */
static flt 
f_lisp(struct nlf *n, float x)
{
  at *call,*result;

  call = cons(new_number(Ftor(x)),NIL);
  result = apply(n->atf,call);
  x = rtoF(Number(result));
  return x;
}

DX(xnlf_f_lisp)
{
  struct nlf *n;
  at *p;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  ifn (FUNCTIONP(p))
    error(NIL,"not a function",p);
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->atf = p;
  n->f = f_lisp;
  return new_extern(&nlf_class,n);
}




/* ------- BELL NLF FUNCS ----------- */


static flt
f_bell(struct nlf *n, float x)
{
  flt y;
  if (x >= Fone)
    return Fzero;
  if (x <= Fminus)
    return Fzero;
  y = Fsub(Fone,Fmul(x,x));
  y = Fmul(y, Fmul(y,y));
  return y;
}

DX(xnlf_f_bell)
{
  struct nlf *n;
  ARG_NUMBER(0);
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = Fzero;
  n->Parms.B = Fzero;
  n->Parms.C = Fzero;
  n->Parms.D = Fzero;
  n->atf = NIL;
  n->f = f_bell;
  return new_extern(&nlf_class,n);
}



static flt
df_bell(struct nlf *n, float x)
{
  flt y;
  if (x >= Fone)
    return Fzero;
  if (x <= Fminus)
    return Fzero;
  y = Fsub(Fone,Fmul(x,x));
  y = Fmul(Fmul(y, y), x);
  return Fmul( rtoF(-6.0), y);
}

DX(xnlf_df_bell)
{
  struct nlf *n;
  ARG_NUMBER(0);
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = Fzero;
  n->Parms.B = Fzero;
  n->Parms.C = Fzero;
  n->Parms.D = Fzero;
  n->atf = NIL;
  n->f = df_bell;
  return new_extern(&nlf_class,n);
}





/* ------- TANH NLF FUNCS ----------- */



/* f_tanh
 * Sigmoidal function		Base function is F(x) = tanh(x)
 */
static flt 
f_tanh(struct nlf *n, float x)
{
	flt y;
	
	y = Ftanh( Fmul(n->Parms.B,x) );
	y = Fadd ( Fmul(n->Parms.A,y), n->Parms.D);
	return Fadd ( Fmul(n->Parms.C,x), y);
}

DX(xnlf_f_tanh)
{
  struct nlf *n;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = AFLT(1);
  n->Parms.B = AFLT(2);
  n->Parms.C = AFLT(3);
  n->Parms.D = AFLT(4);
  n->atf = NIL;
  n->f = f_tanh;
  return new_extern(&nlf_class,n);
}


/* df_tanh
 * 'f_tanh' first derivative
 * needs :	P1 = AB+C
 *		P2 = AB
 */
static flt 
df_tanh(struct nlf *n, float x)
{
	flt y;
	
	y = Ftanh (Fmul( n->Parms.B,x ));
	y = Fmul( n->Parms.P2, Fmul(y,y) );
	return Fsub( n->Parms.P1,y ) ;
}

DX(xnlf_df_tanh)
{
  struct nlf *n;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = AFLT(1);
  n->Parms.B = AFLT(2);
  n->Parms.C = AFLT(3);
  n->Parms.D = AFLT(4);
  n->Parms.P1 = Fadd(Fmul(n->Parms.A,n->Parms.B),n->Parms.C);
  n->Parms.P2 = Fmul(n->Parms.A,n->Parms.B);
  n->atf = NIL;
  n->f = df_tanh;
  return new_extern(&nlf_class,n);
}



/* ------- LINEAR NLF FUNCS ----------- */



/* f_lin
 * linear function			Base function is F(x) = x
 * needs P1 = AB+C
 */
static flt 
f_lin(struct nlf *n, float x)
{
	flt y;
	
	y = Fmul(n->Parms.P1,x);
	return Fadd(y,n->Parms.D);
}

DX(xnlf_f_lin)
{
  struct nlf *n;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = AFLT(1);
  n->Parms.B = AFLT(2);
  n->Parms.C = AFLT(3);
  n->Parms.D = AFLT(4);
  n->Parms.P1 = Fadd(Fmul(n->Parms.A,n->Parms.B),n->Parms.C);
  n->atf = NIL;
  n->f = f_lin;
  return new_extern(&nlf_class,n);
}


/* ------- PIECEWISE NLF FUNCS ----------- */



/* f_piecewise 				
 * Base function is F(x) = if -1<x<1:  x
 *			   else:       sgn(x)
 * piecewise linear function
 */
static flt 
f_piecewise(struct nlf *n, float x)
{
	flt y;
	
	y = Fmul(n->Parms.B,x);
	if (y > Fone)
		y = Fone;
	else if (y < Fminus)
		y = Fminus; 
	y = Fadd(Fmul(n->Parms.A,y), Fmul(n->Parms.C,x));
	return Fadd(y,n->Parms.D);
}

DX(xnlf_f_piecewise)
{
  struct nlf *n;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = AFLT(1);
  n->Parms.B = AFLT(2);
  n->Parms.C = AFLT(3);
  n->Parms.D = AFLT(4);
  n->atf = NIL;
  n->f = f_piecewise;
  return new_extern(&nlf_class,n);
}




/* f_threshold
 * threshold function			Base function is F(x) = sgn(x)
 */
static flt 
f_threshold(struct nlf *n, float x)
{
	flt y;
	
	y = Fmul(x,n->Parms.B);
	if (y >= Fzero)
		y = Fone;
	else
		y = Fminus;
	y = Fadd(Fmul(n->Parms.A,y), Fmul(n->Parms.C,x));
	return Fadd(y,n->Parms.D);
}

DX(xnlf_f_threshold)
{
  struct nlf *n;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  n = allocate( &nlf_alloc );
  n->type = TYPE_STD;
  n->Parms.A = AFLT(1);
  n->Parms.B = AFLT(2);
  n->Parms.C = AFLT(3);
  n->Parms.D = AFLT(4);
  n->atf = NIL;
  n->f = f_threshold;
  return new_extern(&nlf_class,n);
}



/* -------- SPLINE FUNCTIONS -------- */

static void 
precompute_spline(struct nlf *n, at *xl, at *yl)
{
    int i;
    flt *u,*x,*y,*y2,sig,p;
    int size;

    size = length(xl);
    if (length(yl) != size)
	error(NIL,"x and y listes have different length",NIL);

    n->type = TYPE_SPLINE;
    n->Splin.size = size;
    n->atf = NIL;

    ifn (n->Splin.x = calloc( 3*size, sizeof(flt) ) )
	error(NIL,"no room for a new spline",NIL);

    n->Splin.y  = n->Splin.x + size;
    n->Splin.y2 = n->Splin.y + size;

    for (u = n->Splin.x; CONSP(xl); xl=xl->Cdr)
        if ( Car(xl) && NUMBERP(Car(xl)) )
	    *u++ = rtoF( Number(Car(xl)) );
	else
	    error(NIL,"not a number",xl->Car);

    for (u = n->Splin.y; CONSP(yl); yl=Cdr(yl))
        if ( Car(yl) && NUMBERP(Car(yl)) )
	    *u++ = rtoF( Number(Car(yl)) );
	else
	    error(NIL,"not a number", Car(yl));

/* y2 calculation */

    u = calloc( size, sizeof(flt) );
    x = n->Splin.x - 1;
    y = n->Splin.y - 1;
    y2 = n->Splin.y2 - 1;

    y2[1] = y2[size] = u[1] = Fzero;
    for( i=2; i < size; i++) {
	sig = Fdiv( Fsub(x[i],x[i-1]), Fsub(x[i+1],x[i-1]) );
	p = Fadd( Fmul(sig,y2[i-1]), Ftwo );
	y2[i] = Fdiv( Fsub(sig,Fone), p );
	u[i] = Fsub(Fdiv( Fsub(y[i+1],y[i]), Fsub(x[i+1],x[i]) ),
		    Fdiv( Fsub(y[i],y[i-1]), Fsub(x[i],x[i-1]) ) );
	u[i] = Fdiv( Fsub( Fdiv( Fmul(rtoF(6.0),u[i]), Fsub(x[i+1],x[i-1]) ),
		           Fmul(sig,u[i-1]) ),
                     p );
    }
    for( i=size-1; i > 0; i-- ) {
	y2[i] = Fadd( Fmul(y2[i],y2[i+1]), u[i]);
    }
    free(u);
}

/* f_spline
 * spline interpolation
 */
static flt 
f_spline(struct nlf *n, float xx)
{
        int k,klo,khi,size;
	flt *x,*y,*y2;
	flt a,b,c,d,h;

	x = n->Splin.x - 1;
	y = n->Splin.y - 1;
	y2 = n->Splin.y2 - 1;
	size = n->Splin.size;

	klo = 1;
	khi = size;

	while( khi-klo > 1 ) {
	    k = (khi+klo)>>1;
	    if (x[k]>xx)
		khi = k;
	    else
		klo = k;
	}
	h = Fsub( x[khi],x[klo] );
	if (h <= Fzero)
	    error(NIL,"unordered X values in the spline",NIL);
	
        if (klo == 1 && xx < x[klo]) {
	    a = Fsub(xx,x[klo]);
	    b = Fdiv( Fsub( y[khi],y[klo] ), h);
	    return Fadd( y[klo], Fmul(b,a));
	} else if ( khi == size && xx > x[khi] ) {
	    a = Fsub(xx,x[khi]);
	    b = Fdiv( Fsub( y[khi],y[klo] ), h);
	    return Fadd( y[khi], Fmul(b,a));
	} else {
	    a = Fdiv( Fsub( x[khi],xx ) , h);
	    b = Fsub( Fone, a);
	    h = Fdiv( Fmul(h,h),rtoF(6.0) );
	    c = Fmul( Fsub( Fmul(a,Fmul(a,a)),a), h );
	    d = Fmul( Fsub( Fmul(b,Fmul(b,b)),b), h );
	    return Fadd( Fadd( Fmul(a,y[klo]), Fmul(b,y[khi]) ),
			 Fadd( Fmul(c,y2[klo]), Fmul(d, y2[khi]) ) );
	}

}

DX(xnlf_f_spline)
{
  struct nlf *n;
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  n = allocate( &nlf_alloc );
  n->f = f_spline;
  precompute_spline(n,APOINTER(1),APOINTER(2));
  return new_extern(&nlf_class,n);
}

/* df_spline
 *  spline interpolation
 */
static flt 
df_spline(struct nlf *n, float xx)
{
        int k,klo,khi,size;
	flt *x,*y,*y2;
	flt a,b,c,h;

	x = n->Splin.x - 1;
	y = n->Splin.y - 1;
	y2 = n->Splin.y2 - 1;
	size = n->Splin.size;

	klo = 1;
	khi = size;

	while( khi-klo > 1 ) {
	    k = (khi+klo)>>1;
	    if (x[k]>xx)
		khi = k;
	    else
		klo = k;
	}
	h = Fsub( x[khi],x[klo] );
	c = Fdiv( Fsub( y[khi],y[klo]), h);
	if (h <= Fzero)
	    error(NIL,"unordered X values in the spline",NIL);
	
        if (klo == 1 && xx < x[klo]) {
	    return c;
	} else if ( khi == size && xx > x[khi] ) {
	    return c;
	} else {
	    a = Fdiv( Fsub( x[khi],xx ) , h);
	    b = Fsub( Fone, a);

	    h = Fdiv( h,rtoF(6.0) );
	    a = Fsub( Fmul( rtoF(3.0),Fmul( a,a )) , Fone );
	    a = Fmul( Fmul(a,h), y2[klo] );
	    b = Fsub( Fmul( rtoF(3.0),Fmul( b,b )) , Fone );
	    b = Fmul( Fmul(b,h), y2[khi] );
	    
	    return( Fadd(c, Fsub(b,a)) );
	}
}

DX(xnlf_df_spline)
{
  struct nlf *n;
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  n = allocate( &nlf_alloc );
  n->f = df_spline;
  precompute_spline(n,APOINTER(1),APOINTER(2));
  return new_extern(&nlf_class,n);
}


/* ddf_spline
 * Cubic spline interpolation
 */
flt ddf_spline(struct nlf *n, float xx)
{
        int k,klo,khi,size;
	flt *x,*y,*y2;
	flt a,b,h;

	x = n->Splin.x - 1;
	y = n->Splin.y - 1;
	y2 = n->Splin.y2 - 1;
	size = n->Splin.size;

	klo = 1;
	khi = size;

	while( khi-klo > 1 ) {
	    k = (khi+klo)>>1;
	    if (x[k]>xx)
		khi = k;
	    else
		klo = k;
	}
	h = Fsub( x[khi],x[klo] );
	if (h <= Fzero)
	    error(NIL,"unordered X values in the spline",NIL);

        if (klo == 1 && xx < x[klo]) {
	    return Fzero;
	} else if ( khi == size && xx > x[khi] ) {
	    return Fzero;
	} else {
	    a = Fdiv( Fsub( x[khi],xx ) , h);
	    b = Fsub( Fone, a);

	    return( Fadd( Fmul(a,y2[klo]), Fmul(b,y2[khi]) ) );
	}
}
DX(xnlf_ddf_spline)
{
  struct nlf *n;
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  n = allocate( &nlf_alloc );
  n->f = ddf_spline;
  precompute_spline(n,APOINTER(1),APOINTER(2));
  return new_extern(&nlf_class,n);
}


/* ------- FINITE DIFFERENCE DERIVATIVES -------- */



/* df_all
 * Computes the first derivative with finite difference method 
 */
static flt 
df_all(struct nlf *n, float x)
{
	flt dy,dx,dx2;
	
	n = Mptr(n->atf);
	dx = rtoF(1.0/256.0);
	dx2 = rtoF(1.0/128.0);
	
	dy = Fsub( (*(n->f))(n,Fadd(x,dx)) , (*(n->f))(n,Fsub(x,dx)) );
	return Fdiv( dy, dx2 );
}

DX(xnlf_df_all)
{
  struct nlf *n;
  at *p;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  ifn  (p && EXTERNP(p) && (Class(p)==&nlf_class))
    error(NIL,"not a nlf",p);
  n = allocate(&nlf_alloc);
  n->type = TYPE_STD;
  n->f = df_all;
  n->atf = p;
  return new_extern(&nlf_class,n);
}



/* ddf_all
 * Computes the second derivative with finite difference method
 */
static flt 
ddf_all(struct nlf *n, float x)
{
	flt dy,dx,dx2;
	
	n = Mptr(n->atf);
	dx  = rtoF(1.0/128.0);
	dx2 = rtoF(1.0/16384.0);
	
	dy = Fadd( (*(n->f))(n,Fsub(x,dx)), (*(n->f))(n,Fadd(x,dx)) );
	dy = Fsub( dy, Fmul(Ftwo, (*(n->f))(n,x)) );
	dy = Fdiv( dy, dx2 );
	return dy;
}

DX(xnlf_ddf_all)
{
  struct nlf *n;
  at *p;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  ifn  (p && EXTERNP(p) && (Class(p)==&nlf_class))
    error(NIL,"not a nlf",p);
  n = allocate(&nlf_alloc);
  n->type = TYPE_STD;
  n->f = ddf_all;
  n->atf = p;
  return new_extern(&nlf_class,n);
}


/* --------- INITIALISATION CODE --------- */


void init_nlf(void)
{
  class_define("NLF",&nlf_class );
  
  dx_define("nlf-f-0",xnlf_f_0);
  dx_define("nlf-f-lisp",xnlf_f_lisp);
  dx_define("nlf-f-tanh",xnlf_f_tanh);
  dx_define("nlf-df-tanh",xnlf_df_tanh);
  dx_define("nlf-f-bell",xnlf_f_bell);
  dx_define("nlf-df-bell",xnlf_df_bell);
  dx_define("nlf-f-lin",xnlf_f_lin);
  dx_define("nlf-f-piecewise",xnlf_f_piecewise);
  dx_define("nlf-f-threshold",xnlf_f_threshold);
  dx_define("nlf-f-spline",xnlf_f_spline);
  dx_define("nlf-df-spline",xnlf_df_spline);
  dx_define("nlf-ddf-spline",xnlf_ddf_spline);
  dx_define("nlf-df-all",xnlf_df_all);
  dx_define("nlf-ddf-all",xnlf_ddf_all);
}
