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
 * $Id: lisp_c.c,v 1.13 2002-07-19 03:29:19 leonb Exp $
 **********************************************************************/


#include "header.h"
#include "check_func.h"
#include "dh.h"

#if HAVE_LIBBFD
# define DLDBFD 1
# include "dldbfd.h"
#endif

/* From symbol.c */
extern at *at_true;

/* Forward */
static void update_c_from_lisp();
static void update_lisp_from_c();
static at *dharg_to_at();
static void at_to_dharg();

/* Flags */
static int dont_track_cside = 0;
static int dont_warn = 0;



/* -----------------------------------------
   STORAGE TYPE <-> DHT TYPE
   ----------------------------------------- */

static int storage_to_dht[ST_LAST];
static int dht_to_storage[DHT_LAST];


static void
init_storage_to_dht(void) 
{
  int i;
  for(i=0;i<ST_LAST;i++)
    storage_to_dht[i] = -1;
  for(i=0;i<DHT_LAST;i++)
    dht_to_storage[i] = -1;
  storage_to_dht[ST_F] = DHT_FLT;
  storage_to_dht[ST_D] = DHT_REAL;
  storage_to_dht[ST_I32] = DHT_INT;
  storage_to_dht[ST_I16] = DHT_SHORT;
  storage_to_dht[ST_I8] = DHT_BYTE;
  storage_to_dht[ST_U8] = DHT_UBYTE;
  storage_to_dht[ST_GPTR] = DHT_GPTR;
  dht_to_storage[DHT_FLT] = ST_F;
  dht_to_storage[DHT_REAL] = ST_D;
  dht_to_storage[DHT_INT] = ST_I32;
  dht_to_storage[DHT_BYTE] = ST_I8;
  dht_to_storage[DHT_UBYTE] = ST_U8;
  dht_to_storage[DHT_SHORT] = ST_I16;
  dht_to_storage[DHT_GPTR] = ST_GPTR;
}



/* -----------------------------------------
   THE ITEM AVL TREE
   ----------------------------------------- */



/* C objects are identified and mapped to their LISP counterpart
 * by the means of an balanced tree of AVLNODE structures.
 * Finding the TYPE or the LISP counterpart of a C object is quick
 * because the tree is sorted according to the address of the C object.
 *
 * Conversely LISP objects (i.e. INDEX, STORAGE, OOSTRUCTs) have
 * a pointer named CPTR to the corresponding C object if any.
 * 
 * Several functions are provided for dynamically adjusting the
 * AVL tree. These functions may be called by the interpreter
 * (the object is said to belong to LISP) or by compiled code
 * like pools (the object is said to belong to C).
 *
 * Warning: Everything is less clear STR and LISTS which are
 * only partially supported. We do not support STRs and LISTs
 * that last between DH calls (just like the old code).
 */


typedef struct avlnode {
  /* doubly linked list */
  struct avlnode *chnxt;
  struct avlnode *chprv;
  /* information stored for ctype */
  void *citem;                /* address of the C object */
  at   *litem;                /* pointer to the LISP object */
  char  belong;               /* who created this object */
  char  need_update;          /* needs update */
  char  pad;                  /* free byte */
  char  cinfo;                /* type of this object */
  void *cmoreinfo;            /* extra information on type */
  /* tree management */
  struct avlnode *lt;         /* left subtree */
  struct avlnode *rt;         /* right subtree */
  char avl_lthread;           /* set if no left subtree */
  char avl_rthread;           /* set if no right subtree */
  char avl_bf;                /* which side of the tree is deepest */
} avlnode;


/* values for field CINFO */
enum CINFO {
  CINFO_UNLINKED,
  /* fully supported */
  CINFO_OBJ,
  CINFO_IDX,
  CINFO_SRG,
  CINFO_STR,
  /* The next type is only partially supported:
   * Compiled objects will not remain between DH calls
   * Field LITEM will always be null.
   */
  CINFO_LIST
};

/* values for field BF */
enum BF {
  AVLBALANCED,
  AVLLEFTHEAVY,
  AVLRIGHTHEAVY
};

/* values for field BELONG */
enum BELONG {
  BELONG_LISP,
  BELONG_C
};


/* fast allocation of avl nodes */
static struct alloc_root 
avlnode_alloc = {
  NULL,
  NULL,
  sizeof(struct avlnode),
  1000,
};

static avlnode *root;
static int nobjects;

/* link containing node that we may update */
static avlnode dummy_upds = { &dummy_upds, &dummy_upds };
/* link containing temporary nodes */
static avlnode dummy_tmps = { &dummy_tmps, &dummy_tmps };


/* ------------------------------------------
   ROUTINE FOR IMPLEMENTING A DOUBLY LINKED LIST
   ------------------------------------------ */

void
avlchain_set(avlnode *n, avlnode *head)
{
  if (n->chprv && n->chnxt)
    {
      n->chprv->chnxt = n->chnxt;
      n->chnxt->chprv = n->chprv;
    }
  if (head)
    {
      n->chnxt = head->chnxt;
      n->chprv = head;
      n->chnxt->chprv = n;
      n->chprv->chnxt = n;
    }
  else
    {
      n->chnxt = n->chprv = 0;
    }
}


/* ------------------------------------------
   ROUTINES FOR IMPLEMENTING A THREADED AVL TREE
   (do not read this now, derived from libg++)
   ------------------------------------------ */


#define lthread(x)       ((x)->avl_lthread)
#define rthread(x)       ((x)->avl_rthread)
#define set_lthread(x,y) (x)->avl_lthread = (y)
#define set_rthread(x,y) (x)->avl_rthread = (y)
#define bf(x)            ((x)->avl_bf)
#define set_bf(x,y)      ((x)->avl_bf=(y))


/* avl_succ -- return next node */

#ifdef __GNUC__
inline
#endif
static avlnode *
avl_succ(avlnode *t)
{
  avlnode *r = t->rt;
  if (!t->avl_rthread)
    while (!r->avl_lthread)
      r = r->lt;
  return r;
}

/* avl_pred -- return previous node */

#ifdef __GNUC__
inline
#endif
static avlnode *
avl_pred(avlnode *t)
{
  avlnode *l = t->lt;
  if (!t->avl_lthread)
    while (!l->avl_rthread)
      l = l->rt;
  return l;
}


/* avl_first -- return first node greater than key */

static avlnode *
avl_first(void *key)
{
  avlnode *t = root;
  if (t==0)
    return 0;
  for(;;)
    {
      if (key<t->citem) 
        {
          if (t->avl_lthread)
            return t;
          t = t->lt;
        } 
      else if (key>t->citem) 
        {
          if (t->avl_rthread)
            return avl_succ(t);
          t = t->rt;
        }
      else
        return t;
    }
}


/* avl_find -- find node for a given key */

static avlnode *
avl_find(void *key)
{
  avlnode *t = root;
  if (t==0)
    return 0;
  for(;;)
    {
      if (key<t->citem) 
        {
          if (t->avl_lthread)
            return 0;
          t = t->lt;
        } 
      else if (key>t->citem) 
        {
          if (t->avl_rthread)
            return 0;
          t = t->rt;
        }
      else
        return t;
    }
}


/* avl_add -- adds a node to a key */

static int _need_rebalancing;
static void *_target_item;
static avlnode *_found_node;
static int _already_found;

static avlnode *
_avlnode_new(void *key)
{
  avlnode *n = allocate(&avlnode_alloc);
  memset(n,0,sizeof(avlnode));
  n->citem = key;
  return n;
}

static void 
_avl_add(avlnode **tp)
{
  avlnode *t = *tp;

  if (_target_item == t->citem)
    {
      _found_node = t;
      return;
    }
  else if (_target_item < t->citem)
    {
      if (lthread(t))
        {
          ++nobjects;
          _found_node = _avlnode_new(_target_item);
          set_lthread(_found_node, 1);
          set_rthread(_found_node, 1);
          _found_node->lt = t->lt;
          _found_node->rt = t;
          t->lt = _found_node;
          set_lthread(t, 0);
          _need_rebalancing = 1;
        }
      else
        _avl_add(&(t->lt));
      if (_need_rebalancing)
        {
          switch(bf(t))
            {
            case AVLRIGHTHEAVY:
              set_bf(t, AVLBALANCED);
              _need_rebalancing = 0;
              return;
            case AVLBALANCED:
              set_bf(t, AVLLEFTHEAVY);
              return;
            case AVLLEFTHEAVY:
              {
                avlnode* l = t->lt;
                if (bf(l) == AVLLEFTHEAVY)
                  {
                    if (rthread(l))
                      t->lt = l;
                    else
                      t->lt = l->rt;
                    set_lthread(t, rthread(l));
                    l->rt = t;
                    set_rthread(l, 0);
                    set_bf(t, AVLBALANCED);
                    set_bf(l, AVLBALANCED);
                    *tp = t = l;
                    _need_rebalancing = 0;
                  }
                else
                  {
                    avlnode* r = l->rt;
                    set_rthread(l, lthread(r));
                    if (lthread(r))
                      l->rt = r;
                    else
                      l->rt = r->lt;
                    r->lt = l;
                    set_lthread(r, 0);
                    set_lthread(t, rthread(r));
                    if (rthread(r))
                      t->lt = r;
                    else
                      t->lt = r->rt;
                    r->rt = t;
                    set_rthread(r, 0);
                    if (bf(r) == AVLLEFTHEAVY)
                      set_bf(t, AVLRIGHTHEAVY);
                    else
                      set_bf(t, AVLBALANCED);
                    if (bf(r) == AVLRIGHTHEAVY)
                      set_bf(l, AVLLEFTHEAVY);
                    else
                      set_bf(l, AVLBALANCED);
                    set_bf(r, AVLBALANCED);
                    *tp = t = r;
                    _need_rebalancing = 0;
                    return;
                  }
              }
            }
        }
    }
  else
    {
      if (rthread(t))
        {
          ++nobjects;
          _found_node = _avlnode_new(_target_item);
          set_rthread(t, 0);
          set_lthread(_found_node, 1);
          set_rthread(_found_node, 1);
          _found_node->lt = t;
          _found_node->rt = t->rt;
          t->rt = _found_node;
          _need_rebalancing = 1;
        }
      else
        _avl_add(&(t->rt));
      if (_need_rebalancing)
        {
          switch(bf(t))
            {
            case AVLLEFTHEAVY:
              set_bf(t, AVLBALANCED);
              _need_rebalancing = 0;
              return;
            case AVLBALANCED:
              set_bf(t, AVLRIGHTHEAVY);
              return;
            case AVLRIGHTHEAVY:
              {
                avlnode* r = t->rt;
                if (bf(r) == AVLRIGHTHEAVY)
                  {
                    if (lthread(r))
                      t->rt = r;
                    else
                      t->rt = r->lt;
                    set_rthread(t, lthread(r));
                    r->lt = t;
                    set_lthread(r, 0);
                    set_bf(t, AVLBALANCED);
                    set_bf(r, AVLBALANCED);
                    *tp = t = r;
                    _need_rebalancing = 0;
                  }
                else
                  {
                    avlnode* l = r->lt;
                    set_lthread(r, rthread(l));
                    if (rthread(l))
                      r->lt = l;
                    else
                      r->lt = l->rt;
                    l->rt = r;
                    set_rthread(l, 0);
                    set_rthread(t, lthread(l));
                    if (lthread(l))
                      t->rt = l;
                    else
                      t->rt = l->lt;
                    l->lt = t;
                    set_lthread(l, 0);
                    if (bf(l) == AVLRIGHTHEAVY)
                      set_bf(t, AVLLEFTHEAVY);
                    else
                      set_bf(t, AVLBALANCED);
                    if (bf(l) == AVLLEFTHEAVY)
                      set_bf(r, AVLRIGHTHEAVY);
                    else
                      set_bf(r, AVLBALANCED);
                    set_bf(l, AVLBALANCED);
                    *tp = t = l;
                    _need_rebalancing = 0;
                    return;
                  }
              }
            }
        }
    }
}

static avlnode *
avl_add(void *key)
{
  if (root==0)
    {
      ++nobjects;
      root = _avlnode_new(key);
      root->avl_lthread = root->avl_rthread = 1;
      return root;
    }
  else
    {
      _target_item = key;
      _need_rebalancing = 0;
      _avl_add(&root);
      return _found_node;
    }
}



/* avl_del -- delete a specific item */


void
_avlnode_copy(avlnode *from, avlnode *to)
{
  to->citem = from->citem;
  to->litem = from->litem;
  to->belong= from->belong;
  to->need_update= from->need_update;
  to->pad = from->pad;
  to->cinfo = from->cinfo;
  to->cmoreinfo = from->cmoreinfo;
  /* adjust double chain */
  if (to->chnxt && to->chprv)
    avlchain_set(to,0);
  to->chnxt = from->chnxt;
  to->chprv = from->chprv;
  if (to->chnxt)
    to->chnxt->chprv = to;
  if (to->chprv)
    to->chprv->chnxt = to;
  from->chnxt = from->chprv = 0;
}

void 
_avl_del(avlnode* par, avlnode **tp)
{
  int comp;
  avlnode *t = *tp;
  
 
  if (_already_found)
    comp = (rthread(t) ? 0 : 1);
  else if (_target_item < t->citem)
    comp = -1;
  else if (_target_item > t->citem)
    comp = 1;
  else
    comp = 0;

  if (comp == 0)
    {
      if (lthread(t) && rthread(t))
        {
          _found_node = t;
          if (t == par->lt)
            {
              set_lthread(par, 1);
              par->lt = t->lt;
            }
          else
            {
              set_rthread(par, 1);
              par->rt = t->rt;
            }
          _need_rebalancing = 1;
          return;
        }
      else if (lthread(t))
        {
          avlnode* s;
          _found_node = t;
          s = avl_succ(t);
          if (s != 0 && lthread(s))
            s->lt = t->lt;
          *tp = t = t->rt;
          _need_rebalancing = 1;
          return;
        }
      else if (rthread(t))
        {
          avlnode *p;
          _found_node = t;
          p = avl_pred(t);
          if (p != 0 && rthread(p))
            p->rt = t->rt;
          *tp = t = t->lt;
          _need_rebalancing = 1;
          return;
        }
      else                      /* replace item & find someone deletable */
        {
          avlnode* p = avl_pred(t);
          _avlnode_copy(t, p);
          _already_found = 1;
          comp = -1;            /* fall through below to left */
        }
    }

  if (comp < 0)
    {
      if (lthread(t))
        return;
      _avl_del(t, &(t->lt));
      if (!_need_rebalancing)
        return;
      switch (bf(t))
        {
        case AVLLEFTHEAVY:
          set_bf(t, AVLBALANCED);
          return;
        case AVLBALANCED:
          set_bf(t, AVLRIGHTHEAVY);
          _need_rebalancing = 0;
          return;
        case AVLRIGHTHEAVY:
          {
            avlnode* r = t->rt;
            switch (bf(r))
              {
              case AVLBALANCED:
                if (lthread(r))
                  t->rt = r;
                else
                  t->rt = r->lt;
                set_rthread(t, lthread(r));
                r->lt = t;
                set_lthread(r, 0);
                set_bf(t, AVLRIGHTHEAVY);
                set_bf(r, AVLLEFTHEAVY);
                _need_rebalancing = 0;
                *tp = t = r;
                return;
              case AVLRIGHTHEAVY:
                if (lthread(r))
                  t->rt = r;
                else
                  t->rt = r->lt;
                set_rthread(t, lthread(r));
                r->lt = t;
                set_lthread(r, 0);
                set_bf(t, AVLBALANCED);
                set_bf(r, AVLBALANCED);
                *tp = t = r;
                return;
              case AVLLEFTHEAVY:
                {
                  avlnode* l = r->lt;
                  set_lthread(r, rthread(l));
                  if (rthread(l))
                    r->lt = l;
                  else
                    r->lt = l->rt;
                  l->rt = r;
                  set_rthread(l, 0);
                  set_rthread(t, lthread(l));
                  if (lthread(l))
                    t->rt = l;
                  else
                    t->rt = l->lt;
                  l->lt = t;
                  set_lthread(l, 0);
                  if (bf(l) == AVLRIGHTHEAVY)
                    set_bf(t, AVLLEFTHEAVY);
                  else
                    set_bf(t, AVLBALANCED);
                  if (bf(l) == AVLLEFTHEAVY)
                    set_bf(r, AVLRIGHTHEAVY);
                  else
                    set_bf(r, AVLBALANCED);
                  set_bf(l, AVLBALANCED);
                  *tp = t = l;
                  return;
                }
              }
          }
        }
    }
  else
    {
      if (rthread(t))
        return;
      _avl_del(t, &(t->rt));
      if (!_need_rebalancing)
        return;
      switch (bf(t))
        {
        case AVLRIGHTHEAVY:
          set_bf(t, AVLBALANCED);
          return;
        case AVLBALANCED:
          set_bf(t, AVLLEFTHEAVY);
          _need_rebalancing = 0;
          return;
        case AVLLEFTHEAVY:
          {
            avlnode* l = t->lt;
            switch (bf(l))
              {
              case AVLBALANCED:
                if (rthread(l))
                  t->lt = l;
                else
                  t->lt = l->rt;
                set_lthread(t, rthread(l));
                l->rt = t;
                set_rthread(l, 0);
                set_bf(t, AVLLEFTHEAVY);
                set_bf(l, AVLRIGHTHEAVY);
                _need_rebalancing = 0;
                *tp = t = l;
                return;
              case AVLLEFTHEAVY:
                if (rthread(l))
                  t->lt = l;
                else
                  t->lt = l->rt;
                set_lthread(t, rthread(l));
                l->rt = t;
                set_rthread(l, 0);
                set_bf(t, AVLBALANCED);
                set_bf(l, AVLBALANCED);
                *tp = t = l;
                return;
              case AVLRIGHTHEAVY:
                {
                  avlnode* r = l->rt;
                  set_rthread(l, lthread(r));
                  if (lthread(r))
                    l->rt = r;
                  else
                    l->rt = r->lt;
                  r->lt = l;
                  set_lthread(r, 0);
                  set_lthread(t, rthread(r));
                  if (rthread(r))
                    t->lt = r;
                  else
                    t->lt = r->rt;
                  r->rt = t;
                  set_rthread(r, 0);
                  if (bf(r) == AVLLEFTHEAVY)
                    set_bf(t, AVLRIGHTHEAVY);
                  else
                    set_bf(t, AVLBALANCED);
                  if (bf(r) == AVLRIGHTHEAVY)
                    set_bf(l, AVLLEFTHEAVY);
                  else
                    set_bf(l, AVLBALANCED);
                  set_bf(r, AVLBALANCED);
                  *tp = t = r;
                  return;
                }
              }
          }
        }
    }
}


int
avl_del(void *key)
{
  _need_rebalancing = 0;
  _already_found = 0;
  _found_node = 0;
  _target_item = key;
    
  if (root)
    _avl_del(root, &root);
  if (!_found_node)
    return 0;
  avlchain_set(_found_node, 0);
  deallocate(&avlnode_alloc, (struct empty_alloc *) _found_node);
  if (--nobjects == 0)
    root = 0;
  return 1;
}



/* -----------------------------------------
   DEBUG ROUTINES
   ----------------------------------------- */


static void
avl_pnode(avlnode *n)
{
  if (n->belong == BELONG_C)
    sprintf(string_buffer,"C %#8x ", (unsigned int)(n->citem));
  else if (n->belong == BELONG_LISP)
    sprintf(string_buffer,"L %#8x ", (unsigned int)(n->citem));
  else
    sprintf(string_buffer,"? %#8x ", (unsigned int)(n->citem));
  print_string(string_buffer);

  if (n->cinfo == CINFO_IDX)
    sprintf(string_buffer,"%-16s ","idx");
  else if (n->cinfo == CINFO_SRG)
    sprintf(string_buffer,"%-16s ","srg");
  else if (n->cinfo == CINFO_STR)
    sprintf(string_buffer,"%-16s ","str");
  else if (n->cinfo == CINFO_LIST)
    sprintf(string_buffer,"%-16s ","list");
  else if (n->cinfo == CINFO_OBJ)
    sprintf(string_buffer,"obj:%-12s ",
            ((dhclassdoc_t*)(n->cmoreinfo))->lispdata.lname);
  else if (n->cinfo == CINFO_UNLINKED)
    sprintf(string_buffer,"%-16s ","unlinked");
  else	
    sprintf(string_buffer,"%-16s ","[:-(]");
  print_string(string_buffer);

  if (n->litem==0)
    sprintf(string_buffer,"~\n");
  else if (n->litem->count > 0)
    sprintf(string_buffer,"%s\n",first_line(n->litem));
  else
    sprintf(string_buffer,"[:-(]\n");
  print_string(string_buffer);

}

static void
avl_display(avlnode *n, int tab)
{
  int i;
  if (!n->avl_lthread)
    avl_display(n->lt,tab+1); 
  i=0;
  string_buffer[0] = 0;
  while (i++ < tab)
    strcat(string_buffer," ");
  if (n->avl_bf == AVLLEFTHEAVY)
    strcat(string_buffer,"<");
  else if (n->avl_bf == AVLLEFTHEAVY)
    strcat(string_buffer,">");
  else
    strcat(string_buffer,"=");
  while (i++ < 18)
    strcat(string_buffer," ");
  print_string(string_buffer);
  avl_pnode(n); 
  if (!n->avl_rthread)
    avl_display(n->rt,tab+1); 
}

at *
lisp_c_map(void *p)
{
  if (p==(void*)0)
    {
      if (root)
        avl_display(root,0); 
      return NEW_NUMBER(nobjects);
    }
  else if (p==(void*)1)
    {
      avlnode *n;
      int no = 0;
      for (n = dummy_upds.chnxt; n != &dummy_upds; n=n->chnxt, no++)
        avl_pnode(n);
      return NEW_NUMBER(no);
    }
  else if (p==(void*)2)
    {
      avlnode *n;
      int no = 0;
      for (n = dummy_tmps.chnxt; n != &dummy_tmps; n=n->chnxt, no++)
        avl_pnode(n);
      return NEW_NUMBER(no);
    }
  else
    {
      avlnode *n = avl_find(p);
      if (n) {
        avl_pnode(n);
        LOCK(n->litem);
        return n->litem;
      }
      return NIL;
    }
}

DX(xlisp_c_map)
{
  at *p = 0;
  void *cptr = 0;
  if (arg_number == 0)
    return NEW_NUMBER(nobjects);
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p==0)
    return lisp_c_map(0);
  else if (p->flags & C_NUMBER)
    return lisp_c_map((void*)(unsigned long)(p->Number));
  else if (p->flags & X_OOSTRUCT)
    cptr = ((struct oostruct *)(p->Object))->cptr;
  else if (p->flags & X_INDEX)
    cptr = ((struct index *)(p->Object))->cptr;
  else if (storagep(p))
    cptr = ((struct storage *)(p->Object))->cptr;
  else if (p->flags & C_GPTR)
    cptr = p->Gptr;
  if (cptr) {
    avlnode *n = avl_find(cptr);
    if (n) {
      avl_pnode(n); 
      LOCK(p);
      return p;
    }
  }
  return NIL;
}


/* -----------------------------------------------------
   ROUTINES MANAGING THE AVL TREE FROM INTERPRETER CODE
   ----------------------------------------------------- */


/* alloc_srg -- allocate space for a SRG */

static avlnode *
alloc_srg(int type)
{
  /* A Lisp owned SRG associated to a Lisp object directly points into the
   * data area of the Lisp object.
     *
     * A Lisp owned SRG not associated to a Lisp storage (e.g. a temporary)
     * owns its data block. It must be freed when the object is freed or
     * disowned when we associate a lisp storage (make_lisp_from_c)
     *
     * For your information, a C owned SRG manages completely its data
     * block. These SRG are not allocated with this function and are not freed
     * by any function in this file. The data must be copied back and forth when 
     * synchronizing the L and C sides.
     */
    
  avlnode *n;
  struct srg *cptr = malloc(sizeof(struct srg));
  if (!cptr) 
    error(NIL,"Out of memory",NIL);
  cptr->size = 0;
  cptr->type = type;
  cptr->data = 0;
  cptr->flags = STS_MALLOC;
  n = avl_add(cptr);
  if (n==0)
    error(NIL,"lisp_c internal: cannot add element to item map",NIL);
  n->cinfo = CINFO_SRG;
  n->belong = BELONG_LISP;
  return n;
}


/* alloc_idx -- allocate space for an IDX */

static avlnode *
alloc_idx(int ndim)
{
  /* We allocate the IDX structure and the DIM and MOD arrays
   * in a single memory block.
     */
  avlnode *n;
  struct idx *cptr = malloc(sizeof(struct idx) + 2*ndim*sizeof(int) );
  if (!cptr) 
    error(NIL,"Out of memory",NIL);
  cptr->ndim = ndim;
  cptr->dim = (int*)(cptr+1);
  cptr->mod = cptr->dim + ndim;
  n = avl_add(cptr);
  if (n==0)
    error(NIL,"lisp_c internal: cannot add element to item map",NIL);
  n->cinfo = CINFO_IDX;
  n->belong = BELONG_LISP;
  return n;
}

/* alloc_obj -- allocate an object */

static avlnode *
alloc_obj(dhclassdoc_t *classdoc)
{
  /* Object allocation is plain vanilla :-)
   */
  avlnode *n;
  void *cptr;
  cptr = malloc(classdoc->lispdata.size);
  if (!cptr) 
    error(NIL,"Out of memory",NIL);
  memset(cptr,0,classdoc->lispdata.size);
  *((void**)cptr) = (void*)(classdoc->lispdata.vtable);
  n = avl_add(cptr);
  if (n==0)
    error(NIL,"lisp_c internal: cannot add element to item map",NIL);
  n->belong = BELONG_LISP;
  n->cinfo = CINFO_OBJ;
  n->cmoreinfo = classdoc;
  return n;
}


/* alloc_str -- allocate a string */

static avlnode *
alloc_str(void)
{
  /* STR contain a freshly allocated copy of the string. 
   * Space for this copy is allocated using srg_resize. 
   * We must free it with srg_free. 
   */
  avlnode *n = alloc_srg(ST_U8);
  n->cinfo = CINFO_STR;
  return n;
}


/* alloc_list -- allocate a list */

static avlnode *
alloc_list(int ndim)
{
  /* LIST size is known from the LIST type.
   * We allocate everything in a single memory block 
   * We should not call srg_free to destroy it. 
     */
  avlnode *n;
  int i = (sizeof(struct srg)+sizeof(dharg)-1)/sizeof(dharg);
  struct srg *srg = malloc((ndim+i)*sizeof(dharg));
  if (!srg) 
    error(NIL,"Out of memory",NIL);
  srg->type = ST_D;
  srg->data = ((dharg*)(srg)) + i;
  srg->size = ndim;
  srg->flags = 0;
  n = avl_add(srg);
  if (n==0)
    error(NIL,"lisp_c internal: cannot add element to item map",NIL);
  n->belong = BELONG_LISP;
  n->cinfo = CINFO_LIST;
  n->cmoreinfo = 0;  /* need to be updated later */
  return n;
}




/* lside_create_srg -- call this for creating an SRG from interpreted code */

static avlnode *
lside_create_srg(at *p)
{
  avlnode *n;
  struct srg *cptr;
  struct storage *st;

  /* check */
  ifn (p && storagep(p))
    error(NIL,"storage expected",p);
  st = p->Object;
  if (st->cptr)
    {
      n = avl_find(st->cptr);
      if (n==0)
        error(NIL,"lisp_c internal: cannot find srg",p);
    }
  else
    {
      /* allocate */
      n = alloc_srg(st->srg.type);
      cptr = n->citem;
      n->litem = p;
      st->cptr = cptr;
      avlchain_set(n, &dummy_upds);
      /* special marker for read-only storages */
      if (st->srg.flags & STF_RDONLY)
        n->cmoreinfo = st;
      /* update c object */
      update_c_from_lisp(n);
    }
  /* c object must be updated */
  return n;
}


/* lside_create_idx -- call this for creating an IDX from interpreted code */

static avlnode *
lside_create_idx(at *p)
{
  avlnode *n;
  struct idx *cptr;
  struct index *ind;
    
  /* check */
  ifn (p && (p->flags & X_INDEX))
    error(NIL,"Not an index",p);
  ind = p->Object;
  if (ind->cptr)
    {
      n = avl_find(ind->cptr);
      if (n==0)
        error(NIL,"lisp_c internal: cannot find idx",p);
    }
  else 
    {
      /* allocate */
      n = alloc_idx(ind->ndim);
      cptr = n->citem;
      n->litem = p;
      ind->cptr = cptr;
      avlchain_set(n, &dummy_upds);
      /* update c object */
      update_c_from_lisp(n);
    }
  return n;
}


/* lside_create_obj -- call this for creating an object from interpreted code */

static avlnode *
lside_create_obj(at *p)
{
  avlnode *n;
  struct oostruct *object;
  class *objcl;
  dhclassdoc_t *classdoc;
  void *cptr;

  /* check type */
  ifn (p && (p->flags & X_OOSTRUCT))
    error(NIL,"Object expected",p);
  object = p->Object;
  if (object && object->cptr)
    {
      n = avl_find(object->cptr);
      if (n==0)
        error(NIL,"lisp_c internal: cannot find object",p);
      if (n->cinfo == CINFO_UNLINKED)
        error(NIL,"found object with unlinked class", p);
    } 
  else
    {
      /* get compiled class */
      objcl = p->Class;
      while (objcl && !objcl->classdoc)
        objcl = objcl->super;
      ifn (objcl)
        error(NIL,"This is not an instance of a compiled class",p);        
      /* get and check classdoc */
      if (CONSP(objcl->priminame))
        check_primitive(objcl->priminame);
      classdoc = objcl->classdoc;
      /* allocate object */
      n = alloc_obj(classdoc);
      cptr = n->citem;
      if (object) 
        {
          n->litem = p;
          object->cptr = cptr;
          avlchain_set(n, &dummy_upds);
          /* update c object */
          update_c_from_lisp(n);
        }
      else
        {
          avlchain_set(n, &dummy_tmps);
        }
    }
  /* c object must be updated */
  return n;
}


/* lside_create_str -- creates a string */

static avlnode *
lside_create_str(at *p)
{
  avlnode *n;
  struct string *str;
  struct srg *srg;
    
  ifn (EXTERNP(p, &string_class))
    error(NIL,"String expected",p);
  
  str = p->Object;
  if (str->cptr)
    {
      n = avl_find(str->cptr);
      if (n==0)
        error(NIL,"lisp_c internal: cannot find string",p);
      return n;
    }
  else
    {
      /* create storage pointing to the string */
      n = alloc_str();
      srg = n->citem;
      n->litem = p;
      str->cptr = srg;
      avlchain_set(n, &dummy_upds);
      /* update string (never changed after this) */
      srg = n->citem;
      srg_resize(srg, 1+strlen(str->start),__FILE__,__LINE__);
      strcpy(srg->data, str->start);
      /* return */
      return n;
    }
}





/* lside_check_ownership -- check that object belongs to lisp */

int
lside_check_ownership(void *cptr)
{
  avlnode *n;
  n = avl_find(cptr);
  return (cptr==0) || (n==0) || (n->belong==BELONG_LISP);
}


/* lside_destroy_item -- called when a mirrored lisp object is purged */

void 
lside_destroy_item(void *cptr)
{
  if (cptr)
    {
      avlnode *n = avl_find(cptr);        
      if (n)
        {
          avlchain_set(n, 0);
          switch (n->belong)
            {
            case BELONG_C:
              /* The lisp counterpart of this C object is no longer there.
               * Nevertheless the object is still there until C calls
               * 'cside-destroy-item'. We must update it using lisp information.
               */
              update_c_from_lisp(n);
              n->litem = 0;
              return;
            case BELONG_LISP:
              /* The lisp counterpart of this C object is being destroyed.
               * We must destroy the object. We do not need to destroy
               * the data blocks associated to this object (e.g. storage data)
               * because this data block was owned by the lisp object...
               */
              free(n->citem);
              avl_del(cptr);
              return;
            }
        }
      error(NIL,"lisp_c internal : corrupted data structure",NIL);
    }
}



/* lside_dld_partial -- called from DLD.C when objects become non executable */

int
lside_mark_unlinked(void *cdoc)
{
  avlnode *n;
  int count = 0;
  n = avl_first(0);
  while (n)
    {
      if (n->cmoreinfo == cdoc)
        if (n->cinfo == CINFO_OBJ)
          {
            n->cinfo = CINFO_UNLINKED;
            n->cmoreinfo = 0;
            count += 1;
          }
      n = avl_succ(n);
    }
  return count;
}



/* -------------------------------------------------
   ROUTINES MANAGING THE AVL TREE FROM COMPILED CODE
   ------------------------------------------------- */


/* cside_create_idx -- call this when creating an IDX from C code */

void 
cside_create_idx(void *cptr)
{
  if (!dont_track_cside)
    {
      avlnode *n = avl_add(cptr);
      if (n != 0 ) {
        n->cinfo = CINFO_IDX;
        n->belong = BELONG_C;
      } else
        error(NIL,"lisp_c internal : idx created twice",NIL);
    }
}

/* cside_create_srg -- call this when creating an SRG from C code */

void 
cside_create_srg(void *cptr)
{
  if (!dont_track_cside)
    {
      avlnode *n = avl_add(cptr);
      if (n != 0 ) {
        n->cinfo = CINFO_SRG;
        n->belong = BELONG_C;
      } else
        error(NIL,"lisp_c internal : srg created twice",NIL);
    }
}

/* cside_create_obj -- call this when creating an object from C code */

void 
cside_create_obj(void *cptr, dhclassdoc_t *kname)
{
  if (!dont_track_cside)
    {
      avlnode *n = avl_add(cptr);
      if (n != 0 ) {
        n->cinfo = CINFO_OBJ;
        n->cmoreinfo = kname;
        n->belong = BELONG_C;
      } else
        error(NIL,"lisp_c internal : obj created twice",NIL);
    }
}

/* cside_create_str -- call this when creating an STR from C code */

void 
cside_create_str(void *cptr)
{
  if (!dont_track_cside)
    {
      avlnode *n = avl_add(cptr);
      if (n != 0 ) {
        n->cinfo = CINFO_STR;
        n->belong = BELONG_C;
      } else
        error(NIL,"lisp_c internal : str created twice",NIL);
    }
}


/* transmute_object_into_gptr -- 
   trick a lisp object into a gptr */

static void
transmute_object_into_gptr(at *p, void *px)
{
  /* This is bad practice */
  if (p && p->count>0 && (p->flags & C_EXTERN))
    {
      /* clean object up */
      (*p->Class->self_dispose)(p);
      /* disguise it as a gptr */
      p->flags &= ~(C_CONS|C_EXTERN|C_NUMBER);
      p->flags |= C_GPTR;
      p->Gptr = px;
    }
}


/* cside_destroy_node -- destroys an avlnode */

static void
cside_destroy_node(avlnode *n)
{
  /* There is no need to free this object.
   * This is the reponsibility of the compiled code who effectively
   * owns this object and told us that the object was being deleted.
     */
  if (n->belong != BELONG_C) 
    return;
  /* delete lisp object as well */
  if (n->litem)
    {
      avlchain_set(n, 0);
      switch (n->cinfo)
        {
        case CINFO_SRG:
          ((struct storage *)(n->litem->Object))->cptr = 0;
          /* Do not transmute SRG (they are trusted by indexes) */
          update_lisp_from_c(n);
          break;
        case CINFO_OBJ:
          ((struct oostruct*)(n->litem->Object))->cptr = 0;
          transmute_object_into_gptr(n->litem, n->citem);
          break;
        case CINFO_IDX:
          ((struct index *)(n->litem->Object))->cptr = 0;
          transmute_object_into_gptr(n->litem, n->citem);
          break;
        case CINFO_STR:
          ((struct string *)(n->litem->Object))->cptr = 0;
          transmute_object_into_gptr(n->litem, n->citem);
          break;
        }
    }
  /* delete avl map entry */
  avl_del(n->citem);
}


/* cside_destroy_item -- call this before destroying an item from C code */

void 
cside_destroy_item(void *cptr)
{
  avlnode *n;
  if ((n = avl_find(cptr)))
    cside_destroy_node(n);
}


/* cside_destroy_range -- call this before destroying several items from C code */

void 
cside_destroy_range(void *from, void *to)
{
  avlnode *n, *m;
  n = avl_first(from);
  while (n && n->citem < to)
    {
      m = avl_succ(n);
      if (n->belong==BELONG_C) 
        cside_destroy_node(n);
      n = m;
    }
}


/* lisp-c-dont-track-cside -- stopstracking cside item (like old SN) */

DX(xlisp_c_dont_track_cside)
{
  int nitem = 0;
  avlnode *n, *m;

  ARG_NUMBER(0);
  dont_track_cside = 1;
  n = avl_first(0);
  while (n)
    {
      m = avl_succ(n);
      if (n->belong==BELONG_C) {
        cside_destroy_node(n);
        nitem ++;
      }
      n = m;
    }
  return NEW_NUMBER(nitem);
}





/* -----------------------------------------
   THE KILL LIST
   ----------------------------------------- */

/*
 * When we udpate the lisp objects with information stored in the C code, we
 * must replace the contents of the object slots with the new content. In the
 * process, we must reduce the counter of the old value. This may call a
 * destructor and subsequently call a DH. This a problem because DH_LISTEVAL
 * is not a nice reentrant function.
 *
 * Instead of unlocking these objects we store them in a list by stealing
 * their lock. At the end of listeval, we just UNLOCK the list.
 */

static at *delayed_kill_list = 0;

#define DELAYED_UNLOCK(new,old) \
   if (new==old) { UNLOCK(old); } \
   else { delayed_kill_list = cons(old,delayed_kill_list); }




/* -----------------------------------------
   GET A LISP OBJECT FOR A C OBJECT
   ----------------------------------------- */


/* lisp2c_warning -- prints a warning message */

static void
lisp2c_warning(char *s, at *errctx)
{
  if (dont_warn)
    return;
  printf("*** lisp_c_warning %s\n", s);
  if (errctx) 
    printf("***    in object %s\n",pname(errctx));
}



/* make_lisp_from_c -- get a lisp object for a c object (no synchronization) */

static at *
make_lisp_from_c(avlnode *n, void *px)
{
  /* Complain if pointer is invalid */
  if (n==0)
    {
      lisp2c_warning("(out): Found dangling pointer",0);
      return NEW_GPTR((unsigned long)px);
    }
  if (n->cinfo == CINFO_UNLINKED)
    {
      lisp2c_warning("(out): Found pointer to unlinked object",0);
      return NEW_GPTR((unsigned long)px);
    }
  if (px != n->citem)
    {
      printf("*** lisp_c internal problem in 'make_lisp_from_c'");
    }
    
  /* Return existing object if any */
  if (n->litem)
    {
      /* object should be up to date */
      LOCK(n->litem);
      return n->litem;
    }
    
  /* Create lisp object when it does not exist */
  switch (n->cinfo)
    {
    case CINFO_IDX:
      {
        struct idx *idx = n->citem;
        avlnode *nst = avl_find(idx->srg);
        at *atst = make_lisp_from_c(nst, idx->srg);
        at *atind = new_index(atst);
        struct index *ind = atind->Object;
        UNLOCK(atst);
        ind->cptr = idx;
        n->litem = atind;
        avlchain_set(n, &dummy_upds);
        update_lisp_from_c(n);
        return atind;
      }
    case CINFO_SRG:
      {
        struct srg *srg = n->citem;
        at *atst = new_storage(srg->type, 0);
        struct storage *st = atst->Object;
        if (n->belong == BELONG_LISP)
          {
            /* If this object belong to LISP and was not previously
             * associated to a lisp storage, we must transfer the ownership
             * of the data block of the C object to the Lisp storage.
             */
            st->srg.data = srg->data;
            st->srg.size = srg->size;
            st->srg.flags = srg->flags;
          }
        else
          {
            /* We patch the flag STS_MALLOC in order to allow
             * function srg_resize (update_lisp_from_c) to work.
             */
            if (st->srg.data || st->srg.size)
              error(NIL,"lisp_c internal: new storage non zero size",atst);
            st->srg.flags = STS_MALLOC;
          }
        /* We can now update the avl tree */
        st->cptr = srg;
        n->litem = atst;
        avlchain_set(n, &dummy_upds);
        update_lisp_from_c(n);
        return atst;
      }
    case CINFO_OBJ:
      {
        at *cl;
        at *atobj;
        dhclassdoc_t *classdoc = n->cmoreinfo;
        /* Create object */
        if (! (cl = classdoc->lispdata.atclass))
          error(NIL,"lisp_c internal: "
                "classdoc appears to be uninitialized",NIL);
        atobj = new_oostruct(cl);
        /* Update avlnode */
        n->litem = atobj;
        ((struct oostruct*)(atobj->Object))->cptr = n->citem;
        avlchain_set(n, &dummy_upds);
        /* Update object */
        update_lisp_from_c(n);
        return atobj;
      }
    case CINFO_STR:
      {
        at *p;
        struct srg *srg = n->citem;
        struct string *str;
        if (! srg->data )
          lisp2c_warning("(out): found uninitialized string",0);
        p = new_string(srg->data);
        str = p->Object;
        str->cptr = srg;
        n->litem = p;
        avlchain_set(n, &dummy_upds);
        return p;
      }
    case CINFO_LIST:
      {
        /* We do not update the LITEM field
         * for these partially supported objects.
         * They do not last between DH calls.
         */
        int i;
        at *p = NIL;
        at **where = &p;
        struct srg *srg = n->citem;
        dharg *arg = srg->data;
        dhrecord *drec = n->cmoreinfo;
        if (drec==0 || drec->op!=DHT_LIST)
          error(NIL,"lisp_c internal: "
                "untyped list made it to make-lisp-from-c",NIL);
        if (drec->ndim != srg->size)
          error(NIL,"lisp_c internal: "
                "list changed size",NIL);
        drec++;
        for (i=0; i<srg->size; i++)
          {
            *where = cons(dharg_to_at(arg,drec,NIL),NIL);
            arg += 1;
            drec = drec->end;
            where = &((*where)->Cdr);
          }
        return p;
      }
    default:
      error(NIL,"lisp_c internal: "
            "strange type in avl map",NIL);
    }
}





/* -----------------------------------------
   DHARG MANAGEMENT
   ----------------------------------------- */


/* dharg_to_address -- store dharg value at a specific location */

static void
dharg_to_address(dharg *arg, char *addr, dhrecord *drec)
{
  switch(drec->op) {
  case DHT_BYTE:
  case DHT_BOOL:
  case DHT_NIL:
    *((char *) addr) = arg->dh_char;
    break;
  case DHT_UBYTE:
    *((unsigned char *) addr) = arg->dh_uchar;
    break;
  case DHT_SHORT:
    *((short *) addr) = arg->dh_short;
    break;
  case DHT_INT:
    *((int *) addr) = arg->dh_int;
    break;
  case DHT_FLT:
    *((flt *) addr) = arg->dh_flt;
    break;
  case DHT_REAL:
    *((real *) addr) = arg->dh_real;
    break;
  case DHT_GPTR:
    *((gptr *) addr) = arg->dh_gptr;
    break;
  default:	/* Well, its probably a pointer */
    *((gptr *) addr) = arg->dh_gptr;
    break;
  }
}


/* address_to_dharg -- copy contents of specific location into dharg */

static void
address_to_dharg(dharg *arg, char *addr, dhrecord *drec)
{
  switch(drec->op) {
  case DHT_BYTE:
  case DHT_BOOL:
  case DHT_NIL:
    arg->dh_char = *((char *) addr);
    break;
  case DHT_UBYTE:
    arg->dh_uchar = *((unsigned char *) addr);
    break;
  case DHT_SHORT:
    arg->dh_short = *((short *) addr);
    break;
  case DHT_INT:
    arg->dh_int = *((int *) addr);
    break;
  case DHT_FLT:
    arg->dh_flt = *((flt *) addr);
    break;
  case DHT_REAL:
    arg->dh_real = *((real *) addr);
    break;
  default:
    arg->dh_gptr = *((gptr *) addr);
  }
}


/* lisp2c_error -- richer error message */

static void
lisp2c_error(char *s, at *errctx, at *p)
{
  char errmsg[512];
  if (errctx == 0)
    sprintf(errmsg,"(lisp_c) %c%s", 
            toupper(s[0]), s+1 );
  else
    sprintf(errmsg,"(lisp_c) %c%s\n***    in object %s", 
            toupper(s[0]), s+1, 
            first_line(errctx) );
  error(NIL,errmsg,p);
}



/* at_to_dharg -- fills dharg with the c equivalent of a lisp object */

static void
at_to_dharg(at *at_obj, dharg *arg, dhrecord *drec, at *errctx)
{
  avlnode *n;
  struct srg *srg;
  struct storage *st;
  struct index *ind;
  dhclassdoc_t *cdoc;
  dharg *larg;
  class *cl;
  at *p;
  int i;

  switch(drec->op) 
    {
    case DHT_FLT:
      if (!at_obj)
        arg->dh_flt = 0;
      else if (at_obj->flags & C_NUMBER)
        arg->dh_flt = (flt) at_obj->Number; 
      else
        lisp2c_error("FLT expected",errctx,at_obj);
      return;
    case DHT_REAL:
      if (!at_obj)
        arg->dh_real = 0;
      else if (at_obj->flags & C_NUMBER)
        arg->dh_real = (real) at_obj->Number; 
      else
        lisp2c_error("REAL expected",errctx,at_obj);
      return;
    case DHT_INT:
      if (!at_obj)
        arg->dh_int = 0;
      else if ((at_obj->flags & C_NUMBER) && 
               (at_obj->Number == (int)(at_obj->Number)) )
        arg->dh_int = (int) at_obj->Number; 
      else
        lisp2c_error("INT expected",errctx,at_obj);
      return;
    case DHT_SHORT:
      if(!at_obj)
        arg->dh_short = 0;
      else if ((at_obj->flags & C_NUMBER) &&
               (at_obj->Number == (short)(at_obj->Number)) )
        arg->dh_short = (short) at_obj->Number; 
      else
        lisp2c_error("SHORT expected",errctx,at_obj);
      return;
    case DHT_BYTE:
      if(!at_obj)
        arg->dh_char = 0;
      else if ((at_obj->flags & C_NUMBER) &&
               (at_obj->Number == (char)(at_obj->Number)) )
        arg->dh_char = (char) at_obj->Number; 
      else
        lisp2c_error("BYTE expected",errctx,at_obj);
      return;
    case DHT_UBYTE:
      if(!at_obj)
        arg->dh_uchar = 0;
      else if ((at_obj->flags & C_NUMBER) &&
               (at_obj->Number == (unsigned char)(at_obj->Number)) )
        arg->dh_uchar = (unsigned char) at_obj->Number; 
      else
        lisp2c_error("UBYTE expected",errctx,at_obj);
      return;
    case DHT_GPTR:
      if (at_obj == 0)
        arg->dh_gptr = (gptr) 0;
      else if (at_obj->flags & C_GPTR)
        arg->dh_gptr = (gptr) at_obj->Gptr;             
      else
        lisp2c_error("GPTR expected",errctx,at_obj);
      return;
    case DHT_NIL:
      if (at_obj == 0)
        arg->dh_char = (char) 0;
      else
        lisp2c_error("NIL expected",errctx,at_obj);
      return;
    case DHT_BOOL:
      if (at_obj == 0)
        arg->dh_char = 0;
      else if (at_obj == at_true)
        arg->dh_char = 1;
      else
        lisp2c_error("BOOL expected",errctx,at_obj);
      return;
    case DHT_SRG:
      if (at_obj == 0)
        {
          /* happens in constructors */
          arg->dh_srg_ptr = 0;
          return;
        }
      if (GPTRP(at_obj))
        {
          if (!dont_track_cside) 
            lisp2c_warning("(in): found GPTR instead of SRG", errctx);
          arg->dh_srg_ptr = at_obj->Gptr;
          return;
        }
      if (!storagep(at_obj))
        lisp2c_error("STORAGE expected",errctx,at_obj);
      /* check type and access */
      st = at_obj->Object;
      if (storage_to_dht[st->srg.type] != (drec+1)->op)
        lisp2c_error("STORAGE has illegal type",errctx,at_obj);
      if ((st->srg.flags & STF_RDONLY) && 
          (drec->access == DHT_WRITE) )
        lisp2c_error("STORAGE is read only",errctx,at_obj);                
      /* create object */
      n = lside_create_srg(at_obj);
      (arg->dh_srg_ptr) = (struct srg *)(n->citem);
      return;
    case DHT_IDX:
      if (at_obj == 0)
        {
          /* happens in constructors */
          arg->dh_idx_ptr = 0;
          return;
        }
      if (GPTRP(at_obj))
        {
          if (!dont_track_cside) 
            lisp2c_warning("(in): found GPTR instead of IDX", errctx);
          arg->dh_idx_ptr = at_obj->Gptr;
          return;
        }
      if(! EXTERNP(at_obj, &index_class))
        lisp2c_error("IDX expected",errctx,at_obj);
      /* check type and access */
      ind = at_obj->Object;
      if (ind->ndim != drec->ndim)
        lisp2c_error("INDEX has wrong number of dimensions",
                     errctx, at_obj);
      if (storage_to_dht[ind->st->srg.type] != (drec+2)->op)
        lisp2c_error("INDEX is based on a STORAGE with illegal type",
                     errctx, at_obj);
      if ((ind->st->srg.flags & STF_RDONLY) && 
          (drec->access == DHT_WRITE))
        lisp2c_error("INDEX is read only", errctx, at_obj);            
      /* create object */
      n = lside_create_idx(at_obj);
      (arg->dh_idx_ptr) = (struct idx *)(n->citem);
      return;
        
    case DHT_OBJ:
      if (at_obj == 0)
        {
          /* happens in constructors */
          arg->dh_obj_ptr = 0;
          return;
        }
      if (at_obj->flags & C_GPTR)
        {
          if (!dont_track_cside)
            lisp2c_warning("(in): found GPTR instead of OBJ", errctx);
          arg->dh_obj_ptr = at_obj->Gptr;
          return;
        }
      if (at_obj->flags & X_OOSTRUCT)
        lisp2c_error("OBJECT expected", errctx, at_obj);
      /* check type */
      for (cl = at_obj->Class; cl; cl=cl->super)
        if ((cdoc = cl->classdoc))
          if (cdoc == (dhclassdoc_t*)(drec->arg))
            break;
      if (cl==NULL)
        lisp2c_error("OBJECT has illegal type",errctx,at_obj);
      /* create object */
      n = lside_create_obj(at_obj);
      (arg->dh_obj_ptr) = n->citem;
      return;
        
    case DHT_STR:
      if (at_obj == 0)
        {
          /* happens in constructors */
          arg->dh_srg_ptr = 0;
          return;
        }
      if (GPTRP(at_obj))
        {
          if (!dont_track_cside)
            lisp2c_warning("(in): found GPTR instead of STR", errctx);
          arg->dh_srg_ptr = at_obj->Gptr;
          return;
        }
      if (! EXTERNP(at_obj, &string_class))
        lisp2c_error("STRING expected",errctx,at_obj);
      n = lside_create_str(at_obj);
      (arg->dh_srg_ptr) = n->citem;
      return;

    case DHT_LIST:
      /* Create and *initialize* the SRG:
       * UPDATE_C_FROM_LISP does nothing. 
       * This is an exception to the general rule
       * because STRs and compiled LISTs are immutable.
       */
      if (at_obj == 0)
        {
          /* happens in constructors */
          arg->dh_srg_ptr = 0;
          return;
        }
      if (at_obj->flags & C_GPTR)
        {
          if (!dont_track_cside) 
            lisp2c_warning("(in): found GPTR instead of LIST", errctx);
          arg->dh_srg_ptr = at_obj->Gptr;
          return;
        }
      if (! (at_obj->flags & C_CONS))
        lisp2c_error("LIST expected", errctx, at_obj);
      if(length(at_obj) != drec->ndim)
        lisp2c_error("LIST length do not match",errctx,at_obj);
      /* create D storage for the list */
      n = alloc_list(drec->ndim);
      n->cmoreinfo = drec;
      srg = n->citem;
      avlchain_set(n, &dummy_tmps);
      /* fill the list elements */
      i = drec->ndim;
      larg = srg->data;
      p = at_obj;
      drec++;
      while (--i >= 0) {
        at_to_dharg(p->Car, larg, drec, at_obj);
        drec = drec->end;
        larg += 1;
        p = p->Cdr;
      }
      /* return */
      (arg->dh_srg_ptr) = srg;
      return;
	        
    default:
      lisp2c_error("Unknown DHDOC type",errctx,at_obj);
    }
}





/* dharg_to_at -- builds at object from dharg */

static at*
dharg_to_at(dharg *arg, dhrecord *drec, at *errctx)
{
  avlnode *n;

  switch(drec->op) {

  case DHT_NIL:
    return NIL;
  case DHT_INT:
    return NEW_NUMBER(arg->dh_int);
  case DHT_BYTE:
    return NEW_NUMBER(arg->dh_char);
  case DHT_UBYTE:
    return NEW_NUMBER(arg->dh_uchar);
  case DHT_SHORT:
    return NEW_NUMBER(arg->dh_short);
  case DHT_BOOL:
    (arg->dh_char==0) ? NIL : true();
    return (arg->dh_char==0) ? NIL : true();
  case DHT_REAL:
    return NEW_NUMBER(arg->dh_real);
  case DHT_FLT:
    return NEW_NUMBER(arg->dh_flt);

  case DHT_GPTR:
    if (!arg->dh_gptr)
      return NIL;
    return NEW_GPTR(arg->dh_gptr);
        
  case DHT_SRG:
    if (arg->dh_srg_ptr==0) 
      return NIL;
    n = avl_find(arg->dh_srg_ptr);
    if (n)
      return make_lisp_from_c(n,arg->dh_srg_ptr);
    if (!dont_track_cside)
      lisp2c_warning("(out): Dangling pointer instead of SRG", errctx);
    return NEW_GPTR(arg->dh_srg_ptr);
        
  case DHT_IDX:
    if (arg->dh_idx_ptr==0) 
      return NIL;
    n = avl_find(arg->dh_idx_ptr);
    if (n)
      return make_lisp_from_c(n,arg->dh_idx_ptr);
    if (!dont_track_cside) 
      lisp2c_warning("(out): Dangling pointer instead of IDX", errctx);
    return NEW_GPTR(arg->dh_idx_ptr);

  case DHT_OBJ:
    if (arg->dh_obj_ptr==0) 
      return NIL;
    n = avl_find(arg->dh_obj_ptr);
    if (n)
      return make_lisp_from_c(n,arg->dh_obj_ptr);
    if (!dont_track_cside)
      lisp2c_warning("(out): Dangling pointer instead of OBJ", errctx);
    return NEW_GPTR(arg->dh_obj_ptr);
        
  case DHT_STR:
    if (arg->dh_srg_ptr==0) 
      return NIL;
    n = avl_find(arg->dh_srg_ptr);
    if (n)
      return make_lisp_from_c(n,arg->dh_srg_ptr);
    if (!dont_track_cside) 
      lisp2c_warning("(out): Dangling pointer instead of STR", errctx);
    return NEW_GPTR(arg->dh_srg_ptr);
        
  case DHT_LIST:
    if (arg->dh_srg_ptr==0)
      return NIL;
    n = avl_find(arg->dh_srg_ptr);
    if (n && n->cmoreinfo==0)
      n->cmoreinfo = drec;
    if (n)
      return make_lisp_from_c(n,arg->dh_srg_ptr);
    if (!dont_track_cside)
      lisp2c_warning("(out): Dangling pointer instead of LIST", errctx);
    return NEW_GPTR(arg->dh_srg_ptr);
        
  default:
    error(NIL,"lisp_c internal: unknown op in dhrecord",NIL);
  }
}


/* -----------------------------------------
   SYNCHRONIZE LISP AND C SIDES
   ----------------------------------------- */


/* update_c_from_lisp -- copy lisp to c for a specific entry of the map */

static void
update_c_from_lisp(avlnode *n)
{
  n->need_update = 0;
  if (n->litem == 0)
    {
      /* nothing to synchronize */
      return;
    }
  switch (n->cinfo)
    {

    case CINFO_SRG:
      {
        struct srg *cptr = n->citem;
        struct storage *st = n->litem->Object;
            
        /* Possibly broken code */        
        if (n->cmoreinfo)
          storage_read_srg(st);
        else
          storage_write_srg(st); 
            
        /* Synchronize compiled object */
        if ((n->belong == BELONG_C) && 
            (cptr->flags & STS_MALLOC) )
          {
            /* C allocated SRG manage their own data block */
            int bytes;
            ifn (st->srg.flags & STS_MALLOC)
              error(NIL,"lisp_c internal: expected ram storage",n->litem);
            if (cptr->size < st->srg.size) 
              srg_resize(cptr,st->srg.size,__FILE__,__LINE__);
            bytes = cptr->size * storage_type_size[cptr->type];
            if (bytes>0 && cptr->data && st->srg.data)
              memcpy(cptr->data, st->srg.data, bytes);
          }
        else
          {
            /* Other SRG point to the same data area */
            cptr->flags = st->srg.flags;
            cptr->type = st->srg.type;
            cptr->size = st->srg.size;
            cptr->data = st->srg.data;
          } 
        break;
      }

    case CINFO_IDX:
      {
        int i;
        struct idx *idx = n->citem;
        struct index *ind = n->litem->Object;
        avlnode *nst;
        /* setup storage */
        nst = lside_create_srg(ind->atst);
        idx->srg = nst->citem;
        /* copy index structure:
         * we cannot use index_to_idx because it overwrites
         * the DIM and MOD pointers instead of copying the values
         */
        idx->ndim = ind->ndim;
        idx->offset = ind->offset;
        idx->flags = ind->flags;
        for (i=0; i<ind->ndim; i++)
          {
            idx->dim[i] = ind->dim[i];
            idx->mod[i] = ind->mod[i];
          }
        break;
      }

    case CINFO_UNLINKED:
      lisp2c_warning("(in): Found object with unlinked class",0);
      return;
        
    case CINFO_OBJ:
      {
        void *cptr = n->citem;
        struct oostruct *object = n->litem->Object;
        dhclassdoc_t *cdoc, *super, *class_list[1024];
        dhrecord *drec;
        dharg tmparg;
        int k,j,sl;
        
        if (object==0)
          /* happens during call to the destructor */
          return;
        cdoc = n->cmoreinfo;
        if (cdoc==0)
          error(NIL,"lisp_c internal: corrupted class information",NIL);
        k = sl = 0;
        super=cdoc;
        while (super) 
          {
            class_list[k++] = super;
            sl += super->argdata->ndim;
            super = super->lispdata.ksuper;
          }
        if (sl > object->size)
          error(NIL,"lisp_c internal: class slot mismatch",n->litem);
        sl = object->size - 1;
        while (--k>=0)
          {
            super=class_list[k];
            j = super->argdata->ndim;
            drec = super->argdata + 1;
            while (--j>=0)
              {
                /* quick check of slot name */
                char *pos;
                struct symbol *symb = object->slots[sl].symb->Object;
                if (symb->name->name[0] != drec->name[0])
                  error(NIL,"lisp_c internal : object slot mismatch",n->litem);
                /* copy field described by current record */
                at_to_dharg(object->slots[sl].val,&tmparg,drec+1,n->litem);
                pos = (char*)cptr + (unsigned int)(drec->arg);
                dharg_to_address(&tmparg, pos, drec+1);
                /* next record */
                drec = drec->end;
                sl -= 1;
              }
          }
        break;
      }
    case CINFO_STR:
      /* Strings are never modified by interpreted code */
    case CINFO_LIST:
      /* Partial support: lists do not last between DH calls */
    default:
      break;
    }
}



/* update_lisp_from_c -- copy c to lisp for a specific entry of the map */

static void
update_lisp_from_c(avlnode *n)
{
  n->need_update = 0;
  if (n->litem == 0)
    {
      /* nothing to synchronize */
      return;
    }
  switch (n->cinfo)
    {

    case CINFO_SRG:
      {
        struct srg *cptr = n->citem;
        struct storage *st = n->litem->Object;
        if ((n->belong == BELONG_C) && 
            (st->srg.flags & STS_MALLOC))
          {
            /* C allocated SRG manage their own data block */
            int bytes;
            if (! (st->srg.flags & STS_MALLOC))
              error(NIL,"lisp_c internal: expected ram storage",n->litem);
            if (st->srg.size < cptr->size)
              srg_resize(&st->srg,cptr->size,__FILE__,__LINE__);
            bytes = st->srg.size * storage_type_size[st->srg.type];
            if (bytes>0)
              memcpy(st->srg.data, cptr->data, bytes);
          }
        else
          {
            /* Other SRG share their data block */
            st->srg.flags = cptr->flags;
            st->srg.type = cptr->type;
            st->srg.size = cptr->size;
            st->srg.data = cptr->data;
          } 
        /* possibly broken code */ 
        storage_rls_srg(st); 
        break;
      }

    case CINFO_IDX:
      {
        int i;
        at *origst;
        avlnode *nst;
        struct idx *idx = n->citem;
        struct index *ind = n->litem->Object;
        /* copy index structure */
        ind->ndim = idx->ndim;
        ind->offset = idx->offset;
        ind->flags = idx->flags;
        for(i=0;i<idx->ndim;i++) 
          {
            ind->mod[i] = idx->mod[i];
            ind->dim[i] = idx->dim[i];
          }
        /* find the storage */
        nst = avl_find(idx->srg);
        if (nst==0) 
          {
            /* danger: storage has been deallocated! */
            lisp2c_warning("(out) : Found idx with dangling storage!",0);
            ind->atst = NULL;
            delete_at(n->litem);
            n->litem = 0;
            return;
          } 
        /* plug the storage into lisp object */
        origst = ind->atst;
        ind->atst = make_lisp_from_c(nst, idx->srg);
        ind->st = ind->atst->Object;
        DELAYED_UNLOCK(ind->atst, origst);
        break;
      }
        
    case CINFO_UNLINKED:
      lisp2c_warning("(out) : Found C object with unlinked class",0);
      break;
        
    case CINFO_OBJ:
      {
        void *cptr = n->citem;
        struct oostruct *object = n->litem->Object;
        dhclassdoc_t *cdoc, *super, *class_list[1024];
        dhrecord *drec;
        dharg tmparg;
        int k,j,sl;
        at *orig, *new;
            
        if (object==0)
          /* happens during call to the destructor */
          return;
        cdoc = n->cmoreinfo;
        if (cdoc==0)
          error(NIL,"lisp_c internal: corrupted class information",NIL);
            
        k = sl = 0;
        super = cdoc;
        while (super) 
          {
            class_list[k++] = super;
            sl += super->argdata->ndim;
            super = super->lispdata.ksuper;
          }
        if (sl > object->size)
          error(NIL,"lisp_c internal: class slot mismatch",n->litem);
        sl = object->size - 1;
        while (--k>=0)
          {
            super=class_list[k];
            j = super->argdata->ndim;
            drec = super->argdata + 1;
            while (--j>=0)
              {
                /* quick check of slot name */
                char *pos;
                struct symbol *symb = object->slots[sl].symb->Object;
                if (symb->name->name[0] != drec->name[0])
                  error(NIL,"lisp_c internal : object slot mismatch",n->litem);
                /* copy field described by current record */
                pos = (char*)cptr + (unsigned int)(drec->arg);
                address_to_dharg(&tmparg, pos, drec+1);
                orig = object->slots[sl].val;
                new = dharg_to_at(&tmparg, drec+1, n->litem);
                object->slots[sl].val = new;
                DELAYED_UNLOCK(new, orig);
                /* next record */
                drec = drec->end;
                sl -= 1;
              }
          }
        break;
      }
        
    case CINFO_STR:
      /* Strings are never modified by compiled code */
    case CINFO_LIST:
      /* Lists are never modified by compiled code */
    default:
      break;
    }
}





/* -----------------------------------------
   CALL A DH FUNCTION
   ----------------------------------------- */


/* build_at_temporary -- build an AT using the temporary style of dhrecord */

static void
build_at_temporary(dhrecord *drec, dharg *arg)
{
  switch (drec->op)
    {
    case DHT_SRG:
      {
        avlnode *n = alloc_srg(dht_to_storage[(drec+1)->op]);
        arg->dh_srg_ptr = n->citem;
        avlchain_set(n, &dummy_tmps); 
        break;
      }
    case DHT_IDX:
      {
        avlnode *n = alloc_idx(drec->ndim);
        arg->dh_idx_ptr = n->citem;
        avlchain_set(n, &dummy_tmps);
        break;
      }
    case DHT_OBJ:
      {
        dhclassdoc_t *classdoc = (dhclassdoc_t*)(drec->arg);
        avlnode *n = alloc_obj(classdoc);
        arg->dh_obj_ptr = n->citem;
        avlchain_set(n, &dummy_tmps);
        break;
      }
    case DHT_STR:
      {
        avlnode *n = alloc_str();
        arg->dh_srg_ptr = n->citem;
        avlchain_set(n, &dummy_tmps);
        break;
      }
    case DHT_LIST:
      {
        avlnode *n = alloc_list(drec->ndim);
        n->cmoreinfo = 0; /* TEMPs have strange DHDOCs */
        arg->dh_srg_ptr = n->citem;
        avlchain_set(n, &dummy_tmps);
        break;
      }
    default:
      {
        error(NIL,"lisp_c internal: unknown temporary style",NIL);
      }
    }
}



/* set the need_update field */

static void
set_update_flag(void)
{
  avlnode *n = dummy_upds.chnxt;
  while (n != &dummy_upds)
    {
      n->need_update = 1;
      n = n->chnxt;
    }
}


/* full_update_c_from_lisp -- copy lisp to c */

static void
full_update_c_from_lisp(void)
{
  avlnode *n = dummy_upds.chnxt;
  while (n != &dummy_upds)
    {
      if (n->litem == 0)
        error(NIL,"internal error in UPDS chain",NIL);
      if (n->need_update)
        update_c_from_lisp(n);            
      n = n->chnxt;
    }
}

/* full_update_lisp_from_c -- copy c to lisp */

static void
full_update_lisp_from_c(void)
{
  avlnode *n = dummy_upds.chnxt;
  while (n != &dummy_upds)
    {
      if (n->litem == 0)
        error(NIL,"internal error in UPDS chain",NIL);
      if (n->need_update)
        update_lisp_from_c(n);
      n = n->chnxt;
    }
}


/* wipe_out_temps -- remove temporary LISP objects */

static void 
wipe_out_temps(void)
{
  while (dummy_tmps.chnxt != &dummy_tmps)
    {
      void *cptr;
      dhclassdoc_t *cdoc;
      avlnode *n = dummy_tmps.chnxt;

      if (n->belong!=BELONG_LISP  || n->litem!=0)
        error(NIL,"internal error in TMPS chain",NIL);
      cptr = n->citem;
      switch(n->cinfo)
        {
        case CINFO_STR:
        case CINFO_SRG:
          srg_free(cptr);
          break;
        case CINFO_OBJ:
          /* this horrible hack is used for freeing temporary pools */
          cdoc = n->cmoreinfo;
          if (!strcmp("pool", cdoc->lispdata.cname))
            {
#if DLDBFD
              void (*func)() = (void*)dld_get_func("C_free_C_pool");
              if (func) (*func)(cptr);
#else
              C_free_C_pool(cptr);
#endif
            }
          break;
        }
      avlchain_set(n, 0);
      avl_del(cptr);
      free(cptr);
    }
}


/* run_time_error -- called by compiled code when an error occurs */

static int     run_time_error_flag;
static jmp_buf run_time_error_jump;

void
run_time_error(char *s)
{
  if (run_time_error_flag)
    {
      printf("\n\n*** lisp_c runtime error: %s\007\007\n",s);
      print_dh_trace_stack();
      dh_trace_root = 0;
      longjmp(run_time_error_jump,-1);
    }
  else
    {
      dh_trace_root = 0;
      error(NIL, s, NIL);
    }
}



/* dh_listeval -- calls a compiled function */
    
at *
dh_listeval(at *p, at *q)
{
#define MAXARGS 1024
  dhdoc_t *kname;
  struct cfunction *cfunc;
  dhrecord *drec;
  at *atgs[MAXARGS];
  dharg args[MAXARGS];
  at *atfuncret;
  dharg funcret;
  int nargs;
  int ntemps;
  int errflag;
  int i;
    
  /* Find and check the DHDOC */
  cfunc = p->Object;
  kname = cfunc->info;
  drec = kname->argdata;
  if(drec->op != DHT_FUNC)
    error(NIL, "(lisp_c) a function dhdoc was expected", NIL);
  if (CONSP(cfunc->name))
    check_primitive(cfunc->name);
  
  /* Count the arguments */
  nargs = drec->ndim;
  ntemps = ((dhrecord *) drec->name)->ndim;
  if ( nargs + ntemps > MAXARGS)
    error(NIL,"(lisp_c) too many arguments and temporaries",NIL);
  
  /* Copy and evaluate arguments list */
  for(i=0; i<nargs; i++) {
    q = q->Cdr;
    ifn (CONSP(q))
      need_error(0,nargs,NIL);
    atgs[i] = (*argeval_ptr)(q->Car);
  }
  if (q->Cdr)	
    need_error(0,nargs,NIL);

  /* Make compiled version of the arguments */
  for (i=0, drec= drec+1; i<nargs; i++)
    {
      if (!atgs[i] && 
          drec->op!=DHT_NIL && 
          drec->op!=DHT_GPTR && 
          drec->op!=DHT_BOOL)
        error(NIL,"(lisp_c) illegal nil argument",NIL);
      at_to_dharg(atgs[i], &args[i], drec, NIL);
      drec = drec->end;
    }
  
  /* Prepare temporaries */
  if (ntemps != 0) 
    {
      drec++;
      for(i=nargs; i<nargs+ntemps; i++) 
        {
          build_at_temporary(drec, &args[i]);
          drec = drec->end;
        }
      drec++;
    }
    
  /* Synchronize all compiled objects */
  set_update_flag();
  full_update_c_from_lisp();

  /* Prepare delayed_kill_list and environment */
  if (run_time_error_flag)
    lisp2c_warning("reentrant call to compiled code",0);
  delayed_kill_list = 0;
  errflag = setjmp(run_time_error_jump);
  dh_trace_root = 0;
    
  /* Call compiled code */
  if (!errflag)
    {
      run_time_error_flag = 1;
      /* Call the test function if it exists */
      if (kname->lispdata.dhtest)
        (*kname->lispdata.dhtest->lispdata.call)(args-1);
      /* Call to the function */
      funcret = (*kname->lispdata.call)(args-1);
    }
    
  /* Prepare for the update */
  run_time_error_flag = 0;
  dh_trace_root = 0;
  set_update_flag();
    
  /* Build return value */
  atfuncret = NIL;
  if (!errflag)
    {
      if (drec->op != DHT_RETURN)
        error(NIL,"lisp_c internal: cannot find DHT_RETURN in DHDOC",NIL);
      atfuncret = dharg_to_at(&funcret, drec+1, NIL);
    }
    
  /* Synchronize all compiled objects */
  full_update_lisp_from_c();
  /* Remove objects owned by LISP and not associated to LISP object */
  wipe_out_temps();
  /* Execute pending deletions */
  UNLOCK(delayed_kill_list);
  /* Unlock all arguments */
  for (i=0; i<nargs; i++)
    UNLOCK(atgs[i]);
  /* return */
  if (errflag)
    error(NIL,"Run-time error in compiled code",NIL);
  return atfuncret;
}


/* lisp-c-no-warnings -- revert to old mode */

DY(ylisp_c_no_warnings)
{
  at *ans;
  int sav_dont_warn = dont_warn;
  struct context mycontext;
  context_push(&mycontext);
  if (setjmp(context->error_jump)) 
    {
      context_pop();
      dont_warn = sav_dont_warn;
      longjmp(context->error_jump, -1L);
    }
  dont_warn = 1;
  ans = progn(ARG_LIST);
  dont_warn = sav_dont_warn;
  context_pop();
  return ans;
}



/* -----------------------------------------
   INTERPRETED CASTS
   ----------------------------------------- */


/* (to-int <arg>) */
DX(xto_int)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER( AINTEGER(1) );
}

/* (to-flt <arg>) */
DX(xto_flt)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER( Ftor( AFLT(1) ) );
}

/* (to-real <arg>) */
DX(xto_real)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER( AREAL(1) );
}

/* (to-number <arg>) */
DX(xto_number)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER( AREAL(1) );
}

/* (to-bool <arg>) */
DX(xto_bool)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (APOINTER(1))
    return true();
  return NIL;
}

/* (to-obj [<class>] <gptr>)  */
DX(xto_obj)
{
  class *cl = NULL;
  at *p = NULL;
  avlnode *n;
  /* parse arguments */
  ALL_ARGS_EVAL;
  switch (arg_number)
    {
    case 1:
      p = APOINTER(1);
      break;
    case 2:
      p = APOINTER(1);
      if (! EXTERNP(p, &class_class))
        error(NIL,"not a class",p);
      cl = p->Object;
      p = APOINTER(2);
      break;
    default:
      ARG_NUMBER(-1);
    }
  if (!p)
    {
      return p;
    }
  else if (p->flags & X_OOSTRUCT)
    {
      LOCK(p);
    }
  else if (p->flags & C_GPTR)
    {
      /* search object */
      if (! (n = avl_find(p->Gptr)))
        error(NIL,"Object pointed to by this GPTR has been deallocated",p);
      /* make lisp object */
      delayed_kill_list = 0;
      p = make_lisp_from_c(n, p->Gptr);
      UNLOCK(delayed_kill_list);
    }
  else 
    {
      error(NIL,"Expecting GPTR or OBJECT",p);
    }
  /* check class */
  if (cl)
    {
      class *clm;
      if (!( p->flags & X_OOSTRUCT))
        error(NIL,"GPTR does not point to an object",APOINTER(2));
      clm = p->Class;
      while (clm && (clm != cl))
        clm = clm->super;
      if (clm != cl)
        error(NIL,"GPTR does not point to an object of this class",
              APOINTER(1));
    }
  /* return */
  return p;
}


/* (to-gptr <obj>) */
DX(xto_gptr)
{
  at *p;
  avlnode *n = 0;
    
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p==0)
    {
      return NIL;
    }
  else if (EXTERNP(p, &index_class))
    {
      n = lside_create_idx(p);
      return NEW_GPTR(n->citem);
    }      
  else if (p && (p->flags & X_OOSTRUCT))
    {
      n = lside_create_obj(p);
      return NEW_GPTR(n->citem);
    }      
  else if (storagep(p))
    {
      n = lside_create_srg(p);
      return NEW_GPTR(n->citem);
    }      
  else if (EXTERNP(p, &string_class))
    {
      n = lside_create_str(p);
      return NEW_GPTR(n->citem);
    }      
#if DLDBFD
  else if (EXTERNP(p, &dh_class))
    {
      struct cfunction *cfunc;
      dhdoc_t *dhdoc;
      cfunc = p->Object;
      if (CONSP(cfunc->name))
        check_primitive(cfunc->name);
      if (( dhdoc = (dhdoc_t*)(cfunc->info) ))
        return NEW_GPTR(dld_get_func(dhdoc->lispdata.c_name));
    }
#endif
  error(NIL,"Cannot make a compiled version of this lisp object",p);
}




/* -----------------------------------------
   INITIALIZATION
   ----------------------------------------- */


void 
init_lisp_c(void)
{
  init_storage_to_dht();
  dx_define("lisp-c-map", xlisp_c_map);
  dx_define("lisp-c-dont-track-cside", xlisp_c_dont_track_cside);
  dy_define("lisp-c-no-warnings", ylisp_c_no_warnings);
  dx_define("to-int", xto_int);
  dx_define("to-flt", xto_flt);
  dx_define("to-real", xto_real);
  dx_define("to-number", xto_number);
  dx_define("to-bool", xto_bool);
  dx_define("to-gptr", xto_gptr);
  dx_define("to-obj", xto_obj);
}

