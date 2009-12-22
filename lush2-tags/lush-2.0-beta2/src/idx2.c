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
  dx_define("idx-minus",Xidx_maminus);
  dx_define("idx-abs",Xidx_maabs);
  dx_define("idx-sqrt",Xidx_masqrt);
  dx_define("idx-inv",Xidx_mainv);
  dx_define("idx-qtanh",Xidx_maqtanh);
  dx_define("idx-qdtanh",Xidx_maqdtanh);
  dx_define("idx-stdsigmoid",Xidx_mastdsigmoid);
  dx_define("idx-dstdsigmoid",Xidx_madstdsigmoid);
  dx_define("idx-expmx",Xidx_maexpmx);
  dx_define("idx-dexpmx",Xidx_madexpmx);
  dx_define("idx-sin",Xidx_masin);
  dx_define("idx-cos",Xidx_macos);
  dx_define("idx-atan",Xidx_maatan);
  dx_define("idx-log",Xidx_malog);
  dx_define("idx-exp",Xidx_maexp);
}
