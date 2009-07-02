#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INT long int
#define INT_INF LONG_MAX
#define INF DBL_MAX

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

/*============================================================================
   user controlled parameters for the conjugate gradient algorithm
               (default values in cg_default)                                 */
typedef struct cg_parameter_struct /* user controlled parameters */
{
   /* parameters values that the user may wish to modify */
/*----------------------------------------------------------------------------*/
    /* T => print final statistics
       F => no printout of statistics */
    int PrintFinal ;

    /* Level 0  = no printing), ... , Level 3 = maximum printing */
    int PrintLevel ;

    /* T => print parameters values
       F => do not display parmeter values */
    int PrintParms ;

    /* T => use approximate Wolfe line search
       F => use ordinary Wolfe line search, switch to approximate Wolfe when
                |f_k+1-f_k| < AWolfeFac*C_k, C_k = average size of cost  */
    int    AWolfe ;
    double AWolfeFac ;

    /* factor in [0, 1] used to compute average cost magnitude C_k as follows:
       Q_k = 1 + (Qdecay)Q_k-1, Q_0 = 0,  C_k = C_k-1 + (|f_k| - C_k-1)/Q_k */
    double Qdecay ;

    /* Stop Rules:
       T => ||proj_grad||_infty <= max(grad_tol,initial ||grad||_infty*StopFact)
       F => ||proj_grad||_infty <= grad_tol*(1 + |f_k|) */
    int    StopRule ;
    double StopFac ;

    /* T => estimated error in function value is eps*Ck,
       F => estimated error in function value is eps */
    int    PertRule ;
    double eps ;

    /* T => attempt quadratic interpolation in line search when
                |f_k+1 - f_k|/f_k <= QuadCutoff
       F => no quadratic interpolation step */
    int    QuadStep ;
    double QuadCutOff ;

    /* T => check that f_k+1 - f_k <= debugtol*C_k
       F => no checking of function values */
    int    debug ;
    double debugtol ;

    /* if step is nonzero, it is the initial step of the initial line search */
    double step ;

    /* abort cg after maxit_fac*n iterations */
    double maxit_fac ;

    /* maximum number of times the bracketing interval grows or shrinks
       in the line search is nexpand */
    int nexpand ;

   /* maximum number of secant iterations in line search is nsecant */
    int nsecant ;

    /* conjugate gradient method restarts after (n*restart_fac) iterations */
    double restart_fac ;

    /* stop when -alpha*dphi0 (estimated change in function value) <= feps*|f|*/
    double feps ;

    /* after encountering nan, growth factor when searching for
       a bracketing interval */
    double nan_rho ;

/*============================================================================
       technical parameters which the user probably should not touch          */
    double           delta ; /* Wolfe line search parameter */
    double           sigma ; /* Wolfe line search parameter */
    double           gamma ; /* decay factor for bracket interval width */
    double             rho ; /* growth factor when searching for initial
                                bracketing interval */
    double             eta ; /* lower bound for the conjugate gradient update
                                parameter beta_k is eta*||d||_2 */
    double            psi0 ; /* factor used in starting guess for iteration 1 */
    double            psi1 ; /* in performing a QuadStep, we evaluate the
                                function at psi1*previous step */
    double            psi2 ; /* when starting a new cg iteration, our initial
                                guess for the line search stepsize is
                                psi2*previous step */
} cg_parameter ;

typedef struct cg_stats_struct /* statistics returned to user */
{
    double               f ; /*function value at solution */
    double           gnorm ; /* max abs component of gradient */
    INT               iter ; /* number of iterations */
    INT              nfunc ; /* number of function evaluations */
    INT              ngrad ; /* number of gradient evaluations */
} cg_stats ;

/* prototypes */

int cg_descent /*  return:
                      -2 (function value became nan)
                      -1 (starting function value is nan)
                       0 (convergence tolerance satisfied)
                       1 (change in func <= feps*|f|)
                       2 (total iterations exceeded maxit)
                       3 (slope always negative in line search)
                       4 (number secant iterations exceed nsecant)
                       5 (search direction not a descent direction)
                       6 (line search fails in initial interval)
                       7 (line search fails during bisection)
                       8 (line search fails during interval update)
                       9 (debugger is on and the function value increases)
                      10 (out of memory) */
(
    double            *x, /* input: starting guess, output: the solution */
    INT                n, /* problem dimension */
    cg_stats      *Stats, /* structure with statistics (see cg_descent.h) */
    cg_parameter  *UParm, /* user parameters, NULL = use default parameters */
    double      grad_tol, /* StopRule = 1: |g|_infty <= max (grad_tol,
                                           StopFac*initial |g|_infty) [default]
                             StopRule = 0: |g|_infty <= grad_tol(1+|f|) */
    double        (*value) (double *, INT),  /* f = value (x, n) */
    void           (*grad) (double *, double *, INT), /* grad (g, x, n) */
    double      (*valgrad) (double *, double *, INT), /* f = valgrad (g,x,n)*/
    double         *Work  /* either size 4n work array or NULL */
) ;

void cg_default /* set default parameter values */
(
    cg_parameter   *Parm
) ;

int  cg_readParms /* return:
                             0 (parameter file was read)
                            -1 (parameter file not found)
                            -2 (missing entry in parameter file)
                            -3 (comment in parameter file too long) */
(
    char        *filename ,
    cg_parameter    *Parm
) ;
