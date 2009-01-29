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
	evaluation routines 
        EVAL APPLY PROGN MAPCAR MAPC RMAPCAR RMAPC LIST QUOTE
********************************************************************** */

#include "header.h"

#ifdef UNIX
# include <signal.h>
  static sigset_t sigint_mask;
# define BLOCK_SIGINT   sigprocmask(SIG_BLOCK,&sigint_mask,NULL);
# define UNBLOCK_SIGINT sigprocmask(SIG_UNBLOCK,&sigint_mask,NULL);
#endif

bool debug_mode = false;

/*
 * eval(p) <=> (*eval_ptr)(p)   (MACRO)
 * 
 * when eval_ptr is eval_std: normal mode 
 * when eval_ptr is eval_debug: trace mode
 */

at *(*eval_ptr) (at *);

#define PUSH_EVAL_PTR(func) {  \
   at *(*old_eval_ptr) (at *) = eval_ptr; \
   eval_ptr = func;
 
#define POP_EVAL_PTR  eval_ptr = old_eval_ptr; }

struct call_chain *top_link = NULL;

at *eval_std(at *p)
{
   if (CONSP(p)) {
      struct call_chain link;
      link.prev = top_link;
      link.this_call = p;
      top_link = &link;
      
      CHECK_MACHINE("on");
      
      at *q = eval_std(Car(p));
      if (q)
         p = Class(q)->listeval(q, p);
      else
         p = null_class.listeval(q, p);

      top_link = top_link->prev;
      return p;
      
   } else if (p) {
      return Class(p)->selfeval(p);

   } else
      return NIL;
}

DX(xeval)
{
   ARG_NUMBER(1);
   return eval(APOINTER(1));
}


/* call_trace_hook
 * Calls function trace-hook used for debugging 
 */

/* build list of calls */
at *call_stack(void)
{
   struct call_chain *link = top_link;
   at *chain = NIL;
   at **pc = &chain;
   while (link) {
      *pc = new_cons(link->this_call, NIL);
      pc = &Cdr(*pc);
      link = link->prev;
   }
   return chain;
}

DX(xcall_stack)
{
   ARG_NUMBER(0);
   return call_stack();
}

static at *at_trace;

static bool call_trace_hook(int tab, const char *line, at *expr, at *info)
{
   if (tab>0 && !info)
      info = call_stack();

   at *args = new_cons(NEW_NUMBER(tab),
                       new_cons(new_string(line),
                                new_cons(expr, new_cons(info, NIL))));
   eval_ptr = eval_std;
   at *ans = apply(at_trace, args);
   return (ans != NIL);
}


/* eval_debug 
 * is used by function DEBUG.
 * calls function TRACE-HOOK before and after evaluation
 * user breaks are ignored while in TRACE-HOOK
 */

at *eval_debug(at *q)
{
   int tab = ++error_doc.debug_tab;
   
   BLOCK_SIGINT;
   bool flag = call_trace_hook(tab, first_line(q), q, NIL);
   UNBLOCK_SIGINT;

   eval_ptr = (flag ? eval_debug : eval_std);
   at *ans = eval_std(q);
   
   BLOCK_SIGINT;
   flag = call_trace_hook(-tab, first_line(ans), q, ans);
   UNBLOCK_SIGINT;

   eval_ptr = (flag ? eval_debug : eval_std);
   error_doc.debug_tab = tab-1;
   return ans;
}

/* apply(f, args) apply function f to args, where args
 * are already evaluated arguments.
 */

static at *at_applystack;

at *apply(at *p, at *q)
{
   ifn (p)
      RAISEF("cannot apply nil", NIL);

   SYMBOL_PUSH(at_applystack, q);
   at *res = Class(p)->listeval(p, new_cons(p, at_applystack));
   SYMBOL_POP(at_applystack);
   return res;
}

DX(xapply)
{ 
   if (arg_number < 2)
      RAISEFX("at least two arguments expected", NIL);

   at *p = ALIST(arg_number);
   while (arg_number-- > 2)
      p = new_cons(APOINTER(arg_number), p);

   return apply(APOINTER(1), p);
}

/*
 * progn(p) evals all the elements of the p list. returns the last evaluation
 * result
 */
at *progn(at *p)
{
   MM_ENTER;

   at *q = NIL;
   while (CONSP(p)) {
      q = eval(Car(p));
      p = Cdr(p);
   }
   if (p){
      RAISEF("form not a proper list", p);
   }

   MM_RETURN(q);
}

DY(yprogn)
{
   return progn(ARG_LIST);
}


/*
 * prog1(p) evals all the elements of the p list. returns the first
 * evaluation result.
 */
at *prog1(at *p)
{
   at *ans = NIL;
   if (CONSP(p)) {
      ans = eval(Car(p));
      p = Cdr(p);
   }
   while (CONSP(p)) {
      eval(Car(p));
      p = Cdr(p);
   }
   if (p) {
      RAISEF("form not a proper list", p);
   }
   return ans;
}

DY(yprog1)
{
   return prog1(ARG_LIST);
}



/*
 * mapc mapcar mapcan
 */


/* compose list of cars of lists and return it */
/* return nil if any of the lists is nil       */

static at *next_args(at **listv, int nargs)
{
   at *args = NIL;
   at **where = &args;
   
   for (int i=0; i<nargs; i++)
      if (listv[i]==NIL) {
         return NIL;
      } else {
         *where = new_cons(Car(listv[i]), NIL);
         where = &Cdr(*where);
         ifn (CONSP(listv[i]))
            error(NIL, "some argument is not a proper list", NIL); 
         listv[i] = Cdr(listv[i]);
      }
   return args;
}

at *mapc(at *f, at **listv, int n)
{
   at *result = listv[0];
   at *args;
   while ((args = next_args(listv, n)))
      apply(f, args);

   return result;
}

DX(xmapc)
{
   if (arg_number < 2)
      RAISEFX("arguments missing", NIL);
   
   return mapc(APOINTER(1), &APOINTER(2), arg_number-1);
}

at *mapcar(at *f, at **listv, int n)
{
   at *result = NIL;
   at **where = &result;
   at *args;
   while ((args = next_args(listv, n))) {
      *where = new_cons(apply(f, args), NIL);
      where = &Cdr(*where);
   }
   return result;
}

DX(xmapcar)
{
   if (arg_number < 2)
      RAISEFX("arguments missing", NIL);
   
   return mapcar(APOINTER(1), &APOINTER(2), arg_number-1);
}

at *mapcan(at *f, at **listv, int n)
{
   at *result = NIL;
   at **where = &result;
   at *args;
   while ((args = next_args(listv, n))) {
      while (CONSP(*where))
         where = &Cdr(*where);
      *where = apply(f, args);
   }
   return result;
}

DX(xmapcan)
{
   if (arg_number < 2)
      RAISEFX("arguments missing", NIL);

   return mapcan(APOINTER(1), &APOINTER(2), arg_number-1);
}

/*  unzip_bindings: splice a list of pairs */

static char *errmsg_vardecl1 = "not a list of pairs";
static char *errmsg_vardecl2 = "not a valid variable declaration form";
static char *errmsg_vardecl3 = "number of values does not match number of variables";

static char *unzip_bindings(at *l, at **l1, at **l2)
{
   at **where1 = l1;
   at **where2 = l2;
   at *q = l;
   
   *l1 = *l2 = NIL;
   while (CONSP(q)) {
      at *pair = Car(q);
      q = Cdr(q);

      ifn (CONSP(pair) && LASTCONSP(Cdr(pair))) {
         return errmsg_vardecl1;
      }
      *where1 = new_cons(Car(pair), NIL);
      *where2 = new_cons(Cadr(pair), NIL);
      where1 = &Cdr(*where1);
      where2 = &Cdr(*where2);
   }
   if (q) {
      return errmsg_vardecl2;
   }
   return (char *)NULL;
}

/* let, let*, for, each, all */

at *let(at *vardecls, at *body)
{
   at *syms, *vals;
   RAISEF(unzip_bindings(vardecls, &syms, &vals), vardecls);

   at *func = new_de(syms, body);
   at *result = de_class.listeval(func, new_cons(func, vals));
   return result;
}

DY(ylet)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("invalid 'let' form", NIL);
   return let(Car(ARG_LIST), Cdr(ARG_LIST));
}

at *lete(at *vardecls, at *body)
{
   at *syms, *vals;
   RAISEF(unzip_bindings(vardecls, &syms, &vals), vardecls);

   at *func = new_de(syms, body);
   at *result = de_class.listeval(func, new_cons(func, vals));

   /* before we return, explicitly delete all local variables */
   while (CONSP(vals)) {
      lush_delete(Car(vals));
      vals = Cdr(vals);
   }
   return result;
}

DY(ylete)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("invalid 'lete' form", NIL);
   return lete(Car(ARG_LIST), Cdr(ARG_LIST));
}

at *letS(at *vardecls, at *body)
{
   at *q;
   for (q = vardecls; CONSP(q); q = Cdr(q)) {
      at *pair = Car(q);
      ifn (CONSP(pair) && LASTCONSP(Cdr(pair)))
         RAISEF(errmsg_vardecl1, vardecls);

      if (SYMBOLP(Car(pair))) {
         at *val = eval(Cadr(pair));
         SYMBOL_PUSH(Car(pair), val);
         
      } else if (CONSP(Car(pair))) {
         at *syms = Car(pair);
         at *vals = eval(Cadr(pair));
         if (length(syms) != (length(vals))) {
            RAISEF(errmsg_vardecl3, vardecls);
         }
         while (CONSP(syms)) {
            SYMBOL_PUSH(Car(syms), Car(vals));
            syms = Cdr(syms);
            vals = Cdr(vals);
         }
      }
   }
   if (q)
      RAISEF(errmsg_vardecl2, NIL);

   at *ans = progn(body);

   for (q = vardecls; q; q = Cdr(q)) {
      if (SYMBOLP(Caar(q))) {
         SYMBOL_POP(Caar(q));
      } else {
         at *syms = Caar(q);
         while (CONSP(syms)) {
            SYMBOL_POP(Car(syms));
            syms = Cdr(syms);
         }
      }
   }
   return ans;
}

DY(yletS)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("invalid 'let*' form", NIL);
   return letS(Car(ARG_LIST), Cdr(ARG_LIST));
}
  

/*
 * (quote a1) returns a1 without evaluation
 */

static at *at_quote = NIL;

at *quote(at *p)
{
   return new_cons(at_quote,new_cons(p, NIL));
}


DY(yquote)
{
   ifn (CONSP(ARG_LIST) && (LASTCONSP(ARG_LIST)))
      RAISEFX("one argument expected", NIL);
   return Car(ARG_LIST);
}


/*
 * (debug ...) eval lists in debug mode
 */
DY(ydebug)
{
   at *ans;
   PUSH_EVAL_PTR(eval_debug) {
      ans = progn(ARG_LIST);
   } POP_EVAL_PTR;

   return ans;
}


/*
 * (nodebug ...) eval lists in normal mode
 */
DY(ynodebug)
{
   at *ans;
   PUSH_EVAL_PTR(eval_std) {
      ans = progn(ARG_LIST);
   } POP_EVAL_PTR;

   return ans;
}

/* --------- INITIALISATION CODE --------- */

void init_eval(void)
{
#ifdef UNIX
   sigemptyset(&sigint_mask);
   sigaddset(&sigint_mask, SIGINT);
#endif

   eval_ptr = eval_std;
   dx_define("eval", xeval);
   dx_define("apply", xapply);
   dx_define("call-stack", xcall_stack);
   dx_define("mapc", xmapc);
   dx_define("mapcar", xmapcar);
   dx_define("mapcan", xmapcan);

   dy_define("progn", yprogn);
   dy_define("prog1", yprog1);
   dy_define("let", ylet);
   dy_define("lete", ylete);
   dy_define("let*", yletS);
   dy_define("quote", yquote);
   dy_define("debug", ydebug);
   dy_define("nodebug", ynodebug);
   
   at_applystack = var_define("APPLY-STACK");
   at_trace = var_define("trace-hook");
   at_quote = var_define("quote");
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
