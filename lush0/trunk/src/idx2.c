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

/* =========== scalar functions on MA elements ================== */

#undef case_type2
#define case_type2(storage_type, t1, t2, FUNC_NAME) \
    case_type2_2arg(storage_type, t1, t2, FUNC_NAME) 
#undef check_number_of_type
#define check_number_of_type check_unitype2
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    case_type2(storage_type, t1, t1, FUNC_NAME)

Xidx_ioa(maminus, check_main_maout)
Xidx_ioa(maabs, check_main_maout)
Xidx_ioa(masqrt, check_main_maout)
Xidx_ioa(mainv, check_main_maout)
Xidx_ioa(maqtanh, check_main_maout)
Xidx_ioa(maqdtanh, check_main_maout)
Xidx_ioa(mastdsigmoid, check_main_maout)
Xidx_ioa(madstdsigmoid, check_main_maout)
Xidx_ioa(maexpmx, check_main_maout)
Xidx_ioa(madexpmx, check_main_maout)
Xidx_ioa(masin, check_main_maout)
Xidx_ioa(macos, check_main_maout)
Xidx_ioa(maatan, check_main_maout)
Xidx_ioa(malog, check_main_maout)
Xidx_ioa(maexp, check_main_maout)



void init_idx2(void)
{
  dx_define("idx_minus",Xidx_maminus);
  dx_define("idx_abs",Xidx_maabs);
  dx_define("idx_sqrt",Xidx_masqrt);
  dx_define("idx_inv",Xidx_mainv);
  dx_define("idx_qtanh",Xidx_maqtanh);
  dx_define("idx_qdtanh",Xidx_maqdtanh);
  dx_define("idx_stdsigmoid",Xidx_mastdsigmoid);
  dx_define("idx_dstdsigmoid",Xidx_madstdsigmoid);
  dx_define("idx_expmx",Xidx_maexpmx);
  dx_define("idx_dexpmx",Xidx_madexpmx);
  dx_define("idx_sin",Xidx_masin);
  dx_define("idx_cos",Xidx_macos);
  dx_define("idx_atan",Xidx_maatan);
  dx_define("idx_log",Xidx_malog);
  dx_define("idx_exp",Xidx_maexp);
}
