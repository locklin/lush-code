
#include "allocate.h"
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
} avlnode_t;


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

static avlnode_t *root;
static int nobjects;


/* ------------------------------------------
   ROUTINE FOR IMPLEMENTING A DOUBLY LINKED LIST
   ------------------------------------------ */

void
avlchain_set(avlnode_t *n, avlnode_t *head)
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
static avlnode_t *
avl_succ(avlnode_t *t)
{
  avlnode_t *r = t->rt;
  if (!t->avl_rthread)
    while (!r->avl_lthread)
      r = r->lt;
  return r;
}

/* avl_pred -- return previous node */

#ifdef __GNUC__
inline
#endif
static avlnode_t *
avl_pred(avlnode_t *t)
{
  avlnode_t *l = t->lt;
  if (!t->avl_lthread)
    while (!l->avl_rthread)
      l = l->rt;
  return l;
}


/* avl_first -- return first node greater than key */

static avlnode_t *
avl_first(void *key)
{
  avlnode_t *t = root;
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

static avlnode_t *
avl_find(void *key)
{
  avlnode_t *t = root;
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
static avlnode_t *_found_node;
static int _already_found;

static avlnode_t *
_avlnode_new(void *key)
{
  avlnode_t *n = allocate(&avlnode_alloc);
  memset(n,0,sizeof(struct avlnode));
  n->citem = key;
  return n;
}

static void 
_avl_add(avlnode_t **tp)
{
  avlnode_t *t = *tp;

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
                avlnode_t *l = t->lt;
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
                    avlnode_t *r = l->rt;
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
                avlnode_t *r = t->rt;
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
                    avlnode_t *l = r->lt;
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

static avlnode_t *
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
_avlnode_copy(avlnode_t *from, avlnode_t *to)
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
_avl_del(avlnode_t *par, avlnode_t **tp)
{
  int comp;
  avlnode_t *t = *tp;
  
 
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
          avlnode_t *s;
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
          avlnode_t *p;
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
          avlnode_t *p = avl_pred(t);
          _avlnode_copy(p, t);
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
            avlnode_t *r = t->rt;
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
                  avlnode_t *l = r->lt;
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
            avlnode_t *l = t->lt;
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
                  avlnode_t *r = l->rt;
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
avl_pnode(avlnode_t *n)
{
  if (n->belong == BELONG_C)
    sprintf(string_buffer,"C %p ", (n->citem));
  else if (n->belong == BELONG_LISP)
    sprintf(string_buffer,"L %p ", (n->citem));
  else
    sprintf(string_buffer,"? %p ", (n->citem));
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

  if (n->litem==0 || ZOMBIEP(n->litem))
    sprintf(string_buffer,"~\n");
  else
    sprintf(string_buffer,"%s\n",first_line(n->litem));
  print_string(string_buffer);

}

static void
avl_display(avlnode_t *n, int tab)
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
  else if (n->avl_bf == AVLRIGHTHEAVY)
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
