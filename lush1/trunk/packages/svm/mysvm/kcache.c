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
 * $Id: kcache.c,v 1.6 2004-11-02 19:00:54 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "messages.h"
#include "kcache.h"

#ifndef min
# define min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
#endif

typedef struct row_s row_t;

struct head_s
{
  row_t *prev;
  row_t *next;
};

struct row_s 
{
  struct head_s lru;
  int i;
  int size;
  float diag;
  float data[1];
};

struct mysvm_kcache_s {
  mysvm_kernel_t func;
  void *closure;
  int maxsize;
  int cursize;
  int l;
  int *i2r;
  int *r2i;
  row_t **row;
  struct head_s lru;
};

static void *
xmalloc(int n)
{
  void *p = malloc(n);
  if (! p) 
    mysvm_error("Function malloc() has returned zero\n");
  return p;
}

static void *
xrealloc(void *ptr, int n)
{
  if (! ptr)
    ptr = malloc(n);
  else
    ptr = realloc(ptr, n);
  if (! ptr) 
    mysvm_error("Function realloc has returned zero\n");
  return ptr;
}

static void
xminsize(mysvm_kcache_t *self, int n)
{
  int ol = self->l;
  if (n > ol)
    {
      int nl = max(256,ol);
      while (nl < n)
	nl = nl + nl;
      self->i2r = (int*)xrealloc(self->i2r, nl * sizeof(int));
      self->r2i = (int*)xrealloc(self->r2i, nl * sizeof(int));
      self->row = (row_t**)xrealloc(self->row, nl * sizeof(row_t*));
      while (ol < nl)
	{
	  self->i2r[ol] = ol;
	  self->r2i[ol] = ol;
	  self->row[ol] = NULL;
	  ol++;
	}
      self->cursize += (nl - ol) * (2*sizeof(int) + sizeof(row_t*));
      self->l = nl;
    }
}

mysvm_kcache_t* 
mysvm_kcache_create(mysvm_kernel_t kernelfunc, void *closure)
{
  mysvm_kcache_t *self;
  self = (mysvm_kcache_t*)xmalloc(sizeof(mysvm_kcache_t));
  memset(self, 0, sizeof(mysvm_kcache_t));
  self->l = 0;
  self->func = kernelfunc;
  self->closure = closure;
  self->cursize = sizeof(mysvm_kcache_t);
  self->maxsize = 256*1024*1024;
  self->lru.prev = (row_t*) &self->lru;
  self->lru.next = (row_t*) &self->lru;
  return self;
}

void 
mysvm_kcache_destroy(mysvm_kcache_t *self)
{
  if (self)
    {
      int i;
      if (self->i2r)
	free(self->i2r);
      if (self->r2i)
	free(self->r2i);
      if (self->row)
	for (i=0; i<self->l; i++)
	  if (self->row[i])
	    free(self->row[i]);
      if (self->row)
	free(self->row);
      memset(self, 0, sizeof(mysvm_kcache_t));
      free(self);
    }
}

int *
mysvm_kcache_i2r(mysvm_kcache_t *self, int n)
{
  xminsize(self, n);
  return self->i2r;
}

int *
mysvm_kcache_r2i(mysvm_kcache_t *self, int n)
{
  xminsize(self, n);
  return self->r2i;
}

static void
xunlink(row_t *r)
{
  if (r->size > 0)
    {
      r->lru.prev->lru.next = r->lru.next;
      r->lru.next->lru.prev = r->lru.prev;
      r->lru.next = r->lru.prev = 0;
    }
}

static void
xlink(mysvm_kcache_t *self, row_t *r)
{
  if (r->size > 0)
    {
      r->lru.prev = (row_t*)&self->lru;
      r->lru.next = self->lru.next;
      r->lru.prev->lru.next = r;
      r->lru.next->lru.prev = r;
    }
}

static row_t *
xnewrow(mysvm_kcache_t *self, int i, int len)
{
  int sz = sizeof(row_t) + (len-1)*sizeof(float);
  row_t *r = (row_t*)xmalloc(sz);
  r->i = i;
  r->size = len;
  r->lru.prev = r->lru.next = 0;
  self->cursize += sz;
  return r;
}

static void
xfreerow(mysvm_kcache_t *self, row_t *r)
{
  int len = r->size;
  int sz = sizeof(row_t) + (len-1)*sizeof(float);
  free(r);
  self->cursize -= sz;
}

static void
xtruncate(mysvm_kcache_t *self, row_t *row, int n)
{
  row_t *r;
  int p;
  int i = row->i;
  ASSERT(n >= 0);
  ASSERT(n < row->size);
  ASSERT(row == self->row[i]);
  r = xnewrow(self, i, n);
  r->diag = row->diag;
  for (p=0; p<n; p++)
    r->data[p] = row->data[p];
  xunlink(row);
  self->row[i] = r;
  xlink(self, r);
  xfreerow(self, row);
}

static void
xswap(mysvm_kcache_t *self, int i1, int i2, int r1, int r2)
{
  row_t *r = self->lru.next;
  while (r != (row_t*)& self->lru)
    {
      row_t *next = r->lru.next;
      int n = r->size;
      int rr = self->i2r[r->i];
      if (r1 < n)
	{
	  if (r2 < n)
	    {
	      float t1 = r->data[r1];
	      float t2 = r->data[r2];
	      r->data[r1] = t2;
	      r->data[r2] = t1;
	    }
	  else if (rr == r2)
            {
              r->data[r1] = r->diag;
            }
          else
            {
              row_t *ar = self->row[i2];
              if (ar && rr < ar->size && rr != r1)
                r->data[r1] = ar->data[rr];
              else
                xtruncate(self, r, r1);
            }
	}
      else if (r2 < n)
        {
          if (rr == r1)
            {
              r->data[r2] = r->diag;
            }
          else 
            {
              row_t *ar = self->row[i1]; 
              if (ar && rr < ar->size && rr != r2)
                r->data[r2] = ar->data[rr];
              else
                xtruncate(self, r, r2);
            }
        }
      r = next;
    }
  self->r2i[r1] = i2;
  self->r2i[r2] = i1;
  self->i2r[i1] = r2;
  self->i2r[i2] = r1;
}

void 
mysvm_kcache_swap_rr(mysvm_kcache_t *self, int r1, int r2)
{
  xminsize(self, 1+max(r1,r2));
  xswap(self, self->r2i[r1], self->r2i[r2], r1, r2);
}

void 
mysvm_kcache_swap_ii(mysvm_kcache_t *self, int i1, int i2)
{
  xminsize(self, 1+max(i1,i2));
  xswap(self, i1, i2, self->i2r[i1], self->i2r[i2]);
}

void 
mysvm_kcache_swap_ri(mysvm_kcache_t *self, int r1, int i2)
{
  xminsize(self, 1+max(r1,i2));
  xswap(self, self->r2i[r1], i2, r1, self->i2r[i2]);
}

double 
mysvm_kcache_query(mysvm_kcache_t *self, int i, int j)
{
  int l = self->l;
  ASSERT(i>=0);
  ASSERT(j>=0);
  if (i<l && j<l)
    {
      /* check cache */
      row_t *r = self->row[i];
      if ((r = self->row[i]))
	{
	  int p = self->i2r[j];
	  if (p < r->size)
	    return r->data[p];
	  else if (i == j)
	    return r->diag;
	}
      else if ((r = self->row[j]))
	{
	  int p = self->i2r[i];
	  if (p < r->size)
	    return r->data[p];
	}
    }
  /* compute */
  return (*self->func)(i, j, self->closure);
}

static void 
xpurge(mysvm_kcache_t *self)
{
  if (self->cursize > self->maxsize)
    {
      row_t *r = self->lru.prev;
      while ( self->cursize > self->maxsize && r != self->lru.next )
	{
	  row_t *prev = r->lru.prev;
          xtruncate(self, r, 0);
	  r = prev;
	}
    }
}

float *
mysvm_kcache_query_row(mysvm_kcache_t *self, int i, int len)
{
  int olen = 0;
  row_t *r;
  row_t *nr;
  int q;
  int p;
  
  ASSERT(i>=0);
  ASSERT(len>0);
  if (i >= self->l || len>= self->l)
    xminsize(self, max(1+i,len));
  r = self->row[i];
  /* Fast path */
  if (r)
    {
      olen = r->size;
      if (len <= olen)
	{
	  xunlink(r);
	  xlink(self, r);
	  return r->data;
	}
    }
  /* Allocate */
  nr = xnewrow(self, i, len);
  if (r)
    {
      olen = r->size;
      nr->diag = r->diag;
      for (p=0; p<olen; p++)
	nr->data[p] = r->data[p];
      xunlink(r);
      xfreerow(self, r);
      self->row[i] = 0;
    }
  else
    {
      self->row[i] = 0;
      nr->diag = (*self->func)(i, i, self->closure);
    }
  /* Complete */
  q = self->i2r[i];
  for (p=olen; p<len; p++)
    {
      int j = self->r2i[p];
      r = self->row[j];
      if (r && q < r->size)
	nr->data[p] = r->data[q];
      else if (p != q)
	nr->data[p] = (*self->func)(i, j, self->closure);
      else 
        nr->data[p] = nr->diag;
    }
  /* Make sure the cache does not exceed the max size */
  xpurge(self);
  /* Return */
  self->row[i] = nr;
  xlink(self, nr);
  return nr->data;
}

void 
mysvm_kcache_set_maximum_size(mysvm_kcache_t *self, int entries)
{
  ASSERT(self);
  ASSERT(entries>0);
  self->maxsize = entries;
  xpurge(self);
}

int 
mysvm_kcache_get_maximum_size(mysvm_kcache_t *self)
{
  ASSERT(self);
  return self->maxsize;
}

int 
mysvm_kcache_get_current_size(mysvm_kcache_t *self)
{
  ASSERT(self);
  return self->cursize;
}

