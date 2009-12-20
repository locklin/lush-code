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
#undef uchar
#include <cxtypes.h>


int cvmat_typecode(index_t *ind, int nchannels) {
  
  ifn (nchannels>0 && nchannels<5)
    RAISEF("invalid number of channels", NEW_NUMBER(nchannels));
  
  storage_type_t stt = IND_STTYPE(ind);
  int typecode = -1;

  switch (stt) {
  case ST_FLOAT: 
    typecode = CV_MAKE_TYPE(CV_32F, nchannels);
    break;

  case ST_DOUBLE: 
    typecode = CV_MAKE_TYPE(CV_64F, nchannels);
    break;

  case ST_INT: 
    typecode = CV_MAKE_TYPE(CV_32S, nchannels);
    break;

  case ST_SHORT: 
    typecode = CV_MAKE_TYPE(CV_16S, nchannels);
    break;

  case ST_CHAR:
    typecode = CV_MAKE_TYPE(CV_8S, nchannels);
    break;

  case ST_UCHAR:
    typecode = CV_MAKE_TYPE(CV_8U, nchannels);
    break;
  
  default:
    RAISEF("invalid storage type", NIL);
  }
  return typecode;
}


DX(xcvmat_typecode) {

  ARG_NUMBER(2);

  return NEW_NUMBER(cvmat_typecode(AINDEX(1), AINTEGER(2)));
}

void init_aux_cv_arrays()
{
  dx_define("cvmat-typecode", xcvmat_typecode);
}
