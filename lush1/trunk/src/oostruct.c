/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA
 * 
 ***********************************************************************/

/***********************************************************************
 * $Id: oostruct.c,v 1.2 2002-04-29 22:21:58 leonb Exp $
 **********************************************************************/

/***********************************************************************
  object oriented structures
********************************************************************** */

#include "header.h"

extern struct alloc_root symbol_alloc;

int in_object_scope = 0;
class *rootclasslist = 0;

static at *at_progn;
static at *at_mexpand;
static at *at_this;
static at *at_destroy;
static at *at_unknown;
static at *getmethod(class *cl, at *prop);
static void send_delete(at *p);


/* ------ ZOMBIE CLASS DEFINITION -------- */


static char *
zombie_name(at *p)
{
  return "( )";
}

static void
zombie_dispose(at *p)
{
  /* is it an old oostruct? */
  if (p->Object) {
    register struct oostruct *s;
    int i;
    s = p->Object;
    for(i=0;i<s->size;i++)
      UNLOCK(s->slots[i].val);
    free(s);
  }
}

static void
zombie_action(at *q, void (*action) (at *))
{
  /* is it an old oostruct? */
  if (q->Object) {
    register struct oostruct *s;
    int i;
    s = q->Object;
    for(i=0;i<s->size;i++)
      (*action)(s->slots[i].val);
  }
}

static at *
zombie_eval(at *p)
{
  return NIL;
}


class zombie_class =
{
  zombie_dispose,
  zombie_action,
  zombie_name,
  zombie_eval,
  generic_listeval,
};


/* ------ OBJECT CLASS DEFINITION -------- */


void 
oostruct_dispose(at *q)
{
  struct context mycontext;
  register struct oostruct *s;
  int oldready;
  int errflag;
  
/* 
 * This routine may be called during garbage collection.
 * If an error occurs during the destructor action,
 * an error handler zombifies the object.
 * The garbage will be restarted
 * but its destructors are cancelled.
 */

  s = q->Object;

  oldready = error_doc.ready_to_an_error;
  error_doc.ready_to_an_error = TRUE;
  q->count++;
  q->flags |= C_GARBAGE;
  context_push(&mycontext);

  errflag = sigsetjmp(context->error_jump,1);
  if (errflag == 0)
    send_delete(q);

  context_pop();
  error_doc.ready_to_an_error = oldready;
  q->count--;

  UNLOCK(s->class);
  zombie_dispose(q);
  q->Class = &zombie_class;
  q->Object = NIL;
  q->flags = C_EXTERN | X_ZOMBIE;
  if (errflag)
    siglongjmp(context->error_jump, -1L);
}


static void 
oostruct_action(at *q, void (*action) (at *))
{
  register struct oostruct *s;
  s = q->Object;
  (*action)(s->class);
  zombie_action(q,action);
}

static int
oostruct_compare(at *p, at *q, int order)
{
  int i;
  struct oostruct *s1 = p->Object;
  struct oostruct *s2 = q->Object;
  if (order)
    error(NIL,"Cannot rank objects", NIL);
  for(i=0;i<s1->size;i++)
    if (!eq_test(s1->slots[i].val, s2->slots[i].val))
      return 1;
  return 0;
}

static unsigned long
oostruct_hash(at *p)
{
  int i;
  unsigned long x = 0x87654321;
  struct oostruct *s1 = p->Object;
  x ^= hash_pointer((void*)p->Class);
  for(i=0;i<s1->size;i++)
  {
    x = (x<<1) | ((long)x<0 ? 1 : 0);
    x ^= hash_value(s1->slots[i].val);
  }
  return x;
}

at *
oostruct_getslot(at *obj, at *prop)
{
  int i;
  at *slot;
  struct oostruct *s = obj->Object;
  slot = prop->Car;
  ifn (slot && (slot->flags & X_SYMBOL))
    error(NIL,"Illegal slot name",slot);
  for(i=0; i<s->size; i++)
    if (slot == s->slots[i].symb)
      return getslot(s->slots[i].val, prop->Cdr);
  error(NIL,"Unknown slot name",slot);
  return NIL;
}


void
oostruct_setslot(at *obj, at *prop, at *val)
{
  int i;
  at *slot;
  struct oostruct *s = obj->Object;
  slot = prop->Car;
  ifn (slot && (slot->flags & X_SYMBOL))
    error(NIL,"Illegal slot name",slot);
  for(i=0; i<s->size; i++)
    if (slot == s->slots[i].symb)
    {
      setslot(&s->slots[i].val, prop->Cdr, val);
      return;
    }
  error(NIL,"Unknown slot name",slot);
}


class object_class =
{
  oostruct_dispose,
  oostruct_action,
  generic_name,
  generic_eval,
  generic_listeval,
  generic_serialize,
  oostruct_compare,
  oostruct_hash,
  oostruct_getslot,
  oostruct_setslot
};


class number_class =
{
  NIL,
  NIL,
  NIL,
  NIL,
  NIL,
};


class gptr_class =
{
  NIL,
  NIL,
  NIL,
  NIL,
  NIL,
};


/* ------ CLASS CLASS DEFINITION -------- */



static void 
clear_hashok(struct class *cl)
{
  cl->hashok = 0;
  cl = cl->subclasses;
  while (cl) {
    clear_hashok(cl);
    cl = cl->nextclass;
  }
}

static void
dispose_hashtable(class *s)
{
  int i;
  if (s->hashtable) {
    for (i=0; i<s->hashsize; i++) {
      UNLOCK(s->hashtable[i].symbol);
      UNLOCK(s->hashtable[i].function);
    }
    free(s->hashtable);
    s->hashtable = 0L;
    s->hashsize = 0;
    s->hashok = 0;
  }
}


static void 
class_dispose(at *q)
{
  register class *s, **p;

  s = q->Object;
  q->Object = NIL;
  /* Unlink subclass chain */
  if (s->super && s->atsuper->Object) 
  {
    p = &(s->super->subclasses);
    while (*p && (*p != s))
      p = &( (*p)->nextclass);
    if (*p == s)
      *p = s->nextclass;
  }
  /* Free class resources */
  UNLOCK(s->keylist);
  UNLOCK(s->defaults);
  UNLOCK(s->atsuper);
  UNLOCK(s->classname);
  UNLOCK(s->methods);
  dispose_hashtable(s);
  if (s->goaway)
    free(s);
}



static void 
class_action(at *q, void (*action) (at *))
{
  register class *s;
  int i;

  s = q->Object;
  (*action)(s->keylist);
  (*action)(s->defaults);
  (*action)(s->atsuper);
  (*action)(s->classname);
  (*action)(s->methods);
  if (s->hashtable) 
    for (i=0; i<s->hashsize; i++)
      if (s->hashtable[i].function) { 
	(*action)(s->hashtable[i].symbol);
	(*action)(s->hashtable[i].function);
      }
}


static char *
class_name(at *p)
{
  class *c;
  c = p->Object;

  if (c->classname)
    sprintf(string_buffer, "::class:%s", nameof(c->classname));
  else
    sprintf(string_buffer, "::class:%lx", (long)c);

  return string_buffer;
}



class class_class =
{
  class_dispose,
  class_action,
  class_name,
  generic_eval,
  generic_listeval,
};



/* ---------- CLASS ACCESSSES --------------- */


DX(xslots)
{
  at *q,*d, *ans;
  class *cl;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  ifn (q && (q->flags & C_EXTERN) && (q->Class == &class_class))
    error(NIL,"not a class",q);
  
  cl = q->Object;
  q = cl->keylist;
  d = cl->defaults;
  ans = NIL;
  while (CONSP(q) && (CONSP(d))) {
    LOCK(d->Car);
    LOCK(q->Car);
    if (d->Car)
    	ans = cons(cons(q->Car,cons(d->Car,NIL)), ans);
    else
    	ans = cons(q->Car, ans);
    q=q->Cdr;
    d=d->Cdr;
  }
  return ans;
}

DX(xmethods)
{
  at *q;
  class *cl;
  at *ans=NIL;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  ifn (q && (q->flags & C_EXTERN) && (q->Class == &class_class))
    error(NIL,"not a class",q);
  
  cl = q->Object;
  q = cl->methods;
  while (CONSP(q)) {
    if (CONSP(q->Car)) {
      LOCK(q->Car->Car);
      ans = cons(q->Car->Car,ans);
      q = q->Cdr;
    }
  }
  return ans;
}
  
DX(xsuper)
{
  at *q;
  class *cl;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  ifn (q && (q->flags & C_EXTERN) && (q->Class == &class_class))
    error(NIL,"not a class",q);
  
  cl = q->Object;
  LOCK(cl->atsuper);
  return cl->atsuper;
}

DX(xsubclasses)
{
  at *q;
  at *ans = NIL;
  class *cl;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  
  if (q == NIL) {
    cl = rootclasslist;
  } else if (EXTERNP(q, &class_class)) {
    cl = q->Object;
    cl = cl->subclasses;
  } else
    error(NIL,"not a class",q);
  
  while (cl) {
    q = cl->backptr;
    LOCK(q);
    ans = cons(q,ans);
    cl = cl->nextclass;
  }
  return ans;
}

DX(xclassname)
{
  at *q;
  class *cl;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  ifn (q && (q->flags & C_EXTERN) && (q->Class == &class_class))
    error(NIL,"not a class",q);
  
  cl = q->Object;
  LOCK(cl->classname);
  return cl->classname;
}



/* ------------- CLASS DEFINITION -------------- */

void 
class_define(char *name, class *cl)
{
  at *symb;
  at *classat;
  symb = new_symbol(name);
  classat = new_extern(&class_class,cl);
  cl->classname = symb;
  cl->backptr = classat;
  cl->goaway = 0;
  cl->dontdelete = 0;
  cl->slotssofar = 0;
  cl->super = 0L;
  cl->atsuper = NIL;
  cl->nextclass = rootclasslist;
  rootclasslist = cl;
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, classat);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
  }
  UNLOCK(classat);
}




at *
defclass(at *classname, at *superclass, at *keylist, at *defaults)
{
  static void clear_hashok(struct class *cl);

  class *cl,*super;
  at *p,*q;
  int i;
  
  ifn (classname && (classname->flags & X_SYMBOL))
    error(NIL,"not a symbol",classname);

  p = keylist;
  q = defaults;
  i = 0;
  while (CONSP(p) && CONSP(q)) {
    ifn (p->Car && (p->Car->flags & X_SYMBOL))
      error(NIL,"not a symbol",p->Car);
    p = p->Cdr;
    q = q->Cdr;
    i++;
  }
  if (p || q)
    error(NIL,"slot list and default list have different lengths",NIL);
  
  /* builds the new class */
  
  ifn (superclass && (superclass->flags & C_EXTERN)
       && (superclass->Class == &class_class))
    error(NIL,"not a class",superclass);
  super = superclass->Object;
  
  cl = malloc(sizeof(struct class));
  *cl = object_class;
  cl->slotssofar = super->slotssofar+i;
  cl->goaway = 1;
  cl->dontdelete = 0;
  
  /* Sets up classname */
  cl->classname = classname;
  LOCK(classname);
  
  /* Sets up lists */
  cl->super = super;
  cl->atsuper = superclass;
  LOCK(superclass);
  cl->nextclass = super->subclasses;
  cl->subclasses = 0L;
  super->subclasses = cl;
  
  /* Initialize the slots */
  cl->keylist = keylist;
  LOCK(keylist);
  cl->defaults = defaults;
  LOCK(defaults);

  /* Initialize the methods and hash table */
  cl->methods = NIL;
  cl->hashtable = 0L;
  cl->hashsize = 0;
  cl->hashok = 0;
  
  /* Create AT and returns it */
  UNLOCK(p);
  p = new_extern(&class_class,cl);
  cl->backptr = p;
  return p;
}


DX(xdefclass)
{
  at *cl,*p;
  at *keylist,*defaults;
  int i;

  if (arg_number < 2)
    ARG_NUMBER(-1);
  ARG_EVAL(2);

  keylist = NIL;
  defaults = NIL;

  for(i=3;i<=arg_number;i++) {
    p = APOINTER(i);
    if (CONSP(p) && CONSP(p->Cdr) && !p->Cdr->Cdr) {
      LOCK(p->Car);
      keylist = cons(p->Car,keylist);
      defaults = cons(eval(p->Cdr->Car),defaults);
    } else if (p && (p->flags & X_SYMBOL)) {
      LOCK(p);
      keylist = cons(p,keylist);
      defaults = cons(NIL,defaults);
    } else
      error(NIL,"symbol or (symbol value) expected",p);
  }
    
  cl = APOINTER(1);
  p = defclass(cl,APOINTER(2),keylist,defaults);
  var_set(cl,p);
  UNLOCK(p);
  LOCK(cl);
  UNLOCK(defaults);
  UNLOCK(keylist);
  return cl;
}


/* -------- OBJECT DEFINITION ----------- */



at *
new_oostruct(at *cl)
{
  class *c;
  class *sc;

  register int len,i;
  register at *q, *p;
  struct oostruct *s;

  ifn( cl && (cl->flags & C_EXTERN) && (cl->Class == &class_class))
    error(NIL,"not a class",cl);
  c = cl->Object;
  ifn( c->self_dispose == oostruct_dispose )
    error(NIL,"cannot 'new' on an intrinsinc class",cl);
  
  len = c->slotssofar;
  s = malloc(sizeof(struct oostruct)+(len-1)*sizeof(struct oostructitem));
  s->size = len;
  s->class = cl;
  LOCK(cl);
  
  i = 0;
  sc = c;
  while (sc) {
    p = sc->keylist;
    q = sc->defaults;
    while (CONSP(p) && CONSP(q)) {
      s->slots[i].symb = p->Car;
      s->slots[i].val = q->Car;
      LOCK(q->Car);
      p = p->Cdr;
      q = q->Cdr;
      i++;
    }
    sc = sc->super;
  }
  q = new_extern(c,s);
  q->flags |= X_OOSTRUCT;
  return q;
}

DY(ynew)
{
  at *q,*k,*f,*ans;
  class *cl;

  ifn ( CONSP(ARG_LIST) )
    error(NIL,"some argument expected",NIL);

  q = eval(ARG_LIST->Car);
  ans = new_oostruct(q);
  cl = q->Object;

  if ((f = getmethod(cl, cl->classname))) {
    k = eval_a_list(ARG_LIST->Cdr);
    f = letslot(ans,f,k,-1);
    UNLOCK(f);
    UNLOCK(k);
  } else if (ARG_LIST->Cdr)
    error(NIL,"no arg expected (no constructor!)",q);

  UNLOCK(q);
  return ans;
}



/* ---------- SCOPE OPERATOR -------------- */

DY(yscope)
{
  at *p = ARG_LIST;
  if (! CONSP(p))
    error(NIL,"arguments required",NIL);
  
  if (! p->Cdr) {
    /*
     * :globalvar
     */
    struct symbol *symb;
    if (! EXTERNP(p->Car, &symbol_class) )
      error(NIL,"Not a symbol",p->Car);
    symb = p->Car->Object;
    while(symb->next)
      symb = symb->next;
    if (symb->valueptr) 
    {
      LOCK(symb->value);
      return symb->value;
    } 
    return NIL;
  } 
  else 
  {
    /* 
     * :object:slot:slot:...:slot
     * (scope object slot slot .... slot)
     */
    at *obj = eval(p->Car);
    at *ans = getslot(obj, p->Cdr);
    UNLOCK(obj);
    return ans;
  }
}



at *
getslot(at *obj, at *prop)
{
  if (!prop)
  {
    LOCK(obj);
    return obj;
  }
  else
  {
    class *cl;
    ifn (obj && (obj->flags & C_EXTERN) 
	 && (cl=obj->Class) && (cl->getslot) )
      error(NIL,"This object does not accept scope syntax",obj);
    if (!LISTP(prop))
      error(NIL,"Illegal scope specification",prop);
    return (*cl->getslot)(obj, prop);
  }
}

void 
setslot(at **pobj, at *prop, at *val)
{
  if (!prop)
  {
    at *old = *pobj;
    LOCK(val);
    *pobj = val;
    UNLOCK(old);
  }
  else
  {
    class *cl;
    at *obj = *pobj;
    ifn (obj && (obj->flags & C_EXTERN) 
	 && (cl=obj->Class) && (cl->setslot) )
      error(NIL,"This object does not accept scope syntax",obj);
    if (!LISTP(prop))
      error(NIL,"Illegal scope specification",prop);
    (*cl->setslot)(obj, prop, val);    
  }
}





/* ---------- OBJECT CONTEXT -------------- */


at *
letslot(at *obj, at *f, at *q, int howmuch)
{
  register struct oostruct *s;
  register int i;
  struct symbol *symb;
  at *ans;
  
  ifn (obj) 
    {
      error(NIL,"Cannot send a message to the empty list",NIL);
    }
  else ifn (obj->flags & X_OOSTRUCT)
    {
      symbol_push(at_this,obj);
      ans = apply(f, q);
      symbol_pop(at_this);
    }
  else
    {
      s = obj->Object;
      if (howmuch>0)
	howmuch = s->size - howmuch;
      else
	howmuch = 0;
      
      /* Stack SLOTS */
      for(i=s->size-1; i>=howmuch; i--) {
	at *atsymb = s->slots[i].symb;
	symb = allocate(&symbol_alloc);
	symb->mode = SYMBOL_UNLOCKED;
	symb->next = (struct symbol *)(atsymb->Object);
	symb->name = symb->next->name;
	symb->value = NIL;
	symb->valueptr = &(s->slots[i].val);
	atsymb->Object = symb;
      }

      /* Stack THIS */
      symb = allocate(&symbol_alloc);
      symb->mode = SYMBOL_UNLOCKED;
      symb->next = (struct symbol *)(at_this->Object);
      symb->name = symb->next->name;
      symb->valueptr = &(symb->value);
      symb->value = obj;
      at_this->Object = symb;
      LOCK(obj);
      
      in_object_scope++;
      ans = apply(f,q);
      in_object_scope--;

      /* Unstack THIS */
      UNLOCK(symb->value);
      at_this->Object = symb->next;
      deallocate(&symbol_alloc, (struct empty_alloc *) symb);
  
      /* Unstack SLOTS */
      for(i=howmuch; i<s->size; i++) {
	at *atsymb = s->slots[i].symb;
	symb = (struct symbol *) (atsymb->Object);
	atsymb->Object = symb->next;
	deallocate(&symbol_alloc, (struct empty_alloc *) symb);
      }
    }
  return ans;
}

DY(yletslot)
{
  at *q,*p,*ans;

  ifn ( CONSP(ARG_LIST) )
    error(NIL,"some argument expected",NIL);

  q = eval(ARG_LIST->Car);
  p = eval(at_progn);
  ans = letslot(q, p, ARG_LIST->Cdr, -1);
  UNLOCK(q);
  UNLOCK(p);
  return ans;
}



/* ---------------- METHODS --------------- */


static void
putmethod(class *cl, at *name, at *value)
{
  register at **last, *list, *q;
  clear_hashok(cl);
  last = &(cl->methods);
  list = *last;
  while (CONSP(list)) {
    q = list->Car;
    if (! CONSP(q))
      error("oostruct/putmethod","Internal error",NIL);
    if (q->Car == name) 
      {
        if (value)
          {
            /* replace */
            LOCK(value);
            UNLOCK(q->Cdr);
            q->Cdr = value;
            return;
          }
        else
          {
            /* remove */
            *last = list->Cdr;
            list->Cdr = NIL;
            UNLOCK(list);
            return;
          }
      }
    last = &(list->Cdr);
    list = *last;
  }
  /* append */
  if (value)
    {
      LOCK(value);
      LOCK(name);
      UNLOCK(list);
      *last = cons( cons(name,value), NIL);
    }
}

DX(xputmethod)
{
  at *atclass, *atname, *atfun;
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  atclass = APOINTER(1);
  if (! EXTERNP(atclass, &class_class))
    error(NIL,"Not a class", atclass);
  atname = APOINTER(2);
  if (! EXTERNP(atname, &symbol_class))
    error(NIL,"Not a symbol", atname);
  atfun = APOINTER(3);
  if (atfun && !(atfun->flags & X_FUNCTION))
    error(NIL,"Not a function", atfun);
  putmethod(atclass->Object, atname, atfun);
  LOCK(atname);
  return atname;
}

static at *
getmethod(class *cl, at *prop)
{
  /* WARNING: RETURN UNLOCKED */        
  register at *list, *q;
  list = cl->methods;
  while (CONSP(list)) {
    q = list->Car;
    if (q->Car == prop) 
      return q->Cdr;
    list = list->Cdr;
  }
  return NIL;
}

#define HASH(q,size) ((unsigned long)(q) % (size-1) )

static void 
update_hashtable(class *cl)
{
  int nclass;
  int size;
  int hx;
  class *c;
  at *p, *q;
  at *prop, *value;

  nclass = 0;
  c = cl;
  while (c) {
    nclass += length(c->methods);
    c = c->super;
  }
  size = 16;
  while (size < nclass)
    size *= 2;

 restart:
  /* While we cannot store everything */
  size = 2*size;
  /* Grab a larger hashtable */
  dispose_hashtable(cl);
  ifn (cl->hashtable = malloc( size * sizeof(struct hashelem) ))
    error(NIL,"No memory",NIL);
  for (hx=0; hx<size; hx++) {
    cl->hashtable[hx].symbol = NIL;
    cl->hashtable[hx].function = NIL;
    cl->hashtable[hx].sofar = 0;
  }
  cl->hashsize = size;
  /* Grab the methods */
  c = cl;
  while (c) {
    p = c->methods;
    while (CONSP(p)) {
      prop = p->Car->Car;
      value = p->Car->Cdr;
      p = p->Cdr;
      hx = HASH(prop,size);
      if ((q = cl->hashtable[hx].symbol))
	if (q != prop) 
	  if ((q = cl->hashtable[++hx].symbol))
	    if (q != prop)
	      goto restart;
      ifn (q) {
	LOCK(prop);
	LOCK(value);
	cl->hashtable[hx].symbol = prop;
	cl->hashtable[hx].function = value;
	cl->hashtable[hx].sofar = c->slotssofar;
      }
    }
    c = c->super;
  }
  cl->hashok = 1;
}


static struct hashelem *
getmethod_quick(class *cl, at *prop)
{
/* WARNING: RETURN UNLOCKED */
  struct hashelem *hx;
  if (! cl->hashok)
    update_hashtable(cl);
  if (cl->hashtable) 
  {
    hx = cl->hashtable + HASH(prop, cl->hashsize);
    if (hx->symbol==prop)
      return hx;
    hx++;
    if (hx->symbol==prop)
      return hx;
  }
  return NIL;
}



/* WARNING: RETURN LOCKED */
at *
checksend(class *cl, at *prop)
{
  struct hashelem *hx;
  at *ans;
  if ((hx = getmethod_quick(cl,prop))) {
    ans = hx->function;
    LOCK(ans);
    return ans;
  } 
  return NIL;
}

DX(xchecksend)
{
  at *classat;
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  classat = APOINTER(1);
  ifn(classat && (classat->flags & C_EXTERN) 
      && (classat->Class == &class_class))
    error(NIL, "not a class", classat);
  return checksend(classat->Object,APOINTER(2));
}



/* ---------------- DELETE ---------------- */


static void
send_delete(at *p)
{
  register class *cl;
  register struct oostruct *s;
  register at *f;

  ifn (p && (p->flags & X_OOSTRUCT) )
    return;

  cl = p->Class;
  s = p->Object;

  while (cl) {
    if ((f = getmethod(cl,at_destroy))) {
      f = letslot(p,f,NIL,cl->slotssofar);
      UNLOCK(f);
    }
    cl = cl->super;
  }
}



void 
delete_at(at *p)
{
  if (!p || (p->flags & X_ZOMBIE) || (p->flags & C_GARBAGE)) {
    /* already deleted, or being deleted */
    return;
    
  } else if(p->flags & X_OOSTRUCT) {
    struct oostruct *s = p->Object;
    p->count++;
    p->flags |= C_GARBAGE;
    send_delete(p);
    p->count--;
    UNLOCK(s->class);
   
  } else if (p->flags & C_EXTERN) {
    if (p->Class->dontdelete)
      error(NIL,"Cannot delete this object",p);
    (*p->Class->self_dispose)(p);
    p->Object = NIL;
    
  } else if (p->flags & C_NUMBER) {
    p->Object = NIL;

  } else if (p->flags & C_CONS) {
    UNLOCK(p->Car);
    UNLOCK(p->Cdr);
    p->Object = NIL;
  }

  p->Class = &zombie_class;
  p->flags = C_EXTERN | X_ZOMBIE;
}


DX(xdelete)
{
  at *p;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  delete_at(p);
  return NIL;
}




/* ---------------- SEND ------------------ */



static at *
call_method(at *obj, struct hashelem *hx, at *args)
{
  at *p, *q;
  at *fun = hx->function;

  if (! (fun && fun->flags & X_FUNCTION))
    error(NIL,"Internal error", fun);
  
  if (fun->Class == &de_class)
    {
      // DE
      p = eval_a_list(args);
      q = letslot(obj, fun, p, hx->sofar);
      UNLOCK(p);
      return q;
    }
  else if (fun->Class == &df_class)
    {
      // DF
      return letslot(obj, fun, args, hx->sofar);
    }
  else if (fun->Class == &dm_class)
    {
      // DM
      p = cons(new_cons(fun, args), NIL);
      q = letslot(obj, at_mexpand, p, hx->sofar);
      UNLOCK(p);
      p = eval(q);
      UNLOCK(q);
      return p;
    }
  else
    {
      // DX, DY, DH
      LOCK(fun);
      p = cons(fun, new_cons(obj, args));
      q = (*fun->Class->list_eval)(fun, p);
      UNLOCK(p);
      return q;
    }
}


DY(ysend)
{
  register at *q, *symbol, *obj;
  struct hashelem *hx;
  class *cl;
  at *args, *ans;
  /* args */
  q = ARG_LIST;
  ifn (CONSP(q) && CONSP(q->Cdr))
    error(NIL, "arguments expected", NIL);
  obj = (*argeval_ptr)(q->Car);
  if ( obj && (obj->flags & C_EXTERN))
    cl = obj->Class;
  else if (obj && (obj->flags & C_NUMBER))
    cl = &number_class;
  else if (obj && (obj->flags & C_GPTR))
    cl = &gptr_class;
  else
    error(NIL,"can't send a message to",obj);
  /* supersend */
  symbol = q->Cdr->Car;
  if (CONSP(symbol)) 
    {
      ans = symbol->Car;
      symbol = symbol->Cdr;
      while (cl && cl->classname != ans)
        cl = cl->super;
      if (! cl)
        error(NIL,"cannot find superclass",ans);
    }
  /* send */
  if ((hx = getmethod_quick(cl,symbol))) 
    return call_method(obj, hx, q->Cdr->Cdr);
  /* send -unknown */
  if ((hx = getmethod_quick(cl,at_unknown))) 
    {
      LOCK(q->Cdr->Car);
      args = cons(q->Cdr->Car, new_cons(q->Cdr->Cdr, NIL));
      ans = call_method(obj, hx, args);
      UNLOCK(args);
      return ans;
    }
  /* fail */
  error(NIL,"unimplemented message",obj);
}


DX(xapplysend)
{
  register at *symbol, *obj, *args;
  struct hashelem *hx;
  class *cl;
  at *ans;

  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  
  obj = APOINTER(1);
  if ( obj && (obj->flags & C_EXTERN))
    cl = obj->Class;
  else if (obj && (obj->flags & C_NUMBER))
    cl = &number_class;
  else if (obj && (obj->flags & C_GPTR))
    cl = &gptr_class;
  else
    error(NIL,"can't send a message to",obj);
  
  symbol = APOINTER(2);
  if (CONSP(symbol)) {
    ans = symbol->Car;
    symbol = symbol->Cdr;
    while (cl && cl->classname != ans)
      cl = cl->super;
    ifn(cl)
      error(NIL,"cannot find superclass",ans);
  }
  
  args = APOINTER(3);
  /* Send */
  argeval_ptr = eval_nothing;
  if ((hx = getmethod_quick(cl,symbol))) 
    return call_method(obj, hx, args);
  /* Send -unknown */
  symbol = APOINTER(2);
  if ((hx = getmethod_quick(cl,at_unknown))) 
    {
      LOCK(symbol);
      args = cons(symbol, new_cons(args, NIL));
      ans = call_method(obj, hx, args);
      UNLOCK(args);
      return ans;
    }
  /* fail */
  error(NIL,"unimplemented message",obj);
}



DX(xsender)
{
  at *p;
  struct symbol *symb;
  ARG_NUMBER(0);
  symb = at_this->Object;
  ifn (symb->next)
    return NIL;
  p = *(symb->next->valueptr);
  LOCK(p);
  return p;
}









/* ---------------- MISC ------------------ */

/*
 * classof( p ) returns the class of the object P.
 */

at *
classof(at *p)
{
  ifn (p)
    return NIL;
  if (p->flags & C_NUMBER) {
    p = number_class.backptr;
    LOCK(p);
    return p;
  }
  if (p->flags & C_GPTR) {
    p = gptr_class.backptr;
    LOCK(p);
    return p;
  }
  if (p->flags & C_EXTERN) {
    p = p->Class->backptr;
    LOCK(p);
    return p;
  }
  return NIL;
}

DX(xclassof)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return classof(APOINTER(1));
}




int 
is_of_class(at *p, struct class *cl)
{
  struct class *c;

  ifn (p)
    return 0;
  else if (p->flags & C_NUMBER)
    c = &number_class;
  else if (p->flags & C_GPTR)
    c = &gptr_class;
  else if (p->flags & C_EXTERN)
    c = p->Class;
  else
    return 0;

  while (c && c != cl)
    c = c->super;
  if (c == cl)
    return 1;
  else
    return 0;
}

DX(xis_of_class)
{
  at *p, *q;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  
  p = APOINTER(1);
  q = APOINTER(2);
  ifn (q && (q->flags & C_EXTERN) && (q->Class == &class_class))
    error(NIL,"not a class",q);
  if (is_of_class(p,q->Object))
    return true();
  else
    return NIL;
}



/* --------- INITIALISATION CODE --------- */

void 
init_oostruct(void)
{

  class_define("object",&object_class );  
  class_define("ZOMBIE",&zombie_class );
  class_define("NUMBER",&number_class );
  class_define("GPTR",&gptr_class );
  class_define("class",&class_class );
  class_class.dontdelete = 1;

  dy_define("scope",yscope);
  dx_define("slots",xslots);
  dx_define("super",xsuper);
  dx_define("subclasses",xsubclasses);
  dx_define("methods",xmethods);
  dx_define("classname",xclassname);
  dx_define("classof",xclassof);
  dx_define("is_of_class",xis_of_class);
  dx_define("defclass",xdefclass);
  dy_define("new",ynew);
  dx_define("delete",xdelete);
  dy_define("letslot",yletslot);
  dx_define("check==>",xchecksend);
  dx_define("putmethod",xputmethod);
  dy_define("==>",ysend);
  dx_define("apply==>",xapplysend);
  dx_define("sender",xsender);
  
  at_this = var_define("this");
  at_progn = var_define("progn");
  at_destroy = var_define("-destructor");
  at_mexpand = var_define("macro-expand");
  at_unknown = var_define("-unknown");
}
