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
 * $Id: function.c,v 1.5 2002-05-07 00:14:20 leonb Exp $
 **********************************************************************/


#include "header.h"

static at *at_optional, *at_rest;
static void parse_optional_stuff(at *formal_list, at *real_list);


static struct alloc_root cfunc_alloc = {
  NULL, NULL, sizeof(struct cfunction), 200 
};

static struct alloc_root lfunc_alloc = {
  NULL, NULL, sizeof(struct lfunction), 200 
};


/* General cfunc routines -----------------------------	 */


static void 
cfunc_dispose(at *p)
{
  struct cfunction *func = p->Object;
  UNLOCK(func->name);
  deallocate(&cfunc_alloc, (struct empty_alloc *) func);
}

static void 
cfunc_action(at *p, void (*action) (at *p))
{
  struct cfunction *func = p->Object;
  (*action) (func->name);
}

static char *
cfunc_name(at *p)
{
  struct cfunction *func = p->Object;
  at *name = func->name;
  if (CONSP(name))
    name = name->Cdr;
  if (! EXTERNP(name, &symbol_class))
    return generic_name(p);
  sprintf(string_buffer, "::%s:", nameof(p->Class->classname));
  strcat(string_buffer, nameof(name));
  return string_buffer;
}


/* General lfunc routines -----------------------------	 */


static void 
lfunc_dispose(at *p)
{
  struct lfunction *func = p->Object;
  UNLOCK(func->formal_arg_list);
  UNLOCK(func->evaluable_list);
  deallocate(&lfunc_alloc, (struct empty_alloc *) func);
}

static void 
lfunc_action(at *p, void (*action) (at *p))
{
  struct lfunction *func = p->Object;
  (*action) (func->formal_arg_list);
  (*action) (func->evaluable_list);
}


static void
pop_args(at *formal_list)
{
  while (CONSP(formal_list)) {
    pop_args(formal_list->Car);
    formal_list = formal_list->Cdr;
  }
  if (formal_list && (formal_list->flags & X_SYMBOL))
    /* symbol_pop(formal_list);			 */
    /* WE POP IT INLINE				 */
  {
    extern struct alloc_root symbol_alloc;
    struct symbol *symb;

    symb = (struct symbol *) (formal_list->Object);
    if (symb->next) {
      formal_list->Object = symb->next;
      UNLOCK(symb->value);
      deallocate(&symbol_alloc, (struct empty_alloc *) symb);
    }
  }
}


static void
push_args(at *formal_list, at *real_list)
{
  /* fast non tail-recursive loop for parsing the trees */
  
  while (CONSP(formal_list) && 
	 CONSP(real_list) && 
	 formal_list->Car!=at_rest &&
	 formal_list->Car!=at_optional) {
    push_args(formal_list->Car, real_list->Car);
    real_list = real_list->Cdr;
    formal_list = formal_list->Cdr;
  };
  
  /* parsing a single symbol in a tree */
  
  if (formal_list) {
    
    if (formal_list->flags & X_SYMBOL) {
      symbol_push( formal_list, real_list );
      return;
      
    } else if ((formal_list->flags&C_CONS) && 
	       (formal_list->Car==at_optional) ) {
      parse_optional_stuff(formal_list,real_list);
      return;
      
    } else if ((formal_list->flags&C_CONS) && 
	       (formal_list->Car==at_rest) ) {
      parse_optional_stuff(formal_list,real_list);
      return;
      
    } else
      error(NIL,"missing arguments",formal_list);
  }

  if (real_list) {
    error(NIL, "too many arguments", real_list);
  }
}


static void 
parse_optional_stuff(at *formal_list, at *real_list)
{
  if (CONSP(formal_list) && formal_list->Car == at_optional) {

    push_args(at_optional,NIL);
    formal_list = formal_list->Cdr;
    
    while (CONSP(formal_list) && formal_list->Car!=at_rest) {
      if (CONSP(formal_list) 
	  && formal_list->Car
	  && (formal_list->Car->flags&X_SYMBOL)) {
	
	if (CONSP(real_list)) {
	  push_args(formal_list->Car,real_list->Car);
	  real_list = real_list->Cdr;
	} else {
	  push_args(formal_list->Car,NIL);
	}
	
      } else if (CONSP(formal_list) 
		 && CONSP(formal_list->Car)
		 && formal_list->Car->Car
		 && (formal_list->Car->Car->flags&X_SYMBOL)
		 && CONSP(formal_list->Car->Cdr)
		 && formal_list->Car->Cdr->Cdr == NIL ) {
	if (CONSP(real_list)) {
	  push_args(formal_list->Car->Car,real_list->Car);
	  real_list = real_list->Cdr;
	} else {
	  push_args(formal_list->Car->Car,formal_list->Car->Cdr->Car);
	}
	
      } else
	error(NIL,"error in &optional syntax",formal_list);

      formal_list = formal_list->Cdr;
    } 
  }
  if (CONSP(formal_list) && formal_list->Car==at_rest) {
    push_args(at_rest,NIL);
    formal_list=formal_list->Cdr;
    if (CONSP(formal_list)
	&& formal_list->Cdr==NIL
	&& formal_list->Car
	&& (formal_list->Car->flags&X_SYMBOL) ) {
      push_args(formal_list->Car,real_list);
      return;
    } else
      error(NIL,"error in &rest syntax",formal_list);
  }
  
  if (formal_list)
    error(NIL,"error in &optional syntax",formal_list);		
  if (real_list)
    error(NIL,"too many arguments",real_list);
}


at *
eval_a_list(at *p)
{
  at **now;
  at *list;

  list = p;
  p = NIL;
  now = &p;

  while (CONSP(list)) {
    *now = cons((*argeval_ptr) (list->Car), NIL);
    now = &((*now)->Cdr);
    list = list->Cdr;
  }
  if (list)
    *now = eval(list);
  return p;
}


/* DX class -------------------------------------------	 */



at **dx_stack;
at **dx_sp;

at *
dx_listeval(at *p, at *q)
{
  int arg_num;
  at *answer, **arg_pos, **spbuff;
  struct cfunction *cfunc = p->Object;
  at *(*call)(int, at**) = cfunc->call;
  check_primitive(p);
  cfunc = p->Object;
  arg_pos = spbuff = dx_sp;
  arg_num = 0;
  q = q->Cdr;
  while (CONSP(q)) {
    arg_num++;
    if (++spbuff >= dx_stack + DXSTACKSIZE)
      error(NIL, "sorry, stack full (Merci Yann)", NIL);
    *spbuff = q->Car;
    LOCK(q->Car);
    q = q->Cdr;
  }
  if (q)
    error("dx", "Bad argument list", NIL);
  
  dx_sp = spbuff;
  answer = call(arg_num, arg_pos);
  while (arg_pos < spbuff) {
    UNLOCK(*spbuff);
    spbuff--;
  }
  dx_sp = spbuff;
  if (answer && (answer->flags&X_ZOMBIE)) {
    UNLOCK(answer);
    return NIL;
  } else
    return answer;
}

class dx_class =
{
  cfunc_dispose,
  cfunc_action,
  cfunc_name,
  generic_eval,
  dx_listeval,
};

at *
new_dx(at *name, at *(*addr)(int, at **))
{
  struct cfunction *cfunc;
  at *p;
  
  cfunc = allocate(&cfunc_alloc);
  cfunc->call = cfunc->info = addr;
  LOCK(name);
  cfunc->name = name;
  p = new_extern(&dx_class, cfunc);
  p->flags |= X_FUNCTION;
  return p;
}


/* DY class -------------------------------------------	 */

at *
dy_listeval(at *p, at *q)
{
  struct cfunction *cfunc = p->Object;
  at *(*call)(at*) = cfunc->call;
  check_primitive(p);
  q = (*call)(q->Cdr);
  if (q && (q->flags&X_ZOMBIE)) {
    UNLOCK(q);
    return NIL;
  } else
    return q;
}

class dy_class =
{
  cfunc_dispose,
  cfunc_action,
  cfunc_name,
  generic_eval,
  dy_listeval,
};

at *
new_dy(at *name, at *(*addr)(at *))
{
  struct cfunction *cfunc;
  at *p;
  
  cfunc = allocate(&cfunc_alloc);
  LOCK(name);
  cfunc->name = name;
  cfunc->call = cfunc->info = addr;
  p = new_extern(&dy_class, cfunc);
  p->flags |= X_FUNCTION;
  return p;
}


/* DE class -------------------------------------------	 */

at *
de_listeval(at *p, at *q)
{
  struct lfunction *lfunc;
  at *answer;

  lfunc = p->Object;
  q = eval_a_list(q->Cdr);
  push_args(lfunc->formal_arg_list, q);
  UNLOCK(q);
  answer = progn(lfunc->evaluable_list);
  pop_args(lfunc->formal_arg_list);
  return answer;
}


class de_class =
{
  lfunc_dispose,
  lfunc_action,
  generic_name,
  generic_eval,
  de_listeval,
};


at *
new_de(at *formal, at *evaluable)
{
  struct lfunction *lfunc;
  at *p;

  lfunc = allocate(&lfunc_alloc);
  lfunc->formal_arg_list = formal;
  LOCK(formal);
  lfunc->evaluable_list = evaluable;
  LOCK(evaluable);
  p = new_extern(&de_class, lfunc);
  p->flags |= X_FUNCTION;
  return p;
}

DY(ylambda)
{
  at *q;
  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr))
    error("lambda", "illegal definition of function", NIL);
  return new_de(q->Car, q->Cdr);
}

DY(yde)
{
  at *q, *symbol;
  at *func;

  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr) && CONSP(q->Cdr->Cdr))
    error("de", "illegal definition of function", NIL);
  symbol = q->Car;
  ifn(symbol && (symbol->flags & X_SYMBOL))
    error("de", "not a symbol", symbol);
  func = new_de(q->Cdr->Car, q->Cdr->Cdr);
  var_set(symbol, func);
  UNLOCK(func);
  LOCK(symbol);
  return symbol;
}


/* DF class -------------------------------------------	 */

at *
df_listeval(at *p, at *q)
{
  struct lfunction *lfunc;
  at *answer;

  lfunc = p->Object;
  push_args(lfunc->formal_arg_list, q->Cdr);
  answer = progn(lfunc->evaluable_list);
  pop_args(lfunc->formal_arg_list);
  return answer;
}

class df_class =
{
  lfunc_dispose,
  lfunc_action,
  generic_name,
  generic_eval,
  df_listeval,
};

at *
new_df(at *formal, at *evaluable)
{
  struct lfunction *lfunc;
  at *p;

  lfunc = allocate(&lfunc_alloc);
  lfunc->formal_arg_list = formal;
  LOCK(formal);
  lfunc->evaluable_list = evaluable;
  LOCK(evaluable);
  p = new_extern(&df_class, lfunc);
  p->flags |= X_FUNCTION;
  return p;
}

DY(yflambda)
{
  at *q;
  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr))
    error("flambda", "illegal definition of function", NIL);
  return new_df(q->Car, q->Cdr);
}

DY(ydf)
{
  at *q, *symbol;
  at *func;

  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr) && CONSP(q->Cdr->Cdr))
    error("df", "illegal definition of function", NIL);
  symbol = q->Car;
  ifn(symbol && (symbol->flags & X_SYMBOL))
    error("df", "not a symbol", symbol);
  func = new_df(q->Cdr->Car, q->Cdr->Cdr);
  var_set(symbol, func);
  UNLOCK(func);
  LOCK(symbol);
  return symbol;
}

/* DM class -------------------------------------------	 */


at *
dm_listeval(at *p, at *q)
{
  struct lfunction *lfunc;
  lfunc = p->Object;
  push_args(lfunc->formal_arg_list, q);
  q = progn(lfunc->evaluable_list);
  pop_args(lfunc->formal_arg_list);
  p = eval(q);
  UNLOCK(q);
  return p;
}

class dm_class =
{
  lfunc_dispose,
  lfunc_action,
  generic_name,
  generic_eval,
  dm_listeval,
};


at *
new_dm(at *formal, at *evaluable)
{
  struct lfunction *lfunc;
  at *p;

  lfunc = allocate(&lfunc_alloc);
  lfunc->formal_arg_list = formal;
  LOCK(formal);
  lfunc->evaluable_list = evaluable;
  LOCK(evaluable);
  p = new_extern(&dm_class, lfunc);
  p->flags |= X_FUNCTION;
  return p;
}

DY(ymlambda)
{
  at *q;
  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr))
    error("mlambda", "illegal definition of function", NIL);
  return new_dm(q->Car, q->Cdr);
}

DY(Ydm)
{
  at *q, *symbol;
  at *func;

  q = ARG_LIST;
  ifn(CONSP(q) && CONSP(q->Cdr) && CONSP(q->Cdr->Cdr))
    error("dm", "illegal definition of function", NIL);
  symbol = q->Car;
  ifn(symbol && (symbol->flags & X_SYMBOL))
    error("dm", "not a symbol", symbol);
  func = new_dm(q->Cdr->Car, q->Cdr->Cdr);
  var_set(symbol, func);
  UNLOCK(func);
  LOCK(symbol);
  return symbol;
}

at *
dm_macro_expand(at *p, at *q)
{
  struct lfunction *lfunc;
  at *before_eval;

  ifn(p && (p->flags & C_EXTERN) && p->Class == &dm_class)
    error("macro_expand", "not a macro", p);
  lfunc = p->Object;
  push_args(lfunc->formal_arg_list, q);
  before_eval = progn(lfunc->evaluable_list);
  pop_args(lfunc->formal_arg_list);
  return before_eval;
}

DY(ymacro_expand)
{
  at *p, *ans;
  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Car) && (!ARG_LIST->Cdr))
    error(NIL, "syntax error", NIL);
  p = eval(ARG_LIST->Car->Car);
  ans = dm_macro_expand(p, ARG_LIST->Car);
  UNLOCK(p);
  return ans;
}


/* General purpose routines -------------------	 */

/*
 * function definition extraction given a lisp function, builds the list  (X
 * ARG  {..LST..} ) where  X is   'lambda', 'flambda', 'mlambda', or   'de
 * XX' , 'df XX',   'dm XX', if a name is found, ARG is the formal arguments
 * list, LST is the definition of the function.
 */

at *
funcdef(at *f)
{
  struct lfunction *func;
  char *s = 0;

  if (EXTERNP(f, &de_class))
    s = "lambda";
  if (EXTERNP(f, &df_class))
    s = "flambda";
  if (EXTERNP(f, &dm_class))
    s = "mlambda";
  if (! s)
    return NIL;
  func = f->Object;
  return cons(named(s),
              new_cons(func->formal_arg_list, 
                       func->evaluable_list) );
}

DX(xfuncdef)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return funcdef(APOINTER(1));
}


DX(xfunctionp)
{
  at *p;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p && (p->flags & X_FUNCTION)) {
    LOCK(p);
    return p;
  } else
    return NIL;
}

/*
 * error routines: need_error... implanted in the macros  APOINTER & co
 */

static char need_arg_num[80];
static char *need_errlist[] =
{ "bad argument number",
  "not a number",
  "not a list",
  "empty list",
  "not a string",
  "not a cell",
  "not a clist",
  "not a symbol",
  "not an external object",
  "not a nlf descriptor",
  "not an index"
};


gptr 
need_error(int i, int j, at **arg_array_ptr)
{
  char *s2;
  at *p;

  s2 = need_errlist[i];
  p = NIL;
  if (i)
    p = arg_array_ptr[j];
  else if (j > 1) {
    sprintf(need_arg_num, "%d arguments were expected", j);
    s2 = need_arg_num;
  } else if (j == 1)
    s2 = "one argument was expected";
  else if (j == 0)
    s2 = "no arguments expected";

  ifn(p && p->count)
    p = NIL;
  error(NIL, s2, p);
  return NIL;
}



void 
arg_eval(at **arg_array, int i)
{
  at *temp;

  temp = arg_array[i];
  arg_array[i] = (*argeval_ptr) (temp);
  UNLOCK(temp);
}

void 
all_args_eval(at **arg_array, int i)
{
  at *temp;

  while (i) {
    temp = *++arg_array;
    *arg_array = (*argeval_ptr) (temp);
    UNLOCK(temp);
    i--;
  }
}

/* WARNING: AUTOMATIC PROGRAM GENERATION */

void 
init_function(void)
{
  if (! (dx_stack = malloc(sizeof(at *) * DXSTACKSIZE)))
    abort("Not enough memory");

  class_define("DX",&dx_class );
  class_define("DY",&dy_class );
  class_define("DE",&de_class );
  class_define("DF",&df_class );
  class_define("DM",&dm_class );

  at_optional = var_define("&optional");
  at_rest = var_define("&rest");

  dy_define("lambda", ylambda);
  dy_define("de", yde);
  dy_define("flambda", yflambda);
  dy_define("df", ydf);
  dy_define("mlambda", ymlambda);
  dy_define("dm", Ydm);
  dy_define("macro_expand", ymacro_expand);
  dx_define("funcdef", xfuncdef);
  dx_define("functionp", xfunctionp);
}
