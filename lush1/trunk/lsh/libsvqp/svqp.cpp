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
 * $Id: svqp.cpp,v 1.3 2003-09-04 00:22:03 leonb Exp $
 **********************************************************************/

#include "svqp.h"

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
  delete mem;
}

void
SimpleQuadraticProgram::compute_Ax(const svreal *x, svreal *y)
{
  const svreal *aa = a;
  for (int i=0; i<n; i+=1,aa+=n)
    y[i] = dot(x,aa,n);
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
  mem = new svreal[n+n];
  b = mem;
  tmp = mem+n;
}

QuadraticProgram::~QuadraticProgram()
{
  delete mem;
}

svreal
QuadraticProgram::compute_gx(const svreal *x, svreal *g)
{
  compute_Ax(x,g);
  for (int i=0; i<n; i++)
    {
      tmp[i] = g[i]/2 - b[i];
      g[i] = g[i] - b[i];
    }
  return dot(tmp,x,n);
}

svreal 
QuadraticProgram::compute_ggx(const svreal*, const svreal *z)
{
  compute_Ax(z,tmp);    // can be faster when A is symetric
  return dot(tmp,z,n);
}

void
QuadraticProgram::compute_Ax(const svreal *x, svreal *y)
{
}




// ================================================================
// ConvexProgram
// ================================================================


#define CLAMPNO  0
#define CLAMPMIN 1
#define CLAMPMAX 2


ConvexProgram::ConvexProgram(int n)
  : n(n)
{
  // allocate memory
  mem     = new svreal[6*n];
  clamp   = new char[n];
  x       = mem;
  cmin    = mem+n;
  cmax    = mem+2*n;
  g       = mem+3*n;
  z       = mem+4*n;
  gsav    = mem+5*n;
  // initialize variables
  sumflag = 0;
  ktflag  = 1;
  w       = 0;
  err     = 0;
  epsgr   = (svreal)1E-8;
  epskt   = (svreal)1E-20;
  maxst   = (svreal)1E+10;
  nactive = n;
}

ConvexProgram::~ConvexProgram()
{
  delete mem;
  delete clamp;
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
  w = - compute_gx(x, g);
  for (i=0; i<n; i++)
    g[i] = - g[i];
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
        return -1;
      }
  // averaged w for detecting instabilities
  svreal avgw = 0;
  // main loop
  for (;;)
    {
      // compute gradient
      w = - compute_gx(x, g);
      for (i=0; i<n; i++)
        g[i] = - g[i];
      // test instabilities
      if (iterations < n)
        avgw = w;
      else
        avgw += (w-avgw)/(5*n);
      if (w<avgw && iterations>n+n)
        {
          err = "Numerical instability (EPSGR is too small)";
          return -1;
        }
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
        }
      // compute search direction
      if (restartp)
        {
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
      if (curvature >= epskt)
        {
          svreal ostep = zgsav / curvature;
          if (ostep < step)
            step = ostep;
        }
      else if (step >= maxst)
        {
          err = "Function is not convex (negative curvature)";
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
  return iterations;
}


svreal
ConvexProgram::compute_gx(const svreal *x, svreal *g)
{
  return 0;
}

svreal 
ConvexProgram::compute_ggx(const svreal*, const svreal *z)
{
  return 0;
}





