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
 * $Id: adaptknn.c,v 1.3 2005/06/03 04:10:09 leonb Exp $
 **********************************************************************/


#include "header.h"
#include "codebook.h"

#ifndef MAXFLOAT
# define MAXFLOAT ((flt)(1e38))
#endif


/**************************************************************************/
/*                                                                        */
/*                 PACKAGE UTILITY : Routines needed in some programs     */
/*                                                                        */
/**************************************************************************/


/* compute square distance between V1 and V2 */

static flt 
dist2(flt *V1, flt *V2, int m)
{
  flt    som2 = Fzero;
  int    i;
  
  for (i=0;i<m;i++)
    som2+=(V1[i]-V2[i])*(V1[i]-V2[i]);
  return(som2);
}



/* compute dist2 between vector v and all codewords */

static void 
vect_dist2(flt *v, struct codebook *cb, flt *vd)
{
  int i;
  for (i=0;i<cb->ncode;i++)
    vd[i]=dist2(v,cb->code[i].word,cb->ndim);
}





/* return the two nearest neighbours */

static void
one_nn(flt *x, struct codebook *cb, int *ind)
{
  flt dmin = MAXFLOAT;
  flt dist;
  int id = -1;
  int i;
  
  for (i=0;i<cb->ncode;i++)
    {
      dist=dist2(x,cb->code[i].word,cb->ndim);
      if (dist < dmin)
	{
	  dmin=dist;
	  id=i;
	}
    }
  *ind = id;
}


/* return the two nearest neighbours */

static void
two_nn(flt *x, struct codebook *cb, int *ind1, int *ind2)
{
  flt dmin1 = MAXFLOAT;
  flt dmin2 = MAXFLOAT;
  flt dist;
  int id1 = -1;
  int id2 = -1;
  int i;
  
  for (i=0;i<cb->ncode;i++)
    {
      dist=dist2(x,cb->code[i].word,cb->ndim);
      
      if (dist < dmin1)
	{
	  dmin2 = dmin1;
	  id2 = id1;
	  dmin1 = dist;
	  id1 = i;
	}
      else if (dist < dmin2)
	{
	  dmin2 = dist;
	  id2 = i;
	}
    }
  *ind1 = id1;
  *ind2 = id2;
}


/* return the nearest neighbour global and in class c */

static void
one_nn_and_a_half(flt *x, struct codebook *cb, int c, int *ind1, int *ind2)
{
  flt dmin1 = MAXFLOAT;
  flt dmin2 = MAXFLOAT;
  flt dist;
  int id1 = -1;
  int id2 = -1;
  int i;
  
  for (i=0;i<cb->ncode;i++)
    {
      dist=dist2(x,cb->code[i].word,cb->ndim);
      
      if (dist < dmin1)
	{
	  dmin1 = dist;
	  id1 = i;
	}
      if (cb->code[i].label==c && dist<dmin2)
	{
	  dmin2 = dist;
	  id2 = i;
	}
    }
  *ind1 = id1;
  *ind2 = id2;
}






/* compute k_means distorsion */

static flt  
distorsion(struct codebook *cb, struct codebook *cbref)
{
  flt    distor = Fzero;
  int    i,ind;
  
  for (i=0;i<cb->ncode;i++)
    {
      one_nn(cb->code[i].word,cbref,&ind);
      distor += dist2(cb->code[i].word,cbref->code[ind].word,cb->ndim);
    }
  distor/=cb->ncode;
  return(distor);
}













/**************************************************************************/
/*                                                                        */
/*                 PACKAGE KNN : Nearest Neighbors routines               */
/*                                                                        */
/**************************************************************************/




/* OK returns the k nearest neighbour ------------------------------------*/

#define SWAP(p,q) \
 { ftmp=x[p]; x[p]=x[q]; x[q]=ftmp; itmp=i[p]; i[p]=i[q]; i[q]=itmp; }


static void 
knnsub(flt *x, int *i, int n, int k)
{
  int lo,hi;
  flt pivot;
  flt ftmp;
  int itmp;
  
  if (n<2)
    return;
  
  SWAP(1,(n>>1));   /* make sure that x[1]<=x[0]<=x[n-1] */
  if (x[1]>x[0])
    SWAP(1,0);
  if (x[0]>x[n-1])
    SWAP(0,n-1);
  if (x[1]>x[0])
    SWAP(1,0);
  pivot = x[0];
  
  lo=1;
  hi=n-1;
  for(;;) {
    do lo++; while (x[lo]<pivot);
    do hi--; while (x[hi]>pivot);
    if (lo>hi) break;
    SWAP(lo,hi);
  }
  SWAP(0,hi);
  
  if (lo>=k)
    knnsub(x,i,hi,k);
  else {
    knnsub(x,i,hi,hi);
    knnsub(x+lo,i+lo,n-lo,k-hi);
  }
}




int *
knn(flt *x, struct codebook *cb, int k, int *res)
{
  if (k==1) 
    {
      one_nn(x,cb,&res[0]);
      return res;
    }
  else if (k==2)
    {
      two_nn(x,cb,&res[0],&res[1]);
      return res;
    }
  else if (k>cb->ncode) 
    {
      error(NIL,"Reference codebook is too small",NIL);
    }
  else
    {
      int i;
      int *idx = 0;
      flt *dst = 0;

      idx=(int*)malloc(cb->ncode*sizeof(int));
      dst=(flt*)malloc(cb->ncode*sizeof(flt));
      ifn(idx && dst) {
	if (idx) free(idx);
	if (dst) free(dst);
	error(NIL,"No memory",NIL);
      }

      for (i=0;i<cb->ncode;i++) {
      	idx[i] = i;
	dst[i] = dist2(x,cb->code[i].word,cb->ndim);;
      }
      knnsub(dst,idx,cb->ncode,k);
      for (i=0; i<k; i++) 
	res[i] = idx[i];
      if (idx) free(idx);
      if (dst) free(dst);
      return res;
    }
}


/* OK classify vector x using KNN rule in codebook cb --------------------*/

static int 
knn_class(flt *x, struct codebook *cb, int nclass, int k)
{
  int *tbl = 0;
  int *ctab = 0;
  int i,n,c;
  
  tbl=(int*)malloc(k*sizeof(int));
  ctab=(int*)malloc(nclass*sizeof(int));
  ifn ( tbl && ctab) {
    if (tbl) free(tbl);
    if (ctab) free(ctab);
    error(NIL,"No memory",NIL);
  }
  
  knn(x, cb, k, tbl);
  for (i=0; i<nclass; i++)
    ctab[i] = 0;
  for (i=0; i<k; i++) {
    c = cb->code[tbl[i]].label;
    if (c>=0 && c<nclass)
      ctab[c]++;
  }
  n = c = -1;
  for (i=0; i<nclass; i++)
    if (ctab[i]>n) {
      c = i;
      n = ctab[i];
    } else if (ctab[i]==n) {
      c = -1;
    }
  if (tbl) free(tbl);
  if (ctab) free(ctab);
  return c;
}




/* OK classify all cbtst's codewords using KNN rule in cbref codebook ----*/

static flt perf_knn_err;
static flt perf_knn_rej;
static int number_of_labels(struct codebook *cb);

flt 
perf_knn(struct codebook *cbtst, struct codebook *cbref, int knn)
{
  int      i;
  int      err=0,rej=0;
  int      clas,nclas;

  nclas = number_of_labels(cbref);
  for (i=0;i<cbtst->ncode;i++) {
    CHECK_MACHINE("on");
    clas = knn_class(cbtst->code[i].word,cbref,nclas,knn);
    if (clas == -1)
      rej += 1;
    else if (clas != cbtst->code[i].label)
      err += 1;
  }
  perf_knn_err = (flt)(err*100)/(flt)(cbtst->ncode);
  perf_knn_rej = (flt)(rej*100)/(flt)(cbtst->ncode);
  return (100 - perf_knn_err - perf_knn_rej);
}










/**************************************************************************/
/*                                                                        */
/*                 PACKAGE K-MEANS : K-means routines                     */
/*                                                                        */
/**************************************************************************/






/* k-means_sub routine :
 * Adapt cbref using codebook cb
 */

static void
k_means_sub(struct codebook *cb, struct codebook *cbref, int maxpass)
{
  int km;
  int i,j,ppr,k;
  int pass = 0;
  int *pds = 0;
  flt dist,distmin,Ndistor,Odistor;
  
  Ndistor = Fone;
  Odistor = MAXFLOAT;

  km = cbref->ncode;
  if (km > cb->ncode)
    error(NIL,"Not enough patterns in codebook",NIL);
  if (cbref->ndim != cb->ndim)
    error(NIL,"Codebooks have different widths",NIL);

  if (!(pds=(int*)malloc(km*sizeof(int))))
    error(NIL,"No memory",NIL);
  for (i=0;i < km;i++)
    pds[i]=1;
  
  /* adaptation */
  
  while ( (Ndistor>0.00001) && ((Odistor-Ndistor)/Ndistor > 0.0001) )
    {
      CHECK_MACHINE("on");
      if (pass != 0) 
	Odistor = Ndistor;
      for (i=0; i<cb->ncode; i++)
	{
          ppr=1;
          distmin = MAXFLOAT;
          for (j=0;j<km;j++)
	    {
	      dist=dist2(cbref->code[j].word,cb->code[i].word,cb->ndim);
	      if (dist < distmin)
		{
		  distmin=dist;
		  ppr=j;
		}
	    }
          if (distmin != 0.0)
	    {
              for (k=0; k<cb->ndim; k++)
		cbref->code[ppr].word[k] = 
		  ((pds[ppr]*cbref->code[ppr].word[k])+cb->code[i].word[k])
		  /(pds[ppr]+1);
              pds[ppr]++;
	    }
	}
      pass++;
      if (pass>=maxpass)
        break;
      Ndistor=distorsion(cb,cbref);
    }
  free(pds);
}

at *
k_means(struct codebook *cb, int km)
{
  at *ans;
  struct codebook *cbref;
  int i,k;
  
  ans=new_codebook(km,cb->ndim);
  cbref=ans->Object;
  for (i=0;i < km;i++)
    {
      cbref->code[i].label = cb->code[i].label;
      for(k=0;k<cb->ndim;k++)
	cbref->code[i].word[k] = cb->code[i].word[k];
    }
  k_means_sub(cb,cbref,32768);
  return ans;
}







/**************************************************************************/
/*                                                                        */
/*                 PACKAGE LVQ : Routines needed in lvq programs          */
/*                                                                        */
/**************************************************************************/










/* give number of labels in a codewords --------------------------------- */

static int 
number_of_labels(struct codebook *cb)
{
  int i;
  int      nb=-1;
  for (i = 0; i < cb->ncode; i++) {
    if (cb->code[i].label > nb) 
      nb=cb->code[i].label;
    if (cb->code[i].label < 0)
      error(NIL,"Codebook labels is negative for item",NEW_NUMBER(i));
  }
  return(nb+1);
}





/* adaptation of codewords references ----------------------------------- */

#ifdef __GNUC__
inline
#endif
static void 
adapt(struct codeword *d, struct codeword *r, flt a, int dim)
{
  int i;
  for (i = 0; i < dim; i++)
    r->word[i] -= a*(d->word[i] - r->word[i]);
}





/* LVQ's learning rate computation -------------------------------------- */

#ifdef __GNUC__
inline
#endif
static flt 
alpha(flt a0, int it, int nbit)
{
  return (a0*(Fone-((flt)it)/(flt)nbit));
}	  





/* test if x falls in a defined window ----------------------------------- */

static int 
in_window(struct codeword *mi, struct codeword *mj, 
	  struct codeword *x, int dim, flt w )
{
  int  i;
  flt  ps = Fzero;
  flt  refdist = Fzero;
  flt  dref,ddat;
  
  for (i = 0; i < dim; i++)
    {
      dref = mj->word[i] - mi->word[i];
      ddat = x->word[i] - mi->word[i];
      refdist += (dref*dref);
      ps += ddat*dref;
    }
  return( fabs(2*ps-refdist) <= w*refdist );
}






/* LVQ1 rules adaptation of cbref's codewords --------------------------- */

void 
learn_lvq1(struct codebook *cbdata, struct codebook *cbref, 
	   int nbit, flt a0)
{
  int  i,j,nn1;
  flt  alph = Fzero;
  
  for (i = 0; i < nbit; i++)
    {
      CHECK_MACHINE("on");
      for (j = 0; j < cbdata->ncode; j++)
	{
          one_nn(cbdata->code[j].word,cbref,&nn1);
          if (cbdata->code[j].label != cbref->code[nn1].label)
	    {
	      alph=alpha(a0,i,nbit);
	      adapt(&(cbdata->code[j]),&(cbref->code[nn1]),alph,cbdata->ndim);
	    }
          else
	    adapt(&(cbdata->code[j]),&(cbref->code[nn1]),-alph,cbdata->ndim);
	}
    }
}





/* LVQ2 rules adaptation of cbref's codewords --------------------------- */

void 
learn_lvq2(struct codebook *cbdata, struct codebook *cbref, 
	   int nbit, flt a0, flt win)
{
  int i,j,nn1,nn2;
  flt alph = Fzero;
  
  for (i = 0; i < nbit; i++) {
    CHECK_MACHINE("on");
    for (j = 0; j < cbdata->ncode; j++)
      {
	two_nn(cbdata->code[j].word,cbref,&nn1,&nn2);
	if (cbdata->code[j].label != cbref->code[nn1].label)
	  {
	    if (cbdata->code[j].label == cbref->code[nn2].label)
	      if (in_window(&(cbref->code[nn1]), &(cbref->code[nn2]),
			    &(cbdata->code[j]), cbdata->ndim,
			    win ) )
		{
		  alph=alpha(a0,i,nbit);
		  adapt(&(cbdata->code[j]),&(cbref->code[nn1]),alph,cbdata->ndim);
		  adapt(&(cbdata->code[j]),&(cbref->code[nn2]),-alph,cbdata->ndim);
		}
	  }
      }
  }
}




/* LVQ2 rules adaptation of cbref's codewords --------------------------- */

void 
learn_lvq3(struct codebook *cbdata, struct codebook *cbref, 
	   int nbit, flt a0, flt win)
{
  int i,j,nn1,nn2,c;
  flt alph = Fzero;
  
  for (i = 0; i < nbit; i++) {
    CHECK_MACHINE("on");
    for (j = 0; j < cbdata->ncode; j++)
      {
	c = cbdata->code[j].label;
	one_nn_and_a_half(cbdata->code[j].word,cbref,c,&nn1,&nn2);
	if (c!=cbref->code[nn1].label)
	  {
	    if (nn2<0)
	      error(NIL,"Class not represented in the references",
		    NEW_NUMBER(c));
	    if (in_window(&(cbref->code[nn1]), &(cbref->code[nn2]),
			  &(cbdata->code[j]), cbdata->ndim,
			  win ) )
	      {
		alph=alpha(a0,i,nbit);
		adapt(&(cbdata->code[j]),&(cbref->code[nn1]),alph,cbdata->ndim);
		adapt(&(cbdata->code[j]),&(cbref->code[nn2]),-alph,cbdata->ndim);
	      }
	  }
      }
  }
}









/**************************************************************************/
/* ********************************************************************** */
/* *                                                                    * */
/* *                PACKAGE DX interface to AdaptKnn routines           * */
/* *                                                                    * */
/* ********************************************************************** */
/**************************************************************************/





/* knn classification rule on a codebook data in a codebook reference -- */
/* (knn <vector> <cbref> <k>)                                            */



DX (xknn)
{
  flt *x;
  struct codebook *cb;
  int k;
  int d=0;
  int *res = 0;
  
  at *ans = NIL;
  at **where = &ans;

  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  x = make_nrvector(APOINTER(1),0,&d);
  cb = check_codebook(APOINTER(2));
  k = AINTEGER(3);
  
  if (d != cb->ndim)
    error(NIL,"Vector and codebook have different sizes",APOINTER(1));
  ifn (res = (int*)malloc(k*sizeof(int)))
    error(NIL,"No memory",NIL);
  knn(x,cb,k,res);
  for (d=0; d<k; d++) {
    *where = cons( NEW_NUMBER(res[d]) ,NIL);
    where = &( (*where)->Cdr );
  }
  free(res);
  return ans;
}


/* compute the class by voting the k nearest neighbours ---------------- */

DX(xknn_class)
{
  flt *x;
  struct codebook *cb;
  int d=0;
  int k;
  int clas, nclas;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  x = make_nrvector(APOINTER(1),0,&d);
  cb = check_codebook(APOINTER(2));
  k = AINTEGER(3);
  if (d != cb->ndim)
    error(NIL,"Vector and codebook have different sizes",APOINTER(1));
  
  nclas = number_of_labels(cb);
  clas = knn_class(x,cb,nclas,k);
  return NEW_NUMBER(clas);
}




/* compute the distorsion between two codebooks ------------------------ */

DX(xcodebook_distorsion)
{
  struct codebook *cb;
  struct codebook *cbref;
  flt res;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  cb = check_codebook(APOINTER(1));
  cbref = check_codebook(APOINTER(2));
  if (cb->ndim != cbref->ndim)
    error(NIL,"Codebook dimension mismatch",NIL);
  res = distorsion(cb,cbref);
  return NEW_NUMBER(Ftor(res));
}




/* compute a vector of distances --------------------------------------- */

DX(xcodebook_distances)
{
  struct codebook *cb;
  flt *v;
  flt *vd;
  at *ans;
  
  ALL_ARGS_EVAL;
  if (arg_number==2) {
    cb = check_codebook(APOINTER(2));
    ans = F_matrix(1,&(cb->ncode));
  } else {
    ARG_NUMBER(3);
    cb = check_codebook(APOINTER(2));
    ans = APOINTER(3);
    LOCK(ans);
  }
  easy_index_check(APOINTER(1), 1, &(cb->ndim));
  v = make_nrvector(APOINTER(1), 0, NULL);
  easy_index_check(ans, 1, &(cb->ncode));
  vd = make_nrvector(ans, 0, NULL);
  vect_dist2(v,cb,vd);
  return ans;
}



/* compute a global codebook reference --------------------------------- */
/* (k_means <cbdata> <k>)                                                */

DX (xk_means)
{
  struct codebook *cbdata;
  int knn;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  cbdata = check_codebook(APOINTER(1));
  knn = AINTEGER(2);
  if (knn<1)
    error(NIL,"k must be larger than 1",APOINTER(2));
  return(k_means(cbdata,knn));
}

DX (xk_means_internal)
{
  struct codebook *cbdata, *cbref;
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  cbdata = check_codebook(APOINTER(1));
  cbref =  check_codebook(APOINTER(2));
  k_means_sub(cbdata,cbref,AINTEGER(1));
  return NIL;
}





/* LVQ learning procedure ----------------------------------------------- */
/*     (learn_lvq 1 <version> <cbdata> <cbref> <nbiter> [a0])             */
/*     (learn_lvq 2|3 <version> <cbdata> <cbref> <nbiter> [a0] [win])     */



DX (xlearn_lvq)
{
  int version;
  int niter;
  struct codebook *cbdata,*cbref;
  flt a0 = (flt)0.1;
  flt win = (flt)0.01;

  ALL_ARGS_EVAL;
  
  switch(arg_number) {
  case 6:
    win = AFLT(6);
    a0 = AFLT(5);
    break;
  case 5:
    a0 = AFLT(5);
    break;
  case 4:
    break;
  default:
    ARG_NUMBER(-1);
  }
      
  version = AINTEGER(1);
  cbdata = check_codebook(APOINTER(2));
  cbref = check_codebook(APOINTER(3));
  niter = AINTEGER(4);

  if (cbdata->ndim != cbref->ndim)
    error(NIL,"Codebooks have different sizes",NIL);

  switch(version)
    {
    case 1:
      if (arg_number>5)
	error(NIL,"A window size cannot be specified with LVQ1",NIL);
      learn_lvq1(cbdata,cbref,niter,a0);
      break;
    case 2:
      learn_lvq2(cbdata,cbref,niter,a0,win);
      break;
    case 3:
      learn_lvq3(cbdata,cbref,niter,a0,win);
      break;
    default:
      error(NIL,"This version of LVQ is not implemented",APOINTER(1));
    }
  return NIL;
}




/* Perf_KNN give score of correct classification ----------------------- */
/* (perf_knn <cbdata> <cbref> [knn])                                     */



DX (xperf_knn)
{
  struct codebook *cbdata, *cbref;
  int knn = 1;
  at *flag = NIL;
  flt res;
  
  ALL_ARGS_EVAL;

  switch(arg_number) {
  case 4:
    flag = APOINTER(4);
    knn = AINTEGER(3);
    break;
  case 3:
    knn = AINTEGER(3);
    break;
  default:
    ARG_NUMBER(2);
  }

  if (knn<1)
    error(NIL,"k must be larger than 1",APOINTER(3));
  
  cbdata=check_codebook(APOINTER(1));
  cbref=check_codebook(APOINTER(2));
  
  if (cbdata->ndim != cbref->ndim)
    error(NIL,"Codebooks have different sizes",NIL);

  res = perf_knn(cbdata,cbref,knn);

  if (!flag)
    return NEW_NUMBER(Ftor(res));
  else
    return cons(NEW_NUMBER(Ftor(perf_knn_err)),
		cons(NEW_NUMBER(Ftor(perf_knn_rej)),
		     NIL) );
}


/* init ---------------------------------------------------------------- */

void 
init_adaptknn(void)
{
  dx_define("knn",xknn);
  dx_define("knn-class",xknn_class);
  dx_define("codebook-distances",xcodebook_distances);
  dx_define("codebook-distorsion",xcodebook_distorsion);
  dx_define("perf-knn",xperf_knn);
  dx_define("k-means",xk_means);
  dx_define("k-means-internal",xk_means_internal);
  dx_define("learn-lvq",xlearn_lvq);
}

