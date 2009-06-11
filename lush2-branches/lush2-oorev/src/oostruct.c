/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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
  object oriented structures
********************************************************************** */

#include "header.h"
#include "dh.h"

/* objects with less or equal MIN_NUM_SLOTS slots will
 * be allocated via mm_alloc.
 */
#define MIN_NUM_SLOTS  8

static at *at_progn;
static at *at_mexpand;
static at *at_this;
static at *at_destroy;
static at *at_listeval;
static at *at_unknown;

static struct hashelem *_getmethod(class_t *cl, at *prop);
static at *call_method(at *obj, struct hashelem *hx, at *args);
static void send_delete(object_t *);

void clear_object(object_t *obj, size_t _)
{
   obj->cl = NULL;
   obj->size = 0;
   obj->backptr = NULL;
}

void mark_object(object_t *obj)
{
   MM_MARK(obj->cl);
   MM_MARK(obj->backptr);
   for (int i = 0; i < obj->size; i++) {
      MM_MARK(obj->slots[i].symb);
      at *p = obj->slots[i].val;
      /* this is a hack until I figure how to do finalization right */
      if (HAS_BACKPTR_P(p))
         MM_MARK(Mptr(p))
      else
         MM_MARK(p);
   }
}

bool finalize_object(object_t *obj)
{
   if (obj->cl)
      object_class->dispose(obj);
   return true;
}

static mt_t mt_object = mt_undefined;

/* ------ OBJECT CLASS DEFINITION -------- */


object_t *oostruct_dispose(object_t *obj)
{
   if (!obj->cl)
      return obj;

   int oldready = error_doc.ready_to_an_error;
   error_doc.ready_to_an_error = true;

   struct context mycontext;
   context_push(&mycontext);
  
   int errflag = sigsetjmp(context->error_jump,1);
   /* when obj->cl is clear, destructor was already called */
   if (errflag==0 && obj->cl)
      send_delete(obj);
   
   context_pop();
   error_doc.ready_to_an_error = oldready;
   
   if (obj->cptr) {
      lside_destroy_item(obj->cptr);
      obj->cptr = NULL;
   }
   zombify(obj->backptr);
   obj->cl = NULL;
   if (errflag) {
      if (mm_collect_in_progress())
         /* we cannot longjump when called by a finalizer */
         fprintf(stderr, "*** Warning: error occurred in finalizer\n");
      else
         siglongjmp(context->error_jump, -1L);
   }
   return obj;
}

static at *oostruct_listeval(at *p, at *q)
{
   return send_message(NIL, p, at_listeval, Cdr(q));
}

static int oostruct_compare(at *p, at *q, int order)
{
   if (order)
      error(NIL, "cannot rank objects", NIL);

   object_t *obj1 = Mptr(p);
   object_t *obj2 = Mptr(q);
  
   for(int i=0; i<obj1->size; i++)
      if (!eq_test(obj1->slots[i].val, obj2->slots[i].val))
         return 1;
   return 0;
}

static unsigned long oostruct_hash(at *p)
{
   unsigned long x = 0x87654321;
   object_t *obj = Mptr(p);

   x ^= hash_pointer((void*)Class(p));
   for(int i=0; i<obj->size; i++) {
      x = (x<<1) | ((long)x<0 ? 1 : 0);
      x ^= hash_value(obj->slots[i].val);
   }
   return x;
}

at *oostruct_getslot(at *p, at *prop)
{
   at *slot = Car(prop);
   ifn (SYMBOLP(slot))
      error(NIL,"not a slot name", slot);

   object_t *obj = Mptr(p);
   for(int i=0; i<obj->size; i++)
      if (slot == obj->slots[i].symb)
         return getslot(obj->slots[i].val, Cdr(prop));
   
   error(NIL, "not a slot", slot);
   return NIL;
}


void oostruct_setslot(at *p, at *prop, at *val)
{
   at *slot = Car(prop);
   ifn (SYMBOLP(slot))
      error(NIL, "not a slot name", slot);

   object_t *obj = Mptr(p);
   for(int i=0; i<obj->size; i++)
      if (slot == obj->slots[i].symb) {
         setslot(&obj->slots[i].val, Cdr(prop), val);
         return;
      }
   error(NIL, "not a slot", slot);
}


/* ------ CLASS CLASS DEFINITION -------- */

static void generic_mark_at(at *a)
{
   MM_MARK(Mptr(a));
}

static const char *generic_name(at *p)
{
   if (Class(p)->classname)
      sprintf(string_buffer, "::%s:%p", NAMEOF(Class(p)->classname),Mptr(p));
   else
      sprintf(string_buffer, "::%p:%p", Class(p), Mptr(p));
   
   return mm_strdup(string_buffer);
}

static at *generic_selfeval(at *p)
{
   return p;
}

/* at *generic_listeval(at *p, at *q) */
/* { */
/*    /\* looking for stacked functional values *\/ */
/*    at *pp = Car(q);			 */
   
/*    /\* added stacked search on 15/7/88 *\/ */
/*    if (SYMBOLP(pp)) { */
/*       symbol_t *s = Symbol(pp); */
/*       s = s->next; */
/*       while (s && s->valueptr) { */
/*          pp = *(s->valueptr); */
/*          if (pp && Class(pp)->listeval != generic_listeval) { */
/*             if (eval_ptr == eval_debug) { */
/*                print_tab(error_doc.debug_tab); */
/*                print_string("  !! inefficient stacked call\n"); */
/*             } */
/*             return Class(pp)->listeval(pp, q); */
/*          } */
/*          s = s->next; */
/*       } */
/*    } */
/*    if (LISTP(p)) */
/*       error("eval", "not a function call", q); */
/*    else */
/*       error(pname(p), "can't evaluate this list", NIL); */
/* } */

static at *generic_listeval(at *p, at *q)
{
   if (LISTP(p))
      error("eval", "not a function call", q);
   else
      error(pname(p), "can't evaluate this list", NIL);
}
#define generic_dispose   NULL
#define generic_serialize NULL
#define generic_compare   NULL
#define generic_hash      NULL
#define generic_getslot   NULL
#define generic_setslot   NULL

void class_init(class_t *cl) {

   /* initialize class functions */
   cl->dispose = generic_dispose;
   cl->mark_at = generic_mark_at;
   cl->name = generic_name;
   cl->selfeval = generic_selfeval;
   cl->listeval = generic_listeval;
   cl->serialize = generic_serialize;
   cl->compare = generic_compare;
   cl->hash = generic_hash;
   cl->getslot = generic_getslot;
   cl->setslot = generic_setslot;

   /* initialize class data */
   cl->classname = NIL;
   cl->priminame = NIL;
   cl->backptr = NIL;
   cl->atsuper = NIL;
   cl->super = NULL;
   cl->subclasses = NULL;
   cl->nextclass = NULL;
   
   cl->slotssofar = 0;
   cl->keylist = NIL;
   cl->defaults = NIL;
   cl->methods = NIL;
   cl->hashtable = NULL;
   cl->hashsize = 0;
   cl->hashok = false;
   cl->dontdelete = false;
   cl->live = true;
   cl->classdoc = NULL;
   cl->kname = NULL;
}

void clear_class(class_t *cl, size_t _)
{
   cl->keylist = NULL;
   cl->defaults = NULL;
   cl->super = NULL;
   cl->subclasses = NULL;
   cl->nextclass = NULL;
   cl->classname = NULL;
   cl->priminame = NULL;
   cl->backptr = NULL;
   cl->methods = NULL;
   cl->hashtable = NULL;
   cl->kname = NULL;
}

void mark_class(class_t *cl)
{
   MM_MARK(cl->keylist);
   MM_MARK(cl->defaults);
   MM_MARK(cl->super);
   MM_MARK(cl->subclasses);
   MM_MARK(cl->nextclass);
   MM_MARK(cl->classname);
   MM_MARK(cl->priminame);
   MM_MARK(cl->backptr);
   MM_MARK(cl->methods);
   MM_MARK(cl->hashtable);
   MM_MARK(cl->kname);
}

bool finalize_class(class_t *cl)
{
   class_class->dispose(cl);
   return true;
}

static mt_t mt_class = mt_undefined;


static void clear_hashok(class_t *cl)
{
   cl->hashok = 0;
   cl = cl->subclasses;
   while (cl) {
      clear_hashok(cl);
      cl = cl->nextclass;
   }
}

static class_t *class_dispose(class_t *cl)
{
   /* remove atclass in dhclassdoc */
   if (cl->classdoc)
      cl->classdoc->lispdata.atclass = 0;
   
   /* unlink from subclass chain */
   if (cl->super) {
      class_t **p = &(cl->super->subclasses);
      while (*p && (*p != cl))
         p = &((*p)->nextclass);
      if (*p == cl)
         *p = cl->nextclass;
      cl->super = NULL;
   }
   zombify(cl->backptr);
   return NULL;
}

static const char *class_name(at *p)
{
   class_t *cl = Mptr(p);
   
   if (cl->classname)
      sprintf(string_buffer, "::class:%s", NAMEOF(cl->classname));
   else
      sprintf(string_buffer, "::class:%p", cl);
   
   if (!cl->live)
      strcat(string_buffer, " (obsolete)");

   return mm_strdup(string_buffer);
}



/* ---------- CLASS ACCESSSES --------------- */

DX(xslots)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   at *ks = cl->keylist;
   at *ds = cl->defaults;
   at *ans = NIL;
   while (CONSP(ks) && (CONSP(ds))) {
      if (Car(ds))
         ans = new_cons(
            new_cons(Car(ks), new_cons(Car(ds), NIL)), ans);
      else
         ans = new_cons(Car(ks), ans);
      ks = Cdr(ks);
      ds = Cdr(ds);
   }
   return ans;
}

DX(xmethods)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   at *mthds = NIL;
   at *q = cl->methods;
   while (CONSP(q)) {
      if (CONSP(Car(q))) 
         if (Cdar(q) && !ZOMBIEP(Cdar(q))) {
            mthds = new_cons(Caar(q), mthds);
         }
      q = Cdr(q);
   }
   return reverse(mthds);
}
  
DX(xsuper)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   return cl->atsuper;
}

DX(xsubclasses)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   cl = cl->subclasses;
   at *ans = NIL;
   while (cl) {
      ans = new_cons(cl->backptr, ans);
      cl = cl->nextclass;
   }
   return ans;
}

DX(xclassname)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   return cl->classname;
}



/* ------------- CLASS DEFINITION -------------- */

bool builtin_class_p(const class_t *cl) {
   return (cl->dispose != (dispose_func_t *)oostruct_dispose);
}

DX(xbuiltin_class_p) 
{
   ARG_NUMBER(1);
   if (builtin_class_p(ACLASS(1)))
      return t();
   else
      return NIL;
}

/* allocate and initialize new builtin class */
at *new_builtin_class(class_t **pcl, class_t *super)
{
   extern void dbg_notify(void *, void *);

   class_t *cl = mm_alloc(mt_class);
   add_notifier(cl, dbg_notify, NULL);
   class_init(cl);
   if (super) {
      cl->super = super;
      cl->atsuper = super->backptr;
   }
   assert(class_class);
   cl->backptr = new_at(class_class, cl);
   *pcl = cl;
   return cl->backptr;
}

at *new_ooclass(at *classname, at *superclass, at *keylist, at *defaults)
{
   ifn (SYMBOLP(classname))
      error(NIL, "not a valid class name", classname);
  
   at *p = keylist;
   at *q = defaults;
   int i = 0;
   while (CONSP(p) && CONSP(q)) {
      ifn (SYMBOLP(Car(p)))
         error(NIL, "not a symbol", Car(p));
      p = Cdr(p);
      q = Cdr(q);
      i++;
   }
   if (p || q)
      error(NIL, "slots and defaults not of same length", NIL);
  
   /* builds the new class */
   ifn (CLASSP(superclass))
      error(NIL, "not a class", superclass);
   class_t *super = Mptr(superclass);
   if (builtin_class_p(super))
      error(NIL, "superclass not descendend of class <object>", superclass);

   class_t *cl = mm_alloc(mt_class);
   *cl = *object_class;
   cl->slotssofar = super->slotssofar+i;
   
   /* Sets up classname */
   cl->classname = classname;
   cl->priminame = classname;
   
   /* Sets up lists */
   cl->super = super;
   cl->atsuper = superclass;
   cl->nextclass = super->subclasses;
   cl->subclasses = 0L;
   super->subclasses = cl;
  
   /* Initialize the slots */
   cl->keylist = keylist;
   cl->defaults = defaults;
   
   /* Initialize the methods and hash table */
   cl->methods = NIL;
   cl->hashtable = 0L;
   cl->hashsize = 0;
   cl->hashok = 0;
   
   /* Initialize DHCLASS stuff */
   cl->classdoc = 0;
   cl->kname = 0;
   
   /* Create AT and returns it */
   assert(class_class);
   cl->backptr = new_at(class_class, cl);
   return cl->backptr;
}

DX(xmake_class)
{
   at *classname = NIL;
   at *superclass = NIL;
   at *keylist = NIL;
   at *defaults = NIL;
   
   switch (arg_number) {
   case 4:
      defaults = APOINTER(4);
      keylist = APOINTER(3);
   case 2:
      superclass = APOINTER(2);
      classname = APOINTER(1);
      break;
   default:
      ARG_NUMBER(4);
   }
   return new_ooclass(classname, superclass, keylist, defaults);
}


/* -------- OBJECT DEFINITION ----------- */

at *new_object(class_t *cl)
{
   if (builtin_class_p(cl))
      error(NIL, "not a subclass of ::class:object", cl->backptr);
   
   int len = cl->slotssofar;
   size_t s = sizeof(object_t)+len*sizeof(struct oostructitem);
   object_t *obj = (len <= MIN_NUM_SLOTS) ?
      mm_alloc(mt_object) : mm_allocv(mt_object, s);
   if (!obj)
      error(NIL, "not enough memory", NIL);
   obj->cl = cl;
   obj->size = len;
   obj->cptr = NULL;
   
   /* initialize slots */
   int i = 0;
   const class_t *super = cl;
   while (super) {
      at *p = super->keylist;
      at *q = super->defaults;
      while (CONSP(p) && CONSP(q)) {
         obj->slots[i].symb = Car(p);
         obj->slots[i].val = Car(q);
         p = Cdr(p);
         q = Cdr(q);
         i++;
      }
      super = super->super;
   }
   obj->backptr = new_at(cl, obj);
   return obj->backptr;
}

DY(ynew)
{
   ifn (CONSP(ARG_LIST))
      error(NIL, "some argument expected", NIL);
   
   at *q = eval(Car(ARG_LIST));
   ifn (CLASSP(q))
      RAISEFX("not a class", q);
   class_t *cl = Mptr(q);
   at *ans = new_object(cl);
   
   struct hashelem *hx = _getmethod(cl, cl->classname);
   if (hx)
      call_method(ans, hx, Cdr(ARG_LIST));
   else if (Cdr(ARG_LIST))
      RAISEF("constructor expects no arguments", NIL);
   
   return ans;
}

DX(xnew_empty)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   return new_object(cl);
}

/* ---------- SCOPE OPERATOR -------------- */

at *getslot(at *obj, at *prop)
{
   if (!prop) {
      return obj;
      
   } else {
      const class_t *cl = obj ? Class(obj) : NULL;
      ifn (cl && cl->getslot)
         error(NIL, "object does not accept scope syntax", obj);
      ifn (LISTP(prop))
         error(NIL, "illegal scope specification", prop);
      return (*cl->getslot)(obj, prop);
   }
}

void setslot(at **pobj, at *prop, at *val)
{
   if (!prop) {
      *pobj = val;

   } else {
      at *obj = *pobj;
      const class_t *cl = obj ? Class(obj) : NULL;
      ifn (cl && cl->setslot)
         error(NIL, "object does not accept scope syntax", obj);
      ifn (LISTP(prop))
         error(NIL, "illegal scope specification", prop);
      (*cl->setslot)(obj, prop, val);    
   }
}





/* ---------- OBJECT CONTEXT -------------- */


at *with_object(at *p, at *f, at *q, int howfar)
{
   MM_ENTER;

   at *ans = NIL;
   if (OBJECTP(p)) {
      object_t *obj = Mptr(p);
     
      if (howfar > 0)
         howfar = obj->size - howfar;
      else
         howfar = 0;

      /* push object environment */
      for (int i = obj->size-1; i >= howfar; i--)
         Symbol(obj->slots[i].symb) = symbol_push(Symbol(obj->slots[i].symb), 0 , &(obj->slots[i].val));
      SYMBOL_PUSH(at_this, p);

      ans = apply(f, q);
      
      /* pop object environment */
      SYMBOL_POP(at_this);
      for(int i = howfar; i < obj->size; i++)
         SYMBOL_POP(obj->slots[i].symb);
      
   } else {
      if (p == NIL)
         printf("*** Warning\007 (with-object () ...)\n");
      SYMBOL_PUSH(at_this, p);
      ans = apply(f, q);
      SYMBOL_POP(at_this);
   }
   
   MM_RETURN(ans);
}

DY(ywith_object)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("no arguments", NIL);

   int howfar = -1;
   at *l = ARG_LIST;
   at *p = eval(Car(l));
   
   if (CLASSP(p)) {
      l = Cdr(l);
      at *q = eval(Car(l));
      if (OBJECTP(q)) {
         const class_t *cl = Class(q);
         while (cl && cl != Mptr(p))
            cl = cl->super;
         if (! cl)
            RAISEFX("object not an instance of this superclass", p);
         howfar = cl->slotssofar;
      }
   }
   //at *q = eval(at_progn);
   at *q = symbol_class->selfeval(at_progn);
   return with_object(p, q, Cdr(l), howfar);
}


/* ---------------- METHODS --------------- */


void putmethod(class_t *cl, at *name, at *value)
{
   ifn (SYMBOLP(name))
      RAISEF("not a symbol", name);
   if (value && !FUNCTIONP(value))
      RAISEF("not a function", value);

   clear_hashok(cl);
   at **last = &(cl->methods);
   at *list = *last;
   while (CONSP(list)) {
      at *q = Car(list);
      ifn (CONSP(q))
         RAISEF("not a pair", q);
      if (Car(q) == name) {
         if (value) {
            /* replace */
            Cdr(q) = value;
            return;
         } else {
            /* remove */
            *last = Cdr(list);
            Cdr(list) = NIL;
            return;
         }
      }
      last = &Cdr(list);
      list = *last;
   }
   /* not an existing method, append */
   if (value)
      *last = new_cons(new_cons(name, value), NIL);
}

DX(xputmethod)
{
   ARG_NUMBER(3);
   putmethod(ACLASS(1), APOINTER(2), APOINTER(3));
   return APOINTER(2);
}

void clear_method_hash(struct hashelem *ht, size_t s)
{
   memset(ht, 0, s);
}

void mark_method_hash(struct hashelem *ht)
{
   int n = mm_sizeof(ht)/sizeof(struct hashelem);
   for (int i = 0; i < n; i++) {
      MM_MARK(ht[i].symbol);
      MM_MARK(ht[i].function);
   }
}

mt_t mt_method_hash = mt_undefined;


#define HASH(q,size) ((unsigned long)(q) % (size-3))

static void update_hashtable(class_t *cl)
{
   int nclass = 0;
   class_t *c = cl;
   while (c) {
      nclass += length(c->methods);
      c = c->super;
   }
   int size = 16;
   while (size < nclass)
      size *= 2;
   
restart:
   size *= 2;
   
   /* Make larger hashtable */
   size_t s = size * sizeof(struct hashelem);
   cl->hashtable = mm_allocv(mt_method_hash, s);
   
   /* Grab the methods */
   at **pp, *p;
   c = cl;
   while (c) {
      pp = &c->methods;
      while ((p = *pp) && CONSP(p)) {
         at *prop = Caar(p);
         at *value = Cdar(p);
         if (!value || ZOMBIEP(value)) {
            *pp = Cdr(p);
            Cdr(p) = NIL;
            continue;
         }
         pp = &Cdr(p);
         int hx = HASH(prop,size);
         at *q = cl->hashtable[hx].symbol;
         if (q)
            if (q != prop) 
               if ((q = cl->hashtable[++hx].symbol))
                  if (q != prop)
                     if ((q = cl->hashtable[++hx].symbol))
                        if (q != prop)
                           goto restart;
         if (! q) {
            cl->hashtable[hx].symbol = prop;
            cl->hashtable[hx].function = value;
            cl->hashtable[hx].sofar = c->slotssofar;
         }
      }
      c = c->super;
   }
   cl->hashsize = size;
   cl->hashok = 1;
}

static struct hashelem *_getmethod(class_t *cl, at *prop)
{
   if (!cl->hashok)
      update_hashtable(cl);

   if (!cl->hashtable) 
      return NIL;
   
   struct hashelem *hx = cl->hashtable + HASH(prop, cl->hashsize);
   if (hx->symbol != prop)
      if ((++hx)->symbol != prop)
         if ((++hx)->symbol != prop)
            return NIL;
   
   if (ZOMBIEP(hx->function)) {
      clear_hashok(cl);
      return _getmethod(cl, prop);
   }
   return hx;
}

at *getmethod(class_t *cl, at *prop)
{
   struct hashelem *hx = _getmethod(cl, prop);
   return hx ? hx->function : NIL;
}

DX(xgetmethod)
{
   ARG_NUMBER(2);
   return getmethod(ACLASS(1), APOINTER(2));
}



/* ---------------- SEND ------------------ */



static at *call_method(at *obj, struct hashelem *hx, at *args)
{
   at *fun = hx->function;
   assert(FUNCTIONP(fun));
   
   if (Class(fun) == de_class) {
      // DE
      at *p = eval_arglist(args);
      return with_object(obj, fun, p, hx->sofar);

   } else if (Class(fun) == df_class) {
      // DF
      return with_object(obj, fun, args, hx->sofar);

   } else if (Class(fun) == dm_class) {
      // DM
      at *p = new_cons(new_cons(fun, args), NIL);
      at *q = with_object(obj, at_mexpand, p, hx->sofar);
      return eval(q);
      
   } else {
      // DX, DY, DH
      at *p = new_cons(fun, new_cons(obj, args));
      return Class(fun)->listeval(fun, p);
   }
}

at *send_message(at *classname, at *obj, at *method, at *args)
{
   class_t *cl = classof(obj);

   /* find superclass */
   if (classname) {
      ifn (SYMBOLP(classname))
         error(NIL, "not a class name", classname);
      while (cl && cl->classname != classname)
         cl = cl->super;
      ifn (cl)
         error(NIL, "cannot find class", classname);
   }
   /* send */
   ifn (SYMBOLP(method))
      error(NIL, "not a method name", method);
   struct hashelem *hx = _getmethod(cl, method);
   if (hx) 
      return call_method(obj, hx, args);
   
   /* send -unknown */
   hx = _getmethod(cl, at_unknown);
   if (hx) {
      at *arg = new_cons(method, new_cons(args, NIL));
      return call_method(obj, hx, arg);
   }
   /* fail */
   error(NIL, "method not found", method);
}

DY(ysend)
{
   /* Get arguments */
   at *q = ARG_LIST;
   ifn (CONSP(q) && CONSP(Cdr(q)))
      RAISEFX("arguments expected", NIL);
   at *obj = eval(Car(q));
   at *method = eval(Cadr(q));
   at *args = Cddr(q);
   
   /* Send */
   return (CONSP(method) ? 
           send_message(Car(method), obj, Cdr(method), args) :
           send_message(NIL, obj, method, args));
}

DX(xsender)
{
   ARG_NUMBER(0);
   
   symbol_t *symb = Symbol(at_this);
   ifn (symb->next)
      return NIL;
   return *(symb->next->valueptr);
}


/* ---------------- DELETE ---------------- */

static void send_delete(object_t *obj)
{
   /* Only send destructor to objects owned by lisp */
   ifn (lisp_owns_p(obj->cptr))
      return;
   
   at *f = NIL;
   class_t *cl = obj->cl;
   /* Call all destructors defined in class hierarchy */
   while (cl) {
      struct hashelem *hx = _getmethod(cl, at_destroy);
      cl = cl->super;
      if (! hx)
         break;
      else if (hx->function == f)
         continue;
      call_method(obj->backptr, hx, NIL);
      f = hx->function;
   }
}


void lush_delete(at *p)
{
   if (!p || ZOMBIEP(p))
      return;

   if (Class(p)->dontdelete)
      error(NIL, "cannot delete this object", p);
   
   run_notifiers(p);

   if (Class(p)->dispose)
      Mptr(p) = Class(p)->dispose(Mptr(p));
   else
      Mptr(p) = NULL;
   
   zombify(p);
}

/* similar to lush_delete, but don't raise error with permanent objects */
void lush_delete_maybe(at *p)
{
   if (p) {
      if (!Class(p)->dontdelete)
         lush_delete(p);
   }
}

DX(xdelete)
{
   ARG_NUMBER(1);
   lush_delete(APOINTER(1));
   return NIL;
}



/* ---------------- MISC ------------------ */

/*
 * classof( p ) returns the class of the object P.
 */


class_t *classof(at *p)
{
   return p ? Class(p) : null_class;
}

DX(xclassof)
{
   ARG_NUMBER(1);
   return classof(APOINTER(1))->backptr;
}

bool isa(at *p, const class_t *cl)
{
   class_t *c = classof(p);
   while (c && c != cl)
      c = c->super;
   return c == cl;
}

DX(xisa)
{
   ARG_NUMBER(2);
   at *p = APOINTER(1);
   at *q = APOINTER(2);
   ifn (CLASSP(q))
      RAISEFX("not a class", q);
   if (isa(p, Mptr(q)))
      return t();

#ifdef DHCLASSDOC
   if (GPTRP(p)) {
      /* Handle gptrs by calling to-obj behind the scenes. Ugly. */
      int flag = 0;
      at *sym = named("to-obj");
      at *fun = find_primitive(0, sym);
      if (fun && (Class(fun) == dx_class)) {
         at *arg = new_cons(p,NIL);
         sym = apply(fun, arg);
         flag = isa(sym, Mptr(q));
      }
      if (flag)
         return t();
   }
#endif
   return NIL;
}


void pre_init_oostruct(void)
{
   if (mt_class == mt_undefined)
      mt_class = MM_REGTYPE("class", sizeof(class_t),
                            clear_class, mark_class, 0);
   if (!class_class) {
      class_class = mm_alloc(mt_class);
      class_init(class_class);
      class_class->dispose = (dispose_func_t *)class_dispose;
      class_class->name = class_name;
      class_class->dontdelete = true;
   }
}

/* --------- INITIALISATION CODE --------- */

class_t *object_class, *class_class = NULL;

#define SIZEOF_OBJECT  (sizeof(object_t) + MIN_NUM_SLOTS*sizeof(struct oostructitem))

void init_oostruct(void)
{
   mt_object = MM_REGTYPE("object", SIZEOF_OBJECT,
                          clear_object, mark_object, finalize_object);
   mt_method_hash = 
      MM_REGTYPE("method-hash", 0, 
                 clear_method_hash, mark_method_hash, 0);

   pre_init_oostruct();
   class_class->backptr = new_at(class_class, class_class);
   class_define("class", class_class);

   /* 
    * mm_alloc object_class to avoid hickup in mark_class
    */
   new_builtin_class(&object_class, NIL);
   object_class->dispose = (dispose_func_t *)oostruct_dispose;
   object_class->listeval = oostruct_listeval;
   object_class->compare = oostruct_compare;
   object_class->hash = oostruct_hash;
   object_class->getslot = oostruct_getslot;
   object_class->setslot = oostruct_setslot;
   class_define("object", object_class);  
   
   dx_define("slots",xslots);
   dx_define("super",xsuper);
   dx_define("subclasses",xsubclasses);
   dx_define("methods",xmethods);
   dx_define("classname",xclassname);
   dx_define("classof",xclassof);
   dx_define("isa",xisa);
   dx_define("make-class",xmake_class);
   dx_define("builtin-class-p",xbuiltin_class_p);
   dy_define("new",ynew);
   dx_define("new-empty",xnew_empty);
   dx_define("delete",xdelete);
   dy_define("with-object",ywith_object);
   dx_define("getmethod",xgetmethod);
   dx_define("putmethod",xputmethod);
   dy_define("send",ysend);
   dx_define("sender",xsender);
   
   at_this = var_define("this");
   at_progn = var_define("progn");
   at_destroy = var_define("-destructor");
   at_listeval = var_define("-listeval");
   at_mexpand = var_define("macroexpand");
   at_unknown = var_define("-unknown");
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
