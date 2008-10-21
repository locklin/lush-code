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
 * $Id: symbol.c,v 1.10 2006/02/20 16:04:00 leonb Exp $
 **********************************************************************/

#include "header.h"

typedef unsigned char   uchar;

/* globally defined names */

struct hash_name **names = NULL;

/* Notes on the symbol implementation:
 *
 * 1. For every symbol there is a unique hash_name in names
 *    and a unique AT (hn->named).
 *
 * 2. All hash_name are in names, which is a memory root.
 *
 * 3. Symbol bindings are represented by a chain of objects
 *    of type symbol_t (a misnomer, should have been called
 *    binding_t). This chain is sometimes referred to as the
 *    "symbol stack".
 *
 * 4. A symbol is globally bound if the symbol_t at the end
 *    of the chain (i.e., the one with s->next == NULL) has
 *    a valueptr != NULL.
 *
 * 5. If a symbol is globally bound, its associated AT shall
 *    not be reclaimed by purge_names.
 * 
 * 6. Safely removing unused hash_names is a bit tricky and
 *    requires doing a synchronous GC. It is implemented 
 *    function purge_names.
 */

/* predefined symbols */

at *at_t;
static at *at_scope;
static bool purging_symbols = false;

/* contains the symbol names and hash */

typedef struct hash_name {
   char *name;
   at *named;
   struct hash_name *next;
   unsigned long hash;
} hash_name_t;


void clear_symbol_hash(hash_name_t *hn)
{
   hn->name = NULL;
   hn->named = NULL;
   hn->next = NULL;
}

void mark_symbol_hash(hash_name_t *hn)
{
   MM_MARK(hn->name);
   if (!purging_symbols) {
      MM_MARK(hn->named);
   } else if (hn->named) {
      MM_MARK(Mptr(hn->named));  /* weak reference when purging */
   }
   MM_MARK(hn->next);
}

static mt_t mt_symbol_hash = mt_undefined;


void clear_symbol(symbol_t *s)
{
   s->next = NULL;
   s->hn = NULL;
   s->value = NIL;
   s->valueptr = NULL;
}

void mark_symbol(symbol_t *s)
{
   //MM_MARK(s->hn); /* not necessary, all hns are in names */

   if (s->valueptr)
      MM_MARK(*(s->valueptr));

   if (s->next)
      mm_mark(s->next);

   else if (s->valueptr)
      mm_mark(SYM_HN(s)->named);
}

/* 
 * When an AT referencing a symbol is reclaimed,
 * clear the back reference in the symbol hash.
 */

void at_symbol_notify(at *p, void *_)
{
   symbol_t *s = Symbol(p);
   assert(s->next==NULL);
   assert(s->valueptr==NULL);
   assert(SYM_HN(s)->named == p);
   SYM_HN(s)->named = NIL;
   //search_by_name(s->hn->name, -1); /* done by purge_names directly */
}   

mt_t mt_symbol = mt_undefined;

/*
 * search_by_name(s, mode) returns a ptr to the HASH_NAME node 
 * for name S. Returns NIL if an error occurs. If MODE is 0,
 * just search, if MODE is +1 creates if not found, and if 
 * MODE is -1 deletes it hash node.
 * This is the only function that directly manipulates the
 * static hash table 'names'.
 */

static hash_name_t *search_by_name(const char *s, int mode)
{
   /* Calculate hash value */
   unsigned long hash = 0;
   const uchar *ss = (const uchar *)s;
   while (*ss) {
      uchar c = *ss++;
      if (! c)
         break;
      hash = (hash<<6) | ((hash&0xfc000000)>>26);
      hash ^= c;
   }
   hash_name_t **lasthn = names + (hash % (HASHTABLESIZE-1));
   
   /* Search in list and operate */
   hash_name_t *hn = *lasthn;
   while (hn && strcmp(s, hn->name)) {
      lasthn = &(hn->next);
      hn = *lasthn;
   }
   if (hn && !hn->named)
      printf("search_by_name turned up unused symbol (%s)\n",
             hn->name);
   if (!hn && mode==1) {
      hn = mm_alloc(mt_symbol_hash);
      hn->hash = hash;
      hn->name = mm_strdup(s);
      *lasthn = hn;
      
   } else if (hn && mode==-1) {
      *lasthn = hn->next;    // unlink this hn
      hn = NULL;             // return NULL
   }
   return hn;
}

void purge_names(void)
{
   /* if a gc is under way, wait until done */
   MM_NOGC;
   MM_NOGC_END;

   purging_symbols = true;
   mm_collect_now();
   purging_symbols = false;

   for (int i = 0; i < HASHTABLESIZE; i++) {
      hash_name_t *hn, **phn = &names[i];
      while ((hn = *phn)) {
         if (!hn->named) {
            *phn = hn->next;
         } else
            phn = &(hn->next);
      }
   }
}

#if defined UNIX && HAVE_LIBREADLINE

/* used for readline completion in unix.c */
char *symbol_generator(const char *text, int state)
{
   if (!text || !text[0]) return NULL;

   static int hni;
   static hash_name_t *hn;
   ifn (state) {
      hni = 0;
      hn = 0;
   }
   
   while (hni < HASHTABLESIZE) {
      /* move to next */
      if (!hn)  {
         hn = names[hni];
         hni++;
         continue;
      } else if (!hn->named) {
         hn = hn->next;
         continue;
      }
      hash_name_t *h = hn;
      hn = hn->next;

      /* compare symbol names */
      if (text[0]=='|' || tolower(text[0])==h->name[0]) {
         const uchar *a = (const uchar *) text;
         uchar *b = (uchar *) pname(h->named);
         int i = 0;
         while (a[i]) {
            uchar ac = a[i];
            uchar bc = b[i];
            if (text[0] != '|') 
               ac = tolower(ac);
            if (text[0]!='|' && ac=='_')
               ac = '-';
            if (ac != bc) 
               break;
            i++;
         }
         ifn (a[i])
            return strdup((const char *) b);
      }
   }
   return 0;
}

#endif // UNIX


/*
 * named(s) finds the SYMBOL named S. Actually does the same job as
 * new_symbol. doesn't return NIL if the symbol doesn't exists, but creates
 * it.
 */

at *named(const char *s)
{
   return new_symbol(s);
}

DX(xnamed)
{
   ARG_NUMBER(1);
   ALL_ARGS_EVAL;
   return named(ASTRING(1));
}

/*
 * namedclean(s) applies standard name canonicalization
 * (lowercase, some _ become -)
 */

at *namedclean(const char *n)
{
   char *d = strdup(n);
   if (!d)
      RAISEF("not enough memory", NIL);

   if (*d == '|') {
      char *s = d+1;
      for (; *s; s++)
         s[-1] = *s;
      s[-1] = 0;
      if (s>d+1 && s[-2] == '|')
         s[-2] = 0;

   } else {
      for (char *s = d; *s; s++)
         if (s>d && *s=='_')
            *s = '-';
         else if (isascii(*(unsigned char*)s))
            *s = tolower(*(unsigned char*)s);
   }
   return new_symbol(d);
}

DX(xnamedclean)
{
   ARG_NUMBER(1);
   ALL_ARGS_EVAL;
   return namedclean(ASTRING(1));
}


/*
 * nameof(p) returns the name of the SYMBOL p
 */

char *nameof(symbol_t *s)
{
   if (SYM_HN(s))
      return SYM_HN(s)->name;
   else
      return NIL;
}

char *NAMEOF(at *p)
{
   assert(SYMBOLP(p));
   return nameof(Symbol(p));
}

DX(xnameof)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   char *s = nameof(ASYMBOL(1));
   if (!s)
      RAISEFX("not a symbol", APOINTER(1));
   return new_string(s);
}


#define iter_hash_name(i,hn) \
  for (i=names; i<names+HASHTABLESIZE; i++) \
  for (hn= *i; hn; hn = hn->next)

/* sorted list of globally defined symbols */

at *global_names(void)
{
   hash_name_t **j, *hn;

   at **where, *answer = NIL;
   iter_hash_name(j, hn) {
      char *name = hn->name;
      where = &answer;
      while (*where && strcmp(name, String(Car(*where))) > 0)
         where = &Cdr(*where);
      *where = new_cons(new_string(name), *where);
   }
   return answer;
}

DX(xsymblist)
{
   ARG_NUMBER(0);
   return global_names();
}

/* symbols and values, not sorted */
at *global_defs()
{
   purge_names();

   at *ans = NIL;
   at **where = &ans;
   hash_name_t **j, *hn;
   iter_hash_name(j, hn)
      if (SYMBOLP(hn->named)) {
         at *p = hn->named;
         symbol_t *symb = Symbol(p);
         while (symb->next)
            symb = symb->next;
         if (symb->valueptr) {
            at *val = *symb->valueptr;  /* globally bound */
            *where = new_cons(new_cons(p, val), NIL);
            where = &Cdr(*where);
            /* locked? */
            if (SYMBOL_LOCKED_P(Symbol(p))) {
               *where = new_cons(p, NIL);
               where = &Cdr(*where);
            }
         }
      }
   return ans;
}

DX(xglobal_defs)
{
   ARG_NUMBER(0);
   return global_defs();
}


at *oblist(void)
{
   hash_name_t **j, *hn;
   
   at *answer = NIL;
   iter_hash_name(j, hn) {
      if (hn->named)
         answer = new_cons(hn->named, answer);
   }
   return answer;
}

DX(xoblist)
{
   ARG_NUMBER(0);
   return oblist();
}

/*
 * Class functions
 */

static char *symbol_name(at *p)
{
   char *s = NAMEOF(p);
   if (s)
      return s;
   else
      return Class(p)->name(p);
}


static at *symbol_selfeval(at *p)
{
   symbol_t *symb = Symbol(p);
   at *q = symb->valueptr ? *(symb->valueptr) : NIL;
   
   if (ZOMBIEP(q)) {
      *(symb->valueptr) = NIL;
      return NIL;
   } else {
      return q;
   }
}

static unsigned long symbol_hash(at *p)
{
   symbol_t *s = Symbol(p);
   return SYM_HN(s)->hash;
}


/* Others functions on symbols	 */

at *setq(at *p, at *q)
{
   if (SYMBOLP(p)) {             /* (setq symbol value) */
      symbol_t *s = Symbol(p);
      ifn (s->valueptr)
         fprintf(stderr, "+++ Warning: use <defvar> to declare global variable <%s>.\n",
                 SYM_HN(s)->name ? SYM_HN(s)->name : "??" );
      sym_set(s, q, false);
      
   } else if (CONSP(p)) {          /* scope specification */
      if (Car(p)!=at_scope || !CONSP(Cdr(p)))
         RAISEF("syntax error", p);
      p = Cdr(p);
      ifn (Cdr(p)) {              /* (setq :symbol value)  */
         ifn (SYMBOLP(Car(p)))
            error(NIL, "illegal global scope specification (not a symbol)", p); 
         sym_set(Mptr(Car(p)), q, true);
         
      } else {                    /* (setq :object:slot:slot:slot value ) */
         at *obj = eval(Car(p));
         setslot(&obj, Cdr(p), q);
      }
   } else
      RAISEF("neither a symbol nor scope specification", p);
   
   return q;
}

DX(xsetq)
{
   if ((arg_number & 1) || (arg_number < 2))
      RAISEF("even number of arguments expected", NIL);
   at *q = NIL;
   for (int i = 2; i <= arg_number; i += 2) {
      ARG_EVAL(i);
      q = setq(APOINTER(i-1), APOINTER(i));
   }
   return q;
}

/* reset symbols to global binding (used by error routine) */
void reset_symbols(void)
{
   hash_name_t **j, *hn;

   iter_hash_name(j, hn) {
      at *p = hn->named;
      if (p) {
         symbol_t *s = Symbol(p);
         while (s->next)
            s = s->next;
         Symbol(p) = s;
      }
   }
}

/* push the value q on the symbol stack */
symbol_t *symbol_push(symbol_t *s, at *q)
{
   symbol_t *sym = mm_alloc(mt_symbol);
   sym->next = s;
   sym->hn = SYM_HN(s);
   sym->value = q;
   sym->valueptr = &(sym->value);
   return sym;
}

/* pop a value off the symbol stack */
symbol_t *symbol_pop(symbol_t *s)
{
   return s->next; 
}

at *new_symbol(const char *str)
{
   if (str[0] == ':' && str[1] == ':')
      error(NIL, "belongs to a reserved package... ", new_string(str));
   
   hash_name_t *hn = search_by_name(str, 1);
   ifn (hn->named) {
      /* symbol does not exist yet, create new */
      symbol_t *s = mm_alloc(mt_symbol);
      s->hn = hn;
      hn->named = new_extern(&symbol_class, s);
      add_notifier(hn->named, (wr_notify_func_t *)at_symbol_notify, NULL);
   }
   return hn->named;
}

DY(yscope)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("no arguments", NIL);
   
   at *p = ARG_LIST;
   ifn (Cdr(p)) {
      /* :globalvar */
      ifn (SYMBOLP(Car(p)))
         RAISEFX("not a symbol", Car(p));
      symbol_t *symb = Symbol(Car(p));
      while(symb->next)
         symb = symb->next;
      if (symb->valueptr)
         return symb->value;
      return NIL;
   } else {
      /* 
       * :object:slot:slot:...:slot
       * (scope object slot slot .... slot)
       */
      at *obj = eval(Car(p));
      return getslot(obj, Cdr(p));
   }
}


DX(xlock_symbol)
{
   for (int i = 1; i <= arg_number; i++) {
      LOCK_SYMBOL(ASYMBOL(i));
   }
   return NIL;
}

DX(xunlock_symbol)
{
   for (int i = 1; i <= arg_number; i++) {
      UNLOCK_SYMBOL(ASYMBOL(i));
   }
   return NIL;
}

DX(xsymbolp)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   return SYMBOLP(p) ? p : NIL;
}

/* for debugging purposes */
/*
DX(xsymbol_stack)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   symbol_t *s = ASYMBOL(1);
   at *ans = NIL;
   do {
      ans = new_cons(s->valueptr ? *(s->valueptr) : NIL, ans);
      s = s->next;
   } while (s);

   return ans;
}
*/
   
DX(xsymbol_locked_p)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return SYMBOL_LOCKED_P(ASYMBOL(1)) ? t() : NIL;
}

DX(xsymbol_globally_bound_p)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   symbol_t *s = ASYMBOL(1);
   while (s->next)
      s = s->next;
   if (s->valueptr)
      return t();
   return NIL;
}

DX(xsymbol_globally_locked_p)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   symbol_t *s = ASYMBOL(1);
   while (s->next)
      s = s->next;
   if (SYMBOL_LOCKED_P(s))
      return t();
   return NIL;
}


/* --------- SYMBOL ACCESS FROM C -------- */


at *sym_get(symbol_t *symb, bool in_global_scope)
{
   if (in_global_scope)
      while (symb->next)
         symb = symb->next;

   ifn (symb->valueptr) {
      symb->valueptr = &(symb->value);
      symb->value = NIL;
   }
   
   return *(symb->valueptr);
}

at *var_get(at *p)
{
   if (!SYMBOLP(p))
      RAISEF("not a symbol", p);
   return sym_get(Symbol(p), false);
}

void sym_set(symbol_t *s, at *q, bool in_global_scope)
{
   if (in_global_scope)
      while (s->next)
         s = s->next;

   if ((uintptr_t)(s->hn) & SYMBOL_LOCKED_BIT)
      error(NIL, "locked symbol", SYM_HN(s)->named);
  
   ifn (s->valueptr) {
      s->value = NIL;
      s->valueptr = &(s->value);
   }
   *(s->valueptr) = q;
}

void var_set(at *p, at *q) 
{
   ifn (SYMBOLP(p))
      RAISEF("not a symbol", p);
   sym_set(Symbol(p), q, false);
}

/* set, even is symbol is locked */
void var_SET(at *p, at *q)
{
   symbol_t *symb = Symbol(p);
   ifn (symb->valueptr) {
      symb->valueptr = &(symb->value);
      symb->value = NIL;
   }
   *(symb->valueptr) = q;
}


void var_lock(at *p)
{
   assert(SYMBOLP(p));
   LOCK_SYMBOL(Symbol(p));
}


at *var_define(char *str)
{
   at *p = named(str);
   symbol_t *s = Symbol(p);
   s->valueptr = &(s->value);
   return p;
}


DX(xset)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   symbol_t *s = ASYMBOL(1);
   at *q = APOINTER(2);
   sym_set(s, q, false);
   return q;
}


void pre_init_symbol(void)
{
   if (mt_symbol_hash == mt_undefined)
      mt_symbol_hash =
         MM_REGTYPE("symbol-hash", sizeof(hash_name_t),
                    clear_symbol_hash, mark_symbol_hash, 0);
   if (mt_symbol == mt_undefined)
      mt_symbol = MM_REGTYPE("symbol", sizeof(symbol_t),
                             clear_symbol, mark_symbol, 0);
   
   if (!names) {
      size_t s = sizeof(hash_name_t *) * HASHTABLESIZE;
      names = mm_allocv(mt_refs, s);
      MM_ROOT(names);
   }
}
      
/* --------- INITIALISATION CODE --------- */

class_t symbol_class;

void init_symbol(void)
{
   pre_init_symbol();

   at_scope = var_define("scope");
   at_t = var_define("t");
   var_set(at_t, at_t);
   var_lock(at_t);
   
   /* set up symbol_class */
   class_init(&symbol_class, false);
   symbol_class.name = symbol_name;
   symbol_class.selfeval = symbol_selfeval;
   symbol_class.hash = symbol_hash;
   symbol_class.dontdelete = true;
   class_define("SYMBOL", &symbol_class);
   
   dx_define("global-defs", xglobal_defs);
   dx_define("namedclean", xnamedclean);
   dx_define("named", xnamed);
   dx_define("nameof", xnameof);
   dx_define("symblist", xsymblist);
   dx_define("oblist", xoblist);
   dx_define("set", xset);
   dx_define("setq", xsetq);
   dy_define("scope",yscope);
   dx_define("lock-symbol", xlock_symbol);
   dx_define("unlock-symbol", xunlock_symbol);
   dx_define("symbolp", xsymbolp);    
   //dx_define("symbol-stack", xsymbol_stack);
   dx_define("symbol-locked-p", xsymbol_locked_p);
   dx_define("symbol-globally-bound-p", xsymbol_globally_bound_p);
   dx_define("symbol-globally-locked-p", xsymbol_globally_locked_p);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
