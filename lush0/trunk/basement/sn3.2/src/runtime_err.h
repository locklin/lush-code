/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

static char rterr_bound[]="indices are out of bound";
static char rterr_rtype[]="invalid return type: for loop must be executed once";
static char rterr_dim[]="dimension is out of bounds";
static char rterr_loopdim[]="looping dimensions ar different";
static char rterr_emptystr[]="empty string";
static char rterr_range[]="range error";
static char rterr_srg_of[]="change in idx could cause srg overflow";

#define RTERR_BOUND(test) if (test) { run_time_error(rterr_bound); }
#define RTERR_RTYPE(test) if (test) { run_time_error(rterr_rtype); }
#define RTERR_DIM(test) if (test) { run_time_error(rterr_dim); }
#define RTERR_LOOPDIM(test) if (test) { run_time_error(rterr_loopdim); }
#define RTERR_EMPTYSTR(test) if (test) { run_time_error(rterr_emptystr); }
#define RTERR_RANGE(test) if (test) { run_time_error(rterr_range); }
#define RTERR_SRG_OVERFLOW run_time_error(rterr_srg_of); 

/* gen purpose version */
#define RTERR_GEN(test,errstr) if (test) { run_time_error(errstr); }
