
#include "header.h"
#include "dh.h"
#include "idxmac.h"
#include "idxops.h"
#include "nrheader.h"

#define gen_malloc(a) alloca(a)
#define gen_free(a) ;

void ludcmp(), lubksb(), sort(), sort2(), tred2(), tred2nov(), tqli(),
    tqlinov(), svdcmp(), svbksb(), indexx();

int *make_nr_idx1_int(index)
    struct idx *index;
{
    if(index->mod[0] != 1)
	run_time_error("nr: Cannot handle this submatrix (copy it)\n");
    return IDX_PTR(index, int) -1;
}

extern double **dmake_nr_idx2();
extern double *dmake_nr_idx1();

/******************************************************************************
 *
 *  (ludcmp <a> <d> <r>) 
 *	Compute the LU decomposition of idx2.  On return <a> contains the LU
 *	decomposition, <d> contains the permutations, and <r> whatever.
 *
 ******************************************************************************/
int
C_nr_ludcmp (PL0_a, PL0_indx, PL0_d)
struct idx *PL0_a;
struct idx *PL0_indx;
struct idx *PL0_d;
{
    int i,j, n_dim, *ind;
    double **mat, *indx, *temp, res;
    
    n_dim = PL0_indx->dim[0];
    mat = dmake_nr_idx2(PL0_a);

    indx = dmake_nr_idx1(PL0_indx);
    /* ind need to be contiguous */
    ind = (int *) gen_malloc(n_dim*sizeof(int *));
    if (!ind) nrerror("allocation failure in ludcmp");

    ludcmp(mat, n_dim, ind-1, &res);
    for(i=0;i<n_dim;i++)
	indx[i+1] = ind[i];

    free(mat+1);
    gen_free(ind); 
    *IDX_PTR(PL0_d, double) = res;

    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_ludcmp)
{
    dharg ret;
    ret.dh_bool = C_nr_ludcmp (a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_ludcmp, X_nr_ludcmp, "C_nr_ludcmp", NULL, NULL)
{
    DH_FUNC(3),
    DH_IDX (DHT_WRITE, 2),
    DH_SRG (DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 0),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *  (lubksb <a> <d> <b>) 
 *	Solve the system a.x = b.  On entry <a> contains a  LU decomposition,
 *      <d> contains the permutations, and <b> the vector of the equation.  
 *	On exit <b> contains <x>.
 *
 ******************************************************************************/
int
C_nr_lubksb (PL0_a, PL0_b, PL0_c)
struct idx *PL0_a;
struct idx *PL0_b;
struct idx *PL0_c;
{
    int i,j, n_dim, *index;
    double **mat, *indx, *b, res;
    
    n_dim = PL0_a->dim[0];

    mat = dmake_nr_idx2(PL0_a);
    indx = dmake_nr_idx1(PL0_b);
    b = dmake_nr_idx1(PL0_c);

    index = (int *) gen_malloc(n_dim*sizeof(int *));
    if (!index) nrerror("allocation failure in lubksb");
    for(i=0;i<n_dim;i++)
	index[i] = indx[i+1];

    lubksb(mat, n_dim, index-1, b);

    free(mat+1);
    gen_free(index); 
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_lubksb)
{
    dharg ret;
    ret.dh_bool = C_nr_lubksb (a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_lubksb, X_nr_lubksb, "C_nr_lubksb", NULL, NULL)
{
    DH_FUNC(3),
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif


/*
 * C_FUNCTION
 */
int
C_nr_sort (PL0_a)
struct idx *PL0_a;
{
    int n_dim;
    double *ra;
    
    n_dim = PL0_a->dim[0];
    ra = dmake_nr_idx1(PL0_a);

    if (n_dim>1)
	sort(n_dim,ra);
    return 0;
}

/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_sort)
{
    dharg ret;
    ret.dh_bool = C_nr_sort (a[1].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_sort, X_nr_sort, "C_nr_sort", NULL, NULL)
{
    DH_FUNC(1),
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif


/*
 * C_FUNCTION
 */
int
C_nr_sort2 (PL0_a, PL0_b)
struct idx *PL0_a;
struct idx *PL0_b;
{
    int n_dim;
    double *ra, *rb;
    
    n_dim = PL0_a->dim[0];
    ra = dmake_nr_idx1(PL0_a);
    rb = dmake_nr_idx1(PL0_b);
    
    if (n_dim>1)
	sort2(n_dim,ra,rb);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_sort2)
{
    dharg ret;
    ret.dh_bool = C_nr_sort2 (a[1].dh_idx_ptr, a[2].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_sort2, X_nr_sort2, "C_nr_sort2", NULL, NULL)
{
    DH_FUNC(2),
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
 

/*
 * C_FUNCTION
 */
int
C_nr_indexx (PL0_a, PL0_b)
struct idx *PL0_a;
struct idx *PL0_b;
{
    int n_dim;
    real *ra;
    int *rb;
    
    n_dim = PL0_a->dim[0];
    ra = dmake_nr_idx1(PL0_a);
    rb = make_nr_idx1_int(PL0_b);
    
    indexx(n_dim,ra,rb);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_indexx)
{
    dharg ret;
    ret.dh_bool = C_nr_indexx (a[1].dh_idx_ptr, a[2].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_indexx, X_nr_indexx, "C_nr_indexx", NULL, NULL)
{
    DH_FUNC(2),
    DH_IDX (DHT_READ, 1),
    DH_SRG(DHT_READ),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_INT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *   --------- EIGENVALUES/ EIGENVECTORS -----------
 *
******************************************************************************/
int
C_nr_tred2 (PL0_a, PL0_b, PL0_c)
struct idx *PL0_a;
struct idx *PL0_b;
struct idx *PL0_c;
{
    int n_dim;
    double **mat, *d, *e;
    
    n_dim = PL0_a->dim[0];
    mat = dmake_nr_idx2(PL0_a);
    d = dmake_nr_idx1(PL0_b);
    e = dmake_nr_idx1(PL0_c);

    tred2(mat, n_dim, d, e);

    free(mat+1);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_tred2)
{
    dharg ret;
    ret.dh_bool = C_nr_tred2 (a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_tred2, X_nr_tred2, "C_nr_tred2", NULL, NULL)
{
    DH_FUNC(3),
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *  same as tred2 but does not compute eigenvectors
 *
******************************************************************************/
int
C_nr_tred2nov (PL0_a, PL0_b, PL0_c)
struct idx *PL0_a;
struct idx *PL0_b;
struct idx *PL0_c;
{
    int n_dim;
    double **mat, *d, *e;
    
    n_dim = PL0_a->dim[0];
    mat = dmake_nr_idx2(PL0_a);
    d = dmake_nr_idx1(PL0_b);
    e = dmake_nr_idx1(PL0_c);

    tred2nov(mat, n_dim, d, e);

    free(mat+1);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_tred2nov)
{
    dharg ret;
    ret.dh_bool = C_nr_tred2nov (a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_tred2nov, X_nr_tred2nov, "C_nr_tred2nov", NULL, NULL)
{
    DH_FUNC(3),
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *  tqli
 *
******************************************************************************/
int 
C_nr_tqli (PL0_b, PL0_c, PL0_a)
struct idx *PL0_b;
struct idx *PL0_c;
struct idx *PL0_a;
{
    int n_dim;
    double **mat, *d, *e;
    
    n_dim = PL0_a->dim[0];
    d = dmake_nr_idx1(PL0_b);
    e = dmake_nr_idx1(PL0_c);
    mat = dmake_nr_idx2(PL0_a);

    tqli(d, e, n_dim, mat);

    free(mat+1);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_tqli)
{
    dharg ret;
    ret.dh_bool = C_nr_tqli (a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_tqli, X_nr_tqli, "C_nr_tqli", NULL, NULL)
{
    DH_FUNC(3),
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *  tqlinov: same as tqli but does not compute eigenvectors
 *
******************************************************************************/
int
C_nr_tqlinov (PL0_b, PL0_c)
struct idx *PL0_b;
struct idx *PL0_c;
{
    int n_dim;
    double **mat, *d, *e;
    
    n_dim = PL0_b->dim[0];
    d = dmake_nr_idx1(PL0_b);
    e = dmake_nr_idx1(PL0_c);

    tqlinov(d, e, n_dim);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_tqlinov)
{
    dharg ret;
    ret.dh_bool = C_nr_tqlinov (a[1].dh_idx_ptr, a[2].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_tqlinov, X_nr_tqlinov, "C_nr_tqlinov", NULL, NULL)
{
    DH_FUNC(2),
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif

/******************************************************************************
 *
 *  (nr-svdcmp <a> <w> <v> <d>)                            
 *
 * Computes the singular value decomposition of a <m>x<n> matrix <a>.  The
 * premultiplication matrix is stored in <a>, the eigenvalues are stored in
 * vector <w> of size <n>, and the transposed postmultiplication matrix is 
 * stored in the <n>x<n> matrix v.
 *
 * This function nullifies
 * all eigenvalues smaller than <d> times the maximal eigenvalue.  A typical
 * value for <d> is <1e-6>.  This is especially useful for using <nr-svbksb>:
 * It removes equations that have probably been corrupted by numerical noise.
 *
 * This function returns the empty list.
 *
 ******************************************************************************/
int 
C_nr_svdcmp(PL0_a, PL0_w, PL0_v, L0_d)
struct idx *PL0_a;
struct idx *PL0_w;
struct idx *PL0_v;
double L0_d;
{
    int i, ma, na, *ind;
    double **mat, **v, *w, limit;
    
    ma = PL0_a->dim[0];
    na = PL0_a->dim[1];
    mat = dmake_nr_idx2(PL0_a);
    w = dmake_nr_idx1(PL0_w);
    v = dmake_nr_idx2(PL0_v);

    svdcmp(mat,ma,na,w,v);

    /* clip eigenvalues if a fourth argument is there */
    
    limit = w[1];
    for (i=2; i<=na; i++)
	if (w[i]>limit)
	    limit=w[i];
    limit *= L0_d;
    for (i=1; i<=na; i++)
	if (w[i]<limit)
	    w[i]=0.0;

    free(mat+1);
    free(v+1); 
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_svdcmp)
{
    dharg ret;
    ret.dh_bool = C_nr_svdcmp (a[1].dh_idx_ptr, a[2].dh_idx_ptr,
				   a[3].dh_idx_ptr, a[4].dh_real);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_svdcmp, X_nr_svdcmp, "C_nr_svdcmp", NULL, NULL)
{
    DH_FUNC(4),
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif


/******************************************************************************
 *  
 *   (nr-svbksb u w v b x)  
 *  
 *   Solves equation <a.x=b> for a vector <x>.
 *  
 *   The <m>x<n> matrix <a> is the product of the <m>x<n> matrix <u>, the 
 *  diagonal matrix specified by the eigenvalues vector <w> of size <n> and the
 *  <n>x<n> matrix <v>.  These matrices are returned by the function 
 *  <nr-svdcmp>.
 *  
 *   The right hand side of the equation is specified by the vector <b> of size
 *  <m>.  The result is stored into vector <x>, of size <n>.  This function
 *  returns the vector <x>.
 *  
 *   Solving a big system by using <nr-svdcmp> then <nr-svbksb> gives much 
 *  better result than a brute force solution, if you remember to clip the
 *  smallest eigenvalues.  This method is called "regularization of an 
 *  ill-posed problem". 
 *
 *  It is however five times slower, at least, than the usual LU decomposition
 *  solving algorithm.
 *  
 *  A much faster regularization method works with positive symmetric matrices.
 *  Before using the usual LU decomposition solving method, you might add a 
 *  small positive value (like <1e-6>) to the elements of the diagonal of the
 *  matrix.
******************************************************************************/

int 
C_nr_svbksb (PL0_u, PL0_w, PL0_v, PL0_b, PL0_x)
    struct idx *PL0_u;
    struct idx *PL0_w;
    struct idx *PL0_v;
    struct idx *PL0_b;
    struct idx *PL0_x;
{
    double **u, *w, **v, *b, *x;
    int m,n,i,j;

    m = PL0_u->dim[0];
    n = PL0_u->dim[1];
    u = dmake_nr_idx2(PL0_u);
    w = dmake_nr_idx1(PL0_w);
    v = dmake_nr_idx2(PL0_v);
    b = dmake_nr_idx1(PL0_b);
    x = dmake_nr_idx1(PL0_x);
    svbksb(u,w,v,m,n,b,x);
    free(u+1);
    free(v+1);
    return 0;
}


/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_nr_svbksb)
{
    dharg ret;
    ret.dh_bool = C_nr_svbksb (a[1].dh_idx_ptr, a[2].dh_idx_ptr,
			       a[3].dh_idx_ptr, a[4].dh_idx_ptr,
			       a[5].dh_idx_ptr);
    return ret;
}


/*
 * K_RECORD
 */
DHDOC (K_nr_svbksb, X_nr_svbksb, "C_nr_svbksb", NULL, NULL)
{
    DH_FUNC(5),
    DH_IDX (DHT_READ, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_READ, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_READ, 2),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_READ, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_IDX (DHT_WRITE, 1),
    DH_SRG(DHT_WRITE),
    DH_REAL,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif


#ifndef NOLISP
/*
 * INIT FUNCTION
 */
void 
    init_nr_some_double ()
{
    dh_define ("nr-ludcmp", &K_nr_ludcmp);
    dh_define ("nr-lubksb", &K_nr_lubksb);
    dh_define ("nr-sort", &K_nr_sort);
    dh_define ("nr-sort2", &K_nr_sort2);
    dh_define ("nr-indexx", &K_nr_indexx);     
    dh_define ("nr-tred2", &K_nr_tred2);
    dh_define ("nr-tred2nov", &K_nr_tred2nov);

    dh_define ("nr-svdcmp", &K_nr_svdcmp);
    dh_define ("nr-svbksb", &K_nr_svbksb);
    
    dh_define ("nr-tqli", &K_nr_tqli);
    dh_define ("nr-tqlinov", &K_nr_tqlinov);
}

int majver_nr_some_double = 3;
int minver_nr_some_double = 1;

#endif
