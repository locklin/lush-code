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
  T-Lisp3: LYB YLC 1/88
  allocate.c:
  This is a generic package for allocation and deallocation
  with a free list. The objects and the allocation structure
  are described in a  'generic_alloc structure'
  
  Contains also the garbage collector...
  
  $Id: allocate.c,v 0.1 2001-10-31 17:30:28 profshadoko Exp $
********************************************************************** */

#include "header.h"


/* From SYMBOL.C */
void kill_name (struct hash_name *hn);

/* From AT.C */
extern struct alloc_root at_alloc;

/* From OOSTRUCT.C */
extern void oostruct_dispose(at *p);



/* *** Memory (re-)allocation for Non-Lisp objects 
       yet with simple error testing;
       could be handy for more sioux use *** */

gptr
tl_malloc (size_t size)
{
  gptr value = malloc (size);
#ifdef MALLOC_DEBUG
  if (!malloc_verify()) {
    fprintf(stderr, "Hmmm, heap not clean any more!!\n");
  }
#endif /* MALLOC_DEBUG */
  if (value == 0)
    error (NIL, "Virtual memory exhausted", NIL);
  /* LYB NOTE
   * removed string "TL_Malloc" that was the first
   * argument of the function error(). The user does
   * not care about that. He cares about the name
   * of the Lisp function (default when NIL is given)
   */
  return value;
}


gptr
tl_realloc (gptr address, size_t size)
{
  gptr value = realloc (address, size);

  if (value == 0)
    error (NIL, "Virtual memory exhausted", NIL);
  /* LYB NOTE
   * removed string "TL_Realloc" that was the first
   * argument of the function error(). The user does
   * not care about that. He cares about the name
   * of the Lisp function (default when NIL is given)
   */
  return value;
}


/*
 * Allocate( &alloc_root_structure ) returns an instance of elems described
 * in the alloc_root_structure
 */


gptr 
allocate(struct alloc_root *ar)
{
  struct empty_alloc *answer;
  
  ifn(answer = ar->freelist) {
    struct chunk_header *chkhd;
    
    if ((chkhd = tl_malloc(ar->elemsize * ar->chunksize
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
 * Protected symbols are stored in the protected list...
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
 * At Garbage collector. It is called whenever an orror occurs. It
 * removes old unused ATs, and computes the count field
 * again.
 * 
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
  struct chunk_header *i;
  struct at *p;
  struct hash_name **j, *hn;

  if (flag) {

    /*
     * Flags every used cons as garbage
     * Sets high counts to avoid possible problems
     */
    
    iter_on(&at_alloc, i, p) {
      p->count += 0x40000000;
      p->flags |= C_GARBAGE;          
    }

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

    iter_on(&at_alloc, i, p)
      if ((p->flags & C_GARBAGE) && (p->flags & C_EXTERN)) {
	if (p->flags & X_OOSTRUCT)
	  oostruct_dispose(p); /* class may be gone already */
	else
	  (*p->Class->self_dispose)(p);
	p->Class = &zombie_class;
	p->Object = NIL;
	p->flags  = C_EXTERN | C_GARBAGE | X_ZOMBIE;
      }
  
  /*
   * Now, we may rebuild the count fields:
   * 1st: reset all count fields and flags
   * 2nd: mark named symbols and protected objects
   */
  
    iter_on(&at_alloc, i, p) {
      p->count = 0;
      p->flags &= ~(C_GARBAGE|C_MARK|C_MULTIPLE);
    }
    
    iter_hash_name(j, hn)
      if (hn->named)
	mark(hn->named);
    mark(protected);
    
    /*
     * Now, we may rebuild the free list:
     */
    
    at_alloc.freelist = NIL;
    for (i = at_alloc.chunklist; i; i = i->next)
      for (p = i->begin; (gptr) p < i->end; p++)
	if (p->count==0) {
	  deallocate(&at_alloc, (struct empty_alloc *) p);
	}
    
    /*
     * In addition, we remove non referenced
     * unbound symbols whose NOPURGE flag is cleared.
     */
    
    iter_hash_name(j, hn) {
      at *q;
      struct symbol *symb;
      q = hn->named;
      if ( !q ) {
	kill_name(hn);
      } else if (q->count==1) {
	symb = (struct symbol*)(q->Object);
	if (!symb->nopurge)
	  if (   symb->valueptr==0 
	      || *(symb->valueptr)==0
	      || ((*(symb->valueptr))->flags & X_ZOMBIE) )
	    kill_name(hn);
      }
    }

  } else {

    /* flag = 0: destroy all 
     *  - dont call the oostruct destructors..
     *  - dont destroy classes
     */

    iter_on(&at_alloc, i, p)
      p->count += 0x40000000;
    
    iter_on(&at_alloc, i, p)
      if (    (p->flags & C_EXTERN)
	  && !(p->flags & X_OOSTRUCT) 
	  &&  (p->Class != &class_class) ) {
	
	(*p->Class->self_dispose)(p);
	p->Class = &zombie_class;
	p->Object = NIL;
	p->flags  = C_EXTERN | C_GARBAGE | X_ZOMBIE;
      }
  }
}


/* Malloc replacement */

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
    return z;
}


void *lush_calloc(int x,int y,char *file,int line)
{
    void *z = calloc(x,y);
    if (malloc_file)
	fprintf(malloc_file,"%x\tcalloc\t%d\t%s:%d\n",(unsigned int)z,x*y,file,line);
    return z;
}

void *lush_realloc(void *x,int y,char *file,int line)
{
    void *z = (void*)realloc(x,y);
    if (malloc_file) {
	fprintf(malloc_file,"%x\trefree\t%d\t%s:%d\n",(unsigned int)x,y,file,line);
	fprintf(malloc_file,"%x\trealloc\t%d\t%s:%d\n",(unsigned int)z,y,file,line);
    }
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

