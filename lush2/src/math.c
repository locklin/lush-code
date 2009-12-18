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

#include "header.h"
#include "amath-macros.h"
#include <tgmath.h>

#define SGN(x) (x < 0.0 ? -1.0 : (x > 0.0 ? 1.0 : 0.0))
#define PIECE(x) (x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x))
#define RECT(x) (x < -1.0 ? 0.0 : (x > 1.0 ? 0.0 : 1.0))
#define ADD1(x) (x + 1.0)
#define SUB1(x) (x - 1.0)
#define MUL2(x) (x * 2.0)
#define DIV2(x) (x / 2.0)

DX_UNARY_FUNC(piece, PIECE)
DX_UNARY_FUNC(rect, RECT)
DX_UNARY_FUNC(sgn, SGN)
DX_UNARY_FUNC(abs, fabs)
DX_UNARY_FUNC(ceil, ceil)
DX_UNARY_FUNC(floor, floor)
DX_UNARY_FUNC(round, round)
DX_UNARY_FUNC(trunc, trunc)
DX_UNARY_FUNC(sqrt, sqrt)
DX_UNARY_FUNC(cbrt, cbrt)
DX_UNARY_FUNC(sin, sin)
DX_UNARY_FUNC(cos, cos)
DX_UNARY_FUNC(tan, tan)
DX_UNARY_FUNC(asin, asin)
DX_UNARY_FUNC(acos, acos)
DX_UNARY_FUNC(atan, atan)
DX_UNARY_FUNC(exp, exp)
DX_UNARY_FUNC(exp2, exp2)
DX_UNARY_FUNC(gamma, tgamma)
DX_UNARY_FUNC(lgamma, lgamma)
DX_UNARY_FUNC(expm1, expm1)
DX_UNARY_FUNC(log, log)
DX_UNARY_FUNC(log10, log10)
DX_UNARY_FUNC(log2, log2)
DX_UNARY_FUNC(log1p, log1p)
DX_UNARY_FUNC(sinh, sinh)
DX_UNARY_FUNC(cosh, cosh)
DX_UNARY_FUNC(tanh, tanh)
DX_UNARY_FUNC(asinh, asinh)
DX_UNARY_FUNC(acosh, acosh)
DX_UNARY_FUNC(atanh, atanh)

DX_UNARY_FUNC(add1, ADD1)
DX_UNARY_FUNC(sub1, SUB1)
DX_UNARY_FUNC(mul2, MUL2)
DX_UNARY_FUNC(div2, DIV2)

// C99 FP functions
DX_UNARY_FUNC_TO_INT(isfinite, isfinite)
DX_UNARY_FUNC_TO_INT(isinf, isinf)
DX_UNARY_FUNC_TO_INT(isnan, isnan)
DX_UNARY_FUNC_TO_INT(isnormal, isnormal)
DX_UNARY_FUNC_TO_INT(signbit, signbit)

DX(xatan2)
{
   ARG_NUMBER(2);
   if (ISINDEX(1) || ISINDEX(2)) {
      index_t *res, *a, *b;
      index_t ba_, *ba = &ba_;
      index_t bb_, *bb = &bb_;
      a = as_double_array(APOINTER(1));
      b = as_double_array(APOINTER(2));
      res = make_array(ST_DOUBLE, index_broadcast2(a, b, &ba, &bb), NIL);
      BINARY_FUNC(ba, bb, res, double, double, atan2);
      return res->backptr;
      
   } else if (ISNUMBER(1) && ISNUMBER(2)) {
      return NEW_NUMBER(atan2(AREAL(1), AREAL(2)));

   } else
      RAISEF("invalid argument(s) (not an index nor a number)", NIL);
   return NIL;
}


/* --------- SOLVE --------- */

static double solve(real x1, real x2, double (*f)(double))
{
   double y1 = (*f) (x1);
   double y2 = (*f) (x2);
   
   if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
      error("findroot", "bad search range", NIL);
  
   while (fabs(x1 - x2) > (1e-7) * fabs(x1 + x2)) {
      double x = (x1 + x2) / 2;
      double y = (*f) (x);

      if ((y1 >= 0 && y <= 0) || (y1 <= 0 && y >= 0)) {
         x2 = x;
         y2 = y;
      } else {
         x1 = x;
         y1 = y;
      }
   }
   return x1;
}

static at *solve_function;

double solve_call(double x)
{
   at *expr = new_cons(solve_function, new_cons(NEW_NUMBER(x), NIL));
   at *ans = eval(expr);
   return Number(ans);
}

DX(xfindroot)
{
   ARG_NUMBER(3);
   at *p = APOINTER(3);
   if (!FUNCTIONP(p)) {
      RAISEF("not a function", p);
   } else {
      solve_function = p;
   }
      
   p = NEW_NUMBER(solve(AREAL(1), AREAL(2), solve_call));
   solve_function = NIL;
   return p;
}

/* --------- INITIALISATION CODE --------- */

void init_math(void) {
 
   dx_define("0-x-1", xpiece);
   dx_define("0-1-0", xrect);
   dx_define("sgn", xsgn);
   dx_define("abs", xabs);
   dx_define("ceil", xceil);
   dx_define("floor", xfloor);
   dx_define("round", xround);
   dx_define("trunc", xtrunc);
   dx_define("sqrt", xsqrt);
   dx_define("cbrt", xcbrt);
   dx_define("sin", xsin);
   dx_define("cos", xcos);
   dx_define("tan", xtan);
   dx_define("asin", xasin);
   dx_define("acos", xacos);
   dx_define("atan", xatan);
   dx_define("atan2", xatan2);
   dx_define("exp", xexp);
   dx_define("exp2", xexp2);
   dx_define("exp-1", xexpm1);
   dx_define("gamma", xgamma);
   dx_define("lgamma", xlgamma);
   dx_define("log", xlog);
   dx_define("log10", xlog10);
   dx_define("log2", xlog2);
   dx_define("log1+", xlog1p);
   dx_define("sinh", xsinh);
   dx_define("cosh", xcosh);
   dx_define("tanh", xtanh);
   dx_define("asinh", xasinh);
   dx_define("acosh", xacosh);
   dx_define("atanh", xatanh);

   dx_define("1+", xadd1);
   dx_define("1-", xsub1);
   dx_define("2*", xmul2);
   dx_define("2/", xdiv2);
   
   dx_define("isfinite", xisfinite);
   dx_define("isinf", xisinf);
   dx_define("isnan", xisnan);
   dx_define("isnormal", xisnormal);
   dx_define("signbit", xsignbit);
   
   dx_define("findroot", xfindroot);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
