// -*- C++ -*-

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
 * $Id: svqp.cpp,v 1.10 2004/09/16 18:11:33 leonb Exp $
 **********************************************************************/

#include "svqp.h"

#define VERBOSE
#ifdef VERBOSE
# include <stdarg.h>
# include <stdio.h>
#endif

//////////////////////////////////////
///
/// This is an implementation of 
/// the good old conjugate gradient
/// method (a-la-GP2).
///
/// Leon Bottou - 1997
///
/////////////////////////////////////





// ================================================================
// Utilities
// ================================================================


inline svreal
dot(const svreal *x, const svreal *y, int n)
{
  svreal sum=0.0;
  for (int i=0; i<n; i++)
    sum += *x++ * *y++;
  return sum;
}



// ================================================================
// ConvexProgram
// ================================================================


void 
ConvexProgram::info(int level, const char *fmt, ...)
{
#ifdef VERBOSE
  if (level <= verbosity)
    {
      va_list ap;
      va_start(ap, fmt);
      vfprintf(stdout, fmt, ap);
      va_end(ap);
      fflush(stdout);
    }
#endif
}


#define CLAMPNO  0
#define CLAMPMIN 1
#define CLAMPMAX 2


ConvexProgram::ConvexProgram(int n)
  : n(n)
{
  // allocate memory
  mem     = new svreal[7*n];
  clamp   = new char[n];
  x       = mem;
  cmin    = mem+n;
  cmax    = mem+2*n;
  g       = mem+3*n;
  z       = mem+4*n;
  gsav    = mem+5*n;
  grad    = mem+6*n;
  // initialize variables
  sumflag = 0;
  ktflag  = 1;
  w       = 0;
  err     = 0;
  epsgr   = (svreal)1E-8;
  epskt   = (svreal)1E-20;
  maxst   = (svreal)1E+10;
  nactive = n;
  verbosity = 1;
}

ConvexProgram::~ConvexProgram()
{
  delete [] mem;
  delete [] clamp;
}


inline void 
ConvexProgram::project_with_linear_constraint(void)
{
  int i;
  int oldactive;
  // Iterate (maximum n times, usually two times)
  do 
  {
    oldactive = nactive;
    // Compute gradient correction
    svreal gcorr = 0;
    for (i=0; i<n; i++)
      {
        if (clamp[i] == CLAMPNO)
          gcorr += g[i];
        else
          g[i] = 0;
      }
    gcorr = gcorr / nactive;
    // Check constraints
    for (i=0; i<n; i++)
      {
        if (clamp[i] == CLAMPNO)
          {
            g[i] = g[i] - gcorr;
            if (g[i] < 0)
              {
                if (x[i] <= cmin[i]+epskt)
                  {
                    x[i]      = cmin[i];
                    g[i]      = 0;
                    clamp[i]  = CLAMPMIN;
                    restartp  = 1;
                    nactive   -= 1;
                  }
              }
            else
              {
                if (x[i] >= cmax[i]-epskt)
                  {
                    x[i]      = cmax[i];
                    g[i]      = 0;
                    clamp[i]  = CLAMPMAX;
                    restartp  = 1;
                    nactive   -= 1;
                  }
              }
          }
      }
  } 
  while (nactive < oldactive);
}


inline void 
ConvexProgram::project_without_linear_constraint(void)
{
  for (int i=0; i<n; i++)
    {
      if (clamp[i] != CLAMPNO)
        {
          g[i] = 0;
        }
      else if (g[i] < 0)
        {
          if (x[i] <= cmin[i]+epskt)
            {
              x[i]      = cmin[i];
              g[i]      = 0;
              clamp[i]  = CLAMPMIN;
              restartp  = 1;
              nactive   -= 1;
            }
        }
      else
        {
          if (x[i] >= cmax[i]-epskt)
            {
              x[i]      = cmax[i];
              g[i]      = 0;
              clamp[i]  = CLAMPMAX;
              restartp  = 1;
              nactive   -= 1;
            }
        }
    }
}



svbool
ConvexProgram::adjust_clamped_variables(void)
{
  int i;
  int oldactive = nactive;
  // Recompute gradient again
  for (i=0; i<n; i++)
    g[i] = - grad[i];
  // Reset clamp status
  nactive = n;
  for (i=0; i<n; i++)
    clamp[i] = CLAMPNO;
  // Project gradient and recompute clamp status
  if (sumflag)
    project_with_linear_constraint();
  else
    project_without_linear_constraint();
  // Recompute gradient_norm
  gnorm = dot(g,g,n);
  // Return true if variables have been unclamped
  return (nactive > oldactive);
}



int 
ConvexProgram::run(void)
{
  // initialize variables
  int i;
  int itercg = 0;
  for (i=0; i<n; i++)
    clamp[i] = CLAMPNO;
  restartp = 1;
  iterations = 0;
  // test feasibility
  for (i=0; i<n; i++)
    if (x[i]<cmin[i] || x[i]>cmax[i])
      {
        err = "Initial x vector is not feasible";
	info(1,"? %s\n", err);
        return -1;
      }
  // averaged w for detecting instabilities
  svreal avgw = 0;
  // main loop
  for (;;)
    {
      // compute gradient
      w = - compute_gx(x, grad);
      if (err)
	return -1;
      for (i=0; i<n; i++)
        g[i] = - grad[i];
      // test instabilities
      if (iterations < n)
        avgw = w;
      else
        avgw += (w-avgw)/(5*n);
      if (w<avgw && iterations>n+n)
        {
          err = "Numerical instability (EPSGR is too small)";
	  info(1,"? %s\n", err);
          return -1;
        }
      // messages deoend on verbosity settings.
      if (((verbosity>=2) || (iterations%40==0)) && iterations>0)
	info(1, " iteration %d: w=%e g=%e active=%d\n", 
	     iterations, w, gnorm, nactive);
      // project gradient and compute clamp status
      if (sumflag)
        project_with_linear_constraint();
      else
        project_without_linear_constraint();
      // compute gradient_norm and test termination
      gnorm = dot(g,g,n);
      if (gnorm<epsgr)
        {
          if (! ktflag)
            break;
          // Reexamine clamping status
          if (! adjust_clamped_variables())
            break;
          if (gnorm<epsgr)
            break;
          // Continue processing
	  info(1,"@");
          restartp = 1;
        }
      // compute search direction
      if (restartp)
        {
	  if (itercg<2 && perform_coordinate_descent())
	    {
	      info(1,"+");
	      if (gnorm<epsgr)
		break;
	    }
	  else
	    info(1,"-");
          // Just copy gradient into search direction
          itercg = 0;
          for (i=0; i<n; i++)
            z[i] = gsav[i] = g[i];
          // Save previous gradient
          restartp = 0;
          zgsav = gnorm;
        }
      else
        {
	  info(1,">");
          // Self restarting Hestenes Stiefel
          for (i=0; i<n; i++)
            gsav[i] = g[i] - gsav[i];
          svreal beta = dot(g,gsav,n) / zgsav;
          for(i=0; i<n; i++)
            z[i] = g[i] + beta * z[i];
          // Save previous gradient
          restartp = 0;
          for (i=0; i<n; i++)
            gsav[i] = g[i];
          zgsav = dot(z,gsav,n);
        }
      // Compute clipping constraint
      svreal step = maxst;
      for (i=0; i<n; i++)
        {
          if (z[i] < 0)
            {
              svreal ostep = (cmin[i]-x[i]) / z[i];
              if (ostep < step)
                step = ostep;
            }
          else if (z[i] > 0)
            {
              svreal ostep = (cmax[i]-x[i]) / z[i];
              if (ostep < step)
                step = ostep;
            }
        }
      // Compute optimal quadratic step
      svreal curvature = compute_ggx(x,z);
      if (err)
	return -1;
      if (curvature >= epskt)
        {
          svreal ostep = zgsav / curvature;
          if (ostep < step)
            step = ostep;
        }
      else if (step >= maxst)
        {
          err = "Function is not convex (negative curvature)";
	  info(1,"? %s\n", err);
          return -1;
        }
      // perform update
      for (i=0; i<n; i++)
        x[i] += step * z[i];
      // count iterations
      iterations += 1;
      itercg += 1;
    }
  // Termination
  info(1,"\n");
  return iterations;
}

svreal
ConvexProgram::compute_gx(const svreal *x, svreal *g)
{
  err = "compute_gx is not defined";
  info(1,"? %s\n", err);
  return 0;
}

svreal 
ConvexProgram::compute_ggx(const svreal*, const svreal *z)
{
  err = "compute_ggx is not defined";
  info(1,"? %s\n", err);
  return 0;
}

svbool 
ConvexProgram::perform_coordinate_descent(void)
{
  return 0;
}




// ================================================================
// QuadraticProgram
// ================================================================
//     Maximize          - 1/2 x.A.x + b.x
//     with              Cmin_i <= x_i <= Cmax_i
//     and (optionally)  sum_i x_i = 0



QuadraticProgram::QuadraticProgram(int n)
  : ConvexProgram(n)
{
  mem = new svreal[n+n+n];
  b = mem;
  tmp1 = mem+n;
  tmp2 = mem+n+n;
  arow = 1;
}

QuadraticProgram::~QuadraticProgram()
{
  delete [] mem;
}

svbool 
QuadraticProgram::perform_coordinate_descent(void)
{
  int maxrun = n;
  // Bail out if compute_Arow does nothing
  if (! arow)
    return 0;
  // Loop at most n times without reaching constraints.
  while (maxrun > 0)
    {
      svreal gmax = -maxst;
      svreal gmin = +maxst;
      int gmaxidx = -1;
      int gminidx = -1;
      for (int i=0; i<n; i++)
	{
	  svreal gx = grad[i];
	  if (gx>gmax && x[i]>cmin[i]+epskt)
	    {
	      gmax = gx;
	      gmaxidx = i;
	    }
	  if (gx<gmin && x[i]<cmax[i]-epskt)
	    {
	      gmin = gx;
	      gminidx = i;
	    }
	}
      if (gmax - gmin < epsgr)
	break;
      if (sumflag)
	{
	  int j;
	  svreal ostep;
	  svreal step = maxst;
	  svreal curvature;
	  // get rows
	  svreal *rmax = compute_Arow(gmaxidx, tmp1);
	  svreal *rmin = compute_Arow(gminidx, tmp2);
	  if (! (rmax && rmin))
	    { arow = 0; return 0; }
	  // box constraints
	  ostep = x[gmaxidx] - cmin[gmaxidx];
	  if (ostep < step)
	    step = ostep;
	  ostep = cmax[gminidx] - x[gminidx];
	  if (ostep < step)
	    step = ostep;
	  // newton
	  curvature = rmax[gmaxidx] + rmin[gminidx]
	          - ( rmax[gminidx] + rmin[gmaxidx] );
	  if (curvature > epskt)
	    {
	      ostep = (gmax - gmin) / curvature;
	      if (ostep < step)
		{
		  maxrun -= 1;
		  step = ostep;
		}
	    }
	  if (step >= maxst)
	    break;
	  // update
	  x[gmaxidx] -= step;
	  x[gminidx] += step;
	  for (j=0; j<n; j++)
	    grad[j] = grad[j] + step * ( rmin[j] - rmax[j] );
	}
      else
	{
	  int j;
	  svreal ostep;
	  svreal step = maxst;
	  svreal curvature;
	  svreal *rmax;
	  // select best axis.
	  if (fabs(gmax) < fabs(gmin))
	    {
	      gmax = gmin;
	      gmaxidx = gminidx;
	    }
	  // get row
	  rmax = compute_Arow(gmaxidx, tmp2);
	  if (! rmax)
	    { arow = 0; return 0; }
	  // box constraints
	  if (gmax > 0)
	    ostep = x[gmaxidx] - cmin[gmaxidx];
	  else
	    ostep = cmax[gmaxidx] - x[gmaxidx];
	  if (ostep < step)
	    step = ostep;
	  // newton
	  curvature = rmax[gmaxidx];
	  if (curvature > epskt)
	    {
	      ostep = fabs(gmax) / curvature;
	      if (ostep < step)
		{
		  step = ostep;
		  maxrun -= 1;
		}
	    }
	  // update
	  if (gmax > 0)
	    step = - step;
	  x[gmaxidx] += step;
	  for (j=0; j<n; j++)
	    grad[j] = grad[j] + step * rmax[j];
	}
    }
  // Restore and return
  adjust_clamped_variables();
  // Success.
  return 1;
}

svreal
QuadraticProgram::compute_gx(const svreal *x, svreal *g)
{
  compute_Ax(x,g);
  for (int i=0; i<n; i++)
    {
      tmp1[i] = g[i]/2 - b[i];
      g[i] = g[i] - b[i];
    }
  return dot(tmp1,x,n);
}

svreal 
QuadraticProgram::compute_ggx(const svreal*, const svreal *z)
{
  compute_Ax(z,tmp1);    // can be faster when A is symetric
  return dot(tmp1,z,n);
}

void
QuadraticProgram::compute_Ax(const svreal *x, svreal *y)
{
  int i,j;
  for (j=0; j<n; j++)
    y[j] = 0;
  for (i=0; i<n; i++)
    {
      svreal xx = x[i];
      if (xx)
	{
	  svreal *row = compute_Arow(i, tmp2);
	  if (row) 
	    {
	      for (j=0; j<n; j++)
		y[j] += xx * row[j];
	    } 
	  else 
	    {
	      err = "neither compute_Ax not compute_Arow are defined";
	      info(0, "?%s\n", err);
	      return;
	    }
	}
    }
}

svreal *
QuadraticProgram::compute_Arow(int i, svreal *r)
{
  return 0;
}


// ================================================================
// SimpleQuadraticProgram
// ================================================================
//     Maximize          - x.A.x + b.x
//     with              Cmin_i <= x_i <= Cmax_i
//     and (optionally)  sum_i x_i = 0



SimpleQuadraticProgram::SimpleQuadraticProgram(int n)
  : QuadraticProgram(n)
{
  mem = new svreal[n*n];
  a = mem;
}

SimpleQuadraticProgram::~SimpleQuadraticProgram()
{
  delete [] mem;
}

svreal *
SimpleQuadraticProgram::compute_Arow(int i, svreal *r)
{
  return a + i*n;
}




