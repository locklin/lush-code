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

/***********************************************************************
 *  This file is derived from SN-2.8
 *    Copyright (C) 1987-1999 Neuristique s.a.
 ***********************************************************************/

/***********************************************************************
 * $Id: miscop.c,v 1.1 2003/03/18 18:17:18 leonb Exp $
 **********************************************************************/



/******************************************************************
*                                                                       
*       mapneur.c: sequencer functions and utilities(?)      LYB 90     
*                                                                       
******************************************************************/


#include "defn.h"

/* transfers and computations on unit fields */
static int offset1, offset2, offset3;
static flt coeff1, coeff2, coeff3;


/******** compute nfields location ***********/

/* get offset of a unit field wrt the base adress of a unit */
static int 
nfield_offset(int champ)
{
  switch (champ)
    {
    case F_VAL:
      return &(neuraddress[0]->Nval)-(flt*)neuraddress[0];
    case F_SUM:
      return &(neuraddress[0]->Nsum)-(flt*)neuraddress[0];
    case F_GRAD:
      return &(neuraddress[0]->Ngrad)-(flt*)neuraddress[0];
    case F_BACKSUM:
      return &(neuraddress[0]->Nbacksum)-(flt*)neuraddress[0];
#ifndef SYNEPSILON
    case F_EPSILON:
      return &(neuraddress[0]->Nepsilon)-(flt*)neuraddress[0];
#endif
#ifdef DESIRED
    case F_DESIRED:
      return &(neuraddress[0]->Ndesired)-(flt*)neuraddress[0];
    case F_FREEDOM:
      return &(neuraddress[0]->Nfreedom)-(flt*)neuraddress[0];
#endif
#ifdef NEWTON
    case F_GGRAD:
      return &(neuraddress[0]->Nggrad)-(flt*)neuraddress[0];
    case F_SQBACKSUM:
      return &(neuraddress[0]->Nsqbacksum)-(flt*)neuraddress[0];
#ifndef SYNEPSILON
    case F_SIGMA:
      return &(neuraddress[0]->Nsigma)-(flt*)neuraddress[0];
#endif
#endif
      
#ifndef NOSPARE
    case F_SPARE1:
      return &(neuraddress[0]->Nspare1)-(flt*)neuraddress[0];
    case F_SPARE2:
      return &(neuraddress[0]->Nspare2)-(flt*)neuraddress[0];
    case F_SPARE3:
      return &(neuraddress[0]->Nspare3)-(flt*)neuraddress[0];
#endif
    default:
      error(NIL,"illegal Nfield number",NEW_NUMBER(champ));
    }
}



/******** ops on nfields ***********/


static void 
setfield(neurone *n)
{
  flt *m;
  m= (flt*)n;
  *(m+offset1) = coeff1;
}

static void 
set_field(at *l1, int f1, float v)
{
    offset1=nfield_offset(f1);
    coeff1 = v;
    mapneur(l1,setfield);
}

DX(xset_nfield)
{
    ALL_ARGS_EVAL;
    ARG_NUMBER(3);
    set_field(ACONS(1),AINTEGER(2),AFLT(3));
    LOCK(APOINTER(3));
    return APOINTER(3);
}



static void 
cpfield(neurone *n)
{
  flt *m;
  m= (flt*)n;
  *(m+offset1) = *(m+offset2);
}

static void 
cp2field(neurone *n1, neurone *n2)
{
  *((flt *)n1+offset1) = *((flt *)n2+offset2);
}

void 
copy_field(at *l1, int f1, at *l2, int f2)
{
  offset1=nfield_offset(f1);
  offset2=nfield_offset(f2);
  if (l2==NIL) {
    mapneur(l1,cpfield);
  } else {
    map2neur(l1,l2,cp2field);
  }
}

DX(xcopy_nfield)
{
  int f1, f2;
  at *l1, *l2;
  ALL_ARGS_EVAL;
  switch(arg_number) 
    {
    case 3:
      l1 = ACONS(1);
      f1 = AINTEGER(2);
      l2 = NIL;
      f2 = AINTEGER(3);
      break;
    case 4:
      l1 = ACONS(1);
      f1 = AINTEGER(2);
      l2 = ACONS(3);
      f2 = AINTEGER(4);
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  copy_field(l1,f1,l2,f2);
  return NIL;
}

static void opfield(neurone *n)   /* ouaf ouaf gnark */
           
{
  flt *m;
  m= (flt*)n;
  *(m+offset1)= Fadd( Fmul(coeff2,*(m+offset2)), Fmul(coeff3,*(m+offset3)));
}

void op_field(at *l, int f1, int f2, float g2, int f3, float g3)
{
  offset1=nfield_offset(f1);
  offset2=nfield_offset(f2);
  offset3=nfield_offset(f3);
  coeff2 =g2;
  coeff3 =g3;
  mapneur(l,opfield);
}

DX(xop_nfield)
{
  int f1, f2, f3;
  flt g2, g3;
  at *l;

  ARG_NUMBER(6);
  ALL_ARGS_EVAL;
  l = ACONS(1);
  f1 = AINTEGER(2);
  f2 = AINTEGER(3); g2 = AFLT(4);
  f3 = AINTEGER(5); g3 = AFLT(6);
  op_field(l,f1,f2,g2,f3,g3);
  return NIL;
}

static flt dotprod;

static void 
mulfield(neurone *n)
{
  flt *m;
  m = (flt*)n;
  *(m+offset1)=Fmul(*(m+offset2),*(m+offset3));
  dotprod = Fadd( dotprod, *(m+offset1) );
}

static flt 
mul_field(at *l, int f1, int f2, int f3)
{
  offset1=nfield_offset(f1);
  offset2=nfield_offset(f2);
  offset3=nfield_offset(f3);
  dotprod=Flt0;
  mapneur(l,mulfield);
  return dotprod;
}

DX(xmul_nfield)
{
  int f1, f2, f3;
  at *l;
  flt v;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  l = ACONS(1);
  f1 = AINTEGER(2);
  f2 = AINTEGER(3);
  f3 = AINTEGER(4);
  v=mul_field(l,f1,f2,f3);
  return NEW_NUMBER(Ftofp(v));
}

static void 
invfield(neurone *n)
{
  flt *m;
  m= (flt*)n;
  *(m+offset1)=Fdiv(coeff2,*(m+offset2));
  dotprod = Fadd(dotprod, *(m+offset1));
}

static flt 
invert_field(at *l, int f1, int f2, float c2)
{
  offset1=nfield_offset(f1);
  offset2=nfield_offset(f2);
  coeff2=c2;
  dotprod=Flt0;
  mapneur(l,invfield);
  return dotprod;
}

DX(xinvert_nfield)
{
  int f1, f2;
  flt c2;
  at *l;
  flt v;

  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  l = ACONS(1);
  f1 = AINTEGER(2);
  f2 = AINTEGER(3);
  c2 = AFLT(4);
  v=invert_field(l,f1,f2,c2);
  return NEW_NUMBER(Ftofp(v));
}


/******** ops on sfields ***********/

static flt 
read_sfield(synapse *s, int f)
{
  switch (f) {
  case FS_VAL:
    return s->Sval;
  case FS_DELTA:
    return s->Sdelta;
#ifdef ITERATIVE
  case FS_ACC:
    return s->Sacc;
#endif
#ifdef SYNEPSILON
  case FS_EPSILON:
    return s->Sepsilon;
#ifdef NEWTON
  case FS_SIGMA:
    return s->Ssigma;
#ifdef ITERATIVE
  case FS_HESS:
    return s->Shess;
#endif
#endif
#endif
  default:
    error(NIL,"illegal sfield number",NEW_NUMBER(f));
  }
}


static void 
write_sfield(synapse *s, int f, float v)
{
  switch (f) {
  case FS_VAL:
    s->Sval = v;
    break;
  case FS_DELTA:
    s->Sdelta = v;
    break;
#ifdef ITERATIVE
  case FS_ACC:
    s->Sacc = v;
    break;
#endif
#ifdef SYNEPSILON
  case FS_EPSILON:
    s->Sepsilon = v;
    break;
#ifdef NEWTON
  case FS_SIGMA:
    s->Ssigma = v;
    break;
#ifdef ITERATIVE
  case FS_HESS:
    s->Shess = v;
    break;
#endif
#endif
#endif
  default:
    error(NIL,"illegal sfield number",NEW_NUMBER(f));
  }
}


static void 
setsfield(neurone *n)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval)
    write_sfield(s,offset1,coeff1);
}

DX(xset_sfield)
{
    ALL_ARGS_EVAL;
    ARG_NUMBER(3);
    offset1 = AINTEGER(2);
    coeff1 = AFLT(3);
    mapneur(ACONS(1),setsfield);
    LOCK(APOINTER(3));
    return APOINTER(3);
}

static void 
cpsfield(neurone *n)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval)
    write_sfield(s,offset1,read_sfield(s,offset2));
}

static void 
cp2sfield(neurone *n1, neurone *n2)
{
  synapse *s1,*s2;
  for (s1=n1->FSamont, s2=n2->FSamont;
       s1 && s2;
       s1=s1->NSaval, s2=s2->NSaval )
    write_sfield(s1,offset1,read_sfield(s2,offset2));
}

DX(xcopy_sfield)
{
  ALL_ARGS_EVAL;
  switch(arg_number) {
  case 3:
    offset1 = AINTEGER(2);
    offset2 = AINTEGER(3);
    mapneur(ACONS(1),cpsfield);
    break;
  case 4:
    offset1 = AINTEGER(2);
    offset2 = AINTEGER(4);
    map2neur(ACONS(1),ACONS(3),cp2sfield);
    break;
  default:
    ARG_NUMBER(-1);
  }
  return NIL;
}

static void 
opsfield(neurone *n)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval)
    write_sfield(s,offset1,
		 Fadd(Fmul(coeff2,read_sfield(s,offset2)),
		      Fmul(coeff3,read_sfield(s,offset3)) ) );
}
DX(xop_sfield)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(6);

  offset1 = AINTEGER(2);
  offset2 = AINTEGER(3); coeff2 = AFLT(4);
  offset3 = AINTEGER(5); coeff3 = AFLT(6);

  mapneur(ACONS(1),opsfield);
  return NIL;
}

static void 
mulsfield(neurone *n)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval)
    write_sfield(s,offset1,
		 Fmul(read_sfield(s,offset2),
		      read_sfield(s,offset3)));
}

DX(xmul_sfield)
{
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  offset1 = AINTEGER(2);
  offset2 = AINTEGER(3);
  offset3 = AINTEGER(4);
  mapneur(ACONS(1),mulsfield);
  return NIL;
}

static void 
invsfield(neurone *n)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval)
    write_sfield(s,offset1,
		 Fdiv(coeff2,read_sfield(s,offset2)));
}

DX(xinvert_sfield)
{
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  offset1 = AINTEGER(2);
  offset2 = AINTEGER(3);
  coeff2 = AFLT(4);

  mapneur(ACONS(1),invsfield);
  return NIL;
}


/******** ops between nfields and sfields ***********/


/* takes the field f2 from the pre-synaptic cells of cell k,
   and copy it to field f1 of the connecting synapses       */

static void 
set_SwN(int k, int f1, int f2)
{
  neurone *n;
  synapse *s;
  n=neuraddress[k];
  offset2=nfield_offset(f2);
  switch (f1) {
  case FS_VAL :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sval= *((flt *)(s->Namont)+offset2);
    break;
  case FS_DELTA :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sdelta= *((flt *)(s->Namont)+offset2);
    break;
#ifdef ITERATIVE
  case FS_ACC :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sacc = *((flt *)(s->Namont)+offset2);
    break;
#endif
#ifdef SYNEPSILON
  case FS_EPSILON :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
       s->Sepsilon = *((flt *)(s->Namont)+offset2);
    break;
#ifdef NEWTON
  case FS_SIGMA :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Ssigma= *((flt *)(s->Namont)+offset2);
    break;
#endif
#endif
  default:
    error(NIL,"illegal Sfield number",NEW_NUMBER(f1));
  }
}

DX(xset_swn)
{
  int n, f2, f3;
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  n = AINTEGER(1);
  f2 = AINTEGER(2);
  f3 = AINTEGER(3);
  set_SwN(n,f2,f3);
  return NIL;
}

static void 
acc_SwN(int k, int f1, int f2)
{
  neurone *n;
  synapse *s;
  n=neuraddress[k];
  offset2=nfield_offset(f2);
  switch (f1) {
  case FS_VAL :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sval += *((flt *)(s->Namont)+offset2);
    break;
  case FS_DELTA :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sdelta += *((flt *)(s->Namont)+offset2);
    break;
#ifdef ITERATIVE
  case FS_ACC :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Sacc += *((flt *)(s->Namont)+offset2);
    break;
#endif
#ifdef SYNEPSILON
  case FS_EPSILON :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
       s->Sepsilon += *((flt *)(s->Namont)+offset2);
    break;
#ifdef NEWTON
  case FS_SIGMA :
    for ( s=n->FSamont; s!=NIL; s=s->NSaval)
      s->Ssigma += *((flt *)(s->Namont)+offset2);
    break;
#endif
#endif
  default:
    error(NIL,"illegal Sfield number",NEW_NUMBER(f1));
  }
}

DX(xacc_swn)
{
  int n, f2, f3;
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  n = AINTEGER(1);
  f2 = AINTEGER(2);
  f3 = AINTEGER(3);
  acc_SwN(n,f2,f3);
  return NIL;
}





/* ============== FLAG FUNCTIONS ============= */

DX(xdesired_mode)
{
  ARG_NUMBER(0);
#ifdef DESIRED
  return t();
#else
  return NIL;
#endif
}

DX(xnewton_mode)
{
  ARG_NUMBER(0);
#ifdef NEWTON
  return t();
#else
  return NIL;
#endif
}

DX(xiterative_mode)
{
  ARG_NUMBER(0);
#ifdef ITERATIVE
  return t();
#else
  return NIL;
#endif
}

DX(xsynepsilon_mode)
{
  ARG_NUMBER(0);
#ifdef SYNEPSILON
  return t();
#else
  return NIL;
#endif
}


/* add a zero mean gaussian noise on unit pointed by n */

static flt v_add_gauss;

static void 
add_gauss(neurone *n)
{
  n->Nval = Fadd( n->Nval, Fmul(v_add_gauss,Fgauss()) );
}

/* add a zero mean gaussian noise on a list of units */
DX(xgauss_state)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  v_add_gauss = AFLT(2);
  mapneur(ALIST(1),add_gauss);
  return NIL;
}

static flt flip_prob;

static void 
flip_bit(neurone *n)
{
  if ( Frand() < flip_prob )
    n->Nval = Fsub( Flt0, n->Nval );
}

/* reverse sign of a cell state with a given probability */
DX(xflip_state)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  flip_prob = AFLT(2);
  mapneur(ALIST(1),flip_bit);
  return NIL;
}




/* --------- INITIALISATION CODE --------- */

void 
init_miscop(void)
{
  dx_define("set-nfield",xset_nfield);
  dx_define("copy-nfield",xcopy_nfield);
  dx_define("op-nfield",xop_nfield);
  dx_define("mul-nfield",xmul_nfield);
  dx_define("invert-nfield",xinvert_nfield);
  dx_define("set-sfield",xset_sfield);
  dx_define("copy-sfield",xcopy_sfield);
  dx_define("op-sfield",xop_sfield);
  dx_define("mul-sfield",xmul_sfield);
  dx_define("invert-sfield",xinvert_sfield);
  dx_define("set-swn",xset_swn);
  dx_define("acc-swn",xacc_swn);
  dx_define("desired-mode",xdesired_mode);
  dx_define("newton-mode",xnewton_mode);
  dx_define("iterative-mode",xiterative_mode);
  dx_define("synepsilon-mode",xsynepsilon_mode);
  dx_define("gauss-state",xgauss_state);
  dx_define("flip-state",xflip_state);
}
