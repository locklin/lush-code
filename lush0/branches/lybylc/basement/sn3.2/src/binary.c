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

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/

#include "header.h"

extern at *at_true;

static int tokensize;
static int lasttoken = T_UNDEF;
static union { char c[sizeof(int)]; int i; } byte_ordering_tester;
#define MUST_SWAP_P  (byte_ordering_tester.c[0])


/*************************************/
/* UTILITIES */


static void
flip_ptr16(p)
unsigned char *p;
{
  if (MUST_SWAP_P) {
      unsigned char tmp = p[0];
      p[0] = p[1];
      p[1] = tmp;
  }
}

static void
flip_ptr32(p)
unsigned char *p;
{
  if (MUST_SWAP_P) {
      unsigned char tmp = p[0];
      p[0] = p[3];
      p[3] = tmp;
      tmp = p[1];
      p[1] = p[2];
      p[2] = tmp;
  }
}

static void
flip_ptr64(p)
unsigned char *p;
{
  if (MUST_SWAP_P) {
      unsigned char tmp = p[0];
      p[0] = p[7];
      p[7] = tmp;
      tmp = p[1];
      p[1] = p[6];
      p[6] = tmp;
      tmp = p[2];
      p[2] = p[5];
      p[5] = tmp;
      tmp = p[3];
      p[3] = p[4];
      p[4] = tmp;
  }
}

static int 
frag4(s)
int s;
{
  static int f[] = {0,3,2,1};
  return s+f[s&0x3];
}



/*************************************/
/*        ERROR PROTECTION           */
/*************************************/

/*
 * Resynchronize the connection
 * if an error occurs
 */


static struct context bincontext;
static at *binerrortable=NULL;

static void 
protect_error(rt)
at *rt;
{
  void i_sync();
  void o_sync();
  void o_token();
  at *in_at_rt();
  
  ifn (rt && rt->ctype==XT_RTABLE)
    return;
  if (binerrortable)
    error("binary.c/protect_error","Corrupted remote subsystem",NIL);
  binerrortable = rt;
  
  context_push(&bincontext);
  ifn (setjmp(context->error_jump))
    return;
       
    
  /* BRANCH HERE WHEN AN ERROR OCCURS */

  if (   binerrortable 
      && binerrortable->ctype == XT_RTABLE
      && !ferror((FILE *) (((struct rtable*) (binerrortable->Object))->fin))
      && !feof((FILE *) (((struct rtable*)(binerrortable->Object))->fin))
      && !ferror((FILE *) (((struct rtable*)(binerrortable->Object))->fout))){

    struct rtable *r;
    at *p;
    r = binerrortable->Object;
    binerrortable = NIL;

#ifdef LEON_FUTURE_SYNCHRO
    /* Push a synchronisation request */
    o_sync(r->fout);
    /* Now, the remote process synchronizes */
    i_sync(r->fin);
#else
    /* Push an error token */
    o_token(r->fout,T_ERROR);
    FFLUSH(r->fout);
    /* Wait for the complaint of the remote process */
    for (;;)
      if (i_token(r->fin) == T_ERROR)
	if (tokensize == 0)
	  if (i_token(r->fin) == T_LPAREN) {
	    lasttoken = T_LPAREN;
	    break;
	  }
    p = in_at_rt(r->fin,rt);
    o_sync(r->fout);
    FFLUSH(r->fout);
    UNLOCK(p);
#endif
  }
  
  /* remove the traces */
  context_pop();
  longjmp(context->error_jump,-1L);
}



static void 
unprotect_error(rt)
at *rt;
{
  ifn (rt && rt->ctype==XT_RTABLE)
    return;
  ifn (binerrortable)
    error("binary.c/unprotect_error","Corrupted remote subsystem",NIL);
  binerrortable = NULL;
  context_pop();
}








/*************************************/
/*         OUTPUT FUNCTIONS          */
/*************************************/



static void 
o_int4(f,xa)
File *f;
int xa;
{
    int x = xa;
    flip_ptr32((unsigned char *)&x);
    PFWRITE(&x, sizeof(int), 1, f);
}


static void 
o_flt4(f,xa)
File *f;
double xa;
{
    flt x = xa;
    flip_ptr32((unsigned char *)&x);
    PFWRITE(&x, sizeof(flt), 1, f);  
}

void 
o_token(f,t)
File *f;
int t;
{
    o_int4(f,(TOKENOFFSET+t)<<24);
    if (FERROR(f))
        TEST_FILE_ERROR(f);
}

static void 
o_token_sz(f,t,sz)
File *f;
int t;
int sz;
{
    o_int4(f,((TOKENOFFSET+t)<<24)+(sz&0xffffff));
    if (FERROR(f))
        TEST_FILE_ERROR(f);
}


void 
o_sync(f)
File *f;
{
    o_token(f,T_SYNC);
    o_int4(f,0x000004d2);
    o_int4(f,0x000010e1);
    o_int4(f,0x00001a85);
    o_int4(f,0x00002694);
    o_int4(f,0x00000000);
    FFLUSH(f);
}

static void 
compact_o_token(f,t,short_i)
File *f;
int t;
unsigned int short_i;
{
    o_int4(f,((TOKENOFFSET+t)<<24) + (short_i & 0xffffff));
    if (FERROR(f))
        TEST_FILE_ERROR(f);
}

void 
bugged_boser_lisp_out_int(f, i)
File *f;
int i;
{
    if (i >=0 && i< 838808)
        /* SHOULD BE 8388608 = 2^23 */
        /* but we emulate Boser Lisp's bug for compatibility */
        /* with GDBM database files (key encoding) */
        compact_o_token(f, T_SHORT_INT, i);
    else
    {	
        o_token(f, T_INT);
        o_int4(f, i);
    }
}


void 
out_real(f,ra)
File *f;
double ra;
{
    real r = ra;
    o_token(f,T_REAL);
    flip_ptr64((unsigned char*)&r);
    PFWRITE(&r,sizeof(real),1,f);
}

char zeros[5]="\0\0\0\0";

static void 
out_str(f,t,s)
File *f;
int t;
char *s;
{
  int l = strlen(s), pad;
  static int pad_len[] = {0,3,2,1};
  pad=pad_len[l & 0x3];

  o_token_sz(f,t,l);
  PFWRITE(s, l, sizeof(char), f);
  if (pad>0)
    PFWRITE(zeros, pad, sizeof(char), f);
}


/* 
 * The protocol do not support
 * all the types of matrix we want.
 * Only two types, AT and F are
 * transmitted.
 */


static void 
out_array(f,m,r)
File *f;
struct index *m;
at *r;
{
  int j,off, d[MAXDIMS], ndim;
  struct idx idx;
  struct srg srg;
  at **data;
  
  o_token_sz(f,T_ARRAY,m->ndim);
  for (j=0;j<m->ndim;j++) {
    d[j]=0;
    o_int4(f,m->dim[j]);
  }

  index_to_idx(m, &idx);
  srg = m->st->srg;
  idx.srg = &srg;
  
  if (srg.type!=ST_AT)
    error("binary.c/out_array","Type is not ST_AT",NIL);


  /* This is an optimized begin_idx_aloop, end_idx_aloop */
  off = 0;
  data = IDX_DATA_PTR(&idx);
  ndim = m->ndim;
  j = ndim;
  while (j >= 0) {
    if (j == ndim) {
      j--;
      out_at(f, data[off], r);
    }
    if (++d[j] < m->dim[j]) {
      off += m->mod[j];
      j++;
    } else {
      ifn (r && r!=at_true) {
	CHECK_MACHINE("on");
      }
      off -= m->dim[j] * m->mod[j];
      d[j--] = -1;
    }
  }
  storage_rls_srg(m->st, &srg);
}


static void 
out_matrix(f,m)
File *f;
struct index *m;
{
  int j,off,d[MAXDIMS],ndim;
  struct idx idx;
  struct srg srg;
  ptr data;
  flt (*getf)();
  
  o_token_sz(f,T_MATRIX,m->ndim);
  for (j=0;j<m->ndim;j++) {
    d[j]=0;
    o_int4(f,m->dim[j]);
  }
  
  index_to_idx(m, &idx);
  srg = m->st->srg;
  idx.srg = &srg;

  /* This is an optimized begin_idx_aloop, end_idx_aloop */
  off = 0;
  data = IDX_DATA_PTR(&idx);

  getf = storage_type_getf[srg.type];
  ndim = m->ndim;
  j = ndim;
  while (j >= 0) {
    if (j == ndim) {
      j--;
      o_flt4(f, (*getf)(data,off));
    }
    if (++d[j] < m->dim[j]) {
      off += m->mod[j];
      j++;
    } else {
      off -= m->dim[j] * m->mod[j];
      d[j--] = -1;
    }
  }
  storage_rls_srg(m->st, &srg);
}

static void
out_index(f,m,r)
File *f;
struct index *m;
at *r;
{
  switch( m->st->srg.type ) {
  case ST_AT:
    out_array(f,m,r);
    break;
  default:
    out_matrix(f,m);
    break;
  }
}


static void 
out_robject(f,r,rt)
File *f;
struct rhandle *r;
at *rt;
{
  if (rt!=r->rtable) {
    protect_error(rt);
    error("binary protocol","Wrong remote connection",rt);
    unprotect_error(rt);
  }
  o_token_sz(f,T_RCLASS,r->classid);
  o_int4(f,r->address);
}


static void 
out_oostruct(f,p)
File *f;
at *p;
{
  class *c;
  struct oostruct *s;
  char *str;
  int i;

  s = p->Slots;
  c = classof(p);
  str = nameof(c->classname);
  out_str(f, T_CLASS, str);
  if (p->Object) {
    out_at(f,p,at_true);
  }
  o_token(f,T_LPAREN);
  for(i=0;i<s->size;i++) {
    out_str(f,T_SYMBOL,nameof(s->slots[i].symb));
    out_at(f,s->slots[i].val,NIL);
  }
  o_token(f,T_RPAREN);
}




void
out_rcall(fout,r,roaddress,rmaddress)
File *fout;
struct rtable *r;
int roaddress;
int rmaddress;
{
  if (r->async) 
    o_token_sz(fout, T_RFCALLA, roaddress );
  else
    o_token_sz(fout, T_RFCALL, roaddress );
  o_int4(fout,rmaddress);
}




void 
out_at(f,p,r)
File *f;
at *p;
at *r;
{
  ifn (r && r->ctype==XT_RTABLE) {
    protect_error(r);
    CHECK_MACHINE("on");
    unprotect_error(r);
  }
  
  if (!p)
    o_token(f,T_NIL);
  else if (p->Slots && !r)
    out_oostruct(f,p);
  else
    switch(p->ctype)  {
      
    case XT_CONS: 
      {
	o_token(f,T_LPAREN);
	while( CONSP(p) ) {
	  out_at(f,p->Car,r);
	  p = p->Cdr;
	}
	if (p) {
	  o_token(f,T_DOT);
	  out_at(f,p,r);
	}
	o_token(f,T_RPAREN); 
      }
      break;

    case XT_INDEX:
      out_index(f,p->Object,r);
      break;

    case XT_NUMBER:
      out_real(f,p->Number);
      break;
      
    case XT_SYMBOL:
      if (p==at_true)
	o_token(f,T_TRUE);
      else
	out_str(f,T_SYMBOL,nameof(p));
      break;

    case XT_STRING:
      out_str(f,T_STRING,SADD(p->Object));
      break;
      
    case XT_ROBJECT:
      out_robject(f,p->Object,r);
      break;
      
    default:
      protect_error(r);
      error("binary protocol","No binary encoding for object",p);
      unprotect_error(r);
    }
}


DX(xwrite_binary)
{
  at *p;
  int i;

  if (arg_number<2)
    ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  if (ISSTRING(1)) {
    p = OPEN_WRITE(ASTRING(1), NIL);
  } else if ((p = APOINTER(1)) && (p->ctype==XT_FILE_WO)) {
    LOCK(p);
  } else if ((p = APOINTER(1)) && (p->ctype==XT_U8STORAGE)) {
	 p = new_extern(&file_R_class,pseudo_fopen(p, "w"),XT_FILE_WO);
  } else
    error(NIL,"not a file",p);
  
  for(i=2;i<=arg_number;i++) {
    out_at(p->Object,APOINTER(i),NIL);
    TEST_FILE_ERROR(p->Object);
  }

  UNLOCK(p);
  return NIL;
}




/*************************************/
/*        INPUT FUNCTIONS            */
/*************************************/


static int
i_int4(f)
File *f;
{
  int x;
  PFREAD(&x,sizeof(int),1,f);
  flip_ptr32(&x);
  return x;
}


static char*
i_str(f,sz)
File *f;
int sz;
{
  static int buffersize = 0;
  static char *buffer = 0;

  if (sz>=buffersize-8) {
    if (buffer)
      free(buffer);
    buffer = malloc( buffersize = sz+1024 );
  }
  PFREAD(buffer,sizeof(char),frag4(sz),f);
  buffer[sz]=0;
  return buffer;
}


int 
i_token(f)
File *f;
{
  unsigned int x;
  x = lasttoken;
  if (x != T_UNDEF) {
    lasttoken = T_UNDEF;
    return x;
  } else {
    x = i_int4(f);
    if (FEOF(f))
      error("read_binary","unexpected EOF: Remote program is dead",NIL);
    if (FERROR(f))
      TEST_FILE_ERROR(f);
    tokensize = x&0xffffff;
    return (x>>24)-TOKENOFFSET;
  }
}


void 
i_sync(f)
File *f;
{
  for (;;)
      if (i_token(f) == T_SYNC)
          if (tokensize == 0)
              if (i_int4(f) == 0x000004d2L)
                  if (i_int4(f) == 0x000010e1L)
                      if (i_int4(f) == 0x00001a85L)
                          if (i_int4(f) == 0x00002694L)
                              if (i_int4(f) == 0x00000000L)
                                  break;
}


static real
in_real(f)
File *f;
{
    real x;
    int tk;
    tk = i_token(f);
    switch(tk)
    {
    case T_REAL:
        PFREAD(&x,sizeof(real),1,f);
        flip_ptr64((unsigned char*)&x);
        return x;
    case T_INT:
        tk = i_int4(f);
        return (real)tk;
    default:
        error("read_binary","A number was expected",NIL);
    }
}


at * 
in_at_rt(f,rt)
File *f;
at *rt;
{
  int tk;

  while (1) {
  
    switch(tk=i_token(f)) {
      
    case T_LPAREN:
      {
	at *ans; 
	at **where = &ans;
	
	do {
	  *where = cons(in_at_rt(f,rt),NIL);
	  where = &( (*where)->Cdr );
	  lasttoken = i_token(f);
	} while (lasttoken != T_RPAREN && lasttoken != T_DOT);
	if (i_token(f) == T_DOT) {
	  *where = in_at_rt(f,rt);
	  ifn (i_token(f) == T_RPAREN) {
	    protect_error(rt);
	    error("binary protocol","illegal dotted pair",NIL);
	    unprotect_error(rt);
	  }
	}
	return ans;
      }
      
    case T_SYMBOL:
      return named( i_str(f,tokensize) );
      
    case T_REAL:
    case T_INT:
      lasttoken = tk;
      return NEW_NUMBER( in_real(f) );
      
    case T_STRING:
      return new_string( i_str(f,tokensize) );
      
    case T_CLASS:
      {
	struct oostruct *s;
	at *c,*p,*v,*k;
	int i;
	
	k = named( i_str(f,tokensize) );
	c = eval(k);
	ifn (c && (c->ctype==XT_CLASS)) {
	  protect_error(rt);
	  error("binary protocol","Cannot read an object of undefined class",k);
	  unprotect_error(rt);
	}
	UNLOCK(k);

	lasttoken = i_token(f);
	
	if (lasttoken==T_LPAREN) {
	  p = new_object(c);
	} else {
	  k = in_at_rt(f,rt);
	  p = cast(k,c);
	  UNLOCK(k);
	}
	
	ifn (s = p->Slots) {
	  protect_error(rt);
	  error("binary protocol","class has no slots?",c);
	  unprotect_error(rt);
	}
	if (i_token(f) != T_LPAREN) {
	  protect_error(rt);
	  error("binary protocol","bad binary oostruct",NIL);
	  unprotect_error(rt);
	}
	
	i = -1;
	while ((lasttoken=i_token(f)) != T_RPAREN) {
	  k = in_at_rt(f,rt);
	  v = in_at_rt(f,rt);
	  /* A complex logic allows for missing or reordered slots */
	  while ( ++i < s->size)
              if (k==s->slots[i].symb)
                  goto slotfound;
          for(i=0; i<s->size; i++)
	      if (k == s->slots[i].symb)
                  goto slotfound;
	  
	  protect_error(rt);
	  error("read_binary","slot not found",k);
	  unprotect_error(rt);
	  
	slotfound:
	  UNLOCK(k);
	  k = s->slots[i].val;
	  s->slots[i].val = v;
	  UNLOCK(k);
	} 
	if (i_token(f) != T_RPAREN) {
	  protect_error(rt);
	  error("binary protocol","bad binary oostruct",NIL);
	  unprotect_error(rt);
	}
	UNLOCK(c);
	return p;
      }
      

  case T_MATRIX:
	{
	  int ndim,i,size;
	  int dim[MAXDIMS];
	  at *ans, *atst;
	  struct storage *st;
	  flt *p;
	  
	  ndim = tokensize;
	  size = 1;
	  for (i=0;i<ndim;i++) {
	    dim[i] = i_int4(f);
	    size *= dim[i];
	  }
	  
	  protect_error(rt);
	  atst = new_F_storage();
          if (size>0)	
              storage_malloc(atst,size,0);
	  st = atst->Object;
	  unprotect_error(rt);
          
	  if (size>0) {
              PFREAD(st->srg.data,sizeof(flt),size,f);
              TEST_FILE_ERROR(f);
          }
          if (MUST_SWAP_P) {
              p = (flt*)(st->srg.data);
              for (i=0; i<size; i++)
                  flip_ptr32(p++);
          }
          ans = new_index(atst);
	  if (size>0) 
              index_dimension(ans,ndim,dim);
	  UNLOCK(atst);
	  return ans;
      }
      
    case T_ARRAY:
	{
	  int ndim,i,size;
	  int dim[MAXDIMS];
	  at *ans, *atst;
	  struct storage *st;
	  at *p;
	  ndim = tokensize;
	  size = 1;
	  for (i=0;i<ndim;i++) {
              dim[i] = i_int4(f);
	    size *= dim[i];
	  }
	  protect_error(rt);
	  atst = new_AT_storage();
	  if (size>0) storage_malloc(atst,size,1);
	  st = atst->Object;
	  unprotect_error(rt);
	  for (i=0; i<size; i++) {
	    p = in_at_rt(f,rt);
	    (*st->setat)(st,i,p);
	    UNLOCK(p);
	  }
	  ans = new_index(atst);
	  if (size>0) index_dimension(ans,ndim,dim);
	  UNLOCK(atst);
	  return ans;
      }
      
    case T_NIL:
      return NIL;
      
    case T_TRUE:
      return true();
      
    case T_DREF:
      {
	at *p;
#ifdef DEBUG
	printf("dref::");
#endif
	if (!rt)
	  error("read_binary","Unexpected DREF token",NIL);
	protect_error(rt);
	p = new_robject(rt,-1, tokensize);
	delete_at(p);
	unprotect_error(rt);
	/* LOOPS AGAIN */
	break;
      }
      
    case T_RREF:
      {
	at *p;
#ifdef DEBUG
	printf("rref::");
#endif
	if (!rt)
	  error("read_binary","Unexpected RREF token",NIL);
	
	protect_error(rt);
	p = new_robject(rt,-1, tokensize);
	unprotect_error(rt);
	return p;
      }
      
    case T_RCLASS:
      {
	at *p;
	int classid;
	int address;
#ifdef DEBUG
	printf("rclass::");
#endif
	if (!rt)
	  error("read_binary","Unexpected RCLASS token",NIL);
	
	address = i_int4(f);
	classid = tokensize;
	protect_error(rt);
	p = new_robject(rt,classid,address);
	unprotect_error(rt);
	return p;
      }
    /*case T_SHORT_INT:
      {
      	lasttoken = tk;
      	return NEW_NUMBER(((int) i_int2(f)));
      }*/
      
    case T_ERROR:
      {
	if (rt && rt->Object) {
	  at *p,*q;
	  p = in_at_rt(f,rt);
	  
#ifndef LEON_FUTURE_SYNCHRO	
	  {
	    struct rtable *r;
	    r = rt->Object;
	    o_sync(r->fout);
	  }
#endif
	  if (CONSP(p)) {
	    q = p->Car;
	    p = p->Cdr;
	    if (p && !p->Cdr)
	      p = p->Car;
	    if (q && q->ctype==XT_STRING)
	      error(NIL,SADD(q->Object), p);
	  }
	  error(NIL,"(REMOTE) error",p);
	}
      }
      
#ifdef LEON_FUTURE_SYNCHRO
      /* On reception of a synchro message, echo it */
    case T_SYNC: 
      {
	if (rt && rt->ctype==XT_RTABLE) {
	  struct rtable *r;
	  lasttoken = tk;
	  r = rt->Object;
	  i_sync(r->fin);
	  o_sync(r->fout);
	  /* LOOP AGAIN */
	  break;
	}
      }
#endif
      
    default:
      TEST_FILE_ERROR(f);
      protect_error(rt);
      error("binary protocol","illegal token",NEW_NUMBER(tk));
      unprotect_error(rt);
    }
  }
}

at
  *in_at(f)
File *f;
{
  return in_at_rt(f,NIL);
}


DX(xread_binary)
{
  at *p = NIL;
  File *f;
  at *ans;
  struct rtable *rt;

  if (arg_number==0) {
    p = NIL;
    rt = NIL;
    ifn (f = context->input_file)
      error(NIL,"no input file!",NIL);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);

    if (ISSTRING(1)) {
      p = OPEN_READ(ASTRING(1), NIL);
      f = p->Object;
      rt = NIL;
    } else if (p && (p->ctype==XT_FILE_RO)) {
      LOCK(p);
      f = p->Object;
      rt = NIL;
    } else if (p && (p->ctype==XT_U8STORAGE)) {
      p = new_extern(&file_R_class,f=pseudo_fopen(p, "r"),XT_FILE_RO);
      rt = NIL;
    } else if (p && (p->ctype==XT_RTABLE)) {
      LOCK(p);
      p = APOINTER(1);
      rt = p->Object;
      ifn (f = context->input_file)
	error(NIL,"not a file",p);
    } else {
    	error(NIL,"not a file",p); }
  }
  ans = in_at_rt(f,rt);
  UNLOCK(p);
  return ans;
}







/***************************************/

void init_binary()
{
    if (sizeof(int) != 4)
        printf("*** warning: sizeof(int)=%d, binary files will be incompatible\n");
    if (sizeof(flt) != 4)
        printf("*** warning: sizeof(flt)=%d, binary files will be incompatible\n");
    if (sizeof(real) != 8)
        printf("*** warning: sizeof(real)=%d, binary files will be incompatible\n");
    if (sizeof(int)==4 && sizeof(flt)==4 && sizeof(real)==8)
        byte_ordering_tester.i = 1;
    dx_define("write-binary",xwrite_binary);
    dx_define("read-binary",xread_binary);
}
