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
  array.c
  - array and matrix class definition
  $Id: array.c,v 1.1.1.1 2002-04-16 17:37:38 leonb Exp $
********************************************************************** */

#include "header.h"


/* --------- ERROR MESSAGES --------- */

static char           x_array_error[]="not an array or a matrix";
static char illegal_subscript_error[]="illegal_subscript";
static char   subscript_range_error[]="subscript out of range";
static char      unknown_array_type[]="internal error: unknown matrix type";
static char    unhandled_array_type[]="internal error: unhandled matrix type";
static char     square_matrix_error[]="square matrix expected";
static char     _1D_2D_matrix_error[]="1D or 2D Matrix expected";
static char        _2D_matrix_error[]="2D matrix expected";
static char         dimension_error[]="Unproper dimension";
static char   unspecified_dimension[]="only one unspecified dimension allowed";
static char            copy_any_mat[]="matrix.c/copy_any_matrix";



/* --------- UTILITIES --------- */

static int mind0[MAXDIMS];


/* --------- ALLOCATION STRUCTURE --------- */


struct alloc_root array_alloc = {
  NIL,
  NIL,
  sizeof(struct array),
  25
};


/* --------- CLASS DEFINITIONS --------- */



static void 
array_dispose(at *p)
{
  struct array *arr;
  register at **here;
  register int i, d;

  arr = p->Object;

  if (arr->flags & SUB_ARRAY) {
    UNLOCK(arr->main_array);
  } else if (arr->flags & ALLOCATED_ARRAY) {
    here = arr->data;
    i = 1;
    for (d = 0; d < arr->ndim; d++)
      i *= arr->dim[d];
    for (d = 0; d < i; d++) {
      UNLOCK(*here);
      here++;
    }
    free(arr->data);
  }
  deallocate(&array_alloc, (struct empty_alloc *) arr);
}


static void 
array_action(at *p, void (*action) (at *))
{
  struct array *arr;
  register at **here;
  register int i, d;

  arr = p->Object;

  if (arr->flags & SUB_ARRAY) {
    (*action)(arr->main_array);
  } else if (arr->flags & ALLOCATED_ARRAY) {
    here = arr->data;
    i = 1;
    for (d = 0; d < arr->ndim; d++)
      i *= arr->dim[d];
    for (d = 0; d < i; d++) {
      (*action) (*here);
      here++;
    }
  }
}


char *
array_name(at *p)		/* default name for unprintable matrix */
{
  register struct array *arr;
  register char *s;
  register int d;

  arr = p->Object;
  s = string_buffer;
  sprintf(s, "::%s:", nameof(p->Class->classname));
  while (*s)
    s++;
  for (d = 0; d < arr->ndim; d++) {
    sprintf(s, "%dx", arr->dim[d]);
    while (*s)
      s++;
  }
  *--s = 0;
  return string_buffer;
}


static at *array_listeval(at *p, register at *q); /* forward */
int array_compare(at *p, at *q, int order);       /* forward */
unsigned long array_hash(at *p);                  /* forward */
at *array_getslot(at *obj, at *prop);             /* forward */
void array_setslot(at *obj, at *prop, at *val);   /* forward */


class array_class = {
  array_dispose,
  array_action,
  array_name,
  generic_eval,
  array_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot,
};


/* --------- ARRAY AND MATRIX CREATION --------- */



/*
 * array(nd,d) returns a new array with nd dimensions (d[0],d[1] etc...)
 */
at *
array(int nd, int *d)
{
  at *ans;
  register struct array *arr;
  register int i, size;

  arr = allocate(&array_alloc);
  arr->flags = 0;
  arr->ndim = nd;
  arr->data = NIL;
  arr->main_array = NIL;
  arr->nr0 = NIL;
  arr->nr1 = NIL;
  ans = new_extern(&array_class, arr);

  size = 1;
  for (i = nd - 1; i >= 0; i--) {
    if (d[i] < 1)
      error(NIL, "negative or zero size", NIL);

    arr->dim[i] = d[i];
    arr->modulo[i] = size;
    size *= d[i];
  }

  ifn(arr->data = tl_malloc(size * sizeof(at *)))
    error(NIL, "no room for a new array", NIL);

  for (i = 0; i < size; i++)
    ((at **) (arr->data))[i] = NIL;

  arr->flags |= ALLOCATED_ARRAY;
  ans->flags |= X_ARRAY;
  return ans;
}

DX(xarray)
{
  register int nd, i;
  int dim[MAXDIMS];

  nd = arg_number;
  if (nd < 1 || nd > MAXDIMS)
    ARG_NUMBER(-1);
  ALL_ARGS_EVAL;
  for (i = 0; i < nd; i++)
    dim[i] = AINTEGER(i + 1);

  return array(nd, dim);
}



/* --------- ARRAY AND MATRIX REFERENCE --------- */


/* loop to access an array (matrix) element or subarray (submatrix) */

at *
array_ref(at *(*aref)(void*, int), register struct array *arr, at **p)
{
  register at *q;
  register int i, j;
  int start, end;
  at *myp[MAXDIMS];

  at *ans = NIL;
  register at **where = &ans;

  /* 1: numeric arguments only */

  j = 0;
  for (i = 0; i < arr->ndim; i++) {
    register int k;

    q = p[i];
    if (q && (q->flags & C_NUMBER)) {
      k = (int) (q->Number);
      if (k < 0 || k >= arr->dim[i])
	error(NIL, subscript_range_error, q);
      j += (arr->modulo[i] == 1 ? k : k * arr->modulo[i]);
    } else
      goto list_ref;
  }

  return (*aref) (arr->data, j);

  /* 2 found a non numeric argument */

list_ref:

  for (j = 0; j < arr->ndim; j++)
    myp[j] = p[j];

  start = 0;
  end = arr->dim[i] - 1;

  if (q) {
    ifn(CONSP(q) && CONSP(q->Cdr) && !q->Cdr->Cdr &&
	q->Car && (q->Car->flags & C_NUMBER) &&
	q->Cdr->Car && (q->Cdr->Car->flags & C_NUMBER))
      error(NIL, illegal_subscript_error, NIL);

    start = (int)(q->Car->Number);
    end = (int)(q->Cdr->Car->Number);
  }
  myp[i] = NEW_NUMBER(0);
  for (j = start; j <= end; j++) {
    myp[i]->Number = j;
    *where = cons(array_ref(aref, arr, myp), NIL);
    where = &((*where)->Cdr);
    CHECK_MACHINE("on");
  }
  UNLOCK(myp[i]);
  return ans;
}


/* loop to modify an array (matrix) element or subarray (submatrix) */

at *
array_set(void (*aset)(void*, int, at*), 
	  struct array *arr, at **p, at *value, int mode)
{	
  at *q;
  int i, j;
  int start, end;
  at *myp[MAXDIMS];

  /* Meaning of argument MODE:
   *  0: 'value is constant'
   *  1: 'get values from sublistes'
   *  2: 'get values in sequence'
   */

  /* Test for mapped arrays */
  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "mapped arrays are read-only", NIL);

  /* 1: numeric arguments only */

  j = 0;
  for (i = 0; i < arr->ndim; i++) {
    int k;

    q = p[i];
    if (q && (q->flags & C_NUMBER)) {
      k = (int) (q->Number);
      if (k < 0 || k >= arr->dim[i])
	error(NIL, "indice out of range", q);
      j += (arr->modulo[i] == 1 ? k : k * arr->modulo[i]);
    } else
      goto list_set;
  }

  if (mode == 2) {
    (*aset) (arr->data, j, value->Car);
    return value->Cdr;
  } else {
    (*aset) (arr->data, j, value);
    return NIL;
  }

  /* 2 found a non numeric argument */

list_set:

  for (j = 0; j < arr->ndim; j++)
    myp[j] = p[j];

  start = 0;
  end = arr->dim[i] - 1;

  if (q) {
    ifn(CONSP(q) && CONSP(q->Cdr) && !q->Cdr->Cdr &&
	q->Car && (q->Car->flags & C_NUMBER) &&
	q->Cdr->Car && (q->Cdr->Car->flags & C_NUMBER))
      error(NIL, "illegal indice", NIL);

    start = (int)(q->Car->Number);
    end = (int)(q->Cdr->Car->Number);
  }
  if (mode == 1 && length(value) > end - start + 1)
    mode = 2;
  myp[i] = NEW_NUMBER(0);
  for (j = start; j <= end; j++) {
    myp[i]->Number = j;

    ifn(CONSP(value))
      mode = 0;
    switch (mode) {
      case 0:
	array_set(aset, arr, myp, value, mode);
	break;
      case 1:
	array_set(aset, arr, myp, value->Car, mode);
	value = value->Cdr;
	break;
      case 2:
	value = array_set(aset, arr, myp, value, mode);
	break;
    }
    CHECK_MACHINE("on");
  }
  UNLOCK(myp[i]);
  return value;
}


/* functions to access or modify a single array element */

static at *
arrayR(void *data, int depl)
{
  at **start = data;
  at *val;

  val = start[depl];
  if (val && val->flags&X_ZOMBIE) {
    UNLOCK(val);
    start[depl]=NIL;
    return NIL;
  } else {
    LOCK(val);
    return val;
  }
}

static void
arrayS(void *data, int depl, at *val)
{
  at **start = data;
  at *tmp = start[depl];
  LOCK(val);
  start[depl] = val;
  UNLOCK(tmp);
}

/* Real access functions */

static at *
array_listeval(at *p, at *q)
{
  struct array *arr;
  int i;
  at *qsav;
  at *myp[MAXDIMS];

  arr = p->Object;
  qsav = eval_a_list(q->Cdr);
  q = qsav;

  for (i = 0; i < arr->ndim; i++) {
    ifn(CONSP(q))
      error(NIL, "not enough subscripts", qsav);
    myp[i] = q->Car;
    q = q->Cdr;
  }
  if (!q) {
    q = array_ref(arrayR, arr, myp);
    UNLOCK(qsav);
    return q;
  } else if (CONSP(q) && (!q->Cdr)) {
    array_set(arrayS, arr, myp, q->Car, 1);
    UNLOCK(qsav);
    LOCK(p);
    return p;
  } else
    error(NIL, "too many subscripts", qsav);
}



/* Scoping functions 
 * for accessing arrays and matrix using the following syntax:
 *
 *  :m:(i j)
 *  (setq :m:(i j) 3)
 */

at *
array_getslot(at *obj, at *prop)
{
  int i;
  at *p, *arg, *ans;
  struct array *arr = obj->Object;
  struct class *cl = obj->Class;
  /* Checks */
  p = prop->Car;
  if (!LISTP(p))
    error(NIL,"Subscript(s) expected with this object",obj);
  for (i=0; i<arr->ndim; i++)
  {
    if (!CONSP(p))
      error(NIL,"Not enough subscripts for array access",obj);
    p = p->Cdr;
  }
  if (p)
    error(NIL,"Too many subscripts for array access",obj);
  /* Access */
  arg = new_cons(obj,prop->Car);
  ans = (*cl->list_eval)(obj,arg);
  UNLOCK(arg);
  if (prop->Cdr)
  {
    p = ans;
    ans = getslot(p,prop->Cdr);
    UNLOCK(p);
  }
  return ans;
}


void
array_setslot(at *obj, at *prop, at *val)
{
  int i;
  at *p, *arg;
  at **where;
  struct array *arr = obj->Object;
  struct class *cl = obj->Class;
  /* Build listeval argument */
  p = prop->Car;
  if (!LISTP(p))
    error(NIL,"Subscript(s) expected with this object",obj);
  arg = new_cons(obj,NIL);
  where = &(arg->Cdr);
  for (i=0; i<arr->ndim; i++)
  {
    if (!CONSP(p)) 
      error(NIL,"Not enough subscripts for array access",obj);
    *where = new_cons(p->Car,NIL);
    where = &((*where)->Cdr);
    p = p->Cdr;
  }
  if (p)
    error(NIL,"Too many subscripts for array access",obj);
  /* Access */
  if (prop->Cdr)
  {
    p = (*cl->list_eval)(obj,arg);
    UNLOCK(arg);
    setslot(&p, prop->Cdr, val);
    UNLOCK(p);
  }
  else
  {
    *where = new_cons(val,NIL);
    (*cl->list_eval)(obj,arg);
    UNLOCK(arg);
  }
}





/* --------- ARRAY AND MATRIX PUBLICTY */


/*
 * bound(p,n) returns the size of array or matrix P in his Nth dimension
 */
int 
bound(at *p, int n)
{
  struct array *arr;

  ifn(p && (p->flags & X_ARRAY))
    error(NIL, x_array_error, p);

  arr = p->Object;

  if (n == 0)
    return arr->ndim;
  if (n > 0 && n <= arr->ndim)
    return arr->dim[n - 1];
  error(NIL, "bad array or matrix dimension index", NIL);
}

DX(xbound)
{
  at *ans;		/* (bounds ARR)   returns the dimension list. */
  at *p;		/* (bounds ARR 0) returns the number of dims. */
  int d;		/* (bounds ARR D) , the LAST INDEX in Dth
				 * dim. */

  ALL_ARGS_EVAL;
  if (arg_number == 2) {
    d = AINTEGER(2);
    if (d == 0)
      ans = NEW_NUMBER(bound(APOINTER(1), d));
    else
      ans = NEW_NUMBER(bound(APOINTER(1), d) - 1);
  } else {
    ARG_NUMBER(1);
    ans = NIL;
    p = APOINTER(1);
    for (d = bound(p, (int) 0); d > 0; d--)
      ans = cons(NEW_NUMBER(bound(p, d)), ans);
  }
  return ans;
}


/*
 * arrayp(p) Returns TRUE if p is an array or a matrix
 */

int 
arrayp(at *p)
{
  if (p && (p->flags & C_EXTERN) && (p->Class == &array_class))
    return TRUE;
  else
    return FALSE;
}

DX(xarrayp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (arrayp(APOINTER(1)))
    return true();
  else
    return NIL;
}




/* ------- DATA DUPLICATION */


/*
 * copy-any-matrix copies a matrix into another, with conversions
 * and dimension changes.
 */

at *
copy_any_matrix(at *p, at *q)
{
  struct array *old, *arr;
  int d1[MAXDIMS], d2[MAXDIMS], mode;
  int j1, j2, off1, off2;

  ifn(p && (p->flags & X_ARRAY))
    error(NIL, x_array_error, p);
  old = p->Object;

  ifn(q) {
    if (p->Class == &matrix_class)
      q = matrix(old->ndim, old->dim);
    else if (p->Class == &pmatrix_class)
      q = pmatrix(old->ndim, old->dim);
    else if (p->Class == &dmatrix_class)
      q = dmatrix(old->ndim, old->dim);
    else if (p->Class == &smatrix_class)
      q = smatrix(old->ndim, old->dim);
    else if (p->Class == &imatrix_class)
      q = imatrix(old->ndim, old->dim);
    else if (p->Class == &bmatrix_class)
      q = bmatrix(old->ndim, old->dim);
    else if (p->Class == &array_class)
      q = array(old->ndim, old->dim);
    else
      error(copy_any_mat,unknown_array_type,NIL);
  } else {
    LOCK(q);
  }

  ifn(q && (q->flags & X_ARRAY))
    error(NIL, x_array_error, q);
  arr = q->Object;

  if (old->flags & PRINT_ARRAY)
    arr->flags |= PRINT_ARRAY;

  off1 = off2 = 1;
  for (j1=0; j1<old->ndim; j1++) {
    off1 *= old->dim[j1];
    d1[j1]=0;
  }
  for(j2=0; j2<arr->ndim; j2++) {
    off2 *= arr->dim[j2];
    d2[j2] = 0;
  }
  if (off1!=off2)
    error(NIL, "matrices have different number of elements", NIL);

  off1 = off2 = 0;

  mode = 0;
  if (p->Class == &matrix_class)
    mode |= 1;
  else if (p->Class == &pmatrix_class)
    mode |= 2;
  else if (p->Class == &array_class)
    mode |= 3;
  else if (p->Class == &dmatrix_class)
    mode |= 4;
  else if (p->Class == &imatrix_class)
    mode |= 5;
  else if (p->Class == &bmatrix_class)
    mode |= 6;
  else if (p->Class == &smatrix_class)
    mode |= 7;
  else
    error(copy_any_mat,unknown_array_type,p);

  if (q->Class == &matrix_class)
    mode |= 1<<4;
  else if (q->Class == &pmatrix_class)
    mode |= 2<<4;
  else if (q->Class == &array_class)
    mode |= 3<<4;
  else if (q->Class == &dmatrix_class)
    mode |= 4<<4;
  else if (q->Class == &imatrix_class)
    mode |= 5<<4;
  else if (q->Class == &bmatrix_class)
    mode |= 6<<4;
  else if (q->Class == &smatrix_class)
    mode |= 7<<4;
  else
    error(copy_any_mat,unknown_array_type,q);

  j1 = old->ndim;
  j2 = arr->ndim;

  while (j1>=0 && j2>=0) {

#ifdef DEBUG
    { int i;
      for (i=0;i<old->ndim;i++)
	printf("%d ",d1[i]);
      printf("--> ");
      for (i=0;i<arr->ndim;i++)
	printf("%d ",d2[i]);
      printf("\n");
    }
#endif

    switch (mode) {
    case 7*(16+1):
      ((short *) (arr->data))[off2] =
	((short *) (old->data))[off1];
      break;
    case 6*(16+1):
      ((unsigned char *) (arr->data))[off2] =
	((unsigned char *) (old->data))[off1];
      break;
    case 5*(16+1):
      ((int *) (arr->data))[off2] =
	((int *) (old->data))[off1];
      break;
    case 4*(16+1):
      ((real *) (arr->data))[off2] =
	((real *) (old->data))[off1];
      break;
    case 3*(16+1):
      ((at **) (arr->data))[off2] = p =
	((at **) (old->data))[off1];
      LOCK(p);
      break;
    case 2*(16+1):
      ((unsigned char *) (arr->data))[off2] =
	((unsigned char *) (old->data))[off1];
      break;
    case 1*(16+1):
      ((flt *) (arr->data))[off2] =
	((flt *) (old->data))[off1];
      break;
    default:
      {
	real x;
	
	switch (mode & 0xf) {
	case 1:
	  x = Ftor(((flt *) (old->data))[off1]);
	  break;
	case 2:
	  x = unpack(((unsigned char *) (old->data))[off1]);
	  break;
	case 3:
	  {
	    at *n;
	    
	    n = ((at **) (old->data))[off1];
	    ifn(n && (n->flags & C_NUMBER))
	      error(NIL, "array element is'nt a number", n);
	    x = n->Number;
	    break;
	  }
	case 4:
	  x = ((real *) (old->data))[off1];
	  break;
	case 5:
	  x = ((int *) (old->data))[off1];
	  break;
	case 6:
	  x = ((unsigned char*) (old->data))[off1];
	  break;
	case 7:
	  x = ((short *) (old->data))[off1];
	  break;
	default:
	  error(copy_any_mat,unhandled_array_type,NIL);
	}

	switch (mode & 0xf0) {
	case 1<<4:
	  ((flt *) (arr->data))[off2] = rtoF(x);
	  break;
	case 2<<4:
	  ((unsigned char *) (arr->data))[off2] = pack(x);
	  break;
	case 3<<4:
	  {
	    at **p = &(((at **) (arr->data))[off2]);
	    UNLOCK(*p);
	    *p = new_number(x);
	  }
	  break;
	case 4<<4:
	  ((real *) (arr->data))[off2] = x;
	  break;
	case 5<<4:
	  ((int *) (arr->data))[off2] = (int)(x);
	  break;
	case 6<<4:
	  if (x<0) 
	    x = 0;
	  else if (x>255)
	    x = 255;
	  ((unsigned char*) (arr->data))[off2] = (unsigned char)(x);
	  break;
	case 7<<4:
	  ((short *) (arr->data))[off2] = (short)(x);
	  break;
	default:
	  error(copy_any_mat,unhandled_array_type,NIL);
	}
      }
    }
    
    j1--;
    do {
      if (j1<0)
	break;
      if (++d1[j1] < old->dim[j1]) {
	off1 += old->modulo[j1];
	j1++;
      } else {
	CHECK_MACHINE("on");
	off1 -= old->dim[j1] * old->modulo[j1];
	d1[j1--] = -1;
      }
    } while (j1<old->ndim);
    
    j2--;
    do {
      if (j2<0)
	break;
      if (++d2[j2] < arr->dim[j2]) {
	off2 += arr->modulo[j2];
	j2++;
      } else {
	CHECK_MACHINE("on");
	off2 -= arr->dim[j2] * arr->modulo[j2];
	d2[j2--] = -1;
      }
    } while (j2<arr->ndim);
  }
  return q;
}

DX(xcopy_any_matrix)
{
  at *p;

  ALL_ARGS_EVAL;
  p = APOINTER(1);
  if (arg_number == 1)
    return copy_any_matrix(p, NIL);
  ARG_NUMBER(2);
  p = APOINTER(2);
  return copy_any_matrix(APOINTER(1), p);
}


DX(xcopy_matrix)
{
  at *p,*q;
  struct array *ap,*aq;
  int i;

  ALL_ARGS_EVAL;
  p = APOINTER(1);
  if (arg_number == 1)
    return copy_any_matrix(p, NIL);
  ARG_NUMBER(2);
  q = APOINTER(2);
  ifn (p && (p->flags&X_ARRAY))
    error(NIL,x_array_error,p); 
  ifn (q && (q->flags&X_ARRAY))
    error(NIL,x_array_error,q); 
  ap = p->Object;
  aq = q->Object;
  ifn (ap->ndim == aq->ndim)
    error(NIL,"matrices have different number of dimensions",NIL);
  for (i=0;i<ap->ndim;i++)
    ifn (ap->dim[i] == aq->dim[i])
      error(NIL,"matrices have different size",NIL);
  return copy_any_matrix(p,q);
}


/* ------- DATA COMPARISON */

int
array_compare(at *p, at *q, int order)
{
  int d[MAXDIMS];
  int i, j, mode, off1, off2;
  struct array *ap = p->Object;
  struct array *aq = q->Object;
  /* cannot rank objects */
  if (order)
    error(NIL,"Cannot rank matrices or arrays", NIL);
  /* check dimension */
  if (ap->ndim != aq->ndim)
    return 1;
  for (i=0; i<ap->ndim; i++)
    if (ap->dim[i] != aq->dim[i])
      return 1;
  /* mode */
  if (p->Class == &matrix_class)
    mode = 1;
  else if (p->Class == &pmatrix_class)
    mode = 2;
  else if (p->Class == &array_class)
    mode = 3;
  else if (p->Class == &dmatrix_class)
    mode = 4;
  else if (p->Class == &imatrix_class)
    mode = 5;
  else if (p->Class == &bmatrix_class)
    mode = 6;
  else if (p->Class == &smatrix_class)
    mode = 7;
  else
    error(NIL,unknown_array_type,p);
  /* initialize */
  off1 = off2 = 0;
  j = ap->ndim;
  for (i=0; i<MAXDIMS; i++)
    d[i] = 0;
  /* iterate */
  while (j>=0)
  {
    /* compare */
    switch(mode)
    {

    case 1: /* FMATRIX */
      if ( ((flt*)(ap->data))[off1] != 
           ((flt*)(aq->data))[off2] ) 
        return 1;
#if defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)
      { float delta = (float)(((flt*)(ap->data))[off1]-((flt*)(aq->data))[off2]);
        if (*(long*)&delta)   /* Visual C++ does handle NaN as IEEE says */
          return 1; 
      }
#endif
      break;

    case 4: /* DMATRIX */
      if ( ((real*)(ap->data))[off1] != 
           ((real*)(aq->data))[off2] ) 
        return 1;
#if defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)
      { float delta = (float)(((real*)(ap->data))[off1]-((real*)(aq->data))[off2]);
        if (*(long*)&delta)   /* Visual C++ does handle NaN as IEEE says */
          return 1; 
      }
#endif
      break;

    case 6: /* BMATRIX */
    case 2: /* PMATRIX */
      if ( ((unsigned char*)(ap->data))[off1] != 
           ((unsigned char*)(aq->data))[off2] ) 
        return 1;
      break;

    case 3: /* ARRAY */
      if (!eq_test( ((at**)(ap->data))[off1], 
                    ((at**)(aq->data))[off2] ) ) 
        return 1;
      break;

    case 5: /* IMATRIX */
      if ( ((int*)(ap->data))[off1] != 
           ((int*)(aq->data))[off2] ) 
        return 1;
      break;

    case 7: /* SMATRIX */
      if ( ((short*)(ap->data))[off1] != 
           ((short*)(aq->data))[off2] ) 
        return 1;
      break;
    }
    /* next element */
    j--;
    do {
      if (j<0)
	break;
      if (++d[j] < ap->dim[j]) {
	off1 += ap->modulo[j];
	off2 += aq->modulo[j];
	j++;
      } else {
	CHECK_MACHINE("on");
	off1 -= ap->dim[j] * ap->modulo[j];
	off2 -= aq->dim[j] * aq->modulo[j];
	d[j--] = -1;
      }
    } while (j<ap->ndim);
  }
  return 0;
}


unsigned long
array_hash(at *p)
{
  int d[MAXDIMS];
  int i, j, mode, off1;
  struct array *ap = p->Object;
  unsigned long x = 0;
  union {
    real r; 
    flt f; 
    int i; 
    short s; 
    unsigned char b;
    unsigned long x;
  } u;

  /* mode */
  if (p->Class == &matrix_class)
    mode = 1;
  else if (p->Class == &pmatrix_class)
    mode = 2;
  else if (p->Class == &array_class)
    mode = 3;
  else if (p->Class == &dmatrix_class)
    mode = 4;
  else if (p->Class == &imatrix_class)
    mode = 5;
  else if (p->Class == &bmatrix_class)
    mode = 6;
  else if (p->Class == &smatrix_class)
    mode = 7;
  else
    error(NIL,unknown_array_type,p);
  /* initialize */
  off1 = 0;
  j = ap->ndim;
  for (i=0; i<MAXDIMS; i++)
    d[i] = 0;
  /* iterate */
  while (j>=0)
  {
    /* hash */
    u.x = 0;
    switch (mode)
    {
    case 1:
      u.f = ((flt*)(ap->data))[off1];
      break;
    case 6:
    case 2:
      u.b = ((unsigned char*)(ap->data))[off1];
      break;
    case 3:
      u.x = hash_value(((at**)(ap->data))[off1]);
      break;
    case 4:
      u.r = ((real*)(ap->data))[off1];
      break;
    case 5:
      u.i = ((int*)(ap->data))[off1];
      break;
    case 7:
      u.s = ((short*)(ap->data))[off1];
      break;
    }
    x = (x<<1) | ((long)x<0 ? 0:1);
    x ^= u.x;
    /* next element */
    j--;
    do {
      if (j<0)
	break;
      if (++d[j] < ap->dim[j]) {
	off1 += ap->modulo[j];
	j++;
      } else {
	CHECK_MACHINE("on");
	off1 -= ap->dim[j] * ap->modulo[j];
	d[j--] = -1;
      }
    } while (j<ap->ndim);
  }
  return x;
}



/* ------- DATA REDESCRIPTION */

/*
 * submatrix extract a new descriptor on a part only of a same matrix
 * WARNING : maxd[i] excluded
 * NULL mind or maxd means extensive bounds are used.
 */

at *
submatrix(at *array, int *mind, int *maxd)
{
  struct array *old, *arr;
  int i, j, d;
  at *ans;

  old = array->Object;
  arr = allocate(&array_alloc);
  arr->flags = SUB_ARRAY;
  if (old->flags & PRINT_ARRAY)
    arr->flags |= PRINT_ARRAY;
  if (old->flags & MAPPED_ARRAY)
    arr->flags |= MAPPED_ARRAY;
  arr->data = NIL;
  if (old->flags & SUB_ARRAY)
    ans = old->main_array;
  else
    ans = array;
  LOCK(ans);
  arr->main_array = ans;

  arr->nr0 = NIL;
  arr->nr1 = NIL;

  ans = new_extern(array->Class, arr);

  ifn (mind)
    mind = mind0;
  ifn (maxd)
    maxd = old->dim;

  for (j = i = 0; i < old->ndim; i++) {
    if (mind[i] < 0 || mind[i] >= old->dim[i])
      error(NIL, subscript_range_error, NEW_NUMBER(mind[i]));
    if (maxd[i] > old->dim[i])
      error(NIL, subscript_range_error, NEW_NUMBER(maxd[i]));
    d = maxd[i] - mind[i];
    if (d > 0) {
      arr->dim[j] = d;
      arr->modulo[j++] = old->modulo[i];
    }
  }
  arr->ndim = j;
  for (i = d = 0; i < old->ndim; i++)
    d += mind[i] * old->modulo[i];

  if (array->Class == &matrix_class)
    arr->data = (flt *) (old->data) + d;
  else if (array->Class == &pmatrix_class)
    arr->data = (unsigned char *) (old->data) + d;
  else if (array->Class == &dmatrix_class)
    arr->data = (real *) (old->data) + d;
  else if (array->Class == &smatrix_class)
    arr->data = (short *) (old->data) + d;
  else if (array->Class == &imatrix_class)
    arr->data = (int *) (old->data) + d;
  else if (array->Class == &bmatrix_class)
    arr->data = (unsigned char *) (old->data) + d;
  else if (array->Class == &array_class)
    arr->data = (at **) (old->data) + d;
  else
    error(copy_any_mat, subscript_range_error, NIL);

  ans->flags |= X_ARRAY;
  return ans;
}

DX(xsubmatrix)
{
  int i;
  int mind[MAXDIMS], maxd[MAXDIMS];
  at *q;
  struct array *arr;

  if (arg_number < 2)
    ARG_NUMBER(-1);
  ALL_ARGS_EVAL;

  q = APOINTER(1);
  ifn(q && (q->flags & X_ARRAY))
    error(NIL, x_array_error, q);
  arr = q->Object;
  ARG_NUMBER(arr->ndim + 1);

  for (i = 0; i < arr->ndim; i++) {
    q = APOINTER(i + 2);
    ifn(q) {
      mind[i] = 0;
      maxd[i] = arr->dim[i];
    } else
    if (q->flags & C_NUMBER)
      mind[i] = maxd[i] = (int)(q->Number);
    else if (CONSP(q) && CONSP(q->Cdr) && !q->Cdr->Cdr &&
	     q->Car && (q->Car->flags & C_NUMBER) &&
	     q->Cdr->Car && (q->Cdr->Car->flags & C_NUMBER)) {
      mind[i] = (int)(q->Car->Number);
      maxd[i] = (int)(q->Cdr->Car->Number) + 1;
      if (maxd[i] <= mind[i])
	error(NIL, "min bound is greater than max bound", NIL);
    } else
      error(NIL, illegal_subscript_error, q);
  }
  return submatrix(APOINTER(1), mind, maxd);
}




/*
 * mtranspose returns a new descriptor on the same array switching two dimensions
 */


at *
mtranspose(at *m, int n1, int n2)
{
  at *ans;
  int i;
  struct array *arr;

  ifn(m && (m->flags & X_ARRAY))
    error(NIL, x_array_error, m);

  arr = m->Object;

  if( n1 > n2 ) {
    i = n1;
    n1 = n2;
    n2 = i;
  }

  if(arr->ndim <= n2 || n1 < 0) {
    error(NIL,"unproper dimension range",m);
  }

  ans = submatrix(m, NULL, NULL);
  arr = ans->Object;

  i = arr->dim[n2];
  arr->dim[n2] = arr->dim[n1];
  arr->dim[n1] = i;

  i = arr->modulo[n2];
  arr->modulo[n2] = arr->modulo[n1];
  arr->modulo[n1] = i;

  return ans;
}

DX(xmtranspose)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  return mtranspose(APOINTER(1),AINTEGER(2),AINTEGER(3));
}



/*
 * transpose returns a new descriptor on the same 2D-array switching the dimensions
 */


at *
transpose(at *m)
{
  at *ans;
  int i;
  struct array *arr;

  ifn(m && (m->flags & X_ARRAY))
    error(NIL, x_array_error, m);

  arr = m->Object;

  switch(arr->ndim)
    {
    case 2:
      /* no break */
      break;
    case 1:
      arr->dim[1]=1;
      break;
    default:
      error(NIL,_1D_2D_matrix_error,m);
    }

  ans = submatrix(m, NULL, NULL);
  arr = ans->Object;
  if (arr->ndim == 1) {
    arr->dim[1] = 1;
    arr->modulo[1] = 0;
    arr->ndim = 2;
  }
  i = arr->dim[0];
  arr->dim[0] = arr->dim[1];
  arr->dim[1] = i;
  
  i = arr->modulo[0];
  arr->modulo[0] = arr->modulo[1];
  arr->modulo[1] = i;
 
  return ans;
}

DX(xtranspose)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return transpose(APOINTER(1));
}



at *
diagonal(at *m)
{
  at *ans;
  struct array *arr;

  ifn(m && (m->flags & X_ARRAY))
    error(NIL, x_array_error, m);

  arr = m->Object;

  if (arr->ndim != 2)
    error(NIL, _2D_matrix_error, m);

  if (arr->dim[0] != arr->dim[1])
    error(NIL, square_matrix_error, m);

  ans = submatrix(m, NULL, NULL);
  arr = ans->Object;
  arr->ndim = 1;
  arr->modulo[0] += arr->modulo[1];

  return ans;
}

DX(xdiagonal)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return diagonal(APOINTER(1));
}


at *
matrix_rotate(at *m, int n)
{
  at *ans;
  int sz;
  struct array *arr;

  n = ((n%4)+4)%4;
  if( n%2 )
    {
      ans = transpose(m);
    }
  else
    {
      ans = submatrix(m,NULL,NULL);
    }

  sz = array_elt_size(m);

  arr = ans->Object;

  switch(n)
    {
    case 0:
      /* do nothing */
      break;
    case 1:
      /* reverse dimension 1 */
      arr->data = (char *)(arr->data) + sz * arr->modulo[1] * (arr->dim[1] -1);
      arr->modulo[1] = - arr->modulo[1];
      break;
    case 2:
      /* reverse dimension 0 */
      arr->data = (char *)(arr->data) + sz * arr->modulo[0] * (arr->dim[0] -1);
      arr->modulo[0] = - arr->modulo[0];
      /* reverse dimension 1 */
      arr->data = (char *)(arr->data) + sz * arr->modulo[1] * (arr->dim[1] -1);
      arr->modulo[1] = - arr->modulo[1];
      break;
    case 3:
      /* reverse dimension 0 */
      arr->data = (char *)(arr->data) + sz * arr->modulo[0] * (arr->dim[0] -1);
      arr->modulo[0] = - arr->modulo[0];
      break;
    }

  return ans;
}

DX(xmatrix_rotate)
{
  int n;
  ALL_ARGS_EVAL;
  switch(arg_number)
    {
    case 1:
      n = 1;
      break;
    case 2:
      n = AINTEGER(2);
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }

  return matrix_rotate(APOINTER(1),n);
}


at *
matrix_split_dim(at *m, int n, int j, int *t)
{
  at *ans;
  struct array *arr;
  int i, s, u=-1;

  ifn(m && (m->flags & X_ARRAY))
    error(NIL, x_array_error, m);

  arr = m->Object;


  /* n < 0 for the lowest-weight */
  if( n < 0 )
    {
      n += arr->ndim;
    }

  /* check the dimensions are meaningfull and respect MAXDIMS */
  if( n < 0 || n > (arr->ndim -1) )
    error(NIL,dimension_error,m);
  if( arr->ndim + j > MAXDIMS )
    error(NIL,"Too many dimensions",NIL);

  /* get the volume of data to split */
  s = 1;
  for( i=0 ; i<j ; i++ )
    {
      if( ! t[i] )
	{
	  if( u >= 0 )
	    error(0,unspecified_dimension,0);
	  u = i;
	}
      else
	{
	  s *= t[i];
	}
    }

  /* if needed compute the unique unspecified diemnsion */
  if( u >= 0 )
    {
      t[u] = arr->dim[n] / s;
      s *= t[u];
    }

  /* check volume equality */
  if( arr->dim[n] != s )
    error(NIL,"unconsistent dimension split",m);


  /* get a raw version of the result */
  ans = submatrix(m,NULL,NULL);
  arr = ans->Object;

  /* displace the low-weight dimensions for safety */
  for( i=arr->ndim -1 ; i>n ; i-- )
    {
      arr->dim[i+j-1] = arr->dim[i];
      arr->modulo[i+j-1] = arr->modulo[i];
    }

  s = arr->modulo[n] * arr->dim[n];
  /* store the new dimensions and compute the new modulos */
  for( i=n ; i<n+j ; i++ )
    {
      arr->dim[i] = t[i-n];
      s /= t[i-n];
      arr->modulo[i] = s;
    }
  arr->ndim += j-1;


  /* that is all folks */
  return ans;
}


DX(xmatrix_split_dim)
{
  int j, t[MAXDIMS];
  at *l;

  ALL_ARGS_EVAL;
  ARG_NUMBER(3);

  for( j=0, l = ACONS(3); l ; l=l->Cdr, j++ )
    {
      if(j>=MAXDIMS)
	error(NIL,"Too many dimensions",NIL);

      if(!CONSP(l))
	DX_ERROR(2,3);
      if(l->Car && !NUMBERP(l->Car))
	DX_ERROR(1,3);
      t[j] = (int)(l->Car ? l->Car->Number : 0);
    }

  return matrix_split_dim(APOINTER(1),AINTEGER(2),j,t);
}


at * 
matrix_merge_dim(at *m, int n, int l)
{
  at *ans;
  int i, s;
  struct array *arr;
  
  ifn(m && (m->flags & X_ARRAY))
    error(NIL, x_array_error, m);

  arr = m->Object;

  /* n < 0 for the lowest-weight */
  if( n < 0 )
    {
      n += arr->ndim;
    }

  if( n<0 || l<0 || n+l > arr->ndim )
    error(NIL,dimension_error,m);

  /* check consistency of various offsets and compute data volume */
  s = 1;
  for( i=n+1 ; i<n+l ; i++ )
    {
      if( arr->modulo[i-1] != arr->modulo[i] * arr->dim[i] )
	return 0;

      s *= arr->dim[i];
    }

  /* get a raw version of the result */
  ans = submatrix(m,NULL,NULL);
  arr = ans->Object;

  /* set the merged dimension */
  arr->dim[n] *= s;
  arr->modulo[n] /= s;
  arr->ndim += 1-l;
  
  /* displace low-weights dimensions */
  for( i=n+1 ; i<n+l ; i++ )
    {
      arr->dim[i] = arr->dim[i+l-1];
      arr->modulo[i] = arr->modulo[i+l-1];
    }

  /* that is all folks */
  return ans;
}


DX(xmatrix_merge_dim)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  return matrix_merge_dim(APOINTER(1),AINTEGER(2),AINTEGER(3));
}



/*
 * munfold_dim
 * Returns a splitted matrix with possible overlap/gaps
 *  Arguments are:
 *   atm: TLisp object containing a matrix
 *   n  : rank of dimension to split
 *   dn : lower-weight new dimension
 *   st : step (further modulo factor)
 */

at *
munfold_dim(at *atm, int n, int dn, int st)
{
  struct array *m;
  int i;

  ifn(atm && (atm->flags & X_ARRAY))
    error(NIL, x_array_error, atm);

  m = atm->Object;

  /* n < 0 for the lowest-weight */
  if( n < 0 )
    {
      n += m->ndim;
    }

  /* check the dimensions are meaningfull and respect MAXDIMS */
  if( n < 0 || n > (m->ndim -1) )
    error(NIL,dimension_error,atm);
  if( m->ndim + 1 > MAXDIMS )
    error(NIL,"Too many dimensions",NIL);

  /* check the overlaping split is consistent */
  if( dn <= 0 || st <= 0 )
    error(0,"dimensions must be positive",0);
  if( (m->dim[n] - dn) % st )
    error(0,"unconsistent dimension overlaping split",0);

  /* get a raw version of the result */
  atm = submatrix(atm,NULL,NULL);
  m = atm->Object;

  /* displace the low-weight dimensions for safety */
  for( i=m->ndim -1 ; i>n ; i-- )
    {
      m->dim[i+1] = m->dim[i];
      m->modulo[i+1] = m->modulo[i];
    }

  /* store the new dimensions and compute the new modulos */
  m->ndim++;
  m->dim[n+1] = dn;
  m->modulo[n+1] = m->modulo[n];
  m->dim[n] = 1 + ((m->dim[n] - dn) / st);
  m->modulo[n] *= st;

  return atm;
}

DX(xmunfold_dim)
{

  ALL_ARGS_EVAL;
  ARG_NUMBER(4);

  return munfold_dim(APOINTER(1),AINTEGER(2),AINTEGER(3),AINTEGER(4));
}


at *
mrep_dim(at *atm, int n, int k)
{
  struct array *m;
  int i;

  ifn(atm && (atm->flags & X_ARRAY))
    error(NIL, x_array_error, atm);

  m = atm->Object;

  /* n < 0 for the lowest-weight */
  if( n < 0 )
    {
      n += m->ndim;
    }

  /* check the dimensions are meaningfull and respect MAXDIMS */
  if( n < 0 || n > (m->ndim -1) )
    error(NIL,dimension_error,atm);
  if( m->ndim + 1 > MAXDIMS )
    error(NIL,"Too many dimensions",NIL);

  /* get a raw version of the result */
  atm = submatrix(atm,NULL,NULL);
  m = atm->Object;

  /* displace the low-weight dimensions for safety */
  for( i=m->ndim -1 ; i>=n ; i-- )
    {
      m->dim[i+1] = m->dim[i];
      m->modulo[i+1] = m->modulo[i];
    }

  /* decide if lower or higher weight will receive k
     */
  if( k < 0 ) /* higher */
    {
      n++;
      k = -k;
    }

  /* store the new dimensions and compute the new modulos */
  m->ndim++;
  m->dim[n] = k;
  m->modulo[n] = 0;

  return atm;
}

DX(xmrep_dim)
{

  ALL_ARGS_EVAL;
  ARG_NUMBER(3);

  return mrep_dim(APOINTER(1),AINTEGER(2),AINTEGER(3));
}



/* --------- INITIALISATION CODE --------- */

void 
init_array(void)
{
  int i;
  
  class_define("ARRAY" ,&array_class );
  array_class.dontdelete = 1;
  for(i=0;i<MAXDIMS;i++)
    mind0[i]=0;
  
  dx_define("array"    , xarray);
  dx_define("bound"    , xbound);
  dx_define("arrayp"   , xarrayp);
  dx_define("copy_matrix", xcopy_matrix);
  dx_define("copy_any_matrix", xcopy_any_matrix);
  dx_define("submatrix", xsubmatrix);
  dx_define("transpose", xtranspose);
  dx_define("mtranspose", xmtranspose);
  dx_define("diagonal" , xdiagonal);
  dx_define("rotate"   , xmatrix_rotate);
  dx_define( "mmerge-dim"  , xmatrix_merge_dim );
  dx_define( "msplit-dim"  , xmatrix_split_dim );
  dx_define( "munfold-dim" , xmunfold_dim      );
  dx_define( "mrep_dim"    , xmrep_dim         );
}
