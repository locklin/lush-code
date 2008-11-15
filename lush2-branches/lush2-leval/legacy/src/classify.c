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
 * $Id: classify.c,v 1.3 2006/04/10 20:47:38 leonb Exp $
 **********************************************************************/

/***********************************************************************
	statistic functions: mean sdev hamming_norm
	small mathematic functions: solve
********************************************************************** */

#include "header.h"


#define iter_list(l,p)							\
	for ( ;l;l=l->Cdr )						\
		ifn (CONSP(l) && (p=l->Car) && (p->flags & C_NUMBER))	\
			error(NIL,nonnumlist,NIL);			\
		else

#define iter_2_list(l1,p1,l2,p2)					     \
	for ( ;l1 || l2; l1=l1->Cdr,l2=l2->Cdr )			     \
		ifn ( CONSP(l1) && CONSP(l2) && (p1=l1->Car) && (p2=l2->Car) \
		     && (p1->flags & C_NUMBER) && (p2->flags & C_NUMBER) )   \
		     error (NIL, (l1 && l2) ? nonnumlist : notsamelen, NIL );\
		else

static char nonnumlist[] = "numeric list expected";
static char notsamelen[] = "listes have different length";


/* --------- usual functions --------- */


real 
mean(at *l)
{
  at *p;
  real sum;
  int len;

  sum = 0;
  len = 0;

  iter_list(l, p) {
    CHECK_MACHINE("on");
    len++;
    sum += p->Number;
  }
  if (len)
    return sum / len;
  else
    error(NIL,"Division by zero",NIL);
}

DX(xmean)
{
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  return NEW_NUMBER(mean(ACONS(1)));
}




static real 
moment2(at *l1, at *l2)
{
  at *p1, *p2;
  real sum;
  int len;

  sum = 0;
  len = 0;

  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    len++;
    sum += p1->Number * p2->Number;
  }
  if (len)
    return sum / len;
  else
    error(NIL,"Division by zero",NIL);
}

real 
cov(at *l1, at *l2)
{
  return moment2(l1, l2) - mean(l1) * mean(l2);
}

DX(xcov)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return NEW_NUMBER(cov(ACONS(1), ACONS(2)));
}

real 
sdev(at *l)
{
  real xmean;
  xmean = mean(l);
/* Due to finite FPU precision, moment2(l,l) might 
   be smaller than xmean^2 ; e.g. for a list of 10
   numbers 4.56 on a sun4 */
  {
    real square_diff = moment2(l, l) - xmean * xmean;
    if (square_diff < 0) {
      return((real)0.0);
    } else {
      return(sqrt(square_diff));
    }
  }
#if 0
  return sqrt(moment2(l, l) - xmean * xmean);
#endif /* 0 */
}

DX(xsdev)
{
  ARG_NUMBER(1);
  ALL_ARGS_EVAL;
  return NEW_NUMBER(sdev(ACONS(1)));
}



/* --------- median, fractiles, etc... --------- */

static real *
list_to_vector(at *l, int *pn)
{
  at *p, *q;
  int i;
  real *v;

  i = 0;
  q = l;
  iter_list(q, p) {
    CHECK_MACHINE("on");
    i++;
  }
  if (i==0)
    error(NIL,"Empty list",l);
  *pn = i;
  v = malloc(i*sizeof(real));
  i = 0;
  q = l;
  iter_list(q, p) {
    v[i++] = p->Number;
  }
  return v;
}


static real
ksmallest(real *v, unsigned int n, unsigned int k)
{
  int lo = 0;
  int hi = n-1;
#define swap(x,y) { real tmp; tmp=x; x=y; y=tmp; }
  while (lo<hi)
    {
      int m,l,h;
      real pivot;
      /* Sort v[lo], v[m], v[hi] by insertion */
      m = (lo+hi)/2;
      if (v[lo]>v[m])
        swap(v[lo],v[m]);
      if (v[m]>v[hi]) {
        swap(v[m],v[hi]);
        if (v[lo]>v[m])
          swap(v[lo],v[m]);
      }
      /* Extract pivot, place sentinel */
      pivot = v[m];
      v[m] = v[lo+1];
      v[lo+1] = v[lo];
      v[lo] = pivot;
      /* Partition */
      l = lo;
      h = hi;
     loop:
      do ++l; while (v[l]<pivot);
      do --h; while (v[h]>pivot);
      if (l < h) { 
        swap(v[l],v[h]); 
        goto loop; 
      }
      /* Finish up */
      if (k <= h)
        hi = h; 
      else if (k >= l)
        lo = l;
      else
        break;
    }
  return v[k];
#undef swap
}

DX(xmedian)
{
  real x;
  real *v;
  int n;
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  v = list_to_vector(APOINTER(1),&n);
  x = ksmallest(v, n, (n-1)/2);
  free(v);
  return NEW_NUMBER(x);
}

DX(xksmallest)
{
  real x;
  real *v;
  int n,k;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  v = list_to_vector(APOINTER(1),&n);
  k = AINTEGER(2) - 1;
  if (k<0 || k>=n) 
    error(NIL,"Argument is out of bounds", APOINTER(2));
  x = ksmallest(v, n, k);
  free(v);
  return NEW_NUMBER(x);
}

DX(xquantile)
{
  real x;
  real *v;
  int n,k;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  v = list_to_vector(APOINTER(1),&n);
  x = AREAL(2);
  if (x<0 || x>1)
    error(NIL,"illegal fractile",APOINTER(2));
  k = (int)floor(n*x);
  if (k>=n) k=n-1;
  x = ksmallest(v, n, k);
  free(v);
  return NEW_NUMBER(x);
}




/* --------- linear regression --------- */


static real lrA, lrB, lrR2;

static real 
regression(at *xl, at *yl)
{
  real mx, my, c;

  mx = mean(xl);
  my = mean(yl);
  c = moment2(xl, yl) - mx * my;
  lrA = c / (moment2(xl, xl) - mx * mx);
  lrB = my - lrA * mx;
  lrR2 = lrA * c / (moment2(yl, yl) - my * my);
  return lrR2;
}

DX(xregression)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  regression(ACONS(1), ACONS(2));
  return cons(NEW_NUMBER(lrA),
	      cons(NEW_NUMBER(lrB),
		   cons(NEW_NUMBER(sqrt(lrR2)), NIL)));
}


/* --------- list operations --------- */

/* real  */
/* sum(at *l) */
/* { */
/*   real answer = 0.0; */
/*   at *p; */

/*   iter_list(l, p) { */
/*     CHECK_MACHINE("on"); */
/*     answer += p->Number; */
/*   } */
/*   return answer; */
/* } */

/* DX(xsum) */
/* { */
/*   ARG_NUMBER(1); */
/*   ARG_EVAL(1); */
/*   return NEW_NUMBER(sum(ALIST(1))); */
/* } */

/* real  */
/* sup(at *l) */
/* { */
/*   real answer = 0; */
/*   int flag = 0; */
/*   at *p; */

/*   iter_list(l, p) { */
/*     CHECK_MACHINE("on"); */
/*     if (!flag) { */
/*       answer = p->Number; */
/*       flag = 1; */
/*     } else if (p->Number > answer) */
/*       answer = p->Number; */
/*   } */
/*   return answer; */
/* } */

/* DX(xsup) */
/* { */
/*   ARG_NUMBER(1); */
/*   ARG_EVAL(1); */
/*   return NEW_NUMBER(sup(ACONS(1))); */
/* } */

/* real  */
/* inf(at *l) */
/* { */
/*   real answer = 0; */
/*   int flag = 0; */
/*   at *p; */

/*   iter_list(l, p) { */
/*     CHECK_MACHINE("on"); */
/*     if (!flag) { */
/*       answer = p->Number; */
/*       flag = 1; */
/*     } else if (p->Number < answer) */
/*       answer = p->Number; */
/*   } */
/*   return answer; */
/* } */

/* DX(xinf) */
/* { */
/*   ARG_NUMBER(1); */
/*   ARG_EVAL(1); */
/*   return NEW_NUMBER(inf(ACONS(1))); */
/* } */

static at *
diff_listes(at *l1, at *l2)
{
  at **where;
  at *answer;
  at *p1, *p2;

  answer = NIL;
  where = &answer;
  iter_2_list(l1, p1, l2, p2) {
    *where = cons(NEW_NUMBER(p1->Number - p2->Number), NIL);
    where = &((*where)->Cdr);
    CHECK_MACHINE("on");
  }
  return answer;
}

DX(xdiff_listes)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return diff_listes(ALIST(1), ALIST(2));
}

static at *
add_listes(at *l1, at *l2)
{
  at **where;
  at *answer;
  at *p1, *p2;

  answer = NIL;
  where = &answer;
  iter_2_list(l1, p1, l2, p2) {
    *where = cons(NEW_NUMBER(p1->Number + p2->Number), NIL);
    where = &((*where)->Cdr);
    CHECK_MACHINE("on");
  }
  return answer;
}

DX(xadd_listes)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return add_listes(ALIST(1), ALIST(2));
}

at *
find_interval(at *l, real target, real width)
{
  at **where;
  at *answer;
  int num;
  at *p;

  where = &answer;
  answer = NIL;
  num = 0;

  iter_list(l, p) {
    CHECK_MACHINE("on");
    if (p->Number >= target - width && p->Number <= target + width) {
      *where = cons(NEW_NUMBER(num), NIL);
      where = &((*where)->Cdr);
    }
    num++;
  }
  return answer;
}

DX(xfind_interval)
{
  real width;

  ALL_ARGS_EVAL;
  width = 0.0;
  if (arg_number != 2) {
    ARG_NUMBER(3);
    width = AREAL(3);
  }
  return find_interval(ALIST(1), AREAL(2), width);
}

/* --------- list normes & distances --------- */


real 
sup_norm(at *l)
{
  at *p;
  real ans = 0;
  real this;
  int flag = 0;

  iter_list(l, p) {
    CHECK_MACHINE("on");
    this = fabs(p->Number);
    if (!flag) {
      ans = this;
      flag = 1;
    } else if (this > ans)
      ans = this;
  }
  return ans;
}

real 
sup_dist(at *l1, at *l2)
{
  at *p1, *p2;
  real ans = 0;
  real this;
  int flag = 0;

  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    this = fabs(p1->Number - p2->Number);
    if (!flag) {
      ans = this;
      flag = 1;
    } else if (this > ans)
      ans = this;
  }
  return ans;
}

DX(xsup_dist)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 1:
    return NEW_NUMBER(sup_norm(ACONS(1)));
  case 2:
    return NEW_NUMBER(sup_dist(ACONS(1), ACONS(2)));
  default:
    ARG_NUMBER(-1);
  }
  return NIL;
}


real 
abs_norm(at *l)
{
  at *p;
  real ans;

  ans = 0.0;
  iter_list(l, p) {
    CHECK_MACHINE("on");
    ans += fabs(p->Number);
  }
  return ans;
}

real 
abs_dist(at *l1, at *l2)
{
  at *p1, *p2;
  real ans;

  ans = 0.0;
  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    ans += fabs(p1->Number - p2->Number);
  }
  return ans;
}

DX(xabs_dist)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 1:
      return NEW_NUMBER(abs_norm(ACONS(1)));
    case 2:
      return NEW_NUMBER(abs_dist(ACONS(1), ACONS(2)));
    default:
      ARG_NUMBER(-1);
  }
  return NIL;
}

real 
sqr_norm(at *l)
{
  at *p;
  real ans;

  ans = 0.0;
  iter_list(l, p) {
    CHECK_MACHINE("on");
    ans += p->Number * p->Number;
  }
  return ans;
}

real 
sqr_dist(at *l1, at *l2)
{
  at *p1, *p2;
  real ans, aux;

  ans = 0.0;
  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    aux = p1->Number - p2->Number;
    ans += aux * aux;
  }
  return ans;
}

DX(xsqr_dist)
{
  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 1:
      return NEW_NUMBER(sqr_norm(ACONS(1)));
    case 2:
      return NEW_NUMBER(sqr_dist(ACONS(1), ACONS(2)));
    default:
      ARG_NUMBER(-1);
  }
  return NIL;
}



real 
hamming_norm(at *l, double margin)
{
  at *p;
  real ans;

  ans = 0.0;
  iter_list(l, p) {
    CHECK_MACHINE("on");
    if (fabs(p->Number) > margin)
      ans += 1.0;
  }
  return ans;
}

real 
hamming_dist(at *l1, at *l2, double margin)
{
  at *p1, *p2;
  real ans;

  ans = 0.0;
  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    if (fabs(p1->Number - p2->Number) > margin)
      ans += 1.0;
  }
  return ans;
}

DX(xhamming_dist)
{
  real margin = 0.0;

  ALL_ARGS_EVAL;
  if (ISNUMBER(1)) {
    margin = AREAL(1);
    arg_number--;
    arg_array++;
  }
  switch (arg_number) {
    case 1:
      return NEW_NUMBER(hamming_norm(ACONS(1), margin));
    case 2:
      return NEW_NUMBER(hamming_dist(ACONS(1), ACONS(2), margin));
    default:
      ARG_NUMBER(-1);
  }
  return NIL;
}



real 
quadrant_dist(at *l1, at *l2)
{
  at *p1, *p2;
  real ans;

  ans = 0.0;
  iter_2_list(l1, p1, l2, p2) {
    CHECK_MACHINE("on");
    if (p1->Number > 0 && p2->Number <= 0)
      ans += 1.0;
    else if (p1->Number < 0 && p2->Number >= 0)
      ans += 1.0;
  }
  return ans;
}

DX(xquadrant_dist)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  return NEW_NUMBER(quadrant_dist(ACONS(1), ACONS(2)));
}


/* --------- SOLVE --------- */

real 
solve(real x1, real x2, real (*f)(real))
{
  real x, y, y1, y2;

  y1 = (*f) (x1);
  y2 = (*f) (x2);

  if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
    error("solve", "bad search range", NIL);

  while (fabs(x1 - x2) > (1e-7) * fabs(x1 + x2)) {
    x = (x1 + x2) / 2;
    y = (*f) (x);

    if ((y1 >= 0 && y <= 0) || (y1 <= 0 && y >= 0)) {
      x2 = x;
      y2 = y;
    } else {
      x1 = x;
      y1 = y;
    }
  }
  return x1;
}

static at *solve_function;

real 
solve_call(real x)
{
  at *expr, *ans;

  LOCK(solve_function);
  expr = cons(solve_function, cons(NEW_NUMBER(x), NIL));
  ans = eval(expr);
  x = ans->Number;
  UNLOCK(expr);
  UNLOCK(ans);
  return x;
}

DX(xsolve)
{
  at *p;


  ARG_NUMBER(3);
  ALL_ARGS_EVAL;

  p = APOINTER(3);
  ifn(p->flags & X_FUNCTION)
    error(NIL, "not a function", p);
  else
  solve_function = p;

  p = NEW_NUMBER(solve(AREAL(1), AREAL(2), solve_call));
  solve_function = NIL;
  return p;
}


/* --------- INITIALISATION CODE --------- */

void 
init_classify(void)
{
  dx_define("mean", xmean);
  dx_define("cov", xcov);
  dx_define("sdev", xsdev);
  dx_define("median", xmedian);
  dx_define("ksmallest", xksmallest);
  dx_define("quantile", xquantile);
  dx_define("regression", xregression);
  dx_define("solve", xsolve);
  /* dx_define("sum", xsum); */
  dx_define("diff-listes", xdiff_listes);
  dx_define("add-listes", xadd_listes);
  dx_define("find-interval", xfind_interval);
  dx_define("sup-dist", xsup_dist);
  dx_define("abs-dist", xabs_dist);
  dx_define("sqr-dist", xsqr_dist);
  dx_define("hamming-dist", xhamming_dist);
  dx_define("quadrant-dist", xquadrant_dist);
}
