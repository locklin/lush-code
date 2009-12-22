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

bool integerp(double d)
{
   return d == nearbyint(d);
}

DX(xintegerp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(integerp(AREAL(1)));
}


bool oddp(double d)
{
   ifn (integerp(d))
      RAISEF("not an integer", NEW_NUMBER(d));
   return d/2.0 != nearbyint(d/2.0);
}

DX(xoddp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(oddp(AREAL(1)));
}


bool evenp(double d)
{
   ifn (integerp(d))
      RAISEF("not an integer", NEW_NUMBER(d));
   return d/2.0 == nearbyint(d/2.0);
}

DX(xevenp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(evenp(AREAL(1)));
}


void init_number(void)
{
   dx_define("integerp", xintegerp);
   dx_define("oddp", xoddp);
   dx_define("evenp", xevenp);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
