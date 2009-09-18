/* Although there is a rigorous theory justifying a Wolfe line search,
   the performance of the Approximate Wolfe line search is often superior.
   Nonetheless, the user can turn off the Approximate Wolfe line search
   by setting AWolfe to FALSE and AWolfeFac to 0.  (Since AWolfe is
   FALSE by default, we only need to adjust AWolfeFac). When the
   code detects that the Wolfe line search fails, then it will
   automatically attempt the approximate Wolfe line search. To
   see that the Wolfe line search failed, we also need to set the
   PrintLevel to at least 1.  The synopsis of the output is the following:

   ....
   iter:    28 f =  -6.530787e+02 gnorm =   2.893704e-07 AWolfe =  0
   QuadOK:  0 initial a:   3.503331e-01 f0:  -6.530787e+02 dphi:  -5.261712e-13
   Wolfe line search

   WOLFE LINE SEARCH FAILS
   Approximate Wolfe line search

   iter:    29 f =  -6.530787e+02 gnorm =   1.420203e-07 AWolfe =  1
   QuadOK:  0 initial a:   3.860072e-01 f0:  -6.530787e+02 dphi:  -2.202809e-13
   Approximate Wolfe line search
   ....
   Termination status: 0
   Convergence tolerance for gradient satisfied
   maximum norm for gradient:  7.794164e-09
   function value:            -6.530787e+02

   cg  iterations:                  34
   function evaluations:           157
   gradient evaluations:           150


   The large number of function and gradient evaluations arose when the
   Wolfe line search failed.  If the error tolerance is increased to 1.e-6,
   then the Wolfe line search is successful, giving the following results:

   Termination status: 0
   Convergence tolerance for gradient satisfied
   maximum norm for gradient:  7.801362e-07
   function value:            -6.530787e+02

   cg  iterations:                  26
   function evaluations:            49
   gradient evaluations:            33
   
   On the other hand, if we turn on the Approximate Wolfe line search by
   resetting AWolfeFac to its default value, we
   obtain slightly faster convergence:

   Termination status: 0
   Convergence tolerance for gradient satisfied
   maximum norm for gradient:  9.667383e-07
   function value:            -6.530787e+02

   cg  iterations:                  25
   function evaluations:            48
   gradient evaluations:            31 */

#include <math.h>
#include "cg_user.h"

double myvalue
(
    double   *x,
    INT       n
) ;

void mygrad
(
    double    *g,
    double    *x,
    INT        n
) ;

double myvalgrad
(
    double    *g,
    double    *x,
    INT        n
) ;

int main (void)
{
    double *x ;
    INT i, n ;
    cg_parameter Parm ;

    /* allocate work space */
    n = 100 ;
    x = (double *) malloc (n*sizeof (double)) ;

    /* starting guess */
    for (i = 0; i < n; i++) x [i] = 1. ;

    /* set default parameter values */
    cg_default (&Parm) ;

    /* turn off approximate Wolfe line search */
    Parm.AWolfeFac = 0. ;
    Parm.PrintLevel = 1 ;

    /* solve the problem with error tolerance 1.e-8 */
    cg_descent(x, n, NULL, &Parm, 1.e-8, myvalue, mygrad, myvalgrad, NULL) ;

    /* starting guess */
    for (i = 0; i < n; i++) x [i] = 1. ;

Parm.PrintLevel = 0 ;
    /* solve the problem with error tolerance 1.e-6 */
    cg_descent(x, n, NULL, &Parm, 1.e-6, myvalue, mygrad, myvalgrad, NULL) ;

    /* starting guess */
    for (i = 0; i < n; i++) x [i] = 1. ;

    /* restore default parameter value for AWolfeFac */
    cg_default (&Parm) ;

    /* solve the problem with error tolerance 1.e-8 */
    cg_descent(x, n, NULL, &Parm, 1.e-6, myvalue, mygrad, myvalgrad, NULL) ;

    free (x) ; /* free work space */
}

double myvalue
(
    double   *x,
    INT       n
)
{
    double f, t ;
    INT i ;
    f = 0. ;
    for (i = 0; i < n; i++)
    {
        t = i+1 ;
        t = sqrt (t) ;
        f += exp (x [i]) - t*x [i] ;
    }
    return (f) ;
}

void mygrad
(
    double    *g,
    double    *x,
    INT        n
)
{
    double t ;
    INT i ;
    for (i = 0; i < n; i++)
    {
        t = i + 1 ;
        t = sqrt (t) ;
        g [i] = exp (x [i]) -  t ;
    }
    return ;
}

double myvalgrad
(
    double    *g,
    double    *x,
    INT        n
)
{
    double ex, f, t ;
    INT i ;
    f = (double) 0 ;
    for (i = 0; i < n; i++)
    {
        t = i + 1 ;
        t = sqrt (t) ;
        ex = exp (x [i]) ;
        f += ex - t*x [i] ;
        g [i] = ex -  t ;
    }
    return (f) ;
}
