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
      FUNC_NAME(&i1, t1); \
      break; 
#define case_type2_2arg(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      check_number_of_type \
      FUNC_NAME(&i1, &i2, t1, t2); \
      break; 
#define case_type2_3arg(storage_type, t1, t2, FUNC_NAME) \
    case storage_type: \
      check_number_of_type \
      FUNC_NAME(&i1, &i2, &i3, t1, t1, t2); \
      break; \

#define case_type2(storage_type, t1, t2, FUNC_NAME) \
    case_type2_2arg(storage_type, t1, t2, FUNC_NAME) 

#ifdef SHORT_IDX
#define switch_type2(t1, FUNC_NAME) \
    switch((&i2)->srg->type) { \
    case_type2(ST_I32, t1, int, FUNC_NAME) \
    case_type2(ST_F, t1, flt, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#else
#define switch_type2(t1, FUNC_NAME) \
    switch((&i2)->srg->type) { \
    case_type2(ST_U8, t1, unsigned char, FUNC_NAME) \
    case_type2(ST_I8, t1, char, FUNC_NAME) \
    case_type2(ST_I16, t1, short, FUNC_NAME) \
    case_type2(ST_I32, t1, int, FUNC_NAME) \
    case_type2(ST_F, t1, flt, FUNC_NAME) \
    case_type2(ST_D, t1, real, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#endif

#ifdef SHORT_IDX
#define switch_type3(t1, FUNC_NAME) \
    switch((&i3)->srg->type) { \
    case_type2(ST_I32, t1, int, FUNC_NAME) \
    case_type2(ST_F, t1, flt, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#else
#define switch_type3(t1, FUNC_NAME) \
    switch((&i3)->srg->type) { \
    case_type2(ST_U8, t1, unsigned char, FUNC_NAME) \
    case_type2(ST_I8, t1, char, FUNC_NAME) \
    case_type2(ST_I16, t1, short, FUNC_NAME) \
    case_type2(ST_I32, t1, int, FUNC_NAME) \
    case_type2(ST_F, t1, flt, FUNC_NAME) \
    case_type2(ST_D, t1, real, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for second argument", NIL); \
    } 
#endif

#define check_unitype2 \
    if((&i1)->srg->type != (&i2)->srg->type) \
       error(NIL, "All idxs must be of the same type", NIL);
#define check_unitype3 \
    if(((&i1)->srg->type != (&i3)->srg->type) || \
       ((&i1)->srg->type != (&i2)->srg->type)) \
       error(NIL, "All idxs must be of the same type", NIL);
#define check_multitype2
#define check_multitype3 \
    if((&i1)->srg->type != (&i2)->srg->type) \
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
    switch((&i1)->srg->type) { \
    case_type1(ST_I32, int, FUNC_NAME) \
    case_type1(ST_F, flt, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for first argument", NIL); \
    } 
#else
#define switch_type1(FUNC_NAME) \
    switch((&i1)->srg->type) { \
    case_type1(ST_U8, unsigned char, FUNC_NAME) \
    case_type1(ST_I8, char, FUNC_NAME) \
    case_type1(ST_I16, short, FUNC_NAME) \
    case_type1(ST_I32, int, FUNC_NAME) \
    case_type1(ST_F, flt, FUNC_NAME) \
    case_type1(ST_D, real, FUNC_NAME) \
    default: \
      error(NIL, "Unknown type for first argument", NIL); \
    } 
#endif

/* ================================================================ */
/* Macros that define function with one output index */

#define Xidx_o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    struct idx i1;		\
    struct index *ind1;		\
    ALL_ARGS_EVAL;		\
    ARG_NUMBER(1);		\
    ind1 = AINDEX(1); \
    index_write_idx(ind1, &i1);		\
		    \
    CHECK_FUNC(&i1);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
		    \
    index_rls_idx(ind1, &i1);		\
    LOCK(APOINTER(1)); \
    return APOINTER(1);	\
    }
  
/* ================================================================ */
/* Macros that define function with one input index and one output index */

#define Xidx_io0(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p2, *atst; \
    struct idx i1, i2;		\
    struct index *ind1, *ind2;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==1) { \
       ind1 = AINDEX(1); \
       atst = new_storage_nc(ind1->st->srg.type, 1); \
       p2 = new_index(atst); \
       index_dimension(p2, 0, NULL); \
       ind2 = p2->Object; \
    } else { \
       ARG_NUMBER(2); \
       ind1 = AINDEX(1); \
       p2 = APOINTER(2); \
       ind2 = AINDEX(2); \
       LOCK(p2);  \
    } \
    \
    if(ind2->ndim != 0) ERRBADARGS;		\
    index_read_idx(ind1, &i1);		\
    index_write_idx(ind2, &i2);		\
		    \
    CHECK_FUNC(&i1, &i2);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
		    \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
 \
     return p2;		\
  }

#define Xidx_ioa(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p2; \
    struct idx i1, i2;		\
    struct index *ind1, *ind2;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==1) { \
       ind1 = AINDEX(1); \
       p2 = create_samesize_matrix(APOINTER(1)); \
       ind2 = p2->Object; \
    }  else { \
       ARG_NUMBER(2); \
       ind1 = AINDEX(1); \
       p2 = APOINTER(2); \
       ind2 = AINDEX(2); \
       LOCK(p2); \
    } \
    if(ind1->ndim != ind2->ndim) ERRBADARGS;		\
    \
    index_read_idx(ind1, &i1);		\
    index_write_idx(ind2, &i2);		\
  		\
    CHECK_FUNC(&i1, &i2);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
  		\
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
  		\
    return p2;		\
  }
  
/* ================================================================ */
/* Macros that define function with two input indices and one output index */

#define Xidx_aiai0o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3, *atst; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       atst = new_storage_nc(ind1->st->srg.type, 1); \
       p3 = new_index(atst); \
       index_dimension(p3, 0, NULL); \
       ind3 = p3->Object; \
    } else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       if(ind3->ndim != 0) ERRBADARGS;		\
       LOCK(p3);  \
    } \
    if(ind1->ndim != ind2->ndim) ERRBADARGS;		\
    		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
  		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    /* name2(Midx_,FUNC_NAME)(&i1, &i2, &i3, BASETYPE); */ \
    switch_type1(name2(Midx_,FUNC_NAME)); \
  		\
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
	\
    return p3; \
  }

#define Xidx_ai0iao(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = create_samesize_matrix(APOINTER(1)); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       LOCK(p3); \
    } \
    if(ind1->ndim != ind3->ndim) ERRBADARGS;		\
    if(ind2->ndim != 0) ERRBADARGS;		\
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
  }

#define Xidx_aiaiao(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = create_samesize_matrix(APOINTER(1)); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       LOCK(p3); \
    } \
    if((ind1->ndim != ind3->ndim) || (ind1->ndim != ind2->ndim)) ERRBADARGS;\
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
  }

#define Xidx_2i1i1o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = create_samesize_matrix(APOINTER(2)); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       LOCK(p3); \
    } \
    if(ind1->ndim != 2 || ind2->ndim != 1 || ind3->ndim != 1) ERRBADARGS;\
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
	    }

#define Xidx_4i2i2o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = create_samesize_matrix(APOINTER(2)); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       LOCK(p3); \
    } \
    if(ind1->ndim != 4 || ind2->ndim != 2 || ind3->ndim != 2) ERRBADARGS;\
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
	    }
  
#define Xidx_1i1i2o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3, *atst; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    int dim[2];\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       if(ind1->ndim != 1 || ind2->ndim != 1) ERRBADARGS;\
       atst = new_storage_nc(ind1->st->srg.type, ind1->dim[0]*ind2->dim[0]); \
       p3 = new_index(atst); \
       dim[0] = ind1->dim[0]; \
       dim[1] = ind2->dim[0]; \
       index_dimension(p3, 2, dim); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       if(ind1->ndim != 1 || ind2->ndim != 1 || ind3->ndim != 2) ERRBADARGS;\
       LOCK(p3); \
    } \
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
	    }

#define Xidx_2i2i4o(FUNC_NAME, CHECK_FUNC) \
  DX(name2(Xidx_,FUNC_NAME))		\
  {		\
    at *p3, *atst; \
    struct idx i1, i2, i3;		\
    struct index *ind1, *ind2, *ind3;		\
    int dim[4];\
    ALL_ARGS_EVAL;		\
    if (arg_number==2) { \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       if(ind1->ndim != 2 || ind2->ndim != 2) ERRBADARGS;\
       atst = new_storage_nc(ind1->st->srg.type,  \
		   ind1->dim[0]*ind1->dim[1] * ind2->dim[0]*ind2->dim[1]); \
       p3 = new_index(atst); \
       dim[0] = ind1->dim[0]; \
       dim[1] = ind1->dim[1]; \
       dim[2] = ind2->dim[0]; \
       dim[3] = ind2->dim[1]; \
       index_dimension(p3, 4, dim); \
       ind3 = p3->Object; \
    }  else { \
       ARG_NUMBER(3); \
       ind1 = AINDEX(1); \
       ind2 = AINDEX(2); \
       p3 = APOINTER(3); \
       ind3 = AINDEX(3); \
       if(ind1->ndim != 2 || ind2->ndim != 2 || ind3->ndim != 4) ERRBADARGS;\
       LOCK(p3); \
    } \
  		\
    index_read_idx(ind1, &i1);		\
    index_read_idx(ind2, &i2);		\
    index_write_idx(ind3, &i3);		\
    CHECK_FUNC(&i1, &i2, &i3);		\
    switch_type1(name2(Midx_,FUNC_NAME)); \
    index_rls_idx(ind1, &i1);		\
    index_rls_idx(ind2, &i2);		\
    index_rls_idx(ind3, &i3);		\
  		\
    return p3; \
	    }

