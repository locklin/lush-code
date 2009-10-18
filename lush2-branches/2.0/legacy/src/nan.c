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

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>
#endif
#ifdef HAVE_FPU_CONTROL_H
# include <fpu_control.h>
#endif
#ifdef HAVE_FENV_H
# include <fenv.h>
#endif
#ifdef WIN32
# include <float.h>
#endif
#include <signal.h>

typedef RETSIGTYPE (*SIGHANDLERTYPE)();

#ifdef linux
# ifdef __hppa__          /* Checked (debian) 2003-07-14 */
#  define BROKEN_SIGFPE
# endif
# ifdef __mips__          /* Checked (debian) 2004-03-06 */
#   define BROKEN_SIGFPE
# endif
#endif


/*================
  The IEEE spec are re-created. 
  Maybe it would be better to use ieee_functions ?
  ================*/

static int ieee_nanf[1];
static int ieee_inftyf[1];
static int ieee_nand[2];
static int ieee_inftyd[2];
static int ieee_present = 0;
static int fpe_inv = 0;
static int fpe_ofl = 0;

/*================
  The following functions are the only primitives for Nan
  for TL C code.
  ================*/

flt getnanF (void)
{
   if (ieee_present <= 0)
      error(NIL,"IEEE754 is not supported on this computer",NIL);
   char *p = (char *)ieee_nanf;
   return * (flt*) p;
}

flt infinityF (void)
{
   if (ieee_present <= 0)
      error(NIL,"IEEE754 is not supported on this computer",NIL);
   char *p = (char *)ieee_inftyf;
   return * (flt*) p;
}

int isinfF(flt x)
{
   union { float f; int i; } u;
   int ix;
   if (sizeof(flt)!=sizeof(ieee_nanf))
      return 0;
   if (ieee_present <= 0)
      return 0;
   u.f = x;
   ix = u.i;
   ix &= 0x7fffffff;
   ix ^= 0x7f800000;
   return (ix == 0);
}

int isnanF(flt x)
{
   union { float f; int i; } u;
   int ix;
   if (sizeof(flt)!=sizeof(ieee_nanf))
      return 0;
   if (ieee_present <= 0)
      return 0;
   u.f = x;
   ix = u.i;
   ix &= 0x7fffffff;
   ix = 0x7f800000 - ix;
   return (ix < 0);
}

real getnanD (void)
{
   if (ieee_present <= 0)
      error(NIL,"IEEE754 is not supported on this computer",NIL);
   char *p = (char *)ieee_nand;
   return * (real*) p;
}

real infinityD (void)
{
   if (ieee_present <= 0)
      error(NIL,"IEEE754 is not supported on this computer",NIL);
   char *p = (char *)ieee_inftyd;
   return * (real*) p;
}

int isinfD(real x)
{
   int ix, jx, a;
   union { int i; char c[sizeof(int)]; } u;
   union { real r; int i[2]; } v;
   if (sizeof(real)!=sizeof(ieee_nand))
      return 0;
   if (ieee_present <= 0)
      return 0;
   u.i = 1;
   a = u.c[0];
   v.r = x;
   ix = v.i[a];
   jx = v.i[1-a];
   ix ^= 0x7ff00000;
   if (ix&0x7fffffff)
      return 0;
   if (jx == 0)
      return 1;
   return 0;
}

int isnanD(real x)
{
   int ix, jx, a;
   union { int i; char c[sizeof(int)]; } u;
   union { real r; int i[2]; } v;
   if (sizeof(real)!=sizeof(ieee_nand))
      return 0;
   if (ieee_present <= 0)
      return 0;
   u.i = 1;
   a = u.c[0];
   v.r = x;
   ix = v.i[a];
   jx = v.i[1-a];
   ix ^= 0x7ff00000;
   if (ix&0x7ff00000)
      return 0;
   if (ix & 0xfffff || jx)
      return 1;
   return 0;
}

/* this requires C99 support */

double epsD(double x) {
   x = copysign(x, 1.0);
   double y = nextafter(x, INFINITY);
   return y-x;
}

float epsF(float x) {
   x = copysignf(x, 1.0);
   float y = nextafterf(x, INFINITY);
   return y-x;
}

/*================
  The following functions are the Lush primitives
  dedicated to Nan at Lush level.
  ================*/


/* DX(xinfinity) */
/* { */
/*   ARG_NUMBER(0); */
/*   return NEW_NUMBER(infinityD()); */
/* } */


DX(xinfinityp)
{
   ARG_NUMBER(1);
   return isinfD(AREAL(1)) ? t() : NIL;
}


/* DX(xnan) */
/* { */
/*   ARG_NUMBER(0); */
/*   if (ieee_present > 0) */
/*     return NEW_NUMBER(getnanD()); */
/*   return NIL; */
/* } */

DX(xnanp)
{
   ARG_NUMBER(1);
   return isnanD(AREAL(1)) ? t() : NIL;
}

DX(xnot_nan)
{
   ARG_NUMBER(1);
   return isnanD(AREAL(1)) ? NIL : APOINTER(1);
}

DX(xeps)
{
   ARG_NUMBER(1);
   double x = AREAL(1);
   if (isinfD(x) || isnanD(x))
      RAISEF("eps not defined for number", NEW_NUMBER(x));
   return NEW_NUMBER(epsD(x));
}

/*================
  Chek FPU exception mode
  ================*/

/* setup_fpu -- set/reset FPU exception mask */

static int setup_fpu(int doINV, int doOFL)
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
  
#elif defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)

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
  
   int mask = 0;
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
#if defined(HAVE___SETFPUCW)
   __setfpucw( mask );
   return 1;
#elif defined(_FPU_SETCW)
   _FPU_SETCW( mask );
   return 1;
#undef DO
#endif

#endif
   
   /* Default */
   return 0;
}


/* fpe_irq -- signal handler for floating point exception */

#ifdef WIN32
static void fpe_irq(int sig, int num)
{
   _clearfp();
   if (ieee_present)
      setup_fpu(fpe_inv, fpe_ofl);
   signal(SIGFPE, (void(*)(int))fpe_irq);
   switch(num)
   {
   case _FPE_INVALID:
      error(NIL, "Floating exception: invalid", NIL);
   case _FPE_OVERFLOW:
      error(NIL, "Floating exception: overflow", NIL);
   case _FPE_ZERODIVIDE:
      error(NIL, "Floating exception: division by zero", NIL);
   }
}
#else
static RETSIGTYPE fpe_irq(void)
{
   if (ieee_present)
      setup_fpu(fpe_inv, fpe_ofl);
   signal(SIGFPE, (SIGHANDLERTYPE)fpe_irq);
   error(NIL, "Floating exception", NIL);
}
#endif

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
   fpe_isnan = isnanD(3.0 + getnanD());
#ifdef __alpha__
   asm ("trapb");
#endif
   signal(SIGFPE, SIG_IGN);
}

/* set_fpe_irq -- set signal handler for FPU exceptions */

static void set_fpe_irq(void)
{
   /* Setup fpu exceptions */
   setup_fpu(true, true);
   /* Check NAN behavior */
#ifdef BROKEN_SIGFPE
   ieee_present = 0;
#else
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
#endif
   /* We can now setup the real fpe handler */
   signal(SIGFPE, (SIGHANDLERTYPE)fpe_irq);
#ifdef HAVE_IEEE_HANDLER
   ieee_handler("set","common",fpe_irq);
#endif
}



DY(yprogn_without_fpe)
{
   int inv,ofl;
   struct context mycontext;

   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1)) {
      setup_fpu(fpe_inv, fpe_ofl);
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }
   inv = fpe_inv;
   ofl = fpe_ofl;
   setup_fpu(false, false);
   fpe_inv = inv;
   fpe_ofl = ofl;
   at *answer = progn(ARG_LIST);
   context_pop();
   return answer;
}




/*================
  Interpretor initialization
  ================*/

void init_nan(void)
{
   if (sizeof(flt)==4 && sizeof(real)==8 && sizeof(int)==4) {
      /* Setup bit patterns */
      int a = 1;
      a = *(char*) &a;
      ieee_nanf[0]   = 0xffc00000;
      ieee_inftyf[0] = 0x7f800000;
      ieee_nand[a]     = 0xfff80000;
      ieee_nand[1-a]   = 0x00000000;
      ieee_inftyd[a]   = 0x7ff00000;
      ieee_inftyd[1-a] = 0x00000000;
      /* Setup FPE IRQ with first evaluation of IEEE compliance */
      ieee_present = ( sizeof(real)==8 && sizeof(int)==4 );
      set_fpe_irq();
      /* Check that NaN works as expected */
      if (ieee_present) {
         char *nand = (char *)ieee_nand;
         char *inftyd = (char *)ieee_inftyd;
         if (!isnanD(*(real*)nand + 3.0) ||
             !isnanD(*(real*)nand - 3.0e40) ||
             !isinfD(*(real*)inftyd - 3.0e40) ) {
            ieee_present = 0;
            set_fpe_irq();
         }
      }
   }
   /* Define functions */
   /*  dx_define("nan"    , xnan    ); */
   dx_define("nanp"   , xnanp   );
   /* dx_define("infinity", xinfinity); */
   dx_define("infinityp", xinfinityp);
   dx_define("not-nan", xnot_nan); 
   dx_define("eps", xeps);
   dy_define("progn-without-fpe", yprogn_without_fpe);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
