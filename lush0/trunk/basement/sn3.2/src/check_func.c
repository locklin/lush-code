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

/* ======= Functions that check the dimensions of index parameters ======= */

#define CHECK_DHDOCS

#include "header.h"
#include "idxmac.h"
#include "idxops.h"
#include "check_func.h"



/******************************************************************************
 *
 *  Trace functions
 *
******************************************************************************/


struct dh_trace_stack *dh_trace_root = 0;

/* print_dh_recent: print n recent functions */

void 
print_dh_recent(n,f)
int n;
FILE *f;
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

void 
print_dh_trace_stack()
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
 *  Check functions
 *
******************************************************************************/


char *UNSIZED_MATRIX = "matrix is have not been sized";
char *NOT_SAME_DIM = "matrix must have same dimensions";
char *OUT_OF_MEMORY = "out of memory (reallocating storage)";
char *CANNOT_REALLOC = "cannot reallocate storage";
char *BAD_DIMENSIONS = "arguments have the wrong dimensions";

#undef CHECK_DHDOCS

/* check if arg1 is dimensioned.  This cover all the cases: 
   check_m0in check_m1in check_m2in ...
   check_m0out check_m1out check_m2out ...
   */

/* check if first arg is dimensioned, and if total number of elements
   of 2nd arg is same as first arg.
   if 2nd arg unsized, give it the same structure as first arg
*/
int check_main_maout(i1, i2)
struct idx *i1, *i2;
{
    Mcheck_main_maout(i1, i2);
}

int check_main_main_maout(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_main_main_maout(i0, i1, i2);
}

/* check if 1st arg is dimensioned (ma), and second is m0 */
int check_main_m0out(i1, i2)
struct idx *i1, *i2;
{
    Mcheck_main_m0out(i1, i2);
}

/* check if 1st and 2nd arg have same global size (ma), and second is m0 */
int check_main_main_m0out(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_main_main_m0out(i0, i1, i2);
}

/* check if first arg is dimensioned, and if total number of elements
   of 3rd arg is same as first arg.
   if 3rd arg unsized, give it the same structure as first arg
   2nd arg must be an M0.
*/
int check_main_m0in_maout(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_main_m0in_maout(i0, i1, i2);
}

int check_main_main_maout_dot21(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_main_main_maout_dot21(i0, i1, i2);
}

int check_main_main_maout_dot42(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_main_main_maout_dot42(i0, i1, i2);
}

int check_m1in_m1in_m2out(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_m1in_m1in_m2out(i0, i1, i2);
}

int check_m2in_m2in_m4out(i0, i1, i2)
struct idx *i0, *i1, *i2;
{
    Mcheck_m2in_m2in_m4out(i0, i1, i2);
}




/******************************************************************************
 *
 *  SRG allocation
 *
******************************************************************************/

/*
 * srg_resize : this function is called from Msrg_resize.
 * ( warning: srg_resize assumes that new_size > sr->size )
 */
#ifndef NOLISP

int
srg_resize(sr, new_size, file, line) 
struct srg *sr;
int new_size;
char *file;
int line;
{
    if(sr->flags & STS_MALLOC)  { 
        char *malloc_ptr; 
        int st_size = storage_type_size[sr->type] * new_size; 
        if (sr->size != 0) {
            if (st_size==0) {
                sn3_free(sr->data, file, line); 
                sr->data = 0;
            } else {
		malloc_ptr = (char *) sn3_realloc(sr->data, st_size, file, line); 
		if(malloc_ptr == 0) 
		    error(NIL, OUT_OF_MEMORY, NIL); 
		sr->data = malloc_ptr; 
            } 
        } else { 
            if (st_size != 0) {
                malloc_ptr = (char *) sn3_malloc(st_size, file, line); 
                if(malloc_ptr == 0) 
		    error(NIL, OUT_OF_MEMORY, NIL); 
                sr->data = malloc_ptr; 
            } 
        } 
        sr->size = new_size; 
        sr->flags &= ~STF_UNSIZED;
    } else 
        error(NIL, CANNOT_REALLOC, NIL); 
}

#endif /* NOLISP */

/* Same as above, except that it calls run_time_error instead of error */
int
srg_resize_compiled(sr, new_size, file, line) 
struct srg *sr;
int new_size;
char *file;
int line;
{
    if(sr->flags & STS_MALLOC)  { 
        char *malloc_ptr; 
        int st_size = storage_type_size[sr->type] * new_size; 
        if (sr->size != 0) {
            if (st_size==0) {
                sn3_free(sr->data, file, line); 
                sr->data = 0;
            } else {
		malloc_ptr = (char *) sn3_realloc(sr->data, st_size, file, line); 
		if(malloc_ptr == 0) 
		    run_time_error(OUT_OF_MEMORY); 
		sr->data = malloc_ptr; 
            } 
        } else { 
            if (st_size != 0) {
                malloc_ptr = (char *) sn3_malloc(st_size, file, line); 
                if(malloc_ptr == 0) 
		    run_time_error(OUT_OF_MEMORY); 
                sr->data = malloc_ptr; 
            } 
        } 
        sr->size = new_size; 
        sr->flags &= ~STF_UNSIZED;
    } else 
        run_time_error(CANNOT_REALLOC); 
}


/* 
 * srg_free : this function is not called from Msrg_free.
 * (it is used when registering a destructor in a pool)
 */

void
srg_free(sr)
struct srg *sr;
{
    if (sr->flags & STS_MALLOC)
        if ( sr->size && sr->data )
        {
            free(sr->data);
            sr->data = 0;
            sr->size = 0;
        }
}


/*
 * These functions are used from the interpreted only.
 */
#ifndef NOLISP

void is_sized(i1)
    struct idx *i1;
{ 
    if((i1)->flags & IDF_UNSIZED)
        error(NIL, UNSIZED_MATRIX, NIL); 
}

DX(Xis_sized)
{		
    struct idx i1;
    struct index *ind1;

    ALL_ARGS_EVAL;
    ARG_NUMBER(1);
    ind1 = AINDEX(1);

    index_read_idx(ind1, &i1);
    is_sized(i1);
    index_rls_idx(ind1, &i1);		
    return NIL;
}

/* check if arg1 and arg2 have the same size.  This cover all the cases
   where there are two "in" matrices of the same size.  For example:
       check_m0in_m0in_... check_m1in_m1in_... check_m2in_m2in_... 
*/

void Csame_size(i1, i2)
    struct idx *i1, *i2;
{ 

    if(((i1)->flags & IDF_UNSIZED) || ((i2)->flags & IDF_UNSIZED))
        error(NIL, UNSIZED_MATRIX, NIL); 

    if((i1)->dim[0] != (i2)->dim[0])
        error(NIL, NOT_SAME_DIM, NIL);
}

DX(Xsame_size)
{		
    struct idx i1,i2;
    struct index *ind1, *ind2;

    ALL_ARGS_EVAL;
    ARG_NUMBER(2);
    ind1 = AINDEX(1);
    ind2 = AINDEX(2);

    index_read_idx(ind1, &i1);
    index_read_idx(ind2, &i2);
    Csame_size(i1, i2);
    index_rls_idx(ind1, &i1);		
    index_rls_idx(ind2, &i2);		
    return NIL;
}

/* check if arg1 and arg2 have the same size or dimension arg2 according 
   to arg1.
   This cover all the cases where there is one "out" matrix which should be the
   same size of a "in" matrix.  For example:
       check_m0in_..._m0out check_m1in_m1out_... check_main_maout_...
*/

void size_or_check(i1, i2)
    struct idx *i1;
    struct idx *i2;
{ 
    is_sized(i1);
    
    if ((i2)->flags & IDF_UNSIZED) {
	int j, m=1;
	for (j=(i2)->ndim-1; j>=0; --j) {
	    (i2)->dim[j]=(i1)->dim[j];
	    (i2)->mod[j]=m;
	    m *= (i2)->dim[j];
	} 
        { 
            int j, siz=(i2)->offset+1; 
            for(j=0; j<(i2)->ndim; j++) 
                siz += ((i2)->dim[j] - 1) * (i2)->mod[j]; 
            if(siz > (i2)->srg->size) {
                if(((i2)->srg)->size < siz) 
                    srg_resize((i2)->srg, siz, __FILE__, __LINE__);
            }
        }
        
        (i2)->flags &= ~IDF_UNSIZED;
    } else {  /* both are dimensioned, then check */
        int s1=1, s2=1, j;
        for (j=0; j< (i1)->ndim; j++)
            s1 *= (i1)->dim[j];
        for (j=0; j< (i2)->ndim; j++)
            s2 *= (i2)->dim[j];
        if (s1 != s2)
            error(NIL,BAD_DIMENSIONS,NIL);
    }
}

DX(Xsize_or_check)
{		
    struct idx i1,i2;
    struct index *ind1, *ind2;

    ALL_ARGS_EVAL;
    ARG_NUMBER(2);
    ind1 = AINDEX(1);
    ind2 = AINDEX(2);

    index_read_idx(ind1, &i1);
    index_write_idx(ind2, &i2);
    size_or_check(i1,i2);
    index_rls_idx(ind1, &i1);		
    index_rls_idx(ind2, &i2);		
    return NIL;
}


void init_check_func()
{
	dx_define("idx-is-sized", Xis_sized);
	dx_define("idx-same-size", Xsame_size);
	dx_define("idx-size-or-check", Xsize_or_check);
}

#endif /* NOLISP */
