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
 *  This file is derived from SN-2.8
 *    Copyright (C) 1987-1999 Neuristique s.a.
 ***********************************************************************/

/***********************************************************************
 * $Id: sn2sn1.c,v 1.3 2005-08-04 18:20:42 leonb Exp $
 **********************************************************************/


#include "defn.h"


/* ============= GLOBALS ============= */

neurone *neurbase = NIL;
int neurnombre = 0, neurmax = 0;
neurone **neuraddress = NIL;

synapse *synbase = NIL;
int synnombre = 0, synmax = 0;

#ifdef ITERATIVE
weight *weightbase = NIL;
int weightnombre = 0;
int weightmax = 0;
#endif

flt theta, alpha, decay;
#ifdef NEWTON
flt mygamma, mu;
#endif

/* ============= SEQUENCER ============= */


/* mapneur(p,f)
   map2neur(p1,p2,f)
    Call the given function with one or two pointer(s) as argument. This(These)
    pointer(s) give the address of the neurons whose numbers are stored in the
    listes p, p1 or p2.

    Recursively scans sublistes.
*/

void 
mapneur(at *q, void (*f)(neurone *))
{
  at *ql;
  int i;
  
  while(q) {
    if (! (q->flags & C_CONS))
      error(NIL,"bad neuron numbers list",NIL);
    else if ((ql = q->Car)) {
      if (ql->flags & C_NUMBER) {
	i = (int)(ql->Number);
	if ( i >= 0  &&  i<neurnombre )
	  (*f)( neuraddress [i] );
	else
	  error(NIL,"bad neuron number",ql);
      } else if ( ql->flags & C_CONS )
	mapneur (ql,f);
      else
	error(NIL,"bad neuron number",ql);
    }
    q  = q->Cdr;
  };
}

void 
map2neur(at *q1, at *q2, void (*f) (neurone *, neurone *))
{
  at *ql1,*ql2;
  int i,j;
  
  while (q1 && q2) {
    if ( (!(q1->flags & C_CONS)) || (!(q2->flags & C_CONS)) ) {
      error(NIL,"bad neuron numbers list",NIL);
    } else if ((ql1 = q1->Car) && (ql2 = q2->Car) ) {
      if ( (ql1->flags & C_NUMBER) && (ql2->flags & C_NUMBER) ) {
	i = (int)(ql1->Number);
	j = (int)(ql2->Number);
	if ( i<0  ||  i>=neurnombre )
	  error(NIL,"bad neuron number",ql1);
	if ( j<0  ||  j>=neurnombre )
	  error(NIL,"bad neuron number",ql2);
	(*f)( neuraddress[i], neuraddress[j] );
	
      } else if ( (ql1->flags & C_CONS) && (ql2->flags & C_CONS) )
	map2neur(ql1,ql2,f);
      else
	error(NIL,"listes doesn't have the same structure",NIL);
    }
    q1 = q1->Cdr;
    q2 = q2->Cdr;
  };
  if ( q1 || q2)
    error(NIL,"listes doesn't have the same length",NIL);
}





/* ================ GET_PATTERN, GLOB VARS ========== */

at *var_Nnum, *var_Snum, *var_Nmax, *var_Smax, *var_age;
#ifdef ITERATIVE
at *var_Wnum, *var_Wmax;
at *w_matrix, *w_matrix_var;
#endif
/* compatibility */
at *var_alpha, *var_decay, *var_theta;
at *var_nlf, *var_dnlf, *var_ddnlf;
#ifdef NEWTON
at *var_gamma, *var_mu;
#endif


/********* reads inside Lisp variables ***********/

void 
growage(void)
{
        real i;
        at *q;
	
        q = var_get(var_age);
        ifn (q->flags & C_NUMBER)
	  error("age","should contain an integer",NIL);
        i = q->Number+1.0;
        var_set(var_age,NEW_NUMBER(i));
        UNLOCK(q);
}

int 
readage(void)
{
        at *q;
	
        q = var_get(var_age);
        ifn (q->flags & C_NUMBER)
	  error("age","should contain an integer",NIL);
        return (int)(q->Number);
}

/************ get_patterns ***********/


DX(xget_pattern_2)
{
  at *mat;
  struct index *ind;
  struct idx id;
  int type;
  flt (*getf)(gptr,int);
  gptr base;
  int nlist = 1;
  at *nptr = 0;

  ALL_ARGS_EVAL;
  if (arg_number<=1)
    ARG_NUMBER(-1);
  mat  = APOINTER(1);
  if (! matrixp(mat))
    error(NIL,"not a matrix", mat);
  ind = mat->Object;
  
  type = ind->st->srg.type;
  getf = storage_type_getf[type];
  index_read_idx(ind,&id);
  base = IDX_DATA_PTR(&id);
  begin_idx_aloop1(&id, off) 
    {
      int nnum;
      if (! nptr)
        {
          nlist += 1;
          nptr = APOINTER(nlist);
        }
      if (! CONSP(nptr) && ! NUMBERP(nptr->Car))
        error(NIL,"bad neuron number list",nptr);
      nnum = (int) (nptr->Car->Number);
      nptr = nptr->Cdr;
      if (nnum < 0 || nnum >= neurnombre)
        error(NIL,"bad neuron number", nptr->Car);
      neuraddress[nnum]->Nval = (*getf)(base, off);
    } 
  end_idx_aloop1(&id, off);
  index_rls_idx(ind,&id);

  if (nptr && nlist < arg_number)
    error(NIL,"Neuron lists do not match matrix size", mat);
  
  return NIL;
}


/********** initialisation ***********/

void 
init_sn2sn1(void)
{

/*  Variables Definitions
 *  Those vars are left unchaged  for preserving
 *  backwards compatibility...
 *  Future vars should be accessed via special functions
 */
  var_Nnum = var_define("nnum");
  var_SET(var_Nnum,NEW_NUMBER(0));
  var_lock(var_Nnum);
  var_Nmax = var_define("nmax");
  var_SET(var_Nmax,NEW_NUMBER(0));
  var_lock(var_Nmax);
  var_Smax = var_define("smax");
  var_SET(var_Smax,NEW_NUMBER(0));
  var_lock(var_Smax);
  var_Snum = var_define("snum");
  var_SET(var_Snum,NEW_NUMBER(0));
  var_lock(var_Snum);
  var_age = var_define("age");
  var_set(var_age,NEW_NUMBER(0));
#ifdef ITERATIVE
  var_Wnum = var_define("wnum");
  var_SET(var_Wnum,NEW_NUMBER(0));
  var_lock(var_Wnum);
  var_Wmax = var_define("wmax");
  var_SET(var_Wmax,NEW_NUMBER(0));
  var_lock(var_Wmax);
#endif

/*
 * The theta, gamma, ,mu alpha, and decay global vars are only kept
 *  for compatibility, as well as the nlf, dnlf and ddnlf functions.
 */
  var_alpha = var_define("alpha");
  var_set(var_alpha,NEW_NUMBER(0.0));
  var_decay = var_define("decay");
  var_set(var_decay,NEW_NUMBER(0.0));
  var_theta = var_define("theta");
  var_set(var_theta,NEW_NUMBER(0.0));
#ifdef NEWTON
  var_gamma = var_define("gamma");
  var_set(var_gamma,NEW_NUMBER(0.05));
  var_mu = var_define("mu");
  var_set(var_mu,NEW_NUMBER(0.05));
#endif
  var_nlf=var_define("nlf");
  var_dnlf=var_define("dnlf");
  var_ddnlf=var_define("ddnlf");
  dx_define("get-pattern-2",xget_pattern_2);
}

