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
 * $Id: at.c,v 1.3 2002-06-27 20:49:57 leonb Exp $
 **********************************************************************/

/***********************************************************************
  Definition of the AT object
  General use routines as CONS, NUMBER, EXTERNAL, CAR, CDR etc...
********************************************************************** */

#include "header.h"


struct alloc_root at_alloc = {
  NIL,
  NIL,
  sizeof(at),
  CONSCHUNKSIZE,
};

/*
 * purge(q) It's the function called by the macro UNLOCK. This function is
 * critical for speed. So we don't use 'deallocate'.
 */


void 
purge(at *q)
{
  register at *h;

purge_loop:

  if (q->flags & C_FINALIZER) 
    run_finalizers(q);

  if (q->flags & C_CONS) 
    {
      if ((h = q->Car))
        if (! (--(h->count)))
          purge(h);
      h = q->Cdr;
      
      ((struct empty_alloc *) q)->next = at_alloc.freelist;
      at_alloc.freelist = (struct empty_alloc *) q;
      
      if (h && !(--(h->count))) 
        { 
          q = h;
          goto purge_loop;
        }
    } 
  else if (q->flags & C_EXTERN) 
    {
      (*q->Class->self_dispose) (q);
      ((struct empty_alloc *) q)->next = at_alloc.freelist;
      at_alloc.freelist = (struct empty_alloc *) q;
    } 
  else
    {
      ((struct empty_alloc *) q)->next = at_alloc.freelist;
      at_alloc.freelist = (struct empty_alloc *) q;
    }
}



/*
 * new_cons(car,cdr)
 * 
 * It's the C equivalent of the usual 'cons' function. This call is critical for
 * performance, so we use 'allocate' only if we have to allocate a chunk.
 */

at *
cons(at *car, at *cdr)			/* CONS: you have to LOCK car and cdr */
{
  register at *new;

  if ((new = (at *) at_alloc.freelist))
    at_alloc.freelist = ((struct empty_alloc *) new)->next;
  else
    new = allocate(&at_alloc);

  new->flags = C_CONS;
  new->count = 1;
  new->Car = car;
  new->Cdr = cdr;

  return new;
}

at *
new_cons(at *car, at *cdr)		/* NEW_CONS: LOCKs car and cdr		 */
{
  register at *new;

  if ((new = (at *) at_alloc.freelist))
    at_alloc.freelist = ((struct empty_alloc *) new)->next;
  else
    new = allocate(&at_alloc);

  new->flags = C_CONS;
  new->count = 1;
  LOCK(car);
  LOCK(cdr);
  new->Car = car;
  new->Cdr = cdr;

  return new;
}

DX(xcons)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return new_cons(APOINTER(1), APOINTER(2));
}


/*
 * new_number(x) returns a LISP number for the real x. NEW_NUMBER(x) idem.
 * This macro includes the conversion of x to a real.
 */

at *
new_number(double x)
{
  register at *new;

  new = allocate(&at_alloc);
  new->flags = C_NUMBER;
  new->count = 1;
  new->Number = x;

  return new;
}

/*
 * new_gptr(x) returns a LISP gptr for the real x. NEW_GPTR(x) idem.
 * This macro includes the conversion of x to a real.
 */

at * 
new_gptr(gptr x)
{
  register at *new;

  new = allocate(&at_alloc);
  new->flags = C_GPTR;
  new->count = 1;
  new->Gptr = x;

  return new;
}



/*
 * This strange function allows us to compute the 'simulated
 * bumping list' during a progn. This is used for the interpreted
 * 'in-pool' and 'in-stack' emulation.
 */

int compute_bump_active = 0;
static at *compute_bump_list = 0;

DY(ycompute_bump)
{
    at *ans;
    at *bump;
    at *temp;
    at **where;
    int flag;
    at *sav_compute_bump_list = compute_bump_list;
    int sav_compute_bump_active = compute_bump_active;
    
    /* execute program in compute_bump mode */
    compute_bump_active = 1;
    compute_bump_list = NIL;
    ans = progn(ARG_LIST);
    bump = compute_bump_list;
    compute_bump_list = sav_compute_bump_list;
    compute_bump_active = sav_compute_bump_active;

    /* remove objects with trivial count */
    flag = 1;
    while (flag)
    {
        flag = 0;
        where = &bump;
        while (CONSP(*where))
        {
            if ((*where)->Car && (*where)->Car->count>1)
            {
                where = &((*where)->Cdr);
            } 
            else
            {
                flag = 1;
                temp = *where;
                *where = (*where)->Cdr;
                temp->Cdr = NIL;
                UNLOCK(temp);
            }
        }
    }
    /* return everything */
    return cons(ans, bump);
}


/*
 * new_extern(class,object) returns a LISP descriptor for an EXTERNAL object
 * of class class
 */

at *
new_extern(TLclass *cl, void *obj)
{
  register at *new;

  new = allocate(&at_alloc);
  new->flags = C_EXTERN;
  new->count = 1;
  new->Class = cl;
  new->Object = obj;
  /* This is used by 'compute_bump' */
  if (compute_bump_active) {
    compute_bump_list = cons(new,compute_bump_list);
    LOCK(new);
  }
  return new;
}



/* ----- misc list operations ------ */

at *
car(at *q)
{
  ifn(q)
    return NIL;
  ifn(q->flags & C_CONS)
    error(NIL, "not a list", q);
  LOCK(q->Car);
  return q->Car;
}

DX(xcar)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return car(ALIST(1));
}

at *
cdr(at *q)
{
  ifn(q)
    return NIL;
  ifn(q->flags & C_CONS)
    error(NIL, "not a list", q);
  LOCK(q->Cdr);
  return q->Cdr;
}

DX(xcdr)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return cdr(ALIST(1));
}

at *
caar(at *q)
{
  register at *p;

  p = car(q);
  q = car(p);
  UNLOCK(p);
  return q;
}

DX(xcaar)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return caar(ALIST(1));
}

at *
cadr(at *q)
{
  register at *p;

  p = cdr(q);
  q = car(p);
  UNLOCK(p);
  return q;
}

DX(xcadr)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return cadr(ALIST(1));
}

at *
cdar(at *q)
{
  register at *p;

  p = car(q);
  q = cdr(p);
  UNLOCK(p);
  return q;
}

DX(xcdar)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return cdar(ALIST(1));
}

at *
cddr(at *q)
{
  register at *p;

  p = cdr(q);
  q = cdr(p);
  UNLOCK(p);
  return q;
}

DX(xcddr)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return cddr(ALIST(1));
}

at *
rplaca(at *q, at *p)
{
  at *t;
  if (q && (q->flags & C_CONS)) {
    t = q->Car;
    LOCK(p);
    q->Car = p;
    UNLOCK(t);
  } else
    error("rplaca", "not a cons", q);
  LOCK(q);
  return q;
}

DX(xrplaca)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return rplaca(APOINTER(1), APOINTER(2));
}

at *
rplacd(at *q, at *p)
{
  at *t;
  if (q && (q->flags & C_CONS)) {
    t = q->Cdr;
    LOCK(p);
    q->Cdr = p;
    UNLOCK(t);
  } else
    error("rplacd", "not a cons", q);
  LOCK(q)
    return q;
}

DX(xrplacd)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return rplacd(APOINTER(1), APOINTER(2));
}

at *
displace(at *q, at *p)
{
  at *t1, *t2;
  if (q && (q->flags & C_CONS)) {
    t1 = q->Car;
    t2 = q->Cdr;
    LOCK(p->Car);
    LOCK(p->Cdr);
    q->Car = p->Car;
    q->Cdr = p->Cdr;
    UNLOCK(t1);
    UNLOCK(t2);
  } else
    error("displace", "not a cons", q);
  LOCK(q);
  return q;
}

DX(xdisplace)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return displace(APOINTER(1), APOINTER(2));
}

DX(xlistp)
{
  register at *q;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  if (!q)
    return true();
  else if (q->flags & C_CONS) {
    LOCK(q);
    return q;
  } else
    return NIL;
}

DX(xconsp)
{
  register at *q;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  if (CONSP(q)) {
    LOCK(q);
    return q;
  } else
    return NIL;
}

DX(xatomp)
{
  register at *q;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  q = APOINTER(1);
  if (!q)
    return true();
  else
    ifn(q->flags & C_CONS) {
    LOCK(q);
    return q;
    }
  else
  return NIL;
}

static at *
numberp(at *q)
{
  if (q && (q->flags & C_NUMBER)) {
    LOCK(q);
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

static at *
externp(at *q)
{
  if (q && (q->flags & C_EXTERN)) {
    LOCK(q);
    return q;
  } else
    return NIL;
}

DX(xexternp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return externp(APOINTER(1));
}

static at *
null(at *q)
{
  ifn(q)
    return true();
  else
    return NIL;
}

DX(xnull)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return null(APOINTER(1));
}


int 
length(at *p)
{
  at *q = p;
  int i = 0;
  while (CONSP(p)) 
  {
    i += 1;
    p = p->Cdr;
    if (p == q)
      return -1;
    if (i & 1)
      q = q->Cdr;
  }
  return i;
}

DX(xlength)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(length(ALIST(1)));
}


at *
last(register at *list)
{
  if (CONSP(list)) {
    int toggle = 1;
    at *q = list;
    while (CONSP(list->Cdr)) {
      list = list->Cdr;
      if (list == q)
        error(NIL,"circular list has no last element",NIL);
      toggle ^= 1;
      if (toggle)
        q = q->Cdr;
    }
    if (list->Cdr) {
      LOCK(list->Cdr);
      return list->Cdr;
    } else {
      LOCK(list->Car);
      return list->Car;
    }
  } else
    return NIL;
}

DX(xlast)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return last(ALIST(1));
}


at *
lastcdr(register at *list)
{
  if (CONSP(list)) {
    at *q = list;
    int toggle = 0;
    while (CONSP(list->Cdr)) {
      list = list->Cdr;
      if (list == q)
        error(NIL,"circular list has no last element",NIL);
      toggle ^= 1;
      if (toggle)
        q = q->Cdr;
    }
    LOCK(list);
    return list;
  } else
    return NIL;
}

DX(xlastcdr)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return lastcdr(ALIST(1));
}


at *
member(at *elem, register at *list)
{
  at *q = list;
  int toggle = 0;
  while (CONSP(list)) {
    if (eq_test(list->Car, elem)) 
    {
      LOCK(list);
      return list;
    }
    list = list->Cdr;
    if (list == q)
      break;
    toggle ^= 1;
    if (toggle)
      q = q->Cdr;
  }
  return NIL;
}

DX(xmember)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return member(APOINTER(1), ALIST(2));
}


at *
append(register at *l1, at *l2)
{
  at *answer = NIL;
  register at **where = &answer;
  at *q = l1;
  int toggle = 0;

  while (CONSP(l1)) {
    *where = new_cons(l1->Car, NIL);
    where = &((*where)->Cdr);
    l1 = l1->Cdr;
    if (l1 == q)
      error(NIL,"Cannot append after a circular list",NIL);
    toggle ^= 1;
    if (toggle)
      q = q->Cdr;
  }
  LOCK(l2);
  *where = l2;
  return answer;
}

DX(xappend)
{
  register at *last, *old;
  register int i;

  ALL_ARGS_EVAL;
  if (arg_number == 0)
    return NIL;

  i = arg_number;
  last = APOINTER(i);
  LOCK(last);

  while (--i > 0) {
    old = last;
    last = append(ALIST(i), last);
    UNLOCK(old);
  }
  return last;
}


at *
nfirst(int n, at *l)
{
  at *answer = NIL;
  register at **where = &answer;
  while ( CONSP(l) && n-- > 0) 
    {
      *where = new_cons(l->Car, NIL);
      where = &((*where)->Cdr);
      l = l->Cdr;
    }
  if (l && n > 0) 
    {
      LOCK(l);
      *where = l;
    }
  return answer;
}

DX(xnfirst)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return nfirst( AINTEGER(1), APOINTER(2) );
}


at *
nth(register at *l, register int n)
{
  while (n > 0 && CONSP(l)) {
    CHECK_MACHINE("on");
    n--;
    l = l->Cdr;
  }
  if (n == 0 && CONSP(l)) {
    LOCK(l->Car);
    return l->Car;
  } else
    return NIL;
}

DX(xnth)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  if (ISLIST(1))
    return nth(APOINTER(1), AINTEGER(2) - 1);
  else
    return nth(ALIST(2), AINTEGER(1));
}


at *
nthcdr(register at *l, register int n)
{
  while (n > 0 && CONSP(l)) {
    n--;
    l = l->Cdr;
  }
  if (n == 0 && CONSP(l)) {
    LOCK(l);
    return l;
  } else
    return NIL;
}

DX(xnthcdr)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  if (ISLIST(1))
    return nthcdr(APOINTER(1), AINTEGER(2) - 1);
  else
    return nthcdr(ALIST(2), AINTEGER(1));
}


at *
reverse(register at *l)
{
  register at *r;
  at *q = l;
  int toggle = 0;

  r = NIL;
  while (CONSP(l)) {
    LOCK(l->Car);
    r = cons(l->Car, r);
    l = l->Cdr;
    if (l == q)
      error(NIL,"Cannot reverse a circular list",NIL);
    toggle ^= 1;
    if (toggle)
      q = q->Cdr;
  }
  return r;
}

DX(xreverse)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return reverse(ALIST(1));
}


static at **
flat1(register at *l, register at **where)
{
  at *slow = l;
  char toggle = 0;
  struct recur_elt elt;

  while (CONSP(l)) 
  {
    if (!recur_push_ok(&elt, &flat1, l->Car))
      error(NIL,"Cannot flatten circular list",NIL);
    where = flat1(l->Car, where);
    recur_pop(&elt);
    l = l->Cdr;
    if (l==slow)
      error(NIL,"Cannot flatten circular list",NIL);
    toggle ^= 1;
    if (toggle)
      slow = slow->Cdr;
  }
  if (l) {
    *where = new_cons(l, NIL);
    return &((*where)->Cdr);
  } else
    return where;
}

at *
flatten(at *l)
{
  at *ans = NIL;
  flat1(l, &ans);
  return ans;
}

DX(xflatten)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return flatten(APOINTER(1));
}


at *
assoc(at *k, at *l)
{
  at *p;
  at *q = l;
  int toggle = 0;
  while ( CONSP(l) ) 
  {
    p = l->Car;
    if (CONSP(p) && eq_test(p->Car,k)) {
      LOCK(p);
      return p;
    }
    l = l->Cdr;
    if (l==q)
      break;
    toggle ^= 1;
    if (toggle)
      q = q->Cdr;
  }
  return NIL;
}
DX(xassoc)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return assoc(APOINTER(1),APOINTER(2));
}


/* ----- unodes ------ */


static at *
new_unode(at *doc)
{
  return new_cons(NIL,doc);
}

static at *			
unode_dive(at *p)
{
  /* Warning: Return Unlocked AT* */
  at *q = p;
  while (CONSP(q) && q->Car)
    q = q->Car;
  ifn (CONSP(q))
    error(NIL,"Not a valid unode",p);
  return q;
}

static at *
unode_val(at *p)
{
  p = unode_dive(p);
  p = p->Cdr;
  LOCK(p);
  return p;
}

static at *
unode_eq(at *p1, at *p2)
{
  at *q1, *q2;
  q1 = unode_dive(p1);
  q2 = unode_dive(p2);
  if ( q1 != q2 )
    return NIL;
  LOCK(q1);
  return q1;
}

static at *
unode_unify(at *p1, at *p2, at *combine)
{
  at *q1, *q2;
  at *node;
  at *doc;
  q1 = unode_dive(p1);
  q2 = unode_dive(p2);
  if ( q1 == q2 )
    return NIL;
  if (combine) {
    LOCK(q1->Cdr);
    LOCK(q2->Cdr);
    node = cons(q1->Cdr,cons(q2->Cdr,NIL));
    doc = apply(combine,node);
    UNLOCK(node);
  } else
    doc = NIL;
  node = new_unode(doc);
  UNLOCK(doc);
  LOCK(node);
  LOCK(node);
  q1->Car = node;
  q2->Car = node;
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
  at *doc;
  ALL_ARGS_EVAL;
  if (arg_number==2)
    doc = NIL;
  else {
    ARG_NUMBER(3);
    doc = APOINTER(3);
  }
  return unode_unify(APOINTER(1),APOINTER(2),doc);
}



/* ------- refcount info ------- */

int 
used(void)
{
  register struct chunk_header *ch;
  register struct at *q;
  register int inuse;

  inuse = 0;
  q = named("result");
  var_set(q, NIL);
  UNLOCK(q);
  iter_on(&at_alloc, ch, q)
    inuse++;
  return inuse;
}

DX(xused)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(used());
}


/* Return the counter of an object */

DX(xcounter)
{
  at *p;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p)
    return NEW_NUMBER(p->count-1);
  return NIL;
}





/* ----- default class definition ------ */

void 
generic_dispose(at *p)
{
}

void 
generic_action(at *p, void (*action) (at *))
{
}

char *
generic_name(at *p)
{
  if (p->Class->classname)
    sprintf(string_buffer, "::%s:%lx", 
	    nameof(p->Class->classname),(long)p->Object);
  else
    sprintf(string_buffer, "::%lx:%lx", 
	    (long)p->Class, (long)p->Object);

  return string_buffer;
}

at *
generic_eval(at *p)
{
  LOCK(p);
  return p;
}

at *
generic_listeval(at *p, at *q)
{
  register struct symbol *s;
  register at *pp;

  /* looking for stacked functional values */
  pp = q->Car;			

  /* added stacked search on 15/7/88 */
  if (pp && (pp->flags & X_SYMBOL)) {
    s = pp->Object;
    s = s->next;
    while (s && s->valueptr) {
      pp = *(s->valueptr);
      if (pp && (pp->flags & C_EXTERN) &&
	  (pp->Class->list_eval != generic_listeval)) {
	
	if (eval_ptr == eval_debug) {
	  print_tab(error_doc.debug_tab);
	  print_string("  !! inefficient stacked call\n");
	}
	return (*(pp->Class->list_eval)) (pp, q);
      }
      s = s->next;
    }
  }
  if (LISTP(p))
    error("eval", "not a function call", q);
  else
    error(pname(p), "can't evaluate this list", NIL);
}




/* --------- INITIALISATION CODE --------- */

void 
init_at(void)
{
  dy_define("compute-bump", ycompute_bump);

  dx_define("cons", xcons);
  dx_define("car", xcar);
  dx_define("cdr", xcdr);
  dx_define("caar", xcaar);
  dx_define("cadr", xcadr);
  dx_define("cdar", xcdar);
  dx_define("cddr", xcddr);
  dx_define("rplaca", xrplaca);
  dx_define("rplacd", xrplacd);
  dx_define("displace", xdisplace);
  dx_define("listp", xlistp);
  dx_define("consp", xconsp);
  dx_define("atomp", xatomp);
  dx_define("numberp", xnumberp);
  dx_define("externp", xexternp);
  dx_define("null", xnull);
  dx_define("length", xlength);
  dx_define("last", xlast);
  dx_define("lastcdr", xlastcdr);
  dx_define("member", xmember);
  dx_define("append", xappend);
  dx_define("nfirst", xnfirst);
  dx_define("nth", xnth);
  dx_define("nthcdr", xnthcdr);
  dx_define("reverse", xreverse);
  dx_define("flatten", xflatten);
  dx_define("assoc", xassoc);
  dx_define("new_unode", xnew_unode);
  dx_define("unode_val", xunode_val);
  dx_define("unode_uid", xunode_uid);	
  dx_define("unode_eq", xunode_eq);
  dx_define("unode_unify", xunode_unify);

  dx_define("used", xused);
  dx_define("COUNT",xcounter);

}

