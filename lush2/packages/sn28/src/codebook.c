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
 * $Id: codebook.c,v 1.1 2003/03/18 00:52:22 leonb Exp $
 **********************************************************************/


#include "header.h"
#include "codebook.h"

static at *matrix_to_codebook(at *mat);


/* allocation ----------------------------------- */


struct alloc_root alloc_codebook = {
 NULL,
 NULL,
 sizeof(struct codebook),
 20
};


/* class ---------------------------------------- */


static void 
codebook_dispose(at *p)
{
  struct codebook *cb;
  cb = p->Object;
  if (cb->code)
    free(cb->code);
  if (cb->word_matrix)
    UNLOCK(cb->word_matrix);
  deallocate(&alloc_codebook,(void*)cb);
}

static void 
codebook_action(at *p, void (*action) (at*))
{
  struct codebook *cb;
  cb = p->Object;
  if (cb->word_matrix)
    (*action)(cb->word_matrix);
}


static char *
codebook_name(at *p)
{
  struct codebook *cb;
  cb = p->Object;
  sprintf(string_buffer,"::%s:%dx%d",
	  nameof(p->Class->classname), cb->ncode, cb->ndim );
  return string_buffer;
}


static at *
codebook_eval(at *p)
{
  struct codebook *cb;
  cb = p->Object;
  p = cb->word_matrix;
  LOCK(p);
  return p;
}

static at *
codebook_listeval(at *p, at *q)
{
  struct codebook *cb;
  cb = p->Object;
  return (*index_class.list_eval)(cb->word_matrix,q);
}

static void
codebook_serialize(at **pp, int code)
{
  struct codebook *cb;
  int i;
  at *p;
  if (code==SRZ_READ) {
    /* we know that atomic elements will be resolved */
    if (serialize_atstar(&p, code))
      error(NIL,"Serialization error (unresolved matrix)",NIL);
    *pp = matrix_to_codebook(p);
    cb = (*pp)->Object;
    UNLOCK(p);
  } else {
    cb = (*pp)->Object;
    serialize_atstar(&cb->word_matrix, code);
  }
  for (i=0; i<cb->ncode; i++)
    serialize_int( &cb->code[i].label, code);
}



class codebook_class =
{
  codebook_dispose,
  codebook_action,
  codebook_name,
  codebook_eval,
  codebook_listeval,
  codebook_serialize
};



/* check --------------------------------------- */

struct codebook *
check_codebook(at *p)
{
  ifn (p && (p->flags & C_EXTERN)
         && (p->Class == &codebook_class))
    error(NIL,"Not a codebook",p);
  return p->Object;
}


/* define labels--------------------------------- */


static void 
store_codebook_label(at *atcb, at *arg)
{
  struct codebook *cb;
  struct index *arr;
  int d[1];
  int i;

  cb = check_codebook(atcb);
  d[1] = cb->ncode;
  arr = easy_index_check(arg, 1, d);
  for(i=0; i<cb->ncode; i++) 
    cb->code[i].label = easy_index_get(arr, &i);
}



/* creation ------------------------------------ */


static at *
matrix_to_codebook(at *mat)
{
  struct codebook *cb;
  struct codeword *cw;
  struct index *arr;
  char *err;
  flt *f;
  int d[2];
  int i;

  d[0] = d[1] = 0;
  arr = easy_index_check(mat, 2, d);
  if ((err = not_a_nrmatrix(mat)))
    error(NIL, err, mat);
  if (! (cw = calloc(d[0], sizeof(struct codeword))))
    error(NIL,"No memory",NIL);
  cb = allocate(&alloc_codebook);
  cb->ncode = d[0];
  cb->ndim = d[1];
  cb->code = cw;
  cb->word_matrix = mat;
  LOCK(mat);
  f = (flt*)(arr->st->srg.data) + arr->offset;
  for (i=0;i<cb->ncode;i++) {
    cb->code[i].word = f;
    f += arr->mod[0];
  }
  return new_extern(&codebook_class,cb);
}


at *
new_codebook(int ncode, int ndim)
{
  int d[2];
  at *mat, *ans;

  if (ndim<1 || ndim>100000)
    error(NIL,"Word dimension is incorrect",NEW_NUMBER(ndim));
  if (ncode<0 || ncode>10000000)
    error(NIL,"Codebook size is incorrect",NEW_NUMBER(ncode));
  
  d[0] = ncode;
  d[1] = ndim;
  mat = F_matrix(2,d);
  ans = matrix_to_codebook(mat);
  UNLOCK(mat);
  return ans;
}



/* Creates a new codebook...
 * (new-codebook <mat2d>)
 * (new-codebook <mat2d> <mat1d>)
 * (new-codebook <ncode> <ndim> ) 
 */

DX(xnew_codebook)
{
  ALL_ARGS_EVAL;
  
  if (arg_number==1)
    return matrix_to_codebook(APOINTER(1));
  else {
    ARG_NUMBER(2);
    if (ISNUMBER(1))
      return new_codebook(AINTEGER(1),AINTEGER(2));
    else {
      at *ans;
      ans = matrix_to_codebook(APOINTER(1));
      store_codebook_label(ans,APOINTER(2));
      return ans;
    }
  }
}


/* set/get pattern ----------------------------- */


DX(xcodebook_word) 
{
  ALL_ARGS_EVAL;
  switch(arg_number)
    {
    default:
      {
        ARG_NUMBER(-1); /* noreturn */
        return NIL;
      }
    case 1: 
      {
	struct codebook *cb;
	at *p;
	cb = check_codebook(APOINTER(1));
	p = cb->word_matrix;
	LOCK(p);
	return p;
      }
    case 2:
    case 3:
      {
	struct codebook *cb;
        struct index *arr;
	at *p, *q;
	int dim[2];
        int start[2];
	int i;        
	cb = check_codebook(APOINTER(1));
	p = cb->word_matrix;
        dim[0] = dim[1] = 0;
        arr = easy_index_check(p, 2, dim);
	i = AINTEGER(2);
	if (i<0 || i>=cb->ncode)
	  error(NIL,"subscript out of range",APOINTER(2));
	start[0] = i;
        dim[0] = 0;
        start[1] = 0;
        dim[1] = cb->ndim;
	q = new_index(arr->atst);
        index_from_index(q, p, dim, start);
	if (arg_number==3)
	  copy_matrix(APOINTER(3),q);	  
	return q;
      }
    }
}




/* set/get label ------------------------------- */


DX(xcodebook_label) 
{
  ALL_ARGS_EVAL;
  switch(arg_number)
    {
    default:
      ARG_NUMBER(-1); /* never return */
      return NIL;
    case 1: 
      {
	struct codebook *cb;
        struct index *arr;
	int i;
	at *p;

	cb = check_codebook(APOINTER(1));
	i = cb->ncode;
	p = I32_matrix(1,&i);
        arr = p->Object;
        for (i=0; i<cb->ncode; i++) 
          easy_index_set(arr, &i, (real)cb->code[i].label);
        return p;
      }
    case 2:
    case 3:
      {
	struct codebook *cb;
	int i, label;
	
	cb = check_codebook(APOINTER(1));
	i = AINTEGER(2);
	if (i<0 || i>=cb->ncode)
	  error(NIL,"Subscript out of range",APOINTER(2));
	if (arg_number==3) {
	  label = AINTEGER(3);
	  if (label<0 || label>1000000)
	    error(NIL,"Class label is strange",APOINTER(3));
	  cb->code[i].label = label;
	}
	
	return NEW_NUMBER(cb->code[i].label);
      }
    }
}






/* labels -------------------------------------- */

static at *
codebook_labels(struct codebook *cb)
{
  int      i;
  int      nb=-1;
  at      *ans = NIL;
  at      **where = &ans;
  char    *tbl;

  for (i = 0; i < cb->ncode; i++) {
    if (cb->code[i].label > nb) 
      nb = cb->code[i].label;
    if (cb->code[i].label < 0)
      error(NIL,"Codebook labels is negative for item",NEW_NUMBER(i));
  }

  ifn( tbl = calloc( nb+1, sizeof(char)) )
    error(NIL,"No memory",NIL);
  for (i=0; i<=nb; i++)
    tbl[i] = 0;
  for (i=0; i<cb->ncode; i++)
    tbl[cb->code[i].label] = 1;
  
  for (i=0; i<=nb; i++)
    if (tbl[i]) {
      *where = cons(NEW_NUMBER(i),NIL);
      where = &((*where)->Cdr);
    }
  free(tbl);
  return ans;
}

DX(xcodebook_labels)
{
  struct codebook *cb;
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  cb = check_codebook(APOINTER(1));
  return codebook_labels(cb);
}

		 




/* split --------------------------------------- */

at *
split_codebook(at *p, int i, int j)
{
  struct codebook *cb;
  at *q, *r;
  struct codebook *cbnew;
  struct index *arr;
  int dim[2];
  int start[2];
  int k;
  
  cb = check_codebook(p);
  if (i>j || i<0 || j>=cb->ncode)
    error(NIL,"Illegal subscript",NIL);
  
  p = cb->word_matrix;
  dim[0] = dim[1] = 0;
  arr = easy_index_check(p, 2, dim);
  start[0] = i;
  dim[0] = j-i+1;
  start[1] = 0;
  dim[1] = cb->ndim;
  q = new_index(arr->atst);
  index_from_index(q, p, dim ,start);
  r = matrix_to_codebook(q);
  cbnew = r->Object;
  UNLOCK(q);
  for (k=0; k<cbnew->ncode; k++)
    cbnew->code[k].label = cb->code[i++].label;
  return r;
}


DX(xsplit_codebook)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  return split_codebook(APOINTER(1),AINTEGER(2),AINTEGER(3));
}





/* merge --------------------------------------- */

at *
merge_codebook(int n, at **atcbs)
{
  int ndim = 0;
  int ncode = 0;
  int i,j,k,now;
  flt *s,*d;
  
  struct codebook *cb;
  struct codebook *newcb;
  at *ans;
  
  for (i=0;i<n;i++) {
    cb = check_codebook(atcbs[i]);
    if (ndim!=0 && cb->ndim != ndim)
      error(NIL,"Codebook items have different sizes",atcbs[i]);
    ndim = cb->ndim;
    ncode += cb->ncode;
  }
  
  ans = new_codebook(ncode,ndim);
  newcb = ans->Object;

  now = 0;
  for(i=0;i<n;i++) {
    cb = check_codebook(atcbs[i]);
    for (j=0; j<cb->ncode; j++) {
      s = cb->code[j].word;
      d = newcb->code[now].word;
      for(k=0; k<ndim; k++)
	*d++ = *s++;
      newcb->code[now].label = cb->code[j].label;
      now++;
    }
  }
  return ans;
}

DX(xmerge_codebook)
{
  if (arg_number<1)
    ARG_NUMBER(-1);
  ALL_ARGS_EVAL;
  return merge_codebook(arg_number, arg_array+1);
}





/* select --------------------------------------- */

at *
select_codebook(int n, at **atcbs, int (*test) (int))
{
  int ndim = 0;
  int ncode = 0;
  int i,j,k,now;
  flt *s,*d;
  
  struct codebook *cb;
  struct codebook *newcb;
  at *ans;
  
  for (i=0; i<n; i++) {
    cb = check_codebook(atcbs[i]);
    if (ndim!=0 && cb->ndim != ndim)
      error(NIL,"Codebook items have different sizes",atcbs[i]);
    ndim = cb->ndim;
    for (j=0; j<cb->ncode; j++) {
      if ( (*test)(cb->code[j].label) )
	ncode += 1;
    }
  }
  
  ans = new_codebook(ncode,ndim);
  newcb = ans->Object;

  now = 0;
  for(i=0; i<n; i++) {
    cb = check_codebook(atcbs[i]);
    for (j=0; j<cb->ncode; j++) {
      if ( (*test)(cb->code[j].label) ) {
	if (now >= ncode)
	  error(NIL,"The predicate is not deterministic (i)",NIL);
	s = cb->code[j].word;
	d = newcb->code[now].word;
	for(k=0; k<ndim; k++) *d++ = *s++;
	newcb->code[now].label = cb->code[j].label;
	now++;
      }
    }
  }
  if (now!=ncode)
    error(NIL,"The predicate is not deterministic (ii)",NIL);
  return ans;
}


static at *func;

static int
test(int i)
{
  at *p,*q;
  LOCK(func);
  q = cons(func, cons( NEW_NUMBER(i), NIL));
  p = eval(q);
  UNLOCK(q);
  if (p) {
    UNLOCK(p);
    return 1;
  }
  return 0;
}


DX(xselect_codebook)
{
  if (arg_number<2)
    ARG_NUMBER(-1);
  ALL_ARGS_EVAL;
  func = APOINTER(1);
  return select_codebook(arg_number-1, arg_array+2, test);
}








/* load/save -------------------------------------- */






/* ----- save a codebook in ascii format */

DX (xsave_ascii_codebook)
{
  struct codebook *cb;
  int             i,j;
  FILE            *f;
  at              *file;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);

  cb = check_codebook(APOINTER(1));

  file = APOINTER(2);
  if (EXTERNP(file, &string_class))
    {
      file = OPEN_WRITE(SADD(file->Object),"cb");
      f = file->Object;
    } 
  else if (EXTERNP(file, &file_W_class))
    {
      f = file->Object;
      LOCK(file);
    } 
  else 
    error(NIL,"Not a file descriptor",file);
  
  fprintf(f,"%d\n",cb->ncode);
  fprintf(f,"%d\n",cb->ndim);
  for (i=0;i < cb->ncode;i++)
    {
      for (j=0;j < cb->ndim;j++)
	fprintf(f,"%f  ",cb->code[i].word[j]);
      fprintf(f,"\n");
      fprintf(f,"%d\n",cb->code[i].label);
    }
  test_file_error(f);
  UNLOCK(file);
  return NIL;
  
}






/* ----- save a codebook in binary format */

DX (xsave_codebook)
     
{
  struct codebook *cb;
  int             i;
  FILE            *f;
  at              *file;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);

  cb = check_codebook(APOINTER(1));
  
  file = APOINTER(2);
  if (EXTERNP(file, &string_class))
    {
      file = OPEN_WRITE(SADD(file->Object),"cb");
      f = file->Object;
    } 
  else if (EXTERNP(file, &file_W_class))
    {
      f = file->Object;
      LOCK(file);
    } 
  else 
    error(NIL,"Not a file descriptor",file);

  fwrite (&(cb->ncode), sizeof (int), 1, f);
  fwrite (&(cb->ndim), sizeof (int), 1, f);
  for (i=0;i < cb->ncode;i++)
    {
      fwrite (cb->code[i].word, sizeof (flt), cb->ndim, f);
      fwrite (&(cb->code[i].label), sizeof (int), 1, f);
    }
  test_file_error(f);
  UNLOCK(file);
  return NIL;
}






/* ----- load a codebook in ascii format */


DX (xload_ascii_codebook)
     
{
  struct codebook *cb;
  int             i,j,nb,dim;
  FILE            *f;
  at              *ans;
  at              *file;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);

  file = APOINTER(1);
  if (EXTERNP(file, &string_class))
    {
      file = OPEN_READ(SADD(file->Object),"cb");
      f = file->Object;
    } 
  else if (EXTERNP(file, &file_W_class))
    {
      f = file->Object;
      LOCK(file);
    } 
  else 
    error(NIL,"Not a file descriptor",file);

  nb = dim = -1;
  fscanf(f," %d %d ",&nb,&dim);
  test_file_error(f);
  if ( nb<1 || dim<1 || dim>100000 || nb>10000000 )
    error(NIL,"Illegal file format",NIL);

  ans=new_codebook(nb,dim);
  cb=ans->Object;
  for (i=0; i<cb->ncode; i++) 
    {
      for (j=0; j<cb->ndim; j++) 
	{
	  if (fscanf(f," %f",&(cb->code[i].word[j]))!=1)
	    error(NIL,"Malformed file",NIL);
	}
      if (fscanf(f," %d",&(cb->code[i].label)) != 1)
	error(NIL,"Malformed file",NIL);
    }
  test_file_error(f);
  UNLOCK(file);
  return ans;
}






/* ----- load a codebook in binary format */

DX (xload_codebook)
     
{
  struct codebook *cb;
  int i,nb,dim;
  FILE *f;
  at *file;
  at *ans;

  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  
  file = APOINTER(1);
  if (EXTERNP(file, &string_class))
    {
      file = OPEN_READ(SADD(file->Object),"cb");
      f = file->Object;
    } 
  else if (EXTERNP(file, &file_W_class))
    {
      f = file->Object;
      LOCK(file);
    } 
  else 
    error(NIL,"Not a file descriptor",file);
  
  fread (&nb, sizeof (int), 1, f);
  fread (&dim, sizeof (int), 1, f);
  test_file_error(f);

  if ( nb<1 || dim<1 || dim>100000 || nb>10000000 )
    error(NIL,"Illegal file format",NIL);

  ans=new_codebook(nb,dim);
  cb=ans->Object;

  for (i=0;i < cb->ncode;i++) {
    fread (cb->code[i].word, sizeof (flt), cb->ndim, f);
    fread (&(cb->code[i].label), sizeof (int), 1, f);
  }
  test_file_error(f);
  UNLOCK(file);
  return ans;
}






/* init ---------------------------------------- */



void init_codebook(void)
{
  class_define("CODEBOOK",&codebook_class );
  dx_define("new-codebook",xnew_codebook);

  dx_define("codebook-word",xcodebook_word);
  dx_define("codebook-label",xcodebook_label);
  dx_define("codebook-labels",xcodebook_labels);

  dx_define("codebook-split",xsplit_codebook);
  dx_define("codebook-merge",xmerge_codebook);
  dx_define("codebook-select",xselect_codebook);

  dx_define("save-ascii-codebook",xsave_ascii_codebook);
  dx_define("save-codebook",xsave_codebook);
  dx_define("load-ascii-codebook",xload_ascii_codebook);
  dx_define("load-codebook",xload_codebook);
}




