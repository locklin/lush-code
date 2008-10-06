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
 * $Id: calls.c,v 1.8 2003/12/15 14:00:10 leonb Exp $
 **********************************************************************/

/***********************************************************************
	This file contains general use functions
	numeric functions, logical functions, comparison functions,
	and control structure functions.
********************************************************************** */

#include "header.h"
#include "mm.h"

/* --------- UTILITY FUNCTIONS --------- */


/* comp_test
 * -- rank two lisp objects
 *    no need to clutter this code with infinite recursion
 *    avoidance tricks.
 */

int comp_test(at *p, at *q)
{
   if (p && q) {
      if (NUMBERP(p) && NUMBERP(q)) {
         real r1 = Number(p);
         real r2 = Number(q);
         if (r1<r2)
            return -1;
         else if (r1>r2)
            return 1;
         else
            return 0;
      } else if (GPTRP(p) && GPTRP(q)) {
         unsigned long r1 = (unsigned long)Gptr(p);
         unsigned long r2 = (unsigned long)Gptr(q);
         if (r1<r2)
            return -1;
         else if (r1>r2)
            return 1;
         else
            return 0;

      } else if (Class(p)==Class(q) && Class(p)->compare) {
         int ans = 0;
         struct recur_elt elt;
         if (recur_push_ok(&elt, &comp_test, p)) {
            ans = Class(p)->compare(p,q,true);
            recur_pop(&elt);
         }
         return ans;
      }
      
   }
   error(NIL, "cannot rank these objects", NIL);
}



/* eq_test
 * -- Equality test for lisp objects
 */

int eq_test(at *p, at *q)
{
   int ans = true;
   at *pslow = p;
   char toggle = 0;

again:
   /* Trivial */
   if (p == q)
      return true;
   else if (!p || !q)
      return false;
   /* List */
   else if (CONSP(p) && CONSP(q)) {
      struct recur_elt elt;
      if (recur_push_ok(&elt, &eq_test, p)) {
         ans = eq_test(Car(p), Car(q));
         recur_pop(&elt);
      }
      if (ans) {
         /* go to next list element */
         p = Cdr(p);
         q = Cdr(q);
         if (p==pslow) /* circular list detected */
            return ans;
         toggle ^= 1;
         if (toggle)
            pslow = Cdr(pslow);
         goto again;
      }
      return ans;
      /* Number */
   } else if (NUMBERP(p) && NUMBERP(q)) {

#if defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)
      if (Number(p) == Number(q)) {
         float delta = (float)(Number(p) - Number(q));
         if (! *(long*)&delta)
            return true;
         return false;
      }
#else
      return (Number(p) == Number(q));
#endif
      /* GPTR */
   } else if (GPTRP(p) && GPTRP(q)) {
      return (Gptr(p) == Gptr(q));

      /* Comparison method provided */
   } else if (Class(p)==Class(q) && Class(p)->compare) {
      struct recur_elt elt;
      if (recur_push_ok(&elt, &eq_test, p)) {
         ans = ! Class(p)->compare(p,q,false);
         recur_pop(&elt);
      }
      return ans;
    }
   
   /* No way to prove that objects are equal */
   return false;
}


/* --------- WEIRD FUNCTIONS --------- */

static int tlsizeof(char *s)
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
   if( ! strcmp( s , "at" ) )
      return sizeof(at);
   error(NIL,"unsupported C type",new_string(s));
}

DX(xsizeof)
{
   ARG_NUMBER(1);
   ALL_ARGS_EVAL;
   return NEW_NUMBER(tlsizeof(ASTRING(1)));
}

DX(xatgptr)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return APOINTER(1) ? NEW_GPTR(APOINTER(1)) : NIL;
}



/* --------- MAKELIST FUNCTIONS --------- */

/*
 * range ex: (range 1 5) gives (1 2 3 4 5) and (range 2 4 .3) gives (2  2.3
 * 2.6  2.9  3.2  3.5  3.8)
 */

DX(xrange)
{
   ALL_ARGS_EVAL;

   real high, low = 1.0;
   real delta = 1.0;
   
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
         delta = -delta;
   if (! delta)
      error(NIL, "illegal arguments", NIL);
   
   at *answer = NIL;
   at **where = &answer;

   if (delta > 0) {
      for (real i=low; i<=high; i+=delta) {
         *where = new_cons(NEW_NUMBER(i), NIL);
         where = &Cdr(*where);
      }
   } else {
      for (real i=low; i>=high; i+=delta) {
         *where = new_cons(NEW_NUMBER(i), NIL);
         where = &Cdr(*where);
      }
   }
   return answer;
}

DX(xrange_star)
{
   ALL_ARGS_EVAL;

   real high, low = 0.0;
   real delta = 1.0;
   
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
         delta = -delta;
   if (! delta)
      error(NIL, "illegal arguments", NIL);
  
   at *answer = NIL;
   at **where = &answer;
   if (delta > 0) {
      for (real i=low; i<high; i+=delta) {
         *where = new_cons(NEW_NUMBER(i), NIL);
         where = &Cdr(*where);
      }
   } else {
      for (real i=low; i>high; i+=delta) {
         *where = new_cons(NEW_NUMBER(i), NIL);
         where = &Cdr(*where);
      }
   }
   return answer;
}


/* --------- LOGICAL FUNCTIONS --------- */



/*
 * and or operations logiques
 */


DX(xand)
{
   if (arg_number==0)
      return t();

   at *p = NIL;
   for (int i = 1; i <= arg_number; i++) {
      ARG_EVAL(i);
      ifn ((p = APOINTER(i)))
         return NIL;
   }
   return p;
}

DX(xor)
{
   at *p = NIL;
   for (int i = 1; i <= arg_number; i++) {
      ARG_EVAL(i);
      if ((p = APOINTER(i))) {
         return p;
      }
   }
   return NIL;
}


/* --------- COMPARISONS OPERATORS --------- */


DX(xeq)
{
   if (arg_number==1)
      return t();

   if (arg_number>1) {
      ALL_ARGS_EVAL;
      int n = 1;
      while (n<arg_number && eq_test(APOINTER(n), APOINTER(n+1)))
         n++;
      if (n==arg_number)
         return t();
      else
         return NIL;
   }
   ARG_NUMBER(-1);
   return NIL;
}

DX(xeqptr)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   if (APOINTER(1) == APOINTER(2))
      return t();
   else
      return NIL;
}

DX(xne)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   
   ifn (eq_test(APOINTER(1), APOINTER(2)))
      return t();
   else
      return NIL;
}

DX(xeq0)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   if (ISNUMBER(1))
      if (AREAL(1) == 0.0)
         return NEW_NUMBER(0);
   return NIL;
}

DX(xne0)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   
   if (ISNUMBER(1)) {
      if (AREAL(1) == 0.0)
         return NIL;
   } 
   if (APOINTER(1))
      return APOINTER(1);
   else
      return t();
}

DX(xgt)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   
   if (comp_test(APOINTER(1), APOINTER(2)) > 0)
      return t();
   else
      return NIL;
}

DX(xge)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;

   if (comp_test(APOINTER(1), APOINTER(2)) >= 0)
      return t();
   else
      return NIL;
}

DX(xlt)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   
   if (comp_test(APOINTER(1), APOINTER(2)) < 0)
      return t();
   else
      return NIL;
}

DX(xle)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;

   if (comp_test(APOINTER(1), APOINTER(2)) <= 0)
      return t();
   else
      return NIL;
}

/* --------- CONTROL STRUCTURES --------- */

DY(yif)
{
   ifn (CONSP(ARG_LIST) && CONSP(Cdr(ARG_LIST)))
      RAISEFX("syntax error", NIL);
   at *q = eval(Car(ARG_LIST));
   if (q)
      return eval(Cadr(ARG_LIST));
   else
      return progn(Cddr(ARG_LIST));
}

DY(ywhen)
{
   ifn (CONSP(ARG_LIST) && CONSP(Cdr(ARG_LIST)))
      RAISEFX("syntax error", NIL);
   at *q = eval(Car(ARG_LIST));
   if (q)
      return progn(Cdr(ARG_LIST));
   else
      return NIL;
}


DY(ycond)
{
   at *l = ARG_LIST;
   
   while (CONSP(l)) {
      at *q = Car(l);
      ifn (CONSP(q)) {
         RAISEFX("syntax error", q);
      } else {
         if ((q = eval(Car(q))))
            return progn(Cdar(l));
      }
      l = Cdr(l);
   }
   if (l) 
      RAISEFX("not a proper list", NIL);
   return NIL;
}


DY(yselectq)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("not a proper list", ARG_LIST);
   
   at *item = eval(Car(ARG_LIST));
   at *l = Cdr(ARG_LIST);
   
   while (CONSP(l)) {
      at *q = Car(l);
      ifn (CONSP(q)) {
         RAISEFX("syntax error", q);
      } else {
         if (Car(q)==at_t || eq_test(Car(q), item))
            return progn(Cdr(q));
         else if (CONSP(Car(q)) && member(item, Car(q)))
            return progn(Cdar(l));
      }
      CHECK_MACHINE("on");
      l = Cdr(l);
   }
   if (!l)
      return NIL;
   
   RAISEFX("not a proper list", ARG_LIST);
}


DY(ywhile)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("syntax error", ARG_LIST);
   
   at *q2, *q1 = NIL;
   while ((q2 = eval(Car(ARG_LIST)))) {
      q1 = progn(Cdr(ARG_LIST));
      CHECK_MACHINE("on");
   }
   return q1;
}


DY(ydowhile)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("syntax error", ARG_LIST);

   at *q1 = NIL;
   do {
      q1 = progn(Cdr(ARG_LIST));
      CHECK_MACHINE("on");
   } while (eval(Car(ARG_LIST)));
   return q1;
}


DY(yrepeat)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("syntax error", ARG_LIST);
   
   at *q = eval(Car(ARG_LIST));
   if (!NUMBERP(q)) {
      RAISEFX("not a number", q);

   } else if (Number(q)<0 || Number(q)>1e9) {
      RAISEFX("out of range", q);
   }
   int i = (int)Number(q);
   at *p = NIL;
   while (i--) {
      p = progn(Cdr(ARG_LIST));
      CHECK_MACHINE("on");
   }
   return p;
}

/* --------- INITIALISATION CODE --------- */

void init_calls(void)
{
   dx_define("sizeof", xsizeof);
   dx_define("atgptr", xatgptr);
   dx_define("range", xrange);
   dx_define("range*", xrange_star);
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
   dy_define("if", yif);
   dy_define("when", ywhen);
   dy_define("cond", ycond);
   dy_define("selectq", yselectq);
   dy_define("while", ywhile);
   dy_define("do-while", ydowhile);
   dy_define("repeat", yrepeat);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
