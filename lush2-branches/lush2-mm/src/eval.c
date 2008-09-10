/***********************************************************************
 * 
 *  PSU Lush
 *    Copyright (C) 2005 Ralf Juengling
 *  Derived from LUSH Lisp Universal Shell
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
 * $Id: eval.c,v 1.2 2002/07/25 01:13:13 leonb Exp $
 **********************************************************************/

/***********************************************************************
	evaluation routines 
        EVAL APPLY PROGN MAPCAR MAPC RMAPCAR RMAPC LIST QUOTE
********************************************************************** */

#include "header.h"
#include "mm.h"

#ifdef UNIX
# include <signal.h>
  static sigset_t sigint_mask;
# define BLOCK_SIGINT   sigprocmask(SIG_BLOCK,&sigint_mask,NULL);
# define UNBLOCK_SIGINT sigprocmask(SIG_UNBLOCK,&sigint_mask,NULL);
#endif

/*
 * eval(p) <=> (*eval_ptr)(p)   (MACRO)
 * 
 * when eval_ptr is eval_std: normal mode 
 * when eval_ptr is eval_debug: trace mode
 */

at *(*eval_ptr) (at *);
at *(*argeval_ptr) (at *);

#define PUSH_ARGEVAL_PTR(func) {  \
  at *(*old_eval_ptr) (at *) = argeval_ptr; \
  argeval_ptr = func;
 
#define POP_ARGEVAL_PTR  argeval_ptr = old_eval_ptr; }

#define PUSH_EVAL_PTR(func) {  \
  at *(*old_eval_ptr) (at *) = eval_ptr; \
  eval_ptr = func;
 
#define POP_EVAL_PTR  eval_ptr = old_eval_ptr; }

at *eval_std(at *p)
{
   extern at *generic_listeval(at *p, at *q);

   if (CONSP(p)) {
      ED_PUSH_CALL(p);
      
      argeval_ptr = eval_ptr;
      CHECK_MACHINE("on");
      
      at *q = eval_std(p->Car);
      if (q)
         p = (q->Class->listeval)(q, p);
      else
         p = generic_listeval(q, p);

      ED_POP_CALL();
      return p;
      
   } else if (p) {
      assert(p->Class->selfeval);
      return (*(p->Class->selfeval)) (p);
   } else
      return NIL;
}


DX(xeval)
{
   if (arg_number < 1)
      RAISEFX("no arguments", NIL);
   
   for (int i = 1; i < arg_number; i++) {
      ARG_EVAL(i);
      eval(APOINTER(i));
   }
   ARG_EVAL(arg_number);
   return eval(APOINTER(arg_number));
}


/* eval_nothing
 * is used to prevent argument evaluation when executing DF functions
 */

at *eval_nothing(at *q)
{
   return q;
}


/* call_trace_hook
 * Calls function trace-hook used for debugging 
 */

static at *at_trace;

static bool call_trace_hook(int tab, char *line, at *expr, at *info)
{
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
   bool flag = call_trace_hook(tab, first_line(q), q, error_doc.this_call);
   UNBLOCK_SIGINT;

   eval_ptr = argeval_ptr = (flag ? eval_debug : eval_std);
   at *ans = eval_std(q);
   
   BLOCK_SIGINT;
   flag = call_trace_hook(-tab, first_line(ans), q, ans);
   UNBLOCK_SIGINT;

   eval_ptr = argeval_ptr = (flag ? eval_debug : eval_std);
   error_doc.debug_tab = tab-1;
   return ans;
}

/*
 * apply(f,args) C implementation eval(cons(f,arg)) LISP
 * implementation try to care about the function type.
 */

at *apply(at *p, at *q)
{
   ifn (p)
      RAISEF("cannot apply nil", NIL);
   p = eval(p);
   q = new_cons(p, q);
   CHECK_MACHINE("on");
   
   at *result;
   PUSH_ARGEVAL_PTR(eval_nothing) {
      assert(p->Class->listeval);
      result = (*(p->Class->listeval)) (p, q);
   } POP_ARGEVAL_PTR;

   return result;
}

DY(yapply)
{ 
   at *p = ARG_LIST;
   ifn (CONSP(p) && CONSP(p->Cdr))
      RAISEF("at least two arguments expected", NIL);
   
   at *q;
   if (p->Cdr->Cdr) {
      at *args = eval_a_list(p->Cdr);
      at *l1 = nfirst(length(args)-1, args);
      at *l2 = lasta(args);
      q = append(l1, l2);
   } else
      q = eval(p->Cdr->Car);
   
   p = eval(p->Car);
   return apply(p, q);
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
      q = eval(p->Car);
      p = p->Cdr;
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
      ans = eval(p->Car);
      p = p->Cdr;
   }
   while (CONSP(p)) {
      eval(p->Car);
      p = p->Cdr;
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

static inline at *next_args(at **listv, int nargs)
{
   at *args = NIL;
   at **where = &args;
   
   for (int i=0; i<nargs; i++)
      if (listv[i]==NIL) {
         return NIL;
      } else {
         *where = new_cons(listv[i]->Car, NIL);
         where = &((*where)->Cdr);
         ifn (CONSP(listv[i]))
            error(NIL, "some argument is not a proper list", NIL); 
         listv[i] = listv[i]->Cdr;
      }
   return args;
}

#define INIT_LISTV(ls, n)                      \
  for (n=0; n<=MAXARGMAPC; n++) {              \
    if (ls==NIL)                               \
      break;                                   \
    ifn (LISTP(ls->Car))                       \
      RAISEF("not a list", ls->Car);           \
    listv[n] = ls->Car;                        \
    ls = ls->Cdr;                              \
  }                                            \
  if (ls!=NIL)                                 \
    RAISEF("too many arguments", NIL);

at *mapc(at *f, at *lists)
{
   at *listv[MAXARGMAPC];
   int n;
   INIT_LISTV(lists, n);

   at *result = listv[0];
   at *args;
   while ((args = next_args(listv, n)))
      apply(f, args);

   return result;
}

at *mapcar(at *f, at *lists)
{
   at *listv[MAXARGMAPC];
   int n;
   INIT_LISTV(lists, n);

   at *result = NIL;
   at **where = &result;
   at *args;
   while ((args = next_args(listv, n))) {
      *where = new_cons(apply(f, args), NIL);
      where = &((*where)->Cdr);
   }
   return result;
}

at *mapcan(at *f, at *lists)
{
   at *listv[MAXARGMAPC];
   int n;
   INIT_LISTV(lists, n);
   
   at *result = NIL;
   at **where = &result;
   at *args;
   while ((args = next_args(listv, n))) {
      while (CONSP(*where))
         where = &((*where)->Cdr);
      *where = apply(f, args);
   }
   return result;
}

#define DYMAP(mapx)  DY(name2(y,mapx)) {        \
  ifn (CONSP(ARG_LIST))                         \
    RAISEF("arguments missing", NIL);           \
                                                \
  at *fn = eval(ARG_LIST->Car);                 \
  at *lists = eval_a_list(ARG_LIST->Cdr);       \
  if (lists==NIL)                               \
    RAISEF("list argument(s) missing", NIL);    \
                                                \
  at *result = mapx(fn, lists);                 \
  return result;                                \
}

DYMAP(mapc);
DYMAP(mapcar);
DYMAP(mapcan);


/*  unzip_and_eval: splice a list of pairs,
    eval 2nd element of each pair             */

static char *errmsg_vardecl1 = "not a list of pairs";
static char *errmsg_vardecl2 = "not a valid variable declaration form";
static char *errmsg_vardecl3 = "number of values does not match number of variables";

char *unzip_and_eval_cdr(at *l, at **l1, at **l2)
{
   at **where1 = l1;
   at **where2 = l2;
   at *q = l;
   
   *l1 = *l2 = NIL;
   while (CONSP(q)) {
      at *pair = q->Car;
      q = q->Cdr;

      ifn (CONSP(pair) && LASTCONSP(pair->Cdr)) {
         return errmsg_vardecl1;
      }
      *where1 = new_cons(pair->Car, NIL);
      *where2 = new_cons(eval(pair->Cdr->Car), NIL);
      where1 = &((*where1)->Cdr);
      where2 = &((*where2)->Cdr);
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
   RAISEF(unzip_and_eval_cdr(vardecls, &syms, &vals), vardecls);

   at *func = new_df(syms, body);
   at *result = apply(func, vals);
   return result;
}

DY(ylet)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("invalid 'let' form", NIL);
   return let(ARG_LIST->Car, ARG_LIST->Cdr);
}

at *letS(at *vardecls, at *body)
{
   at *q;
   for (q = vardecls; CONSP(q); q = q->Cdr) {
      at *pair = q->Car;
      ifn (CONSP(pair) && LASTCONSP(pair->Cdr))
         RAISEF(errmsg_vardecl1, vardecls);

      if (SYMBOLP(pair->Car)) {
         at *val = eval(pair->Cdr->Car);
         SYMBOL_PUSH(pair->Car, val);
         
      } else if (CONSP(pair->Car)) {
         at *syms = pair->Car;
         at *vals = eval(pair->Cdr->Car);
         if (length(syms) != (length(vals))) {
            RAISEF(errmsg_vardecl3, vardecls);
         }
         while (CONSP(syms)) {
            SYMBOL_PUSH(syms->Car, vals->Car);
            syms = syms->Cdr;
            vals = vals->Cdr;
         }
      }
   }
   if (q)
      RAISEF(errmsg_vardecl2, NIL);

   at *ans = progn(body);

   for (q = vardecls; q; q = q->Cdr) {
      if (SYMBOLP(q->Car->Car)) {
         SYMBOL_POP(q->Car->Car);
      } else {
         at *syms = q->Car->Car;
         while (CONSP(syms)) {
            SYMBOL_POP(syms->Car);
            syms = syms->Cdr;
         }
      }
   }
   return ans;
}

DY(yletS)
{
   ifn (CONSP(ARG_LIST))
      RAISEF("invalid 'let*' form", NIL);
   return letS(ARG_LIST->Car, ARG_LIST->Cdr);
}
  

DY(yfor)
{
   ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Car))
      RAISEFX("syntax error", NIL);

   at *sym, *p = ARG_LIST->Car;
   ifn (SYMBOLP(sym = p->Car))
      RAISEFX("not a symbol", sym);

   at *num = NIL;
   p = p->Cdr;
   ifn (CONSP(p) && (num = eval(p->Car)) && NUMBERP(num))
      RAISEFX("not a number", num);
   double start = Number(num);

   p = p->Cdr;
   ifn (CONSP(p) && (num = eval(p->Car)) && NUMBERP(num))
      RAISEFX("not a number", num);
   double end = Number(num);

   p = p->Cdr;
   double step = 1.0;
   if (CONSP(p) && !p->Cdr) {
      ifn (CONSP(p) && (num = eval(p->Car)) && NUMBERP(num))
         RAISEFX("not a number", num);
      step = Number(num);

   } else if (p) {
      RAISEFX("syntax error", p);
   }

   SYMBOL_PUSH(sym, NIL);
   symbol_t *symbol = sym->Object;
   num = NIL;

   if ((start <= end) && (step >= 0)) {
      for (double i = start; i <= end; i += step) {
         symbol->value = NEW_NUMBER(i);
         num = progn(ARG_LIST->Cdr);
      }
   } else if ((start >= end) && (step <= 0)) {
      for (real i = start; i >= end; i += step) {
         symbol->value = NEW_NUMBER(i);
         num = progn(ARG_LIST->Cdr);
      }
   }
   SYMBOL_POP(sym);
   return num;
}

/*
 * (quote a1) returns a1 without evaluation
 */

DX(xquote)
{
   ARG_NUMBER(1);
   at *q = APOINTER(1);
   return q;
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

   argeval_ptr = eval_ptr = eval_std;
   dx_define("eval", xeval);
   dy_define("progn", yprogn);
   dy_define("prog1", yprog1);
   dy_define("apply", yapply);
   dy_define("mapc", ymapc);
   dy_define("mapcar", ymapcar);
   dy_define("mapcan", ymapcan);
   dy_define("let", ylet);
   dy_define("let*", yletS);
   dy_define("for", yfor);
   dx_define("quote", xquote);
   dy_define("debug", ydebug);
   dy_define("nodebug", ynodebug);
   
   at_trace = var_define("trace-hook");
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
