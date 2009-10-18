#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define ZERO ((double) 0)
#define ONE ((double) 1)
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

typedef struct cg_com_struct /* common variables */
{
    /* parameters computed by the code */
    INT              n ; /* problem dimension, saved for reference */
    INT             nf ; /* number of function evaluations */
    INT             ng ; /* number of gradient evaluations */
    int         QuadOK ; /* T (quadratic step successful) */
    double       alpha ; /* stepsize along search direction */
    double           f ; /* function value for step alpha */
    double          df ; /* function derivative for step alpha */
    double       fpert ; /* perturbation is eps*Ck if PertRule is T */
    double          f0 ; /* old function value */
    double          Ck ; /* average cost as given by the rule:
                            Qk = Qdecay*Qk + 1, Ck += (fabs (f) - Ck)/Qk */
    double    wolfe_hi ; /* upper bound for slope in Wolfe test */
    double    wolfe_lo ; /* lower bound for slope in Wolfe test */
    double   awolfe_hi ; /* upper bound for slope, approximate Wolfe test */
    int         AWolfe ; /* F (use Wolfe line search)
                                T (use approximate Wolfe line search)
                                do not change user's AWolfe, this value can be
                                changed based on AWolfeFac */
    double          *x ; /* current iterate */
    double      *xtemp ; /* x + alpha*d */
    double          *d ; /* current search direction */
    double      *gtemp ; /* gradient at x + alpha*d */
    double   (*cg_value) (double *, INT) ; /* f = cg_value (x, n) */
    void      (*cg_grad) (double *, double *, INT) ; /* cg_grad (g, x, n) */
    double (*cg_valgrad) (double *, double *, INT) ; /* f = cg_valgrad (g,x,n)*/
    cg_parameter *Parm ; /* user parameters */
} cg_com ;

/* prototypes */

int cg_Wolfe
(
    double   alpha, /* stepsize */
    double       f, /* function value associated with stepsize alpha */
    double    dphi, /* derivative value associated with stepsize alpha */
    cg_com    *Com  /* cg com */
) ;

double cg_f
(
    double   *x,
    cg_com *Com
) ;

void cg_g
(
    double   *g,
    double   *x,
    cg_com *Com
) ;

double cg_fg
(
    double   *g,
    double   *x,
    cg_com *Com
) ;


int cg_tol
(
    double         f, /* function value associated with stepsize */
    double     gnorm, /* gradient sup-norm */
    int     StopRule, /* T => |grad|_infty <=max (tol, |grad|_infty*StopFact)
                          F => |grad|_infty <= tol*(1+|f|)) */
    double       tol   /* tolerance */
) ;

double cg_dot
(
    double *x, /* first vector */
    double *y, /* second vector */
    INT     n  /* length of vectors */
) ;

void cg_copy
(
    double *y, /* output of copy */
    double *x, /* input of copy */
    int     n  /* length of vectors */
) ;

void cg_step
(
    double *xtemp, /*output vector */
    double     *x, /* initial vector */
    double     *d, /* search direction */
    double  alpha, /* stepsize */
    INT         n   /* length of the vectors */
) ;

int cg_line
(
    double  dphi0, /* function derivative at starting point (alpha = 0) */
    cg_com   *Com  /* cg com structure */
) ;

int cg_lineW
(
    double  dphi0, /* function derivative at starting point (alpha = 0) */
    cg_com   *Com  /* cg com structure */
) ;

int cg_update
(
    double        *a, /* left side of bracketing interval */
    double    *dphia, /* derivative at a */
    double        *b, /* right side of bracketing interval */
    double    *dphib, /* derivative at b */
    double    *alpha, /* trial step (between a and b) */
    double      *phi, /* function value at alpha (returned) */
    double     *dphi, /* function derivative at alpha (returned) */
    cg_com      *Com  /* cg com structure */
) ;

int cg_updateW
(
    double        *a, /* left side of bracketing interval */
    double    *dpsia, /* derivative at a */
    double        *b, /* right side of bracketing interval */
    double    *dpsib, /* derivative at b */
    double    *alpha, /* trial step (between a and b) */
    double      *phi, /* function value at alpha (returned) */
    double     *dphi, /* derivative of phi at alpha (returned) */
    double     *dpsi, /* derivative of psi at alpha (returned) */
    cg_com      *Com  /* cg com structure */
) ;

void cg_printParms
(
    cg_parameter  *Parm
) ;
