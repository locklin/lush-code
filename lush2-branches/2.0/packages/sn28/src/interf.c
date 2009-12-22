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
 * $Id: interf.c,v 1.1 2003/03/18 18:17:18 leonb Exp $
 **********************************************************************/


/************************************************************************
 *
 *       interf.c: access routines to neurons and synapses fields
 *    
 ************************************************************************/

#include "defn.h"

/* Routines d'acces aux champs des neurones:
   elles ont toujours la syntaxe
   ( Nxxx NUMERO ) en lecture, ou ( Nxxx NUMERO VAL ) en ecriture
   avec  xxx = grad, backsum, val, sum, epsilon.
   toutes passent par Nacces(p,champ)
   */

/* returns the fanin of a cell */
static int 
fanin(neurone *n)
{
  synapse *s;
  int num;
  num=0;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval )  num++;
  return num;
}

DX(xnfanin)
{
  int i;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  if (i<0 || i>=neurnombre)
    error(NIL,"illegal cell number",APOINTER(1));
  return NEW_NUMBER(fanin(neuraddress[i]));
}

#ifdef SYNEPSILON

/* returns the average of the epsilons of the pre-synaptic weights  */
static flt 
averagesyneps(neurone *n)
{
  synapse *s;
  flt sum;
  int num;
  Fclr(sum); num=0;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    num++;
    sum = Fadd(sum, s->Sepsilon);
  }
  if (num>0) return Fdiv(sum,InttoF(num));
  else return Flt0;
}

/* sets all the epsilons of the cell pointed by n to v */
static void 
setsyneps(neurone *n, flt v)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) 
    s->Sepsilon = v;
}

#ifdef NEWTON
/* avsigma: returns the sum of the second derivatives attached to
   the weights belonging to a cell */
static flt 
avsigma(neurone *n)
{
  synapse *s;
  flt sum;
  int num;
  Fclr(sum); num=0;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    num++;
    sum = Fadd(sum, s->Ssigma);
  }
  if (num>0) 
    return Fdiv(sum,InttoF(num));
  else 
    return Flt0;
}

/* sets all the sigma of the cell pointed by n to v */
static void 
setsynsigma(neurone *n, flt v)
{
  synapse *s;
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    s->Ssigma=v;;
  }
}

/* the following endif is for NEWTON */
#endif
/* the following endif is for SYNEPSILON */
#endif

static flt 
get_Nfield(int numero, int champ)
{
  flt val;
  
  switch (champ)
    {
    case F_VAL:
      val=neuraddress[numero]->Nval;
      break;
    case F_SUM:
      val=neuraddress[numero]->Nsum;
      break;
    case F_GRAD:
      val=neuraddress[numero]->Ngrad;
      break;
    case F_BACKSUM:
      val=neuraddress[numero]->Nbacksum;
      break;
    case F_EPSILON:
#ifdef SYNEPSILON
      val=averagesyneps(neuraddress[numero]);
#else
      val=neuraddress[numero]->Nepsilon;
#endif
      break;
#ifdef DESIRED
    case F_DESIRED:
      val=neuraddress[numero]->Ndesired;
      break;
    case F_FREEDOM:
      val=neuraddress[numero]->Nfreedom;
      break;
#endif
#ifdef NEWTON
    case F_GGRAD:
      val=neuraddress[numero]->Nggrad;
      break;
    case F_SQBACKSUM:
      val=neuraddress[numero]->Nsqbacksum;
      break;
    case F_SIGMA:
#ifdef SYNEPSILON
      val=avsigma(neuraddress[numero]);
      break;
#else
      val=neuraddress[numero]->Nsigma;
      break;
#endif
      
#endif
#ifndef NOSPARE
    case F_SPARE1:
      val=neuraddress[numero]->Nspare1;
      break;
    case F_SPARE2:
      val=neuraddress[numero]->Nspare2;
      break;
    case F_SPARE3:
      val=neuraddress[numero]->Nspare3;
      break;
#endif
    default:
      error(NIL,"illegal Nfield number",NEW_NUMBER(champ));
    }
  return val;
}

static void 
set_Nfield(int numero, int champ, float val)
{
  switch (champ)
    {
    case F_VAL:
      neuraddress[numero]->Nval=val;
      break;
    case F_SUM:
      neuraddress[numero]->Nsum=val;
      break;
    case F_GRAD:
      neuraddress[numero]->Ngrad=val;
      break;
    case F_BACKSUM:
      neuraddress[numero]->Nbacksum=val;
      break;
    case F_EPSILON:
#ifdef SYNEPSILON
      setsyneps(neuraddress[numero],val);
#else
      neuraddress[numero]->Nepsilon=val;
#endif
      break;
#ifdef DESIRED
    case F_DESIRED:
      neuraddress[numero]->Ndesired=val;
      break;
    case F_FREEDOM:
      neuraddress[numero]->Nfreedom=val;
      break;
#endif
#ifdef NEWTON
    case F_GGRAD:
      neuraddress[numero]->Nggrad=val;
      break;
    case F_SQBACKSUM:
      neuraddress[numero]->Nsqbacksum=val;
      break;
    case F_SIGMA:
#ifdef SYNEPSILON
      setsynsigma(neuraddress[numero],val);
      break;
#else
      neuraddress[numero]->Nsigma=val;
      break;
#endif
      
#endif
#ifndef NOSPARE
    case F_SPARE1:
      neuraddress[numero]->Nspare1=val;
      break;
    case F_SPARE2:
      neuraddress[numero]->Nspare2=val;
      break;
    case F_SPARE3:
      neuraddress[numero]->Nspare3=val;
      break;
#endif
    default:
      error(NIL,"illegal Nfield number",NEW_NUMBER(champ));
    }
}

static at *
Nacces(int arg_number, at **arg_array, int champ)
{
  flt val = Fzero;
  int numero;
  
  ALL_ARGS_EVAL;
  if (arg_number <1 || arg_number >=3)
    ARG_NUMBER(-1);
  numero = AINTEGER(1);
  if (numero<0 || numero>=neurnombre)
    error(NIL,"illegal neuron number",NEW_NUMBER(numero));
  switch(arg_number) {
  case 1:
    val=get_Nfield(numero,champ);
    break;
  case 2:
    val = AFLT(2);
    set_Nfield(numero,champ,val);
    break;
  }
  return NEW_NUMBER(Ftofp(val));
}


/* get/set the value of a neuron field given its number */
DX(xnfield)
{
  flt val = Fzero;
  int numero,champ;
  
  ALL_ARGS_EVAL;
  if (arg_number <2 || arg_number>3)
    ARG_NUMBER(-1);
  numero = AINTEGER(1);
  champ = AINTEGER(2);
  if (numero<0 || numero>=neurnombre)
    error(NIL,"illegal neuron number",NEW_NUMBER(numero));
  
  switch(arg_number) {
  case 2:
    val=get_Nfield(numero,champ);
    break;
  case 3:
    val = AFLT(3);
    set_Nfield(numero,champ,val);
    break;
  }
  return NEW_NUMBER(Ftofp(val));
}


DX(xnval)
{
  return Nacces(arg_number,arg_array,F_VAL);
}

DX(xnsum)
{
  return Nacces(arg_number,arg_array,F_SUM);
}

DX(xngrad)
{
  return Nacces(arg_number,arg_array,F_GRAD);
}

DX(xnbacksum)
{
  return Nacces(arg_number,arg_array,F_BACKSUM);
}

DX(xnepsilon)
{
  return Nacces(arg_number,arg_array,F_EPSILON);
}

#ifdef DESIRED

DX(xndesired)
{
  return Nacces(arg_number,arg_array,F_DESIRED);
}

DX(xnfreedom)
{
  return Nacces(arg_number,arg_array,F_FREEDOM);
}

#endif

#ifdef NEWTON

DX(xnggrad)
{
  return Nacces(arg_number,arg_array,F_GGRAD);
}

DX(xnsqbacksum)
{
  return Nacces(arg_number,arg_array,F_SQBACKSUM);
}

DX(xnsigma)
{
  return Nacces(arg_number,arg_array,F_SIGMA);
}

#endif

#ifndef NOSPARE
DX(xnspare)
{
  return Nacces(arg_number,arg_array,F_SPARE1);
}

DX(xnspare1)
{
  return Nacces(arg_number,arg_array,F_SPARE1);
}
DX(xnspare2)
{
  return Nacces(arg_number,arg_array,F_SPARE2);
}
DX(xnspare3)
{
  return Nacces(arg_number,arg_array,F_SPARE3);
}
#endif


/* Routines sur l'etat des synapses
   forme (Sval N1 N2) (Sval N1 N2 VAL)
   (Sdelta N1 N2) (Sdelta N1 N2 VAL )
   N1 et N2 sont les numeros des neurones aval et amont.
   */

synapse *
searchsynapse(int amont, int aval)
{
  synapse *s;
  neurone *n;
  if ( aval<0 || amont<0
      || aval>=neurnombre || amont>=neurnombre )
    error(NIL,"illegal neuron number",NIL);
  s=neurbase[amont].FSaval;
  n= &(neurbase[aval]);
  while ( s!=NIL )
    {     
      if (s->Naval==n)
	break;
      s=s->NSamont;
    }
  return s;
}


DX(xsval)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Sval;
      break;
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Sval=val;
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}

DX(xsdelta)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Sdelta;
      break;
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Sdelta=val;
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}


/* returns the length of the weight vector of a cell */
static flt 
weight_sqr_norm(neurone *n, int *fan)
{
  synapse *s;
  flt sum;
  Fclr(sum);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    (*fan)++;
    sum = Fadd(sum, Fmul(s->Sval,s->Sval));
  }
  return sum;
}

/* returns the mean square weight of a cell or of several cells */
DX(xweight_sqr_mean)
{
  int fanin = 0;
  flt norm;
  int i,j;
  
  Fclr(norm);
  ALL_ARGS_EVAL;
  for(j=1;j<=arg_number;j++) {
    i = AINTEGER(j);
    norm=Fadd( norm, weight_sqr_norm( neuraddress [i], &fanin) );
  }
  if (fanin>0)
    return NEW_NUMBER( Ftofp(Fdiv(norm,InttoF(fanin))));
  else
    return NEW_NUMBER(0);
}

/* returns the length of the weight-change of a cell */
static flt 
delta_sqr_norm(neurone *n, int *fan)
{
  synapse *s;
  flt sum;
  Fclr(sum);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    (*fan)++;
    sum = Fadd(sum, Fmul(s->Sdelta, s->Sdelta));
  }
  return sum;
}

/* returns the mean square weight-change of a cell or of several cells */
DX(xdelta_sqr_mean)
{
  int fanin = 0;
  flt norm;
  int i,j;
  
  Fclr(norm);
  ALL_ARGS_EVAL;
  for(j=1;j<=arg_number;j++) {
    i = AINTEGER(j);
    norm=Fadd( norm, delta_sqr_norm( neuraddress [i], &fanin) );
  }
  if (fanin>0)
    return NEW_NUMBER( Ftofp(Fdiv(norm,InttoF(fanin))));
  else
    return NEW_NUMBER(0);
}


#ifdef SYNEPSILON
DX(xsepsilon)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Sepsilon;
      break;
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Sepsilon=val;
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}

#ifdef NEWTON
DX(xssigma)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Ssigma;
      break;
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Ssigma=val;
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}

/* the following endif is for NEWTON */
#endif

/* the following endif is for SYNEPSILON */
#endif

#ifdef ITERATIVE

/* returns the number of connexions sharing a same weight */
DX(xscounter)
{
  synapse *s;
  int amont,aval;
  int val;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  amont = AINTEGER(1);
  aval = AINTEGER(2);
  s=searchsynapse(amont,aval);
  if (s==NIL)
    val = 0;
  else
    val = (int)(s->Scounter);
  return NEW_NUMBER( val);
}

DX(xsacc)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Sacc;
      break;
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Sacc=val;
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}

DX(xsindex)
{
  synapse *s;
  int amont,aval;
  int val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val= (weight *)(s->w) - weightbase;
      break;
      
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AINTEGER(3);
      s=searchsynapse(amont,aval);
      if (val>=weightmax)
	error(NIL,"out of range",NEW_NUMBER(val));
      if (s==NIL)
	error(NIL,"synapse not found",NIL);
      else {
	s->w->Wcounter = Fsub(s->w->Wcounter,Fone);
	s->w = weightbase + val;
	s->w->Wcounter = Fadd(s->w->Wcounter,Fone);
      }
      break;
      
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
  return NEW_NUMBER(val);
}

#ifdef NEWTON
DX(xshess)
{
  synapse *s;
  int amont,aval;
  flt val;
  
  ALL_ARGS_EVAL;
  switch (arg_number) 
    {
    case 2:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	return NIL;
      else
	val=s->Shess;
      break;
      
    case 3:
      amont = AINTEGER(1);
      aval = AINTEGER(2);
      val = AFLT(3);
      s=searchsynapse(amont,aval);
      if (s==NIL)
	error(NIL,"can't set unexistent synapse",NIL);
      else
	s->Shess=val;
      break;
      
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return NEW_NUMBER( Ftofp(val));
}

#endif
#endif



/* --------- INITIALISATION CODE --------- */


void init_interf(void)
{
  dx_define("nfanin",xnfanin);
  dx_define("nfield",xnfield);
  dx_define("nval",xnval);
  dx_define("nsum",xnsum);
  dx_define("ngrad",xngrad);
  dx_define("nbacksum",xnbacksum);
  dx_define("nepsilon",xnepsilon);
#ifdef DESIRED
  dx_define("ndesired",xndesired);
  dx_define("nfreedom",xnfreedom);
#endif
#ifdef NEWTON
  dx_define("nggrad",xnggrad);
  dx_define("nsqbacksum",xnsqbacksum);
  dx_define("nsigma",xnsigma);
#endif
#ifndef NOSPARE
  dx_define("nspare",xnspare);
  dx_define("nspare1",xnspare1);
  dx_define("nspare2",xnspare2);
  dx_define("nspare3",xnspare3);
#endif
  dx_define("sval",xsval);
  dx_define("sdelta",xsdelta);
  dx_define("mean-sqr-weight",xweight_sqr_mean);
  dx_define("mean-sqr-delta",xdelta_sqr_mean);
#ifdef SYNEPSILON
  dx_define("sepsilon",xsepsilon);
#ifdef NEWTON
  dx_define("ssigma",xssigma);
#endif
#endif
#ifdef ITERATIVE
  dx_define("scounter",xscounter);
  dx_define("sacc",xsacc);
  dx_define("sindex",xsindex);
#ifdef NEWTON
  dx_define("shess",xshess);
#endif
#endif
}

