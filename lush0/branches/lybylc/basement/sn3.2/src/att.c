/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */


#include <math.h>
#include "header.h"

/* ============================================ */
/* NR intERFACE DEFINITIONS                     */
/* ============================================ */



extern flt** make_nrmatrix();
extern flt * make_nrvector();
extern void nrerror();
extern void free_vector();
extern void free_matrix();

/* ============================================ */
/* SIMPLE EXAMPLES:  GCD, LCM, ATAN2            */
/* ============================================ */





/* find greatest common divisor of two
   integers, by JSD from Knuth 
   and from the oldest algorithm ever */

/* THIS IS THE C FUNCTION THAT DOES THE JOB */
int gcd( aa,  bb)
int aa, bb; 
{
  int a = aa>0 ? aa : -aa;	 /* take absolute value */
  int b = bb>0 ? bb : -bb;
  while (b) {			/* if b is zero, quit */
    int r = a % b;		/* look at remainder */
    a = b;
    b = r;			/* r is less than a */
  }
  return a;
}

/* THIS IS THE C TO LISP intERFACE */
DX(xgcd)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return NEW_NUMBER( gcd(AINTEGER(1), AINTEGER(2)) );
}


/* And least common multiple.
   Definition is questionable if an argument is zero; 
   return lcm=0 in that case for lack of anything better.
*/

int lcm( aa, bb)
int aa, bb;
{
  int a = aa>0 ? aa : -aa;
  int b = bb>0 ? bb : -bb;
  if (!a) return 0;
  if (!b) return 0;
  return a / gcd(a, b) * b;
}
DX(xlcm)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return NEW_NUMBER( lcm(AINTEGER(1), AINTEGER(2)) );
}



/* HYPOT, ATAN2 */

DX(xhypot)
{
  real hypot();
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  return NEW_NUMBER(hypot(AREAL(1),AREAL(2)));
}

DX(xatan2)
{
  real atan2();
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  return NEW_NUMBER(atan2(AREAL(1),AREAL(2)));
}





/* ============================================ */
/* SUBSAMPLE                                    */
/* ============================================ */

/* subsample matrix by performing local averaging on m line by n column
   windows.  result is put in a second matrix */

at *subsample(v1,m,n,ans)
at *v1,*ans;                /* type 'at' designates a lisp atom */
int n,m;
{
  flt **a, **res, norm;
  int ma=0,na=0,mk,nk,mr=0,nr=0;
  int i,j;

  register flt s, *p;
  register int k,l,mki,nkj;

  mk = m; nk= n;
  /* make_nrmatrix: first arg:  lisp atom */
  /* it must be a non-transposed matrix --- error otherwise */
  /* return value = pointer (flt**) to data area of matrix */
  /* last 2 args: passed in as zero ==> passed back as y,x sizes */
    a    = make_nrmatrix(v1,0,0,&ma,&na);
  /* check if ratio and sizes are compatible */
    if (( ma % mk )||( na % nk)) {
      error(NIL,"matrix sizes and subsampling ratios are incompatible",NIL);
    }
    mr = ma/mk; nr=na/nk;         /* calculate expected size of output */
  /* last 2 args passed in as nonzero ==> error if <> actual sizes */
    res  = make_nrmatrix(ans,0,0,&mr,&nr);
    norm= 1.0/(mk*nk);

    for (i=0; i<mr; i++) {
      mki=mk*i;
      for (j=0; j<nr; j++) {
        nkj=nk*j;
        s = 0.0;
        for (k=0; k<mk; k++) {
          p = a[mki+k] + nkj; 
          for (l=0; l<nk; l++) { s += (*(p++)); }
        }
	res[i][j] = s*norm;
      }
    }

    return NIL;
}

/* subsample matrix. result in another matrix. */
/* called in lisp as (subsample input-matrix m n ans) */
/* which must already exist as matrices of the correct size */

DX(xsubsample)
{
    ALL_ARGS_EVAL;
    ARG_NUMBER(4);
    return subsample(APOINTER(1),AINTEGER(2),AINTEGER(3),APOINTER(4));
}







/* ============================================ */
/* RESAMPLE                                     */
/* ============================================ */


/* resample matrix, image size normalization 
   result is put in a second matrix */

#define i2oi(x) (oioff+(scali*(x-iioff)))
#define i2oj(x) (ojoff+(scalj*(x-ijoff)))

at *resample(v1,ans)
at *v1,*ans;                /* type 'at' designates a lisp atom */
{
  flt **inmat, **outmat, *p; 
  int ii,ij, ii1=0, ij1=0, oi,oj, oi1=0, oj1=0;
  flt scali, scalj, iioff, ijoff, oioff, ojoff;
  flt ipix_surface;
  int i,j;
  flt x, y, xw, yw, dx, dy, dxw, dyw;
  int xint, yint, xwint, ywint;
  flt inpix;

  /* make_nrmatrix: first arg:  lisp atom */
  /* it must be a non-transposed matrix --- error otherwise */
  /* return value = pointer (flt**) to data area of matrix */
  /* last 2 args: passed in as zero ==> passed back as y,x sizes */
  inmat    = make_nrmatrix(v1,0,0,&ii1,&ij1);
  outmat   = make_nrmatrix(ans,0,0,&oi1,&oj1);

  ii=ii1-1; ij=ij1-1;
  oi=oi1-1; oj=oj1-1;
  iioff = ii1/2.0; ijoff=ij1/2.0;
  oioff = oi1/2.0; ojoff=oj1/2.0;

  /* determine scaling factors */
  scali = (flt)oi1/(flt)ii1; 
  scalj = (flt)oj1/(flt)ij1;
  /* compute surface of input pixel after scaling */
  ipix_surface = scali*scalj;
  /* clear output matrix */
  for (i=0; i<oi1; i++){ 
    p = outmat[i];
    for (j=0; j<oj1; j++){ *p++ = 0.0; }
  }

  /* for each input pixel */
  for (i=0; i<ii1; i++){ 
    for (j=0; j<ij1; j++) {
      /* fetch input pixel intensity */
      inpix=inmat[i][j];

      /* do the computation only if pixel is non 0 */
      if (inpix!=0.0) {

        /* compute coordinates of upper left corner of 
           current input pixel in target map  */
        x = i2oi(i);  y=i2oj(j);
        /* same for lower right corner
           there is a 0.99 here to prevent a mess up when the lower and/or
           right edge falls exactly on a pixel boundary */
        xw = x + 0.99*scali; yw= y + 0.99*scalj;
        /* compute integer parts to know index of target pixels */
        xint = (int)x; yint= (int)y; xwint= (int)xw; ywint= (int)yw;

        /* now update target map depending on where the pixel ends up */
	if (xint==xwint){  /* is it split by a vertical pixel boundary */
	  /* not split */
	  if (yint==ywint){
	    /* not split */
	    outmat[xint][yint] += inpix*ipix_surface;
	  } else { /* horizontally split */
	    outmat[xint][yint] += inpix*(ywint-y)*scali;
	    outmat[xint][ywint] += inpix*(yw-ywint)*scali;
	  }
        } else { /* split vertically */
	  if (yint==ywint) { /* is it also split horizontally */
	    /* nope, just vertically */
	    outmat[xint][yint] += inpix*(xwint-x)*scalj;
	    outmat[xwint][yint] += inpix*(xw-xwint)*scalj;
	  } else { /* arrgh, split in both directions */
	    dx=xwint-x; dxw=xw-xwint;
	    dy=ywint-y; dyw=yw-ywint;
	    outmat[xint][yint] += inpix*dx*dy;
	    outmat[xwint][yint] += inpix*dxw*dy;
	    outmat[xint][ywint] += inpix*dx*dyw;
	    outmat[xwint][ywint] += inpix*dxw*dyw;
	  }
        }
      }  /* end of inpix!=0 */
    }
  } /* end for i */
  return NIL;
}

/* resample matrix. result in another matrix. */
/* called in lisp as (resample input-matrix ans) */
/* which must already exist  */

DX(xresample)
{
    ALL_ARGS_EVAL;
    ARG_NUMBER(2);
    return resample(APOINTER(1),APOINTER(2));
}


/* ============================================ */
/* NORMALIZE_GREY                               */
/* ============================================ */


/* normalize the grey levels of matrix v1 between Mn and Mx */
at *norm_grey(v1,Mn,Mx)
at *v1;                /* type 'at' designates a lisp atom */
flt Mn, Mx;
{
  flt **a;
  int ma=0,na=0;
  int i,j;

  register flt mx, mn, *p;
  register flt ratio;

  /* make_nrmatrix: first arg:  lisp atom */
  /* it must be a non-transposed matrix --- error otherwise */
  /* return value = pointer (flt**) to data area of matrix */
  /* last 2 args: passed in as zero ==> passed back as y,x sizes */
    a    = make_nrmatrix(v1,0,0,&ma,&na);
    mn = mx = a[0][0];

    /* compute min and max */
    for (i=0; i<ma; i++) {
      p = a[i];
      for (j=0; j<na; j++) {
        if (*p>mx) mx = *p;
        if (*p<mn) mn = *p;
        p++;
      }
    }

    /* normalize grey levels */
    ratio = (Mx-Mn) / (mx - mn);
    for (i=0; i<ma; i++) {
      p = a[i];
      for (j=0; j<na; j++) {
	*p = (*p - mn) * ratio + Mn;
        p++;
      }
    }

    return NIL;
}

/* this is the interface from lisp to the c prog that does the work (above) */
DX(xnorm_grey)
{
    ALL_ARGS_EVAL;
    if (arg_number == 1) {
      return norm_grey(APOINTER(1), -1.0, 1.0);
    } else {
      ARG_NUMBER(3);
      return norm_grey(APOINTER(1), AFLT(2), AFLT(3));
    }
}






/* ============================================ */
/* THRESHOLD_MATRIX                             */
/* ============================================ */





/* threshold the values of a matrix and set them to Mn or Mx */
at *threshold_matrix(v1,th,Mn,Mx)
at *v1;                /* type 'at' designates a lisp atom */
flt th, Mn, Mx;
{
  flt **a;
  int ma=0,na=0;
  int i,j;

  register flt *p;

  /* make_nrmatrix: first arg:  lisp atom */
  /* it must be a non-transposed matrix --- error otherwise */
  /* return value = pointer (flt**) to data area of matrix */
  /* last 2 args: passed in as zero ==> passed back as y,x sizes */
    a    = make_nrmatrix(v1,0,0,&ma,&na);

    for (i=0; i<ma; i++) {
      p = a[i];
      for (j=0; j<na; j++) {
	*p = (*p > th) ? Mx : Mn;
        p++;
      }
    }

    return NIL;
}

DX(xthreshold_matrix)
{
    ALL_ARGS_EVAL;
    if (arg_number == 2) {
      return threshold_matrix(APOINTER(1), AFLT(2), -1.0, 1.0);
    } else {
      ARG_NUMBER(4);
      return threshold_matrix(APOINTER(1), AFLT(2), AFLT(3), AFLT(4));
    }
}






/* ============================================ */
/* ABS_MATRIX                                   */
/* ============================================ */





/* apply absolute value to elements of matrix v1 */
at *abs_matrix(v1)
at *v1;                /* type 'at' designates a lisp atom */
{
  flt **a;
  int ma=0,na=0;
  int i,j;
  
  register flt *p;

  /* make_nrmatrix: first arg:  lisp atom */
  /* it must be a non-transposed matrix --- error otherwise */
  /* return value = pointer (flt**) to data area of matrix */
  /* last 2 args: passed in as zero ==> passed back as y,x sizes */
    a    = make_nrmatrix(v1,0,0,&ma,&na);

    for (i=0; i<ma; i++) {
      p = a[i];
      for (j=0; j<na; j++) {
	*p = ( *p >0.0) ? *p : -(*p) ;
        p++;
      }
    }

    return NIL;
}

DX(xabs_matrix)
{
    ALL_ARGS_EVAL;
    ARG_NUMBER(1);
    return abs_matrix(APOINTER(1));
}



/* ============================================ */
/* BSEARCH_MATRIX                               */
/* ============================================ */

DX(xbsearch_matrix)
{
  flt v;
  flt *a;
  int m;
  int n,lo,hi;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  a = make_nrvector(APOINTER(1),0,&m);
  v = AFLT(2);

  lo = 0;
  hi = m;
  while (hi-lo>1) {
    n = (lo+hi)/2;
    /* printf("%d in [%d %d[\n",n,lo,hi); */
    if (v>=a[n])
      lo = n;
    else
      hi = n;
  }
  return NEW_NUMBER(lo);
}


/* ================================================================ */
/*   apply a DZ to all the elements of a matrix */
/* ================================================================ */
void
  dz_matrix(dz,m)
at *m, *dz;
{
  int mi,mj;
  flt **fm;
  int i,j,k,r;
  flt x;
  struct dz_cell *dzcell;

  fm = make_nrmatrix(m,0,0,&mi,&mj); 

  ifn (dz && dz->ctype==XT_DZ)
    error(NIL,"Not a DZ",dz);
  dzcell = dz->Object;
  if (dzcell->num_arg != 1)
    error(NIL,"DZ must take one argument",dz);

  for (i=0; i<mi; i++)
    for (j=0; j<mj; j++) {
      dz_stack[0] = x = fm[i][j];
      fm[i][j] = dz_execute(x,dzcell);
    }
}


DX(xdz_matrix) 
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  dz_matrix(p=APOINTER(1),APOINTER(2));
  LOCK(p);
  return p;
}





/* ============================================ */
/* DZ_2MATRIX                                   */
/* DTW_MATRIX                                   */
/* (signature project utilities)                */
/* ============================================ */





void
  dz_2matrix(m1,m2,md,dz)
at *m1, *m2, *md, *dz;
{
  int m1i,m1d, m2j,m2d, mdi,mdj;
  flt **fm1, **fm2, **fmd;
  int i,j,k,r;
  flt x;
  struct dz_cell *dzcell;

  fm1 = make_nrmatrix(m1,0,0,&m1d,&m1i); 
  fm2 = make_nrmatrix(m2,0,0,&m2d,&m2j);
  fmd = make_nrmatrix(md,0,0,&mdi,&mdj);

  ifn (m1i==mdi && m2j==mdj)
    error(NIL,"Dimension mismatch",NIL);

  ifn (dz && dz->ctype==XT_DZ)
    error(NIL,"Not a DZ",dz);
  dzcell = dz->Object;
  if (dzcell->num_arg != m1d+m2d)
    error(NIL,"Incorrect number of arguments",dz);

  for (i=0; i<m1i; i++)
    for (j=0; j<m2j; j++) {
      r = 0;
      for(k=0;k<m1d;k++)
	dz_stack[r++] = fm1[k][i];
      for(k=0;k<m2d;k++)
	dz_stack[r++] = x = fm2[k][j];
      fmd[i][j] = dz_execute(x,dzcell);
    }
}


DX(xdz_2matrix) 
{
  at *p;
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  dz_2matrix(APOINTER(1),APOINTER(2),APOINTER(3),p=APOINTER(4));
  LOCK(p);
  return p;
}



static flt min_arg_min(v0,v1,v2,aptr)
flt v0,v1,v2;
flt *aptr;
{
  flt y = v0;
  flt x = 0;
  if (v1<y) { y=v1; x=1; }
  if (v2<y) { y=v2; x=2; } 
  *aptr = x;
  return y;
}
		      
flt dtw_matrix(s1,s2,minmat,pathmat,dz,epsi_l,epsi_h,c0,c1,c2)
at *s1, *s2, *minmat, *pathmat, *dz;
flt epsi_h, epsi_l;
flt c0,c1,c2;
{
  int m1i,m1d, m2j,m2d, mdi,mdj;
  flt **fm1, **fm2, **fmin, **fpath;
  int i,j,k,r,lb,ub;
  flt x;
  struct dz_cell *dzcell;

  fm1  = make_nrmatrix(s1,0,0,&m1d,&m1i); 
  fm2  = make_nrmatrix(s2,0,0,&m2d,&m2j);
  fmin = make_nrmatrix(minmat,0,0,&mdi,&mdj);

  ifn (m1i==mdi && m2j==mdj)
    error(NIL,"Dimension mismatch",NIL);
  fpath = make_nrmatrix(pathmat,0,0,&mdi,&mdj);
  ifn (m1i==mdi && m2j==mdj)
    error(NIL,"Dimension mismatch",NIL);

  ifn (dz && dz->ctype==XT_DZ)
    error(NIL,"Not a DZ",dz);
  dzcell = dz->Object;
  if (dzcell->num_arg != m1d+m2d)
    error(NIL,"Incorrect number of arguments",dz);

  for (j=0; j<m2j; j++)
    for (i=0; i<m1i; i++)
      fmin[i][j] = 1.0e6;

  r = 0;
  for(k=0;k<m1d;k++)
    dz_stack[r++] = fm1[k][0];
  for(k=0;k<m2d;k++)
    dz_stack[r++] = x = fm2[k][0];
  fmin[0][0] = dz_execute(x,dzcell);

  for (j=1; j<m2j; j++) {
				/* Lower bound */
    lb = 0; 
    r = j*epsi_l; 			if (r>lb) lb = r;
    r = (m1i-1) - ((m2j-1)-j)*epsi_h; 	if (r>lb) lb = r;
				/* Upper bound */
    ub = m1i;
    r = j*epsi_h;			if (r<ub) ub = r;
    r = (m1i-1) - ((m2j-1)-j)*epsi_l;	if (r<ub) ub = r;

    for (i=lb; i<=ub; i++) {
				/* Compute the distance */
      r = 0;
      for(k=0;k<m1d;k++)
	dz_stack[r++] = fm1[k][i];
      for(k=0;k<m2d;k++)
	dz_stack[r++] = x = fm2[k][j];
      x = dz_execute(x,dzcell);
				/* Find the minimum */
      if (i>=2)
	fmin[i][j] = x + min_arg_min(c0+fmin[i][j-1],
				     c1+fmin[i-1][j-1],
				     c2+fmin[i-2][j-1],
				     &fpath[i][j] );
      else if (i==1)
	fmin[1][j] = x + min_arg_min(c0+fmin[1][j-1],
				       c1+fmin[0][j-1],
				     1e6,
				     &fpath[1][j] );
      else if (i==0) {
	fmin[0][j] = x;
	fpath[0][j] = 0;
      }
    }
  }
  return fmin[m1i-1][m2j-1] / (flt)m2j;
}


DX(xdtw_matrix) 
{
  at *p;
  flt x;
  ARG_NUMBER(10);
  ALL_ARGS_EVAL;
  x = dtw_matrix(APOINTER(1),APOINTER(2),
		 APOINTER(3),APOINTER(4),
		 APOINTER(5),
		 AFLT(6),AFLT(7),
		 AFLT(8),AFLT(9),AFLT(10));
  return NEW_NUMBER((real)(x));
}




/* ============================================ */
/* ANNA ENCODING ROUTINES                       */
/* (matrix2string <matrix>)                     */
/* (string2matrix <string> <destmat>)           */
/* (matrix2anna_state <matrix> [<r>])           */
/* (anna_state2matrix <string> <destmat> [<r>]) */
/* ============================================ */


DX(xmatrix2string) 
{
  at *p;
  flt **fp;
  char *sp;
  int n,m,i,j,c;

  at *new_sized_string();

  ARG_NUMBER(1);
  ARG_EVAL(1);

  p = APOINTER(1);
  fp = make_nrmatrix(p,0,0,&m,&n);

  p = new_sized_string(m*n);
  sp = SADD(p->Object);

  for (j=0;j<m;j++)
    for(i=0;i<n;i++) {
      c = fp[j][i];
      if (c<1 || c>255)
	error(NIL,"matrix element overflow string capacity",NEW_NUMBER(c));
      *sp++ = c;
    }
  *sp = 0;
  return p;
}


DX(string2matrix)
{
  at *p;
  flt **fp;
  char *sp;
  int n,m,i,j;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  
  sp = ASTRING(1);
  p = APOINTER(2);
  fp = make_nrmatrix(p,0,0,&m,&n);
  
  if (m*n != strlen(sp))
    error(NIL,"string does not match matrix size",NEW_NUMBER(strlen(sp)));
  for (j=0;j<m;j++)
    for(i=0;i<n;i++) {
      fp[j][i] = *sp++;
    }
  LOCK(p);
  return p;
}



DX(xmatrix2anna_state) 
{
  at *p;
  double r,x;
  flt **fp;
  char *sp;
  int n,m,i,j,c;

  at *new_sized_string();
  double floor();

  ALL_ARGS_EVAL;

  p = APOINTER(1);
  fp = make_nrmatrix(p,0,0,&m,&n);

  r = 3.0/1.7;
  if (arg_number!=1) {
    ARG_NUMBER(2);
    r = 3.0/AREAL(2);
  }

  p = new_sized_string(m*n);
  sp = SADD(p->Object);

  for (j=0;j<m;j++)
    for(i=0;i<n;i++) {
      x = r * (real)(fp[j][i]);
      if (x < 0) {
	c = floor(-x+0.5);
	*sp++ = (c>3 ? 3 : c) | 0x84;
      } else {
	c = floor(x+0.5);
        *sp++ = (c>3 ? 3 : c) | 0x80;
      }
    }
  *sp = 0;
  return p;
}


DX(anna_state2matrix)
{
  at *p;
  double r;
  flt **fp;
  char *sp;
  int n,m,i,j;

  static double tab [] = {   
    (0.0), (1.0/3.0), (2.0/3.0), (1.0), (0.0), (-1.0/3.0), (-2.0/3.0), (-1.0), };
  
  ALL_ARGS_EVAL;
  
  sp = ASTRING(1);

  p = APOINTER(2);
  fp = make_nrmatrix(p,0,0,&m,&n);
  
  r = 1.7;
  if (arg_number!=2) {
    ARG_NUMBER(3);
    r = AREAL(3);
  }
  
  if (m*n != strlen(sp))
    error(NIL,"string does not match matrix size",NEW_NUMBER(strlen(sp)));
  
  for (j=0;j<m;j++)
    for(i=0;i<n;i++) {
      fp[j][i] = (flt)( r*tab[*sp++ & 0x7] );
    }
  LOCK(p);
  return p;
}





/* ============================================ */
/* Example of DY:  LET*JSD                      */
/* ============================================ */


DY(yletstarjsd)
{
  at *ans;
  at *yletstar();
  in_object_scope++;
  ans = yletstar(ARG_LIST);
  in_object_scope--;
  return ans;
}



/* ============================================ */
/* DECLARATION OF THE LISP ROUTINES             */
/*  dx_define("lisp-name", xc-to-lisp-name);    */
/* ============================================ */



void init_att()
{
	 dx_define("gcd",xgcd);
	 dx_define("lcm",xlcm);
	 dx_define("atan2",xatan2);
	 dx_define("hypot",xhypot);

         dx_define("subsample",xsubsample);
         dx_define("resample",xresample);
         dx_define("normalize_grey",xnorm_grey);
         dx_define("threshold_matrix",xthreshold_matrix);
         dx_define("abs_matrix",xabs_matrix);
	 
	 dx_define("bsearch_matrix",xbsearch_matrix);
	 dx_define("dz_matrix",xdz_matrix);
	 
	 dx_define("dz_2matrix",xdz_2matrix);
	 dx_define("dtw_matrix",xdtw_matrix);

	 dx_define("matrix2string", xmatrix2string);
	 dx_define("string2matrix", string2matrix);
	 dx_define("matrix2anna_state", xmatrix2anna_state);
	 dx_define("anna_state2matrix", anna_state2matrix);

	 dy_define("let*jsd",yletstarjsd);
}

