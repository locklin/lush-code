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
#include <limits.h>


//static char badcomplex[] = "not a complex number";

/*
#define COMPLEXP(x)  ((x)&&((x)->class == &complex_class))

// --------- COMPLEX CLASS ---------


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
  union { real r; long l[2]; } u[2];
  complexreal *c = p->Object;
  unsigned long x = 0x1011;
  u[0].r = Creal(*c);
  u[1].r = Cimag(*c);
  x ^= u[0].l[0];
  x = (x<<1)|((long)x<0 ? 0 : 1);
  x ^= u[1].l[0];
  if (sizeof(real) >= 2*sizeof(unsigned long)) {
    x ^= u[0].l[1];
    x = (x<<1)|((long)x<0 ? 0 : 1);
    x ^= u[1].l[1];
  }
  return x;
}

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
  if (COMPLEXP(p))
    return 2;
  return 0;
}

complexreal 
get_complex(at *p)
{
  if (NUMBERP(p))
    return Cnew(Number(p), 0);
  else if (COMPLEXP(p))
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
    return t();
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

*/

/* --------- NUMERICS FUNCTIONS --------- */

#define PLUS(a, b)      (a + b)
#define MINUS(a, b)     (a - b)
#define TIMES(a, b)     (a * b)
#define OVER(a, b)      (a / b)
#define CAND(a, b)      (a && b)
#define COR(a, b)       (a || b)
#define MIN(a, b)       ((a < b) ? a : b)
#define MAX(a, b)       ((a > b) ? a : b)

DX_COMMUTATIVE_BINARY_OP(add, PLUS, 0.0)
DX_COMMUTATIVE_BINARY_OP(mul, TIMES, 1.0)
DX_COMMUTATIVE_BINARY_OP(max, MAX, (-INFINITY))
DX_COMMUTATIVE_BINARY_OP(min, MIN, INFINITY)
DX_COMMUTATIVE_BINARY_OP(cand, CAND, true)
DX_COMMUTATIVE_BINARY_OP(cor, COR, false)

DX_NONCOMMUTATIVE_BINARY_OP(sub, MINUS, 0.0)
DX_NONCOMMUTATIVE_BINARY_OP(div, OVER, 1.0)
DX_NONCOMMUTATIVE_BINARY_OP(power, pow, M_E)

/* these are hacks that work with two args only */
/* XXX: get this right                          */
#define CEQ(a, b)       (isnan(a) ? b : (a == b))
#define CNE(a, b)       (isnan(a) ? b : (a != b))
#define CGT(a, b)       (isnan(a) ? b : (a > b))
#define CLT(a, b)       (isnan(a) ? b : (a < b))
#define CGE(a, b)       (isnan(a) ? b : (a >= b))
#define CLE(a, b)       (isnan(a) ? b : (a <= b))

DX_COMMUTATIVE_BINARY_OP(ceq, CEQ, NAN)
DX_COMMUTATIVE_BINARY_OP(cne, CNE, NAN)
DX_COMMUTATIVE_BINARY_OP(cgt, CGT, NAN)
DX_COMMUTATIVE_BINARY_OP(cge, CGE, NAN)
DX_COMMUTATIVE_BINARY_OP(clt, CLT, NAN)
DX_COMMUTATIVE_BINARY_OP(cle, CLE, NAN)


DX(xdivi)
{
   ARG_NUMBER(2);
   int dv = AINTEGER(2);
   if (dv == 0)
      RAISEF("divide by zero", NIL);
   return NEW_NUMBER(AINTEGER(1) / dv);
}

DX(xmodi)
{
   ARG_NUMBER(2);
   int dv = AINTEGER(2);
   if (dv == 0)
      RAISEF("divide by zero", NIL);
   return NEW_NUMBER(AINTEGER(1) % dv);
}


DX(xgptrplus)
{
   ARG_NUMBER(2);
   at *a = APOINTER(1);
   ifn (GPTRP(a) || MPTRP(a))
      error(NIL, "not a pointer", a);

   char *p = Gptr(a);
   int i = AINTEGER(2);
   return NEW_GPTR(p+i);
}

DX(xgptrminus)
{
   ARG_NUMBER(2);
   at *a = APOINTER(1);
   at *b = APOINTER(2);
   ifn (GPTRP(a) || MPTRP(a))
      error(NIL, "not a pointer", a);
   ifn (GPTRP(b) || MPTRP(b))
      error(NIL, "not a pointer", b);

   char *p = Gptr(a);
   char *q = Gptr(b);
   return NEW_NUMBER((int)(p-q));
}


DX(xtestbit)
{
   ARG_NUMBER(2);
   int v = AINTEGER(1);
   int b = AINTEGER(2);
   return NEW_BOOL(v & (1<<b));
}

DX(xbitand)
{
   int x = ~0;
   for (int i=1; i<=arg_number; i++)
      x &= AINTEGER(i);
   return NEW_NUMBER(x);
}

DX(xbitor)
{
   int x = 0;
   for (int i=1; i<=arg_number; i++)
      x |= AINTEGER(i);
   return NEW_NUMBER(x);
}

DX(xbitxor)
{
   int x = 0;
   for (int i=1; i<=arg_number; i++)
      x ^= AINTEGER(i);
   return NEW_NUMBER(x);
}

DX(xbitshl)
{
   ARG_NUMBER(2);
   int x1 = AINTEGER(1);
   int x2 = AINTEGER(2);
   return NEW_NUMBER(x1 << x2);
}

DX(xbitshr)
{
   ARG_NUMBER(2);
   int x1 = AINTEGER(1);
   int x2 = AINTEGER(2);
   return NEW_NUMBER(x1 >> x2);
}


/* --------- INITIALISATION CODE --------- */

//class_t complex_class;

void init_arith(void)
{
   /* setting up complex_class */
   /*
   class_init(&complex_class);
   complex_class.dispose = complex_dispose;
   complex_class.name = complex_name;
   complex_class.serialize = complex_serialize;
   complex_class.compare = complex_compare;
   complex_class.hash = complex_hash;
   class_define("COMPLEX", &complex_class);
   */

   /* Complex */
   /*
   dx_define("complex", xcomplex);
   dx_define("complexp", xcomplexp);
   dx_define("creal", xcreal);
   dx_define("cimag", xcimag);
   dx_define("cabs", xcabs);
   dx_define("carg", xcarg);
   dx_define("conj", xconj);
   */
   /* operations */
   dx_define("+", xadd);
   dx_define("*", xmul);
   dx_define("-", xsub);
   dx_define("/", xdiv);
   dx_define("**", xpower);
   dx_define("modi", xmodi);
   dx_define("div", xdivi);
   dx_define("max", xmax);
   dx_define("min", xmin);

   /* C-style logical operations */
   dx_define("cand", xcand);
   dx_define("cor", xcor);
   /* C-style comparison operations */
   dx_define("c==", xceq);
   dx_define("c!=", xcne);
   dx_define("c>", xcgt);
   dx_define("c<", xclt);
   dx_define("c>=", xcge);
   dx_define("c<=", xcle);

   /* pointer arithmetic */
   dx_define("gptr+", xgptrplus);
   dx_define("gptr-", xgptrminus);

   /* Bit operations */
   dx_define("testbit", xtestbit);
   dx_define("bitand", xbitand);
   dx_define("bitor", xbitor);
   dx_define("bitxor", xbitxor);
   dx_define("bitshl", xbitshl);
   dx_define("bitshr", xbitshr);
   
   /* define minimum and maximum values for integers */
   at* p = var_define("+int-min+");
   var_set(p, NEW_NUMBER(INT_MIN));
   var_lock(p);
   p = var_define("+int-max+");
   var_set(p, NEW_NUMBER(INT_MAX));
   var_lock(p);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
