/***********************************************************************
  SN3: Heir of the SN family
  (LYB) (YLC) 91 (PYS) 93
  ******************************************************************/



/* ---------------------------------------- */
/* TRACING CALLS                            */
/* ---------------------------------------- */


extern struct dh_trace_stack {
    char *info;
    struct dh_trace_stack *next;
} *dh_trace_root;


extern void print_dh_recent(int,FILE*);
extern void print_dh_trace_stack(void);

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
/* CHECKING MATRICES                        */
/* ---------------------------------------- */

extern char *UNSIZED_MATRIX;
extern char *NOT_SAME_DIM;
extern char *OUT_OF_MEMORY;
extern char *CANNOT_REALLOC;
extern char *BAD_DIMENSIONS;

void srg_resize_compiled(struct srg* ,int ,char *, int);

#define Mis_sized(i1) \
    if((i1)->flags & IDF_UNSIZED) \
        run_time_error(UNSIZED_MATRIX); 

#define Mis_sized_is_sized(i1, i2) \
    if(((i1)->flags & IDF_UNSIZED) || ((i2)->flags & IDF_UNSIZED)) \
        run_time_error(UNSIZED_MATRIX); 

#define Msame_size1(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    if((i1)->dim[0] != (i2)->dim[0]) \
        run_time_error(NOT_SAME_DIM);

#define Msame_size2(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    if(((i1)->dim[0] != (i2)->dim[0]) || ((i1)->dim[1] != (i2)->dim[1])) \
	    run_time_error(NOT_SAME_DIM);

#define Msame_size(i1,i2) \
    Mis_sized_is_sized(i1, i2) \
    { int j; \
    for(j=0; j<(i1)->ndim; j++) \
	if((i1)->dim[j] != (i2)->dim[j]) \
	    run_time_error(NOT_SAME_DIM);}

#define Msrg_resize(sr, new_size) \
if((sr)->size < new_size) \
   srg_resize_compiled(sr, new_size, __FILE__, __LINE__);

#define Midx_checksize0(i1) { \
    int siz = (i1)->offset + 1; \
    Msrg_resize((i1)->srg, siz) \
}

#define Midx_checksize1(i1) { \
    int siz = 1 + (i1)->offset + ((i1)->dim[0] - 1) * (i1)->mod[0]; \
    Msrg_resize((i1)->srg, siz) \
}

#define Midx_checksize(i1) { \
    int j, siz=(i1)->offset+1; \
    for(j=0; j<(i1)->ndim; j++) \
	siz += ((i1)->dim[j] - 1) * (i1)->mod[j]; \
    Msrg_resize((i1)->srg, siz); \
}

#define Mcheck0(i1) \
    if ((i1)->flags & IDF_UNSIZED) { \
        Midx_checksize0(i1);  \
        (i1)->flags &= ~IDF_UNSIZED; \
    }

#define Msize_or_check0(i1, i2) \
    Mis_sized(i1) \
    if ((i2)->flags & IDF_UNSIZED) { \
        Midx_checksize0(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    }

#define Msize_or_check1(i1, i2) \
    Mis_sized(i1) \
    if ((i2)->flags & IDF_UNSIZED) { \
        (i2)->dim[0]=(i1)->dim[0]; \
        (i2)->mod[0]=1; \
        Midx_checksize1(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else \
        if ((i1)->dim[0] != (i2)->dim[0]) \
            run_time_error(BAD_DIMENSIONS); 
 
#define Msize_or_check2(i1, i2) \
    Mis_sized(i1) \
    if ((i2)->flags & IDF_UNSIZED) { \
	(i2)->dim[1]=(i1)->dim[1]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i1)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else \
        if (((i1)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[1]!=(i2)->dim[1])) \
            run_time_error(BAD_DIMENSIONS); 

#define Msize_or_check(i1, i2) \
    Mis_sized(i1) \
    if ((i2)->flags & IDF_UNSIZED) { \
	int j, m=1; \
	for (j=(i2)->ndim-1; j>=0; --j) { \
	    (i2)->dim[j]=(i1)->dim[j]; \
	    (i2)->mod[j]=m; \
	    m *= (i2)->dim[j]; \
	} \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else {  /* both are dimensioned, then check */ \
        int s1=1, s2=1, j; \
        for (j=0; j< (i1)->ndim; j++)  \
            s1 *= (i1)->dim[j]; \
        for (j=0; j< (i2)->ndim; j++) \
            s2 *= (i2)->dim[j]; \
        if (s1 != s2) \
            run_time_error(BAD_DIMENSIONS); \
    }

#define Msize_or_check_1D(dim0, i2) \
    if ((i2)->flags & IDF_UNSIZED) { \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=1; \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 1 || (i2)->dim[0] != dim0) \
            run_time_error(BAD_DIMENSIONS); \
    }

#define Msize_or_check_2D(dim0, dim1, i2) \
    if ((i2)->flags & IDF_UNSIZED) { \
	(i2)->dim[1]=dim1; \
	(i2)->mod[1]=1; \
	(i2)->dim[0]=dim0; \
	(i2)->mod[0]=dim1; \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else {  /* both are dimensioned, then check */ \
	if ((i2)->ndim != 2 || (i2)->dim[0] != dim0 || (i2)->dim[1] != dim1) \
            run_time_error(BAD_DIMENSIONS); \
    }

/******************************************************************************/

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
    if ((i2)->flags & IDF_UNSIZED) { \
	(i2)->dim[1]=(i1)->dim[0]; \
	(i2)->mod[1]=1;  \
	(i2)->dim[0]=(i0)->dim[0]; \
	(i2)->mod[0]=(i2)->dim[1]; \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else \
        if (((i0)->dim[0]!=(i2)->dim[0]) || ((i1)->dim[0]!=(i2)->dim[1])) \
            run_time_error(BAD_DIMENSIONS); 

#define Mcheck_m2in_m2in_m4out(i0, i1, i2) \
    Mis_sized_is_sized(i0, i1); \
    if ((i2)->flags & IDF_UNSIZED) { \
	(i2)->dim[3]=(i1)->dim[1]; \
	(i2)->mod[3]=1;  \
	(i2)->dim[2]=(i1)->dim[0]; \
	(i2)->mod[2]=(i2)->dim[3]; \
	(i2)->dim[1]=(i0)->dim[1]; \
	(i2)->mod[1]= (i2)->dim[3] * (i2)->dim[2]; \
	(i2)->dim[0]=(i0)->dim[0]; \
	(i2)->mod[0]= (i2)->mod[1] * (i2)->dim[1]; \
        Midx_checksize(i2);  \
        (i2)->flags &= ~IDF_UNSIZED; \
    } else \
        if (((i0)->dim[0]!=(i2)->dim[0]) || ((i0)->dim[1]!=(i2)->dim[1]) || \
            ((i1)->dim[0]!=(i2)->dim[2]) || ((i1)->dim[1]!=(i2)->dim[3])) \
            run_time_error(BAD_DIMENSIONS); 

/* Mcheck_m1in_m0out, Mcheck_m2in_m0out --> Mcheck_main_m0out */
/* Mcheck_m1in_m1out, Mcheck_m2in_m2out --> Mcheck_main_maout */
/* Mcheck_m1in_m0in_m1out, Mcheck_m2in_m0in_m2out --> Mcheck_main_m0in_maout */
/* Mcheck_m1in_m1in_m1out, Mcheck_m2in_m2in_m2out --> Mcheck_main_main_maout */



