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
 * $Id: svqp.h,v 1.4 2004/05/12 17:46:49 leonb Exp $
 **********************************************************************/

//////////////////////////////////////
///
/// This is an implementation of 
/// the good old conjugate gradient
/// method (a-la-GP2).
///
/// Leon Bottou - 1997
///
/////////////////////////////////////

#include <stdlib.h>
#include <math.h>


// ================================================================
//  Convex Programming for Support Vectors Machines
// ================================================================
//
//  This package provides a hierarchy of classes for solving 
//  convex programming problems related to support vector machines.
//  This library does not implement chunking strategies.
//  These strategies should be implemented by the caller.
//
// ----------------------------------------------------------------
//  Abstract Class "ConvexProgram"
//
//     Maximize          - f(x)
//     with              Cmin_i <= x_i <= Cmax_i
//     and (optionally)  sum_i x_i = cste
//
//  Function f must be a convex function of vector x.
//  It is defined by virtual member functions computing:
//   - value of f on point x
//   - gradient of f on point x
//   - curvature of f on point x along direction z.
//  
// ----------------------------------------------------------------
//  Abstract Class "QuadraticProgram"
//
//     Maximize          - 1/2 x.A.x + b.x
//     with              Cmin_i <= x_i <= Cmax_i
//     and (optionally)  sum_i x_i = cste
//
//  This is a subclass of "ConvexProgram".
//  Matrix A is implicitely defined by a virtual
//  function which computes rows of matrix A.
//
// ----------------------------------------------------------------
//  Class "SimpleQuadraticProgram
//
//  This is a subclass of "ConvexProgram"
//  where matrix A is defined extensively.
//  Programs performing chunking should
//  directly subclass "QuadraticProgram" and
//  define a virtual function which computes
//  product A.x using the cached information
//  directly.
//
// ----------------------------------------------------------------
//  Using "SimpleQuadraticProgram"
//
//   1- Define a SimpleQuadraticProgram object.
//      We assume that this object is named "pgm".
//
//   2- Setup vector variables defining the problem.
//           pgm.a, pgm.b, 
//           pgm.cmin, pgm.cmax
//      You can overwrite the vector elements or
//      redefine the vector pointers. 
//
//   3- Set the optional constraint flag
//           pgm.sumflag   if constraint sum_i x_i must be handled.   
//
//   4- Initialize vector pgm.x with a feasible value.
//      You can overwrite the vector elements or
//      redefine the vector pointer. 
//
//   5- Set optional accuracy parameters
//           pgm.epsgr   stopping criterion on norm of gradient (1E-8).
//           pgm.epskt   accuracy of kuhn-tucker test (1E-20).
//           pgm.maxst   maximum gradient multiplier (1E+10).
//
//   6- Call member function run().
//           Function returns -1 if a problem occurs.
//           A description of the problem is found in pgm.err.
//           Functions returns a number of iterations on success.
//           The solution is found in pgm.x (vector) and pgm.w (value).
//
//
// ----------------------------------------------------------------
//  Subclassing "QuadraticProgram" or "ConvexProgram"
//
//  1- Define subclasses that define the virtual functions
//           compute_Ax and/or compute_Arow (QuadraticProgram)
//           compute_gx, compute_ggx (ConvexProgram)
//
//  2- Initialize the base class by specifying the vector sizes
//
//  3- Setup vector variables
//           pgm.b (QuadraticProgram only)
//           pgm.cmin, pgm.cmax
//      You can overwrite the vector elements or
//      redefine the vector pointers. 
//
//  4- Proceed as explained in steps 3,4,5,6 
//     of the SimpleQuadraticProgram procedure.
//
// ----------------------------------------------------------------



        

// ================================================================
// General Declarations
// ================================================================





// svreal - typedef for floating point type (float or double)
typedef double svreal;

// svbool - typedef for boolean value
typedef char svbool;




// ================================================================
// ConvexProgram
// ================================================================




// ConvexProgram - abstract class for convex programming

class ConvexProgram
{
  //     Maximize          - f(x)
  //     with              Cmin_i <= x_i <= Cmax_i
  //     and (optionally)  sum_i x_i = constant
public:
  // constructor/destructor
  ConvexProgram(int n);
  virtual ~ConvexProgram();
  // n -- dimension of vectors x,cmin,cmax
  const int n;
  // cmin, cmax -- problem parameters
  svreal *cmin, *cmax;
  // sumflag -- set if linear constraint must be considered
  svbool sumflag;     
  // ktflag -- set if optimizer must loop until all kt conditions are met.
  svbool ktflag;     
  // x -- current solution vector
  svreal *x;
  // w -- current solution value
  svreal w;
  // error -- description of error
  const char *err;
  // epsgr -- stopping criterion on gradient norm
  svreal epsgr;
  // epskt -- clamping proximity for constraints
  svreal epskt;
  // maxst -- maximal gradient step value
  svreal maxst;
  // verbosity -- how talkative we are
  int verbosity;
  // run() -- compute optimum, return -1 or number of iterations
  int run(void);

  // compute_gx -- stores gradient of f into vector g and returns f(x)
  virtual svreal compute_gx(const svreal *x, svreal *g);
  // compute_ggx -- returns curvature along direction z at point x
  virtual svreal compute_ggx(const svreal *x, const svreal *z);

protected:
  // conjugate gradient stuff
  int        iterations;
  svreal    *grad;
  svreal    *g;
  svreal    *z;
  svreal     gnorm;
  // info about past iteration
  svbool     restartp;
  svreal    *gsav;
  svreal     zgsav;
  // clamping information
  char      *clamp;
  int        nactive;
  // memory allocation
  svreal    *mem;
  // projection
  void   project_with_linear_constraint(void);
  void   project_without_linear_constraint(void);
  svbool adjust_clamped_variables(void);
  // coordinate descent
  virtual svbool perform_coordinate_descent(void);
  // debug
  void   info(int level, const char *format, ...);
};





// ================================================================
// QuadraticProgram
// ================================================================


class QuadraticProgram : public ConvexProgram
{
  //     Maximize          - 1/2 x.A.x + b.x
  //     with              Cmin_i <= x_i <= Cmax_i
  //     and (optionally)  sum_i x_i = constant

public:
  // constructor/destructor
  QuadraticProgram(int n);
  virtual ~QuadraticProgram();
  // b -- linear term
  svreal *b;
  // compute_Arow -- returns the ith row of matrix A. 
  //   Vector r can be used as storage space for the
  //   returned row, but this is not required.
  virtual svreal *compute_Arow(int i, svreal *r);
  // compute_Ax -- computes Ax into y.
  virtual void compute_Ax(const svreal *x, svreal *y);
  
protected:
  // memory allocation
  svreal *tmp1;
  svreal *tmp2;
  svreal *mem;
  svbool  arow;
  // overrides
  virtual svreal compute_gx(const svreal *x, svreal *g);
  virtual svreal compute_ggx(const svreal *x, const svreal *z);
  virtual svbool perform_coordinate_descent(void);
};





// ================================================================
// SimpleQuadraticProgram
// ================================================================


class SimpleQuadraticProgram : public QuadraticProgram
{
  //     Maximize          - 1/2 x.A.x + b.x
  //     with              Cmin_i <= x_i <= Cmax_i
  //     and (optionally)  sum_i x_i = constant
  
public:
  // constructor/destructor
  SimpleQuadraticProgram(int n);
  virtual ~SimpleQuadraticProgram();
  // a -- quadratic matrix (line order)
  svreal *a;
  
protected:
  // memory allocation
  svreal *mem;
  // overrides
  virtual svreal *compute_Arow(int i, svreal *r);
};
