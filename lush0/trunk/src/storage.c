/***********************************************************************
  Lush:
  (PS) & (LYB) 1991-2001
  ******************************************************************/

/******************************************************************************
 *
 *	STORAGE.C.  Storage management for arrays, etc...
 *
 ******************************************************************************/




/* A storage is a one dimensional area
 * that can hold numbers or lisp objects.
 * A storage may be a piece of memory, 
 * a mapped file, or even a "virtual storage".
 *
 * Storage elements can be accessed on a one by one basis
 * with the <(*st->getat)()>, <(*st->setat)()> functions.
 *
 * It is also possible to access several element at a 
 * time with the <index_{read,write,rls}_idx> which actually 
 * call the storage <(*st->{read,write,rls}_idx)()> vectors.
 * See "index.c" to get more details on the <struct idx>.
 */


#include "header.h"

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif


/* ------- THE ALLOC STRUCTURE ------ */

struct alloc_root storage_alloc = {
  NULL,
  NULL,
  sizeof(struct storage),
  100,
};


/* ------- THE STORAGE_TYPE ARRAYS ----------- */

/*
 * These arrays contain the necessary information
 * about the numerical types of all our storage classes.
 *
 * 'storage_type_size' describes the size of each element
 * 'storage_type_getf' returns a value given a start ptr and an offset
 * 'storage_type_setf' sets a value given a start ptr and an offset
 */


int storage_type_size[ST_LAST] = {
  sizeof(at*),
  sizeof(char),
  sizeof(flt),
  sizeof(real),
  sizeof(long),
  sizeof(short),
  sizeof(char),
  sizeof(unsigned char),
  sizeof(gptr),
};



/* --- the getf functions --- */


static flt AT_getf(gptr pt, int off)
{
  at *p;
  p = ((at **)pt)[off];
  ifn (p && NUMBERP(p))
    error(NIL,"the accessed element is not a number",NIL);
  return p->Number;
}

static flt P_getf(gptr pt, int off)
{
  int b;
  b = ((char *)pt)[off];
  return (flt)b / 16.0;
}

static flt GPTR_getf(gptr pt, int off)
{
  error(NIL,"the accessed element is not a number",NIL);
}


#define Generic_getf(Prefix,Type)   					     \
static flt name2(Prefix,_getf)(gptr pt, int off)        		     \
{   									     \
  return (flt)(((Type*)pt)[off]);   							     \
}

Generic_getf(F, flt)
Generic_getf(D, real)
Generic_getf(I32, long)
Generic_getf(I16, short)
Generic_getf(I8, char)
Generic_getf(U8, unsigned char)

#undef Generic_getf


flt (*storage_type_getf[ST_LAST])(gptr, int) = {
  AT_getf,
  P_getf,
  F_getf,
  D_getf,
  I32_getf,
  I16_getf,
  I8_getf,
  U8_getf,
  GPTR_getf,
};



/* --- the setf functions --- */


static void AT_setf(gptr pt, int off, flt x)
{
  at *p;
  p = ((at **)pt)[off];
  UNLOCK(p);
  ((at **)pt)[off] = NEW_NUMBER(x);
}

static void P_setf(gptr pt, int off, flt x)
{
  if (x > 8.0-1.0/16.0)
    ((char *)pt)[off] = 127;
  else if (x < -8.0)
    ((char *)pt)[off] = -128;
  else
    ((char *)pt)[off] = x*16.0;
}

static void GPTR_setf(gptr pt, int off, flt x)
{
  error(NIL,"gptr arrays do not contain numbers",NIL);
}

#define Generic_setf(Prefix,Type)   					     \
static void name2(Prefix,_setf)(gptr pt, int off, flt x)		     \
{   									     \
  ((Type *)pt)[off] = x;   							     \
}
    
Generic_setf(F, flt)
Generic_setf(D, real)
Generic_setf(I32, long)
Generic_setf(I16, short)
Generic_setf(I8, char)
Generic_setf(U8, unsigned char)

#undef Generic_setf


void (*storage_type_setf[ST_LAST])(gptr, int, flt) = {
  AT_setf,
  P_setf,
  F_setf,
  D_setf,
  I32_setf,
  I16_setf,
  I8_setf,
  U8_setf,
  GPTR_setf,
};




/* ------- THE CLASS FUNCTIONS ------ */



static void 
  storage_dispose(at *p)
{
  struct storage *st;
  st = p->Object;
  if (st) 
  {
      
      if (st->srg.type == ST_AT) 
      {
          at **data;
          int i;
          data = st->srg.data;
          for (i=0; i<st->srg.size; i++)
              UNLOCK(data[i]);
      }
      
      if (st->srg.flags & STS_MALLOC) 
      {
          if(st->srg.size!=0)
              if (st->allinfo.sts_malloc.addr)
                  free(st->allinfo.sts_malloc.addr);
      }
#ifdef HAVE_MMAP
    if (st->srg.flags & STS_MMAP) 
    {
	if (st->allinfo.sts_mmap.addr) 
        {
	    if(st->srg.size==0)
		printf("unmapping a mapped storage of zero size. OK?\n");
	    munmap(st->allinfo.sts_mmap.addr, st->allinfo.sts_mmap.len);
	}
    }
#endif
    
#ifdef DISKARRAY
    --- UNIMPLEMENTED ---
#endif

#ifdef REMOTEARRAY
    --- UNIMPLEMENTED ---
#endif
        
        if (st->cptr)
        {
            lside_destroy_item(st->cptr);
        }
      deallocate(&storage_alloc, (struct empty_alloc *) st);
  }
}

static void
  storage_action(at *p, void (*action) (at *))
{
  struct storage *st;

  st = p->Object;
  if (st && st->srg.type==ST_AT) {
    at **data;
    int i;
    data = st->srg.data;
    for (i=0; i<st->srg.size; i++)
      (*action)(data[i]);
  }
#ifdef HAVE_MMAP
#endif
#ifdef DISKARRAY
#endif
#ifdef REMOTEARRAY
#endif
}

static char* 
  storage_name(at *p)
{
  struct storage *st;
  
  st = p->Object;
  if (st->srg.flags & STS_MALLOC)
    sprintf(string_buffer, "::%s:ram@%lx:<%d>", 
	    nameof(p->Class->classname), (long)st->srg.data, st->srg.size);
  else if (st->srg.flags & STS_MMAP)
    sprintf(string_buffer, "::%s:mmap@%lx:<%d>", 
	    nameof(p->Class->classname), (long)st->srg.data, st->srg.size);
  else if (st->srg.flags & STS_DISK)
    sprintf(string_buffer, "::%s:disk@%lx:<%d>", 
	    nameof(p->Class->classname), (long)st->srg.data, st->srg.size);
  else if (st->srg.flags & STS_MMAP)
    sprintf(string_buffer, "::%s:remote@%lx:<%d>", 
	    nameof(p->Class->classname), (long)st->srg.data, st->srg.size);
  else if (st->srg.flags & STS_STATIC)
    sprintf(string_buffer, "::%s:static@%lx",
	    nameof(p->Class->classname), (long)st->srg.data);
  else if (st->srg.flags & STF_UNSIZED)
    sprintf(string_buffer, "::%s:unsized@%lx",
	    nameof(p->Class->classname), (long)st->srg.data);
  else 
    sprintf(string_buffer, "::%s:strange@%lx",
	    nameof(p->Class->classname), (long)st->srg.data);
  return string_buffer;
}


static at
  *storage_listeval(at *p, at *q)
{
  struct storage *st;
  int off;
  
  st = p->Object;
  if (st->srg.flags & STF_UNSIZED)
    error(NIL,"unsized storage",p);
  if (st) {
    q = eval_a_list(q->Cdr);
    ifn (CONSP(q) && q->Car && NUMBERP(q->Car))
      error(NIL,"illegal subscript",q);
    off = q->Car->Number;
    if (off<0 || off>=st->srg.size)
      error(NIL,"subscript out of range",q);
    if (q->Cdr) {
      ifn (CONSP(q->Cdr) && !q->Cdr->Cdr)
	error(NIL,"bad value",q);
      (*st->setat)(st,off,q->Cdr->Car);
    }
    return (*st->getat)(st,off);
  }
  return NIL;
}

/* dummy at the moment. must be written */
int storage_compare(at *p, at*q, int n) { return 0; }
unsigned long storage_hash( at *p){ return 0; }
at *storage_getslot(at *p, at *q){ return (at *)0; }
void storage_setslot(at *p, at *q, at *r){ }


#define Generic_class(Prefix)   					     \
class name2(Prefix,_storage_class) = {   				     \
  storage_dispose,   							     \
  storage_action,   							     \
  storage_name,   							     \
  generic_eval,   							     \
  storage_listeval,   							     \
  generic_serialize,   							     \
  storage_compare,   							     \
  storage_hash,   							     \
  storage_getslot,   							     \
  storage_setslot   							     \
}

Generic_class(AT);
Generic_class(P);
Generic_class(F);
Generic_class(D);
Generic_class(I32);
Generic_class(I16);
Generic_class(I8);
Generic_class(U8);
Generic_class(GPTR);



/* ----------- VECTORIZED FUNCTIONS ------------- */



/*
 *  Here are the GETAT/SETAT functions 
 *  for the malloc and mmap type storages.
 * 
 *  The default function uses SETF/GETF
 *  The AT storage have special functions
 *  The D and I32 storage have too, because FLTs
 *  have not enough dynamic to represent them all.
 */


/*  STS_MALLOC and STS_MMAP getat */

static at* AT_getat(struct storage *st, int off)
{
  at **pt;
  at *p;
  pt = st->srg.data;
  p = pt[off];
  LOCK(p);
  return p;
}

static at* D_getat(struct storage *st, int off)
{
  real *pt;
  pt = st->srg.data;
  return NEW_NUMBER( pt[off] );
}


static at* I32_getat(struct storage *st, int off)
{
  long *pt;
  pt = st->srg.data;
  return NEW_NUMBER( pt[off] );
}


static at* N_getat(struct storage *st, int off)
{
  flt (*get)(gptr,int);
  get = storage_type_getf[st->srg.type];
  return NEW_NUMBER( (*get)(st->srg.data,off) );
}

static at* GPTR_getat(struct storage *st, int off)
{
  gptr *pt;
  pt = st->srg.data;
  if (pt[off])
      return new_gptr(pt[off]);
  else
      return NIL;
}


/*  STS_MALLOC and STS_MMAP setat */

static void AT_setat(struct storage *st, int off, at *x)
{
  at **pt;
  at *p;
  if (st->srg.flags & STF_RDONLY)
    error(NIL,"read only storage",NIL);
  pt = st->srg.data;
  p = pt[off];
  LOCK(x);
  UNLOCK(p);
  pt[off] = x;
}

static void D_setat(struct storage *st, int off, at *x)
{
  real *pt;
  pt = st->srg.data;
  if (st->srg.flags & STF_RDONLY)
    error(NIL,"read only storage",NIL);
  ifn (NUMBERP(x))
    error(NIL,"not a number",x);
  pt[off] = x->Number;
}


static void I32_setat(struct storage *st, int off, at *x)
{
  long *pt;
  pt = st->srg.data;
  if (st->srg.flags & STF_RDONLY)
    error(NIL,"read only storage",NIL);
  ifn (NUMBERP(x))
    error(NIL,"not a number",x);
  pt[off] = (long)(x->Number);
}

static void N_setat(struct storage *st, int off, at *x)
{
  void (*set)(gptr,int,flt);
  if (st->srg.flags & STF_RDONLY)
    error(NIL,"read only storage",NIL);
  ifn (NUMBERP(x))
    error(NIL,"not a number",x);
  set = storage_type_setf[st->srg.type];
  (*set)(st->srg.data, off, (flt)x->Number);
}


static void GPTR_setat(struct storage *st, int off, at *x)
{
  gptr *pt;
  pt = st->srg.data;
  if (x == 0)
    pt[off] = 0;
  else if (GPTRP(x))
    pt[off] = x->Gptr;
  else
    error(NIL,"Not a gptr",x);
}



/*
 *  Here are the READ_IDX/WRITE_IDX/RLS_IDX functions 
 *  for the malloc and mmap type storages.
 */

    
/*
 * storage_read_srg()
 * storage_write_srg()
 * storage_rls_srg()
 *
 * Given the storage, these function return a
 * srg structure pointing to the data area;
 */

void storage_read_srg(struct storage *st)
{
    if (st->srg.flags & STF_UNSIZED)
	error(NIL,"unsized storage",NIL);
    (st->read_srg)(st);
}

void storage_write_srg(struct storage *st)
{
    (st->write_srg)(st);
}

void storage_rls_srg(struct storage *st)
{
    (st->rls_srg)(st);
}



/* STS_MALLOC and STS_MMAP read_srg */

void simple_read_srg(struct storage *st)
{
    if(st->srg.flags & STS_MALLOC)
	st->srg.data = st->allinfo.sts_malloc.addr;
}

/* STS_MALLOC and STS_MMAP write_srg */

void simple_write_srg(struct storage *st)
{
    if (st->srg.flags & STF_RDONLY)
	error(NIL,"read only storage",NIL);
    if(st->srg.flags & STS_MALLOC)
	st->srg.data = st->allinfo.sts_malloc.addr;
}

/* STS_MALLOC and STS_MMAP rls_srg */

void simple_rls_srg(struct storage *st)
{
    if(st->srg.flags & STS_MALLOC)
	st->allinfo.sts_malloc.addr = st->srg.data;
    ;
}


/* ------- CREATION OF UNSIZED STORAGES ------- */


/* The C functions always return an unsized storage.
 * The Lisp functions return a malloc storage if a <size>
 * argument is provided.
 */


#define Generic_new_storage(Prefix, AtPrefix, Type)   			     \
at *name3(new_,Prefix,_storage)()  	 				     \
{   									     \
  struct storage *st;                                                        \
  at *atst;         						             \
  st = allocate(&storage_alloc);   					     \
  st->cptr = NULL;	   						     \
  st->srg.type = name2(ST_,Prefix);   					     \
  st->srg.flags = STF_UNSIZED;   					     \
  st->srg.size = 0;   							     \
  st->srg.data = NULL;   						     \
  st->read_srg = simple_read_srg;					     \
  st->write_srg = simple_write_srg;					     \
  st->rls_srg = simple_rls_srg;			   			     \
  st->getat = name2(AtPrefix,_getat);   				     \
  st->setat = name2(AtPrefix,_setat);   				     \
  atst = new_extern( &name2(Prefix,_storage_class), st);		     \
  atst->flags |= X_STORAGE;                                                  \
  st->atst = atst;                                                           \
  return atst;                                                               \
}   									     \
DX( name3(xnew_,Prefix,_storage) )  	 				     \
{   								       	     \
  at *p;								     \
  p = name3(new_,Prefix,_storage)();    				     \
  if (arg_number>0) {							     \
    ARG_NUMBER(1);   							     \
    ARG_EVAL(1);   							     \
    storage_malloc(p,AINTEGER(1),1);					     \
  }									     \
  return p;  								     \
}

Generic_new_storage(AT, AT, at*)
Generic_new_storage(P, N, char)
Generic_new_storage(F, N, flt)
Generic_new_storage(D, D, real)
Generic_new_storage(I32, I32, long)
Generic_new_storage(I16, N, short)
Generic_new_storage(I8, N, char)
Generic_new_storage(U8, N, unsigned char)
Generic_new_storage(GPTR, GPTR, gptr)

#undef Generic_new_storage



/*
 * A couple of generic functions. 
 * If size is non zero, returns a malloc storage
 */



at * new_storage(int type, int size)
{
  at *p;
  switch( type ) {
  case ST_D:
    p = new_D_storage(); break;
  case ST_F:
    p = new_F_storage(); break;
  case ST_I32:
    p = new_I32_storage(); break;
  case ST_I16:
    p = new_I16_storage(); break;
  case ST_I8:
    p = new_I8_storage(); break;
  case ST_U8:
    p = new_U8_storage(); break;
  case ST_AT:
    p = new_AT_storage(); break;
  case ST_P:
    p = new_P_storage(); break;
  case ST_GPTR:
    p = new_GPTR_storage(); break;
  default:
    error("storage.c/new_storage","unknown storage type",NEW_NUMBER(type));
  }
  if (size!=0)
    storage_malloc(p,size,1);
  return p;
}



at *
  new_storage_nc(int type, int size)
{
  at *p;
  switch( type ) {
  case ST_D:
    p = new_D_storage(); break;
  case ST_F:
    p = new_F_storage(); break;
  case ST_I32:
    p = new_I32_storage(); break;
  case ST_I16:
    p = new_I16_storage(); break;
  case ST_I8:
    p = new_I8_storage(); break;
  case ST_U8:
    p = new_U8_storage(); break;
  case ST_AT:
    p = new_AT_storage(); break;
  case ST_P:
    p = new_P_storage(); break;
  case ST_GPTR:
    p = new_GPTR_storage(); break;
  default:
    error("storage.c/new_storage","unknown storage type",NEW_NUMBER(type));
  }
  if (size!=0)
    storage_malloc(p,size,0);
  return p;
}





/* ------------ ALLOCATION: MALLOC ------------ */

void 
  storage_malloc(at *atp, int size, int clear_flag)
{
  struct storage *st;
  
  ifn (storagep(atp))
    error(NIL,"not a storage",atp);

  st = atp->Object;
  if (size<=0)
    error(NIL,"Illegal size",NEW_NUMBER(size));
  ifn (st->srg.flags & STF_UNSIZED)
    error(NIL,"An unsized storage is required",atp);
  
  if (clear_flag) {
    st->allinfo.sts_malloc.addr 
      = calloc(size , storage_type_size[st->srg.type]);
  } else if (st->srg.type==ST_AT) {
    error(NIL,"An AT storage must be initialized",atp);
  } else {
    st->allinfo.sts_malloc.addr 
      = malloc(size * storage_type_size[st->srg.type]);    
  }
  
  if (!st->allinfo.sts_malloc.addr)
    error(NIL,"not enough memory",NIL);

  st->srg.data = st->allinfo.sts_malloc.addr;
  st->srg.flags = STS_MALLOC & ~STF_UNSIZED;
  st->srg.size = size;
}

DX(xstorage_malloc)
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  storage_malloc(p,AINTEGER(2),1);
  LOCK(p);
  return p;
}

DX(xstorage_malloc_nc)
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  storage_malloc(p,AINTEGER(2),0);
  LOCK(p);
  return p;
}




void 
  storage_realloc(at *atp, int size, int clear_flag)
{
  struct storage *st;
  gptr data;

  ifn (storagep(atp))
    error(NIL,"not a storage",atp);

  st = atp->Object;
  if (size<=0)
    error(NIL,"Illegal size",NEW_NUMBER(size));
  ifn (st->srg.flags & STS_MALLOC)
    error(NIL,"Only a ram based storage can be enlarged",atp);
  if (size < st->srg.size)
    error(NIL,"Attempt to reduce the size of a storage",atp);

  if(st->srg.size==0) 
      /* (this is possible, storage comes from compiled code) */
      data = malloc(size * storage_type_size[st->srg.type]);
  else
      data = realloc(st->allinfo.sts_malloc.addr,
		     size * storage_type_size[st->srg.type]);

  if (!data) 
    error(NIL,"not enough memory",NIL);

  if (clear_flag) {		/* Clear it by hand */
    char *begin;
    char *end;
    begin = (char*)(data) + st->srg.size * storage_type_size[st->srg.type];
    end = (char*)(data) + size * storage_type_size[st->srg.type];
    while (begin<end)
      *begin++ = 0;
  } else if (st->srg.type==ST_AT) {
    error(NIL,"An AT storage must be initialized",atp);
  } 
  
  st->allinfo.sts_malloc.addr  = data;
  st->srg.data = data;
  st->srg.size = size;
}

DX(xstorage_realloc)
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  storage_realloc(p,AINTEGER(2),1);
  LOCK(p);
  return p;
}

DX(xstorage_realloc_nc)
{
  at *p;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p = APOINTER(1);
  storage_realloc(p,AINTEGER(2),0);
  LOCK(p);
  return p;
}









/* ------------ ALLOCATION: MMAP ------------ */


#ifdef HAVE_MMAP

void storage_mmap(at *atp, at *atf, int offset)
{
  struct storage *st;
  File *f;
  int len;

  ifn (storagep(atp))
    error(NIL,"not a storage",atp);
  
  st = atp->Object;
  f = atf->Object;

  if (FSEEK(f,0,2)==-1)
    test_file_error(NIL);
  len = FTELL(f);
  REWIND(f);
  
  if (st->srg.type == ST_AT)
    error(NIL,"cannot map an AT storage",atp);
  
  ifn (st->srg.flags & STF_UNSIZED)
    error(NIL,"An unsized storage is required",NIL);
  
  st->srg.size = (len - offset) / storage_type_size[st->srg.type];
  st->allinfo.sts_mmap.len  = len;
  st->allinfo.sts_mmap.addr = mmap(0,len,PROT_READ,MAP_SHARED,FILENO(f),0);
  if (st->allinfo.sts_mmap.addr==(void*)-1L)
    test_file_error(NIL);
  st->srg.data = (char*)(st->allinfo.sts_mmap.addr)+offset;
  st->srg.flags = (STS_MMAP | STF_RDONLY) & ~STF_UNSIZED;
}

DX(xstorage_mmap)
{
  at *atp, *atf;
  int offset = 0;

  ALL_ARGS_EVAL;
  if (arg_number==3)
    offset = AINTEGER(3);
  else
    ARG_NUMBER(2);
  
  atp = APOINTER(1);
  atf = APOINTER(2);

  if (EXTERNP(atf,&file_R_class)) {
    LOCK(atf);
  } else if (EXTERNP(atf,&string_class)) {
    atf = OPEN_READ(SADD(atf->Object),NULL);
  } else
    error(NIL,"Neither a string, nor a file descriptor",atf);

  storage_mmap(atp,atf,offset);
  LOCK(atp);
  UNLOCK(atf);
  return atp;
}


#endif


/* ------------ ALLOCATION FUNCTION: DISK -------- */

#ifdef DISKARRAY
    --- UNIMPLEMENTED ---
#endif

/* ------------ ALLOCATION FUNCTION: REMOTE ------- */

#ifdef REMOTEARRAY
    --- UNIMPLEMENTED ---
#endif




/* ============== Lush FUNCTIONS ================= */

int
 storagep(at *p)
{
  if (EXTERNP(p,&AT_storage_class)) return TRUE;
  if (EXTERNP(p,&P_storage_class)) return TRUE;
  if (EXTERNP(p,&F_storage_class)) return TRUE;
  if (EXTERNP(p,&D_storage_class)) return TRUE;
  if (EXTERNP(p,&I32_storage_class)) return TRUE;
  if (EXTERNP(p,&I16_storage_class)) return TRUE;
  if (EXTERNP(p,&I8_storage_class)) return TRUE;
  if (EXTERNP(p,&U8_storage_class)) return TRUE;
  if (EXTERNP(p,&GPTR_storage_class)) return TRUE;
  return FALSE;
}

DX(xstoragep)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (storagep(APOINTER(1)))
    return true();
  else
    return NIL;
}


DX(xwritablep)
{
  at *p;
  struct storage *st;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);

  if (storagep(p))
    st = p->Object;
  /*
  else if (INDEXP(p))
    st = ((struct index*)(p->Object))->st;
  */
  else
    error(NIL,"neiher an index, nor a storage",p);

  if (st->srg.flags & STF_RDONLY)
    return NIL;
  else
    return true();
}

DX(xstorage_read_only)
{
  at *p;
  struct storage *st;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);

  if (storagep(p))
    st = p->Object;
  /* 
  else if (indexp(p))
    st = ((struct index*)(p->Object))->st;
  */
  else
    error(NIL,"neiher an index, nor a storage",p);
  st->srg.flags |= STF_RDONLY;
  LOCK(p);
  return p;
}

DX(xstorage_size)
{
  at *p;
  struct storage *st;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);

  if (storagep(p))
    st = p->Object;
  /* 
  else if (indexp(p))
    st = ((struct index*)(p->Object))->st;
  */
  else
    error(NIL,"neiher an index, nor a storage",p);

  return NEW_NUMBER(st->srg.size);
}


/* ------------ Lush: STORAGE_CLEAR ------------ */


/* 
 * clear strategy: AT:         use <*st->setat>
 *                 STS_MALLOC: do it directly!
 *                 other:      use <*st->setd>
 */

void storage_clear(at *p)
{
  struct storage *st;

  if (storagep(p))
    st = p->Object;
  /* 
  else if (indexp(p))
    st = ((struct index*)(p->Object))->st;
  */
  else
    error(NIL,"neiher an index, nor a storage",p);

  if (st->srg.type==ST_AT) 
    {
      int i;
      for (i=0; i<st->srg.size; i++)
	(*st->setat)(st,i,NIL);
    } 
  else if (!(st->srg.flags & STS_MALLOC))
    {
      int i;
      at *p;
      p = NEW_NUMBER(0);
      for (i=0; i<st->srg.size; i++)
	(*st->setat)(st,i,p);
      UNLOCK(p);
    } 
  else 
    {
      int n;
      long *p;
      char *q;
      p = st->srg.data;
      n = storage_type_size[st->srg.type] * st->srg.size;
      while (n>=sizeof(long)) {
	*p++ = 0;
	n -= sizeof(long);
      }
      q = (char*)p;
      while (n>0) {
	*q++ = 0;
	n -= sizeof(char);
      }
    }
}

DX(xstorage_clear)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  storage_clear(APOINTER(1));
  return NIL;
}



/* ------------ Lush: STORAGE_LOAD ------------ */


/*
 * If the storage is unsized, malloc it 
 * If the storage is sized, load only the useful part
 */


int storage_load(at *atp, at *atf)
{
  struct storage *st;
  File *f;
  int nrec;
  char *pt;

  ifn (storagep(atp))
    error(NIL,"not a storage",atp);

  st = atp->Object;
  f = atf->Object;

  if (st->srg.type==ST_AT)
    error(NIL,"cannot load a AT storage",atp);
  if (st->srg.flags & STF_UNSIZED) {
    int len;
    int here;
    here = FTELL(f);
    if (FSEEK(f,0,2)==-1)
      test_file_error(NIL);
    len = FTELL(f);
    if (FSEEK(f,here,0)==-1)
      test_file_error(NIL);
    if (len==0) { return 0; }
    storage_malloc(atp,len/storage_type_size[st->srg.type],0);
  }

  (*st->write_srg)(st);
  pt = st->srg.data;
  nrec = PFREAD(pt, storage_type_size[st->srg.type], st->srg.size, f);
  (*st->rls_srg)(st);
  if (nrec<st->srg.size)
   error(NIL,"End of file reached!",atf);
  TEST_FILE_ERROR(f);
  return 1;
}

DX(xstorage_load)
{
  at *atp, *atf;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  atp = APOINTER(1);
  atf = APOINTER(2);

  if (EXTERNP(atf,&file_R_class)) {
    LOCK(atf);
  } else if (EXTERNP(atf,&string_class)) {
    atf = OPEN_READ(SADD(atf->Object),NULL);
  } else
    error(NIL,"Neither a string, nor a file descriptor",atf);

  if (!storage_load(atp,atf)) { 
     UNLOCK(atf);
     return NIL; 
  }
  UNLOCK(atf);
  LOCK(atp);
  return atp;
}




/* ------------ Lush: STORAGE_LOAD ------------ */


/*
 * save a size storage only..
 */

void storage_save(at *atp, at *atf)
{
  struct storage *st;
  File *f;
  int nrec;
  char *pt;

  ifn (storagep(atp))
    error(NIL,"not a storage",atp);
  
  st = atp->Object;
  f = atf->Object;
  
  if (st->srg.type==ST_AT)
    error(NIL,"cannot save a AT storage",atp);
  
  if (st->srg.flags & STF_UNSIZED) 
    error(NIL,"cannot save an unsized storage",atp);
  
  (*st->read_srg)(st);
  pt = st->srg.data;

  nrec = PFWRITE(pt,storage_type_size[st->srg.type],st->srg.size,f);
  (*st->rls_srg)(st);
  
  TEST_FILE_ERROR(f);
  if (nrec<st->srg.size)
    error(NIL,"Storage has not been completely saved",atp);
}

DX(xstorage_save)
{
  at *atp, *atf;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  atp = APOINTER(1);
  atf = APOINTER(2);

  ifn (storagep(atp))
    error(NIL,"not a storage",atp);

  if (EXTERNP(atf,&file_W_class)) {
    LOCK(atf);
  } else if (EXTERNP(atf,&string_class)) {
    atf = OPEN_WRITE(SADD(atf->Object),NULL);
  } else
    error(NIL,"Neither a string, nor a file descriptor",atf);

  storage_save(atp,atf);
  LOCK(atp);
  UNLOCK(atf);
  return atp;
}




/* ------------ THE INITIALIZATION ------------ */


void init_storage()
{
  class_define("ATSTORAGE",&AT_storage_class);
  class_define("PSTORAGE",&P_storage_class);
  class_define("FSTORAGE",&F_storage_class);
  class_define("DSTORAGE",&D_storage_class);
  class_define("I32STORAGE",&I32_storage_class);
  class_define("I16STORAGE",&I16_storage_class);
  class_define("I8STORAGE",&I8_storage_class);
  class_define("U8STORAGE",&U8_storage_class);
  class_define("GPTRSTORAGE",&GPTR_storage_class);

  dx_define("new_at_storage",xnew_AT_storage);
  dx_define("new_p_storage",xnew_P_storage);
  dx_define("new_f_storage",xnew_F_storage);
  dx_define("new_d_storage",xnew_D_storage);
  dx_define("new_i32_storage",xnew_I32_storage);
  dx_define("new_i16_storage",xnew_I16_storage);
  dx_define("new_i8_storage",xnew_I8_storage);
  dx_define("new_u8_storage",xnew_U8_storage);
  dx_define("new_gptr_storage",xnew_GPTR_storage);

  dx_define("storage_malloc",xstorage_malloc);
  dx_define("storage_malloc_nc",xstorage_malloc_nc);
  dx_define("storage_realloc",xstorage_realloc);
  dx_define("storage_realloc_nc",xstorage_realloc_nc);
#ifdef HAVE_MMAP
  dx_define("storage_mmap",xstorage_mmap);
#endif
#ifdef DISKARRAY
  dx_define("storage_disk",xstorage_disk);
#endif
#ifdef REMOTEARRAY
  dx_define("storage_remote",xstorage_remote);
#endif

  dx_define("storagep",xstoragep);
  dx_define("writablep",xwritablep);
  dx_define("storage_read_only", xstorage_read_only);
  dx_define("storage_size",xstorage_size);
  dx_define("storage_clear",xstorage_clear);
  dx_define("storage_load",xstorage_load);
  dx_define("storage_save",xstorage_save);
}
