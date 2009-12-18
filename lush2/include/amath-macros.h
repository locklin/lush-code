/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann LeCun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as 
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *  MA  02110-1301  USA
 *
 ***********************************************************************/

/* support for applying math functions element-wise to arrays */
/* see arith.c and math.c for examples                        */


/* output may have different type than input */
#define UNARY_FUNC(i, o, Ti, To, FUNC) \
{ \
  Ti *ip = IND_BASE_TYPED((i), Ti); \
  To *op = IND_BASE_TYPED((o), To); \
  begin_idx_aloop2((i), (o), k, l) { \
      op[l] = FUNC(ip[k]); \
  } end_idx_aloop2((i), (o), k, l) \
}

/* output may have different type than input */
#define BINARY_FUNC(i1, i2, o, Ti, To, FUNC) \
{ \
  Ti *i1p = IND_BASE_TYPED((i1), Ti); \
  Ti *i2p = IND_BASE_TYPED((i2), Ti); \
  To *op = IND_BASE_TYPED((o), To); \
  begin_idx_aloop3((i1), (i2), (o), j, k, l) { \
      op[l] = FUNC(i1p[j], i2p[k]); \
  } end_idx_aloop3((i1), (i2), (o), j, k, l) \
}

#define DX_UNARY_FUNC(NAME, FUNC)                            \
DX(name2(x, NAME))                                           \
{                                                            \
  ARG_NUMBER(1);                                             \
  if (ISINDEX(1)) {                                          \
    index_t *res, *ind = AINDEX(1);                          \
    switch (IND_STTYPE(ind)) {                               \
                                                             \
    case ST_FLOAT:                                           \
      res = clone_array(ind);                                \
      UNARY_FUNC(ind, res, float, float, FUNC);              \
      break;                                                 \
                                                             \
    case ST_DOUBLE:                                          \
      res = clone_array(ind);                                \
      UNARY_FUNC(ind, res, double, double, FUNC);            \
      break;                                                 \
                                                             \
    default: {                                               \
      index_t *dind = as_double_array(APOINTER(1));          \
      res = clone_array(dind);                               \
      UNARY_FUNC(dind, res, double, double, FUNC);           \
    }}                                                       \
    return res->backptr;                                     \
  } else if (ISNUMBER(1)) {                                  \
    return NEW_NUMBER(FUNC(AREAL(1)));                       \
  } else                                                     \
    RAISEF("not an index nor a number", APOINTER(1));        \
  return NIL;                                                \
}

/* result of these is of type int */
#define DX_UNARY_FUNC_TO_INT(NAME, FUNC)                     \
DX(name2(x, NAME))                                           \
{                                                            \
  ARG_NUMBER(1);                                             \
  if (ISINDEX(1)) {                                          \
    index_t *res, *ind = AINDEX(1);                          \
    switch (IND_STTYPE(ind)) {                               \
                                                             \
    case ST_FLOAT:                                           \
      res = make_array(ST_INT, IND_SHAPE(ind), NIL);         \
      UNARY_FUNC(ind, res, float, int, FUNC);                \
      break;                                                 \
                                                             \
    case ST_DOUBLE:                                          \
      res = make_array(ST_INT, IND_SHAPE(ind), NIL);         \
      UNARY_FUNC(ind, res, double, int, FUNC);               \
      break;                                                 \
                                                             \
    default: {                                               \
      index_t *dind = as_double_array(APOINTER(1));          \
      res = make_array(ST_INT, IND_SHAPE(dind), NIL);        \
      UNARY_FUNC(dind, res, double, int, FUNC);              \
    }}                                                       \
    return res->backptr;                                     \
  } else if (ISNUMBER(1)) {                                  \
    return NEW_NUMBER(FUNC(AREAL(1)));                       \
  } else                                                     \
    RAISEF("not an index nor a number", APOINTER(1));        \
  return NIL;                                                \
}

/* derived from 'Midx_maTOP' in include/idxops.h */
#define BINARY_OP(i0, i1, i2, Type1, Type2, Type3, OP) \
{ \
  Type1 *fp0; \
  Type2 *fp1; \
  Type3 *fp2; \
  fp0 = IND_BASE_TYPED(i0, Type1); \
  fp1 = IND_BASE_TYPED(i1, Type2); \
  fp2 = IND_BASE_TYPED(i2, Type3); \
  begin_idx_aloop3((i0), (i1), (i2), k0, k1, k2) { \
      fp2[k2] = OP(fp0[k0], fp1[k1]); \
  } end_idx_aloop3((i0), (i1), (i2), k0, k1, k2); \
}

/* derived from 'mafop' in include/idxops.h */
#define UNARY_BINOP(i1, i2, Type1, Type2, OP, NELEM) \
{ Type1 *c1; \
  Type2 *c2; \
  c1 = IND_BASE_TYPED((i1), Type1); \
  c2 = IND_BASE_TYPED((i2), Type2); \
  begin_idx_aloop2( (i1), (i2), k, l) { \
      c2[l] = OP(NELEM, c1[k]); \
  } end_idx_aloop2( (i1), (i2), k, l) \
}

#define DX_COMMUTATIVE_BINARY_OP(NAME, OP, NELEM)            \
DX(name2(x, NAME))                                           \
{                                                            \
  double sum = NELEM;                                        \
  int i = 0;                                                 \
  while (++i <= arg_number) {                                \
    if (ISINDEX(i))                                          \
      break;                                                 \
    sum = OP(sum, AREAL(i));                                 \
  }                                                          \
  at *atsum = NEW_NUMBER(sum);                               \
  if (i>arg_number)                                          \
    return atsum;                                            \
                                                             \
  /* we encountered some index argument */                   \
  index_t *res = as_double_array(atsum);                     \
                                                             \
  /* pick up where you left off */                           \
  i--;                                                       \
  while (++i <= arg_number) {                                \
    index_t *a = res;                                        \
    index_t *b = DOUBLE_ARRAY_P(APOINTER(i)) ? AINDEX(i) :   \
                 as_double_array(APOINTER(i));               \
                                                             \
    if (shape_equalp(IND_SHAPE(a), IND_SHAPE(b))) {          \
      /* handle special case fast */                         \
      BINARY_OP(a, b, res, double, double, double, OP);      \
                                                             \
    } else {                                                 \
      /* general case */                                     \
      index_t *ba = NULL;                                    \
      index_t *bb = NULL;                                    \
      shape_t *shp = index_broadcast2(a, b, &ba, &bb);       \
      ifn (shape_equalp(shp, IND_SHAPE(res))) {              \
	res = make_array(ST_DOUBLE, shp, NIL);               \
      }                                                      \
      BINARY_OP(ba, bb, res, double, double, double, OP);    \
    }                                                        \
  }                                                          \
  return res->backptr;                                       \
}

#define DX_NONCOMMUTATIVE_BINARY_OP(NAME, OP, NELEM)         \
                                                             \
static at *name2(unary, NAME)(at *arg) {                     \
                                                             \
  if (NUMBERP(arg))                                          \
    return NEW_NUMBER(OP(NELEM, Number(arg)));               \
                                                             \
  else if (INDEXP(arg)) {                                    \
    index_t *ind = Mptr(arg);                                \
    index_t *res = clone_array(ind);                         \
    UNARY_BINOP(ind, res, double, double, OP, NELEM);        \
    return res->backptr;                                     \
                                                             \
  } else {                                                   \
    RAISE(NIL, "neither a number nor an array", arg);        \
    return NIL; /* avoid compiler warning */                 \
  }                                                          \
}                                                            \
DX(name2(x, NAME))                                           \
{                                                            \
  if (arg_number<1)                                          \
    ARG_NUMBER(-1);                                          \
                                                             \
  /* case 1: one argument */                                 \
  if (arg_number==1)                                         \
    return name2(unary, NAME)(APOINTER(1));                  \
                                                             \
  /* case 2: more than one argument */                       \
  double sum = 0;                                            \
  at *atsum;                                                 \
  int i = 1;                                                 \
                                                             \
  if (ISINDEX(1)) {                                          \
    atsum = APOINTER(1);                                     \
    i++;                                                     \
    goto indexresult;                                        \
  } else                                                     \
    sum = AREAL(1);                                          \
                                                             \
  while (++i <= arg_number) {                                \
    if (ISINDEX(i))                                          \
      break;                                                 \
    sum = OP(sum, AREAL(i));                                 \
  }                                                          \
                                                             \
  atsum = NEW_NUMBER(sum);                                   \
  if (i>arg_number)                                          \
    return atsum;                                            \
                                                             \
  indexresult:                                               \
  /* we encountered some index argument */                   \
  ;                                                          \
  index_t *res = DOUBLE_ARRAY_P(atsum) ?                     \
    copy_array((index_t *)Mptr(atsum)) :                     \
    as_double_array(atsum);                                  \
                                                             \
  /* pick up where you left off */                           \
  i--;                                                       \
  while (++i <= arg_number) {                                \
    index_t *a = res;                                        \
    index_t *b = DOUBLE_ARRAY_P(APOINTER(i)) ? AINDEX(i) :   \
                 as_double_array(APOINTER(i));               \
                                                             \
    if (shape_equalp(IND_SHAPE(a), IND_SHAPE(b))) {          \
      /* handle special case fast */                         \
      BINARY_OP(a, b, res, double, double, double, OP);      \
                                                             \
    } else {                                                 \
      /* general case */                                     \
      index_t *ba = NULL;                                    \
      index_t *bb = NULL;                                    \
      shape_t *shp = index_broadcast2(a, b, &ba, &bb);       \
      ifn (shape_equalp(shp, IND_SHAPE(res))) {              \
	res = make_array(ST_DOUBLE, shp, NIL);               \
      }                                                      \
      BINARY_OP(ba, bb, res, double, double, double, OP);    \
    }                                                        \
  }                                                          \
  return res->backptr;                                       \
}
