/* exterior and dot products with square of one of the arguments */
/* this is only needed for computing second derivatives in lenet */

#include "header.h"
#include "dh.h"
#include "idxmac.h"
#include "idxops.h"

/* exterior product with square of second argument */
#define Midx_m1squextm1(i1, i2, o1, Type1, Type2, Type3) \
{ \
  register Type2 *c2; \
  register Type3 *d1; \
  Type1 *c1; \
  Type2 *c2_0; \
  Type3 *d1_0; \
  register int c2_m0 = (i2)->mod[0], \
               d1_m1 = (o1)->mod[1]; \
  int          c1_m0 = (i1)->mod[0], \
               d1_m0 = (o1)->mod[0]; \
  register int j, jmax = (o1)->dim[1]; \
  int i, imax = (o1)->dim[0]; \
  c1 = IDX_PTR((i1), Type1); \
  c2_0 = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++) { \
    d1 = d1_0; \
    c2 = c2_0; \
    for (j=0; j<jmax; j++) { \
      *d1 = (*c1)*(*c2)*(*c2); \
      d1 += d1_m1; \
      c2 += c2_m0; \
    } \
    d1_0 += d1_m0; \
    c1 += c1_m0; \
  } \
} 

#define Midx_m2squextm2(i1, i2, o1, Type1, Type2, Type3) \
{ \
  register Type2 *c2_0, *c2_1; \
  register Type3 *d1_2, *d1_3; \
  Type3  *d1_0, *d1_1; \
  Type1  *c1_0, *c1_1; \
  Type2  *ker; \
  register int c2_m0 = (i2)->mod[0], \
               c2_m1 = (i2)->mod[1], \
               d1_m2 = (o1)->mod[2], \
               d1_m3 = (o1)->mod[3]; \
  int          c1_m0 = (i1)->mod[0], \
               c1_m1 = (i1)->mod[1], \
               d1_m0 = (o1)->mod[0], \
               d1_m1 = (o1)->mod[1]; \
  register int k,l, lmax = (o1)->dim[3], kmax = (o1)->dim[2]; \
  int i,j, imax = (o1)->dim[0], jmax = (o1)->dim[1]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++) { \
    d1_1 = d1_0; \
    c1_1 = c1_0; \
    for (j=0; j<jmax; j++) { \
      d1_2 = d1_1; \
      c2_0 = ker; \
      for (k=0; k<kmax; k++) { \
        d1_3 = d1_2; \
        c2_1 = c2_0; \
        for (l=0; l<lmax; l++) { \
          *d1_3 = (*c1_1)*(*c2_1)*(*c2_1); \
          d1_3 += d1_m3; \
          c2_1 += c2_m1; \
        } \
        d1_2 += d1_m2; \
        c2_0 += c2_m0; \
      } \
      d1_1 += d1_m1; \
      c1_1 += c1_m1; \
    } \
    d1_0 += d1_m0; \
    c1_0 += c1_m0; \
  } \
}

/*           WITH accumulation */

#define Midx_m1squextm1acc(i1, i2, o1, Type1, Type2, Type3) \
{ \
  register Type2 *c2; \
  register Type3 *d1; \
  Type1 *c1; \
  Type2 *c2_0; \
  Type3 *d1_0; \
  register int c2_m0 = (i2)->mod[0], \
               d1_m1 = (o1)->mod[1]; \
  int          c1_m0 = (i1)->mod[0], \
               d1_m0 = (o1)->mod[0]; \
  register int j, jmax = (o1)->dim[1]; \
  int i, imax = (o1)->dim[0]; \
  c1 = IDX_PTR((i1), Type1); \
  c2_0 = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++) { \
    d1 = d1_0; \
    c2 = c2_0; \
    for (j=0; j<jmax; j++) { \
      *d1 += (*c1)*(*c2)*(*c2); \
      d1 += d1_m1; \
      c2 += c2_m0; \
    } \
    d1_0 += d1_m0; \
    c1 += c1_m0; \
  } \
} 


#define Midx_m2squextm2acc(i1, i2, o1, Type1, Type2, Type3) \
{ \
  register Type2 *c2_0, *c2_1; \
  register Type3 *d1_2, *d1_3; \
  Type1  *c1_0, *c1_1; \
  Type2  *ker; \
  Type3  *d1_0, *d1_1; \
  register int c2_m0 = (i2)->mod[0], \
               c2_m1 = (i2)->mod[1], \
               d1_m2 = (o1)->mod[2], \
               d1_m3 = (o1)->mod[3]; \
  int          c1_m0 = (i1)->mod[0], \
               c1_m1 = (i1)->mod[1], \
               d1_m0 = (o1)->mod[0], \
               d1_m1 = (o1)->mod[1]; \
  register int k,l, kmax = (o1)->dim[2], lmax = (o1)->dim[3]; \
  int i,j, imax = (o1)->dim[0], jmax = (o1)->dim[1]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++) { \
    d1_1 = d1_0; \
    c1_1 = c1_0; \
    for (j=0; j<jmax; j++) { \
      d1_2 = d1_1; \
      c2_0 = ker; \
      for (k=0; k<kmax; k++) { \
        d1_3 = d1_2; \
        c2_1 = c2_0; \
        for (l=0; l<lmax; l++) { \
          *d1_3 += (*c1_1)*(*c2_1)*(*c2_1); \
          d1_3 += d1_m3; \
          c2_1 += c2_m1; \
        } \
        d1_2 += d1_m2; \
        c2_0 += c2_m0; \
      } \
      d1_1 += d1_m1; \
      c1_1 += c1_m1; \
    } \
    d1_0 += d1_m0; \
    c1_0 += c1_m0; \
  } \
}

#define Midx_m2squdotm1(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1; \
  register Type2 *c2; \
  Type1 *c1_0; \
  Type2 *ker; \
  register int c1_m1 = (i1)->mod[1],   \
               c2_m0 = (i2)->mod[0];   \
  register int j, jmax = (i2)->dim[0]; \
  int          c1_m0 = (i1)->mod[0],   \
               d1_m0 = (o1)->mod[0];   \
  Type3 *d1, f; \
  int i, imax = (o1)->dim[0]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++){ \
    f = 0; \
    c1 = c1_0; \
    c2 = ker; \
    if(c1_m1==1 && c2_m0==1) \
      for (j=0; j<jmax; j++) { \
        f += (*c1)*(*c1)*(*c2++); \
	c1++; \
	}\
    else \
      for (j=0; j<jmax; j++) { \
	f += (*c1)*(*c1)*(*c2); \
        c1 += c1_m1; \
        c2 += c2_m0; \
      } \
    *d1 = f; \
     d1 += d1_m0; \
     c1_0 += c1_m0; \
  } \
}


/* multiply M4 by M2, result in M2 */
#define Midx_m4squdotm2(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1, *c1_2; \
  register Type2 *c2, *c2_0; \
  Type1 *c1_0, *c1_1; \
  Type2 *ker; \
  register int c1_m2 = (i1)->mod[2],   \
               c2_m0 = (i2)->mod[0],   \
               c1_m3 = (i1)->mod[3],   \
               c2_m1 = (i2)->mod[1];   \
  register int k,l, kmax = (i2)->dim[0], lmax = (i2)->dim[1]; \
  Type3 *d1_0, *d1, f; \
  int          c1_m0 = (i1)->mod[0],   \
               d1_m0 = (o1)->mod[0],   \
               c1_m1 = (i1)->mod[1],   \
               d1_m1 = (o1)->mod[1];   \
  int i,j, imax = (o1)->dim[0], jmax = (o1)->dim[1]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++){ \
    c1_1 = c1_0; \
    d1 = d1_0; \
    for (j=0; j<jmax; j++) { \
      f = 0; \
      c1_2 = c1_1; \
      c2_0 = ker; \
      for (k=0; k<kmax; k++) { \
        c1 = c1_2; \
        c2 = c2_0; \
        for (l=0; l<lmax; l++) { \
          f += (*c1)*(*c1)*(*c2); \
          c1 += c1_m3; \
          c2 += c2_m1; \
        } \
        c1_2 += c1_m2; \
        c2_0 += c2_m0; \
      } \
      *d1 = f; \
      d1 += d1_m1; \
      c1_1 += c1_m1; \
    } \
    d1_0 += d1_m0; \
    c1_0 += c1_m0; \
  } \
}

/*           with accumulation ============ */

/* multiply M2 by M1, result in M1 */
#define Midx_m2squdotm1acc(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1; \
  register Type2 *c2; \
  Type1  *c1_0; \
  Type2  *ker; \
  register int c1_m1 = (i1)->mod[1],   \
               c2_m0 = (i2)->mod[0];   \
  register int j, jmax = (i2)->dim[0]; \
  int          c1_m0 = (i1)->mod[0],   \
               d1_m0 = (o1)->mod[0];   \
  Type3 *d1, f; \
  int i, imax = (o1)->dim[0]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++){ \
    f = *d1; \
    c1 = c1_0; \
    c2 = ker; \
    for (j=0; j<jmax; j++) { \
      f += (*c1)*(*c1)*(*c2); \
      c1 += c1_m1; \
      c2 += c2_m0; \
    } \
    *d1 = f; \
    d1 += d1_m0; \
    c1_0 += c1_m0; \
  } \
}

/* multiply M4 by M2, result in M2 */
#define Midx_m4squdotm2acc(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1, *c1_2; \
  register Type2 *c2, *c2_0; \
  Type1 *c1_0, *c1_1; \
  Type2 *ker; \
  register int c1_m2 = (i1)->mod[2],   \
               c2_m0 = (i2)->mod[0],   \
               c1_m3 = (i1)->mod[3],   \
               c2_m1 = (i2)->mod[1];   \
  register int k,l, kmax = (i2)->dim[0], lmax = (i2)->dim[1]; \
  Type3 *d1_0, *d1, f; \
  int          c1_m0 = (i1)->mod[0],   \
               d1_m0 = (o1)->mod[0],   \
               c1_m1 = (i1)->mod[1],   \
               d1_m1 = (o1)->mod[1];   \
  int i,j, imax = (o1)->dim[0], jmax = (o1)->dim[1]; \
  c1_0 = IDX_PTR((i1), Type1); \
  ker = IDX_PTR((i2), Type2); \
  d1_0 = IDX_PTR((o1), Type3); \
  for (i=0; i<imax; i++){ \
    c1_1 = c1_0; \
    d1 = d1_0; \
    for (j=0; j<jmax; j++) { \
      f = *d1; \
      c1_2 = c1_1; \
      c2_0 = ker; \
      for (k=0; k<kmax; k++) { \
        c1 = c1_2; \
        c2 = c2_0; \
        for (l=0; l<lmax; l++) { \
          f += (*c1)*(*c1)*(*c2); \
          c1 += c1_m3; \
          c2 += c2_m1; \
        } \
        c1_2 += c1_m2; \
        c2_0 += c2_m0; \
      } \
      *d1 = f; \
      d1 += d1_m1; \
      c1_1 += c1_m1; \
    } \
    d1_0 += d1_m0; \
    c1_0 += c1_m0; \
  } \
}

/*                       accumulate result in a 0D vector */
/* compute square dot product M1^2 x M1 accumulated into M0 */ 
#define Midx_m1squdotm1acc(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1; \
  register Type2 *c2; \
  register int c1_m0 = (i1)->mod[0],   \
               c2_m0 = (i2)->mod[0];   \
  Type3 *d1, f; \
  register int i, imax = (i1)->dim[0]; \
  c1 = IDX_PTR((i1), Type1); \
  c2 = IDX_PTR((i2), Type2); \
  d1 = IDX_PTR((o1), Type3); \
  f = *d1; \
  for (i=0; i<imax; i++){ \
    f += (*c1)*(*c1)*(*c2); \
    c1 += c1_m0; \
    c2 += c2_m0; \
  } \
  *d1 = f; \
}

/* compute dot product M1 x M1 accumulated into M0 */ 
#define Midx_m2squdotm2acc(i1, i2, o1, Type1, Type2, Type3) \
{ register Type1 *c1; \
  register Type2 *c2; \
  Type1 *c1_0; \
  Type2 *c2_0; \
  register int j, imax = (i1)->dim[0], jmax = (i2)->dim[1]; \
  register int c1_m0 = (i1)->mod[0],   \
               c2_m0 = (i2)->mod[0];   \
  int          c1_m1 = (i1)->mod[1],   \
               c2_m1 = (i2)->mod[1];   \
  Type3 *d1, f; \
  int i;    \
  c1_0 = IDX_PTR((i1), Type1); \
  c2_0 = IDX_PTR((i2), Type2); \
  d1 = IDX_PTR((o1), Type3); \
  f = *d1; \
  for (i=0; i<imax; i++){ \
    c1 = c1_0; \
    c2 = c2_0; \
    for (j=0; j<jmax; j++) { \
      f += (*c1)*(*c1)*(*c2); \
      c1 += c1_m1; \
      c2 += c2_m1; \
    } \
    c1_0 += c1_m0; \
    c2_0 += c2_m0; \
  } \
  *d1 = f; \
}

/* ========= outer products for square back convolutions etc ================ */
#ifdef Midx_m1squextm1
void Cidx_m1squextm1(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m1squextm1(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m1squextm1) {
  dharg ret;
/* Midx_m1squextm1( a[1].dh_in, a[2].dh_in, a[3].dh_out, BASETYPE); */
  Midx_m1squextm1( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m1squextm1, Xidx_m1squextm1,       "Cidx_m1squextm1", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m1squextm1, Xidx_m1squextm1, "Cidx_m1squextm1", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,0), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_OUT,0,1), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifdef Midx_m2squextm2
void Cidx_m2squextm2(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m2squextm2(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m2squextm2) {
  dharg ret;
/* Midx_m2squextm2( a[1].dh_in, a[2].dh_in, a[3].dh_out, BASETYPE); */
  Midx_m2squextm2( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m2squextm2, Xidx_m2squextm2,       "Cidx_m2squextm2", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m2squextm2, Xidx_m2squextm2, "Cidx_m2squextm2", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 4),	/* DH_IDX4(DHT_OUT,0,1,2,3), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

/* ========= outer products for square back convolutions etc ================ */
/*           with accumulation */
#ifdef Midx_m1squextm1acc
void Cidx_m1squextm1acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m1squextm1acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m1squextm1acc) {
  dharg ret;
/* Midx_m1squextm1acc( a[1].dh_in, a[2].dh_in, a[3].dh_out, BASETYPE); */
  Midx_m1squextm1acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m1squextm1acc, Xidx_m1squextm1acc,       "Cidx_m1squextm1acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m1squextm1acc, Xidx_m1squextm1acc, "Cidx_m1squextm1acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,0), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_OUT,0,1), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifdef Midx_m2squextm2acc
void Cidx_m2squextm2acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m2squextm2acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m2squextm2acc) {
  dharg ret;
/* Midx_m2squextm2acc( a[1].dh_in, a[2].dh_in, a[3].dh_inout, BASETYPE); */
  Midx_m2squextm2acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m2squextm2acc, Xidx_m2squextm2acc,       "Cidx_m2squextm2acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m2squextm2acc, Xidx_m2squextm2acc, "Cidx_m2squextm2acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 4),	/* DH_IDX4(DHT_INOUT,0,1,2,3), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

/* ============== square convolutions mono and bi-dimensional ====== */
#ifdef Midx_m2squdotm1
void Cidx_m2squdotm1(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m2squdotm1(i1,i2, o1, BASETYPE, BASETYPE, BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m2squdotm1) {
  dharg ret;
/* Midx_m2squdotm1( a[1].dh_in, a[2].dh_in, a[3].dh_out, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m2squdotm1( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m2squdotm1, Xidx_m2squdotm1, "Cidx_m2squdotm1", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m2squdotm1, Xidx_m2squdotm1, "Cidx_m2squdotm1", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_OUT,0), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifdef Midx_m4squdotm2
void Cidx_m4squdotm2(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m4squdotm2(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m4squdotm2) {
  dharg ret;
/* Midx_m4squdotm2( a[1].dh_in, a[2].dh_in, a[3].dh_out, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m4squdotm2( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m4squdotm2, Xidx_m4squdotm2, "Cidx_m4squdotm2", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m4squdotm2, Xidx_m4squdotm2, "Cidx_m4squdotm2", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 4),	/* DH_IDX4(DHT_IN,0,1,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_OUT,0,1), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif


/* ============== convolutions mono and bi-dimensional ====== */
#ifdef Midx_m2squdotm1acc
void Cidx_m2squdotm1acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m2squdotm1acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m2squdotm1acc) {
  dharg ret;
/* Midx_m2squdotm1acc( a[1].dh_in, a[2].dh_in, a[3].dh_inout, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m2squdotm1acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m2squdotm1acc, Xidx_m2squdotm1acc,       "Cidx_m2squdotm1acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m2squdotm1acc, Xidx_m2squdotm1acc, "Cidx_m2squdotm1acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_INOUT,0), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifdef Midx_m4squdotm2acc
void Cidx_m4squdotm2acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m4squdotm2acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m4squdotm2acc) {
  dharg ret;
/* Midx_m4squdotm2acc( a[1].dh_in, a[2].dh_in, a[3].dh_inout, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m4squdotm2acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m4squdotm2acc, Xidx_m4squdotm2acc,       "Cidx_m4squdotm2acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m4squdotm2acc, Xidx_m4squdotm2acc, "Cidx_m4squdotm2acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 4),	/* DH_IDX4(DHT_IN,0,1,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,2,3), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_INOUT,0,1), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};

#endif
#endif

#ifdef Midx_m1squdotm1acc
void Cidx_m1squdotm1acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m1squdotm1acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m1squdotm1acc) {
  dharg ret;
/* Midx_m1squdotm1acc( a[1].dh_in, a[2].dh_in, a[3].dh_inout, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m1squdotm1acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m1squdotm1acc, Xidx_m1squdotm1acc,       "Cidx_m1squdotm1acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m1squdotm1acc, Xidx_m1squdotm1acc, "Cidx_m1squdotm1acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,0), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 1),	/* DH_IDX1(DHT_IN,0), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 0),	/* DH_IDX0(DHT_INOUT), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifdef Midx_m2squdotm2acc
void Cidx_m2squdotm2acc(i1,i2, o1) struct idx *i1, *i2, *o1; { 
    Midx_m2squdotm2acc(i1,i2, o1,BASETYPE,BASETYPE,BASETYPE); 
}
#ifndef NOLISP
DH(Xidx_m2squdotm2acc) {
  dharg ret;
/* Midx_m2squdotm2acc( a[1].dh_in, a[2].dh_in, a[3].dh_inout, BASETYPE,BASETYPE,BASETYPE); */
  Midx_m2squdotm2acc( a[1].dh_idx_ptr, a[2].dh_idx_ptr, a[3].dh_idx_ptr, BASETYPE,BASETYPE,BASETYPE);
/* a[4].dh_bool = 0; */
  ret.dh_bool = 0;
  return ret;
}
/* DHDOC(Kidx_m2squdotm2acc, Xidx_m2squdotm2acc,       "Cidx_m2squdotm2acc", NULL, DHT_BOOL, 0 ) { */
DHDOC (Kidx_m2squdotm2acc, Xidx_m2squdotm2acc, "Cidx_m2squdotm2acc", NULL, NULL) {
    DH_FUNC(3),
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 2),	/* DH_IDX2(DHT_IN,0,1), */
    DH_SRG (DHT_READ),
    DH_FLT,
    DH_IDX (DHT_READ, 0),	/* DH_IDX0(DHT_INOUT), */
    DH_SRG (DHT_WRITE),
    DH_FLT,
    DH_RETURN,
    DH_BOOL,
    DH_END_FUNC,
    DH_NIL,
};
#endif
#endif

#ifndef NOLISP
void
init_idxsquops()
{
#ifdef Midx_m1squextm1
  dh_define("idx_m1squextm1",&Kidx_m1squextm1);
#endif
#ifdef Midx_m2squextm2
  dh_define("idx_m2squextm2",&Kidx_m2squextm2);
#endif

#ifdef Midx_m1squextm1acc
  dh_define("idx_m1squextm1acc",&Kidx_m1squextm1acc);
#endif
#ifdef Midx_m2squextm2acc
  dh_define("idx_m2squextm2acc",&Kidx_m2squextm2acc);
#endif

#ifdef Midx_m2squdotm1
  dh_define("idx_m2squdotm1",&Kidx_m2squdotm1);
#endif
#ifdef Midx_m4squdotm2
  dh_define("idx_m4squdotm2",&Kidx_m4squdotm2);
#endif
#ifdef Midx_m2squdotm1acc
  dh_define("idx_m2squdotm1acc",&Kidx_m2squdotm1acc);
#endif
#ifdef Midx_m4squdotm2acc
  dh_define("idx_m4squdotm2acc",&Kidx_m4squdotm2acc);
#endif

#ifdef Midx_m1squdotm1acc
  dh_define("idx_m1squdotm1acc",&Kidx_m1squdotm1acc);
#endif
#ifdef Midx_m2squdotm2acc
  dh_define("idx_m2squdotm2acc",&Kidx_m2squdotm2acc);
#endif

}
#endif

int majver_idxsquops = 3;
int minver_idxsquops = 2;

