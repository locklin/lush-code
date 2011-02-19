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

/* some additional functions needed by the lush compiler */

#include "header.h"
#include "idxmac.h"
#include "idxops.h"
#include "check_func.h"
#include "dh.h"


/******************************************************************************
 *
 *  Error messages
 *
 ******************************************************************************/

char *rterr_bound = "indices are out of bound";
char *rterr_rtype = "invalid return type: for loop must be executed once";
char *rterr_dim = "dimension is out of bounds";
char *rterr_loopdim = "looping dimensions are different";
char *rterr_emptystr = "empty string";
char *rterr_emptyidx = "array is empty";
char *rterr_range = "range error";
char *rterr_srg_overflow = "change in idx could cause srg overflow";
char *rterr_not_same_dim = "matrix must have same dimensions";
char *rterr_unmanaged = "not a managed address";
char *rterr_out_of_memory = "out of memory (reallocating storage)";
char *rterr_cannot_realloc = "cannot reallocate storage";
char *rterr_cannot_shrink = "cannot decrease storage size";
char *rterr_bad_dimensions = "arguments have the wrong dimensions";


/******************************************************************************
 *
 *  Trace functions
 *
 ******************************************************************************/


struct dh_trace_stack *dh_trace_root = 0;

/* print_dh_recent: print n recent functions */

void print_dh_recent(int n, FILE *f)
{
    struct dh_trace_stack *st = dh_trace_root;

    while (st && n>0) {
      fprintf(f,"%s",st->info);
      st = st->next;
      if (st && n>0)
        fprintf(f,"/");
      n--;
    }
    fprintf(f,"\n");
}


/* print_dh_trace_stack: prints where we are in compiled code. */

void print_dh_trace_stack(void)
{
    struct dh_trace_stack *st = dh_trace_root;
    
    /* Safely called from compiled code or from lush_error. */
    while (st) 
    {
        int lastcount = 0;
        const char *lastinfo = st->info;
        while (st && st->info==lastinfo) {
            lastcount += 1;
            st = st->next;
        }
        if (lastcount > 1)
            printf("** in: %s (%d recursive calls)\n", lastinfo, lastcount);
        else
            printf("** in: %s\n", lastinfo);
    }
}



/******************************************************************************
 *
 *  Matrix check functions
 *
 *****************************************************************************/


void check_main_maout(index_t *i1, index_t *i2)
{
   Mcheck_main_maout(i1, i2);
}

void check_main_maout_any(index_t *i1, index_t *i2)
{
   if (idx_emptyp(i2))
   {
      if (i1->ndim != i2->ndim)
         lush_error(rterr_not_same_dim);
      Mcheck_main_maout(i1, i2);
   }
   else
   {
      size_t n1=1, n2=1;
      for (int i=0; i<i1->ndim; i++)
         n1 *= i1->dim[i];
      for (int i=0; i<i2->ndim; i++)
        n2 *= i2->dim[i];
      if (n1 != n2)
         lush_error(rterr_not_same_dim);
   }
}

void check_main_main_maout(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_main_main_maout(i0, i1, i2);
}

void check_main_m0out(index_t *i1, index_t *i2)
{
   Mcheck_main_m0out(i1, i2);
}

void check_main_main_m0out(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_main_main_m0out(i0, i1, i2);
}

void check_main_m0in_maout(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_main_m0in_maout(i0, i1, i2);
}

void check_main_main_maout_dot21(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_main_main_maout_dot21(i0, i1, i2);
}

void check_main_main_maout_dot42(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_main_main_maout_dot42(i0, i1, i2);
}

void check_m1in_m1in_m2out(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_m1in_m1in_m2out(i0, i1, i2);
}

void check_m2in_m2in_m4out(index_t *i0, index_t *i1, index_t *i2)
{
   Mcheck_m2in_m2in_m4out(i0, i1, i2);
}

bool idx_emptyp(index_t *i0)
{
   for (int i=0; i<i0->ndim; i++)
      if (i0->dim[i] == 0)
         return true;
   return false;
}



/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
