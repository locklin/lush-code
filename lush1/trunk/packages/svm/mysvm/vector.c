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
 * $Id: vector.c,v 1.1 2004-08-30 22:07:28 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "messages.h"
#include "vector.h"


#ifndef min
# define min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
#endif

static void* 
xmalloc(int n)
{
  void *p = malloc(n);
  if (! p) 
    mysvm_error("Function malloc() has returned zero\n");
  return p;
}


/* ------------------------------------- */
/* SIMPLE VECTORS */


mysvm_vector_t *
mysvm_vector_create(int size)
{
  mysvm_vector_t *v;
  ASSERT(size>1);
  v = xmalloc(sizeof(mysvm_vector_t) + (size-1)*sizeof(double));
  v->size = size;
  return v;
}

void 
mysvm_vector_destroy(mysvm_vector_t *v)
{
  v->size = 0;
  free(v);
}

double 
mysvm_vector_dot_product(mysvm_vector_t *v1, mysvm_vector_t *v2)
{
  int i;
  int n = min(v1->size, v2->size);
  double sum = 0;
  for (i=0; i<n; i++)
    sum += v1->data[i] * v2->data[i];
  return sum;
}


double 
mysvm_vector_l2_distance(mysvm_vector_t *v1, mysvm_vector_t *v2)
{
  int i;
  int n = min(v1->size, v2->size);
  double sum = 0;
  for (i=0; i<n; i++)
    {
      double dif = v1->data[i] - v2->data[i];
      sum += dif * dif;
    }
  return sum;
}




/* ------------------------------------- */
/* SPARSE VECTORS */


mysvm_sparsevector_t *
mysvm_sparsevector_create(void)
{
  mysvm_sparsevector_t *v;
  v = xmalloc(sizeof(mysvm_sparsevector_t));
  v->size = 0;
  v->npairs = 0;
  v->pairs = 0;
  v->last = &v->pairs;
  return v;
}

void 
mysvm_sparsevector_destroy(mysvm_sparsevector_t *v)
{
  mysvm_sparsevector_pair_t *p = v->pairs;
  while (p)
    {
      mysvm_sparsevector_pair_t *q = p->next;
      free(p);
      p = q;
    }
  v->size = 0;
  v->npairs = 0;
  v->pairs = NULL;
  v->last = NULL;
  free(v);
}

double 
mysvm_sparsevector_get(mysvm_sparsevector_t *v, int index)
{
  mysvm_sparsevector_pair_t *p = v->pairs;
  ASSERT(index>=0 && index<v->size);
  while (p && p->index < index)
    p = p->next;
  if (p->index == index)
    return p->data;
  return 0;
}

static void 
quickappend(mysvm_sparsevector_t *v, int index, double data)
{
  mysvm_sparsevector_pair_t *d = xmalloc(sizeof(mysvm_sparsevector_pair_t));
  ASSERT(index >= v->size);
  d->next = 0;
  d->index = index;
  d->data = data;
  *(v->last) = d;
  v->last = &(d->next);
  v->size = index + 1;
  v->npairs += 1;
}

void 
mysvm_sparsevector_set(mysvm_sparsevector_t *v, int index, double data)
{
  if (index >= v->size)
    {
      /* Quick append */
      quickappend(v, index, data);
    }
  else
    {
      /* Slow insert */
      mysvm_sparsevector_pair_t **pp = &v->pairs;
      mysvm_sparsevector_pair_t *p, *d;
      while ( (p=*pp) && (p->index<index) )
	pp = &(p->next);
      ASSERT(p);
      if (p->index==index)
	{
	  p->data = data;
	  return;
	}
      d = xmalloc(sizeof(mysvm_sparsevector_pair_t));
      d->next = p;
      d->index = index;
      d->data = data;
      *pp = d;
      v->npairs += 1;
    }
}

mysvm_sparsevector_t *
mysvm_sparsevector_combine(mysvm_sparsevector_t *v1, double c1,
			   mysvm_sparsevector_t *v2, double c2)
{
  
  mysvm_sparsevector_t *r;
  mysvm_sparsevector_pair_t *p1 = v1->pairs;
  mysvm_sparsevector_pair_t *p2 = v2->pairs;
  r = mysvm_sparsevector_create();
  
  while (p1 && p2)
    {
      if (p1->index < p2->index)
	{
	  quickappend(r, p1->index, c1*p1->data);
	  p1 = p1->next;
	}
      else if (p1->index > p2->index)
	{
	  quickappend(r, p2->index, c2*p2->data);
	  p2 = p2->next;
	}
      else
	{
	  quickappend(r, p1->index, c1*p1->data + c2*p2->data);
	  p1 = p1->next;
	  p2 = p2->next;
	}
    }
  while (p1)
    {
      quickappend(r, p1->index, c1*p1->data);
      p1 = p1->next;
    }
  while (p2)
    {
      quickappend(r, p2->index, c2*p2->data);
      p2 = p2->next;
    }
  return r;
}

double 
mysvm_sparsevector_dot_product(mysvm_sparsevector_t *v1, 
			       mysvm_sparsevector_t *v2)
{
  double sum = 0;
  mysvm_sparsevector_pair_t *p1 = v1->pairs;
  mysvm_sparsevector_pair_t *p2 = v2->pairs;
  while (p1 && p2)
    {
      if (p1->index < p2->index)
	p1 = p1->next;
      else if (p1->index > p2->index)
	p2 = p2->next;
      else
	{
	  sum += p1->data * p2->data;
	  p1 = p1->next;
	  p2 = p2->next;
	}
    }
  return sum;
}

double 
mysvm_sparsevector_l2_distance(mysvm_sparsevector_t *v1, 
			       mysvm_sparsevector_t *v2)
{
  double sum = 0;
  mysvm_sparsevector_pair_t *p1 = v1->pairs;
  mysvm_sparsevector_pair_t *p2 = v2->pairs;
  while (p1 && p2)
    {
      if (p1->index < p2->index)
	p1 = p1->next;
      else if (p1->index > p2->index)
	p2 = p2->next;
      else
	{
	  double dif = p1->data - p2->data;
	  sum += dif * dif;
	  p1 = p1->next;
	  p2 = p2->next;
	}
    }
  return sum;
}

