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
 * $Id: allocate.c,v 1.8 2002-07-19 18:42:19 leonb Exp $
 **********************************************************************/

/***********************************************************************
  This is a generic package for allocation and deallocation
  with a free list. The objects and the allocation structure
  are described in a  'generic_alloc structure'
  Contains also the garbage collector...
********************************************************************** */

#include "header.h"

/* From SYMBOL.C */
void kill_name (struct hash_name *hn);

/* From OOSTRUCT.C */
extern void oostruct_dispose(at *p);


/*
 * Allocate( &alloc_root_structure ) returns an instance 
 * of elems described in the alloc_root_structure
 */

gptr 
allocate(struct alloc_root *ar)
{
  struct empty_alloc *answer;
  
  ifn(answer = ar->freelist) {
    struct chunk_header *chkhd;
    
    if ((chkhd = malloc(ar->elemsize * ar->chunksize
			+ (int) sizeof(struct chunk_header)))) {
      chkhd->begin = (char *) chkhd +
	(int) sizeof(struct chunk_header);
      chkhd->end = (char *) (chkhd->begin) +
	ar->chunksize * ar->elemsize;
      chkhd->next = ar->chunklist;
      ar->chunklist = chkhd;
      
      for (answer = chkhd->begin;
	   answer < (struct empty_alloc *) chkhd->end;
	   answer = (struct empty_alloc *) ((char *) answer
					    + ar->elemsize)) {
	answer->next = ar->freelist;
	answer->used = 0;
	ar->freelist = answer;
      }
    } else
      error("allocate", "not enough memory", NIL);
    
    answer = ar->freelist;
  }
  ar->freelist = answer->next;
  answer->used = -1;
  return answer;
}

void 
deallocate(struct alloc_root *ar, struct empty_alloc *elem)
{
  if (elem) {
    elem->next = ar->freelist;
    elem->used = 0;
    ar->freelist = elem;
  }
}


/*
 * Finalizers
 */

typedef struct s_finalizer {
  struct s_finalizer *next;
  at *target;
  void (*func)(at*,void*);
  void *arg;
} finalizer;

static struct alloc_root finalizer_alloc = {
  NIL, NIL, sizeof(at), 100
};

static finalizer *finalizers[HASHTABLESIZE];

void 
add_finalizer(at *q, void (*func)(at*,void*), void *arg)
{
  if (q)
    {
      unsigned int h;
      finalizer *f = allocate(&finalizer_alloc);
      h = hash_pointer(q) % HASHTABLESIZE;
      f->next = finalizers[h];
      f->target = q;
      f->func = func;
      f->arg = arg;
      finalizers[h] = f;
      q->flags |= C_FINALIZER;
    }
}

void
del_finalizers(void *arg)
{
  int h;
  for (h=0; h<HASHTABLESIZE; h++)
    {
      finalizer *f;
      finalizer **pf = &finalizers[h];
      while ((f = *pf))
        {
          if (f->arg == arg)
            {
              *pf = f->next;
              deallocate(&finalizer_alloc, (struct empty_alloc*)f);
            }
          else
            pf = &(f->next);
        }
    }
}

void 
run_finalizers(at *q)
{
  unsigned int h = hash_pointer(q) % HASHTABLESIZE;
  finalizer **fp = & finalizers[h];
  finalizer *todo = 0;
  /* collect */
  while (*fp)
    {
      finalizer *f = *fp;
      if (f->target != q)
        {
          fp = &f->next;
        }
      else
        {
          *fp = f->next;
          f->next = todo;
          todo = f;
        }
    }
  /* execute */
  q->flags &= ~ C_FINALIZER;
  while (todo)
    {
      finalizer *f = todo;
      todo = f->next;
      (*f->func)(q, f->arg);
      deallocate(&finalizer_alloc, (struct empty_alloc*)f);
    }
}

/*
 * Protected objects are stored in the protected list...
 */

static at *protected = NIL;

void 
protect(at *q)
{
  at *p;

  p = protected;
  while (CONSP(p)) {
    if (p->Car==q)
      return;
    p = p->Cdr;
  }
  LOCK(q);
  protected = cons(q,protected);
}

void 
unprotect(at *q)
{
  at **p;
  
  p = &protected;
  while(CONSP((*p))) {
    if ((*p)->Car==q) {
      q = *p;
      *p = (*p)->Cdr;
      q->Cdr = NIL;
      UNLOCK(q);
    } else
      p = &((*p)->Cdr);
  }
}


/*
 * This garbage collector is called whenever an orror occurs. It
 * removes old unused ATs, and computes the count field again.
 * NOTE  that the AT allocation and deallocation functions are defined in file
 * AT.C. For efficiency, they bypass the generic allocator.
 */


/*
 * mark follows any link, rebuilding the count field
 */

static void 
mark(at *p)
{
 mark_loop:
  if (p) {
    if (p->count++ == 0) {
      if (p->flags & C_CONS) {
	mark(p->Car);
	p = p->Cdr;
	goto mark_loop;
      } else if (p->flags & C_EXTERN)
	(*(p->Class->self_action)) (p, mark);
    }
  }
}


/* 
 * unflag recursively removes the C_GARBAGE flag 
 */

static void 
unflag(at *p)
{
 unflag_loop:
  if (p && (p->flags & C_GARBAGE)) {
    p->flags &= ~C_GARBAGE;
    if (p->flags & C_CONS) {
      unflag(p->Car);
      p = p->Cdr;
      goto unflag_loop;
    } else if (p->flags & C_EXTERN)
      (*(p->Class->self_action)) (p, unflag);
  }
}




/*
 * symbol_popall destroys stacked symbol context
 */

static void 
symbol_popall(at *p)
{
  struct symbol *s1, *s2;
  extern struct alloc_root symbol_alloc;

  s1 = p->Object;
  while ((s2 = s1->next)) {
    deallocate(&symbol_alloc, (struct empty_alloc *) s1);
    s1 = s2;
  }
  p->Object = s1;
}

/*
 * garbage If called in 'STARLush' function, - unwinds symbol stacked values
 * - rebuilds the allocation structure, - erase unused symbols
 * if flag is TRUE: keep used symbols
 * if flag is FALSE: destroy all
 */

void 
garbage(int flag)
{
  struct hash_name **j, *hn;

  if (flag) 
    {
      
      /*
       * Flags every used cons as garbage
       * Sets high counts to avoid possible problems
       */
      begin_iter_at(p) 
        {
          p->count += 0x40000000;
          p->flags |= C_GARBAGE;          
        } 
      end_iter_at(p);
      
      /*
       * Pops all symbols 
       */
      iter_hash_name(j, hn)
        if (hn->named)
          symbol_popall(hn->named);
      
      /*
       * Unflag all named objects and protected symbols
       * and their contents
       */
      iter_hash_name(j, hn)
        if (hn->named)
          unflag(hn->named);
      unflag(protected);
      
      /*
       *  We want to call the destructors of
       *  all garbage objects (C_GARBAGE=1).
       *  We thus zombify them all.
       */
      begin_iter_at(p) 
        {
          if (p->flags & C_GARBAGE)
            {
              /* destructors */
              if (p->flags & C_EXTERN)
                {
                  if (p->flags & X_OOSTRUCT)
                    oostruct_dispose(p);
                  else 
                    (*p->Class->self_dispose)(p);
                  p->Class = &zombie_class;
                  p->Object = NIL;
                  p->flags  = C_EXTERN | C_GARBAGE | X_ZOMBIE;
                }
              /* finalizers */
              if (p->flags & C_FINALIZER)
                run_finalizers(p);
            }
        }
      end_iter_at(p);
      
      /*
       * Now, we may rebuild the count fields:
       * 1st: reset all count fields and flags
       * 2nd: mark named symbols and protected objects
       */
      begin_iter_at(p) 
        {
          p->count = 0;
          p->flags &= ~(C_GARBAGE|C_MARK|C_MULTIPLE);
        }
      end_iter_at(p);
      iter_hash_name(j, hn)
        if (hn->named)
          mark(hn->named);
      mark(protected);
      
      /*
       * Now, we may rebuild the free list:
       */
      at_alloc.freelist = NIL;
      {
        at *p;
        struct chunk_header *i; 
        for (i = at_alloc.chunklist; i; i = i->next)
          for (p = i->begin; (gptr) p < i->end; p++)
            if (p->count==0) 
              deallocate(&at_alloc, (struct empty_alloc *) p);
      }
      
      /*
       * In addition, we remove non referenced
       * unbound symbols whose NOPURGE flag is cleared.
       */
      iter_hash_name(j, hn) 
        {
          at *q;
          struct symbol *symb;
          q = hn->named;
          if (q == 0)
            {
              kill_name(hn);
            } 
          else if (q->count==1)
            {
              symb = (struct symbol*)(q->Object);
              if (! symb->nopurge)
                if (! (q->flags & C_FINALIZER))
                  if ( !symb->valueptr ||
                       !*(symb->valueptr) ||
                       ((*(symb->valueptr))->flags & X_ZOMBIE) )
                    kill_name(hn);
            }
        }
      
    } 
  else 
    {
      /* flag = 0: destroy all 
       *  - dont call the oostruct destructors..
       *  - dont call the finalizers..
       *  - dont destroy classes
       */
      begin_iter_at(p) 
        {
          p->count += 0x40000000;
        }
      end_iter_at(p);
      begin_iter_at(p) 
        {
          if ((p->flags & C_EXTERN)
              && !(p->flags & X_OOSTRUCT) 
              &&  (p->Class != &class_class) ) 
            {
              (*p->Class->self_dispose)(p);
              p->Class = &zombie_class;
              p->Object = NIL;
              p->flags  = C_EXTERN | C_GARBAGE | X_ZOMBIE;
            }
        }
      end_iter_at(p);
    }
}




/*
 * Malloc replacement 
 */

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef cfree

static FILE *malloc_file = 0;

void set_malloc_file(char *s)
{
    if (malloc_file) 
	fclose(malloc_file);
    if (s)
	malloc_file = fopen(s,"w");
    else
	malloc_file = NULL;
}


void *lush_malloc(int x, char *file, int line)
{
    void *z = malloc(x);
    if (malloc_file)
	fprintf(malloc_file,"%x\tmalloc\t%d\t%s:%d\n",(unsigned int)z,x,file,line);
    if (!z)
      error (NIL, "Memory exhausted", NIL);
    return z;
}


void *lush_calloc(int x,int y,char *file,int line)
{
    void *z = calloc(x,y);
    if (malloc_file)
	fprintf(malloc_file,"%x\tcalloc\t%d\t%s:%d\n",(unsigned int)z,x*y,file,line);
    if (!z)
      error (NIL, "Memory exhausted", NIL);
    return z;
}

void *lush_realloc(void *x,int y,char *file,int line)
{
    void *z = (void*)realloc(x,y);
    if (malloc_file) {
	fprintf(malloc_file,"%x\trefree\t%d\t%s:%d\n",(unsigned int)x,y,file,line);
	fprintf(malloc_file,"%x\trealloc\t%d\t%s:%d\n",(unsigned int)z,y,file,line);
    }
    if (!z)
      error (NIL, "Memory exhausted", NIL);
    return z;
}


void lush_free(void *x,char *file,int line)
{
    free(x);
    if (malloc_file)
	fprintf(malloc_file,"%x\tfree\t%d\t%s:%d\n",(unsigned int)x,0,file,line);
}

void lush_cfree(void *x,char *file,int line)
{
    cfree(x);
    if (malloc_file)
	fprintf(malloc_file,"%x\tcfree\t%d\t%s:%d\n",(unsigned int)x,0,file,line);
}

