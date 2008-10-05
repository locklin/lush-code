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
 * $Id: at.c,v 1.5 2002/11/06 16:30:49 leonb Exp $
 **********************************************************************/

/***********************************************************************
  Definition of the AT object
  General use routines as CONS, NUMBER, EXTERNAL, CAR, CDR etc...
********************************************************************** */

#include "header.h"
#include "mm.h"

static void clear_at(at *a)
{
   a->class = &null_class;
}

static void mark_at(at *a)
{
   if (CONSP(a)) {
      MM_MARK(Car(a));
      MM_MARK(Cdr(a));
      
   } else if (NUMBERP(a) || OBJECTP(a)) {
      mm_mark(Mptr(a));
        
   } else if (CLASSP(a)) {
      if (((class_t *)Gptr(a))->managed)
         mm_mark(Mptr(a));
      
   } else if (GPTRP(a) || RFILEP(a) || WFILEP(a) || ZOMBIEP(a)) {
      // nothing to mark

   } else if (Class(a)) {
      MM_MARK(Mptr(a));
   }
}

mt_t mt_at = mt_undefined;



LUSHAPI at *new_cons(at *car, at *cdr) 
{
   at *new = mm_alloc(mt_at);
   Car(new) = car;
   Cdr(new) = cdr;
   Class(new) = &cons_class;
   return new;
}

DX(xcons)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   return new_cons(APOINTER(1), APOINTER(2));
}


mt_t mt_double = mt_undefined;

at *new_number(double x)
{
   double *d = mm_alloc(mt_double);
   *d = x;
   return new_extern(&number_class, d);
}

at *new_gptr(gptr p)
{
   at *new = mm_alloc(mt_at);
   Gptr(new) = p;
   Class(new) = &gptr_class;
   return new;
}

/*
 * This strange function allows us to compute the 'simulated
 * bumping list' during a progn. This is used for the interpreted
 * 'in-pool' and 'in-stack' emulation.
 */

/* int compute_bump_active = 0; */
static at *compute_bump_list = 0;

/* DY(ycompute_bump) */
/* { */
/*    at *ans; */
/*    at *bump; */
/*    at *temp; */
/*    at *sav_compute_bump_list = compute_bump_list; */
/*    int sav_compute_bump_active = compute_bump_active; */
    
/*    /\* execute program in compute_bump mode *\/ */
/*    compute_bump_active = 1; */
/*    compute_bump_list = NIL; */
/*    at *ans = progn(ARG_LIST); */
/*    at *bump = compute_bump_list; */
/*    compute_bump_list = sav_compute_bump_list; */
/*    compute_bump_active = sav_compute_bump_active; */
    
/*    /\* remove objects with trivial count *\/ */
/*    int flag = 1; */
/*    while (flag) { */
/*       flag = 0; */
/*       at **where = &bump; */

/*       while (CONSP(*where)) { */
/*          if ((*where)->Car && (*where)->Car->count>1) { */
/*             where = &((*where)->Cdr); */
/*          } else { */
/*             flag = 1; */
/*             at* temp = *where; */
/*             *where = (*where)->Cdr; */
/*             temp->Cdr = NIL; */
/*          } */
/*       } */
/*    } */
/*    /\* return everything *\/ */
/*    return new_cons(ans, bump); */
/* } */


/*
 * new_extern(class,object) returns a LISP descriptor for an EXTERNAL object
 * of class class
 */

at *new_extern(class_t *cl, void *obj)
{
   at *new = mm_alloc(mt_at);
   Mptr(new) = obj;
   Class(new) = cl;
   
   /* This is used by 'compute_bump' */
/*    if (compute_bump_active) { */
/*       compute_bump_list = cons(new,compute_bump_list); */
/*    } */
   return new;
}


DX(xconsp)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   if (CONSP(APOINTER(1))) {
      return APOINTER(1);
   } else
      return NIL;
}


DX(xatomp)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   at *q = APOINTER(1);
   if (!q) {
      return t();
   } else ifn (CONSP(q)) {
      return q;
   } else
      return NIL;
}


static at *numberp(at *q)
{
   if (NUMBERP(q)) {
      return q;
   } else
      return NIL;
}

DX(xnumberp)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return numberp(APOINTER(1));
}


static at *null(at *q)
{
   ifn(q)
      return t();
   else
      return NIL;
}

DX(xnull)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return null(APOINTER(1));
}

void zombify(at *p)
{
   if (p)
      Class(p) = &null_class;
}

/* ----- unodes ------ */


static at *new_unode(at *doc)
{
   return new_cons(NIL,doc);
}

static at *unode_dive(at *p)
{
   at *q = p;
   while (CONSP(q) && Car(q))
      q = Car(q);
   ifn (CONSP(q))
      error(NIL,"Not a valid unode",p);
   return q;
}

static at *unode_val(at *p)
{
   p = unode_dive(p);
   p = Cdr(p);
   return p;
}

static at *unode_eq(at *p1, at *p2)
{
   at *q1, *q2;
   q1 = unode_dive(p1);
   q2 = unode_dive(p2);
   if (q1 != q2)
      return NIL;
   return q1;
}

static at *unode_unify(at *p1, at *p2, at *combine)
{
   at *q1 = unode_dive(p1);
   at *q2 = unode_dive(p2);
   if (q1 == q2)
      return NIL;

   at *doc;
   if (combine) {
      at *node = new_cons(Cdr(q1), new_cons(Cdr(q2),NIL));
      doc = apply(combine, node);
  } else
     doc = NIL;
   at *node = new_unode(doc);
   Car(q1) = node;
   Car(q2) = node;
   return node;
}

DX(xnew_unode)
{
   ALL_ARGS_EVAL;
   if (arg_number==0)
       return new_unode(NIL);

   ARG_NUMBER(1);
   return new_unode(APOINTER(1));
}

DX(xunode_uid)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return NEW_NUMBER((long)unode_dive(APOINTER(1)));
}

DX(xunode_val)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return unode_val(APOINTER(1));
}

DX(xunode_eq)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   return unode_eq(APOINTER(1),APOINTER(2));
}

DX(xunode_unify)
{
   ALL_ARGS_EVAL;
   at *doc;
   if (arg_number==2)
      doc = NIL;
   else {
      ARG_NUMBER(3);
      doc = APOINTER(3);
   }
   return unode_unify(APOINTER(1),APOINTER(2),doc);
}


/* ----- default class definition ------ */

char *generic_name(at *p)
{
   if (Class(p)->classname)
      sprintf(string_buffer, "::%s:%lx", 
              nameof(Class(p)->classname),(long)Mptr(p));
   else
      sprintf(string_buffer, "::%lx:%lx", 
              (long)Class(p), (long)Mptr(p));
   
   return string_buffer;
}

at *generic_selfeval(at *p)
{
   return p;
}

at *generic_listeval(at *p, at *q)
{
   /* looking for stacked functional values */
   at *pp = Car(q);			
   
   /* added stacked search on 15/7/88 */
   if (SYMBOLP(pp)) {
      symbol_t *s = Symbol(pp);
      s = s->next;
      while (s && s->valueptr) {
         pp = *(s->valueptr);
         if (pp && Class(pp)->listeval != generic_listeval) {
            if (eval_ptr == eval_debug) {
               print_tab(error_doc.debug_tab);
               print_string("  !! inefficient stacked call\n");
            }
            return Class(pp)->listeval(pp, q);
         }
         s = s->next;
      }
   }
   if (LISTP(p))
      error("eval", "not a function call", q);
   else
      error(pname(p), "can't evaluate this list", NIL);
}

static char *null_name(at *p)
{
   return "( )";
}

static at *null_selfeval(at *p)
{
   return NIL;
}

static at *null_listeval(at *p, at *q)
{
   error("eval", "not a function (null)", Car(q));
}

#define generic_dispose   NULL
#define generic_action    NULL
#define generic_serialize NULL
#define generic_compare   NULL
#define generic_hash      NULL
#define generic_getslot   NULL
#define generic_setslot   NULL

void class_init(class_t *cl, bool managed) {

   /* initialize class functions */
   cl->dispose = generic_dispose;
   cl->action = generic_action;
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
   cl->managed = managed;
   cl->dontdelete = false;
   cl->live = true;
   cl->classdoc = NULL;
   cl->kname = NULL;

   /* static classes may reference managed objects */
   if (!managed) {
      //MM_ROOT(cl->keylist);    /* NULL for static classes */
      //MM_ROOT(cl->defaults);   /* ! */
      //MM_ROOT(cl->super);      /* ! */
      //MM_ROOT(cl->subclasses); /* ! */
      //MM_ROOT(cl->nextclass);  /* ! */
      MM_ROOT(cl->classname);
      MM_ROOT(cl->priminame);
      MM_ROOT(cl->backptr);
      MM_ROOT(cl->methods);
      MM_ROOT(cl->hashtable);
      MM_ROOT(cl->kname);
   }
}

void clear_class(class_t *cl)
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
   class_class.dispose(cl);
   return true;
}

mt_t mt_class = mt_undefined;

/* --------- INITIALISATION CODE --------- */

class_t number_class, gptr_class, cons_class, null_class;

extern void pre_init_symbol(void);
extern void pre_init_function(void);
extern void pre_init_module(void);

void init_at(void)
{
   mt_at = MM_REGTYPE("at", sizeof(at),
                      clear_at, mark_at, 0);
   MM_ROOT(compute_bump_list);

   mt_class = MM_REGTYPE("class", sizeof(class_t),
                         clear_class, mark_class, 0);

   mt_double = MM_REGTYPE("double", sizeof(double), 0, 0, 0);

   /* bootstrapping the type registration */
   pre_init_symbol();
   pre_init_function();
   pre_init_module();

   /* set up builtin classes */
   class_init(&number_class, false);
   class_define("NUMBER", &number_class);
   
   class_init(&gptr_class, false);
   class_define("GPTR", &gptr_class);

   class_init(&cons_class, false);
   class_define("CONS", &cons_class);

   class_init(&null_class, false);
   null_class.name = null_name;
   null_class.selfeval = null_selfeval;
   null_class.listeval = null_listeval;
   class_define("NULL", &null_class);

  //dy_define("compute-bump", ycompute_bump);

   dx_define("cons", xcons);
   dx_define("consp", xconsp);
   dx_define("atomp", xatomp);
   dx_define("numberp", xnumberp);
   dx_define("null", xnull);
   dx_define("new-unode", xnew_unode);
   dx_define("unode-val", xunode_val);
   dx_define("unode-uid", xunode_uid);	
   dx_define("unode-eq", xunode_eq);
   dx_define("unode-unify", xunode_unify);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
