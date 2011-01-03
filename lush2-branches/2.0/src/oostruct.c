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
  object oriented structures
********************************************************************** */

#include "header.h"
#include "dh.h"

#define SIZEOF_OBJECT  (sizeof(object_t) + MIN_NUM_SLOTS*sizeof(at *))
#define SIZEOF_OBJECT2  (sizeof(object_t) + 2*MIN_NUM_SLOTS*sizeof(at *))

/* Notes on the object implementation:
 *
 * 1. An object of a compiled class or which has a compiled
 *    superclass has a "compiled part". The compiled part is
 *    the C-side representation of the object that is passed
 *    to compiled functions and methods.
 * 
 * 2. When the interpreter creates an OO object it also
 *    creates its compiled part (in new_object) and creates
 *    cref objects for all compiled slots. There is no copying
 *    of data for compiled slots, the data is in the compiled
 *    part and is being accessed from lisp code through the 
 *    cref objects.
 * 
 * 3. When OO objects are being created in compiled code, they
 *    are being created without the representation for the
 *    interpreter (new_cobject in dh.c). The part for the 
 *    interpreter is being created when necessary by the
 *    dh-interface.
 * 
 * 4. The OO objects used by the interpreter keep the compiled
 *    parts alive but not the other way around. Thus, the 
 *    finalizer for object_t objects must NULLify the C-side 
 *    __lptr.
 *
 * 5. Both the compiled representation and the interpreter
 *    representation need finalizers since C-side objects
 *    may exist without the interpreter counterpart. To ensure
 *    that compiled destructors are not called twice, compiled
 *    destructors NULLify the Vtbl-pointer when done.
 *
 * 6. Evaluating the destructor of an object may cause an
 *    an error. Calling the destructor code from a finalizer
 *    directly is therefore not an option (the error function
 *    longjumps and leaves cmm in undefined state). The finalizer
 *    for objects therefore puts unreachable objects in a queue
 *    to delay execution of the destructor until it is safe to
 *    do so (when no garbage collection is under way).
 */

static at *at_progn;
static at *at_mexpand;
static at *at_this;
static at *at_destroy;
static at *at_listeval;
static at *at_emptyp;
static at *at_unknown;
static at *at_pname;
static object_t *unreachables = NULL;

static struct hashelem *_getmethod(class_t *cl, at *prop);
static at *call_method(at *obj, struct hashelem *hx, at *args);


static void clear_object(void *obj, size_t n)
{
   memset(obj, 0, n);
}

static void mark_object(object_t *obj)
{
   MM_MARK(obj->backptr);
   MM_MARK(obj->cptr);
   MM_MARK(obj->next_unreachable);
   class_t *cl = Class(obj->backptr);
   MM_MARK(cl);
   for (int i = 0; i < cl->num_cslots; i++)
      MM_MARK(obj->slots[i]);

   for (int i = cl->num_cslots; i < cl->num_slots; i++) {
      at *p = obj->slots[i];
      if (HAS_BACKPTR_P(p))
         MM_MARK(Mptr(p))
      else
         MM_MARK(p);
   }

}

static object_t *oostruct_dispose(object_t *);
static bool finalize_object(object_t *obj)
{
   if (ZOMBIEP(obj->backptr))
      return true;
   
   /* Since destructor code may fail, we don't want cmm to call  */
   /* destructors directly (i.e., we can't call oostruct_dispose */
   /* here.) Instead we save unreachable objects and call their  */
   /* destructor later.                                          */
   obj->next_unreachable = unreachables;
   unreachables = obj;
   return false;
}

static mt_t mt_object = mt_undefined;
static mt_t mt_object2 = mt_undefined;

bool oostruct_idle(void)
{
   int n = 0;
   while (unreachables && n++<40) { 
      object_t *obj = unreachables;
      unreachables = obj->next_unreachable;
      object_class->dispose(obj);
   }
   return n>0;
}

static object_t *oostruct_dispose(object_t *obj)
{
   /* oostruct gets called only once */
   assert(!ZOMBIEP(obj->backptr));

   obj->next_unreachable = NULL;

   int oldready = error_doc.ready_to_an_error;
   error_doc.ready_to_an_error = true;
   struct lush_context mycontext;
   context_push(&mycontext);
   
   int errflag = sigsetjmp(context->error_jump,1);
   if (errflag==0) {
      /* call all destructors for interpreted part */
      /* destructors for compiled part are called  */
      /* by finalizer for compiled part            */
      at *f = NIL;
      class_t *cl = Class(obj->backptr);
      while (cl) {
         struct hashelem *hx = _getmethod(cl, at_destroy);
         cl = cl->super;
         if (! hx)
            break;
         else if (hx->function == f)
            continue;
         else if (classof(hx->function) == dh_class)
            break;
         call_method(obj->backptr, hx, NIL);
         f = hx->function;
      }
   }
   
   context_pop();
   error_doc.ready_to_an_error = oldready;
   if (obj->cptr)
      obj->cptr->__lptr = NULL;
   zombify(obj->backptr);
   
   if (errflag)
      siglongjmp(context->error_jump, -1L);

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
   class_t *cl1 = Class(obj1->backptr);
   class_t *cl2 = Class(obj2->backptr);
   
   if (cl1->num_slots != cl2->num_slots)
      return 1;
   
   /* check that slots names match before comparing values */
   for (int i=0; i<cl1->num_slots; i++)
      if (cl1->slots[i] != cl2->slots[i])
         return 1;

   for(int i=0; i<cl1->num_slots; i++) {
      if (!eq_test(obj1->slots[i], obj2->slots[i]))
         return 1;
   }
   return 0;
}

static unsigned long oostruct_hash(at *p)
{
   unsigned long x = 0x87654321;
   object_t *obj = Mptr(p);
   class_t *cl = Class(p);
   x ^= hash_pointer((void*)cl);
   for(int i=0; i<cl->num_slots; i++) {
      x = (x<<1) | ((long)x<0 ? 1 : 0);
      x ^= hash_value(obj->slots[i]);
   }
   return x;
}

at *oostruct_getslot(at *p, at *prop)
{
   at *slot = Car(prop);
   ifn (SYMBOLP(slot))
      error(NIL,"not a slot name", slot);
   prop = Cdr(prop);

   object_t *obj = Mptr(p);
   class_t *cl = Class(obj->backptr);
   for (int i=0; i<cl->num_slots; i++)
      if (slot == cl->slots[i]) {
         at *sloti = obj->slots[i];
         if (i<cl->num_cslots)
            sloti = eval(sloti);
         if (prop)
            return getslot(sloti, prop);
         else 
            return sloti;
      }
   error(NIL, "not a slot", slot);
}


void oostruct_setslot(at *p, at *prop, at *val)
{
   at *slot = Car(prop);
   ifn (SYMBOLP(slot))
      error(NIL, "not a slot name", slot);
   prop = Cdr(prop);

   object_t *obj = Mptr(p);
   class_t *cl = Class(obj->backptr);
   for(int i=0; i<cl->num_slots; i++)
      if (slot == cl->slots[i]) {
         if (prop)
            setslot(&obj->slots[i], prop, val);
         else if (i<cl->num_cslots) {
            class_t *cl = classof(obj->slots[i]);
            cl->setslot(obj->slots[i], NIL, val);
         } else
            obj->slots[i] = val;
         return;
      }
   error(NIL, "not a slot", slot);
}


/* ------ CLASS CLASS DEFINITION -------- */

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

void class_init(class_t *cl)
{
   /* initialize class functions */
   cl->dispose = generic_dispose;
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
   
   cl->slots = NIL;
   cl->defaults = NIL;
   cl->num_slots = 0;
   cl->num_cslots = 0;
   cl->myslots = NIL;
   cl->mydefaults = NIL;
   cl->methods = NIL;
   cl->hashtable = NULL;
   cl->hashsize = 0;
   cl->hashok = false;
   cl->dontdelete = false;
   cl->live = true;
   cl->managed = true;

   cl->has_compiled_part = false;
   cl->classdoc = NULL;
   cl->kname = NULL;
}

void clear_class(class_t *cl, size_t _)
{
   cl->slots = NULL;
   cl->defaults = NULL;
   cl->myslots = NULL;
   cl->mydefaults = NULL;
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
   MM_MARK(cl->slots);
   MM_MARK(cl->defaults);
   MM_MARK(cl->myslots);
   MM_MARK(cl->mydefaults);
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
   /* mark class and all subclasses as obsolete */
   zombify_subclasses(cl);
   cl->live = false;
   zombify(cl->backptr);
   return cl;
}

static const char *class_name(at *p)
{
   class_t *cl = Mptr(p);
   
   if (cl->classname)
      sprintf(string_buffer, "::class:%s", NAMEOF(cl->classname));
   else
      sprintf(string_buffer, "::class:%p", cl);
   
   if (!cl->live)
      strcat(string_buffer, " (unlinked)");

   return mm_strdup(string_buffer);
}



/* ---------- CLASS ACCESSSES --------------- */

DX(xallslots)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   return vector2list(cl->num_slots, cl->slots);
}

DX(xslots)
{
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);
   return cl->myslots;
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

/* mark dead all subclasses of class cl */
void zombify_subclasses(class_t *cl)
{
   class_t *subs = cl->subclasses;
   while (subs) {
      subs->live = false;
      zombify_subclasses(subs);
      subs = subs->nextclass;
   }
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

extern void dbg_notify(void *, void *);

/* allocate and initialize new builtin class */
class_t *new_builtin_class(class_t *super)
{
   class_t *cl = mm_alloc(mt_class);
   add_notifier(cl, dbg_notify, NULL);
   class_init(cl);
   if (super) {
      cl->super = super;
      cl->atsuper = super->backptr;
   }
   assert(class_class);
   cl->backptr = new_at(class_class, cl);
   return cl;
}

class_t *new_ooclass(at *classname, at *atsuper, at *new_slots, at *defaults)
{
   ifn (SYMBOLP(classname))
      error(NIL, "not a valid class name", classname);
   ifn (length(new_slots) == length(defaults))
      error(NIL, "number of slots must match number of defaults", NIL);
   ifn (CLASSP(atsuper))
      error(NIL, "not a class", atsuper);

   class_t *super = Mptr(atsuper);
   if (builtin_class_p(super))
      error(NIL, "superclass not descendend of class <object>", atsuper);

   at *p = new_slots;
   at *q = defaults;
   int n = 0;
   while (CONSP(p) && CONSP(q)) {
      ifn (SYMBOLP(Car(p)))
         error(NIL, "not a symbol", Car(p));
      for (int i=0; i<super->num_slots; i++)
         if (Car(p) == super->slots[i])
            error(NIL, "a superclass has slot of this name", Car(p));
      p = Cdr(p);
      q = Cdr(q);
      n++;
   }
   assert(!p && !q);
  
   /* builds the new class */
   class_t *cl = mm_alloc(mt_class);
   *cl = *object_class;
   if (new_slots) {
      int num_slots = n + super->num_slots;
      cl->slots = mm_allocv(mt_refs, num_slots*sizeof(at *));
      cl->defaults = mm_allocv(mt_refs, num_slots*sizeof(at *));
      int i = 0;
      for (; i<super->num_slots; i++) {
         cl->slots[i] = super->slots[i];
         cl->defaults[i] = super->defaults[i];
      }
      for (at *s = new_slots, *d = defaults; CONSP(s); i++, s = Cdr(s), d = Cdr(d)) {
         cl->slots[i] = Car(s);
         cl->defaults[i] = Car(d);
      }
      assert(i == num_slots);
      cl->num_slots = num_slots;
   } else {
      cl->slots = super->slots;
      cl->defaults = super->defaults;
      cl->num_slots = super->num_slots;
   }

   /* Sets up classname */
   cl->classname = classname;
   cl->priminame = classname;
   
   /* Sets up lists */
   cl->super = super;
   cl->atsuper = atsuper;
   cl->nextclass = super->subclasses;
   cl->subclasses = 0L;
   super->subclasses = cl;
  
   /* Initialize the slots */
   cl->myslots = new_slots;
   cl->mydefaults = defaults;
   
   /* Initialize the methods and hash table */
   cl->methods = NIL;
   cl->hashtable = 0L;
   cl->hashsize = 0;
   cl->hashok = 0;
   
   /* Initialize DHCLASS stuff */
   cl->has_compiled_part = super->has_compiled_part;
   cl->num_cslots = super->num_cslots;
   cl->classdoc = 0;
   cl->kname = 0;
   
   /* Create AT and returns it */
   assert(class_class);
   cl->backptr = new_at(class_class, cl);
   return cl;
}

DX(xmake_class)
{
   at *classname = NIL;
   at *superclass = NIL;
   at *keylist = NIL;
   at *defaults = NIL;
   
   switch (arg_number) {
   case 4:
      defaults = ALIST(4);
      keylist = ALIST(3);
   case 2:
      superclass = APOINTER(2);
      classname = APOINTER(1);
      break;
   default:
      ARG_NUMBER(4);
   }
   return new_ooclass(classname, superclass, keylist, defaults)->backptr;
}


/* -------- OBJECT DEFINITION ----------- */

static void make_cref_slots(object_t *obj)
{
   assert(obj->cptr);
   dhclassdoc_t *cdoc = obj->cptr->Vtbl->Cdoc;
   const class_t *cl = Mptr(cdoc->lispdata.atclass);

   /* initialize compiled slots */
   int k = cl->num_cslots;
   int j = cl->num_cslots;
   while (cl) {
      /* do slots declare in this class */
      dhrecord *drec = cl->classdoc->argdata;
      j -= drec->ndim;
      assert(j>=0);
      drec = drec + 1;
      for (int i=j; i<k; i++) {
         assert(drec->op == DHT_NAME);
         void *p = (char *)(obj->cptr)+(ptrdiff_t)(drec->arg);
         obj->slots[i] = new_cref((drec+1)->op, p);
         //assign(cl->slots[i], cl->defaults[i]);
         drec = drec->end;
      }
      assert(drec->op == DHT_END_CLASS);
      cl = cl->super;
      k = j;
   }
   assert(j==0);
}

static object_t *_new_object(class_t *cl, struct CClass_object *cobj)
{
   MM_ENTER;
   if (builtin_class_p(cl))
      error(NIL, "not a subclass of class 'object'", cl->backptr);
   
   size_t s = sizeof(object_t)+cl->num_slots*sizeof(at *);
   object_t *obj = NULL;
   if (s <= SIZEOF_OBJECT)
      obj = mm_alloc(mt_object);
   else
      obj = mm_allocv(mt_object2, s);

   if (cl->has_compiled_part) {
      if (cobj) {
         assert(cobj->Vtbl); /* if NULL, object has been destructed */
         obj->cptr = cobj;
         cobj->__lptr = obj;
         make_cref_slots(obj);

      } else {
         const class_t *c = cl;
         while (c && !c->classdoc) 
            c = c->super;
         assert(c);
         if (CONSP(c->priminame))
            check_primitive(c->priminame, c->classdoc);
         obj->cptr = new_cobject(c->classdoc);
         obj->cptr->__lptr = obj;
         make_cref_slots(obj);
      }
   }
   /* initialize non-compiled slots */
   for (int i=cl->num_cslots; i<cl->num_slots; i++)
      obj->slots[i] = cl->defaults[i];

   obj->backptr = new_at(cl, obj);

   MM_RETURN(obj);
}

object_t *new_object(class_t *cl)
{
   int n = 0;
   while (unreachables && n++<2) {
      object_t *obj = unreachables;
      unreachables = obj->next_unreachable;
      object_class->dispose(obj);
   }

   ifn (cl->live)
      error(NIL, "class is obsolete", cl->classname);
   
   return _new_object(cl, NULL);
}

object_t *object_from_cobject(struct CClass_object *cobj)
{
   ifn (cobj->Vtbl && cobj->Vtbl->Cdoc && cobj->Vtbl->Cdoc->lispdata.atclass)
      error(NIL, "attempt to access instance of unlinked class", NIL);
   assert(cobj->Vtbl);
   
   object_t *obj = cobj->__lptr;
   ifn (obj) {
      class_t *cl = Mptr(cobj->Vtbl->Cdoc->lispdata.atclass);
      obj = _new_object(cl, cobj);
   }
   return obj;
}

DY(ynew)
{
   ifn (CONSP(ARG_LIST))
      error(NIL, "some argument expected", NIL);
   
   at *q = eval(Car(ARG_LIST));
   ifn (CLASSP(q))
      RAISEFX("not a class", q);
   class_t *cl = Mptr(q);
   at *ans = NEW_OBJECT(cl);
   
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
   return NEW_OBJECT(cl);
}

/* ---------- OBJECT CONTEXT -------------- */

/* copied from symbol.c */

#define LOCK_SYMBOL(s)         SET_PTRBIT(s->hn, SYMBOL_LOCKED_BIT)
#define MARKVAR_SYMBOL(s)      SET_PTRBIT(s->hn, SYMBOL_VARIABLE_BIT)

at *with_object(at *p, at *f, at *q, int howfar)
{
   assert(howfar>=0);
   MM_ENTER;
   at *ans = NIL;
   if (OBJECTP(p)) {
      object_t *obj = Mptr(p);
      class_t *cl = Class(obj->backptr);
      if (howfar > cl->num_slots)
         howfar = cl->num_slots;
      
      /* push object environment */
      for (int i = 0; i<howfar; i++) {
         Symbol(cl->slots[i]) = symbol_push(Symbol(cl->slots[i]), 0 , &(obj->slots[i]));
         if (i < cl->num_cslots)
            MARKVAR_SYMBOL(Symbol(cl->slots[i]));
      }
      SYMBOL_PUSH(at_this, p);
      LOCK_SYMBOL(Symbol(at_this));

      ans = apply(f, q);
      
      /* pop object environment */
      SYMBOL_POP(at_this);
      for (int i = 0; i<howfar; i++)
         SYMBOL_POP(cl->slots[i]);
      
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

   at *l = ARG_LIST;
   at *p = eval(Car(l));
   int howfar = Class(p)->num_slots;
   
   if (CLASSP(p)) {
      l = Cdr(l);
      at *q = eval(Car(l));
      if (OBJECTP(q)) {
         const class_t *cl = Class(q);
         while (cl && cl != Mptr(p))
            cl = cl->super;
         if (! cl)
            RAISEFX("object not an instance of this superclass", p);
         howfar = cl->num_slots;
      }
      p = q;
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
            cl->hashtable[hx].sofar = c->num_slots;
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
   else if (method == at_pname) // special method?
      return NEW_STRING(cl->name(obj));

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
   
   if (!obj)
      fprintf(stderr, "*** Warning: sending %s to nil\n", pname(method));

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

void lush_delete(at *p)
{
   if (!p || ZOMBIEP(p))
      return;

   class_t *cl = classof(p);
   if (cl->dontdelete)
      error(NIL, "cannot delete this object", p);
   
   run_notifiers(p);
   
   if (cl->has_compiled_part) {
      assert(isa(p, object_class));
      /* OO objects may have two parts          */
      /* lush_delete has to delete both of them */
      object_t *obj = Mptr(p);
      struct CClass_object *cobj = obj->cptr;
      oostruct_dispose(obj);
      cobj->Vtbl->Cdestroy(cobj);
      
   } else {
      if (Class(p)->dispose)
         Mptr(p) = Class(p)->dispose(Mptr(p));
      else
         Mptr(p) = NULL;
   }
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

DY(yclassof)
{
   at *q = ARG_LIST;
   ifn (LASTCONSP(q))
      RAISEFX("syntax error", new_cons(NEW_SYMBOL("classof"), q));

   at *obj = Car(q);
   class_t *cl = SYMBOLP(obj) ? classof(Value(obj)) : classof(eval(obj));
   return cl->backptr;
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
   return NIL;
}

DX(xemptyp)
{
   ARG_NUMBER(1);
   at *obj = APOINTER(1);
   return obj ? send_message(NIL, obj, at_emptyp, NIL) : at_t;
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


void init_oostruct(void)
{
   mt_object = MM_REGTYPE("object", SIZEOF_OBJECT,
                          clear_object, mark_object, finalize_object);
   mt_object2 = MM_REGTYPE("object2", SIZEOF_OBJECT2,
                          clear_object, mark_object, finalize_object);
   mt_method_hash = 
      MM_REGTYPE("method-hash", 0, 
                 clear_method_hash, mark_method_hash, 0);

   pre_init_oostruct();
   MM_ROOT(unreachables);
   class_class->backptr = new_at(class_class, class_class);
   class_define("Class", class_class);

   /* 
    * mm_alloc object_class to avoid hickup in mark_class
    */
   object_class = new_builtin_class(NIL);
   object_class->dispose = (dispose_func_t *)oostruct_dispose;
   object_class->listeval = oostruct_listeval;
   object_class->compare = oostruct_compare;
   object_class->hash = oostruct_hash;
   object_class->getslot = oostruct_getslot;
   object_class->setslot = oostruct_setslot;
   class_define("object", object_class);  
   
   dx_define("allslots", xallslots);
   dx_define("slots",xslots);
   dx_define("super",xsuper);
   dx_define("subclasses",xsubclasses);
   dx_define("methods",xmethods);
   dx_define("classname",xclassname);
   dy_define("classof", yclassof);
   dx_define("isa",xisa);
   dx_define("emptyp",xemptyp);
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
   at_emptyp = var_define("-emptyp"); 
   at_mexpand = var_define("macroexpand");
   at_unknown = var_define("-unknown");
   at_pname = var_define("pname");
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
