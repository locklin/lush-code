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

/* Notes on the symbol implementation:
 *
 * 1. For every symbol there is a unique hash_name in names
 *    and a unique AT (hn->named).
 *
 * 2. All live hash_names are in names, which is a memory root.
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
 * 5. As long as a symbol is globally bound, its associated AT
 *    shall not be reclaimed.
 *
 * 6. When a hash_name is found to be unreachable, it is
 *    tentatively moved into the purgatory. It may be resurrected
 *    from there when a search for it occurs before the next gc.
 *    (The purgatory is necessary for asynchronous gc to work
 *    correctly).
 */

#include "header.h"

#define SYMBOL_CACHE_SIZE      128

#define SYMBOL_TYPELOCKED_BIT  2

#define SYMBOL_LOCKED_P(s)     ((uintptr_t)(s->hn) & SYMBOL_LOCKED_BIT)
#define SYMBOL_TYPELOCKED_P(s) ((uintptr_t)(s->hn) & SYMBOL_TYPELOCKED_BIT)
#define SYMBOL_VARIABLE_P(s)   ((uintptr_t)(s->hn) & SYMBOL_VARIABLE_BIT)
#define MARKVAR_SYMBOL(s)      SET_PTRBIT(s->hn, SYMBOL_VARIABLE_BIT)
#define LOCK_SYMBOL(s)         SET_PTRBIT(s->hn, SYMBOL_LOCKED_BIT)
#define UNLOCK_SYMBOL(s)       UNSET_PTRBIT(s->hn, SYMBOL_LOCKED_BIT)
#define TYPELOCK_SYMBOL(s)     SET_PTRBIT(s->hn, SYMBOL_TYPELOCKED_BIT)
#define TYPEUNLOCK_SYMBOL(s)   UNSET_PTRBIT(s->hn, SYMBOL_TYPELOCKED_BIT)
#define SYM_HN(s)              ((hash_name_t *)CLEAR_PTR((s)->hn))
#define SYM_AT(s)              (SYM_HN(s)->backptr)

/* globally defined names */

typedef struct hash_name {
   const char *name;
   at *backptr;
   struct hash_name *next;
   unsigned long hash;
} hash_name_t;

/* hash table of currently used symbol names */
static hash_name_t **live_names = NULL;
static hash_name_t **purgatory = NULL;

/* cache for bindings */
static symbol_t **cache = NULL;
static int cache_index = 0;

/* predefined symbols */
at *at_t;
static at *at_scope;

/* forward declarations */
static bool unlink_symbol_hash(hash_name_t *);
static hash_name_t *resurrect_or_new_symbol_hash(const char *, unsigned long);
static hash_name_t *get_symbol_hash_by_name(const char *);


static void clear_symbol_hash(hash_name_t *hn, size_t _)
{
   hn->name = NULL;
   hn->backptr = NULL;
   hn->next = NULL;
}

static void mark_symbol_hash(hash_name_t *hn)
{
   MM_MARK(hn->name);
   if (hn->backptr)
      MM_MARK(Mptr(hn->backptr));  /* weak reference to top of symbol stack */
   MM_MARK(hn->next);
}

static mt_t mt_symbol_hash = mt_undefined;


static void clear_symbol(symbol_t *s, size_t _)
{
   memset(s, 0, sizeof(symbol_t));
}

static void mark_symbol(symbol_t *s)
{
   //MM_MARK(s->hn); /* not necessary, all hns are in names */

   if (s->valueptr)
      MM_MARK(*(s->valueptr));

   if (s->next)
      MM_MARK(s->next)
   else if (s->valueptr) /* when a global symbol mark the AT */
      MM_MARK(SYM_AT(s));
}

/* 
 * When an AT referencing a symbol is reclaimed,
 * remove the hash_name from the symbol hash (names).
 */

static mt_t mt_symbol = mt_undefined;

/*
 * Return a pointer to a hash node for name <s> or NULL when
 * no such names was found in <names>.
 * 
 * When <mode>==0 just search, when <mode>==1 create new hash node
 * if not found, when <mode>==-1 unlink if found.
 *
 */

static void clear_at_symbol(at *a, size_t _)
{
   a->head.cl = symbol_class;
   Symbol(a) = NULL;
}

static void mark_at_symbol(at *a)
{
   MM_MARK(Symbol(a));
}

/* ats for symbols are special, they need a finalizer */
static bool finalize_at_symbol(at *a)
{
   symbol_t *s = Symbol(a);
/*    assert(s->next==NULL); */
/*    assert(s->valueptr==NULL); */
/*    assert(SYM_AT(s)==a); */
   return unlink_symbol_hash(SYM_HN(s));
}

static mt_t mt_at_symbol = mt_undefined;

/* 
 * If symbol hash is in live_names, move it to purgatory and
 * return false. If it is in purgatory, unlink it and return true.
 */
static bool unlink_symbol_hash(hash_name_t *hn)
{
   hash_name_t **lasthn = live_names + (hn->hash % (HASHTABLESIZE-1));
   hash_name_t **lastpurg = purgatory + (hn->hash % (HASHTABLESIZE-1));
   hash_name_t *lhn = *lasthn;

   while (lhn && (lhn != hn)) {
      lasthn = &(lhn->next);
      lhn = *lasthn;
   }

   if (lhn) {
      /* found, move to purgatory */
      *lasthn = lhn->next;
      lhn->next = *lastpurg;
      *lastpurg = lhn;
      return false;

   } else {
      /* it must be in purgatory then */
      lhn = *lastpurg;
      while (lhn && (lhn != hn)) {
         lastpurg = &(lhn->next);
         lhn = *lastpurg;
      }
      assert(lhn);
      *lastpurg = lhn->next;
      lhn->next = NULL;
      return true;
   }
}

at *NEW_SYMBOL(const char *name)
{
   return SYM_AT(new_symbol(mm_strdup(name)));
}

symbol_t *new_symbol(const char *name)
{
   assert(mm_ismanaged(name));
   if (name[0] == ':' && name[1] == ':')
      error(NIL, "belongs to a reserved package... ", NEW_STRING(name));
   
   hash_name_t *hn = get_symbol_hash_by_name(name);
   assert(hn->backptr);
   return Symbol(hn->backptr);
}

static hash_name_t *get_symbol_hash_by_name(const char *s)
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
   
   /* Search in live_names */
   hash_name_t **lasthn = live_names + (hash % (HASHTABLESIZE-1));
   hash_name_t *hn = *lasthn;
   while (hn && strcmp(s, hn->name)) {
      lasthn = &(hn->next);
      hn = *lasthn;
   }

   return hn ? hn : resurrect_or_new_symbol_hash(s, hash);
}

static hash_name_t *resurrect_or_new_symbol_hash(const char *s, unsigned long hash)
{
   /* check purgatory, create new if not found there */  
   hash_name_t **lastpurg = purgatory + (hash % (HASHTABLESIZE-1));
   hash_name_t *hn = *lastpurg;
   while (hn && strcmp(s, hn->name)) {
      lastpurg = &(hn->next);
      hn = *lastpurg;
   }
   if (hn) {
      /* unlink and move to live_names */
      *lastpurg = hn->next;
   } else {
      hn = mm_alloc(mt_symbol_hash);
      hn->hash = hash;
      hn->name = s;
      hn->backptr = mm_alloc(mt_at_symbol);
      AssignClass(hn->backptr, symbol_class);
      if (cache_index) {
         Symbol(hn->backptr) = cache[cache_index];
         cache[cache_index--] = NULL;
      } else
         Symbol(hn->backptr) = mm_alloc(mt_symbol);
      Symbol(hn->backptr)->hn = hn;
   }

   /* link in at front of bucket */
   hash_name_t **lasthn = live_names + (hash % (HASHTABLESIZE-1));
   hn->next = *lasthn;
   *lasthn = hn;
   return hn;
}


/* push the value q on the symbol stack */

symbol_t *symbol_push(symbol_t *s, at *q, at **valueptr)
{
   symbol_t *sym = NULL;
   if (cache_index) {
      sym = cache[cache_index];
      cache[cache_index--] = NULL;
   } else
      sym = mm_alloc(mt_symbol);
   sym->next = s;
   sym->hn = SYM_HN(s);
   if (valueptr) {
      sym->valueptr = valueptr;
   } else {
      sym->value = q;
      sym->valueptr = &(sym->value);
   }
   return sym;
}

/* pop a value off the symbol stack */
symbol_t *symbol_pop(symbol_t *s)
{
   if (cache_index==SYMBOL_CACHE_SIZE) {
      return s->next;
   } else {
      symbol_t *next = s->next;
      memset(s, 0, sizeof(symbol_t));
      cache[++cache_index] = s;
      return next;
   }
}


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
         hn = live_names[hni];
         hni++;
         continue;
      } else
         assert(hn->backptr);
/*          if (!hn->backptr) { */
/*          hn = hn->next; */
/*          continue; */
/*       } */
      hash_name_t *h = hn;
      hn = hn->next;

      /* compare symbol names */
      if (text[0]=='|' || text[0]==h->name[0] || tolower(text[0])==h->name[0]) {
         const uchar *a = (const uchar *) text;
         uchar *b = (uchar *) pname(h->backptr);
         int i = 0;
         while (a[i]) {
            uchar ac = a[i];
            uchar bc = b[i];
            if (text[0] != '|' && !context->input_case_sensitive)
               ac = tolower(ac);
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


/*
 * named(s) finds the SYMBOL named S. NEW_SYMBOL is similar but requires
 * that the argument be a managed string.
 */

at *named(const char *s)
{
   return NEW_SYMBOL(s);
}

DX(xnamed)
{
   ARG_NUMBER(1);
   return SYM_AT(new_symbol(ASTRING(1)));
}

/*
 * namedclean(s) applies standard name canonicalization
 */

at *namedclean(const char *n)
{
   char *d = mm_strdup(n);
   if (!d)
      RAISEF("not enough memory", NIL);

   if (*d == '|') {
      char *s = d+1;
      for (; *s; s++)
         s[-1] = *s;
      s[-1] = 0;
      if (s>d+1 && s[-2] == '|')
         s[-2] = 0;

   } else if (!context->input_case_sensitive) {
      for (char *s = d; *s; s++)
         if (isascii(*(unsigned char*)s))
            *s = tolower(*(unsigned char*)s);
   }
   return NEW_SYMBOL(d);
}

DX(xnamedclean)
{
   ARG_NUMBER(1);
   return namedclean(ASTRING(1));
}


/*
 * nameof(p) returns the name of the SYMBOL p
 */

const char *nameof(symbol_t *s)
{
   if (SYM_HN(s))
      return SYM_HN(s)->name;
   else
      return NIL;
}

const char *NAMEOF(at *p)
{
   assert(SYMBOLP(p));
   return nameof(Symbol(p));
}

DX(xnameof)
{
   ARG_NUMBER(1);
   const char *s = nameof(ASYMBOL(1));
   if (!s)
      RAISEFX("not a symbol", APOINTER(1));
   return NEW_STRING(s);
}


#define iter_hash_name(i,hn) \
  for (i=live_names; i<live_names+HASHTABLESIZE; i++) \
  for (hn= *i; hn; hn = hn->next)

/* sorted list of globally defined symbols */

static at *global_names(void)
{
   mm_collect_now(); /* remove non-globals now */

   hash_name_t **j, *hn;

   at **where, *answer = NIL;
   iter_hash_name(j, hn) {
      const char *name = hn->name;
      where = &answer;
      while (*where && strcmp(name, String(Car(*where))) > 0)
         where = &Cdr(*where);
      *where = new_cons(hn->backptr, *where);
   }
   return answer;
}

DX(xsymbols)
{
   ARG_NUMBER(0);
   return global_names();
}

/* symbols and values, not sorted */
at *global_defs()
{
   mm_collect_now();  /* remove non-globals now */

   at *ans = NIL;
   at **where = &ans;
   hash_name_t **j, *hn;
   iter_hash_name(j, hn) {
      assert(SYMBOLP(hn->backptr));
      if (SYMBOLP(hn->backptr)) {
         at *p = hn->backptr;
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
   }
   return ans;
}

DX(xglobal_defs)
{
   ARG_NUMBER(0);
   return global_defs();
}


/*
 * Class functions
 */

static const char *symbol_name(at *p)
{
   const char *s = NAMEOF(p);
   assert(s);
   return s;
}


static at *symbol_selfeval(at *p)
{
   symbol_t *s = Symbol(p);

   if (SYMBOL_VARIABLE_P(s)) {
      class_t *cl = classof(*(s->valueptr));
      return cl->selfeval(*(s->valueptr));

   } else if (s->valueptr) {
      if (ZOMBIEP(*(s->valueptr)))
         *(s->valueptr) = NIL;
      return *(s->valueptr);

   } else {
      return NIL;
   }
}


static unsigned long symbol_hash(at *p)
{
   symbol_t *s = Symbol(p);
   return SYM_HN(s)->hash;
}


/* Others functions on symbols	 */

at *getslot(at *obj, at *prop)
{
   if (!prop) {
      return obj;
      
   } else {
      const class_t *cl = classof(obj);
      ifn (cl->getslot)
         error(NIL, "object does not accept scope syntax", obj);
      ifn (LISTP(prop))
         error(NIL, "illegal scope specification", prop);
      return (*cl->getslot)(obj, prop);
   }
}

void setslot(at **pobj, at *prop, at *val)
{
   if (!prop) {
      *pobj = val;

   } else {
      at *obj = *pobj;
      const class_t *cl = classof(obj);
      ifn (cl->setslot)
         error(NIL, "object does not accept scope syntax", obj);
      ifn (LISTP(prop))
         error(NIL, "illegal scope specification", prop);
      (*cl->setslot)(obj, prop, val);    
   }
}


static void sym_set(symbol_t *, at *, bool);

at *setq(at *p, at *q)
{
   if (SYMBOLP(p)) {             /* (setq symbol value) */
      symbol_t *s = Symbol(p);
      
      if (SYMBOL_VARIABLE_P(s)) {
         class_t *cl = classof(*(s->valueptr));
         cl->setslot(*(s->valueptr), NIL, q);
         return q;
      }
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

DY(ysetq)
{
   at *p = ARG_LIST;
   at *res = NIL;
   while (CONSP(p)) {
      if (LASTCONSP(p))
         RAISEF("even number of arguments expected", NIL);
      at *q = Car(p);
      p = Cdr(p);
      res = setq(q, eval(Car(p)));
      p = Cdr(p);
   }
   return res;
}

/* reset symbols to global binding (used by error routine) */
void reset_symbols(void)
{
   hash_name_t **j, *hn;

   iter_hash_name(j, hn) {
      at *p = hn->backptr;
      if (p) {
         symbol_t *s = Symbol(p);
         while (s->next)
            s = s->next;
         Symbol(p) = s;
      }
   }
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


DY(ylock_symbol)
{
   at *p = ARG_LIST;
   while (CONSP(p)) {
      LOCK_SYMBOL(Symbol(Car(p)));
      p = Cdr(p);
   }
   return NIL;
}

DY(yunlock_symbol)
{
   at *p = ARG_LIST;
   while (CONSP(p)) {
      UNLOCK_SYMBOL(Symbol(Car(p)));
      p = Cdr(p);
   }
   return NIL;

}

DX(xsymbolp)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   return SYMBOLP(p) ? p : NIL;
}

/* for debugging purposes */
/*
DX(xsymbol_stack)
{
   ARG_NUMBER(1);
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
   return SYMBOL_LOCKED_P(ASYMBOL(1)) ? t() : NIL;
}

DX(xsymbol_globally_bound_p)
{
   ARG_NUMBER(1);
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

   if (SYMBOL_LOCKED_P(s))
      error(NIL, "symbol locked", SYM_AT(s));

   ifn (s->valueptr) {
      s->value = NIL;
      s->valueptr = &(s->value);
   }
/*    if (SYMBOL_TYPELOCKED_P(s)) { */
/*       class_t *cl = classof(*(s->valueptr)); */
/*       ifn (isa(q, cl)) */
/*          error(NIL, "invalid assignment to type-locked symbol", SYM_HN(s)->named); */
/*    } */
   *(s->valueptr) = q;
}

void var_set(at *p, at *q) 
{
   ifn (SYMBOLP(p))
      RAISEF("not a symbol", p);
   sym_set(Symbol(p), q, true);
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

bool symbol_locked_p(symbol_t *s)
{
   return SYMBOL_LOCKED_P(s);
}

/* bool symbol_typelocked_p(symbol_t *s) */
/* { */
/*    return SYMBOL_TYPELOCKED_P(s); */
/* } */


DX(xset)
{
   ARG_NUMBER(2);
   ASYMBOL(1);
   return setq(APOINTER(1), APOINTER(2));
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

   if (mt_at_symbol == mt_undefined)
      mt_at_symbol = MM_REGTYPE("at-symbol", sizeof(struct at),
                                clear_at_symbol, mark_at_symbol, finalize_at_symbol);
   
   if (!live_names) {
      size_t s = sizeof(hash_name_t *) * HASHTABLESIZE + \
         sizeof(void *) * (SYMBOL_CACHE_SIZE + 1);
      live_names = mm_allocv(mt_refs, 2*s);
      purgatory = &(live_names[HASHTABLESIZE]);
      cache = (void *) (&live_names[2*HASHTABLESIZE]);
      MM_ROOT(live_names);
   }

   if (!symbol_class) {
      symbol_class = new_builtin_class(NIL);
      symbol_class->name = symbol_name;
      symbol_class->selfeval = symbol_selfeval;
      symbol_class->hash = symbol_hash;
      symbol_class->dontdelete = true;
   }
}
      
/* --------- INITIALISATION CODE --------- */

class_t *symbol_class = NULL;

void init_symbol(void)
{
   pre_init_symbol();

   at_scope = var_define("scope");
   at_t = var_define("t");
   var_set(at_t, at_t);
   var_lock(at_t);
   
   /* set up symbol_class */
   class_define("Symbol", symbol_class);
   
   dx_define("global-defs", xglobal_defs);
   dx_define("namedclean", xnamedclean);
   dx_define("named", xnamed);
   dx_define("nameof", xnameof);
   dx_define("symbols", xsymbols);
   dx_define("set", xset);
   dy_define("setq", ysetq);
   dy_define("scope",yscope);
   dy_define("lock-symbol", ylock_symbol);
   dy_define("unlock-symbol", yunlock_symbol);
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
