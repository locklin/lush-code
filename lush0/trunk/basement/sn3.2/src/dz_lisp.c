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
 *	DZ_LISP.C. 
 *	Lisp functions for the DZ objects
 *
 ******************************************************************************/

#include "header.h"

typedef union dz_inst inst;
extern int dz_offname;

/* --------------- THE DZ POOL ---------------- */

struct alloc_root dz_alloc =
{       NULL,
        NULL,
        sizeof(struct dz_cell),
        10
};

/* ----------- DZ CLASS FUNCTIONS -------------- */


void 
  dz_dispose(p)
at *p;
{
  struct dz_cell *dz;
    
  if (dz = p->Object)
    if (dz->program)
      free(dz->program);
  deallocate(&dz_alloc, dz);
}

/* This is where the calculator get called to evaluate the DZ function */

static at* 
  dz_listeval(p,q)
register at *p, *q;
{
  struct dz_cell *dz;
  at *qq;
  int i;
  real x;
  
  dz = p->Object;
  qq = q = eval_a_list(q->Cdr);
  
  for (i=0;i<dz->num_arg;i++) {
    ifn(CONSP(qq))
      error(NIL, "Not enough arguments", q);
    ifn (qq && qq->Car && qq->Car->ctype==XT_NUMBER)
      error(NIL,"Not a number", q->Car);
    dz_stack[i] = (flt)(qq->Car->Number);
    qq = qq->Cdr;
  }
  if (qq)
    error(NIL,"Too many arguments",q);
  
  x = (double)dz_execute(dz_stack[i-1],dz);
  UNLOCK(q);
  return NEW_NUMBER(x);
}


/* --------------- THE DZ CLASS -------------- */


class dz_class =
{       
  dz_dispose,
  generic_action,
  generic_name,
  generic_eval,
  dz_listeval,
  XT_DZ,
};




/* --------- THE DZ FAST ACCESS FUNCTIONS ----- */


static flt 
  dz_exec_2(x0,x1,dz)
struct dz_cell *dz;
flt x0,x1;
{
  dz_stack[0]=x0;
  return dz_execute(x1,dz);
}

static flt 
  dz_exec_3(x0,x1,x2,dz)
struct dz_cell *dz;
flt x0,x1,x2;
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  return dz_execute(x2,dz);
}

static flt 
  dz_exec_4(x0,x1,x2,x3,dz)
struct dz_cell *dz;
flt x0,x1,x2,x3;
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  return dz_execute(x3,dz);
}

static flt 
  dz_exec_5(x0,x1,x2,x3,x4,dz)
struct dz_cell *dz;
flt x0,x1,x2;
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  return dz_execute(x4,dz);
}

static flt 
  dz_exec_6(x0,x1,x2,x3,x4,x5,dz)
struct dz_cell *dz;
flt x0,x1,x2,x3,x4,x5;
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  dz_stack[4]=x4;
  return dz_execute(x5,dz);
}


static flt 
  dz_exec_7(x0,x1,x2,x3,x4,x5,x6,dz)
struct dz_cell *dz;
flt x0,x1,x2,x3,x4,x5,x6;
{
  dz_stack[0]=x0;
  dz_stack[1]=x1;
  dz_stack[2]=x2;
  dz_stack[3]=x3;
  dz_stack[4]=x4;
  dz_stack[5]=x5;
  return dz_execute(x6,dz);
}

static flt 
  dz_exec_8(x0,x1,x2,x3,x4,x5,x6,x7,dz)
struct dz_cell *dz;
flt x0,x1,x2,x3,x4,x5,x6,x7;
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

static flt
  dz_error()
{
  error("C error","C call to a dz with more than 8 arguments",NIL);
}

static flt (*dz_call[])() = {
  dz_error,
  dz_execute,
  dz_exec_2,
  dz_exec_3,
  dz_exec_4,
  dz_exec_5,
  dz_exec_6,
  dz_exec_7,
  dz_exec_8,
};



/* --------- THE DZ CREATION FUNCTIONS -------- */

at *dz_new(narg, reqs, plen)
int narg,reqs,plen;
{
  struct dz_cell *dz;
  
  /* check stack and extend it */
  if (reqs>DZ_STACK_SIZE) {
    error(NIL,"DZ stack is too small",NIL);
  }
  
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
    dz->call = dz_error;
  else
    dz->call = dz_call[narg];
  
  return new_extern(&dz_class,dz,XT_DZ);
}


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



static void
  make_progmap_1(ins,inum_p)
at *ins;
int *inum_p;
{
  int op;
  static int find_n_arg();
  static int find_r_arg();
  static int find_op();
  
  (*inum_p)++;
  progmap[*inum_p].pc = progmap[*inum_p-1].pc;
  progmap[*inum_p].sp = progmap[*inum_p-1].sp;
  progmap[*inum_p].sg = progmap[*inum_p-1].sg;
  progmap[*inum_p].label = NULL;
  
  if (ins && ins->ctype==XT_STRING) {
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
    error("dz_lisp.c/acc_inst_sizes","unknown dz_opnames[op][0]",NIL);
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
    error("dz_lisp.c/acc_inst_sizes","unknown dz_opnames[op][0]",NIL);
  }
}


static void
  make_progmap_2(ins,inum_p)
at *ins;
int *inum_p;
{
  static int find_l_arg();
  static int find_op();
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
	error(NIL,"Jump to a stupid location",ins);

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
	error("dz_lisp.c/make_progmap_2",
	      "unknown stack behavior for a branch",ins);
      }
      
      if ( progmap[*inum_p].sg == progmap[i].sg ) {
	if ( brsp != progmap[i].sp )
	  error(NIL,"stack mismatch in a jump",ins); 
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
  make_progmap()
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
      error(NIL,"statement not reached",q->Car);
    if (progmap[inum].sp < 0)
      error(NIL,"stack is popped below the ground level",q->Car);
    if (progmap[inum].sp > this_reqstack)
      this_reqstack = progmap[inum].sp + 1;
    q = q->Cdr;
  }
  if (progmap[this_imax].sp != 1)
    error(NIL,"Stack must be properly rewinded",
	  NEW_NUMBER(progmap[this_imax].sp));
}



static int
  find_opname(s)
char *s;
{
  int op;
  for(op=1;dz_opnames[op];op++)
    if (!strcmp(s, dz_opnames[op]+dz_offname))
      return op;
  return 0;
}

static int
  find_op(ins)
at *ins;
{
  int op;
  char *s;
  ifn (CONSP(ins) && ins->Car && ins->Car->ctype==XT_STRING)
    error(NIL,"illegal instruction",ins);
  op = find_opname(SADD(ins->Car->Object));
  ifn (op)
    error(NIL,"unknown opcode",ins);
  return op;
}

static int
  find_label(s)
char *s;
{
  int i;
  for (i=0; i<=this_imax; i++)
    if (progmap[i].label)
      if (!strcmp(s,progmap[i].label))
	return i;
  error(NIL,"label not found",new_string(s));
}



static int 
  find_r_arg(ins,inum)
at *ins;
int inum;
{
  at *q;
  int arg;
  char *s;

  ifn (CONSP(ins) && CONSP(ins->Cdr) 
       && ins->Cdr->Car && !ins->Cdr->Cdr)
    error(NIL,"one operand required",ins);
  
  q = ins->Cdr->Car;
  
  if (q->ctype==XT_STRING) 
    {
      s = SADD(q->Object);
      if ((!strncmp(s,"arg",3)) 
	  && (arg=atoi(s+3))>0 && arg<=this_num_arg)
	return progmap[inum].sp - arg;
      arg = find_label(s);
      return progmap[inum].sp - progmap[arg].sp;
    }
  else if (q->ctype==XT_NUMBER) 
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
  find_l_arg(ins,inum)
at *ins;
int inum;
{
  at *q;
  int arg;
  char *s;

  ifn (CONSP(ins) && CONSP(ins->Cdr) 
       && ins->Cdr->Car && !ins->Cdr->Cdr)
    error(NIL,"one operand required",ins);
  
  q = ins->Cdr->Car;
  
  if (q->ctype==XT_STRING) 
    {
      s = SADD(q->Object);
      arg = find_label(s);
      return progmap[arg].pc - progmap[inum].pc - 1;
    }
  else if (q->ctype==XT_NUMBER) 
    {
      arg = q->Number;
      if ((real)arg!=q->Number)
	error(NIL,"integer operand expected",ins);
      return arg;
    }
  error(NIL,"illegal operand",ins);
}

static int
  find_n_arg(ins,pc_p)
at *ins;
inst **pc_p;
{
  struct index *ind;
  int i, n;
  at *q;
  flt *f;
  
  ifn (CONSP(ins) && CONSP(ins->Cdr)
       && (q=ins->Cdr->Car) && (!ins->Cdr->Cdr)
       && q->ctype==XT_INDEX)
    error(NIL,"one index expected as operand",ins);
  
  n = 0;
  ind = easy_index_check(q,1,&n);
  
  if (pc_p) {
    (*pc_p)->code.arg = n;
    for (i=0;i<n;i++) {
      (*pc_p)++;
      (*pc_p)->constant = easy_index_get(ind,&i);
    }
  }
  return n;
}

static void 
  find_i_arg(ins,pc_p)
at *ins;
inst **pc_p;
{
  ifn (CONSP(ins) && CONSP(ins->Cdr)
       && ins->Cdr->Car && ins->Cdr->Car->ctype==XT_NUMBER )
    error(NIL,"operand expected",ins);
  (*pc_p)++;
  (*pc_p)->constant = ins->Cdr->Car->Number;
}

static void
  gen_code(ins,pc_p,inum_p)
at *ins;
inst **pc_p;
int *inum_p;
{
  char *s;
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
	    error(NIL,"stack relative operand is 0",ins);
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



DX(xdz_load)
{
  at *p,*ans;
  struct dz_cell *dz;
  inst *pc;
  int nargs;
  int inum;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  nargs = AINTEGER(1);
  p = ACONS(2);
  
  /* count the sizes of the dz, check stack and syntax */

  this_prog = p;
  this_num_arg = nargs;

  progmap = (ptr)alloca( sizeof(struct progmap) * (2+length(p)) );
  ifn (progmap)
    error(NIL,"Not enough memory",NIL);
    make_progmap();

  /* allocate program */

  ans = dz_new(this_num_arg, this_reqstack, progmap[this_imax].pc);
  dz = ans->Object;
  pc = dz->program;
  ifn (pc)
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
  progmap = NULL;
  return ans;
}


void 
  dz_define(name,opcode,cfun)
char *name;
char *opcode;
flt (*cfun)();
{
  at *symb,*func;
  struct dz_cell *dz;
  int op;
  
  func = dz_new(1,1,1);
  dz = func->Object;
  /* if a specific C function is provided, use it */
  if (cfun)
    dz->call = cfun;

  op = find_opname(opcode);
  ifn (op && dz_opnames[op][0]=='o' && dz_opnames[op][0]=='o') {
    sprintf(string_buffer, "illegal opcode %s in %s", opcode, name);
    error(NIL,string_buffer,NIL);
  }

  dz->program[0].code.op = op;
  dz->program[0].code.arg = 0;
  dz->program[1].code.op = 0;
  dz->program[1].code.arg = 0;
  
  symb = new_symbol(name);
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    sprintf(string_buffer, "symbol already exists: %s", name);
    error("dz_lisp.c/dz_define", string_buffer, NIL);
  }
  var_set(symb, func);
  ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
  UNLOCK(func);
  UNLOCK(symb);
}


/* ------- FUNCTION TO GET INFORMATION ON DZ -------- */

int
  dzp(p)
at *p;
{
  if (p && p->ctype==XT_DZ)
    return TRUE;
  return FALSE;
}

DX(xdzp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (dzp(APOINTER(1)))
    return true();
  else
    return NIL;
}



static at *
  gen_inst(pc_p)
union dz_inst **pc_p;
{
  int op,arg,i;
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
      at *p;
      struct index *ind;

      p = F_matrix(1,&arg);
      ind = p->Object;
      for(i=0;i<arg;i++) {
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
	  error("dz_lisp.c/xdzdef","going past program",p);
      }
    }
    return ans;
}


int dz_trace=0;

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
  lisp_spline(arg_number,arg_array,op)
int arg_number;
at *arg_array[];
char *op;
{
  struct index *ind;
  int dim[2];
  int i,n;
  flt *wspace;
  flt *x,*y;

  at *ans;
  struct dz_cell *dz;
  inst *pc;

  ARG_NUMBER(1);
  ARG_EVAL(1);

  dim[0] = 0;
  dim[1] = 2;
  ind = easy_index_check(APOINTER(1),2,dim);
  n = dim[0];

  wspace = (flt*)alloca( sizeof(flt) * 3 * n );
  ifn (wspace)
    error(NIL,"Not enough memory",NIL);
  
  x = wspace;
  y = wspace+dim[0];
  for(i=0;i<n;i++) {
    dim[0]=i;
    dim[1]=0;
    *x++ = easy_index_get(ind,dim);
    dim[1]=1;
    *y++ = easy_index_get(ind,dim);
  }
  Fsplinit(n,wspace);
  
  ans = dz_new(1, 1, 3*n+1);
  dz = ans->Object;
  pc = dz->program;
  
  i = find_opname(op);
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


void init_dz_lisp()
{
    class_define("DZ",&dz_class,NIL);

    dx_define("dz_load",  xdz_load);
    dx_define("dzp",      xdzp);
    dx_define("dz_def",   xdz_def);
    dx_define("dz_trace", xdz_trace);
    dx_define("dz_spline", xdz_spline);
    dx_define("dz_dspline", xdz_dspline);

    dz_define("sgn", "SGN", NULL);
    dz_define("abs", "ABS", NULL);
    dz_define("int", "INT", NULL);
    dz_define("sqrt", "SQRT", NULL);
    dz_define("0-x-1", "PIECE", NULL);
    dz_define("0-1-0", "RECT", NULL);
    dz_define("sin", "SIN", NULL);
    dz_define("cos", "COS", NULL);
    dz_define("tan", "TAN", NULL);
    dz_define("asin", "ASIN", NULL);
    dz_define("acos", "ACOS", NULL);
    dz_define("atan", "ATAN", NULL);
    dz_define("exp", "EXP", NULL);
    dz_define("exp-1", "EXPM1", NULL);
    dz_define("log", "LOG", NULL);
    dz_define("log1+", "LOG1P", NULL);
    dz_define("tanh", "TANH", NULL);
    dz_define("atanh", "ATANH", NULL);
    dz_define("cosh", "COSH", NULL);
    dz_define("sinh", "SINH", NULL);

    dz_define("qtanh", "QTANH", FQtanh);
    dz_define("qdtanh", "QDTANH", FQDtanh);
    dz_define("qstdsigmoid", "QSTDSIGMOID", FQstdsigmoid);
    dz_define("qdstdsigmoid", "QDSTDSIGMOID", FQDstdsigmoid);

#ifdef SLOW_EXPONENTIAL
#define P(x) x
#else
#define P(x) NULL
#endif

    dz_define("qexpmx", "QEXPMX", P(FQexpmx));
    dz_define("qdexpmx", "QDEXPMX", P(FQDexpmx));
    dz_define("qexpmx2", "QEXPMX2", P(FQexpmx2));
    dz_define("qdexpmx2", "QDEXPMX2", P(FQDexpmx2));

#undef P
}
