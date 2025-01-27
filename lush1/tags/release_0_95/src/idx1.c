/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

/***********************************************************************
 * $Id: idx1.c,v 1.1 2002-04-18 20:17:13 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "idxmac.h"
#include "idxops.h"
#include "check_func.h"
#include "idx.h"

static char badargs[]="bad arguments";
#define ERRBADARGS error(NIL,badargs,NIL)
extern at *create_samesize_matrix(at*);

/* All the code below is interpreted, so all the macros such as
   Mis_size, check_main_maout,... should not call run_time_error,
   but error instead.  To avoid that problem once and for all
   the following macro redefine run_time_error:
   */
#ifdef run_time_error
#undef run_time_error
#endif
#define run_time_error(s) error(NIL, s, NIL);

/******************** FUNCTION DEFINITIONS (1 arguments) ******************* */
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    case_type2_1arg(storage_type, t1, t1, FUNC_NAME) 

Xidx_o(maclear, Mis_sized)

/******************** FUNCTION DEFINITIONS (2 arguments) ******************* */
#undef case_type2
#define case_type2(storage_type, t1, t2, FUNC_NAME) \
    case_type2_2arg(storage_type, t1, t2, FUNC_NAME) 

/* Multitypes */
#undef check_number_of_type
#define check_number_of_type check_multitype2
#undef case_type1
#define case_type1(storage_type, t1, FUNC_NAME) \
    unfold_switch(storage_type,t1, FUNC_NAME);

Xidx_ioa(macopy, check_main_maout)
Xidx_io0(masum, check_main_m0out)
Xidx_io0(masup, check_main_m0out)
Xidx_io0(mainf, check_main_m0out)
Xidx_io0(masumacc, check_main_m0out)
Xidx_io0(masupacc, check_main_m0out)
Xidx_io0(mainfacc, check_main_m0out)
Xidx_io0(masumsqr, check_main_m0out)
Xidx_io0(masumsqracc, check_main_m0out)

void init_idx1(void)
{
#ifdef Midx_maclear
  dx_define("idx_clear",Xidx_maclear);
#endif

#ifdef Midx_macopy
  dx_define("idx_copy",Xidx_macopy);
#endif

#ifdef Midx_masum
  dx_define("idx_sum",Xidx_masum);
#endif
#ifdef Midx_masup
  dx_define("idx_sup",Xidx_masup);
#endif
#ifdef Midx_mainf
  dx_define("idx_inf",Xidx_mainf);
#endif
#ifdef Midx_masumacc
  dx_define("idx_sumacc",Xidx_masumacc);
#endif
#ifdef Midx_masupacc
  dx_define("idx_supacc",Xidx_masupacc);
#endif
#ifdef Midx_mainfacc
  dx_define("idx_infacc",Xidx_mainfacc);
#endif

#ifdef Midx_masumsqr
  dx_define("idx_sumsqr",Xidx_masumsqr);
#endif
#ifdef Midx_masumsqracc
  dx_define("idx_sumsqracc",Xidx_masumsqracc);
#endif
}
