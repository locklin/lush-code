/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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

/******************************************************************************
   The following macros are a hack.  i1, i2, and i3 are assumed to be
   defined.  Depending on the use, case_type2 will be undefined and
   redefined and will use some of i1, i2 and i3.  For this reason,
   i1, i2 and i3 do not appear as arguments (which varies).  The is
   the tree call for the macros: 

   switch_type1-->case_type1-->switch_type2-->case_type2+->case_type2_1arg
                                           |               case_type2_2arg
                                           |               case_type2_3arg
                  case_type1 --------------+

******************************************************************************/

#define case_type2_1arg(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      FUNC_NAME(ind1, t1); \
      break; 
#define case_type2_1arg1par(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      FUNC_NAME(ind1, val, t1); \
      break; 
#define case_type2_2arg(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      check_number_of_type \
      FUNC_NAME(ind1, ind2, t1, t2); \
      break; 
#define case_type2_3arg(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      check_number_of_type \
      FUNC_NAME(ind1, ind2, ind3, t1, t1, t2); \
      break; \

#define case_type2(storage_type, t1, t2, FUNC_NAME) \
    case_type2_2arg(storage_type, t1, t2, FUNC_NAME) 

#ifdef SHORT_IDX
#define switch_type2(t1, FUNC_NAME) \
    switch((ind2)->st->type) { \
    case_type2(ST_INT, t1, int, FUNC_NAME) \
    case_type2(ST_FLOAT, t1, float, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#else
#define switch_type2(t1, FUNC_NAME) \
    switch((ind2)->st->type) { \
    case_type2(ST_UCHAR, t1, unsigned char, FUNC_NAME) \
    case_type2(ST_CHAR, t1, char, FUNC_NAME) \
    case_type2(ST_SHORT, t1, short, FUNC_NAME) \
    case_type2(ST_INT, t1, int, FUNC_NAME) \
    case_type2(ST_FLOAT, t1, float, FUNC_NAME) \
    case_type2(ST_DOUBLE, t1, double, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#endif

#ifdef SHORT_IDX
#define switch_type3(t1, FUNC_NAME) \
    switch((ind3)->st->type) { \
    case_type2(ST_INT, t1, int, FUNC_NAME) \
      case_type2(ST_FLOAT, t1, float, FUNC_NAME)    \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#else
#define switch_type3(t1, FUNC_NAME) \
    switch((ind3)->st->type) { \
    case_type2(ST_UCHAR, t1, unsigned char, FUNC_NAME) \
    case_type2(ST_CHAR, t1, char, FUNC_NAME) \
    case_type2(ST_SHORT, t1, short, FUNC_NAME) \
    case_type2(ST_INT, t1, int, FUNC_NAME) \
    case_type2(ST_FLOAT, t1, float, FUNC_NAME) \
    case_type2(ST_DOUBLE, t1, double, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#endif

#define check_unitype2 \
    if((ind1)->st->type != (ind2)->st->type) \
       error(NIL, "All idxs must be of the same type", NIL);
#define check_unitype3 \
    if(((ind1)->st->type != (ind3)->st->type) || \
       ((ind1)->st->type != (ind2)->st->type)) \
       error(NIL, "All idxs must be of the same type", NIL);
#define check_multitype2
#define check_multitype3 \
    if((ind1)->st->type != (ind2)->st->type) \
       error(NIL, "Input idxs must be of the same type", NIL);

#define unfold_switch(storage_type,t1, FUNC_NAME) \
    case storage_type: \
      switch_type2(t1, FUNC_NAME); \
      break; 
#define case_type1(storage_type, t1, FUNC_NAME) \
    unfold_switch(storage_type,t1, FUNC_NAME)

#undef check_number_of_type
#undef case_type1
#ifdef UNITYPE
#define check_number_of_type check_unitype2
#define case_type1(storage_type, t1, FUNC_NAME) \
    case_type2(storage_type, t1, t1, FUNC_NAME)
#else
#define check_number_of_type check_multitype
#define case_type1(storage_type, t1, FUNC_NAME) \
    unfold_switch(storage_type,t1, FUNC_NAME)
#endif

#ifdef SHORT_IDX
#define switch_type1(FUNC_NAME) \
    switch((ind1)->st->type) { \
    case_type1(ST_INT, int, FUNC_NAME) \
    case_type1(ST_FLOAT, float, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for first argument", NIL); \
    } 
#else
#define switch_type1(FUNC_NAME) \
    switch((ind1)->st->type) { \
    case_type1(ST_UCHAR, unsigned char, FUNC_NAME) \
    case_type1(ST_CHAR, char, FUNC_NAME) \
    case_type1(ST_SHORT, short, FUNC_NAME) \
    case_type1(ST_INT, int, FUNC_NAME) \
    case_type1(ST_FLOAT, float, FUNC_NAME) \
    case_type1(ST_DOUBLE, double, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for first argument", NIL); \
    } 
#endif

/* ================================================================ */
/* Macros that define function with one output index */

#define Xidx_o(FUNC_NAME, CHECK_FUNC)           \
  DX(name2(Xidx_,FUNC_NAME))                    \
  {                                             \
    ARG_NUMBER(1);                              \
    index_t *ind1 = AINDEX(1);                  \
    CHECK_FUNC(ind1);                           \
    switch_type1(name2(Midx_,FUNC_NAME));       \
    return APOINTER(1);                         \
  }
  
/* ================================================================ */
/* Macros that define function with one input index and one input parameter */

#define Xidx_po(FUNC_NAME, CHECKFUNC)           \
  DX(name2(Xidx_,FUNC_NAME))                    \
  {                                             \
    ARG_NUMBER(2);                              \
    index_t *ind1 = AINDEX(1);                  \
    double val = AREAL(2);                      \
    CHECKFUNC(ind1);                            \
    switch_type1(name2(Midx_,FUNC_NAME));       \
    at *p = APOINTER(1);                        \
    return p;                                   \
  }

/* ================================================================ */
/* Macros that define function with one input index and one output index */

#define Xidx_io0(FUNC_NAME, CHECK_FUNC)                   \
  DX(name2(Xidx_,FUNC_NAME))                              \
  {                                                       \
    at *p2;                                               \
    index_t *ind1, *ind2;                                 \
    if (arg_number==1) {                                  \
      ind1 = AINDEX(1);                                   \
      ind2 = make_array(IND_STTYPE(ind1), SHAPE0D, NIL);  \
      p2 = ind2->backptr;                                 \
    } else {                                              \
      ARG_NUMBER(2);                                      \
      ind1 = AINDEX(1);                                   \
      p2 = APOINTER(2);                                   \
      ind2 = AINDEX(2);                                   \
    }                                                     \
    if(IND_NDIMS(ind2) != 0) ERRBADARGS;                  \
    CHECK_FUNC(ind1, ind2);                               \
    switch_type1(name2(Midx_,FUNC_NAME));                 \
    return p2;                                            \
  }

#define Xidx_ioa(FUNC_NAME, CHECK_FUNC)                \
  DX(name2(Xidx_,FUNC_NAME))                           \
  {                                                    \
    at *p2;                                            \
    index_t *ind1, *ind2;                              \
    if (arg_number==1) {                               \
      ind1 = AINDEX(1);                                \
      p2 = CLONE_ARRAY(ind1);                          \
      ind2 = Mptr(p2);                                 \
    }  else {                                          \
      ARG_NUMBER(2);                                   \
      ind1 = AINDEX(1);                                \
      p2 = APOINTER(2);                                \
      ind2 = AINDEX(2);                                \
    }                                                  \
    if(IND_NDIMS(ind1) != IND_NDIMS(ind2)) ERRBADARGS; \
    CHECK_FUNC(ind1, ind2);                            \
    switch_type1(name2(Midx_,FUNC_NAME));              \
    return p2;                                         \
  }
  
/* ================================================================ */
/* Macros that define function with two input indices and one output index */

#define Xidx_aiai0o(FUNC_NAME, CHECK_FUNC)                \
  DX(name2(Xidx_,FUNC_NAME))                              \
  {                                                       \
    at *p3;                                               \
    index_t *ind1, *ind2, *ind3;                          \
    if (arg_number==2) {                                  \
      ind1 = AINDEX(1);                                   \
      ind2 = AINDEX(2);                                   \
      ind3 = make_array(IND_STTYPE(ind1), SHAPE0D, NIL);  \
      p3 = ind3->backptr;                                 \
    } else {                                              \
      ARG_NUMBER(3);                                      \
      ind1 = AINDEX(1);                                   \
      ind2 = AINDEX(2);                                   \
      p3 = APOINTER(3);                                   \
      ind3 = AINDEX(3);                                   \
      if(IND_NDIMS(ind3) != 0) ERRBADARGS;                \
    }                                                     \
    if (IND_NDIMS(ind1) != IND_NDIMS(ind2)) ERRBADARGS;   \
    CHECK_FUNC(ind1, ind2, ind3);                         \
    switch_type1(name2(Midx_,FUNC_NAME));                 \
    return p3;                                            \
  }

#define Xidx_ai0iao(FUNC_NAME, CHECK_FUNC)             \
  DX(name2(Xidx_,FUNC_NAME))                           \
  {                                                    \
    at *p3;                                            \
    index_t *ind1, *ind2, *ind3;                       \
    if (arg_number==2) {                               \
      ind1 = AINDEX(1);                                \
      ind2 = AINDEX(2);                                \
      p3 = CLONE_ARRAY(ind1);                          \
      ind3 = Mptr(p3);                                 \
    }  else {                                          \
      ARG_NUMBER(3);                                   \
      ind1 = AINDEX(1);                                \
      ind2 = AINDEX(2);                                \
      p3 = APOINTER(3);                                \
      ind3 = AINDEX(3);                                \
    }                                                  \
    if(IND_NDIMS(ind1) != IND_NDIMS(ind3)) ERRBADARGS; \
    if(IND_NDIMS(ind2) != 0) ERRBADARGS;               \
    CHECK_FUNC(ind1, ind2, ind3);                      \
    switch_type1(name2(Midx_,FUNC_NAME));              \
    return p3;                                         \
  }

#define Xidx_aiaiao(FUNC_NAME, CHECK_FUNC)      \
  DX(name2(Xidx_,FUNC_NAME))                    \
  {                                             \
    at *p3;                                     \
    index_t *ind1, *ind2, *ind3;                \
    if (arg_number==2) {                        \
      ind1 = AINDEX(1);                         \
      ind2 = AINDEX(2);                         \
      p3 = CLONE_ARRAY(ind1);                   \
      ind3 = Mptr(p3);                          \
    }  else {                                   \
      ARG_NUMBER(3);                            \
      ind1 = AINDEX(1);                         \
      ind2 = AINDEX(2);                         \
      p3 = APOINTER(3);                         \
      ind3 = AINDEX(3);                         \
    }                                           \
    if((IND_NDIMS(ind1) != IND_NDIMS(ind3)) ||  \
       (IND_NDIMS(ind1) != IND_NDIMS(ind2)))    \
      ERRBADARGS;                               \
    CHECK_FUNC(ind1, ind2, ind3);               \
    switch_type1(name2(Midx_,FUNC_NAME));       \
    return p3;                                  \
  }

#define Xidx_2i1i1o(FUNC_NAME, CHECK_FUNC)              \
  DX(name2(Xidx_,FUNC_NAME))                            \
  {                                                     \
    at *p3;                                             \
    index_t *ind1, *ind2, *ind3;                        \
    if (arg_number==2) {                                \
      ind1 = AINDEX(1);                                 \
      ind2 = AINDEX(2);                                 \
      ind3 = make_array(IND_STTYPE(ind1),               \
                        SHAPE1D(IND_DIM(ind1,0)), NIL); \
      p3 = ind3->backptr;                               \
    }  else {                                           \
      ARG_NUMBER(3);                                    \
      ind1 = AINDEX(1);                                 \
      ind2 = AINDEX(2);                                 \
      p3 = APOINTER(3);                                 \
      ind3 = AINDEX(3);                                 \
    }                                                   \
    if(IND_NDIMS(ind1) != 2 || IND_NDIMS(ind2) != 1 ||  \
       IND_NDIMS(ind3) != 1) ERRBADARGS;                \
    CHECK_FUNC(ind1, ind2, ind3);                       \
    switch_type1(name2(Midx_,FUNC_NAME));               \
    return p3;                                          \
  }

#define Xidx_4i2i2o(FUNC_NAME, CHECK_FUNC)             \
  DX(name2(Xidx_,FUNC_NAME))                           \
  {                                                    \
    at *p3;                                            \
    index_t *ind1, *ind2, *ind3;                       \
    if (arg_number==2) {                               \
      ind1 = AINDEX(1);                                \
       ind2 = AINDEX(2);                               \
       int ndim1 = IND_NDIMS(ind1);                    \
       IND_NDIMS(ind1) = 2;                            \
       ind3 = make_array(IND_STTYPE(ind1),             \
                         IND_SHAPE(ind1), NIL);        \
       IND_NDIMS(ind1) = ndim1;                        \
       p3 = ind3->backptr;                             \
    }  else {                                          \
      ARG_NUMBER(3);                                   \
       ind1 = AINDEX(1);                               \
       ind2 = AINDEX(2);                               \
       p3 = APOINTER(3);                               \
       ind3 = AINDEX(3);                               \
    }                                                  \
    if(IND_NDIMS(ind1) != 4 || IND_NDIMS(ind2) != 2 || \
       IND_NDIMS(ind3) != 2) ERRBADARGS;               \
    CHECK_FUNC(ind1, ind2, ind3);                      \
    switch_type1(name2(Midx_,FUNC_NAME));              \
    return p3;                                         \
  }
  
#define Xidx_1i1i2o(FUNC_NAME, CHECK_FUNC)                      \
  DX(name2(Xidx_,FUNC_NAME))                                    \
  {                                                             \
    at *p3;                                                     \
    index_t *ind1, *ind2, *ind3;                                \
    if (arg_number==2) {                                        \
      ind1 = AINDEX(1);                                         \
      ind2 = AINDEX(2);                                         \
      if(IND_NDIMS(ind1) != 1 ||                                \
         IND_NDIMS(ind2) != 1) ERRBADARGS;                      \
      ind3 = make_array(IND_STTYPE(ind1),                       \
                        SHAPE2D(IND_DIM(ind1, 0),               \
                                IND_DIM(ind2, 0)), NIL);        \
      p3 = ind3->backptr;                                       \
    }  else {                                                   \
      ARG_NUMBER(3);                                            \
      ind1 = AINDEX(1);                                         \
      ind2 = AINDEX(2);                                         \
      p3 = APOINTER(3);                                         \
      ind3 = AINDEX(3);                                         \
      if(IND_NDIMS(ind1) != 1 ||                                \
         IND_NDIMS(ind2) != 1 ||                                \
         IND_NDIMS(ind3) != 2) ERRBADARGS;                      \
    }                                                           \
    CHECK_FUNC(ind1, ind2, ind3);                               \
    switch_type1(name2(Midx_,FUNC_NAME));                       \
    return p3;                                                  \
  }

#define Xidx_2i2i4o(FUNC_NAME, CHECK_FUNC)                \
  DX(name2(Xidx_,FUNC_NAME))                              \
  {                                                       \
    at *p3;                                               \
    index_t *ind1, *ind2, *ind3;                          \
    if (arg_number==2) {                                  \
      ind1 = AINDEX(1);                                   \
      ind2 = AINDEX(2);                                   \
      if(IND_NDIMS(ind1) != 2 || IND_NDIMS(ind2) != 2)    \
        ERRBADARGS;                                       \
      ind3 = make_array(IND_STTYPE(ind1),                 \
                        SHAPE4D(IND_DIM(ind1, 0),         \
                                IND_DIM(ind1, 1),         \
                                IND_DIM(ind2, 0),         \
                                IND_DIM(ind2, 1)), NIL);  \
      p3 = ind3->backptr;                                 \
    }  else {                                             \
      ARG_NUMBER(3);                                      \
      ind1 = AINDEX(1);                                   \
      ind2 = AINDEX(2);                                   \
      p3 = APOINTER(3);                                   \
      ind3 = AINDEX(3);                                   \
      if(IND_NDIMS(ind1) != 2 || IND_NDIMS(ind2) != 2 ||  \
         IND_NDIMS(ind3) != 4) ERRBADARGS;                \
    }                                                     \
    CHECK_FUNC(ind1, ind2, ind3);                         \
    switch_type1(name2(Midx_,FUNC_NAME));                 \
    return p3;                                            \
  }



/* -------------------------------------------------------------
   Local Variables:
   c-font-lock-extra-types: (
     "FILE" "\\sw+_t" "at" "gptr" "real" "flt" "intg" )
   End:
   ------------------------------------------------------------- */
