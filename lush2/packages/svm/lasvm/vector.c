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
 * $Id: vector.c,v 1.4 2006/04/04 22:01:24 leonb Exp $
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


static void
resize(lasvm_sparsevector_t *v, int n)
{
  int i;
  lasvm_sparsevector_pair_t *newpairs;
  if (n < v->size)
    n = v->size;
  newpairs = (lasvm_sparsevector_pair_t*)xmalloc(sizeof(lasvm_sparsevector_pair_t)*(n+1));
  for (i=0; i<v->size; i++)
    newpairs[i] = v->pairs[i];
  for (; i<=n; i++)
    newpairs[i].index = -1;
  if (v->pairs)
    free(v->pairs);
  v->pairs = newpairs;
  v->maxsize = n;
}

static lasvm_sparsevector_pair_t *
makeroom(lasvm_sparsevector_t *v)
{
  int n = v->size + v->size/2;
  if (n < v->size + 16)
    n = v->size + 16;
  resize(v, n);
  return v->pairs;
}


lasvm_sparsevector_t *
lasvm_sparsevector_create(void)
{
  lasvm_sparsevector_t *v;
  v = (lasvm_sparsevector_t*)xmalloc(sizeof(lasvm_sparsevector_t));
  v->size = 0;
  v->maxsize = 0;
  v->pairs = 0;
  makeroom(v);
  return v;
}

void 
lasvm_sparsevector_clear(lasvm_sparsevector_t *v)
{
  v->size = 0;
  resize(v, 0);
}

void 
lasvm_sparsevector_destroy(lasvm_sparsevector_t *v)
{
  if (v->pairs)
    free(v->pairs);
  v->pairs = 0;
  free(v);
}

static lasvm_sparsevector_pair_t *
search(lasvm_sparsevector_t *v, int index)
{
  int lo = 0;
  int hi = v->size - 1;
  lasvm_sparsevector_pair_t *pairs = v->pairs;
  while (lo <= hi)
    {
      int d = (lo + hi + 1) / 2;
      if (index == pairs[d].index)
        return &pairs[d];
      else if (index < pairs[d].index)
        hi = d-1;
      else
        lo = d+1;
    }
  return 0;
}

double 
lasvm_sparsevector_get(lasvm_sparsevector_t *v, int index)
{
  lasvm_sparsevector_pair_t *p = search(v, index);
  return (p) ? p->value : 0;
}

void 
lasvm_sparsevector_set(lasvm_sparsevector_t *v, int index, double value)
{
  int size = v->size;
  lasvm_sparsevector_pair_t *pairs = v->pairs;
  if (value)
    {
      if (size>0 && index>pairs[size-1].index && size<v->maxsize)
        {
          // quick append
          pairs[size].index = index;
          pairs[size].value = value;
          v->size = size + 1;
        }
      else
        {
          lasvm_sparsevector_pair_t *p = search(v, index);
          if (p)
            {
              // update
              p->value = value;
            }
          else
            {
              // insert
              int i;
              if (size >= v->maxsize)
                pairs = makeroom(v);
              for (i=size; i>0 && pairs[i-1].index>index; i--)
                pairs[i] = pairs[i-1];
              pairs[i].index = index;
              pairs[i].value = value;
              v->size = size + 1;
            }
        }
    }
  else
    {
      lasvm_sparsevector_pair_t *p = search(v, index);
      if (p) 
        {
          // remove
          int i;
          for (i=p-pairs; i<=size; i++)
            pairs[i] = pairs[i+1];
          v->size = size - 1;
        }
    }
}


lasvm_sparsevector_t *
lasvm_sparsevector_combine(lasvm_sparsevector_t *v1, double c1,
			   lasvm_sparsevector_t *v2, double c2)
{
  int k = 0;
  lasvm_sparsevector_pair_t *pairs;
  lasvm_sparsevector_pair_t *p1 = v1->pairs;
  lasvm_sparsevector_pair_t *p2 = v2->pairs;
  lasvm_sparsevector_t *r = lasvm_sparsevector_create();
  resize(r, v1->size + v2->size);
  pairs = r->pairs;
  while (p1->index>=0 && p2->index>=0)
    {
      if (p1->index < p2->index)
	{
          pairs[k].index = p1->index;
          pairs[k].value = c1 * p1->value;
          k++; p1++;
        }
      else if (p1->index > p2->index)
	{
          pairs[k].index = p2->index;
          pairs[k].value = c2 * p2->value;
          k++; p2++;
	}
      else
	{
          float value = c1 * p1->value + c2 * p2->value;
          if (value)
            {
              pairs[k].index = p1->index;
              pairs[k].value = value;
              k++;
            }
          p1++; p2++;
        }
    }
  while (p1->index>=0)
    {
      pairs[k].index = p1->index;
      pairs[k].value = c1 * p1->value;
      k++; p1++;
    }
  while (p2->index>=0)
    {
      pairs[k].index = p2->index;
      pairs[k].value = c2 * p2->value;
      k++; p2++;
    }
  r->size = k;
  resize(r, k);
  return r;
}


double 
lasvm_sparsevector_dot_product(lasvm_sparsevector_t *v1, 
			       lasvm_sparsevector_t *v2)
{
  double sum = 0;
  lasvm_sparsevector_pair_t *p1 = v1->pairs;
  lasvm_sparsevector_pair_t *p2 = v2->pairs;
  while (p1->index>=0 && p2->index>=0)
    {
      if (p1->index < p2->index)
	p1++;
      else if (p1->index > p2->index)
	p2++;
      else
	{
	  sum += p1->value * p2->value;
	  p1++;
          p2++;
	}
    }
  return sum;
}


