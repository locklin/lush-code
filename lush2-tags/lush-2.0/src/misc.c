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

/* Naming conventions of the C API:
 * --------------------------------
 * o Function names are lower case
 * o The dash ("-") in Lisp names translates to underscor ("_")
 * o Special characters in Lisp function names translate to single
 *   uppercase letters as follows:
 *   ! -> D  (Destructive)
 *   ? -> P  (Predicate)
 *   * -> S  (Star or Special)
 *   
 * Examples:
 * copy-index <-> copy_index
 * idx-trim!  <-> idx_trimD
 * let*       <-> letS
 */ 

#include "header.h"

static char table_c2lisp[256];
static char table_lisp2c[256];
static char* c_special    = "_DPSO";
static char* lisp_special = "-!?*/";

void api_init_tables(void)
{
   for (int c=0; c<256; c++) {
      table_c2lisp[c] = (char)c;
      table_lisp2c[c] = (char)c;
   }
   for (uchar c=0; c<strlen(c_special); c++) {
      table_c2lisp[(uchar)c_special[c]] = lisp_special[c];
      table_lisp2c[(uchar)lisp_special[c]] = c_special[c];
   }
}

char *api_translate_c2lisp(const char *cname)
{
   static char nameb[400];
   char *namebp = nameb;
   
   while (*cname)
      *(namebp++) = table_c2lisp[(uchar)*(cname++)];
   *(namebp) = '\0';
   return nameb;
}

char *api_translate_lisp2c(const char *lname)
{
   static char nameb[400];
   char *namebp = nameb;

   while (*lname)
      *(namebp++) = table_lisp2c[(uchar)*(lname++)];
   *(namebp) = '\0';
   return nameb;
}

/*
 * error routines: need_error... implanted in the macros  APOINTER & co
 */

static char need_arg_num[80];
static char *need_errlist[] = {
   "incorrect number of arguments",
   "not a real number",
   "not a list",
   "empty list",
   "not a string",
   "not a cell",
   "not a clist",
   "not a symbol",
   "not an object",
   "not a gptr",
   "not a storage",
   "not an index",
   "not a class"
};

gptr need_error(int i, int j, at **arg_array_ptr)
{
   char *s2 = need_errlist[i];
   at *p = NIL;
   
   if (i)
      p = arg_array_ptr[j];
   
   else if (j > 1) {
      sprintf(need_arg_num, "%d arguments were expected", j);
      s2 = need_arg_num;

   } else if (j == 1)
      s2 = "one argument was expected";
  
   else if (j == 0)
      s2 = "no arguments were expected";
  
   error(NIL, s2, p);
   return NIL;
}

/* this is called by BLAS and LAPACK */
int xerbla_(char *name, int *info)
{
   char tname[7];
   strncpy(tname, name, 6);
   tname[6] = '\0';
   char errmsg[200];
   sprintf(errmsg, "%dth argument to BLAS/LAPACK-function %6s invalid\n", *info, tname);
   lush_error(errmsg);
   return 0;
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
