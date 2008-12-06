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
 * $Id: storage.c,v 1.14 2006/03/26 01:36:55 leonb Exp $
 **********************************************************************/


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
#include "check_func.h"
#include <inttypes.h>

#ifdef HAVE_MMAP
#include <sys/types.h>
#include <sys/mman.h>
#endif

void clear_storage(storage_t *st)
{
   st->data = NULL;
   st->backptr = NULL;
   st->cptr = NULL;
}

void mark_storage(storage_t *st)
{
   MM_MARK(st->backptr);
   if (!st->cptr || lisp_owns_p(st->cptr))
      MM_MARK(st->data);
}

static storage_t *storage_dispose(storage_t *);

bool finalize_storage(storage_t *st)
{
   //printf("finalize_storage(0x%x)\n", (unsigned int)st);
   storage_dispose(st);
   return true;
}

static mt_t mt_storage = mt_undefined;


/* ------- THE STORAGE_TYPE ARRAYS ----------- */

/*
 * These arrays contain the necessary information
 * about the numerical types of all our storage classes.
 *
 * storage_sizeof[t]  is the size of a storage element of type t
 * storage_getX[t]    returns as X the value given a start ptr and offset
 * storage_setX[t]    sets a value given as X
 */

size_t storage_sizeof[ST_LAST] = {
   sizeof(at*),           /* atom   */
   sizeof(flt),           /* float  */
   sizeof(real),          /* double */
   sizeof(int),           /* int    */
   sizeof(short),         /* short  */
   sizeof(char),          /* byte   */
   sizeof(unsigned char), /* ubyte  */
   sizeof(gptr),          /* gptr   */
};


static flt AT_getf(gptr pt, size_t off)
{
   at *p = ((at **)pt)[off];
   ifn (p && NUMBERP(p))
      error(NIL, "accessed element is not a number", p);
   return Number(p);
}

static flt GPTR_getf(gptr pt, size_t off)
{
   error(NIL, "accessed element is not a number", NIL);
}

#define Generic_getf(Prefix,Type)   					     \
static flt name2(Prefix,_getf)(gptr pt, size_t off)        		     \
{   									     \
  return (flt)(((Type*)pt)[off]);  					     \
}

Generic_getf(F, flt)
Generic_getf(D, real)
Generic_getf(I32, int)
Generic_getf(I16, short)
Generic_getf(I8, char)
Generic_getf(U8, unsigned char)
#undef Generic_getf

flt (*storage_getf[ST_LAST])(gptr, size_t) = {
   AT_getf, F_getf, D_getf, I32_getf, I16_getf, I8_getf, U8_getf, GPTR_getf,
};



static void AT_setf(gptr pt, size_t off, flt x)
{
   ((at **)pt)[off] = NEW_NUMBER(x);
}

static void GPTR_setf(gptr pt, size_t off, flt x)
{
   error(NIL, "gptr arrays do not contain numbers", NIL);
}

#define Generic_setf(Prefix,Type)   					     \
static void name2(Prefix,_setf)(gptr pt, size_t off, flt x)		     \
{   									     \
  ((Type *)pt)[off] = x; 						     \
}
    
Generic_setf(F, flt)
Generic_setf(D, real)
Generic_setf(I32, int)
Generic_setf(I16, short)
Generic_setf(I8, char)
Generic_setf(U8, unsigned char)
#undef Generic_setf

void (*storage_setf[ST_LAST])(gptr, size_t, flt) = {
   AT_setf, F_setf, D_setf, I32_setf, I16_setf, I8_setf, U8_setf, GPTR_setf,
};



static real AT_getr(gptr pt, size_t off)
{
   at *p = ((at **)pt)[off];
   ifn (p && NUMBERP(p))
      error(NIL, "accessed element is not a number", NIL);
   return Number(p);
}

static real GPTR_getr(gptr pt, size_t off)
{
   error(NIL, "accessed element is not a number", NIL);
}

#define Generic_getr(Prefix,Type)   					     \
static real name2(Prefix,_getr)(gptr pt, size_t off)        		     \
{   									     \
  return (real)(((Type*)pt)[off]);   					     \
}

Generic_getr(F, flt)
Generic_getr(D, real)
Generic_getr(I32, int)
Generic_getr(I16, short)
Generic_getr(I8, char)
Generic_getr(U8, unsigned char)
#undef Generic_getr

real (*storage_getr[ST_LAST])(gptr, size_t) = {
   AT_getr, F_getr, D_getr, I32_getr, I16_getr, I8_getr, U8_getr, GPTR_getr,
};



static void AT_setr(gptr pt, size_t off, real x)
{
   ((at **)pt)[off] = NEW_NUMBER(x);
}

static void GPTR_setr(gptr pt, size_t off, real x)
{
   error(NIL, "gptr arrays do not contain numbers", NIL);
}

#define Generic_setr(Prefix,Type)                             \
static void name2(Prefix,_setr)(gptr pt, size_t off, real x)  \
{   						      	      \
  ((Type *)pt)[off] = x;   				      \
}
    
Generic_setr(F, flt)
Generic_setr(D, real)
Generic_setr(I32, int)
Generic_setr(I16, short)
Generic_setr(I8, char)
Generic_setr(U8, unsigned char)
#undef Generic_setr

void (*storage_setr[ST_LAST])(gptr, size_t, real) = {
   AT_setr, F_setr, D_setr, I32_setr, I16_setr, I8_setr, U8_setr, GPTR_setr,
};



static at *AT_getat(storage_t *st, size_t off)
{
   at **pt = st->data;
   at *p = pt[off];
   return p;
}

static at *N_getat(storage_t *st, size_t off)
{
   real (*get)(gptr,size_t) = storage_getr[st->type];
   return NEW_NUMBER( (*get)(st->data, off) );
}

static at *GPTR_getat(storage_t *st, size_t off)
{
   gptr *pt = st->data;
   return new_gptr(pt[off]);
}

at *(*storage_getat[ST_LAST])(storage_t *, size_t) = {
   AT_getat, N_getat, N_getat, N_getat, N_getat, N_getat, N_getat, GPTR_getat
};


void get_write_permit(storage_t *st)
{
   if (st->flags & STF_RDONLY)
      error(NIL, "read only storage", NIL);
}

static void AT_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   at **pt = st->data;
   pt[off] = x;
}

static void N_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   ifn (NUMBERP(x))
      error(NIL, "not a number", x);
   void (*set)(gptr,size_t,real) = storage_setr[st->type];
   (*set)(st->data, off, Number(x));
}

static void GPTR_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   ifn (GPTRP(x))
      error(NIL, "not a gptr", x);
   gptr *pt = st->data;
   pt[off] = Gptr(x);
}

void (*storage_setat[ST_LAST])(storage_t *, size_t, at *) = {
   AT_setat, N_setat, N_setat, N_setat, N_setat, N_setat, N_setat, GPTR_setat
};


/* ------- THE CLASS FUNCTIONS ------ */

static storage_t *storage_dispose(storage_t *st)
{
   if (st) {
      if (st->cptr)
         lside_destroy_item(st->cptr);

#ifdef HAVE_MMAP
      if (st->flags & STS_MMAP) {
         if (st->mmap_addr) {
#ifdef UNIX
            munmap(st->mmap_addr, st->mmap_len);
#endif
#ifdef WIN32
            UnmapViewOfFile(st->mmap_addr);
            CloseHandle((HANDLE)(st->mmap_xtra));
#endif
            st->mmap_addr = NULL;
         }
      }
#endif // HAVE_MMAP
      zombify(st->backptr);
   }
   return NULL;
}

static const char *storage_name(at *p)
{
   storage_t *st = (storage_t *)Mptr(p);
 
  if (st->flags & STS_MALLOC)
      sprintf(string_buffer, "::%s:ram@%"PRIxPTR":<%"PRIdPTR">", 
              NAMEOF(Class(p)->classname), (uintptr_t)st->data, st->size);
   else if (st->flags & STS_MMAP)
      sprintf(string_buffer, "::%s:mmap@%"PRIxPTR":<%"PRIdPTR">", 
              NAMEOF(Class(p)->classname), (uintptr_t)st->data, st->size);
   else if (st->flags & STS_STATIC)
      sprintf(string_buffer, "::%s:static@%"PRIxPTR,
              NAMEOF(Class(p)->classname), (uintptr_t)st->data);
   else if (st->data == NULL)
      sprintf(string_buffer, "::%s:unsized@%"PRIxPTR,
              NAMEOF(Class(p)->classname), (uintptr_t)st->data);
   else
      sprintf(string_buffer, "::%s:strange@%"PRIxPTR,
              NAMEOF(Class(p)->classname), (uintptr_t)st->data);
  return mm_strdup(string_buffer);
}

static at *storage_listeval(at *p, at *q)
{
   storage_t *st = Mptr(p);

   if (!st->data)
      error(NIL, "unsized storage", p);

   q = eval_arglist(Cdr(q));
   ifn (CONSP(q) && Car(q) && NUMBERP(Car(q)))
      error(NIL, "illegal subscript", q);

   size_t off = Number(Car(q));
   if (off<0 || off>=st->size)
      error(NIL, "subscript out of range", q);

   if (Cdr(q)) {
      ifn (CONSP(Cdr(q)) && !Cddr(q))
         error(NIL,"bad value",q);
      storage_setat[st->type](st, off, Cadr(q));
   }
   return storage_getat[st->type](st, off);
}


static void swap_buffer(char *b, int n, int m)
{
   char buffer[16];
   for (int i=0; i<n; i++) {
      for (int j=0; j<m; j++) buffer[j] = b[m-j-1];
      for (int j=0; j<m; j++) *b++ = buffer[j];
   }
}

#define STORAGE_NORMAL (0x53524758)
#define STORAGE_SWAPPED (0x58475253)

static void storage_serialize(at **pp, int code)
{
   storage_t *st;
   int type ,flags;
   size_t size;

   if (code != SRZ_READ) {
      st = Mptr(*pp);
      type = st->type;
      flags = st->flags;
      size = st->size;
   }
   // Read/write basic info
   serialize_int(&type, code);
   serialize_int(&flags, code);
   serialize_size(&size, code);

   // Create storage if needed
   if (code == SRZ_READ)
      *pp = MAKE_STORAGE(type, size, NIL);
   
   // Read/write storage data
   st = Mptr(*pp);
   if (type == ST_AT) {
      at **data = st->data;
      for (int i=0; i<size; i++)
         serialize_atstar( &data[i], code);

   } else  {
      FILE *f = serialization_file_descriptor(code);
      if (code == SRZ_WRITE) {
         extern int in_bwrite;
         in_bwrite += sizeof(int) + size * storage_sizeof[type];
         write4(f, STORAGE_NORMAL);
         storage_save(st, f);
         
      } else if (code == SRZ_READ) {
         int magic = read4(f);
         storage_load(st, f);
         st = Mptr(*pp);
         if (magic == STORAGE_SWAPPED)
            swap_buffer(st->data, size, storage_sizeof[type]);
         else if (magic != STORAGE_NORMAL)
            RAISEF("Corrupted binary file",NIL);
      }
   }
}


/* ----------- VECTORIZED FUNCTIONS ------------- */

/*
 * storage_read_srg()
 * storage_write_srg()
 * storage_rls_srg()
 *
 * Given the storage, these function return a
 * srg structure pointing to the data area;
 */


/* void storage_read_srg(storage_t *st) */
/* { */
/*    if (st->data == NULL) */
/*       error(NIL, "unsized storage", NIL); */
/*    (st->read_srg)(st); */
/* } */

/* void storage_write_srg(storage_t *st) */
/* { */
/*    (st->write_srg)(st); */
/* } */

/* void storage_rls_srg(storage_t *st) */
/* { */
/*    (st->rls_srg)(st); */
/* } */


/* /\* STS_MALLOC and STS_MMAP read_srg *\/ */

/* static void simple_read_srg(storage_t *st) */
/* { */
/*    if(st->srg.flags & STS_MALLOC) */
/*       st->srg.data = st->addr; */
/* } */

/* /\* STS_MALLOC and STS_MMAP write_srg *\/ */

/* static void simple_write_srg(storage_t *st) */
/* { */
/*    if (st->srg.flags & STF_RDONLY) */
/*       error(NIL, "read only storage", NIL); */
/*    if(st->srg.flags & STS_MALLOC) */
/*       st->srg.data = st->addr; */
/* } */

/* /\* STS_MALLOC and STS_MMAP rls_srg *\/ */

/* static void simple_rls_srg(storage_t *st) */
/* { */
/*    if (st->srg.flags & STS_MALLOC) */
/*       st->addr = st->srg.data; */
/* } */

/* ------- CREATION OF UNSIZED STORAGES ------- */

extern void dbg_notify(void *, void *);

storage_t *new_storage(storage_type_t t)
{
   storage_t *st = mm_alloc(mt_storage);
   st->type = t;
   st->flags = 0;
   st->size = 0;
   //st->data = NULL; // set by mm_alloc
   st->backptr = new_extern(&storage_class[st->type], st);
   st->cptr = NULL;
   //add_notifier(st, dbg_notify, NULL);
   return st;
}

DX(xnew_storage) {
  
   ARG_NUMBER(1);
   class_t *cl = ACLASS(1);

   storage_type_t t = ST_FIRST;
   for (; t < ST_LAST; t++)
      if (cl==&storage_class[t]) break;
   if (t == ST_LAST)
      RAISEF("not a storage class", APOINTER(1));
   return NEW_STORAGE(t);
}

storage_t *make_storage(storage_type_t t, size_t n, at *init)
{
   if (n<0)
      RAISEF("invalid size", NEW_NUMBER(n));
   
   storage_t *st = new_storage(t);
   storage_malloc(st, n, init);
   return st;
}

/* ------------ ALLOCATION: MALLOC ------------ */

void storage_malloc(storage_t *st, size_t n, at *init)
{
   ifn (st->data == NULL)
      RAISEF("storage must be unsized", st->backptr);
   
   /* allocate memory and initialize srg */
   size_t s = n*storage_sizeof[st->type];
   st->data  = mm_allocv(st->type==ST_AT ? mt_refs : mt_blob, s);
   st->flags = STS_MALLOC;
   st->size  = n;

   /* clear gptr storage data (ATs are cleared by mm) */
   if (init && n>0)
      storage_clear(st, init, 0);

   else if (st->type == ST_GPTR) {
      gptr *pt = st->data;
      for (int i=0; i<n; i++) 
         pt[i] = NULL;
   }
}

DX(xstorage_malloc)
{
   ARG_NUMBER(3);
   int n = AINTEGER(2);
   if (n<0)
      RAISEFX("invalid size", NEW_NUMBER(n));
   storage_malloc(ASTORAGE(1), n, APOINTER(3));
   return NIL;
}

void storage_realloc(storage_t *st, size_t size, at *init)
{
   if (size < st->size)
      RAISEF("storage size cannot be reduced", st->backptr);
   
   ifn (st->flags & STS_MALLOC)
      RAISEF("only RAM based storages can be enlarged", st->backptr);
   
   /* reallocate memory and update srg */
   size_t s = size*storage_sizeof[st->type];
   size_t olds = st->size*storage_sizeof[st->type];
   gptr olddata = st->data;
   MM_ANCHOR(olddata);
   if (st->type==ST_AT || st->type==ST_GPTR) {
      st->data = mm_allocv(mt_refs, s);  /* aborts if OOM */
      memcpy(st->data, olddata, olds);
   } else
      st->data = mm_realloc(olddata, s);

   if (st->data == NULL) {
      st->data = olddata;
      RAISEF("not enough memory", NIL);
   }
   size_t oldsize = st->size;
   st->size = size;

   if (init) {
      /* temporarily clear read only flag to allow initialization */
      short flags = st->flags;
      st->flags &= ~STF_RDONLY;
      storage_clear(st, init, oldsize);
      st->flags = flags;
   }
}

DX(xstorage_realloc)
{
   if (arg_number<2 || arg_number>3)
      ARG_NUMBER(-1);
   at *init = arg_number==3 ? APOINTER(3) : NIL;
   storage_realloc(ASTORAGE(1), AINTEGER(2), init);
   return NIL;
}

/* ------------ STORAGE_CLEAR ------------ */


/* 
 * clear strategy: AT:         use <*st->setat>
 *                 STS_MALLOC: do it directly!
 *                 other:      use <*st->setat>
 *
 */

void storage_clear(storage_t *st, at *init, size_t from)
{
   /* don't need to check read-only status here because 
      it will be checked by the setat function below */
   int size = st->size;
   if (from<0 || from>=size)
      RAISEF("invalid value for 'from'", NEW_NUMBER(from));
   
   /* clear from from to to */
   if (st->type == ST_AT) {
      for (int off = from; off < size; off++)
         (storage_setat[st->type])(st, off, init);
      
   } else if (storage_setat[st->type] == N_setat) {
      get_write_permit(st);
      if (st->flags & STS_MALLOC) {
         void (*set)(gptr, size_t, real);
         set = storage_setr[st->type];
         for (int off = from; off < size; off++)
            (*set)(st->data, off, Number(init));
      } else
         for (int off = from; off<size; off++)
            N_setat(st, off, init);
      
   } else if (storage_setat[st->type] == GPTR_setat) {
      get_write_permit(st);
      gptr *pt = st->data;
      for (int off=from; off<size; off++)
         pt[off] = Gptr(init);
   } else
      RAISEF("don't know how to clear this storage", st->backptr);
}

DX(xstorage_clear)
{
   ARG_NUMBER(2);
   storage_clear(ASTORAGE(1), APOINTER(2), 0);
   return NIL;
}


/* ------------ ALLOCATION: MMAP ------------ */


#ifdef HAVE_MMAP

void storage_mmap(storage_t *st, FILE *f, size_t offset)
{
#if HAVE_FSEEKO
   if (fseeko(f,(off_t)0,SEEK_END)==-1)
      test_file_error(NIL);
#else
   if (fseek(f,0,SEEK_END)==-1)
      test_file_error(NIL);
#endif
#if HAVE_FTELLO
   size_t len = (size_t)ftello(f);
#else
   size_t len = (size_t)ftell(f);
#endif
   rewind(f);
   if (st->type == ST_AT)
      RAISEF("cannot map an AT storage", st->backptr);
   ifn (st->data == NULL)
      RAISEF("unsized storage required", st->backptr);
#ifdef UNIX
   gptr addr = mmap(0,len,PROT_READ,MAP_SHARED,fileno(f),0);
   if (addr == (void*)-1L)
      test_file_error(NIL);
#endif
#ifdef WIN32
   gptr xtra, addr;
   if (! (xtra = (gptr)CreateFileMapping((HANDLE)(_get_osfhandle(fd)), 
                                         NULL, PAGE_READONLY, 0, len, NULL)))
      RAISEF("cannot create file mapping",NIL);
   if (! (addr = (gptr)MapViewOfFile((HANDLE)(xtra), 
                                     FILE_MAP_READ, 0, 0, size + pos)))
      RAISEF("cannot create view on mapped file",NIL);
   st->mmap_xtra = xtra;
#endif
   st->flags = STS_MMAP | STF_RDONLY;
   st->mmap_len = len;
   st->mmap_addr = addr;
   st->size = (len - offset) / storage_sizeof[st->type];
   st->data = (char *)(st->mmap_addr)+offset;
}

DX(xstorage_mmap)
{
   if (arg_number<2 || arg_number>3)
      ARG_NUMBER(-1);
   size_t offset = (arg_number==3) ? AINTEGER(3) : 0;
   storage_t *st = ASTORAGE(1);
   at *atf = APOINTER(2);

   if (RFILEP(atf)) {
      // fine
   } else if (STRINGP(atf)) {
      atf = OPEN_READ(String(atf), NULL);
   } else
      RAISEF("neither a string nor a file descriptor", atf);
   
   storage_mmap(st, Gptr(atf), offset);
   return NIL;
}


#endif


/* ============== Lush FUNCTIONS ================= */

DX(xstoragep)
{
   ARG_NUMBER(1);
   return STORAGEP(APOINTER(1)) ? t() : NIL;
}

bool storage_classp(const at *p)
{
   ifn (CLASSP(p))
      return false;
   class_t *cl = Mptr(p);

   for (storage_type_t t = ST_FIRST; t < ST_LAST; t++)
      if (cl == &storage_class[t]) 
         return true;
   return false;
}

DX(xstorage_classp)
{
   ARG_NUMBER(1);
   return storage_classp(APOINTER(1)) ? t() : NIL;
}

bool storage_readonlyp(const storage_t *st) 
{
   return (st->flags & STF_RDONLY);
}

DX(xstorage_readonlyp)
{
   ARG_NUMBER(1);
   storage_t *st = ASTORAGE(1);;
   return (st->flags & STF_RDONLY) ? t() : NIL;
}

DX(xstorage_make_readonly)
{
   ARG_NUMBER(1);
   storage_t *st = ASTORAGE(1);
   st->flags |= STF_RDONLY;
   return NIL;
}


size_t storage_nelems(const storage_t *st) 
{
   return st->size;
}

DX(xstorage_nelems)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(storage_nelems(ASTORAGE(1)));
}


size_t storage_nbytes(const storage_t *st) 
{
   return st->size*storage_sizeof[st->type];
}

DX(xstorage_nbytes)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(storage_nbytes(ASTORAGE(1)));
}

/* ------------ STORAGE_LOAD ------------ */


/* storage_load:
 * If the storage is unsized, malloc it.
 * If the storage is sized, load only the useful part.
 */

void storage_load(storage_t *st, FILE *f)
{
   if (st->type == ST_AT)
      RAISEF("cannot load an AT storage", NIL);
   if (st->data == NULL) {
      assert(st->size == 0);
    
#if HAVE_FSEEKO
      off_t here = ftello(f);
      if (fseeko(f,0,SEEK_END)==-1)
         test_file_error(NIL);
      off_t len = ftello(f);
      if (fseeko(f,here,SEEK_SET)==-1)
         test_file_error(NIL);
#else
      off_t here = ftell(f);
      if (fseek(f,0,SEEK_END)==-1)
         test_file_error(NIL);
      int len = ftell(f);
      if (fseek(f,here,SEEK_SET)==-1)
         test_file_error(NIL);
#endif
      if (len==0) 
         return;
      else
         storage_malloc(st,(size_t)len/storage_sizeof[st->type],0);
   }
   
   get_write_permit(st);
   char *pt = st->data;
   int nrec = fread(pt, storage_sizeof[st->type], st->size, f);
   if (nrec < st->size)
      RAISEF("file is too small",NIL);
   test_file_error(f);
}

DX(xstorage_load)
{
   ARG_NUMBER(2);
   storage_t *st = ASTORAGE(1);
   at *atf = APOINTER(2);
   
   if (RFILEP(atf)) {
      // fine
   } else if (STRINGP(atf)) {
      atf = OPEN_READ(String(atf), NULL);
   } else
      RAISEF("neither a string nor a file descriptor",atf);

   storage_load(st, Gptr(atf));
   return NIL;
}


/* ------------ Lush: STORAGE_SAVE ------------ */


/*
 * save a sized storage.
 */

void storage_save(storage_t *st, FILE *f)
{
   if (st->type == ST_AT)
      RAISEF("cannot save an AT storage",NIL);
   if (st->data == NULL) 
      RAISEF("cannot save an unsized storage",NIL);
  
   char *pt = st->data;
   int nrec = fwrite(pt, storage_sizeof[st->type], st->size, f);
   test_file_error(f);
   if (nrec < st->size)
      RAISEF("storage could not be saved completely",NIL);
}

DX(xstorage_save)
{
   ARG_NUMBER(2);
   storage_t *st = ASTORAGE(1);
   at *atf = APOINTER(2);

   if (WFILEP(atf)) {
      // fine
   } else if (STRINGP(atf)) {
      atf = OPEN_WRITE(String(atf), NULL);
   } else
      RAISEF("neither a string nor a file descriptor", atf);
   storage_save(st, Gptr(atf));
  return NIL;
}




/* ------------ THE INITIALIZATION ------------ */

class_t abstract_storage_class;
class_t storage_class[ST_LAST];

#define Generic_storage_class_init(tok)		                     \
  class_init(&(storage_class[ST_ ## tok]), false);    			     \
  (storage_class[ST_ ## tok]).dispose = (dispose_func_t *)storage_dispose;	     \
  (storage_class[ST_ ## tok]).name = storage_name;		     \
  (storage_class[ST_ ## tok]).listeval = storage_listeval;	     \
  (storage_class[ST_ ## tok]).serialize = storage_serialize;	     \
  (storage_class[ST_ ## tok]).super = &abstract_storage_class;	     \
  (storage_class[ST_ ## tok]).atsuper = abstract_storage_class.backptr; \
  class_define(#tok "STORAGE", &(storage_class[ST_ ## tok]));


void init_storage()
{
   mt_storage = MM_REGTYPE("storage", sizeof(storage_t),
                           clear_storage, mark_storage, finalize_storage);

   /* set up storage_classes */
   class_init(&abstract_storage_class, false);
   class_define("STORAGE",&abstract_storage_class);
   Generic_storage_class_init(AT);
   Generic_storage_class_init(F);
   Generic_storage_class_init(D);
   Generic_storage_class_init(I32);
   Generic_storage_class_init(I16);
   Generic_storage_class_init(I8);
   Generic_storage_class_init(U8);
   Generic_storage_class_init(GPTR);

   dx_define("new-storage", xnew_storage);
   dx_define("storage-malloc",xstorage_malloc);
   dx_define("storage-realloc",xstorage_realloc);
   dx_define("storage-clear",xstorage_clear);
#ifdef HAVE_MMAP
   dx_define("storage-mmap",xstorage_mmap);
#endif
   dx_define("storagep",xstoragep);
   dx_define("storage-classp", xstorage_classp);
   dx_define("storage-readonlyp",xstorage_readonlyp);
   dx_define("storage-make-readonly", xstorage_make_readonly);
   dx_define("storage-load",xstorage_load);
   dx_define("storage-save",xstorage_save);
   dx_define("storage-nelems",xstorage_nelems);
   dx_define("storage-nbytes",xstorage_nbytes);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
