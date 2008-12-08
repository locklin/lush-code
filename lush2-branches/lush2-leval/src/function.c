/***********************************************************************
 * 
 *  PSU Lush
 *    Copyright (C) 2005 Ralf Juengling.
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
 * $Id: function.c,v 1.16 2004/07/20 18:51:06 leonb Exp $
 **********************************************************************/

#include "header.h"

void clear_cfunction(cfunction_t *f)
{
   f->name = NULL;
   f->kname = NULL;
}

void mark_cfunction(cfunction_t *f)
{
   MM_MARK(f->name);
   MM_MARK(f->kname);
}

mt_t mt_cfunction = mt_undefined;

const char *func_name(at *p) {
   
   cfunction_t *func = Mptr(p);
   at *name = func->name;
   at *clname = NIL;
  
   if (CONSP(name) && MODULEP(Car(name)))
      name = Cdr(name);

  if (CONSP(name) && SYMBOLP(Car(name))) {
     clname = Car(name);
     name = Cdr(name);
  }
  if (SYMBOLP(name)) {
     sprintf(string_buffer, "::%s:", NAMEOF(Class(p)->classname));
     if (SYMBOLP(clname)) {
        strcat(string_buffer, NAMEOF(clname));
        strcat(string_buffer, ".");
     }
     strcat(string_buffer, NAMEOF(name));
     return mm_strdup(string_buffer);
  }
  /* Kesako? */
  assert(Class(p)->name);
  return (Class(p)->name)(p);
}

/* General lfunc routines -----------------------------	 */

void clear_lfunction(lfunction_t *f)
{
   f->formal_args = NULL;
   f->body = NULL;
}

void mark_lfunction(lfunction_t *f)
{
   MM_MARK(f->formal_args);
   MM_MARK(f->body);
}

static mt_t mt_lfunction = mt_undefined;

static at *at_optional, *at_rest, *at_define_hook;
static void parse_optional_stuff(at *formal_list, at *real_list);


/* static void */
/* pop_args(at *formal_list) */
/* { */
/*   while (CONSP(formal_list)) { */
/*     pop_args(Car(formal_list)); */
/*     formal_list = Cdr(formal_list); */
/*   } */
/*   if SYMBOLP(formal_list) */
/*     SYMBOL_POP(formal_list); */
/* } */

static void pop_args(at *formal_list) {
   
   while (CONSP(formal_list)) {
      pop_args(Car(formal_list));
      formal_list = Cdr(formal_list);
   }
   if (SYMBOLP(formal_list)) {
      symbol_t *symb = Mptr(formal_list);
      assert(symb);
      if (symb->next)
         Mptr(formal_list) = symb->next;
   }
}

static void push_args(at *formal_list, at *real_list) {

   /* fast non tail-recursive loop for parsing the trees */
   while (CONSP(formal_list) && CONSP(real_list) && 
          Car(formal_list)!=at_rest &&
          Car(formal_list)!=at_optional) {
      push_args(Car(formal_list), Car(real_list));
      real_list = Cdr(real_list);
      formal_list = Cdr(formal_list);
   };
   
   /* parsing a single symbol in a tree */
   if (SYMBOLP(formal_list)) {
      SYMBOL_PUSH(formal_list, real_list);
   
   } else if (CONSP(formal_list)) {
      parse_optional_stuff(formal_list, real_list);

   } else if (formal_list && !real_list) {
      error(NIL, "missing arguments", formal_list);

   } else if (real_list && !formal_list)
      error(NIL, "too many arguments", real_list);
   
   return;
}


static void  parse_optional_stuff(at *formal_list, at *real_list) {

   if (CONSP(formal_list) && Car(formal_list) == at_optional) {
      push_args(at_optional, NIL);
      formal_list = Cdr(formal_list);
      
      while (CONSP(formal_list)) {
         at *flcar = Car(formal_list);
         if (flcar==at_rest) break;
         
         if (SYMBOLP(flcar)) {
            if (CONSP(real_list)) {
               push_args(flcar, Car(real_list));
               real_list = Cdr(real_list);
            } else
               push_args(flcar, NIL);
            
         } else if (CONSP(flcar) && SYMBOLP(Car(flcar)) && LASTCONSP(Cdr(flcar))) {
            if (CONSP(real_list)) {
               push_args(Car(flcar), Car(real_list));
               real_list = Cdr(real_list);
            } else 
               push_args(Car(flcar), Cadr(flcar));
	
         } else
            error(NIL, "error in &optional syntax", formal_list);
         
         formal_list = Cdr(formal_list);
      } 
   }
   /* &rest */
   if (CONSP(formal_list) && Car(formal_list)==at_rest) {
      push_args(at_rest, NIL);
      formal_list=Cdr(formal_list);
      if (LASTCONSP(formal_list) && SYMBOLP(Car(formal_list))) {
         push_args(Car(formal_list), real_list);
         return;
      } else
         error(NIL, "error in &rest syntax", formal_list);
   }
   
   if (formal_list)
      error(NIL, "error in &optional syntax", formal_list);		
   if (real_list)
      error(NIL, "too many arguments", real_list);
}


at *eval_arglist(at *p)
{
   MM_ENTER;

   at *list = p;
   at **now = &p;
   p = NIL;
   
   while (CONSP(list)) {
      *now = new_cons(eval(Car(list)), NIL);
      now = &Cdr(*now);
      list = Cdr(list);
   }
   if (list)
      *now = eval(list);

   MM_RETURN(p);
}

at *eval_arglist_dm(at *p)
{
   at *list = p;
   at **now = &p;
   p = NIL;
   
   while (CONSP(list)) {
      *now = new_cons(Car(list), NIL);
      now = &Cdr(*now);
      list = Cdr(list);
   }
   if (list)
      *now = eval(list);

   return p;
}

at *dx_stack[DXSTACKSIZE];
at **dx_sp = dx_stack;

/* version for DXs and DHs, return old dx stack pointer */
at **eval_arglist_dx(at *q)
{
   at **arg_pos = dx_sp;

   at *q2 = q;
   while (CONSP(q)) {
      if (++dx_sp >= dx_stack + DXSTACKSIZE)
         error(NIL, "sorry, stack full (Merci Yann)", NIL);
      *dx_sp = eval(Car(q));
      q = Cdr(q);
   }
   if (q)
      q = eval(q);

   while (CONSP(q)) {
      if (++dx_sp >= dx_stack + DXSTACKSIZE)
         error(NIL, "sorry, stack full (Merci Yann)", NIL);
      *dx_sp = Car(q);
      q = Cdr(q);
   }
   if (q)
      RAISEF("bad argument list", q2);
   
   return arg_pos;
}

/* reset DX stack, used after error */
void reset_dx_stack(void)
{
   dx_sp = dx_stack;
   return;
}


/* DX class -------------------------------------------	 */

at *dx_listeval(at *p, at *q)
{
   MM_ENTER;

   cfunction_t *f = Mptr(p);
   if (CONSP(f->name))
      check_primitive(f->name, f->info);

   at **arg_pos = eval_arglist_dx(Cdr(q));
   int arg_num = (int)(dx_sp - arg_pos);
   at *ans = DXCALL(f)(arg_num, arg_pos);
   dx_sp = arg_pos;

   MM_RETURN(ans);
}

at *new_dx(at *name, at *(*addr)(int, at **))
{
   cfunction_t *f = mm_alloc(mt_cfunction);
   f->call = f->info = addr;
   f->name = name;
   f->kname = 0;
   return new_extern(&dx_class, f);
}


/* DY class -------------------------------------------	 */

at *dy_listeval(at *p, at *q)
{
   MM_ENTER;
   
   cfunction_t *f = Mptr(p);
   if (last(q, 0))
      q = eval_arglist_dm(q);
   if (CONSP(f->name))
      check_primitive(f->name, f->info);
   
   at *ans = DYCALL(f)(Cdr(q));
   
   MM_RETURN(ZOMBIEP(ans) ? NIL : ans);
}


at *new_dy(at *name, at *(*addr)(at *))
{
   cfunction_t *f = mm_alloc(mt_cfunction);
   f->call = f->info = addr;
   f->name = name;
   f->kname = 0;
   return new_extern(&dy_class, f);
}


/* Macros for making DYs for de, df, and dm ----------------------- */

/* hook for code transform at definition */
#define TRANSFORM_DEF(q, sdx)  \
  at *define_hook = eval(at_define_hook); \
  if (define_hook) { \
    at *def = new_cons(named(sdx), q); \
    at *tdef = apply(define_hook, def); \
    q = Cdr(tdef); \
  }

#define DY_DEF(NAME) \
DY(name2(y,NAME)) \
{ \
  at *q = ARG_LIST; \
  TRANSFORM_DEF(q, enclose_in_string(NAME)); \
  ifn (CONSP(q) && CONSP(Cdr(q))) \
    RAISEF("syntax error", new_cons(named(enclose_in_string(NAME)), q)); \
  ifn (SYMBOLP(Car(q))) \
    RAISEF("syntax error in definition: not a symbol", Car(q)); \
  at *p = Car(q); \
  at *func = name2(new_,NAME)(Cadr(q), Cddr(q)); \
  sym_set(Symbol(p), func, true); \
  return p; \
}


/* DE class -------------------------------------------	 */

at *de_listeval(at *p, at *q)
{
   MM_ENTER;

   lfunction_t *f = Mptr(p);
   q = eval_arglist(Cdr(q));
   push_args(f->formal_args, q);
   at *ans = progn(f->body);
   pop_args(f->formal_args);

   MM_RETURN(ans);
}

at *new_de(at *formal, at *body)
{
   lfunction_t *f = mm_alloc(mt_lfunction);
   f->formal_args = formal;
   f->body = body;
   return new_extern(&de_class, f);
}

DY(ylambda)
{
  at *q = ARG_LIST;
  ifn (CONSP(q) && CONSP(Cdr(q))) 
     RAISEFX("syntax error in function definition", NIL);
  return new_de(Car(q), Cdr(q));
}

DY_DEF(de);


/* DF class -------------------------------------------	 */

at *df_listeval(at *p, at *q)
{
   MM_ENTER;

   lfunction_t *f = Mptr(p);
   if (last(q, 0))
      q = eval_arglist_dm(q);
   push_args(f->formal_args, Cdr(q));
   at *ans = progn(f->body);
   pop_args(f->formal_args);

   MM_RETURN(ans);
}

at *new_df(at *formal, at *body)
{
  lfunction_t *f = mm_alloc(mt_lfunction);
  f->formal_args = formal;
  f->body = body;
  return new_extern(&df_class, f);
}

DY(yflambda)
{
   at *q = ARG_LIST;
   ifn (CONSP(q) && CONSP(Cdr(q)))
      RAISEF("illegal definition of function", NIL);
   return new_df(Car(q), Cdr(q));
}

DY_DEF(df);

/* DM class -------------------------------------------	 */


at *dm_listeval(at *p, at *q)
{
   MM_ENTER;

   lfunction_t *f = Mptr(p);
   if (last(q, 0))
      q = eval_arglist_dm(q);
   push_args(f->formal_args, q);
   at *m = progn(f->body);
   pop_args(f->formal_args);

   MM_RETURN(eval(m));
}

at *new_dm(at *formal, at *body)
{
   lfunction_t *f = mm_alloc(mt_lfunction);
   f->formal_args = formal;
   f->body = body;
   return new_extern(&dm_class, f);
}

DY(ymlambda)
{
   at *q = ARG_LIST;
   ifn(CONSP(q) && CONSP(Cdr(q)))
      RAISEFX("illegal definition of function", NIL);
   return new_dm(Car(q), Cdr(q));
}

DY_DEF(dm);

static at *macroexpand(at *p, at *q)
{
   ifn (p && Class(p) == &dm_class)
      RAISEF("not a macro", p);

   lfunction_t *f = Mptr(p);
   push_args(f->formal_args, q);
   at *ans = progn(f->body);
   pop_args(f->formal_args);
   return ans;
}

DY(ymacroexpand)
{
   ifn (CONSP(ARG_LIST) && CONSP(Car(ARG_LIST)) && !Cdr(ARG_LIST))
      RAISEFX("syntax error", NIL);
   at *p = eval(Caar(ARG_LIST));
   return macroexpand(p, Car(ARG_LIST));
}


/* General purpose routines -------------------	 */

/*
 * function definition extraction given a lisp function, builds the list  (X
 * ARG  {..LST..} ) where  X is   'lambda', 'flambda', 'mlambda', or   'de
 * XX' , 'df XX',   'dm XX', if a name is found, ARG is the formal arguments
 * list, LST is the definition of the function.
 */

at *funcdef(at *p)
{
   char *s = NIL;
   
   if (Class(p) == &de_class)
      s = "lambda";
   if (Class(p) == &df_class)
      s = "flambda";
   if (Class(p) == &dm_class)
      s = "mlambda";
   if (!s)
      return NIL;

   lfunction_t *f = Mptr(p);
   return new_cons(named(s), new_cons(f->formal_args, f->body));
}

DX(xfuncdef)
{
   ARG_NUMBER(1);
   return funcdef(APOINTER(1));
}

DX(xfunctionp)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   return FUNCTIONP(p) ? p : NIL;
}

void pre_init_function(void)
{
   if (mt_cfunction == mt_undefined)
      mt_cfunction = 
         MM_REGTYPE("cfunction", sizeof(cfunction_t),
                    clear_cfunction, mark_cfunction, 0);
   if (mt_lfunction == mt_undefined)
      mt_lfunction =
         MM_REGTYPE("lfunction", sizeof(lfunction_t),
                    clear_lfunction, mark_lfunction, 0);
}

class_t function_class;
class_t dx_class, dy_class, de_class, df_class, dm_class;

void init_function(void)
{
   pre_init_function();

   /* set up function classes */
   class_init(&function_class, false);
   class_define("FUNCTION", &function_class);
   
   class_init(&dx_class, false);
   dx_class.name = func_name;
   dx_class.listeval = dx_listeval;
   dx_class.super = &function_class;
   dx_class.atsuper = function_class.backptr;
   class_define("DX",&dx_class);
   
   class_init(&dy_class, false);
   dy_class.name = func_name;
   dy_class.listeval = dy_listeval;
   dy_class.super = &function_class;
   dy_class.atsuper = function_class.backptr;
   class_define("DY",&dy_class);
   
   class_init(&de_class, false);
   de_class.listeval = de_listeval;
   de_class.super = &function_class;
   de_class.atsuper = function_class.backptr;
   class_define("DE",&de_class);

   class_init(&df_class, false);
   df_class.listeval = df_listeval;
   df_class.super = &function_class;
   df_class.atsuper = function_class.backptr;
   class_define("DF",&df_class);

   class_init(&dm_class, false);
   dm_class.listeval = dm_listeval;
   dm_class.super = &function_class;
   dm_class.atsuper = function_class.backptr;
   class_define("DM",&dm_class);

   at_optional = var_define("&optional");
   at_rest = var_define("&rest");
   at_define_hook = var_define("define-hook");
   
   dy_define("lambda", ylambda);
   dy_define("de", yde);
   dy_define("flambda", yflambda);
   dy_define("df", ydf);
   dy_define("mlambda", ymlambda);
   dy_define("dm", ydm);
   dy_define("macroexpand", ymacroexpand);
   dx_define("funcdef", xfuncdef);
   dx_define("functionp", xfunctionp);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
