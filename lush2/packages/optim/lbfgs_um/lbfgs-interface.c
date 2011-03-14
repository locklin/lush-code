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

   /* these control line search behavior */
   htable_set(p, NEW_SYMBOL("ls-gtol"), NEW_NUMBER(lb3_.gtol));
   htable_set(p, NEW_SYMBOL("ls-stpmin"), NEW_NUMBER(lb3_.stpmin));
   htable_set(p, NEW_SYMBOL("ls-stpmax"), NEW_NUMBER(lb3_.stpmax));

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
static int lbfgs(index_t *x0, at *f, at *g, double gtol, htable_t *p, at *vargs)
{
   /* argument checking and setup */

   extern void lbfgs_(int *n, int *m, double *x, double *fval, double *gval, \
                      int *diagco, double *diag, int iprint[2], double *gtol, \
                      double *xtol, double *w, int *iflag);

   ifn (IND_STTYPE(x0) == ST_DOUBLE)
      error(NIL, "not an array of doubles", x0->backptr);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", f);
   ifn (Class(f)->listeval)
      error(NIL, "not a function", g);
   ifn (gtol > 0)
      error(NIL, "threshold value not positive", NEW_NUMBER(gtol));
   
   at *gx = copy_array(x0)->backptr;
   at *(*listeval_f)(at *, at *) = Class(f)->listeval;
   at *(*listeval_g)(at *, at *) = Class(g)->listeval;
   at *callf = new_cons(f, new_cons(x0->backptr, vargs));
   at *callg = new_cons(g, new_cons(gx, new_cons(x0->backptr, vargs)));

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
   int diagco = false;
   double *diag = mm_blob(n*sizeof(double));
   double *w = mm_blob((n*(m+m+1)+m+m)*sizeof(double));
   double xtol = eps(1); /* machine precision */
   int iflag = 0;

   ifn (n>0)
      error(NIL, "empty array", x0->backptr);
   ifn (m>0)
      error(NIL, "m-parameter must be positive", NEW_NUMBER(m));

   int iter = 0;
   /* reverse communication loop */
   do {
      iter++;
      fval = Number(listeval_f(Car(callf), callf));
      listeval_g(Car(callg), callg);
      lbfgs_(&n, &m, x, &fval, gval, &diagco, diag, iprint, &gtol, &xtol, w, &iflag);
      assert(iflag<2);
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
   int _errno = lbfgs(x0, APOINTER(2), APOINTER(3), gtol, p, vargs);
   var_set(NEW_SYMBOL("*lbfgs-errno*"), NEW_NUMBER(_errno));
   if (_errno != 0)
      fprintf(stderr, "*** Warning: lbfgs failed to converge (see *lbfgs-errno*)\n");
   return x0->backptr;
}


void init_lbfgs_interface()
{
   dx_define("lbfgs-params", xlbfgs_params);
   dx_define("lbfgs", xlbfgs);
}

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
