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

#ifdef HAVE_FPU_CONTROL_H
#  include <fpu_control.h>
#else
#  ifdef HAVE___SETFPUCW
#    define _FPU_SETCW(cw) __setfpucw(cw)
#  endif 
#  define fpu_control_t unsigned int
#  define _FPU_SINGLE   0
#  define _FPU_DOUBLE   1
#  define _FPU_EXTENDED 2
#endif

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>
#endif

#include <fenv.h>
#include <signal.h>
#include <float.h>
#if defined(__MACOSX__) && defined(__SSE__)
#  include <xmmintrin.h>
#endif

#define KEY_INVALID    "invalid"
#define KEY_DENORM     "denorm"
#define KEY_DIVBYZERO  "div-by-zero"
#define KEY_OVERFLOW   "overflow"
#define KEY_UNDERFLOW  "underflow"
#define KEY_INEXACT    "inexact"
#define KEY_ALL_EXCEPT "all"

#define KEY_DOWNWARD   "downward"
#define KEY_TONEAREST  "to-nearest"
#define KEY_TOWARDZERO "toward-zero"
#define KEY_UPWARD     "upward"

#define KEY_SINGLE     "single"
#define KEY_DOUBLE     "double"
#define KEY_EXTENDED   "extended"


/* testing for special floating point values */

DX(xinfinityp)
{
   ARG_NUMBER(1);
   return isinf(AREAL(1)) ? t() : NIL;
}

DX(xnanp)
{
   ARG_NUMBER(1);
   return isnan(AREAL(1)) ? t() : NIL;
}


double eps(double x) {
   x = copysign(x, 1.0);
   double y = nextafter(x, INFINITY);
   return y-x;
}

float epsf(float x) {
   x = copysignf(x, 1.0);
   float y = nextafterf(x, INFINITY);
   return y-x;
}

DX(xeps)
{
   ARG_NUMBER(1);
   double x = AREAL(1);
   ifn (isfinite(x))
      RAISEF("eps not defined for number", NEW_NUMBER(x));
   return NEW_NUMBER(eps(x));
}


#ifdef _FPU_GETCW
DY(ywith_fpu_env)
{
   struct lush_context mycontext;
   fpu_control_t control;
   fexcept_t status;
   _FPU_GETCW(control);
   fegetexceptflag(&status, FE_ALL_EXCEPT);

   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1)) {
      _FPU_SETCW(control);
      fesetexceptflag(&status, FE_ALL_EXCEPT);
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(ARG_LIST);

   context_pop();
   _FPU_SETCW(control);
   fesetexceptflag(&status, FE_ALL_EXCEPT);
   return answer;
}
#else
DY(ywith_fpu_env)
{
   struct lush_context mycontext;
   fenv_t current_fenv, *saved_fenv = &current_fenv;

   if (fegetenv(saved_fenv)) {
      fprintf(stderr, "*** Warning: could not save FPU environment, restoring standard environment\n");
      saved_fenv = NULL;
   }

   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1)) {
      fesetenv(saved_fenv);
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(ARG_LIST);

   context_pop();
   if (saved_fenv) fesetenv(saved_fenv);
   return answer;
}
#endif

static const char *errmsg_keyword = "not a valid keyword";

static int parse_excepts(int arg_number, at **arg_array)
{
   char *keywords = KEY_INVALID KEY_DIVBYZERO KEY_OVERFLOW KEY_UNDERFLOW KEY_INEXACT KEY_ALL_EXCEPT;
   int excepts = 0;
   for (int i=1; i<= arg_number; i++) {
      char *kw = strstr(keywords, nameof(ASYMBOL(i)));
      switch (1+(kw-keywords)) {
      case 1: 
         excepts |= FE_INVALID;
         break;
         
      case sizeof(KEY_INVALID): 
         excepts |= FE_DIVBYZERO; 
         break;
         
      case sizeof(KEY_INVALID KEY_DIVBYZERO):
         excepts |= FE_OVERFLOW;
         break;
         
      case sizeof(KEY_INVALID KEY_DIVBYZERO KEY_OVERFLOW):
         excepts |= FE_UNDERFLOW;
         break;

      case sizeof(KEY_INVALID KEY_DIVBYZERO KEY_OVERFLOW KEY_UNDERFLOW):
         excepts |= FE_INEXACT;
         break;

      case sizeof(KEY_INVALID KEY_DIVBYZERO KEY_OVERFLOW KEY_UNDERFLOW KEY_INEXACT):
         excepts |= FE_ALL_EXCEPT;
         break;

      default:
         RAISEF(errmsg_keyword, APOINTER(i));
      }
   }
   return excepts;
}

static at *unparse_excepts(int excepts)
{
   at *es = NIL;
   
   if (excepts & FE_INEXACT)
      es = new_cons(NEW_SYMBOL(KEY_INEXACT), es);

   if (excepts & FE_UNDERFLOW)
      es = new_cons(NEW_SYMBOL(KEY_UNDERFLOW), es);

   if (excepts & FE_OVERFLOW)
      es = new_cons(NEW_SYMBOL(KEY_OVERFLOW), es);

   if (excepts & FE_DIVBYZERO)
      es = new_cons(NEW_SYMBOL(KEY_DIVBYZERO), es);

   if (excepts & FE_INVALID)
      es = new_cons(NEW_SYMBOL(KEY_INVALID), es);
   
   return es;
}

static void warn_notrap(void)
{
   static int not_warned = 3;
   if (not_warned) {
      fprintf(stderr, "*** Warning: trapping FPU exceptions not supported on this platform\n");
      not_warned--;
   }
}

static void fpu_trap (int excepts)
{
   feclearexcept(excepts);
#ifdef HAVE_FEENABLEEXCEPT
   feenableexcept(excepts | fegetexcept());
#elif defined(__MACOSX__) && defined(__SSE__)
   unsigned int es = _MM_GET_EXCEPTION_MASK();
   if (excepts & FE_INEXACT)    es &= ~_MM_MASK_INEXACT;
   if (excepts & FE_UNDERFLOW)  es &= ~_MM_MASK_UNDERFLOW;
   if (excepts & FE_OVERFLOW)   es &= ~_MM_MASK_OVERFLOW;
   if (excepts & FE_DIVBYZERO)  es &= ~_MM_MASK_DIV_ZERO;
   if (excepts & FE_INVALID)    es &= ~_MM_MASK_INVALID;
   _MM_SET_EXCEPTION_MASK(es);
#elif HAVE_FPSETMASK
   fp_except es = fpgetmask();
   if (excepts & FE_INEXACT)    es |= FP_X_IMP;
   if (excepts & FE_UNDERFLOW)  es |= FP_X_UFL;
   if (excepts & FE_OVERFLOW)   es |= FP_X_OFL;
   if (excepts & FE_DIVBYZERO)  es |= FP_X_DZ;
   if (excepts & FE_INVALID)    es |= FP_X_INV;
   fpsetmask(es);
#else
   warn_notrap();
#endif
}

static void fpu_untrap(int excepts)
{
#ifdef HAVE_FEENABLEEXCEPT
   fedisableexcept(excepts | ~fegetexcept());
#elif defined(__MACOSX__) && defined(__SSE__)
   unsigned int es = _MM_GET_EXCEPTION_MASK();
   if (excepts & FE_INEXACT)    es |= _MM_MASK_INEXACT;
   if (excepts & FE_UNDERFLOW)  es |= _MM_MASK_UNDERFLOW;
   if (excepts & FE_OVERFLOW)   es |= _MM_MASK_OVERFLOW;
   if (excepts & FE_DIVBYZERO)  es |= _MM_MASK_DIV_ZERO;
   if (excepts & FE_INVALID)    es |= _MM_MASK_INVALID;
   _MM_SET_EXCEPTION_MASK(es);
#elif HAVE_FPSETMASK
   fp_except es = fpgetmask();
   if (excepts & FE_INEXACT)    es &= ~FP_X_IMP;
   if (excepts & FE_UNDERFLOW)  es &= ~FP_X_UFL;
   if (excepts & FE_OVERFLOW)   es &= ~FP_X_OFL;
   if (excepts & FE_DIVBYZERO)  es &= ~FP_X_DZ;
   if (excepts & FE_INVALID)    es &= ~FP_X_INV;
   fpsetmask(es);
#else
   warn_notrap();
#endif
}

/* trap some or all exceptions */
DX(xfpu_trap)
{
   int excepts = parse_excepts(arg_number, arg_array);
   fpu_trap(excepts);
   return NIL;
}

/* mask some or all exceptions */
DX(xfpu_untrap)
{
   int excepts = parse_excepts(arg_number, arg_array);
   fpu_untrap(excepts);
   return NIL;
}

/* test for named exceptions flags */
DX(xfpu_test)
{
   int excepts = parse_excepts(arg_number, arg_array);
   excepts = fetestexcept(excepts);
   return unparse_excepts(excepts);
}

/* clear named exception flags */
DX(xfpu_clear)
{
   int excepts = parse_excepts(arg_number, arg_array);
   feclearexcept(excepts);
   return NIL;
}


static void set_fpu_precision(fpu_control_t prec)
{
#ifdef _FPU_GETCW
   fpu_control_t mode; 
   _FPU_GETCW(mode);
   mode &= ~(_FPU_SINGLE | _FPU_DOUBLE | _FPU_EXTENDED);
   mode |= prec;
   _FPU_SETCW(mode);
#else
   static int not_warned = 3;
   if (not_warned) {
      fprintf(stderr, "*** Warning: Setting FPU precision not supported on this platform\n");
      not_warned--;
   }
#endif
}


/* configure FPU internal precision */
DX(xfpu_precision)
{
   char *keywords = KEY_SINGLE KEY_DOUBLE KEY_EXTENDED;
   fpu_control_t mode = 0;
   for (int i=1; i<= arg_number; i++) {
      char *kw = strstr(keywords, nameof(ASYMBOL(i)));
      switch (1+(kw-keywords)) {
      case 1:
         mode = _FPU_SINGLE;
         break;
         
      case sizeof(KEY_SINGLE): 
         mode = _FPU_DOUBLE;
         break;
         
      case sizeof(KEY_SINGLE KEY_DOUBLE):
         mode = _FPU_EXTENDED;
         break;
         
      default:
         RAISEFX(errmsg_keyword, APOINTER(i));
      }
   }
   set_fpu_precision(mode);
   return NIL;
}

/* configure FPU rounding behavior */
DX(xfpu_round)
{
   char *keywords = KEY_TOWARDZERO KEY_TONEAREST KEY_UPWARD KEY_DOWNWARD;
   int mode = 0;
   for (int i=1; i<= arg_number; i++) {
      char *kw = strstr(keywords, nameof(ASYMBOL(i)));
      switch (1+(kw-keywords)) {
      case 1:
         mode = FE_TOWARDZERO;
         break;
           
      case sizeof(KEY_TOWARDZERO): 
         mode = FE_TONEAREST; 
         break;
         
      case sizeof(KEY_TOWARDZERO KEY_TONEAREST):
         mode = FE_UPWARD;
         break;

      case sizeof(KEY_TOWARDZERO KEY_TONEAREST KEY_UPWARD):
         mode = FE_DOWNWARD;
         break;

      default:
         RAISEFX(errmsg_keyword, APOINTER(i));
      }
   }
   if (fesetround(mode))
      fprintf(stderr, "*** Warning: Could not change FPU rounding mode\n");
   return NIL;
}


/* report in string what FPU status flags are set */
char *sprint_excepts(char *buf, int excepts)
{
   buf[0] = '\0';

   if (excepts & FE_DIVBYZERO)
      buf = strcat(buf, " "KEY_DIVBYZERO);

   if (excepts & FE_INVALID)
      buf = strcat(buf, " "KEY_INVALID);

   if (excepts & FE_INEXACT)
      buf = strcat(buf, " "KEY_INEXACT);

   if (excepts & FE_OVERFLOW)
      buf = strcat(buf, " "KEY_OVERFLOW);

   if (excepts & FE_UNDERFLOW)
      buf = strcat(buf, " "KEY_UNDERFLOW);

   return buf;
}

DX(xfpu_info)
{
   ARG_NUMBER(0);
   char buffer[256], *buf = buffer;

#ifdef _FPU_GETCW
   print_char('\n');
   print_string("Operating precision   :");
   fpu_control_t mode;
   _FPU_GETCW(mode);
   switch (mode & (_FPU_SINGLE | _FPU_DOUBLE | _FPU_EXTENDED)) {
   case _FPU_SINGLE  : print_string(" "KEY_SINGLE); break;
   case _FPU_DOUBLE  : print_string(" "KEY_DOUBLE); break;
   case _FPU_EXTENDED: print_string(" "KEY_EXTENDED); break;
   default: print_string(" Unknown"); break;
   }
#else
   print_char('\n');
   print_string("Operating precision   :");
   switch (FLT_EVAL_METHOD) {
   case 0: print_string(" "KEY_SINGLE); break;
   case 1: print_string(" "KEY_DOUBLE); break;
   case 2: print_string(" "KEY_EXTENDED); break;
   default: print_string(" Unknown"); break;
   }
#endif   


#if 0
   print_char('\n');
   print_string("Rounding mode         :");
#pragma STDC FENV_ACCESS ON
   switch (FLT_ROUNDS) {
   case 0: print_string(" "KEY_TOWARDZERO); break;
   case 1: print_string(" "KEY_TONEAREST); break;
   case 2: print_string(" "KEY_UPWARD); break;
   case 3: print_string(" "KEY_DOWNWARD); break;
   default: print_string(" Unknown"); break;
   }
#else
   print_char('\n');
   print_string("Rounding mode         :");
   switch (fegetround()) {
   case FE_TOWARDZERO: print_string(" "KEY_TOWARDZERO); break;
   case FE_TONEAREST:  print_string(" "KEY_TONEAREST); break;
   case FE_UPWARD:     print_string(" "KEY_UPWARD); break;
   case FE_DOWNWARD:   print_string(" "KEY_DOWNWARD); break;
   default: print_string(" Unknown"); break;
   }
#endif

   print_char('\n');
   print_string("FPU exceptions set    :");
   print_string(sprint_excepts(buf, fetestexcept(FE_ALL_EXCEPT)));
   print_char('\n');

#if HAVE_FEENABLEEXCEPT
   print_string("FPU exceptions trapped:");
   print_string(sprint_excepts(buf, fegetexcept()));
   print_char('\n');
#elif defined(__MACOSX__) && defined(__SSE__)
   print_string("FPU exceptions trapped:");
   int excepts = FE_ALL_EXCEPT;
   unsigned int es = _MM_GET_EXCEPTION_MASK();
   if (es & _MM_MASK_INEXACT)   excepts &= ~FE_INEXACT;
   if (es & _MM_MASK_UNDERFLOW) excepts &= ~FE_UNDERFLOW;
   if (es & _MM_MASK_OVERFLOW)  excepts &= ~FE_OVERFLOW;
   if (es & _MM_MASK_DIV_ZERO)  excepts &= ~FE_DIVBYZERO;
   if (es & _MM_MASK_INVALID)   excepts &= ~FE_INVALID;
   print_string(sprint_excepts(buf, excepts));
   print_char('\n');
#elif HAVE_FPSETMASK
   print_string("FPU exceptions trapped:");
   fp_except es = fpgetmask();
   int excepts = 0;
   if (es & FP_X_IMP) excepts |= FE_INEXACT;
   if (es & FP_X_UFL) excepts |= FE_UNDERFLOW;
   if (es & FP_X_OFL) excepts |= FE_OVERFLOW;
   if (es & FP_X_DZ)  excepts |= FE_DIVBYZERO;
   if (es & FP_X_INV) excepts |= FE_INVALID;
   print_string(sprint_excepts(buf, excepts));
   print_char('\n');
#endif
   return NIL;
}

void fpu_reset(void)
{
//#pragma STDC FENV_ACCESS ON
   fesetenv(FE_DFL_ENV);
   fpu_trap(FE_OVERFLOW);
#ifdef _FPU_GETCW
   set_fpu_precision(_FPU_EXTENDED);
#endif
}

DX(xfpu_reset)
{
   fpu_reset();
   return NIL;
}


void init_nan(void)
{
   /* set up and save standard fpu environment */
   fpu_reset();

   dx_define("infinityp", xinfinityp);
   dx_define("nanp", xnanp);
   dx_define("eps", xeps);
   dy_define("with-fpu-env", ywith_fpu_env);
   dx_define("fpu-info", xfpu_info);
   dx_define("fpu-reset", xfpu_reset);
   dx_define("fpu-trap", xfpu_trap);
   dx_define("fpu-untrap", xfpu_untrap);
   dx_define("fpu-test", xfpu_test);
   dx_define("fpu-clear", xfpu_clear);
   dx_define("fpu-round", xfpu_round);
   dx_define("fpu-precision", xfpu_precision);

   // avoid compiler warning about unused function
   (void)&warn_notrap;
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
