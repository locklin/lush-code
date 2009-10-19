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
 * $Id: defn.h,v 1.4 2005/08/04 18:20:41 leonb Exp $
 **********************************************************************/

/******************************************************************
 *                                                                       
 *       defn.h: network definitions         LYB 90
 *                                                                       
 ******************************************************************/

#ifndef DEFN_H

#include "header.h"



/* ------------- NETWORK.H ------------------ */

/******** standard neuron structure *********/

typedef struct neurone
{
  flt             Nval, Nsum;
#ifndef SYNEPSILON
  flt             Nepsilon;
#endif
  flt             Ngrad, Nbacksum;
#ifdef DESIRED
  flt             Ndesired;
  flt             Nfreedom;
#endif
#ifdef NEWTON
  flt             Nggrad, Nsqbacksum;
#ifndef SYNEPSILON
  flt             Nsigma;
#endif
#endif
#ifndef NOSPARE
  flt             Nspare1, Nspare2, Nspare3;
#endif
  struct synapse  *FSaval, *FSamont;      /* listes de synapses   */
#ifndef NONEURTYPE
  struct neurtype *type;  /* type (optionnel) */
  void *parms;            /* parameters (optionnel) */
#endif
} neurone;

extern neurone *neurbase, **neuraddress;
extern int neurnombre,neurmax;


/******** standard weight structure *********/

#ifdef ITERATIVE
typedef struct weight
{
  flt Wval, Wdelta, Wcounter, Wacc;
#ifdef NEWTON
  flt Whess;
#endif
} weight;

extern weight *weightbase;
extern int weightnombre, weightmax;
#endif


/******** standard synapse structure *********/

typedef struct synapse
{
#ifdef SYNEPSILON
  flt Sepsilon; /* one epsilon field per synapse */
#ifdef NEWTON
  flt Ssigma;   /* & one second derivative per synapse */
#endif
#endif
#ifndef ITERATIVE
  flt Sval, Sdelta;
#else
  weight *w;
#endif
  struct neurone  *Namont, *Naval;        /* pre & post synaptic neurons */
  struct synapse  *NSamont, *NSaval;      /* synapse's listess   */
} synapse;

extern synapse *synbase;
extern int synnombre,synmax;


/* iterative hook */
/* CAUTION: always refer to Sxxx fields using ...->Sxxx */

#ifdef ITERATIVE
#define Sval w->Wval
#define Sdelta w->Wdelta
#define Scounter w->Wcounter
#define Sacc w->Wacc
#define Shess w->Whess
#endif


/******** neuron type structure *************/

#ifndef NONEURTYPE

typedef struct neurtype {
  /* housekeeping */
  char *name;
  void (*detach_parms)(neurone *);
  /* forward prop */
  void (*updN_sum)(neurone *);
  void (*updN_val)(neurone *);
  /* backward prop */
  synapse *(*updN_backsum_term)(neurone *, synapse *);
  void (*updN_grad)(neurone *);
  /* perturbations */
  void (*updN_deltastate)(neurone *);
  /* second order */
#ifdef NEWTON
  synapse *(*updN_sqbacksum_term)(neurone *, synapse *);
  void (*updN_ggrad)(neurone *);
  void (*updN_lmggrad)(neurone *);
  void (*updN_sigma)(neurone *);
#endif
  /* weight update */
#ifdef ITERATIVE
  void (*updS_acc)(neurone *);
#else
  void (*updS_all)(neurone *);
#ifdef NEWTON
  void (*updS_new_all)(neurone *);
#endif
#endif
} neurtype;

#endif


/* ------------ NLF STRUCTURE ------------ */


struct nlf {
  short used;
  short type;
  flt (*f)(struct nlf*, flt);
  at *atf;
  union {
    struct {
      flt A,B,C,D;
      flt P1,P2;
    } parms;
    struct {
      int size;
      flt *x,*y, *y2;
    } splin;
  } u;
};


extern class nlf_class;
extern struct nlf *nlf,*dnlf,*ddnlf;

#define NLFP(p)  ((p) && ((p)->flags&C_EXTERN) && ((p)->Class==&nlf_class))

#define CALLNLF(x)  (*((nlf)->f))(nlf,(x))
#define CALLDNLF(x)  (*((dnlf)->f))(dnlf,(x))
#define CALLDDNLF(x)  (*((ddnlf)->f))(ddnlf,(x))

#define GETNLF(i) { at *p=APOINTER(i);\
  ifn(p && (p->flags&C_EXTERN) && (p->Class==&nlf_class))\
    error(NIL,"not a nlf",p);\
  nlf = p->Object;}

#define GETDNLF(i) { at *p=APOINTER(i);\
  ifn(p && (p->flags&C_EXTERN) && (p->Class==&nlf_class))\
    error(NIL,"not a nlf",p);\
  dnlf = p->Object;}

#define GETDDNLF(i) { at *p=APOINTER(i);\
  ifn(p && (p->flags&C_EXTERN) && (p->Class==&nlf_class))\
    error(NIL,"not a nlf",p);\
  ddnlf = p->Object;}


/* ------------- SN2SN1.H ------------------ */

#define Fclr(x)   ((x)=Fzero)
#define fptoF(x)  dtoF(x)
#define Ftofp(x)  Ftod(x)
#define FtoInt(x) (int)Ftod(x)
#define InttoF(x) dtoF((double)(x))
#define Flt0      Fzero
#define FltHalf   Fhalf
#define Flt1      Fone
#define Flt2      Ftwo

#ifndef Fadd
#define Fadd(x,y) ((x)+(y))
#endif
#ifndef Fsub
#define Fsub(x,y) ((x)-(y))
#endif
#ifndef Fmul
#define Fmul(x,y) ((x)*(y))
#endif
#ifndef Fdiv
#define Fdiv(x,y) ((x)/(y))
#endif

extern flt theta, alpha, decay;
#ifdef NEWTON
extern flt mygamma, mu;
#endif
extern at *var_Nnum, *var_Snum, *var_Nmax, *var_Smax, *var_age;
#ifdef ITERATIVE
extern at *var_Wnum, *var_Wmax;
extern at *w_matrix, *w_matrix_var;
#endif

/* compatibility */

extern at *var_alpha, *var_decay, *var_theta;
extern at *var_nlf, *var_dnlf, *var_ddnlf;
#ifdef NEWTON
extern at *var_gamma, *var_mu;
#endif


/* ------------- FIELD NUMBERS ---------------- */

#define F_VAL          ((int)1)
#define F_SUM          ((int)2)
#define F_GRAD         ((int)3)
#define F_BACKSUM      ((int)4)
#define F_EPSILON      ((int)5)
#define F_DESIRED      ((int)6)
#define F_FREEDOM      ((int)7)
#define F_GGRAD        ((int)8)
#define F_SQBACKSUM    ((int)9)
#define F_SIGMA       ((int)10)
#define F_SPARE1      ((int)11)
#define F_SPARE2      ((int)12)
#define F_SPARE3      ((int)13)

#define FS_VAL         ((int)1)
#define FS_DELTA       ((int)2)
#define FS_ACC         ((int)3)
#define FS_EPSILON     ((int)4)
#define FS_SIGMA       ((int)5)
#define FS_HESS	       ((int)6)


/* ------------- PROTOTYPES ---------------- */

void growage(void);
int readage(void);
synapse *searchsynapse(int amont, int aval);
void mapneur(register at *q, void (*f) (neurone*));
void map2neur(register at *q1, register at *q2, void (*f)(neurone*, neurone*));
synapse *connection(int amont, int aval, float val, float eps);
synapse *dup_connection(int, int, int, int, int Flag, float val, float eps);
#ifndef NONEURTYPE
void set_neurtype(neurone *n, neurtype *newtype, void *newparms);
#endif

#define DEFN_H
#endif
