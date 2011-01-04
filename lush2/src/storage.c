/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann LeCun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as 
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *  MA  02110-1301  USA
 *
 ***********************************************************************/

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
#include "dh.h"

#include <errno.h>
#include <inttypes.h>
#ifdef HAVE_MMAP
# include <sys/mman.h>
#endif


static void clear_storage(storage_t *st, size_t _)
{
   st->data = NULL;
   st->backptr = NULL;
}

static void mark_storage(storage_t *st)
{
   MM_MARK(st->backptr);
   if (st->kind == STS_MANAGED)
      MM_MARK(st->data);
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
   sizeof(bool),
   sizeof(char),
   sizeof(unsigned char),
   sizeof(short),
   sizeof(int),
   sizeof(float),
   sizeof(double),
   sizeof(gptr),
   sizeof(mptr),
   sizeof(at*),
};


static float at_getf(gptr pt, size_t off)
{
   at *p = ((at **)pt)[off];
   ifn (NUMBERP(p))
      error(NIL, "accessed element is not a number", p);
   return Number(p);
}

static float gptr_getf(gptr pt, size_t off)
{
   error(NIL, "accessed element is not a number", NIL);
}

static void at_setf(gptr pt, size_t off, float x)
{
   ((at **)pt)[off] = NEW_NUMBER(x);
}

static void gptr_setf(gptr pt, size_t off, float x)
{
   error(NIL, "gptr arrays do not contain numbers", NIL);
}

#define Generic_getf_setf(type)                           \
   static float type##_getf(gptr pt, size_t off)          \
   {                                                      \
      return (float)(((type *)pt)[off]);                  \
   }                                                      \
                                                          \
   static void type##_setf(gptr pt, size_t off, float x)  \
   {                                                      \
      ((type *)pt)[off] = x;                              \
   }

Generic_getf_setf(bool)
Generic_getf_setf(char)
Generic_getf_setf(uchar)
Generic_getf_setf(short)
Generic_getf_setf(int)
Generic_getf_setf(float)
Generic_getf_setf(double)
#undef Generic_getf_setf

float (*storage_getf[ST_LAST])(gptr, size_t) = {
   bool_getf,
   char_getf,
   uchar_getf,
   short_getf,
   int_getf,
   float_getf,
   double_getf,
   gptr_getf,
   gptr_getf,  /* ST_MPTR */
   at_getf,
};

void (*storage_setf[ST_LAST])(gptr, size_t, float) = {
   bool_setf,
   char_setf,
   uchar_setf,
   short_setf,
   int_setf,
   float_setf,
   double_setf,
   gptr_setf,
   gptr_setf,  /* ST_MPTR */
   at_setf
};


static double at_getd(gptr pt, size_t off)
{
   at *p = ((at **)pt)[off];
   ifn (p && NUMBERP(p))
      error(NIL, "accessed element is not a number", NIL);
   return Number(p);
}

static double gptr_getd(gptr pt, size_t off)
{
   error(NIL, "accessed element is not a number", NIL);
}

static void at_setd(gptr pt, size_t off, double x)
{
   ((at **)pt)[off] = NEW_NUMBER(x);
}

static void gptr_setd(gptr pt, size_t off, double x)
{
   error(NIL, "gptr arrays do not contain numbers", NIL);
}

#define Generic_getd_setd(type)                            \
   static double type##_getd(gptr pt, size_t off)          \
   {                                                       \
      return (double)(((type *)pt)[off]);                  \
   }                                                       \
                                                           \
   static void type##_setd(gptr pt, size_t off, double x)  \
   {                                                       \
      ((type *)pt)[off] = x;                               \
   }

Generic_getd_setd(bool)
Generic_getd_setd(char)
Generic_getd_setd(uchar)
Generic_getd_setd(short)
Generic_getd_setd(int)
Generic_getd_setd(float)
Generic_getd_setd(double)
#undef Generic_getd_setd

double (*storage_getd[ST_LAST])(gptr, size_t) = {
   bool_getd,
   char_getd,
   uchar_getd,
   short_getd,
   int_getd,
   float_getd,
   double_getd,
   gptr_getd,
   gptr_getd,  /* ST_MPTR */
   at_getd
};

void (*storage_setd[ST_LAST])(gptr, size_t, double) = {
   bool_setd,
   char_setd,
   uchar_setd,
   short_setd,
   int_setd,
   float_setd,
   double_setd,
   gptr_setd,
   gptr_setd,  /* ST_MPTR */
   at_setd
};


static at *Number_getat(storage_t *st, size_t off)
{
   double (*get)(gptr,size_t) = storage_getd[st->type];
   return NEW_NUMBER( (*get)(st->data, off) );
}

static at *gptr_getat(storage_t *st, size_t off)
{
   gptr *pt = st->data;
   return NEW_GPTR(pt[off]);
}

static at *mptr_getat(storage_t *st, size_t off)
{
   gptr *pt = st->data;
   return NEW_MPTR(pt[off]);
}

static at *at_getat(storage_t *st, size_t off)
{
   at **pt = st->data;
   at *p = pt[off];
   return p;
}

at *(*storage_getat[ST_LAST])(storage_t *, size_t) = {
   Number_getat,
   Number_getat,
   Number_getat,
   Number_getat,
   Number_getat,
   Number_getat,
   Number_getat,
   gptr_getat,
   mptr_getat,
   at_getat
};


void get_write_permit(storage_t *st)
{
   if (st->isreadonly)
      error(NIL, "read-only storage", NIL);
}

static void Number_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   ifn (NUMBERP(x))
      error(NIL, "not a number", x);
   void (*set)(gptr,size_t,real) = storage_setd[st->type];
   (*set)(st->data, off, Number(x));
}

static void gptr_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   ifn (GPTRP(x))
      error(NIL, "not a gptr", x);
   gptr *pt = st->data;
   pt[off] = Gptr(x);
}

static void mptr_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   ifn (MPTRP(x))
      error(NIL, "not an mptr", x);
   gptr *pt = st->data;
   pt[off] = Mptr(x);
}

static void at_setat(storage_t *st, size_t off, at *x)
{
   get_write_permit(st);
   at **pt = st->data;
   pt[off] = x;
}

void (*storage_setat[ST_LAST])(storage_t *, size_t, at *) = {
   Number_setat,
   Number_setat,
   Number_setat,
   Number_setat,
   Number_setat,
   Number_setat,
   Number_setat,
   gptr_setat,
   mptr_setat,
   at_setat
};


/* ------- THE CLASS FUNCTIONS ------ */

static const char *storage_name(at *p)
{
   storage_t *st = (storage_t *)Mptr(p);
   char *kind = "";
   switch (st->kind) {
   case STS_NULL:    kind = "unallocated"; break;
   case STS_MANAGED: kind = "managed"; break;
   case STS_FOREIGN: kind = "foreign"; break;
   case STS_MMAP:    kind = "mmap"; break;
   case STS_STATIC:  kind = "static"; break;
   default:
      fprintf(stderr, "internal error: invalid storage kind");
      abort();
   }
   const char *clname = nameof(Symbol(Class(p)->classname));
   sprintf(string_buffer, "::%s:%s@%p:<%"PRIdPTR">", clname, kind, st->data, st->size);
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
   
   ssize_t off = Number(Car(q));
   if (off<0 || off>=st->size)
      error(NIL, "subscript out of range", q);
   
   if (Cdr(q)) {
      ifn (CONSP(Cdr(q)) && !Cddr(q))
         error(NIL, "one or two arguments expected",q);
      storage_setat[st->type](st, off, Cadr(q));
      return st->backptr;
   } else
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
   int type, kind;
   size_t size;

   if (code != SRZ_READ) {
      st = Mptr(*pp);
      type = (int)st->type;
      kind = (int)st->kind;
      size = st->size;
   }
   // Read/write basic info
   serialize_int(&type, code);
   serialize_int(&kind, code);
   serialize_size(&size, code);

   // Create storage if needed
   if (code == SRZ_READ) {
      st = new_storage_managed((storage_type_t)type, size, NIL);
      *pp = st->backptr;
   }

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
         if (magic == STORAGE_SWAPPED)
            swap_buffer(st->data, size, storage_sizeof[type]);
         else if (magic != STORAGE_NORMAL)
            RAISEF("Corrupted binary file",NIL);
      }
   }
}

storage_t *new_storage(storage_type_t t)
{
   storage_t *st = mm_alloc(mt_storage);
   st->data = NULL;
   st->size = 0;
   st->type = t;
   st->kind = STS_NULL;
   st->isreadonly = false;
   st->backptr = new_at(storage_class[st->type], st);
   return st;
}

DX(xnew_storage) 
{
   ARG_NUMBER(1);
   storage_type_t t = dht_from_cname(ASYMBOL(1));
   ifn (t < ST_LAST)
      RAISEF("not a storage class", APOINTER(1));
   return new_storage(t)->backptr;
}


storage_t *new_storage_managed(storage_type_t t, size_t n, at *init)
{
   storage_t *st = new_storage(t);
   if (n) storage_alloc(st, n, init);
   return st;
}

DX(xnew_storage_managed)
{
   if (arg_number<2 || arg_number>3)
      ARG_NUMBER(-1);

   storage_type_t t = dht_from_cname(ASYMBOL(1));
   ifn (t < ST_LAST)
      RAISEF("not a storage class", APOINTER(1));
   int n = AINTEGER(2);
   at *init = NIL;
   if (arg_number == 3) {
      if (t <= ST_DOUBLE)
         ADOUBLE(3);
      init = APOINTER(3);
   }
   return new_storage_managed(t, n, init)->backptr;
}


storage_t *new_storage_foreign(storage_type_t t, size_t n, void *p, bool readonly)
{
   storage_t *st = mm_alloc(mt_storage);
   st->type = t;
   st->kind = STS_FOREIGN;
   st->isreadonly = readonly;
   st->size = n;
   st->data = (void *)p;
   st->backptr = new_at(storage_class[st->type], st);
   return st;
}

DX(xnew_storage_foreign)
{
   if (arg_number<3 || arg_number>4)
      ARG_NUMBER(-1);

   storage_type_t t = dht_from_cname(ASYMBOL(1));
   ifn (t < ST_LAST)
      RAISEF("not a storage class", APOINTER(1));
   int n = AINTEGER(2);
   void *p = AGPTR(3);
   bool readonly = (arg_number>3) ? (APOINTER(4)!=NIL) : true;
   
   return new_storage_foreign(t, n, p, readonly)->backptr;
}


/* new storage with <n> elements of type <t> at <p> */
storage_t *new_storage_static(storage_type_t t, size_t n, const void *p)
{
   storage_t *st = mm_alloc(mt_storage);
   st->type = t;
   st->kind = STS_STATIC;
   st->isreadonly = true;
   st->size = n;
   st->data = (void *)p;
   st->backptr = new_at(storage_class[st->type], st);
   return st;
}

#ifdef HAVE_MMAP

static void storage_notify(storage_t *st, void *_)
{
   if (st->kind == STS_MMAP) {
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
}

storage_t *new_storage_mmap(storage_type_t t, FILE *f, size_t offs, bool ro)
{
   storage_t *st = mm_allocv(mt_storage, sizeof(storage_t));
   errno = 0;
#if HAVE_FSEEKO
   if (fseeko(f,(off_t)0,SEEK_END)==-1)
      test_file_error(NULL, errno);
#else
   if (fseek(f,0,SEEK_END)==-1)
      test_file_error(NULL, errno);
#endif
#if HAVE_FTELLO
   size_t len = (size_t)ftello(f);
#else
   size_t len = (size_t)ftell(f);
#endif
   rewind(f);
   if (t==ST_MPTR || t==ST_GPTR)
      RAISEF("cannot mmap a pointer storage", st->backptr);
   if (t==ST_AT)
      RAISEF("cannot mmap an atom-storage", st->backptr);
#ifdef UNIX
   errno = 0;
   gptr addr = mmap(0,len,(ro ? PROT_READ : PROT_WRITE),MAP_SHARED,fileno(f),0);
   if (addr == (void*)-1L)
      test_file_error(NULL, errno);
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
   st->type = t;
   st->kind = STS_MMAP;
   st->isreadonly = ro;
   st->mmap_len = len;
   st->mmap_addr = addr;
   st->size = (len - offs) / storage_sizeof[st->type];
   st->data = (char *)(st->mmap_addr)+offs;
   st->backptr = new_at(storage_class[st->type], st);
   add_notifier(st, (wr_notify_func_t *)storage_notify, NULL);
   return st;
}

DX(xnew_storage_mmap)
{
   if (arg_number<2 || arg_number>4)
      ARG_NUMBER(-1);

   storage_type_t t = dht_from_cname(ASYMBOL(1));
   ifn (t < ST_LAST)
      RAISEF("not a storage class", APOINTER(1));
   size_t offset = (arg_number>2) ? AINTEGER(3) : 0;
   bool readonly = (arg_number>3) ? (APOINTER(4)!=NIL) : true;

   at *atf = APOINTER(2);
   if (RFILEP(atf) && !readonly) {
      RAISEF("file not writeable", atf);

   } else if (STRINGP(atf)) {
      atf = readonly ? OPEN_READ(String(atf), NULL) : OPEN_WRITE(String(atf), NULL);
   } else
      RAISEF("neither a string nor a file descriptor", atf);
   
   return new_storage_mmap(t, Gptr(atf), offset, readonly)->backptr;
}

#endif // HAVE_MMAP

/* ------------ ALLOCATION: MALLOC ------------ */

void storage_alloc(storage_t *st, size_t n, at *init)
{
   char _errmsg[200], *errmsg = &_errmsg[0];

   ifn (st->data == NULL)
      RAISEF("storage must be unsized", st->backptr);
   
   /* allocate memory and initialize srg */
   size_t s = n*storage_sizeof[st->type];
   void *data = NULL;
   if (st->type==ST_AT || st->type==ST_MPTR)
      data = mm_malloc(mt_refs, s);
   else
      data = mm_malloc(mt_blob, s);

   if (!data) {
      sprintf(errmsg, "not enough memory for storage of size %.1f MByte", ((double)s)/(2<<20));
      RAISEF(errmsg, NIL);
   }
   st->data = data;
   st->kind = STS_MANAGED;
   st->size  = n;
   
   /* clear gptr storage data (ATs and MPTRs are cleared by mm) */
   if (init && n>0)
      storage_clear(st, init, 0);

   else if (st->type == ST_GPTR) {
      gptr *pt = st->data;
      for (int i=0; i<n; i++) 
         pt[i] = NULL;
   }
}

DX(xstorage_alloc)
{
   ARG_NUMBER(3);
   int n = AINTEGER(2);
   if (n<0)
      RAISEFX("invalid size", NEW_NUMBER(n));
   if (n) storage_alloc(ASTORAGE(1), n, APOINTER(3));
   return NIL;
}

void storage_realloc(storage_t *st, size_t size, at *init)
{
   if (size < st->size)
      RAISEF("storage size cannot be reduced", st->backptr);
   
   size_t s = size*storage_sizeof[st->type];
   size_t olds = st->size*storage_sizeof[st->type];
   gptr olddata = st->data;

   if (st->kind == STS_NULL) {
      /* empty storage */
      assert(st->data == NULL);
      storage_alloc(st, size, init);
      return;

   } else {
      /* reallocate memory and update srg */
      if (st->kind == STS_MANAGED)
         MM_ANCHOR(olddata);
      if (st->type==ST_AT || st->type==ST_MPTR)
         st->data = mm_allocv(mt_refs, s);
      else 
         st->data = mm_blob(s);
      
      if (st->data) {
         memcpy(st->data, olddata, olds);
         st->kind = STS_MANAGED;
      }
   }
   
   if (st->data == NULL) {
      st->data = olddata;
      RAISEF("not enough memory", NIL);
   }
   
   size_t oldsize = st->size;
   st->size = size;
   
   if (init) {
      /* temporarily clear read only flag to allow initialization */
      bool isreadonly = st->isreadonly;
      storage_clear(st, init, oldsize);
      st->isreadonly = isreadonly;
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

void storage_clear(storage_t *st, at *init, size_t from)
{
   /* don't need to check read-only status here because 
      it will be checked by the setat function below */
   int size = st->size;
   if (from>=size)
      RAISEF("invalid value for 'from'", NEW_NUMBER(from));
   
   /* clear from from to to */
   if (st->type == ST_AT) {
      for (int off = from; off < size; off++)
         (storage_setat[st->type])(st, off, init);
      
   } else if (storage_setat[st->type] == Number_setat) {
      get_write_permit(st);
      void (*set)(gptr, size_t, real) = storage_setd[st->type];
      for (int off = from; off < size; off++)
         set(st->data, off, Number(init));

   } else if (storage_setat[st->type] == gptr_setat) {
      get_write_permit(st);
      gptr *pt = st->data;
      for (int off=from; off<size; off++)
         pt[off] = Gptr(init);

   } else if (storage_setat[st->type] == mptr_setat) {
      get_write_permit(st);
      gptr *pt = st->data;
      for (int off=from; off<size; off++)
         pt[off] = Mptr(init);
   } else
      RAISEF("don't know how to clear this storage", st->backptr);
}

DX(xstorage_clear)
{
   ARG_NUMBER(2);
   storage_clear(ASTORAGE(1), APOINTER(2), 0);
   return NIL;
}


/* ============== Lush FUNCTIONS ================= */

DX(xstoragep)
{
   ARG_NUMBER(1);
   return STORAGEP(APOINTER(1)) ? t() : NIL;
}

bool storage_readonlyp(const storage_t *st) 
{
   return st->isreadonly;
}

DX(xstorage_readonlyp)
{
   ARG_NUMBER(1);
   storage_t *st = ASTORAGE(1);;
   return st->isreadonly ? t() : NIL;
}

DX(xstorage_set_readonly)
{
   ARG_NUMBER(1);
   storage_t *st = ASTORAGE(1);
   st->isreadonly = true;
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
      errno = 0;
      if (fseeko(f,0,SEEK_END)==-1)
         test_file_error(NULL, errno);
      off_t len = ftello(f);
      errno = 0;
      if (fseeko(f,here,SEEK_SET)==-1)
         test_file_error(NULL, errno);
#else
      off_t here = ftell(f);
      errno = 0;
      if (fseek(f,0,SEEK_END)==-1)
         test_file_error(NULL, errno);
      int len = ftell(f);
      errno = 0;
      if (fseek(f,here,SEEK_SET)==-1)
         test_file_error(NULL, errno);
#endif
      if (len==0) 
         return;
      else
         storage_alloc(st,(size_t)len/storage_sizeof[st->type],0);
   }
   
   get_write_permit(st);
   char *pt = st->data;
   errno = 0;
   int nrec = fread(pt, storage_sizeof[st->type], st->size, f);
   if (nrec < st->size)
      RAISEF("file is too small",NIL);
   test_file_error(f, errno);
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
   errno = 0;
   int nrec = fwrite(pt, storage_sizeof[st->type], st->size, f);
   test_file_error(f, errno);
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

class_t *abstract_storage_class;
class_t *storage_class[ST_LAST];

#define Generic_storage_class_init(st, lname)                       \
   (storage_class[st]) = new_builtin_class(abstract_storage_class); \
   (storage_class[st])->name = storage_name;                        \
   (storage_class[st])->listeval = storage_listeval;                \
   (storage_class[st])->serialize = storage_serialize;              \
   class_define(#lname "Storage", (storage_class[st]))


void init_storage()
{
   assert(ST_FIRST==0);
   assert(sizeof(char)==sizeof(uchar));

#ifdef HAVE_MMAP                           
   size_t storage_size = offsetof(storage_t, mmap_addr);
#else
   size_t storage_size = sizeof(storage_t);
#endif

   mt_storage = MM_REGTYPE("storage", storage_size,
                           clear_storage, mark_storage, 0);

   /* set up storage_classes */
   abstract_storage_class = new_builtin_class(NIL);
   class_define("storage", abstract_storage_class);
   Generic_storage_class_init(ST_BOOL, Bool);
   Generic_storage_class_init(ST_AT, Atom);
   Generic_storage_class_init(ST_FLOAT, Float);
   Generic_storage_class_init(ST_DOUBLE, Double);
   Generic_storage_class_init(ST_INT, Int);
   Generic_storage_class_init(ST_SHORT, Short);
   Generic_storage_class_init(ST_CHAR, Char);
   Generic_storage_class_init(ST_UCHAR, UChar);
   Generic_storage_class_init(ST_GPTR, Gptr);
   Generic_storage_class_init(ST_MPTR, Mptr);

   at *p = var_define("storage-classes");
   at *l = NIL;
   for (storage_type_t st=ST_FIRST; st<ST_LAST; st++)
      l = new_cons(storage_class[st]->backptr, l);
   var_set(p, reverse(l));
   var_lock(p);

   dx_define("new-storage", xnew_storage);
   dx_define("new-storage/managed", xnew_storage_managed);
   dx_define("new-storage/foreign", xnew_storage_foreign);
#ifdef HAVE_MMAP
   dx_define("new-storage/mmap",xnew_storage_mmap);
#endif
   dx_define("storage-alloc",xstorage_alloc);
   dx_define("storage-realloc",xstorage_realloc);
   dx_define("storage-clear",xstorage_clear);
   dx_define("storagep",xstoragep);
   dx_define("storage-readonlyp",xstorage_readonlyp);
   dx_define("storage-set-readonly", xstorage_set_readonly);
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
