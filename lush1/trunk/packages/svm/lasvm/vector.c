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
 * $Id: vector.c,v 1.2 2005-02-22 17:46:33 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "messages.h"
#include "vector.h"


#ifdef __cplusplus__
#define this  mythis
#define or    myor
#define and   myand
#endif

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
    lasvm_error("Function malloc() has returned zero\n");
  return p;
}


/* ------------------------------------- */
/* SIMPLE VECTORS */


lasvm_vector_t *
lasvm_vector_create(int size)
{
  lasvm_vector_t *v;
  ASSERT(size>1);
  v = (lasvm_vector_t*)xmalloc(sizeof(lasvm_vector_t) + (size-1)*sizeof(double));
  v->size = size;
  return v;
}

void 
lasvm_vector_destroy(lasvm_vector_t *v)
{
  v->size = 0;
  free(v);
}

double 
lasvm_vector_dot_product(lasvm_vector_t *v1, lasvm_vector_t *v2)
{
  int i;
  int n = min(v1->size, v2->size);
  double sum = 0;
  for (i=0; i<n; i++)
    sum += v1->data[i] * v2->data[i];
  return sum;
}



/* ------------------------------------- */
/* SPARSE VECTORS */


lasvm_sparsevector_t *
lasvm_sparsevector_create(void)
{
  lasvm_sparsevector_t *v;
  v = (lasvm_sparsevector_t*)xmalloc(sizeof(lasvm_sparsevector_t));
  v->size = 0;
  v->npairs = 0;
  v->pairs = 0;
  v->last = &v->pairs;
  return v;
}

void 
lasvm_sparsevector_clear(lasvm_sparsevector_t *v)
{
  lasvm_sparsevector_pair_t *p = v->pairs;
  while (p)
    {
      lasvm_sparsevector_pair_t *q = p->next;
      free(p);
      p = q;
    }
  v->size = 0;
  v->npairs = 0;
  v->pairs = NULL;
  v->last = &v->pairs;
}

void 
lasvm_sparsevector_destroy(lasvm_sparsevector_t *v)
{
  lasvm_sparsevector_clear(v);
  free(v);
}

double 
lasvm_sparsevector_get(lasvm_sparsevector_t *v, int index)
{
  lasvm_sparsevector_pair_t *p = v->pairs;
  ASSERT(index>=0);
  while (p && p->index < index)
    p = p->next;
  if (p && p->index == index)
    return p->data;
  return 0;
}

static void 
quickappend(lasvm_sparsevector_t *v, int index, double data)
{
  lasvm_sparsevector_pair_t *d;
  d = (lasvm_sparsevector_pair_t*)xmalloc(sizeof(lasvm_sparsevector_pair_t));
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
lasvm_sparsevector_set(lasvm_sparsevector_t *v, int index, double data)
{
  if (index >= v->size)
    {
      /* Quick append */
      quickappend(v, index, data);
    }
  else
    {
      /* Slow insert */
      lasvm_sparsevector_pair_t **pp = &v->pairs;
      lasvm_sparsevector_pair_t *p, *d;
      while ( (p=*pp) && (p->index<index) )
	pp = &(p->next);
      ASSERT(p);
      if (p && p->index == index)
	{
	  p->data = data;
	  return;
	}
      d = (lasvm_sparsevector_pair_t*)xmalloc(sizeof(lasvm_sparsevector_pair_t));
      d->next = p;
      d->index = index;
      d->data = data;
      *pp = d;
      v->npairs += 1;
    }
}

lasvm_sparsevector_t *
lasvm_sparsevector_combine(lasvm_sparsevector_t *v1, double c1,
			   lasvm_sparsevector_t *v2, double c2)
{
  
  lasvm_sparsevector_t *r;
  lasvm_sparsevector_pair_t *p1 = v1->pairs;
  lasvm_sparsevector_pair_t *p2 = v2->pairs;
  r = lasvm_sparsevector_create();
  
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
lasvm_sparsevector_dot_product(lasvm_sparsevector_t *v1, 
			       lasvm_sparsevector_t *v2)
{
  double sum = 0;
  lasvm_sparsevector_pair_t *p1 = v1->pairs;
  lasvm_sparsevector_pair_t *p2 = v2->pairs;
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


