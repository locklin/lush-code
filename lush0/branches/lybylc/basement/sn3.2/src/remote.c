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

/* REMOTE PROCEDURE CALL */

#define NODEBUG


/* REMOTE TABLE OBJECT ... */

static void 
  rtable_dispose(p)
at *p;
{
  struct rtable *r;
  int i;

  r = p->Object;
  if (r) {
    UNLOCK(r->atfin);
    UNLOCK(r->atfout);
    for (i=0;i<r->nrc;i++)
      UNLOCK(r->classes[i]);
    
#ifndef LEON_FUTURE_REMOTE_GC
    /* this thing is just awful */
    {
      struct rhandle *rh;
      rh = r->head.next;
      while (rh!=&(r->head)) {
	UNLOCK(rh->backptr);
	rh = rh->next;
      }
    }
#endif

    free(r);
  }
}



static void 
  rtable_action(p,action)
at *p;
void (*action)();
{
  struct rtable *r;
  int i;
  r = p->Object;
  if (r) {
    (*action)(r->atfin);
    (*action)(r->atfout);
    for (i=0;i<r->nrc;i++)
      (*action)(r->classes[i]);

#ifndef LEON_FUTURE_REMOTE_GC
    /* this thing is just awful */
    {
      struct rhandle *rh;
      rh = r->head.next;
      while (rh!=&(r->head)) {
	(*action)(rh->backptr);
	rh = rh->next;
      }
    }
#endif
  }
}



class rtable_class = {
  rtable_dispose,
  rtable_action,
  generic_name,
  generic_eval,
  generic_listeval,
  XT_RTABLE,
  CL_NODELETE,
};



at 
  *new_rtable(nrc,atfin,atfout)
int nrc;
at *atfin, *atfout;
{
  File *fin, *fout;
  struct rtable *r;
  struct oostruct *s;
  at *ans;

  fin = atfin->Object;
  fout = atfout->Object;
  r = calloc( sizeof(struct rtable) + (nrc-1)*sizeof(at*), 1 );
  ifn (r)
    error(NIL,"memory exhausted",NIL);
  r->atfin = atfin;
  r->atfout = atfout;
  r->fin = fin;
  r->fout = fout;
  r->nrc = nrc;
  r->async = FALSE;

  r->head.prev = &(r->head);
  r->head.next = &(r->head);
  r->head.backptr = NIL;
  r->head.rtable = NIL;
  r->head.classid = -1;
  r->head.address = -1;

  LOCK(atfin);
  LOCK(atfout);
  s = new_oostruct(rtable_class.backptr,nrc+1);
  ans = new_extern(&rtable_class,r,XT_RTABLE);
  ans->Slots = s;
  return ans;
}







/* REMOTE HANDLE ... */

struct alloc_root rhandle_alloc =
{
  NIL,
  NIL,
  sizeof(struct rhandle),
  100
};


static void 
  rhandle_dispose(p)
at *p;
{
  struct rhandle *r;
  if (r=p->Object) {

#ifdef DEBUG
    printf("dispose:: class=%d, addr=%x\n",
	   r->classid, r->address);
#endif

#ifdef LEON_FUTURE_REMOTE_GC
    if (r->prev && r->next) {
      struct rtable *rt;
      rt = r->rtable->Object;
      out_dref(rt->fout,r->classid,r->address);
    }
#endif

    UNLOCK(r->rtable);
    if (r->prev) 
      r->prev->next = r->next;
    if (r->next)
      r->next->prev = r->prev;
    r->backptr = NIL;
    r->rtable = NIL;
    deallocate(&rhandle_alloc,r);
    p->Object = 0;

  }
}

static void 
  rhandle_action(p,action)
at *p;
void (*action)();
{
  struct rhandle *r;
  if (r=p->Object) {
    (*action)(r->rtable);
  }
}


/* REMOTE OBJECT ... */


static char *
  robject_name(p)
at *p;
{
  struct rhandle *r;
  if (r=p->Object) 
    sprintf(string_buffer, "::%s:%lx", 
	    nameof(p->Class->classname),(long) (r->address));
  else
    sprintf(string_buffer, "::%s:%lx", 
	    nameof(p->Class->classname),(long) 0);
  return string_buffer;
}

class robject_class = {
  rhandle_dispose,
  rhandle_action,
  robject_name,
  generic_eval,
  generic_listeval,
  XT_ROBJECT,
};


at
  *new_robject(rt,classid,address)
at *rt;
int classid;
int address;
{
  struct rhandle *rh;
  struct rtable *r;
  at *ans;
  
  r = rt->Object;

#ifdef DEBUG
  printf("classid=%d, address=%x\n",classid,address);
#endif

  /* Sweep through the rtable queue to see if it already exists */

  rh = r->head.next;
  while (rh != &(r->head)) {
    if (rh->address == address) 
      if (classid==-1 || classid==rh->classid) {
	ans = rh->backptr;
	LOCK(ans);
	return ans;
      }
    rh = rh->next;
  }
  

  /* It does not exists. Create one */

  if (classid==-1) {
    error(NIL,"RREF token received for an unknown object",NIL);
  }
  
  if (classid<0 || classid>=r->nrc || !r->classes[classid] )
    error(NIL,"bad class id",NEW_NUMBER(classid));

  rh = allocate(&rhandle_alloc);

  rh->rtable = rt;
  LOCK(rt);
  rh->classid = classid;
  rh->address = address;
  ans = new_extern(r->classes[classid]->Object,rh,XT_ROBJECT);
  rh->backptr = ans;

  rh->prev = &(r->head);
  rh->next = r->head.next;
  r->head.next->prev = rh;
  r->head.next = rh;

#ifndef LEON_FUTURE_REMOTE_GC
  LOCK(ans);
#endif

  return ans;
}


/* REMOTE CALL ... */

class rcall_class = {
  rhandle_dispose,
  rhandle_action,
  generic_name,
  generic_eval,
  generic_listeval,
  XT_RCALL,
};


at
  *new_rcall(rt,name,classid,address)
at *rt;
at *name;
int classid;
int address;
{
  struct rhandle *rh;
  struct rtable *r;
  at *ans;
  
  ifn (r = rt->Object)
    error(NIL,"Empty remote table",rt);

  if (classid<0 || classid>=r->nrc || !r->classes[classid] )
    error(NIL,"bad class id",NEW_NUMBER(classid));
  rh = allocate(&rhandle_alloc);
  rh->rtable = rt;
  LOCK(rt);

  rh->next = NIL;
  rh->prev = NIL;

  rh->classid = classid;
  rh->address = address;
  ans = new_extern(&rcall_class,rh,XT_RCALL);
  rh->backptr = ans;

  putmethod(r->classes[classid]->Object,ans,name);
  return ans;
}


/* RCLASS */


void
  new_rclass(rt,name,classid,parentid)
at *rt;
at *name;
int classid;
int parentid;
{
  struct rtable *r;
  at *p;

  ifn (r = rt->Object)
    error(NIL,"Empty remote table",rt);

  /* set the class in r */
  if (classid<0 || classid>=r->nrc)
    error(NIL,"bad class id",NEW_NUMBER(classid));
  if (parentid>=r->nrc)
    error(NIL,"bad class id",NEW_NUMBER(parentid));
  if (r->classes[classid])
    error(NIL,"redefinition of class",NEW_NUMBER(classid));
  if (parentid>=0 && !r->classes[parentid])
    error(NIL,"superclass still is undefined",NEW_NUMBER(classid));
  if (parentid>=0)
    p = r->classes[parentid];
  else
    p = robject_class.backptr;
  r->classes[classid] = defclass(name,p,NIL,NIL);
}







/* BINDING PROTOCOL */


extern at *at_true;


at *
  bind_remote_process(atfin,atfout,table)
at *atfin,*atfout;
at **table;
{
  File *fin,*fout;
  int nrc;
  struct oostruct *s;
  struct rtable *r;
  struct rhandle  *rf;
  at *p, *q;
  int i;
  char *name;
  int classid,parentid;
  int address;
  at *rtable;
  
  
  ifn (atfin && atfin->ctype == XT_FILE_RO)
    error(NIL,"not a read file descriptor",atfin);
  ifn (atfout && atfout->ctype == XT_FILE_WO)
    error(NIL,"not a write file descriptor",atfout);
  
  fin = atfin->Object;
  fout = atfout->Object;
  
  /* INIT THE TRANSACTION */
  out_at(fout,at_true,NIL);        
  FFLUSH(fout);

  /* GET THE NUMBER OF REMOTE CLASSES */
  p = in_at(fin);
  ifn (p && p->ctype==XT_NUMBER)
    error(NIL,"number expected on conenction",p);
  nrc = (int)(p->Number);
  UNLOCK(p);

#ifdef DEBUG
  printf("-Receiving %d rclasses\n",nrc);
  fflush(stdout);
#endif

 
  /* allocate the remote table */
  *table = rtable = new_rtable(nrc,atfin,atfout);
  r = rtable->Object;
  s = rtable->Slots;
  
  /* GET THE REMOTE CLASS NAMES AND IDS */
  for (i=0; i<nrc; i++) {

    /* get a list (name classid parentid) */
    p = in_at(fin);
    
    /* parse the list */
    q = p;
    ifn (CONSP(q) && (q=q->Car) && (q->ctype==XT_STRING))
      goto illegalrc;
    name = SADD(q->Object);
    q = p->Cdr;
    ifn (CONSP(q) && (q=q->Car) && (q->flags&C_NUMBER))
      goto illegalrc;
    classid = (int)(q->Number);
    q = p->Cdr->Cdr;
    ifn (CONSP(q) && (q=q->Car) && (q->flags&C_NUMBER))
      goto illegalrc;
    parentid = (int)(q->Number);
    q = named(name);
    UNLOCK(p);
    
#ifdef DEBUG
    printf("-Defining class %s, %d, %d\n",name,classid,parentid);
    fflush(stdout);
#endif
    new_rclass(rtable,q,classid,parentid);
    s->slots[classid].symb = q;
    s->slots[classid].val = r->classes[classid];
    LOCK(r->classes[classid]);
    UNLOCK(q);
  }

  
  /* GET THE METHODS */
  forever {

    /* get a list (name, address, classid) */
    p = in_at_rt(fin,rtable);
    ifn (p && (p->flags&C_CONS))
      break;
    
    /* parse the list */
    q = p;
    ifn (CONSP(q) && (q=q->Car) && (q->ctype==XT_STRING))
      goto illegalrc;
    name = SADD(q->Object);
    q = p->Cdr;
    ifn (CONSP(q) && (q=q->Car) && (q->flags&C_NUMBER))
      goto illegalrc;
    address = (int)(q->Number);
    q = p->Cdr->Cdr;
    ifn (CONSP(q) && (q=q->Car) && (q->flags&C_NUMBER))  {
    illegalrc:
      error(NIL,"illegal remote class description",p);
    }

    classid = (int)(q->Number);
    if (classid<0 || classid>=nrc)
      error(NIL,"bad class id",NEW_NUMBER(classid));
    q = named(name);
    UNLOCK(p);

#ifdef DEBUG
    printf("-Defining method %s, %d, %d\n",name,address,classid);
    fflush(stdout);
#endif
    
    /* creates the rcall object and puts it */
    p = new_rcall(rtable,q,classid,address);
    UNLOCK(p);
    UNLOCK(q);
    
  }
  
  /* RETURN THE BASE OBJECT */
  ifn (p && (p->ctype==XT_ROBJECT))
    error(NIL,"remote object expected",p);

  s->slots[s->size-1].symb = at_true;
  s->slots[s->size-1].val = p;
  LOCK(p);

  return p;
}


DX(xbind_remote_process)
{
  at *symb=NIL;
  at *table;
  at *ans;

  ALL_ARGS_EVAL;
  if (arg_number==3)
    symb = APOINTER(3);
  else
    ARG_NUMBER(2);
  
  if (symb && symb->ctype!=XT_SYMBOL)
    error(NIL,"Not a symbol",symb);
  ans = bind_remote_process(APOINTER(1),APOINTER(2),&table);
  if (symb)
    var_set(symb,table);
  UNLOCK(table);
  return ans;
}

/* RPC PROTOCOL */

at 
  *remote_call(object,method,args)
at *object;
at *method;
at *args;
{
  at *rt;
  struct rtable *r;
  struct rhandle *ro;
  struct rhandle *rm;
  File *fout,*fin;

  ifn (object && (object->ctype==XT_ROBJECT) && (ro=object->Object))
    error(NIL,"not a valid remote object",object);
  ifn (method && (method->ctype==XT_RCALL) && (rm=method->Object))
    error(NIL,"not a valid remote call",method);

  rt = ro->rtable;
  ifn (rt && rt==rm->rtable && rt->ctype==XT_RTABLE && (r=rt->Object))
    error(NIL,"invalid remote table",rt);
  
  fout = r->fout;
  fin = r->fin;

  out_rcall(fout,r,ro->address,rm->address);
  out_at(fout,args,rt);
  FFLUSH(fout);

  if (r->async)
    return NIL;
  else
    return in_at_rt(fin,rt);
}


DX(xasynchronous_rcalls)
{
  struct rtable *rt;
  at *p;

  ALL_ARGS_EVAL;
  if (arg_number!=1)
    ARG_NUMBER(2);

  p = APOINTER(1);
  ifn (p && p->ctype==XT_RTABLE && p->Object)
    error(NIL,"not a valid remote table",p);
  rt = p->Object;
  
  if (arg_number==2)
    if (APOINTER(2))
      rt->async = TRUE;
    else
      rt->async = FALSE;
  
  if (rt->async)
    return true();
  else
    return NIL;
}



/* INIT_REMOTE */

void
  init_remote()
{
  class_define("RTABLE",&rtable_class);
  class_define("ROBJECT",&robject_class);
  class_define("RCALL",&rcall_class);

  dx_define("remote",xbind_remote_process);
  dx_define("asynchronous_rcalls",xasynchronous_rcalls);
}
