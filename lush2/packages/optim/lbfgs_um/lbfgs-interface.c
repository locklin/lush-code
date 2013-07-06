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

#include "header.h"

extern struct {
   int lp;
   int mp;
   double gtol;
   double stpmin;
   double stpmax;
} lb3_;

struct htable {
   at *backptr;
};

static htable_t *lbfgs_params(void)
{
   htable_t *p = new_htable(31, false, false);

   /* these control verbosity */
   htable_set(p, NEW_SYMBOL("iprint-1"), NEW_NUMBER(-1));
   htable_set(p, NEW_SYMBOL("iprint-2"), NEW_NUMBER(0));
   htable_set(p, NEW_SYMBOL("warn"), NEW_NUMBER(1));

   /* these control line search behavior */
   htable_set(p, NEW_SYMBOL("ls-gtol"), NEW_NUMBER(lb3_.gtol));
   htable_set(p, NEW_SYMBOL("ls-stpmin"), NEW_NUMBER(lb3_.stpmin));
   htable_set(p, NEW_SYMBOL("ls-stpmax"), NEW_NUMBER(lb3_.stpmax));
   htable_set(p, NEW_SYMBOL("ls-maxfev"), NEW_NUMBER(20)); // only used by mcsrch

   /* LBFGS parameters */
   htable_set(p, NEW_SYMBOL("lbfgs-m"), NEW_NUMBER(7));

   /* this controls termination */
   htable_set(p, NEW_SYMBOL("maxiter"), NEW_NUMBER(1E9));

   return p;
}


DX(xlbfgs_params)
{
   ARG_NUMBER(0);
   return lbfgs_params()->backptr;
}


/* interface calling into the fortran routine */
static int lbfgs(index_t *x0, at *f, at *g, at *fdiag, double gtol, htable_t *p, at *vargs)
{
   /* argument checking and setup */

   extern void lbfgs_(int *n, int *m, double *x, double *fval, double *gval, \
                      int *diagco, double *diag, int iprint[2], double *gtol, \
                      double *xtol, double *w, int *iflag);
                        
   int diagco = false;

   ifn (IND_STTYPE(x0) == ST_DOUBLE)
      error(NIL, "not an array of doubles", x0->backptr);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", f);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", g);
   if (fdiag) {
      ifn (Class(fdiag)->listeval)
         error(NIL, "not a function", fdiag);
      else
         diagco = true;
   }
   ifn (gtol > 0)
      error(NIL, "threshold value not positive", NEW_NUMBER(gtol));
   
   at *gx = copy_array(x0)->backptr;
   at *dx = copy_array(x0)->backptr;
   at *(*listeval_f)(at *, at *) = Class(f)->listeval;
   at *(*listeval_g)(at *, at *) = Class(g)->listeval;
   at *(*listeval_d)(at *, at *) = diagco ? Class(fdiag)->listeval : listeval_g;
   at *callf = new_cons(f, new_cons(x0->backptr, vargs));
   at *callg = new_cons(g, new_cons(gx, new_cons(x0->backptr, vargs)));
   at *calld = diagco ? new_cons(fdiag, new_cons(dx, new_cons(x0->backptr, vargs))) : NIL;

   htable_t *params = lbfgs_params();
   if (p) htable_update(params, p);
   int iprint[2];
   iprint[0] = (int)Number(htable_get(params, NEW_SYMBOL("iprint-1")));
   iprint[1] = (int)Number(htable_get(params, NEW_SYMBOL("iprint-2")));
   lb3_.gtol = Number(htable_get(params, NEW_SYMBOL("ls-gtol")));
   lb3_.stpmin = Number(htable_get(params, NEW_SYMBOL("ls-stpmin")));
   lb3_.stpmax = Number(htable_get(params, NEW_SYMBOL("ls-stpmax")));
   int maxiter = (int)Number(htable_get(params, NEW_SYMBOL("maxiter"))); 
   ifn (maxiter > 0)
      error(NIL, "maxiter value not positive", NEW_NUMBER(maxiter));

   int m = (int)Number(htable_get(params, NEW_SYMBOL("lbfgs-m")));
   int n = index_nelems(x0);
   double *x = IND_ST(x0)->data;
   double  fval;
   double *gval = IND_ST(Mptr(gx))->data;
   double *w = mm_blob((n*(m+m+1)+m+m)*sizeof(double));
   double *diag = IND_ST(Mptr(dx))->data;
   double xtol = eps(1); /* machine precision */
   int iflag = 0;

   ifn (n>0)
      error(NIL, "empty array", x0->backptr);
   ifn (m>0)
      error(NIL, "m-parameter must be positive", NEW_NUMBER(m));

   /* reverse communication loop */
   int iter = 1;
   fval = Number(listeval_f(Car(callf), callf));
   listeval_g(Car(callg), callg);
   if (diagco)
      listeval_d(Car(calld), calld);

   do {
      lbfgs_(&n, &m, x, &fval, gval, &diagco, diag, iprint, &gtol, &xtol, w, &iflag);
      if (iflag == 1) {
         fval = Number(listeval_f(Car(callf), callf)); 
         listeval_g(Car(callg), callg);
         iter++;
      } else if (iflag == 2) {
         listeval_d(Car(calld), calld);
      }
   } while ((iflag > 0) && (iter < maxiter) && !break_attempt);

   if (iflag > 0 || break_attempt)
      iflag = 100;   /* signal termination by maxiter */
   return iflag;
}

DX(xlbfgs)
{
   if (arg_number < 4)
      ARG_NUMBER(4);

   index_t *x0 = copy_array(AINDEX(1));
   double gtol = ADOUBLE(4);
   htable_t *p = NULL;
   at *vargs = NIL;
   if (arg_number>4 && HTABLEP(APOINTER(5)))
      p = (htable_t *)Mptr(APOINTER(5));
   for (int i=arg_number; i>5; i--)
      vargs = new_cons(APOINTER(i), vargs);
   int _errno = lbfgs(x0, APOINTER(2), APOINTER(3), NIL, gtol, p, vargs);
   var_set(NEW_SYMBOL("*lbfgs-errno*"), NEW_NUMBER(_errno));
   double dowarn = 1;
   if (p && NUMBERP(htable_get(p, NEW_SYMBOL("warn"))))
      dowarn = Number(htable_get(p, NEW_SYMBOL("warn")));
   if (_errno!=0 && dowarn)
      fprintf(stderr, "*** Warning: lbfgs failed to converge (see *lbfgs-errno*)\n");
   return x0->backptr;
}

DX(xlbfgsS)
{
   if (arg_number < 5)
      ARG_NUMBER(5);

   index_t *x0 = copy_array(AINDEX(1));
   double gtol = ADOUBLE(5);
   htable_t *p = NULL;
   at *vargs = NIL;
   if (arg_number>5 && HTABLEP(APOINTER(6)))
      p = (htable_t *)Mptr(APOINTER(6));
   for (int i=arg_number; i>6; i--)
      vargs = new_cons(APOINTER(i), vargs);
   int _errno = lbfgs(x0, APOINTER(2), APOINTER(3), APOINTER(4), gtol, p, vargs);
   var_set(NEW_SYMBOL("*lbfgs-errno*"), NEW_NUMBER(_errno));
   double dowarn = 1;
   if (p && NUMBERP(htable_get(p, NEW_SYMBOL("warn"))))
      dowarn = Number(htable_get(p, NEW_SYMBOL("warn")));
   if (_errno!=0 && dowarn)
      fprintf(stderr, "*** Warning: lbfgs failed to converge (see *lbfgs-errno*)\n");
   return x0->backptr;
}

/* Interface to the line search routine MCSRCH.     */
/* The first four arguments get updated in-place,   */
/* the return value is an error code (see lbfgs.f). */
static int mcsrch(index_t *x0, index_t *fval, index_t *gval, index_t *stp, int *nfe, \
                  index_t *sd, at *f, at *g, htable_t *p, at *vargs)
{
   /* argument checking and setup */
   extern void mcsrch_(int *n, double *x, double *fx, double *gx, double *s, \
                       double *stp, double *ftol, double *xtol, int *maxfev, \
                       int *info, int *nfev, double *wa);

   ifn (IND_STTYPE(x0) == ST_DOUBLE)
      error(NIL, "x not an array of doubles", x0->backptr);
   ifn (index_contiguousp(x0))
      error(NIL, "x not a contiguous array", x0->backptr);
   ifn (IND_STTYPE(fval) == ST_DOUBLE)
      error(NIL, "fval not an array of doubles", fval->backptr);
   ifn (IND_STTYPE(gval) == ST_DOUBLE)
      error(NIL, "gval not an array of doubles", gval->backptr);
   ifn (index_contiguousp(gval))
      error(NIL, "gval not a contiguous array", gval->backptr);
   ifn (IND_STTYPE(stp) == ST_DOUBLE)
      error(NIL, "stp not an array of doubles", stp->backptr);
   ifn (shape_equalp(IND_SHAPE(x0), IND_SHAPE(gval)))
      error(NIL, "shape of x and gval don't match", NIL);
   ifn (shape_nelems(IND_SHAPE(fval)))
      error(NIL, "fval not a scalar", fval->backptr);
   ifn (shape_nelems(IND_SHAPE(stp)))
      error(NIL, "stp not a scalar", stp->backptr);
   ifn (IND_STTYPE(sd) == ST_DOUBLE)
      error(NIL, "sd not an array of double", sd->backptr);
   ifn (index_nelems(x0) == index_nelems(sd))
      error(NIL, "number of elements of x and sd don't match", NIL);
   ifn (index_contiguousp(sd))
      error(NIL, "sd not a contiguous array", sd->backptr);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", f);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", g);

   int n = index_nelems(x0);
   ifn (n>0)
      error(NIL, "x empty", x0->backptr);

   double *x = IND_ST(x0)->data;
   double *fx = IND_ST(fval)->data;
   double *gx = IND_ST(gval)->data;
   double *st = IND_ST(stp)->data;
   double *s = IND_ST(sd)->data;

   at *(*listeval_f)(at *, at *) = Class(f)->listeval;
   at *(*listeval_g)(at *, at *) = Class(g)->listeval;
   at *callf = new_cons(f, new_cons(x0->backptr, vargs));
   at *callg = new_cons(g, new_cons(gval->backptr, new_cons(x0->backptr, vargs)));

   htable_t *params = lbfgs_params();
   if (p) htable_update(params, p);
   lb3_.gtol = Number(htable_get(params, NEW_SYMBOL("ls-gtol")));
   lb3_.stpmin = Number(htable_get(params, NEW_SYMBOL("ls-stpmin")));
   lb3_.stpmax = Number(htable_get(params, NEW_SYMBOL("ls-stpmax")));
   int maxfev = Number(htable_get(params, NEW_SYMBOL("ls-maxfev")));

   double *wa = mm_blob(n*sizeof(double));
   double ftol = 1e-4;
   double xtol = eps(1);

   /* reverse communication loop */
   int info = 0;
   while (mcsrch_(&n, x, fx, gx, s, st, &ftol, &xtol, &maxfev, &info, nfe, wa), info==-1) {
      *fx = Number(listeval_f(Car(callf), callf));
      listeval_g(Car(callg), callg);
      if (break_attempt)
         break;
   };
   if (info == -1) {
      assert(break_attempt);
      info = 3;
   }
   
   return info;
}

DX(xmcsrch)
{
   if (arg_number < 7)
      ARG_NUMBER(7);

   htable_t *p = NULL;
   if (arg_number>7 && HTABLEP(APOINTER(8)))
      p = (htable_t *)Mptr(APOINTER(8));
   
   at *vargs = NIL;
   for (int i=arg_number; i>8; i--)
      vargs = new_cons(APOINTER(i), vargs);
   
   int nfe = 0;
   int _errno = mcsrch(AINDEX(1), AINDEX(2), AINDEX(3), AINDEX(4), &nfe,
                       AINDEX(5), APOINTER(6), APOINTER(7), p, vargs);
   var_set(NEW_SYMBOL("*mcsrch-errno*"), NEW_NUMBER(_errno));

#if 0
   double dowarn = 1;
   if (p && NUMBERP(htable_get(p, NEW_SYMBOL("warn"))))
      dowarn = Number(htable_get(p, NEW_SYMBOL("warn")));
   if (_errno!=1 && dowarn)
      fprintf(stderr, "*** Warning: lbfgs failed to converge (see *lbfgs-errno*)\n");
#endif

   return NEW_NUMBER(nfe);
}

void init_lbfgs_interface()
{
   dx_define("lbfgs-params", xlbfgs_params);
   dx_define("lbfgs", xlbfgs);
   dx_define("lbfgs*", xlbfgsS);
   dx_define("mcsrch", xmcsrch);
}

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
