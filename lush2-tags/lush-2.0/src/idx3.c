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

Xidx_aiai0o(madotma, check_main_main_m0out)
Xidx_aiai0o(madotmaacc, check_main_main_m0out)
Xidx_ai0iao(madotm0, check_main_m0in_maout)
Xidx_ai0iao(madotm0acc, check_main_m0in_maout)
Xidx_ai0iao(maaddm0, check_main_m0in_maout)
Xidx_ai0iao(maaddm0acc, check_main_m0in_maout)
Xidx_aiai0o(masqrdist, check_main_main_m0out)
Xidx_aiai0o(masqrdistacc, check_main_main_m0out)

void init_idx3(void)
{
#ifdef Midx_madotma
  dx_define("idx-dot",Xidx_madotma);
#endif
#ifdef Midx_madotmaacc
  dx_define("idx-dotacc",Xidx_madotmaacc);
#endif

#ifdef Midx_madotm0
  dx_define("idx-dotm0",Xidx_madotm0);
#endif
#ifdef Midx_madotm0acc
  dx_define("idx-dotm0acc",Xidx_madotm0acc);
#endif

#ifdef Midx_maaddm0
  dx_define("idx-addm0",Xidx_maaddm0);
#endif
#ifdef Midx_maaddm0acc
  dx_define("idx-addm0acc",Xidx_maaddm0acc);
#endif

#ifdef Midx_masqrdist
  dx_define("idx-sqrdist",Xidx_masqrdist);
#endif
#ifdef Midx_masqrdistacc
  dx_define("idx-sqrdistacc",Xidx_masqrdistacc);
#endif
}
