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
 * $Id: header.h,v 1.17 2002-05-06 21:29:07 leonb Exp $
 **********************************************************************/

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

#define TLOPEN_MAJOR  40
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
extern TLAPI int break_attempt;
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
TLAPI FILE* unix_popen(const char *, const char *);
TLAPI int   unix_pclose(FILE *);
#endif


#ifdef WIN32
/* interruptions */
extern TLAPI int break_attempt;
extern TLAPI int kill_attempt;
TLAPI void lastchance(char *s) no_return;
/* system override */
TLAPI void  win32_exit(int);
TLAPI int   win32_isatty(int);
TLAPI void  win32_user_break(char *s);
TLAPI FILE* win32_popen(char *, char *);
TLAPI int   win32_pclose(FILE *);
TLAPI void  win32_fmode_text(FILE *f);
TLAPI void  win32_fmode_binary(FILE *f);
TLAPI int   win32_waitproc(void *wproc);
/* event management */
TLAPI void set_trigger(void (*trigger)(void), void (*process)(void)); 
TLAPI void block_async_trigger(void);
TLAPI void unblock_async_trigger(void);
TLAPI void call_sync_trigger(void);
TLAPI void wait_trigger(void);
/* console management */
TLAPI void console_getline(char *prompt, char *buf, int size);
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

/* specials flags improving the tests speed */

#define   X_SYMBOL      (1<<8 )
#define   X_FUNCTION    (1<<9 )
#define   X_STRING      (1<<10)
#define   X_OOSTRUCT    (1<<11)
#define   X_ZOMBIE      (1<<12) /* zombie means nil */
#define   X_STORAGE     (1<<13)	
#define   X_INDEX       (1<<14)	

/* Some useful macros */

#define LOCK(x)         { if (x) (x)->count++; }
#define UNLOCK(x)       { if ( (x) && --((x)->count)==0 )  purge(x); }

#define CONSP(x)        ((x)&&((x)->flags&C_CONS))
#define LISTP(x)        (!(x)||((x)->flags&C_CONS))
#define NUMBERP(x)	((x)&&((x)->flags&C_NUMBER))
#define GPTRP(x)	((x)&&((x)->flags&C_GPTR))
#define EXTERNP(x,cl)	((x)&&((x)->flags&C_EXTERN)&&((x)->Class==(cl)))

extern TLAPI at *(*eval_ptr) (at*);
extern TLAPI at *(*argeval_ptr) (at*);

#define NEW_NUMBER(x)   new_number((real)(x))
#define NEW_GPTR(x)     new_gptr((gptr)(x))
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
  at*              classname;   /* class name */
  at*              priminame;   /* class name for binary files */
  at*              backptr;
  int              slotssofar;    
  at*              keylist;
  at*              defaults;
  at*              atsuper;
  struct class*    super;	/* link to superclass */
  struct class*    subclasses;	/* link to subclasses */
  struct class*    nextclass;	/* next subclass      */
  at*              methods;
  struct hashelem* hashtable;
  int              hashsize;
  char	    	   hashok;
  char             goaway;
  char 	           dontdelete;
} class;


struct hashelem {
  at *symbol;
  at *function;
  int sofar;
};


TLAPI void purge (register at *q);
TLAPI at *new_gptr(gptr x);
TLAPI at *new_number(double x);
TLAPI at *new_extern(class *cl, void *obj);
TLAPI at *new_cons(at *car, at *cdr);
TLAPI at *cons(at *car, at *cdr);
TLAPI at *car(at *q);
TLAPI at *caar(at *q);
TLAPI at *cadr(at *q);
TLAPI at *cdr(at *q);
TLAPI at *cdar(at *q);
TLAPI at *cddr(at *q);
TLAPI at *rplaca(at *q, at *p);
TLAPI at *rplacd(at *q, at *p);
TLAPI at *displace(at *q, at *p);
TLAPI int length(at *p);
TLAPI at *member(at *elem, at *list);
TLAPI at *nfirst(int n, at *l);
TLAPI at *nth(at *l, int n);
TLAPI at *nthcdr(at *l, int n);
TLAPI at *last(at *list);
TLAPI at *lastcdr(at *list);
TLAPI at *flatten(at *l);
TLAPI at *append(at *l1, at *l2);
TLAPI at *reverse(at *l);
TLAPI at *assoc(at *k, at *l);
TLAPI int used(void);

TLAPI void generic_dispose(at *p);
TLAPI void generic_action(at *p, void (*action) (at*));
TLAPI char *generic_name(at *p);
TLAPI at *generic_eval(at *p);
TLAPI at *generic_listeval(at *p, at *q);
#define generic_serialize NULL
#define generic_compare   NULL
#define generic_hash      NULL
#define generic_getslot   NULL
#define generic_setslot   NULL


/* EVAL.H ----------------------------------------------------- */

TLAPI at *eval_std(at *p);
TLAPI at *eval_debug(at *q);
TLAPI at *eval_nothing(at *q);
TLAPI at *apply(at *q, at *p);
TLAPI at *progn(at *p);
TLAPI at *prog1(at *p);
TLAPI at *mapc(at *f, at **listes, int arg_number);
TLAPI at *rmapc(at *f, at **listes, int arg_number);
TLAPI at *mapcar(at *f, at **listes, int arg_number);
TLAPI at *rmapcar(at *f, at **listes, int arg_number);




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

TLAPI gptr allocate(struct alloc_root *ar);
TLAPI void deallocate(struct alloc_root *ar, struct empty_alloc *elem);
TLAPI void garbage(int flag);
TLAPI void protect(at *q);
TLAPI void unprotect(at *q);

/* allocation functions */
LUSHAPI void *lush_malloc(int,char*,int);
LUSHAPI void *lush_calloc(int,int,char*,int);
LUSHAPI void *lush_realloc(gptr,int,char*,int);
LUSHAPI void lush_free(gptr,char*,int);
LUSHAPI void lush_cfree(gptr,char*,int);
LUSHAPI void set_malloc_file(char*);

/* malloc debug file (from sn3.2) */
#define malloc(x)    lush_malloc(x,__FILE__,__LINE__)
#define calloc(x,y)  lush_calloc(x,y,__FILE__,__LINE__)
#define realloc(x,y) lush_realloc(x,y,__FILE__,__LINE__)
#define free(x)      lush_free(x,__FILE__,__LINE__)
#define cfree(x)     lush_cfree(x,__FILE__,__LINE__)

/* tl compatible malloc functions */
#define tl_malloc(x)    lush_malloc(x,__FILE__,__LINE__)
#define tl_realloc(x,y) lush_realloc(x,y,__FILE__,__LINE__)

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


/* SYMBOL.H ---------------------------------------------------- */


struct hash_name {		/* this one contains the symbol's names this
				 * one contains the symbol's names */
  short used;
  char name[NAMELENGTH];
  at *named;
  struct hash_name *next;

};

/* now the hash table declaration */
extern TLAPI struct hash_name *names[];

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


extern TLAPI class symbol_class;

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


TLAPI at *new_symbol(char *s);
TLAPI at *named(char *s);
TLAPI char *nameof(at *p);
TLAPI void symbol_push (at *p, at *q);
TLAPI void symbol_pop (at *p);
TLAPI at *setq(at *p, at *q);	/* Warning: Never use the result. */
TLAPI at *symblist(void); 
TLAPI at *oblist(void);
TLAPI at *true(void);
TLAPI void var_set(at *p, at *q);
TLAPI void var_SET(at *p, at *q); /* Set variable regardless of lock mode */
TLAPI void var_lock(at *p);
TLAPI at *var_get(at *p);
TLAPI at *var_define(char *s);




/* TOPLEVEL.H ------------------------------------------------- */


struct recur_elt { 
  struct recur_elt *next; 
  void *call; 
  at *p;
};

extern TLAPI struct recur_doc {
  /* hash table for detecting infinite recursion */
  int hsize;
  int hbuckets;
  struct recur_elt **htable;
} recur_doc;


extern TLAPI struct error_doc {	   
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

extern TLAPI struct context {
  struct context *next;
  sigjmp_buf error_jump;
  FILE *input_file;
  FILE *output_file;
  short input_tab;
  short output_tab;
} *context;

TLAPI void context_push(struct context *newc);
TLAPI void context_pop(void);
TLAPI int  recur_push_ok(struct recur_elt *elt, void *call, at *p);
TLAPI void recur_pop(struct recur_elt *elt);
TLAPI void toplevel(char *in, char *out, char *new_prompt);
TLAPI void error(char *prefix, char *text, at *suffix) no_return;
TLAPI void user_break(char *s);
TLAPI void init_lush (char *program_name);
TLAPI void start_lisp(int argc, char **argv);
TLAPI void clean_up(void);
TLAPI void abort (char *s) no_return;



/* STRING.H ---------------------------------------------------- */

extern TLAPI class string_class;

struct string {
  int flag;
  char *start;
  void *cptr;
};

#define STRING_UNUSED    0
#define STRING_SAFE      1
#define STRING_ALLOCATED 2

#define SADD(str)       (((struct string *)(str))->start)

TLAPI at *new_string(char *s);
TLAPI at *new_safe_string(char *s);
TLAPI at *new_string_bylen(int n);
TLAPI at *str_val(char *s);
TLAPI char *str_number(double x);
TLAPI char *str_number_hex(double x);
TLAPI char *str_gptr(gptr x);

TLAPI char *regex_compile(char *pattern, short int *bufstart, short int *bufend,
			  int strict, int *rnum);
TLAPI int regex_exec(short int *buffer, char *string, 
		     char **regptr, int *reglen, int nregs);
TLAPI int regex_seek(short int *buffer, char *string, char *seekstart, 
		     char **regptr, int *reglen, int nregs, 
		     char **start, char **end);

extern TLAPI char *string_buffer;
extern TLAPI char null_string[];
extern TLAPI char digit_string[];
extern TLAPI char special_string[];
extern TLAPI char aspect_string[];


/* FUNCTION.H -------------------------------------------------- */

/*
 * function are implemented as external objects 
 * pointing to this structure:
 */

struct cfunction {
  int used;
  at *name;
  void *call;
  void *info;
};

struct lfunction {
  int used;
  at *formal_arg_list;
  at *evaluable_list;
};

extern TLAPI class de_class;
extern TLAPI class df_class;
extern TLAPI class dm_class;
extern TLAPI class dx_class;		/* dx functions are external C_function */
extern TLAPI class dy_class;		/* dy functions have unflattened args. */

extern TLAPI at **dx_stack, **dx_sp;
extern TLAPI struct alloc_root function_alloc;

TLAPI at *new_de(at *formal, at *evaluable);
TLAPI at *new_df(at *formal, at *evaluable);
TLAPI at *new_dm(at *formal, at *evaluable);
TLAPI at *new_dx(at *name, at *(*addr)(int,at**));
TLAPI at *new_dy(at *name, at *(*addr)(at *));
TLAPI at *funcdef(at *f);
TLAPI at *eval_a_list(at *p);
TLAPI gptr need_error(int i, int j, at **arg_array_ptr);
TLAPI void arg_eval(at **arg_array, int i);
TLAPI void all_args_eval(at **arg_array, int i);

/* This is the interface header builder */

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
#define ASYMBOL(i)      ( ISSYMBOL(i) ? APOINTER(i)->Object:DX_ERROR(7,i) )
#define AOBJECT(i)      ( ISOBJECT(i) ? APOINTER(i)->Object:DX_ERROR(8,i) )
#define AINDEX(i)       ( ISINDEX(i) ? APOINTER(i)->Object:DX_ERROR(10,i) )

#define ARG_NUMBER(i)	if (arg_number != i)  DX_ERROR(0,i);
#define ARG_EVAL(i)	arg_eval(arg_array,i)
#define ALL_ARGS_EVAL	all_args_eval(arg_array,arg_number)


/* FILEIO.H ------------------------------------------------- */

extern TLAPI class file_R_class, file_W_class;
extern TLAPI char *path[];
extern TLAPI char file_name[];

#define OPEN_READ(f,s)  new_extern(&file_R_class,open_read(f,s))
#define OPEN_WRITE(f,s) new_extern(&file_W_class,open_write(f,s))

TLAPI char *cwd(char *s);
TLAPI at *files(char *s);
TLAPI int dirp(char *s);
TLAPI int filep(char *s);
TLAPI char *dirname(char *fname);
TLAPI char *basename(char *fname, char *suffix);
TLAPI char *concat_fname(char *from, char *fname);
TLAPI void clean_tmp_files(void);
TLAPI char *tmpname(char *s, char *suffix);
TLAPI char *search_file(char *s, char *suffixes);
TLAPI void test_file_error(FILE *f);
TLAPI FILE *open_read(char *s, char *suffixes);
TLAPI FILE *open_write(char *s, char *suffixes);
TLAPI FILE *open_append(char *s, char *suffixes);
TLAPI FILE *attempt_open_read(char *s, char *suffixes);
TLAPI FILE *attempt_open_write(char *s, char *suffixes);
TLAPI FILE *attempt_open_append(char *s, char *suffixes);
TLAPI void file_close(FILE *f);
TLAPI void set_script(char *s);
TLAPI int read4(FILE *f);
TLAPI int write4(FILE *f, unsigned int l);
TLAPI long file_size(FILE *f);
#ifndef HAVE_STRERROR
TLAPI char *strerror(int errno);
#endif


/* IO.H ----------------------------------------------------- */


extern TLAPI char *line_pos;
extern TLAPI char *line_buffer;
extern TLAPI char *prompt_string;

TLAPI void print_char (char c);
TLAPI void print_string(char *s);
TLAPI void print_list(at *list);
TLAPI void print_tab(int n);
TLAPI char *pname(at *l);
TLAPI char *first_line(at *l);
TLAPI char read_char(void);
TLAPI char next_char(void);
TLAPI int ask (char *t);
TLAPI char *read_string(char *s);
TLAPI char *read_word(void);
TLAPI char *dmc(char *s, at *l);
TLAPI char skip_char(char *s);
TLAPI at *read_list(void);



/* HTABLE.H ------------------------------------------------- */

extern TLAPI class htable_class;

TLAPI unsigned long hash_value(at *);
TLAPI unsigned long hash_pointer(at *);
TLAPI at  *new_htable(int nelems, int pointerhashp);
TLAPI void htable_set(at *htable, at *key, at *value);
TLAPI at  *htable_get(at *htable, at *key);


/* CLASSIFY.H --------------------------------------------------- */

TLAPI real mean(at *l);
TLAPI real sdev(at *l);
TLAPI real cov(at *l1, at *l2);
TLAPI real sum(at *p);
TLAPI real sup(at *p);
TLAPI real inf(at *p);
TLAPI at *rank(at *l, real target, real width);
TLAPI real sup_norm(at *l);
TLAPI real sup_dist(at *l1, at *l2);
TLAPI real abs_norm(at *l);
TLAPI real abs_dist(at *l1, at *l2);
TLAPI real sqr_norm(at *l);
TLAPI real sqr_dist(at *l1, at *l2);
TLAPI real hamming_norm(at *l, real margin);
TLAPI real hamming_dist(at *l1, at *l2, real margin);
TLAPI real quadrant_dist(at *l1, at *l2);
TLAPI real solve(real x1, real x2, real (*f) (real));


/* CALLS.H ----------------------------------------------------- */

TLAPI at *makelist(int n, at *v);
TLAPI int comp_test(at *p, at *q);
TLAPI int eq_test (at *p, at *q);


/* OOSTRUCT.H ----------------------------------------------------- */

extern TLAPI class class_class;
extern TLAPI class object_class;
extern TLAPI class number_class;
extern TLAPI class gptr_class;
extern TLAPI class zombie_class;
extern TLAPI struct alloc_root class_alloc;

struct oostruct {
  int size;
  at *class;
  void *cptr;
  struct oostructitem { at *symb, *val; } slots[1];
};

TLAPI at *new_ooclass(at *classname, at *superclass, at *keylist, at *defaults);
TLAPI void putmethod(class *cl, at *name, at *fun);
TLAPI at *new_oostruct(at *cl);
TLAPI at *letslot(at *obj, at *f, at *q, int howmuch);
TLAPI at *checksend(class *cl, at *prop);
TLAPI at *send_message(at *classname, at *obj, at *method, at *args);
TLAPI at *classof(at *p);
TLAPI int is_of_class(at *p, class *cl);
TLAPI void delete_at(at *p);
TLAPI at *getslot(at*, at*);
TLAPI void setslot(at**, at*, at*);


/* MODULE.H --------------------------------------------------- */

extern LUSHAPI class module_class;

TLAPI void class_define(char *name, class *cl);
TLAPI void dx_define(char *name, at *(*addr) (int, at **));
TLAPI void dy_define(char *name, at *(*addr) (at *));

LUSHAPI at *find_primitive(at *module, at *name);
LUSHAPI at *module_load(char *filename);
LUSHAPI void module_unload(at *atmodule);


/* DATE.H ----------------------------------------------------- */

#define DATE_YEAR       0
#define DATE_MONTH      1
#define DATE_DAY        2
#define DATE_HOUR       3
#define DATE_MINUTE     4
#define DATE_SECOND     5

extern char *ansidatenames[];
extern class date_class;

TLAPI char *str_date( at *p, int *pfrom, int *pto );
TLAPI at *new_date( char *s, int from, int to );


/* BINARY.H ----------------------------------------------------- */

enum serialize_action {
  SRZ_SETFL, 
  SRZ_CLRFL,
  SRZ_WRITE,
  SRZ_READ
};

TLAPI int bwrite(at *p, FILE *f);
TLAPI at *bread(FILE *f, at *opt);

/* serialization functions */
TLAPI void serialize_char(char *data, int code);
TLAPI void serialize_short(short int *data, int code);
TLAPI void serialize_int(int *data, int code);
TLAPI void serialize_string(char **data, int code, int maxlen);
TLAPI void serialize_chars(void **data, int code, int len);
TLAPI void serialize_flt(flt *data, int code);
TLAPI void serialize_real(real *data, int code);
TLAPI void serialize_float(float *data, int code);
TLAPI void serialize_double(double *data, int code);
TLAPI int  serialize_atstar(at **data, int code);
TLAPI FILE *serialization_file_descriptor(int code);

/* NAN.H -------------------------------------------------------- */

TLAPI flt getnanF (void);
TLAPI int isnanF(flt x);
TLAPI flt infinityF (void);
TLAPI int isinfF(flt x);
TLAPI real getnanD (void);
TLAPI int  isnanD(real x);
TLAPI real infinityD (void);
TLAPI int  isinfD(real x);


/* DZ.H ------------------------------------------------------ */

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

extern LUSHAPI class dz_class;
extern LUSHAPI int dz_trace;
extern LUSHAPI flt dz_stack[DZ_STACK_SIZE];
LUSHAPI flt dz_execute(flt x, struct dz_cell *dz);
LUSHAPI void dz_define(char *name, char *opcode, flt (*cfun)(flt));


/* STORAGE.H --------------------------------------------------- */

extern LUSHAPI class  AT_storage_class;
extern LUSHAPI class   P_storage_class;
extern LUSHAPI class   F_storage_class;
extern LUSHAPI class   D_storage_class;
extern LUSHAPI class I32_storage_class;
extern LUSHAPI class I16_storage_class;
extern LUSHAPI class  I8_storage_class;
extern LUSHAPI class  U8_storage_class;
extern LUSHAPI class PTR_storage_class;

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
extern LUSHAPI real (*storage_type_getr[ST_LAST])(gptr, int);
extern LUSHAPI void (*storage_type_setr[ST_LAST])(gptr, int, real);

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
  void (*read_srg)(struct storage *);	
  void (*write_srg)(struct storage *);	
  void (*rls_srg)(struct storage *);	
  at*  (*getat)(struct storage *,int);
  void (*setat)(struct storage *,int,at*);
  at *atst;          /* pointer on the at storage */
  struct srg *cptr;  /* srg structure for the C side (lisp_c) */
  
  /* Allocation dependent info */
  
  union allinfo {

    struct { 
      gptr addr; 
    } sts_malloc;		/* for malloc... */

#ifdef HAVE_MMAP
    struct {			
      gptr addr;
      gptr xtra;
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
LUSHAPI void storage_mmap(at*, FILE*, int);
LUSHAPI int storagep(at*);
LUSHAPI void storage_clear(at *p);
LUSHAPI int storage_load(at*, FILE*);
LUSHAPI void storage_save(at*, FILE*);


/* INDEX.H ---------------------------------------------- */

#define MAXDIMS 8

#define IDF_HAS_NR0  1
#define IDF_HAS_NR1  2
#define IDF_UNSIZED  4

extern LUSHAPI class index_class;

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
LUSHAPI at *copy_matrix(at *, at *);

LUSHAPI at *AT_matrix(int,int*);	/* Simultaneous creation       */
LUSHAPI at *F_matrix(int,int*);	/* of an index and its storage */
LUSHAPI at *D_matrix(int,int*);
LUSHAPI at *P_matrix(int,int*);
LUSHAPI at *I32_matrix(int,int*);
LUSHAPI at *I16_matrix(int,int*);
LUSHAPI at *I8_matrix(int,int*);
LUSHAPI at *U8_matrix(int,int*);
LUSHAPI at *GPTR_matrix(int,int*);

/* Functions related to <struct idx> objects */

LUSHAPI void index_read_idx(struct index *, struct idx *);
LUSHAPI void index_write_idx(struct index *, struct idx *);
LUSHAPI void index_rls_idx(struct index *, struct idx *);

/* Other functions (TL compatible) */
TLAPI void import_raw_matrix(at *p, FILE *f, int offset);
TLAPI void import_text_matrix(at *p, FILE *f);
TLAPI int save_matrix_len (at *p);
TLAPI void save_matrix(at *p, FILE *f);
TLAPI void export_matrix(at *p, FILE *f);
TLAPI void save_ascii_matrix(at *p, FILE *f);
TLAPI void export_ascii_matrix(at *p, FILE *f);
TLAPI at *load_matrix(FILE *f);


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
    } while (_j_<(idx)->ndim);  					     \
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

#ifndef CHECK_FUNC_H
#include "check_func.h"
#endif


/* DH.H -------------------------------------------------- */

/*
 * DH are C functions working on matrices and numbers.
 * They may be compiled using 'dh-compile'.
 * 
 */

extern LUSHAPI class dh_class;
extern LUSHAPI class cclass_class;

/* LISP_C.H ---------------------------------------------- */

extern LUSHAPI void lside_dld_partial(gptr);
extern LUSHAPI void lside_destroy_item(gptr);

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
