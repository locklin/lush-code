/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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
    const char *info;
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
extern LUSHAPI char *rterr_emptyidx;
extern LUSHAPI char *rterr_range;
extern LUSHAPI char *rterr_srg_overflow;
extern LUSHAPI char *rterr_not_same_dim;
extern LUSHAPI char *rterr_unmanaged;
extern LUSHAPI char *rterr_out_of_memory;
extern LUSHAPI char *rterr_cannot_realloc;
extern LUSHAPI char *rterr_cannot_shrink;
extern LUSHAPI char *rterr_bad_dimensions;

#define RTERR_GEN(test,errstr) \
  if (test) { lush_error(errstr); }
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
  lush_error(rterr_srg_overflow);


/* ---------------------------------------- */
/* CHECKING OBJECTS                         */
/* ---------------------------------------- */

/* ---------------------------------------- */
/* CHECKING MATRICES                        */
/* ---------------------------------------- */

LUSHAPI bool idx_emptyp(index_t *);

#define Mnocheck(...)

#define Mis_nonempty(i1) \
  if (idx_emptyp(i1))              \
    lush_error(rterr_emptyidx); 

#define Mis_sized Mis_nonempty

#define Mis_nonempty_is_nonempty(i1, i2) \
    Mis_nonempty(i1); Mis_nonempty(i2)

#define Mis_sized Mis_nonempty

#define Msame_size1(i1,i2) \
    if((i1)->dim[0] != (i2)->dim[0]) \
        lush_error(rterr_not_same_dim);

#define Msame_size2(i1,i2) \
    if(((i1)->dim[0] != (i2)->dim[0]) || ((i1)->dim[1] != (i2)->dim[1])) \
	    lush_error(rterr_not_same_dim);

#define Msame_size(i1,i2) \
    { \
    for(int j=0; j<(i1)->ndim; j++) \
	if((i1)->dim[j] != (i2)->dim[j]) \
	    lush_error(rterr_not_same_dim);}

#define Mstr_alloc(s, len)  \
    s = mm_blob(len*sizeof(char)); 

#define Msrg_resize(sr, new_size)                                    \
   if((sr)->size < (size_t)(new_size)) {                             \
      storage_realloc(sr, new_size, 0);                              \
   } else if ((sr)->size > (size_t)(new_size)) {                     \
      storage_realloc(sr, new_size, 0);                              \
      fprintf(stderr, "*** Warning: shrinking storage at %p\n", sr); \
   }

#define Midx_checksize0(i1) { \
    size_t siz = (i1)->offset + 1; \
    Msrg_resize((i1)->st, siz) \
}

#define Midx_checksize1(i1) { \
    size_t siz = 1 + (i1)->offset + ((i1)->dim[0] - 1) * (i1)->mod[0]; \
    Msrg_resize((i1)->st, siz) \
}

#define Midx_checksize(i1) { \
    size_t siz=(i1)->offset+1; \
    for(int j=0; j<(i1)->ndim; j++) \
	siz += ((i1)->dim[j] - 1) * (i1)->mod[j]; \
    Msrg_resize((i1)->st, siz); \
}

#define Mcheck0(i1) \
    if (idx_emptyp(i1)) { \
        Midx_checksize0(i1);  \
    }

#define Msize_or_check0(i1, i2) \
    if (idx_emptyp(i2)) { \
        Midx_checksize0(i2);  \
    }

#define Msize_or_check1(i1, i2) \
    if (idx_emptyp(i2)) { \
        (i2)->dim[0]=(i1)->dim[0]; \
        (i2)->mod[0]=1; \
        Midx_checksize1(i2);  \
    } else \
        if ((i1)->dim[0] != (i2)->dim[0]) \
            lush_error(rterr_bad_dimensions); 
 
#define Msize_or_check2(i1, i2) \
    if (idx_emptyp(i2)) { \
	(i2)->dim[1]=(i1)->dim[1]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i1)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
    } else \
        if (((i1)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[1]!=(i2)->dim[1])) \
            lush_error(rterr_bad_dimensions); 

#define Msize_or_check(i1, i2) \
    if (idx_emptyp(i2)) { \
	size_t m=1; \
	for (int j=(i2)->ndim-1; j>=0; --j) { \
	  (i2)->dim[j]=(i1)->dim[j]; \
	  (i2)->mod[j]=m; \
	  m *= (i2)->dim[j]; \
	} \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
        if ((i1)->ndim != (i2)->ndim) \
          lush_error(rterr_bad_dimensions); \
        for (int j=0; j< (i1)->ndim; j++)  \
          if ((i1)->dim[j] != (i2)->dim[j]) \
            lush_error(rterr_bad_dimensions); \
    }

#define Msize_or_check_1D(dim0, i2) \
    if (idx_emptyp(i2)) { \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=1; \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 1 || (i2)->dim[0] != dim0) \
            lush_error(rterr_bad_dimensions); \
    }

#define Msize_or_check_2D(dim0, dim1, i2) \
    if (idx_emptyp(i2)) { \
	(i2)->dim[1]=dim1; \
	(i2)->mod[1]=1; \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=dim1; \
        Midx_checksize(i2);  \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 2 || (i2)->dim[0] != dim0 || (i2)->dim[1] != dim1) \
            lush_error(rterr_bad_dimensions); \
    }

#define Mcheck_main(i1) \
  /* Mis_sized(i1); */

#define Mcheck_main_maout(i1, i2) \
    Msize_or_check(i1,i2);

#define Mcheck_main_main_maout(i0, i1, i2) \
    Msame_size(i0, i1); \
    Msize_or_check(i1, i2);

#define Mcheck_main_main_maout2(i0, i1, i2) \
    Msize_or_check(i1, i2);

#define Mcheck_main_main_maout_dot21(i0, i1, i2) \
    Msize_or_check_1D((i0)->dim[1], i1); \
    Msize_or_check_1D((i0)->dim[0], i2);

#define Mcheck_main_main_maout_dot42(i0, i1, i2) \
    Msize_or_check_2D((i0)->dim[2], (i0)->dim[3], i1); \
    Msize_or_check_2D((i0)->dim[0], (i0)->dim[1], i2);

#define Mcheck_main_m0out(i1, i2) \
    Mcheck0(i2);

#define Mcheck_main_main_m0out(i0, i1, i2) \
    Msame_size(i0, i1); \
    Mcheck0(i2);    

#define Mcheck_main_m0in_maout(i0, i1, i2) \
    Msize_or_check(i0, i2);

#define Mcheck_m0in_m0out(i1, i2) \
    Msize_or_check0(i1,i2)

#define Mcheck_m1out(i1) \
  /* Mis_sized(i1); */

#define Mcheck_m1in_m1in_m2out(i0, i1, i2) \
    if (idx_emptyp(i2)) { \
	(i2)->dim[1]=(i1)->dim[0]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i0)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
    } else \
        if (((i0)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[0]!=(i2)->dim[1])) \
            lush_error(rterr_bad_dimensions); 

#define Mcheck_m2in_m2in_m4out(i0, i1, i2) \
    if (idx_emptyp(i2)) { \
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
            lush_error(rterr_bad_dimensions); 

/* Mcheck_m1in_m0out, Mcheck_m2in_m0out --> Mcheck_main_m0out */
/* Mcheck_m1in_m1out, Mcheck_m2in_m2out --> Mcheck_main_maout */
/* Mcheck_m1in_m0in_m1out, Mcheck_m2in_m0in_m2out --> Mcheck_main_m0in_maout */
/* Mcheck_m1in_m1in_m1out, Mcheck_m2in_m2in_m2out --> Mcheck_main_main_maout */

LUSHAPI void check_main_maout(index_t *i1, index_t *i2);

LUSHAPI void check_main_maout_any(index_t *i1, index_t *i2);

LUSHAPI void check_main_main_maout(index_t *i0, 
                                   index_t *i1, index_t *i2);
LUSHAPI void check_main_m0out(index_t *i1, index_t *i2);
LUSHAPI void check_main_main_m0out(index_t *i0, 
                                   index_t *i1, index_t *i2);
LUSHAPI void check_main_m0in_maout(index_t *i0, 
                                   index_t *i1, index_t *i2);
LUSHAPI void check_main_main_maout_dot21(index_t *i0, 
                                         index_t *i1, index_t *i2);
LUSHAPI void check_main_main_maout_dot42(index_t *i0, 
                                         index_t *i1, index_t *i2);
LUSHAPI void check_m1in_m1in_m2out(index_t *i0, 
                                   index_t *i1, index_t *i2);
LUSHAPI void check_m2in_m2in_m4out(index_t *i0, 
                                   index_t *i1, index_t *i2);


/* ---------------------------------------- */
/* THE END                                  */
/* ---------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
