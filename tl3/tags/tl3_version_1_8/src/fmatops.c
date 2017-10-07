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
 
/***********************************************************************
  TL3: (C) LYB YLC 1988
  matrix.c
  - array and matrix class definition
  $Id: fmatops.c,v 1.2 2005-11-14 19:00:24 leonb Exp $
********************************************************************** */

#include "header.h"

static at * mindex_compute (at *m0, at *mans);
static at * mindex_apply (at *midx, at *m0, at *mans);
static void index_apply (int n,
			 int *idx     , int modi,
			 flt *f1_start, int mod1,
			 flt *f2      , int mod2,
			 flt *mbuf    , int mod );
static at * mindex_inv (at *midx, at *mans);
static at * mseek (at *m0, flt x, int way, at *midx);
static at * mset (at * v1, at * v2, at * ans);

static void msort_vector (int n, int modn, flt * data, int modi,
			  int * idata);
static void qsort_mod_idx (float * arr, int * idx, int n,
		    int mod, int modi);
static int moccur (at * v1, flt val1, flt valans, at * va);
static void moccur_op (flt * datum1, flt * datum2, flt val, flt valans, int *num);
static void nan_moccur_op (flt * datum1, flt * datum2, flt val, flt valans, int *num);
static int matrix_sweep_and_exec (int ndim_still, int *pdim,
				  int *pmod1, ptr pdata1,
				  int *pmod2, ptr pdata2,
				  flt x, flt y, int *pcount,
				  void (*op) (flt *, flt *, flt, flt, int *));

static int   m_buffer_size=0; /* bytes */
static flt * m_buffer=0;



static void buffer_check(flt **pbuffer, int *psize, int reqsize)
{
  if( *psize < reqsize ) {
    if( ! *psize ) *psize = 4096; /* bytes */
    while( *psize < reqsize ) {
      *psize *= 2;
    }
    if( *pbuffer ) free( *pbuffer );
    *pbuffer = tl_malloc( *psize );
  }
}



/* --------- MATRIX CALCULUS --------- */

/*
 * m+m m-m m*m
 * m*c m+c
 *
 * convolve
 * dot-product
 * trace transpose
 *
 * mean-mat sdev-mat inf-mat sup-mat
 * 
 */


/* MISC STUFFS */


struct array *
check_matrix(at *p, int *n, int *m)
{
  struct array *arr;

  ifn(p && (p->flags & X_ARRAY)) {
    error(NIL, "not a matrix", p);
  }
  ifn(p->Class == &matrix_class) {
    error(NIL, "fmatrix only are allowed", p);
  }
  arr = p->Object;

  if (arr->ndim == 1) {
    arr->dim[1] = 1;
    arr->modulo[1] = 0;
  } else if ( m ) {
    if ( arr->ndim != 2 ) error(NIL, "two dimensions matrix expected", p);
  } else if ( n ) {
    ifn ( arr->ndim == 1 || (arr->ndim == 2 && arr->dim[1] == 1) ) {
      error(NIL, "one dimension vector expected", p);
    }
  }
  if ((n && *n && (arr->dim[0] != *n)) ||
      (m && *m && (arr->dim[1] != *m))) {
    error(NIL, "wrong matrix size", p);
  }
  if( n ) *n = arr->dim[0];
  if( m ) *m = arr->dim[1];
  return arr;
}


struct array *
check_vector(at *p, int *n)
{
  return check_matrix(p,n,(void *)0);
}


struct array *
answer_matrix(at **ans, int *n, int *m)
{
  int dim[2];

  if (*ans) {
    LOCK(*ans);
  } else {
    if (!*n || !*m)
      error(NIL, "internal error: answer_matrix : unknown size", NIL);
    dim[0] = *n;
    dim[1] = *m;
    *ans = (dim[1] == 1) ? matrix(1, dim) : matrix(2, dim);
    if (*n < 10 && *m < 10)
      ((struct array *) ((*ans)->Object))->flags |= PRINT_ARRAY;
  }
  return check_matrix(*ans, n, m);
}

struct array *
answer_vector(at **ans, int *n)
{
  if (*ans) {
    LOCK(*ans);
  } else {
    if (!*n)
      error(NIL, "internal error: answer_vector : unknown size", NIL);
    *ans = matrix(1, n);
    if (*n < 10)
      ((struct array *) ((*ans)->Object))->flags |= PRINT_ARRAY;
  }
  return check_vector(*ans, n);
}



static at *
mdiv(at *v1, at *ans, int flag, flt x)
{
  int n = 0;
  int m = 0;
  register int i;
  register flt *f1, *fa;
  flt *ff1, *ffa;
  struct array *vv1, *vva;
  int v1m0,v1m1,vam0,vam1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    fa = ffa;
    while (i--) {
      *fa = (flag && (*f1 == Fzero)) ? x : Fdiv(Fone, *f1);
      fa += vam1;
      f1 += v1m1;
    }
    ffa += vam0;
    ff1 += v1m0;
  }
  return ans;
}

DX(xmdiv)
{
  at *p3=0,*p2=0;
  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 3:
    p3 = APOINTER(3);
  case 2:
    p2 = APOINTER(2);
  case 1:
    return mdiv(APOINTER(1), p2, (p3 ? 1 : 0), (p3 ? AFLT(3) : Fzero));
  default:
    ARG_NUMBER(-1);
    return NIL;
  }
}

static at *
mdop(double (*dop)(double), at *v1, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *fa;
  flt *ff1, *ffa;
  struct array *vv1, *vva;
  int v1m0,v1m1,vam0,vam1;

  if( ! dop )
    error(0,"unknown operator",0);

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    fa = ffa;
    while (i--) {
      *fa = dtoF(dop(Ftod(*f1)));
      fa += vam1;
      f1 += v1m1;
    }
    ffa += vam0;
    ff1 += v1m0;
  }
  return ans;
}


DX(xmsqrt)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(sqrt, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmexp)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(exp, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmlog)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(log, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmcos)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(cos, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmsin)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(sin, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmtan)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(tan, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmasin)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(asin, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmacos)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(acos, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmatan)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(atan, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmsinh)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(sinh, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmcosh)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(cosh, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}

DX(xmtanh)
{
  ALL_ARGS_EVAL;
  if( arg_number != 1 && arg_number != 2)
    ARG_NUMBER(-1);
  return mdop(tanh, APOINTER(1), (arg_number >= 2) ? APOINTER(2) : 0);
}




/* M+M, aM+bM, M-M, MxM */

static at *
maddm(at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;
  
  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = Fadd(*f1, *f2);
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmaddm)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return maddm(APOINTER(1), APOINTER(2), NIL);
    case 3:
      return maddm(APOINTER(1), APOINTER(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}


static at *
mlinm(float a1, at *v1, float a2, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;
  
  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = Fadd(Fmul(a1,*f1), Fmul(a2,*f2));
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmlinm)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 4:
      return mlinm(AFLT(1), APOINTER(2), AFLT(3), APOINTER(4), NIL);
    case 5:
      return mlinm(AFLT(1), APOINTER(2), AFLT(3), APOINTER(4), APOINTER(5));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}


static at *
mbary(float a1, at *v1, float a2, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt sum;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;
  
  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  sum = a1 + a2;
  a1 /= sum;
  a2 /= sum;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = (isnanF(*f1)) ? *f2 : (isnanF(*f2)) ? *f1 :
	Fadd(Fmul(a1,*f1), Fmul(a2,*f2));
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmbary)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 4:
      return mbary(AFLT(1), APOINTER(2), AFLT(3), APOINTER(4), NIL);
    case 5:
      return mbary(AFLT(1), APOINTER(2), AFLT(3), APOINTER(4), APOINTER(5));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}


static at *
msubm(at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;

  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = Fsub(*f1, *f2);
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmsubm)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return msubm(APOINTER(1), APOINTER(2), NIL);
    case 3:
      return msubm(APOINTER(1), APOINTER(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}



static at *
mmulm(at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;

  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = Fmul(*f1, *f2);
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmmulm)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return mmulm(APOINTER(1), APOINTER(2), NIL);
    case 3:
      return mmulm(APOINTER(1), APOINTER(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}

static at *
mdivm(at *v1, at *v2, at *ans, int flag, flt x)
{
  int n = 0;
  int m = 0;
  register int i;
  register flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;

  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa = (flag && (*f2 == Fzero)) ? x : Fdiv(*f1, *f2);
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmdivm)
{
  at *p3=0,*p4=0;
  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 4:
    p4 = APOINTER(4);
  case 3:
    p3 = APOINTER(3);
  case 2:
    return mdivm(APOINTER(1), APOINTER(2), p3, (p4 ? 1 : 0),
		 (p4 ? (NUMBERP(p4) ? AFLT(4) : Fzero) : Fzero) );
  default:
    ARG_NUMBER(-1);
    return NIL;
  }
}

static at *
mmulaccm(at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa;
  struct array *vv1, *vv2, *vva;

  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_matrix(v2, &n, &m);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ff2 = vv2->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      *fa += Fmul(*f1, *f2);
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

DX(xmmulaccm)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return mmulaccm(APOINTER(1), APOINTER(2), NIL);
    case 3:
      return mmulaccm(APOINTER(1), APOINTER(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}


/* M*M */


static at *
mdotm(at *v1, at *v2, at *ans)
{
  int n = 0;
  int k = 0;
  int m = 0;
  int i, j;
  flt *p1, *p2, *fa;
  struct array *vv1, *vv2;
  flt *ffa, *f1, *f2;
  struct array *vva;
  flt sum;
  at *nans;
  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;
  /* Check arguments */
  vv1 = check_matrix(v1, &n, &k);
  vv2 = check_matrix(v2, &k, &m);
  vva = answer_matrix(&ans, &n, &m);
  /* Allocate temp matrix if matrices overlap */
  nans = ans;
  if (vva->flags & SUB_ARRAY) 
    nans = vva->main_array; 
  if (vv1->flags & SUB_ARRAY) 
    v1 = vv1->main_array;
  if (vv2->flags & SUB_ARRAY) 
    v2 = vv2->main_array;
  if (nans==v1 || nans==v2) {
    nans = matrix(2,vva->dim);
    vva = nans->Object;
  } else {
    nans = NIL;
  }
  /* Gather iteration data */
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];
  /* Iterate */
  f1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    fa = ffa;
    f2 = vv2->data;
    while (i--) {
      sum = Fzero;
      p1 = f1;
      p2 = f2;
      j = k;
      while (j--) {
	sum = Fadd(sum, Fmul(*p1, *p2));
	p1 += v1m1;
	p2 += v2m0;
      }
      *fa = sum;
      fa += vam1;
      f2 += v2m1;
    }
    f1 += v1m0;
    ffa += vam0;
  }
  /* Copy temp matrix if necessary */
  if (nans)
  {
    copy_any_matrix(nans,ans);
    UNLOCK(nans);
    nans = NIL;
  }
  return ans;
}

DX(xmdotm)
{
  at *p3=0;
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 3:
      p3=APOINTER(3);
    case 2:
      return mdotm(APOINTER(1), APOINTER(2), p3);
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}


/* M*D */

static at *
mdotd(at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  struct array *vv1, *vv2;
  flt *ffa, *fa, *ff1, *f1, *f2;
  struct array *vva;

  int vam0,v1m0,v2m0;
  int vam1,v1m1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_vector(v2, &m);
  v2m0 = vv2->modulo[0];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    f2 = vv2->data;
    fa = ffa;
    while (i--) {
      *fa = (*f1) * (*f2);
      f1 += v1m1;
      f2 += v2m0;
      fa += vam1;
    }
    ff1 += v1m0;
    ffa += vam0;
  }
  return ans;
}

DX(xmdotd)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return mdotd(APOINTER(1), APOINTER(2), NIL);
    case 3:
      return mdotd(APOINTER(1), APOINTER(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}



/* M+VVT */

static at *
maddvvt(at *v1, at *v2, at *v3, at *ans)
{
  int n = 0;
  int i,j;
  flt *f1, *f2, *f3, *fa;
  flt *ff1, *ffa;
  struct array *vv1, *vv2, *vv3, *vva;
  
  int vam0,v1m0,v2m0,v3m0;
  int vam1,v1m1;

  vv1 = check_matrix(v1, &n, &n);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vv2 = check_vector(v2, &n);
  v2m0 = vv2->modulo[0];
  vv3 = check_vector(v3, &n);
  v3m0 = vv3->modulo[0];
  vva = answer_matrix(&ans, &n, &n);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;

  f2 = vv2->data;

  j = n;
  while (j--) {
    i = n;
    f1 = ff1;
    f3 = vv3->data;
    fa = ffa;
    while (i--) {
      *fa = Fadd(*f1, Fmul(*f2,*f3));
      fa += vam1;
      f1 += v1m1;
      f3 += v3m0;
    }
    ffa += vam0;
    ff1 += v1m0;
    f2 += v2m0;
  }
  return ans;
}

DX(xmaddvvt)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 3:
      return maddvvt(APOINTER(1), APOINTER(2), APOINTER(3), NIL);
    case 4:
      return maddvvt(APOINTER(1), APOINTER(2), APOINTER(3), APOINTER(4));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}



/* M+C M*C */


static at *
maddc(at *v1, float c, at *ans)
{
  int n = 0;
  int m = 0;
  int i;

  flt *f1, *fa;
  flt *ff1, *ffa;
  struct array *vv1, *vva;
  int v1m0,v1m1,vam0,vam1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    fa = ffa;
    while (i--) {
      *fa = Fadd(*f1, c);
      fa += vam1;
      f1 += v1m1;
    }
    ffa += vam0;
    ff1 += v1m0;
  }
  return ans;
}

DX(xmaddc)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return maddc(APOINTER(1), AFLT(2), NIL);
    case 3:
      return maddc(APOINTER(1), AFLT(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}



static at *
mdotc(at *v1, float c, at *ans)
{
  int n = 0;
  int m = 0;
  int i;
  flt *f1, *fa;
  flt *ff1, *ffa;
  struct array *vv1, *vva;
  int v1m0,v1m1,vam0,vam1;

  vv1 = check_matrix(v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];
  vva = answer_matrix(&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (n--) {
    i = m;
    f1 = ff1;
    fa = ffa;
    while (i--) {
      *fa = Fmul(*f1, c);
      fa += vam1;
      f1 += v1m1;
    }
    ffa += vam0;
    ff1 += v1m0;
  }
  return ans;
}

DX(xmdotc)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 2:
      return mdotc(APOINTER(1), AFLT(2), NIL);
    case 3:
      return mdotc(APOINTER(1), AFLT(2), APOINTER(3));
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
}



/* CONVOLVE */


static at *
convolve(at *v1, int sx, int sy, at *v2, at *ans)
{
  int m0 = 0;
  int m1 = 0;
  int k0 = 0;
  int k1 = 0;
  int r0 = 0;
  int r1 = 0;
  int i, j, k;
  int vam0,v1m0,v2m0;
  int vam1,v1m1,v2m1;
  int v1m1sx,v1m0sy;

  struct array *vv1, *vv2;
  struct array *vva;
  flt *f1, *fa, *ff1, *ffa;
  flt *kkptr, *mmptr;
  flt *kptr,*mptr, point;

  vv1 = check_matrix(v1, &m0, &m1);
  v1m0 = vv1->modulo[0];
  v1m0sy = v1m0*sy;
  v1m1 = vv1->modulo[1];
  v1m1sx = v1m1*sx;
  vv2 = check_matrix(v2, &k0, &k1);
  v2m0 = vv2->modulo[0];
  v2m1 = vv2->modulo[1];

  r0 = (m0-k0+sy)/sy;
  r1 = (m1-k1+sx)/sx;

  if ( (r0*sy+k0-sy != m0) ||
       (r1*sx+k1-sx != m1) ||
       r0 < 1 || r1 < 1 )
    error(NIL,"kernel size, matrix size and step do not match",NIL);

  vva = answer_matrix(&ans, &r0, &r1);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];

  ff1 = vv1->data;
  ffa = vva->data;
  while (r0--) {
    i = r1;
    f1 = ff1;
    fa = ffa;
    while (i--) {
      kkptr = vv2->data;
      mmptr = f1;
      k = k0;
      point = Fzero;
      while (k--) {
	j = k1;
	kptr = kkptr;
	mptr = mmptr;
	while (j--) {
	  point = Fadd(point,Fmul(*kptr,*mptr));
	  kptr += v2m1;
	  mptr += v1m1;
	}
	kkptr += v2m0;
	mmptr += v1m0;
      }
      *fa = point;
      fa += vam1;
      f1 += v1m1sx;
    }
    ffa += vam0;
    ff1 += v1m0sy;
  }
  return ans;
}

DX(xconvolve)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 4:
      return convolve(APOINTER(1), AINTEGER(2), AINTEGER(3), APOINTER(4),
		      NIL);
    case 5:
      return convolve(APOINTER(1), AINTEGER(2), AINTEGER(3), APOINTER(4),
		      APOINTER(5));
    default:
      ARG_NUMBER(4);
      return NIL;
  }
}




/* TRACE */

static flt 
trace(at *mat)
{
  int n = 0;
  struct array *v;
  flt sum, *f;

  v = check_matrix(mat, &n, &n);
  f = v->data;
  sum = Fzero;

  while (n--) {
    sum = Fadd(sum, *f);
    f += v->modulo[0] + v->modulo[1];
  }
  return sum;
}

DX(xtrace)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Ftor(trace(APOINTER(1))));
}


/* DOT PRODUCT */

static flt 
dot_product(at *v1, at *v2)
{
  int n = 0;
  flt *f1, *f2, sum;
  struct array *vv1, *vv2;
  int v1m,v2m;


  vv1 = check_vector(v1, &n);
  vv2 = check_vector(v2, &n);

  sum = Fzero;
  f1 = vv1->data;
  f2 = vv2->data;
  v1m = vv1->modulo[0];
  v2m = vv2->modulo[0];
  while (n--) {
    sum = Fadd(sum, Fmul(*f1, *f2));
    f1 += v1m;
    f2 += v2m;
  }
  return sum;
}

DX(xdot_product)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  return NEW_NUMBER(Ftor(dot_product(APOINTER(1), APOINTER(2))));
}





/* INF_MAT SUP_MAT MEAN_MAT SDEV_MAT */

static flt 
inf_mat(at *mat)
{
  int i, n = 0, m = 0;
  struct array *v;
  flt ans, *f, *fa;

  v = check_matrix(mat, &n, &m);
  fa = v->data;
  ans = *fa;

  while (n--) {
    i = m;
    f = fa;
    while (i--) {
      if ( (! isnanF(*f)) && Fsgn(Fsub(*f,ans)) < 0 )
	ans = *f;
      f += v->modulo[1];
    }
    fa += v->modulo[0];
  }
  return ans;
}

DX(xinf_mat)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Ftor(inf_mat(APOINTER(1))));
}

static flt 
sup_mat(at *mat)
{
  int i, n = 0, m = 0;
  struct array *v;
  flt ans, *f, *fa;

  v = check_matrix(mat, &n, &m);
  fa = v->data;
  ans = *fa;

  while (n--) {
    i = m;
    f = fa;
    while (i--) {
      if ( (! isnanF(*f)) && Fsgn(Fsub(*f,ans)) > 0 )
	ans = *f;
      f += v->modulo[1];
    }
    fa += v->modulo[0];
  }
  return ans;
}

DX(xsup_mat)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Ftor(sup_mat(APOINTER(1))));
}

static flt 
mean_mat(at *mat)
{
  int i, n = 0, m = 0;
  struct array *v;
  flt ans, size, *f, *fa;

  v = check_matrix(mat, &n, &m);
  fa = v->data;
  ans = Fzero;

  size = Fzero;
  while (n--) {
    i = m;
    f = fa;
    while (i--) {
      if( ! isnanF(*f) ){
	ans = Fadd(ans, *f);
	size = Fadd(size,Fone);
      }
      f += v->modulo[1];
    }
    fa += v->modulo[0];
  }

  return (Fzero == size) ? getnanF(): Fdiv(ans,size) ;
}

DX(xmean_mat)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Ftor(mean_mat(APOINTER(1))));
}

static flt 
sdev_mat(at *mat)
{
  int i, n = 0, m = 0;
  struct array *v;
  flt sum, mean, tmp, size, *f, *fa;

  v = check_matrix(mat, &n, &m);
  fa = v->data;
  sum = Fzero;
  size = Fzero;
  mean = mean_mat(mat);

  while (n--) {
    i = m;
    f = fa;
    while (i--) {
      if( ! isnanF(*f) ){
	tmp = Fsub(*f,mean);
	sum = Fadd(sum,Fmul(tmp,tmp));
	size = Fadd( size, Fone );
      }
      f += v->modulo[1];
    }
    fa += v->modulo[0];
  }
  return (Fzero == size) ? getnanF(): Fsqrt( Fdiv(sum,size) );
}

DX(xsdev_mat)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Ftor(sdev_mat(APOINTER(1))));
}



/* --------- COMPLEX MATRIX OPERATIONS --------- */


/* ---------
   minvdet
   ---------
   Overwrites a fmatrix with its inverse
   and returns its determinent
   Returns 0 for singular matrix
   */


/* LUfact carries out LU factorization of aits argument TLisp matrix
   A vector is also given as argument for storing pivots.
   It returns +/- 1 according to the permutations and 0 on singularity.
   */

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;


static float 
LUfact(float *a00, int N, int mi, int mj, unsigned int *pivot)
{
  int i, j, k, imax = 0;
  float temp, sca, S, sdet;
  float *v, *ai, *aij, *aj, *aik, *akj;
  sdet = (float)1.0;	           /* C'est la qu'on met le signe de det(A) */
  v = (float *)tl_malloc(sizeof(float)*N);
				   /* Vecteur pour la normalisation         */
  ai=a00;			   /* Adresse du debut de la premiere ligne */
  for(i=0;i<N;i++) {
    sca = (float)0.0;
    aij=ai;			   /* Adresse du debut de la ligne i        */
    for(j=0;j<N;j++) {
      if((temp = (float)fabs(*aij)) > sca) sca = temp;
      aij += mj;
    }
    if (sca == 0.0) {
      free(v);
#ifdef ERROR_WHEN_SINGULAR
      error(NIL,"Matrix is singular.",NIL);
#else
      return (float)0.0;
#endif
    }
    v[i] = (float)1.0/sca;
    ai += mi;			   /* On rajoute une ligne                  */
  } 
				   /* On peut y aller maintenant            */
  aj = a00;			   /* Adresse du debut de la premiere col.  */
  for(j=0;j<N;j++) {		   /* C'est la methode de Crout (cf. NR)    */
    aij=aj;
    for(i=0;i<j;i++) {             /* Elements jusqu'a la diagonale (exclue)*/
      S = *aij;
      aik = a00 + i*mi;            /* Initialise ai0 et a0j                 */
      akj = aj;
      for(k=0;k<i;k++) {
        S -= (*aik)*(*akj);
        aik += mj;                 /* Suivant ... */
        akj += mi;                 /* Suivant ... */
      }
      *aij = S;
      aij += mi;
    }                              /* Fin de la premiere boucle sur i       */
    sca = (float)0.0;              /* Recherche du pivot                    */
    for(i=j;i<N;i++) {             /* De la diagonale jusqu'en bas          */
      S = *aij;                    /* aij est toujours bon                  */
      aik = a00 + i*mi;
      akj = aj;
      for(k=0;k<j;k++) {
        S -= (*aik)*(*akj);
        aik += mj;                 /* Next col. */
        akj += mi;                 /* Next row  */
      }
      *aij = S;                    /* La ligne i est elle notre pivot ?     */
      if ((temp=v[i]*(float)fabs(S)) > sca) {
        sca = temp;
        imax = i;
      }
      aij += mi;
    }                              /* Fin de la deuxieme boucle sur i       */
    if(imax != j) {                /* Si le pivot est dans une autre ligne  */
      aik = a00 + imax*mi;
      akj = a00 + j*mi;            /* !!: utilise akj pour _ajk_            */
      for(k=0;k<N;k++) {
        SWAP(*aik, *akj);
        aik += mj;                 /* Next col. */
        akj += mj;                 /* Next col. */
        }
      sdet = -sdet;                /* Permutation => chgt signe de det(A)   */
      v[imax]=v[j];
    }
    pivot[j] = imax;
    if(*(aj+j*mi) == 0.0) {
      free(v);
#ifdef ERROR_WHEN_SINGULAR
      error(NIL,"Matrix is singular.",NIL);
#else
      return (float)0.0;
#endif
    }
    if(j < N) {                    /* Si necessaire . . . */
      temp = (float)1.0/(*(aj+j*mi));
      aij = aj + (j+1)*mi;
      for(i=j+1;i<N;i++) {         /* ... on divise les elements sous la    */
        *aij *= temp;              /*     diagonale par le pivot            */
        aij += mi;                 /* Next row */
      }
    }
    aj += mj;                      /* Next col. */
  }
  free(v);
  return sdet;
}


#undef SWAP


/* LUsolve solves a linear system such as AX = B with:
   - A LU-factorized (see LUfact), 
   - pivot the permutation vector computed by LUfact
   - b the vector of second members (subscripted from 0)

   Variable flag is used for computing nothing
   when second member is made of 0.

   Output is stored in b.
   */

static void 
LUsolve(float *a00, int N, int mi, int mj, unsigned int *pivot, float *b)
{
  int i;
  int j, ip, id=0, flag=0;
  float S;
  float *ai, *aij;

  ai = a00;
  for(i=0;i<N;i++) {           /* On resoud d'abord L.Y = B    :-)          */
    ip = pivot[i];
    S = b[ip];                 /* Permutation selon le vecteur pivot        */
    b[ip] = b[i];
    if (flag) {
      aij = ai + id*mj;
      for(j=id;j<=i-1;j++) {        /* NOOP si id > i       */
        S -= *aij * b[j];
        aij += mj;
      }
    }
    else if (S) {              /* Sinon on regarde si l'element est nul     */
      flag = 1;
      id = i;
    }
    b[i] = S;
    ai += mi;
  }
  ai = a00 + (N-1)*mi;
  for(i=N-1;i>=0;i--) {        /* Maintenant on resoud U.X = Y              */
    S = b[i];
    aij = ai + (i+1)*mj;
    for(j=i+1;j<N;j++) {
      S -= *aij * b[j];
      aij += mj;
    }
    b[i] = S / *(ai + i*mj);   /* On divise par l'element diagonal          */
    ai -= mi;
  }
}


static double 
minvdet(float *a00, int N, int mi, int mj, int flag)
{
  int i,j;
  unsigned int *piv;
  double det = 1.0;
  float *inv, *b, *invij, *invj, *aij;

  inv = (float *)tl_malloc(sizeof(float)*N*N);
  b = (float *)tl_malloc(sizeof(float)*N);
  piv = (unsigned int *)tl_malloc(sizeof(int)*N);

  det = (double) LUfact(a00, N, mi, mj, piv);
  aij = a00;
  for(i=0;i<N;i++) {                 /* Calcul du determinant */
    det *= (double) *aij;
    aij += mi + mj;
  }
  if (det == 0.0) {
    free(inv);
    free(b);
    free(piv);
    return 0.0;
  }
  flag = 1; /* !!!! */
  if( flag ) {
    invj = inv;
    for(j=0;j<N;j++) {
      for(i=0;i<N;i++) b[i]=(float)0.0;/* Initialise le jieme vecteur B       */
      b[j] = (float)1.0;
      LUsolve(a00, N, mi, mj, piv, b); /* Resoud A.X = B                      */
      invij = invj;
      for(i=0;i<N;i++) {
	*invij = b[i];                 /* Recopie le resultat dans inv        */
	invij += mi;                   /* New row  */
      }
      invj += mj;                      /* New col. */
    }
    memcpy((void *) a00, (void *) inv, N*N*sizeof(float));
  }
  free(inv);
  free(b);
  free(piv);
  return det;
}

#ifdef COMPILE_XMLUFACT
DX(xmlufact)
{
  struct array *arr;
  int n=0;
  unsigned int *piv;
  double signe;

  ARG_NUMBER(1);
  ALL_ARGS_EVAL;

  arr = check_matrix(APOINTER(1),&n,&n);
  piv = (unsigned int *) tl_malloc(sizeof(int)*n);

  signe = LUfact(arr->data,n,arr->modulo[0],arr->modulo[1],piv);

  return NEW_NUMBER(signe);
}
#endif

DX(xminvdet)
{
  struct array *arr;
  int n=0;
  double res;
  
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;

  arr = check_matrix(APOINTER(1),&n,&n);
 
  res = minvdet(arr->data,n,arr->modulo[0],arr->modulo[1], 0);

  return NEW_NUMBER(res);
}


/* ---------
   xmseigvv
   ---------
   Overwrites a symetric fmatrix by its eigen vectors and
   returns a vector filled with the corresponding eigen values.
   Returns nil in case of problem
   Uses Householder tridiagonal reduction (hh_tdiag_red based on tred2)
   followed by QL algorithm with implicit shifts (tdiag_ql_ishift based on tqli).

   */


#define SIGN(a,b) ((b)<0 ? -(float)fabs(a) : (float)fabs(a))


/* tridiag transforms a symetric matrix into a tridiagonal
   using multiplications by matrices of householder.

   IN  : a  : data pointer
         n  : matrix size
         mi : modulo on lines
         mj : modulo on columns
   OUT : a  : products of used Householder matrices
         d  : diagonal elements of tridiagonal matrix
         e  : under/over-diagonal elements of tridiagonal matrix (e[0] = 0).
   */


static void 
tridiag(float *a, int n, int mi, int mj, float *d, float *e)
{
  int i, j, k, l;
  float scale, GdK, sig, g, f;
  float *ai, *aik, *aij, *aj, *aji, *ajk;
  
  ai = a + n * mi;                  /* une ligne apres la fin de la matrice */
  for(i=n-1;i>=1;i--) {
    ai -= mi;                       /* ligne precedente */
    l = i - 1 ;
    scale = (float)0.0 ;
    sig = (float)0.0 ;              /* Pour mettre le sigma de la HT */
    if(l > 0) {
      aik = ai ;                    /* Debut ligne i */
      for(k=0;k<=l;k++) {           /* On regarde a gauche de la diagonale */
	scale += (float)fabs(*aik);
        aik += mj ;                 /* col. suivante */
      }
      if(scale == 0.0) {
	e[i]= *(a + i * mi + l * mj) ;
      } else {
        aik = ai ;                  /* Re-debut re-ligne i */
	for(k=0;k<=l;k++) {
	  *aik /= scale;
	  sig += (*aik) * (*aik) ;  /* On remplit sigma */
          aik += mj ;               /* Re-col. re-suivante */
	}
	f = *(ai + l * mj) ;
        g = SIGN((float)sqrt(sig), -f);
	sig -= f * g ;              /* a[i][i-1] +/- racine(sigma) */ 
	e[i] = scale * g ;
	*(ai + l * mj) = f - g ;    /* pour a[i][l] */
	f = (float)0.0 ;
        aji = a + i * mj ;          /* Sert a mettre les infos de sortie */
        aij = ai ;                  /* Element de la matrice initiale */
        aj = a ;
	for(j=0;j<=l;j++) {
	  *aji = *aij / sig ;
	  g = (float)0.0 ;
          aik = ai;                 /* a[i][0..j] */
          ajk = aj;                 /* a[j][0..j] */
	  for(k=0;k<=j;k++) {
	    g += *ajk * (*aik) ;
            aik += mj ;             /* Next col. */
            ajk += mj ;             /* Next col. */
	  }                         /* now *aik = a[i][j+1] */
          ajk = aj + mi + j * mj ;  /* !! : C'est a[j+1..l][j] ! */
	  for(k=j+1;k<=l;k++) {
	    g += *ajk * (*aik) ;
            aik += mj ;             /* Next col. */
            ajk += mi ;             /* Next ROW  */
	  }                         /* Fin boucle sur k */
	  e[j] = g / sig ;
	  f += e[j] * (*aij);
          aji += mi ;               /* Next row  */
          aj += mi ;                /* Next row  */
          aij += mj ;               /* Next col. */
	}                           /* Fin boucle sur j */
	GdK = f / (sig + sig) ;
        aij = ai ;
        aj = a ;
	for(j=0;j<=l;j++) {
	  f = *aij ;
          g = e[j] - GdK * f ;
          e[j] = g;
          aik = ai ;
          ajk = aj ;
	  for(k=0;k<=j;k++) {
	    *ajk -= (f * e[k] + g * (*aik)) ;
            aik += mj ;
            ajk += mj ;
	  }                         /* Fin boucle sur k */
          aij += mj ;
          aj += mi ;
	}                           /* Fin boucle sur j */
      }
    } else {
      e[i] = *(ai + l * mj);        /* a[i][l] */
    }                               /* fin else du if(l > 0) */
    d[i] = sig ;
  }                                 /* Fin boucle sur i */
  d[0]=(float)0.0;
  e[0]=(float)0.0;
  ai = a;                          /* a[0..n-1][0] */
  for(i=0;i<n;i++) {               /* Calcule Q */
    l=i-1;
    if(d[i]) {
      aj = a ;                      /* !! : a[0][0..j..l] */
      for(j=0;j<=l;j++) {
	g=(float)0.0;
        ajk = aj ;                  /* !! : a[0..k..l][j] */
        aik = ai ;                  /* a[i][0..k..l]      */
	for(k=0;k<=l;k++) {
	  g += *aik * (*ajk);
          ajk += mi ;               /* Next ROW  */
          aik += mj ;               /* next col. */
	}
        ajk = aj ;                  /* !! : a[0..k..l][j] */
        aik = a + i * mj ;          /* !! : a[0..k..l][i] */
	for(k=0;k<=l;k++) {
	  *ajk -= g * (*aik);
          ajk += mi ;               /* Next ROW  */
          aik += mi ;               /* Next ROW  */
	}
      aj += mj ;                    /* Next col. */
      }
    }                               /* Fin du if(d[i]) */
    d[i] = *(ai + i * mj) ;
    *(ai + i * mj) = (float)1.0 ;
    aji = a + i * mj ;              /* a[0..j..l][i] */
    aij = ai ;                      /* a[i][0..j..l] */
    for(j=0;j<=l;j++) {
      *aij = (float)0.0 ;
      *aji = (float)0.0 ;
      aji += mi ;                   /* Next row  */
      aij += mj ;                   /* Next col. */
    }
    ai += mi ;                      /* Ligne suiv. */
  }                                 /* Fin boucle sur i */
}

/* mseigvv
   IN  : a  : pointer on a TLisp symetric matrix
         n  : matrix size
	 mi : "modulo" on lines
	 mj : "modulo" on columns
	 mv : "modulo" of vector for eigen values
   OUT : a  : eigen vectors (as columns)
         v  : vector of eigen values
   */

static void 
mseigvv(float *a, int n, int mi, int mj, float *v, int mv)
{
  float *e, *d, *aki, *vj;
  int i, j, k, l, m, nbit, itmax;
  float dd, shft, r, s, p, b, f, c;

  e = (float *) tl_malloc(n * sizeof(float));
  d = (float *) tl_malloc(n * sizeof(float));
  for(i=0;i<n;i++) {
    e[i] = (float)0.0 ;
    d[i] = (float)0.0 ;
  }

  tridiag(a, n, mi, mj, d, e);
  /* d contient les elements diagonaux 0..N-1 de la tridiagonalisee.
     e contient les elements sous-diagonaux 1..N-1 de la meme.
     e[0]=0. on decale tout et on met e[N-1]=0
   */

  for (i=0; i<n-1; i++)
    e[i] = e[i+1];
  e[n-1] = (flt)0.0;

  itmax = 32 + 4 * n;
  
  for(l=0;l<n;l++) {
    nbit = 0;
    do {
      for(m=l;m<n-1;m++) {
        dd = (float)fabs(d[m]) + (float)fabs(d[m+1]) ;
        /* e[m] est l'element sous d[m] et a gauche de d[m+1] */
        if((fabs(e[m]) + dd) == dd) break ;
      }
      if(m != l) {
	if(nbit++ == itmax) {
          free(e);
          free(d);
          error(0,"Numerical problems during eigenvalue decomposition",0);
        }
	shft = (d[l+1] - d[l]) / ((float)2.0 * e[l]) ;
	r = (float)sqrt((shft * shft) + 1.0) ;
	shft = d[m] - d[l] + e[l] / (shft + SIGN(r,shft)) ;
	c = (float)1.0 ;
        s = (float)1.0 ;
	p = (float)0.0 ;
	for(i=m-1;i>=l;i--) {
	  b = c * e[i] ;
	  f = s * e[i] ;
	  if(fabs(f) >= fabs(shft)) {
	    c = shft / f ;
	    r = (float)sqrt((c * c) + 1.0 ) ;
	    e[i+1] = f * r ;
            s = (float)1.0 / r ;
	    c *= s ;
	  } else {
	    s = f / shft ;
	    r = (float)sqrt((s * s) + 1.0) ;
	    e[i+1] = shft * r ;
            c = (float)1.0 / r ;
	    s *= c ;
	  }
	  shft = d[i+1] - p ;
	  r = (d[i] - shft) * s + (float)2.0 * c * b ;
	  p = s * r ;
	  d[i+1] = shft + p ;
	  shft = c * r - b ;
          aki = a + mj * i ;                 /* a[0..k..n-1][i] */
	  for(k=0;k<n;k++) { 	             /* Calcul des vecteurs propres */
	    f = *(aki + mj) ;
	    *(aki + mj) = s * (*aki) + c * f ;
	    *aki = c * (*aki) - s * f ;
            aki += mi;
	  }
	}
	d[l] = d[l] - p ;
	e[l] = shft ;
	e[m] = (float)0.0;
      }
    } while (m != l);
  }
  vj = v;         /* On met les valeurs propres dans la matrice qui va bien */
  for(j=0;j<n;j++) {
    *vj = d[j];
    vj += mv ;
  }
  free(d);
  free(e);
}


#undef SIGN


DX(xmseigvv)
{
  int n=0;
  at *atres;
  struct array *arr, *res;

  ARG_NUMBER(1);
  ALL_ARGS_EVAL;

  arr = check_matrix(APOINTER(1),&n,&n);

  atres = matrix(1,&n);
  res = check_vector(atres,&n);
  mseigvv(arr->data,n,arr->modulo[0],arr->modulo[1],res->data,res->modulo[0]);

  return atres;
}

/* --- former tl/stat fmatops --- */

/* *** Check dimensions of an imatrix;
   *** if 0 simply copy them from the imatrix *** */

static struct array *
check_imatrix (at *p, int *n, int *m)
{
  struct array *arr;

  ifn (p && (p->flags & X_ARRAY))
    error (NIL, "not a matrix", p);
  ifn (p->Class == &imatrix_class)
    error (NIL, "imatrix only are allowed", p);
  arr = p->Object;

  if (arr->ndim == 1) {
    arr->dim[1] = 1;
    arr->modulo[1] = 0;
  }
  else if (arr->ndim != 2) {
    error (NIL, "one or two dimensional matrix expected", p);
  }

  if ((*n && (arr->dim[0] != *n)) ||
      (*m && (arr->dim[1] != *m)))
    error (NIL, "wrong matrix size", p);
  *n = arr->dim[0];
  *m = arr->dim[1];

  return arr;
}

/* *** Allocate and lock imatrix on need *** */

static struct array *
answer_imatrix (at **ans, int *n, int *m)
{
  int dim[2];

  if (*ans) {
    LOCK (*ans);
  }
  else {
    if (!*n || !*m)
      error (NIL, "internal error: answer_matrix : unknown size", NIL);
    dim[0] = *n;
    dim[1] = *m;
    *ans = (dim[1] == 1) ? imatrix (1, dim) : imatrix (2, dim);
    if (*n < 10 && *m < 10)
      ((struct array *) ((*ans)->Object))->flags |= PRINT_ARRAY;
  }
  return ((*ans)->Object);
}

/* *** Fill int array with identity values;
   *** This function is used in (mseek )     *** */

#define TL3_INITIAL_IDENTITY_SIZE 32

static void
identity_range (int *identity, int offset, int n)
{
  int i;
  int *i_ptr = identity + offset;
  for (i = offset; i < n; i++, i_ptr++) {
    *i_ptr = i;
  }
}

/* ##################### MATRIX OPERATIONS ####################### */

/* *** (mset <msel> <m1> [<ans>]) *** */

DX (xmset)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 2:
     return mset (APOINTER (1), APOINTER (2), NIL);
   case 3:
     return mset (APOINTER (1), APOINTER (2), APOINTER (3));
   default:
     ARG_NUMBER (-1);
     return (NIL);              /* Never reached */
  }
}

static at *
mset (at *v1, at *v2, at *ans)
{
  int n = 0;
  int m = 0;
  register int i;
  register flt *f1, *f2, *fa;
  flt *ff1, *ff2, *ffa, c2;
  struct array *vv1, *vv2, *vva;

  flt default_value = getnanF();
  int needs_init_answer_matrix = 0;

  int vam0, v1m0, v2m0;
  int vam1, v1m1, v2m1;

  vv1 = check_matrix (v1, &n, &m);
  v1m0 = vv1->modulo[0];
  v1m1 = vv1->modulo[1];

  ff1 = vv1->data;

  if (NUMBERP (ans)) {
    default_value = rtoF(ans->Number);
    needs_init_answer_matrix = TRUE;
    ans = NIL;     /* This forces answer_matrix() to allocate a new matrix */
  }
  if (NUMBERP (v2)) {
    vv2 = NULL;
    v2m0 = 0;
    v2m1 = 0;
    ff2 = &c2;
    c2 = rtoF(v2->Number);
  }
  else {
    vv2 = check_matrix (v2, &n, &m);
    v2m0 = vv2->modulo[0];
    v2m1 = vv2->modulo[1];
    ff2 = vv2->data;
  }

  vva = answer_matrix (&ans, &n, &m);
  vam0 = vva->modulo[0];
  vam1 = vva->modulo[1];
  ffa = vva->data;

  while (n--) {
    i = m;
    f1 = ff1;
    f2 = ff2;
    fa = ffa;
    while (i--) {
      if (*f1) {
        *fa = *f2;
      }
      else {
        if (needs_init_answer_matrix)
          *fa = default_value;
      }
      fa += vam1;
      f1 += v1m1;
      f2 += v2m1;
    }
    ffa += vam0;
    ff1 += v1m0;
    ff2 += v2m0;
  }
  return ans;
}

/* *** (msort <m0> [<m1> ... <mn>] ) *** */

DX (xmsort)
{
  int n = 0;
  int m = 1;
  at *m0, *mk, *midx=0;
  struct array *mm0, *mmk, *mmidx;

  int j;

  ALL_ARGS_EVAL;
  if (arg_number < 1) {
    error (NIL, "No args. Usage: (msort <m0> [<m1> ... <mn>] ) ", NIL);
  }
  m0 = APOINTER(1);
  mm0 = check_matrix (m0, &n, &m);
  mmidx =  answer_imatrix (&midx, mm0->dim, &m);
  msort_vector (mm0->dim[0], mm0->modulo[0], mm0->data, mmidx->modulo[0],
		(int *)mmidx->data);

  /* Now apply index to all other matrices in the arg_list (if any) */

  n = mm0->dim[0];

  for (j=2; j<=arg_number; j++) {
    mk = APOINTER(j);
    mmk = check_matrix (mk, &n, &m);

    index_apply(n,
		mmidx->data, mmidx->modulo[0],
		mmk->data,   mmk->modulo[0],
		mmk->data,   mmk->modulo[0],
		m_buffer,    1
		);
  }

  /* return atom with index matrix */
  return midx;
}

static void
msort_vector (int n, int modn, flt * data, int modi, int *idata)
{
  int i;
  long nr_of_nans = 0;
  float *f1 = data;
  float *f2 = data;             /* element to be swapped when encounetring NaN */
  int *i1;

  i1 = idata;
  for (i = 0; i < n; i++, i1 += modi) {  /* initialise indices */
    *i1 = i;
  }

  /* Arno; Wed Oct 23 17:15:32 MET DST 1996; no consequent use yet
     of a math library which does *not* give SIGFPE on numerous
     NaN-involved operations; so we have to filter out the NaNs first */

  for (i = 0; i < n; i++, f1 += modn)
    if (isnanF (*f1)) {                  /* NaN indices at the head */
      *(idata+(nr_of_nans*modi)) = i;    /* First swap indices */
      *(idata +(i*modi)) = nr_of_nans;
      *f1 = *f2;                /* Then swap values (has no sense if NaNs are already 
                                   at head; but alas; otherwise have to test all the time
                                   for equalness of *f1 and *f2 */
      *f2 = getnanF ();
      nr_of_nans++;
      f2 += modn;
    }
  i1 = idata+(nr_of_nans*modi);

  qsort_mod_idx (f2, i1, n - nr_of_nans, modn, 1);
}

/*
 * Sort a vector of floats in place using the QuickSort algorithm. 
 * After Numerical Recipes 2nd edition (sort).
 */

#define TL3_NSTACK 	80
#define TL3_M 		7
#define TL3_SWAPF(x,y) 	{ float tmp=(x); (x)=(y); (y)=tmp; }
#define TL3_SWAPI(x,y) 	{ int tmp=(x); (x)=(y); (y)=tmp; }

static void
qsort_mod_idx (float *arr, int *idx, int n, int mod, int modi)
{
  long i, j, k, l, m, p; /* negative modulo *are* possible */
  int jstack = 0;
  int istack[TL3_NSTACK];
  float a;

  m = n - 1;
  l = 0;
  for (;;) {                    /* Sort segment [l,m] */
    if (m - l < TL3_M) {            /* Insertion sort */
      for (j = l + 1; j <= m; j++) {
        a = arr[j * mod];
        p = idx[j * modi];
        for (i = j - 1; i >= l; i--) {
          if (arr[i * mod] <= a)
            break;
          arr[(i + 1) * mod] = arr[i * mod];
          idx[(i + 1) * modi] = idx[i * modi];
        }
        arr[(i + 1) * mod] = a;
        idx[(i + 1) * modi] = p;
      }
      if (jstack == 0)          /* Pop stack and sort again */
        return;
      m = istack[--jstack];
      l = istack[--jstack];
    }
    else {                      /* Choose pivot a[l+1] <= (a=a[l]) <= a[m] */
      k = (l + m) >> 1;
      TL3_SWAPF (arr[k * mod], arr[(l + 1) * mod]);
      TL3_SWAPI (idx[k * modi], idx[(l + 1) * modi]);
      if (arr[(l + 1) * mod] > arr[m * mod]) {
        TL3_SWAPF (arr[(l + 1) * mod], arr[m * mod]);
        TL3_SWAPI (idx[(l + 1) * modi], idx[m * modi]);
      }
      if (arr[l * mod] > arr[m * mod]) {
        TL3_SWAPF (arr[l * mod], arr[m * mod]);
        TL3_SWAPI (idx[l * modi], idx[m * modi]);
      }
      if (arr[(l + 1) * mod] > arr[l * mod]) {
        TL3_SWAPF (arr[(l + 1) * mod], arr[l * mod]);
        TL3_SWAPI (idx[(l + 1) * modi], idx[l * modi]);
      }

      i = l + 1;                /* Do partitionning */
      j = m;
      a = arr[l * mod];
      p = idx[l * modi];
      for (;;) {
        do
          i++;
        while (arr[i * mod] < a);
        do
          j--;
        while (arr[j * mod] > a);
        if (j < i)
          break;
        TL3_SWAPF (arr[i * mod], arr[j * mod]);
        TL3_SWAPI (idx[i * modi], idx[j * modi]);
      }
      arr[l * mod] = arr[j * mod];  /* Insert partitionning element */
      idx[l * modi] = idx[j * modi];
      arr[j * mod] = a;
      idx[j * modi] = p;

      if (jstack >= TL3_NSTACK - 2)
        error (NIL, "quicksort stack is full", NIL);

      if (m - i + 1 >= j - l) { /* Push into stack and sort again */
        istack[jstack++] = i;
        istack[jstack++] = m;
        m = j - 1;
      }
      else {
        istack[jstack++] = l;
        istack[jstack++] = j - 1;
        l = i;
      }
    }
  }
}

#undef TL3_NSTACK
#undef TL3_M
#undef TL3_SWAP

/* *** (mindex-compute <m0> [<indx>]) *** */

DX (xmindex_compute)
{
  at *ans = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 2:
     ans = APOINTER (2);
   case 1:
     return mindex_compute (APOINTER (1), ans);
   default:
     ARG_NUMBER (-1);
     return NIL;
  }
}


static at *
mindex_compute (at *m0, at *ans)
{
  int n = 0;
  int m = 1;
  struct array *mm0, *mmans, *va;

  long i;
  int mod;
  float *f1;
  float *f2;

  mm0 = check_matrix (m0, &n, &m);
  mmans = answer_imatrix (&ans, mm0->dim, &m);
  buffer_check(&m_buffer,&m_buffer_size, sizeof(flt) * n);

  /* copy m0 to buffer.
     Oops; this code *isnt* quite reentrant (MT-Safe??)
     But reusable and straightforward instead (XD) */

  mod = mm0->modulo[0];
  f1 = (float *) mm0->data;
  f2 = m_buffer;
  for (i = 0; i < n; i++, f2++, f1 += mod) {
    *f2 = *f1;
  }
  mod = 1;              /* since f2 is straight array; hmmm */
  va = answer_imatrix (&ans, &n, &m);
  msort_vector (mm0->dim[0], mod, m_buffer,va->modulo[0], (int *)va->data);

  /* return atom with index matrix */
  return ans;
}

/* ***  (mindex-apply <indx> <m0> [<ans>]) *** */

DX (xmindex_apply)
{
  at *m2 = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 3:
     m2 = APOINTER (3);
   case 2:
     return mindex_apply (APOINTER (1), APOINTER (2), m2);
   default:
     ARG_NUMBER (-1);
     return NIL;
  }
}

static at *
mindex_apply (at *midx, at *msrc, at *mans)
{
  int n=0, k=0, m=1;
  int overlp=0;
  struct array *mmsrc, *mmidx, *mmans;

  mmsrc = check_matrix (msrc, &k, &m);
  mmidx = check_imatrix (midx, &n, &m);
  mmans = answer_matrix (&mans, &n, &m);

  if(((mmsrc->flags & SUB_ARRAY) ? mmsrc->main_array : msrc)
     ==
     ((mmans->flags & SUB_ARRAY) ? mmans->main_array : mans)
     )
    {
      buffer_check(&m_buffer,&m_buffer_size, sizeof(flt) * n);
      overlp = 1;
    }

  index_apply(n,
	      mmidx->data, mmidx->modulo[0],
	      mmsrc->data, mmsrc->modulo[0],
	      mmans->data, mmans->modulo[0],
	      (overlp ? m_buffer : 0), (overlp ? 1 : 0)
	      );

  return mans;
}

static void
index_apply (int n,
	     int *idx     , int modi,
	     flt *f1_start, int mod1,
	     flt *f2      , int mod2,
	     flt *mbuf    , int mod )
{
  int i;
  flt *f;

  if( mbuf ) {
    f = mbuf;
    mod=1;
  } else {
    f = f2;
    mod = mod2;
  }

  for (i = 0; i < n; i++, f += mod, idx += modi) {
    *f = *(f1_start + ((*idx) * mod1));
  }

  if( mbuf ) {
    for (i = 0; i < n; i++, mbuf += mod, f2 += mod2) {
      *f2 = *mbuf;
    }
  }

}

/* *** (mindex-compose <indx1> ... <indxn>) *** */

#define TL3_SWAP_IPTR() \
{  int *tmp=(Ans_ptr);\
   (Ans_ptr)=(Buf_ptr); (Buf_ptr)=tmp;\
   (ans_ptr)=(Ans_ptr); (buf_ptr)=(Buf_ptr); }

DX (xmindex_compose)
{
  int n = 0;
  int m = 1;
  int size;
  struct array *mmidx, *mnext_midx, *mmans;
  at *midx, *next_midx, *mans = NIL;
  int i,j;
  int mod;
  int *swap_buffer;
  int *Ans_ptr, *Buf_ptr;
  int *ans_ptr, *buf_ptr, *iidx, *ians;

  ALL_ARGS_EVAL;
  if (arg_number < 1) {
    error (NIL, "No args. Usage: (mindex-compose <indx0> [<indx1> ... <indxn>] ) ", NIL);
  }
  midx = APOINTER(1);
  mmidx = check_imatrix (midx, &n, &m);
  size = n;
  mod = mmidx->modulo[0];
  iidx = (int *) mmidx->data;
  mmans = answer_imatrix (&mans, &n, &m);
  ians = (int *) mmans->data;
  /* Now compose index with all other indices in the arg_list (if any) */
  if (arg_number < 2) {
    return midx;
  }
  else {
    swap_buffer = (int *) tl_malloc (2 * n);
    Ans_ptr = swap_buffer;
    Buf_ptr = swap_buffer + n;
    ans_ptr = Ans_ptr;
    for (i = 0; i < n; i++, ans_ptr++, iidx += mod) {
      *ans_ptr = *iidx;         /* Copy indx0 into buffer */
    }
    ans_ptr = Ans_ptr;
    buf_ptr = Buf_ptr;
  }
  for (j=2; j < arg_number; j++) {
    next_midx = APOINTER(j);
    n = 0;                      /* Allow for vectors of decreasing size */
    mnext_midx = check_imatrix (next_midx, &n, &m);
    if (mnext_midx->dim[0] > size) {
      error (NIL, "only allowing decreasing vector sizes", next_midx);
    }
    else {
      size = mnext_midx->dim[0];
    }
    mod = mnext_midx->modulo[0];
    iidx = (int *) mnext_midx->data;
    for (i = 0; i < size; i++, buf_ptr++) {
      *buf_ptr = *(iidx + (*(ans_ptr + mod * i)));  /* apply next index */
    }
    TL3_SWAP_IPTR ();
  }

  /* finally copy answer_buffer back to answermatrix */
  mod = mmans->modulo[0];
  for (i = 0; i < size; i++, ans_ptr++, ians += mod) {
    *ians = *ans_ptr;           /* Copy indx0 into buffer */
  }
  /* and reduce size of answer-array */
  mmans->dim[0] = size;

  free (swap_buffer);
  return mans;
}
#undef TL3_SWAP_IPTR

/* *** (mindex-inv <indx> [<ians>]) *** */

DX (xmindex_inv)
{
  at *mans = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 2:
     mans = APOINTER (2);
   case 1:
     return mindex_inv (APOINTER (1), mans);
   default:
     ARG_NUMBER (-1);
     return NIL;
  }
}

static at *
mindex_inv (at *midx, at *mans)
{
  int n = 0;
  int m = 1;
  struct array *mmidx, *mmans;
  int i;
  int mod, modans;
  int *iidx, *iinv;

  mmidx = check_imatrix (midx, &n, &m);
  mmans = answer_imatrix (&mans, &n, &m);

  mod = mmidx->modulo[0];
  modans = mmans->modulo[0];
  iidx = (int *) mmidx->data;
  iinv = (int *) mmans->data;
  for (i = 0; i < n; i++) {
    *(iinv + (*(iidx + mod * i) * modans)) = i;
  }
  return (mans);
}

/* ***  (mseek <m0> <x> [<indx>])     ***
   ***  (mseek-up <m0> <x> [<indx>]   ***
   ***  (mseek-down <m0> <x> [<indx>] *** */

DX (xmseek)
{
  at *idx = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 3:
     idx = APOINTER (3);
   case 2:
     return mseek (APOINTER (1), AFLT(2), 0, idx);
   default:
     ARG_NUMBER (-1);
     return (NIL);              /* Never reached */
  }
}

DX (xmseek_up)
{
  at *idx = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 3:
     idx = APOINTER (3);
   case 2:
     return mseek (APOINTER (1), AFLT(2), 1, idx);
   default:
     ARG_NUMBER (-1);
     return (NIL);              /* Never reached */
  }
}

DX (xmseek_down)
{
  at *idx = NIL;

  ALL_ARGS_EVAL;
  switch (arg_number) {
   case 3:
     idx = APOINTER (3);
   case 2:
     return mseek (APOINTER (1), AFLT(2), -1, idx);
   default:
     ARG_NUMBER (-1);
     return (NIL);              /* Never reached */
  }
}

static at *
mseek (at *m0, flt x, int way, at *midx)
{
  struct array *a;
  struct array *mmidx;
  float *p;
  int mod, mod_idx, i, j, k, m = 1, n = 0;
  int *iidx;

  static int *identity;
  static int N = 0;

  if (N == 0) {
    N = TL3_INITIAL_IDENTITY_SIZE;
    identity = (int *) tl_malloc ((sizeof (int)) * TL3_INITIAL_IDENTITY_SIZE);
    identity_range (identity, 0, N);
  }

  a = check_vector (m0, &n);
  p = a->data;
  mod = a->modulo[0];

  if (midx) {
    mmidx = check_imatrix (midx, &n, &m);
    iidx = (int *) mmidx->data;
    mod_idx = mmidx->modulo[0];
  }
  else {                        /* Suppose matrix <m0> is already sorted */
    while (N < n) {
      N *= 2;
      free (identity); /* no longer re-alloc */
      identity = (int * ) tl_malloc ((sizeof (int) * N));

      identity_range (identity, 0, N);
    }
    iidx = identity;
    mod_idx = 1;
  }

  i = 0;
  while (isnanF (*(p + (*(iidx + mod_idx * i) * mod))))
    i++;                        /* Skip leading NaNs */
  k = n - 1;
  j = k / 2;
  while (i < k && x != (*(p + (*(iidx + mod_idx * j) * mod)))) {
    if ((*(p + (*(iidx + mod_idx * j) * mod))) > x)
      k = (i == j) ? i : j - 1;
    else
      i = (j == k) ? k : j + 1;
    j = (i + k) / 2;
  }
  if (way == 0 && x != (*(p + (*(iidx + mod_idx * j) * mod))))
    return NIL;

  if (way > 0) {
    while (j < n - 1 && (*(p + (*(iidx + mod_idx * j) * mod))) <= x) {
      j++;
    }
    if (((*(p + (*(iidx + mod_idx * j) * mod))) > x) && j > 0 && ((*(p + (*(iidx + mod_idx * (j - 1)) * mod))) == x))
      j--;
    else if ((*(p + (*(iidx + mod_idx * j) * mod))) < x)
      return NIL;
  }
  else {
    while (0 < j && (*(p + (*(iidx + mod_idx * j) * mod))) >= x) {
      j--;
    }
    if (((*(p + (*(iidx + mod_idx * j) * mod))) < x) && j < n - 1 && ((*(p + (*(iidx + mod_idx * (j + 1)) * mod))) == x))
      j++;
    else if ((*(p + (*(iidx + mod_idx * j) * mod))) > x)
      return NIL;
  }

  return NEW_NUMBER (j);
}



/* Recursively go through all dimensions of a (yet single) matrix and execute
   operation (void)(op)(...) on it; 
   Arno : we'd probably want tu use varargs as well as vector-scalar
   and/or vector-vector operations on the lowest level.   
   Check varags code to know wether or not this causes to much overhead!! */

static int
matrix_sweep_and_exec (int ndim_still, int *pdim,
		       int *pmod1, ptr pdata1, /* pdata1 null => unused */ 
		       int *pmod2, ptr pdata2, /* pdata2 null => unused */ 
		       flt x, flt y, int *pcount,
                       void (*op) (flt *, flt *, flt, flt, int *))
{
  int i;
  flt *pd1 = pdata1; /* flt only XD Tue Feb  4 12:11:49 MET 1997 */
  flt *pd2 = pdata2;
  int mod1, mod2;

  mod1 = pd1 ? *pmod1 : 0;
  mod2 = pd2 ? *pmod2 : 0;

  if (ndim_still <= 0) {
    error (NIL, "matrix with no dimensions given", NIL);
    return (-1);
  }
  if (ndim_still == 1) {
    for (i = 0;
	 i < *pdim;
	 i++, pd1+=mod1, pd2+=mod2 ) {
      op (pd1, pd2, x, y, pcount);
    }
  } else {
    for (i = 0;
	 i < *pdim;
	 i++, pd1+=mod1, pd2+=mod2 ) {
      matrix_sweep_and_exec (ndim_still - 1, pdim + 1,
			     pmod1 + 1, pd1,
			     pmod2 + 1, pd2,
			     x, y, pcount,
			     op
			     );
    }
  }
  return (0);
}

DX (xmoccur)
{
  flt f3 = Fzero;
  at *m4 = 0;

  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 4:
    m4 = APOINTER(4);
    f3 = AFLT (3);
    break;
  case 3:
    m4 = APOINTER(1);
    f3 = AFLT (3);
    break;
  case 2:
    break;
  default:
    ARG_NUMBER (-1);
  }
  if (APOINTER(2))
    return NEW_NUMBER(moccur (APOINTER (1), AFLT (2), f3, m4));
  else
    return NEW_NUMBER(0);
}

static int
moccur (at *v1, flt val1, flt valans, at* va)
{
  struct array *vv1, *vva;
  int num = 0;

  vv1 =      check_matrix (v1, (void *)0, (void *)0)    ;
  vva = va ? check_matrix (va, (void *)0, (void *)0) : 0;
  if (isnanF(val1)) {
    matrix_sweep_and_exec (vv1->ndim, vv1->dim, vv1->modulo, vv1->data,
			   (vva ? vva->modulo : 0), ( vva ? vva->data : 0),
			   val1, valans, &num, nan_moccur_op
			   );
  } else {
    matrix_sweep_and_exec (vv1->ndim, vv1->dim, vv1->modulo, vv1->data,
			   (vva ? vva->modulo : 0), (vva ? vva->data : 0),
			   val1, valans, &num, moccur_op
			   );
  }
  return num;
}

static void
moccur_op (flt * datum1, flt * datum2, flt val, flt valans, int *num)
{

  if (*datum1 == val) {
    if( datum2 ) *datum2 = valans;
    (*num)++;
  }
}

static void
nan_moccur_op (flt * datum1, flt * datum2, flt val1, flt valans, int *num)
{

  if (isnanF(*datum1)) {
    if( datum2 ) *datum2 = valans;
    (*num)++;
  }
}

/* --------- INITIALISATION CODE --------- */


void 
init_fmatops(void)
{
  dx_define("m+m", xmaddm);
  dx_define("am+bm", xmlinm);
  dx_define("mbary", xmbary);
  dx_define("m-m", xmsubm);
  dx_define("mxm", xmmulm);
  dx_define("m/m", xmdivm);
  dx_define("mx+m", xmmulaccm);
  dx_define("m*m", xmdotm);
  dx_define("m*d", xmdotd);
  dx_define("m+c", xmaddc);
  dx_define("m*c", xmdotc);
  dx_define("m+vvt", xmaddvvt);
  dx_define("convolve", xconvolve);
  dx_define("dot_product", xdot_product);
  dx_define("trace", xtrace);

  dx_define("m/",xmdiv);
  dx_define("msqrt",xmsqrt);
  dx_define("mexp",xmexp);
  dx_define("mlog",xmlog);
  dx_define("mcos",xmcos);
  dx_define("msin",xmsin);
  dx_define("mtan",xmtan);
  dx_define("masin",xmasin);
  dx_define("macos",xmacos);
  dx_define("matan",xmatan);
  dx_define("msinh",xmsinh);
  dx_define("mcosh",xmcosh);
  dx_define("mtanh",xmtanh);

  dx_define("inf_mat",xinf_mat);
  dx_define("sup_mat",xsup_mat);
  dx_define("mean_mat",xmean_mat);
  dx_define("sdev_mat",xsdev_mat);
  dx_define("minvdet", xminvdet);
  dx_define("mseigvv",xmseigvv);

  dx_define ("mseek", xmseek);
  dx_define ("mseek-up", xmseek_up);
  dx_define ("mseek-down", xmseek_down);
  dx_define ("mset", xmset);
  dx_define ("mindex-compute", xmindex_compute);
  dx_define ("mindex-apply", xmindex_apply);
  dx_define ("mindex-inv", xmindex_inv);
  dx_define ("msort", xmsort);
  dx_define ("mindex-compose", xmindex_compose);
  dx_define ("moccur", xmoccur);
}
