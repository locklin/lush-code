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

#include "cmm.h"

/* VERSION.H --------------------------------------------------- */

#define LUSH_MAJOR  50
#define LUSH_MINOR  01
#define TLOPEN_MAJOR  LUSH_MAJOR
#define TLOPEN_MINOR  LUSH_MINOR


/* MISC.H ------------------------------------------------------ */


#define ifn(s)          if(!(s))
#define forever         for(;;)
#define until(s)        while(!(s))

#if defined(__GNUC__)
#define no_return __attribute__((__noreturn__))
#endif
#ifndef no_return
#define no_return /**/
#endif

/* Actually defined in dh.h */
typedef struct dhclassdoc_s dhclassdoc_t;
typedef struct dhdoc_s dhdoc_t;

/* From main */
extern LUSHAPI int lush_argc;
extern LUSHAPI char** lush_argv;


/* Large integers:
   Lush storages and matrices on 64 bit platforms
   are limited by the size of integers (often 32 bits).
   We are fixing this in two steps:
   1- Replace all relevant occurences of 'int' by 'intg'.
   2- Change the definition of intx from 'int' to 'long'.
*/

#ifdef INTG_IS_LONG
typedef long intg;
#else
typedef int intg;
#endif

/* Dealing with errors. */

/* lifted from TOPLEVEL.H; find better solution */
typedef struct at at;
LUSHAPI void error(const char *prefix, const char *text, at *suffix) no_return;

LUSHAPI char *api_translate_c2lisp(const char*);
LUSHAPI char *api_translate_lisp2c(const char*);

#ifdef __ICC
#pragma warning (disable:279)
#endif
#define RAISE(caller, msg, p)   \
   ((msg) ? (error(caller, msg, p),0) : 0) 

#define RAISEF(msg, p) \
  ((msg) ? (error(api_translate_c2lisp(__func__), msg, p),0) : 0)

#define RAISEFX(msg, p) \
  ((msg) ? (error(api_translate_c2lisp(__func__)+1, msg, p),0) : 0)


/* OS.H ---------------------------------------------------------- */


#ifdef UNIX
/* interruptions */
extern LUSHAPI int break_attempt;
LUSHAPI void lastchance(const char *s) no_return;
/* unix hooks */
void init_unix(void);
void fini_unix(void);
void toplevel_unix(void);
void console_getline(char *prompt, char *buf, int size);
/* openTL entry points */
void init_user(void);
int  init_user_dll(int major, int minor);
/* replacement functions */
LUSHAPI pid_t filteropen(const char *cmd, FILE **pfw, FILE **pfr);
LUSHAPI pid_t filteropenpty(const char *cmd, FILE **pfw, FILE **pfr);
LUSHAPI FILE* unix_popen(const char *cmd, const char *mode);
LUSHAPI int   unix_pclose(FILE *f);
LUSHAPI int   unix_setenv(const char *name, const char *value);
/* cygwin */
# ifdef __CYGWIN32__
LUSHAPI void cygwin_fmode_text(FILE *f);
LUSHAPI void cygwin_fmode_binary(FILE *f);
# endif
#endif

#ifdef WIN32
/* interruptions */
extern LUSHAPI int break_attempt;
extern LUSHAPI int kill_attempt;
LUSHAPI void lastchance(const char *s) no_return;
/* system override */
LUSHAPI void  win32_exit(int);
LUSHAPI int   win32_isatty(int);
LUSHAPI void  win32_user_break(char *s);
LUSHAPI FILE* win32_popen(char *, char *);
LUSHAPI int   win32_pclose(FILE *);
LUSHAPI void  win32_fmode_text(FILE *f);
LUSHAPI void  win32_fmode_binary(FILE *f);
LUSHAPI int   win32_waitproc(void *wproc);
/* console management */
LUSHAPI void console_getline(char *prompt, char *buf, int size);
/* openTL entry points */
void init_user(void);
DLLEXPORT int init_user_dll(int major, int minor);
#endif



/* AT.H -------------------------------------------------------- */

#define NUM_PTRBITS       3
#define PTRBITS(p)        ((uintptr_t)(p) & ((1<<NUM_PTRBITS) - 1))
#define SET_PTRBIT(p, b)  { p = (void *)((uintptr_t)(p) | b); }
#define UNSET_PTRBIT(p,b) { p = (void *)((uintptr_t)(p) & ~b); }
#define CLEAR_PTR(p)      ((void *)((uintptr_t)(p)&~((1<<NUM_PTRBITS) - 1)))

#define CONS_BIT          1

typedef struct class_s class_t;

extern LUSHAPI class_t *class_class;
extern LUSHAPI class_t *object_class;
extern LUSHAPI class_t *cons_class;
extern LUSHAPI class_t *nil_class;
extern LUSHAPI class_t *number_class;
extern LUSHAPI class_t *gptr_class;
extern LUSHAPI class_t *mptr_class;
extern LUSHAPI class_t *window_class;

struct at {
   union {
      class_t *cl;
      struct at *car;
   } head;
   union {
      double *d;
      void   *p;
      const char *c;
      struct lush_symbol *s;
      struct at *cdr;
   } payload;
};

#define Class(q)  classof(q)
#define Number(q) (*(q)->payload.d)
#define String(q) ((q)->payload.c)
#define Symbol(q) ((q)->payload.s)
#define Value(q)  (Symbol(q)->valueptr ? *Symbol(q)->valueptr : NIL)
#define Gptr(q)   ((q)->payload.p)
#define Mptr(q)   ((q)->payload.p)
#define Car(q)    ((at *)CLEAR_PTR((q)->head.car))
#define Cdr(q)    ((q)->payload.cdr)
#define Caar(q)   Car(Car(q))
#define Cadr(q)   Car(Cdr(q))
#define Cdar(q)   Cdr(Car(q))
#define Cddr(q)   Cdr(Cdr(q))

#define AssignClass(q, _cl) ((q)->head.cl) = _cl
#define AssignCar(q, _car)  (q)->head.car = (struct at *)((uintptr_t)(_car) | CONS_BIT)


#define CONSP(p)        ((p)&&(((uintptr_t)((p)->head.cl)) & CONS_BIT))
#define FUNCTIONP(x)    ((x)&&(Class(x)->super == function_class))
#define LASTCONSP(x)    (CONSP(x) && !CONSP(Cdr(x)))
#define LISTP(x)        (!(x)||(Class(x) == cons_class))
#define NUMBERP(x)	((x)&&(Class(x) == number_class))
#define GPTRP(x)	((x)&&(Class(x) == gptr_class))
#define MPTRP(x)	((x)&&(Class(x) == mptr_class))
#define OBJECTP(x)      ((x)&&(Class(x)->dispose == object_class->dispose))
#define CLASSP(x)       ((x)&&(Class(x) == class_class))
#define SYMBOLP(x)      ((x)&&(Class(x) == symbol_class))
#define STORAGEP(x)     ((x)&&(Class(x)->super == abstract_storage_class))
#define INDEXP(x)       ((x)&&(Class(x) == index_class))
#define STRINGP(x)      ((x)&&(Class(x) == string_class))
#define ZOMBIEP(x)      ((x)&&(Class(x) == nil_class))
#define WINDOWP(x)      ((x)&&(Class(x) == window_class))

#define HAS_BACKPTR_P(p) ((p)&&(Class(p)== index_class || \
                                Class(p)->dispose == object_class->dispose || \
                                Class(p)->super == abstract_storage_class || \
                                Class(p) == window_class) )
#define MANAGEDP(p)     ((p)&&(((uintptr_t)((p)->head.cl)) & MANAGED_BIT))

extern LUSHAPI at *(*eval_ptr) (at*);
#define eval(q)    (*eval_ptr)(q)

/*
 * The class structure defines the behavior of
 * each type of object.
 */

typedef void *dispose_func_t(void *);
struct class_s {
   /* class vectors */
   void*          (*dispose)      (void *);
   const char*    (*name)         (at*);
   at*            (*selfeval)     (at*);
   at*            (*listeval)     (at*, at*);
   void           (*serialize)    (at**, int);
   int            (*compare)      (at*, at*, int);
   unsigned long  (*hash)         (at*);
   at*		  (*getslot)      (at*, at*);
   void           (*setslot)      (at*, at*, at*);
   /* class information */
   at*              classname;   /* class name */
   at*              priminame;   /* class name for binary files */
   at*              backptr;     /* back pointer to class object */
   at*              atsuper;     /* superclass object */
   struct class_s*  super;	 /* link to superclass */
   struct class_s*  subclasses;	 /* link to subclasses */
   struct class_s*  nextclass;	 /* next subclass of the same superclass */
   at*             *slots;       /* names (symbols) of all slots */
   at*             *defaults;    /* defaults for all slots */
   int              num_slots;   /* number of slots */  
   int              num_cslots;  /* number of slots in compiled object */  
   at*              myslots;     /* symbols of slots excluding superclass's */
   at*              mydefaults;  /* default values excluding superclass's */
   at*              methods;     /* alist of methods */
   struct hashelem* hashtable;   /* buckets for hashed methods */
   int              hashsize;    /* number of buckets */
   bool	    	    hashok;      /* is the hash table up-to-date */
   bool 	    dontdelete;  /* instances should not be deleted */
   bool             live;        /* true if class is current */
   bool             managed;     /* object address is a managed address */
   /* additional info for dhclasses */
   bool             has_compiled_part;
   dhclassdoc_t    *classdoc;  
   char            *kname;
};

struct hashelem {
   at *symbol;
   at *function;
   int sofar;
};


LUSHAPI at *new_cons(at *car, at *cdr);
LUSHAPI at *new_at(class_t *cl, void *obj);
LUSHAPI at *new_at_number(double x);
LUSHAPI static inline at *new_at_gptr(gptr x)
{
   extern class_t *gptr_class;
   return new_at(gptr_class, x);
}
LUSHAPI static inline at *new_at_mptr(gptr x)
{
   extern class_t *mptr_class;
   return new_at(mptr_class, x);
}

LUSHAPI void zombify(at *p);
extern  at  *at_NULL;

#define NEW_NUMBER(x)   new_at_number((real)(x))
#define NEW_GPTR(x)     new_at_gptr((gptr)(x))
#define NEW_MPTR(x)     new_at_mptr((gptr)(x))
#define NEW_BOOL(x)     ((x) ? t() : NIL)

/* number.h */

LUSHAPI bool integerp(double x);
LUSHAPI bool oddp(double x);
LUSHAPI bool evenp(double x);

/* LIST.H ----------------------------------------------------- */

LUSHAPI at *car(at *q);
LUSHAPI at *caar(at *q);
LUSHAPI at *cadr(at *q);
LUSHAPI at *cdr(at *q);
LUSHAPI at *cdar(at *q);
LUSHAPI at *cddr(at *q);
LUSHAPI at *rplaca(at *q, at *p);
LUSHAPI at *rplacd(at *q, at *p);
LUSHAPI at *displace(at *q, at *p);
LUSHAPI at *make_list(int n, at *v);
LUSHAPI at *vector2list(int n, at **v);
LUSHAPI at *copy_tree(at *p);
LUSHAPI int length(at *p);
LUSHAPI at *member(at *elem, at *list);
LUSHAPI at *nfirst(int n, at *l);
LUSHAPI at *nth(at *l, int n);
LUSHAPI at *nthcdr(at *l, int n);
LUSHAPI at *lasta(at *list);
LUSHAPI at *last(at *list, int n);
LUSHAPI at *flatten(at *l);
LUSHAPI at *append(at *l1, at *l2);
LUSHAPI at *reverse(at *l);
LUSHAPI at *assoc(at *k, at *l);



/* EVAL.H ----------------------------------------------------- */

LUSHAPI at *eval_std(at *p);
LUSHAPI at *eval_brk(at *p);
LUSHAPI at *eval_debug(at *q);
LUSHAPI at *call_stack(void);
LUSHAPI at *quote(at *p);
LUSHAPI at *apply(at *q, at *p);
LUSHAPI at *progn(at *p);
LUSHAPI at *prog1(at *p);
LUSHAPI at *mapc(at *f, at **listv, int n);
LUSHAPI at *mapcar(at *f, at **listv, int n);
LUSHAPI at *mapcan(at *f, at **listv, int n);


/* weakref.h */

typedef void wr_notify_func_t(void *, void *);
LUSHAPI void add_notifier(void *, wr_notify_func_t *, void *);
LUSHAPI void run_notifiers(void *);
LUSHAPI void del_notifiers_with_context(void *);
LUSHAPI void del_notifiers_for_target(void *);
LUSHAPI void protect(at *q);
LUSHAPI void unprotect(at *q);


/* cref.h */

extern LUSHAPI class_t *cref_class;
#define CREFP(x) ((x)&&(Class(x)->super == cref_class))
LUSHAPI at *new_cref(int, void *);
LUSHAPI at *assign(at *, at *);


/* SYMBOL.H ---------------------------------------------------- */

extern LUSHAPI class_t *symbol_class;

typedef struct lush_symbol {
   struct lush_symbol *next;
   struct hash_name *hn;
   at **valueptr;
   at *value;
} symbol_t;


#define SYMBOL_LOCKED_BIT      1
#define SYMBOL_VARIABLE_BIT    4

/* symbol creation */
LUSHAPI symbol_t *new_symbol(const char *);  /* new_symbol expects a managed string */
LUSHAPI at *NEW_SYMBOL(const char *);        /* NEW_SYMBOL takes any string */
LUSHAPI at *named(const char *);
LUSHAPI at *namedclean(const char *);
extern  at *at_t; 
#define t()           at_t

LUSHAPI const char *nameof(symbol_t *);
LUSHAPI const char *NAMEOF(at *);
LUSHAPI symbol_t *symbol_push(symbol_t *, at *, at **);
LUSHAPI symbol_t *symbol_pop(symbol_t *);
static inline void SYMBOL_PUSH(at *p, at *q)
{
   Symbol(p) = symbol_push(Symbol(p), q, NULL);
}
static inline void SYMBOL_POP(at *p)
{ 
   //const char *name = NAMEOF(p);
   symbol_t *sym = symbol_pop(Symbol(p));
   if (sym)
      Symbol(p) = sym;
   //else
   //   printf("FIXME: popping %s too many (self-modifying code ?)\n", name);
}

LUSHAPI at   *getslot(at*, at*);
LUSHAPI void  setslot(at**, at*, at*);
LUSHAPI at   *setq(at *p, at *q);
LUSHAPI at   *global_defs(void);
LUSHAPI void  reset_symbols(void);
LUSHAPI void  var_set(at *p, at *q);
LUSHAPI void  var_SET(at *p, at *q); /* Set variable regardless of lock mode */
LUSHAPI void  var_lock(at *p);
LUSHAPI at   *var_get(at *p);
LUSHAPI at   *var_define(char *s);
LUSHAPI bool  symbol_locked_p(symbol_t *);

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

struct call_chain {
   struct call_chain *prev;
   at *this_call;
};

extern struct call_chain *top_link;

extern LUSHAPI struct error_doc {	   
   /* contains info for printing error messages */
   //at *this_call;
   at *error_call;
   const char *error_prefix;
   const char *error_text;
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

struct lush_context {
  struct lush_context *next;
  sigjmp_buf error_jump;
  const char *input_string;
  FILE *input_file;
  short input_tab;
  short input_case_sensitive;
  FILE *output_file;
  short output_tab;
};
extern LUSHAPI struct lush_context *context;

LUSHAPI int  recur_push_ok(struct recur_elt *elt, void *call, at *p);
LUSHAPI void recur_pop(struct recur_elt *elt);
LUSHAPI void context_push(struct lush_context *newc);
LUSHAPI void context_pop(void);
LUSHAPI void toplevel(const char *in, const char *out, const char *new_prompt);
//LUSHAPI void error(char *prefix, char *text, at *suffix) no_return;
LUSHAPI void user_break(char *s);
LUSHAPI void init_lush (char *program_name);
LUSHAPI void start_lisp(int argc, char **argv, int quiet);
LUSHAPI void clean_up(void);
LUSHAPI void lush_abort (char *s) no_return;

/* STRING.H ---------------------------------------------------- */

extern LUSHAPI class_t *string_class;

LUSHAPI static inline at *new_at_string(const char *ms)
{  
   extern class_t *string_class;
   return new_at(string_class, (void *)ms);
}
#define NEW_STRING new_at_string

LUSHAPI at *make_string(const char *s);
LUSHAPI at *make_string_of_length(size_t n);
LUSHAPI int str_find(const char *s1, const char *s2, int start);
LUSHAPI at *str_val(const char *s);
LUSHAPI const char *str_number(double x);
LUSHAPI const char *str_number_hex(double x);

LUSHAPI const char *regex_compile(const char *pattern, 
                                  unsigned short *bufstart, unsigned short *bufend,
                                  int strict, int *rnum);
LUSHAPI int regex_exec(unsigned short *buffer, const char *string, 
		     const char **regptr, int *reglen, int nregs);
LUSHAPI int regex_seek(unsigned short *buffer, const char *string, const char *seekstart, 
		     const char **regptr, int *reglen, int nregs, 
		     const char **start, const char **end);

extern LUSHAPI char string_buffer[];
extern LUSHAPI at  *null_string; 

typedef struct large_string {
  char *p;
  char buffer[1024];
  at *backup;
  at **where;
} large_string_t;

LUSHAPI void large_string_init(large_string_t *ls);
LUSHAPI void large_string_add(large_string_t *ls, const char *s, int len);
LUSHAPI at * large_string_collect(large_string_t *ls);

LUSHAPI at* str_mb_to_utf8(const char *s);
LUSHAPI at* str_utf8_to_mb(const char *s);


/* FUNCTION.H -------------------------------------------------- */

/*
 * function are implemented as external objects 
 * pointing to this structure:
 */

struct cfunction {
   at *name;
   void *(*call)();
   void *info;
   char *kname;
};

#define DYCALL(cf) ((at *(*)(at*))(cf->call))
#define DXCALL(cf) ((at *(*)(int, at**))(cf->call))

typedef struct cfunction cfunction_t;

struct lfunction {
   at *formal_args;
   at *body;
};

typedef struct lfunction lfunction_t;

extern LUSHAPI class_t *function_class;         /* parent of all function classes */
extern LUSHAPI class_t *de_class;
extern LUSHAPI class_t *df_class;
extern LUSHAPI class_t *dm_class;
extern LUSHAPI class_t *dx_class;		/* dx functions are external C_function */
extern LUSHAPI class_t *dy_class;		/* dy functions have unflattened args. */

LUSHAPI at *new_de(at *formal, at *evaluable);
LUSHAPI at *new_df(at *formal, at *evaluable);
LUSHAPI at *new_dm(at *formal, at *evaluable);
LUSHAPI at *new_dx(at *name, at *(*addr)(int,at**));
LUSHAPI at *new_dy(at *name, at *(*addr)(at *));
LUSHAPI at *funcdef(at *f);
LUSHAPI at *eval_arglist(at *p);
LUSHAPI at *eval_arglist_dm(at *p);
LUSHAPI at **eval_arglist_dx(at *p);
LUSHAPI gptr need_error(int i, int j, at **arg_array_ptr);
LUSHAPI at *let(at *vardecls, at *body);
LUSHAPI at *letS(at *vardecls, at *body);
LUSHAPI at *lete(at *vardecls, at *body);

/* This is the interface header builder */

#define DX(Xname)  static at *Xname(int arg_number, at **arg_array)
#define DY(Yname)  static at *Yname(at *ARG_LIST)

/* Macros and functions used in argument transmission in DX functions */

#define ISNUMBER(i)	(NUMBERP(arg_array[i]))
#define ISGPTR(i)	(GPTRP(arg_array[i]))
#define ISLIST(i)	(LISTP(arg_array[i]))
#define ISCONS(i)	(CONSP(arg_array[i]))
#define ISSTRING(i)	(STRINGP(arg_array[i]))
#define ISSTORAGE(i)	(STORAGEP(arg_array[i]))
#define ISINDEX(i)	(INDEXP(arg_array[i]))
#define ISSYMBOL(i)     (SYMBOLP(arg_array[i]))
//#define ISOBJECT(i)     (OBJECTP(arg_array[i]))
#define ISCLASS(i)      (CLASSP(arg_array[i]))
#define DX_ERROR(i,j)   (need_error(i,j,arg_array))

#define APOINTER(i)     ( arg_array[i] )
#define ADOUBLE(i)      ( ISNUMBER(i) ? Number(APOINTER(i)) :(long)DX_ERROR(1,i))
#define AREAL           ADOUBLE
#define AGPTR(i)        ( ISGPTR(i) ? Gptr(APOINTER(i)):(gptr)DX_ERROR(9,i))
#define AINTEGER(i)     ( (intg) AREAL(i) )
#define AFLOAT(i)       ( rtoF(AREAL(i)) )
#define ALIST(i)        ( ISLIST(i) ? APOINTER(i):(at*)DX_ERROR(2,i) )
#define ACONS(i)        ( ISCONS(i) ? APOINTER(i):(at*)DX_ERROR(3,i) )
#define ASTRING(i)      ( ISSTRING(i) ? String(APOINTER(i)) : (char*)DX_ERROR(4,i) )
#define ASYMBOL(i)      ( ISSYMBOL(i) ? Symbol(APOINTER(i)) : (symbol_t *)DX_ERROR(7,i) )
#define ASTORAGE(i)     ( ISSTORAGE(i) ? (storage_t *)Mptr(APOINTER(i)) : (storage_t *)DX_ERROR(10,i) )
#define AINDEX(i)       ( ISINDEX(i) ? (index_t *)Mptr(APOINTER(i)) : (index_t *)DX_ERROR(11,i) )
#define ACLASS(i)       ( ISCLASS(i) ? (class_t *)Mptr(APOINTER(i)) : (class_t *)DX_ERROR(12,i) )

#define ARG_NUMBER(i)	if (arg_number != i)  DX_ERROR(0,i);


/* FILEIO.H ------------------------------------------------- */

extern LUSHAPI class_t *rfile_class, *wfile_class;
extern LUSHAPI char file_name[], lushdir[];

#define OPEN_READ(f,s)  new_rfile(open_read(f,s))
#define OPEN_WRITE(f,s) new_wfile(open_write(f,s))

LUSHAPI at *new_rfile(const FILE *f);
LUSHAPI at *new_wfile(const FILE *f);

LUSHAPI const char *cwd(const char *s);
LUSHAPI at *files(const char *s);
LUSHAPI bool dirp(const char *s);
LUSHAPI bool filep(const char *s);
LUSHAPI const char *lush_dirname(const char *fname);
LUSHAPI const char *lush_basename(const char *fname, const char *suffix);
LUSHAPI const char *concat_fname(const char *from, const char *fname);
LUSHAPI const char *relative_fname(const char *from, const char *fname);
LUSHAPI void unlink_tmp_files(void);
LUSHAPI const char *tmpname(const char *s, const char *suffix);
LUSHAPI const char *search_file(const char *s, const char *suffixes);
LUSHAPI void test_file_error(FILE *f, int errno);
LUSHAPI FILE *open_read(const char *s, const char *suffixes);
LUSHAPI FILE *open_write(const char *s, const char *suffixes);
LUSHAPI FILE *open_append(const char *s, const char *suffixes);
LUSHAPI FILE *attempt_open_read(const char *s, const char *suffixes);
LUSHAPI FILE *attempt_open_write(const char *s, const char *suffixes);
LUSHAPI FILE *attempt_open_append(const char *s, const char *suffixes);
LUSHAPI void file_close(FILE *f);
LUSHAPI void set_script(const char *s);
LUSHAPI int read4(FILE *f);
LUSHAPI int write4(FILE *f, unsigned int l);
LUSHAPI off_t file_size(FILE *f);

#define RFILEP(x) ((x)&&(Class(x) == rfile_class))
#define WFILEP(x) ((x)&&(Class(x) == wfile_class))


/* IO.H ----------------------------------------------------- */


extern LUSHAPI char *line_pos;
extern LUSHAPI char *line_buffer;
extern LUSHAPI char *prompt_string;

LUSHAPI void print_char (char c);
LUSHAPI void print_string(const char *s);
LUSHAPI void print_list(at *list);
LUSHAPI void print_tab(int n);
LUSHAPI const char *pname(at *l);
LUSHAPI const char *first_line(at *l);
LUSHAPI char read_char(void);
LUSHAPI char next_char(void);
LUSHAPI int  ask(const char *t);
LUSHAPI const char *dmc(const char *s, at *l);
LUSHAPI char skip_char(const char *s);
LUSHAPI char skip_to_expr(void);
LUSHAPI at *read_list(void);



/* HTABLE.H ------------------------------------------------- */

extern LUSHAPI class_t *htable_class;
#define HTABLEP(x)   ((x)&&(Class(x) == htable_class))

struct htable;
typedef struct htable htable_t;

LUSHAPI unsigned long hash_value(at *);
LUSHAPI unsigned long hash_pointer(at *);
LUSHAPI htable_t *new_htable(int, bool, bool);
LUSHAPI at  *NEW_HTABLE(int, bool, bool);
LUSHAPI void htable_clear(htable_t *ht);
LUSHAPI void htable_delete(htable_t *ht, at *key);
LUSHAPI at  *htable_get(htable_t *ht, at *key);
LUSHAPI void htable_set(htable_t *ht, at *key, at *value);
LUSHAPI void htable_update(htable_t *ht1, htable_t *ht2);


/* CALLS.H ----------------------------------------------------- */

LUSHAPI int comp_test(at *p, at *q);
LUSHAPI int eq_test (at *p, at *q);


/* ARITH.H ----------------------------------------------------- */

extern LUSHAPI class_t complex_class;
#ifdef HAVE_COMPLEXREAL
LUSHAPI at *new_complex(complexreal z);
LUSHAPI int complexp(at*);
LUSHAPI complexreal get_complex(at*);
#endif

/* OOSTRUCT.H ----------------------------------------------------- */

struct CClass_object;

typedef struct object {
   at *backptr;
   struct CClass_object *cptr;
   struct object *next_unreachable;
   at *slots[];
} object_t;

LUSHAPI class_t  *new_builtin_class(class_t *super);
LUSHAPI class_t  *new_ooclass(at *classname, at *superclass, at *keylist, at *defaults);
LUSHAPI object_t *new_object(class_t *cl);
LUSHAPI object_t *object_from_cobject(struct CClass_object *);
#define NEW_OBJECT(cl) new_object(cl)->backptr
LUSHAPI bool builtin_class_p(const class_t *cl);
LUSHAPI void putmethod(class_t *cl, at *name, at *fun);
LUSHAPI at  *getmethod(class_t *cl, at *prop);
LUSHAPI at  *with_object(at *obj, at *f, at *q, int howmuch);
LUSHAPI at  *send_message(at *classname, at *obj, at *method, at *args);
LUSHAPI bool isa(at *p, const class_t *cl);
LUSHAPI void lush_delete(at *p);       /* avoid conflict with C++ keyword */
LUSHAPI void lush_delete_maybe(at *p);
LUSHAPI void zombify_subclasses(class_t *);
LUSHAPI static inline class_t *classof(const at *p)
{
   if (p)
      return CONSP(p) ? cons_class : p->head.cl;
   else
      return nil_class;
}
LUSHAPI bool oostruct_idle(void);

/* MODULE.H --------------------------------------------------- */

extern LUSHAPI class_t *module_class;
#define MODULEP(x)  ((x)&&Class(x) == module_class)

LUSHAPI void class_define(const char *name, class_t *cl);
LUSHAPI void dx_define(const char *name, at *(*addr) (int, at **));
LUSHAPI void dy_define(const char *name, at *(*addr) (at *));
LUSHAPI void dxmethod_define(class_t *cl, const char *name, at *(*addr) (int, at **));
LUSHAPI void dymethod_define(class_t *cl, const char *name, at *(*addr) (at *));
LUSHAPI void dhclass_define(const char *name, dhclassdoc_t *kclass);
LUSHAPI void dh_define(const char *name, dhdoc_t *kname);
LUSHAPI void dhmethod_define(dhclassdoc_t *kclass, const char *name, dhdoc_t *kname);
LUSHAPI void check_primitive(at *prim, void *info);
LUSHAPI at *find_primitive(at *module, at *name);
LUSHAPI at *module_list(void);
LUSHAPI at *module_load(const char *filename, at *hook);
LUSHAPI void module_unload(at *atmodule);


/* DATE.H ----------------------------------------------------- */

#define DATE_YEAR       0
#define DATE_MONTH      1
#define DATE_DAY        2
#define DATE_HOUR       3
#define DATE_MINUTE     4
#define DATE_SECOND     5

extern class_t *date_class;

LUSHAPI char *str_date( at *p, int *pfrom, int *pto );
LUSHAPI at *new_date( char *s, int from, int to );
LUSHAPI at *new_date_from_time( void *clock, int from, int to );

/* BINARY.H ----------------------------------------------------- */

enum serialize_action {
  SRZ_SETFL, 
  SRZ_CLRFL,
  SRZ_WRITE,
  SRZ_READ
};

LUSHAPI int bwrite(at *p, FILE *f, int opt);
LUSHAPI at *bread(FILE *f, int opt);

/* serialization functions */
LUSHAPI void serialize_bool(bool *data, int code);
LUSHAPI void serialize_char(char *data, int code);
LUSHAPI void serialize_uchar(uchar *data, int code);
LUSHAPI void serialize_short(short int *data, int code);
LUSHAPI void serialize_int(int *data, int code);
LUSHAPI void serialize_size(size_t *data, int code);
LUSHAPI void serialize_offset(ptrdiff_t *data, int code);
LUSHAPI void serialize_string(char **data, int code, int maxlen);
LUSHAPI void serialize_chars(void **data, int code, int len);
LUSHAPI void serialize_float(float *data, int code);
LUSHAPI void serialize_double(double *data, int code);
LUSHAPI int  serialize_atstar(at **data, int code);
LUSHAPI FILE *serialization_file_descriptor(int code);

/* FPU.H -------------------------------------------------------- */

LUSHAPI double eps(double x);
LUSHAPI float epsf(float x);
LUSHAPI void fpu_reset(void);


/* DH.H -------------------------------------------------- */

typedef struct index index_t;
typedef struct storage storage_t;

#include "dh.h"


/* STORAGE.H --------------------------------------------------- */

/* 
 * The field 'type' of a storage defines the type of the elements.
 * Integer type ST_ID should be used for indexing.
 */

typedef enum dht_type storage_type_t;

#define ST_BOOL       DHT_BOOL
#define ST_CHAR       DHT_CHAR
#define ST_UCHAR      DHT_UCHAR
#define ST_SHORT      DHT_SHORT
#define ST_INT        DHT_INT
#define ST_FLOAT      DHT_FLOAT
#define ST_DOUBLE     DHT_DOUBLE
#define ST_GPTR       DHT_GPTR
#define ST_MPTR       DHT_MPTR
#define ST_AT         DHT_AT

#define ST_FIRST      ST_BOOL
#define ST_LAST       ((storage_type_t)(ST_AT + 1))

/*
 * The other flags define the nature of the storage (STS).
 */

enum storage_kind {
   STS_NULL = 0,           /* storage object without memory */
   STS_MANAGED,            /* memory managed by lush runtime */
   STS_FOREIGN,            /* pointer given to us, lush won't free */
   STS_MMAP,               /* memory mapped via mmap */
   STS_STATIC,             /* memory in data segment */
};

struct storage {
   at    *backptr;
   gptr   data;
   size_t size;
   bool  isreadonly;
   enum storage_kind kind:16;
   storage_type_t    type:16;
#ifdef HAVE_MMAP
   gptr   mmap_addr;
   size_t mmap_len;
# ifdef WIN32
   gptr   mmap_xtra;
# endif
#endif
};

/* typedef struct storage  storage_t; // defined above */ 

extern LUSHAPI class_t *abstract_storage_class;
extern LUSHAPI class_t *storage_class[ST_LAST];
extern LUSHAPI size_t   storage_sizeof[ST_LAST];
extern LUSHAPI flt    (*storage_getf[ST_LAST])(gptr, size_t);
extern LUSHAPI void   (*storage_setf[ST_LAST])(gptr, size_t, flt);
extern LUSHAPI real   (*storage_getd[ST_LAST])(gptr, size_t);
extern LUSHAPI void   (*storage_setd[ST_LAST])(gptr, size_t, real);
extern LUSHAPI at *   (*storage_getat[ST_LAST])(storage_t *, size_t);
extern LUSHAPI void   (*storage_setat[ST_LAST])(storage_t *, size_t, at *);

/* storage creation */
LUSHAPI storage_t *new_storage(storage_type_t);
LUSHAPI storage_t *new_storage_managed(storage_type_t, size_t, at*);
LUSHAPI storage_t *new_storage_foreign(storage_type_t, size_t, void *, bool);
LUSHAPI storage_t *new_storage_mmap(storage_type_t, FILE*, size_t, bool);
LUSHAPI storage_t *new_storage_static(storage_type_t, size_t, const void *);

/* storage properties */
LUSHAPI bool   storage_classp(const at*);
LUSHAPI bool   storage_readonlyp(const storage_t *);
LUSHAPI size_t storage_nelems(const storage_t *);
LUSHAPI size_t storage_nbytes(const storage_t *);

/* storage manipulation */

LUSHAPI void get_write_permit(storage_t *);
LUSHAPI void storage_alloc(storage_t*, size_t, at*);
LUSHAPI void storage_realloc(storage_t*, size_t, at*);
LUSHAPI void storage_clear(storage_t*, at*, size_t);
LUSHAPI void storage_load(storage_t*, FILE*);
LUSHAPI void storage_save(storage_t*, FILE*);

/* INDEX.H ---------------------------------------------- */

#define MAXDIMS 11

extern LUSHAPI class_t *index_class;

struct index {			
   at *backptr;
   int ndim;			/* number of dimensions */
   size_t dim[MAXDIMS];		/* array size for each dimension */
   ptrdiff_t mod[MAXDIMS];      /* stride for each dimension */
   ptrdiff_t offset;		/* in element size */
   storage_t *st;		/* a pointer to the storage */
};

/* shape_t and subscript_t are used as argument types in the index 
   C API. shape_t coincides with the head of the index structure. */

typedef struct shape {
  int ndims;
  size_t  dim[MAXDIMS];
} shape_t;

typedef struct subscript {
  int ndims;
  ptrdiff_t dim[MAXDIMS];
} subscript_t;

#define IND_ST(ind)        (((index_t *)ind)->st)
#define IND_ATST(ind)      (((index_t *)ind)->st->backptr)
#define IND_STTYPE(ind)    (IND_ST(ind)->type)
#define IND_STNELEMS(ind)  (IND_ST(ind)->size)
#define IND_SHAPE(ind)     ((shape_t *)&(ind->ndim))
#define IND_NDIMS(ind)     ((ind)->ndim)
#define IND_DIM(ind, n)   ((ind)->dim[n])
#define IND_MOD(ind, n)   ((ind)->mod[n])
#define IND_BASE(ind)      (gptr) ((char *) IND_ST(ind)->data + \
                           (ind)->offset * storage_sizeof[IND_STTYPE(ind)])
#define IND_BASE_TYPED(ind, T) ((T *)(IND_ST(ind)->data) + (ind)->offset)
LUSHAPI size_t index_nelems(const index_t*);

/* Function related to <struct index> objects */

/* index and array creation */
LUSHAPI index_t *new_index(storage_t*, shape_t*);
LUSHAPI index_t *make_array(storage_type_t, shape_t*, at*);
LUSHAPI index_t *clone_array(index_t*);
LUSHAPI index_t *copy_index(index_t*);
LUSHAPI index_t *copy_array(index_t*);

#define NEW_INDEX(st, shp) (new_index(st, shp)->backptr)
#define MAKE_ARRAY(t, shp, i) (make_array(t, shp, i)->backptr)
#define CLONE_ARRAY(ind)  MAKE_ARRAY(IND_STTYPE(ind), IND_SHAPE(ind), NIL)
#define COPY_INDEX(ind) (copy_index(ind)->backptr)
#define COPY_ARRAY(ind) (copy_array(ind)->backptr)

/* argument processing */
#define DOUBLE_ARRAY_P(p) (INDEXP(p) && IND_STTYPE(Mptr(p))==ST_DOUBLE)
#define INT_ARRAY_P(p)  (INDEXP(p) && IND_STTYPE(Mptr(p))==ST_INT)
LUSHAPI index_t *as_contiguous_array(index_t *ind);
LUSHAPI index_t *as_double_array(at *arg);
LUSHAPI index_t *as_int_array(at *arg);

/* index predicates */
LUSHAPI bool index_numericp(const index_t *);
LUSHAPI bool index_readonlyp(const index_t *);
LUSHAPI bool index_emptyp(const index_t *);
LUSHAPI bool index_contiguousp(const index_t*);
LUSHAPI bool index_malleablep(const index_t*);
LUSHAPI bool index_broadcastable_p(index_t*, index_t*);
LUSHAPI bool same_shape_p(index_t *, index_t *);

/* dealing with shapes */
LUSHAPI bool   shape_equalp(shape_t *, shape_t *);
LUSHAPI size_t shape_nelems(shape_t*);
LUSHAPI shape_t *shape_copy(shape_t*, shape_t*);
LUSHAPI shape_t* shape_set(shape_t*, int, size_t, size_t, size_t, size_t, size_t, size_t);
#define SHAPE0D                 shape_set(NIL, 0, 0, 0, 0, 0, 0, 0)
#define SHAPE1D(d1)             shape_set(NIL, 1, d1, 0, 0, 0, 0, 0)
#define SHAPE2D(d1, d2)         shape_set(NIL, 2, d1, d2, 0, 0, 0, 0)
#define SHAPE3D(d1, d2, d3)     shape_set(NIL, 3, d1, d2, d3, 0, 0, 0)
#define SHAPE4D(d1, d2, d3, d4) shape_set(NIL, 4, d1, d2, d3, d4, 0, 0)
#define SHAPE5D(d1, d2, d3, d4, d5) \
   shape_set(NIL, 5, d1, d2, d3, d4, d5, 0)
#define SHAPE6D(d1, d2, d3, d4, d5, d6) \
   shape_set(NIL, 6, d1, d2, d3, d4, d5, d6)

/* index manipulation */
LUSHAPI index_t *index_reshape(index_t*, shape_t*);
LUSHAPI index_t *index_ravel(index_t *);
LUSHAPI index_t *index_trim(index_t*, int d, ptrdiff_t nz, size_t ne);
LUSHAPI index_t *index_strim(index_t *, int, size_t);
LUSHAPI index_t *index_extend(index_t*, int d, ptrdiff_t ne);
LUSHAPI index_t *index_extendS(index_t*, subscript_t*);
LUSHAPI index_t *index_expand(index_t*, int d, size_t ne);
LUSHAPI index_t *index_liftD(index_t*, shape_t*);
LUSHAPI index_t *index_lift(index_t*, shape_t*);
LUSHAPI index_t *index_nickD(index_t*, int);
LUSHAPI index_t *index_nick(index_t*, int);
LUSHAPI index_t *index_shift(index_t*, int d, ptrdiff_t ne);
LUSHAPI index_t *index_shiftS(index_t*, subscript_t*);
LUSHAPI index_t *index_select(index_t*, int d, ptrdiff_t n);
LUSHAPI index_t *index_selectS(index_t*, subscript_t*);
LUSHAPI index_t *index_sinkD(index_t *, shape_t *);
LUSHAPI index_t *index_sink(index_t *, shape_t *);
LUSHAPI index_t *index_transpose(index_t*, shape_t*);
LUSHAPI index_t *index_reverse(index_t*, int d);
LUSHAPI index_t *index_broadcast1(index_t*blank, index_t*ref);
LUSHAPI shape_t *index_broadcast2(index_t*, index_t*, index_t**, index_t**);

/* in-place index manipulation */
LUSHAPI index_t *index_reshapeD(index_t*, shape_t *);
LUSHAPI index_t *index_trimD(index_t*, int d, ptrdiff_t nz, size_t ne);
LUSHAPI index_t *index_strimD(index_t *, int, size_t);
LUSHAPI index_t *index_extendD(index_t*, int d, ptrdiff_t ne);
LUSHAPI index_t *index_extendSD(index_t*, subscript_t*);
LUSHAPI index_t *index_expandD(index_t*, int d, size_t ne);
LUSHAPI index_t *index_shiftD(index_t*, int d, ptrdiff_t ne);
LUSHAPI index_t *index_shiftSD(index_t*, subscript_t*);
LUSHAPI index_t *index_reverseD(index_t*, int d);

LUSHAPI void easy_index_check(index_t*, shape_t*);
//LUSHAPI real easy_index_get(index_t*, size_t*);
//LUSHAPI void easy_index_set(index_t*, size_t*, real);

/* Other functions */
LUSHAPI index_t *index_copy(index_t *, index_t *);
LUSHAPI index_t *array_copy(index_t *, index_t *);
LUSHAPI void     array_swap(index_t*, index_t*);
LUSHAPI index_t *array_extend(index_t*, int d, ptrdiff_t ne, at* init);
LUSHAPI index_t *array_select(index_t*, int d, ptrdiff_t n);
LUSHAPI index_t *array_take2(index_t*, index_t *ss);
LUSHAPI index_t *array_take3(index_t*, int d, index_t *ss);
LUSHAPI index_t *array_put(index_t*, index_t *ss, index_t *vals);
LUSHAPI index_t *array_range(double, double, double);
LUSHAPI index_t *array_rangeS(double, double, double);

LUSHAPI void import_array(index_t*, FILE*, size_t);
LUSHAPI void import_array_text(index_t*, FILE*);
LUSHAPI int  save_array_len (index_t*);
LUSHAPI void save_array(index_t*, FILE*);
LUSHAPI void export_array(index_t*, FILE*);
LUSHAPI void save_array_text(index_t*, FILE*);
LUSHAPI void export_array_text(index_t*, FILE*);
LUSHAPI at  *load_array(FILE*);


/* 
 * Loops over all elements of idx <idx>
 * The variable <ptr> is an offset
 * referencing each element of <idx>.
 * It is incremented by the loop, over all the idx.
 */

#define begin_idx_aloop1(idx,ptr) { 					     \
  size_t _d_[MAXDIMS]; 						     \
  ptrdiff_t ptr = 0;							     \
  int _j_;                                                                   \
  bool emptyp = false;                                                       \
  for (_j_=0;_j_<(idx)->ndim; _j_++ ) 					     \
    { _d_[_j_]=0; emptyp = emptyp || (idx)->dim[_j_] == 0; }                 \
  _j_ = emptyp ? -1 : (idx)->ndim;					     \
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
 * The variables <ptr1> and <ptr2> are offsets
 * referencing the element of <idx1> and <idx2>.
 * Idxs <idx1> and <idx2> don't need to have the same structure,
 * but they must have the same number of elements.
 */

#define begin_idx_aloop2(idx1, idx2, ptr1, ptr2) { 			     \
  size_t _d1_[MAXDIMS]; 						     \
  size_t _d2_[MAXDIMS];						             \
  ptrdiff_t ptr1 = 0;							     \
  ptrdiff_t ptr2 = 0;							     \
  int _j1_, _j2_;                                                            \
  bool emptyp1 = false;                                                      \
  bool emptyp2 = false;                                                      \
  for (_j1_=0;_j1_<(idx1)->ndim; _j1_++ ) 				     \
    { _d1_[_j1_]=0; emptyp1 = emptyp1 || (idx1)->dim[_j1_] == 0; }           \
  for (_j2_=0;_j2_<(idx2)->ndim; _j2_++ ) 				     \
    { _d2_[_j2_]=0; emptyp2 = emptyp2 || (idx2)->dim[_j2_] == 0; }           \
  _j1_ = emptyp1 ? -1 : (idx1)->ndim;					     \
  _j2_ = emptyp2 ? -1 : (idx2)->ndim;					     \
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
 * The variables <ptr0><ptr1> and <ptr2> are offsets
 * referencing the elements of <idx0><idx1> and <idx2>.
 * Idxs <idx0><idx1> and <idx2> don't need to have the same structure,
 * but they must have the same number of elements.
 */

#define begin_idx_aloop3(idx0, idx1, idx2, ptr0, ptr1, ptr2) { 	             \
  size_t _d0_[MAXDIMS]; 						     \
  size_t _d1_[MAXDIMS]; 						     \
  size_t _d2_[MAXDIMS]; 						     \
  ptrdiff_t ptr0 = 0;						     	     \
  ptrdiff_t ptr1 = 0;						     	     \
  ptrdiff_t ptr2 = 0;							     \
  int _j0_, _j1_, _j2_;                                                      \
  bool emptyp0 = false;                                                      \
  bool emptyp1 = false;                                                      \
  bool emptyp2 = false;                                                      \
  for (_j0_=0;_j0_<(idx0)->ndim; _j0_++ ) 				     \
    { _d0_[_j0_]=0; emptyp0 = emptyp0 || (idx0)->dim[_j0_] == 0; }           \
  for (_j1_=0;_j1_<(idx1)->ndim; _j1_++ ) 				     \
    { _d1_[_j1_]=0; emptyp1 = emptyp1 || (idx1)->dim[_j1_] == 0; }           \
  for (_j2_=0;_j2_<(idx2)->ndim; _j2_++ ) 				     \
    { _d2_[_j2_]=0; emptyp2 = emptyp2 || (idx2)->dim[_j2_] == 0; }           \
  _j1_ = emptyp0 ? -1 : (idx0)->ndim;					     \
  _j1_ = emptyp1 ? -1 : (idx1)->ndim;					     \
  _j2_ = emptyp2 ? -1 : (idx2)->ndim;					     \
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



/* EVENT.H ----------------------------------------------------- */


/* Event sources */
LUSHAPI void  block_async_poll(void);
LUSHAPI void  unblock_async_poll(void);
LUSHAPI void  unregister_poll_functions(void *handle);
LUSHAPI void *register_poll_functions(int  (*spoll)(void), 
                                      void (*apoll)(void),
                                      void (*bwait)(void), 
                                      void (*ewait)(void), int fd );

/* Event queues */ 
LUSHAPI void *timer_add(at *handler, int delay, int period);
LUSHAPI void *timer_abs(at *handler, real date);
LUSHAPI void  timer_del(void *handle);
LUSHAPI int   timer_fire(void);
LUSHAPI void  event_add(at *handler, at *event);
LUSHAPI at   *event_peek(void);
LUSHAPI at   *event_get(void *handler, bool remove);
LUSHAPI at   *event_wait(bool console);
LUSHAPI void  process_pending_events(void);

/* Compatible event queue functions */
LUSHAPI void  enqueue_event(at*, int event, int, int, int, int);
LUSHAPI void  enqueue_eventdesc(at*, int event, int, int, int, int, char*);
#define EVENT_NONE        (-1L)
#define EVENT_ASCII_MIN   (0L)
#define EVENT_ASCII_MAX   (255L)
#define EVENT_MOUSE_DOWN  (1001L)
#define EVENT_MOUSE_UP    (1002L)
#define EVENT_MOUSE_DRAG  (1003L)
#define EVENT_ARROW_UP    (1011L)
#define EVENT_ARROW_RIGHT (1012L)
#define EVENT_ARROW_DOWN  (1013L)
#define EVENT_ARROW_LEFT  (1014L)
#define EVENT_FKEY        (1015L)
#define EVENT_RESIZE      (2001L)
#define EVENT_HELP        (2002L)
#define EVENT_DELETE      (2003L)
#define EVENT_SENDEVENT   (2004L)
#define EVENT_ALARM       (3001L)
#define EVENT_EXPOSE      (4001L)
#define EVENT_GLEXPOSE    (4002L)



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


/* -------------------------------------------------------------
   Local Variables:
   c-font-lock-extra-types: ("FILE" "\\sw+_t" "at" "gptr" "real" "flt" "intg")
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
