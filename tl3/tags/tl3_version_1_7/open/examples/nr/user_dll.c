/*   TL3 Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/************************************************************/
/************************************************************/
/**                                                        **/
/**   TL/OPEN EXAMPLE: NUMERICAL RECIPES INTERFACE         **/
/**      Please refer to the TL/Open manual for            **/
/**      additional information                            **/
/**                                                        **/
/************************************************************/
/************************************************************/



/* --- HEADER SECTION --- 
 *
 * This section loads the appropriate header files
 * and make sure that the correct libraries will be loaded.
 * (WIN32 details are specific to Visual C++ 4.x.)
 */


/* There are some conflicts between the names used by TL3 
 * and the names used by NUMREC. The following defines
 * avoid prototype conflict.
 */

#define matrix     DUMMYmatrix
#define imatrix    DUMMYimatrix
#define dmatrix    DUMMYdmatrix
#define erf        DUMMYerf
#define erfc       DUMMYerfc
#define rank       DUMMYrank

/* Include the TL/Open header file 
 */

#include "tlopen.h"

/* Restore the initial symbol names
 * The following functions now refer to the NR
 * functions rather than the TL functions
 */

#undef matrix
#undef imatrix
#undef dmatrix
#undef erf
#undef erfc
#undef rank


/* Certain routines require a declaration
 * because they return floating point numbers
 */

float bessi(), bessi0(), bessi1(), bessj(), bessj0();
float bessj1(), bessk(), bessk0(), bessk1(), bessy();
float bessy0(), bessy1(), beta(), betacf(), betai();
float bico(), bnldev(), brent(), cel(), chebev();
float dbrent(), df1dim(), el2(), erf(), erfc();
float erfcc(), evlmem(), expdev(), f1dim(), factln();
float factrl(), gamdev(), gammln(), gammp(), gammq();
float gasdev(), golden(), midexp(), midinf(), midpnt();
float midsql(), midsqu(), plgndr(), poidev(), probks();
float qgaus(), qromb(), qromo(), qsimp(), qtrap();
float quad3d(), ran0(), ran1(), ran2(), ran3();
float ran4(), rofunc(), rtbis(), rtflsp(), rtnewt();
float rtsafe(), rtsec(), trapzd(), zbrent();


/* The other routines use K&R prototypes
 * and are not even declared
 */

#pragma warning(disable: 4013)



/* --- NR UTILITY ROUTINES ---
 *
 * The following routines replace those
 * usually provided with the NRUTIL.C file.
 *
 * Note that we use the old K&R function declaration
 * like the 1st edition of the numerical recipes code.
 */


/* nrerror -- called by NR when an error occurs */
void nrerror(s)
char *s;
{
  error(NIL,s,NIL);
}

/* vector -- allocate temporary float vector */
float *vector(low,high)
int low,high;
{
  float *v;
  v=(float *)malloc( (high-low+1)*sizeof(float));
  if (!v)
    error(NIL,"no memory",NIL);
  return v-low;
}

/* free_vector -- free temporary float vector */
void free_vector(v,low,high)
float *v;
int low,high;
{
  free(v+low);
}

/* dvector -- allocate temporary double vector */
double *dvector(low,high)
int low,high;
{
  double *v;
  v=(double *)malloc( (high-low+1)*sizeof(double));
  if (!v) 
    error(NIL,"no memory",NIL);
  return v-low;
}

/* free_dvector -- free temporary double vector */
void free_dvector(v,low,high)
double *v;
int low,high;
{
  free(v+low);
}

/* ivector -- allocate temporary integer vector */
int *ivector(low,high)
int low,high;
{
  int *v;
  v=(int *)malloc( (high-low+1)*sizeof(int));
  if (!v)
    error(NIL,"no memory",NIL);
  return v-low;
}

/* free_ivector -- free temporary integer vector */
void free_ivector(v,low,high)
int *v;
int low,high;
{
  free(v+low);
}

/* matrix -- allocate temporary float matrix */
float **matrix(rlow,rhigh,clow,chigh)
int rlow,rhigh,clow,chigh;
{
  int i;
  float **m,*p;
  m=(float **) malloc((rhigh-rlow+1)*sizeof(float*));
  p=(float *) malloc((chigh-clow+1)*(rhigh-rlow+1)*sizeof(float) );
  if (!m || !p) 
    error(NIL,"no memory",NIL);
  m -= rlow;
  for(i=rlow;i<=rhigh;i++) {
    m[i]= p - clow;
    p += chigh-clow+1;
  }
  return m;
}

/* free_matrix -- free temporary float matrix */
void free_matrix(m,rlow,rhigh,clow,chigh)
float **m;
int rlow,rhigh,clow,chigh;
{
  free(m[rlow]+clow);
  free(m+rlow);
}

/* dmatrix -- allocate temporary double matrix */
double **dmatrix(rlow,rhigh,clow,chigh)
int rlow,rhigh,clow,chigh;
{
  int i;
  double **m,*p;
  m=(double **) malloc((rhigh-rlow+1)*sizeof(double*));
  p=(double *) malloc((chigh-clow+1)*(rhigh-rlow+1)*sizeof(double));
  if (!m || !p) 
    error(NIL,"no memory",NIL);
  m -= rlow;
  for(i=rlow;i<=rhigh;i++) {
    m[i]= p - clow;
    p += chigh-clow+1;
  }
  return m;
}

/* free_dmatrix -- free temporary double matrix */
void free_dmatrix(m,rlow,rhigh,clow,chigh)
double **m;
int rlow,rhigh,clow,chigh;
{
  free(m[rlow]+clow);
  free(m+rlow);
}

/* imatrix -- allocate temporary integer matrix */
int **imatrix(rlow,rhigh,clow,chigh)
int rlow,rhigh,clow,chigh;
{
  int i;
  int **m,*p;
  m=(int **) malloc((rhigh-rlow+1)*sizeof(int*));
  p=(int *) malloc((chigh-clow+1)*(rhigh-rlow+1)*sizeof(int));
  if (!m || !p) 
    error(NIL,"no memory",NIL);
  m -= rlow;
  for(i=rlow;i<=rhigh;i++) {
    m[i]= p - clow;
    p += chigh-clow+1;
  }
  return m;
}

/* free_imatrix -- free temporary integer matrix */
void free_imatrix(m,rlow,rhigh,clow,chigh)
int **m;
int rlow,rhigh,clow,chigh;
{
  free(m[rlow]+clow);
  free(m+rlow);
}


/* --- TL UTILITY ROUTINES ---
 *
 * The following routines help implementing
 * the interface functions.
 */


/* at_to_float -- converts lisp object to float */
static float 
at_to_float(at *p)
{
  if (! NUMBERP(p))
    error(NIL,"Number expected",p);
  return (float)(p->Number);
}

/* var_to_float -- get variable value as a float */
static float 
var_to_float(at *p)
{
  float ans;
  p = var_get(p);
  if (! NUMBERP(p))
    error(NIL,"Number expected",p);
  ans = (float)(p->Number);
  UNLOCK(p);
  return ans;
}


/* var_set_unlock -- set variable and unlock argument */
static void 
var_set_unlock(at *v, at *q)
{
  /* THIS FUNCTION ASSUMES THAT Q IS OVERLOCKED BY ONE!
   * This function is useful when argument Q is returned
   * by a function (and therefore is already locked).
   * It avoids saving and unlocking Q in the calling
   * function.
   */
  var_set(v,q);
  UNLOCK(q);
}


/* nr1_to_tl3_vector -- creates a tl3 vector */ 
static at *
nr1_to_tl3_vector(float *x, int n)
{
  at *ans = fmatrix(1, &n);
  struct array *arr = ans->Object;
  float *data = arr->data;
  memcpy(data, x+1, n*sizeof(float));
  return ans;
}


/* tl3_vector_to_nr1 -- copy a tl3 vector into nr vector */
static void 
tl3_vector_to_nr1(at *vec, float *y)
{
  int n = 0;
  float *x = get_nr1_vector(vec, &n);
  memcpy(y+1, x+1, n*sizeof(float));
}



/* These static variables are used
 * to communicate between the interface function
 * and helper functions
 */

static at *pt_func1, *pt_func2;
static at *pt_asub, *pt_atsub;
static int size;






/* --- PRIMITIVE SECTION --- 
 *
 * This section contains your new TL3 primitives.
 * You may also choose to implement them in auxilliary files.
 */


/* -------------- DX chapter 1 --------------- */

DX(xnr_flmoon)
{
  int n,nph,jd;
  float frac;
  ALL_ARGS_EVAL; ARG_NUMBER(4);
  n = AINTEGER(1);
  nph = AINTEGER(2);
  ASYMBOL(3);
  ASYMBOL(4);
  flmoon(n,nph,&jd,&frac);
  var_set_unlock(APOINTER(3),NEW_NUMBER(jd));
  var_set_unlock(APOINTER(4),NEW_NUMBER(frac));
  return NIL;
}

DX(xnr_julday)
{
  int mm,id,iyyy;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  mm = AINTEGER(1);
  id = AINTEGER(2);
  iyyy = AINTEGER(3);
  return NEW_NUMBER( julday(mm,id,iyyy) );
}

DX(xnr_caldat)
{
  int julian, mm,id,iyyy;
  ALL_ARGS_EVAL; ARG_NUMBER(4);
  julian = AINTEGER(1);
  ASYMBOL(2);
  ASYMBOL(3);
  ASYMBOL(4);
  caldat(julian,&mm,&id,&iyyy);
  var_set_unlock(APOINTER(2),NEW_NUMBER(mm));
  var_set_unlock(APOINTER(3),NEW_NUMBER(id));
  var_set_unlock(APOINTER(4),NEW_NUMBER(iyyy));
  return NIL;
}


/* -------------- DX chapter 2 --------------- */


DX(xnr_gaussj)
{
  int n=0, m=0;
  float **a, **b;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  b = get_nr1_matrix(APOINTER(2),&n,&m);
  gaussj(a,n,b,m);
  return NIL;
}


DX(xnr_ludcmp)  
{
  /*  (nr-ludcmp <matrix> <index>) --> <sign>  */
  int n=0,*indx;
  float **a, p;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  indx = get_nr1_ivector(APOINTER(2),&n);
  ludcmp(a,n,indx,&p);
  return NEW_NUMBER(p);
}

DX(xnr_lubksb) 
{
  float **a,*b;
  int n=0,*indx;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  indx = get_nr1_ivector(APOINTER(2),&n);
  b = get_nr1_vector(APOINTER(3),&n);
  lubksb(a,n,indx,b);
  return NIL;
}

DX(xnr_tridag)
{
  float *a,*b,*c,*r,*u;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(5);
  a = get_nr1_vector(APOINTER(1),&n);
  b = get_nr1_vector(APOINTER(2),&n);
  c = get_nr1_vector(APOINTER(3),&n);
  r = get_nr1_vector(APOINTER(4),&n);
  u = get_nr1_vector(APOINTER(5),&n);
  tridag(a,b,c,r,u,n);
  return NIL;
}


DX(xnr_mprove)
{
  float **a, **alud, *b, *x;
  int n=0,*indx;
  ALL_ARGS_EVAL; ARG_NUMBER(5);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  alud = get_nr1_matrix(APOINTER(2),&n,&n);
  indx = get_nr1_ivector(APOINTER(3),&n);
  b = get_nr1_vector(APOINTER(4),&n);
  x = get_nr1_vector(APOINTER(5),&n);
  mprove(a,alud,n,indx,b,x);
  return NIL;
}

DX(xnr_vander)
{
  float *x,*w,*q;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  x = get_nr1_vector(APOINTER(1),&n);
  w = get_nr1_vector(APOINTER(2),&n);
  q = get_nr1_vector(APOINTER(3),&n);
  vander(x,w,q,n);
  return NIL;
}

DX(xnr_toeplz)
{
  float *x,*w,*q;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  x = get_nr1_vector(APOINTER(1),&n);
  w = get_nr1_vector(APOINTER(2),&n);
  q = get_nr1_vector(APOINTER(3),&n);
  toeplz(x,w,q,n);
  return NIL;
}

DX(xnr_svdcmp)
{
  float **a, *w, **v;
  int n=0, m=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&m,&n);
  w = get_nr1_vector(APOINTER(2),&n);
  v = get_nr1_matrix(APOINTER(3),&n,&n);
  svdcmp(a,m,n,w,v);
  return NIL;
}


DX(xnr_svbksb)
{
  float **a, *w, **v, *b, *x;
  int n=0, m=0;
  ALL_ARGS_EVAL; ARG_NUMBER(5);
  a = get_nr1_matrix(APOINTER(1),&m,&n);
  w = get_nr1_vector(APOINTER(2),&n);
  v = get_nr1_matrix(APOINTER(3),&n,&n);
  b = get_nr1_vector(APOINTER(4),&n);
  x = get_nr1_vector(APOINTER(5),&n);
  svbksb(a,w,v,m,n,b,x);
  return NIL;
}



/* asub -- K&R style is required by first edition NR  */
void asub(xin, xout) /* not static */
float *xin, *xout;
{
  at *args, *result, *ret;
  args = cons(nr1_to_tl3_vector(xin,size), 
	      cons(ret=nr1_to_tl3_vector(xout,size), 
                   NIL ));
  result = apply(pt_asub,args);
  tl3_vector_to_nr1(ret, xout);
  UNLOCK(args);
  UNLOCK(result);
}

/* atsub -- K&R style is required by first edition NR  */
void atsub(xin, xout) /* not static */
float *xin, *xout;
{
  at *args, *result, *ret;
  args = cons(nr1_to_tl3_vector(xin,size), 
	      cons(ret=nr1_to_tl3_vector(xout,size), 
                   NIL ));
  result = apply(pt_atsub,args);
  tl3_vector_to_nr1(ret, xout);
  UNLOCK(args);
  UNLOCK(result);
}

DX(xnr_sparse)
{
  /* (nr-sparse <rhs-b> <lhs-x> <asub> <atsub>) --> rsq */
  float *b,*x,rsq;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(4);
  b = get_nr1_vector(APOINTER(1),&n);
  x = get_nr1_vector(APOINTER(2),&n);
  size = n;
  pt_asub = APOINTER(3);
  pt_atsub = APOINTER(4);
  sparse(b,n,x,&rsq);
  return NEW_NUMBER(rsq);
}


/* -------------- DX chapter 3 --------------- */

/* -------------- DX chapter 4 --------------- */

/* -------------- DX chapter 5 --------------- */

/* -------------- DX chapter 6 --------------- */

/* -------------- DX chapter 7 --------------- */

/* -------------- DX chapter 8 --------------- */

DX(xnr_sort)
{
  int n=0; 
  float *ra;
  ALL_ARGS_EVAL; ARG_NUMBER(1);
  ra = get_nr1_vector(APOINTER(1),&n);
  if (n>1) /* yet another bug in NR subroutines */
    sort(n,ra);
  return NIL;
}

DX(xnr_sort2)
{
  int n=0; 
  float *ra, *rb;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  ra = get_nr1_vector(APOINTER(1),&n);
  rb = get_nr1_vector(APOINTER(2),&n);
  if (n>1) /* same bug in NR subroutines */
    sort2(n,ra,rb);
  return NIL;
}

DX(xnr_indexx)
{
  int n=0, *indx;
  float *arrin;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  arrin = get_nr1_vector(APOINTER(1),&n);
  indx = get_nr1_ivector(APOINTER(2),&n);
  if (n>1) /* same bug in NR subroutines */
    indexx(n,arrin,indx);
  else
    indx[1]=1;
  return NIL;
}

DX(xnr_sort3)
{
  int n=0; 
  float *ra, *rb, *rc;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  ra = get_nr1_vector(APOINTER(1),&n);
  rb = get_nr1_vector(APOINTER(2),&n);
  rc = get_nr1_vector(APOINTER(3),&n);
  if (n>1) /* same bug in NR subroutines */
    sort3(n,ra,rb,rc);
  return NIL;
}

DX(xnr_rank)
{
  int n=0, *irank, *indx;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  indx = get_nr1_ivector(APOINTER(1),&n);
  irank = get_nr1_ivector(APOINTER(2),&n);
  rank(n,indx,irank);
  return NIL;
}

DX(xnr_qcksrt)
{
  int n=0; 
  float *ra;
  ALL_ARGS_EVAL; ARG_NUMBER(1);
  ra = get_nr1_vector(APOINTER(1),&n);
  if (n>1) /* same bug in NR subroutines */
    qcksrt(n,ra);
  return NIL;
}



/* -------------- DX chapter 9 --------------- */

/* -------------- DX chapter 10 -------------- */


/* func1 -- K&R style is required by first edition NR  */
static float func1(x)
float x;
{
  at *args, *result;
  float ans;
  args = cons(NEW_NUMBER(x),NIL);
  result = apply(pt_func1,args);
  UNLOCK(args);
  ans = at_to_float(result);
  UNLOCK(result);
  return ans;
}

/* func2 -- K&R style is required by first edition NR */
static float func2(x)
float x;
{
  at *args, *result;
  float ans;
  args = cons(NEW_NUMBER(x),NIL);
  result = apply(pt_func2,args);
  UNLOCK(args);
  ans = at_to_float(result);
  UNLOCK(result);
  return ans;
}

/* func1multi -- K&R style is required by first edition NR */
static float func1multi(x)
float *x;
{
  at *args, *result;
  float ans;
  
  args = cons(nr1_to_tl3_vector(x,size), NIL);
  result = apply(pt_func1,args);
  ans = at_to_float(result);
  UNLOCK(args);
  UNLOCK(result);
  return ans;
}

/* dfunc2multi -- K&R style is required by first edition NR */
static float dfunc2multi(x,y)
float *x, *y;
{
  at *args, *result, *ret;
  float ans;
  if (y) 
    args = cons(nr1_to_tl3_vector(x,size), 
		cons(ret=nr1_to_tl3_vector(y,size), 
                     NIL ));
  else
    args = cons(nr1_to_tl3_vector(x,size), 
		cons(NIL,NIL));
  result = apply(pt_func1,args);
  ans = at_to_float(result);
  if (y)
    tl3_vector_to_nr1(ret,y);
  UNLOCK(args);
  UNLOCK(result);
  return ans;
}


DX(xnr_mnbrak)
{
  float ax,bx,cx,fa,fb,fc;
  ALL_ARGS_EVAL; ARG_NUMBER(7); 
  ASYMBOL(1); ASYMBOL(2); ASYMBOL(3);
  ax = var_to_float(APOINTER(1));
  bx = var_to_float(APOINTER(2));
  cx = var_to_float(APOINTER(3));
  ASYMBOL(4); ASYMBOL(5); ASYMBOL(6);
  pt_func1 = APOINTER(7);
  mnbrak(&ax,&bx,&cx,&fa,&fb,&fc,func1);
  var_set_unlock(APOINTER(1),NEW_NUMBER(ax));
  var_set_unlock(APOINTER(2),NEW_NUMBER(bx));
  var_set_unlock(APOINTER(3),NEW_NUMBER(cx));
  var_set_unlock(APOINTER(4),NEW_NUMBER(fa));
  var_set_unlock(APOINTER(5),NEW_NUMBER(fb));
  var_set_unlock(APOINTER(6),NEW_NUMBER(fc));
  return NIL;
}

DX(xnr_golden)
{
  float ax,bx,cx,tol,xmin,ans;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  ax = (float)AREAL(1);
  bx = (float)AREAL(2);
  cx = (float)AREAL(3); 
  pt_func1 = APOINTER(4);
  tol = (float)AREAL(5);
  ASYMBOL(6);
  ans = golden(ax,bx,cx,func1,tol,&xmin);
  var_set_unlock(APOINTER(6),NEW_NUMBER(xmin));
  return NEW_NUMBER(ans);
}

DX(xnr_brent)
{
  float ax,bx,cx,tol,xmin,ans;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  ax = (float)AREAL(1);
  bx = (float)AREAL(2);
  cx = (float)AREAL(3); 
  pt_func1 = APOINTER(4);
  tol = (float)AREAL(5);
  ASYMBOL(6);
  ans = brent(ax,bx,cx,func1,tol,&xmin);
  var_set_unlock(APOINTER(6),NEW_NUMBER(xmin));
  return NEW_NUMBER(ans);
}


DX(xnr_dbrent)
{
  float ax,bx,cx,tol,xmin,ans;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  ax = (float)AREAL(1);
  bx = (float)AREAL(2);
  cx = (float)AREAL(3); 
  pt_func1 = APOINTER(4);
  pt_func2 = APOINTER(5);
  tol = (float)AREAL(6);
  ASYMBOL(7);
  ans = dbrent(ax,bx,cx,func1,func2,tol,&xmin);
  var_set_unlock(APOINTER(6),NEW_NUMBER(xmin));
  return NEW_NUMBER(ans);
}


DX(xnr_amoeba)
{
  /* (nr-amoeba <p> <y> <ftol> <func>) --> nfunk */
  float **p, *y, ftol;
  int ndim=0, ndimp1, nfunk;
  ALL_ARGS_EVAL; ARG_NUMBER(4);
  p = get_nr1_matrix(APOINTER(1),&ndimp1,&ndim);
  if (ndimp1!=ndim+1)
    error(NIL,"dimension mismatch",NIL);
  y = get_nr1_vector(APOINTER(2),&ndimp1);
  ftol = (float)AREAL(3);
  pt_func1 = APOINTER(4);
  size = ndim;
  amoeba(p,y,ndim,ftol,func1multi,&nfunk);
  return NEW_NUMBER(nfunk);
}

DX(xnr_powell)
{
  float *p,**xi,ftol,fret;
  int n=0, iter;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  p = get_nr1_vector(APOINTER(1),&n);
  xi = get_nr1_matrix(APOINTER(2),&n,&n);
  ftol = (float)AREAL(3);
  ASYMBOL(4);
  ASYMBOL(5);
  size = n;
  pt_func1 = APOINTER(6);
  powell(p,xi,n,ftol,&iter,&fret,func1multi);
  var_set_unlock(APOINTER(4),NEW_NUMBER(iter));
  var_set_unlock(APOINTER(5),NEW_NUMBER(fret));
  return NIL;
}

DX(xnr_frprmn)
{
  float *p,ftol,fret;
  int n=0, iter;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  p = get_nr1_vector(APOINTER(1),&n);
  ftol = (float)AREAL(2);
  ASYMBOL(3);
  ASYMBOL(4);
  size = n;
  pt_func1 = APOINTER(5);
  pt_func2 = APOINTER(6);
  frprmn(p,n,ftol,&iter,&fret,func1multi,dfunc2multi);
  var_set_unlock(APOINTER(3),NEW_NUMBER(iter));
  var_set_unlock(APOINTER(4),NEW_NUMBER(fret));
  return NIL;
}



DX(xnr_dfpmin)
{
  float *p,ftol,fret;
  int n=0, iter;
  ALL_ARGS_EVAL; ARG_NUMBER(6);
  p = get_nr1_vector(APOINTER(1),&n);
  ftol = (float)AREAL(2);
  ASYMBOL(3);
  ASYMBOL(4);
  size = n;
  pt_func1 = APOINTER(5);
  pt_func2 = APOINTER(6);
  dfpmin(p,n,ftol,&iter,&fret,func1multi,dfunc2multi);
  var_set_unlock(APOINTER(3),NEW_NUMBER(iter));
  var_set_unlock(APOINTER(4),NEW_NUMBER(fret));
  return NIL;
}

DX(xnr_simplx)
{
  int m,n,m1,m2,m3,icase,*izrov,*iposv;
  int mp2=0, np1=0;
  float **a;
  ALL_ARGS_EVAL; ARG_NUMBER(7);
  a = get_nr1_matrix(APOINTER(1),&mp2,&np1);
  m = mp2 - 2;
  n = np1 - 1;
  m1 = AINTEGER(2);
  m2 = AINTEGER(3);
  m3 = AINTEGER(4);
  ASYMBOL(5);
  izrov = get_nr1_ivector(APOINTER(6),&n);
  iposv = get_nr1_ivector(APOINTER(7),&m);
  simplx(a,m,n,m1,m2,m3,&icase,izrov,iposv);
  var_set_unlock(APOINTER(5),NEW_NUMBER(icase));
  return NIL;
}


/* -------------- DX chapter 11 -------------- */


DX(xnr_jacobi)
{
  /* (nr-jacobi <a> <d> <v> ) --> <nrot> */
  float **a,*d,**v;
  int n=0,nrot;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  d = get_nr1_vector(APOINTER(2),&n);
  v = get_nr1_matrix(APOINTER(3),&n,&n);
  jacobi(a,n,d,v,&nrot);
  return NEW_NUMBER(nrot);
}

DX(xnr_eigsrt)
{
  float *d, **v;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  d = get_nr1_vector(APOINTER(1),&n);
  v = get_nr1_matrix(APOINTER(2),&n,&n);
  eigsrt(d,v,n);
  return NIL;
}


DX(xnr_tred2)
{
  float **a,*d,*e;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  d = get_nr1_vector(APOINTER(2),&n);
  e = get_nr1_vector(APOINTER(3),&n);
  tred2(a,n,d,e);
  return NIL;
}

DX(xnr_tqli)
{
  float *d,*e,**z;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  d = get_nr1_vector(APOINTER(1),&n);
  e = get_nr1_vector(APOINTER(2),&n);
  z = get_nr1_matrix(APOINTER(3),&n,&n);
  tqli(d,e,n,z);
  return NIL;
}

DX(xnr_balanc)
{
  float **a;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(1);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  balanc(a,n);
  return NIL;
}

DX(xnr_elmhes)
{
  float **a;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(1);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  elmhes(a,n);
  return NIL;
}

DX(xnr_hqr)
{
  float **a, *wr,*wi;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  wr = get_nr1_vector(APOINTER(2),&n);
  wi = get_nr1_vector(APOINTER(2),&n);
  hqr(a,n,wr,wi);
  return NIL;
}


#ifdef TQLINOV_AND_TRED2NOV

DX(xnr_tred2_nov)
{
  float **a,*d,*e;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  a = get_nr1_matrix(APOINTER(1),&n,&n);
  d = get_nr1_vector(APOINTER(2),&n);
  e = get_nr1_vector(APOINTER(3),&n);
  tred2_nov(a,n,d,e);
  return NIL;
}

DX(xnr_tqli_nov)
{
  /* (nr-tqli-nov <d> <e>) */
  float *d,*e;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  d = get_nr1_vector(APOINTER(1),&n);
  e = get_nr1_vector(APOINTER(2),&n);
  tqli_nov(d,e,n);
  return NIL;
}

#endif

/* -------------- DX chapter 12 -------------- */


static void
check_power_of_two(int n)
{
  int m = n;
  if (m<=2)
    error(NIL,"ridiculous fft size",NEW_NUMBER(n));
  while (!(m&1))
    m>>=1;
  m>>=1;
  if (m!=0)
    error(NIL,"fft size is not a power of two",NEW_NUMBER(n));
}

DX(xnr_four1)
{
  int twonn=0, isign;
  float *data;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  data = get_nr1_vector(APOINTER(1),&twonn);
  isign = AINTEGER(2);
  check_power_of_two(twonn);
  four1(data,twonn/2,isign);
  return NIL;
}

DX(xnr_twofft)
{
  int nn=0, twonn;
  float *data1, *data2, *fft1, *fft2;
  ALL_ARGS_EVAL; ARG_NUMBER(4);
  data1 = get_nr1_vector(APOINTER(1),&nn);
  data2 = get_nr1_vector(APOINTER(2),&nn);
  check_power_of_two(nn);
  twonn = nn*2;
  fft1 = get_nr1_vector(APOINTER(3),&twonn);
  fft2 = get_nr1_vector(APOINTER(4),&twonn);
  twofft(data1,data2,fft1,fft2,nn);
  return NIL;
}

DX(xnr_realft)
{
  float *data;
  int n=0, isign;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  data = get_nr1_vector(APOINTER(1),&n);
  isign = AINTEGER(2);
  check_power_of_two(n);
  realft(data,n/2,isign);
  return NIL;
}

DX(xnr_sinft)
{
  float *data;
  int n=0;
  ALL_ARGS_EVAL; ARG_NUMBER(1);
  data = get_nr1_vector(APOINTER(1),&n);
  check_power_of_two(n);
  sinft(data,n);
  return NIL;
}

DX(xnr_convlv)
{
  float *data,*respns,*ans;
  int n=0,m,twon,isign;
  ALL_ARGS_EVAL; ARG_NUMBER(5);
  data = get_nr1_vector(APOINTER(1),&n);
  respns = get_nr1_vector(APOINTER(2),&n);
  m = AINTEGER(3);
  if (m>=n)
    error(NIL,"Size of the response function is too large",APOINTER(3));
  isign = AINTEGER(4);
  twon = 2*n;
  check_power_of_two(n);
  ans = get_nr1_vector(APOINTER(5),&twon);
  convlv(data,n,respns,m,isign,ans);
  return NIL;
}

DX(xnr_correl)
{
  float *data1,*data2,*ans;
  int n=0, twon;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  data1 = get_nr1_vector(APOINTER(1),&n);
  data2 = get_nr1_vector(APOINTER(2),&n);
  twon = 2*n;
  ans = get_nr1_vector(APOINTER(3),&twon);
  check_power_of_two(n);
  correl(data1,data2,n,ans);
  return NIL;
}

DX(xnr_memcof)
{
  /* (nr-memcof <data> <cof>) --> <pm> */
  float *data,*cof,pm;
  int n=0, m=0;
  ALL_ARGS_EVAL; ARG_NUMBER(2);
  data = get_nr1_vector(APOINTER(1),&n);
  cof = get_nr1_vector(APOINTER(2),&m);
  memcof(data,n,m,&pm,cof);
  return NEW_NUMBER(pm);
}

DX(xnr_evlmem)
{
  float fdt,*cof,pm;
  int m=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  fdt = (float)AREAL(1);
  cof = get_nr1_vector(APOINTER(2),&m);
  pm = (float)AREAL(3);
  return NEW_NUMBER(evlmem(fdt,cof,m,pm));
}

DX(xnr_predic)
{
  float *data,*d,*future;
  int ndata=0,npoles=0,nfut=0;
  ALL_ARGS_EVAL; ARG_NUMBER(3);
  data = get_nr1_vector(APOINTER(1),&ndata);
  d = get_nr1_vector(APOINTER(2),&npoles);
  future = get_nr1_vector(APOINTER(3),&nfut);
  predic(data,ndata,d,npoles,future,nfut);
  return NIL;
}

DX(xnr_fourn)
{
  /* (nr-fourn <mat> <isign>) */
  /* Rather different from NR conventions! */
  at *p;
  struct array *arr;
  int i, j, isign;
  float *data;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  isign = AINTEGER(2);
  p = APOINTER(1);
  ifn(p && (p->flags&C_EXTERN) && (p->Class==&matrix_class))
    error(NIL,"not a matrix",p);
  arr = p->Object;
  if (arr->flags & MAPPED_ARRAY)
    error(NIL, "mapped arrays are illegal here", p);
  j = 1;
  for (i=arr->ndim-1; i>=0; i--) {
    if (arr->modulo[i]!=j)
      error(NIL,"copy this matrix first",p);
    check_power_of_two(arr->dim[i]);
    j *= arr->dim[i];
  }
  data = arr->data;
  fourn( data-1, arr->dim-1, arr->ndim, isign);
  return NIL;
}



/* -------------- DX chapter 13 -------------- */

/* -------------- DX chapter 14 -------------- */





/* --- INITIALISATION SECTION --- 
 * 
 * Function 'init_user_dll' is called when you load 
 * the TLOpen extension. You may :
 * -- perform your initializations here
 * -- declare your primitives here
 */


int init_user_dll(int major, int minor)
{
  /* Check version */
  if (major!=TLOPEN_MAJOR || minor<TLOPEN_MINOR)
    return -2;

  /* CHAPTER 1 (complete) */
  dx_define("nr-flmoon", xnr_flmoon);
  dx_define("nr-julday", xnr_julday);	       /* ok */
  dx_define("nr-caldat", xnr_caldat);	       /* ok */

  /* CHAPTER 2 (complete) */
  dx_define("nr-gaussj", xnr_gaussj);	       /* ok */
  dx_define("nr-ludcmp", xnr_ludcmp);	       /* ok */
  dx_define("nr-lubksb", xnr_lubksb);	       /* ok */
  dx_define("nr-tridag", xnr_tridag);
  dx_define("nr-mprove", xnr_mprove);
  dx_define("nr-vander", xnr_vander);
  dx_define("nr-toeplz", xnr_toeplz);
  dx_define("nr-svdcmp", xnr_svdcmp);
  dx_define("nr-svbksb", xnr_svbksb);
  dx_define("nr-sparse", xnr_sparse);

  /* CHAPTER 3 */
  /* CHAPTER 4 */
  /* CHAPTER 5 */
  /* CHAPTER 6 */
  /* CHAPTER 7 */
  /* CHAPTER 8 */
  dx_define("nr-sort", xnr_sort);	       /* ok */
  dx_define("nr-sort2", xnr_sort2);
  dx_define("nr-indexx", xnr_indexx);	       /* ok */
  dx_define("nr-sort3", xnr_sort3);
  dx_define("nr-rank", xnr_rank);
  dx_define("nr-qcksrt", xnr_qcksrt);

  /* CHAPTER 9 */
  /* CHAPTER 10 */
  dx_define("nr-mnbrak", xnr_mnbrak);
  dx_define("nr-golden", xnr_golden);
  dx_define("nr-brent", xnr_brent);
  dx_define("nr-dbrent", xnr_dbrent);
  dx_define("nr-amoeba", xnr_amoeba);	       /* ok */
  dx_define("nr-powell", xnr_powell);
  dx_define("nr-frprmn", xnr_frprmn);
  dx_define("nr-dfpmin", xnr_dfpmin);
  dx_define("nr-simplx", xnr_simplx);

  /* CHAPTER 11 */
  dx_define("nr-jacobi", xnr_jacobi);
  dx_define("nr-eigsrt", xnr_eigsrt);
  dx_define("nr-tred2", xnr_tred2);
  dx_define("nr-tqli", xnr_tqli);
  dx_define("nr-balanc", xnr_balanc);
  dx_define("nr-elmhes", xnr_elmhes);
  dx_define("nr-hqr", xnr_hqr);
#ifdef TQLI_TRED_NO_VECTOR
  dx_define("nr-tred2-nov", xnr_tred2_nov);
  dx_define("nr-tqli-nov", xnr_tqli_nov);
#endif

  /* CHAPTER 12 */
  dx_define("nr-four1", xnr_four1);
  dx_define("nr-twofft", xnr_twofft);
  dx_define("nr-realft", xnr_realft);
  dx_define("nr-sinft", xnr_sinft);
  dx_define("nr-convlv", xnr_convlv);
  dx_define("nr-correl", xnr_correl);
  dx_define("nr-memcof", xnr_memcof);
  dx_define("nr-evlmem", xnr_evlmem);
  dx_define("nr-predic", xnr_predic);
  dx_define("nr-fourn", xnr_fourn);

  /* CHAPTER 13 */
  /* CHAPTER 14 */
  /* THE END */
  return 0;
}
