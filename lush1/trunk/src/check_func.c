
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
 * $Id: check_func.c,v 1.6 2004-04-16 14:42:58 leonb Exp $
 **********************************************************************/

/* Functions that check the dimensions of index parameters */

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
char *rterr_range = "range error";
char *rterr_srg_of = "change in idx could cause srg overflow";
char *rterr_unsized_matrix = "matrix has not been sized";
char *rterr_not_same_dim = "matrix must have same dimensions";
char *rterr_out_of_memory = "out of memory (reallocating storage)";
char *rterr_cannot_realloc = "cannot reallocate storage";
char *rterr_bad_dimensions = "arguments have the wrong dimensions";



/******************************************************************************
 *
 *  Trace functions
 *
 ******************************************************************************/


struct dh_trace_stack *dh_trace_root = 0;

/* print_dh_recent: print n recent functions */

void 
print_dh_recent(int n, FILE *f)
{
    struct dh_trace_stack *st = dh_trace_root;
    st = dh_trace_root;
    while (st && n-->0) 
    {
        fprintf(f,"%s",st->info);
        st = st->next;
        if (st && n>0)
            fprintf(f,"/");
        else
            fprintf(f,"\n");
    }
}


/* print_dh_trace_stack: prints where we are in compiled code. */

void print_dh_trace_stack(void)
{
    struct dh_trace_stack *st = dh_trace_root;
    char *lastinfo;
    int lastcount;
    
    /* Safely called from compiled code or from RUN_TIME_ERROR. */
    st = dh_trace_root;
    while (st) 
    {
        lastcount = 0;
        lastinfo = st->info;
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
 *  Object class membership test
 *
 *****************************************************************************/

int
test_obj_class(void *obj, void *classvtable)
{
  if (obj)
    {
      struct VClass_object *vtable = *(struct VClass_object**)obj;
      while (vtable && vtable != classvtable)
        {
	  /* This is tricky because Cdoc contains
	     different things in the NOLISP case. */
#ifndef NOLISP
	  dhclassdoc_t *cdoc = (dhclassdoc_t*)(vtable->Cdoc);
	  if (! cdoc)
	    run_time_error("Found null Cdoc in virtual table");
	  if (vtable != cdoc->lispdata.vtable)
	    run_time_error("Found improper Cdoc in virtual table");
	  cdoc = cdoc->lispdata.ksuper;
	  vtable = (cdoc) ? cdoc->lispdata.vtable : 0;
#else
	  vtable = (struct VClass_object*)(vtable->Cdoc);
#endif
	}
      if (vtable && vtable == classvtable)
	return 1;
    }
  return 0;
}

void
check_obj_class(void *obj, void *classvtable)
{
  if (! obj)
    run_time_error("Casting a null gptr as an object");
  if (! test_obj_class(obj, classvtable))
    run_time_error("Illegal object cast");
}


/******************************************************************************
 *
 *  Storage allocation
 *
 *****************************************************************************/

void
srg_resize_compiled(struct srg *sr, int new_size, char *file, int line) 
{
  if(sr->flags & STS_MALLOC)  { 
    char *malloc_ptr; 
    int st_size = storage_type_size[sr->type] * new_size; 
    if (sr->size != 0) {
      if (st_size==0) {
        lush_free(sr->data, file, line); 
        sr->data = 0;
      } else {
        malloc_ptr = (char *) lush_realloc(sr->data, st_size, file, line); 
        if(malloc_ptr == 0) 
          run_time_error(rterr_out_of_memory); 
        sr->data = malloc_ptr; 
      } 
    } else { 
      if (st_size != 0) {
        malloc_ptr = (char *) lush_malloc(st_size, file, line); 
        if(malloc_ptr == 0) 
          run_time_error(rterr_out_of_memory); 
        sr->data = malloc_ptr; 
      } 
    } 
    sr->size = new_size; 
    sr->flags &= ~STF_UNSIZED;
  } else 
    run_time_error(rterr_cannot_realloc); 
}

void
srg_resize(struct srg *sr, int new_size, char *file, int line) 
{
#ifndef NOLISP
  if(sr->flags & STS_MALLOC) { 
    char *malloc_ptr; 
    int st_size = storage_type_size[sr->type] * new_size; 
    if (sr->size != 0) {
      if (st_size==0) {
	lush_free(sr->data, file, line); 
	sr->data = 0;
      } else {
        malloc_ptr = (char *) lush_realloc(sr->data, st_size, file, line); 
        if(malloc_ptr == 0) 
          error(NIL, rterr_out_of_memory, NIL); 
        sr->data = malloc_ptr; 
      } 
    } else { 
      if (st_size != 0) {
        malloc_ptr = (char *) lush_malloc(st_size, file, line); 
        if(malloc_ptr == 0) 
          error(NIL, rterr_out_of_memory, NIL); 
        sr->data = malloc_ptr; 
      } 
    } 
    sr->size = new_size; 
    sr->flags &= ~STF_UNSIZED;
  } else {
    error(NIL, rterr_cannot_realloc, NIL); 
  }
#else
  srg_resize_compiled(sr, new_size, file, line);
#endif
}

void
srg_free(struct srg *sr)
{
  if (sr->flags & STS_MALLOC)
    if ( sr->size && sr->data )
      {
        free(sr->data);
        sr->data = 0;
        sr->size = 0;
      }
}


/******************************************************************************
 *
 *  Matrix check functions
 *
 *****************************************************************************/


void 
check_main_maout(struct idx *i1, struct idx *i2)
{
  Mcheck_main_maout(i1, i2);
}

void 
check_main_maout_any(struct idx *i1, struct idx *i2)
{
  if (i2->flags & IDF_UNSIZED)
    {
      if (i1->ndim != i2->ndim)
        run_time_error(rterr_not_same_dim);
      Mcheck_main_maout(i1, i2);
    }
  else
    {
      int i, n1=1, n2=1;
      for (i=0; i<i1->ndim; i++)
        n1 *= i1->dim[i];
      for (i=0; i<i2->ndim; i++)
        n2 *= i2->dim[i];
      if (n1 != n2)
        run_time_error(rterr_not_same_dim);
    }
}

void 
check_main_main_maout(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_main_main_maout(i0, i1, i2);
}

void 
check_main_m0out(struct idx *i1, struct idx *i2)
{
  Mcheck_main_m0out(i1, i2);
}

void 
check_main_main_m0out(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_main_main_m0out(i0, i1, i2);
}

void 
check_main_m0in_maout(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_main_m0in_maout(i0, i1, i2);
}

void 
check_main_main_maout_dot21(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_main_main_maout_dot21(i0, i1, i2);
}

void 
check_main_main_maout_dot42(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_main_main_maout_dot42(i0, i1, i2);
}

void 
check_m1in_m1in_m2out(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_m1in_m1in_m2out(i0, i1, i2);
}

void 
check_m2in_m2in_m4out(struct idx *i0, struct idx *i1, struct idx *i2)
{
  Mcheck_m2in_m2in_m4out(i0, i1, i2);
}






