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

/* make a list from a vector of objects */
at *vector2list(int n, at **vec)
{
   MM_ENTER;
   at *l = NIL;
   at **where = &l;
   for (int i=0; i<n;i++) {
      *where = new_cons(vec[i], NIL);
      where = &Cdr(*where);
   }
   MM_RETURN(l);
}

DX(xlist)
{
   at *l = NIL;
   while (arg_number)
      l = new_cons(APOINTER(arg_number--), l);
   return l;
}


at *make_list(int n, at *v)
{
   at *ans = NIL;
   if (n < 0)
      RAISEF("value must be non-negative", NEW_NUMBER(n));
   if (n > 32767)
      RAISEF("value too large", NEW_NUMBER(n));

   MM_ENTER;
   while (n--)
      ans = new_cons(v, ans);
   MM_RETURN(ans);
}

DX(xmake_list)
{
   ARG_NUMBER(2);
   return make_list(AINTEGER(1), APOINTER(2));
}


at *car(at *q)
{
   ifn (LISTP(q))
      RAISEF("not a list", q);
   return q ? Car(q) : q;
}

DX(xcar)
{
   ARG_NUMBER(1);
   return car(ALIST(1));
}


at *cdr(at *q)
{
   ifn (LISTP(q))
      RAISEF("not a list", q);
   return q ? Cdr(q) : q;
}

DX(xcdr)
{
   ARG_NUMBER(1);
   return cdr(ALIST(1));
}


at *caar(at *q)
{
   return car(car(q));
}

DX(xcaar)
{
   ARG_NUMBER(1);
   return caar(ALIST(1));
}


at *cadr(at *q)
{
   return car(cdr(q));
}

DX(xcadr)
{
   ARG_NUMBER(1);
   return cadr(ALIST(1));
}


at *cdar(at *q)
{
   return cdr(car(q));
}

DX(xcdar)
{
   ARG_NUMBER(1);
   return cdar(ALIST(1));
}


at *cddr(at *q)
{
   return cdr(cdr(q));
}

DX(xcddr)
{
   ARG_NUMBER(1);
   return cddr(ALIST(1));
}


at *rplaca(at *q, at *p)
{
   if (CONSP(q))
      AssignCar(q, p);
   else
      RAISEF("not a cons", q);

   return q;
}

DX(xrplaca)
{
   ARG_NUMBER(2);
   return rplaca(APOINTER(1), APOINTER(2));
}


at *rplacd(at *q, at *p)
{
  if (CONSP(q))
     Cdr(q) = p;
  else
     RAISEF("not a cons", q);

  return q;
}

DX(xrplacd)
{
   ARG_NUMBER(2);
   return rplacd(APOINTER(1), APOINTER(2));
}


at *displace(at *q, at *p)
{
   ifn (CONSP(q))
      RAISEF("not a cons", q);
   ifn (CONSP(p))
      RAISEF("not a cons", p);
  
   AssignCar(q, Car(p));
   Cdr(q) = Cdr(p);
   return q;
}

DX(xdisplace)
{
   ARG_NUMBER(2);
   return displace(APOINTER(1), APOINTER(2));
}


at *copy_tree(at *p)
{
   MM_ENTER;
   
   if (CONSP(p)) {
      /* detect circular lists */
      at *p0 = p;
      bool move_p0 = false;
      at *q = NIL;
      at **qp = &q;
      while (CONSP(p)) {
         *qp = new_cons(Car(p), NIL);
         qp = &Cdr(*qp);
         p = Cdr(p);
         if (p == p0)
            RAISEF("can't do circular structures", NIL);
         move_p0 = !move_p0;
         if (move_p0)
            p0 = Cdr(p0);
      }
      *qp = copy_tree(p);

      /* descend */
      p = q;
      while (CONSP(p)) {
         AssignCar(p, copy_tree(Car(p)));
         p = Cdr(p);
      }
      MM_RETURN(q);
    
   } else
       MM_RETURN(p);
}

DX(xcopy_tree) 
{
   ARG_NUMBER(1);
   return copy_tree(APOINTER(1));
}


DX(xlistp)
{
   ARG_NUMBER(1);
   at *q = APOINTER(1);
   if (!q) {
      return t();
   } else if (CONSP(q)) {
      return q;
   } else
      return NIL;
}


int length(at *p)
{
   at *q = p;
   int i = 0;
   while (CONSP(p)) {
      i++;
      p = Cdr(p);
      if (p == q)
         return -1;
      if (i & 1)
         q = Cdr(q);
   }
   return i;
}

DX(xlength)
{
   ARG_NUMBER(1);
   if (ISINDEX(1)) {
      index_t *ind = AINDEX(1);
      if (IND_NDIMS(ind)<1)
         return NIL;
      else
         return NEW_NUMBER(IND_DIM(ind, 0));
      
   } else if (ISLIST(1)) {
      return NEW_NUMBER(length(ALIST(1)));
      
   } else if (ISSTRING(1)) {
      return NEW_NUMBER(strlen(ASTRING(1)));
      
   } else
      RAISEFX("object is not a sequence", APOINTER(1));
   return NIL; // make compiler happy
}


at *lasta(at *list)
{
   ifn (CONSP(list))
      return NIL;

   int toggle = 1;
   at *q = list;
   while (CONSP(Cdr(list))) {
      list = Cdr(list);
      if (list == q)
         RAISEF("circular list has no last element",NIL);
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   if (Cdr(list)) {
      return Cdr(list);
   } else {
      return Car(list);
   }
}

DX(xlasta)
{
   ARG_NUMBER(1);
   return lasta(ALIST(1));
}


at *lastcons(at *list)
{
   ifn (CONSP(list))
      return NIL;

   at *q = list;
   int toggle = 0;
   while (CONSP(Cdr(list))) {
      list = Cdr(list);
      if (list == q)
         RAISEF("circular list has no last element",NIL);
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   return list;
}

at *last(at *p, int n)
{
   if (p == NIL)
      return NIL;
   else if (n == 1)
      return lastcons(p);
   else if (n == 0)
      return Cdr(lastcons(p));  
   else {
      int l = length(p);
      if (n >= l)
         return p;
      else
         return nthcdr(p, l-n);
   }
}

DX(xlast)
{
   int n = 1;
   if (arg_number == 2)
      n = AINTEGER(2);
   else if (arg_number>2 || arg_number<1)
      ARG_NUMBER(-1);
   return last(ALIST(1), n);
}


at *member(at *elem, at *list)
{
   at *q = list;
   int toggle = 0;
   while (CONSP(list)) {
      if (eq_test(Car(list), elem))
         return list;
      list = Cdr(list);
      if (list == q)
         break;
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   return NIL;
}

DX(xmember)
{
   ARG_NUMBER(2);
   return member(APOINTER(1), ALIST(2));
}


at *append(at *l1, at *l2)
{
   MM_ENTER;
   at *answer = NIL;
   at **where = &answer;
   at *q = l1;
   int toggle = 0;

   while (CONSP(l1)) {
      *where = new_cons(Car(l1), NIL);
      where = &Cdr(*where);
      l1 = Cdr(l1);
      if (l1 == q) {
         RAISEF("cannot append to a circular list",NIL);
      }
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   if (l1 != NIL) {
      *where = l1;
      RAISEF("not a proper list", answer);
   }
   *where = l2;

   MM_RETURN(answer);
}

DX(xappend)
{
   if (arg_number == 0)
      return NIL;

   at *last = APOINTER(arg_number);
   while (--arg_number > 0)
      last = append(ALIST(arg_number), last);

   return last;
}


at *nfirst(int n, at *l)
{
   MM_ENTER;
   at *answer = NIL;
   at **where = &answer;

   while (CONSP(l) && n>0) {
      *where = new_cons(Car(l), NIL);
      where = &Cdr(*where);
      n--;
      l = Cdr(l);
   }
   if (l && n>0)
      *where = l;

   MM_RETURN(answer);
}

DX(xnfirst)
{
   ARG_NUMBER(2);
   return nfirst(AINTEGER(1), APOINTER(2));
}


at *nth(at *l, int n)
{
   while (n>0 && CONSP(l)) {
      CHECK_MACHINE("on");
      n--;
      l = Cdr(l);
   }
   if (n==0 && CONSP(l))
      return Car(l);
   else
      return NIL;
}

DX(xnth)
{
   ARG_NUMBER(2);
   return nth(ALIST(2), AINTEGER(1));
}


at *nthcdr(at *l, int n)
{
   while (n>0 && CONSP(l)) {
      CHECK_MACHINE("on");
      n--;
      l = Cdr(l);
   }
   if (n==0 && CONSP(l))
      return l;
   else
      return NIL;
}

DX(xnthcdr)
{
   ARG_NUMBER(2);
   return nthcdr(ALIST(2), AINTEGER(1));
}


at *reverse(at *l)
{
   MM_ENTER;
   at *q = l;
   int toggle = 0;
   at *r = NIL;
   while (CONSP(l)) {
      r = new_cons(Car(l), r);
      l = Cdr(l);
      if (l == q) {
         RAISEF("cannot reverse a circular list",NIL);
      }
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   MM_RETURN(r);
}

DX(xreverse)
{
   ARG_NUMBER(1);
   return reverse(ALIST(1));
}


static at **flat1(at *l, at **where)
{
   at *slow = l;
   bool toggle = 0;
   struct recur_elt elt;
   
   while (CONSP(l)) {
      ifn (recur_push_ok(&elt, (void *)&flat1, Car(l)))
         error("flatten", "cannot flatten circular list",NIL);
      where = flat1(Car(l), where);
      recur_pop(&elt);
      l = Cdr(l);
      if (l==slow)
         error("flatten", "cannot flatten circular list",NIL);
      toggle = !toggle;
      if (toggle)
         slow = Cdr(slow);
   }
   if (l) {
      *where = new_cons(l, NIL);
      return &Cdr(*where);
   } else
      return where;
}

at *flatten(at *l)
{
   MM_ENTER;
   at *ans = NIL;
   flat1(l, &ans);
   MM_RETURN(ans);
}

DX(xflatten)
{
   ARG_NUMBER(1);
   return flatten(APOINTER(1));
}


at *assoc(at *k, at *l)
{
   at *q = l;
   int toggle = 0;
   while (CONSP(l)) 
   {
      at *p = Car(l);
      if (CONSP(p) && eq_test(Car(p),k))
         return p;

      l = Cdr(l);
      if (l==q)
         break;
      toggle ^= 1;
      if (toggle)
         q = Cdr(q);
   }
   return NIL;
}

DX(xassoc)
{
   ARG_NUMBER(2);
   return assoc(APOINTER(1),APOINTER(2));
}


void init_list(void)
{
   dx_define("car", xcar);
   dx_define("cdr", xcdr);
   dx_define("caar", xcaar);
   dx_define("cadr", xcadr);
   dx_define("cdar", xcdar);
   dx_define("cddr", xcddr);
   dx_define("rplaca", xrplaca);
   dx_define("rplacd", xrplacd);
   dx_define("displace", xdisplace);
   dx_define("list", xlist);
   dx_define("make-list", xmake_list);
   dx_define("listp", xlistp);
   dx_define("copy-tree", xcopy_tree);
   dx_define("length", xlength);
   dx_define("lasta", xlasta);
   dx_define("last", xlast);
   dx_define("member", xmember);
   dx_define("append", xappend);
   dx_define("nfirst", xnfirst);
   dx_define("nth", xnth);
   dx_define("nthcdr", xnthcdr);
   dx_define("reverse", xreverse);
   dx_define("flatten", xflatten);
   dx_define("assoc", xassoc);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
