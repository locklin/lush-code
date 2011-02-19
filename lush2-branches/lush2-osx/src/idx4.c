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

#include "header.h"
#include "idxmac.h"
#include "idxops.h"
#include "check_func.h"
#include "idx.h"

static char badargs[]="bad arguments";
#define ERRBADARGS error(NIL,badargs,NIL)

/* All the code below is interpreted, so all the macros such as
   Mis_size, check_main_maout,... should not call lush_error,
   but error instead.  To avoid that problem once and for all
   the following macro redefine lush_error:
   */
#ifdef lush_error
#undef lush_error
#endif
#define lush_error(s) error(NIL, s, NIL);

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
      if((&i2)->srg->type != ST_INT) \
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
  dx_define("idx-m2dotm1", Xidx_m2dotm1);
#endif
#ifdef Midx_m4dotm2
  dx_define("idx-m4dotm2", Xidx_m4dotm2);
#endif
#ifdef Midx_m2dotm1acc
  dx_define("idx-m2dotm1acc", Xidx_m2dotm1acc);
#endif
#ifdef Midx_m4dotm2acc
  dx_define("idx-m4dotm2acc", Xidx_m4dotm2acc);
#endif


#ifdef Midx_maadd
  dx_define("idx-add",Xidx_maadd);
#endif
#ifdef Midx_masub
  dx_define("idx-sub",Xidx_masub);
#endif
#ifdef Midx_mamul
  dx_define("idx-mul",Xidx_mamul);
#endif
#ifdef Midx_madiv
  dx_define("idx-div",Xidx_madiv);
#endif

#ifdef Midx_m1extm1
  dx_define("idx-m1extm1",Xidx_m1extm1);
#endif
#ifdef Midx_m1extm1acc
  dx_define("idx-m1extm1acc",Xidx_m1extm1acc);
#endif
#ifdef Midx_m2extm2
  dx_define("idx-m2extm2",Xidx_m2extm2);
#endif
#ifdef Midx_m2extm2acc
  dx_define("idx-m2extm2acc",Xidx_m2extm2acc);
#endif

/*
  dx_define("idx-m1permute",Xidx_m1permute);
  dx_define("idx-m2permute",Xidx_m2permute);
*/

}
