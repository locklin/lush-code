/***********************************************************************
  SN3: Heir of the SN family
  (LYB) (YLC) 91
  ******************************************************************/


#ifndef IDXMAC_H
#define IDXMAC_H

void run_time_error(char *);

/* ================= IDX and LIST ACCESS TO THE DATA =================== */

#define IDX_PTR(id, Type) ((Type *)((id)->srg->data) + (id)->offset)

#define L_ACCESS(lname, n) \
    ((int *) ((char *) (((struct srg *) (lname))->data) + sizeof(dharg) * (n)))

/* ============== IDX STRUCTURE DIRECT MANIPULATION =========== */

/* Change dimension d of idx i to sz 
 * do not change the modulos 
 */
#define Midx_dim(i,d) ((i)->dim[d])

/* Change modulo d of idx i to sz 
 */
#define Midx_mod(i, d) ((i)->mod[d])

/* Change offset of idx i to sz 
 */
#define Midx_offset(i) ((i)->offset)

/* expect min and max to have been defined has int */
#define SRG_BOUNDS(idx, ndim, min, max)  \
min = (idx)->offset; \
max = (idx)->offset; \
{ \
    int i; \
    for(i=0;i<ndim;i++) \
        if((idx)->mod[i]<0) \
            min += ((idx)->dim[i]-1) * (idx)->mod[i]; \
        else \
            max += ((idx)->dim[i]-1) * (idx)->mod[i]; \
}

/* ============== IDX CREATION AND INITIALISATION ============== */

/* Following macros depend on MAXDIM */
#define Midx_copy_dim0(ni,i) 
#define Midx_copy_dim1(ni,i) \
    (ni)->dim[0] = (i)->dim[0]; (ni)->mod[0] = (i)->mod[0]; 
#define Midx_copy_dim2(ni,i) Midx_copy_dim1((ni),i); \
    (ni)->dim[1]=(i)->dim[1]; (ni)->mod[1]=(i)->mod[1]; 
#define Midx_copy_dim3(ni,i) Midx_copy_dim2((ni),i); \
    (ni)->dim[2]=(i)->dim[2]; (ni)->mod[2]=(i)->mod[2]; 
#define Midx_copy_dim4(ni,i) Midx_copy_dim3((ni),i); \
    (ni)->dim[3]=(i)->dim[3]; (ni)->mod[3]=(i)->mod[3]; 
#define Midx_copy_dim5(ni,i) Midx_copy_dim4((ni),i); \
    (ni)->dim[4]=(i)->dim[4]; (ni)->mod[4]=(i)->mod[4]; 
#define Midx_copy_dim6(ni,i) Midx_copy_dim5((ni),i); \
    (ni)->dim[5]=(i)->dim[5]; (ni)->mod[5]=(i)->mod[5]; 
#define Midx_copy_dim7(ni,i) Midx_copy_dim6((ni),i); \
    (ni)->dim[6]=(i)->dim[6]; (ni)->mod[6]=(i)->mod[6]; 


#define Midx_update_mod_from_dim(i0) \
{ \
    int i,siz=1; \
    for(i=(i0)->ndim - 1 ;i>= 0;i--) { \
	(i0)->mod[i] = siz; \
	siz *= (i0)->dim[i]; \
    } \
    (i0)->flags &= ~IDF_UNSIZED; \
    (i0)->offset = 0; \
    Msrg_resize(i0->srg, siz); \
}

#define Midx_setdim3(id,di0,di1,di2) \
    (id)->dim[0] = di0; \
    (id)->dim[1] = di1; \
    (id)->dim[2] = di2; 
#define Midx_setdim6(id,di0,di1,di2,di3,di4,di5) \
    Midx_setdim3(id,di0,di1,di2) \
    (id)->dim[3] = di3; \
    (id)->dim[4] = di4; \
    (id)->dim[5] = di5; 

#define Midx_init_dim0(id) \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim1(id,di0) \
    (id)->dim[0] = di0; \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim2(id,di0,di1) \
    (id)->dim[0] = di0; \
    (id)->dim[1] = di1; \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim3(id,di0,di1,di2) \
    Midx_setdim3(id,di0,di1,di2) \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim4(id,di0,di1,di2,di3) \
    Midx_setdim3(id,di0,di1,di2) \
    (id)->dim[3] = di3; \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim5(id,di0,di1,di2,di3,di4) \
    Midx_setdim3(id,di0,di1,di2) \
    (id)->dim[3] = di3; \
    (id)->dim[4] = di4; \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim6(id,di0,di1,di2,di3,di4,di5) \
    Midx_setdim6(id,di0,di1,di2,di3,di4,di5) \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim7(id,di0,di1,di2,di3,di4,di5,di6) \
    Midx_setdim6(id,di0,di1,di2,di3,di4,di5) \
    (id)->dim[6] = di6; \
    Midx_update_mod_from_dim(id);
#define Midx_init_dim8(id,di0,di1,di2,di3,di4,di5,di6,di7) \
    Midx_setdim6(id,di0,di1,di2,di3,di4,di5) \
    (id)->dim[6] = di6; \
    (id)->dim[7] = di7; \
    Midx_update_mod_from_dim(id);

/* ============== IDX STRUCTURE COMPLEX MANIPULATION =========== */


/* Unfold dimension d of an index,
 * appending an extra dimension of size sz,
 * stepping by st
 */  
#define Midx_unfold(i, d, sz, st, Type) \
{ \
  register int nd; \
  nd = ((i)->ndim)++; \
  (i)->mod[nd] = (i)->mod[d]; \
  (i)->dim[nd] = sz; \
  (i)->mod[d] *= st; \
  (i)->dim[d] = 1 + ((i)->dim[d]- sz) / st; \
}

/* Replace the last d dimension
 * by their diagonal
 */
#define Midx_diagonal(i, d, Type) \
{ \
  register int n,m; \
  for (m=0, n=(i)->ndim-d; n<(i)->ndim; n++) \
    m += (i)->mod[n]; \
  n = (i)->ndim - d; \
  (i)->mod[n] = m; \
  (i)->ndim = n+1; \
}
  
/* Restrict dimension d to size sz,
 * starting at position st.
 */
#define Midx_narrow(i, d, sz, st, Type) \
{ \
  (i)->dim[d] = sz; \
  (i)->offset += st*(i)->mod[d]; \
}

/* Returns the nth slice of the index along dimension d.
 * Dimension d is then collapsed.
 */

#define Midx_select(newi, i, dd, nn, Type) \
{ \
  register int j, temp = ((i)->ndim)-1; \
  (newi)->flags = (i)->flags; \
  (newi)->offset = (i)->offset + nn*((i)->mod[dd]); \
  (newi)->ndim = temp; \
  (newi)->srg = (i)->srg; \
  for (j=0;j<(dd);j++) { \
    (newi)->dim[j] = (i)->dim[j]; \
    (newi)->mod[j] = (i)->mod[j]; \
  } \
  for (j=(dd); j<temp; j++) { \
      (newi)->dim[j] = (i)->dim[j+1];  \
      (newi)->mod[j] = (i)->mod[j+1];  } \
}

/* Transpose the dimensions of i,
 * according to a permutation vector p
 */
#define Midx_tranpose(i, p, Type) \
{ \
  int tmp[MAXDIMS]; \
  register int j; \
  for (j=0; j<(i)->ndim; j++) { tmp[j] = (i)->dim[j]; } \
  for (j=0; j<(i)->ndim; j++) { (i)->dim[j] = tmp[p[j]] ; } \
  for (j=0; j<(i)->ndim; j++) { tmp[j] = (i)->mod[j]; } \
  for (j=0; j<(i)->ndim; j++) { (i)->mod[j] = tmp[p[j]] ; } \
}

/* Transpose dimensions d0 and d1 ot i.
 */
#define Midx_transpose2(i, d0, d1, Type) \
{ \
  register int b; \
  b = (i)->dim[d0]; (i)->dim[d0]=(i)->dim[d1]; (i)->dim[d1]=b; \
  b = (i)->mod[d0]; (i)->mod[d0]=(i)->mod[d1]; (i)->mod[d1]=b; \
} 


/* Declare new index newi (for clone & transclone) */
#define Midx_declare0(newi) \
struct idx name2(_idx_,newi); \
struct idx *newi = &name2(_idx_,newi)

#define Midx_declare(newi, ndim) \
int name2(_dim_,newi)[ndim]; \
int name2(_mod_,newi)[ndim]; \
struct idx name2(_idx_,newi); \
struct idx *newi = &name2(_idx_,newi)

#define Midx_init(newi, new_dim) \
    (newi)->ndim = new_dim; \
    (newi)->dim = name2(_dim_,newi); \
    (newi)->mod = name2(_mod_,newi); 

/* Declare new srg newi  */
#define Msrg_declare(newi) \
struct srg name2(_srg_,newi); \
struct srg *newi = & name2(_srg_,newi)

#define Msrg_init(newi, srg_type) \
    (newi)->size = 0; \
    (newi)->type = srg_type; \
    (newi)->flags = STS_MALLOC;

#define Msrg_free(newi) \
    if(newi->size != 0 && ((newi)->flags & STS_MALLOC)) \
        free(newi->data);

/* Define new index newi as a clone of index i */

#define Midx_short_clone(newi, i) \
{ int j; \
  newi->flags = (i)->flags; \
  newi->ndim = (i)->ndim; \
  newi->offset = (i)->offset; \
  newi->srg = (i)->srg; 

#define Midx_clone0(ni,i) Midx_short_clone(ni,i);Midx_copy_dim0(ni,i)}
#define Midx_clone1(ni,i) Midx_short_clone(ni,i);Midx_copy_dim1(ni,i)}
#define Midx_clone2(ni,i) Midx_short_clone(ni,i);Midx_copy_dim2(ni,i)}
#define Midx_clone3(ni,i) Midx_short_clone(ni,i);Midx_copy_dim3(ni,i)}
#define Midx_clone4(ni,i) Midx_short_clone(ni,i);Midx_copy_dim4(ni,i)}
#define Midx_clone5(ni,i) Midx_short_clone(ni,i);Midx_copy_dim5(ni,i)}
#define Midx_clone6(ni,i) Midx_short_clone(ni,i);Midx_copy_dim6(ni,i)}
#define Midx_clone7(ni,i) Midx_short_clone(ni,i);Midx_copy_dim7(ni,i)}

#define Midx_clone(newi, i, Type) \
{ int j; \
  newi->flags = (i)->flags; \
  newi->ndim = (i)->ndim; \
  newi->offset = (i)->offset; \
  newi->srg = (i)->srg; \
  for (j=0;j<(i)->ndim;j++) { \
    newi->dim[j] = (i)->dim[j]; \
    newi->mod[j] = (i)->mod[j]; \
  } \
}

/* Define new index newi as a clone of index i
 */
#define Midx_assign(newi, i, Type) \
{ int j; \
  (newi)->type = (i)->type; \
  (newi)->ndim = (i)->ndim; \
  (newi)->offset = (i)->offset; \
  (newi)->srg = (i)->srg; \
  for (j=0;j<(i)->ndim;j++) { \
    (newi)->dim[j] = (i)->dim[j]; \
    (newi)->mod[j] = (i)->mod[j]; \
  } \
}



/* Define new index newi as a transposed clone of index i
 */
#define Midx_transclone(newi, i, p, Type) \
{ int j; \
/*  newi.type = (i).type;*/ \
  newi->flags = (i)->flags; \
  newi->ndim = (i)->ndim; \
  newi->offset = (i)->offset; \
  newi->srg = (i)->srg; \
  for (j=0;j<(i)->ndim;j++) { \
    (newi)->dim[j] = (i)->dim[p[j]]; \
    (newi)->mod[j] = (i)->mod[p[j]]; \
  } \
}

/* Print error message 'message' if idx is not contiguous (a submatrix) */

#define Midx_contiguep0(idx, var) var = 1;
#define Midx_contiguep1(idx, var) \
    var = 1; if(1 != (idx)->mod[0]) var = 0;
#define Midx_contiguep2(idx, var) \
    var = 1; if(1 != (idx)->mod[1] || (idx)->dim[1] != (idx)->mod[0]) var = 0;
#define Midx_contiguep3(idx, var) \
    var = 1; if(1 != (idx)->mod[2] || (idx)->dim[2] != (idx)->mod[1] || \
    (idx)->dim[1]*(idx)->dim[2] != (idx)->mod[0]) var = 0;
#define Midx_contiguep4(idx, var) \
{   int size = 1, i; var = 1\
    for(i=3;i>=0;i--) { \
	if(size != (idx)->mod[i]) var = 0; \
	size *= (%s)->dim[i]; }}
#define Midx_contiguep5(idx, var) \
{   int size = 1, i; var = 1\
    for(i=4;i>=0;i--) { \
	if(size != (idx)->mod[i]) var = 0; \
	size *= (%s)->dim[i]; }}
#define Midx_contiguep6(idx, var) \
{   int size = 1, i; var = 1\
    for(i=5;i>=0;i--) { \
	if(size != (idx)->mod[i]) var = 0; \
	size *= (%s)->dim[i]; }}
#define Midx_contiguep7(idx, var) \
{   int size = 1, i; var = 1\
    for(i=6;i>=0;i--) { \
	if(size != (idx)->mod[i]) var = 0; \
	size *= (%s)->dim[i]; }}
#define Midx_contiguep8(idx, var) \
{   int size = 1, i; var = 1\
    for(i=7;i>=0;i--) { \
	if(size != (idx)->mod[i]) var = 0; \
	size *= (%s)->dim[i]; }}

/* ============ ELOOPS, BLOOPS SUPPORT ============== */


/* low level macros for the loops.
 */

#define Midxlow_binit(a1) \
  int _i, _sz = (a1)->dim[0]

#define Midxlow_einit(a1) \
  int _i, _sz = (a1)->dim[(a1)->ndim - 1]

/* BREAKING CONVENTION to save indirection overhead in bloops.
 *   'anl' token will represent a struct idx, not a pointer.
 *   'clone',  and 'advance' macros are modified to 
 *   support this.
 */
#define Midxlow_declare(an, anl) \
  struct idx anl; \
  int name2(anl,_Inc)

#define Midxlow_bclone(an, anl) \
  (&anl)->flags = (an)->flags; \
  (&anl)->ndim = (an)->ndim - 1; \
  (&anl)->dim = (an)->dim + 1; \
  (&anl)->mod = (an)->mod + 1; \
  (&anl)->offset = (an)->offset; \
  (&anl)->srg = (an)->srg; \
  name2(anl,_Inc) = (an)->mod[0]

#define Midxlow_eclone(an, anl) \
/*  (&anl).type = (an).type; */ \
  (&anl)->flags = (an)->flags; \
  (&anl)->ndim = (an)->ndim - 1; \
  (&anl)->dim = (an)->dim; \
  (&anl)->mod = (an)->mod; \
  (&anl)->offset = (an)->offset; \
  (&anl)->srg = (an)->srg; \
  name2(anl,_Inc) = (an)->mod[(an)->ndim - 1]

#define Midxlow_loop() \
  for(_i=0; _i<_sz; _i++) 

#define Midxlow_advance(an, anl, Type) \
  (&anl)->offset += name2(anl,_Inc)  



/* Replication gnu-emacs kbd macro:
 * Copies the current line, replicates the previous loop,
 * adding a new looping variable named according to the 
 * current yank buffer.
 *
(fset 'next-macro "xsea	ba	_begin_xsea	for	wxsea	ba	_begin_ffff_bxsearch-fo	Typexsea	ba	, l,,xsearch-fo	clone  Midxlow_declare(,l); \\xsearch-for	loop  Midx_low_clone(,l); \\bbwxsea	forw	_end_f_ùbxsearch-fo	Typexsea	back	, l,,}  M<idxlow_advance(,l,Type;); \\")
 *
 */






/* ==================  BLOOPS ===================== */


/* bloops
 * Generated using the previous emacs macros...
 *
 * typical usage:  Midx_begin_bloop2( al, a, bl, b ) {
 *                    \* DO IT HERE *\
 *                  } Midx_end_bloop2( al, a, bl, b );
 */




#define Midx_begin_bloop1( a1l, a1, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_loop() {

#define Midx_end_bloop1( a1l, a1, Type ) \
    Midxlow_advance(a1,a1l,Type); \
  } \
}

#define Midx_begin_bloop2( a1l ,a1, a2l ,a2, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_loop() {

#define Midx_end_bloop2( a1l, a1, a2l, a2, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
  } \
}

#define Midx_begin_bloop3( a1l, a1, a2l, a2, a3l, a3, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_loop() {

#define Midx_end_bloop3( a1l, a1, a2l, a2, a3l, a3, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
  } \
}

#define Midx_begin_bloop4( a1l, a1, a2l, a2, a3l, a3, a4l, a4, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_loop() {

#define Midx_end_bloop4( a1l, a1, a2l, a2, a3l, a3, a4l, a4, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
  } \
}

#define Midx_begin_bloop5( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_loop() {

#define Midx_end_bloop5( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
  } \
}

#define Midx_begin_bloop6( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_loop() {

#define Midx_end_bloop6( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
  } \
}

#define Midx_begin_bloop7( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_loop() {

#define Midx_end_bloop7( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
  } \
}

#define Midx_begin_bloop8( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_bclone(a8,a8l); \
  Midxlow_loop() {

#define Midx_end_bloop8( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
  } \
}

#define Midx_begin_bloop9( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_bclone(a8,a8l); \
  Midxlow_bclone(a9,a9l); \
  Midxlow_loop() {

#define Midx_end_bloop9( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
  } \
}

#define Midx_begin_bloop10( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_bclone(a8,a8l); \
  Midxlow_bclone(a9,a9l); \
  Midxlow_bclone(a10,a10l); \
  Midxlow_loop() {

#define Midx_end_bloop10( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
  } \
}

#define Midx_begin_bloop11( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_declare(a11,a11l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_bclone(a8,a8l); \
  Midxlow_bclone(a9,a9l); \
  Midxlow_bclone(a10,a10l); \
  Midxlow_bclone(a11,a11l); \
  Midxlow_loop() {

#define Midx_end_bloop11( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
    Midxlow_advance(a11,a11l,Type); \
  } \
}

#define Midx_begin_bloop12( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, a12l, a12, Type ) \
{ \
  Midxlow_binit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_declare(a11,a11l); \
  Midxlow_declare(a12,a12l); \
  Midxlow_bclone(a1,a1l); \
  Midxlow_bclone(a2,a2l); \
  Midxlow_bclone(a3,a3l); \
  Midxlow_bclone(a4,a4l); \
  Midxlow_bclone(a5,a5l); \
  Midxlow_bclone(a6,a6l); \
  Midxlow_bclone(a7,a7l); \
  Midxlow_bclone(a8,a8l); \
  Midxlow_bclone(a9,a9l); \
  Midxlow_bclone(a10,a10l); \
  Midxlow_bclone(a11,a11l); \
  Midxlow_bclone(a12,a12l); \
  Midxlow_loop() {

#define Midx_end_bloop12( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, a12l, a12, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
    Midxlow_advance(a11,a11l,Type); \
    Midxlow_advance(a12,a12l,Type); \
  } \
}


/* ==================  ELOOPS ===================== */

/* eloops
 * Generated using the previous emacs macros...
 *
 * typical usage:  Midx_begin_eloop2( al, a, bl,b ) {
 *                    \* DO IT HERE *\
 *                  } Midx_end_eloop2( al, a, bl,b );
 */


#define Midx_begin_eloop1( a1l, a1, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_loop() {

#define Midx_end_eloop1( a1l, a1, Type ) \
    Midxlow_advance(a1,a1l,Type); \
  } \
}

#define Midx_begin_eloop2( a1l, a1, a2l, a2, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_loop() {

#define Midx_end_eloop2( a1l, a1, a2l, a2, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
  } \
}

#define Midx_begin_eloop3( a1l, a1, a2l, a2, a3l, a3, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_loop() {

#define Midx_end_eloop3( a1l, a1, a2l, a2, a3l, a3, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
  } \
}

#define Midx_begin_eloop4( a1l, a1, a2l, a2, a3l, a3, a4l, a4, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_loop() {

#define Midx_end_eloop4( a1l, a1, a2l, a2, a3l, a3, a4l, a4, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
  } \
}

#define Midx_begin_eloop5( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_loop() {

#define Midx_end_eloop5( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
  } \
}

#define Midx_begin_eloop6( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_loop() {

#define Midx_end_eloop6( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
  } \
}

#define Midx_begin_eloop7( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_loop() {

#define Midx_end_eloop7( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
  } \
}

#define Midx_begin_eloop8( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_eclone(a8,a8l); \
  Midxlow_loop() {

#define Midx_end_eloop8( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
  } \
}

#define Midx_begin_eloop9( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_eclone(a8,a8l); \
  Midxlow_eclone(a9,a9l); \
  Midxlow_loop() {

#define Midx_end_eloop9( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
  } \
}

#define Midx_begin_eloop10( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_eclone(a8,a8l); \
  Midxlow_eclone(a9,a9l); \
  Midxlow_eclone(a10,a10l); \
  Midxlow_loop() {

#define Midx_end_eloop10( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
  } \
}

#define Midx_begin_eloop11( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_declare(a11,a11l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_eclone(a8,a8l); \
  Midxlow_eclone(a9,a9l); \
  Midxlow_eclone(a10,a10l); \
  Midxlow_eclone(a11,a11l); \
  Midxlow_loop() {

#define Midx_end_eloop11( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
    Midxlow_advance(a11,a11l,Type); \
  } \
}

#define Midx_begin_eloop12( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, a12l, a12, Type ) \
{ \
  Midxlow_einit(a1); \
  Midxlow_declare(a1,a1l); \
  Midxlow_declare(a2,a2l); \
  Midxlow_declare(a3,a3l); \
  Midxlow_declare(a4,a4l); \
  Midxlow_declare(a5,a5l); \
  Midxlow_declare(a6,a6l); \
  Midxlow_declare(a7,a7l); \
  Midxlow_declare(a8,a8l); \
  Midxlow_declare(a9,a9l); \
  Midxlow_declare(a10,a10l); \
  Midxlow_declare(a11,a11l); \
  Midxlow_declare(a12,a12l); \
  Midxlow_eclone(a1,a1l); \
  Midxlow_eclone(a2,a2l); \
  Midxlow_eclone(a3,a3l); \
  Midxlow_eclone(a4,a4l); \
  Midxlow_eclone(a5,a5l); \
  Midxlow_eclone(a6,a6l); \
  Midxlow_eclone(a7,a7l); \
  Midxlow_eclone(a8,a8l); \
  Midxlow_eclone(a9,a9l); \
  Midxlow_eclone(a10,a10l); \
  Midxlow_eclone(a11,a11l); \
  Midxlow_eclone(a12,a12l); \
  Midxlow_loop() {

#define Midx_end_eloop12( a1l, a1, a2l, a2, a3l, a3, a4l, a4, a5l, a5, a6l, a6, a7l, a7, a8l, a8, a9l, a9, a10l, a10, a11l, a11, a12l, a12, Type ) \
    Midxlow_advance(a1,a1l,Type); \
    Midxlow_advance(a2,a2l,Type); \
    Midxlow_advance(a3,a3l,Type); \
    Midxlow_advance(a4,a4l,Type); \
    Midxlow_advance(a5,a5l,Type); \
    Midxlow_advance(a6,a6l,Type); \
    Midxlow_advance(a7,a7l,Type); \
    Midxlow_advance(a8,a8l,Type); \
    Midxlow_advance(a9,a9l,Type); \
    Midxlow_advance(a10,a10l,Type); \
    Midxlow_advance(a11,a11l,Type); \
    Midxlow_advance(a12,a12l,Type); \
  } \
}


/* =============  End of IDXMAC_H =============== */
#endif 






