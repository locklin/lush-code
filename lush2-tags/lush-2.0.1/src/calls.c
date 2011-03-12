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
	This file contains general use functions
	numeric functions, logical functions, comparison functions,
	and control structure functions.
********************************************************************** */

#include "header.h"

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
         if (recur_push_ok(&elt, (void *)&comp_test, p)) {
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
      if (recur_push_ok(&elt, (void *)&eq_test, p)) {
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
      if (recur_push_ok(&elt, (void *)&eq_test, p)) {
         ans = !Class(p)->compare(p,q,false);
         recur_pop(&elt);
      }
      return ans;
    }
   
   /* No way to prove that objects are equal */
   return false;
}


/* --------- WEIRD FUNCTIONS --------- */

static int tlsizeof(const char *s)
{
   if (!strcmp(s, "bool"))
      return sizeof(bool);
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
   if( ! strcmp(s, "object"))
      return sizeof(object_t);
   error(NIL,"unsupported C type", NEW_STRING(s));
}

DX(xsizeof)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(tlsizeof(ASTRING(1)));
}

DX(xatgptr)
{
   ARG_NUMBER(1);
   return APOINTER(1) ? NEW_GPTR(APOINTER(1)) : NIL;
}



/* --------- LOGICAL FUNCTIONS --------- */



/*
 * and or operations logiques
 */

/* parallel and */
DX(xpand)
{
   at *res = t();
   for (int i=1; i<arg_number; i++)
      if (APOINTER(i) == NIL) {
         res = NIL;
         break;
      }
   return res;
}

DY(yand)
{
   at *res = t();
   at *p = ARG_LIST;
   while (CONSP(p)) {
      ifn ((res = eval(Car(p))))
         break;
      p = Cdr(p);
   }
   return res;
}

/* parallel or */
DX(xpor)
{
   at *res = NIL;
   for (int i=1; i<=arg_number; i++)
      if (APOINTER(i)) {
         res = APOINTER(i);
         break;
      }
   return res;
}

DY(yor)
{
   at *res = NIL;
   at *p = ARG_LIST;
   while (CONSP(p)) {
      if ((res = eval(Car(p))))
         break;
      p = Cdr(p);
   }
   return res;
}


/* --------- COMPARISONS OPERATORS --------- */


DX(xeq)
{
   if (arg_number==1)
      return t();

   if (arg_number>1) {
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
   if (APOINTER(1) == APOINTER(2))
      return t();
   else
      return NIL;
}

DX(xne)
{
   ARG_NUMBER(2);
   ifn (eq_test(APOINTER(1), APOINTER(2)))
      return t();
   else
      return NIL;
}

DX(xeq0)
{
   ARG_NUMBER(1);
   if (ISNUMBER(1))
      if (AREAL(1) == 0.0)
         return NEW_NUMBER(0);
   return NIL;
}

DX(xne0)
{
   ARG_NUMBER(1);
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
   if (comp_test(APOINTER(1), APOINTER(2)) > 0)
      return t();
   else
      return NIL;
}

DX(xge)
{
   ARG_NUMBER(2);
   if (comp_test(APOINTER(1), APOINTER(2)) >= 0)
      return t();
   else
      return NIL;
}

DX(xlt)
{
   ARG_NUMBER(2);
   if (comp_test(APOINTER(1), APOINTER(2)) < 0)
      return t();
   else
      return NIL;
}

DX(xle)
{
   ARG_NUMBER(2);
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
   if (l)
      RAISEFX("not a proper list", ARG_LIST);
   return NIL;
}


DY(ywhile)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("syntax error", ARG_LIST);

   at *q1 = NIL;
   at *q2 = eval(Car(ARG_LIST));
   MM_ENTER;
   while (q2) {
      q1 = progn(Cdr(ARG_LIST));
      q2 = eval(Car(ARG_LIST));
      if (break_attempt) break;
      MM_EXIT;
      MM_ANCHOR(q1);
   }
   CHECK_MACHINE("on");
   return q1;
}


DY(ydowhile)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("syntax error", ARG_LIST);

   at *q1 = NIL;
   at *q2 = NIL;
   MM_ENTER;
   do {
      q1 = progn(Cdr(ARG_LIST));
      q2 = eval(Car(ARG_LIST));
      if (break_attempt) break;
      MM_EXIT;
      MM_ANCHOR(q1);
   } while (q2);
   CHECK_MACHINE("on");
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
   at *res = NIL;
   MM_ENTER;
   while (i--) {
      res = progn(Cdr(ARG_LIST));
      if (break_attempt) break;
      MM_EXIT;
      MM_ANCHOR(res);
   }
   CHECK_MACHINE("on");
   return res;
}

DY(yfor)
{
   ifn(CONSP(ARG_LIST) && CONSP(Car(ARG_LIST)))
      RAISEFX("syntax error", NIL);

   at *sym, *p = Car(ARG_LIST);
   ifn (SYMBOLP(sym = Car(p)))
      RAISEFX("not a symbol", sym);

   at *num = NIL;
   p = Cdr(p);
   ifn (CONSP(p) && (num = eval(Car(p))) && NUMBERP(num))
      RAISEFX("not a number", num);
   double start = Number(num);

   p = Cdr(p);
   ifn (CONSP(p) && (num = eval(Car(p))) && NUMBERP(num))
      RAISEFX("not a number", num);
   double end = Number(num);

   p = Cdr(p);
   double step = 1.0;
   if (CONSP(p) && !Cdr(p)) {
      ifn (CONSP(p) && (num = eval(Car(p))) && NUMBERP(num))
         RAISEFX("not a number", num);
      step = Number(num);

   } else if (p) {
      RAISEFX("syntax error", p);
   }

   SYMBOL_PUSH(sym, NIL);
   symbol_t *symbol = Mptr(sym);
   at *res = NIL;
   MM_ENTER;
   if ((start <= end) && (step >= 0)) {
      for (double i = start; i <= end; i += step) {
         symbol->value = NEW_NUMBER(i);
         res = progn(Cdr(ARG_LIST));
         if (break_attempt) break;
         MM_EXIT;
         MM_ANCHOR(res);
      }
   } else if ((start >= end) && (step <= 0)) {
      for (real i = start; i >= end; i += step) {
         symbol->value = NEW_NUMBER(i);
         res = progn(Cdr(ARG_LIST));
         if (break_attempt) break;
         MM_EXIT;
         MM_ANCHOR(res);
      }
   }
   SYMBOL_POP(sym);
   CHECK_MACHINE("on");
   return res;
}


/* --------- INITIALISATION CODE --------- */

void init_calls(void)
{
   dx_define("sizeof", xsizeof);
   dx_define("atgptr", xatgptr);
   dx_define("==", xeqptr);
   dx_define("=", xeq);
   dx_define("<>", xne);
   dx_define("0=", xeq0);
   dx_define("0<>", xne0);
   dx_define(">", xgt);
   dx_define(">=", xge);
   dx_define("<", xlt);
   dx_define("<=", xle);
   dx_define("pand", xpand);
   dx_define("por", xpor);

   dy_define("and", yand);
   dy_define("or", yor);
   dy_define("if", yif);
   dy_define("when", ywhen);
   dy_define("cond", ycond);
   dy_define("selectq", yselectq);
   dy_define("while", ywhile);
   dy_define("do-while", ydowhile);
   dy_define("repeat", yrepeat);
   dy_define("for", yfor);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
