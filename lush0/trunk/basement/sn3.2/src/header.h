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

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/
  

#ifndef HEADER_H
#define HEADER_H

#ifndef DEFINE_H
#include "define.h"
#endif

#ifndef FLTLIB_H
#include "fltlib.h"
#endif

/* MISC.H ------------------------------------------------------ */


#define ifn(s)          if(!(s))
#define forever         for(;;)
#define until(s)        while(!(s))

/* generic pointer */
typedef void* gptr;


/* AT.H --- EVAL.H -------------------------------------------- */

/* struct at is the CONS structure */

typedef struct at {
  unsigned int count;
  unsigned short flags;
  unsigned short ctype;
  struct oostruct *slots;
  union {
    struct {			/* cons */
      struct at *at_car, *at_cdr;
    } at_cons;

    real at_number;		/* number */

    gptr at_gptr;		/* a generic pointer */

    struct {			/* external */
      ptr at_object;
      struct class *at_class;
    } at_extern;

  } at_union;
} at;

/* shorthands */

#define Cdr     at_union.at_cons.at_cdr
#define Car     at_union.at_cons.at_car
#define Number  at_union.at_number
#define Gptr	at_union.at_gptr
#define Object  at_union.at_extern.at_object
#define Class   at_union.at_extern.at_class
#define Slots   slots


/* flags: 
 *  o- define which branch of the UNION, 
 *  o- Garbage stuffs 
 */

#define C_CONS          1	/* It's a  CONS */
#define C_NUMBER        2	/* It's a  NUMBER */
#define C_EXTERN        4	/* It's a (CLASS INSTANCE) pair */
#define C_ZOMBIE        8       /* It's a ZOMBIE object */
#define C_GPTR		16      /* It's a POINTER, just an address */

#define C_XREF          64  	/* General purpose flag */
#define C_GARBAGE       128  	/* Garbage Collector flag */

enum ctypes {

  XT_OBJECT, 
  XT_CLASS, 

  XT_CONS, 
  XT_NUMBER, 
  XT_GPTR, 
  XT_SYMBOL, 
  XT_STRING,

  XT_FILE_RO, 
  XT_FILE_WO,

  XT_DE, 
  XT_DF, 
  XT_DM, 
  XT_DX, 
  XT_DY, 
  XT_DZ,

  /* compiled functions, class and objects */
  XT_DH,
  XT_COBJECT,
  XT_CCLASS,

  /* compiled constant */
  XT_CINDEX, 
  XT_CF,
  XT_CD,
  XT_CI32,
  XT_CI16,
  XT_CI8,
  XT_CU8,

  XT_ROBJECT, 
  XT_RTABLE, 
  XT_RCALL,

  XT_WINDOW,

  XT_ATSTORAGE,
  XT_PSTORAGE,
  XT_FSTORAGE,
  XT_DSTORAGE,
  XT_I32STORAGE,
  XT_I16STORAGE,
  XT_I8STORAGE,
  XT_U8STORAGE,

  XT_INDEX,
  XT_DB,
  XT_HASH,
  XT_GP2,
  XT_GPTRSTORAGE,

/* never delete this one */
  XT_LAST
};


/* Some useful macros */

#define LOCK(x)         { if (x) (x)->count++; }
#define UNLOCK(x)       { if ( (x) && --((x)->count)<=0 ) purge(x); }

#define CONSP(x)        ((x) && ((x)->flags & C_CONS))
#define LISTP(x)        (!(x) || ((x)->flags & C_CONS))

/* The evaluator hooks */

extern at *(*eval_ptr) ();
extern at *(*argeval_ptr) ();


/*
 * CONS vs. NEW_CONS NEW_CONS uses normal arguments lock convention. CONS
 * assumes that his arguments are overlocked by one. Typically the expression
 * cons( NEW_NUMBER(2), new_cons(e,l) ) needs no UNLOCK statement
 */


#define NEW_NUMBER(x)   new_number((real)(x))
#define NEW_GPTR(x)	new_gptr((gptr)(x))
#define eval(q)         (*eval_ptr)(q)

/*
 * class: is the generic structure for external objects of same type. the
 * question is   "What_to_do_if"
 */

typedef struct class class;

struct class {
  void (*self_dispose) ();	/* called by PURGE */
  void (*self_action) ();	/* called by GARBAGE */
  char *(*self_name) ();	/* called by PNAME */
  at *(*self_eval) ();		/* called by EVAL */
  at *(*list_eval) ();		/* called by EVAL of ( me ... ) */
  
  unsigned char ctype;          /* the ctype of the object */
  unsigned char flags;          /* CL_XXX flags */

  short 	slotssofar;    
  at           *backptr;
  class        *super;

  at           *classname;
  at           *keylist;
  at           *defaults;
  at           *atsuper;
  at           *hashmethod[HASHMETHODS];

  at           *at_cclass;
  at           *source;
};

#define CL_GOAWAY 	1
#define CL_NODELETE	2

extern int length();
extern at *eval_std(), *eval_debug(), *apply(), *progn(), *car(), *cdr();
extern at *member(), *nth(), *nthcdr(), *last(), *lastcdr();
extern at *flatten(), *append(), *reverse();
extern at *new_number(), *new_gptr(), *new_extern(), *new_cons(), *cons();


/* ALLOC.H ---------------------------------------------------- */



struct empty_alloc {		/* skeleton structure used by the alloc
				 * routines.  skeleton structure used by the
				 * alloc routines. */
  int used, flags;
  struct empty_alloc *next;
};

struct chunk_header {		/* this structure describe each element
				 * chunk.     this structure describe each
				 * element chunk. */
  struct chunk_header *next;
  ptr begin;
  ptr end;
  ptr pad;			/* too clever compiler may cause problems */
};

struct alloc_root {		  /* this structure contains the allocation
				   * infos */
  struct empty_alloc *freelist;	  /* List of free elems */
  struct chunk_header *chunklist; /* List of active chunkes */
  int elemsize;			  /* Size of one elems */
  int chunksize;		  /* Number of elems in one chunk */
};


extern ptr allocate();
extern void deallocate();
extern void garbage(), unflag(), mark(), don_t_purge();
extern void protect(), unprotect();


/*
 * Following, a very general iterator. A loop on all the used elements of a
 * ALLOC_BASE structure may be written just as
 * 
 * { register struct chunk_header *current_chunk; register struct xxxx
 * *looping_var; iter_on( &xxxx_alloc, current_chunk, looping_var ) { Let's
 * go } }
 */
#define iter_on(base,i,elem)                                                   \
 for(i=(base)->chunklist; i; i = i->next )                                     \
 for(elem=i->begin;(ptr)elem<i->end;elem=(ptr)((char*)elem+(base)->elemsize))\
 if ( ((struct empty_alloc *) elem) -> used )



/* malloc debug file */

#define malloc(x)    sn3_malloc(x,__FILE__,__LINE__)
#define calloc(x,y)  sn3_calloc(x,y,__FILE__,__LINE__)
#define realloc(x,y)  sn3_realloc(x,y,__FILE__,__LINE__)
#define free(x)      sn3_free(x,__FILE__,__LINE__)
#define cfree(x)     sn3_cfree(x,__FILE__,__LINE__)
void *sn3_malloc(), *sn3_calloc(), *sn3_realloc();
void sn3_free(), sn3_cfree();
void set_malloc_file();


/* stack alloc (certain machine don't understand alloca) */

#ifndef NOALLOCA
#define stack_alloc(v) alloca(v)
#define stack_free(v)
#else
#define stack_alloc(v) malloc(v)
#define stack_free(v) free(v)
#endif





/* SYMBOL.H ---------------------------------------------------- */


struct hash_name {		/* this one contains the symbol's names this
				 * one contains the symbol's names */
  short used;
  char name[NAMELENGTH];
  at *named;
  struct hash_name *next;

};

/* now the hash table declaration */
extern struct hash_name *names[];

/*
 * Iterator on the hash_name nodes. usage {   struct hash_name **i; struct
 * hash_name *hn; iter_hash_name(i,hn) Let's go }
 */
#define iter_hash_name(i,hn) \
  for (i=names; i<names+HASHTABLESIZE; i++) \
  for (hn= *i; hn; hn = hn->next )          \
  if (hn->used)


extern class symbol_class;

struct symbol {			/* each symbol is an external AT which */
  short mode, nopurge;		/* points to this structure */
  struct symbol *next;
  struct hash_name *name;
  at *value;
  at **valueptr;
};

#define SYMBOL_UNUSED   0
#define SYMBOL_LOCKED   2
#define SYMBOL_UNLOCKED 1

extern at *new_symbol(), *named();
extern char *nameof();
extern at *symblist();
extern at *true();
extern at *getprop();
extern void putprop();

extern void var_set(), var_lock();
extern at *var_get(), *var_define();

/* PSEUDO_IO.H ------------------------------------------------- */

extern int st_write(), st_read(), st_seek(), st_close();
extern int st_flush(), fclose(), st_fputs(), st_fgetc(), st_fputc();
extern int st_ferror(), st_feof(), FPRINTF(), st_scanf1(), st_ungetc();
extern int myfwrite(), st_fileno();
extern void st_rewind(),st_clearerr(); 
extern void st_test_file_error();
extern long st_tell();
extern char *read_token();

typedef struct IO_METHODS {
	int (*this_fwrite)();	
	int (*this_fread)(); 
	int (*this_fseek)();
	void (*this_rewind)();
	long (*this_ftell)();
	int (*this_fclose)();
	int (*this_fflush)();
	int (*this_fputs)();
	int (*this_fgetc)();
	int (*this_fputc)();
	int (*this_ferror)();
	void (*this_test_file_error)();
	int (*this_feof)();
	int (*this_fscanf)();
	int (*this_ungetc)();
	void (*this_clearerr)();
	int (*this_fileno)();
} io_methods;

#define REAL_FILE 1
#define PSEUDO_FILE 0

typedef struct PSEUDO_FILE_IOBUF {
    at *st_at;    /* pointer to at that points to storage where data is stored */
    long pos;     /* offset within storage (current position, returned by ftell) */
    char mode;    /* "r" (reading), "w" (writing), "a" (append) */
    bool eof;
} pfiob;

typedef struct GENERAL_FILE {
    union {
        FILE *fp;
        pfiob *pfp;
    } stream;
    bool type;                /* 0 = pseudo-file, 1 = real file */
    io_methods *methods;      /* a table of functions (pseudo_io.c) */
} File;

#define PFWRITE(source, n, size, pf)  \
	(*(((File *)pf)->methods->this_fwrite))(source, n, size, ((File *)pf)->stream.fp)
#define PFREAD(dest, n, size, pf) \
	(*(((File *)pf)->methods->this_fread))(dest, n, size, ((File *)pf)->stream.fp)
#define FSEEK(pf, offset, ptrname) \
	(*(((File *)pf)->methods->this_fseek))(((File *)pf)->stream.fp, offset, ptrname)
#define REWIND(pf) \
	(*(((File *)pf)->methods->this_rewind))(((File *)pf)->stream.fp)
#define FTELL(pf) \
	(*(((File *)pf)->methods->this_ftell))(((File *)pf)->stream.fp)
#define FCLOSE(pf) \
	(*(((File *)pf)->methods->this_fclose))(((File *)pf))
#define FFLUSH(pf) \
	(*(((File *)pf)->methods->this_fflush))(((File *)pf)->stream.fp)
#define FPUTS(s, pf) \
	(*(((File *)pf)->methods->this_fputs))(s, ((File *)pf)->stream.fp)
#define FGETC(pf) \
	(*(((File *)pf)->methods->this_fgetc))(((File *)pf)->stream.fp)
#define FPUTC(c, pf) \
	(*(((File *)pf)->methods->this_fputc))(c, ((File *)pf)->stream.fp)
#define FERROR(pf) \
	(*(((File *)pf)->methods->this_ferror))(((File *)pf)->stream.fp)
#define TEST_FILE_ERROR(pf) \
	(*(((File *)pf)->methods->this_test_file_error))(((File *)pf)->stream.fp)
#define FEOF(pf) \
        (*(((File *)pf)->methods->this_feof))(((File *)pf)->stream.fp)
#define FSCANF1(pf, format, addr) \
	(*(((File *)pf)->methods->this_fscanf))(((File *)pf)->stream.fp, format, addr)
#define UNGETC(c, pf) \
        (*(((File *)pf)->methods->this_ungetc))(c, ((File *)pf)->stream.fp)
#define CLEARERR(pf) \
        (*(((File *)pf)->methods->this_clearerr))(((File *)pf)->stream.fp)
#define FILENO(pf) \
	(*(((File *)pf)->methods->this_fileno))(((File *)pf)->stream.fp)

extern File *real_fopen(), *pseudo_fopen(), *new_real_File();

/* TOPLEVEL.H ------------------------------------------------- */

extern struct error_doc {	/* it contains all the useful infos */
  at *this_call;                /* for printing error messsages */
  char *error_prefix;
  char *error_text;
  at *error_suffix;
  short debug_tab;
  short ready_to_an_error;
  short script_mode;
  File *script_file;
} error_doc;

#define SCRIPT_OFF      0
#define SCRIPT_INPUT    1
#define SCRIPT_OUTPUT   2

/*
 * This structure contains - set_jmp info used by exception propagation. -
 * i/o streams description and variables. They are pushed on a linked stack
 * by lisp level i/o functions and by the 'toplevel' or 'load' function. So
 * these functions  are able to redefine 'error_jump' and to perform some
 * cleanup after an error
 */

extern struct context {
  struct context *next;
  jmp_buf error_jump;
  File *input_file;
  File *output_file;
  short input_tab;
  short output_tab;
} *context;


extern void context_push(), context_pop();
extern void toplevel();
extern void error(), print_error();
extern void start_lisp();

extern char breakdebug[];
extern char breaksimple[];

/* STRING.H ---------------------------------------------------- */

extern class string_class;

/*
 * Coarse implementation: A string is an external at pointing to this
 * structure. 'flag' indicates that we have called 'malloc' to allocate the
 * string space and we ought to 'free' it at purge time  (STRING_ALLOCATED)
 * or no (STRING_SAFE).
 */

struct string {
  int flag;
  int pad;			/* 12 bytes required by (DE)ALLOCATE */
  char *start;
  void *cptr;
};

#define STRING_UNUSED           0
#define STRING_SAFE             1
#define STRING_ALLOCATED        2

#define SADD(str)       (((struct string *)(str))->start)

extern at *new_string(), *new_safe_string(), *str_val();
extern char *index(), *rindex();
extern char *str_left(), *str_right(), *str_mid();
extern char *str_concat(), *str_number(), *str_pointer();

extern char *string_buffer;
extern char null_string[], digit_string[], special_string[], aspect_string[];


/* FUNCTION.H -------------------------------------------------- */

/*
 * function are implemented as external ATs pointing to this union
 */

typedef struct l_object_type {
    int pad;			/* 12 bytes required by (DE)ALLOCATE */
    at *formal_arg_list;
    at *evaluable_list;
} l_object_type;

typedef struct c_object_type {
    int pad;			/* 12 bytes required by (DE)ALLOCATE */
    void *address;              /* pointer on C object */
    int kind;                   /* State determined by the dynamic loader */
} c_object_type;

union function {
    l_object_type lfunc;
    c_object_type cfunc;
};

#define COBJECT_STANDARD       0	/* Standard C function */
#ifdef DLD
#define COBJECT_DYNLINKED      1	/* Dynamically linked */
#define COBJECT_DYNUNLINKED    2	/* Dynamically linked, then unlinked */
#define COBJECT_DYNUNREFRED    3	/* Dyn. linked, no longer refered */
#endif

extern class de_class;
extern class df_class;
extern class dm_class;
extern class dx_class;		/* dx functions are external C_function */
extern class dy_class;		/* dy functions have unflattened args. */


extern at **dx_stack, **dx_sp;
extern int functionp();
extern at *new_de(), *new_df(), *new_dm(), *new_dx(), *new_dy();
extern at *funcdef(), *eval_a_list();
extern void dx_define(), dy_define();
extern ptr need_error();
extern void arg_eval(), all_args_eval();


/* This is used by the DLD stuff */

#ifdef DLD
extern int doing_dynlink;
#endif

/* This is the interface header builder */

#define DX(Xname)       at *Xname(arg_number,arg_array)         \
                        int arg_number;                         \
                        at *arg_array[];	/* WARNING base 1 */

#define DY(Yname)       at *Yname(ARG_LIST)                     \
                        at *ARG_LIST;


/* Macros and functions used in argument transmission in DX functions */


#define ISNUMBER(i)	( arg_array[i] && (arg_array[i]->ctype==XT_NUMBER) )
#define ISGPTR(i)	( arg_array[i] && (arg_array[i]->ctype==XT_GPTR) )
#define ISLIST(i)	( !arg_array[i] || (arg_array[i]->ctype==XT_CONS) )
#define ISCONS(i)	( arg_array[i] && (arg_array[i]->ctype==XT_CONS) )
#define ISSTRING(i)	( arg_array[i] && (arg_array[i]->ctype==XT_STRING) )
#define ISSYMBOL(i)	( arg_array[i] && (arg_array[i]->ctype==XT_SYMBOL) )
#define ISOBJECT(i) 	( arg_array[i] && (arg_array[i]->flags & C_EXTERN) )
#define ISINDEX(i)      ( arg_array[i] && (arg_array[i]->ctype==XT_INDEX) )
#define ISDB(i)      ( arg_array[i] && (arg_array[i]->ctype==XT_DB) )
#define ISHASH(i)      ( arg_array[i] && (arg_array[i]->ctype==XT_HASH) )
#define DX_ERROR(i,j)   ( need_error(i,j,arg_array))

#define APOINTER(i)     ( arg_array[i] )
#define AREAL(i)        ( ISNUMBER(i) ? APOINTER(i)->Number:(int)DX_ERROR(1,i))
#define AINTEGER(i)     ( (int) AREAL(i) )
#define AFLT(i)         ( (flt) AREAL(i) )
#define ALIST(i)        ( ISLIST(i) ? APOINTER(i):(at*)DX_ERROR(2,i) )
#define ACONS(i)        ( ISCONS(i) ? APOINTER(i):(at*)DX_ERROR(3,i) )
#define ASTRING(i)      ( ISSTRING(i) ? SADD( AOBJECT(i)):(char*)DX_ERROR(4,i) )
#define ACELL(i)        ( ISCELL(i) ? APOINTER(i)->Object:DX_ERROR(5,i) )
#define ASYMBOL(i)      ( ISSYMBOL(i) ? APOINTER(i)->Object:DX_ERROR(7,i) )
#define AOBJECT(i)      ( ISOBJECT(i) ? APOINTER(i)->Object:DX_ERROR(8,i) )
#define AINDEX(i)       ( ISINDEX(i)  ? APOINTER(i)->Object:DX_ERROR(9,i) )
#define AGPTR(i)      	( ISGPTR(i) ? APOINTER(i)->Gptr:(gptr)DX_ERROR(10,i) )
#define AHASH(i)      	( ISHASH(i) ? APOINTER(i)->Object:DX_ERROR(11,i) )



#define ARG_NUMBER(i)	if (arg_number != i)  DX_ERROR(0,i);
#define ARG_EVAL(i)	arg_eval(arg_array,i)
#define ALL_ARGS_EVAL	all_args_eval(arg_array,arg_number)


/* FILEIO.H ------------------------------------------------- */

extern class file_R_class, file_W_class;

#define OPEN_READ(f,s)  new_extern(&file_R_class,open_read(f,s), XT_FILE_RO)
#define OPEN_WRITE(f,s) new_extern(&file_W_class,open_write(f,s), XT_FILE_WO)

#define file_close(f) FCLOSE(f)

extern File *open_read(), *open_write(), *open_append();
extern void set_script(), test_file_error();
extern char *add_suffix(), *search_file();

extern char file_name[];


/* IO.H ----------------------------------------------------- */

extern int okeventhandle;

extern void setgetline();
extern void print_string(), print_list(), print_pretty(), print_tab();
extern char *read_string(), *read_word();
extern char *pname(), *first_line(), *dmc();
extern char skip_char();
extern at *read_list();

extern char *line_pos, *line_buffer;


/* CLASSIFY.H --------------------------------------------------- */


extern real mean(), sdev(), cov(), regression();
extern real sup_norm(), abs_norm(), sqr_norm(), hamming_norm();
extern real sup_dist(), abs_dist(), sqr_dist(), hamming_dist();
extern at *rank(), *diff_listes(), *sum_listes();


/* OOSTRUCT.H ----------------------------------------------------- */

extern class class_class;
extern class object_class;
extern class number_class;

extern struct alloc_root class_alloc;

extern int in_object_scope;
extern at *at_this, *at_scope;

struct oostruct {
  int size;                     /* number of slots */
  at *class;                    /* class of object */
  void *cptr;                   /* equivalent C object (lisp_c) */
  struct oostructitem { at *symb, *val; } slots[1];
};

extern void generic_dispose(), generic_action();
extern at *generic_eval();
extern at  *generic_listeval();
extern char *generic_name();

extern at *at_this;

extern void oostruct_dispose();
extern void oostruct_action();
extern struct oostruct *new_oostruct();
extern at **lvalue_in_scope();

extern at *new_object();
extern at *cast();

extern void class_define();
extern at *defclass();
extern void putmethod();

extern at *letslot();
extern at *call_as_method();
extern at *send_sgi();
extern at *checksend();
extern class *classof();

extern void delete_at();


/* BINARY.H ----------------------------------------------------- */

enum Token {
  T_UNDEF, T_EOFT, T_SYMBOL, T_STRING, T_REAL,
  T_LPAREN, T_RPAREN, T_MATRIX, T_DOT, T_REF, 
  T_NIL, T_DX, T_DE, T_DF, T_DM, T_INT, T_TRUE, T_RCLASS, T_RFCALL,
  T_ERROR, T_CLASS, T_ARRAY, T_SYNC, T_RREF, T_DREF, T_RFCALLA, T_SHORT_INT};

#define TOKENOFFSET 128

extern void out_at();
extern void out_real();
extern void out_rcall();
extern at* in_at();
extern at* in_at_rt();


/* REMOTE.H ----------------------------------------------------- */

struct rhandle {
  struct rhandle *next, *prev;
  at *backptr;
  at *rtable;
  int classid;
  int address;
};

struct rtable {
  at *atfin, *atfout;
  File *fin,*fout;
  int nrc;
  int async;
  struct rhandle head;
  struct at* classes[1];
};  


extern class rtable_class;
extern class robject_class;
extern class rcall_class;

extern at *new_rtable();
extern at *new_robject();
extern at *new_rcall();

extern at* bind_remote_process();
extern at* remote_call();


/* STORAGE.H --------------------------------------------------- */

extern class 
   AT_storage_class,
    P_storage_class,
    F_storage_class,
    D_storage_class,
  I32_storage_class,
  I16_storage_class,
   I8_storage_class,
   U8_storage_class,
 GPTR_storage_class;

/* 
 * The field 'type' of a storage defines the type
 * of the elements stored inside
 */
  
enum storage_type {
  ST_AT,
  ST_P,
  ST_F, ST_D,
  ST_I32, ST_I16, ST_I8, ST_U8,
  ST_GPTR,
  /* TAG */
  ST_LAST
};

extern int storage_type_size[ST_LAST];
extern flt (*storage_type_getf[ST_LAST])();
extern void (*storage_type_setf[ST_LAST])();


/* 
 * General purpose flags (STF)
 */

#define STF_UNSIZED   (1<<0)	/* still unbound */
#define STF_RDONLY    (1<<15)	/* read only storage */

/*
 * The other flags define the
 * nature of the storage (STS)
 */

#define STS_MALLOC    (1<<1)	/* in memory via malloc */
#define STS_MMAP      (1<<2)	/* mapped via mmap */
#define STS_DISK      (1<<3)	/* on disk */
#define STS_REMOTE    (1<<4)	/* over a remote connection */
#define STS_STATIC    (1<<5)	/* over a remote connection */

/* The "light" storage structure */

struct srg {	
    short flags;
    unsigned char type;
    unsigned char pad;
    int size;
    ptr data;
};

struct storage {

  struct srg srg;
  void (*read_srg)();		/* given an index, returns an idx for reading  */
  void (*write_srg)();		/* given an index, returns an idx for writing  */
  void (*rls_srg)();		/* release an idx structure */
  at*  (*getat)();              /* given an offset, gets a at value */
  void (*setat)();		/* given an at at a specific offset */
  at *atst;                     /* pointer on the at storage */
  struct srg *cptr;             /* srg structure for the C side (lisp_c) */
  
  /* Allocation dependent info */
  
  union allinfo {

    struct { 
      ptr addr; 
    } sts_malloc;		/* for malloc... */

#ifdef MMAPARRAY
    struct {			
      ptr addr;
      int len;
    } sts_mmap;			/* for mmaps... */
#endif
#ifdef DISKARRAY
    struct {
      FILE *f;		/* ??? -> File ??? */
      int blocksize;
      struct storage_cache *cache;
    } sts_disk;			/* for disk... */
#endif
#ifdef REMOTEARRAY    
    struct {
      struct rhandle *obj, *rpcget, *rpcset;
      struct storage_cache *cache;
    } sts_remote;		/* for remote... */
#endif

  } allinfo;
};


at *new_AT_storage();
at *new_P_storage();
at *new_F_storage();
at *new_D_storage();
at *new_I32_storage();
at *new_I16_storage();
at *new_I8_storage();
at *new_U8_storage();
at *new_GPTR_storage();
at *new_storage();
at *new_storage_nc();

int storagep();

void storage_malloc();
void storage_realloc();
#ifdef MMAPARRAY
void storage_mmap();
#endif
#ifdef DISKARRAY
void storage_disk();
#endif
#ifdef REMOTEARRAY
void storage_remote();
#endif

void storage_clear();
int storage_load();
void storage_save();

void init_storage();




/* INDEX.H ---------------------------------------------------- */

#define MAXDIMS 8

#define IDF_HAS_NR0  1
#define IDF_HAS_NR1  2
#define IDF_UNSIZED  4

extern class index_class;

/* The "light" idx structure */

#define IDX_DATA_PTR(idx) \
    (ptr) ((char *) (idx)->srg->data + \
	   (idx)->offset * storage_type_size[(idx)->srg->type])

struct idx {	
    short ndim;
    short flags;
    int offset;	
    int *dim;
    int *mod;
    struct srg *srg;
};

/* The "heavy" index structure */

struct index {			
  short flags;			/* flags */

  /* The definition of the index */ 
  /*   Field names are similar to those of the */
  /*   idx structure. A idx macro will work on */
  /*   index structures! */

  short ndim;			
  int offset;			/* in element size */
  int dim[MAXDIMS];		
  int mod[MAXDIMS];		
  
  at *atst;			/* a lisp handle to the storage object */
  struct storage *st;		/* a pointer to the storage */
  
  flt **nr0;			/* The Numerical Recipes pointers (base 0) */
  flt **nr1;                    /* The Numerical Recipes pointers (base 1) */
  struct idx *cptr;              /* struxt idx for the C side (lisp_c) */
};


/* Function related to <struct index> objects */

int indexp(), matrixp(), arrayp();
at *new_index();
void index_dimension();
void index_undimension();
void index_from_index();

struct index *easy_index_check();
real easy_index_get();
void easy_index_set();

char *not_a_nrvector();
char *not_a_nrmatrix();
flt *make_nrvector();
flt **make_nrmatrix();

at *AT_matrix();		/* Simultaneous creation       */
at *F_matrix();			/* of an index and its storage */
at *D_matrix();
at *I32_matrix();
at *I16_matrix();
at *I8_matrix();
at *U8_matrix();
at *GPTR_matrix();

/* Functions related to <struct idx> objects */

void index_read_idx();
void index_write_idx();
void index_rls_idx();


/* Other functions */

at *load_matrix();
void save_flt_matrix();
void save_packed_matrix();
void save_ascii_matrix();

void init_index();



/* 
 * Loops over all elements of idx <idx>
 * The variable <ptr> must be a pointer
 * referencing the first element of <idx>.
 * It is incremented by the loop, over all the idx.
 */

#define begin_idx_aloop1(idx,ptr) { 					     \
  int _d_[MAXDIMS], _j_; 						     \
  int ptr = 0;								     \
  for (_j_=0;_j_<(idx)->ndim; _j_++ ) 					     \
    _d_[_j_]=0; 							     \
  _j_ = (idx)->ndim; 							     \
  while (_j_>=0) {

#define end_idx_aloop1(idx,ptr) 					     \
    _j_--; 								     \
    do { 								     \
      if (_j_<0) break; 						     \
      if (++_d_[_j_] < (idx)->dim[_j_]) {				     \
	ptr+=(idx)->mod[_j_];						     \
	_j_++;								     \
      } else { 								     \
	ptr-=(idx)->dim[_j_]*(idx)->mod[_j_]; 				     \
	_d_[_j_--] = -1; 						     \
      } 								     \
    } while (_j_<(idx)->ndim);  						     \
  } 									     \
}


/* 
 * Independently loops over all elements of both idxs <idx1> and <idx2>
 * The variables <ptr1> and <ptr2> must be pointers
 * referencing the first element of <idx1> and <idx2>.
 * Idxs <idx1> and <idx2> don't need to have the same structure,
 * but they must have the same number of elements.
 */

#define begin_idx_aloop2(idx1, idx2, ptr1, ptr2) { 			     \
  int _d1_[MAXDIMS], _j1_; 						     \
  int _d2_[MAXDIMS], _j2_;						     \
  int ptr1 = 0;							     	     \
  int ptr2 = 0;								     \
  for (_j1_=0;_j1_<(idx1)->ndim; _j1_++ ) 				     \
    _d1_[_j1_]=0; 							     \
  for (_j2_=0;_j2_<(idx2)->ndim; _j2_++ ) 				     \
    _d2_[_j2_]=0; 							     \
  _j1_ = (idx1)->ndim;      						     \
  _j2_ = (idx2)->ndim; 							     \
  while (_j1_>=0 && _j2_>=0) {
    
#define end_idx_aloop2(idx1, idx2, ptr1,ptr2) 				     \
    _j1_--;								     \
    _j2_--; 								     \
    do { 								     \
      if (_j1_<0) 							     \
	break; 								     \
      if (++_d1_[_j1_] < (idx1)->dim[_j1_]) {				     \
	ptr1 += (idx1)->mod[_j1_];					     \
	_j1_++;								     \
      } else { 								     \
	ptr1 -= (idx1)->dim[_j1_]*(idx1)->mod[_j1_];			     \
	_d1_[_j1_--] = -1; 						     \
      } 								     \
    } while (_j1_<(idx1)->ndim); 					     \
    do { 								     \
      if (_j2_<0) break; 						     \
      if (++_d2_[_j2_] < (idx2)->dim[_j2_]) {				     \
	ptr2 += (idx2)->mod[_j2_];					     \
	_j2_++;								     \
      } else { 								     \
	ptr2 -= (idx2)->dim[_j2_]*(idx2)->mod[_j2_]; 			     \
	_d2_[_j2_--] = -1; 						     \
      } 								     \
    } while (_j2_<(idx2)->ndim); 					     \
  } 									     \
}


/* 
 * Independently loops over all elements of both idxs <idx0>,<idx1> and <idx2>
 * The variables <ptr0><ptr1> and <ptr2> must be pointers
 * referencing the first element of <idx0><idx1> and <idx2>.
 * Idxs <idx0><idx1> and <idx2> don't need to have the same structure,
 * but they must have the same number of elements.
 */

#define begin_idx_aloop3(idx0, idx1, idx2, ptr0, ptr1, ptr2) { 	             \
  int _d0_[MAXDIMS], _j0_; 						     \
  int _d1_[MAXDIMS], _j1_; 						     \
  int _d2_[MAXDIMS], _j2_;						     \
  int ptr0 = 0;							     	     \
  int ptr1 = 0;							     	     \
  int ptr2 = 0;								     \
  for (_j0_=0;_j0_<(idx0)->ndim; _j0_++ ) 				     \
    _d0_[_j0_]=0; 							     \
  for (_j1_=0;_j1_<(idx1)->ndim; _j1_++ ) 				     \
    _d1_[_j1_]=0; 							     \
  for (_j2_=0;_j2_<(idx2)->ndim; _j2_++ ) 				     \
    _d2_[_j2_]=0; 							     \
  _j0_ = (idx0)->ndim;      						     \
  _j1_ = (idx1)->ndim;      						     \
  _j2_ = (idx2)->ndim; 							     \
  while (_j0_>=0 && _j1_>=0 && _j2_>=0) {
    
#define end_idx_aloop3(idx0, idx1, idx2, ptr0, ptr1,ptr2) 		     \
    _j0_--;								     \
    _j1_--;								     \
    _j2_--; 								     \
    do { 								     \
      if (_j0_<0) 							     \
	break; 								     \
      if (++_d0_[_j0_] < (idx0)->dim[_j0_]) {				     \
	ptr0 += (idx0)->mod[_j0_];					     \
	_j0_++;								     \
      } else { 								     \
	ptr0 -= (idx0)->dim[_j0_]*(idx0)->mod[_j0_];			     \
	_d0_[_j0_--] = -1; 						     \
      } 								     \
    } while (_j0_<(idx0)->ndim); 					     \
    do {                                                                     \
      if (_j1_<0) 							     \
	break; 								     \
      if (++_d1_[_j1_] < (idx1)->dim[_j1_]) {				     \
	ptr1 += (idx1)->mod[_j1_];					     \
	_j1_++;								     \
      } else { 								     \
	ptr1 -= (idx1)->dim[_j1_]*(idx1)->mod[_j1_];			     \
	_d1_[_j1_--] = -1; 						     \
      } 								     \
    } while (_j1_<(idx1)->ndim); 					     \
    do { 								     \
      if (_j2_<0) break; 						     \
      if (++_d2_[_j2_] < (idx2)->dim[_j2_]) {				     \
	ptr2 += (idx2)->mod[_j2_];					     \
	_j2_++;								     \
      } else { 								     \
	ptr2 -= (idx2)->dim[_j2_]*(idx2)->mod[_j2_]; 			     \
	_d2_[_j2_--] = -1; 						     \
      } 								     \
    } while (_j2_<(idx2)->ndim); 					     \
  } 									     \
}


/* DZ.H ------------------------------------------------------ */


extern class dz_class;
extern char *dz_opnames[];
extern  int  dz_offname;
extern flt dz_stack[];
#define DZ_STACK_SIZE 1000

extern flt dz_execute();

union dz_inst {
  struct op_type {
    short op;
    short arg;
  }   code;
  flt constant;
};

struct dz_cell {
  flt (*call)();
  int num_arg;
  int program_size;
  int required_stack;
  union dz_inst *program;
};

/* DH.H -------------------------------------------------- */

/*
 * DH are C functions working on matrices and numbers.
 * They may be compiled using 'dh-compile'.
 * 
 */

extern class dh_class;
extern class cclass_class;
extern void dh_define();
typedef void (*method_ptr)();



/* LISP_C.H ---------------------------------------------- */

void cside_create_idx();
void cside_create_srg();
void cside_create_obj();
void cside_create_str();
void cside_destroy_item();
void cside_destroy_range();

void lside_destroy_item();
void lside_dld_partial();


#endif
