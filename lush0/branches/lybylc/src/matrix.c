/*   Lush Lisp interpreter and development tools
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
  T-Lisp3: (C) LYB YLC 1988
  matrix.c
  - array and matrix class definition
  $Id: matrix.c,v 0.1.1.1 2001-10-31 17:31:20 profshadoko Exp $
********************************************************************** */

#ifdef WIN32
#ifndef HAVE_MMAP
#define HAVE_MMAP
#endif
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif /* WIN32 */

#include "header.h"

#ifdef HAVE_MMAP
#ifdef HAVE_SYS_MMAN_H
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif 
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif


/* From ARRAY.C */

char *array_name(at *p);
int array_compare(at *p, at* q, int order);
unsigned long array_hash(at *p);
at *array_getslot(at *obj, at *prop);
void array_setslot(at *obj, at *prop, at *val);

at *array_ref(at *(*aref)(void*, int), 
              struct array *arr, at **p);

at *array_set(void (*aset)(void*, int, at*), 
              struct array *arr, at **p, at *value, int mode);



/* Matrix use extensively array's routines and structures */


/* ---- CLASS FUNCTIONS AND DEFINITION */


static void 
matrix_dispose(at *p)
{
  struct array *arr;

  arr = p->Object;

  if (arr->flags & SUB_ARRAY) 
    {
      UNLOCK(arr->main_array);
    } 
  else if (arr->flags & ALLOCATED_ARRAY)
    {
      free(arr->data);
    }
#ifdef HAVE_MMAP
  else if (arr->flags & MAPPED_ARRAY) 
    {
      int i, size;
      char *start, *end;
      
      if (p->Class == &pmatrix_class)
	size = sizeof(char);
      else if (p->Class == &matrix_class)
	size = sizeof(flt);
      else if (p->Class == &bmatrix_class)
	size = sizeof(unsigned char);
      else if (p->Class == &imatrix_class)
	size = sizeof(int);
      else if (p->Class == &dmatrix_class)
	size = sizeof(real);
      else if (p->Class == &smatrix_class)
	size = sizeof(short);
      else
	error("matrix.c/submatrix",
	      "internal error: unknown matrix type",NIL);
      
      for (i = 0; i < arr->ndim; i++)
	size *= arr->dim[i];
      start = (char*)((long)(arr->data)&0xffffff00);
      end = (char*)(arr->data) + size;
#ifdef UNIX
      munmap(start, end-start);
#endif
#ifdef WIN32
      UnmapViewOfFile(start);
      if (arr->xtra)
	CloseHandle((HANDLE)(arr->xtra));
      arr->xtra = 0;
#endif
    }
#endif
  if (arr->nr0)
    free(arr->nr0);
  if (arr->nr1)
    free(arr->nr1);
  deallocate(&array_alloc, (struct empty_alloc *) arr);
}

static void 
matrix_action(at *p, void (*action) (at *))
{
  struct array *arr;

  arr = p->Object;
  if (arr->flags & SUB_ARRAY)
    (*action) (arr->main_array);
}



static at *matrix_listeval(at *p, at *q);	/* forward */
class matrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  matrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};

static at *pmatrix_listeval(at *p, at *q);	/* forward */
class pmatrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  pmatrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};

static at *dmatrix_listeval(at *p, at *q);	/* forward */
class dmatrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  dmatrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};

static at *smatrix_listeval(at *p, at *q);	/* forward */
class smatrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  smatrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};

static at *imatrix_listeval(at *p, at *q);	/* forward */
class imatrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  imatrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};

static at *bmatrix_listeval(at *p, at *q);	/* forward */
class bmatrix_class =
{
  matrix_dispose,
  matrix_action,
  array_name,
  generic_eval,
  bmatrix_listeval,
  generic_serialize,
  array_compare,
  array_hash,
  array_getslot,
  array_setslot
};










/* ----- MATRIX CREATION */


/*
 * matrix(nd,d) pmatrix(nd,d) imatrix(nd,d) dmatrix(nd,d) smatrix(nd,d)
 * return a new matrix with nd dimensions
 * (d[0],d[1] etc.. )
 */


static at *
any_matrix(int nd, int *d, int elsize, TLclass *mclass)
{
  at *ans;
  struct array *arr;
  int i, size;

  arr = allocate(&array_alloc);
  arr->flags = 0;
  arr->ndim = nd;
  arr->data = NIL;
  arr->main_array = NIL;
  arr->nr0 = NIL;
  arr->nr1 = NIL;

  ans = new_extern(mclass, arr);
  size = 1;
  for (i = nd - 1; i >= 0; i--) {
    if (d[i] < 1)
      error(NIL, "negative or zero size", NIL);
    arr->dim[i] = d[i];
    arr->modulo[i] = size;
    size *= d[i];
  }
  size *= elsize;
  ifn(arr->data = tl_malloc(size))
    error(NIL, "no room for a new array", NIL);
  memset(arr->data, 0, size);
  arr->flags |= ALLOCATED_ARRAY;
  ans->flags |= X_ARRAY;
  return ans;
}

static at * 
any_xmatrix(int arg_number, at **arg_array, int elsize, TLclass *mclass)
{
  int nd, i;
  int dim[MAXDIMS];
  nd = arg_number;
  if (nd < 1 || nd > MAXDIMS)
    ARG_NUMBER(-1);
  ALL_ARGS_EVAL;
  for (i = 0; i < nd; i++)
    dim[i] = AINTEGER(i + 1);
  return any_matrix(nd, dim, elsize, mclass);
}


at *
fmatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(flt),&matrix_class);
}
DX(xfmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(flt),&matrix_class);
}


at *
matrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(flt),&matrix_class);
}
DX(xmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(flt),&matrix_class);
}


at *
pmatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(char),&pmatrix_class);
}
DX(xpmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(char),&pmatrix_class);
}


at *
dmatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(real),&dmatrix_class);
}
DX(xdmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(real),&dmatrix_class);
}


at *
smatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(short),&smatrix_class);
}
DX(xsmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(short),&smatrix_class);
}


at *
imatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(int),&imatrix_class);
}
DX(ximatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(int),&imatrix_class);
}


at *
bmatrix(int nd, int *d)
{
  return any_matrix(nd,d,sizeof(unsigned char),&bmatrix_class);
}
DX(xbmatrix) 
{
  return any_xmatrix(arg_number,arg_array,sizeof(unsigned char),&bmatrix_class);
}













/* ---- MATRIX ACCESS (LISTEVAL) FUNCTION */

/*
 * pack unpack
 * 
 * utilities for matrix in packed format
 */

real 
unpack(int b)
{
  if (b & 0x80)
    b |= ~0x7fL;
  else
    b &= 0x7fL;
  return (real) b / 16.0;
}

unsigned char 
pack(double x)
{
  if (x > 8.0 - 1.0 / 16.0)
    return 0x7fL;
  else if (x < -8.0)
    return 0x80L;
  else
    return (unsigned char)(x * 16.0);
}


/* functions for acceding a single matrix element */


static at *
matrixR(void *data, int depl)
{
  float *start = data;
  return NEW_NUMBER(Ftor(start[depl]));
}

static void
matrixS(void *data, int depl, at *val)
{
  float *start = data;
  if (val && (val->flags & C_NUMBER))
    start[depl] = rtoF(val->Number);
  else if (!val)
    start[depl] = Fzero;
  else
    error(NIL, "number expected", val);
}

static at *
pmatrixR(void *data, int depl)
{
  unsigned char *start = data;
  return NEW_NUMBER(unpack(start[depl]));
}

static void
pmatrixS(void *data, int depl, at *val)
{
  unsigned char *start = data;

  if (val && (val->flags & C_NUMBER))
    start[depl] = pack(val->Number);
  else if (!val)
    start[depl] = 0;
  else
    error(NIL, "number expected", val);
}

static at *
dmatrixR(void *data, int depl)
{
  double *start = data;
  return NEW_NUMBER(start[depl]);
}

static void
dmatrixS(void *data, int depl, at *val)
{
  double *start = data;
  if (val && (val->flags & C_NUMBER))
    start[depl] = val->Number;
  else if (!val)
    start[depl] = 0.0;
  else
    error(NIL, "number expected", val);
}

static at *
smatrixR(void *data, int depl)
{
  short int *start = data;
  return NEW_NUMBER(start[depl]);
}

static void
smatrixS(void *data, int depl, at *val)
{
  short int *start = data;
  if (val && (val->flags & C_NUMBER))
    start[depl] = (short)(val->Number);
  else if (!val)
    start[depl] = 0;
  else
    error(NIL, "number expected", val);
}

static at *
imatrixR(void *data, int depl)
{
  int *start = data;
  return NEW_NUMBER(start[depl]);
}

static void
imatrixS(void *data, int depl, at *val)
{
  int *start = data;
  if (val && (val->flags & C_NUMBER))
    start[depl] = (int)(val->Number);
  else if (!val)
    start[depl] = 0;
  else
    error(NIL, "number expected", val);
}

static at *
bmatrixR(void *data, int depl)
{
  unsigned char *start = data;
  return NEW_NUMBER(start[depl]);
}

static void
bmatrixS(void *data, int depl, at *val)
{
  unsigned char *start = data;
  real w;
  if (val && (val->flags & C_NUMBER)) 
    w = (int)(val->Number);
  else if (!val)
    w = 0;
  else
    error(NIL, "number expected", val);
  if (w<0)
    start[depl] = 0;
  else if (w>255)
    start[depl] = 255;
  else
    start[depl] = (unsigned char)w;
}



/* listeval functions from ARRAY.C */


static at *
any_matrix_listeval(at *p, at *q, 
		    at *(*funcR)(void*, int), 
		    void (*funcS)(void*, int, at*))
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
    q = array_ref(funcR, arr, myp);
    UNLOCK(qsav);
    return q;
  } else if (CONSP(q) && (!q->Cdr)) {
    array_set(funcS, arr, myp, q->Car, 1);
    UNLOCK(qsav);
    LOCK(p)
      return p;
  } else
    error(NIL, "too many subscripts", qsav);
}



static at *
matrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,matrixR,matrixS);
}

static at *
pmatrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,pmatrixR,pmatrixS);
}

static at *
dmatrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,dmatrixR,dmatrixS);
}

static at *
smatrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,smatrixR,smatrixS);
}

static at *
imatrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,imatrixR,imatrixS);
}

static at *
bmatrix_listeval(at *p, at *q)
{
  return any_matrix_listeval(p,q,bmatrixR,bmatrixS);
}








/* MATRIX SPECIFICS FUNCTIONS */


int 
matrixp(at *p)
{
  if (p && (p->flags & C_EXTERN) && (p->flags & X_ARRAY))
    if (p->Class != &array_class)
      return TRUE;
  return FALSE;
}

DX(xmatrixp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (matrixp(APOINTER(1)))
    return true();
  else
    return NIL;
}

static int 
mappedmatrixp(at *p)
{
  struct array *arr;
  if (matrixp(p)) {
    arr = p->Object;
    if (arr->flags & MAPPED_ARRAY)
      return TRUE;
  }
  return FALSE;
}

DX(xmappedmatrixp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (mappedmatrixp(APOINTER(1)))
    return true();
  else
    return NIL;
}

/*
 * (matrix_print_mode  ARRAY | MATRIX   [ EXPR ] ) sets or reads printmode
 * flag.
 */


DX(xmatrix_print_mode)
{
  at *p;
  struct array *arr;

  ALL_ARGS_EVAL;
  if (arg_number != 2 && arg_number != 1)
    ARG_NUMBER(-1);
  p = APOINTER(1);
  ifn(p && (p->flags & X_ARRAY))
    error(NIL, "not an array or a matrix", p);
  arr = p->Object;
  if (arg_number == 2) {
    if (APOINTER(2) && arr->ndim <= 2)
      arr->flags |= PRINT_ARRAY;
    else
      arr->flags &= ~PRINT_ARRAY;
  }
  if (arr->flags & PRINT_ARRAY)
    return true();
  else
    return NIL;
}














/* ---- MATRIX FILE FUNCTIONS */


#define BINARY_MATRIX    (0x1e3d4c51)
#define PACKED_MATRIX	 (0x1e3d4c52)
#define DOUBLE_MATRIX	 (0x1e3d4c53)
#define INTEGER_MATRIX	 (0x1e3d4c54)
#define BYTE_MATRIX	 (0x1e3d4c55)
#define SHORT_MATRIX	 (0x1e3d4c56)
#define ASCII_MATRIX	 (0x2e4d4154)	/* '.MAT' */

#define SWAP(x) ((int)(((x&0xff)<<24)|((x&0xff00)<<8)|\
		      ((x&0xff0000)>>8)|((x&0xff000000)>>24))) 

static int 
read4(FILE *f)
{
  int i;
  int status;
  status = fread(&i, sizeof(int), 1, f);
  if (status != 1)
    test_file_error(f);
  return i;
}

static int 
write4(FILE *f, unsigned int l)
{
  int status;
  status = fwrite(&l, sizeof(int), 1, f);
  if (status != 1)
    test_file_error(f);
  return l;
}


/* --------- Utilities */


static void 
mode_check(at *p, int *size_p, int *elem_p, int *mode_p)
{
  struct array *arr;
  int elsize;
  int mode;
  int size;
  int i;

  if (p->Class==&matrix_class) {
    elsize = sizeof(flt);
    mode = 1;
  } else if (p->Class==&pmatrix_class) {
    elsize = sizeof(char);
    mode = 2;
  } else if (p->Class==&array_class) {
    elsize = 4;
    mode = 3;
  } else if (p->Class==&dmatrix_class) {
    elsize = sizeof(real);
    mode = 4;
  } else if (p->Class==&imatrix_class) {
    elsize = sizeof(int);
    mode = 5;
  } else if (p->Class==&bmatrix_class) {
    elsize = sizeof(unsigned char);
    mode = 6;
  } else if (p->Class==&smatrix_class) {
    elsize = sizeof(short);
    mode = 7;
  } else {
    error("matrix.c/mode_check", "internal error: unknown matrix type",p);
  }

  size = 1;
  arr = p->Object;
  for (i=0; i<arr->ndim; i++) {
    size *= arr->dim[i];
  }
  *size_p = size;
  *elem_p = elsize;
  *mode_p = mode;
}


int 
array_elt_size(at *p)
{
  int sz, esz, md;
  mode_check( p, &sz, &esz, &md );
  return esz;
}


static void 
contiguity_check(at *p)
{
  struct array *arr;
  int size, i;
  
  arr = p->Object;

  if (arr->flags & MAPPED_ARRAY)
    error(NIL,"Mapped matrix are forbidden here",p);
  
  size = 1;
  for (i=arr->ndim-1; i>=0; i--) {
    if (size != arr->modulo[i])
      error(NIL,"You must use a fresh contiguous matrix",p);
    size *= arr->dim[i];
  }
}





/* -------- Importing a Matrix */





/*
 * import_raw_matrix(p,f,n) 
 * import_text_matrix(p,f)
 */

void 
import_raw_matrix(at *p, FILE *f, int offset)
{
  struct array *arr;
  int size, elsize, mode;
  char *pntr;
  char junk;
  
  ifn (matrixp(p))
    error(NIL,"Not a matrix",p);

  mode_check(p, &size, &elsize, &mode);
  contiguity_check(p);
  
  if (mode == 3)
    error(NIL,"arrays are forbidden here",p);
  
  while (offset>0) {
    if (fread(&junk,1,1,f) < 1) {
      test_file_error(f);
      error(NIL, "file is too short", NIL);
    }
    offset--;
  }
  
  arr = p->Object;
  pntr = arr->data;
  if (fread(pntr, elsize, (size_t)size, f) < (size_t)size) {
    test_file_error(f);
    error(NIL, "file is too short", NIL);
  }
}

DX(ximport_raw_matrix)
{
  int offset = 0;
  at *p;
  
  ALL_ARGS_EVAL;
  if (arg_number==3)
    offset = AINTEGER(3);
  else 
    ARG_NUMBER(2);
  
  if (ISSTRING(2)) {
    p = OPEN_READ(ASTRING(2), NULL);
  } else {
    p = APOINTER(2);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class)) 
      error(NIL, "not a string or read descriptor", p);
    LOCK(p);
  }
  
  import_raw_matrix(APOINTER(1),p->Object,offset);
  
  UNLOCK(p);
  p = APOINTER(1);
  LOCK(p);
  return p;
}





void 
import_text_matrix(at *p, FILE *f)
{
  struct array *arr;
  int size, elsize, mode;
  real x;
  int i;
  gptr data;
  
  ifn (matrixp(p))
    error(NIL,"Not a matrix",p);

  mode_check(p, &size, &elsize, &mode);
  contiguity_check(p);
  arr = p->Object;
  data = arr->data;
  
  for (i=0; i<size; i++) {
    if (fscanf(f, "%lf ", &x) != 1)
      error(NIL,"Cannot read a number",NIL);
    switch (mode) {

    case 1:
      ((flt *)(data))[i] = rtoF(x);
      break;

    case 2:
      ((char *)(data))[i] = pack(x);
      break;

    case 3:
      p = ((at**)(data))[i];
      UNLOCK(p);
      p = NEW_NUMBER(x);
      ((at**)(data))[i] = p;
      break;
      
    case 4:
      ((real *)(data))[i] = (real)x;
      break;
      
    case 5:
      ((int *)(data))[i] = (int)x;
      break;

    case 6:
      if (x<0) x = 0;
      if (x>255) x = 255;
      ((unsigned char *)(data))[i] = (unsigned char)x;
      break;
      
    case 7:
      ((short *)(data))[i] = (short)x;
      break;

    default:
      error("matrix.c/import_text_matrix",
	    "internal error: unknown matrix type",p);
    }
  }
}


DX(ximport_text_matrix)
{
  at *p;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  
  if (ISSTRING(2)) {
    p = OPEN_READ(ASTRING(2), NULL);
  } else {
    p = APOINTER(2);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class)) 
      error(NIL, "not a string or read descriptor", p);
    LOCK(p);
  }
  
  import_text_matrix(APOINTER(1),p->Object);
  
  UNLOCK(p);
  p = APOINTER(1);
  LOCK(p);
  return p;
}






/* -------- Saving a Matrix */


/* save_matrix_len(p)
 * returns the number of bytes written by a save_matrix!
 */

int 
save_matrix_len(at *p)
{
  struct array *arr;
  int size, elsize, mode, ndim;
  int count = 0;
  mode_check(p, &size, &elsize, &mode);
  arr = p->Object;
  ndim = arr->ndim;
  if (ndim<3)			/* header */
    count += 5 * sizeof(int);
  else
    count += (ndim+2) * sizeof(int);
  count += elsize*size;		/* data */
  return count;
}


/* BINARY */


/*
 * save_matrix(p,f) 
 * save_ascii_matrix(p,f)
 */

static void 
format_save_matrix(at *p, FILE *f, int h)
{
  struct array *arr;
  int j, offset, ndim;
  int d[MAXDIMS];

  int magic;
  int elsize;
  char *data;

  ifn (matrixp(p))
    error(NIL,"Not a matrix",p);

  if (p->Class==&matrix_class) {
    magic = BINARY_MATRIX;
    elsize = sizeof(flt);
  } else if (p->Class==&pmatrix_class) {
    magic = PACKED_MATRIX;
    elsize = sizeof(char);
  } else if (p->Class==&dmatrix_class) {
    magic = DOUBLE_MATRIX;
    elsize = sizeof(real);
  } else if (p->Class==&smatrix_class) {
    magic = SHORT_MATRIX;
    elsize = sizeof(short);
  } else if (p->Class==&imatrix_class) {
    magic = INTEGER_MATRIX;
    elsize = sizeof(int);
  } else if (p->Class==&bmatrix_class) {
    magic = BYTE_MATRIX;
    elsize = sizeof(unsigned char);
  } else if (p->Class==&array_class) {
    error(NIL,"Cannot save an array of lisp objects",p);
  } else {
    error("matrix.c/save_matrix",
	  "internal error: unknown matrix type",p);
  }
  
  arr = p->Object;
  ndim = arr->ndim;
  offset = 0;
  for (j = 0; j < ndim; j++)
    d[j] = 0;
  
  if( h )
    {
      write4(f, magic);
      write4(f, arr->ndim);
      for (j = 0; j < arr->ndim; j++)
	write4(f, arr->dim[j]);
      while (j++ < 3)
	write4(f, 1);	/* SN1 compatibility */
    }

  data = arr->data;
  j = ndim;
  while (j >= 0) {
    if (j == ndim) {
      j--;
      if (fwrite(  data+elsize*offset, elsize, 1, f) != 1) {
	test_file_error(f);
	error(NIL,"Unknown error while writing",NIL);
      }
    }
    if (++d[j] < arr->dim[j])
      offset += arr->modulo[j++];
    else {
      offset -= arr->dim[j] * arr->modulo[j];
      d[j--] = -1;
    }
  }
}

void
save_matrix(at *p, FILE *f)
{
  format_save_matrix(p, f, (int)1);
}

void
export_matrix(at *p, FILE *f)
{
  format_save_matrix(p, f, (int)0);
}

DX(xsave_matrix)
{
  at *p, *ans;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), "mat");
    ans = new_string(file_name);
  } else {
    p = APOINTER(2);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_W_class)) 
      error(NIL, "not a string or write descriptor", p);
    LOCK(p);
    LOCK(p);
    ans = p;
  }
  save_matrix(APOINTER(1), p->Object);
  UNLOCK(p);
  return ans;
}

DX(xexport_matrix)
{
  at *p, *ans;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), NULL);
    ans = new_string(file_name);
  } else {
    p = APOINTER(2);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_W_class)) 
      error(NIL, "not a string or write descriptor", p);
    LOCK(p);
    LOCK(p);
    ans = p;
  }
  export_matrix(APOINTER(1), p->Object);
  UNLOCK(p);
  return ans;
}


/* ASCII */


static void 
format_save_ascii_matrix(at *p, FILE *f, int h)
{
  struct array *arr;
  int j, offset, ndim;
  int size, elsize, mode;
  int d[MAXDIMS];
  gptr data;
  real x;

  ifn (matrixp(p))
    error(NIL,"Not a matrix",p);

  mode_check(p, &size, &elsize, &mode);
  
  arr = p->Object;
  ndim = arr->ndim;
  data = arr->data;
  offset = 0;
  for (j = 0; j < ndim; j++)
    d[j] = 0;

  FMODE_TEXT(f);
  if( h )
    {
      fprintf(f, ".MAT %d", arr->ndim); 
      for (j = 0; j < arr->ndim; j++)
	fprintf(f, " %d", arr->dim[j]);
      fprintf(f, "\n");
    }

  j = ndim;
  while (j >= 0) {
    if (j == ndim) {
      j--;
      switch (mode) {
      case 1:
	x = Ftor(((flt *) (data))[offset]);
	break;
      case 2:
	x = unpack(((unsigned char *) (data))[offset]);
	break;
      case 3:
	{
	  at *n;
	  n = ((at **) (data))[offset];
	  ifn(n && (n->flags & C_NUMBER)) {
	    FMODE_BINARY(f);
	    error(NIL, "array element is not a number", n);
	  }
	  x = n->Number;
	  break;
	}
      case 4:
	x = ((real *) (data))[offset];
	break;
      case 5:
	x = ((int *) (data))[offset];
        break;
      case 6:
	x = ((unsigned char*) (data))[offset];
        break;
      default:
        FMODE_BINARY(f);
        error("matrix.c/save_ascii_matrix",
	      "internal error: unknown matrix type",p);
      }
      fprintf(f, "%6.4f\n", x);
    }
    if (++d[j] < arr->dim[j])
      offset += arr->modulo[j++];
    else {
      offset -= arr->dim[j] * arr->modulo[j];
      d[j--] = -1;
      FMODE_BINARY(f);
      CHECK_MACHINE("on");
      FMODE_TEXT(f);
    }
  }
  FMODE_TEXT(f);
}

void
save_ascii_matrix(at *p, FILE *f)
{
  format_save_ascii_matrix(p, f, (int)1);
}

void
export_ascii_matrix(at *p, FILE *f)
{
  format_save_ascii_matrix(p, f, (int)0);
}

DX(xsave_ascii_matrix)
{
  at *p, *ans;

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), NULL);
    ans = new_string(file_name);
  } else if ((p = APOINTER(2)) && (p->flags & C_EXTERN)
	     && (p->Class == &file_W_class)) {
    LOCK(p);
    LOCK(p);
    ans = p;
  } else {
    error(NIL, "not a string or write descriptor", p);
  }
  save_ascii_matrix(APOINTER(1), p->Object);
  UNLOCK(p);
  return ans;
}

DX(xexport_ascii_matrix)
{
  at *p, *ans;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), "mat");
    ans = new_string(file_name);
  } else {
    p = APOINTER(2);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_W_class)) 
      error(NIL, "not a string or write descriptor", p);
    LOCK(p);
    LOCK(p);
    ans = p;
  }
  export_ascii_matrix(APOINTER(1), p->Object);
  UNLOCK(p);
  return ans;
}







/* -------- Swapping an area */


static void 
swap_buffer(char *b, int n, int m)
{
  int i,j;
  char buffer[16];
  for (i=0; i<n; i++) {
    for (j=0; j<m; j++)
      buffer[j] = b[m-j-1];
    for (j=0; j<m; j++)
      *b++ = buffer[j];
  }
}






/* -------- Loading a matrix */


at *
load_matrix(FILE *f)
{
  int i;
  at *ans;
  int magic;
  int ndim, dim[MAXDIMS];
  int swapflag = 0;
  
  switch (magic = read4(f)) {
    
  case SWAP( BINARY_MATRIX ):
  case SWAP( PACKED_MATRIX ):
  case SWAP( DOUBLE_MATRIX ):
  case SWAP( SHORT_MATRIX ):
  case SWAP( INTEGER_MATRIX ):
  case SWAP( BYTE_MATRIX ):
    magic = SWAP(magic);
    swapflag = 1;
    /* no break */

  case BINARY_MATRIX:
  case PACKED_MATRIX:
  case DOUBLE_MATRIX:
  case SHORT_MATRIX:
  case INTEGER_MATRIX:
  case BYTE_MATRIX:

    ndim = read4(f);
    if (swapflag) 
      ndim = SWAP(ndim);
    
    if (ndim < 1 || ndim > MAXDIMS) {
      test_file_error(f);
      error(NIL, "bad file format",NIL);
    }

    for (i = 0; i < ndim; i++) {
      dim[i] = read4(f);
      if (swapflag)
	dim[i] = SWAP(dim[i]);
    }

    if (ferror(f) || feof(f)) {
      test_file_error(f);
      error(NIL, "bad file format",NIL);
    }
    while (i++ < 3)
      if (read4(f) != (swapflag ? SWAP(1) : 1)) {
	test_file_error(f);
	error(NIL, "bad file format",NIL);
      }
    break;



  case SWAP(ASCII_MATRIX):
    magic = SWAP(magic);
    /* no break */

  case ASCII_MATRIX:

    if (fscanf(f, " %d", &ndim) != 1) {
      test_file_error(f);
      error(NIL, "bad file format",NIL);
    }
    if (ndim < 1 || ndim > MAXDIMS) {
      test_file_error(f);
      error(NIL, "bad file format",NIL);
    }
    for (i = 0; i < ndim; i++)
      if (fscanf(f, " %d ", &(dim[i])) != 1)  {
	test_file_error(f);
	error(NIL, "bad file format",NIL);
      }
    break;
    
  default:
    error(NIL, "not a recognized matrix file", NIL);
  }
  
  switch (magic) {
  case ASCII_MATRIX:
  case BINARY_MATRIX:
    ans = matrix(ndim, dim);
    break;
  case PACKED_MATRIX:
    ans = pmatrix(ndim, dim);
    break;
  case DOUBLE_MATRIX:
    ans = dmatrix(ndim, dim);
    break;
  case SHORT_MATRIX:
    ans = smatrix(ndim, dim);
    break;
  case INTEGER_MATRIX:
    ans = imatrix(ndim, dim);
    break;
  case BYTE_MATRIX:
    ans = bmatrix(ndim, dim);
    break;
  default:
    error("matrix.c/load_matrix",
	  "internal error: unhandled format",NIL);
  }
  
  if (magic==ASCII_MATRIX)
    import_text_matrix(ans,f);
  else
    import_raw_matrix(ans,f,0);

  /** Swap data if needed */

  if (swapflag) {
    int size, elsize, mode;
    mode_check(ans, &size, &elsize, &mode);
    if (elsize>1)
      swap_buffer( ((struct array*)(ans->Object))->data, size, elsize);
  }

  return ans;
}




DX(xload_matrix)
{
  at *p, *ans;

  if (arg_number == 2) {
    ARG_EVAL(2);
    ASYMBOL(1);
    if (ISSTRING(2)) {
      p = OPEN_READ(ASTRING(2), "mat");
    } else {
      p = APOINTER(2);
      ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class)) 
	error(NIL, "not a string or read descriptor", p);
      LOCK(p);
    }
    ans = load_matrix(p->Object);
    var_set(APOINTER(1), ans);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    if (ISSTRING(1)) {
      p = OPEN_READ(ASTRING(1), "mat");
    } else {
      p = APOINTER(1);
      ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class))
	error(NIL, "not a string or read descriptor", p);
      LOCK(p);
    }
    ans = load_matrix(p->Object);
  }
  UNLOCK(p);
  return ans;
}








/* ------------- Mapping a matrix */


#ifdef HAVE_MMAP

at *
map_matrix(FILE *f)
{
  int i, size, pos, fd;
  int ndim, dim[MAXDIMS];
  int magic;
  struct array *arr;
  at *ans;

  switch (magic = read4(f)) {

  case BINARY_MATRIX:
  case PACKED_MATRIX:
  case DOUBLE_MATRIX:
  case SHORT_MATRIX:
  case INTEGER_MATRIX:
  case BYTE_MATRIX:
    ndim = read4(f);
    if (ndim < 1 || ndim > MAXDIMS)
      goto err_map_matrix;
    for (i = 0; i < ndim; i++)
      dim[i] = read4(f);
    while (i++ < 3)
      if (read4(f) != 1)
	goto err_map_matrix;
    break;
    
  case SWAP( BINARY_MATRIX ):
  case SWAP( PACKED_MATRIX ):
  case SWAP( DOUBLE_MATRIX ):
  case SWAP( SHORT_MATRIX ):
  case SWAP( INTEGER_MATRIX ):
  case SWAP( BYTE_MATRIX ):
    error(NIL,"Can't map this byteswapped matrix",NIL);

  case SWAP( ASCII_MATRIX ):
  case ASCII_MATRIX:
    error(NIL, "can't map an ascii matrix", NIL);

  default:
    error("matrix.c/mapmatrix", 
	  "internal error: unrecognized matrix file", NIL);
  }

  pos = ftell(f);
  if (pos <= 0)
    error(NIL,"Not a seekable file",NIL);
  { 
#ifdef UNIX
    struct stat buf;
    rewind(f);
    fd = fileno(f);
    ifn ( (fstat(fd,&buf)==0) && (S_IFREG & buf.st_mode) )
      error(NIL,"Not a regular file",NIL);
#endif
#ifdef WIN32
    struct _stat buf;
    fd = _fileno(f);
    ifn ( (_fstat(fd,&buf)==0) && (S_IFREG & buf.st_mode) )
      error(NIL,"Not a regular file",NIL);
#endif
  }
  
  size = 1;
  arr = allocate(&array_alloc);
  arr->ndim = ndim;
  arr->data = NIL;
  arr->main_array = NIL;

  arr->nr0 = NIL;
  arr->nr1 = NIL;
  
  for (i = ndim - 1; i >= 0; i--) {
    arr->dim[i] = dim[i];
    arr->modulo[i] = size;
    size *= dim[i];
  }

  switch (magic) {
  case BINARY_MATRIX:
    ans = new_extern(&matrix_class, arr);
    size *= sizeof(flt);
    break;
  case PACKED_MATRIX:
    ans = new_extern(&pmatrix_class, arr);
    size *= sizeof(char);
    break;
  case DOUBLE_MATRIX:
    ans = new_extern(&dmatrix_class, arr);
    size *= sizeof(real);
    break;
  case SHORT_MATRIX:
    ans = new_extern(&smatrix_class, arr);
    size *= sizeof(short);
    break;
  case INTEGER_MATRIX:
    ans = new_extern(&imatrix_class, arr);
    size *= sizeof(int);
    break;
  case BYTE_MATRIX:
    ans = new_extern(&bmatrix_class, arr);
    size *= sizeof(unsigned char);
    break;
  default:
    error("matrix.c/map_matrix",
	  "internal error: unhandled format",NIL);
  }
  arr->data = 0;
  arr->xtra = 0;
  arr->flags = MAPPED_ARRAY;
  ans->flags |= X_ARRAY;
#ifdef UNIX
  errno = 0;
  arr->data = (char*)mmap(0, size+pos, PROT_READ, MAP_SHARED, fd, 0) + pos;
  if (errno)
    test_file_error(NIL);
#endif
#ifdef WIN32
  arr->xtra = (long)CreateFileMapping((HANDLE)(_get_osfhandle(fd)), NULL, 
			              PAGE_READONLY, 0, size+pos, NULL );
  if (!arr->xtra)
    error(NIL,"Cannot create file mapping",NIL);
  arr->data = MapViewOfFile((HANDLE)(arr->xtra), 
			    FILE_MAP_READ, 0, 0, size + pos);
  if (!arr->data)
    error(NIL,"Cannot create view on mapped file",NIL);
  arr->data = (char*)(arr->data) + pos;
#endif
  return ans;

err_map_matrix:
  test_file_error(f);
  error(NIL, "error in matrix file.", NIL);
}

DX(xmap_matrix)
{
  at *p, *ans;

  if (arg_number == 2) {
    ARG_EVAL(2);
    ASYMBOL(1);
    if (ISSTRING(2)) {
      p = OPEN_READ(ASTRING(2), "mat");
    } else {
      p = APOINTER(2);
      ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class))
	error(NIL, "not a string or read descriptor", p);
      LOCK(p);
    }
    ans = map_matrix(p->Object);
    var_set(APOINTER(1), ans);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    if (ISSTRING(1)) {
      p = OPEN_READ(ASTRING(1), "mat");
    } else {
      p = APOINTER(1);
      ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class)) 
	error(NIL, "not a string or read descriptor", p);
      LOCK(p);
    }
    ans = map_matrix(p->Object);
  }
  UNLOCK(p);
  return ans;
}

#endif






/* --- CONTIGUOUS MATRIX INTERFACE */


float *
get_std_vector(at *p, int n)
{
  struct array *arr;

  if (! EXTERNP(p,&matrix_class) )
    error(NIL,"Not a matrix",p);
  arr = p->Object;
  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "A mapped matrix is illegal here", p);

  ifn (arr->ndim == 1)
    error(NIL, "A 1D vector was expected", p);

  ifn (arr->modulo[0] == 1)
    error(NIL, "Cannot handle this submatrix (copy it)", p);
  ifn (arr->dim[0] == n)
    error(NIL, "Vector size is not", NEW_NUMBER(n));

  return arr->data;
}

void *
get_std_matrix(at *p, int m, int n)
{
  struct array *arr;

  if (! EXTERNP(p,&matrix_class) )
    error(NIL,"Not a matrix",p);
  arr = p->Object;
  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "A mapped matrix is illegal here", p);
  
  if (arr->ndim==1 && n==1)
    return get_std_vector(p,m);
  else if (arr->ndim!=2) 
    error(NIL, "A 2D matrix was expected", p);

  ifn (arr->dim[0]==m)
    error(NIL, "Matrix first dimension size is not", NEW_NUMBER(m));
  ifn (arr->dim[1]==n)
    error(NIL, "Matrix second dimension size is not", NEW_NUMBER(n));
  ifn (arr->modulo[0]==n && arr->modulo[1]==1)
    error(NIL, "Cannot handle this submatrix (copy it)", p);

  return arr->data;
}



/* --- NUMERICAL RECIPES INTERFACE */






static struct array *
check_nr_vector(at *p, int *psize, TLclass *mclass)
{
  struct array *arr;
  
  ifn(p && (p->flags & X_ARRAY))
    error(NIL, "not a matrix", p);
  ifn(p->Class == mclass)
    error(NIL, "illegal kind of matrix", p);
  arr = p->Object;
  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "mapped arrays are illegal here", p);
  if (arr->ndim != 1)
    error(NIL, "A vector was expected", p);
  if (arr->modulo[0] != 1)
    error(NIL, "Cannot handle this submatrix (copy it)", p);

  if (psize) {
    if (*psize==0)
      *psize = arr->dim[0];
    else if (*psize!=arr->dim[0])
      error(NIL,"dimension mismatch",p);
  }
  return arr;
}

float *
get_nr1_vector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&matrix_class);
  return (float*)(arr->data)-1;
}


float *
get_nr0_vector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&matrix_class);
  return (float*)(arr->data);
}




double *
get_nr1_dvector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&dmatrix_class);
  return (double*)(arr->data)-1;
}


double *
get_nr0_dvector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&dmatrix_class);
  return (double*)(arr->data);
}


short *
get_nr1_svector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&smatrix_class);
  return (short*)(arr->data)-1;
}


short *
get_nr0_svector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&smatrix_class);
  return (short*)(arr->data);
}




int *
get_nr1_ivector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&imatrix_class);
  return (int*)(arr->data)-1;
}


int *
get_nr0_ivector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&imatrix_class);
  return (int*)(arr->data);
}


unsigned char *
get_nr1_bvector(at *p, int *psize)
{
  struct array *arr;
  arr = check_nr_vector(p,psize,&bmatrix_class);
  return (unsigned char*)(arr->data)-1;
}


unsigned char *
get_nr0_bvector(at *p, int *psize)
{
  struct array *arr;
  
  arr = check_nr_vector(p,psize,&bmatrix_class);
  return (unsigned char*)(arr->data);
}




static struct array *
check_nr_matrix(at *p, int *ps0, int *ps1, TLclass *mclass)
{
  struct array *arr;
  
  ifn(p && (p->flags & X_ARRAY))
    error(NIL, "not a matrix", p);
  ifn(p->Class == mclass)
    error(NIL, "illegal kind of matrix", p);
  arr = p->Object;

  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "mapped arrays are illegal here", p);
  if (arr->ndim != 2)
    error(NIL, "A matrix was expected", p);
  
  if (arr->modulo[1] != 1)
    error(NIL, "Cannot handle this submatrix (copy it)", p);

  if (ps0) {
    if (*ps0==0)
      *ps0 = arr->dim[0];
    else if (*ps0!=arr->dim[0])
      error(NIL,"dimension mismatch (first dimension)",p);
  }
  if (ps1) {
    if (*ps1==0)
      *ps1 = arr->dim[1];
    else if (*ps1!=arr->dim[1])
      error(NIL,"dimension mismatch (second dimension)",p);
  }
  return arr;	     	
}



float **
get_nr1_matrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  float *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&matrix_class);
  if (!arr->nr1) {
    arr->nr1 = tl_malloc(sizeof(float *) * arr->dim[0]);
    if (!arr->nr1)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr1[i] = f-1;
      f += arr->modulo[0];
    }
  }
  return (float**)(arr->nr1)-1;
}
  

float **
get_nr0_matrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  float *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&matrix_class);
  if (!arr->nr0) {
    arr->nr0 = tl_malloc(sizeof(float *) * arr->dim[0]);
    if (!arr->nr0)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr0[i] = f;
      f += arr->modulo[0];
    }
  }
  return (float**)(arr->nr0);
}




double **
get_nr1_dmatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  double *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&dmatrix_class);
  if (!arr->nr1) {
    arr->nr1 = tl_malloc(sizeof(double *) * arr->dim[0]);
    if (!arr->nr1)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr1[i] = f-1;
      f += arr->modulo[0];
    }
  }
  return (double**)(arr->nr1)-1;
}
  

double **
get_nr0_dmatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  double *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&dmatrix_class);
  if (!arr->nr0) {
    arr->nr0 = tl_malloc(sizeof(double *) * arr->dim[0]);
    if (!arr->nr0)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr0[i] = f;
      f += arr->modulo[0];
    }
  }
  return (double**)(arr->nr0);
}



short **
get_nr1_smatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  short *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&smatrix_class);
  if (!arr->nr1) {
    arr->nr1 = tl_malloc(sizeof(short *) * arr->dim[0]);
    if (!arr->nr1)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr1[i] = f-1;
      f += arr->modulo[0];
    }
  }
  return (short**)(arr->nr1)-1;
}
  

short **
get_nr0_smatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  short *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&smatrix_class);
  if (!arr->nr0) {
    arr->nr0 = tl_malloc(sizeof(short *) * arr->dim[0]);
    if (!arr->nr0)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr0[i] = f;
      f += arr->modulo[0];
    }
  }
  return (short**)(arr->nr0);
}


int **
get_nr1_imatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  int *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&imatrix_class);
  if (!arr->nr1) {
    arr->nr1 = tl_malloc(sizeof(int *) * arr->dim[0]);
    if (!arr->nr1)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr1[i] = f-1;
      f += arr->modulo[0];
    }
  }
  return (int**)(arr->nr1)-1;
}
  

int **
get_nr0_imatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  int *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&imatrix_class);
  if (!arr->nr0) {
    arr->nr0 = tl_malloc(sizeof(int *) * arr->dim[0]);
    if (!arr->nr0)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr0[i] = f;
      f += arr->modulo[0];
    }
  }
  return (int**)(arr->nr0);
}




unsigned char **
get_nr1_bmatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  unsigned char *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&bmatrix_class);
  if (!arr->nr1) {
    arr->nr1 = tl_malloc(sizeof(int *) * arr->dim[0]);
    if (!arr->nr1)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr1[i] = f-1;
      f += arr->modulo[0];
    }
  }
  return (unsigned char**)(arr->nr1)-1;
}
  

unsigned char **
get_nr0_bmatrix(at *p, int *ps0, int *ps1)
{
  struct array *arr;
  unsigned char *f;
  int i;

  arr = check_nr_matrix(p,ps0,ps1,&bmatrix_class);
  if (!arr->nr0) {
    arr->nr0 = tl_malloc(sizeof(int *) * arr->dim[0]);
    if (!arr->nr0)
      error(NIL,"Out of memory",NIL);
    f = arr->data;
    for (i=0;i<arr->dim[0];i++) {
      arr->nr0[i] = f;
      f += arr->modulo[0];
    }
  }
  return (unsigned char**)(arr->nr0);
}









/* --------- INITIALISATION CODE --------- */


void 
init_matrix(void)
{
  class_define("MAT",&matrix_class );
  class_define("PMAT",&pmatrix_class );
  class_define("DMAT",&dmatrix_class );
  class_define("SMAT",&smatrix_class );
  class_define("IMAT",&imatrix_class );
  class_define("BMAT",&bmatrix_class );

  matrix_class.dontdelete = 1;
  pmatrix_class.dontdelete = 1;
  dmatrix_class.dontdelete = 1;
  smatrix_class.dontdelete = 1;
  imatrix_class.dontdelete = 1;
  bmatrix_class.dontdelete = 1;

  dx_define("matrix",  xmatrix);
  dx_define("fmatrix", xfmatrix);
  dx_define("pmatrix", xpmatrix);
  dx_define("dmatrix", xdmatrix);
  dx_define("smatrix", xsmatrix);
  dx_define("imatrix", ximatrix);
  dx_define("bmatrix", xbmatrix);
  dx_define("matrixp", xmatrixp);
  dx_define("matrix_print_mode", xmatrix_print_mode);

  dx_define("import-raw-matrix", ximport_raw_matrix);
  dx_define("import-text-matrix", ximport_text_matrix);

  dx_define("export-raw-matrix", xexport_matrix);
  dx_define("export-text-matrix", xexport_ascii_matrix);

  dx_define("save_matrix", xsave_matrix);
  dx_define("save_ascii_matrix", xsave_ascii_matrix);
  dx_define("load_matrix", xload_matrix);

#ifdef HAVE_MMAP
  dx_define("map_matrix", xmap_matrix);
#endif
  dx_define("mappedmatrixp", xmappedmatrixp);
}
