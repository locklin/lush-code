/*   Lush Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
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
 */
 
/***********************************************************************
	T-Lisp3: (C) LYB YLC 1988
	eval.c
	evaluation routines
		EVAL APPLY PROGN
		MAPCAR MAPC RMAPCAR RMAPC
		LIST QUOTE

        $Id: eval.c,v 0.1 2001-10-31 17:30:38 profshadoko Exp $
********************************************************************** */

#include "header.h"


/*
 * eval(p) <=> (*eval_ptr)(p)   (MACRO)
 * 
 * if p is a list extract the first element evaluate it if it is an extern
 * (function,array etc..) result is given by the call of the list_eval
 * function else error else if p is an extern	(symbol,string,...) result is
 * given by the call of the self_eval function else result is p
 * 
 * when eval_ptr is eval_std:	normal mode when eval_ptr is eval_debug:
 * trace mode
 */

at *(*eval_ptr) (at *);
at *(*argeval_ptr) (at *);


at *
eval_std(at *p)
{
  if (p) {
    if (p->flags & C_CONS) {
      at *q;
      q = eval_std(p->Car);
      LOCK(p);
      error_doc.this_call = cons(p,error_doc.this_call);
      argeval_ptr = eval_ptr;
      CHECK_MACHINE("on");

      if (q && (q->flags & C_EXTERN)) {
	p = (*(q->Class->list_eval)) (q, p);
      } else {			
	/* generic_listeval is defined in AT.C */
	p = generic_listeval(q, p);
      }
      UNLOCK(q);
      q = error_doc.this_call;
      error_doc.this_call = q->Cdr;
      q->Cdr = NIL;
      UNLOCK(q);
      return p;
    }
    if (p->flags & C_EXTERN) {
      p = (*(p->Class->self_eval)) (p);
      return p;
    }
  }
  LOCK(p);
  return p;
}


DX(xeval)
{
  int i;
  at *q;

  q = NULL;
  if (arg_number < 1)
    error("eval", "no arguments", NIL);
  for (i = 1; i <= arg_number; i++) {
    ARG_EVAL(i);
    UNLOCK(q);
    q = eval(APOINTER(i));
  }
  return q;
}



/* eval_nothing
 * is used to forbid argument evaluation 
 * when executing DF functions
 */

at *
eval_nothing(at *q)
{
  LOCK(q);
  return q;
}



/* call_trace_hook
 * Calls function trace-hook used for debugging 
 */

static at *at_trace;

static int
call_trace_hook(int tab, char *line,
                at *expr, at *info)
{
  at *call, *ans;
  LOCK(expr);
  LOCK(info);
  call = cons(NEW_NUMBER(tab),
          cons(new_string(line),
           cons(expr, cons(info, NIL))));
  eval_ptr = eval_std;
  ans = apply(at_trace, call);
  UNLOCK(call);
  UNLOCK(ans);
  return (ans != NIL);
}


/* eval_debug 
 * is used by function DEBUG.
 * calls function TRACE-HOOK before and after evaluation
 */

at *
eval_debug(at *q)
{
  int flag;
  int tabsav;
  at *ans;

  /* call hook */
  tabsav = error_doc.debug_tab++;
  flag = call_trace_hook(error_doc.debug_tab, first_line(q),
                         q, error_doc.this_call);
  eval_ptr = argeval_ptr = (flag ? eval_debug : eval_std);
  /* evaluate */
  ans = eval_std(q);
  /* call hook */
  flag = call_trace_hook(-error_doc.debug_tab, first_line(ans),
                         q, ans );
  eval_ptr = argeval_ptr = (flag ? eval_debug : eval_std);
  error_doc.debug_tab = tabsav;
  /* return */
  return ans;
}




/*
 * apply(f,args) C implementation eval(cons(f,arg)) LISP
 * implementation try to care about the function type.
 */

at *
apply(at *q, at *p)
{
  at *result;

  p = new_cons(q, p);
  q = eval(q);
  CHECK_MACHINE("on");

  argeval_ptr = eval_nothing;

  if (q && (q->flags & C_EXTERN))
    result = (*(q->Class->list_eval)) (q, p);
  else
    /* generic_listeval is defined in AT.C */
    /* it performs stacked function search */
    /* added stacked search on 15/7/88 */
    result = generic_listeval(q, p);

  argeval_ptr = eval_ptr;
  UNLOCK(q);
  UNLOCK(p);
  return result;
}

DX(xapply)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return apply(APOINTER(1), APOINTER(2));
}


/*
 * progn(p) evals all the elements of the p list. returns the last evaluation
 * result
 */
at *
progn(at *p)
{
  at *q;

  q = NIL;
  while (CONSP(p)) {
    UNLOCK(q);
    q = eval(p->Car);
    p = p->Cdr;
  }
  if (p)
    error("progn", "why a pointed list ?", NIL);
  return q;
}

DY(yprogn)
{
  return progn(ARG_LIST);
}

/*
 * prog1(p) evals all the elements of the p list. returns the first
 * evaluation result.
 */
at *
prog1(at *p)
{
  at *q, *ans;

  if (CONSP(p)) {
    ans = eval(p->Car);
    p = p->Cdr;
  } else
    ans = NIL;

  while (CONSP(p)) {
    q = eval(p->Car);
    UNLOCK(q);
    p = p->Cdr;
  }
  if (p)
    error("progn", "why a pointed list ?", NIL);

  return ans;
}

DY(yprog1)
{
  return prog1(ARG_LIST);
}



/*
 * mapcar	rmapcar mapc		rmapc
 * 
 * (mapcar f {listes}) build<s the list of the results of the evaluations of the
 * listes (f elem1 elem2 elem3) etc...   and returns it
 * 
 * (mapc f {listes}) returns the result of the last evaluation.
 * 
 * (rmapc f {listes}) (rmapcar f {listes}) idem, but recursively iters in the
 * listes..
 * 
 * RESTRICTION:  limited to MAXARGMAPC arguments.
 */

static int rflag;
static char *mapwhat;

static int 
get_and_shift(at **listes, at **items, int arg_number)
{
  short flag;

  rflag = 1;
  flag = 0;


  while (arg_number--) {
    if (CONSP(*listes) && flag >= 0) {
      *items = (*listes)->Car;
      *listes = (*listes)->Cdr;
      ifn(CONSP(*items))
	rflag = 0;
      flag = 1;
    } else if (*listes)
      error(mapwhat, "bad list as argument", NIL);
    else if (flag <= 0)
      flag = -1;
    else
      error(mapwhat, "lists with different length", NIL);
    listes++;
    items++;
  }

  return (flag > 0) ? TRUE : FALSE;
}


static at *
build_expr(at **args, int arg_number)
{
  at *answer;
  at **where;

  answer = NIL;
  where = &answer;

  while (arg_number--) {
    *where = new_cons(*(args++), NIL);
    where = &((*where)->Cdr);
  }
  return answer;
}



at *
mapc(at *f, at **listes, int arg_number)
{
  at *args[MAXARGMAPC];
  at *toeval;
  at *result;

  mapwhat = "mapc";
  result = NIL;
  while (get_and_shift(listes, args, arg_number)) {
    UNLOCK(result);
    toeval = build_expr(args, arg_number);
    result = apply(f, toeval);
    UNLOCK(toeval);
  }
  return result;
}

DX(xmapc)
{
  at *f, *listes[MAXARGMAPC];
  int i;

  if (arg_number < 2 || arg_number > 2 + MAXARGMAPC)
    DX_ERROR(0, -1);

  ARG_EVAL(1);
  f = APOINTER(1);

  for (i = 2; i <= arg_number; i++) {
    ARG_EVAL(i);
    listes[i - 2] = ALIST(i);
  }

  return mapc(f, listes, arg_number - 1);
}


at *
rmapc(at *f, at **listes, int arg_number)
{
  at *args[MAXARGMAPC];
  at *toeval;
  at *result;

  mapwhat = "rmapc";
  result = NIL;
  while (get_and_shift(listes, args, arg_number)) {
    UNLOCK(result);
    if (rflag)
      result = rmapc(f, args, arg_number);
    else {
      toeval = build_expr(args, arg_number);
      result = apply(f, toeval);
      UNLOCK(toeval);
    }
  }
  return result;
}

DX(xrmapc)
{
  at *f, *listes[MAXARGMAPC];
  int i;

  if (arg_number < 2 || arg_number > 2 + MAXARGMAPC)
    DX_ERROR(0, -1);

  ARG_EVAL(1);
  f = APOINTER(1);

  for (i = 2; i <= arg_number; i++) {
    ARG_EVAL(i);
    listes[i - 2] = ALIST(i);
  }

  return rmapc(f, listes, arg_number - 1);
}


at *
mapcar(at *f, at **listes, int arg_number)
{
  at *args[MAXARGMAPC];
  at *toeval, **where;
  at *answer;

  answer = NIL;
  where = &answer;
  mapwhat = "mapcar";
  while (get_and_shift(listes, args, arg_number)) {
    toeval = build_expr(args, arg_number);
    *where = cons(apply(f, toeval), NIL);
    where = &((*where)->Cdr);
    UNLOCK(toeval);
  }
  return answer;
}

DX(xmapcar)
{
  at *f, *listes[MAXARGMAPC];
  int i;

  if (arg_number < 2 || arg_number > 2 + MAXARGMAPC)
    DX_ERROR(0, -1);

  ARG_EVAL(1);
  f = APOINTER(1);

  for (i = 2; i <= arg_number; i++) {
    ARG_EVAL(i);
    listes[i - 2] = ALIST(i);
  }

  return mapcar(f, listes, arg_number - 1);
}


at *
rmapcar(at *f, at **listes, int arg_number)
{
  at *args[MAXARGMAPC];
  at *toeval, **where;
  at *answer;

  answer = NIL;
  where = &answer;
  mapwhat = "rmapcar";
  while (get_and_shift(listes, args, arg_number)) {
    if (rflag)
      *where = cons(rmapcar(f, args, arg_number), NIL);
    else {
      toeval = build_expr(args, arg_number);
      *where = cons(apply(f, toeval), NIL);
      UNLOCK(toeval);
    }
    where = &((*where)->Cdr);
  }
  return answer;
}

DX(xrmapcar)
{
  at *f, *listes[MAXARGMAPC];
  int i;

  if (arg_number < 2 || arg_number > 2 + MAXARGMAPC)
    DX_ERROR(0, -1);

  ARG_EVAL(1);
  f = APOINTER(1);

  for (i = 2; i <= arg_number; i++) {
    ARG_EVAL(i);
    listes[i - 2] = ALIST(i);
  }

  return rmapcar(f, listes, arg_number - 1);
}


/* let, let*, for, each, all 
 */

static char orthoerr[] = "syntax error in variable declaration part";

static void 
orthogonalize(at *q, at **l1, at **l2)
{
  at **where1, **where2;
  at *pair;

  *l1 = *l2 = NIL;
  where1 = l1;
  where2 = l2;

  while (CONSP(q)) {
    pair = q->Car;
    q = q->Cdr;

    ifn(CONSP(pair) && CONSP(pair->Cdr) && !pair->Cdr->Cdr)
      error(NIL, orthoerr, NIL);

    *where1 = new_cons(pair->Car, NIL);
    *where2 = cons(eval(pair->Cdr->Car), NIL);
    where1 = &((*where1)->Cdr);
    where2 = &((*where2)->Cdr);
  }
  if (q)
    error(NIL, orthoerr, NIL);
}

DY(ylet)
{
  at *function, *result;
  at *l1, *l2;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'let' construction", NIL);

  orthogonalize(ARG_LIST->Car, &l1, &l2);
  function = new_df(l1, ARG_LIST->Cdr);
  result = apply(function, l2);
  UNLOCK(l1);
  UNLOCK(l2);
  UNLOCK(function);
  return result;
}

DY(yletstar)
{
  at *p, *q, *ans, *sym;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'let*' construction", NIL);

  for (q = ARG_LIST->Car; CONSP(q); q = q->Cdr) {
    p = q->Car;
    ifn(CONSP(p) && CONSP(p->Cdr) && !p->Cdr->Cdr)
      error("let*", orthoerr, NIL);
    ifn((sym = p->Car) && (sym->flags & X_SYMBOL))
      error("let*", "symbol expected, found", sym);
    ans = eval(p->Cdr->Car);
    symbol_push(sym, ans);
    UNLOCK(ans);
  }
  if (q)
    error("let*", orthoerr, NIL);
  ans = progn(ARG_LIST->Cdr);
  for (q = ARG_LIST->Car; q; q = q->Cdr)
    symbol_pop(q->Car->Car);

  return ans;
}

DY(yfor)
{
  at *num = NIL;
  struct symbol *symbol;
  at *p, *sym;
  real i;
  real start, end, step;

  static char forerr[] = "bad 'for' syntax";
  static char fornum[] = "not a number";

  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Car))
    error(NIL, forerr, NIL);
  p = ARG_LIST->Car;
  ifn((sym = p->Car) && (sym->flags & X_SYMBOL))
    error("for", "not a symbol", sym);
  p = p->Cdr;
  ifn(CONSP(p) && (num = eval(p->Car)) && (num->flags & C_NUMBER))
    error("for", fornum, num);
  start = num->Number;
  UNLOCK(num);
  p = p->Cdr;
  ifn(CONSP(p) && (num = eval(p->Car)) && (num->flags & C_NUMBER))
    error("for", fornum, num);
  end = num->Number;
  UNLOCK(num);
  p = p->Cdr;
  if (CONSP(p) && !p->Cdr) {
    ifn(CONSP(p) && (num = eval(p->Car)) && (num->flags & C_NUMBER))
      error("for", fornum, num);
    step = num->Number;
    UNLOCK(num);
  } else if (p)
    error(NIL, forerr, p);
  else
    step = ( start < end ) ? 1.0 : -1.0;


  symbol_push(sym, NIL);
  symbol = sym->Object;
  num = NIL;

  if ((start <= end) && (step >= 0)) {
    for (i = start; i <= end; i += step) {
      UNLOCK(num);
      UNLOCK(symbol->value);
      symbol->value = NEW_NUMBER(i);
      num = progn(ARG_LIST->Cdr);
    }
  } else if ((start >= end) && (step <= 0)) {
    for (i = start; i >= end; i += step) {
      UNLOCK(num);
      UNLOCK(symbol->value);
      symbol->value = NEW_NUMBER(i);
      num = progn(ARG_LIST->Cdr);
    }
  }
  symbol_pop(sym);
  return num;
}


DY(yeach)
{
  int narg;
  at *args[MAXARGMAPC];
  at *l1, *l2, *result;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'each' construction", NIL);

  {
    at **array, *alist;

    orthogonalize(ARG_LIST->Car, &l1, &l2);
    array = args;
    alist = l2;
    narg = 0;
    while (alist) {
      if (narg++ >= MAXARGMAPC)
	error(NIL, "Too many variables", NIL);
      *array++ = alist->Car;
      alist = alist->Cdr;
    }
  }
  {
    at *function;

    function = new_df(l1, ARG_LIST->Cdr);
    UNLOCK(l1);
    result = mapc(function, args, narg);
    UNLOCK(l2);
    UNLOCK(function);
    return result;
  }
}


DY(yall)
{
  int narg;
  at *args[MAXARGMAPC];
  at *l1, *l2, *result;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'all' construction", NIL);

  {
    at **array, *alist;

    orthogonalize(ARG_LIST->Car, &l1, &l2);
    array = args;
    alist = l2;
    narg = 0;
    while (alist) {
      if (narg++ >= MAXARGMAPC)
	error(NIL, "Too many variables", NIL);
      *array++ = alist->Car;
      alist = alist->Cdr;
    }
  }
  {
    at *function;

    function = new_df(l1, ARG_LIST->Cdr);
    UNLOCK(l1);
    result = mapcar(function, args, narg);
    UNLOCK(l2);
    UNLOCK(function);
    return result;
  }
}


/*
 * (list a1 a2 a3 a4 ) returns the list of the evaluated aN this is a simple
 * call to the routine EVAL_A_LIST defined in FUNCTION.C
 */

DY(ylist)
{
  extern at *eval_a_list(at *p);

  return eval_a_list(ARG_LIST);
}

/*
 * (quote a1) returns a1 without evaluation
 */

DX(xquote)
{
  at *q;

  ARG_NUMBER(1);
  q = APOINTER(1);
  LOCK(q);
  return q;
}


/*
 * (debug ()) or (debug n) set or reset debug mode
 */
DY(ydebug)
{
  at *(*sav_ptr) (at *);
  at *ans;

  sav_ptr = eval_ptr;
  eval_ptr = eval_debug;
  ans = progn(ARG_LIST);
  eval_ptr = sav_ptr;
  return ans;
}


/*
 * (nodebug ......) evals listes without debugging them
 */

DY(ynodebug)
{
  at *(*sav_ptr) (at *);
  at *ans;

  sav_ptr = eval_ptr;
  eval_ptr = eval_std;
  ans = progn(ARG_LIST);
  eval_ptr = sav_ptr;
  return ans;
}

/* --------- INITIALISATION CODE --------- */

void 
init_eval(void)
{
  argeval_ptr = eval_ptr = eval_std;

  dx_define("eval", xeval);
  dy_define("progn", yprogn);
  dy_define("prog1", yprog1);
  dx_define("apply", xapply);
  dx_define("mapc", xmapc);
  dx_define("rmapc", xrmapc);
  dx_define("mapcar", xmapcar);
  dx_define("rmapcar", xrmapcar);
  dy_define("let", ylet);
  dy_define("let*", yletstar);
  dy_define("for", yfor);
  dy_define("each", yeach);
  dy_define("all", yall);
  dy_define("list", ylist);
  dx_define("quote", xquote);
  dy_define("debug", ydebug);
  dy_define("nodebug", ynodebug);

  at_trace = var_define("trace-hook");
}
