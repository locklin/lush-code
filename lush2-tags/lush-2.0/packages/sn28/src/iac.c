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
 * $Id: iac.c,v 1.3 2006/03/29 19:44:53 leonb Exp $
 **********************************************************************/



/******************************************************************
*                                                                       
*       iac.c: functions for IAC and ART models           LYB+JB 90     
*                                                                       
******************************************************************/

#include "defn.h"


/********* update_excitation **********/

/*
 *  (update-excitation ...layer...)  
 */

#ifndef NOSPARE

static void 
updN_excitation(neurone *n)
{
  synapse *s;
  flt sum,prod;
  
  Fclr(sum);
  for (s=n->FSamont; s!=NIL; s=s->NSaval)
    if ( Fsgn(s->Sval) > 0 ) {
      prod = Fmul( s->Sval, s->Namont->Nval );
      sum  = Fadd( sum,prod );
    };
  n->Nspare1=sum;
}
DX(xupdate_excitation)
{
        int i;

        ALL_ARGS_EVAL;
        for (i=1;i<=arg_number;i++) {
                mapneur(ALIST(i),updN_excitation);
        }
        return NEW_NUMBER(arg_number);
}

#endif






/********* update_inhibition **********/

/*
 *  (update-inhibition ...layer...)  
 */

#ifndef NOSPARE

static void 
updN_inhibition(neurone *n)
{
  synapse *s;
  flt sum,prod;
  
  Fclr(sum);
  for (s=n->FSamont; s!=NIL; s=s->NSaval)
    if ( Fsgn(s->Sval) < 0 ) {
      prod = Fmul( s->Sval, s->Namont->Nval );
      sum  = Fadd( sum,prod );
    };
  n->Nspare2=sum;
}
DX(xupdate_inhibition)
{
        int i;

        ALL_ARGS_EVAL;
        for (i=1;i<=arg_number;i++) {
                mapneur(ALIST(i),updN_inhibition);
        }
        return NEW_NUMBER(arg_number);
}

#endif










/********* update_IAC_activation **********/

/*
 *  (update-IAC-activation layer alpha beta gamma min max rest decay)  
 */

#ifndef NOSPARE
# define alpha iac_alpha
# define decay iac_decay
static flt alpha,beta,iacgamma,max,min,rest,decay;

static void
updN_IAC_activation(neurone *n)
{
  flt net;

  net = Fadd(Fmul(n->Nval, iacgamma),
	     Fadd(Fmul(n->Nspare1,alpha), Fmul(n->Nspare2,beta) ));
  if (Fsgn(net)>0) {
    n->Nsum = Fadd(n->Nsum,Fsub(Fmul(net,Fsub(max,n->Nsum)),
				Fmul(decay,Fsub(n->Nsum,rest)) ));
  } else {
    n->Nsum = Fadd(n->Nsum,Fsub(Fmul(net,Fsub(n->Nsum,min)),
				Fmul(decay,Fsub(n->Nsum,rest)) ));
  }
}
DX(xupdate_IAC_activation)
{
  ARG_NUMBER(8);
  ALL_ARGS_EVAL;
  alpha = AFLT(2);
  beta  = AFLT(3);
  iacgamma = AFLT(4);
  min   = AFLT(5);
  max   = AFLT(6);
  rest  = AFLT(7);
  decay = AFLT(8);
  mapneur(ALIST(1),updN_IAC_activation);
  return NIL;
}

#endif










/********* update_ART_activation **********/

/*
 *  (update-ART-activation layer alpha beta gamma min max rest decay)  
 */

#ifndef NOSPARE

static void
updN_ART_activation(neurone *n)
{
  flt net1,net2;

  if (Fsgn(n->Nval)>0) {
    net1 = Fadd(Fmul(n->Nval, iacgamma),Fmul(n->Nspare1,alpha));
    net2 = Fmul(n->Nspare2,beta);
  } else {
    net1 = Fmul(n->Nspare1,alpha);
    net2 = Fadd(Fmul(n->Nval, iacgamma),Fmul(n->Nspare2,beta));
  }
  n->Nsum = Fsub(Fadd(n->Nsum, Fadd(Fmul(net1,Fsub(max,n->Nsum)),
				    Fmul(net2,Fsub(n->Nsum,min)))),
		 Fmul(decay,Fsub(n->Nsum,rest)) );
}
DX(xupdate_ART_activation)
{
  ARG_NUMBER(8);
  ALL_ARGS_EVAL;
  alpha = AFLT(2);
  beta  = AFLT(3);
  iacgamma = AFLT(4);
  min   = AFLT(5);
  max   = AFLT(6);
  rest  = AFLT(7);
  decay = AFLT(8);
  mapneur(ALIST(1),updN_ART_activation);
  return NIL;
}

#endif





/* --------- INITIALISATION CODE --------- */

void 
init_iac(void)
{
#ifndef NOSPARE
	 dx_define("update-excitation",xupdate_excitation);
	 dx_define("update-inhibition",xupdate_inhibition);
	 dx_define("update-iac-activation",xupdate_IAC_activation);
	 dx_define("update-art-activation",xupdate_ART_activation);
#endif
}
