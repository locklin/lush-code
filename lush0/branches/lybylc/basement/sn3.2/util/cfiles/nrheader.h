#include "defnr.h"

float *vector();
float **matrix();
float **convert_matrix();
double *dvector();
double **dmatrix();
int *ivector();
int **imatrix();
float **submatrix();
void free_vector();
void free_dvector();
void free_ivector();
void free_matrix();
void free_dmatrix();
void free_imatrix();
void free_submatrix();
void free_convert_matrix();
void nrerror();

typedef struct FCOMPLEX {float r,i;} fcomplex;

extern fcomplex Cadd();
extern fcomplex Csub();
extern fcomplex Cmul();
extern fcomplex Complex();
extern fcomplex Conjg();
extern fcomplex Cdiv();
extern float Cabs();
extern fcomplex Csqrt();
extern fcomplex RCmul();

