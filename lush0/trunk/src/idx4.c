/***********************************************************************
  SN3: Heir of the SN family
  (LYB) (YLC) 91
  ******************************************************************/

#include "header.h"
#include "idxmac.h"
#include "idxops.h"
#include "check_func.h"
#include "idx.h"

static char badargs[]="bad arguments";
#define ERRBADARGS error(NIL,badargs,NIL)
extern at *create_samesize_matrix(at *);

/* All the code below is interpreted, so all the macros such as
   Mis_size, check_main_maout,... should not call run_time_error,
   but error instead.  To avoid that problem once and for all
   the following macro redefine run_time_error:
   */
#define run_time_error(s) error(NIL, s, NIL);

/******************** FUNCTION DEFINITIONS (1 arguments) ******************* */

/* Multitypes */

#undef case_type2
#define case_type2(storage_type, t1, t2, FUNC_NAME) \
    case_type2_3arg(storage_type, t1, t2, FUNC_NAME) 

/* Multitypes */
#undef check_number_of_type
#define check_number_of_type check_multitype3
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    unfold_switch(storage_type,t1, FUNC_NAME)

/* From now on, unfold_switch switch on the type fo the last
   argument, not the second */
#undef unfold_switch
#define unfold_switch(storage_type,t1, FUNC_NAME) \
    case storage_type: \
      switch_type3(t1, FUNC_NAME); \
      break; 

/* ============== convolutions mono and bi-dimensional ====== */

Xidx_2i1i1o(m2dotm1, Mcheck_main_main_maout_dot21)
Xidx_4i2i2o(m4dotm2, Mcheck_main_main_maout_dot42)
Xidx_2i1i1o(m2dotm1acc, Mcheck_main_main_maout_dot21)
Xidx_4i2i2o(m4dotm2acc, Mcheck_main_main_maout_dot42)

/* ============== term by term operations ====== */

Xidx_aiaiao(maadd, check_main_main_maout)
Xidx_aiaiao(masub, check_main_main_maout)
Xidx_aiaiao(mamul, check_main_main_maout)
Xidx_aiaiao(madiv, check_main_main_maout)

/* Unitype */
#undef check_number_of_type
#define check_number_of_type check_unitype3
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    case_type2(storage_type, t1, t1, FUNC_NAME)

/* ========= outer products for back convolutions etc ================ */

Xidx_1i1i2o(m1extm1, Mcheck_m1in_m1in_m2out)
Xidx_2i2i4o(m2extm2, Mcheck_m2in_m2in_m4out)
Xidx_1i1i2o(m1extm1acc, Mcheck_m1in_m1in_m2out)
Xidx_2i2i4o(m2extm2acc, Mcheck_m2in_m2in_m4out)

/* ====================== permutation ======================== */

/* Following functions are a special case of macro call for permutation */
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    case storage_type: \
      if((&i2)->srg->type != ST_I32) \
        error(NIL, "Permutation index must be of integer (I32) type", NIL); \
      if((&i1)->srg->type != (&i3)->srg->type) \
        error(NIL, "Input and output idxs must be of the same type", NIL); \
      FUNC_NAME(&i1, &i2, &i3, t1); \
      break; 

/*  Now replaced by g-loop */
/*
Xidx_aiaiao(m1permute, Mcheck_m1in_m1in_m1out2)
Xidx_aiaiao(m2permute, Mcheck_m1in_m2in_m1out2)
*/

void init_idx4(void)
{
#ifdef Midx_m2dotm1
  dx_define("idx_m2dotm1", Xidx_m2dotm1);
#endif
#ifdef Midx_m4dotm2
  dx_define("idx_m4dotm2", Xidx_m4dotm2);
#endif
#ifdef Midx_m2dotm1acc
  dx_define("idx_m2dotm1acc", Xidx_m2dotm1acc);
#endif
#ifdef Midx_m4dotm2acc
  dx_define("idx_m4dotm2acc", Xidx_m4dotm2acc);
#endif


#ifdef Midx_maadd
  dx_define("idx_add",Xidx_maadd);
#endif
#ifdef Midx_masub
  dx_define("idx_sub",Xidx_masub);
#endif
#ifdef Midx_mamul
  dx_define("idx_mul",Xidx_mamul);
#endif
#ifdef Midx_madiv
  dx_define("idx_div",Xidx_madiv);
#endif

#ifdef Midx_m1extm1
  dx_define("idx_m1extm1",Xidx_m1extm1);
#endif
#ifdef Midx_m1extm1acc
  dx_define("idx_m1extm1acc",Xidx_m1extm1acc);
#endif
#ifdef Midx_m2extm2
  dx_define("idx_m2extm2",Xidx_m2extm2);
#endif
#ifdef Midx_m2extm2acc
  dx_define("idx_m2extm2acc",Xidx_m2extm2acc);
#endif

/*
  dx_define("idx_m1permute",Xidx_m1permute);
  dx_define("idx_m2permute",Xidx_m2permute);
*/

}
