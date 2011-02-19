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

/*
 * eval(p) <=> (*eval_ptr)(p)   (MACRO)
 * 
 * when eval_ptr is eval_std:   normal mode 
 * when eval_ptr is eval_brk:   user break occurred
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
      
      at *q = Car(p);
      q = SYMBOLP(q) ? symbol_class->selfeval(q) : eval_std(q);
      if (q)
         p = Class(q)->listeval(q, p);
      else
         p = nil_class->listeval(q, p);

      top_link = top_link->prev;
      return p;
      
   } else if (p) {
      return Class(p)->selfeval(p);

   } else
      return NIL;
}

DX(xeval)
{
  at *q = NULL;
  for (int i = 1; i <= arg_number; i++)
     q = eval(APOINTER(i));
  return q;
}


at *eval_brk(at *p)
{
   user_break("on");
   return eval_std(p);
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
                       new_cons(NEW_STRING(line),
                                new_cons(expr, new_cons(info, NIL))));
   eval_ptr = eval_std;
   at *ans = eval(new_cons(at_trace, args));
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


static at *circular_list(at *a)
{
   at *p = new_cons(a, NIL);
   Cdr(p) = p;
   return p;
}

/* turn everything not a cons or NIL into a circular list */
static bool prep_args(at **listv, int nargs)
{
   bool any_list = false;
   for (int i=0; i<nargs; i++) {
      if (listv[i]==NIL || CONSP(listv[i])) {
         any_list = true;
         break;
      }
   }
   ifn (any_list)
      return false;

   for (int i=0; i<nargs; i++) {
      ifn (listv[i]==NIL || CONSP(listv[i]))
         listv[i] = circular_list(listv[i]);
   }
   
   return any_list;
}

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
   ifn (prep_args(listv, n))
      RAISEF("at least one argument must be a list", NIL);
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
   ifn (prep_args(listv, n))
      RAISEF("at least one argument must be a list", NIL);
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
   ifn (prep_args(listv, n))
      RAISEF("at least one argument must be a list", NIL);
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

/* double quote */
DY(ydquote)
{
   ifn (CONSP(ARG_LIST) && (LASTCONSP(ARG_LIST)))
      RAISEFX("one argument expected", NIL);
   return quote(Car(ARG_LIST));
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
   dy_define("quote", yquote);
   dy_define("dquote", ydquote);
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
