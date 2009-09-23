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
 * $Id: dz.c,v 1.7 2003/09/26 03:54:46 profshadoko Exp $
 **********************************************************************/

/***********************************************************************
   DZ functions use a virtual stack machine with static code analysis.
********************************************************************** */

#include "header.h"


/* --------------- THE DZ MACHINE ---------------- */

/* Don't malloc that! */
int dz_trace=0;
real dz_stack[DZ_STACK_SIZE];
typedef union dz_inst inst;

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

static int dz_offname = 3;
static char *dz_opnames[] = {
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
trace_inst(int ad, inst *pc)
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
print_stack(int pc, real *sp, real x)
{
  printf("%04d: S=[%7.3f",pc,x);
  while (sp>dz_stack)
    printf(" %7.3f",*--sp);
  printf("]\n");
}

static void
print_reg(int pc, real x)
{
  printf("%04d: X=%7.3f\n",pc,x);
}


real 
dz_execute(real top, struct dz_cell *dz)
{

  register real x;
  register inst *pc;
  register real *sp;
  register real y;
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
    MONADIC(OP_ADD1, x += Done );
    MONADIC(OP_SUB1, x -= Done );
    MONADIC(OP_MUL2, x *= Dtwo );
    MONADIC(OP_DIV2, x /= Dtwo );
    MONADIC(OP_SGN, x = Dsgn(x) );
    MONADIC(OP_ABS, x = Dabs(x) );
    MONADIC(OP_INT, x = Dfloor(x) );
    MONADIC(OP_SQRT, x = Dsqrt(x) );
    MONADIC(OP_PIECE, x = Dpiece(x) );
    MONADIC(OP_RECT, x = Drect(x) );
    MONADIC(OP_SIN, x = Dsin(x) );
    MONADIC(OP_COS, x = Dcos(x) );
    MONADIC(OP_TAN, x = Dtan(x) );
    MONADIC(OP_ASIN, x = Dasin(x) );
    MONADIC(OP_ACOS, x = Dacos(x) );
    MONADIC(OP_ATAN, x = Datan(x) );
    MONADIC(OP_EXP, x = Dexp(x) );
    MONADIC(OP_EXPM1, x = Dexpm1(x) );
    MONADIC(OP_LOG, x = Dlog(x) );
    MONADIC(OP_LOG1P, x = Dlog1p(x) );
    MONADIC(OP_TANH, x = Dtanh(x) );
    MONADIC(OP_COSH, x = Dcosh(x) );
    MONADIC(OP_SINH, x = Dsinh(x) );
    MONADIC(OP_ATANH, x = Datanh(x) );
    MONADIC(OP_QTANH, x = DQtanh(x) );
    MONADIC(OP_QDTANH, x = DQDtanh(x) );
    MONADIC(OP_QSTDSIGMOID, x = DQstdsigmoid(x) );
    MONADIC(OP_QDSTDSIGMOID, x = DQDstdsigmoid(x) );
    MONADIC(OP_QEXPMX, x = DQexpmx(x) );
    MONADIC(OP_QDEXPMX, x = DQDexpmx(x) );
    MONADIC(OP_QEXPMX2, x = DQexpmx2(x) );
    MONADIC(OP_QDEXPMX2, x = DQDexpmx2(x) );
#undef MONADIC
    
    /* Special */
  case OP_RAND:
    *sp++ = x;
    x = Drand();
    break;
  case OP_GAUSS:
    *sp++ = x;
    x = Dgauss();
  case OP_SPLINE_n:
    arg = pc[-1].code.arg;
    x = Dspline(x,arg/3,(real*)pc);
    pc += arg;
    break;
  case OP_DSPLINE_n:
    arg = pc[-1].code.arg;
    x = Ddspline(x,arg/3,(real*)pc);
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
    DIADIC(OP_DIVI_i, OP_DIVI_r, OP_DIVI, x = (real)((int)x/(int)y) );
    DIADIC(OP_MODI_i, OP_MODI_r, OP_MODI, x = (real)((int)x%(int)y) );
    DIADIC(OP_POWER_i, OP_POWER_r, OP_POWER, x = Dexp(y*Dlog(x)) );
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




/* --------- THE DZ FAST ACCESS FUNCTIONS ----- */


static real 
dz_exec_2(real x0, real x1, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  return dz_execute(x1,dz);
}

static real 
dz_exec_3(real x0, real x1, real x2, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  return dz_execute(x2,dz);
}

static real 
dz_exec_4(real x0, real x1, real x2, real x3, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  return dz_execute(x3,dz);
}

static real 
dz_exec_5(real x0, real x1, real x2, real x3, 
          real x4, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  return dz_execute(x4,dz);
}

static real 
dz_exec_6(real x0, real x1, real x2, real x3, 
          real x4, real x5, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  dz_stack[4]=x4;
  return dz_execute(x5,dz);
}


static real 
dz_exec_7(real x0, real x1, real x2, real x3, 
          real x4, real x5, real x6, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  dz_stack[4]=x4;
  dz_stack[5]=x5;
  return dz_execute(x6,dz);
}

static real 
dz_exec_8(real x0, real x1, real x2, real x3, 
          real x4, real x5, real x6, real x7, 
          struct dz_cell *dz)
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  dz_stack[4]=x4;
  dz_stack[5]=x5;
  dz_stack[6]=x6;
  return dz_execute(x7,dz);
}

static real
dz_error()
{
  error("C error","C call to a dz with more than 8 arguments",NIL);
}

typedef real (*dz_call_t)(real, ...);

static dz_call_t dz_call[] = {
  (dz_call_t) dz_error,
  (dz_call_t) dz_execute,
  (dz_call_t) dz_exec_2,
  (dz_call_t) dz_exec_3,
  (dz_call_t) dz_exec_4,
  (dz_call_t) dz_exec_5,
  (dz_call_t) dz_exec_6,
  (dz_call_t) dz_exec_7,
  (dz_call_t) dz_exec_8,
};

/* -----------------   DZ CREATION  -------------- */

struct alloc_root dz_alloc = {
  NULL,
  NULL,
  sizeof(struct dz_cell),
  10
};

static at *
dz_new(int narg, int reqs, int plen)
{
  struct dz_cell *dz;
  /* check stack */
  if (reqs > DZ_STACK_SIZE)
    error(NIL,"DZ stack is too small",NIL);
  
  dz = allocate( &dz_alloc );
  dz->num_arg = narg;
  dz->program = NULL;
  dz->program_size = plen;
  dz->required_stack = reqs;
  dz->program = calloc(sizeof(inst),plen+1);
  ifn (dz->program)
    error(NIL,"not enough memory",NIL);
  /* select C access function */
  if (narg>8) 
    dz->call = dz_call[0];
  else
    dz->call = dz_call[narg];
  return new_extern(&dz_class,dz);
}


/* ------------ STATIC CODE ANALYSIS ------------- */

/* This code proves that the virtual machine code will never access illegal
 * stack position.  It works by pre-computing the expected stack structure for
 * each byte code location.  This static code analyser was written in 1991 by
 * Leon Bottou and Patrice Simard, well before the popular java byte code
 * analyzer.
 */

static struct progmap {
  int pc;
  int sp;
  int sg;
  char *label;
} *progmap = NULL;

static at *this_prog;
static int this_num_arg;
static int this_imax;
static int this_reqstack;

static int
find_opname(char *s)
{
  int op;
  for(op=1;dz_opnames[op];op++)
    if (!strcmp(s, dz_opnames[op]+dz_offname))
      return op;
  return 0;
}

static int
find_op(at *ins)
{
  int op;
  if (! (CONSP(ins) && EXTERNP(ins->Car,&string_class)))
    error(NIL,"illegal instruction",ins);
  op = find_opname(SADD(ins->Car->Object));
  ifn (op)
    error(NIL,"unknown opcode",ins);
  return op;
}

static int
find_label(char *s)
{
  int i;
  for (i=0; i<=this_imax; i++)
    if (progmap[i].label)
      if (!strcmp(s,progmap[i].label))
	return i;
  error(NIL,"label not found",new_string(s));
}

static int 
find_r_arg(at *ins, int inum)
{
  at *q;
  int arg;
  char *s;

  ifn (CONSP(ins) && CONSP(ins->Cdr) 
       && ins->Cdr->Car && !ins->Cdr->Cdr)
    error(NIL,"one operand required",ins);
  
  q = ins->Cdr->Car;
  
  if (EXTERNP(q, &string_class))
    {
      s = SADD(q->Object);
      if ((!strncmp(s,"arg",3)) 
	  && (arg=atoi(s+3))>0 && arg<=this_num_arg)
	return progmap[inum].sp - arg;
      arg = find_label(s);
      return progmap[inum].sp - progmap[arg].sp;
    }
  else if (NUMBERP(q))
    {
      arg = q->Number;
      if ((real)arg!=q->Number)
	error(NIL,"integer operand expected",ins);
      if (progmap[inum].sg==0)
	if (arg<0 || arg>=progmap[inum].sp)
	  error(NIL,"illegal stack relative operand",ins);
      return arg;
    }
  error(NIL,"illegal operand",ins);
}

static int
find_l_arg(at *ins, int inum)
{
  at *q;
  int arg;
  char *s;

  ifn (CONSP(ins) && CONSP(ins->Cdr) 
       && ins->Cdr->Car && !ins->Cdr->Cdr)
    error(NIL,"one operand required",ins);
  
  q = ins->Cdr->Car;
  
  if (EXTERNP(q, &string_class))
    {
      s = SADD(q->Object);
      arg = find_label(s);
      return progmap[arg].pc - progmap[inum].pc - 1;
    }
  else if (NUMBERP(q))
    {
      arg = q->Number;
      if ((real)arg!=q->Number)
	error(NIL,"integer operand expected",ins);
      return arg;
    }
  error(NIL,"illegal operand",ins);
}

static int
find_n_arg(at *ins, inst **pc_p)
{
  at *q;
  ifn (CONSP(ins) && CONSP(ins->Cdr) && !ins->Cdr->Cdr &&
       (q = ins->Cdr->Car) && EXTERNP(q, &index_class) )
    error(NIL,"one index expected as operand",ins);
  shape_t shape, *shp = shape_set(&shape, 1, 0, 0, 0, 0);
  index_t *ind = easy_index_check(q, shp);
  if (pc_p) {
    (*pc_p)->code.arg = shp->dim[0];
    for (int i=0; i<shp->dim[0]; i++) {
      (*pc_p)++;
      (*pc_p)->constant = easy_index_get(ind,&i);
    }
  }
  return shp->dim[0];
}

static void 
find_i_arg(at *ins, inst **pc_p)
{
  ifn (CONSP(ins) && 
       CONSP(ins->Cdr) && 
       NUMBERP(ins->Cdr->Car) )
    error(NIL,"operand expected",ins);
  (*pc_p)++;
  (*pc_p)->constant = ins->Cdr->Car->Number;
}

static void
gen_code(at *ins, inst **pc_p, int *inum_p)
{
  int op,arg;
  if (CONSP(ins)) {
    op = find_op(ins);
    switch (dz_opnames[op][0])
      {
      case 'o':
	(*pc_p)->code.op = op;
	(*pc_p)->code.arg = 0;
	(*pc_p)++;
	break;
	
      case 'r':
	arg = find_r_arg(ins,*inum_p);
	(*pc_p)->code.op = op;
	(*pc_p)->code.arg = -arg;
	(*pc_p)++;
	if (arg==0)  {
	  if (!strcmp(dz_opnames[op]+dz_offname,"PUSH@")) 
	    (*pc_p)[-1].code.op = find_opname("DUP");
	  else if (!strcmp(dz_opnames[op]+dz_offname,"POP@"))
	    (*pc_p)[-1].code.op = find_opname("NOP");
	  else
	    error("DZ Analyzer","Stack relative operand is 0",ins);
	}
	break;
	
      case 'i':
	(*pc_p)->code.op = op;
	(*pc_p)->code.arg = 0;
	find_i_arg(ins,pc_p);
	(*pc_p)++;
	break;
	
      case 'n':
	(*pc_p)->code.op = op;
	(*pc_p)->code.arg = 0;
	find_n_arg(ins,pc_p);
	(*pc_p)++;
	break;
	
      case 'l':
	arg = find_l_arg(ins,*inum_p);
	(*pc_p)->code.op = op;
	(*pc_p)->code.arg = arg;
	(*pc_p)++;
	break;
      }
  }
  (*inum_p)++;
}

static void
make_progmap_1(at *ins, int *inum_p)
{
  int op;

  (*inum_p)++;
  progmap[*inum_p].pc = progmap[*inum_p-1].pc;
  progmap[*inum_p].sp = progmap[*inum_p-1].sp;
  progmap[*inum_p].sg = progmap[*inum_p-1].sg;
  progmap[*inum_p].label = NULL;
  
  if (EXTERNP(ins, &string_class)) {
    progmap[*inum_p].label = SADD(ins->Object);
    return;
  }
  
  op = find_op(ins);
  
  switch( dz_opnames[op][0] ) {
  case 'o':
  case 'r':
  case 'l':
    progmap[*inum_p].pc += 1;
    break;
  case 'i':
    progmap[*inum_p].pc += 2;
    break;
  case 'n':
    progmap[*inum_p].pc += 1 + find_n_arg(ins,NULL);
    break;
  default:
    error("DZ Analyzer","Unknown opcode class",NIL);
  }

  switch( dz_opnames[op][1] ) {
  case '<':
    progmap[*inum_p].sp -= 1;
    break;
  case '>':
    progmap[*inum_p].sp += 1;
    break;
  case 'o':
    break;
  case 'a':
    progmap[*inum_p].sp -= find_r_arg(ins,*inum_p-1);
    break;
  case '*':
    progmap[*inum_p].sp = 0;
    progmap[*inum_p].sg++;
  case 'B':
    progmap[*inum_p].sp -= 3;
    break;
  default:
    error("DZ Analyzer","Unknown opcode class",NIL);
  }
}

static void
make_progmap_2(at *ins, int *inum_p)
{
  int op, d, i;
  int brsp;
  int sgsrc, sgdst;

  if (CONSP(ins)) {
    op = find_op(ins);
    if (dz_opnames[op][0]=='l') {

      /* this is a branch */
      d = find_l_arg(ins,*inum_p);
      d += progmap[*inum_p].pc + 1;
      for (i=0; i<=this_imax;i++)
	if (d == progmap[i].pc)
	  break;
      if (d != progmap[i].pc)
	error("DZ Analyzer","Jump to a stupid location",ins);

      switch (dz_opnames[op][1]) {
      case '<':
	brsp = progmap[*inum_p].sp - 1;
	break;
      case 'o':
      case '*':
      case 'B':
	brsp = progmap[*inum_p].sp;
	break;
      case '>':
	brsp = progmap[*inum_p].sp + 1;
	break;
      default:
	error("DZ Analyzer","Unknown stack behavior for a branch",ins);
      }
      
      if ( progmap[*inum_p].sg == progmap[i].sg ) {
	if ( brsp != progmap[i].sp )
	  error("DZ Analyzer","Stack mismatch in a jump",ins); 
      } else {
	d = brsp - progmap[i].sp;
	sgsrc = progmap[i].sg;
	sgdst = progmap[*inum_p].sg;
	for (i=0;i<=this_imax;i++) {
	  if (progmap[i].sg == sgsrc) {
	    progmap[i].sg = sgdst;
	    progmap[i].sp += d;
	  }
	}
      }
    }
  }
  (*inum_p)++;
}

static void 
make_progmap(void)
{
  at *q;
  int inum;
  progmap[0].pc = 0;
  progmap[0].sp = this_num_arg;
  progmap[0].sg = 0;
  progmap[0].label = NULL;

  /* pass 1: fill progmap stupid */
  q = this_prog;
  this_imax = 0;
  while(q) {
    ifn (CONSP(q))
      error(NIL,"not a valid list",this_prog);
    make_progmap_1(q->Car,&this_imax);
    q = q->Cdr;
  }
  
  /* pass 2: solve branches */
  q = this_prog;
  inum = 0;
  while(q) {
    make_progmap_2(q->Car,&inum);
    q = q->Cdr;
  }

  /* pass 3: global complaints */
  q = this_prog;
  this_reqstack = this_num_arg;
  for (inum=1; inum<=this_imax; inum++) {
    if (progmap[inum].sg != 0)
      error("DZ Analyzer","Statement not reached",q->Car);
    if (progmap[inum].sp < 0)
      error("DZ Analyzer","Stack is popped below the ground level",q->Car);
    if (progmap[inum].sp > this_reqstack)
      this_reqstack = progmap[inum].sp + 1;
    q = q->Cdr;
  }
  if (progmap[this_imax].sp != 1)
    error("DZ Analyzer","Stack must be properly rewound",
	  NEW_NUMBER(progmap[this_imax].sp));
}

DX(xdz_load)
{
  at *p,*ans;
  struct dz_cell *dz;
  inst *pc;
  int nargs;
  int inum;
  struct context mycontext;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  nargs = AINTEGER(1);
  p = ACONS(2);
  /* count the sizes of the dz, check stack and syntax */
  this_prog = p;
  this_num_arg = nargs;
  progmap = malloc(sizeof(struct progmap) * (2+length(p)));
  if (!progmap)
    error(NIL,"Not enough memory",NIL);
  /* make sure progmap is released */
  context_push(&mycontext);	/* can be interrupted */
  if (sigsetjmp(context->error_jump, 1)) 
    { 
      context_pop();
      free(progmap);
      progmap = NULL;
      siglongjmp(context->error_jump, -1);
    }
  /* run static analysis */
  make_progmap();
  /* allocate program */
  ans = dz_new(this_num_arg, this_reqstack, progmap[this_imax].pc);
  dz = ans->Object;
  pc = dz->program;
  if (!pc)
    error(NIL,"not enough memory",NIL);
  /* assemble program */
  inum = 0;
  while (p) {
    gen_code(p->Car, &pc, &inum);
    p = p->Cdr;
  }
  pc->code.op = 0;
  pc->code.arg = 0;
  dz->program_size = progmap[this_imax].pc;
  /* finish */
  context_pop();
  free(progmap);
  progmap = NULL;
  return ans;
}


void 
dz_define(char *name, char *opcode, real (*cfun)(real))
{
  struct dz_cell *dz;
  int op;
  at *func = dz_new(1,1,1);
  dz = func->Object;
  /* if a specific C function is provided, use it */
  if (cfun)
    dz->call = (dz_call_t)cfun;
  op = find_opname(opcode);
  ifn (op && dz_opnames[op][0]=='o' && dz_opnames[op][0]=='o') {
    sprintf(string_buffer, "illegal opcode %s in %s", opcode, name);
    error(NIL,string_buffer,NIL);
  }
  dz->program[0].code.op = op;
  dz->program[0].code.arg = 0;
  dz->program[1].code.op = 0;
  dz->program[1].code.arg = 0;

  at *symb = new_symbol(name);
  symbol_t *sym = symb->Object;
  if (sym->mode == SYMBOL_LOCKED)
    RAISEFU(string_buffer, new_string(name));
  var_set(symb, func);
  sym->mode = SYMBOL_LOCKED;
  UNLOCK(func);
  UNLOCK(symb);
}


/* ----------------- DZ CLASS -------------- */

static void 
dz_dispose(at *p)
{
  struct dz_cell *dz;
  if ((dz = p->Object))
    if (dz->program)
      free(dz->program);
  deallocate(&dz_alloc, (void*)dz);
}

static at* 
dz_listeval(at *p, at *q)
{
  struct dz_cell *dz;
  at *qq;
  int i;
  real x;
  dz = p->Object;
  qq = q = eval_a_list(q->Cdr);
  for (i=0;i<dz->num_arg;i++) {
    if (! (CONSP(qq)))
      error(NIL, "Not enough arguments", q);
    if (! (qq && NUMBERP(qq->Car)))
      error(NIL,"Not a number", q->Car);
    dz_stack[i] = (real)(qq->Car->Number);
    qq = qq->Cdr;
  }
  if (qq)
    error(NIL,"Too many arguments",q);
  x = (double)dz_execute(dz_stack[i-1],dz);
  UNLOCK(q);
  return NEW_NUMBER(x);
}

static void
dz_serialize(at **pp, int code)
{
  int ncst;
  struct dz_cell *dz;
  union dz_inst *pc;
  /* Creation */
  if (code == SRZ_READ)
    {
      int narg, reqs, plen;
      serialize_int(&narg, code);
      serialize_int(&reqs, code);
      serialize_int(&plen, code);
      *pp = dz_new(narg, reqs, plen);
      dz = (*pp)->Object;
    }
  else
    {
      dz = (*pp)->Object;
      serialize_int(&dz->num_arg, code);
      serialize_int(&dz->required_stack, code);
      serialize_int(&dz->program_size, code);
    }
  /* Serialization*/
  ncst = 0;
  for (pc = dz->program; pc<dz->program + dz->program_size; pc++)
    {
      if (ncst > 0) 
        {
          ncst++;
          serialize_real(&pc->constant, code);
        }
      else
        {
          serialize_short(&pc->code.op, code);
          switch(dz_opnames[pc->code.op][0])
            {
            case 'n':
              serialize_short(&pc->code.arg, code);
              ncst = pc->code.arg;
              break;
            case 'i':
              ncst = 1;
              break;
            case 'r':
            case 'l':
              serialize_short(&pc->code.arg, code);
              break;
            default:
              break;
            }
        }
    }
}

class dz_class =
{       
  dz_dispose,
  generic_action,
  generic_name,
  generic_eval,
  dz_listeval,
  dz_serialize,
  generic_compare,
  generic_hash,
  generic_getslot,
  generic_setslot,
};


/* ------- FUNCTION TO GET INFORMATION ON DZ -------- */

static int
dzp(at *p)
{
  if (EXTERNP(p, &dz_class))
    return true;
  return false;
}

DX(xdzp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (dzp(APOINTER(1)))
    return t();
  else
    return NIL;
}


static at *
gen_inst(union dz_inst **pc_p)
{
  int op,arg;
  char *s;
  at *ans;
  
  op = (**pc_p).code.op;
  arg = (**pc_p).code.arg;
  s = dz_opnames[op]+dz_offname;
  (*pc_p)++;
  ans = new_string(s);

  switch (dz_opnames[op][0]) {
    
  case 'n':
    {
      at *p = MAKE_ARRAY(ST_D, SHAPE1D(arg), NIL);
      index_t *ind = p->Object;
      int i;
      for(i=0; i<arg; i++) {
	easy_index_set(ind,&i,(real)((*pc_p)->constant));
	(*pc_p)++;
      }
      ans = cons(ans, cons(p,NIL));
    }
    break;
  case 'i':
    ans = cons(ans, cons(NEW_NUMBER( (**pc_p).constant ), NIL));
    (*pc_p)++;
    break;
  case 'r':
    ans = cons(ans, cons( NEW_NUMBER(-arg), NIL));
    break;
  case 'l':
    ans = cons(ans, cons( NEW_NUMBER(arg), NIL ));
    break;
  default:
    ans = cons(ans,NIL);
    break;
  }  
  return ans;
}

DX(xdz_def)
{
    at *p;
    struct dz_cell *dz;
    at *ans;
    at **where;
    inst *pc;

    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    
    if(!dzp(p))
      error(NIL,"not a DZ",p);

    dz = p->Object;
    ans = cons (NEW_NUMBER(dz->num_arg), NIL);
    where = &(ans->Cdr);
    
    pc = dz->program;
    if (pc) {
      while (pc->code.op) {
	*where = cons(gen_inst(&pc),NIL);
	where = &((*where)->Cdr);
	if (pc > dz->program + dz->program_size)
	  error("dz.c/xdzdef","going past program",p);
      }
    }
    return ans;
}



DX(xdz_trace)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (APOINTER(1))
    dz_trace = 1;
  else
    dz_trace = 0;
  return NIL;
}

/* ------- FUNCTION TO COMPILE SPLINE AND DSPLINE -------- */


static at *
lisp_spline(int arg_number, at *arg_array[], char *op)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);

  shape_t shape, *shp = shape_set(&shape, 2, 0, 2, 0, 0);
  index_t *ind = easy_index_check(APOINTER(1), shp);
  intg n = shp->dim[0];
  real *wspace = (real*)alloca( sizeof(real) * 3 * n );
  ifn (wspace)
    error(NIL,"Not enough memory",NIL);
  
  real *x = wspace;
  real *y = wspace+n;;
  intg dim[2];
  for(int i=0; i<n; i++) {
    dim[0]=i;
    dim[1]=0;
    *x++ = easy_index_get(ind, dim);
    dim[1]=1;
    *y++ = easy_index_get(ind, dim);
  }
  Dsplinit(n,wspace);
  
  at *ans = dz_new(1, 1, 3*n+1);
  //struct dz_cell *dz;
  struct dz_cell *dz = ans->Object;
  inst *pc = dz->program;
  
  int i = find_opname(op);
  if (!i)
    error("dz_lisp.c/lisp_spline","Unknown opcode",new_safe_string(op));
  pc->code.op = i;
  pc->code.arg = 3*n;
  x = wspace;
  pc++;
  for (i=0;i<3*n;i++)
    (pc++)->constant = *x++;
  pc->code.op = 0;
  pc->code.arg = 0;
  return ans;
}

DX(xdz_spline)
{
  return lisp_spline(arg_number,arg_array,"SPLINE");
}

DX(xdz_dspline)
{
  return lisp_spline(arg_number,arg_array,"DSPLINE");
}



/* ------- INITIALIZATION FUNCTIONS -------- */


void 
init_dz(void)
{
    class_define("DZ",&dz_class);

    dx_define("dz-load",  xdz_load);
    dx_define("dzp",      xdzp);
    dx_define("dz-def",   xdz_def);
    dx_define("dz-trace", xdz_trace);
    dx_define("dz-spline", xdz_spline);
    dx_define("dz-dspline", xdz_dspline);

    // they are now all defined in "math.h"
/*     dz_define("sgn", "SGN", NULL); */
/*     dz_define("abs", "ABS", NULL); */
/*     dz_define("int", "INT", NULL); */
/*     dz_define("sqrt", "SQRT", NULL); */
/*     dz_define("0-x-1", "PIECE", NULL); */
/*     dz_define("0-1-0", "RECT", NULL); */
/*     dz_define("sin", "SIN", NULL); */
/*     dz_define("cos", "COS", NULL); */
/*     dz_define("tan", "TAN", NULL); */
/*     dz_define("asin", "ASIN", NULL); */
/*     dz_define("acos", "ACOS", NULL); */
/*     dz_define("atan", "ATAN", NULL); */
/*     dz_define("exp", "EXP", NULL); */
/*     dz_define("exp-1", "EXPM1", NULL); */
/*     dz_define("log", "LOG", NULL); */
/*     dz_define("log1+", "LOG1P", NULL); */
/*     dz_define("tanh", "TANH", NULL); */
/*     dz_define("atanh", "ATANH", NULL); */
/*     dz_define("cosh", "COSH", NULL); */
/*     dz_define("sinh", "SINH", NULL); */

    dz_define("qtanh", "QTANH", DQtanh);
    dz_define("qdtanh", "QDTANH", DQDtanh);
    dz_define("qstdsigmoid", "QSTDSIGMOID", DQstdsigmoid);
    dz_define("qdstdsigmoid", "QDSTDSIGMOID", DQDstdsigmoid);

#ifdef SLOW_EXPONENTIAL
#define P(x) x
#else
#define P(x) NULL
#endif

    dz_define("qexpmx", "QEXPMX", P(DQexpmx));
    dz_define("qdexpmx", "QDEXPMX", P(DQDexpmx));
    dz_define("qexpmx2", "QEXPMX2", P(DQexpmx2));
    dz_define("qdexpmx2", "QDEXPMX2", P(DQDexpmx2));

#undef P
}
