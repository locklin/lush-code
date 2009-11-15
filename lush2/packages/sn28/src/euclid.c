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
 * $Id: euclid.c,v 1.2 2003/03/18 19:40:30 leonb Exp $
 **********************************************************************/

/******************************************************************
 *                                                                       
 *       euclid.c: functions for RBF,LVQ,TMAP           LYB 90     
 *                                                                       
 ******************************************************************/


#include "defn.h"


extern void updS_acc(neurone *nptr);
extern void updN_val(neurone *n);
extern void updN_grad(neurone *n);
#ifdef NEWTON
extern void updN_ggrad(neurone *n);
extern void updN_lmggrad(neurone *n);
#endif
#ifdef ITERATIVE
extern void clear_acc(weight *nptr);
extern void updS_val(weight *nptr);
extern void updS_delta(weight *nptr);
#endif
#ifdef NEWTON
#ifdef ITERATIVE
extern void clear_hess(weight *nptr);
extern void updS_hess(neurone *nptr);
extern void Hess_acc(weight *nptr);
#endif
#endif



/********* update_nn_weightedsum **********/

/*
 *  (update-nn-sum [theta] ...layer... )  
 */



static void 
updN_nn_sum(neurone *n)
{
  synapse *s;
  flt sum, dist;
  Fclr(sum);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) 
    {
      dist = Fsub( s->Sval, s->Namont->Nval );
      sum  = Fadd( sum, Fmul(dist,dist) );
    }
  if (theta != Flt0) 
    {
      dist=Fmul(Fgauss(),theta);
      sum=Fadd( sum, dist );
    }
  n->Nsum=sum;
}
DX(xupdate_nn_sum)
{
  int i=1;
  
  ALL_ARGS_EVAL;
  if (i<=arg_number && ISNUMBER(i)) {
    theta = AFLT(1);
    i++;
  } else
    theta = Fzero;

  while (i<=arg_number) {
    mapneur(ALIST(i),updN_nn_sum);
    i++;
  }
  return t();
}

/********* update_nn_state **********/

/*
 *  (update-nn-state [theta] nlf ...layer...)  
 */

DX(xupdate_nn_state)
{
  int i=1;
  
  ALL_ARGS_EVAL;
  if (i<=arg_number && ISNUMBER(i)) {
    theta = AFLT(1);
    i++;
  } else
    theta = Fzero;
  GETNLF(i);
  while(++i<=arg_number) {
    mapneur(ALIST(i),updN_nn_sum);
    mapneur(APOINTER(i),updN_val);
  }
  return t();
}




/********* update-nn-back-sum **********/

/*
 *  (update-nn-back-sum ...layer... )  
 */


static void 
updN_nn_backsum(neurone *n)
{
  synapse *s;
  flt sum, val, prod;
  
  Fclr(sum);
  val = n->Nsum;
  for ( s=n->FSaval; s!=NIL; s=s->NSamont ) {
    prod = Fmul( Fsub(val, s->Sval), s->Naval->Ngrad );
    sum  = Fadd( sum, prod );
  }
  n->Nbacksum=Fmul(Ftwo,sum);
}

DX(xupdate_nn_back_sum)
{
  int i;
  
  ALL_ARGS_EVAL;
  for (i=1;i<=arg_number;i++)
    mapneur(ALIST(i),updN_nn_backsum);
  return t();
}





/********* update_nn_gradient **********/

/*
 *  (update-nn-gradient dnlf ...layer... )  
 */

     
DX(xupdate_nn_gradient)
{
  int i;
  
  ALL_ARGS_EVAL;
  if (arg_number<1)
    ARG_NUMBER(-1);
  GETDNLF(1);
  for (i=2;i<=arg_number;i++) {
    mapneur(ALIST(i),updN_nn_backsum);
    mapneur(ALIST(i),updN_grad);
  }
  return t();
}



/********* update_nn_deltastate ******/

/* 
 * (update-nn-deltastate [<nlf>] <...layers...>)
 */


static void 
updN_nn_deltastate(neurone *n)
{
  synapse *s;
  flt sum,diff1,diff2;
  
  Fclr(sum);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    diff1 = Fsub( s->Sdelta, s->Namont->Ngrad );
    diff2 = Fsub( s->Sval, s->Namont->Nval );
    sum  = Fadd( sum, Fmul(Fmul(Ftwo,diff1),diff2));
  };
  n->Ngrad = Fmul(sum, CALLDNLF(n->Nsum) );
}


DX(xupdate_nn_deltastate)
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
    mapneur(ALIST(i),updN_nn_deltastate);
    i++;
  }
  return t();
}





/********* update_nn_ggradient **********/

/*
 *  (update-nn-ggradient dnlf ddnlf gamma ...layers...)
 *  (update-nn-lmggradient dnlf gamma ...layers...)
 */

#ifdef NEWTON

/* compute the backward sum of the
   2nd-derivative weighted by the
   squared weights */

static void 
updN_nn_sqbacksum(neurone *n)
{
  synapse *s;
  flt sum , prod, val;
  
  Fclr(sum);
  val = n->Nsum;
  if (n->FSaval != NIL) {
    for ( s=n->FSaval; s!=NIL; s=s->NSamont ) {
      flt x = Fsub( val, s->Sval);
      prod = Fmul( Fmul(x,x), s->Naval->Nggrad );
      sum  = Fadd( sum, Fsub( Fmul(Ftwo,prod), s->Naval->Ngrad) );
    }
    n->Nsqbacksum=Fmul( Ftwo, sum);
  }
}

#ifndef SYNEPSILON
static flt 
mean_sqminusw_input(neurone *n)
{
  synapse *s;
  flt sum, nbre;
  
  Fclr(sum);
  Fclr(nbre);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval ) {
    flt x = Fsub(s->Sval,s->Namont->Nval);
    sum = Fadd( sum, Fmul(x,x) );
    nbre = Fadd(nbre,Flt1);
  };
  if (nbre>Flt0) 
    return Fdiv(sum,nbre);
  else
    return Flt0;
}
#endif

/* computes the average curvature for a cell or the connection to a cell */

static void 
updN_nn_sigma(neurone *n)
{
  flt gamma2ggradtwice, gamma1gradonce, oneminusgamma, x;
  synapse *s;
  
  oneminusgamma    = Fsub(Flt1, mygamma);
  gamma2ggradtwice = Fmul(Fmul(Ftwo,mygamma), n->Nggrad );
  gamma1gradonce   = Fmul(mygamma, n->Ngrad);
			  
#ifdef SYNEPSILON
  for ( s=n->FSamont; s!=NIL; s=s->NSaval) {
    x = Fsub(s->Sval,s->Namont->Nval);
    x = Fsub(Fmul(gamma2ggradtwice, Fmul(x,x)), gamma1gradonce);
    s->Ssigma = Fadd(Fmul(oneminusgamma, s->Ssigma),x );
  }
#else
  x = Fsub(Fmul(gamma2ggradtwice, mean_sqminusw_input(n)),
	   gamma1gradonce );
  n->Nsigma = Fadd(Fmul( oneminusgamma, n->Nsigma),x);
#endif
}


DX(xupdate_nn_ggradient)
{
  int i;
  
  ALL_ARGS_EVAL;
  if (arg_number<4)
    ARG_NUMBER(-1);
  GETDNLF(1);
  GETDDNLF(2);
  mygamma = AFLT(3);
  for(i=4;i<=arg_number;i++) {
    mapneur(ALIST(i),updN_nn_sqbacksum);
    mapneur(APOINTER(i),updN_ggrad);
    mapneur(APOINTER(i),updN_nn_sigma);
  }
  return NIL;
}

DX(xupdate_nn_lmggradient)
{
  int i;
  
  ALL_ARGS_EVAL;
  if (arg_number<3)
    ARG_NUMBER(-1);
  GETDNLF(1);
  mygamma = AFLT(2);
  for(i=3;i<=arg_number;i++) {
    mapneur(ALIST(i),updN_nn_sqbacksum);
    mapneur(APOINTER(i),updN_lmggrad);
    mapneur(APOINTER(i),updN_nn_sigma);
  }
  return NIL;
}

#endif /* NEWTON */



/********* find-winner, find-loser **********/

/*
 *  (find-winer layer)
 *  (find-loser layer)
 */


static neurone *winner;

static void 
findwinner(neurone *n)
{

  ifn (winner && Fsgn(Fsub(winner->Nval,n->Nval))>=0)
    winner = n;
}

DX(xfind_winner)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  winner = NIL;
  mapneur(ACONS(1),findwinner);
  return NEW_NUMBER(winner-neurbase);
}

static void 
findloser(neurone *n)
{

  ifn (winner && Fsgn(Fsub(winner->Nval,n->Nval))<=0)
    winner = n;
}

DX(xfind_loser)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  winner = NIL;
  mapneur(ACONS(1),findloser);
  return NEW_NUMBER(winner-neurbase);
}





/********* update_nn_weight **********/

/*
 *  (update-nn-weight alpha decay [...layers...])
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

static void 
updS_nn_acc(neurone *nptr)
{
  int iend;
  synapse *s;
  neurone *amont, *aval;
  int i;
#ifndef SYNEPSILON
  flt preprod;
#endif
  flt eps;
  if (nptr==NULL) {
    aval=neurbase;
    iend=neurnombre; }
  else {
    aval=nptr;
    iend=1; }
#ifdef SYNEPSILON
  for ( i=0; i<iend; i++, aval++ ) {
    for ( s=aval->FSamont; s!=NIL; s=s->NSaval ) {
      eps=s->Sepsilon;
      amont=s->Namont;
      s->Sacc = Fadd(s->Sacc,
		     Fmul(Fsub(s->Sval,amont->Nval),
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
                      Fmul(Fsub(s->Sval,amont->Nval),
			   preprod ) );
    }
  }
#endif /* SYNEPSILON */
}

/* updates all the weight
   variables */

static void 
updS_nn_all(neurone *nptr)
{
  clear_acc(NULL);
  updS_nn_acc(nptr);
  updS_delta(NULL);
  updS_val(NULL);
}

#ifdef NEWTON

/* can be used for iterative
   on-line newton version */

static void 
updS_nn_new_all(neurone *nptr)
{
  clear_acc(NULL);
  updS_nn_acc(nptr);
  clear_hess(NULL);
  updS_hess(nptr);
  Hess_acc(NULL);
  updS_delta(NULL);
  updS_val(NULL);
}
#endif /* NEWTON */


#else /* NOT ITERATIVE */

static void 
updS_nn_all(neurone *nptr)
{
  synapse *s;
  neurone *amont, *aval;
  int i,iend;
  flt eps, delta;
  flt preprod, predecay;
  flt unmoinsalpha;

  unmoinsalpha=Fsub(Flt1, alpha);

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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     Fmul(aval->Ngrad, eps) );
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
		      Fmul(Fsub(s->Sval,amont->Nval),
			   Fmul(preprod, eps) ));
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     Fmul(aval->Ngrad, eps) );
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
		     Fmul(Fsub(s->Sval,amont->Nval),
			  Fmul(preprod, eps) ));
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     preprod );
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
                      Fmul(Fsub(s->Sval,amont->Nval),
			   preprod ) );
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     preprod );
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
                      Fmul(Fsub(s->Sval,amont->Nval),
			   preprod ) );
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
updS_nn_new_all(neurone *nptr)
{
  synapse *s;
  neurone *amont, *aval;
  int i,iend;
  flt eps, delta;
  flt preprod, predecay;
  flt unmoinsalpha;
  
  unmoinsalpha=Fsub(Flt1, alpha);
  
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     Fmul(aval->Ngrad, eps) );
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
        delta = Fadd(Fmul(s->Sdelta, alpha),
		     Fmul(Fsub(s->Sval,amont->Nval),
			  Fmul(preprod, eps) ));
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     Fmul(aval->Ngrad, eps) );
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
		     Fmul(Fsub(s->Sval,amont->Nval),
			  Fmul(preprod, eps) ));
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     preprod );
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
		     Fmul(Fsub(s->Sval,amont->Nval),
			  preprod ) );
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
        delta = Fmul(Fsub(s->Sval,amont->Nval),
		     preprod );
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
		     Fmul(Fsub(s->Sval,amont->Nval),
			  preprod ) );
        s->Sdelta=delta;
        s->Sval = Fadd( Fmul(predecay, s->Sval), delta );
      }
    }
  }  /* end of alpha<>0, decay<>0 */
  
#endif /* SYNEPSILON */
  
}

#endif /* NEWTON */
#endif /* ITERATIVE */


DX(xupdate_nn_weight)
{
  int i;
  alpha = decay = Fzero;

  ALL_ARGS_EVAL;
  if (arg_number<2)
    ARG_NUMBER(-1);

  alpha = AFLT(1);
  decay = AFLT(2);

  if (arg_number==2) {
    updS_nn_all(NULL);
  } else
    for(i=3;i<=arg_number;i++) {
      mapneur(ALIST(i),updS_nn_all);
      i++;
    }
  return NIL;
}

#ifdef NEWTON
DX(xupdate_nn_w_newton)
{
  int i;
  mu = alpha = decay = Fzero;

  ALL_ARGS_EVAL;
  if (arg_number<3)
    ARG_NUMBER(-1);

  alpha = AFLT(1);
  decay = AFLT(2);
  mu = AFLT(3);

  if (arg_number==3) {
    updS_nn_new_all(NULL);
  } else
    for(i=4;i<=arg_number;i++) {
      mapneur(ALIST(i),updS_nn_new_all);
      i++;
    }
  return NIL;
}
#endif





#ifdef ITERATIVE

DX(xupdate_nn_acc)
{
  if (arg_number==0) {
    updS_acc(NULL);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    mapneur(ALIST(1),updS_nn_acc);
  }
  return NIL;
}

#endif /* ITERATIVE */




/* --------- INITIALISATION CODE --------- */

void 
init_euclid(void)
{
  dx_define("update-nn-sum",xupdate_nn_sum);
  dx_define("update-nn-state",xupdate_nn_state);
  dx_define("update-nn-back-sum",xupdate_nn_back_sum);
  dx_define("update-nn-gradient",xupdate_nn_gradient);
  dx_define("update-nn-deltastate",xupdate_nn_deltastate);
#ifdef NEWTON
  dx_define("update-nn-ggradient",xupdate_nn_ggradient);
  dx_define("update-nn-lmggradient",xupdate_nn_lmggradient);
  dx_define("update-nn-w-newton",xupdate_nn_w_newton);
#endif
  dx_define("find-winner",xfind_winner);
  dx_define("find-loser",xfind_loser);
  dx_define("update-nn-weight",xupdate_nn_weight);
#ifdef ITERATIVE
  dx_define("update-nn-acc",xupdate_nn_acc);
#endif
}


