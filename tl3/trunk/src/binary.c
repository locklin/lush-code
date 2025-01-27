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
  binary.c
  This file contains general lisp structure definitions
  $Id: binary.c,v 1.2 2005-11-14 19:00:24 leonb Exp $
  ********************************************************************** */


#include "header.h"


/* From OOSTRUCT.C */
extern class *rootclasslist;




#define BINARYSTART     (0x9f)

enum binarytokens {
  TOK_NULL,
  TOK_REF,
  TOK_DEF,
  TOK_NIL,
  TOK_CONS,
  TOK_NUMBER,
  TOK_STRING,
  TOK_SYMBOL,
  TOK_OBJECT,
  TOK_CLASS,
  TOK_MATRIX,
  TOK_ARRAY,
  TOK_DE,
  TOK_DF,
  TOK_DM,
  TOK_CFUNC,
  TOK_CCLASS,
  TOK_COBJECT,
};








/*** GLOBAL VARIABLES ****/

static int in_bwrite = 0;
static FILE *fin;
static FILE *fout;

void clear_bwrite_flag(void);



/*** THE RELOCATION STRUCTURE **********/


static int relocm = 0;
static int relocn = 0;
static ptr *relocp = 0;
static char *relocf = 0;

#define R_EMPTY   0
#define R_REFD    1
#define R_DEFD    2


static void 
check_reloc_size(int m)
{
  char *f;
  ptr *p;
  
  if (relocm >= m) 
    return;
  
  if (relocm == 0) {
    m = 1 + (m | 0x3ff);
    f = tl_malloc(sizeof(char)*m);
    p = tl_malloc(sizeof(ptr)*m);
  } else {
    m = 1 + (m | 0x3ff);
    f = tl_realloc(relocf,sizeof(char)*m);
    p = tl_realloc(relocp,sizeof(ptr)*m);
  }
  if (f && p) 
    {
      relocf = f;
      relocp = p;
      relocm = m;
    } 
  else 
    {
      if (f) 
	free(f);
      if (p) 
	free(p);
      relocf = 0;
      relocp = 0;
      relocm = 0;
      error(NIL,"Internal error: memory allocation failure",NIL);
    }
}


static void 
clear_reloc(int n)
{
  int i;
  check_reloc_size(n);
  for (i=0; i<n; i++) {
    relocp[i] = 0;
    relocf[i] = 0;
  }
  relocn = n;
}


static void
insert_reloc(void *p)
{
  int i;
  check_reloc_size(relocn+1);
  i = relocn;
  relocn += 1;
  relocp[i] = 0;
  relocf[i] = 0;
  while (i>0 && p<=relocp[i-1]) {
    relocp[i] = relocp[i-1];
    relocf[i] = relocf[i-1];
    i--;
  }
  if (relocp[i]==p)
    error(NIL,"Internal error: relocation requested twice",NIL);
  relocp[i] = p;
  relocf[i] = R_EMPTY;
}


static int 
search_reloc(void *p)
{
  int lo = 0;
  int hi = relocn - 1;
  int k;
  while (hi>=lo) {
    k = (lo+hi)/2;
    if (p>relocp[k])
      lo = k+1;
    else if (p<relocp[k])
      hi = k-1;
    else
      return k;
  }
  error(NIL,"Internal error: relocation search failed",NIL);
}


static void
define_refd_reloc(int k, at *p)
{
  ptr f;
  at **h = relocp[k];
  while (h) {
    f = *h;
    LOCK(p);
    *h = p;
    h = f;
  }
}


static void
forbid_refd_reloc(void)
{
  int i;
  int flag = 0;
  for (i=0; i<relocn; i++) {
    if (relocf[i]!= R_DEFD) 
      flag = 1;
    if (relocf[i]== R_REFD)
      define_refd_reloc(i,NIL);
  }
  if (flag)
    error(NIL,"Corrupted binary file (unresolved relocs)",NIL);
}








/*** SWEEP OVER THE ELEMENTS TO SAVE ***/


static int cond_set_flags(at *p);
static int cond_clear_flags(at *p);
static int local_write(at *p);
static int local_bread(at **pp, at *opt);

/* This function recursively performs an action on
 * all objects under p. If action returns a non 0
 * result, recursion is stopped.
 */ 

static void 
sweep(at *p, int code)
{
  
 again:
  
  switch (code)
    {
    case SRZ_SETFL:
      if (cond_set_flags(p))
	return;
      break;
    case SRZ_CLRFL:
      if (cond_clear_flags(p))
	return;
      break;
    case SRZ_WRITE:
      if (local_write(p))
	return;
      break;
    default:
      error(NIL,"internal error (unknown serialization action)",NIL);
    }
  
  if (!p) 
    return;
  
  else if (p->flags & C_CONS) 
    {
      sweep(p->Car,code);
      p = p->Cdr;
      goto again;
    }
  
  else if (! (p->flags & C_EXTERN))
    return;
  
  else if (p->flags & X_OOSTRUCT)
    {
      struct oostruct *c = p->Object;
      int i;
      sweep(p->Class->classname, code);
      for(i=0;i<c->size;i++) {
	sweep(c->slots[i].symb, code);
	sweep(c->slots[i].val, code);
      }
    }
  
  else if (p->Class == &class_class)
    {
      class *s = p->Object;
      sweep(s->classname, code);
      if (s->atsuper) {
	sweep(s->atsuper, code);
	sweep(s->keylist, code);
	sweep(s->defaults, code);
      }
      sweep(s->methods, code);
    }
  
  else if (p->Class == &array_class)
    {
      struct array *arr;
      int j, offset, ndim;
      int d[MAXDIMS];
      at **data;
      
      arr = p->Object;
      ndim = arr->ndim;
      offset = 0;
      for (j = 0; j < ndim; j++)
	d[j] = 0;
      data = arr->data;
      j = ndim;
      while (j >= 0) {
	if (j == ndim) {
	  j--;
	  sweep(data[offset], code);
	}
	if (++d[j] < arr->dim[j])
	  offset += arr->modulo[j++];
	else {
	  offset -= arr->dim[j] * arr->modulo[j];
	  d[j--] = -1;
	}
      }
    }
  
  else if (   p->Class == &de_class
	   || p->Class == &df_class
	   || p->Class == &dm_class )
    {
      struct function *c = p->Object;
      sweep(c->formal_arg_list, code);
      sweep(c->evaluable_list, code);
    }
  
  else if (   p->Class == &dx_class
	   || p->Class == &dy_class )
    {
      struct function *c = p->Object;
      sweep(c->evaluable_list, code);
    }
  
  else if (   p->Class->serialize )
    {
      sweep(p->Class->classname, code);
      (*p->Class->serialize) (&p, code);
    }
  
}













/*** MARKING, FLAGGING ***/


/* This functions computes all objects
 * which must be relocated when saving object p 
 */

static int 
cond_set_flags(at *p)
{
  if (p) {
    if (p->flags&C_MARK) {
      if (! (p->flags&C_MULTIPLE))
	insert_reloc(p);
      p->flags |= C_MULTIPLE;
      return 1;  /* stop recursion */
    }
    p->flags |= C_MARK;
    return 0;
  }
  return 1;
}

static void 
set_flags(at *p)
{
  sweep(p, SRZ_SETFL);
}



/* This function clears all flags 
 * leaving TL3 ready for another run
 */

static int 
cond_clear_flags(at *p)
{
  if (p && (p->flags&C_MARK)) {
    p->flags &= ~(C_MULTIPLE|C_MARK);
    return 0;
  } else
    return 1; /* stop recursion if already cleared */
}

static void 
clear_flags(at *p)
{
  sweep(p, SRZ_CLRFL);
}








/*** LOW LEVEL CHECK/READ/WRITE ***/


static void 
check(FILE *f)
{
  if (feof(f))
    error(NIL,"End of file during bread",NIL);
  if (ferror(f))
    test_file_error(NIL);
}

/* read */

static int 
read_card8(void)
{
  unsigned char c[1];
  if (fread(c, sizeof(char), 1, fin) != 1)
    check(fin);
  return c[0];
}

static int
read_card24(void)
{
  unsigned char c[3];
  if (fread(c, sizeof(char), 3, fin) != 3)
    check(fin);
  return (((c[0]<<8)+c[1])<<8)+c[2];
}

static int
read_card32(void)
{
  unsigned char c[4];
  if (fread(c, sizeof(char), 4, fin) != 4)
    check(fin);
  return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}

static void
read_buffer(void *s, int n)
{
  if (fread(s, sizeof(char), (size_t)n, fin) != (size_t)n)
    check(fin);
}


/* write */

static void
write_card8(int x)
{
  char c[1];
  in_bwrite += 1;
  c[0] = x;
  if (fwrite(&c, sizeof(char), 1, fout) != 1)
    check(fout);
}

static void
write_card24(int x)
{
  char c[3];
  in_bwrite += 3;
  c[0] = x>>16;
  c[1] = x>>8;
  c[2] = x;
  if (fwrite(&c, sizeof(char), 3, fout) != 3)
    check(fout);
}

static void
write_card32(int x)
{
  char c[4];
  in_bwrite += 4;
  c[0] = x>>24;
  c[1] = x>>16;
  c[2] = x>>8;
  c[3] = x;
  if (fwrite(&c, sizeof(char), 4, fout) != 4)
    check(fout);
}

static void
write_buffer(void *s, int n)
{
  in_bwrite += n;
  if (fwrite(s, sizeof(char), (size_t)n, fout) != (size_t)n)
    check(fout);
}


/*** The swap stuff ***/

static int swapflag;

static void 
set_swapflag(void)
{
  union { int i; char c; } s;
  s.i = 1;
  swapflag = s.c;
}

static void 
swap_buffer(void *bb, int n, int m)
{
  int i,j;
  char *b = bb;
  char buffer[16];
  for (i=0; i<n; i++) {
    for (j=0; j<m; j++)
      buffer[j] = b[m-j-1];
    for (j=0; j<m; j++)
      *b++ = buffer[j];
  }
}


/*** The serialization stuff ***/

void
serialize_char(char *data, int code)
{
  switch (code)
    {
    case SRZ_READ:
      if (read_card8() != 'b')
	error(NIL,"serialization error (char expected)",NIL);
      *data = read_card8();
      return;
    case SRZ_WRITE:
      write_card8('b');
      write_card8(*data);
      return;
    }
}

void
serialize_short(short int *data, int code)
{
  switch(code)
    {
    case SRZ_READ:
      if (read_card8() != 'w')
	error(NIL,"serialization error (short expected)",NIL);
      *data = read_card32();
      return;
    case SRZ_WRITE:
      write_card8('w');
      write_card32((int)*data);
      return;
    }
}

void 
serialize_int(int *data, int code)
{
  switch(code)
    {
    case SRZ_READ:
      if (read_card8() != 'w')
	error(NIL,"serialization error (int expected)",NIL);
      *data = read_card32();
      return;
    case SRZ_WRITE:
      write_card8('w');
      write_card32((int)*data);
      return;
    }
}


void
serialize_string(char **data, int code, int maxlen)
{
  int l;
  char *buffer;
  
  switch (code)
    {
    case SRZ_READ:
      if (read_card8() != 's')
	error(NIL,"serialization error (string expected)",NIL);
      l = read_card24();
      if (maxlen == -1) {
	/* automatic mallocation */
	ifn ((buffer = tl_malloc(l+1))) {
	  error(NIL,"out of memory", NIL);
	}
      } else {
	/* user provided buffer */
	buffer = *data;
	if (l+1 >= maxlen)
	  error(NIL,"serialization error (string too large)",NIL);
      }
      read_buffer(buffer,l);
      buffer[l] = 0;
      *data = buffer;
      return;
      
    case SRZ_WRITE:
      write_card8('s');
      l = strlen(*data);
      write_card24(l);
      write_buffer(*data,l);
      return;
    }
}

void
serialize_chars(void **data, int code, int thelen)
{
  int l;
  char *buffer;
  
  switch (code)
    {
    case SRZ_READ:
      if (read_card8() != 's')
	error(NIL,"serialization error (string expected)",NIL);
      l = read_card32();
      if (thelen == -1) {
	/* automatic mallocation */
	ifn ((buffer = tl_malloc(l))) {
	  error(NIL,"out of memory", NIL);
	}
      } else {
	/* user provided buffer */
	buffer = *data;
	if (l != thelen)
	  error(NIL,"serialization error (lengths mismatch)",NIL);
      }
      read_buffer(buffer,l);
      *data = buffer;
      return;
      
    case SRZ_WRITE:
      write_card8('s');
      write_card32(thelen);
      write_buffer(*data,thelen);
      return;
    }
}

void
serialize_float(float *data, int code)
{
  real x;
  switch (code)
    {
    case SRZ_READ:
      if (read_card8() != 'f')
	error(NIL, "serialization error (float expected)", NIL);
      read_buffer(&x, sizeof(real));
      if (swapflag)
	swap_buffer(&x,1,sizeof(real));
      *data = (float)x;
      return;
    case SRZ_WRITE:
      x = *data;
      write_card8('f');
      if (swapflag)
	swap_buffer(&x,1,sizeof(real));
      write_buffer(&x, sizeof(real));
      return;
    }
}

void
serialize_double(double *data, int code)
{
  real x;
  switch (code)
    {
    case SRZ_READ:
      if (read_card8() != 'f')
	error(NIL, "serialization error (double expected)", NIL);
      read_buffer(&x, sizeof(real));
      if (swapflag)
	swap_buffer(&x,1,sizeof(real));
      *data = x;
      return;
    case SRZ_WRITE:
      x = *data;
      write_card8('f');
      if (swapflag)
	swap_buffer(&x,1,sizeof(real));
      write_buffer(&x, sizeof(real));
      return;
    }
}


int 
serialize_atstar(at **data, int code)
{
  switch (code)
    {
    case SRZ_CLRFL:
    case SRZ_SETFL:
      sweep(*data, code);
      return 0;
    case SRZ_READ:
      if (read_card8() != 'p')
	error(NIL, "Serialization error (pointer expected)", NIL);
      return local_bread(data, NIL);
    case SRZ_WRITE:
      write_card8('p');
      sweep(*data, code);
      return 0;
    }
  error(NIL,"binary internal: wrong serialization mode",NIL);
}





/*** BINARY WRITE ***/


/* This local writing routine 
 * is called from sweep
 */


static int
local_write(at *p)
{
  if (p==0 || (p->flags & X_ZOMBIE))
    {
      write_card8(TOK_NIL);
      return 1;
    }
  
  if (p->flags & C_MULTIPLE) 
    {
      int k = search_reloc(p);
      if (relocf[k]) {
	write_card8(TOK_REF);
	write_card24(k);
	return 1;
      } else {
	write_card8(TOK_DEF);
	write_card24(k);
	relocf[k] = R_DEFD;
	/* continue */
      }
    }
  
  if (p->flags & C_CONS)  
    {
      write_card8(TOK_CONS);
      return 0;
    }
  
  if (p->flags & C_NUMBER)  
    {
      double x = p->Number;
      write_card8(TOK_NUMBER);
      if (swapflag)
	swap_buffer(&x,1,sizeof(real));
      write_buffer( &x, sizeof(real) );
      return 1;
    }
  
  if (! (p->flags & C_EXTERN))
    {
      error(NIL,"Internal error: What's this",p);
    }

  if (p->Class == &string_class)
    {
      char *s = SADD(p->Object);
      int l = strlen(s);
      write_card8(TOK_STRING);
      write_card24(l);
      write_buffer(s,l);
      return 1;
    }
  
  if (p->Class == &symbol_class)
    {
      char *s = nameof(p);
      int l = strlen(s);
      write_card8(TOK_SYMBOL);
      write_card24(l);
      write_buffer(s,l);
      return 1;
    }
  
  if (p->flags & X_OOSTRUCT)
    {
      struct oostruct *s = p->Object;
      write_card8(TOK_OBJECT);
      write_card24(s->size);
      return 0;
    }
  
  if (p->Class == &class_class)
    {
      struct class *c = p->Object;
      if (c->atsuper) 
	write_card8(TOK_CLASS);	
      else
	write_card8(TOK_CCLASS);
      return 0;
    }
  
  if (matrixp(p))
    {
      write_card8(TOK_MATRIX);
      in_bwrite += save_matrix_len(p);
      save_matrix(p,fout);
      return 1;
    }
  
  if (p->Class == &array_class)
    {
      struct array *arr = p->Object;
      int i;
      write_card8(TOK_ARRAY);
      write_card24(arr->ndim);
      for (i=0; i<arr->ndim; i++)
	write_card32(arr->dim[i]);
      return 0;
    }
  
  if (p->Class == &de_class)
    {
      write_card8(TOK_DE);
      return 0;
    }
  
  if (p->Class == &df_class)
    {
      write_card8(TOK_DF);
      return 0;
    }
  
  if (p->Class == &dm_class)
    {
      write_card8(TOK_DM);
      return 0;
    }
  
  if (p->Class == &dx_class || p->Class == &dy_class)
    {
      write_card8(TOK_CFUNC);
      return 0;
    }
  
  if (p->Class->serialize)
    {
      write_card8(TOK_COBJECT);
      return 0;
    }
  error(NIL,"Cannot save this object",p);
}





/* Writes object p on file f.
 * Returns the number of bytes
 */

int 
bwrite(at *p, FILE *f)
{
  int count;
  if (in_bwrite!=0)
    error(NIL,"Recursive binary read/write are forbidden",NIL);
  
  fout = f;
  clear_reloc(0);
  set_flags(p);
  write_card8(BINARYSTART);
  write_card24(relocn);
  sweep(p, SRZ_WRITE);
  clear_flags(p);
  count = in_bwrite;
  clear_bwrite_flag();
  return count;
}

DX(xbwrite)
{
  int count = 0;
  int i;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++) 
    count += bwrite( APOINTER(i), context->output_file );
  return NEW_NUMBER(count);
}




/*** BINARY READ ***/




/* Special routines for reading array and objects */


static void 
local_bread_cobject(at **pp)
{
  at *name;
  at *cptr;
  class *cl; 

  if (local_bread(&name, NIL))
    error(NIL,"Corrupted file (unresolved class name!)",NIL);
  ifn (EXTERNP(name,&symbol_class))
    error(NIL,"Corrupted binary file (class name expected)",name);
  cptr = var_get(name);
  ifn (EXTERNP(cptr,&class_class))
    error(NIL,"Corrupted binary file (class expected)",name);
  UNLOCK(name);
  cl = cptr->Object;
  ifn (cl->serialize)
    error(NIL,"Serialization error (unknown serialization class)", name);
  *pp = 0;
  (*cl->serialize)(pp,SRZ_READ);
  UNLOCK(cptr);
}

static void 
local_bread_object(at **pp,  at *opt)
{
  int size,i,j;
  at *name, *cname, *cptr;
  struct oostruct *s;
  char allow_incomplete = '\1';
  
  if (opt) {
    char *optname = pname(opt);
    if (!strcmp(optname,"strict")) {
      allow_incomplete = '\0';
    } else {
      error(NIL, "Usage: (bread ['strict])", opt);
    }
  }
  size = read_card24();
  if (local_bread(&cname, NIL))
    error(NIL,"Corrupted file (accessing class name)",NIL);
  ifn (EXTERNP(cname,&symbol_class))
    error(NIL,"Corrupted binary file (class name expected)",cname);
  cptr = var_get(cname);
  ifn (EXTERNP(cptr,&class_class))
    error(NIL,"Corrupted binary file (class expected)",cname);
  
  *pp = new_oostruct(cptr);     /* create structure */
  s = (*pp)->Object;            /* access slots */
  if (size > s->size) {
    error(NIL,"Class definition was richer at save time",cname);
  } else {
    if ( (size < s->size) && !allow_incomplete) {
      error(NIL,"Class definition enriched  since save time",NIL);
    }
  }
  for(i=j=0; i<size; i++,j++) 
  {
    if (local_bread(&name, NIL))
      error(NIL,"Corrupted file (accessing slot name)",cname);
    ifn (EXTERNP(name,&symbol_class))
      error(NIL,"Corrupted binary file (slot name expected)",cname);
    while (j<s->size && name!=s->slots[j].symb)
      j += 1;
    if (j>= s->size)
      error(NIL,"Incompatible class definition",cname);
    UNLOCK(name);
    UNLOCK(s->slots[j].val);
    s->slots[j].val = NIL;
    local_bread(&(s->slots[j].val), NIL);
  }
  UNLOCK(cname);
  UNLOCK(cptr);
}

static void
local_bread_class(at **pp)
{
  at *name, *super, *key, *def;
  struct class *cl;
  if (   local_bread(&name, NIL)
      || local_bread(&super, NIL)
      || local_bread(&key, NIL) 
      || local_bread(&def, NIL) )
    error(NIL,"Corrupted file (unresolved critical class component!)",NIL);
  *pp = defclass(name,super,key,def);
  cl = (*pp)->Object;
  local_bread(&cl->methods, NIL);
}


static void
local_bread_cclass(at **pp)
{
  at *q;
  class *cl = rootclasslist;
  
  if (local_bread(&q, NIL))
    error(NIL,"Corrupted file (unresolved symbol!)",NIL);
  while (cl && cl->classname!=q)
    cl = cl->nextclass;
  if (!cl)
    error(NIL,"Cannot read this class",q);
  UNLOCK(q);
  q = cl->backptr;
  LOCK(q);
  *pp = q;
  local_bread(&cl->methods, NIL);
}



static void
local_bread_array(at **pp)
{
  struct array *arr;
  int dim[MAXDIMS];
  int i, size, ndim;
  
  ndim = read_card24();
  size = 1;
  if (ndim>MAXDIMS)
    error(NIL,"corrupted binary file (too many dims in array)",NIL);
  for (i=0; i<ndim; i++) {
    dim[i] = read_card32();
    size *= dim[i];
  }
  *pp = array(ndim,dim);
  arr = (*pp)->Object;
  pp = arr->data;
  for (i=0; i<size; i++)
    local_bread(pp++, NIL);
}



static void 
local_bread_lfunction(at **pp, at *(*new) (at*, at*))
{
  struct function *f;
  *pp = (*new)(NIL,NIL);
  f = (*pp)->Object;
  local_bread( & f->formal_arg_list, NIL );
  local_bread( & f->evaluable_list, NIL );
}


static void 
local_bread_cfunction(at **pp)
{
  struct chunk_header *current_chunk; 
  struct function *f;
  at *q;
  if (local_bread(&q, NIL))
    error(NIL,"Corrupted file (unresolved symbol!)",NIL);
  iter_on( &function_alloc, current_chunk, f ) { 
    if (f->c_function)
      if (q == f->evaluable_list) {
	*pp = f->formal_arg_list;
	LOCK(*pp);
	UNLOCK(q);
	return;
      }
  }
  error(NIL,"Cannot find builtin function",q);
}



/* This is the basic reading routine */

static int
local_bread(at **pp, at *opt)
{
  int tok;
  int ret = 1;
  
 again:
  
  tok = read_card8();
  switch (tok)
    {
      
    case TOK_DEF:
      {
	int xdef;
	xdef = read_card24();
	if (local_bread(pp, opt))
	  error(NIL,"corrupted binary file (unresolved ref follows def)",NIL);	  
	if (relocf[xdef] == R_DEFD)
	  error(NIL,"corrupted binary file (duplicate reloc definition)",NIL);
	if (relocf[xdef] == R_REFD)
	  define_refd_reloc(xdef,*pp);
	relocf[xdef] = R_DEFD;
	relocp[xdef] = *pp;
	return 0;
      }
      
    case TOK_REF:
      {
	int xdef;
	xdef = read_card24();
	if (xdef<0 || xdef>=relocn)
	  error(NIL,"corrupted binary file (illegal ref)",NIL);
	if (relocf[xdef] == R_DEFD) {
	  *pp = relocp[xdef];
	  LOCK(*pp);
	  return 0;
	} else {
	  *pp = relocp[xdef];
	  relocp[xdef] = pp;
	  relocf[xdef] = R_REFD;
	  return ret;
	}
      }
      
    case TOK_NIL:
      {
	*pp = NIL;
	return 0;
      }
      
    case TOK_CONS:
      {
	*pp = new_cons(NIL,NIL);
	local_bread( & ((*pp)->Car), opt );
	pp = & ((*pp)->Cdr);
	ret = 0;
	goto again;
      }
      
    case TOK_NUMBER:
      {
	*pp = new_number(0.0);
	read_buffer( &(*pp)->Number, sizeof(real) );
	if (swapflag)
	  swap_buffer( &(*pp)->Number, 1, sizeof(real) );
	return 0;
      }
      
    case TOK_STRING:
      {
	int l = read_card24();
	*pp = new_string_bylen(l);
	read_buffer(SADD((*pp)->Object),l);
	return 0;
      }
      
    case TOK_SYMBOL:
      {
	int l = read_card24();
	read_buffer(string_buffer,l);
	string_buffer[l] = 0;
	*pp = named(string_buffer);
	return 0;
      }
      
      
    case TOK_OBJECT:
      {
	local_bread_object(pp, opt);
	return 0;
      }
      
    case TOK_CLASS:
      {
	local_bread_class(pp);
	return 0;
      }
      
      
    case TOK_MATRIX:
      {
	*pp = load_matrix(fin);
	return 0;
      }
      
    case TOK_ARRAY:
      {
	local_bread_array(pp);
	return 0;
      }
      
      
    case TOK_DE:
      {
	local_bread_lfunction(pp,new_de);
	return 0;
      }
      
    case TOK_DF:
      {
	local_bread_lfunction(pp,new_df);
	return 0;
      }
      
      
    case TOK_DM:
      {
	local_bread_lfunction(pp,new_dm);
	return 0;
      }
      
    case TOK_CFUNC:
      {
	local_bread_cfunction(pp);
	return 0;
      }
      
    case TOK_CCLASS:
      {
	local_bread_cclass(pp);
	return 0;
      }
      
    case TOK_COBJECT:
      {
	local_bread_cobject(pp);
	return 0;
      }
      
    default:
      error(NIL,"corrupted binary file (illegal token in file)",NEW_NUMBER(tok));
    }
}



at *
bread(FILE *f, at *opt)
{
  at *ans = NIL;
  int tok;
  
  if (in_bwrite!=0)
    error(NIL,"Recursive binary read/write are forbidden",NIL);
  fin = f;
  in_bwrite = -1;
  tok = read_card8();
  if (tok != BINARYSTART)
    error(NIL,"corrupted binary file (cannot find BINARYSTART)",NIL);
  tok = read_card24();
  if (tok<0 || tok>10000000)
    error(NIL,"corrupted binary file (illegal reloc number)",NEW_NUMBER(tok));    
  clear_reloc(tok);
  local_bread(&ans, opt);
  forbid_refd_reloc();
  in_bwrite = 0;
  return ans;
}

DX(xbread)
{
  at *opt = NIL;
  
  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 1:
    opt = APOINTER(1);
  case 0:
    return bread(context->input_file, opt);
  default:
    ARG_NUMBER(-1);
    return NIL;
  }
}


/*** INITIALISATION ***/

void 
clear_bwrite_flag(void)
{
  in_bwrite = 0;
}

void 
init_binary(void)
{
  set_swapflag();
  dx_define("bwrite",xbwrite);
  dx_define("bread",xbread);
}
