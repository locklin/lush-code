
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
 * $Id: check_func.c,v 1.1 2002-04-18 20:17:13 leonb Exp $
 **********************************************************************/

/* Functions that check the dimensions of index parameters */

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
 *  SRG allocation
 *
******************************************************************************/

/*
 * srg_resize : this function is called from Msrg_resize.
 * ( warning: srg_resize assumes that new_size > sr->size )
 */
#ifndef NOLISP

void
srg_resize(struct srg *sr, int new_size, char *file, int line) 
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
		    error(NIL, OUT_OF_MEMORY, NIL); 
		sr->data = malloc_ptr; 
            } 
        } else { 
            if (st_size != 0) {
                malloc_ptr = (char *) lush_malloc(st_size, file, line); 
                if(malloc_ptr == 0) 
		    error(NIL, OUT_OF_MEMORY, NIL); 
                sr->data = malloc_ptr; 
            } 
        } 
        sr->size = new_size; 
        sr->flags &= ~STF_UNSIZED;
    } else {
        error(NIL, CANNOT_REALLOC, NIL); 
    }
}

#endif /* NOLISP */

/* Same as above, except that it calls run_time_error instead of error */
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
		    run_time_error(OUT_OF_MEMORY); 
		sr->data = malloc_ptr; 
            } 
        } else { 
            if (st_size != 0) {
                malloc_ptr = (char *) lush_malloc(st_size, file, line); 
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
 *  Check functions
 *
******************************************************************************/


char *UNSIZED_MATRIX = "matrix has not been sized";
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
void 
check_main_maout(struct idx *i1, struct idx *i2)
{
    Mcheck_main_maout(i1, i2);
}

void 
check_main_main_maout(struct idx *i0, struct idx *i1, struct idx *i2)
{
    Mcheck_main_main_maout(i0, i1, i2);
}

/* check if 1st arg is dimensioned (ma), and second is m0 */
void 
check_main_m0out(struct idx *i1, struct idx *i2)
{
    Mcheck_main_m0out(i1, i2);
}

/* check if 1st and 2nd arg have same global size (ma), and second is m0 */
void 
check_main_main_m0out(struct idx *i0, struct idx *i1, struct idx *i2)
{
    Mcheck_main_main_m0out(i0, i1, i2);
}

/* check if first arg is dimensioned, and if total number of elements
   of 3rd arg is same as first arg.
   if 3rd arg unsized, give it the same structure as first arg
   2nd arg must be an M0.
*/
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






