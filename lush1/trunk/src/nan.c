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
 * $Id: nan.c,v 1.3 2002-11-01 05:09:13 profshadoko Exp $
 **********************************************************************/

#include "header.h"

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_FPU_CONTROL_H
#include <fpu_control.h>
#endif
#ifdef WIN32
#include <float.h>
#endif
#include <signal.h>


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

flt
getnanF (void)
{
  if (ieee_present <= 0)
    error(NIL,"IEEE754 is not supported on this computer",NIL);
  return * (flt*) ieee_nanf;
}

flt
infinityF (void)
{
  if (ieee_present <= 0)
    error(NIL,"IEEE754 is not supported on this computer",NIL);
  return * (flt*) ieee_inftyf;
}

int
isinfF(flt x)
{
  float fx = x;
  int ix = *(int*)&fx;
  if (sizeof(flt)!=sizeof(ieee_nanf))
    return 0;
  if (ieee_present <= 0)
    return 0;
  ix &= 0x7fffffff;
  ix ^= 0x7f800000;
  return (ix == 0);
}

int
isnanF(flt x)
{
  float fx = x;
  int ix = *(int*)&fx;
  if (sizeof(flt)!=sizeof(ieee_nanf))
    return 0;
  if (ieee_present <= 0)
    return 0;
  ix &= 0x7fffffff;
  ix = 0x7f800000 - ix;
  return (ix < 0);
}

real
getnanD (void)
{
  if (ieee_present <= 0)
    error(NIL,"IEEE754 is not supported on this computer",NIL);
  return * (real*) ieee_nand;
}

real
infinityD (void)
{
  if (ieee_present <= 0)
    error(NIL,"IEEE754 is not supported on this computer",NIL);
  return * (real*) ieee_inftyd;
}

int
isinfD(real x)
{
  int ix, jx, a = 1;
  if (sizeof(real)!=sizeof(ieee_nand))
    return 0;
  if (ieee_present <= 0)
    return 0;
  a = * (char*) &a;
  ix = ((int*)&x)[a];
  jx = ((int*)&x)[1-a];
  ix ^= 0x7ff00000;
  if (ix&0x7fffffff)
    return 0;
  if (jx == 0)
    return 1;
  return 0;
}

int
isnanD(real x)
{
  int ix, jx, a = 1;
  if (sizeof(real)!=sizeof(ieee_nand))
    return 0;
  if (ieee_present <= 0)
    return 0;
  a = * (char*) &a;
  ix = ((int*)&x)[a];
  jx = ((int*)&x)[1-a];
  ix ^= 0x7ff00000;
  if (ix&0x7ff00000)
    return 0;
  if (ix & 0xfffff || jx)
    return 1;
  return 0;
}


/*================
  The following functions are the Lush primitives
  dedicated to Nan at Lush level.
  ================*/


DX(xinfinity)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(infinityD());
}


DX(xinfinityp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (isinfD(AREAL(1)))
    return true();
  else 
    return NIL;
}


DX(xnan)
{
  ARG_NUMBER(0);
  if (ieee_present > 0)
    return NEW_NUMBER(getnanD());
  return NIL;
}

DX(xnanp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if ( isnanD(AREAL(1)) )
    return true();
  return NIL;
}

DX(xnot_nan)
{
  at *v = NIL;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (!isnanD(AREAL(1)))
    v = APOINTER(1);
  LOCK(v);
  return v;
}




/*================
  Chek FPU exception mode
  ================*/

/* setup_fpu -- set/reset FPU exception mask */

static int
setup_fpu(int doINV, int doOFL)
{

  /* SysVr4 defines fpsetmask() */
#ifdef HAVE_FPSETMASK
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
#endif
  /* GLIBC __setfpucw() emulates i387 */
#ifdef HAVE_FPU_CONTROL_H 
  int mask = 0;
  fpe_inv = doINV;
  fpe_ofl = doOFL;
#ifdef _FPU_DEFAULT
  mask = _FPU_DEFAULT;
#endif
#ifdef _FPU_MASK_DM
  mask |= _FPU_MASK_DM;
#endif
#ifdef _FPU_MASK_UM
  mask |= _FPU_MASK_UM;
#endif
#ifdef _FPU_MASK_PM
  mask |= _FPU_MASK_PM;
#endif
#ifdef _FPU_MASK_IM
  if (doINV) mask&=(~_FPU_MASK_IM); else mask|=(_FPU_MASK_IM);
#endif
#ifdef _FPU_MASK_IM
  if (doOFL) mask&=(~_FPU_MASK_OM); else mask|=(_FPU_MASK_OM);
#endif
#ifdef _FPU_MASK_IM
  if (doOFL) mask&=(~_FPU_MASK_ZM); else mask|=(_FPU_MASK_ZM);
#endif
#ifdef HAVE___SETFPUCW
  __setfpucw( mask );
  return 1;
#elif defined(_FPU_SETCW)
  _FPU_SETCW( mask );
  return 1;
#endif
#endif
  /* Win32 uses _controlfp() */
#ifdef WIN32
  unsigned int mask = _controlfp(0,0);
  if (doINV) mask&=(~_EM_INVALID); else mask|=(_EM_INVALID);
  if (doOFL) mask&=(~_EM_OVERFLOW); else mask|=(_EM_OVERFLOW);
  if (doOFL) mask&=(~_EM_ZERODIVIDE); else mask|=(_EM_ZERODIVIDE);
  _controlfp(mask, _MCW_EM);
  return 1;
#endif
  /* Default */
  return 0;
}


/* fpe_irq -- signal handler for floating point exception */

#ifdef WIN32
static void 
fpe_irq(int sig, int num)
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
static void 
fpe_irq(void)
{
  if (ieee_present)
    setup_fpu(fpe_inv, fpe_ofl);
  signal(SIGFPE, (void*)fpe_irq);
  error(NIL, "Floating exception", NIL);
}
#endif

static void 
nop_irq(void)
{
}


/* probe_fpe_irq -- signal handler for testing SIGFPE */

static sigjmp_buf probe_fpe_jmp;

static void 
probe_fpe_irq(void)
{
#ifdef WIN32
  _clearfp();
#endif
  siglongjmp(probe_fpe_jmp, 1);
}


/* set_fpe_irq -- set signal handler for FPU exceptions */

static void 
set_fpe_irq(void)
{
  /* Setup fpu exceptions */
  setup_fpu(TRUE,TRUE);
  /* Check NAN behavior */
  while (ieee_present)
    {
      /* Check whether "INV" exception must be masked */
      signal(SIGFPE, (void*)probe_fpe_irq);
      if (!sigsetjmp(probe_fpe_jmp, 1)) 
	{ isnanD(3.0 + getnanD()); break; }
      setup_fpu(FALSE,TRUE);
      /* Check whether all exceptions must be masked */
      signal(SIGFPE, (void*)probe_fpe_irq);
      if (!sigsetjmp(probe_fpe_jmp, 1)) 
	{ isnanD(3.0 + getnanD()); break; }
      setup_fpu(FALSE,FALSE);
      /* Check whether signal must be ignored */
      signal(SIGFPE, (void*)probe_fpe_irq);
      if (!sigsetjmp(probe_fpe_jmp, 1)) 
	{ isnanD(3.0 + getnanD()); break; }
      /* Disable FPE signal at OS level.
       * You would think that SIG_IGN would do it,
       * but Linux forces sigfpe anyway.
       */
      signal(SIGFPE, (void*)nop_irq);
      return;
    }
  /* We can now setup the real fpe handler */
  signal(SIGFPE, (void*)fpe_irq);
#ifdef HAVE_IEEE_HANDLER
  ieee_handler("set","common",fpe_irq);
#endif
}



DY(yprogn_without_fpe)
{
  int inv,ofl;
  struct context mycontext;
  at *answer;

  context_push(&mycontext);
  if (sigsetjmp(context->error_jump, 1)) 
    {
      setup_fpu(fpe_inv, fpe_ofl);
      context_pop();
      siglongjmp(context->error_jump, -1L);
    }
  inv = fpe_inv;
  ofl = fpe_ofl;
  setup_fpu(FALSE,FALSE);
  fpe_inv = inv;
  fpe_ofl = ofl;
  answer = progn(ARG_LIST);
  context_pop();
  return answer;
}




/*================
  Interpretor initialization
  ================*/

void
init_nan(void)
{
  if (sizeof(flt)==4 && sizeof(real)==8 && sizeof(int)==4)
    {
      /* Setup bit patterns */
      int a = 1;
      a = * (char*) &a;
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
      if (!isnanD(*(real*)ieee_nand + 3.0) ||
	  !isnanD(*(real*)ieee_nand - 3.0e40) ||
	  !isinfD(*(real*)ieee_inftyd - 3.0e40) )
	{
	  ieee_present = 0;
	  set_fpe_irq();
	}
    }
  /* Define functions */
  dx_define("nan"    , xnan    );
  dx_define("nanp"   , xnanp   );
  dx_define("infinity", xinfinity);
  dx_define("infinityp", xinfinityp);
  dx_define("not-nan", xnot_nan); 
  dy_define("progn-without-fpe", yprogn_without_fpe);
}
