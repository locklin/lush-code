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
 * $Id: arith.c,v 1.1 2003-12-15 14:00:10 leonb Exp $
 **********************************************************************/

#include "header.h"

static char badcomplex[] = "not a complex number";
static char badmatharg[] = "bad argument in math function";
static char divbyzero[] = "division by zero";


/* --------- COMPLEX CLASS --------- */


static struct alloc_root complex_alloc = {
  NIL, 
  NIL, 
  sizeof(complexreal), 
  250 
};

static void
complex_dispose(at *p)
{
  deallocate(&complex_alloc, p->Object);
}

static char *
complex_name(at *p)
{
  char buf[256];
  complexreal c = *(complexreal*)(p->Object);
  strcpy(buf,"::COMPLEX:(");
  strcat(buf,str_number(Creal(c)));
  strcat(buf,",");
  strcat(buf,str_number(Cimag(c)));
  strcat(buf,")");
  strcpy(string_buffer,buf);
  return string_buffer;
}

static void
complex_serialize(at **pp, int code)
{
  real r,i;
  if (code != SRZ_READ)
    {
      r = Creal(*(complexreal*)((*pp)->Object));
      i = Cimag(*(complexreal*)((*pp)->Object));
    }
  serialize_double( &r, code);
  serialize_double( &i, code);
  if (code == SRZ_READ)
    {
      *pp = new_complex(Cnew(r,i));
    }
}

static int
complex_compare(at *p, at *q, int order)
{
  complexreal *r1 = p->Object;
  complexreal *r2 = q->Object;
  if (order)
    error(NIL,"Cannot rank complex numbers", NIL);
  if ((Creal(*r1)==Creal(*r2)) && (Cimag(*r1)==Cimag(*r2)) )
    return 0;
  return 1;
}

static unsigned long
complex_hash(at *p)
{
  complexreal *c = p->Object;
  real r = Creal(*c);
  real i = Cimag(*c);
  unsigned long x = 0x1011;
  x ^= ((unsigned long*)&r)[0];
  x = (x<<1)|((long)x<0 ? 0 : 1);
  x ^= ((unsigned long*)&i)[0];
  if (sizeof(real) >= 2*sizeof(unsigned long)) {
    x ^= ((unsigned long*)&r)[1];
    x = (x<<1)|((long)x<0 ? 0 : 1);
    x ^= ((unsigned long*)&i)[1];
  }
  return x;
}

class complex_class = {
  complex_dispose,
  generic_action,
  complex_name,
  generic_eval,
  generic_listeval,
  complex_serialize,
  complex_compare,
  complex_hash
};

at *
new_complex(complexreal z)
{
  if (! Cimag(z)) {
    return NEW_NUMBER(Creal(z));
  } else {
    complexreal *c = allocate(&complex_alloc);
    *c = z;
    return new_extern(&complex_class, c);
  }
}

int 
complexp(at* p)
{
  if (NUMBERP(p))
    return 1;
  if (EXTERNP(p, &complex_class))
    return 2;
  return 0;
}

complexreal 
get_complex(at *p)
{
  if (NUMBERP(p))
    return Cnew(p->Number, 0);
  else if (EXTERNP(p, &complex_class))
    return *(complexreal*)(p->Object);
  else
    error(NIL,badcomplex,p);
}


DX(xcomplex)
{
  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  return new_complex(Cnew(AREAL(1),AREAL(2)));
}

DX(xcomplexp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (complexp(APOINTER(1)))
    return true();
  return NIL;
}

DX(xconj)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_complex(Cconj(get_complex(APOINTER(1))));
}

DX(xcreal)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Creal(get_complex(APOINTER(1))));
}

DX(xcimag)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Cimag(get_complex(APOINTER(1))));
}

DX(xcabs)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Cabs(get_complex(APOINTER(1))));
}

DX(xcarg)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(Carg(get_complex(APOINTER(1))));
}


/* --------- NUMERICS FUNCTIONS --------- */


DX(xadd)
{
  real sum;
  int i;

  sum = 0.0;
  i = 0;
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
    sum += AREAL(i);
  return NEW_NUMBER(sum);
}

DX(xmul)
{
  real prod;
  int i;

  prod = 1.0;
  i = 0;
  ALL_ARGS_EVAL;
  while (++i <= arg_number)
    prod *= AREAL(i);
  return NEW_NUMBER(prod);
}

DX(xsub)
{
  real a1, a2;

  if (arg_number == 1) {
    ARG_EVAL(1);
    return NEW_NUMBER(-AREAL(1));
  } else {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    a1 = AREAL(1);
    a2 = AREAL(2);
    return NEW_NUMBER(a1 - a2);
  }
}

DX(xdiv)
{
  real a1, a2;

  if (arg_number == 1) {
    ARG_EVAL(1);
    a1 = 1.0;
    a2 = AREAL(1);
  } else {
    ARG_NUMBER(2);
    ALL_ARGS_EVAL;
    a1 = AREAL(1);
    a2 = AREAL(2);
  }
  return NEW_NUMBER(a1 / a2);
}

DX(xpower)
{
  real r;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  r = AREAL(1);
  if (r < 0)
    error(NIL, badmatharg, APOINTER(1));
  return NEW_NUMBER(pow((double) (r), (double) (AREAL(2))));
}

DX(xadd1)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) + 1.0);
}

DX(xsub1)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) - 1.0);
}

DX(xmul2)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) * 2.0);
}

DX(xdiv2)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(AREAL(1) / 2.0);
}

DX(xdivi)
{
  int dv;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  dv = AINTEGER(2);
  if (dv == 0)
    error(NIL, divbyzero, NIL);
  return NEW_NUMBER(AINTEGER(1) / dv);
}

DX(xmodi)
{
  int dv;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  dv = AINTEGER(2);
  if (dv == 0)
    error(NIL, divbyzero, NIL);
  return NEW_NUMBER(AINTEGER(1) % dv);
}


DX(xbitand)
{
  int i;
  int x = ~0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x &= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitor)
{
  int i;
  int x = 0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x |= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitxor)
{
  int i;
  int x = 0;
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++)
    x ^= AINTEGER(i);
  return NEW_NUMBER(x);
}

DX(xbitshl)
{
  int x1, x2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  x1 = AINTEGER(1);
  x2 = AINTEGER(2);
  return NEW_NUMBER(x1 << x2);
}

DX(xbitshr)
{
  int x1, x2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  x1 = AINTEGER(1);
  x2 = AINTEGER(2);
  return NEW_NUMBER(x1 >> x2);
}


/* --------- RANDOM FUNCTIONS --------- */

DX(xseed)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  Dseed((int)AREAL(1));
  return NIL;
}

DX(xrand)
{
  real lo, hi, rand;

  ALL_ARGS_EVAL;
  if (arg_number == 0) {
    lo = 0.0;
    hi = 1.0;
  } else if (arg_number == 1) {
    hi = AREAL(1);
    lo = -hi;
  } else {
    ARG_NUMBER(2);
    lo = AREAL(1);
    hi = AREAL(2);
  }

  rand = Drand();
  return NEW_NUMBER((hi - lo) * rand + lo);
}


DX(xgauss)
{
  real mean, sdev, rand;

  ALL_ARGS_EVAL;
  if (arg_number == 0) {
    mean = 0.0;
    sdev = 1.0;
  } else if (arg_number == 1) {
    mean = 0.0;
    sdev = AREAL(1);
  } else {
    ARG_NUMBER(2);
    mean = AREAL(1);
    sdev = AREAL(2);
  }

  rand = Dgauss();
  return NEW_NUMBER(sdev * rand + mean);
}


DX(xuniq)
{
  static int uniq = 0;
  ARG_NUMBER(0);
  uniq++;
  return NEW_NUMBER(uniq);
}



/* --------- INITIALISATION CODE --------- */

void 
init_arith(void)
{
  /* Complex */
  class_define("COMPLEX",&complex_class);
  dx_define("complex", xcomplex);
  dx_define("complexp", xcomplexp);
  dx_define("creal", xcreal);
  dx_define("cimag", xcimag);
  dx_define("cabs", xcabs);
  dx_define("carg", xcarg);
  dx_define("conj", xconj);
  /* Operations */
  dx_define("+", xadd);
  dx_define("*", xmul);
  dx_define("-", xsub);
  dx_define("/", xdiv);
  dx_define("**", xpower);
  dx_define("1+", xadd1);
  dx_define("1-", xsub1);
  dx_define("2*", xmul2);
  dx_define("2/", xdiv2);
  dx_define("mod", xmodi);
  dx_define("div", xdivi);
  /* Bit operations */
  dx_define("bitand", xbitand);
  dx_define("bitor", xbitor);
  dx_define("bitxor", xbitxor);
  dx_define("bitshl", xbitshl);
  dx_define("bitshr", xbitshr);
  /* Random operations */
  dx_define("seed", xseed);
  dx_define("rand", xrand);
  dx_define("gauss", xgauss);
  dx_define("uniq", xuniq);
}
