#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "messages.h"
#include "kcache.h"
#include "lasvm.h"

#ifndef min
# define min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef FLT_MAX
# define FLT_MAX 1e+20
#endif
#ifndef FLT_EPSILON
# define FLT_EPSILON 1e-6
#endif


static void *
xmalloc(int n)
{
  void *p = malloc(n);
  if (! p) 
    lasvm_error("Function malloc() has returned NULL.\n");
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
    lasvm_error("Function realloc() has returned NULL.\n");
  return ptr;
}

struct lasvm_s 
{
  lasvm_kcache_t *kernel;
  int     sumflag;
  double  cp;
  double  cn;
  int     maxl;
  int     s;
  int     l;
  double *alpha;
  double *cmin;
  double *cmax;
  double *g;
  double  gmin, gmax;
  int     imin, imax;
  int     minmaxflag;
};

static void
checksize(lasvm_t *self, int l)
{
  int maxl = max(l,256);
  while (maxl < l)
    maxl += maxl;
  self->alpha = (double*)xrealloc(self->alpha, maxl*sizeof(double));
  self->cmin = (double*)xrealloc(self->cmin, maxl*sizeof(double));
  self->cmax = (double*)xrealloc(self->cmax, maxl*sizeof(double));
  self->g = (double*)xrealloc(self->g, maxl*sizeof(double));
  self->maxl = maxl;
}

lasvm_t *
lasvm_create( lasvm_kcache_t *cache,
              int sumflag, double cp, double cn )
{
  lasvm_t *self = (lasvm_t*)xmalloc(sizeof(lasvm_t));
  memset(self, 0, sizeof(lasvm_t));
  self->kernel = cache;
  self->cp = cp;
  self->cn = cn;
  self->sumflag = sumflag;
  checksize(self, 0);
  return self;
}

void 
lasvm_destroy( lasvm_t *self )
{
  if (self->alpha) free(self->alpha);
  if (self->cmin) free(self->cmin);
  if (self->cmax) free(self->cmax);
  if (self->g) free(self->g);
  memset(self, 0, sizeof(lasvm_t));
  free(self);
}

int 
lasvm_get_l( lasvm_t *self )
{
  return self->l;
}

double 
lasvm_get_cp( lasvm_t *self )
{
  return self->cp;
}

double 
lasvm_get_cn( lasvm_t *self )
{
  return self->cn;
}

int
lasvm_get_alpha(lasvm_t *self, double *alpha)
{
  int i;
  int l = self->l;
  for (i=0; i<l; i++)
    alpha[i] = self->alpha[i];
  return l;
}

int
lasvm_get_sv(lasvm_t *self, int *sv)
{
  int i;
  int l = self->l;
  int *r2i = lasvm_kcache_r2i(self->kernel, l);
  for (i=0; i<l; i++)
    sv[i] = r2i[i];
  return l;
}

int
lasvm_get_g(lasvm_t *self, double *g)
{
  int i;
  int l = self->l;
  for (i=0; i<l; i++)
    g[i] = self->g[i];
  return l;
}

static void
minmax( lasvm_t *self )
{
  if (! self->minmaxflag)
    {
      int i;
      int l = self->s;
      int imin = -1;
      int imax = -1;
      double gmin = 0;
      double gmax = 0;
      double *alpha = self->alpha;
      double *g = self->g;
      double *cmin = self->cmin;
      double *cmax = self->cmax;

      if (self->sumflag)
        {
          gmin = FLT_MAX;
          gmax = -FLT_MAX;
        }
      for (i=0; i<l; i++)
        {
          double gi = g[i];
          double ai = alpha[i];
          if (gi<gmin && ai>cmin[i])
            {
              imin = i;
              gmin = gi;
            }
          if (gi>gmax && ai<cmax[i])
            {
              imax = i;
              gmax = gi;
            }
        }
      self->gmin = gmin;
      self->gmax = gmax;
      self->imin = imin;
      self->imax = imax;
      self->minmaxflag = 1;
    }
}

static int
gs1( lasvm_t *self, int i, double epsgr)
{
  int l = self->s;
  int j;
  double g;
  double step, ostep, curv;
  float *row;
  int *r2i;
  /* Determine coordinate to process */
  if (i < 0)
    {
      minmax(self);
      if (self->gmin + self->gmax < 0)
        i = self->imin;
      else
        i = self->imax;
      if (i < 0)
        return 0;
    }
  /* Determine maximal step */  
  g = self->g[i];
  if (g < 0)
    {
      step = self->alpha[i] - self->cmin[i];
      if (self->gmax - g < epsgr)
        return 0;
    }
  else
    {
      step = self->cmax[i] - self->alpha[i];
      if (g - self->gmin < epsgr)
        return 0;
    }
  /* Determine curvature */
  r2i = lasvm_kcache_r2i(self->kernel, l);
  row = lasvm_kcache_query_row(self->kernel, r2i[i], l);
  curv = row[i];
  if (curv >= FLT_EPSILON)
    {
      ostep = fabs(g)/curv;
      if (ostep < step)
        step = ostep;
    }
  else if (curv + FLT_EPSILON <= 0)
    lasvm_error("Kernel is not positive (negative curvature)\n");    
  /* Perform update */
  if (g < 0)
    step = -step;
  self->alpha[i] += step;
  for (j=0; j<l; j++)
    self->g[j] -= step * row[j];
  self->minmaxflag = 0;
  return 1;
}

static int
gs2( lasvm_t *self, int imin, int imax, double epsgr)
{
  int l = self->s;
  int j;
  double gmin, gmax;
  double step, ostep, curv;
  float *rmin, *rmax;
  int *r2i;
  /* Determine coordinate to process */
  if (imin < 0 || imax < 0)
    {
      minmax(self);
      if (imin < 0)
        imin = self->imin;
      if (imax < 0)
        imax = self->imax;
    }
  if (imin < 0 || imax < 0)
    return 0;
  gmin = self->g[imin];
  gmax = self->g[imax];
  if (gmax - gmin < epsgr)
    return 0;
  /* Determine maximal step */  
  step = self->alpha[imin] - self->cmin[imin];
  ostep = self->cmax[imax] - self->alpha[imax];
  if (ostep < step)
    step = ostep;
  /* Determine curvature */
  r2i = lasvm_kcache_r2i(self->kernel, l);
  rmin = lasvm_kcache_query_row(self->kernel, r2i[imin], l);
  rmax = lasvm_kcache_query_row(self->kernel, r2i[imax], l);
  curv = rmax[imax]+rmin[imin]-rmax[imin]-rmin[imax];
  if (curv >= FLT_EPSILON)
    {
      double ostep = (gmax - gmin) / curv;
      if (ostep < step)
        step = ostep;
    }
  else if (curv + FLT_EPSILON <= 0)
    lasvm_error("Kernel is not positive (negative curvature)\n");    
  /* Perform update */
  self->alpha[imax] += step;
  self->alpha[imin] -= step;
  for (j=0; j<l; j++)
    self->g[j] -= step * ( rmax[j] - rmin[j] );
  self->minmaxflag = 0;
  return 1;
}

static void
swap( lasvm_t *self, int r1, int r2)
{
#define swap(type, v)        \
  { type tmp = self->v[r1];  \
    self->v[r1]=self->v[r2]; \
    self->v[r2]=tmp; }

  lasvm_kcache_swap_rr(self->kernel, r1, r2);
  swap(double, alpha);
  swap(double, cmin);
  swap(double, cmax);
  swap(double, g);
  if (self->minmaxflag)
    {
      if (self->imin==r1) 
        self->imin=r2;
      if (self->imin==r2) 
        self->imin=r1;
      if (self->imax==r1) 
        self->imax=r2;
      if (self->imax==r2) 
        self->imax=r1;
    }
#undef swap
}

static void
evict( lasvm_t *self )
{
  int i;
  int l = self->l;
  double *alpha = self->alpha;
  double *cmin = self->cmin;
  double *cmax = self->cmax;
  double *g = self->g;
  
  if (self->sumflag)
    {
      double gmin, gmax;
      minmax(self);
      gmin = self->gmin;
      gmax = self->gmax;
      for (i=0; i<l; i++)
        if (alpha[i] == 0)
          if ((g[i]>=gmax && 0>=cmax[i]) ||
              (g[i]<=gmin && 0<=cmin[i])  )
            swap(self, i--, --l);
      self->l = self->s = l;
    }
  else
    {
      for (i=0; i<l; i++)
        if (alpha[i] == 0)
          if ((g[i]>=0 && 0>=cmax[i]) ||
              (g[i]<=0 && 0<=cmin[i])  )
            swap(self, i--, --l);
      self->l = self->s = l;
    }
}

int 
lasvm_process( lasvm_t *self, int xi, double y )
{
  int l = self->l;
  int *i2r = 0;
  float *row = 0;
  double g;
  int j;
  /* Checks */
  if (self->s != self->l)
    lasvm_error("lasvm_process(): internal error\n");
  if (y != +1 && y != -1)
    lasvm_error("lasvm_process(): argument y must be +1 or -1\n");
  /* Bail out if already in expansion? */
  i2r = lasvm_kcache_i2r(self->kernel, 1+xi);
  if (i2r[xi] < l)
    return self->l; 
  /* Compute gradient */
  g = y;
  if (l > 0)
    {
      row = lasvm_kcache_query_row(self->kernel, xi, l);
      for (j=0; j<l; j++)
	g -= self->alpha[j] * row[j];
    }
  /* Decide insertion */
  if (self->sumflag)
    {
      minmax(self);
      if (self->gmin < self->gmax)
	if ((y>0 && g<self->gmin) || 
	    (y<0 && g>self->gmax)  )
	  {
	    lasvm_kcache_discard_row(self->kernel, xi);
	    return 0;
	  }
    }
  else
    {
      if (y * g < 0)
	{
	  lasvm_kcache_discard_row(self->kernel, xi);
	  return 0;
	}
    }
  /* Insert */
  checksize(self, l+1);
  lasvm_kcache_swap_ri(self->kernel, l, xi);
  self->alpha[l] = 0;
  self->g[l] = g;
  if (y > 0)
    {
      self->cmin[l] = 0;
      self->cmax[l] = self->cp;
    }
  else
    {
      self->cmin[l] = - self->cn;
      self->cmax[l] = 0;
    }
  self->l = self->s = l+1;
  /* Process */
  if (! self->sumflag)
    gs1(self, l, 0);
  else if (y > 0)
    gs2(self, -1, l, 0);
  else 
    gs2(self, l, -1, 0);
  self->minmaxflag = 0;
  return self->l;
}


int 
lasvm_reprocess(lasvm_t *self, double epsgr)
{
  int status;
  if (self->s != self->l)
    lasvm_error("lasvm_process(): internal error\n");
  if (self->sumflag)
    status = gs2(self, -1, -1, epsgr);
  else
    status = gs1(self, -1, epsgr);
  evict(self);
  if (status)
    return self->l;
  return 0;
}

static void
shrink(lasvm_t *self)
{
  int i;
  int s = self->s;
  double *alpha = self->alpha;
  double *cmin = self->cmin;
  double *cmax = self->cmax;
  double *g = self->g;
  if (self->sumflag)
    {
      double gmin, gmax;
      minmax(self);
      gmin = self->gmin;
      gmax = self->gmax;
      for (i=0; i<s; i++)
        if ((g[i]>=gmax && alpha[i]>=cmax[i]) ||
            (g[i]<=gmin && alpha[i]<=cmin[i])  )
          swap(self, i--, --s);
      self->s = s;
    }
  else
    {
      for (i=0; i<s; i++)
        if ((g[i]>=0 && alpha[i]>=cmax[i]) ||
            (g[i]<=0 && alpha[i]<=cmin[i])  )
          swap(self, i--, --s);
      self->s = s;
    }
}

static void
unshrink(lasvm_t *self)
{
  int l = self->l;
  int s = self->s;
  if (s < l)
    {
      double *alpha = self->alpha;
      double *g = self->g;
      int *r2i = lasvm_kcache_r2i(self->kernel, l);
      double a;
      int i,j;
      for(i=s; i<l; i++)
        g[i] = (alpha[i]>0) ? 1.0 : -1.0;
      for(j=0; j<l; j++)
        if ((a = alpha[j]) != 0)
          {
	    int xj = r2i[j];
	    int cached = lasvm_kcache_status_row(self->kernel, xj);
            float *row = lasvm_kcache_query_row(self->kernel, r2i[j], l);
            for (i=s; i<l; i++)
              g[i] -= a * row[i];
	    if (! cached) /* do not keep what was not cached */
	      lasvm_kcache_discard_row(self->kernel, xj);
          }
      self->minmaxflag = 0;
      self->s = l;
    }
}

int 
lasvm_finish(lasvm_t *self, double epsgr)
{
  int iter = 0;
  int siter = 0;
  int status = self->l;
  while( status )
    {
      if (iter >= siter)
        {
          shrink(self);
          siter = iter + min(1000,self->l);
        }
      if (self->sumflag)
        status = gs2(self, -1, -1, epsgr);
      else
        status = gs1(self, -1, epsgr);
      iter++;
    }
  unshrink(self);
  evict(self);
  return iter;
}

double 
lasvm_get_delta(lasvm_t *self)
{
  double d;
  minmax(self);
  d = self->gmax - self->gmin;
  return max(0, d);
}

double lasvm_get_b(lasvm_t *self)
{
  if (self->sumflag)
    {
      minmax(self);
      return - (self->gmax + self->gmin) / 2;
    }
  return 0;
}

double lasvm_get_w2(lasvm_t *self)
{
  int i;
  int l = self->l;
  double *alpha = self->alpha;
  double *g = self->g;
  double s = 0;
  for (i=0; i<l; i++)
    if (alpha[i]>0)
      s += alpha[i] * ( g[i] + 1.0 );
    else if (alpha[i]<0)
      s += alpha[i] * ( g[i] - 1.0 );
  return s/2.0;
}

double 
lasvm_predict(lasvm_t *self, int xi)
{
  int j;
  int l = self->l;
  int cached = lasvm_kcache_status_row(self->kernel, xi);
  float *row = lasvm_kcache_query_row(self->kernel, xi, l);
  double *alpha = self->alpha;
  double s = 0;
  if (self->sumflag)
    minmax(self);
  for (j=0; j<l; j++)
    s += alpha[j] * row[j];
  if (self->sumflag)
    s += (self->gmin + self->gmax) / 2;
  if (! cached) /* do not keep what was not cached */
    lasvm_kcache_discard_row(self->kernel, xi);
  return s;
}


void lasvm_init( lasvm_t *self, int l, 
                 const int *sv, 
                 const double *alpha, 
                 const double *g )
{
  int i,k;
  if (l <= 0)
    lasvm_error("Argument l should be positive.\n");
  checksize(self, l);
  self->l = 0;
  for (i=k=0; i<l; i++)
    {
      if (alpha[i])
        {
          lasvm_kcache_swap_ri(self->kernel, k, sv[i]);
          self->alpha[k] = alpha[i];
          if (alpha[i]>0)
            {
              self->cmin[k] = 0;
              self->cmax[k] = self->cp;
              self->g[k] = +1;
            }
          else
            {
              self->cmin[k] = - self->cn;
              self->cmax[k] = 0;
              self->g[k] = -1;
            }
          if (g)
            self->g[k] = g[i];
          k++;
        }
    }
  if (! g)
    {
      int *r2i = lasvm_kcache_r2i(self->kernel, k);
      for (i=0; i<k; i++)
        {
          double s = self->g[i];
          float *row = lasvm_kcache_query_row(self->kernel, r2i[i] , k);
          int j;
          for (j=0; j<k; j++)
            s -= self->alpha[j] * row[j];
          self->g[i] = s;
        }
    }
  self->l = self->s = k;
}
