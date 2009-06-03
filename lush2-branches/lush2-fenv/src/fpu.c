/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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

#include "header.h"

#ifdef HAVE_FPU_CONTROL_H
# include <fpu_control.h>
#elif HAVE___SETFPUCW
# define _FPU_SETCW(cw) __setfpucw(cw)
typedef int fpu_control_t;
#endif

#define _GNU_SOURCE
#include <fenv.h>
#include <signal.h>
#include <float.h>

typedef RETSIGTYPE (*SIGHANDLERTYPE)();

#ifdef linux
# ifdef __hppa__          /* Checked (debian) 2003-07-14 */
#  define BROKEN_SIGFPE
# endif
# ifdef __mips__          /* Checked (debian) 2004-03-06 */
#   define BROKEN_SIGFPE
# endif
#endif

static int ieee_present = 0;
static int fpe_inv = 0;
static int fpe_ofl = 0;
static fenv_t standard_fenv;

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


/* setup_fpu -- configure FPU exception mask */

int setup_fpu(int doINV, int doOFL)
{
   
#if defined(WIN32)
   
   /* Win32 uses _controlfp() */
   unsigned int mask = _controlfp(0,0);
   fpe_inv = doINV;
   fpe_ofl = doOFL;
   if (doINV) mask&=(~_EM_INVALID); else mask|=(_EM_INVALID);
   if (doOFL) mask&=(~_EM_OVERFLOW); else mask|=(_EM_OVERFLOW);
   if (doOFL) mask&=(~_EM_ZERODIVIDE); else mask|=(_EM_ZERODIVIDE);
   _controlfp(mask, _MCW_EM);
   return 1;

#elif defined(HAVE_FPSETMASK)

   /* SysVr4 defines fpsetmask() */
   
   int mask = 0;
   fpe_inv = doINV;
   fpe_ofl = doOFL;
#ifdef FP_X_INV
   if (doINV) mask |= FP_X_INV;
#endif
#ifdef FP_X_OFL
   if (doOFL) mask |= FP_X_OFL;
#endif
#ifdef FP_X_DZ
   if (doOFL) mask |= FP_X_DZ;
#endif
   fpsetmask( mask );
   return 1;
  
#elif defined(HAVE_FEENABLEEXCEPT)

   /* GLIBC-2.2 model */
   
   int mask1 = 0;
   int mask2 = 0;
   fpe_inv = doINV;
   fpe_ofl = doOFL;

#ifdef FE_INVALID
   if (doINV) 
      mask1 |= FE_INVALID;
   else
      mask2 |= FE_INVALID;    
#endif
#ifdef FE_DIVBYZERO
   if (doOFL) 
      mask1 |= FE_DIVBYZERO;
   else
      mask2 |= FE_DIVBYZERO;    
#endif
#ifdef FE_OVERFLOW
   if (doOFL) 
      mask1 |= FE_OVERFLOW;
   else
      mask2 |= FE_OVERFLOW;    
#endif
   feenableexcept(mask1);
   fedisableexcept(mask2);
   return 1;

#elif defined(HAVE_FPU_CONTROL_H)

   /* Older GLIBC */
  
   fpu_control_t mask = 0;
   fpe_inv = doINV;
   fpe_ofl = doOFL;
   
#ifdef _FPU_DEFAULT
   mask = _FPU_DEFAULT;
#endif
  
#define DO(c,f) mask=((c)?(mask|(f)):(mask&~(f)));

#if defined(__i386__) || defined(__alpha__)
#ifdef _FPU_MASK_IM
   DO(!doINV, _FPU_MASK_IM);
#endif
#ifdef _FPU_MASK_OM
   DO(!doOFL, _FPU_MASK_OM);
#endif
#ifdef _FPU_MASK_ZM
   DO(!doOFL, _FPU_MASK_ZM);
#endif
#else
#ifdef _FPU_MASK_IM
   DO(doINV, _FPU_MASK_IM);
#endif
#ifdef _FPU_MASK_OM
   DO(doOFL, _FPU_MASK_OM);
#endif
#ifdef _FPU_MASK_ZM
   DO(doOFL, _FPU_MASK_ZM);
#endif
#ifdef _FPU_MASK_V
   DO(doINV, _FPU_MASK_V);
#endif
#ifdef _FPU_MASK_O
   DO(doOFL, _FPU_MASK_O);
#endif
#ifdef _FPU_MASK_Z
   DO(doOFL, _FPU_MASK_Z);
#endif
#ifdef _FPU_MASK_OPERR
   DO(doINV, _FPU_MASK_OPERR);
#endif
#ifdef _FPU_MASK_OVFL
   DO(doOFL, _FPU_MASK_OPERR);
#endif
#ifdef _FPU_MASK_DZ
   DO(doOFL, _FPU_MASK_DZ);
#endif
#endif
  /* continue */

#ifdef _FPU_SETCW
   _FPU_SETCW( mask );
   return 1;
#undef DO
#endif

#endif
   
   /* Default */
   return 0;
}


/* probe_fpe_irq -- signal handler for testing SIGFPE */

static int fpe_flag;
static int fpe_isnan;

static RETSIGTYPE probe_fpe_irq(void)
{
#ifdef WIN32
   _clearfp();
#endif
   fpe_flag = 1;
   /* Avoid incorrect restarts */
   signal(SIGFPE, SIG_IGN);
}

static void probe_fpe(void)
{
   signal(SIGFPE, (SIGHANDLERTYPE)probe_fpe_irq);
   fpe_isnan = isnan(3.0 + NAN);
#ifdef __alpha__
   asm ("trapb");
#endif
   signal(SIGFPE, SIG_IGN);
}

/* configure_fpu_traps -- decide what FPU exceptions to trap on */

static void configure_fpu_traps(void)
{
   /* Setup fpu exceptions */
   setup_fpu(true, true);

   /* Check NAN behavior */
   while (ieee_present) {
      signal(SIGFPE, SIG_IGN);
      /* Check whether "INV" exception must be masked */
      fpe_flag = 0;
      probe_fpe();
      if (! fpe_flag) break;
      /* Check whether all exceptions must be masked */
      fpe_flag = 0;
      setup_fpu(false, true);
      probe_fpe();
      if (! fpe_flag) break;
      /* Check whether signal must be ignored */
      fpe_flag = 0;
      setup_fpu(false, false);
      probe_fpe();
      if (! fpe_flag) break;
      fpe_flag = 0;
      return;
   }
}


#ifdef _FPU_GETCW
DY(ywith_fpu_env)
{
   struct context mycontext;
   fpu_control_t saved_fenv;

   _FPU_GETCW(saved_fenv);
   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1)) {
      _FPU_SETCW(saved_fenv);
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(ARG_LIST);

   context_pop();
   _FPU_SETCW(saved_fenv);
   return answer;
}
#else
DY(ywith_fpu_env)
{
   struct context mycontext;
   fenv_t current_fenv, *saved_fenv = &current_fenv;

   if (fegetenv(saved_fenv)) {
      fprintf(stderr, "*** Warning: could not save FPU environment, restoring standard environment\n");
      saved_fenv = &standard_fenv;
   }

   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1)) {
      fesetenv(saved_fenv);
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(ARG_LIST);

   context_pop();
   fesetenv(saved_fenv);
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
         RAISEFX(errmsg_keyword, APOINTER(i));
      }
   }
   return excepts;
}

/* mask some or all exceptions */
DX(xfpu_mask)
{
#ifdef HAVE_FEENABLEEXCEPT
   int excepts = parse_excepts(arg_number, arg_array);
   fedisableexcept(excepts | ~fegetexcept());
#else
   static bool not_warned = true;
   if (not_warned) {
      fprintf(stderr, "*** Warning: setting FPU exception mask not supported on this platform\n");
      not_warned = false;
   }
#endif
   return NIL;
}

/* trap some or all exceptions */
DX(xfpu_unmask)
{
#ifdef HAVE_FEENABLEEXCEPT
   int excepts = parse_excepts(arg_number, arg_array);
   feenableexcept(excepts | fegetexcept());
#else
   static bool not_warned = true;
   if (not_warned) {
      fprintf(stderr, "*** Warning: setting FPU exception mask not supported on this platform\n");
      not_warned = false;
   }
#endif
   return NIL;
}


void set_fpu_precision(fpu_control_t prec)
{
#ifdef _FPU_GETCW
   fpu_control_t mode; 
   _FPU_GETCW(mode);
   mode &= ~(_FPU_SINGLE | _FPU_DOUBLE | _FPU_EXTENDED);
   mode |= prec;
   _FPU_SETCW(mode);
#else
   static bool not_warned = true;
   if (not_warned) {
      fprintf(stderr, "*** Warning: setting FPU precision not supported on this platform\n");
      not_warned = false;
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
      fprintf(stderr, "could not change FPU rounding mode\n");
   return NIL;
}


/* report in string what FPU status flags are set */
char *sprint_excepts(char *buf, int excepts)
{
   buf[0] = '\0';

#ifdef FE_DIVBYZERO
   if (excepts & FE_DIVBYZERO)
      buf = strcat(buf, " "KEY_DIVBYZERO);
#endif
#ifdef FE_INVALID
   if (excepts & FE_INVALID)
      buf = strcat(buf, " "KEY_INVALID);
#endif
#ifdef FE_INEXACT
   if (excepts & FE_INEXACT)
      buf = strcat(buf, " "KEY_INEXACT);
#endif
#ifdef FE_OVERFLOW
   if (excepts & FE_OVERFLOW)
      buf = strcat(buf, " "KEY_OVERFLOW);
#endif
#ifdef FE_UNDERFLOW
   if (excepts & FE_UNDERFLOW)
      buf = strcat(buf, " "KEY_UNDERFLOW);
#endif
   return buf;
}

DX(xfpu_info)
{
   ARG_NUMBER(0);
   char buffer[256], *buf = buffer;
// #pragma STDC FENV_ACCESS ON

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

#if defined(HAVE_FEENABLEEXCEPT)
   print_string("FPU exceptions trapped:");
   print_string(sprint_excepts(buf, fegetexcept()));
   print_char('\n');
#endif

   return NIL;
}

void fpu_reset(void)
{
   fesetenv(&standard_fenv);
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
   ieee_present = ( sizeof(real)==8 && sizeof(int)==4 );

   /* set up and save standard fpu environment */
   fesetenv(FE_DFL_ENV);
   configure_fpu_traps();
   assert(fetestexcept(FE_ALL_EXCEPT)==0);
   fegetenv(&standard_fenv);

   dx_define("infinityp", xinfinityp);
   dx_define("nanp", xnanp);
   dx_define("eps", xeps);
   dy_define("with-fpu-env", ywith_fpu_env);
   dx_define("fpu-info", xfpu_info);
   dx_define("fpu-reset", xfpu_reset);
   dx_define("fpu-mask", xfpu_mask);
   dx_define("fpu-unmask", xfpu_unmask);
   dx_define("fpu-round", xfpu_round);
   dx_define("fpu-precision", xfpu_precision);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
