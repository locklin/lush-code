/*   Lush Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
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
 */
 
/***********************************************************************
  T-Lisp3: (C) LYB YLC 1988
  header.h
  This file contains general lisp structure definitions
  $Id: header.h,v 0.1.1.1 2001-10-31 17:29:27 profshadoko Exp $
********************************************************************** */

#ifndef HEADER_H
#define HEADER_H

#ifndef DEFINE_H
#include "define.h"
#endif

#ifndef FLTLIB_H
#include "fltlib.h"
#endif

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif


/* VERSION.H --------------------------------------------------- */

#define TLOPEN_MAJOR  31
#define TLOPEN_MINOR  10


/* MISC.H ------------------------------------------------------ */


#define ifn(s)          if(!(s))
#define forever         for(;;)
#define until(s)        while(!(s))

#if defined(__GNUC__) && (__GNUC__ > 1 ) && (__GNUC_MINOR__ > 5) /* Arno */
#define no_return __attribute__((__noreturn__))
#endif
#ifndef no_return
#define no_return /**/
#endif


/* OS.H ---------------------------------------------------------- */


#ifdef UNIX
/* interruptions */
extern LUSHAPI int break_attempt;
/* event management */
enum triggerops { OP_BEGIN_WAIT, OP_END_WAIT, OP_ASYNC, OP_SYNC };
void set_trigger(int fd, void (*trigger)(int op), void (*process)(void)); 
void lastchance(char *s) no_return;
void call_sync_trigger(void);
void wait_trigger(void);
/* console management */
void console_getline(char *prompt, char *buf, int size);
/* open entry points */
void init_user(void);
int  init_user_dll(int major, int minor);
/* replacement functions */
LUSHAPI FILE* unix_popen(const char *, const char *);
LUSHAPI int   unix_pclose(FILE *);
#endif


#ifdef WIN32
/* interruptions */
extern LUSHAPI int break_attempt;
extern LUSHAPI int kill_attempt;
LUSHAPI void lastchance(char *s) no_return;
/* system override */
LUSHAPI void  win32_exit(int);
LUSHAPI int   win32_isatty(int);
LUSHAPI void  win32_user_break(char *s);
LUSHAPI FILE* win32_popen(char *, char *);
LUSHAPI int   win32_pclose(FILE *);
LUSHAPI void  win32_fmode_text(FILE *f);
LUSHAPI void  win32_fmode_binary(FILE *f);
LUSHAPI int   win32_waitproc(void *wproc);
/* event management */
LUSHAPI void set_trigger(void (*trigger)(void), void (*process)(void)); 
LUSHAPI void block_async_trigger(void);
LUSHAPI void unblock_async_trigger(void);
LUSHAPI void call_sync_trigger(void);
LUSHAPI void wait_trigger(void);
/* console management */
LUSHAPI void console_getline(char *prompt, char *buf, int size);
/* openTL entry points */
void init_user(void);
DLLEXPORT int init_user_dll(int major, int minor);
#endif



/* AT.H -------------------------------------------------------- */

/* struct at is the CONS structure */

typedef struct at {
  unsigned int count;
  unsigned short flags;
  unsigned short ident;
  union {
    struct {			/* cons */
      struct at *at_car, *at_cdr;
    } at_cons;

    real at_number;		/* number */
    gptr at_gptr;		/* generic pointer */

    struct {			/* external */
      gptr at_object;
      struct class *at_class;
    } at_extern;
  } at_union;
} at;

/* shorthands */

#define Cdr     at_union.at_cons.at_cdr
#define Car     at_union.at_cons.at_car
#define Number  at_union.at_number
#define Gptr    at_union.at_gptr
#define Object  at_union.at_extern.at_object
#define Class   at_union.at_extern.at_class

/* flags */

#define C_CONS          (1<<0)	/* It's a  CONS */
#define C_EXTERN        (1<<1)	/* It's an EXTERNAL */
#define C_NUMBER        (1<<2)	/* It's a  NUMBER */
#define C_GPTR          (1<<3)
#define C_GARBAGE       (1<<4)  /* To destroy... */
#define C_MARK          (1<<5)
#define C_MULTIPLE      (1<<6)


/*
 * Specials flags for EXTERNALs only improving the tests speed.
 */

#define   X_SYMBOL      (1<<8 )
#define   X_FUNCTION    (1<<9 )
#define   X_STRING      (1<<10)
#define   X_OOSTRUCT    (1<<11)
#define   X_ZOMBIE      (1<<12) /* zombie means nil */
#define   X_STORAGE     (1<<13)	
#define   X_INDEX       (1<<14)	
#define   X_ARRAY       (1<<15)	/* obsolete .... */

/* Some useful macros */

#define LOCK(x)         { if (x) (x)->count++; }
#define UNLOCK(x)       { if ( (x) && --((x)->count)==0 )  purge(x); }

#define CONSP(x)        ((x)&&((x)->flags&C_CONS))
#define LISTP(x)        (!(x)||((x)->flags&C_CONS))
#define NUMBERP(x)	((x)&&((x)->flags&C_NUMBER))
#define GPTRP(x)	((x)&&((x)->flags&C_GPTR))
#define EXTERNP(x,cl)	((x)&&((x)->flags&C_EXTERN)&&((x)->Class==(cl)))

extern LUSHAPI at *(*eval_ptr) (at*);
extern LUSHAPI at *(*argeval_ptr) (at*);

#define NEW_NUMBER(x)   new_number((real)(x))
#define eval(q)         (*eval_ptr)(q)


/*
 * The class structure defines the behavior of
 * each type of external objects.
 */

typedef struct class {
  /* class vectors */
  void           (*self_dispose) (at*);
  void           (*self_action)  (at*, void (*f)(at*));
  char*          (*self_name)    (at*);
  at*            (*self_eval)    (at*);
  at*            (*list_eval)    (at*, at*);
  void           (*serialize)    (at**, int);
  int            (*compare)      (at*, at*, int);
  unsigned long  (*hash)         (at*);
  at*		 (*getslot)      (at*, at*);
  void           (*setslot)      (at*, at*, at*);
  /* class information */
  at*              classname;
  at*              backptr;
  short            slotssofar;    
  char             goaway;
  char 	           dontdelete;
  at*              keylist;
  at*              defaults;
  at*              atsuper;
  struct class*    super;	/* link to superclass */
  struct class*    subclasses;	/* link to subclasses */
  struct class*    nextclass;	/* next subclass      */
  at*              methods;
  struct hashelem* hashtable;
  int              hashsize;
  int	    	   hashok;
} class;


struct hashelem {
  at *symbol;
  at *function;
  int sofar;
};


LUSHAPI void purge (register at *q);
LUSHAPI at *new_gptr(gptr x);
LUSHAPI at *new_number(double x);
LUSHAPI at *new_extern(class *cl, void *obj);
LUSHAPI at *new_cons(at *car, at *cdr);
LUSHAPI at *cons(at *car, at *cdr);
LUSHAPI at *car(at *q);
LUSHAPI at *caar(at *q);
LUSHAPI at *cadr(at *q);
LUSHAPI at *cdr(at *q);
LUSHAPI at *cdar(at *q);
LUSHAPI at *cddr(at *q);
LUSHAPI at *rplaca(at *q, at *p);
LUSHAPI at *rplacd(at *q, at *p);
LUSHAPI at *displace(at *q, at *p);
LUSHAPI int length(at *p);
LUSHAPI at *member(at *elem, at *list);
LUSHAPI at *nth(at *l, int n);
LUSHAPI at *nthcdr(at *l, int n);
LUSHAPI at *last(at *list);
LUSHAPI at *lastcdr(at *list);
LUSHAPI at *flatten(at *l);
LUSHAPI at *append(at *l1, at *l2);
LUSHAPI at *reverse(at *l);
LUSHAPI at *assoc(at *k, at *l);
LUSHAPI int used(void);

LUSHAPI void generic_dispose(at *p);
LUSHAPI void generic_action(at *p, void (*action) (at*));
LUSHAPI char *generic_name(at *p);
LUSHAPI at *generic_eval(at *p);
LUSHAPI at *generic_listeval(at *p, at *q);
#define generic_serialize NULL
#define generic_compare   NULL
#define generic_hash      NULL
#define generic_getslot   NULL
#define generic_setslot   NULL


/* EVAL.H ----------------------------------------------------- */

LUSHAPI at *eval_std(at *p);
LUSHAPI at *eval_debug(at *q);
LUSHAPI at *apply(at *q, at *p);
LUSHAPI at *progn(at *p);
LUSHAPI at *prog1(at *p);
LUSHAPI at *mapc(at *f, at **listes, int arg_number);
LUSHAPI at *rmapc(at *f, at **listes, int arg_number);
LUSHAPI at *mapcar(at *f, at **listes, int arg_number);
LUSHAPI at *rmapcar(at *f, at **listes, int arg_number);




/* ALLOC.H ---------------------------------------------------- */



struct empty_alloc {		/* skeleton structure used by the alloc
				 * routines.  skeleton structure used by the
				 * alloc routines. */
  int used;
  struct empty_alloc *next;
};

struct chunk_header {		/* this structure describe each element
				 * chunk.     this structure describe each
				 * element chunk. */
  struct chunk_header *next;
  gptr begin;
  gptr end;
  gptr pad;			/* too clever compiler may cause problems */
};

struct alloc_root {		  /* this structure contains the allocation
				   * infos */
  struct empty_alloc *freelist;	  /* List of free elems */
  struct chunk_header *chunklist; /* List of active chunkes */
  int elemsize;			  /* Size of one elems */
  int chunksize;		  /* Number of elems in one chunk */
};

LUSHAPI gptr tl_malloc (size_t);
LUSHAPI gptr tl_realloc (gptr, size_t);
LUSHAPI gptr  allocate(struct alloc_root *ar);
LUSHAPI void deallocate(struct alloc_root *ar, struct empty_alloc *elem);
LUSHAPI void garbage(int flag);
LUSHAPI void protect(at *q);
LUSHAPI void unprotect(at *q);

/*
 * Following, a very general iterator. A loop on all the used elements of a
 * ALLOC_BASE structure may be written just as
 * 
 * { struct chunk_header *current_chunk; 
 *   struct xxxx *looping_var; 
 *   iter_on( &xxxx_alloc, current_chunk, looping_var ) { ... }
 * }
 */

#define iter_on(base,i,elem) \
 for(i=(base)->chunklist; i; i = i->next ) \
 for(elem=i->begin;(gptr)elem<i->end;elem=(gptr)((char*)elem+(base)->elemsize)) \
 if ( ((struct empty_alloc *) elem) -> used )


/* malloc debug file (from sn3.2) */

#define malloc(x)    lush_malloc(x,__FILE__,__LINE__)
#define calloc(x,y)  lush_calloc(x,y,__FILE__,__LINE__)
#define realloc(x,y)  lush_realloc(x,y,__FILE__,__LINE__)
#define free(x)      lush_free(x,__FILE__,__LINE__)
#define cfree(x)     lush_cfree(x,__FILE__,__LINE__)
void *lush_malloc(int,char*,int), 
     *lush_calloc(int,int,char*,int), 
     *lush_realloc(void*,int,char*,int);
void lush_free(void*,char*,int), 
     lush_cfree(void*,char*,int);
void set_malloc_file(char*);


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
extern LUSHAPI struct hash_name *names[];

/*
 * Iterator on the hash_name nodes.
 * { struct hash_name **i; 
 *   struct hash_name *hn; 
 *   iter_hash_name(i,hn) { ... }
 * }
 */

#define iter_hash_name(i,hn) \
  for (i=names; i<names+HASHTABLESIZE; i++) \
  for (hn= *i; hn; hn = hn->next ) \
  if (hn->used)


extern LUSHAPI class symbol_class;

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


LUSHAPI at *new_symbol(char *s);
LUSHAPI at *named(char *s);
LUSHAPI char *nameof(at *p);
LUSHAPI void symbol_push (at *p, at *q);
LUSHAPI void symbol_pop (at *p);
LUSHAPI at *setq(at *p, at *q);	/* Warning: Never use the result. */
LUSHAPI at *symblist(void); 
LUSHAPI at *oblist(void);
LUSHAPI at *true(void);
LUSHAPI at *getprop(struct symbol *symb, at *prop);
LUSHAPI void putprop(struct symbol *symb, at *value, at *prop);
LUSHAPI void var_set(at *p, at *q);
LUSHAPI void var_SET(at *p, at *q); /* Set variable regardless of lock mode */
LUSHAPI void var_lock(at *p);
LUSHAPI at *var_get(at *p);
LUSHAPI at *var_define(char *s);




/* TOPLEVEL.H ------------------------------------------------- */


struct recur_elt { 
  struct recur_elt *next; 
  void *call; 
  at *p;
};

extern LUSHAPI struct recur_doc {
  /* hash table for detecting infinite recursion */
  int hsize;
  int hbuckets;
  struct recur_elt **htable;
} recur_doc;


extern LUSHAPI struct error_doc {	   
  /* contains info for printing error messages */
  at *this_call;
  at *error_call;
  char *error_prefix;
  char *error_text;
  at *error_suffix;
  short debug_tab;
  short ready_to_an_error;
  short debug_toplevel;
  short script_mode;
  FILE *script_file;
} error_doc;

#define SCRIPT_OFF      0
#define SCRIPT_INPUT    1
#define SCRIPT_PROMPT   2
#define SCRIPT_OUTPUT   3

/*
 * This structure is used to handle exception in the C code.
 */

extern LUSHAPI struct context {
  struct context *next;
  sigjmp_buf error_jump;
  FILE *input_file;
  FILE *output_file;
  short input_tab;
  short output_tab;
} *context;

LUSHAPI void context_push(struct context *newc);
LUSHAPI void context_pop(void);
LUSHAPI int  recur_push_ok(struct recur_elt *elt, void *call, at *p);
LUSHAPI void recur_pop(struct recur_elt *elt);
LUSHAPI void toplevel(char *in, char *out, char *new_prompt);
LUSHAPI void error(char *prefix, char *text, at *suffix) no_return;
LUSHAPI void user_break(char *s);
LUSHAPI void init_lush (char *program_name);
LUSHAPI void start_lisp(int argc, char **argv);
LUSHAPI void clean_up(void);
LUSHAPI void abort (char *s) no_return;



/* STRING.H ---------------------------------------------------- */

extern LUSHAPI class string_class;

struct string {
  int flag;
  char *start;
};

#define STRING_UNUSED    0
#define STRING_SAFE      1
#define STRING_ALLOCATED 2

#define SADD(str)       (((struct string *)(str))->start)

LUSHAPI at *new_string(char *s);
LUSHAPI at *new_safe_string(char *s);
LUSHAPI at *new_string_bylen(int n);
LUSHAPI at *str_val(char *s);
LUSHAPI char *str_number(double x);
LUSHAPI char *str_number_hex(double x);
LUSHAPI char *str_gptr(gptr x);

LUSHAPI char *regex_compile(char *pattern, short int *bufstart, short int *bufend,
			  int strict, int *rnum);
LUSHAPI int regex_exec(short int *buffer, char *string, 
		     char **regptr, int *reglen, int nregs);
LUSHAPI int regex_seek(short int *buffer, char *string, char *seekstart, 
		     char **regptr, int *reglen, int nregs, 
		     char **start, char **end);

extern LUSHAPI char *string_buffer;
extern LUSHAPI char null_string[];
extern LUSHAPI char digit_string[];
extern LUSHAPI char special_string[];
extern LUSHAPI char aspect_string[];


/* FUNCTION.H -------------------------------------------------- */

/*
 * function are implemented as external objects 
 * pointing to this structure:
 */

struct function {
  int used;			/* CFUNC                  LFUNC */
  at *formal_arg_list;		/* backptr              arglist */
  at *evaluable_list;		/* name                   body  */
  void *c_function;		/* call                      0  */
};

extern LUSHAPI class de_class;
extern LUSHAPI class df_class;
extern LUSHAPI class dm_class;
extern LUSHAPI class dx_class;		/* dx functions are external C_function */
extern LUSHAPI class dy_class;		/* dy functions have unflattened args. */

extern LUSHAPI at **dx_stack, **dx_sp;
extern LUSHAPI struct alloc_root function_alloc;

LUSHAPI at *new_de(at *formal, at *evaluable);
LUSHAPI at *new_df(at *formal, at *evaluable);
LUSHAPI at *new_dm(at *formal, at *evaluable);
LUSHAPI at *new_dx(at *(*addr) (int, at **));
LUSHAPI at *new_dy(at *(*addr) (at *));
LUSHAPI at *funcdef(at *f);
LUSHAPI at *eval_a_list(at *p);
LUSHAPI void dx_define(char *name, at *(*addr) (int, at **));
LUSHAPI void dy_define(char *name, at *(*addr) (at *));
LUSHAPI gptr need_error(int i, int j, at **arg_array_ptr);
LUSHAPI void arg_eval(at **arg_array, int i);
LUSHAPI void all_args_eval(at **arg_array, int i);

/* This is the interface header builder:   See  pp.c */

#define DX(Xname)  static at *Xname(int arg_number, at **arg_array)
#define DY(Yname)  static at *Yname(at *ARG_LIST)

/* Macros and functions used in argument transmission in DX functions */

#define ISNUMBER(i)	(NUMBERP(arg_array[i]))
#define ISGPTR(i)	(GPTRP(arg_array[i]))
#define ISLIST(i)	(LISTP(arg_array[i]))
#define ISCONS(i)	(CONSP(arg_array[i]))
#define ISSTRING(i)	(arg_array[i]&&(arg_array[i]->flags & X_STRING))
#define ISCELL(i)	(arg_array[i]&&(arg_array[i]->flags & X_CELL))
#define ISSTORAGE(i)	(arg_array[i]&&(arg_array[i]->flags & X_STORAGE))
#define ISINDEX(i)	(arg_array[i]&&(arg_array[i]->flags & X_INDEX))
#define ISSYMBOL(i)	(arg_array[i]&&(arg_array[i]->flags & X_SYMBOL))
#define ISOBJECT(i) 	(arg_array[i]&&(arg_array[i]->flags & C_EXTERN))
#define DX_ERROR(i,j)   (need_error(i,j,arg_array))

#define APOINTER(i)     ( arg_array[i] )
#define AREAL(i)        ( ISNUMBER(i) ? APOINTER(i)->Number:(long)DX_ERROR(1,i))
#define AGPTR(i)        ( ISGPTR(i) ? APOINTER(i)->Gptr:(gptr)DX_ERROR(1,i))
#define AINTEGER(i)     ( (int) AREAL(i) )
#define AFLT(i)         ( rtoF(AREAL(i)) )
#define ALIST(i)        ( ISLIST(i) ? APOINTER(i):(at*)DX_ERROR(2,i) )
#define ACONS(i)        ( ISCONS(i) ? APOINTER(i):(at*)DX_ERROR(3,i) )
#define ASTRING(i)      ( ISSTRING(i) ? SADD( AOBJECT(i)):(char*)DX_ERROR(4,i) )
#define ACELL(i)        ( ISCELL(i) ? APOINTER(i)->Object:DX_ERROR(5,i) )
#define ASYMBOL(i)      ( ISSYMBOL(i) ? APOINTER(i)->Object:DX_ERROR(7,i) )
#define AOBJECT(i)      ( ISOBJECT(i) ? APOINTER(i)->Object:DX_ERROR(8,i) )
#define AINDEX(i)      ( ISINDEX(i) ? APOINTER(i)->Object:DX_ERROR(10,i) )

#define ARG_NUMBER(i)	if (arg_number != i)  DX_ERROR(0,i);
#define ARG_EVAL(i)	arg_eval(arg_array,i)
#define ALL_ARGS_EVAL	all_args_eval(arg_array,arg_number)


/* FILEIO.H ------------------------------------------------- */

extern LUSHAPI class file_R_class, file_W_class;
extern LUSHAPI char *path[];
extern LUSHAPI char file_name[];

#define OPEN_READ(f,s)  new_extern(&file_R_class,open_read(f,s))
#define OPEN_WRITE(f,s) new_extern(&file_W_class,open_write(f,s))

LUSHAPI char *cwd(char *s);
LUSHAPI at *files(char *s);
LUSHAPI int dirp(char *s);
LUSHAPI int filep(char *s);
LUSHAPI char *dirname(char *fname);
LUSHAPI char *basename(char *fname, char *suffix);
LUSHAPI char *concat_fname(char *from, char *fname);
LUSHAPI void clean_tmp_files(void);
LUSHAPI char *tmpname(char *s, char *suffix);
#ifndef HAVE_STRERROR
LUSHAPI char *strerror(int errno);
#endif

LUSHAPI char *search_file(char *s, char *suffixes);
LUSHAPI void test_file_error(FILE *f);
LUSHAPI FILE *open_read(char *s, char *suffixes);
LUSHAPI FILE *open_write(char *s, char *suffixes);
LUSHAPI FILE *open_append(char *s, char *suffixes);
LUSHAPI FILE *attempt_open_read(char *s, char *suffixes);
LUSHAPI FILE *attempt_open_write(char *s, char *suffixes);
LUSHAPI FILE *attempt_open_append(char *s, char *suffixes);
LUSHAPI void file_close(FILE *f);
LUSHAPI void set_script(char *s);
LUSHAPI long file_size(FILE *f);

/* macros for compatibility with SN3.x files */
#define File FILE
#define PFWRITE(source, n, size, pf) fwrite(source, n, size, pf)  
#define PFREAD(dest, n, size, pf) fread(dest, n, size, pf) 
#define FSEEK(pf, offset, ptrname) fseek(pf, offset, ptrname) 
#define REWIND(pf) rewind(pf) 
#define FTELL(pf) ftell(pf) 
#define FCLOSE(pf) fclose(pf) 
#define FFLUSH(pf) fflush(pf) 
#define FPUTS(s, pf) fputs(s, pf) 
#define FGETC(pf) fgetc(pf) 
#define FPUTC(c, pf) fputc(c, pf) 
#define FERROR(pf) ferror(pf) 
#define TEST_FILE_ERROR(pf) test_file_error(pf) 
#define FEOF(pf) feof(pf) 
#define FSCANF1(pf, format, addr) fscanf(pf, format, addr) 
#define UNGETC(c, pf) ungetc(c, pf) 
#define CLEARERR(pf) clearerr(pf) 
#define FILENO(pf) fileno(pf) 


/* IO.H ----------------------------------------------------- */


extern LUSHAPI char *line_pos;
extern LUSHAPI char *line_buffer;
extern LUSHAPI char *prompt_string;

LUSHAPI void print_char (char c);
LUSHAPI void print_string(char *s);
LUSHAPI void print_list(at *list);
LUSHAPI void print_tab(int n);
LUSHAPI char *pname(at *l);
LUSHAPI char *first_line(at *l);
LUSHAPI char read_char(void);
LUSHAPI char next_char(void);
LUSHAPI int ask (char *t);
LUSHAPI char *read_string(char *s);
LUSHAPI char *read_word(void);
LUSHAPI char *dmc(char *s, at *l);
LUSHAPI char skip_char(char *s);
LUSHAPI at *read_list(void);



/* HTABLE.H ------------------------------------------------- */

extern LUSHAPI class htable_class;

LUSHAPI unsigned long hash_value(at *);
LUSHAPI unsigned long hash_pointer(at *);
LUSHAPI at  *new_htable(int nelems, int pointerhashp);
LUSHAPI void htable_set(at *htable, at *key, at *value);
LUSHAPI at  *htable_get(at *htable, at *key);


/* ARRAY.H --------------------------------------------------- */

extern LUSHAPI class array_class;
extern LUSHAPI struct alloc_root array_alloc;

LUSHAPI at *array(int nd, int *d);
LUSHAPI int bound(at *p, int n);
LUSHAPI int arrayp(at *p);
/* 
LUSHAPI at *copy_any_matrix(at *p, at *q); 
LUSHAPI at *submatrix(at *array, int *mind, int *maxd);
LUSHAPI at *mtranspose(at *m, int n1, int n2);
LUSHAPI at *transpose(at *m);
LUSHAPI at *diagonal(at *m);
LUSHAPI at *matrix_rotate(at *m, int n);
LUSHAPI at *matrix_split_dim(at *m, int n, int j, int *t);
LUSHAPI at *matrix_merge_dim(at *m, int n, int l);
LUSHAPI at *munfold_dim(at *atm, int n, int dn, int st);
LUSHAPI at *mrep_dim(at *atm, int n, int k);
*/





/* MATRIX.H --------------------------------------------------- */

extern LUSHAPI class matrix_class, pmatrix_class;
extern LUSHAPI class dmatrix_class, imatrix_class;
extern LUSHAPI class bmatrix_class, smatrix_class;

LUSHAPI at *matrix(int nd, int *d);
LUSHAPI at *fmatrix(int nd, int *d);
LUSHAPI at *pmatrix(int nd, int *d);
LUSHAPI at *dmatrix(int nd, int *d);
LUSHAPI at *imatrix(int nd, int *d);
LUSHAPI at *bmatrix(int nd, int *d);
LUSHAPI at *smatrix(int nd, int *d);
LUSHAPI int matrixp(at *p);
LUSHAPI real unpack(int b);
LUSHAPI unsigned char pack(double x);
LUSHAPI int array_elt_size (at *p);
LUSHAPI void import_raw_matrix(at *p, FILE *f, int offset);
LUSHAPI void import_text_matrix(at *p, FILE *f);
LUSHAPI int save_matrix_len (at *p);
LUSHAPI void save_matrix(at *p, FILE *f);
LUSHAPI void export_matrix(at *p, FILE *f);
/* LUSHAPI void save_ascii_matrix(at *p, FILE *f); */
/* LUSHAPI void export_ascii_matrix(at *p, FILE *f); */
/* LUSHAPI at *load_matrix(FILE *f); */
/* LUSHAPI at *map_matrix(FILE *f); */
LUSHAPI float *get_std_vector(at *p, int n);
LUSHAPI void  *get_std_matrix(at *p, int m, int n);
LUSHAPI float *get_nr0_vector(at *p, int *psize);
LUSHAPI float **get_nr0_matrix(at *p, int *ps0, int *ps1);
LUSHAPI float *get_nr1_vector(at *p, int *psize);
LUSHAPI float **get_nr1_matrix(at *p, int *ps0, int *ps1);
LUSHAPI double *get_nr0_dvector(at *p, int *psize);
LUSHAPI double **get_nr0_dmatrix(at *p, int *ps0, int *ps1);
LUSHAPI double *get_nr1_dvector(at *p, int *psize);
LUSHAPI double **get_nr1_dmatrix(at *p, int *ps0, int *ps1);
LUSHAPI short *get_nr0_svector(at *p, int *psize);
LUSHAPI short **get_nr0_smatrix(at *p, int *ps0, int *ps1);
LUSHAPI short *get_nr1_svector(at *p, int *psize);
LUSHAPI short **get_nr1_smatrix(at *p, int *ps0, int *ps1);
LUSHAPI int *get_nr0_ivector(at *p, int *psize);
LUSHAPI int **get_nr0_imatrix(at *p, int *ps0, int *ps1);
LUSHAPI int *get_nr1_ivector(at *p, int *psize);
LUSHAPI int **get_nr1_imatrix(at *p, int *ps0, int *ps1);
LUSHAPI unsigned char *get_nr0_bvector(at *p, int *psize);
LUSHAPI unsigned char **get_nr0_bmatrix(at *p, int *ps0, int *ps1);
LUSHAPI unsigned char *get_nr1_bvector(at *p, int *psize);
LUSHAPI unsigned char **get_nr1_bmatrix(at *p, int *ps0, int *ps1);


/* FMATOPS.H ---------------------------------------------------- */


LUSHAPI struct array *check_matrix(at *p, int *n, int *m);
LUSHAPI struct array *answer_matrix(at **ans, int *n, int *m);
LUSHAPI struct array *check_vector(at *p, int *n);
LUSHAPI struct array *answer_vector(at **ans, int *n);


/* CLASSIFY.H --------------------------------------------------- */


LUSHAPI real mean(at *l);
LUSHAPI real sdev(at *l);
LUSHAPI real cov(at *l1, at *l2);
LUSHAPI real sum(at *p);
LUSHAPI real sup(at *p);
LUSHAPI real inf(at *p);
LUSHAPI at *rank(at *l, real target, real width);
LUSHAPI real sup_norm(at *l);
LUSHAPI real sup_dist(at *l1, at *l2);
LUSHAPI real abs_norm(at *l);
LUSHAPI real abs_dist(at *l1, at *l2);
LUSHAPI real sqr_norm(at *l);
LUSHAPI real sqr_dist(at *l1, at *l2);
LUSHAPI real hamming_norm(at *l, real margin);
LUSHAPI real hamming_dist(at *l1, at *l2, real margin);
LUSHAPI real quadrant_dist(at *l1, at *l2);
LUSHAPI real solve(real x1, real x2, real (*f) (real));


/* CALLS.H ----------------------------------------------------- */

LUSHAPI at *makelist(int n, at *v);
LUSHAPI int comp_test(at *p, at *q);
LUSHAPI int eq_test (at *p, at *q);


/* OOSTRUCT.H ----------------------------------------------------- */

extern LUSHAPI class class_class;
extern LUSHAPI class object_class;
extern LUSHAPI class number_class;
extern LUSHAPI class gptr_class;
extern LUSHAPI class zombie_class;
extern LUSHAPI struct alloc_root class_alloc;

struct oostruct {
  int size;
  at *class;
  struct oostructitem { at *symb, *val; } slots[1];
};

LUSHAPI void class_define(char *name, class *cl);
LUSHAPI at *defclass(at *classname, at *superclass, at *keylist, at *defaults);
LUSHAPI at *new_oostruct(at *cl);
LUSHAPI at *letslot(at *obj, at *f, at *q, int howmuch);
LUSHAPI at *checksend(class *cl, at *prop);
LUSHAPI at *classof(at *p);
LUSHAPI int is_of_class(at *p, class *cl);
LUSHAPI void delete_at(at *p);
LUSHAPI at *getslot(at*, at*);
LUSHAPI void setslot(at**, at*, at*);


/* DATE.H ----------------------------------------------------- */

#define DATE_YEAR       0
#define DATE_MONTH      1
#define DATE_DAY        2
#define DATE_HOUR       3
#define DATE_MINUTE     4
#define DATE_SECOND     5

extern char *ansidatenames[];
extern class date_class;

LUSHAPI char *str_date( at *p, int *pfrom, int *pto );
LUSHAPI at *new_date( char *s, int from, int to );


/* BINARY.H ----------------------------------------------------- */

enum serialize_action {
  SRZ_SETFL, 
  SRZ_CLRFL,
  SRZ_WRITE,
  SRZ_READ
};

LUSHAPI int bwrite(at *p, FILE *f);
LUSHAPI at *bread(FILE *f, at *opt);
/* serialization functions */
LUSHAPI void serialize_char(char *data, int code);
LUSHAPI void serialize_short(short int *data, int code);
LUSHAPI void serialize_int(int *data, int code);
LUSHAPI void serialize_string(char **data, int code, int maxlen);
LUSHAPI void serialize_chars(void **data, int code, int len);
LUSHAPI void serialize_flt(flt *data, int code);
LUSHAPI void serialize_real(real *data, int code);
LUSHAPI void serialize_float(float *data, int code);
LUSHAPI void serialize_double(double *data, int code);
LUSHAPI int  serialize_atstar(at **data, int code);


/* NAN.H -------------------------------------------------------- */

LUSHAPI flt getnanF (void);
LUSHAPI int isnanF(flt x);
LUSHAPI flt infinityF (void);
LUSHAPI int isinfF(flt x);
LUSHAPI real getnanD (void);
LUSHAPI int  isnanD(real x);
LUSHAPI real infinityD (void);
LUSHAPI int  isinfD(real x);


/* DZ.H ------------------------------------------------------ */

extern class dz_class;

union dz_inst {
  struct op_type {
    short op;
    short arg;
  } code;
  flt constant;
};

struct dz_cell {
  flt (*call)(flt x0, ...);
  int num_arg;
  int program_size;
  int required_stack;
  union dz_inst *program;
};

extern LUSHAPI int dz_trace;
extern LUSHAPI flt dz_stack[DZ_STACK_SIZE];
LUSHAPI flt dz_execute(flt x, struct dz_cell *dz);
LUSHAPI void dz_define(char *name, char *opcode, flt (*cfun)(flt));


/* STORAGE.H --------------------------------------------------- */

extern LUSHAPI class
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

extern LUSHAPI int storage_type_size[ST_LAST];
extern LUSHAPI flt (*storage_type_getf[ST_LAST])(gptr, int);
extern LUSHAPI void (*storage_type_setf[ST_LAST])(gptr, int, flt);

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
    gptr data;
};

struct storage {

  struct srg srg;
  void (*read_srg)(struct storage *);	/* given an index, returns an idx for reading  */
  void (*write_srg)(struct storage *);	/* given an index, returns an idx for writing  */
  void (*rls_srg)(struct storage *);	/* release an idx structure */
  at*  (*getat)(struct storage *,int);  /* given an offset, gets a at value */
  void (*setat)(struct storage *,int,at*); /* given an at at a specific offset */
  at *atst;                     /* pointer on the at storage */
  struct srg *cptr;             /* srg structure for the C side (lisp_c) */
  
  /* Allocation dependent info */
  
  union allinfo {

    struct { 
      gptr addr; 
    } sts_malloc;		/* for malloc... */

#ifdef HAVE_MMAP
    struct {			
      gptr addr;
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

LUSHAPI void storage_read_srg(struct storage *);
LUSHAPI void storage_write_srg(struct storage *st);
LUSHAPI void storage_rls_srg(struct storage *st);

LUSHAPI at *new_AT_storage(void);
LUSHAPI at *new_P_storage(void);
LUSHAPI at *new_F_storage(void);
LUSHAPI at *new_D_storage(void);
LUSHAPI at *new_I32_storage(void);
LUSHAPI at *new_I16_storage(void);
LUSHAPI at *new_I8_storage(void);
LUSHAPI at *new_U8_storage(void);
LUSHAPI at *new_GPTR_storage(void);
LUSHAPI at *new_storage(int,int);
LUSHAPI at *new_storage_nc(int,int);

LUSHAPI void storage_malloc(at*, int, int);
LUSHAPI void storage_realloc(at*, int, int );
LUSHAPI void storage_mmap(at*, at*, int);
LUSHAPI int storagep(at*);
#ifdef MMAPARRAY
LUSHAPI void storage_mmap(void);
#endif
#ifdef DISKARRAY
LUSHAPI void storage_disk(void);
#endif
#ifdef REMOTEARRAY
LUSHAPI void storage_remote(void);
#endif
LUSHAPI void storage_clear(at *p);
LUSHAPI int storage_load(at*, at*);
LUSHAPI void storage_save(at*, at*);
LUSHAPI void init_storage(void);

/* INDEX.H ---------------------------------------------- */

#define MAXDIMS 8

#define IDF_HAS_NR0  1
#define IDF_HAS_NR1  2
#define IDF_UNSIZED  4

extern class index_class;

/* The "light" idx structure */

#define IDX_DATA_PTR(idx) \
    (gptr) ((char *) (idx)->srg->data + \
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

LUSHAPI int indexp(at*); 
LUSHAPI int matrixp(at*); 
LUSHAPI int arrayp(at*);
LUSHAPI at *new_index(at*);
LUSHAPI void index_dimension(at*,int,int[]);
LUSHAPI void index_undimension(at*);
LUSHAPI void index_from_index(at*,at*,int*,int*);

LUSHAPI struct index *easy_index_check(at*,int,int[]);
LUSHAPI real easy_index_get(struct index*, int*);
LUSHAPI void easy_index_set(struct index*, int*, real);

LUSHAPI char *not_a_nrvector(at*);
LUSHAPI char *not_a_nrmatrix(at*);
LUSHAPI flt *make_nrvector(at*,int,int*);
LUSHAPI flt **make_nrmatrix(at*,int,int,int*,int*);

LUSHAPI at *AT_matrix(int,int*);	/* Simultaneous creation       */
LUSHAPI at *F_matrix(int,int*);		/* of an index and its storage */
LUSHAPI at *D_matrix(int,int*);
LUSHAPI at *I32_matrix(int,int*);
LUSHAPI at *I16_matrix(int,int*);
LUSHAPI at *I8_matrix(int,int*);
LUSHAPI at *U8_matrix(int,int*);
LUSHAPI at *GPTR_matrix(int,int*);

/* Functions related to <struct idx> objects */

LUSHAPI void index_read_idx(struct index *, struct idx *);
LUSHAPI void index_write_idx(struct index *, struct idx *);
LUSHAPI void index_rls_idx(struct index *, struct idx *);


/* Other functions */

LUSHAPI at *load_matrix(at*);
LUSHAPI void save_flt_matrix(at*,at*);
LUSHAPI void save_packed_matrix(at*,at*);
LUSHAPI void save_ascii_matrix(at*,at*);

LUSHAPI void init_index(void);



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


/* CHECK_FUNC.H ---------------------------------------------- */

void check_main_maout(struct idx *, struct idx *);
void check_main_main_maout(struct idx *, struct idx *, struct idx *);
void check_main_m0out(struct idx *, struct idx *);
void check_main_main_m0out(struct idx *, struct idx *, struct idx *);
void check_main_m0in_maout(struct idx *, struct idx *, struct idx *);
void check_main_main_maout_dot21(struct idx *, struct idx *, struct idx *);
void check_main_main_maout_dot42(struct idx *, struct idx *, struct idx *);
void check_m1in_m1in_m2out(struct idx *, struct idx *, struct idx *);
void check_m2in_m2in_m4out(struct idx *, struct idx *, struct idx *);
void is_sized(struct idx *);
void Csame_size(struct idx *, struct idx *);
void size_or_check(struct idx *, struct idx *);


/* LISP_C.H ---------------------------------------------- */

/*
void cside_create_idx();
void cside_create_srg();
void cside_create_obj();
void cside_create_str();
void cside_destroy_item();
void cside_destroy_range();
*/
void lside_destroy_item(gptr);

/*
void lside_dld_partial();
*/

/* CPLUSPLUS --------------------------------------------------- */

#ifdef __cplusplus
}
#ifdef class
#undef class
#endif
#ifdef true
#undef true
#endif
#endif


#endif /* HEADER_H */
