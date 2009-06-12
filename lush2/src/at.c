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
  Definition of the AT object
  General use routines as CONS, NUMBER, EXTERNAL, CAR, CDR etc...
********************************************************************** */

#include "header.h"
#include <inttypes.h>

static void clear_at(at *a, size_t _)
{
   a->head.cl = null_class;
}

static void mark_at(at *a)
{
   Class(a)->mark_at(a);

/*    if (CONSP(a)) { */
/*       MM_MARK(Car(a)); */
/*       MM_MARK(Cdr(a)); */
      
/*    } else if (SYMBOLP(a) || NUMBERP(a) || OBJECTP(a)) { */
/*       MM_MARK(Mptr(a)); */
        
/*    } else if (CLASSP(a)) { */
/*       if (((class_t *)Gptr(a))->managed) */
/*          MM_MARK(Mptr(a)); */
      
/*    } else if (GPTRP(a) || RFILEP(a) || WFILEP(a) || ZOMBIEP(a)) { */
/*       // nothing to mark */

/*    } else  */
/*       MM_MARK(Mptr(a)); */

}

mt_t mt_at = mt_undefined;


at *new_at(class_t *cl, void *obj)
{
   at *new = mm_alloc(mt_at);
   Mptr(new) = obj;
   AssignClass(new, cl);

   /* This is used by 'compute_bump' */
/*    if (compute_bump_active) { */
/*       compute_bump_list = cons(new,compute_bump_list); */
/*    } */
   return new;
}


at *new_cons(at *car, at *cdr) 
{
   at *new = mm_alloc(mt_at);
   AssignCar(new, car);
   Cdr(new) = cdr;
   return new;
}

DX(xcons)
{
    ARG_NUMBER(2);
    return new_cons(APOINTER(1), APOINTER(2));
 }


at *new_number(double x)
{
   double *d = mm_alloc(mt_blob8);
   *d = x;
   return new_at(number_class, d);
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

DX(xconsp)
{
   ARG_NUMBER(1);
   if (CONSP(APOINTER(1))) {
      return APOINTER(1);
   } else
      return NIL;
}


DX(xatom)
{
   ARG_NUMBER(1);
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
   return null(APOINTER(1));
}

void zombify(at *p)
{
   if (p)
      AssignClass(p, null_class);
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

   at *doc = NIL;
   if (combine) {
      at *node = new_cons(Cdr(q1), new_cons(Cdr(q2), NIL));
      doc = apply(combine, node);
   } 
   at *node = new_unode(doc);
   AssignCar(q1, node);
   AssignCar(q2, node);
   return node;
}

DX(xnew_unode)
{
   if (arg_number==0)
       return new_unode(NIL);

   ARG_NUMBER(1);
   return new_unode(APOINTER(1));
}

DX(xunode_uid)
{
   ARG_NUMBER(1);
   return NEW_NUMBER((long)unode_dive(APOINTER(1)));
}

DX(xunode_val)
{
   ARG_NUMBER(1);
   return unode_val(APOINTER(1));
}

DX(xunode_eq)
{
   ARG_NUMBER(2);
   return unode_eq(APOINTER(1),APOINTER(2));
}

DX(xunode_unify)
{
   at *doc;
   if (arg_number==2)
      doc = NIL;
   else {
      ARG_NUMBER(3);
      doc = APOINTER(3);
   }
   return unode_unify(APOINTER(1),APOINTER(2),doc);
}

/* -------- definitions for this module's classes -------- */

static const char *gptr_name(at *p)
{
   sprintf(string_buffer, "#$%p", Gptr(p));
   return mm_strdup(string_buffer);
}

static void cons_mark_at(at *a)
{
   MM_MARK(Car(a));
   MM_MARK(Cdr(a));
}

static void null_mark_at(at *a)
{
   return;
}

static const char *null_name(at *p)
{
   return mm_strdup("( )");
}

static at *null_selfeval(at *p)
{
   return NIL;
}

static at *null_listeval(at *p, at *q)
{
   error(NIL, "not a function (nil)", Car(q));
   return NIL;
}


/* --------- INITIALISATION CODE --------- */

class_t *number_class, *gptr_class, *cons_class, *null_class;

extern void pre_init_symbol(void);
extern void pre_init_function(void);
extern void pre_init_module(void);
extern void pre_init_oostruct(void);

void init_at(void)
{
   assert(sizeof(at)==2*sizeof(void *));
   assert(sizeof(double) <= 8);

   mt_at = MM_REGTYPE("at", sizeof(at),
                      clear_at, mark_at, 0);
   MM_ROOT(compute_bump_list);

   /* bootstrapping the type registration */
   pre_init_oostruct();
   pre_init_symbol();
   pre_init_function();
   pre_init_module();

   /* set up builtin classes */
   new_builtin_class(&number_class, NIL);
   class_define("NUMBER", number_class);

   new_builtin_class(&gptr_class, NIL);
   gptr_class->name = gptr_name;
   gptr_class->mark_at = null_mark_at;
   class_define("GPTR", gptr_class);

   new_builtin_class(&cons_class, NIL);
   cons_class->mark_at = cons_mark_at;
   class_define("CONS", cons_class);
   
   new_builtin_class(&null_class, NIL);
   null_class->mark_at = null_mark_at;
   null_class->name = null_name;
   null_class->selfeval = null_selfeval;
   null_class->listeval = null_listeval;
   class_define("NULL", null_class);

  //dy_define("compute-bump", ycompute_bump);

   dx_define("cons", xcons);
   dx_define("consp", xconsp);
   dx_define("atom", xatom);
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
