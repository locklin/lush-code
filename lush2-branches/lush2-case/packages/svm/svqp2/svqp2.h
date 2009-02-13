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
 * $Id: svqp2.h,v 1.7 2006/10/02 12:55:50 leonb Exp $
 **********************************************************************/




//////////////////////////////////////
///
/// SVQP2 uses a gauss-seidel/smo method.
/// It also implements proper caching etc.
///
/// Leon Bottou - 2004
///
/////////////////////////////////////



// ================================================================
// Quadratic Program Solver
// ================================================================


// SVQP2 - abstract class for convex programming

class SVQP2
{
  //     Maximize          - 1/2 x.A.x + b.x
  //     with              Cmin_i <= x_i <= Cmax_i
  //     and (optionally)  sum_i x_i = constant

  
public:
  
  // DESTRUCTOR.
  
  virtual ~SVQP2();

  // CONSTRUCTOR:
  // Argument N is the dimension of vectors APERM, X, B, CMIN, and CMAX.
  // The constructor allocates all these arrays.  
  // The caller must then set all input variables properly.
  // These are marked as 'MANDATORY INPUT' or 'OPTIONAL INPUT'

  SVQP2(int n);

  // READ-ONLY VARIABLE: 
  // Dimension of the problem. 
  // This is set by the constructor.

  const int n;
  
  // MANDATORY INPUT: AFUNCTION, ACLOSURE, APERM
  // Matrix A is represented by several variables.
  // Element A[i][j] is accessed as
  //    (*Afunction)(Aperm[i], Aperm[j], Aclosure);
  // Array APERM[] initially contains 1...n but can be changed.
  // Cache space is not wasted when the same integer appears twice.
  
  double (*Afunction)(int, int, void *);
  void    *Aclosure;
  int     *Aperm;
  
  // MANDATORY INPUT: B, CMIN, CMAX
  // Space is already allocated for these vectors.
  // The initial value, 0, must be replaced.
  double  *b;
  double  *cmin;
  double  *cmax;

  // MANDATORY INPUT/OUTPUT: X
  // You must provide a feasible X as initial input.
  // This vector is initially set to 0 which might not be feasible.
  
  double  *x;

  // OPTIONAL INPUT: SUMFLAG
  // This flag indicates whether the equality constraint
  // should be considered.  Default: true.

  bool     sumflag;

  // OPTIONAL INPUT: VERBOSITY
  // Set this to 0 for quiet operation.
  // Set this to 2 for very verbose operation.
  // Set this to 3 to enable more debugging code.

  int      verbosity;

  // OPTIONAL INPUTS:
  // - EPSKT is the tolerance for checking KKT conditions (default 1e-20)
  // - EPSGR is the maximal L0 norm of the gradient on exit (default 1e-3)
  // - MAXST is the maximal conjugate gradient step (default 1e20)

  double   epskt;
  double   epsgr;
  double   maxst;

  // OPTIONAL INPUT: MAXCACHESIZE
  // Maximum memory used to cache coefficients of matrix A. 
  // Default: 256M.
  
  long     maxcachesize;

  // PUBLIC FUNCTION: RUN()
  // Calling this function with the defaults arguments runs the solver.  
  // If an error occurs, it returns -1 and sets variable ERRMSG.
  // Otherwise the final result can be found in vector X.
  // Section "Advanced" below discusses the arguments of this function.
  
  int run(bool initialize=true, bool finalize=true);

  // OUTPUT: ERRMSG
  // Error message set when run() returns -1.
  
  const char *errmsg;
  
  // OUTPUT: GMIN, GMAX, W
  // - W is the optimal value of the objective function
  // - GMIN, GMAX are the active gradient bounds.
  //   The SVM threshold is (GMIN+GMAX)/2...
  
  double   gmin;
  double   gmax;
  double   w;


  // ADVANCED: INITIALIZE, FINALIZE, PERMUTATION
  // Sometimes one wishes to perform several optimizations 
  // that differ in vectors B, CMIN and CMAX, but do not
  // modify matrix A. It would be then wasteful to recompute
  // the cached coefficients for each optimization.
  //
  // - Function RUN() usually flushes the cached coefficients 
  //   before starting the optimization. This can be avoided
  //   by setting argument INITIALIZE to false.
  //
  // - Function RUN() then runs the optimizer. The optimizer 
  //   performs complex permutations to the coefficients of 
  //   vectors APERM, CMIN, CMAX, B and X.
  //
  // - Finally function RUN() rearranges the coefficients of
  //   vectors APERM, CMIN, CMAX, B and X to restore the initial order. 
  //   This operation destroys the cache as a side effect.
  //   This can be avoided by setting argument FINALIZE to false.
  //
  // To preserve the cache between successive invocations of RUN(), 
  // the caller needs to set both arguments INITIALIZE and FINALIZE to false.
  // As a consequence it must deal with arbitrary permutations of
  // the vector coefficients. This is done by relying on the contents
  // of vector APERM, or by using function PERMUTATION.
  //
  // Public function PERMUTATION() takes a pointer to a preallocated 
  // vector of N integers and fills it with the new coefficient locations.
  // For instance, TABLE[J] indicates the new location of the coefficient 
  // initially located at position J.
  
  void permutation(int *table);
  
  // INTERNAL
  
#if KSTATS
  static long long kcalcs;
  static long long kreqs;
#endif
  
protected:
  int      iter;		// total iterations
  int      l;                   // Active set size
  double  *g ;                  // [l] Gradient
  double  *xbar ;               // [n] Another x
  double  *gbar ;               // [n] Gradient at xbar
  int     *pivot;		// [n] Pivoting vector
  long     curcachesize;        // Current cache size
  struct Arow;                  // Cached rows
  Arow   **rows;		// [n] Cached rows
  Arow    *lru;                 // Most recently used cache row
  double  *dmem;                // Allocated memory
  int     *imem;                // Allocated memory
  int      vdots;		// Infodot counter
  bool     hitcache;		// Did I hit the cache size limit.
protected:
  void   info(const char*, const char *, ...);
  int    error(const char *message);
  void   cache_init(void);
  void   cache_fini(void);
  long   cache_clean(void);
  void   cache_swap(int, int);
  float *getrow(int, int, bool hot=true);
  void   swap(int, int);
  void   unswap(void);
  void   togbar(int);
  void   shrink(void);
  void   unshrink(bool);
  int    iterate_gs1(void);
  int    iterate_gs2(void);
private:
  SVQP2(const SVQP2&);
  SVQP2& operator=(const SVQP2&);
};

