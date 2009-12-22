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

#include "header.h"
#include <fenv.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

struct error_doc error_doc;
struct recur_doc recur_doc;
struct lush_context *context;

static struct lush_context first_context;
at* at_toplevel;
static at *at_startup;
static at *at_break;
static at *at_debug;
static at *at_file;
static at *at_memstats;
static at *result;
static int quiet;

/* initialization functions */
#ifdef UNIX
extern void init_unix (void);
extern void fini_unix (void);
extern void init_user (void);
#endif
#ifdef WIN32
extern void init_win32(void);
#endif
extern void init_oostruct (void);
extern void init_symbol (void);
extern void init_cref(void);
extern void init_eval (void);
extern void init_function (void);
extern void init_at (void);
extern void init_list(void);
extern void init_calls (void);
extern void init_number(void);
extern void init_arith (void);
extern void init_math (void);
extern void init_string (void);
extern void init_lushregex (void);
extern void init_fileio (char *);
extern void init_io (void);
extern void init_htable(void);
extern void init_toplevel (void);
extern void init_nan (void);
extern void init_binary (void);
extern void init_dump (void);
extern void init_module (char *);
extern void init_date (void);
extern void init_storage (void);
extern void init_index (void);
extern void init_idx1 (void);
extern void init_idx2 (void);
extern void init_idx3 (void);
extern void init_idx4 (void);
extern void init_dh (void);
extern void init_event (void);
extern void init_graphics (void);
extern void init_ps_driver (void);
extern void init_lushrng(void);
extern void init_lisp_driver (void);
extern void init_weakref (void);
#ifndef NOGRAPHICS
#ifdef UNIX
#ifndef X_DISPLAY_MISSING
extern void init_x11_driver (void);
#endif
#endif
#ifdef WIN32
extern void init_win_driver(void);
#endif
#endif
#ifdef WIN32
int isatty (int);
#endif

/* From DUMP.C */
extern int isdump (const char *s);
extern void undump (const char *s);

/* From BINARY.C */
extern int in_bwrite;

/* FORWARD */
static void recur_doc_init(void);


/********************************************/

/*
 * Exiting Lush quickly
 */

void lush_abort(char *s)
{
   unlink_tmp_files();
   FINI_MACHINE;
   FMODE_TEXT(stderr);
   fprintf(stderr,"\nLush Fatal Error: %s\n", s);
   exit(10);
   while(1);
}


/*
 * Exiting Lush cleanly
 */

void clean_up(void)
{
   
   set_script(NIL);
   error_doc.ready_to_an_error = false;
   unlink_tmp_files();
   mm_collect_now();
   FINI_MACHINE;
   FMODE_TEXT(stderr);
   if (! quiet)
      fprintf(stderr, "Bye!\n");
}



/********************************************/

void init_lush(char *program_name)
{
   MM_ENTER;
   MM_NOGC;
   at *p,*version;
   init_weakref();
   init_at();
   init_symbol();
   init_cref();
   init_oostruct();
   init_eval();
   init_function();
   init_string();
   init_lushregex();
   init_fileio(program_name);
   init_list();
   init_calls();
   init_number();
   init_arith();
   init_math();
   init_io();
   init_htable();
   init_toplevel();
   init_nan();
   init_binary();
   init_module(program_name);
   init_date();
   init_storage();
   init_index();
   init_idx1();
   init_idx2();
   init_idx3();
   init_idx4();
   init_dh();
   init_lushrng();
   init_dump();
   init_event();
#ifdef UNIX
   init_unix();
   init_user();
#endif
#ifndef NOGRAPHICS
   init_graphics();
   init_ps_driver();
   init_lisp_driver();
#ifdef UNIX
#ifndef X_DISPLAY_MISSING
   init_x11_driver();
#endif
#endif
#ifdef WIN32
   init_win32();
   init_win_driver();
#endif
#endif
   /* Very simple way to define version :-) */
   version = var_define("version");
   p = make_string("lush2");
   var_set(version,p);
   var_lock(version);
   MM_NOGC_END;
   MM_EXIT;
}



/*
 * start_lisp 
 * Calls the lisp interpretor 
 * - load startup files
 * - launch an interactive toplevel with the user
 */

extern void reset_dx_stack(void); /* defined in function.c */

void start_lisp(int argc, char **argv, int quiet)
{
   at *p, *q;
   at **where;
   const char *s, *r;
   error_doc.script_file = NIL;
   error_doc.script_mode = SCRIPT_OFF;
   error_doc.error_prefix = NIL;
   error_doc.error_text = NIL;
   error_doc.error_suffix = NIL;
   error_doc.ready_to_an_error = false;
   error_doc.debug_toplevel = false;
   error_doc.error_call = NIL;
   top_link = NULL;
   recur_doc_init();
   reset_dx_stack();

   context = &first_context;
   context->next = NIL;
   context->input_string = 0;
   context->input_file = stdin;
   context->input_tab = 0;
   context->input_case_sensitive = 1;
   context->output_file = stdout;
   context->output_tab = 0;
   
   MM_ENTER;
   if (! sigsetjmp(context->error_jump, 1)) {
      s = "stdenv";
      /* Check @-argument */
      if (argc>1 && argv[1][0]=='@') {
         argc--; argv++;
         if (!argv[0][1] && argc>1) {
            /* Case 'lush @ xxxenv' */
            argc--; argv++; s = argv[0];
         } else if (strcmp(&argv[0][1],"@"))
            /* Case 'lush @xxxenv' but not 'lush @@' */
            s = &argv[0][1];
         /* Current dir has priority */
         r = concat_fname(NULL,s);
         if (search_file(r,"|.dump|.lshc|.lsh"))
            s = r;
      }

      /* Search a dump file */
      if ((r = search_file(s, "|.dump")) && isdump(r)) {
         error_doc.ready_to_an_error = false;
         if (! quiet) {
            FMODE_TEXT(stdout);
            fprintf(stdout,"+[%s]\n",r);
            FMODE_BINARY(stderr);
         }
         undump(r);

      } else if ((r = search_file(s,"|.lshc|.lsh"))) {
         error_doc.ready_to_an_error = true;
         if (! quiet)  {
            FMODE_TEXT(stdout);
            fprintf(stdout,"+[%s]\n",r);
            FMODE_BINARY(stderr);
         }
         toplevel(r, NIL, NIL);
         
      } else
         lush_abort("Cannot locate system libraries");

      /* Calls the cold startup procedure with arguments */
      error_doc.ready_to_an_error = true;
      error_doc.debug_toplevel = false;
      error_doc.error_call = NIL;
      error_doc.error_prefix = NIL;
      error_doc.error_text = NIL;
      error_doc.error_suffix = NIL;
      top_link = NULL;
      recur_doc_init();
      reset_dx_stack();

      line_pos = line_buffer;
      *line_buffer = 0;
      in_bwrite = 0;
      p = NIL;
      where = &p;
      for (int i = 1; i<argc; i++) {
         *where = new_cons(make_string(argv[i]),NIL);
         where = &Cdr(*where);
      }
      q = apply(Value(at_startup),p);
   }
   MM_EXIT;     /* reset transient stack after an error */

   /* No interactive loop in quiet mode */
   if (! quiet) {
      MM_ENTER;
      error_doc.ready_to_an_error = false;
      reset_symbols();
      error_doc.ready_to_an_error = true;
      error_doc.debug_toplevel = false;
      error_doc.error_call = NIL;
      error_doc.error_prefix = NIL;
      error_doc.error_text = NIL;
      error_doc.error_suffix = NIL;
      top_link = NULL;
      recur_doc_init();
      reset_dx_stack();
      fpu_reset();

      line_pos = line_buffer;
      *line_buffer = 0;
      in_bwrite = 0;
      for(;;) {
         /* Calls the interactive toplevel */
         q = apply(Value(at_toplevel),NIL);
         if (!isatty(fileno(stdin))) {
            break;
         }
         if (ask("Really quit")) {
            break;
         }
      }
      MM_EXIT;
   }
   /* Finished */
   clean_up();
}


/***********************************************/

/* 
 * Utilities for detection of infinite recursion.
 */

#define HASHP(p) \
  (((unsigned long)(p))^(((unsigned long)(p))>>6))

static void recur_resize_table(int nbuckets)
{
   struct recur_elt **ntable;
   if (! (ntable = malloc(nbuckets * sizeof(void*))))
      error(NIL,"Out of memory",NIL);
   memset(ntable, 0, nbuckets * sizeof(void*));
   if (recur_doc.htable) {
      int i,n;
      struct recur_elt *elt, *nelt;
      for (i=0; i<recur_doc.hbuckets; i++)
         for (elt=recur_doc.htable[i]; elt; elt=nelt) {
            nelt = elt->next;
            n = HASHP(elt->p) % nbuckets;
            elt->next = ntable[n];
            ntable[n]=elt;
         }
      if (recur_doc.htable)
         free(recur_doc.htable);
   }
   recur_doc.htable = ntable;
   recur_doc.hbuckets = nbuckets;
}


static void recur_doc_init()
{
   if (recur_doc.hbuckets == 0)
      recur_resize_table(127);
   memset(recur_doc.htable, 0, 
          recur_doc.hbuckets * sizeof(void*));
   recur_doc.hsize = 0;
}


/* 
 * Detection of infinite recursion.
 */

int recur_push_ok(struct recur_elt *elt, void *call, at *p)
{
   struct recur_elt *tst;
   /* Test with hash table */
   int n = HASHP(p) % recur_doc.hbuckets;
   for  (tst=recur_doc.htable[n]; tst; tst=tst->next)
      if (tst->p==p && tst->call==call)
         return false;
   /* Prepare record */
   elt->call = call;
   elt->p = p;
   elt->next = recur_doc.htable[n];
   recur_doc.htable[n] = elt;
   recur_doc.hsize += 1;
   /* Test if htable needs resizing */
   if (recur_doc.hsize*2 > recur_doc.hbuckets*3) 
      recur_resize_table(recur_doc.hsize*2-1);
   /* Successfully added to hash table */
   return true;
}


void recur_pop(struct recur_elt *elt)
{
   struct recur_elt **tst;
   int n = HASHP(elt->p) % recur_doc.hbuckets;
   /* Search entry in hash table */
   tst=&recur_doc.htable[n];
   while (tst && *tst!=elt)
      tst = &((*tst)->next);
   if (!tst)
      error(NIL,"Internal error in recursion detector", NIL);
   /* Remove entry when found */
   *tst = (*tst)->next;
   recur_doc.hsize -= 1;
}


/***********************************************/


/*
 * context stack handling context_push(s) context_pop()
 */

void context_push(struct lush_context *newc)
{
   *newc = *context;
   newc->next = context;
   context = newc;
}

void context_pop(void)
{
   struct lush_context *oldcontext = context;
   if (context->next)
      context = context->next;
   if (oldcontext->input_string && context->input_string)
      context->input_string = oldcontext->input_string;
   if (oldcontext->input_tab >= 0)
      context->input_tab = oldcontext->input_tab;
   if (oldcontext->output_tab >= 0)
      context->output_tab = oldcontext->output_tab;
}



/***********************************************/


/*
 * toplevel( input_file, output_file, verbose_mode )
 */

static int discard_flag;
static int exit_flag = 0;

void toplevel(const char *in, const char *out, const char *prompts)
{
   MM_ENTER;
   
   FILE *f1, *f2;
   at *ans = NIL;
   at *q1, *q2;
   char *ps1 = 0;
   char *ps2 = 0;
   char *ps3 = 0;
   char *saved_prompt = prompt_string;
   struct lush_context mycontext;

   /* save debug_tab */
   int debug_tab = error_doc.debug_tab;
   
   /* Open files */
   f1 = f2 = NIL;
   if (in) {
      f1 = open_read(in, "|.lshc|.snc|.tlc|.lsh|.sn|.tl");
      ans = make_string(file_name); /* fishy */
   }
   if (out)
      f2 = open_write(out, "script");

   /* Make files current */
   context_push(&mycontext);
   SYMBOL_PUSH(at_file, ans);
   if (f1) {
      context->input_file = f1;
      context->input_tab = 0;
      context->input_case_sensitive = 1;
   }
   if (f2) {
      context->output_file = f2;
      context->output_tab = 0;
   }

   /* Split prompt */
   if (prompts) {
      const char d = '|';
      char *s;
      ps1 = strdup(prompts);
      s = ps1;
      while (*s && *s != d)
         s += 1;
      if (! *s) {
         ps2 = "  ";
         ps3 = ps1;
      } else {
         *s++ = 0;
         ps2 = s;
         while (*s && *s != d)
            s += 1;
         if (! *s) {
            ps3 = ps1;
         } else {
            *s++ = 0;
            ps3 = s;
            while (*s && *s != d)
               s += 1;
            if (*s)
               *s = 0;
         }
      }
   }
   if (sigsetjmp(context->error_jump, 1)) {
      /* An error occurred */
      if (f1)
         file_close(f1);
      if (f2)
         file_close(f2);
      if (ps1)
         free(ps1);
      context_pop();
      SYMBOL_POP(at_file);
      prompt_string = saved_prompt;
      siglongjmp(context->error_jump, -1);
   }
   /* Toplevel loop */
   exit_flag = 0;
   
   while ( !exit_flag && !feof(context->input_file)) {
      MM_ENTER;
      if (context->input_file == stdin)
         fflush(stdout);
      /* Skip junk */
      prompt_string = ps1;
      while (skip_to_expr() == ')')
         read_char();
      /* Read */
      prompt_string = ps2;
      TOPLEVEL_MACHINE;
      q1 = read_list();
      //printf("%s\n",pname(q1));
      if (q1) {
         /* Eval */
         prompt_string = ps3;
         error_doc.debug_tab = 0;
         TOPLEVEL_MACHINE;
         discard_flag = false;
         q2 = eval(q1);
         /* Print */
         if (f2) {
            if (discard_flag) {
               print_string("= ");
               print_string(first_line(q2));
            } else {
               print_string("= ");
               print_list(q2);
	}
            print_char('\n');
         }
         discard_flag = false;
         var_set(result, q2);
      }
      MM_EXIT;
   }
   /* Cleanup */
   if (context->input_file) {
      clearerr(context->input_file);
      if (context->input_file == stdin) {
         line_pos = line_buffer;
         *line_buffer = 0;
      }
   }
   if (f1) {
      file_close(f1);
      strcpy(file_name, String(ans));
   }
   if (f2)
      file_close(f2);
   if (ps1)
      free(ps1);
   context_pop();
   SYMBOL_POP(at_file);
   exit_flag = 0;
   prompt_string = saved_prompt;
   error_doc.debug_tab = debug_tab;

   MM_EXIT;
}

DX(xgc)
{
   ARG_NUMBER(0);
   return NEW_NUMBER(mm_collect_now());
}

DX(xmeminfo)
{
   int level = 1;
   if (arg_number == 1) {
      level = AINTEGER(1);
   } else if (arg_number > 1)
      ARG_NUMBER(-1);

   print_char('\n');
   print_string(mm_info(level));
   return NIL;
}

#define MAX_NUM_MEMTYPES 200

struct htable { at *backptr; };

DY(ymemprof)
{
   struct lush_context c;
   int hist[MAX_NUM_MEMTYPES];
   int n = mm_prof_start(NULL);
   assert(n < MAX_NUM_MEMTYPES);

   context_push(&c);
   if (sigsetjmp(context->error_jump, 1)) {
      mm_prof_stop(hist);
      context_pop();
      siglongjmp(context->error_jump, -1);
   }
   mm_prof_start(hist);
   at *res = progn(ARG_LIST);
   mm_prof_stop(hist);
   context_pop();
   
   /* put results in a hash table and bind *memprof-stats* to it */
   htable_t *stats = new_htable(n, false, true);
   char **key = mm_prof_key();
   for (int i=0; i<n; i++)
      htable_set(stats, NEW_STRING(key[i]), NEW_NUMBER(hist[i]));
   var_set(at_memstats, stats->backptr);
   
   return res;
}

DY(ywith_nogc)
{
   struct lush_context c;
   context_push(&c);

   MM_NOGC;
   if (sigsetjmp(context->error_jump, 1)) {
      MM_NOGC_END;
      context_pop();
      siglongjmp(context->error_jump, -1);
   }
   at *result = progn(ARG_LIST);
   context_pop();
   MM_NOGC_END;
   
   return result;
}

DX(xload)
{
   if (arg_number == 3) {
      toplevel(ASTRING(1), ASTRING(2), ASTRING(3));
      
   } else if (arg_number == 2) {
      toplevel(ASTRING(1), ASTRING(2), NIL);

   } else {
      ARG_NUMBER(1);

      static int excepts = 0;
      excepts = fetestexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
#ifdef HAVE_FEENABLEEXCEPT
      static int traps = 0;
      traps = fegetexcept();
#endif
      toplevel(ASTRING(1), NIL, NIL);
      bool fpu_changed = excepts != fetestexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
#ifdef HAVE_FEENABLEEXCEPT
      fpu_changed |= traps != fegetexcept();
#endif
      if (fpu_changed) {
         fprintf(stderr, "*** Warning: FPU state changed while loading file\n");
         fprintf(stderr, "***        : '%s'\n", ASTRING(1));
      }
   }
   
   return make_string(file_name); /* fishy */
}

/*
 * (discard ....) Evals his argument list, but force toplevel to print only a
 * few characters as result. However, the system variable RESULT contains
 * always the result.
 */
DY(ydiscard)
{
   discard_flag = true;
   return progn(ARG_LIST);
}


/* 
 * exit
 */

void set_toplevel_exit_flag(void)
{
   exit_flag = 1;
}

DX(xexit)
{
   if (arg_number==1) {
      int n = AINTEGER(1);
      clean_up();
      exit(n);
   }
   set_toplevel_exit_flag();
   return NIL;
}


/***********************************************/

/*
 * error-routines
 * 
 * error(prefix,text,atsuffix) is the main error routine. print_error(prefix
 * text atsuffix) prints the error stocked in error_doc.
 */

static char unknown_errmsg[] = "No information on current error condition";

static const char *error_text(void)
{
   extern char print_buffer[];
   const char *prefix = error_doc.error_prefix;
   const char *prefixsep = " : ";
   const char *text = error_doc.error_text;
   const char *textsep = " : ";
   at *suffix = error_doc.error_suffix;
   at *call = error_doc.error_call;
  
   strcpy(print_buffer, unknown_errmsg);
   
   if (!prefix && CONSP(call) && CONSP(Car(call)) && SYMBOLP(Caar(call))) {
      prefix = NAMEOF(Caar(call));
   }
   prefix = prefix ? prefix : "";
   text = text ? text : "";
   if (!*prefix || !*text)
      prefixsep = "";
   if (!*text || !suffix)
      textsep = "";
   sprintf(print_buffer,"%s%s%s%s%s", 
           prefix, prefixsep, text, textsep, 
           suffix ? first_line(suffix) : "" );
   return mm_strdup(print_buffer);
}


void user_break(char *s)
{
   eval_ptr = eval_std;
   if (error_doc.ready_to_an_error == false)
      lastchance("Break");
   
   TOPLEVEL_MACHINE;
   error_doc.error_call = call_stack();
   error_doc.ready_to_an_error = false;
   error_doc.error_prefix = NIL;
   error_doc.error_text = "Break";
   error_doc.error_suffix = NIL;
   line_pos = line_buffer;
   *line_buffer = 0;
   
   if (!error_doc.debug_toplevel && strcmp(s,"on read")) {
      at *(*sav_ptr)(at *) = eval_ptr;
      eval_ptr = eval_std;
      error_doc.ready_to_an_error = true;
      error_doc.debug_toplevel = true;
      
      at *q = apply(Value(at_break),NIL);
      ifn (q) {
         error_doc.debug_toplevel = false;
         error_doc.error_call = NIL;
         error_doc.error_prefix = NIL;
         error_doc.error_text = NIL;
         error_doc.error_suffix = NIL;
         eval_ptr = sav_ptr;
         return;
      }

   } else {
      FILE *old = context->output_file;
      context->output_file = stdout;
      print_string("\n\007*** Break\n");
      context->output_file = old;      
   }
   recur_doc_init();
   error_doc.debug_toplevel = false;
   siglongjmp(context->error_jump, -1);
   while (1) { /* no return */ };
}


void error(const char *prefix, const char *text, at *suffix)
{
   if (in_compiled_code)
      lush_error(text);

   eval_ptr = eval_std;
   
   if (error_doc.ready_to_an_error == false)
      lastchance(text);
  
   TOPLEVEL_MACHINE;
   error_doc.error_prefix = (char *)prefix;
   error_doc.error_text = text;
   error_doc.error_suffix = suffix;
   error_doc.error_call = call_stack();
   error_doc.ready_to_an_error = false;
   recur_doc_init();
   line_pos = line_buffer;
   *line_buffer = 0;

   if (! error_doc.debug_toplevel) {
      eval_ptr = eval_std;
      error_doc.ready_to_an_error = true;
      error_doc.debug_toplevel = true;
      apply(Value(at_debug),NIL);

   } else  {
      FILE *old;
      text = error_text();
      old = context->output_file;
      context->output_file = stdout;
      print_string("\n\007*** ");
      print_string(text);
      print_string("\n");
      context->output_file = old;      
   }
   error_doc.debug_toplevel = false;
   siglongjmp(context->error_jump, -1);
   while (1) { /* no return */ };
}

DX(xerror)
{
   at *symb, *arg;
   const char *errmsg;
   
   switch (arg_number) {
   case 1:
      error(NIL, ASTRING(1), NIL);

   case 2:
      if (SYMBOLP(APOINTER(1))) {
         symb = APOINTER(1);
         errmsg = ASTRING(2);
         arg = NIL;
      } else {
         symb = NIL;
         errmsg = ASTRING(1);
         arg = APOINTER(2);
      }
      break; 

   case 3: 
      ASYMBOL(1);
      symb = APOINTER(1);
      errmsg = ASTRING(2);
      arg = APOINTER(3);
      break;

   default:
      error(NIL,"illegal arguments",NIL);
   }

   /* walk down the call stack until the function symbol */
   if (symb) {
      while (top_link) {
         if (CONSP(top_link->this_call) && Car(top_link->this_call)==symb)
            break;
         top_link = top_link->prev;
      }
   }
   error(NIL, errmsg, arg);
   return NIL;
}

DX(xbtrace)
{
   int n = 0;
   if (arg_number==1) {
      n = AINTEGER(1);
   } else {
      ARG_NUMBER(0);
   }
   
   at *calls = error_doc.error_call;
   if (!calls)
      calls = call_stack();
   if (n < 0)
      return calls;
   
   while (--n!=0 && CONSP(calls)) {
      if (calls == error_doc.error_call)
         print_string("** in:   ");
      else
         print_string("** from: ");
      print_string(first_line(Car(calls)));
      calls = Cdr(calls);
      print_string("\n");
   }
   return NIL;
}

DX(xerrname)
{
   ARG_NUMBER(0);
   return NEW_STRING(error_text());
}

DX(xquiet)
{
   if (arg_number>0)
   {
      extern int line_flush_stdout;
      ARG_NUMBER(1);
      quiet = ((APOINTER(1)) ? true : false);
      line_flush_stdout = ! quiet;
   }
   return ((quiet) ? t() : NIL);
}

DX(xcasesensitive)
{
   if (arg_number>0) {
      ARG_NUMBER(1);
      if (APOINTER(1))
         context->input_case_sensitive = 1;
      else
         context->input_case_sensitive = 0;
   }
   if (context->input_case_sensitive)
      return t();
   return NIL;
}


/* --------- INITIALISATION CODE --------- */


void init_toplevel(void)
{
   //MM_ROOT(error_doc.this_call);
   MM_ROOT(error_doc.error_call);
   MM_ROOT(error_doc.error_suffix);

   at_startup  = var_define("startup");
   at_toplevel = var_define("toplevel");
   at_break =    var_define("break-hook");
   at_debug =    var_define("debug-hook");
   at_file =     var_define("file-being-loaded");
   at_memstats = var_define("*memprof-stats*");
   result =      var_define("result");

   dx_define("gc", xgc);
   dx_define("meminfo", xmeminfo);
   dy_define("memprof", ymemprof);
   dy_define("with-nogc", ywith_nogc);
   dx_define("exit", xexit);
   dx_define("load", xload);
   dy_define("discard", ydiscard);
   dx_define("error", xerror);
   dx_define("btrace", xbtrace);
   dx_define("errname", xerrname);
   dx_define("lush-is-quiet", xquiet);
   dx_define("lush-is-case-sensitive", xcasesensitive);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
