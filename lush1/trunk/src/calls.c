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
 * $Id: calls.c,v 1.4 2002-08-13 16:21:18 leonb Exp $
 **********************************************************************/

/***********************************************************************
	This file contains general use functions
	numeric functions, logical functions, comparison functions,
	and control structure functions.
********************************************************************** */

#include "header.h"

static char badmatharg[] = "bad argument in math function";
static char divbyzero[] = "division by zero";
static char illegarg[] = "illegal argument(s)";
static char pointlist[] = "why a pointed list ?";



/* --------- UTILITY FUNCTIONS --------- */


/* comp_test
 * -- rank two lisp objects
 *    no need to clutter this code with infinite recursion
 *    avoidance tricks.
 */

int 
comp_test(at *p, at *q)
{
  if (p && q) 
  {
    if ((p->flags & C_NUMBER) && 
        (q->flags & C_NUMBER)) 
      {
        real r1 = p->Number;
        real r2 = q->Number;
        if (r1<r2)
          return -1;
        else if (r1>r2)
          return 1;
        else
          return 0;
      }
    else if ((p->flags & C_EXTERN) && 
             (q->flags & C_EXTERN) &&
             (p->Class == q->Class) && 
             (p->Class->compare) )
      {
        int ans = 0;
        struct recur_elt elt;
        if (recur_push_ok(&elt, &comp_test, p))
          {
            ans = (*p->Class->compare)(p,q,TRUE);
            recur_pop(&elt);
          }
        return ans;
      }
    else if ((p->flags & C_GPTR) && 
             (q->flags & C_GPTR) )
      {
        unsigned long r1 = (unsigned long)p->Gptr;
        unsigned long r2 = (unsigned long)q->Gptr;
        if (r1<r2)
          return -1;
        else if (r1>r2)
          return 1;
        else
          return 0;
      }

  }
  error(NIL, "Cannot rank these objects", NIL);
}



/* eq_test
 * -- Equality test for lisp objects
 */

int 
eq_test(at *p, at *q)
{
  int ans = TRUE;
  at *pslow = p;
  struct recur_elt elt;
  char toggle = 0;

again:
  /* Trivial */
  if (p == q)
    return TRUE;
  else if (!p || !q)
    return FALSE;
  /* List */
  else if ((p->flags & C_CONS) && (q->flags & C_CONS)) 
    {
      if (recur_push_ok(&elt, &eq_test, p))
        {
          ans = eq_test(p->Car, q->Car);
          recur_pop(&elt);
        }
      if (ans)
        {
          /* go to next list element */
          p = p->Cdr;
          q = q->Cdr;
          if (p==pslow) /* circular list detected */
            return ans;
          toggle ^= 1;
          if (toggle)
            pslow = pslow->Cdr;
          goto again;
        }
      return ans;
    }
  /* Number */
  else if ((p->flags & C_NUMBER) && 
           (q->flags & C_NUMBER))
    {
#if defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)
      if (p->Number == q->Number) {
        float delta = (float)(p->Number - q->Number);
        if (! *(long*)&delta)
          return TRUE;
      }
#else
      if (p->Number == q->Number)
        return TRUE;
#endif
      return FALSE;
    }
  /* Comparison method provided */
  else if ((p->flags & C_EXTERN) && 
           (q->flags & C_EXTERN) &&
           (p->Class == q->Class) && 
           (p->Class->compare) )
    {
      if (recur_push_ok(&elt, &eq_test, p))
        {
          ans = !(*p->Class->compare)(p,q,FALSE);
          recur_pop(&elt);
        }
      return ans;
    }
  /* GPTR */
  else if ((p->flags & C_GPTR) && 
           (q->flags & C_GPTR) )
    {
      if (p->Gptr == q->Gptr)
        return TRUE;
      return FALSE;
    }
  
  /* No way to prove that objects are equal */
  return FALSE;
}



/* --------- NUMERICS FUNCTIONS --------- */



DX(xadd)
{
  register real sum;
  register int i;

  sum = 0.0;
  i = 0;
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
    sum += AREAL(i);
  return NEW_NUMBER(sum);
}

DX(xmul)
{
  register real prod;
  register int i;

  prod = 1.0;
  i = 0;
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
    prod *= AREAL(i);
  return NEW_NUMBER(prod);
}

DX(xsub)
{
  register real a1, a2;

  if (arg_number == 1) {
    ARG_EVAL(1);
    return NEW_NUMBER(-AREAL(1));
  } else {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    a1 = AREAL(1);
    a2 = AREAL(2);
    return NEW_NUMBER(a1 - a2);
  }
}

DX(xdiv)
{
  register real a1, a2;

  if (arg_number == 1) {
    ARG_EVAL(1);
    a1 = 1.0;
    a2 = AREAL(1);
  } else {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    a1 = AREAL(1);
    a2 = AREAL(2);
  }
  if (a2 == 0.0)
    error(NIL, divbyzero, NIL);
  return NEW_NUMBER(a1 / a2);
}

DX(xpower)
{
  register real r;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  r = AREAL(1);
  if (r < 0)
    error(NIL, badmatharg, APOINTER(1));
  return NEW_NUMBER(pow((double) (r), (double) (AREAL(2))));
}

DX(xadd1)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) + 1.0);
}

DX(xsub1)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) - 1.0);
}

DX(xmul2)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) * 2.0);
}

DX(xdiv2)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) / 2.0);
}

DX(xdivi)
{
  register int dv;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  dv = AINTEGER(2);
  if (dv == 0)
    error(NIL, divbyzero, NIL);
  return NEW_NUMBER(AINTEGER(1) / dv);
}

DX(xmodi)
{
  register int dv;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  dv = AINTEGER(2);
  if (dv == 0)
    error(NIL, divbyzero, NIL);
  return NEW_NUMBER(AINTEGER(1) % dv);
}


DX(xbitand)
{
  int i;
  int x = ~0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x &= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitor)
{
  int i;
  int x = 0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x |= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitxor)
{
  int i;
  int x = 0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x ^= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitshl)
{
  int x1, x2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  x1 = AINTEGER(1);
  x2 = AINTEGER(2);
  return NEW_NUMBER(x1 << x2);
}

DX(xbitshr)
{
  int x1, x2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  x1 = AINTEGER(1);
  x2 = AINTEGER(2);
  return NEW_NUMBER(x1 >> x2);
}



/* --------- WEIRD FUNCTIONS --------- */

static int 
tlsizeof(char *s)
{
  if( ! strcmp( s , "flt" ) )
    return sizeof(flt);
  if( ! strcmp( s , "float" ) )
    return sizeof(float);
  if( ! strcmp( s , "double" ) )
    return sizeof(double);
  if( ! strcmp( s , "real" ) )
    return sizeof(real);
  if( ! strcmp( s , "long" ) )
    return sizeof(int);
  if( ! strcmp( s , "int" ) )
    return sizeof(int);
  if( ! strcmp( s , "short" ) )
    return sizeof(short);
  if( ! strcmp( s , "char" ) )
    return sizeof(char);
  if( ! strcmp( s , "gptr" ) )
    return sizeof(gptr);
  error(NIL,"unsupported C type",new_string(s));
}

DX(xsizeof)
{
  int i;
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  i = tlsizeof(ASTRING(1));
  return NEW_NUMBER(i);
}

DX(xatgptr)
{
  at *p;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p)
    return NEW_GPTR(p);
  else
    return NIL;
}



/* --------- MAKELIST FUNCTIONS --------- */


/*
 * makelist ex:  (makelist 4 'a) -->  (a a a a)
 */
at *
makelist(int n, at *v)
{
  at *ans;

  ans = NIL;
  if (n < 0)
    error(NIL, "illegal negative value", NEW_NUMBER(n));
  if (n > 32767)
    error(NIL, "too large integer in makelist", NEW_NUMBER(n));
  while (n--) {
    LOCK(v);
    ans = cons(v, ans);
  }
  return ans;
}

DX(xmakelist)
{
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  return makelist(AINTEGER(1), APOINTER(2));
}


/*
 * range ex: (range 1 5) gives (1 2 3 4 5) and (range 2 4 .3) gives (2  2.3
 * 2.6  2.9  3.2  3.5  3.8)
 */

DX(xrange)
{
  real i;
  at **where;
  at *answer;
  real low, high;
  real delta;

  low = delta = 1.0;

  ALL_ARGS_EVAL;

  if (arg_number == 3) {
    low = AREAL(1);
    high = AREAL(2);
    delta = AREAL(3);
  } else if (arg_number == 2) {
    low = AREAL(1);
    high = AREAL(2);
  } else {
    ARG_NUMBER(1);
    high = AREAL(1);
  }

  if (arg_number==2)
    if (delta * (high - low) <= 0)
      ifn(delta = -delta)
	error(NIL, illegarg, NIL);

  answer = NIL;
  where = &answer;
  for (i = low; (high - i) * delta >= 0; i += delta) {
    *where = cons(NEW_NUMBER(i), NIL);
    where = &((*where)->Cdr);
  }
  return answer;
}



/* --------- RANDOM FUNCTIONS --------- */

DX(xseed)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  Fseed((int)AREAL(1));
  return NIL;
}

DX(xrand)
{
  real lo, hi, rand;

  ALL_ARGS_EVAL;
  if (arg_number == 0) {
    lo = 0.0;
    hi = 1.0;
  } else if (arg_number == 1) {
    hi = AREAL(1);
    lo = -hi;
  } else {
    ARG_NUMBER(2);
    lo = AFLT(1);
    hi = AFLT(2);
  }

  rand = Ftor(Frand());
  return NEW_NUMBER((hi - lo) * rand + lo);
}


DX(xgauss)
{
  real mean, sdev, rand;

  ALL_ARGS_EVAL;
  if (arg_number == 0) {
    mean = 0.0;
    sdev = 1.0;
  } else if (arg_number == 1) {
    mean = 0.0;
    sdev = AREAL(1);
  } else {
    ARG_NUMBER(2);
    mean = AFLT(1);
    sdev = AFLT(2);
  }

  rand = Ftor(Fgauss());
  return NEW_NUMBER(sdev * rand + mean);
}


DX(xuniq)
{
  static int uniq = 0;
  ARG_NUMBER(0);
  uniq++;
  return NEW_NUMBER(uniq);
}

/* --------- LOGICAL FUNCTIONS --------- */



/*
 * and or operations logiques
 */


DX(xand)
{
  int i;
  at *p = NIL;
  if (arg_number==0)
    return true();
  for (i = 1; i <= arg_number; i++) {
    ARG_EVAL(i);
    if ((p = APOINTER(i)) == NIL)
      return NIL;
  }
  LOCK(p);
  return p;
}

DX(xor)
{
  int i;
  at *p;

  p = NIL;
  for (i = 1; i <= arg_number; i++) {
    ARG_EVAL(i);
    if ((p = APOINTER(i))) {
      LOCK(p);
      return p;
    }
  }
  return NIL;
}


/* --------- COMPARISONS OPERATORS --------- */


DX(xeq)
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  if (eq_test(p, APOINTER(2)))
    return true();
  else
    return NIL;
}


DX(xeqptr)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  if (APOINTER(1) == APOINTER(2))
    return true();
  else
    return NIL;
}

DX(xne)
{
  at *p;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  ifn(eq_test(p, APOINTER(2)))
    return true();
  else
  return NIL;
}

DX(xeq0)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (ISNUMBER(1))
    if (APOINTER(1)->Number == (real) 0)
      return NEW_NUMBER(0);
  return NIL;
}

DX(xne0)
{
  at *p;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p && (p->flags & C_NUMBER))
    if (p->Number == 0.0)
      return NIL;

  ifn(p)
    p = true();
  else
  LOCK(p);
  return p;
}

DX(xgt)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  if (comp_test(APOINTER(1), APOINTER(2)) > 0)
    return true();
  else
    return NIL;
}

DX(xge)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  if (comp_test(APOINTER(1), APOINTER(2)) >= 0)
    return true();
  else
    return NIL;
}

DX(xlt)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  if (comp_test(APOINTER(1), APOINTER(2)) < 0)
    return true();
  else
    return NIL;
}

DX(xle)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  if (comp_test(APOINTER(1), APOINTER(2)) <= 0)
    return true();
  else
    return NIL;
}

DX(xmax)
{
  at *ans;
  int i;

  ALL_ARGS_EVAL;
  if (arg_number < 1)
    DX_ERROR(0, 1);
  ans = APOINTER(1);

  for (i = 2; i <= arg_number; i++)
    if (comp_test(APOINTER(i), ans) > 0)
      ans = APOINTER(i);
  LOCK(ans);
  return ans;
}

DX(xmin)
{
  at *ans;
  int i;

  ALL_ARGS_EVAL;
  if (arg_number < 1)
    DX_ERROR(0, 1);
  ans = APOINTER(1);

  for (i = 2; i <= arg_number; i++)
    if (comp_test(APOINTER(i), ans) < 0)
      ans = APOINTER(i);
  LOCK(ans);
  return ans;
}

/* --------- CONTROL STRUCTURES --------- */

/*
 * (if COND YES NO-1 .. NO-n) (ifn COND NO YES-1 .. YES-n) may be defined as
 * macros:   when  unless
 */

DY(yif)
{
  at *q;

  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "bad 'if' construction", NIL);
  q = eval(ARG_LIST->Car);
  if (q) {
    UNLOCK(q);
    return eval(ARG_LIST->Cdr->Car);
  } else
    return progn(ARG_LIST->Cdr->Cdr);
}

DY(ywhen)
{
  at *q;

  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "bad 'when' construction", NIL);
  q = eval(ARG_LIST->Car);
  if (q) {
    UNLOCK(q);
    return progn(ARG_LIST->Cdr);
  } else
    return NIL;
}


/*
 * (cond  IFLIST-1  ..  IFLIST-n) with IFLIST-? is   ( CONDITION   OK-1 ...
 * OK-n )
 */

DY(ycond)
{
  at *l;
  at *q;

  l = ARG_LIST;

  while (CONSP(l)) {
    q = l->Car;
    ifn(CONSP(q))
      error("cond", "bad 'cond' construction", q);
    else
      if ((q = eval(q->Car))) {
      UNLOCK(q);
      return progn(l->Car->Cdr);
      }
    l = l->Cdr;
  }
  if (l) 
    error("cond", pointlist, NIL);
  return NIL;
}


/*
 * (selectq  EXPR  SEL-LIST-1 ...   SEL-LIST-n) with SEL-LIST-i is  (
 * GOOD-EXPR-LIST    OK-1 .. OK-n )
 */

DY(yselectq)
{
  extern at *at_true;

  at *l;
  at *q;
  at *item;

  ifn(CONSP(ARG_LIST))
    goto errselect;

  item = eval(ARG_LIST->Car);

  l = ARG_LIST->Cdr;
  while (CONSP(l)) {
    q = l->Car;
    ifn(CONSP(q))
      error("selectq", "bad 'selectq' construction", q);
    else
    if (q->Car == at_true || eq_test(q->Car, item)) {
      UNLOCK(item);
      return progn(q->Cdr);
    } else if (CONSP(q->Car) && member(item, q->Car)) {
      UNLOCK(item);
      return progn(l->Car->Cdr);
    }
    CHECK_MACHINE("on");
    l = l->Cdr;
  }
  UNLOCK(item);
  if (!l)
    return NIL;
  errselect:
    error(NIL, pointlist, NIL);
}


/*
 * (while COND LOOP-1 .. LOOP-n) 
 */

DY(ywhile)
{
  at *q1, *q2;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'while' construction", NIL);

  q1 = NIL;
  while ((q2 = eval(ARG_LIST->Car))) {
    UNLOCK(q2);
    UNLOCK(q1);
    q1 = progn(ARG_LIST->Cdr);
    CHECK_MACHINE("on");
  }
  return q1;
}


/*
 * (do-while COND LOOP-1 .. LOOP-n) 
 */

DY(ydowhile)
{
  at *q1, *q2;
  
  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'do-while' construction", NIL);
  q1 = q2 = NIL;
  do {
    UNLOCK(q2);
    UNLOCK(q1);
    q1 = progn(ARG_LIST->Cdr);
    CHECK_MACHINE("on");
  } while ((q2 = eval(ARG_LIST->Car)));
  return q1;
}


/*
 * (repeat N LOOP1 .. LOOPn) repeats N times a progn
 */
DY(yrepeat)
{
  at *q1, *q2;
  int i;

  ifn(CONSP(ARG_LIST))
    error(NIL, "bad 'repeat' construction", NIL);

  q2 = eval(ARG_LIST->Car);
  ifn(q2 && (q2->flags & C_NUMBER))
    error("repeat", "not a number", q2);
  else
  if (q2->Number < 0 || q2->Number > 1e9)
    error("repeat", "out of range", q2);

  q1 = NIL;
  i = (int)(q2->Number);
  UNLOCK(q2);
  while (i--) {
    UNLOCK(q1);
    q1 = progn(ARG_LIST->Cdr);
    CHECK_MACHINE("on");
  }
  return q1;
}

/* --------- INITIALISATION CODE --------- */

void 
init_calls(void)
{
  dx_define("+", xadd);
  dx_define("*", xmul);
  dx_define("-", xsub);
  dx_define("/", xdiv);
  dx_define("**", xpower);
  dx_define("1+", xadd1);
  dx_define("1-", xsub1);
  dx_define("2*", xmul2);
  dx_define("2/", xdiv2);
  dx_define("mod", xmodi);
  dx_define("div", xdivi);
  dx_define("bitand", xbitand);
  dx_define("bitor", xbitor);
  dx_define("bitxor", xbitxor);
  dx_define("bitshl", xbitshl);
  dx_define("bitshr", xbitshr);
  dx_define("sizeof", xsizeof);
  dx_define("atgptr", xatgptr);
  dx_define("makelist", xmakelist);
  dx_define("range", xrange);
  dx_define("seed", xseed);
  dx_define("rand", xrand);
  dx_define("gauss", xgauss);
  dx_define("uniq", xuniq);
  dx_define("and", xand);
  dx_define("or", xor);
  dx_define("==", xeqptr);
  dx_define("=", xeq);
  dx_define("<>", xne);
  dx_define("0=", xeq0);
  dx_define("0<>", xne0);
  dx_define(">", xgt);
  dx_define(">=", xge);
  dx_define("<", xlt);
  dx_define("<=", xle);
  dx_define("max", xmax);
  dx_define("min", xmin);
  dy_define("if", yif);
  dy_define("when", ywhen);
  dy_define("cond", ycond);
  dy_define("selectq", yselectq);
  dy_define("while", ywhile);
  dy_define("do-while", ydowhile);
  dy_define("repeat", yrepeat);
}
