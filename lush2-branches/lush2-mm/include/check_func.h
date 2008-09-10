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
 * $Id: check_func.h,v 1.10 2004/10/23 01:23:55 leonb Exp $
 **********************************************************************/

#ifndef CHECK_FUNC_H
#define CHECK_FUNC_H

#ifndef LUSHAPI
#define LUSHAPI
#endif

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif


/* ---------------------------------------- */
/* TRACING CALLS                            */
/* ---------------------------------------- */


struct dh_trace_stack {
    char *info;
    struct dh_trace_stack *next;
};

extern LUSHAPI struct dh_trace_stack *dh_trace_root;

LUSHAPI void print_dh_recent(int,FILE*);
LUSHAPI void print_dh_trace_stack(void);

#define TRACE_PUSH(s) \
 struct dh_trace_stack _trace; _trace.info = s; \
 _trace.next = dh_trace_root; dh_trace_root = &_trace; 

#define TRACE_POP(s) \
  dh_trace_root = _trace.next;


/* Tracing is enabled when running code from SN.
 * To enable tracing in a standalkone program, define TRACE 
 */

#ifdef NOLISP
#ifndef TRACE
#undef TRACE_PUSH
#undef TRACE_POP
#define TRACE_PUSH(s) /**/
#define TRACE_POP(s) /**/
#endif
#endif


/* ---------------------------------------- */
/* RUNTIME ERRORS                           */
/* ---------------------------------------- */

extern LUSHAPI char *rterr_bound;
extern LUSHAPI char *rterr_rtype;
extern LUSHAPI char *rterr_dim;
extern LUSHAPI char *rterr_loopdim;
extern LUSHAPI char *rterr_emptystr;
extern LUSHAPI char *rterr_range;
extern LUSHAPI char *rterr_srg_overflow;
extern LUSHAPI char *rterr_unsized_matrix;
extern LUSHAPI char *rterr_not_same_dim;
extern LUSHAPI char *rterr_out_of_memory;
extern LUSHAPI char *rterr_cannot_realloc;
extern LUSHAPI char *rterr_cannot_shrink;
extern LUSHAPI char *rterr_bad_dimensions;

#define RTERR_GEN(test,errstr) \
  if (test) { run_time_error(errstr); }
#define RTERR_BOUND(test) \
  RTERR_GEN(test,rterr_bound)
#define RTERR_RTYPE(test) \
  RTERR_GEN(test,rterr_rtype)
#define RTERR_DIM(test) \
  RTERR_GEN(test,rterr_dim)
#define RTERR_LOOPDIM(test) \
  RTERR_GEN(test,rterr_loopdim)
#define RTERR_EMPTYSTR(test) \
  RTERR_GEN(test,rterr_emptystr)
#define RTERR_RANGE(test) \
  RTERR_GEN(test,rterr_range)
#define RTERR_SRG_OVERFLOW \
  run_time_error(rterr_srg_overflow);


/* ---------------------------------------- */
/* CHECKING OBJECTS                         */
/* ---------------------------------------- */

LUSHAPI void check_obj_class(void *obj, void *classvtable);
LUSHAPI int  test_obj_class(void *obj, void *classvtable);

/* ---------------------------------------- */
/* CHECKING MATRICES                        */
/* ---------------------------------------- */


LUSHAPI void *lush_malloc(size_t, char *, int);
//LUSHAPI void srg_resize_compiled(struct srg* ,size_t ,char *, int);
LUSHAPI void srg_resize_mm(struct srg* ,size_t ,char *, int);
LUSHAPI void srg_resize(struct srg *, size_t, char *, int );
LUSHAPI void srg_free(struct srg *);

#define Mis_sized(i1) \
    if(IDX_UNSIZEDP(i1)) \
        run_time_error(rterr_unsized_matrix); 

#define Mis_sized_is_sized(i1, i2) \
    Mis_sized(i1); Mis_sized(i2)

#define Msame_size1(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    if((i1)->dim[0] != (i2)->dim[0]) \
        run_time_error(rterr_not_same_dim);

#define Msame_size2(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    if(((i1)->dim[0] != (i2)->dim[0]) || ((i1)->dim[1] != (i2)->dim[1])) \
	    run_time_error(rterr_not_same_dim);

#define Msame_size(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    { \
    for(int j=0; j<(i1)->ndim; j++) \
	if((i1)->dim[j] != (i2)->dim[j]) \
	    run_time_error(rterr_not_same_dim);}

#define Mstr_alloc(s, len)  \
    s = mm_allocv(mt_blob, len*sizeof(char)); \
    cside_create_str_gc(s);

/* s = lush_malloc(len*sizeof(char), __FILE__, __LINE__);  */

/* #define Msrg_resize(sr, new_size) \ */
/*       srg_resize_compiled(sr, new_size, __FILE__, __LINE__);  */

#define Msrg_resize(sr, new_size) \
   if((sr)->size < (size_t)(new_size)) { \
      srg_resize_mm(sr, new_size, __FILE__, __LINE__); \
   } else if ((sr)->size > (size_t)(new_size)) { \
      srg_resize_mm(sr, new_size, __FILE__, __LINE__); \
      fprintf(stderr, "*** Warning: shrinking storage at %lx (lside: %lx)\n", \
              (long)sr, (long)cside_find_litem(sr)); \
   }
/*       run_time_error(rterr_cannot_shrink); */

#define Midx_checksize0(i1) { \
    size_t siz = (i1)->offset + 1; \
    Msrg_resize((i1)->srg, siz) \
}

#define Midx_checksize1(i1) { \
    size_t siz = 1 + (i1)->offset + ((i1)->dim[0] - 1) * (i1)->mod[0]; \
    Msrg_resize((i1)->srg, siz) \
}

#define Midx_checksize(i1) { \
    size_t siz=(i1)->offset+1; \
    for(int j=0; j<(i1)->ndim; j++) \
	siz += ((i1)->dim[j] - 1) * (i1)->mod[j]; \
    Msrg_resize((i1)->srg, siz); \
}

#define Mcheck0(i1) \
    if (IDX_UNSIZEDP(i1)) { \
        Midx_checksize0(i1);  \
    }

#define Msize_or_check0(i1, i2) \
    Mis_sized(i1) \
    if (IDX_UNSIZEDP(i2)) { \
        Midx_checksize0(i2);  \
    }

#define Msize_or_check1(i1, i2) \
    Mis_sized(i1) \
    if (IDX_UNSIZEDP(i2)) { \
        (i2)->dim[0]=(i1)->dim[0]; \
        (i2)->mod[0]=1; \
        Midx_checksize1(i2);  \
    } else \
        if ((i1)->dim[0] != (i2)->dim[0]) \
            run_time_error(rterr_bad_dimensions); 
 
#define Msize_or_check2(i1, i2) \
    Mis_sized(i1) \
    if (IDX_UNSIZEDP(i2)) { \
	(i2)->dim[1]=(i1)->dim[1]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i1)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
    } else \
        if (((i1)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[1]!=(i2)->dim[1])) \
            run_time_error(rterr_bad_dimensions); 

#define Msize_or_check(i1, i2) \
    Mis_sized(i1) \
    if (IDX_UNSIZEDP(i2)) { \
	size_t m=1; \
	for (int j=(i2)->ndim-1; j>=0; --j) { \
	  (i2)->dim[j]=(i1)->dim[j]; \
	  (i2)->mod[j]=m; \
	  m *= (i2)->dim[j]; \
	} \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
        if ((i1)->ndim != (i2)->ndim) \
          run_time_error(rterr_bad_dimensions); \
        for (int j=0; j< (i1)->ndim; j++)  \
          if ((i1)->dim[j] != (i2)->dim[j]) \
            run_time_error(rterr_bad_dimensions); \
    }

#define Msize_or_check_1D(dim0, i2) \
    if (IDX_UNSIZEDP(i2)) { \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=1; \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 1 || (i2)->dim[0] != dim0) \
            run_time_error(rterr_bad_dimensions); \
    }

#define Msize_or_check_2D(dim0, dim1, i2) \
    if (IDX_UNSIZEDP(i2)) { \
	(i2)->dim[1]=dim1; \
	(i2)->mod[1]=1; \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=dim1; \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 2 || (i2)->dim[0] != dim0 || (i2)->dim[1] != dim1) \
            run_time_error(rterr_bad_dimensions); \
    }

#define Mcheck_main(i1) \
    Mis_sized(i1);

#define Mcheck_main_maout(i1, i2) \
    Mis_sized(i1); \
    Msize_or_check(i1,i2);

#define Mcheck_main_main_maout(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msame_size(i0, i1); \
    Msize_or_check(i1, i2);

#define Mcheck_main_main_maout2(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msize_or_check(i1, i2);

#define Mcheck_main_main_maout_dot21(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msize_or_check_1D((i0)->dim[1], i1); \
    Msize_or_check_1D((i0)->dim[0], i2);

#define Mcheck_main_main_maout_dot42(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msize_or_check_2D((i0)->dim[2], (i0)->dim[3], i1); \
    Msize_or_check_2D((i0)->dim[0], (i0)->dim[1], i2);

#define Mcheck_main_m0out(i1, i2) \
    Mis_sized(i1); \
    Mcheck0(i2);

#define Mcheck_main_main_m0out(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msame_size(i0, i1); \
    Mcheck0(i2);    

#define Mcheck_main_m0in_maout(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    Msize_or_check(i0, i2);

#define Mcheck_m0in_m0out(i1, i2) \
    Msize_or_check0(i1,i2)

#define Mcheck_m1out(i1) \
    Mis_sized(i1);

#define Mcheck_m1in_m1in_m2out(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    if (IDX_UNSIZEDP(i2)) { \
	(i2)->dim[1]=(i1)->dim[0]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i0)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
    } else \
        if (((i0)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[0]!=(i2)->dim[1])) \
            run_time_error(rterr_bad_dimensions); 

#define Mcheck_m2in_m2in_m4out(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    if (IDX_UNSIZEDP(i2)) { \
	(i2)->dim[3]=(i1)->dim[1]; \
	(i2)->mod[3]=1;  \
	(i2)->dim[2]=(i1)->dim[0]; \
	(i2)->mod[2]=(i2)->dim[3]; \
	(i2)->dim[1]=(i0)->dim[1]; \
	(i2)->mod[1]= (i2)->dim[3] * (i2)->dim[2]; \
	(i2)->dim[0]=(i0)->dim[0]; \
	(i2)->mod[0]= (i2)->mod[1] * (i2)->dim[1]; \
        Midx_checksize(i2);  \
    } else \
        if (((i0)->dim[0]!=(i2)->dim[0]) || ((i0)->dim[1]!=(i2)->dim[1]) || \
            ((i1)->dim[0]!=(i2)->dim[2]) || ((i1)->dim[1]!=(i2)->dim[3])) \
            run_time_error(rterr_bad_dimensions); 

/* Mcheck_m1in_m0out, Mcheck_m2in_m0out --> Mcheck_main_m0out */
/* Mcheck_m1in_m1out, Mcheck_m2in_m2out --> Mcheck_main_maout */
/* Mcheck_m1in_m0in_m1out, Mcheck_m2in_m0in_m2out --> Mcheck_main_m0in_maout */
/* Mcheck_m1in_m1in_m1out, Mcheck_m2in_m2in_m2out --> Mcheck_main_main_maout */

LUSHAPI void check_main_maout(struct idx *i1, struct idx *i2);

LUSHAPI void check_main_maout_any(struct idx *i1, struct idx *i2);

LUSHAPI void check_main_main_maout(struct idx *i0, 
                                   struct idx *i1, struct idx *i2);
LUSHAPI void check_main_m0out(struct idx *i1, struct idx *i2);
LUSHAPI void check_main_main_m0out(struct idx *i0, 
                                   struct idx *i1, struct idx *i2);
LUSHAPI void check_main_m0in_maout(struct idx *i0, 
                                   struct idx *i1, struct idx *i2);
LUSHAPI void check_main_main_maout_dot21(struct idx *i0, 
                                         struct idx *i1, struct idx *i2);
LUSHAPI void check_main_main_maout_dot42(struct idx *i0, 
                                         struct idx *i1, struct idx *i2);
LUSHAPI void check_m1in_m1in_m2out(struct idx *i0, 
                                   struct idx *i1, struct idx *i2);
LUSHAPI void check_m2in_m2in_m4out(struct idx *i0, 
                                   struct idx *i1, struct idx *i2);


/* ---------------------------------------- */
/* THE END                                  */
/* ---------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif

/* -------------------------------------------------------------
   Local Variables:
   c-font-lock-extra-types: (
     "FILE" "\\sw+_t" "at" "gptr" "real" "flt" "intg" )
   End:
   ------------------------------------------------------------- */
