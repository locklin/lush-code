/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/***********************************************************************
  SN3:
  (PS) & (LYB) 91
  ******************************************************************/

/******************************************************************************
 *
 *	DZ_MACHINE.C.  This is a virtual stack based machine for numerical 
 *	expressions.  This program should be completely independent of the 
 *	rest except for instruction requiring call to the outside.
 *
 ******************************************************************************/

#ifndef NOLISP
#include "header.h"
#else
#include "fltlib.h"
#include "dz_machine.h"
#endif

/* Don't malloc that! */
flt dz_stack[DZ_STACK_SIZE];

typedef union dz_inst inst;

#ifdef DZ_TRACE
extern int dz_trace;
#endif


/* The first `dz_offname` characters of an opcode describe 
 * its operands and its stack behavior. This
 * information is used by the compiler.
 *
 * 1st character: operand
 *   'o':none 'i':immediat, 'r':stack_relatif, 
 *   'l':pc_relatif, 'n':n immediate operands
 *
 * 2nd character: stack move
 *   '>':push1, 'o':no change, '<':pop1, 'a':POP@,
 *   '*':terminal, 'B':BEGFOR, ENDFOR
 *
 * 3rd character: always space...
 */

int dz_offname = 3;

char *dz_opnames[] = {
  /* Misc */
  "oo <<RETURN>>",
  "oo NOP", 
  "oo STACK", "oo PRINT",
  "o* ERROR",
  
  /* Access */
  "o< POP", "o> DUP",
  "i> PUSH#", "r> PUSH@",
  "ra POP@",
  "r< SET@",
  
  /* Monadic */
  "oo MINUS", "oo INVERT",
  "oo ADD1", "oo SUB1", "oo MUL2", "oo DIV2",

  "oo SGN", "oo ABS", "oo INT", "oo SQRT", "oo PIECE", "oo RECT",
  "oo SIN", "oo COS", "oo TAN", "oo ASIN", "oo ACOS", "oo ATAN",
  "oo EXP", "oo EXPM1", "oo LOG", "oo LOG1P",
  "oo TANH", "oo COSH", "oo SINH", "oo ATANH",
  "oo QTANH", "oo QDTANH",
  "oo QSTDSIGMOID", "oo QDSTDSIGMOID",
  "oo QEXPMX", "oo QDEXPMX",
  "oo QEXPMX2", "oo QDEXPMX2",
  
  /* Special */
  "o> RAND", "o> GAUSS",
  "no SPLINE", "no DSPLINE",
  
  /* Diadic */
  "io ADD#", "ro ADD@", "o< ADD",
  "io MUL#", "ro MUL@", "o< MUL",
  "io SUB#", "ro SUB@", "o< SUB",
  "io DIV#", "ro DIV@", "o< DIV",
  "io MAX#", "ro MAX@", "o< MAX",
  "io MIN#", "ro MIN@", "o< MIN",
  "io MODI#", "ro MODI@", "o< MODI",
  "io DIVI#", "ro DIVI@", "o< DIVI",
  "io POWER#", "ro POWER@", "o< POWER",
  
  /* Branches */
  "l* BR",
  "l< BREQ", "l< BRNEQ", "l< BRGT",
  "l< BRLT", "l< BRGEQ", "l< BRLEQ",

  /* For loops */
  "lB BEGFOR",
  "lB ENDFOR",

/* Tag */
  0
};



/*
 *  Generation of the opcodes
 *  1) copy names
 *  2) emacs macro
 *      C-X ( C-S " C-B C-D O P _ C-S " C-B C-D C-X )
 *
 */

enum opcodes {
  /* Misc */
  OP_RETURN,
  OP_NOP, 
  OP_STACK, 
  OP_PRINT,
  OP_ERROR,
  
  /* Access */
  OP_POP, OP_DUP,
  OP_PUSH_i, OP_PUSH_r,
  OP_POP_r,
  OP_SET_r,
  
  /* Monadic */
  OP_MINUS, OP_INVERT,
  OP_ADD1, OP_SUB1, OP_MUL2, OP_DIV2,
  OP_SGN, OP_ABS, OP_INT, OP_SQRT, OP_PIECE, OP_RECT,
  OP_SIN, OP_COS, OP_TAN, OP_ASIN, OP_ACOS, OP_ATAN,
  OP_EXP, OP_EXPM1, OP_LOG, OP_LOG1P,
  OP_TANH, OP_COSH, OP_SINH, OP_ATANH,
  OP_QTANH, OP_QDTANH,
  OP_QSTDSIGMOID, OP_QDSTDSIGMOID,
  OP_QEXPMX, OP_QDEXPMX,
  OP_QEXPMX2, OP_QDEXPMX2,
  
  /* Special */
  OP_RAND, OP_GAUSS,
  OP_SPLINE_n, OP_DSPLINE_n,
  
  /* Diadic */
  OP_ADD_i, OP_ADD_r, OP_ADD,
  OP_MUL_i, OP_MUL_r, OP_MUL,
  OP_SUB_i, OP_SUB_r, OP_SUB,
  OP_DIV_i, OP_DIV_r, OP_DIV,
  OP_MAX_i, OP_MAX_r, OP_MAX,
  OP_MIN_i, OP_MIN_r, OP_MIN,
  OP_MODI_i, OP_MODI_r, OP_MODI,
  OP_DIVI_i, OP_DIVI_r, OP_DIVI,
  OP_POWER_i, OP_POWER_r, OP_POWER,
  
  /* Branches */
  OP_BR_l, 
  OP_BREQ_l, OP_BRNEQ_l, OP_BRGT_l, OP_BRLT_l, OP_BRGEQ_l, OP_BRLEQ_l,

  /* For loops */
  OP_BEGFOR_l,
  OP_ENDFOR_l,
};



#ifdef DZ_TRACE

static void
  trace_inst(ad,pc)
int ad;
inst *pc;
{
  char *s;
  int i;

  s = dz_opnames[pc->code.op];
  printf("%04d: %s",ad,s+dz_offname);

  switch (s[0]) {
  case 'l':
    printf(" %d",pc->code.arg);
    break;
  case 'r':
    printf(" %d",-pc->code.arg);
    break;
  case 'i':
    printf(" %.3f",(++pc)->constant);
    break;
  case 'n':
    printf(" %d",pc->code.arg);
    for (i=0;i<pc->code.arg;i++)
      printf(",%-.3f",pc[i+1].constant);
    break;
  }
  printf("\n");
}

#endif


static void
  print_stack(pc,sp,x)
int pc;
flt *sp;
flt x;
{
  printf("%04d: S=[%7.3f",pc,x);
  while (sp>dz_stack)
    printf(" %7.3f",*--sp);
  printf("]\n");
}

static void
  print_reg(pc,x)
int pc;
flt x;
{
  printf("%04d: X=%7.3f\n",pc,x);
}


flt dz_execute( top,dz )
flt top;
struct dz_cell *dz;
{

  register flt x;
  register inst *pc;
  register flt *sp;
  register flt y;
  register int arg,op;
  
  x = top;
  pc = dz->program;
  sp = dz_stack + dz->num_arg - 1;

 loop:
  
#ifdef DZ_TRACE
  if (dz_trace)
    trace_inst(pc-dz->program,pc);
#endif
  
  op = pc->code.op;
  pc++;
  
  switch ( op ) {
    
    /* Misc */
  case OP_RETURN:
    return x;
  case OP_NOP:
    break;  
  case OP_STACK:
    print_stack(pc-dz->program,sp,x);
    break;
  case OP_PRINT:
    print_reg(pc-dz->program,x);
    break;
  case OP_ERROR:
    error(NIL,"opcode ERROR: Control expression must return a number.",NIL);
    break;
    
    /* Access */
  case OP_POP:
    x = *--sp;
    break;
  case OP_DUP:
    *sp++ = x;
    break;
  case OP_PUSH_i:
    *sp++ = x;
    x = (pc++)->constant;
    break;
  case OP_PUSH_r:
    *sp = x;
    x = sp[pc[-1].code.arg];
    sp++;
    break;
    
  case OP_POP_r:
    sp = sp+pc[-1].code.arg;
    break;
    
  case OP_SET_r:
    sp[pc[-1].code.arg] = x;
    x = *--sp;
    break;
    
    /* Monadic */
#define MONADIC(op,prog)\
  case op: prog; break

    MONADIC(OP_MINUS, x = -x );
    MONADIC(OP_INVERT, x = 1/x );
    MONADIC(OP_ADD1, x += Fone );
    MONADIC(OP_SUB1, x -= Fone );
    MONADIC(OP_MUL2, x *= Ftwo );
    MONADIC(OP_DIV2, x /= Ftwo );
    MONADIC(OP_SGN, x = Fsgn(x) );
    MONADIC(OP_ABS, x = Fabs(x) );
    MONADIC(OP_INT, x = Ffloor(x) );
    MONADIC(OP_SQRT, x = Fsqrt(x) );

    MONADIC(OP_PIECE, x = Fpiece(x) );
    MONADIC(OP_RECT, x = Frect(x) );
    MONADIC(OP_SIN, x = Fsin(x) );
    MONADIC(OP_COS, x = Fcos(x) );
    MONADIC(OP_TAN, x = Ftan(x) );
    MONADIC(OP_ASIN, x = Fasin(x) );
    MONADIC(OP_ACOS, x = Facos(x) );
    MONADIC(OP_ATAN, x = Fatan(x) );
    MONADIC(OP_EXP, x = Fexp(x) );
    MONADIC(OP_EXPM1, x = Fexpm1(x) );
    MONADIC(OP_LOG, x = Flog(x) );
    MONADIC(OP_LOG1P, x = Flog1p(x) );
    MONADIC(OP_TANH, x = Ftanh(x) );
    MONADIC(OP_COSH, x = Fcosh(x) );
    MONADIC(OP_SINH, x = Fsinh(x) );
    MONADIC(OP_ATANH, x = Fatanh(x) );
    MONADIC(OP_QTANH, x = FQtanh(x) );
    MONADIC(OP_QDTANH, x = FQDtanh(x) );
    MONADIC(OP_QSTDSIGMOID, x = FQstdsigmoid(x) );
    MONADIC(OP_QDSTDSIGMOID, x = FQDstdsigmoid(x) );
    MONADIC(OP_QEXPMX, x = FQexpmx(x) );
    MONADIC(OP_QDEXPMX, x = FQDexpmx(x) );
    MONADIC(OP_QEXPMX2, x = FQexpmx2(x) );
    MONADIC(OP_QDEXPMX2, x = FQDexpmx2(x) );
    
#undef MONADIC
    
    /* Special */
  case OP_RAND:
    *sp++ = x;
    x = Frand();
    break;
  case OP_GAUSS:
    *sp++ = x;
    x = Fgauss();
  case OP_SPLINE_n:
    arg = pc[-1].code.arg;
    x = Fspline(x,arg/3,pc);
    pc += arg;
    break;
  case OP_DSPLINE_n:
    arg = pc[-1].code.arg;
    x = Fdspline(x,arg/3,pc);
    pc += arg;
    break;
    
    
    /* Diadic */
#define DIADIC(opi,opr,ops,prog) \
  case opi: y = (pc++)->constant; prog; break; \
  case opr: y= sp[pc[-1].code.arg]; prog; break; \
  case ops: y = x ; x = *--sp; prog; break
    
    DIADIC(OP_ADD_i, OP_ADD_r, OP_ADD, x+=y );
    DIADIC(OP_MUL_i, OP_MUL_r, OP_MUL, x*=y );
    DIADIC(OP_SUB_i, OP_SUB_r, OP_SUB, x-=y );
    DIADIC(OP_DIV_i, OP_DIV_r, OP_DIV, x/=y );
    DIADIC(OP_MAX_i, OP_MAX_r, OP_MAX, if (y>x) x=y );
    DIADIC(OP_MIN_i, OP_MIN_r, OP_MIN, if (y<x) x=y );
    DIADIC(OP_DIVI_i, OP_DIVI_r, OP_DIVI, x = (flt)((int)x/(int)y) );
    DIADIC(OP_MODI_i, OP_MODI_r, OP_MODI, x = (flt)((int)x%(int)y) );
    DIADIC(OP_POWER_i, OP_POWER_r, OP_POWER, x = Fexp(y*Flog(x)) );
    
#undef DIADIC
    
      
    /* Branches */
    
  case OP_BR_l:
    pc += pc[-1].code.arg;
    break;
    
#define BRANCH(op,cond) \
  case op: if (cond) pc += pc[-1].code.arg; \
    x= *--sp; break;
    
    BRANCH(OP_BREQ_l,x==0);
    BRANCH(OP_BRNEQ_l,x!=0);
    BRANCH(OP_BRGT_l,x>0);
    BRANCH(OP_BRLT_l,x<0);
    BRANCH(OP_BRGEQ_l,x>=0);
    BRANCH(OP_BRLEQ_l,x<=0);
    
#undef BRANCH

    /* For loops */

  case OP_BEGFOR_l:
    y = sp[-2];
    if ( (y>=0 && x<=sp[-1]) || (y<0 && x>=sp[-1]) ) {
      pc += pc[-1].code.arg;
    } else {
      sp -= 2;
      x = *--sp;
    }
    break;
    
  case OP_ENDFOR_l:
    y = sp[-2];
    x += y;
    if ( (y>=0 && x<=sp[-1]) || (y<0 && x>=sp[-1]) ) {
      pc += pc[-1].code.arg;
    } else {
      sp -= 2;
      x = *--sp;
    }
    break;
  }
  
#ifdef DZ_TRACE
  if (dz_trace)
    print_stack(pc-dz->program,sp,x);
#endif
  
  goto loop;
}

