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
 * $Id: gbp.c,v 1.3 2005/08/04 18:20:42 leonb Exp $
 **********************************************************************/


#include "defn.h"


/******** global variables (historical) ********/



/********* update_weightedsum **********/

/*
 *  (update-weighted-sum [theta] ...layer... )  
 */


static void 
updN_sum(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_sum) 
    {
      (*n->type->updN_sum)(n);
    }
  else
#endif
    {
      synapse *s;
      flt sum,prod;
      Fclr(sum);
      for ( s=n->FSamont; s!=NIL; s=s->NSaval ) 
	{
	  prod = Fmul( s->Sval, s->Namont->Nval );
	  sum  = Fadd( sum,prod );
	}
      if (theta != Flt0) 
	{
	  prod=Fmul(Fgauss(),theta);
	  sum=Fadd( sum,prod );
	}
      n->Nsum=sum;
    }
}

DX(xupdate_weighted_sum)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && ISNUMBER(i)) {
    theta = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_theta);
    if (p && (p->flags&C_NUMBER))
      theta = rtoF(p->Number);
    else
      error("theta","Not a number",p);
    UNLOCK(p);
  }
  while (i<=arg_number) {
    mapneur(ALIST(i),updN_sum);
    i++;
  }
  return t();
}




/********* update_state_only **********/

/*
 *  (update-state-only [nlf] ...layer...)
 *  (update-state [theta] [nlf] ...layer...)  
 */


void 
updN_val(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_val)
    {
      (*n->type->updN_val)(n);
    }
  else
#endif
    {
      n->Nval = CALLNLF(n->Nsum);
    }
}

DX(xupdate_state_only)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    nlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_nlf);
    if (NLFP(p))
      nlf = p->Object;
    else
      error("nlf","not a nlf",p);
    UNLOCK(p);
  }
  while(i<=arg_number) {
    mapneur(APOINTER(i),updN_val);
    i++;
  }
  return t();
}


DX(xupdate_state)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && ISNUMBER(i)) {
    theta = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_theta);
    if (p && (p->flags&C_NUMBER))
      theta = rtoF(p->Number);
    else
      error("theta","Not a number",p);
    UNLOCK(p);
  }
  if (i<=arg_number && NLFP(APOINTER(i))) {
    nlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_nlf);
    if (NLFP(p))
      nlf = p->Object;
    else
      error("nlf","not a nlf",p);
    UNLOCK(p);
  }
  while(i<=arg_number) {
    mapneur(ALIST(i),updN_sum);
    mapneur(APOINTER(i),updN_val);
    i++;
  }
  return t();
}




/********* update_back_sum **********/

/*
 *  (update-back-sum ...layer... )  
 */


static void 
updN_backsum(neurone *n)
{
#ifndef NONEURTYPE
  synapse *s = n->FSaval;
  n->Nbacksum = Fzero;
  while (s)
    {
      neurtype *taval = s->Naval->type;
      if (taval && taval->updN_backsum_term)
	s = (*taval->updN_backsum_term)(n, s);
      else {
	flt sum, prod;
	Fclr(sum);
	for (; s && s->Naval->type==taval; s=s->NSamont ) {
	  prod = Fmul( s->Sval, s->Naval->Ngrad );
	  sum  = Fadd( sum,prod );
	}
	n->Nbacksum = Fadd(n->Nbacksum, sum);
      }
    }
#else
  synapse *s;
  flt sum , prod;
  Fclr(sum);
  for ( s=n->FSaval; s!=NIL; s=s->NSamont ) {
    prod = Fmul( s->Sval, s->Naval->Ngrad );
    sum  = Fadd( sum,prod );
  }
  n->Nbacksum=sum;
#endif
}

DX(xupdate_back_sum)
{
  int i;
  
  ALL_ARGS_EVAL;
  for (i=1;i<=arg_number;i++)
    mapneur(ALIST(i),updN_backsum);
  return t();
}





/********* update_gradient_only **********/

/*
 *  (update-gradient-only [dnlf] ...layer... )  
 *  (update-gradient [dnlf] ...layer... )  
 */


void 
updN_grad(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_grad) 
    {
      (*n->type->updN_grad)(n);
    }
  else
#endif
    {
      n->Ngrad = Fmul( n->Nbacksum, CALLDNLF(n->Nsum) );
    }
}

DX(xupdate_gradient_only)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  while (i<=arg_number) {
    mapneur(ALIST(i),updN_grad);
    i++;
  }
  return NEW_NUMBER(arg_number);
}

DX(xupdate_gradient)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  while (i<=arg_number) {
    mapneur(ALIST(i),updN_backsum);
    mapneur(ALIST(i),updN_grad);
    i++;
  }
  return t();
}

/********* update_deltastate ******/




static void 
updN_deltastate(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_deltastate)
    {
      (*n->type->updN_deltastate)(n);
    }
  else
#endif
    {
      synapse *s;
      flt sum,prod;
      Fclr(sum);
      for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
	prod = Fmul( s->Sval, s->Namont->Ngrad );
	sum  = Fadd( sum, prod );
	prod = Fmul( s->Sdelta, s->Namont->Nval );
	sum  = Fadd( sum, prod );
      }
      n->Ngrad = Fmul(sum, CALLDNLF(n->Nsum) );
    }
}


DX(xupdate_deltastate)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  while (i<=arg_number) {
    mapneur(ALIST(i),updN_deltastate);
    i++;
  }
  return t();
}


/********* init_grad_lms **********/

/*
 *  (init-grad-lms [dnlf] outputlayer ideallayer)
 */

static flt energie;

#ifdef NEWTON
static void 
setsqback1(neurone *n)
{
  n->Nsqbacksum=Flt1;
}
#endif  /* NEWTON */

/* initN-lms-grad */

static void 
iniN_lms_grad(neurone *n, neurone *ideal)
{
  flt x;
  x = Fsub( ideal->Nval, n->Nval );
  energie=Fadd(energie,Fmul(FltHalf,Fmul(x,x)));
  n->Nbacksum =  x;
  updN_grad(n);
}

DX(xinit_grad_lms)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  ARG_NUMBER(i+1);
  Fclr(energie);
  map2neur(ALIST(i),ALIST(i+1),iniN_lms_grad);
#ifdef NEWTON
  mapneur(ALIST(i),setsqback1);
#endif
  return NEW_NUMBER(Ftofp(energie));
}


/********* init_grad_thlms **********/

/*
 *  (init-grad-thlms dnlf outputlayer ideallayer [threshold])
 */

static flt g_threshold;
     
static void 
iniN_thlms_grad(neurone *n, neurone *ideal)
{
  flt x;
  x = Fsub( ideal->Nval, n->Nval );
  if ( ideal->Nval > g_threshold) {
    if ( x < Flt0 ) x=Flt0;
  }
  else {
    if ( x > Flt0 ) x=Flt0;
  }
  energie=Fadd(energie,Fmul(FltHalf,Fmul(x,x)));
  n->Nbacksum = x;
  updN_grad(n);
}

DX(xinit_grad_thlms)
{
  int i=1;
  ALL_ARGS_EVAL;
  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  if (arg_number == i+2)
    g_threshold = AFLT(i+2);
  else {
    g_threshold = Fzero;
    ARG_NUMBER(i+1);
  }
  Fclr(energie);
  map2neur(ALIST(i),ALIST(i+1),iniN_thlms_grad);
#ifdef NEWTON
  mapneur(ALIST(i),setsqback1);
#endif
  return NEW_NUMBER(Ftofp(energie));
}






/********* update_ggradient **********/

/*
 *  (update-ggradient [dnlf [ddnlf]] [gamma] ...layers...)
 *  (update-lmggradient [dnlf] [gamma] ...layers...)
 */

#ifdef NEWTON

#ifndef SYNEPSILON
static flt 
mean_sq_input(neurone *n)
{
  synapse *s;
  flt sum, nbre;
  
  Fclr(sum);
  Fclr(nbre);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    flt x = s->Namont->Nval;
    sum  = Fadd( sum, Fmul(x,x) );
    nbre=Fadd(nbre,Flt1);
  };
  if (nbre>Flt0) 
    return Fdiv(sum,nbre);
  else
    return Flt0;
}
#endif

/* compute the backward sum of the
 * 2nd-derivative weighted by the squared weights 
 */
static void 
updN_sqbacksum(neurone *n)
{
  if (n->FSaval != NIL) {
#ifndef NONEURTYPE
    synapse *s = n->FSaval;
    n->Nsqbacksum = Fzero;
    while (s)
      {
        neurtype *taval = s->Naval->type;
        if (taval && taval->updN_sqbacksum_term)
          s = (*taval->updN_sqbacksum_term)(n, s);
        else {
          flt sum, prod;
          Fclr(sum);
          for (; s && s->Naval->type==taval; s=s->NSamont ) {
            prod = Fmul( Fmul(s->Sval,s->Sval), s->Naval->Nggrad );
            sum  = Fadd( sum,prod );
          }
          n->Nsqbacksum = Fadd(n->Nsqbacksum, sum);
        }
      }
#else
    synapse *s;
    flt sum , prod;
    Fclr(sum);
    for ( s=n->FSaval; s!=NIL; s=s->NSamont ) {
      prod = Fmul( Fmul(s->Sval,s->Sval), s->Naval->Nggrad );
      sum  = Fadd( sum,prod );
    }
    n->Nsqbacksum=sum;
#endif
  }
}

/* compute the instantaneous 
 * 2nd-derivative (without the input) 
 */
void 
updN_ggrad(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_ggrad)
    {
      (*n->type->updN_ggrad)(n);
    }
  else
#endif
  {
    flt x = CALLDNLF(n->Nsum);
    n->Nggrad = Fsub(Fmul( n->Nsqbacksum, Fmul(x,x) ),
		     Fmul( n->Nbacksum, CALLDDNLF(n->Nsum)) );
  }
}

/* compute the instantaneous 
 * 2nd-derivative (without the input) 
 */
void 
updN_lmggrad(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_lmggrad)
    {
      (*n->type->updN_lmggrad)(n);
    }
  else
#endif
    {
      flt x = CALLDNLF(n->Nsum);
      n->Nggrad = Fmul( n->Nsqbacksum, Fmul(x,x) );
    }
}

/* computes the average curvature 
 * for a cell or the connection to a cell 
 */
static void 
updN_sigma(neurone *n)
{
#ifndef NONEURTYPE
  if (n->type && n->type->updN_sigma)
    {
      (*n->type->updN_sigma)(n);
    }
  else
#endif
    {
#ifdef SYNEPSILON
      synapse *s;
      flt gammaggrad, oneminusgamma;
      oneminusgamma=Fsub(Flt1, mygamma);
      gammaggrad=Fmul(mygamma, n->Nggrad);
      for ( s=n->FSamont; s!=NIL; s=s->NSaval) 
	{
          flt x = s->Namont->Nval;
	  s->Ssigma = Fadd(Fmul(oneminusgamma, s->Ssigma),
			   Fmul(gammaggrad, Fmul(x,x)) );
	}
#else
      n->Nsigma = Fadd(Fmul( Fsub(Flt1, mygamma), n->Nsigma),
		       Fmul( Fmul(mygamma, n->Nggrad), mean_sq_input(n) ));
#endif
    }
}


DX(xupdate_ggradient)
{
  int i=1;
  
  ALL_ARGS_EVAL;

  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }

  if (i<=arg_number && NLFP(APOINTER(i))) {
    ddnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_ddnlf);
    if (NLFP(p))
      ddnlf = p->Object;
    else
      error("ddnlf","not a nlf",p);
    UNLOCK(p);
  }
  if (i<=arg_number && ISNUMBER(i)) {
    mygamma = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_gamma);
    if (p && (p->flags&C_NUMBER))
      mygamma = rtoF(p->Number);
    else
      error("gamma","Not a number",p);
    UNLOCK(p);
  }

  while(i<=arg_number) {
    mapneur(ALIST(i),updN_sqbacksum);
    mapneur(APOINTER(i),updN_ggrad);
    mapneur(APOINTER(i),updN_sigma);
    i++;
  }
  return NIL;
}


DX(xupdate_lmggradient)
{
  int i=1;
  ALL_ARGS_EVAL;

  if (i<=arg_number && NLFP(APOINTER(i))) {
    dnlf = APOINTER(i)->Object;
    i++;
  } else {
    at *p = var_get(var_dnlf);
    if (NLFP(p))
      dnlf = p->Object;
    else
      error("dnlf","not a nlf",p);
    UNLOCK(p);
  }
  if (i<=arg_number && ISNUMBER(i)) {
    mygamma = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_gamma);
    if (p && (p->flags&C_NUMBER))
      mygamma = rtoF(p->Number);
    else
      error("gamma","Not a number",p);
    UNLOCK(p);
  }

  while(i<=arg_number) {
    mapneur(ALIST(i),updN_sqbacksum);
    mapneur(APOINTER(i),updN_lmggrad);
    mapneur(APOINTER(i),updN_sigma);
    i++;
  }
  return NIL;
}

#endif /* NEWTON */





/********* update_weight **********/

/*
 *  (update-weight [alpha [decay]] [...layers...])
 */

/* this section contains many different versions of the same procedure
 * Four series: 
 * iterative, with one epsilon per cell,
 * iterative, with one epsilon per weight,
 * non-iterative with one epsilon per cell,
 * non-iterative, with one epsilon per weight,
 * each of these with the pseudo-newton algorithm
 */



#ifdef ITERATIVE

void 
clear_acc(weight *nptr)
{
  int iend;
  weight *wei;
  int i;
  
  if (nptr==NULL) {
    wei=weightbase;
    iend=weightnombre; 
  } else {
    wei=nptr;
    iend=1;
  }
  for ( i=0; i<iend; i++,wei++ ) {
    Fclr(wei->Wacc); 
  }
}

void 
updS_acc(neurone *nptr)
{
  int iend;
  synapse *s;
  neurone *amont, *aval;
  int i;
#ifndef SYNEPSILON
  flt preprod;
#endif
  flt eps;

#ifndef NONEURTYPE
  /* Type dependent procedure */
  if (nptr==NULL) 
    {
      aval=neurbase;
      iend=neurnombre;
      for ( i=0; i<iend; i++, aval++ )
	updS_acc(aval);
      return;
    }
  if (nptr->type && nptr->type->updS_acc)
    {
      (*nptr->type->updS_acc)(nptr);
      return;
    }
#endif

  /* Optimized procedure */
  if (nptr==NULL) {
    aval=neurbase;
    iend=neurnombre; 
  } else {
    aval=nptr;
    iend=1; 
  }

#ifdef SYNEPSILON
  for ( i=0; i<iend; i++, aval++ ) {
    for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
      eps=s->Sepsilon;
      amont=s->Namont;
      s->Sacc = Fadd(s->Sacc,
		     Fmul(amont->Nval,
			  Fmul(aval->Ngrad, eps) ) );
    }
  }
#else /* NOT SYNEPSILON */
  for ( i=0; i<iend; i++, aval++ ) {
    eps=aval->Nepsilon;
    preprod=Fmul( aval->Ngrad, eps );
    for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
      amont=s->Namont;
      s->Sacc = Fadd( s->Sacc,
                      Fmul( amont->Nval, preprod ) );
    }
  }
#endif /* SYNEPSILON */
}

#ifdef NEWTON

/* functions to compute and 
 * use the inverse diag hessian
 * in iterative and/or batch mode 
 */
void 
clear_hess(weight *nptr)
{
  int iend;
  weight *wei;
  int i;

  if (nptr==NULL) {
    wei=weightbase;
    iend=weightnombre;
  } else {
    wei=nptr;
    iend=1; 
  }
  for ( i=0; i<iend; i++,wei++ ) { 
    Fclr(wei->Whess); 
  }
}

/* accumulate sigmas
 * into hessian 
 */
void 
updS_hess(neurone *nptr)
{
  int iend;
  synapse *s;
  neurone *aval;
  int i;
#ifndef SYNEPSILON
  flt eps;
#endif

  if (nptr==NIL) {
    aval=neurbase;
    iend=neurnombre; }
  else {
    aval=nptr;
    iend=1; }
#ifdef SYNEPSILON
  for ( i=0; i<iend; i++, aval++ ) {
    for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
      s->Shess = Fadd(s->Shess, s->Ssigma);
    }
  }
#else /* NOT SYNEPSILON */
  for ( i=0; i<iend; i++, aval++ ) {
    eps = aval->Nsigma;
    for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
      s->Shess = Fadd(s->Shess, eps);
    }
  }
#endif /* SYNEPSILON */
}


/* multiply the accumulators
 * by the inverse diagonal
 * hessian (hess) 
 */
void 
Hess_acc(weight *nptr)
{
  int i, iend;
  weight *w;

  if (nptr==NULL) {
    w=weightbase;
    iend=weightnombre;
  } else {
    w=nptr;
    iend=1; 
  }
  for ( i=0; i<iend; i++, w++ ) {
    w->Wacc=Fdiv( w->Wacc, Fadd( mu, Fabs(w->Whess)));
  }
}
#endif /* NEWTON */


/* update the delta 
   (iterative version) */

void 
updS_delta(weight *nptr)
{
  int i, iend;
  weight *w;
  flt unmoinsalpha;

  if (nptr==NULL) {
    w=weightbase;
    iend=weightnombre;
  } else {
    w=nptr;
    iend=1;
  }
  
  if (alpha != Flt0) {
    unmoinsalpha=Fsub(Flt1, alpha);
    for ( i=0; i<iend; i++, w++ ) {
      w->Wdelta=Fadd(Fmul( alpha, w->Wdelta ),
		     Fmul( unmoinsalpha, w->Wacc ) );
    }
  } else {
    for ( i=0; i<iend; i++, w++ ) {
      w->Wdelta= w->Wacc;
    }
  }
}


/* the weights from delta
   (iterative version) */

/* CAUTION: in iterative mode, "decay" is NOT scaled
   by epsilon as in the other modes (since a single weight
   depends on several connections
   and thus several epsilons) */

void 
updS_val(weight *nptr)
{
  int i, iend;
  weight *w;
  flt unmoinsdecay;

  if (nptr==NULL) {
    w=weightbase;
    iend=weightnombre;
  } else {
    w=nptr;
    iend=1; 
  }
  if (decay != Flt0) {
    unmoinsdecay=Fsub(Flt1, decay);
    for ( i=0; i<iend; i++, w++ ) {
      w->Wval=Fadd( Fmul( unmoinsdecay, w->Wval ), w->Wdelta  );
    }
  }
  else {
    for ( i=0; i<iend; i++, w++ ) {
      w->Wval=Fadd( w->Wval, w->Wdelta  );
    }
  }
}


/* updates all the weight
   variables */

static void 
updS_all(neurone *nptr)
{
  clear_acc(NULL);
  updS_acc(nptr);
  updS_delta(NULL);
  updS_val(NULL);
}

#ifdef NEWTON

/* can be used for iterative
   on-line newton version */

static void 
updS_new_all(neurone *nptr)
{
  clear_acc(NULL);
  updS_acc(nptr);
  clear_hess(NULL);
  updS_hess(nptr);
  Hess_acc(NULL);
  updS_delta(NULL);
  updS_val(NULL);
}
#endif /* NEWTON */


#else /* NOT ITERATIVE */

static void 
updS_all(neurone *nptr)
{
  synapse *s;
  neurone *amont, *aval;
  int i,iend;
  flt eps, delta;
  flt preprod, predecay;
  flt unmoinsalpha = Fsub(Flt1, alpha);

#ifndef NONEURTYPE
  /* type dependent procedure */
  if (nptr==NULL) 
    {
      aval=neurbase;
      iend=neurnombre;
      for ( i=0; i<iend; i++, aval++ )
	updS_all(aval);
      return;
    }
  if (nptr->type && nptr->type->updS_all)
    {
      (*nptr->type->updS_all)(nptr);
      return;
    }
#endif

  /* Optimized procedure */
  if (nptr==NULL) {
    aval=neurbase;
    iend=neurnombre;
  } else {
    aval=nptr;
    iend=1;
  }
  
#ifdef SYNEPSILON
  
  if ((alpha==Flt0) && (decay==Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=s->Sepsilon;
        delta = Fmul( amont->Nval,  Fmul(aval->Ngrad, eps) );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha=0, decay=0 */
  
  else if ((alpha != Flt0) && (decay==0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      preprod=Fmul( unmoinsalpha, aval->Ngrad );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=s->Sepsilon;
        delta = Fadd( Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval,  Fmul(preprod, eps) ));
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha<>0, decay=0 */
  
  else if ((alpha==Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=s->Sepsilon;
        predecay=Fsub(Flt1, Fmul(eps, decay));
        delta = Fmul( amont->Nval,  Fmul(aval->Ngrad, eps) );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha=0, decay<>0 */
  
  else if ((alpha!=Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      preprod=Fmul( unmoinsalpha, aval->Ngrad );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=s->Sepsilon;
        predecay=Fsub(Flt1, Fmul(eps, decay));
        delta = Fadd(Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval,  Fmul(preprod, eps) ));
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha<>0, decay<>0 */
  
#else /* NOT SYNEPSILON */

  if ((alpha==Flt0) && (decay==Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=aval->Nepsilon;
      preprod=Fmul(aval->Ngrad, eps);
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fmul( amont->Nval,  preprod );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
        }
      }
    }  /* end of alpha=0, decay=0 */

  else if ((alpha != Flt0) && (decay==0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=aval->Nepsilon;
      preprod=Fmul(aval->Ngrad, eps);
      preprod=Fmul( unmoinsalpha, preprod );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fadd( Fmul(s->Sdelta, alpha),
                      Fmul( amont->Nval, preprod ) );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
        }
      }
    }  /* end of alpha<>0, decay=0 */

  else if ((alpha==Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=aval->Nepsilon;
      preprod=Fmul(aval->Ngrad, eps);
      predecay=Fsub(Flt1, Fmul(eps, decay));
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fmul( amont->Nval,  preprod );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
        }
      }
    }  /* end of alpha=0, decay<>0 */

  else if ((alpha!=Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=aval->Nepsilon;
      preprod=Fmul(aval->Ngrad, eps);
      preprod=Fmul( unmoinsalpha, preprod );
      predecay=Fsub(Flt1, Fmul(eps, decay));
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fadd( Fmul(s->Sdelta, alpha),
                      Fmul( amont->Nval, preprod ) );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
        }
      }
    }  /* end of alpha<>0, decay<>0 */

#endif /* SYNEPSILON */

}

#ifdef NEWTON

/* This is for the newton version 
   for non-iterative on-line
   newton version */

static void 
updS_new_all(neurone *nptr)
{
  synapse *s;
  neurone *amont, *aval;
  int i,iend;
  flt eps, delta;
  flt preprod, predecay;
  flt unmoinsalpha = Fsub(Flt1, alpha);
  
#ifndef NONEURTYPE
  /* type dependent procedure */
  if (nptr==NULL) 
    {
      aval=neurbase;
      iend=neurnombre;
      for ( i=0; i<iend; i++, aval++ )
	updS_new_all(aval);
      return;
    }
  if (nptr->type && nptr->type->updS_new_all)
    {
      (*nptr->type->updS_new_all)(nptr);
      return;
    }
#endif

  /* Optimized procedure */
  if (nptr==NULL) {
    aval=neurbase;
    iend=neurnombre;
  } else {
    aval=nptr;
    iend=1;
  }

#ifdef SYNEPSILON

  if ((alpha==Flt0) && (decay==Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=Fdiv(s->Sepsilon, Fadd( mu, Fabs(s->Ssigma)));
        delta = Fmul( amont->Nval,  Fmul(aval->Ngrad, eps) );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha=0, decay=0 */
  
  else if ((alpha != Flt0) && (decay==0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      preprod=Fmul( unmoinsalpha, aval->Ngrad );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=Fdiv(s->Sepsilon, Fadd( mu, Fabs(s->Ssigma)));
        delta = Fadd( Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval,  Fmul(preprod, eps) ));
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha<>0, decay=0 */
  
  else if ((alpha==Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=Fdiv(s->Sepsilon, Fadd( mu, Fabs(s->Ssigma)));
        predecay=Fsub(Flt1, Fmul(eps, decay));
        delta = Fmul( amont->Nval,  Fmul(aval->Ngrad, eps) );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha=0, decay<>0 */
  
  else if ((alpha!=Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      preprod=Fmul( unmoinsalpha, aval->Ngrad );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        eps=Fdiv(s->Sepsilon, Fadd( mu, Fabs(s->Ssigma)));
        predecay=Fsub(Flt1, Fmul(eps, decay));
        delta = Fadd(Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval,  Fmul(preprod, eps) ));
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha<>0, decay<>0 */
  
#else /* NOT SYNEPSILON */
  
  if ((alpha==Flt0) && (decay==Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=Fdiv(aval->Nepsilon, Fadd( mu, Fabs(aval->Nsigma)));
      preprod=Fmul(aval->Ngrad, eps);
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fmul( amont->Nval,  preprod );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha=0, decay=0 */
  
  else if ((alpha != Flt0) && (decay==0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=Fdiv(aval->Nepsilon, Fadd( mu, Fabs(aval->Nsigma)));
      preprod=Fmul(aval->Ngrad, eps);
      preprod=Fmul( unmoinsalpha, preprod );
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fadd(Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval, preprod ) );
        s->Sdelta=delta;
        s->Sval = Fadd( s->Sval, delta );
      }
    }
  }  /* end of alpha<>0, decay=0 */
  
  else if ((alpha==Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=Fdiv(aval->Nepsilon, Fadd( mu, Fabs(aval->Nsigma)));
      preprod=Fmul(aval->Ngrad, eps);
      predecay=Fsub(Flt1, Fmul(eps, decay));
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fmul( amont->Nval,  preprod );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha=0, decay<>0 */
  
  else if ((alpha!=Flt0) && (decay!=Flt0)) {
    for ( i=0; i<iend; i++, aval++ ) {
      eps=Fdiv(aval->Nepsilon, Fadd( mu, Fabs(aval->Nsigma)));
      preprod=Fmul(aval->Ngrad, eps);
      preprod=Fmul( unmoinsalpha, preprod );
      predecay=Fsub(Flt1, Fmul(eps, decay));
      for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
        amont=s->Namont;
        delta = Fadd(Fmul(s->Sdelta, alpha),
		     Fmul( amont->Nval, preprod ) );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha<>0, decay<>0 */
  
#endif /* SYNEPSILON */
  
}

#endif /* NEWTON */
#endif /* ITERATIVE */


DX(xupdate_weight)
{
  int i=1;
  ALL_ARGS_EVAL;

  if (i<=arg_number && ISNUMBER(i)) {
    alpha = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_alpha);
    if (p && (p->flags&C_NUMBER))
      alpha = rtoF(p->Number);
    else
      error("alpha","Not a number",p);
    UNLOCK(p);
  }

  if (i<=arg_number && ISNUMBER(i)) {
    decay = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_decay);
    if (p && (p->flags&C_NUMBER))
      decay = rtoF(p->Number);
    else
      error("decay","Not a number",p);
    UNLOCK(p);
  }

  if (arg_number<i)
    updS_all(NULL);
  else
    while (i<=arg_number) {
      mapneur(ALIST(i),updS_all);
      i++;
    }
  return NIL;
}

#ifdef NEWTON
DX(xupdate_w_newton)
{
  int i=1;
  ALL_ARGS_EVAL;

  if (i<=arg_number && ISNUMBER(i)) {
    alpha = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_alpha);
    if (p && (p->flags&C_NUMBER))
      alpha = rtoF(p->Number);
    else
      error("alpha","Not a number",p);
    UNLOCK(p);
  }

  if (i<=arg_number && ISNUMBER(i)) {
    decay = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_decay);
    if (p && (p->flags&C_NUMBER))
      decay = rtoF(p->Number);
    else
      error("decay","Not a number",p);
    UNLOCK(p);
  }

  if (i<=arg_number && ISNUMBER(i)) {
    mu = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_mu);
    if (p && (p->flags&C_NUMBER))
      mu = rtoF(p->Number);
    else
      error("mu","Not a number",p);
    UNLOCK(p);
  }

  if (arg_number<i)
    updS_new_all(NULL);
  else
    while (i<=arg_number) {
      mapneur(ALIST(i),updS_new_all);
      i++;
    }
  return NIL;
}
#endif





#ifdef ITERATIVE

/* function accessible 
   only in ITERATIVE mode */

DX(xclear_acc)
{
  ARG_NUMBER(0);
  clear_acc(NULL);
  return NIL;
}

DX(xupdate_acc)
{
  if (arg_number==0)
    updS_acc(NULL);
  else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    mapneur(ALIST(1),updS_acc);
  }
  return NIL;
}

#ifdef NEWTON

DX(xclear_hess)
{
  ARG_NUMBER(0);
  clear_hess(NULL);
  return NIL;
}

DX(xupdate_hess)
{
  if (arg_number==0) {
    updS_hess(NULL);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    mapneur(ALIST(1),updS_hess);
  }
  return NIL;
}

DX(xhessian_scale)
{
  ALL_ARGS_EVAL;
  if (arg_number==0) {
    at *p = var_get(var_mu);
    if (p && (p->flags&C_NUMBER))
      mu = rtoF(p->Number);
    else
      error("mu","not a number",p);
    UNLOCK(p);
  } else {
    ARG_NUMBER(1);
    mu = AFLT(1);
  }
  Hess_acc(NULL);
  return NIL;
}

#endif /* NEWTON */


DX(xupdate_delta)
{
  int i=1;
  ALL_ARGS_EVAL;

  if (i<=arg_number && ISNUMBER(i)) {
    alpha = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_alpha);
    if (p && (p->flags&C_NUMBER))
      alpha = rtoF(p->Number);
    else
      error("alpha","Not a number",p);
    UNLOCK(p);
  }
  if (arg_number<i)
    updS_delta(NULL);
  else
    error(NIL,"Too many arguments",NIL);
  return NIL;
}

/* mise a jour des poids sans mise a jour des acc et delta */
DX(xupdate_wghtonly)
{
  int i=1;
  ALL_ARGS_EVAL;

  if (i<=arg_number && ISNUMBER(i)) {
    decay = AFLT(i);
    i++;
  } else {
    at *p = var_get(var_decay);
    if (p && (p->flags&C_NUMBER))
      decay = rtoF(p->Number);
    else
      error("decay","Not a number",p);
    UNLOCK(p);
  }
  if(arg_number<i)
    updS_val(NULL);
  else 
    error(NIL,"Too many arguments",NIL);
  return NIL;
}



/* Mise a jour des poids a la SCG */

DX(xupdate_wghtonly_conjgrad)
{
  int i,iend;
  weight *wei;
  flt temp;
  flt curvature;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  curvature = AFLT(1);
  if (curvature <= Fzero)
    error(NIL,"Curvature must be positive",APOINTER(1));

  wei = weightbase;
  iend = weightnombre;
  Fclr(temp);
  for (i=0; i<iend; i++, wei++)
    temp = Fadd( temp, Fmul( wei->Wacc, wei->Wdelta ));
  temp = Fdiv(temp,curvature);
  
  wei = weightbase;
  iend = weightnombre;
  for (i=0; i<iend; i++, wei++)
    wei->Wval = Fadd(wei->Wval, Fmul(temp, wei->Wdelta));
  return NIL;
}


#endif /* ITERATIVE */




/* --------- INITIALISATION CODE --------- */

void 
init_gbp(void)
{
  dx_define("update-weighted-sum",xupdate_weighted_sum);
  dx_define("update-state-only",xupdate_state_only);
  dx_define("update-state",xupdate_state);
  dx_define("update-back-sum",xupdate_back_sum);
  dx_define("update-gradient-only",xupdate_gradient_only);
  dx_define("update-gradient",xupdate_gradient);
  dx_define("update-deltastate",xupdate_deltastate);
  dx_define("init-grad-lms",xinit_grad_lms);
  dx_define("init-grad-thlms",xinit_grad_thlms);
#ifdef NEWTON
  dx_define("update-ggradient",xupdate_ggradient);
  dx_define("update-lmggradient",xupdate_lmggradient);
  dx_define("update-w-newton",xupdate_w_newton);
#endif
  dx_define("update-weight",xupdate_weight);
#ifdef ITERATIVE
  dx_define("clear-acc",xclear_acc);
  dx_define("update-acc",xupdate_acc);
#ifdef NEWTON
  dx_define("clear-hess",xclear_hess);
  dx_define("update-hess",xupdate_hess);
  dx_define("hessian-scale",xhessian_scale);
#endif
  dx_define("update-delta",xupdate_delta);
  dx_define("update-wghtonly",xupdate_wghtonly);
  dx_define("update-wghtonly-conjgrad",xupdate_wghtonly_conjgrad);
#endif
}
