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
 * $Id: htable.c,v 1.15 2008/08/17 20:18:44 leonb Exp $
 **********************************************************************/

/***********************************************************************
  This file contains functions for handling hash tables
********************************************************************** */

#include "header.h"

struct htpair
{
   struct htpair *next;
   unsigned long hash;
   at   *key;
   at   *value;
};

static void clear_htpair(struct htpair *h)
{
   h->next = NULL;
   h->key  = NULL;
   h->value = NULL;
}

static void mark_htpair(struct htpair *h)
{
   MM_MARK(h->next);
   MM_MARK(h->key);
   MM_MARK(h->value);
}

/* weak reference to key */
static void mark_htpair_wk(struct htpair *h)
{
   MM_MARK(h->next);
   MM_MARK(h->value);
}

typedef struct hashtable
{
   int   size;
   int   nelems;
   bool  pointerhashp;
   bool  keylockp;
   bool  rehashp;
   bool  raise_keyerror_p;
   struct htpair **table;
} hashtable_t;

static void clear_hashtable(hashtable_t *h)
{
   h->table = NULL;
}

static void mark_hashtable(hashtable_t *h)
{
   MM_MARK(h->table);
}

static hashtable_t *htable_dispose(hashtable_t *h);

static bool finalize_hashtable(hashtable_t *h)
{
   //printf("finalizer for hash table at 0x%x invoked\n",
   //       (unsigned int) h);
   htable_dispose(h);
   return true;
}

static mt_t mt_htpair = mt_undefined;
static mt_t mt_htpair_wk = mt_undefined;
static mt_t mt_hashtable = mt_undefined;





/* HASHTABLE CLASS FUNCTIONS */

static hashtable_t *htable_dispose(hashtable_t *h)
{
   if (h->table) {
      ifn (h->keylockp)    /* we used weak references */
         del_notifiers_with_context(h);
      h->table = NULL;
   }
   return h;
}

static at *htable_listeval(at *p, at *q)
{
   q = Cdr(q);
   int nargs = length(q);

   if (nargs==1) {
      /* retrieve an element */
      q = eval_a_list(q);
      p = htable_get(p, Car(q));
      
   } else if ((nargs&1)==0) {
      /* set or update key-value pair(s) */
      q = eval_a_list(q);
      for (at *qq = q; qq; qq = Cddr(qq))
         htable_set(p, Car(qq), Cadr(qq));

   } else
      error(NIL, "one argument or even number of arguments expected", q);
    
   return p;
}


static void htable_serialize(at **pp, int code)
{
   if (code == SRZ_READ) {
      int nelems;
      int pointerhashp, raise_keyerror_p;
      serialize_int(&raise_keyerror_p, code);
      serialize_int(&pointerhashp, code);
      serialize_int(&nelems, code);

      (*pp) = new_htable(nelems, (bool)pointerhashp, (bool)raise_keyerror_p);
      struct hashtable *ht = Mptr(*pp);
      ht->rehashp = true;
      ht->keylockp = true;
      
      for (int i=0; i<nelems; i++)   {
         struct htpair *p = mm_alloc(mt_htpair);
         p->hash = 0;
         p->next = ht->table[0];
         serialize_atstar(&p->key, code);
         serialize_atstar(&p->value, code);
         ht->table[0] = p;
         ht->nelems++;
      }
      
   } else {
      struct hashtable *ht = Mptr(*pp);
      int i = (int)ht->raise_keyerror_p;
      serialize_int(&i, code);
      i = (int)ht->pointerhashp;
      serialize_int(&i, code);
      i = ht->nelems;
      serialize_int(&i, code);
      for (int i=0; i<ht->size; i++) {
         struct htpair *p;
         for (p=ht->table[i]; p; p=p->next) {
            serialize_atstar(&p->key, code);
            serialize_atstar(&p->value, code);
         }
      }
   }
}

static int htable_compare(at *p, at *q, int order)
{
  if (order)
     error(NIL,"cannot rank hashtables",NIL);
  
  hashtable_t *htp = Mptr(p);
  hashtable_t *htq = Mptr(q);
  
  if (htp->nelems != htq->nelems)
     return 1;

  /* compare elements */
  for (int i=0; i<htp->size; i++) {
     struct htpair *n;
     for (n=htp->table[i]; n; n=n->next) {
        at *val = htable_get(q, n->key);
        if (!val && n->value)
           return 1;
        if (!eq_test(val,n->value))
           return 1;
     }
  }
  return 0;
}


static unsigned long htable_hash(at *p)
{
   unsigned long x = 0;
   struct hashtable *ht = Mptr(p);

   for (int i=0; i<ht->size; i++) {
      for (struct htpair *n=ht->table[i]; n; n=n->next) {
         unsigned long y = hash_value(n->key);
         y = (y<<6) ^ hash_value(n->value);
         x ^= y;
      }
   }
   return x;
}


/* HASHTABLE UTILITIES */


/* hash_pointer -- hash pointer bits */

unsigned long hash_pointer(at *p)
{
   unsigned long x = (unsigned long)p;
   x = x ^ (x>>16);
   return x;
}

/* hash_value -- hash object pointed to by p */

unsigned long hash_value(at *p)
{
   unsigned long x = 0;
   struct recur_elt elt;
   char toggle = 0;
   at *slow = p;
   
again:
   x = (x<<1)|((x&0x80000000) ? 0 : 1);
   if (!p) {
      return x;

   } else if (NUMBERP(p)) {
      unsigned int *up = (unsigned int*)(char*)&Number(p);
      x ^= 0x1010;
      x ^= up[0];
      if (sizeof(real) >= 2*sizeof(unsigned int))
         x ^= up[1];
      
   } else if (CONSP(p)) {
      x ^= 0x2020;
      if (recur_push_ok(&elt, &hash_value, Car(p))) {
         x ^= hash_value(Car(p));
         recur_pop(&elt);
      }
      /* go to next list element */
      p = Cdr(p);
      if (p == slow) /* circular list */
         return x;
      toggle ^= 1;
      if (!toggle)
         slow = Cdr(slow);
      goto again;

   } else if (Class(p)->hash) {
      x ^= 0x3030;
      if (recur_push_ok(&elt, &hash_pointer, p)) {
         x ^= Class(p)->hash(p);
         recur_pop(&elt);
      }

   } else if (GPTRP(p)) {
      x ^= 0x4040;
      x ^= (unsigned long)Gptr(p);

   } else {
      x ^= hash_pointer(p);
   }
   return x;
}


/* htable_resize -- extend hash table */

static void htable_resize(hashtable_t *ht, int newsize)
{
   size_t s = sizeof(struct htpair*) * newsize;
   struct htpair **newtable = mm_allocv(mt_refs, s);
   assert(newtable);
   
   /* relocate hashelt */
   for (int i=0; i<ht->size; i++) {
      struct htpair *n = ht->table[i];
      while (n) {
         struct htpair *m = n->next;
         int j = n->hash % newsize;
         n->next = newtable[j];
         newtable[j] = n;
         n = m;
      }
   }
   ht->table = newtable;
   ht->size = newsize;
}

/* htable_notify -- helper for pointerhashp case */

static void htable_notify(at *k, void *table)
{
   hashtable_t *ht = table;
   unsigned long hash = hash_pointer(k);

   struct htpair *n, **np = &(ht->table[hash % ht->size]);
   while ((n = *np)) {
      if (n->key != k) {
         np = &(n->next);
      } else {
         *np = n->next;
         ht->nelems--;
      }
   }
}

/* htable_rehash -- rehash an existing hash table */

static void htable_rehash(hashtable_t *ht)
{
   /* turn off keylockp when pointerhashp is true */
   if (ht->pointerhashp && ht->keylockp) {
      for (int i=0; i<ht->size; i++) {
         struct htpair *n;
         for (n = ht->table[i]; n; n=n->next)
            add_notifier(n->key, (wr_notify_func_t *)htable_notify, ht);
      }
      ht->keylockp = false;
   }
   /* recompute hash numbers */
   for (int i=0; i<ht->size; i++) {
      struct htpair *n, **np = &ht->table[i];
      while ((n = *np)) {
         /* zap zombies */
         if (ZOMBIEP(n->key) || ZOMBIEP(n->value)) {
            *np = n->next;
            ht->nelems--;
            continue;
         }
         /* recompute hash */
         if (ht->pointerhashp)
            n->hash = hash_pointer(n->key);
         else
            n->hash = hash_value(n->key);
         np = &(n->next);
      }
   }
   /* reorganize hash table */
   htable_resize(ht, ht->size);
   ht->rehashp = false;
}


/* HASHTABLE ACCESS FUNCTIONS */


/* htable_set -- set element by key */

LUSHAPI void htable_set(at *hta, at *key, at *value)
{
   ifn (HTABLEP(hta))
      error(NIL,"not a hash table", hta);
   if (ZOMBIEP(value))
      value = NIL;
   if (ZOMBIEP(key))
      return;

   /* check hash table */
   hashtable_t *ht = Mptr(hta);
   if (ht->rehashp)
      htable_rehash(ht);
   
   /* search for pair */
   unsigned long hash = ht->pointerhashp ? 
      hash_pointer(key) : hash_value(key);

   struct htpair *n, **np = &(ht->table[hash % ht->size]);
   while ((n = *np)) {
      if (key == n->key)
         break;
      if (!ht->pointerhashp)
         if (hash == n->hash)
            if (eq_test(key, n->key))
               break;
      np = &(n->next);
   }

   /* set value */
   bool store_value = value || ht->raise_keyerror_p;
   if (store_value && n) {
      n->value = value;

   } else if (store_value) {
      while (2*ht->size < 3*ht->nelems)
         htable_resize(ht, 2*ht->size - 1);

      struct htpair *n = ht->keylockp ?
         mm_alloc(mt_htpair) : mm_alloc(mt_htpair_wk);
      n->hash = hash;
      n->key = key;
      n->value = value;
      n->next = ht->table[hash % ht->size];
      ht->table[hash % ht->size] = n;
      ht->nelems++;

      if (!ht->keylockp)
         add_notifier(key, (wr_notify_func_t *)htable_notify, ht);
      
   } else if (n) {
      *np = n->next;
      ht->nelems -= 1;
   }
}

/* delete element with key */

LUSHAPI void htable_delete(at *hta, at *key)
{
   hashtable_t *ht = Mptr(hta);
   bool raise_keyerror_p = ht->raise_keyerror_p;
   ht->raise_keyerror_p = false;
   htable_set(hta, key, NIL);
   ht->raise_keyerror_p = raise_keyerror_p;
}

/* delete all contents */

LUSHAPI void htable_clear(at *hta)
{
   hashtable_t *ht = Mptr(hta);
   for (int i=0; i<ht->size; i++) {
      struct htpair *n = ht->table[i];
      while (n) {
         n = n->next;
         ht->nelems -= 1;
      }
      ht->table[i] = NULL;
   }
   assert(ht->nelems==0);
}

/* htable_get -- get element by key */

LUSHAPI at *htable_get(at *hta, at *key)
{
   ifn (HTABLEP(hta))
      error(NIL, "not a hash table", hta);
   
   hashtable_t *ht = Mptr(hta);
   if (ht->rehashp)
      htable_rehash(ht);
   unsigned long hash = ht->pointerhashp ?
      hash_pointer(key) : hash_value(key);

   struct htpair *n, **np = &ht->table[hash % ht->size];
   while ((n = *np)) {
      if (key == n->key)
         break;
      if (!ht->pointerhashp)
         if (hash == n->hash)
            if (eq_test(key, n->key))
               break;
      np = &(n->next);
   }
  
   if (n)
      return n->value;
   else if (ht->raise_keyerror_p)
      error(NIL, "unknown key", key);

   return NIL;
}


/* HASHTABLE OPERATIONS */

DX(xhashcode)
{
   char buffer[24];
   ARG_NUMBER(1);
   ARG_EVAL(1);
   unsigned long x = hash_value(APOINTER(1));
   sprintf(buffer,"%08x", (int)(x));
   return new_string(buffer);
}


LUSHAPI at *new_htable(int nelems, bool pointerhashp, bool raise_keyerror_p)
{ 
   int size = (nelems<16 ? 31 : nelems * 2 - 1);
   size_t s = size * sizeof(struct htpair*);
   hashtable_t *ht = mm_alloc(mt_hashtable);
   assert(ht);
   ht->nelems = 0;
   ht->size = size;
   ht->pointerhashp = pointerhashp;
   ht->keylockp = !pointerhashp;
   ht->rehashp = false;
   ht->raise_keyerror_p = raise_keyerror_p;
   ht->table = mm_allocv(mt_refs, s);
   assert(ht->table);
   return new_extern(&htable_class, ht);
}


DX(xnew_htable)
{
   if (arg_number>3)
      RAISEFX("up to three arguments expected", NIL);

   ALL_ARGS_EVAL;
   int nelems = 0;
   bool pointerhashp = false;
   bool raise_keyerror_p = false;

   if (arg_number>=1)
      nelems = AINTEGER(1);
   if (arg_number>=2)
      pointerhashp = APOINTER(2) != NIL;
   if (arg_number>=3)
      raise_keyerror_p = APOINTER(3) != NIL;

   return new_htable(nelems, pointerhashp, raise_keyerror_p);
}


DX(xhtable_alist)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   at *a = APOINTER(1);
   ifn (HTABLEP(a))
      RAISEFX("not a hash table", a);
   hashtable_t *ht = Mptr(a);
   
   a = NIL;
   for (int i=0; i<ht->size; i++)
      for (struct htpair *n=ht->table[i]; n; n=n->next)
         a = new_cons(new_cons(n->key, n->value), a);
   return a;
}

DX(xcopy_htable)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   at *p = APOINTER(1);
   ifn (HTABLEP(p))
      RAISEFX("not a hash table", p);
   hashtable_t *ht = Mptr(p);

   at* ans = new_htable(ht->size, ht->pointerhashp, ht->raise_keyerror_p);
   for (int i=0; i<ht->size; i++)
      for (struct htpair *n=ht->table[i]; n; n=n->next)
         htable_set(ans, n->key, n->value);
   return ans;
}

DX(xhtable_delete)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   at *ans = APOINTER(1);
   ifn (HTABLEP(ans))
      RAISEFX("not a hash table", ans);
   htable_delete(ans, APOINTER(2));
   return ans;
}

DX(xhtable_clear)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *ans = APOINTER(1);
   ifn (HTABLEP(ans))
      RAISEFX("not a hash table", ans);
   htable_clear(ans);
   return ans;
}

DX(xhtable_update)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   at *ans = APOINTER(1);
   ifn (HTABLEP(ans))
      RAISEFX("not a hash table", ans);
   at *other = APOINTER(2);
   ifn (HTABLEP(other))
      RAISEFX("not a hash table", other);
  
   hashtable_t *ht = Mptr(other);
   for (int i=0; i<ht->size; i++)
      for (struct htpair *n = ht->table[i]; n; n = n->next)
         htable_set(ans, n->key, n->value);
   return ans;
}
  

DX(xhtable_keys)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (HTABLEP(p))
      RAISEFX("not a hash table", p);

   hashtable_t *ht = Mptr(p);
   at *ans = NIL;
   for (int i=0; i<ht->size; i++)
      for (struct htpair *n=ht->table[i]; n; n=n->next)
         ans = new_cons(n->key,ans);
   return ans;
}


DX(xhtable_rehash)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (HTABLEP(p))
      error(NIL,"not a hash table", p);

   hashtable_t *ht = Mptr(p);
   ht->rehashp = true;
   return NEW_NUMBER(ht->nelems);
}


/* htable_size -- returns htable number of elements */

DX(xhtable_size)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (HTABLEP(p))
      RAISEFX("not a hash table", p);

   hashtable_t *ht = Mptr(p);
   return NEW_NUMBER(ht->nelems);
}


/* htable_info -- return hashtable statistics */

#define MAXHITDEPTH 32

DX(xhtable_info)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (HTABLEP(p))
      RAISEFX("not a hash table", p);

   struct hashtable *ht = Mptr(p);
   if (ht->rehashp)
      htable_rehash(ht);

   /* Initialize */
   int hit[MAXHITDEPTH];
   int maxhit = -1;
   for (int j=0; j<MAXHITDEPTH; j++)
      hit[j] = 0;
   /* Iterate and count */
   for (int i=0; i<ht->size; i++) {
      struct htpair *n = n=ht->table[i];
      for (int j=0; n; n=n->next, j++) {
         if (j<MAXHITDEPTH)
            hit[j] += 1;
         if (j>maxhit)
            maxhit = j;
      }
   }
   /* Build result */
   at *ans = NIL;
   if (maxhit>=MAXHITDEPTH)
      ans = new_cons(new_cons(NEW_NUMBER(maxhit),NIL),NIL);

   int j = MAXHITDEPTH-1;
   for (; j>=0; j--)
      if (hit[j]) break;

   for (;j>=0; j--)
      ans = new_cons(NEW_NUMBER(hit[j]),ans);
   ans = new_cons(new_cons(named("hits"),ans),NIL);
   ans = new_cons(new_cons(named("buckets"),NEW_NUMBER(ht->size)),ans);
   ans = new_cons(new_cons(named("size"),NEW_NUMBER(ht->nelems)),ans);
   return ans;
}

/* --- INITIALISATION SECTION --- */

class_t htable_class;

void init_htable(void)
{
   mt_htpair =
      MM_REGTYPE("htpair", sizeof(struct htpair),
                 clear_htpair, mark_htpair, 0);
   mt_htpair_wk =
      MM_REGTYPE("htpair_wk", sizeof(struct htpair),
                 clear_htpair, mark_htpair_wk, 0);
   mt_hashtable =
      MM_REGTYPE("hashtable", sizeof(hashtable_t),
                 clear_hashtable, mark_hashtable, finalize_hashtable);
   
   /* setting up htable_class */
   class_init(&htable_class, false);
   htable_class.dispose = (dispose_func_t *)htable_dispose;
   htable_class.listeval = htable_listeval;
   htable_class.serialize = htable_serialize;
   htable_class.compare = htable_compare;
   htable_class.hash = htable_hash;
   class_define("HTABLE", &htable_class);
   
   dx_define("hashcode",xhashcode);
   dx_define("htable",xnew_htable);
   dx_define("copy-htable",xcopy_htable);
   dx_define("htable-alist",xhtable_alist);
   dx_define("htable-delete", xhtable_delete);
   dx_define("htable-clear", xhtable_clear);
   dx_define("htable-update", xhtable_update);
   dx_define("htable-keys", xhtable_keys);
   dx_define("htable-rehash", xhtable_rehash);
   dx_define("htable-size",xhtable_size);
   dx_define("htable-info",xhtable_info);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
