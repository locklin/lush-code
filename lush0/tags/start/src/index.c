/***********************************************************************
  LUSH:
  (LYB), (YLC) & (PS) 91
  ******************************************************************/

/******************************************************************************
 *
 *	INDEX.C.  Accessing storages as matrices...
 *
 ******************************************************************************/


/*
 * An index is a way to access a storage as a multi-dimensional object.
 * There are two kind of indexes:
 * 
 * 1) <struct index> are index on an arbitrary storage. They describe the
 *    starting offset, the dimensions, and their modulos.  
 * 
 *    The easiest way to access an index element consists in computing
 *    its offset, and using the <(*st->getat)> and <(*st->setat)> routines 
 *    of the related storage. This SLOW task is easily performed with the
 *    <easy_index_{check,set,get}()> routines.
 * 
 *    The second way consists in using the NR interface. Only 1D and 2D 
 *    indexes defined on a mallocated "F" storage are eligible for that 
 *    access method. The <make_nr{vector,matrix}()> functions return
 *    vectors or matrices compatible with the Numerical Recipes Library.
 * 
 *    The third way consists in creating a <struct idx> for reading or
 *    for writing, with the <index_{read,write}_idx()> functions. After
 *    having used these idxs, a call to <index_rls_idx()> function releases
 *    them. 
 * 
 * 2) <struct X_idx> are "light" indexes that always refer to a piece of 
 *    memory. They describe the type of the objects, the starting address,
 *    the dimensions and the modulos, as well as two vectors setf and getf
 *    for accessing the data independently of the type.
 * 
 * Finally, an index may be undimensionned. It is the responsibility
 * of index operation to dimension them according to their needs.
 * Undimensioned index have a negative <ndim> field, and the IDF_UNSIZED
 * flag set.
 *
 * Terminology:
 * A dimensionned index associated to a storage is called a "matrix."
 * A dimensionned index associated to an AT storage is called an "array."
 */





#include "header.h"

/* ------- THE ALLOC STRUCTURE ------ */

struct alloc_root index_alloc = {
  NULL,
  NULL,
  sizeof(struct index),
  100,
};


/* ------- THE CLASS FUNCTIONS ------ */

static void index_dispose(at *p)
{
  struct index *ind;
  if ((ind=p->Object)) 
  {
      UNLOCK(ind->atst);
      if (ind->flags&IDF_HAS_NR0)
          if (ind->nr0)
              free(ind->nr0);
      if (ind->flags&IDF_HAS_NR1)
          if (ind->nr1)
              free(ind->nr1);
      if (ind->cptr)
          lside_destroy_item(ind->cptr);
      deallocate( &index_alloc, (struct empty_alloc *) ind);
  }
}

static void index_action(at *p, void (*action)(at *))
{
  struct index *ind;
  if ((ind=p->Object))
    (*action)(ind->atst);
}

char *index_name(at *p)
{
  struct index *ind;
  char *s;
  int d;

  ind = p->Object;
  s = string_buffer;
  if (ind->flags & IDF_UNSIZED) {
    sprintf(s, "::%s%d:<unsized>", nameof(p->Class->classname), ind->ndim);
  } else {
    sprintf(s, "::%s%d:<", nameof(p->Class->classname), ind->ndim);
    while (*s)
      s++;
    for (d = 0; d < ind->ndim; d++) {
      sprintf(s, "%dx", ind->dim[d]);
      while (*s)
	s++;
    }
    *--s = 0;
    sprintf(s,">");
  }
  return string_buffer;
}


static at *index_listeval(at *p, at *q)
{
  register struct index *ind;
  register int i;
  register at *qsav;
  at *myp[MAXDIMS];

  static at *index_set(struct index*,at**,at*,int);
  static at *index_ref(struct index*,at**);

  ind = p->Object;

  if (ind->flags & IDF_UNSIZED)
    error(NIL,"unsized index",p);

  qsav = eval_a_list(q->Cdr);
  q = qsav;

  for (i = 0; i < ind->ndim; i++) {
    ifn(CONSP(q))
      error(NIL, "not enough dimensions", qsav);
    myp[i] = q->Car;
    q = q->Cdr;
  }
  if (!q) {
    q = index_ref(ind, myp);
    UNLOCK(qsav);
    return q;
  } else if (CONSP(q) && (!q->Cdr)) {
    index_set(ind, myp, q->Car, 1);
    UNLOCK(qsav);
    LOCK(p);
    return p;
  } else
    error(NIL, "too many dimensions", qsav);
}


int index_compare(at *p, at*q, int n) { return 0; }
unsigned long index_hash( at *p){ return 0; }
at *index_getslot(at *p, at *q){ return (at *)0; }
void index_setslot(at *p, at *q, at *r){ }

/* ------------- THE CLASS ------------- */

class index_class = {
  index_dispose,
  index_action,
  index_name,
  generic_eval,
  index_listeval,
  generic_serialize,   							     \
  index_compare,   							     \
  index_hash,   							     \
  index_getslot,   							     \
  index_setslot   							     \
};



/* --------  UTILITY FUNCTIONS ------- */

static void check_sized_index(at *p)
{
  struct index *ind;
  ifn (EXTERNP(p,&index_class))
    error(NIL,"not an index",p);
  ind = p->Object;
  if (ind->flags & IDF_UNSIZED)
    error(NIL,"unsized index",p);
}

/* -------- SIMPLE LISP FUNCTIONS ------- */

int indexp(at *p)
{
  if (EXTERNP(p,&index_class))
    return TRUE;
  return FALSE;
}

DX(xindexp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (indexp(APOINTER(1)))
    return true();
  else
    return NIL;
}

int matrixp(at *p)
{
  struct index *ind;
  if (EXTERNP(p,&index_class)) {
    ind = p->Object;
    ifn (ind->flags & IDF_UNSIZED)
      return TRUE;
  }
  return FALSE;
}
DX(xmatrixp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (matrixp(APOINTER(1)))
    return true();
  else
    return NIL;
}

int arrayp(at *p)
{
  struct index *ind;
  if (EXTERNP(p,&index_class)) {
    ind = p->Object;
    ifn (ind->flags & IDF_UNSIZED)
      if (ind->st->srg.type == ST_AT)
	return TRUE;
  }
  return FALSE;
}
DX(xarrayp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (arrayp(APOINTER(1)))
    return true();
  else
    return NIL;
}


DX(xindex_storage)
{
  at *p;
  struct index *ind;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  ind = AINDEX(1);
  p = ind->atst;
  LOCK(p);
  return p;
}

DX(xindex_nelements)
{
  struct index *ind;
  int size, i;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  ind = AINDEX(1);
  if (ind->flags & IDF_UNSIZED)
    return NEW_NUMBER(0);
  size = 1;
  for(i=0; i<ind->ndim; i++)
    size *= ind->dim[i];
  return NEW_NUMBER(size);
}

DX(xindex_size)
{
  struct index *ind;
  int size, i;
  extern int storage_type_size[];

  ARG_NUMBER(1);
  ARG_EVAL(1);
  ind = AINDEX(1);
  if (ind->flags & IDF_UNSIZED)
    return NEW_NUMBER(0);
  size = storage_type_size[ind->st->srg.type];
  for(i=0; i<ind->ndim; i++)
    size *= ind->dim[i];
  return NEW_NUMBER(size);
}

DX(xindex_ndim)
{
  struct index *ind;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  ind = AINDEX(1);
  if (ind->flags & IDF_UNSIZED)
    return NIL;
  else
    return NEW_NUMBER(ind->ndim);
}

DX(xindex_offset)
{
  struct index *ind;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  ind = AINDEX(1);  
  return NEW_NUMBER(ind->offset);
}

DX(xindex_bound)
{
  at *p;
  struct index *ind;
  int n;

  ALL_ARGS_EVAL;
  if (arg_number<1) 
    ARG_NUMBER(-1);
  ind = AINDEX(1);
  if (arg_number == 1) {
    p = NIL;
    for (n=ind->ndim-1;n>=0;n--)
      p = cons( NEW_NUMBER(ind->dim[n]-1), p);
    return p;
  } else {
    ARG_NUMBER(2);
    n = AINTEGER(2);
    if (n<0)
      n += ind->ndim;
    if (n<0 || n>= ind->ndim)
      error(NIL,"illegal dimension",APOINTER(2));
    return NEW_NUMBER(ind->dim[n]-1);
  }
}

DX(xindex_dim)
{
  at *p;
  struct index *ind;
  int n,q;

  ALL_ARGS_EVAL;
  if (arg_number<1) 
    ARG_NUMBER(-1);
  ind = AINDEX(1);
  if (arg_number == 1) {
    p = NIL;
    for (n=ind->ndim-1;n>=0;n--)
      p = cons( NEW_NUMBER(ind->dim[n]), p);
    return p;
  } else if (arg_number == 2) {
    n = AINTEGER(2);
    if (n<0)
      n += ind->ndim;
    if (n<0 || n>= ind->ndim)
      error(NIL,"illegal dimension",APOINTER(2));
    return NEW_NUMBER(ind->dim[n]);
  } else if (arg_number == 3) {
    n = AINTEGER(2);
    q = AINTEGER(3);
    if (n<0)
      n += ind->ndim;
    if (n<0 || n>= ind->ndim)
      error(NIL,"illegal dimension",APOINTER(2));
    if (ind->dim[n] != q)
      error(NIL,"illegal size",APOINTER(3));
    return NEW_NUMBER(q);
  } else {
    error(NIL,"function requires 1 to 3 arguments",NIL);
  }
}


DX(xindex_mod)
{
  at *p;
  struct index *ind;
  int n;

  ALL_ARGS_EVAL;
  if (arg_number<1) 
    ARG_NUMBER(-1);
  ind = AINDEX(1);
  if (arg_number == 1) {
    p = NIL;
    for (n=ind->ndim-1;n>=0;n--)
      p = cons( NEW_NUMBER(ind->mod[n]), p);
    return p;
  } else {
    ARG_NUMBER(2);
    n = AINTEGER(2);
    if (n<0)
      n += ind->ndim;
    if (n<0 || n>= ind->ndim)
      error(NIL,"illegal dimension",APOINTER(2));
    return NEW_NUMBER(ind->mod[n]);
  }
}



DX(xoldbound)
{
  int n;
  at *p;
  struct index *ind;

  ALL_ARGS_EVAL;

  if (arg_number==1) {

    p = APOINTER(1);
    check_sized_index(p);
    ind = p->Object;
    p = NIL;
    for (n=ind->ndim-1;n>=0;n--)
      p = cons( NEW_NUMBER(ind->dim[n]), p);
    return p;
    
  } else {

    ARG_NUMBER(2);
    p = APOINTER(1);
    check_sized_index(p);
    ind = p->Object;
    n = AINTEGER(2);

    if (n==0)
      return NEW_NUMBER(ind->ndim);
    if (n>=1 && n<=ind->ndim)
      return NEW_NUMBER(ind->dim[n-1]-1);
    error(NIL,"illegal dimension number",APOINTER(2));
  }
}



/* ========== CREATION FUNCTIONS ========== */


/*
 * This function creates an unsized index,
 */

at *new_index(at *atst)
{
  struct index *ind;
  at *atind;

  ifn (storagep(atst))
      error(NIL,"Not a storage",atst);
  LOCK(atst);
  ind = allocate(&index_alloc);
  ind->flags = IDF_UNSIZED;
  ind->atst = atst;
  ind->st = atst->Object;
  ind->ndim = -1;
  ind->offset = 0;
  ind->nr0 = NULL;
  ind->nr1 = NULL;
  ind->cptr = NULL;

  atind = new_extern(&index_class,ind);
  atind->flags |= X_INDEX;
  return atind;
}


/* 
 * This function sets the dimensions of an index
 * If the storage is unsized, size it.
 * If one dimension is zero, compute it according to the storage size.
 */

void index_dimension(at *p, int ndim, int dim[])
{
  struct index *ind;
  struct storage *st;
  at *atst;
  int i;
  int size;
  int unkn;

  ifn (indexp(p))
    error(NIL,"not an index",p);
  ind = p->Object;
  atst = ind->atst;
  st = ind->st;

  ifn (ind->flags & IDF_UNSIZED)
    error(NIL,"Unsized index required",p);
  if (ndim<0 || ndim>MAXDIMS)
    error(NIL,"Illegal number of dimensions",NEW_NUMBER(ndim));

  /* compute size */
  size = 1;
  unkn =-1;
  for (i=0; i<ndim; i++) {
    if (dim[i]>0)
      size *= dim[i];
    else if (dim[i]==0)
      if (unkn<0)
	unkn = i;
      else
	error(NIL,"more than one unspecified dimension",NIL);
    else
      error(NIL,"illegal dimension",NEW_NUMBER(dim[i]));
  }

  /* dimensions storage when needed */
  if (st->srg.flags & STF_UNSIZED) {
    if (unkn>=0)
      error(NIL,"Cannot compute storage size",atst);
    storage_malloc(atst,size,1);
  }

  /* check size, compute unspecified dim */
  if (size>st->srg.size)
    storage_realloc(atst,size,1);
  if (unkn>=0)
    dim[unkn] = st->srg.size / size;
  
  /* update index */
  size = 1;
  for(i=ndim-1;i>=0;i--) {
    ind->dim[i] = dim[i];
    ind->mod[i] = size;
    size *= dim[i];
  }
  ind->flags &= ~IDF_UNSIZED;
  ind->ndim = ndim;
}



/* 
 * This function "undimensions" an index
 */

void index_undimension(at *p)
{
  struct index *ind;

  ifn (indexp(p))
    error(NIL,"not an index",p);
  ind = p->Object;

  if (ind->flags & IDF_HAS_NR0)
    if (ind->nr0)
      free(ind->nr0);
  if (ind->flags & IDF_HAS_NR1)
    if (ind->nr1)
      free(ind->nr1);

  ind->flags = IDF_UNSIZED;
  /* ind->offset = 0; */
  ind->nr0 = NULL;
  ind->nr1 = NULL;
}



/*
 * This function sets the size of an index <newp> by making 
 * it a subindex of an existing sized index <basep>.
 * <dim> gives the dims of the subindex. A 0 dimension
 * shall be collapsed.  <start> gives the starting point
 */


void index_from_index(at *newp, at *basep, int *dim, int *start)
{
  struct index *ind,*oldind;  
  at *atst;
  struct storage *st;
  int i,ndim,off;


  ifn (indexp(newp))
    error(NIL,"not an index",newp);
  ifn (indexp(basep))
    error(NIL,"not an index",basep);

  ind = newp->Object;
  oldind = basep->Object;
  atst = ind->atst;
  st = ind->st;
  ndim = oldind->ndim;

  if (oldind->st != ind->st)
    error(NIL,"Subindex and base index have a different storage",newp);
  ifn (ind->flags & IDF_UNSIZED)
    error(NIL,"Unsized index required",newp);
  if (oldind->flags & IDF_UNSIZED)
    error(NIL,"Unsized base index",basep);
  
  /* check dimensions */
  for(i=0;i<ndim;i++) {
    if (dim[i]<0)
      error(NIL,"illegal dimension",NEW_NUMBER(dim[i]));
    if (start && start[i]<0)
      error(NIL,"illegal start coordinate",NEW_NUMBER(start[i]));
    if ( (start?dim[i]+start[i]:dim[i]) > oldind->dim[i] )
      error(NIL,"this index does not fit in the base index",newp);
  }
  
  /* compute start offset */
  off = oldind->offset;
  if (start)
    for (i=0;i<ndim;i++)
      off += start[i] * oldind->mod[i];

  /* update index */
  ind->offset = off;
  for (i=off=0; i<ndim; i++) {
    if (dim[i]>0) {
      ind->dim[off] = dim[i];
      ind->mod[off] = oldind->mod[i];
      off++;
    }
  }
  ind->ndim = off;
  ind->flags &= ~IDF_UNSIZED;
}



/* ------------- NEW_INDEX -------------- */


static int index_parse_dimlist(at *p, int *dim)
{
  at *l;
  int nd;
  l = p;
  nd=0;
  while(l) {
    ifn (CONSP(l) && NUMBERP(l->Car) )
      error(NIL,"illegal dimension list",p);
    if (nd >= MAXDIMS)
      error(NIL,"Too many dimensions",p);
    dim[nd++] = l->Car->Number;
    l = l->Cdr;
  }
  return nd;
}




DX(xnew_index)
{
  int dim[MAXDIMS];
  int start[MAXDIMS];
  at *p;
  at *ans;
  int nd;

  ALL_ARGS_EVAL;
  if (arg_number<1)
    ARG_NUMBER(-1);

  p = APOINTER(1);

  if (storagep(p)) {
      ans = new_index(p);
      ARG_NUMBER(2);
      nd = index_parse_dimlist(ALIST(2), dim);
      index_dimension(ans,nd,dim);
  }  else if (indexp(p)) {
      struct index *ind;

      ind = p->Object;
      ans = new_index(ind->atst);

      if (arg_number!=2 && arg_number!=3) {
	ARG_NUMBER(-1);
      }

      if (arg_number>=2) {
	nd = index_parse_dimlist(ALIST(2), dim);
	if (nd != ind->ndim)
	  error(NIL,"incorrect number of dimensions",APOINTER(2));
      }
      if (arg_number>=3) {
	nd = index_parse_dimlist(ALIST(3), start);
	if (nd > ind->ndim)
	  error(NIL,"incorrect number of dimensions",APOINTER(3));
	while (nd < ind->ndim)
	  start[nd++] = 0;
      }

      if (arg_number==2)
	index_from_index(ans,p,dim,NIL);
      else if (arg_number==3)
	index_from_index(ans,p,dim,start);
      else
	ARG_NUMBER(-1);
    }
  else
    error(NIL,"neither a storage, nor an index",p);
  return ans;
}

/* ------- DIRECT CREATION FUNCTIONS ------------ */




#define Generic_matrix(Prefix) 						     \
at *name2(Prefix,_matrix)(int ndim, int *dim)				     \
{ 									     \
  at *atst,*ans; 							     \
  atst = name3(new_,Prefix,_storage)();   				     \
  ans = new_index(atst);						     \
  index_dimension(ans,ndim,dim);					     \
  UNLOCK(atst); 							     \
  return ans; 								     \
} 									     \
DX(name3(x,Prefix,matrix)) 						     \
{ 									     \
  int i;								     \
  int dim[MAXDIMS]; 							     \
  ALL_ARGS_EVAL;							     \
  for(i=0;i<arg_number;i++) 						     \
    dim[i]=AINTEGER(i+1); 						     \
  return name2(Prefix,_matrix)(arg_number,dim); 			     \
}

Generic_matrix(AT);
Generic_matrix(P);
Generic_matrix(F); 
Generic_matrix(D);
Generic_matrix(I32);
Generic_matrix(I16);
Generic_matrix(I8);
Generic_matrix(U8);
Generic_matrix(GPTR);

#undef Generic_matrix




/* -------------- SUBINDEX ------------ */


DX(xsubindex)
{
  at *p, *q;
  struct index *ind;
  int dim[MAXDIMS];
  int start[MAXDIMS];
  int i;

  ALL_ARGS_EVAL;
  if (arg_number<1)
    ARG_NUMBER(-1);

  q = APOINTER(1);
  check_sized_index(q);
  ind = q->Object;

  if (arg_number!=ind->ndim+1)
    error(NIL,"wrong number of dimensions",NIL);

  for(i=0;i<ind->ndim; i++) {
    p = APOINTER(i+2);
    if (EXTERNP(p,&index_class)) {
      start[i] = p->Number;
      dim[i] = 0;
    } else if (CONSP(p) && CONSP(p->Cdr) && !p->Cdr->Cdr) {
      if (EXTERNP(p->Car,&index_class)) 
	start[i] = p->Car->Number;
      else
	error(NIL,"not a number",p->Car);
      if (EXTERNP(p->Cdr->Car,&index_class)) 
	dim[i] = 1 + p->Cdr->Car->Number - start[i];
      else
	error(NIL,"not a number",p->Cdr->Car);
    } else if (!p) {
      start[i] = 0;
      dim[i] = ind->dim[i];
    } else
      error(NIL,"illegal range specification",p);
  }
  
  p = new_index(ind->atst);
  index_from_index(p,q,dim,start);
  return p;
}




/* -------- THE REF/SET FUNCTIONS ------- */


/*
 * Loop to access the index elements
 */

static at *index_ref(struct index *ind, at *p[])
{
  at *q;
  int i, j;
  int start, end;
  at *myp[MAXDIMS];
  at *ans = NIL;
  register at **where = &ans;
  struct storage *st;

  /* 1: numeric arguments only */

  st = ind->st;
  j = ind->offset;
  for (i = 0; i < ind->ndim; i++) {
    register int k;
    q = p[i];
    if (q && (q->flags & C_NUMBER)) {
      k = (int) (q->Number);
      if (k < 0 || k >= ind->dim[i])
	error(NIL, "subscript out of range", q);
      j += k * ind->mod[i];
    } else
      goto list_ref;
  }
  return (*st->getat)(st,j);


  /* 2: found a non numeric argument */

list_ref:

  for (j = 0; j < ind->ndim; j++)
    myp[j] = p[j];

  ifn (q) {
    start = 0;
    end = ind->dim[i] - 1;
  } else if (LISTP(q) && CONSP(q->Cdr) && !q->Cdr->Cdr &&
	     q->Car && (q->Car->flags & C_NUMBER) &&
	     q->Cdr->Car && (q->Cdr->Car->flags & C_NUMBER)) {
    start = q->Car->Number;
    end = q->Cdr->Car->Number;
  } else
    error(NIL, "illegal subscript", NIL);
  
  myp[i] = NEW_NUMBER(0);
  for (j = start; j <= end; j++) {
    myp[i]->Number = j;
    *where = cons(index_ref(ind, myp), NIL);
    where = &((*where)->Cdr);
    CHECK_MACHINE("on");
  }
  UNLOCK(myp[i]);
  return ans;
}


/*
 * Loop to modify index elements.
 */

static at *index_set(struct index *ind, at *p[], at *value, int mode)
/* MODE 			    */
/* 0 is 'value is constant'	    */
/* 1 is 'get values from sublists'  */
/* 2 is 'get values in sequence'    */
{				
  at *q;
  int i, j;
  int start, end;
  struct storage *st;
  at *myp[MAXDIMS];


  /* 1: numeric arguments only */

  j = ind->offset;
  for (i = 0; i < ind->ndim; i++) {
    register int k;
    q = p[i];
    if (q && (q->flags & C_NUMBER)) {
      k = (int) (q->Number);
      if (k < 0 || k >= ind->dim[i])
	error(NIL, "indice out of range", q);
      j += k * ind->mod[i];
    } else
      goto list_set;
  }

  if (mode == 2) {
    q = value->Car;
    value = value->Cdr;
  } else {
    q = value;
    value = NIL;
  }

  st = ind->st;
  (*st->setat)(st,j,q);
  return value;

  
  /* 2 found a non numeric argument */
  
 list_set:
  
  for (j = 0; j < ind->ndim; j++)
    myp[j] = p[j];

  ifn (q) {
    start = 0;
    end = ind->dim[i] - 1;
  } else if (LISTP(q) && CONSP(q->Cdr) && !q->Cdr->Cdr &&
	     q->Car && (q->Car->flags & C_NUMBER) &&
	     q->Cdr->Car && (q->Cdr->Car->flags & C_NUMBER)) {
    start = q->Car->Number;
    end = q->Cdr->Car->Number;
  } else
    error(NIL, "illegal subscript", NIL);

  if (mode == 1)
    if (length(value) > end - start + 1)
      mode = 2;

  myp[i] = NEW_NUMBER(0);
  for (j = start; j <= end; j++) {
    myp[i]->Number = j;
    ifn(CONSP(value))
      mode = 0;
    switch (mode) {
    case 0:
      index_set(ind, myp, value, mode);
      break;
    case 1:
      index_set(ind, myp, value->Car, mode);
      value = value->Cdr;
      break;
    case 2:
      value = index_set(ind, myp, value, mode);
      break;
    }
    CHECK_MACHINE("on");
  }
  UNLOCK(myp[i]);
  return value;
}



/* ----------- THE EASY INDEX ACCESS ----------- */


static struct index *lastind = NULL;
static int lastcoord[MAXDIMS];
static int lastoffset;

/*
 * easy_index_check(p,ndim,dim)
 *
 * Check that index <p> has <ndim> dimensions.
 * If dim_p[i] contains a non zero value, <p> must
 * have the specified <i>th dimension. 
 * If dim_p[i] contains a zero value, the <i>th dimension
 *  of <p> is returned in dim_p[i].
 *
 * If <p> is an unsized index, and if dim_p[] specifies
 * all the dimensions, <p> is dimensioned.
 */

struct index *easy_index_check(at *p, int ndim, int dim[])
{
  int i;
  struct index *ind;

  if (!indexp(p))
    error(NIL,"not an index",p);
  ind = p->Object;

  if (ind->flags & IDF_UNSIZED) 
    {
      for(i=0;i<ndim;i++)
	if (dim[i]==0) {
	  sprintf(string_buffer,"unspecified dimension #%d",i);
	  error(NIL,string_buffer,p);
	}
      index_dimension(p,ndim,dim);
    }
  else
    {
      if (ndim != ind->ndim) {
	sprintf(string_buffer,"%dD index expected",ndim);
	error(NIL,string_buffer,p);
      }
      for (i=0;i<ndim;i++) {
	if (dim[i]) {
	  if (dim[i] != ind->dim[i]) {
	    sprintf(string_buffer,"illegal dimension #%d",i);
	    error(NIL,string_buffer,p);
	  }
	} else
	  dim[i] = ind->dim[i];
      }
    }
  lastind = NULL;
  return ind;
}


/*
 * easy_index_get(ind,coord)
 * easy_index_set(ind,coord,x)
 *
 *  Access data through an index.
 *  Assume that a CHECK (above) has been done before.
 *
 *  Note: The last access is cached,
 *        for speeding up the process.
 *        There is a penalty to random accesses!
 */

static int make_offset(struct index *ind, int coord[])
{
  int i,j,k,off,ndim;

  ndim = ind->ndim;
  if (ind==lastind) {
    off = lastoffset;
    for (i=0;i<ndim;i++) {
      k = lastcoord[i];
      j = coord[i];
      switch(j-k) {
      case 1:
	off+=ind->mod[i];
	break;
      case -1:
	off-=ind->mod[i];
	break;
      case 0:
	break;
      default:
	goto offset_byhand;
      }
      lastcoord[i] = j;
    }
    return lastoffset = off;
  }
 offset_byhand:
  off = ind->offset;
  for (i=0;i<ndim;i++) {
    k = lastcoord[i] = coord[i];
    off += k * ind->mod[i];
  }
  lastind = ind;
  lastoffset = off;
  return off;
}


real easy_index_get(struct index *ind, int *coord)
{
  struct storage *st;
  int offset;
  at *p;
  real x;
  
  st = ind->st;
  offset = make_offset(ind,coord);
  p = (*st->getat)(st,offset);	        /* Horrible & Slow! */
  ifn (EXTERNP(p,&index_class)) 
    error(NIL,"Not a number",p);
  x = p->Number;
  UNLOCK(p);
  return x;
}

void easy_index_set(struct index *ind, int *coord, real x)
{
  struct storage *st;
  int offset;
  at *p;
  
  p = NEW_NUMBER(x);
  st = ind->st;
  offset = make_offset(ind,coord);	/* Horrible & Slow! */
  (*st->setat)(st,offset,p);
  UNLOCK(p);
}








/*
 * index_read_idx()
 * index_write_idx()
 * index_rls_idx()
 *
 * Given the index, these function return a
 * idx structure pointing to the data area;
 */


void index_to_idx(struct index *ind, struct idx *idx)
{
  idx->ndim = ind->ndim;
  idx->dim = ind->dim;
  idx->mod = ind->mod;
  idx->offset = ind->offset;
  idx->flags = ind->flags;
  idx->srg = NULL;
}

void idx_to_index(struct idx *idx, struct index *ind)
{
    int i;
    
    ind->ndim = idx->ndim;
    for(i=0;i<idx->ndim;i++) {
      ind->mod[i] = idx->mod[i];
      ind->dim[i] = idx->dim[i];
    }
    ind->offset = idx->offset;
    ind->flags = idx->flags;
}

void index_read_idx(struct index *ind, struct idx *idx)
{
    /* A read idx CANNOT be unsized!  Don't forget that redimensioning
       occasionally cause a reallocation.  You certainly cannot do that 
       on a read only object.  Most (all?)  automatic redimensioning 
       function write in the matrix they redimension. */
    if (ind->flags & IDF_UNSIZED)
	error(NIL,"unsized index",NIL);
    index_to_idx(ind, idx);
    storage_read_srg(ind->st);
    idx->srg = &(ind->st->srg);
}

void index_write_idx(struct index *ind, struct idx *idx)
{
    index_to_idx(ind, idx);
    storage_write_srg(ind->st);
    idx->srg = &(ind->st->srg);
}

void index_rls_idx(struct index *ind, struct idx *idx)
{
    /* 
      if (ind->flags & IDF_UNSIZED)
      error(NIL,"unsized index",NIL); */
    
    idx_to_index(idx, ind);
    storage_rls_srg(ind->st);
}

/* -------- THE NR RECIPES INTERFACE ---------- */



char *not_a_nrvector(at *p)
{
  struct index *ind;
  struct storage *st;

  if (!indexp(p))
    return "Not an index";

  ind = p->Object;
  st = ind->st;

  if (ind->ndim != 1)
    return "Not a 1D index";
  if (st->srg.type != ST_F)
    return "Not an index on a flt storage";
  if (st->srg.flags & STF_RDONLY)
    return "Not an index on a writable storage";
  ifn (st->srg.flags & STS_MALLOC)
    return "Not an index on a memory based storage";
  if (ind->mod[0] != 1)
    return "This index is incompatible with the NR functions";
  return NULL;
}

char *not_a_nrmatrix(at *p)
{
  struct index *ind;
  struct storage *st;

  if (!indexp(p))
    return "Not an index";
  
  ind = p->Object;
  st = ind->st;
  
  if (ind->ndim != 2)
    return "Not a 2D index";
  if (ind->mod[1] != 1)
    return "Not an index on a flt storage";
  if (st->srg.flags & STF_RDONLY)
    return "Not an index on a writable storage";
  ifn (st->srg.flags & STS_MALLOC)
    return "Not an index on a memory based storage";
  if (st->srg.type != ST_F)
    return "This index is incompatible with the NR functions";
  return NULL;
}


DX(xnrvectorp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (not_a_nrvector(APOINTER(1)))
    return NIL;
  else
    return true();
}

DX(xnrmatrixp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (not_a_nrmatrix(APOINTER(1)))
    return NIL;
  else
    return true();
}



/* 
 * Returns a vector compatible with the Numerical Recipes
 * conventions, associated to the 1 dimensional index <p>.
 * Argument <nl> indicates the subscript of the firs element
 * of the vector. Most NR routines require a 1.
 * The size of the vector is returned via the pointer <size_p>
 */


flt *make_nrvector(at *p, int nl, int *size_p)
{
  char *s;
  flt *address;
  struct index *ind;
  
  if ((s = not_a_nrvector(p)))
    error(NIL,s,p);
  ind = p->Object;
  address = (flt*)(ind->st->srg.data) + ind->offset;  
  
  if (size_p)
    *size_p = ind->dim[0];
  return address - nl;
}

/*
 * Returns a matrix compatible with the Numerical Recipes
 * conventions, associated to the 2 dimensional index <p>.
 * Arguments <nrl> and <ncl> indicate the subscript of the 
 * first row and the first column of the matrix. Most NR
 * routines require them to be 1. 
 * The size of the matrix are returned via the pointers
 * <sizec_p> and <sizer_p>
 */

flt **make_nrmatrix(at *p, int ncl, int nrl, int *sizec_p, int *sizer_p)
{
  int i;
  char *s;
  struct index *ind;
  flt ***nr;
  flt *address;
  int nrflag;
  
  if ((s = not_a_nrmatrix(p)))
    error(NIL,s,p);
  ind = p->Object;

  if (nrl==0) {
    nr = &(ind->nr0);
    nrflag = IDF_HAS_NR0;
  } else if (nrl==1) {
    nr = &(ind->nr1);
    nrflag = IDF_HAS_NR1;
  } else
    error("index.c/make_nrmatrix","row base must be 0 or 1",NIL);
  
  if (sizec_p)
    *sizec_p = ind->dim[0];
  if (sizer_p)
    *sizer_p = ind->dim[1];
  
  ifn (ind->flags & nrflag) {
    if (! (ind->flags & nrflag)) {
      (*nr) = malloc( sizeof(flt**) * ind->dim[0] );
      ind->flags |= nrflag;
    }
    address = (flt*)(ind->st->srg.data) + ind->offset;
    for (i=0;i<ind->dim[0];i++) {
      (*nr)[i] = address - nrl;
      address += ind->mod[0];
    }
  }
  return (*nr) - ncl;
}


/* ------------ COPY MATRIX -------------------- */


void copy_matrix(struct index *i1, struct index *i2)
{
  struct idx idx1, idx2;
  
  {				
    int i, sz1, sz2;		
    /* Check Sizes */
    sz1 = sz2 = 1;
    for (i=0; i<i1->ndim; i++)
      sz1 *= i1->dim[i];
    for (i=0; i<i2->ndim; i++)
      sz2 *= i2->dim[i];
    if (sz1 != sz2)
      error(NIL,"matrices have different number of elements",NIL);
  }
  
  /* Access Data */
  index_read_idx(i1,&idx1);
  index_write_idx(i2,&idx2);
  
  if (i1->st->srg.type != i2->st->srg.type)
    goto default_copy;
    
  switch (i1->st->srg.type) {
    
#define GenericCopy(Prefix,Type) 					     \
  case name2(ST_,Prefix): 						     \
    { 									     \
      Type *d1, *d2; 							     \
      d1 = IDX_DATA_PTR(&idx1); 					             \
      d2 = IDX_DATA_PTR(&idx2); 					             \
      begin_idx_aloop2(&idx1,&idx2,p1,p2) { 				     \
        d2[p2] = d1[p1]; 						     \
      } end_idx_aloop2(&idx1,&idx2,p1,p2); 				     \
    } 									     \
    break;

    GenericCopy(P, unsigned char);
    GenericCopy(F, flt);
    GenericCopy(D, real);
    GenericCopy(I32, long);
    GenericCopy(I16, short);
    GenericCopy(I8, char);
    GenericCopy(U8, unsigned char);

#undef GenericCopy

  case ST_AT:
    {
      at **d1, **d2, *r;
      d1 = IDX_DATA_PTR(&idx1); 					             
      d2 = IDX_DATA_PTR(&idx2); 					             
      begin_idx_aloop2(&idx1,&idx2,p1,p2) {
	r = d2[p2];
	LOCK(d1[p1]);
	d2[p2] = d1[p1];
	UNLOCK(r);
      } end_idx_aloop2(&idx1,&idx2,p1,p2);
    }
    break;

  case ST_GPTR:
    {
      gptr *d1, *d2;
      d1 = (gptr *)IDX_DATA_PTR(&idx1); 					             
      d2 = (gptr *)IDX_DATA_PTR(&idx2); 					             
      begin_idx_aloop2(&idx1,&idx2,p1,p2) {
	((gptr *)d2)[p2] = ((gptr *)d1)[p1];
      } end_idx_aloop2(&idx1,&idx2,p1,p2);
    }
    break;

  default:
  default_copy:
    {
      flt (*getf)(gptr,int) = storage_type_getf[idx1.srg->type];
      void (*setf)(gptr,int,flt) = storage_type_setf[idx2.srg->type];
      begin_idx_aloop2(i1,i2,p1,p2) {
	(*setf)(IDX_DATA_PTR(&idx2), p2, (*getf)(IDX_DATA_PTR(&idx1), p1) );
      } end_idx_aloop2(i1,i2,p1,p2);
    }
    break;
  }
  index_rls_idx(i1,&idx1);
  index_rls_idx(i2,&idx2);
}



at *create_samesize_matrix(at *p1)
{
  at *p2, *atst;
  struct index *i1;
  int i,size;
  
  ifn( indexp(p1) )
    error(NIL,"no an index",p1);
  i1 = p1->Object;
  if (i1->flags & IDF_UNSIZED)
    error(NIL,"unsized index",p1);
  size = 1;
  for (i=0; i<i1->ndim; i++)
    size *= i1->dim[i];
  atst = new_storage_nc(i1->st->srg.type, size);
  p2 = new_index(atst);
  index_dimension(p2, i1->ndim, i1->dim);
  UNLOCK(atst);
  return p2;
}


DX(xcopy_matrix)
{
  at *p1, *p2;
  struct index *i1, *i2;

  ALL_ARGS_EVAL;
  if (arg_number==1) {
    p1 = APOINTER(1);
    p2 = create_samesize_matrix(p1);
  } else {
    ARG_NUMBER(2);
    p1 = APOINTER(1);
    p2 = APOINTER(2);
    LOCK(p2);
  }

  ifn( indexp(p1) )
    error(NIL,"no an index",p1);
  ifn( indexp(p2) )
    error(NIL,"no an index",p2);

  i1 = p1->Object;
  i2 = p2->Object;
  if (i2->flags & IDF_UNSIZED)
    index_dimension(p2,i1->ndim,i1->dim);

  copy_matrix(i1,i2);
  return p2;
}


/* --------------- SAVE MATRIX -------------- */



/* SN2 compatible read function.
   The magic number tells the type of the file,
   and reveals if the integers require flipping!  */

#define MAGIC_FLT_MAT    ((int)0x1e3d4c51)
#define MAGIC_PACKED_MAT ((int)0x1e3d4c52)
#define MAGIC_ASCII_MAT  ((int)0x2e4d4154) /* ".MAT" */

#define FLIP(n) ((((n)&0xff000000)>>24)|(((n)&0x00ff0000)>> 8)|\
		 (((n)&0x0000ff00)<< 8)|(((n)&0x000000ff)<<24) )


/* Check that datatypes are ``standard'' */

static void compatible_p(void)
{
  static int warned = 0;
  if (warned)
      return;
  warned = 1;
  if (sizeof(int)!=4) 
  {
      print_string(" ** Warning:\n");
      print_string(" ** Integers are not four bytes wide on this cpu\n");
      print_string(" ** Matrix files may be non portable!\n");
  }
  else 
  {
      union { int i; flt b; } u;
      u.b = 2.125;
      if (sizeof(flt)!=4 || u.i!=0x40080000) 
      {
          print_string(" ** Warning:\n");
          print_string(" ** Flt are not IEEE single precision numbers\n");
          print_string(" ** Matrix files may be non portable!\n");
      }
  }
}


/* Integer R/W functions */

static int read_i(File *f, int flip_p)
{
  int x;
  if (PFREAD(&x, sizeof(int), 1, f) != 1) {
    TEST_FILE_ERROR(f);
    error(NIL,"Cannot read header: EOF found",NIL);
  }
  if (flip_p && sizeof(int)==4)
    x = FLIP(x);
  return x;
}

static void write_i(File *f, int x)
{
  if (PFWRITE(&x, sizeof(int), 1, f) != 1) {
    TEST_FILE_ERROR(f);
    error(NIL,"Cannot write header: EOF found(?)",NIL);
  }
}

void save_flt_matrix(at *atp, at *atf)
{
  File *f;
  int off;
  struct index *ind;
  struct idx idx;
  flt (*getf)(gptr,int);
  flt x;
  
  compatible_p();

  check_sized_index(atp);
  ifn (EXTERNP(atf,&file_W_class)) 
    error(NIL,"not a write file descriptor",atf);

  ind = atp->Object;
  f = atf->Object;

  write_i(f, MAGIC_FLT_MAT);
  write_i(f, ind->ndim);
  for (off = 0; off < ind->ndim; off++)
    write_i(f, ind->dim[off]);
  while (off++ < 3)
    write_i(f, 1);
  
  index_read_idx(ind,&idx);
  getf = storage_type_getf[idx.srg->type];
  begin_idx_aloop1(&idx,off) 
    {
      x = (*getf)(IDX_DATA_PTR(&idx),off);
      PFWRITE(&x, sizeof(flt), 1, f);
    } 
  end_idx_aloop1(&idx,off);
  index_rls_idx(ind,&idx);

  FFLUSH(f);
  TEST_FILE_ERROR(f);
}


void save_packed_matrix(at *atp, at *atf)
{
  File *f;
  int off;
  struct index *ind;
  struct idx idx;
  flt (*getf)(gptr,int);
  flt x;

  compatible_p();

  check_sized_index(atp);
  ifn (EXTERNP(atf,&file_W_class)) 
    error(NIL,"not a write file descriptor",atf);
  
  ind = atp->Object;
  f = atf->Object;
  
  write_i(f, MAGIC_PACKED_MAT);
  write_i(f, ind->ndim);
  for (off = 0; off < ind->ndim; off++)
    write_i(f, ind->dim[off]);
  while (off++ < 3)
    write_i(f, 1);
  
  index_read_idx(ind,&idx);
  getf = storage_type_getf[idx.srg->type];
  begin_idx_aloop1(&idx,off) 
    {
      char b;
      x = (*getf)(IDX_DATA_PTR(&idx),off);
      if (x > 8.0-1.0/16.0)
	b = 127;
      else if (x < -8.0)
	b = -128;
      else
	b = x*16.0;
      PFWRITE(&b, sizeof(char), 1, f);
    } 
  end_idx_aloop1(&idx,off);
  index_rls_idx(ind,&idx);
  FFLUSH(f);
  TEST_FILE_ERROR(f);
}


void save_ascii_matrix(at *atp, at *atf)
{
  File *f;
  int off;
  struct index *ind;
  struct idx idx;
  flt (*getf)(gptr,int);
  flt x;

  check_sized_index(atp);
  ifn (EXTERNP(atf,&file_W_class)) 
    error(NIL,"not a write file descriptor",atf);

  ind = atp->Object;
  f = atf->Object;
  
  fprintf(f,".MAT %d", ind->ndim); 
  for (off=0;off<ind->ndim;off++)
     fprintf(f, " %d", ind->dim[off]);
  fprintf(f,"\n");
  index_read_idx(ind,&idx);
  getf = storage_type_getf[idx.srg->type];
  begin_idx_aloop1(&idx,off) 
  {
      x = (*getf)(IDX_DATA_PTR(&idx),off);
      fprintf(f," %g\n", x);
  } 
  end_idx_aloop1(&idx,off);
  index_rls_idx(ind,&idx);
  FFLUSH(f);
  TEST_FILE_ERROR(f);
}



DX(xsave_flt_matrix)
{
  at *p, *ans;

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), "SNMX@mat");
    ans = new_string(file_name);
    
  } /* else if ((p=APOINTER(2)) && p->ctype == XT_U8STORAGE) {
    p = new_extern(&file_W_class,pseudo_fopen(p, "w"),XT_FILE_WO);
    } */ 
  else if (EXTERNP(p,&file_W_class)) {
    LOCK(p);
    LOCK(p);
    ans = p;
  } else {
    error(NIL, "neither a string nor a U8 storage nor a write file descriptor", p);
  }
  save_flt_matrix(APOINTER(1), p);
  UNLOCK(p);
  return ans;
}

DX(xsave_packed_matrix)
{
  at *p, *ans;

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), "SNMX@mat");
    ans = new_string(file_name);
  } 
  /* else if ((p=APOINTER(2)) && p->ctype == XT_U8STORAGE) {
    p = new_extern(&file_W_class,pseudo_fopen(p, "w"),XT_FILE_WO);
    } */
  else if (EXTERNP(p,&file_W_class)) {
    LOCK(p);
    LOCK(p);
    ans = p;
  } else {
    error(NIL, "neither a string nor a write file descriptor", p);
  }
  save_packed_matrix(APOINTER(1), p);
  UNLOCK(p);
  return ans;
}

DX(xsave_ascii_matrix)
{
  at *p, *ans;

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  if (ISSTRING(2)) {
    p = OPEN_WRITE(ASTRING(2), "mat");
    ans = new_string(file_name);
  } 
  /* else if ((p=APOINTER(2)) && p->ctype == XT_U8STORAGE) {
    p = new_extern(&file_W_class,pseudo_fopen(p, "w"),XT_FILE_WO);
    } */
  else if (EXTERNP(p,&file_W_class)) {
    LOCK(p);
    LOCK(p);
    ans = p;
  } else {
    error(NIL, "neither a string nor a write file descriptor", p);
  }
  save_ascii_matrix(APOINTER(1), p);
  UNLOCK(p);
  return ans;
}


/* --------------- LOAD MATRIX -------------- */






static int load_header(File *f, int *size_p, int *ndim_p, int dim[])
{
  int i;
  int ndim;
  int magic;
  int size;
  int flip_p;
  
  compatible_p();

  magic = read_i(f,0);
  flip_p = 0;
  
  switch (magic) {
    
  case FLIP(MAGIC_FLT_MAT):
  case FLIP(MAGIC_PACKED_MAT):
    
    flip_p = 1;
    
  case MAGIC_FLT_MAT:
  case MAGIC_PACKED_MAT:
    
    ndim = read_i(f, flip_p);
    if (ndim<0 || ndim>MAXDIMS)
      error(NIL,"Corrupted file: Bad number of dimensions", NEW_NUMBER(ndim));
    for (i=0; i<ndim; i++)
      dim[i] = read_i(f, flip_p);
    while (i++<3)
      read_i(f, flip_p);
    break;
    
  case MAGIC_ASCII_MAT:
  case FLIP(MAGIC_ASCII_MAT):
    
    FSCANF1(f," %d",&ndim);
    TEST_FILE_ERROR(f);
    if (ndim<0 || ndim>MAXDIMS)
      error(NIL,"Corrupted file: Invalid ndim",NEW_NUMBER(ndim));
    for (i=0; i<ndim; i++)
      FSCANF1(f, " %d", &(dim[i]));
    break;
    
  default:
    TEST_FILE_ERROR(f);
    error(NIL,"Not a matrix file",NIL);
  }

  *ndim_p = ndim;
  size = 1;
  for (i=0; i<ndim; i++)
    size *= dim[i];
  *size_p = size;
  
  return magic;
}



at *load_matrix(at *atf)
{
  at *atp;
  at *atst;
  File *f;
  struct storage *st;
  int i, magic, ndim, size, dim[MAXDIMS];
  
  ifn (EXTERNP(atf,&file_R_class)) 
    error(NIL,"not a read file descriptor",atf);
  f = atf->Object;
  
  magic = load_header(f,&size,&ndim,dim);
  
  switch (magic) {
      
  case MAGIC_FLT_MAT:
  case FLIP(MAGIC_FLT_MAT):

      atst = new_F_storage();
      storage_malloc(atst,size,0);
      storage_load(atst,atf);
      /* Flip data when magic number is flipped */
      if (magic!=MAGIC_FLT_MAT && 
          sizeof(flt)==sizeof(int) &&
          sizeof(int)==4 )
      {
          int *data;
          st = atst->Object;
          data = st->srg.data;
          for (i=0; i<size; i++, data++)
              *data = FLIP(*data);
      }
      break;
    
  case MAGIC_PACKED_MAT:
  case FLIP(MAGIC_PACKED_MAT):

    atst = new_P_storage();
    storage_malloc(atst,size,0);
    storage_load(atst,atf);
    break;
    
  case MAGIC_ASCII_MAT:
  case FLIP(MAGIC_ASCII_MAT):

    atst = new_F_storage();
    st = atst->Object;
    storage_malloc(atst,size,0);
    for (i=0; i<size; i++) {
      float x;
      flt *data = st->srg.data;
      if (FSCANF1(f," %hf",&x) != 1)
	  error(NIL,"Garbage in ascii matrix file",NIL);
      data[i] = (flt)x;
    }
    TEST_FILE_ERROR(f);
    break;

  default:
    error("index.c/load_matrix","Unknown matrix file type",atf);
  }

  atp = new_index(atst);
  index_dimension(atp,ndim,dim);
  UNLOCK(atst);
  return atp;
}



DX(xload_matrix)
{
  at *p, *ans;

  if (arg_number == 2) {
    ARG_EVAL(2);
    ASYMBOL(1);
    p = APOINTER(2);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
  }
  
  if (EXTERNP(p,&string_class)) {
    p = OPEN_READ(SADD(p->Object),"SNMX@mat");
  } 
  /* else if (p && p->ctype == XT_U8STORAGE) {
    p = new_extern(&file_R_class,pseudo_fopen(p, "r"),XT_FILE_RO);
    } */
  else if (EXTERNP(p,&file_R_class)) {
    LOCK(p);
  } else {
    error(NIL, "neither a string nor a read file descriptor", p);
  }
  
  ans = load_matrix(p);
  
  if (arg_number==2)
    var_set(APOINTER(1), ans);
  UNLOCK(p);
  return ans;
}






/* ------ INDEX STRUCTURE OPERATIONS ------ */



/* (re)set the dimensions of an index */

DX(xindex_redim)
{
  at *p;
  struct index *ind;
  int dim[MAXDIMS];
  int nd;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  p = APOINTER(1);
  ind = AINDEX(1);
  nd = index_parse_dimlist( ALIST(2), dim);
  ifn (ind->flags & IDF_UNSIZED)
    index_undimension(p);
  index_dimension(p,nd,dim);

  return NIL;
}


/* make an index undimensionned */

DX(xindex_undim)
{
  at *p;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  p = APOINTER(1);
  index_undimension(p);

  return NIL;
}


/* xindex_unfold prepares a matrix for a convolution */

DX(xindex_unfold)
{
  at *p;
  struct index *ind;
  int d, sz, st, s, n;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(4);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);
  sz = AINTEGER(3);
  st = AINTEGER(4);
  
  if (d<0 || d>=ind->ndim || sz<1 || st<1)
    error(NIL,"illegal parameters",NIL);

  s = 1+ (ind->dim[d]-sz)/st;
  if (s<=0 || ind->dim[d]!=st*(s-1)+sz)
    error(NIL,"Index dimension does not match size and step", 
	  NEW_NUMBER(ind->dim[d]));

  n = ind->ndim;
  if (n+1 >= MAXDIMS)
    error(NIL,"Too many number of dimensions",NEW_NUMBER(MAXDIMS));
  
  ind->ndim = n+1;
  ind->dim[n] = sz;
  ind->mod[n] = ind->mod[d];
  ind->dim[d] = s;
  ind->mod[d] *= st;

  return NIL;
}


/* xindex_diagonal grabs the diagonal of the last d dimensions */

DX(xindex_diagonal)
{
  at *p;
  struct index *ind;
  int d,i,m,n;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);

  if (d<2 || d>ind->ndim)
    error(NIL,"illegal parameters",NIL);
  m = ind->ndim - d;
  n = ind->dim[m];
  for (i=1;i<d;i++)
    if (ind->dim[m+i] != n)
      error(NIL,"The last dimensions should have the same size",p);
  n = ind->mod[m];
  for (i=1;i<d;i++)
    n += ind->mod[m+i];
  ind->ndim = m+1;
  ind->mod[m] = n;

  return NIL;
}


/* xindex_narrow restrict a dimension */

DX(xindex_narrow)
{
  at *p;
  struct index *ind;
  int d, sz, st;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(4);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);
  sz = AINTEGER(3);
  st = AINTEGER(4);
  
  if (d<0 || d>=ind->ndim || sz<1 || st<0)
    error(NIL,"illegal parameters",NIL);
  if (st+sz > ind->dim[d])
    error(NIL,"specified interval is too large",NIL);

  ind->dim[d] = sz;
  ind->offset += st * ind->mod[d];

  return NIL;
}


/* xindex_select */

DX(xindex_select)
{
  at *p;
  struct index *ind;
  int d, x;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);
  x = AINTEGER(3);
  
  if (d<0 || d>=ind->ndim || x<0 )
    error(NIL,"illegal parameters",NIL);
  if (x >= ind->dim[d])
    error(NIL,"specified subscript is too large",NEW_NUMBER(x));

  ind->offset += x * ind->mod[d];
  ind->ndim -= 1;
  while (d<ind->ndim) {
    ind->dim[d] = ind->dim[d+1];
    ind->mod[d] = ind->mod[d+1];
    d++;
  }

  return NIL;
}


/* xindex_transpose */

DX(xindex_transpose)
{
  at *p;
  struct index *ind;
  int i, nd;
  int table[MAXDIMS];
  int tmp[MAXDIMS];

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  
  nd = index_parse_dimlist( ALIST(2), table);
  if (nd!=ind->ndim)
    error(NIL,"Permutation list is too short or too long",APOINTER(2));
  for (i=0; i<nd; i++)
    tmp[i]=0;
  for (i=0; i<nd; i++)
    if (table[i]<0 || table[i]>=nd || tmp[table[i]])
      error(NIL,"Bad permutation list",APOINTER(2));
    else
      tmp[table[i]] = 1;
  
  for (i=0;i<nd;i++)
    tmp[i] = ind->dim[i];
  for (i=0;i<nd;i++)
    ind->dim[i] = tmp[table[i]];
  for (i=0;i<nd;i++)
    tmp[i] = ind->mod[i];
  for (i=0;i<nd;i++)
    ind->mod[i] = tmp[table[i]];

  return NIL;
}


/* xindex_transpose2 */

DX(xindex_transpose2)
{
  at *p;
  struct index *ind;
  int d1, d2, tmp;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d1 = AINTEGER(2);
  d2 = AINTEGER(3);

  if (d1<0 || d2<0 || d1>=ind->ndim || d2>=ind->ndim || d1==d2)
    error(NIL,"Illegal arguments",NIL);
  tmp = ind->dim[d1];
  ind->dim[d1] = ind->dim[d2];
  ind->dim[d2] = tmp;
  tmp = ind->mod[d1];
  ind->mod[d1] = ind->mod[d2];
  ind->mod[d2] = tmp;

  return NIL;
}


/* xindex_clone */

at *index_clone(at *p)
{
  at *q;
  struct index *ind1, *ind2;
  int i, n;
  check_sized_index(p);
  ind1 = p->Object;
  q = new_index(ind1->atst);
  ind2 = q->Object;
  n = ind1->ndim;
  for (i=0; i<n; i++) {
    ind2->dim[i] = ind1->dim[i];
    ind2->mod[i] = ind1->mod[i];
  }
  ind2->offset = ind1->offset;
  ind2->ndim = n;
  ind2->flags &= ~IDF_UNSIZED;
  return q;
}

DX(xindex_clone)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  return (index_clone(APOINTER(1)));
}


/* xindex_transclone */

DX(xindex_transclone)
{
  at *p, *q;
  struct index *ind1, *ind2;
  int i, j, n;
  int table[MAXDIMS];
  int tmp[MAXDIMS];

  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  p = APOINTER(1);
  check_sized_index(p);
  ind1 = p->Object;

  n = index_parse_dimlist( ALIST(2), table);
  if (n!=ind1->ndim)
    error(NIL,"Permutation list is too short or too long",APOINTER(2));
  for (i=0; i<n; i++)
    tmp[i]=0;
  for (i=0; i<n; i++)
    if (table[i]<0 || table[i]>=n || tmp[table[i]])
      error(NIL,"Bad permutation list",APOINTER(2));
    else
      tmp[table[i]] = 1;

  q = new_index(ind1->atst);
  ind2 = q->Object;
  
  for (i=0; i<n; i++) {
    j = table[i];
    ind2->dim[i] = ind1->dim[j];
    ind2->mod[i] = ind1->mod[j];
  }
  ind2->offset = ind1->offset;
  ind2->ndim = n;
  ind2->flags &= ~IDF_UNSIZED;
  
  return q;
}




/* patch the dim, mod and offset members of an index */

static int index_check_size(struct index *ind)
{
  int j, size_min, size_max;
  size_min = ind->offset;
  size_max = ind->offset;
  if (size_min<0)
    return TRUE;
  for( j=0; j<ind->ndim; j++)
      if(ind->mod[j]<0)
	  size_min += (ind->dim[j] - 1) * ind->mod[j];
      else
	  size_max += (ind->dim[j] - 1) * ind->mod[j];
  if (size_min >= 0 && size_max < ind->st->srg.size)
    return FALSE;
  return TRUE;
}

DX(xindex_change_dim)
{
  at *p;
  struct index *ind;
  int d, x, oldx;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);
  x = AINTEGER(3);

  if  (d<0 || d>=ind->ndim || x<=0)
    error(NIL,"illegal parameters",NIL);
  oldx = ind->dim[d];
  ind->dim[d] = x;
  if (index_check_size(ind)) {
    ind->dim[d] = oldx;
    error(NIL,"Index is larger than its storage",NIL);
  }

  return NIL;
}

DX(xindex_change_mod)
{
  at *p;
  struct index *ind;
  int d, x, oldx;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  d = AINTEGER(2);
  x = AINTEGER(3);

  if  (d<0 || d>=ind->ndim)
    error(NIL,"illegal parameters",NIL);
  oldx = ind->mod[d];
  ind->mod[d] = x;
  if (index_check_size(ind)) {
    ind->mod[d] = oldx;
    error(NIL,"Index is larger than its storage",NIL);
  }
  
  return NIL;
}

DX(xindex_change_offset)
{
  at *p;
  struct index *ind;
  int x, old_val;
  
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  p = APOINTER(1);
  check_sized_index(p);
  ind = p->Object;
  x = AINTEGER(2);

  old_val = ind->offset;
  ind->offset = x;
  if (index_check_size(ind)) {
    ind->offset = old_val;
    error(NIL,"Index is larger than its storage",NIL);
  }

  return NIL;
}



/* ----------------- THE LOOPS ---------------- */

#define MAXEBLOOP 8


static int ebloop_args(at *p, at **syms, at **iats, struct index **inds, at **last_index)
{
  at *q, *r, *s;
  int n;

  ifn ( CONSP(p) && CONSP(p->Car) )
    error(NIL, "syntax error", NIL);
  
  n = 0;
  r = NIL;
  for (q = p->Car; CONSP(q); q = q->Cdr) {
    p = q->Car;
    ifn(CONSP(p) && CONSP(p->Cdr) && !p->Cdr->Cdr)
      error(NIL, "syntax error in variable declaration", p);
    ifn((s = p->Car) && (EXTERNP(s,&symbol_class)))
      error(NIL, "symbol expected", s);
    UNLOCK(r);
    r = eval(p->Cdr->Car);
    
    if (n >= MAXEBLOOP)
      error(NIL,"Looping on too many indexes",NIL);
    syms[n] = s;
    iats[n] = s = index_clone(r);
    inds[n] = s->Object;
    n++;
  }
  *last_index = r;
  if (q)
    error(NIL, "syntax error in variable declaration", q);
  return n;
}


DY(yeloop)
{
  at *ans, *last_index;
  register int i,j,d, n;
  struct index *ind;
  
  at *syms[MAXEBLOOP];
  at *iats[MAXEBLOOP];
  struct index *inds[MAXEBLOOP];
  
  n = ebloop_args(ARG_LIST, syms, iats, inds, &last_index);

  for (i=0; i<n; i++) {
    ind = inds[i];

    j = --(ind->ndim);		/* remove last dimension */

    if (i==0)			/* test equality of looping dimensions */
      d = ind->dim[j];
    else if (d!=ind->dim[j])
      error(NIL,"looping dimension are different",ARG_LIST->Car);

    symbol_push(syms[i], iats[i]);
  }

  /* loop */
  for (j=0; j<d; j++) {
    ans = progn(ARG_LIST->Cdr);
    for (i=0; i<n; i++)
      if (EXTERNP(iats[i],&index_class)) {
	/* check not deleted! */
	ind = inds[i];
	ind->offset += ind->mod[ ind->ndim ];
      }
    UNLOCK(ans);
  }
  for (i=0; i<n; i++) {
    symbol_pop(syms[i]);
    UNLOCK(iats[i]);
  }
  return last_index;
}



DY(ybloop)
{
  at *ans, *last_index;
  register int i,j,d,n,m;
  struct index *ind;
  
  at *syms[MAXEBLOOP];
  at *iats[MAXEBLOOP];
  struct index *inds[MAXEBLOOP];
  
  n = ebloop_args(ARG_LIST, syms, iats, inds, &last_index);

  for (i=0; i<n; i++) {
    ind = inds[i];

    if (i==0)			/* test equality of looping dimensions */
      d = ind->dim[0];
    else if (d!=ind->dim[0])
      error(NIL,"looping dimension are different",ARG_LIST->Car);

    --(ind->ndim);		/* remove one dimension */
    m = ind->mod[0];		/* reorganize the dim and mod arrays */
    for (j=0; j<ind->ndim; j++) {
      ind->dim[j] = ind->dim[j+1];
      ind->mod[j] = ind->mod[j+1];
    }
    ind->mod[ind->ndim] = m;	/* put the stride at the end! */

    symbol_push(syms[i], iats[i]);
  }

  ans = NIL;			/* loop */
  for (j=0; j<d; j++) {
    ans = progn(ARG_LIST->Cdr);
    for (i=0; i<n; i++)
      if (EXTERNP(iats[i],&index_class)) {
	/* check not deleted! */
	ind = inds[i];
	ind->offset += ind->mod[ind->ndim];
      }
    UNLOCK(ans);
  }
  for (i=0; i<n; i++) {
    symbol_pop(syms[i]);
    UNLOCK(iats[i]);
  }
  return last_index;
}










/* ------------ THE INITIALIZATION ------------ */


void init_index()
{
  class_define("INDEX",&index_class);

  /* info */

  dx_define("indexp", xindexp);
  dx_define("matrixp", xmatrixp);
  dx_define("arrayp", xarrayp);

  dx_define("idx-storage", xindex_storage);
  dx_define("idx-size", xindex_size);
  dx_define("idx-nelements", xindex_nelements);
  dx_define("idx-ndim", xindex_ndim);
  dx_define("idx-offset", xindex_offset);
  dx_define("idx-bound", xindex_bound);
  dx_define("idx-dim", xindex_dim);
  dx_define("idx-modulo", xindex_mod);

  /* creation */

  dx_define("new_index", xnew_index);
  dx_define("sub_index", xsubindex);
  
  dx_define("at_matrix", xATmatrix);
  dx_define("p_matrix", xPmatrix);
  dx_define("f_matrix", xFmatrix);
  dx_define("d_matrix", xDmatrix);
  dx_define("i32_matrix", xI32matrix);
  dx_define("i16_matrix", xI16matrix);
  dx_define("i8_matrix", xI8matrix);
  dx_define("u8_matrix", xU8matrix);
  dx_define("gptr_matrix", xGPTRmatrix);

  /* nr */
  
  dx_define("nrvectorp", xnrvectorp);
  dx_define("nrmatrixp", xnrmatrixp);

  /* copy */

  dx_define("copy_matrix", xcopy_matrix);

  /* files */

  dx_define("save_flt_matrix", xsave_flt_matrix);
  dx_define("save_packed_matrix", xsave_packed_matrix);
  dx_define("save_ascii_matrix", xsave_ascii_matrix);
  dx_define("load_matrix", xload_matrix);

  /* structure functions */

  dx_define("idx-redim", xindex_redim);
  dx_define("idx-undim", xindex_undim);
  dx_define("idx-unfold", xindex_unfold);
  dx_define("idx-diagonal", xindex_diagonal);
  dx_define("idx-narrow", xindex_narrow);
  dx_define("idx-select", xindex_select);
  dx_define("idx-transpose", xindex_transpose);
  dx_define("idx-transpose2", xindex_transpose2);
  
  dx_define("idx-clone", xindex_clone);
  dx_define("idx-transclone", xindex_transclone);

  dx_define("idx-changedim",xindex_change_dim);
  dx_define("idx-changemod",xindex_change_mod);
  dx_define("idx-changeoffset",xindex_change_offset);

  /* loops */

  dy_define("idx-eloop", yeloop);
  dy_define("idx-bloop", ybloop);

  /* compatibility */

  dx_define("matrix",xFmatrix);
  dx_define("array",xATmatrix);
  dx_define("submatrix",xsubindex);
  dx_define("bound",xoldbound);
}
