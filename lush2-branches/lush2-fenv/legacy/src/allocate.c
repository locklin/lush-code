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
 * $Id: allocate.c,v 1.15 2004/02/07 01:32:49 leonb Exp $
 **********************************************************************/

/***********************************************************************
  This is a generic package for allocation and deallocation
  with a free list. The objects and the allocation structure
  are described in a  'generic_alloc structure'
  Contains also the garbage collector...
********************************************************************** */

#include "header.h"

/* From SYMBOL.C */
extern void kill_name (struct hash_name *hn);

/* From OOSTRUCT.C */
extern void oostruct_dispose(at *p);

#define SIZEOF_CHUNK_AT (SIZEOF_CELL*CONSCHUNKSIZE)

void allocate_chunk(alloc_root_t *ar) {
  
  assert(ar->freelist==NULL);
  size_t sizeof_cell = ar->sizeof_elem + 2*REDZONE_SIZE;
  size_t sizeof_chunk = sizeof_cell*ar->chunksize;
  chunk_header_t *chkhd = malloc(sizeof(chunk_header_t) + sizeof_chunk);
  //printf("adding new chunk of size %d\n", ar->chunksize);
  if (chkhd) {
    chkhd->begin = (empty_alloc_t *) ((char*)chkhd + sizeof(chunk_header_t));
    chkhd->end = (empty_alloc_t *) ((char*)chkhd->begin + sizeof_chunk);
    chkhd->next = ar->chunklist;
    ar->chunklist = chkhd;
    
    char *p = (char*)chkhd->begin + REDZONE_SIZE;
    for (int i = 0; i < ar->chunksize; i++, p+=sizeof_cell) {
      empty_alloc_t *ea = (empty_alloc_t *)p;
      ea->next = ar->freelist;
      ea->used = 0;
      ar->freelist = ea;
    }
    VALGRIND_MAKE_NOACCESS(chkhd->begin, sizeof_chunk);
  } else
    RAISEF("not enough memory", NIL);
}

void allocate_chunk_at(void) {
  
  extern LUSHAPI alloc_root_t at_alloc;
  assert(at_alloc.freelist==NULL);
  chunk_header_t *chkhd = malloc(sizeof(chunk_header_t) + SIZEOF_CHUNK_AT);
  if (chkhd) {
    chkhd->begin = (empty_alloc_t *) ((char*)chkhd + sizeof(chunk_header_t));
    chkhd->end = (empty_alloc_t *) ((char*)chkhd->begin + SIZEOF_CHUNK_AT);
    chkhd->next = at_alloc.chunklist;
    at_alloc.chunklist = chkhd;
    VALGRIND_MAKE_NOACCESS(chkhd->begin, SIZEOF_CHUNK_AT);
    
    char *p = (char*)chkhd->begin + REDZONE_SIZE;
    for (int i = 0; i < CONSCHUNKSIZE; i++, p+=SIZEOF_CELL) {
      VALGRIND_MAKE_WRITABLE(p, sizeof(at));
      empty_alloc_t *ea = (empty_alloc_t *)p;
      ea->next = at_alloc.freelist;
      ea->used = 0;
      at_alloc.freelist = ea;
    }
  } else
    RAISEF("not enough memory", NIL);
}

/*
 * Finalizers
 */

typedef struct finalizer {
  struct finalizer *next;
  at *target;
  void (*func)(at*,void*);
  void *arg;
} finalizer_t;

static alloc_root_t finalizer_alloc_root = {
  NIL, NIL, sizeof(finalizer_t), 100
};

static finalizer_t *finalizers[HASHTABLESIZE];

void add_finalizer(at *q, void (*func)(at*,void*), void *arg)
{
  if (q) {
    unsigned int h = hash_pointer(q) % HASHTABLESIZE;
    /* Already there? */
    if (q->flags & C_FINALIZER)
      for(finalizer_t *f = finalizers[h]; f; f=f->next)
	if (f->target==q && f->func==func && f->arg==arg)
	  return;
    /* Add a new one */
    finalizer_t *f = allocate(&finalizer_alloc_root);
    f->next = finalizers[h];
    f->target = q;
    f->func = func;
    f->arg = arg;
    finalizers[h] = f;
    q->flags |= C_FINALIZER;
  }
}

void del_finalizers(void *arg)
{
  for (int h=0; h<HASHTABLESIZE; h++) {
    finalizer_t **pf = &finalizers[h];
    finalizer_t *f = *pf;
    while (f) {
      if (f->arg == arg) {
	*pf = f->next;
	deallocate(&finalizer_alloc_root, (empty_alloc_t *)f);
      } else
	pf = &(f->next);
      f = *pf;
    }
  }
}

void run_finalizers(at *q)
{
  unsigned int h = hash_pointer(q) % HASHTABLESIZE;
  finalizer_t **fp = & finalizers[h];
  finalizer_t *todo = NULL;
  /* collect */
  while (*fp) {
    finalizer_t *f = *fp;
    if (f->target != q)
      fp = &f->next;
    else {
      *fp = f->next;
      f->next = todo;
      todo = f;
    }
  }
  /* execute */
  q->flags &= ~ C_FINALIZER;
  while (todo) {
    finalizer_t *f = todo;
    todo = f->next;
    (*f->func)(q, f->arg);
    deallocate(&finalizer_alloc_root, (empty_alloc_t *)f);
  }
}

/*
 * Protected objects are stored in the protected list...
 */

static at *protected = NIL;

void protect(at *q)
{
  at *p = protected;

  while (CONSP(p)) {
    if (p->Car==q)
      return;
    p = p->Cdr;
  }
  LOCK(q);
  protected = cons(q,protected);
}

void unprotect(at *q)
{
  at **p = &protected;

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

static void mark(at *p)
{
 mark_loop:
  if (p) {
    if (p->count++ == 0) {
      if (CONSP(p)) {
	mark(p->Car);
	p = p->Cdr;
	goto mark_loop;
      } else if (EXTERNP(p))
	(*(p->Class->action)) (p, mark);
    }
  }
}


/* 
 * unflag recursively removes the C_GARBAGE flag 
 */

static void unflag(at *p)
{
 unflag_loop:
  if (p && (p->flags & C_GARBAGE)) {
    p->flags &= ~C_GARBAGE;
    if (CONSP(p)) {
      unflag(p->Car);
      p = p->Cdr;
      goto unflag_loop;
    } else if (EXTERNP(p))
      (*(p->Class->action)) (p, unflag);
  }
}




/*
 * symbol_popall destroys stacked symbol context
 */

static void symbol_popall(at *p)
{
  extern struct alloc_root symbol_alloc;

  symbol_t *s1 = at2symbol(p);
  symbol_t *s2 = s1->next;
  while (s2) {
    deallocate(&symbol_alloc, (struct empty_alloc *) s1);
    s1 = s2;
    s2 = s1->next;
  }
  p->Object = s1;
}

/*
 * garbage If called in 'STARLush' function, - unwinds symbol stacked values
 * - rebuilds the allocation structure, - erase unused symbols
 * if flag is TRUE: keep used symbols
 * if flag is FALSE: destroy all
 */

void garbage(int flag)
{
  struct hash_name **j, *hn;

  if (flag) {
      
    /*
     * Flag every used cons as garbage
     * Sets high counts to avoid possible problems
     */
    begin_iter_at(p) {
      p->count += 0x40000000;
      p->flags |= C_GARBAGE;
    } end_iter_at(p);
      
    /*
     * Pop all symbols 
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
     *  But we start by the oostructs
     *  because they can have weird destructors.
     */
    begin_iter_at(p) {
      if (p->flags & C_GARBAGE)
         if (OBJECTP(p)) {
	  int finalize = (p->flags & C_FINALIZER);
	  oostruct_dispose(p);
	  p->flags  = C_GARBAGE;
          assert(ZOMBIEP(p));
	  if (finalize)
	    run_finalizers(p);
	}
    } end_iter_at(p);

    begin_iter_at(p) {
      if (p->flags & C_GARBAGE){
	int finalize = (p->flags & C_FINALIZER);
	if (EXTERNP(p)) {
	  (*p->Class->dispose)(p);
	  p->Class = &zombie_class;
	  p->Object = NIL;
	  p->flags  = C_GARBAGE;
	}
	if (finalize)
	  run_finalizers(p);
      }
    } end_iter_at(p);
      
    /*
     * Now, we may rebuild the count fields:
     * 1st: reset all count fields and flags
     * 2nd: mark named symbols and protected objects
     */
    begin_iter_at(p) {
      p->count = 0;
      p->flags &= ~(C_GARBAGE|C_MARK|C_MULTIPLE);
    } end_iter_at(p);

    iter_hash_name(j, hn)
      if (hn->named)
	mark(hn->named);
    mark(protected);
      
    /*
     * Now, we may rebuild the free list:
     */
    extern alloc_root_t at_alloc;
    at_alloc.freelist = NIL;
    begin_iter_all_at {
      at *p = (at *)__p;
      if (p->count==0) 
	deallocate_at(p);
    } end_iter_all_at;
/*     at_alloc.freelist = NIL; { */
/*       chunk_header_t *hd = at_alloc.chunklist; */
/*       for (; hd; hd = hd->next) */
/* 	for (at *p = (at *)hd->begin; (gptr)p < (gptr)hd->end; p++) */
/* 	  if (p->count==0)  */
/* 	    deallocate(&at_alloc, p); */
/*     } */
      
    /*
     * In addition, we remove non referenced
     * unbound symbols whose NOPURGE flag is cleared.
     */
    iter_hash_name(j, hn) {
      at *q = hn->named;
      if (q == NIL)
	kill_name(hn);
      else if (q->count==1) {
	symbol_t *symb = q->Object;
	ifn (symb->nopurge)
           ifn ((q->flags & C_FINALIZER))
              if (!symb->valueptr ||
                  !*(symb->valueptr) ||
                  ZOMBIEP(*(symb->valueptr)) )
                 kill_name(hn);
      }
    }
    
  } else {
    /* flag = 0: destroy all 
     *  - dont call the oostruct destructors..
     *  - dont call the finalizers..
     *  - dont destroy classes
     */
    begin_iter_all_at {
      at* p = (at *)__p;
      if (p->count)
	p->count += 0x40000000;
    } end_iter_all_at;
    
    begin_iter_at(p) {
      if (EXTERNP(p)
	  && !OBJECTP(p)
	  && (p->Class != &class_class) ) {
	(*p->Class->dispose)(p);
	p->Class = &zombie_class;
	p->Object = NIL;
	p->flags  = C_GARBAGE;
      }
    } end_iter_at(p);
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


void *lush_malloc(size_t x, char *file, int line)
{
    void *z = malloc(x);
    if (malloc_file)
	fprintf(malloc_file,"%p\tmalloc\t%d\t%s:%d\n",z,x,file,line);
    if (x>0 && z==NULL)
      error (NIL, "Memory exhausted", NIL);
    return z;
}


void *lush_calloc(size_t x,int y,char *file,int line)
{
    void *z = calloc(x,y);
    if (malloc_file)
	fprintf(malloc_file,"%p\tcalloc\t%d\t%s:%d\n",z,x*y,file,line);
    if (x>0 && z==NULL)
      error (NIL, "Memory exhausted", NIL);
    return z;
}

void *lush_realloc(void *x,size_t y,char *file,int line)
{
    void *z = (void*)realloc(x,y);
    if (malloc_file) {
	fprintf(malloc_file,"%p\trefree\t%d\t%s:%d\n",x,y,file,line);
	fprintf(malloc_file,"%p\trealloc\t%d\t%s:%d\n",z,y,file,line);
    }
    if (y>0 && z==NULL)
      error (NIL, "Memory exhausted", NIL);
    return z;
}


void lush_free(void *x,char *file,int line)
{
    free(x);
    if (malloc_file)
	fprintf(malloc_file,"%p\tfree\t%d\t%s:%d\n",x,0,file,line);
}

void lush_cfree(void *x,char *file,int line)
{
#if HAVE_CFREE
    cfree(x);
#else
    free(x);
#endif
    if (malloc_file)
	fprintf(malloc_file,"%p\tcfree\t%d\t%s:%d\n",x,0,file,line);
}

