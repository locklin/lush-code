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
 * $Id: kcache.c,v 1.4 2004-10-03 09:38:20 leonb Exp $
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
xminsize(mysvm_kcache_t *this, int n)
{
  int ol = this->l;
  if (n > ol)
    {
      int nl = max(256,ol);
      while (nl < n)
	nl = nl + nl;
      this->i2r = xrealloc(this->i2r, nl * sizeof(int));
      this->r2i = xrealloc(this->r2i, nl * sizeof(int));
      this->row = xrealloc(this->row, nl * sizeof(row_t*));
      while (ol < nl)
	{
	  this->i2r[ol] = ol;
	  this->r2i[ol] = ol;
	  this->row[ol] = NULL;
	  ol++;
	}
      this->cursize += (nl - ol) * (2*sizeof(int) + sizeof(row_t*));
      this->l = nl;
    }
}

mysvm_kcache_t* 
mysvm_kcache_create(mysvm_kernel_t kernelfunc, void *closure)
{
  mysvm_kcache_t *this;
  this = xmalloc(sizeof(mysvm_kcache_t));
  memset(this, 0, sizeof(mysvm_kcache_t));
  this->l = 0;
  this->func = kernelfunc;
  this->closure = closure;
  this->cursize = sizeof(mysvm_kcache_t);
  this->maxsize = 256*1024*1024;
  this->lru.prev = (row_t*) &this->lru;
  this->lru.next = (row_t*) &this->lru;
  return this;
}

void 
mysvm_kcache_destroy(mysvm_kcache_t *this)
{
  if (this)
    {
      int i;
      if (this->i2r)
	free(this->i2r);
      if (this->r2i)
	free(this->r2i);
      if (this->row)
	for (i=0; i<this->l; i++)
	  if (this->row[i])
	    free(this->row[i]);
      if (this->row)
	free(this->row);
      memset(this, 0, sizeof(mysvm_kcache_t));
      free(this);
    }
}

int *
mysvm_kcache_i2r(mysvm_kcache_t *this, int n)
{
  xminsize(this, n);
  return this->i2r;
}

int *
mysvm_kcache_r2i(mysvm_kcache_t *this, int n)
{
  xminsize(this, n);
  return this->r2i;
}

static void
xunlink(row_t *r)
{
  if (r->size > 0)
    {
      r->lru.prev->lru.next = r->lru.next;
      r->lru.next->lru.prev = r->lru.prev;
    }
}

static void
xlink(mysvm_kcache_t *this, row_t *r)
{
  if (r->size > 0)
    {
      r->lru.prev = (row_t*)&this->lru;
      r->lru.next = this->lru.next;
      r->lru.prev->lru.next = r;
      r->lru.next->lru.prev = r;
    }
}

static row_t *
xnewrow(mysvm_kcache_t *this, int i, int len)
{
  int sz = sizeof(row_t) + (len-1)*sizeof(float);
  row_t *r = xmalloc(sz);
  r->i = i;
  r->size = len;
  this->cursize += sz;
  return r;
}

static void
xfreerow(mysvm_kcache_t *this, row_t *r)
{
  int len = r->size;
  int sz = sizeof(row_t) + (len-1)*sizeof(float);
  free(r);
  this->cursize -= sz;
}

static void
xtruncate(mysvm_kcache_t *this, row_t *row, int n)
{
  row_t *r;
  int p;
  int i = row->i;
  ASSERT(n >= 0);
  ASSERT(n < row->size);
  ASSERT(row == this->row[i]);
  r = xnewrow(this, i, n);
  r->diag = row->diag;
  for (p=0; p<n; p++)
    r->data[p] = row->data[p];
  xunlink(row);
  this->row[i] = r;
  xfreerow(this, row);
  xlink(this, r);
}

static void
xswap(mysvm_kcache_t *this, int i1, int i2, int r1, int r2)
{
  row_t *r = this->lru.next;
  while (r != (row_t*)& this->lru)
    {
      row_t *next = r->lru.next;
      int n = r->size;
      int rr = this->i2r[r->i];
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
              row_t *or = this->row[i2];
              if (or && rr < or->size && rr != r1)
                r->data[r1] = or->data[rr];
              else
                xtruncate(this, r, r1);
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
              row_t *or = this->row[i1]; 
              if (or && rr < or->size && rr != r2)
                r->data[r2] = or->data[rr];
              else
                xtruncate(this, r, r2);
            }
        }
      r = next;
    }
  this->r2i[r1] = i2;
  this->r2i[r2] = i1;
  this->i2r[i1] = r2;
  this->i2r[i2] = r1;
}

void 
mysvm_kcache_swap_rr(mysvm_kcache_t *this, int r1, int r2)
{
  xminsize(this, 1+max(r1,r2));
  xswap(this, this->r2i[r1], this->r2i[r2], r1, r2);
}

void 
mysvm_kcache_swap_ii(mysvm_kcache_t *this, int i1, int i2)
{
  xminsize(this, 1+max(i1,i2));
  xswap(this, i1, i2, this->i2r[i1], this->i2r[i2]);
}

void 
mysvm_kcache_swap_ri(mysvm_kcache_t *this, int r1, int i2)
{
  xminsize(this, 1+max(r1,i2));
  xswap(this, this->r2i[r1], i2, r1, this->i2r[i2]);
}

double 
mysvm_kcache_query(mysvm_kcache_t *this, int i, int j)
{
  int l = this->l;
  ASSERT(i>=0);
  ASSERT(j>=0);
  if (i<l && j<l)
    {
      /* check cache */
      row_t *r = this->row[i];
      if ((r = this->row[i]))
	{
	  int p = this->i2r[j];
	  if (p < r->size)
	    return r->data[p];
	  else if (i == j)
	    return r->diag;
	}
      else if ((r = this->row[j]))
	{
	  int p = this->i2r[i];
	  if (p < r->size)
	    return r->data[p];
	}
    }
  /* compute */
  return (*this->func)(i, j, this->closure);
}

static void 
xpurge(mysvm_kcache_t *this)
{
  if (this->cursize > this->maxsize)
    {
      row_t *r = this->lru.prev;
      while ( this->cursize > this->maxsize && r != this->lru.next )
	{
	  row_t *prev = r->lru.prev;
          xtruncate(this, r, 0);
	  r = prev;
	}
    }
}

float *
mysvm_kcache_query_row(mysvm_kcache_t *this, int i, int len)
{
  int olen = 0;
  row_t *r;
  row_t *nr;
  int q;
  int p;
  
  ASSERT(i>=0);
  ASSERT(len>0);
  if (i >= this->l || len>= this->l)
    xminsize(this, max(1+i,len));
  r = this->row[i];
  /* Fast path */
  if (r)
    {
      olen = r->size;
      if (len <= olen)
	{
	  xunlink(r);
	  xlink(this, r);
	  return r->data;
	}
    }
  /* Allocate */
  xpurge(this);
  nr = xnewrow(this, i, len);
  if (r)
    {
      olen = r->size;
      nr->diag = r->diag;
      for (p=0; p<olen; p++)
	nr->data[p] = r->data[p];
      xunlink(r);
      xfreerow(this, r);
      this->row[i] = 0;
    }
  else
    {
      this->row[i] = 0;
      nr->diag = (*this->func)(i, i, this->closure);
    }
  /* Complete */
  q = this->i2r[i];
  for (p=olen; p<len; p++)
    {
      int j = this->r2i[p];
      r = this->row[j];
      if (r && q < r->size)
	nr->data[p] = r->data[q];
      else if (p != q)
	nr->data[p] = (*this->func)(i, j, this->closure);
      else 
        nr->data[p] = nr->diag;
    }
  /* Return */
  this->row[i] = nr;
  xlink(this, nr);
  return nr->data;
}

void 
mysvm_kcache_set_maximum_size(mysvm_kcache_t *this, int entries)
{
  ASSERT(this);
  ASSERT(entries>0);
  this->maxsize = entries;
  xpurge(this);
}

int 
mysvm_kcache_get_maximum_size(mysvm_kcache_t *this)
{
  ASSERT(this);
  return this->maxsize;
}

int 
mysvm_kcache_get_current_size(mysvm_kcache_t *this)
{
  ASSERT(this);
  return this->cursize;
}

