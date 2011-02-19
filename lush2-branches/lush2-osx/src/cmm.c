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

/*
 * Note: This implementation requires that malloc/realloc/calloc
 * deliver pointers that are 8-byte aligned (i.e., the lowest
 * three bits are clear). If your system does not do this, you
 * may try replacing malloc by posix_memalign and compile with 
 * _XOPEN_SOURCE set to 600.
 *
 * According to http://linux.die.net/man/3/posix_memalign,
 * "GNU libc malloc() always returns 8-byte aligned memory
 *  addresses, ..."
 */

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE  199506L
#define _XOPEN_SOURCE    600

#define MM_INTERNAL 1
#include "lushconf.h"
#include "cmm.h"

#ifndef NVALGRIND
#  include <valgrind/memcheck.h>
#else
#  define VALGRIND_CREATE_MEMPOOL(...)
#  define VALGRIND_MEMPOOL_ALLOC(...)
#  define VALGRIND_MEMPOOL_FREE(...)
#  define VALGRIND_CREATE_BLOCK(...)
#  define VALGRIND_MAKE_MEM_NOACCESS(...)
#  define VALGRIND_MAKE_MEM_DEFINED(...)
#  define VALGRIND_MAKE_MEM_UNDEFINED(...)
#  define VALGRIND_CHECK_MEM_IS_DEFINED(...)
#  define VALGRIND_DISCARD(...)
#endif

#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#ifdef MM_SNAPSHOT_GC
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <ctype.h>
#  include <fcntl.h>
#  include <dirent.h>
#endif

#define max(x,y)        ((x)<(y) ? (y) : (x))
#define min(x,y)        ((x)<(y) ? (x) : (y))
#define mm_printf(...)  fprintf(stdlog, __VA_ARGS__)
#define debug(...)      mm_debug_enabled ? (\
        fprintf(stdlog, "mm(%s): ", __func__), \
        fprintf(stdlog, __VA_ARGS__), 0) : 1

#define warn(...)      \
        fprintf(stderr, "mm(%s): ", __func__), \
        fprintf(stderr, __VA_ARGS__), fflush(stderr), 0

#define PPTR(p)         ((uintptr_t)(p))

#define PAGEBITS        12
#define BLOCKBITS       PAGEBITS
#if defined  PAGESIZE && (PAGESIZE != (1<<PAGEBITS))
#  error Definitions related to PAGESIZE inconsistent
#elif !defined PAGESIZE
#  define PAGESIZE      (1<<PAGEBITS)
#endif
#define BLOCKSIZE       (1<<BLOCKBITS)


#define ALIGN_NUM_BITS  3
#define MIN_HUNKSIZE    (1<<ALIGN_NUM_BITS)
#define MIN_NUMBLOCKS   0x100
#define MIN_TYPES       0x100
#define MIN_MANAGED     0x40000
#define MIN_ROOTS       0x100
#define MIN_STACK       0x1000
#define MAX_STACK       0x10000
#define MAX_VOLUME      (0x800000*sizeof(void *))    /* max volume threshold */
#define MAX_BLOCKS      (150*sizeof(void *))
#define NUM_TRANSFER    (PIPE_BUF/sizeof(void *))
#define NUM_IDLE_CALLS  100

#define HMAP_NUM_BITS   4

#ifndef INT_MAX
#  error INT_MAX not defined.
#elif INT_MAX == 9223372036854775807L
#  define HMAP_EPI       16
#  define HMAP_EPI_BITS   4
#elif INT_MAX == 2147483647
#  define HMAP_EPI        8
#  define HMAP_EPI_BITS   3
#elif INT_MAX == 32767
#  define HMAP_EPI        4
#  define HMAP_EPI_BITS   2
#else
#  error Value of INT_MAX not supported.
#endif


typedef struct hunk {
   struct hunk *next;
} hunk_t;

typedef struct block {
   struct block *next;
} block_t;

typedef struct blockrec {
   mt_t       t;            /* type directory entry    */
   int        in_use;       /* number of object in use */
} blockrec_t;

typedef struct info {
   mt_t       t;            /* type of memory object   */
   uint32_t   nh;           /* size of memory object in multiples of MIN_HUNKSIZE !! */
} info_t;

typedef struct typerec {
   char            *name;
   size_t          size;    /* zero when variable size */
   clear_func_t    *clear; 
   mark_func_t     *mark;
   finalize_func_t *finalize;
   uintptr_t       current_a;     /* current address   */
   uintptr_t       current_amax; 
   int             next_b;        /* next block to try */
} typerec_t;

/* heap management */
static size_t     heapsize;
static size_t     hmapsize;
static size_t     volume_threshold;
static int        block_threshold;
static int        num_blocks;
static int        num_free_blocks;
static blockrec_t *blockrecs = NULL;
static char      *heap = NULL;
static unsigned int *restrict hmap = NULL; /* bits for heap objects */
static bool       heap_exhausted = false;

static void *    *restrict managed;
static int        man_size;
static int        man_last = -1;
static int        man_k = -1;
static int        man_t = 0;
static bool       man_is_compact = true;

static void **   *restrict roots;
static int        roots_last = -1;
static int        roots_size;

/* type registry */
static typerec_t *types;
static mt_t       types_last = -1;
static mt_t       types_size;
static int       *profile = NULL;
static int        num_profiles = 0;

/* the marking stack */
static const void *restrict *stack = NULL;
static int        stack_size = MIN_STACK;
static int        stack_last = -1;
static bool       stack_overflowed  = false;
static bool       stack_overflowed2 = false;

/* other state variables */
static int        num_allocs = 0;
static int        num_alloc_blocks = 0;
static int        num_collects = 0;
static size_t     vol_allocs = 0;
static bool       gc_disabled = false;
static int        fetch_backlog = 0;
static bool       collect_in_progress = false;
static bool       mark_in_progress = false;
static bool       collect_requested = false;
static pid_t      collecting_child = 0;
static notify_func_t *client_notify = NULL;
static mt_t       marking_type = mt_undefined;
static const void *marking_object = NULL;
static FILE *     stdlog = NULL;
bool              mm_debug_enabled = false;
static void      *cstack_ptr_high = NULL;
static void      *cstack_ptr_low = NULL;

/* transient object stack */
typedef const void    *stack_elem_t; 
typedef stack_elem_t  *stack_ptr_t;
typedef struct stack mmstack_t;

static mmstack_t   *make_stack(void);
static void         stack_push(mmstack_t *, stack_elem_t);
static stack_elem_t stack_peek(mmstack_t *);
static stack_elem_t stack_pop(mmstack_t *);
static bool         stack_empty(mmstack_t *);
static void         stack_reset(mmstack_t *, stack_ptr_t);
static stack_elem_t stack_elt(mmstack_t *, int);

static mt_t       mt_stack;
static mt_t       mt_stack_chunk;
mmstack_t *const _mm_transients;

/*
 * MM's little helpers
 */

#define BITL               1  /* LIVE/OBSOLETE address */
#define BITN               2  /* NOTIFY                */
#define BITB               4  /* type blob             */
#define BITM               8

/* Note: The same bit is used to mark an address LIVE or
 * OBSOLETE. The LIVE bit is only needed in the marking
 * phase and we make sure all OBSOLETEs are removd before
 * we start marking.
 */

#define LIVE(p)            (((uintptr_t)(p)) & BITL)
#define NOTIFY(p)          (((uintptr_t)(p)) & BITN)
#define BLOB(p)            (((uintptr_t)(p)) & BITB)
#define OBSOLETE           LIVE

#define MARK_LIVE(p)       { p = (void *)((uintptr_t)(p) | BITL); }
#define MARK_NOTIFY(p)     { p = (void *)((uintptr_t)(p) | BITN); }
#define MARK_BLOB(p)       { p = (void *)((uintptr_t)(p) | BITB); }
#define MARK_OBSOLETE      MARK_LIVE

#define UNMARK_LIVE(p)     { p = (void *)((uintptr_t)(p) & ~BITL); }
#define UNMARK_NOTIFY(p)   { p = (void *)((uintptr_t)(p) & ~BITN); }

#define HMAP(a, op, b) (hmap[(((uintptr_t)(a))>>(ALIGN_NUM_BITS + HMAP_EPI_BITS))] op \
                        ((b) << (((((uintptr_t)(a))>>ALIGN_NUM_BITS) & (HMAP_EPI-1)) * HMAP_NUM_BITS)))
#define HMAP_LIVE(a)       HMAP(a, &, BITL)
#define HMAP_NOTIFY(a)     HMAP(a, &, BITN)
#define HMAP_MANAGED(a)    HMAP(a, &, BITM)

#define HMAP_MARK_LIVE(a)     HMAP(a, |=, BITL)
#define HMAP_MARK_NOTIFY(a)   HMAP(a, |=, BITN)
#define HMAP_MARK_MANAGED(a)  HMAP(a, |=, BITM)

#define HMAP_UNMARK_LIVE(a)     HMAP(a, &=~, BITL)
#define HMAP_UNMARK_NOTIFY(a)   HMAP(a, &=~, BITN)
#define HMAP_UNMARK_MANAGED(a)  HMAP(a, &=~, BITM)

#define LBITS(p)           ((uintptr_t)(p) & (MIN_HUNKSIZE-1))
#define CLRPTR(p)          ((void *)((((uintptr_t)(p)) & ~(MIN_HUNKSIZE-1))))
#define ADDRESS_VALID(p)   (!LBITS(p))
#define TYPE_VALID(t)      ((t)>=0 && (t)<=types_last)
#define INDEX_VALID(i)     ((i)>=0 && (i)<=man_last)


#define INHEAP(p)          (((char *)(p) >= heap) && ((char *)(p) < (heap + heapsize)))
#define BLOCK(p)           (((ptrdiff_t)((char *)(p) - heap))>>BLOCKBITS)
#define BLOCK_ADDR(p)      (heap + BLOCKSIZE*BLOCK(p))
#define BLOCKA(a)          (((uintptr_t)((char *)(a)))>>BLOCKBITS)
#define INFO_S(p)          (BLOB(p) ? 0 : ((info_t *)(unseal((char *)p)))->nh*MIN_HUNKSIZE)
#define INFO_T(p)          (BLOB(p) ? mt_blob : ((info_t *)(unseal((char *)p)))->t)

#define FIX_SIZE(s)        if (s % MIN_HUNKSIZE) \
                              s = ((s>>ALIGN_NUM_BITS)+1)<<ALIGN_NUM_BITS

#define ABORT_WHEN_OOM(p)  if (!(p)) { warn("allocation failed\n"); abort(); }

/* loop over all managed addresses in small object heap */
/* NOTE: the real address is 'heap + a', b is the block */
#define DO_HEAP(a, b) { \
   uintptr_t __a = 0; \
   for (int b = 0; b < num_blocks; b++, __a += BLOCKSIZE) { \
      if (blockrecs[b].in_use > 0) { \
         uintptr_t __a_next = __a + BLOCKSIZE; \
         for (uintptr_t a = __a; a < __a_next; a += MIN_HUNKSIZE) { \
            if (HMAP_MANAGED(a))

#define DO_HEAP_END }}}}

/* loop over all other managed addresses                */
/* NOTE: the real address is 'managed[i]', which is not */
/* a cleared address (use CLRPTR to clear)             */ 
#define DO_MANAGED(i) { \
  size_t __lasti = collect_in_progress ? man_k : man_last; \
  for (size_t i = 0; i <= __lasti; i++) { \

#define DO_MANAGED_END }}

#define DISABLE_GC \
   bool __nogc = gc_disabled; \
   gc_disabled = true;

#define ENABLE_GC gc_disabled = __nogc;

#ifdef MM_SNAPSHOT_GC
static void mm_collect(void);
static int fetch_unreachables(void);
#else
#  define mm_collect()          /* noop */
#  define fetch_unreachables()  /* noop */
#endif

static void *seal(const char *p)
{
   p = CLRPTR(p);
   VALGRIND_MAKE_MEM_NOACCESS(p, MIN_HUNKSIZE);
   p += MIN_HUNKSIZE;
   return (void *)p;
}

static void *unseal(const char *p)
{
   p = CLRPTR(p);
   p -= MIN_HUNKSIZE;
   VALGRIND_MAKE_MEM_DEFINED(p, MIN_HUNKSIZE);
   return (void *)p;
}

static void check_num_free_blocks(void)
{
   int n = 0;
   for (int b=0; b < num_blocks; b++)
     if (blockrecs[b].t == mt_undefined)
        n++;
   assert(n==num_free_blocks);
}

static int _find_managed(const void *p);
static bool live(const void *p)
{
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(HMAP_MANAGED(a));
      return HMAP_LIVE(a);
   }
   int i = _find_managed(p);
   assert(i != -1);
   return LIVE(managed[i]);
}

static void mark_live(const void *p)
{
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(HMAP_MANAGED(a));
      HMAP_MARK_LIVE(a);
      return;
   }
   int i = _find_managed(p);
   assert(i != -1);
   MARK_LIVE(managed[i]);
}

static void maybe_trigger_collect(size_t s)
{
   num_allocs += 1;
   vol_allocs += s;
   if (gc_disabled || collect_in_progress)
      return;

   if (num_alloc_blocks >= block_threshold || 
       vol_allocs >= volume_threshold      ||
       collect_requested) {
#ifdef MM_SNAPSHOT_GC      
      if (num_free_blocks < block_threshold) {
         warn("running low on memory, doing synchronous GC\n");
         mm_collect_now();
      } else {
         mm_collect();
         if (!collect_in_progress)
            /* could not spawn child, do it synchronously */
            mm_collect_now();
      }
#else
      mm_collect_now();
#endif
   }
}

#define AMAX(s) ((BLOCKSIZE/(s) - 1)*(s))
 
/* scan small object heap for next free hunk                 */
/* update current_a field for type t, return true on success */  
static bool _update_current_a(mt_t t, typerec_t *tr, uintptr_t s, uintptr_t a)
{
   if (blockrecs[BLOCKA(a)].t != t)
      goto search_for_block;

search_in_block:
   /* search for next free hunk in block */
   while (a <= tr->current_amax && HMAP_MANAGED(a))
      a += s;
   if (a <= tr->current_amax) {
      tr->current_a = a;
      return true;
   }

search_for_block:
   /* search for another block with free hunks */
   ;
   int b = tr->next_b;
   int orig_b = (b+num_blocks-1) % num_blocks;

   while (b != orig_b) {
      if (blockrecs[b].t == mt_undefined) {
         assert(blockrecs[b].in_use == 0);
         blockrecs[b].t = t;
         VALGRIND_CREATE_BLOCK(heap + a, BLOCKSIZE, types[t].name);
         tr->current_a = a = b*BLOCKSIZE;
         tr->current_amax = a + AMAX(s);
         tr->next_b = (b == num_blocks) ? 1 : b+1;
         num_alloc_blocks++;
         num_free_blocks--;
         return true;

      } else if (blockrecs[b].t==t && blockrecs[b].in_use<(BLOCKSIZE/s)) {
         a = b*BLOCKSIZE;
         tr->current_amax = a + AMAX(s);
         tr->next_b = (b == num_blocks) ? 1 : b+1;
         goto search_in_block;
      }
      b = (b+1) % num_blocks;
   }

   /* no free hunk found */
   assert(b == orig_b);
   heap_exhausted = true;
   tr->current_a = 0;
   tr->current_amax = AMAX(s);
   tr->next_b = 0;
   return false;
}

static bool update_current_a(mt_t t)
{
   typerec_t *tr = &types[t];

   if (blockrecs[BLOCKA(tr->current_a)].t == t) {
      tr->current_a += tr->size;
      if (tr->current_a <= tr->current_amax) {
         if (!HMAP_MANAGED(tr->current_a))
            return true;
      } else
         tr->current_a -= tr->size;
   }   
   return _update_current_a(t, tr, tr->size, tr->current_a);
}

/* allocate from small-object heap if possible */
static void *alloc_fixed_size(mt_t t)
{
   if (collect_in_progress && !collecting_child)
      return NULL;

   if (heap_exhausted || !update_current_a(t))
      return NULL;

   void *p = heap + types[t].current_a;
   VALGRIND_MEMPOOL_ALLOC(heap, p, types[t].size);
   blockrecs[BLOCKA(types[t].current_a)].in_use++;
   
   return p;
}

/* allocate with malloc */
static void *alloc_variable_sized(mt_t t, size_t s)
{
   /* make sure s is a multiple of MIN_HUNKSIZE */
   assert(s%MIN_HUNKSIZE == 0);

   if (s > MM_SIZE_MAX) {
      warn("size exceeds MM_SIZE_MAX\n");
      abort();
   }
  
   void *p = NULL;
   /* don't allocate from heap when t==mt_stack */
   if (t && types[t].size>=s && BLOCKSIZE>=s)
      if ((p = alloc_fixed_size(t)))
         goto fetch_and_return;
   
malloc:
   p = malloc(s + MIN_HUNKSIZE);  // + space for info

   if (!p && collecting_child) {
      if (gc_disabled)
         warn("low memory and GC disabled\n");
      else /* try to recover */
         while (collecting_child)
            fetch_unreachables();
      goto malloc; 
   }
   if (!p)
      return NULL;

   assert(!LBITS(p));

   info_t *info = p;
   info->t = t;
   info->nh = s/MIN_HUNKSIZE;
   p = seal(p);

fetch_and_return:
#ifdef MM_SNAPSHOT_GC
   if (collecting_child)
      fetch_unreachables();
#endif
   maybe_trigger_collect(s);
   return p;
}

static bool no_marked_live(void)
{
   uintptr_t __a = 0;
   for (int b = 0; b < num_blocks; b++, __a += BLOCKSIZE) {
      uintptr_t __a_next = __a + BLOCKSIZE;
      for (uintptr_t a = __a; a < __a_next; a += MIN_HUNKSIZE) {
         if (HMAP_LIVE(a)) {
            warn("address 0x%"PRIxPTR" (in block %d) is marked live\n", PPTR(heap + a), b);
            return false;

         }
      }
   }

   for (int i = 0; i <= man_last; i++) {
      if (LIVE(managed[i])) {
         void *p = CLRPTR(managed[i]);
         warn("address 0x%"PRIxPTR" is marked live\n", PPTR(p));
         return false;
      }
   }

   return true;
}


/* 
 * Sort managed array in-place using poplar sort.
 * Information Processing Letters 39, pp 269-276, 1991.
 */

#define MAX_POPLAR 31
static int   poplar_roots[MAX_POPLAR+2] = {-1};
static bool  poplar_sorted[MAX_POPLAR];

#define M(p,q) (((p)+(q))/2)
#define A(q)   (managed[q])

#define SWAP(i,j)             \
{                             \
   void *p = managed[i];      \
   managed[i] = managed[j];   \
   managed[j] = p;            \
}

static void sift(int p, int q)
{
   if (q-p > 1) {
      int x = q;
      int m = M(p,q);
      if (A(q-1) > A(x)) x = q-1;
      if (A(m) > A(x)) x = m;

      if (x != q) {
         SWAP(x,q);
         if (x == q-1) sift(m, x); else sift(p, x);
      }
   }
}

/* increment man_k to man_last while maintaining the poplar invariant */
static void update_man_k(void)
{
   assert(!collect_in_progress);
#  define r poplar_roots

   while (man_k < man_last) {
      man_k++;
      if ((man_t>=2) && (man_k-1+r[man_t-2] == 2*r[man_t-1])) {
         r[--man_t] = man_k;
         sift(r[man_t-1], man_k);
         poplar_sorted[man_t-1] = false;
      } else {
         r[++man_t] = man_k;
         poplar_sorted[man_t-1] = true;
      }
   }
#  undef r
}

/* sort a single poplar using poplar sort ;-) */
static void sort_poplar(int n)
{
   assert(!collect_in_progress);
   
   if (poplar_sorted[n]) return;

   int r[MAX_POPLAR+1], t = 1;
   r[0] = poplar_roots[n];
   r[1] = poplar_roots[n+1];
   
   /* sort, right to left, by always taking the max root */
   int k = r[1]+1;
   while (--k > r[0]) {

      /* move maximum element to the right */
      int m = t;
      for (int j=1; j<t; j++)
         if (A(r[m]) < A(r[j])) m = j;
      
      if (m != t) {
         SWAP(r[m], r[t]);
         sift(r[m-1], r[m]);
      }
      
      if (r[t-1] == k-1) {
         t--;
      } else {
         r[t] = M(r[t-1],k);
         r[++t] = k-1;
      }
   }
   poplar_sorted[n] = true;
}

#undef A
#undef M
#undef SWAP

/* Remove obsolete entries from managed. */
static void compact_managed(void)
{
   assert(!collect_in_progress);

   if (man_is_compact) return;

   /* every poplar is sorted, so we set man_k to the last element */
   /* of the first poplar and recompute the poplar roots quickly  */
   assert(man_t > 0);
   man_k = poplar_roots[1];

   int i, n = 0;
   for (i = 0; i <= man_k; i++) {
      if (!OBSOLETE(managed[i]))
         managed[n++] = managed[i];
   }
   man_k = n-1;
   for (; i <= man_last; i++) {
      if (!OBSOLETE(managed[i]))
         managed[n++] = managed[i];
   }
   man_last = n-1;

   /* rebuild poplars */
#if 1
   {
      /* this only works when managed was sorted up to man_k */
      n = man_k + 1;
      man_t = 0;
      poplar_roots[man_t] = -1;
      int m = (1<<(MAX_POPLAR-1))-1;
      while (n) {
         if (n >= m) {
            poplar_roots[man_t+1] = poplar_roots[man_t] + m;
            poplar_sorted[man_t] = true;
            man_t++;
            n -= m;
         } else
            m = m>>1;
      }
   }
#else
   man_k = -1;
   man_t = 0;
   update_man_k();
#endif
   
   /* shrink when much bigger than necessary */
   if (man_last*4<man_size && man_size > MIN_MANAGED) {
      man_size /= 2;
      debug("shrinking managed table to %d\n", man_size);
      managed = (void *)realloc(managed, man_size*sizeof(void *));
      assert(managed);
   }
   man_is_compact = true;
}


/* binary search in sorted part of managed */
static int bsearch_managed(const void *p, int l, int r)
{
   while (r-l > 1) {
      int n = (r+l)/2;
      if (p >= managed[n])
         l = n;
      else
         r = n;
   }
   if (CLRPTR(managed[l]) == p)
      return l;

   else if (l<man_k && (CLRPTR(managed[l+1]) == p))
      return l+1;

   else 
      return -1;
}

static int _find_managed(const void *p)
{
   int i = -1;
   for (int n = 0; n<man_t && i<0; n++) {
      if (!poplar_sorted[n]) sort_poplar(n);
      i = bsearch_managed(p, poplar_roots[n]+1, poplar_roots[n+1]+1);
   }

   if (i==-1 || collect_in_progress) {
      /* do linear search on excess part */
      for (int n = man_last; n > man_k; n--)
         if (CLRPTR(managed[n]) == p) {
            i = n;
            break;
         }
   }
   return i;
}

/* use this one only when not marking */
static int find_managed(const void *p)
{
   assert(!mark_in_progress);
   int i = _find_managed(p);
   if (i>-1 && OBSOLETE(managed[i]))
      i = -1;
   return i;
}

/*
 * Add address to list of managed addresses in O(1) 
 * amortized time. Do not check for duplicates.
 */
static void add_managed(const void *p)
{
   man_last++;
   if (man_last == man_size) {
      man_size *= 2;
      debug("enlarging managed table to %d\n", man_size);
      managed = (void *)realloc(managed, man_size*sizeof(void *));
      ABORT_WHEN_OOM(managed);
   }
   assert(man_last < man_size);
   managed[man_last] = (void *)p;
}

static void manage(const void *p, mt_t t)
{
   assert(ADDRESS_VALID(p));

   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(!HMAP_MANAGED(a));
      HMAP_MARK_MANAGED(a);
   } else
      add_managed(p);
   
   stack_push(_mm_transients, p);

   if (profile)
      profile[t]++;
}

static void collect_prologue(void)
{
   assert(!collect_in_progress);
   assert(!collecting_child);
   assert(!stack_overflowed2);
   if (mm_debug_enabled)
      assert(no_marked_live());
   
   /* prepare managed array and poplar data */
   update_man_k();
   for (int n = 0; n < man_t; n++)
      sort_poplar(n);
   assert(man_k == man_last);
   
   if (cstack_ptr_high) {
      char here;
      cstack_ptr_low = &here;
      assert(cstack_ptr_low < cstack_ptr_high);
   }
   
   collect_in_progress = true;
   if (mm_debug_enabled) {
      check_num_free_blocks();
      debug("%dth collect after %d allocations:\n",
            num_collects, num_allocs);
      debug("mean alloc %.2f bytes, %d free blocks, down %d blocks)\n",
            (num_allocs ? ((double)vol_allocs)/num_allocs : 0),
            num_free_blocks, num_alloc_blocks);
   }
}

static void collect_epilogue(void)
{
   assert(collect_in_progress);

   if (stack_overflowed2) {
      if (stack_size < MAX_STACK) {
         /* only effective with synchronous collect */
         stack_size *= 2;
         debug("enlarging marking stack to %d\n", stack_size);
      }
      stack_overflowed2 = false;

   } else if (stack_size > MIN_STACK) {
      stack_size /= 2;
      debug("shrinking marking stack to %d\n", stack_size);
   }

   marking_type = mt_undefined;
   marking_object = NULL;
   collect_requested = false;

#ifdef MM_SNAPSHOT_GC
   if (collecting_child) {
      int status; 
      if (waitpid(collecting_child, &status, 0) == -1) {
         char *errmsg = strerror(errno);
         warn("waiting for collecting child failed:\n%s\n", errmsg);
         abort();
      }
      if (!WIFEXITED(status)) {
         warn("collecting child terminated abnormally:\n");
         if (WIFSIGNALED(status))
            warn("terminated by signal %d\n", WTERMSIG(status));
         else if (WIFSTOPPED(status))
            warn("stopped by signal %d\n", WSTOPSIG(status));
#  ifdef WCOREDUMP
         else if (WCOREDUMP(status))
            warn("coredumped\n");
#  endif
         else
            warn("cause unknown (status = %d)\n", status);
         abort();
      }
      //mm_printf("reaped gc child %d\n", collecting_child);
      collecting_child = 0;
   }
#endif
   heap_exhausted = false;
   collect_in_progress = false;
   num_alloc_blocks = 0;
   num_collects += 1;
   num_allocs = 0;
   vol_allocs = 0;
   fetch_backlog = 0;
   compact_managed();
}

/*
 * Push address onto marking stack and mark it if requested.
 */

static void __mm_push(const void *p)
{
   mark_live(p);
   
   stack_last++;
   if (stack_last == stack_size) {
      stack_overflowed = true;
      stack_last--;
   } else {
      stack[stack_last] = p;
   }
}

void _mm_push(const void *p)
{
   if (cstack_ptr_high)
      if (cstack_ptr_low<=p && p<=cstack_ptr_high) // ignore objects on stack
         return;
   if (live(p))
      return;
   else
      __mm_push(p);
}

static const void *pop(void)
{
   return ((stack_last<0) ? NULL : stack[stack_last--]);
}

static bool empty(void)
{
   return stack_last==-1;
}

static void recover_stack(void)
{
   if (!stack_overflowed) return;
   debug("marking stack overflowed, recovering\n");
   assert(empty());
   stack_overflowed = false;
   stack_overflowed2 = true;

   /* mark children of all live objects */
   DO_HEAP(a, b) {
      if (HMAP_LIVE(a)) {
         mark_func_t *mark = types[blockrecs[b].t].mark;
         if (mark)
            mark(heap + a);
      }
   } DO_HEAP_END;
      
   DO_MANAGED(i) {
      if (LIVE(managed[i])) {
         mt_t t  = INFO_T(managed[i]);
         if (types[t].mark)
            types[t].mark(CLRPTR(managed[i]));
      }
   } DO_MANAGED_END;
}

void _mm_check_managed(const void *p)
{
   if (!mm_ismanaged(p)) {
      char *name = (marking_type == mt_undefined) ?
         "undefined" : types[marking_type].name;
      warn("attempt to mark non-managed address\n");
      if (marking_object)
         warn(" 0x%"PRIxPTR" (%s) -> 0x%"PRIxPTR"\n", 
              PPTR(marking_object), name, PPTR(p));
      else
         warn(" 0x%"PRIxPTR"\n", PPTR(p));
      abort();
   }
}

/* trace starting from objects currently in the stack */
static void trace_from_stack(void)
{

process_stack:
   while (!empty()) {
      const void *p = marking_object = pop();
      if (cstack_ptr_high) {
         if (cstack_ptr_low<=p && p<=cstack_ptr_high) // ignore objects on stack
            continue;
      }
      mt_t t = marking_type = mm_typeof(p);
      if (types[t].mark)
         types[t].mark(p);
   }
   if (stack_overflowed) {
      recover_stack();
      goto process_stack;
   }
}

/* run finalizer, return true if q may be reclaimed */
static bool run_finalizer(finalize_func_t *f, void *q)
{
   DISABLE_GC;

   errno = 0;
   bool ok = f(q);
   if (errno) {
      char *errmsg = strerror(errno);
      warn("finalizer of object 0x%"PRIxPTR" (%s) caused an error:\n%s\n",
           PPTR(q), types[mm_typeof(q)].name, errmsg);
      /* harsh, but this is most likely a bug in the finalizers */
      abort();
   }

   ENABLE_GC;
   return ok;
}


static void reclaim_inheap(void *q)
{
   const int b = BLOCK(q);
   const mt_t t = blockrecs[b].t;
   finalize_func_t *f = types[t].finalize;
   if (f)
      if (!run_finalizer(f, q))
         return;

   { 
      const ptrdiff_t a = ((char *)q) - heap;
      assert(HMAP_MANAGED(a));
      if (HMAP_NOTIFY(a)) {
         HMAP_UNMARK_NOTIFY(a);
         client_notify((void *)q);
      }
      HMAP_UNMARK_MANAGED(a);
   }
   VALGRIND_MEMPOOL_FREE(heap, q);
   
   assert(blockrecs[b].in_use > 0);
   blockrecs[b].in_use--;      
   if (blockrecs[b].in_use == 0) {
      blockrecs[b].t = mt_undefined;
      num_free_blocks++;
      VALGRIND_DISCARD((block_t *)BLOCK_ADDR(q));
   }
   if (b < types[t].next_b)
      types[t].next_b = b;
}


static void reclaim_offheap(size_t i)
{
   assert(!OBSOLETE(managed[i]));

   void *q = CLRPTR(managed[i]);
   if (!BLOB(managed[i])) {
      info_t *info_q = unseal(q); 
      finalize_func_t *f = types[info_q->t].finalize;
      if (f)
         if (!run_finalizer(f, q))
            return;
      q = info_q;
   }

   if (NOTIFY(managed[i])) {
      UNMARK_NOTIFY(managed[i]);
      client_notify(managed[i]);
   }
   
   MARK_OBSOLETE(managed[i]);
   man_is_compact = false;

   free(q);
}


static int sweep_now(void)
{
   int n = 0;
   
   /* objects in small object heap */
   DO_HEAP(a, b) {
      if (HMAP_LIVE(a)) {
         HMAP_UNMARK_LIVE(a);
      } else {
         reclaim_inheap(heap + a);
         n++;
      }
   } DO_HEAP_END;

   /* malloc'ed objects */
   DO_MANAGED(i) {
      if (LIVE(managed[i])) {
         UNMARK_LIVE(managed[i]);
      } else {
         reclaim_offheap(i);
         n++;
      }
   } DO_MANAGED_END;

   debug("%d objects reclaimed\n", n);
   return n;
}


/*
 * The transient object stack is implemented as a linked
 * list of chunks and is itself managed.
 */

#define STACK_ELTS_PER_CHUNK  (BLOCKSIZE/sizeof(void *) - 1)

typedef struct stack_chunk {
   struct stack_chunk  *prev;
   stack_elem_t        elems[STACK_ELTS_PER_CHUNK];
} stack_chunk_t;

struct stack {
   stack_ptr_t     sp;
   stack_ptr_t     sp_min;
   stack_ptr_t     sp_max;
   stack_chunk_t  *current;
   stack_elem_t    temp;
};

#define ELT_0(c)      ((c)->elems)
#define ELT_N(c)      ((c)->elems + STACK_ELTS_PER_CHUNK)

#define CURRENT_0(st) (ELT_0((st)->current))
#define CURRENT_N(st) (ELT_N((st)->current))

#define STACK_VALID(st)  ((!(st)->sp && !(st)->current) || \
                          (CURRENT_0(st) <= (st)->sp && \
                           (st)->sp <= CURRENT_N(st)))

static void mark_stack(mmstack_t *st)
{
   assert(STACK_VALID(st));

   if (st->temp) _mm_push(st->temp);

   if (st->sp > CURRENT_0(st))
      *(st->sp - 1) = NULL;

   if (st->current) _mm_push(st->current);
}

static void mark_stack_chunk(stack_chunk_t *c)
{
   if (c->prev) _mm_push(c->prev);
   for (int i = STACK_ELTS_PER_CHUNK-1; i >= 0; i--) {
      if (c->elems[i])
         _mm_push(c->elems[i]);
      else
         break;
   }
 }

static void add_chunk(mmstack_t *st)
{
   /* We're not using mm_alloc, so we don't trigger a collection.
    * Also, stack_chunk allocation is hidden from profiling.
    */
   stack_chunk_t *c = NULL;
   if ((c = alloc_variable_sized(mt_stack_chunk, sizeof(stack_chunk_t)))) {
      ptrdiff_t a = ((char *)c) - heap;
      if (a>=0 && a<heapsize)
         HMAP_MARK_MANAGED(a);
      else
         add_managed(c);
   }
   ABORT_WHEN_OOM(c);
   memset(c, 0, sizeof(stack_chunk_t));
   c->prev = st->current;
   st->current = c;
   st->sp_min = ELT_0(c);
   st->sp = st->sp_max = ELT_N(c);
}


void _mm_pop_chunk(mmstack_t *st)
{
   if (INHEAP(st->current)) {
      /* reclaim stack chunks immediately */
      int b = BLOCK(st->current);
      HMAP_UNMARK_MANAGED(b*BLOCKSIZE);
      blockrecs[b].t = mt_undefined;
      blockrecs[b].in_use = 0;
      num_free_blocks++;
      types[mt_stack_chunk].next_b = b;
   }
   st->current = st->current->prev;
   if (!st->current) {
      warn("stack underflow\n");
      abort();
   }
   st->sp_min = st->sp = ELT_0(st->current);
   st->sp_max = ELT_N(st->current);
}


static mmstack_t *make_stack(void)
{
   DISABLE_GC;
   
   size_t s = MIN_HUNKSIZE*(1+(sizeof(mmstack_t)/MIN_HUNKSIZE));
   mmstack_t *st = alloc_variable_sized(mt_stack, s);
   ABORT_WHEN_OOM(st);
   assert(st && !INHEAP(st));
   memset(st, 0, sizeof(mmstack_t));
   add_managed(st);
   add_chunk(st);

   ENABLE_GC;
   return st;
}

static void stack_push(mmstack_t *st, stack_elem_t e)
{
   assert(e != 0);
   if (st->sp == st->sp_min) {
      st->temp = e;   // save
      add_chunk(st);  // add_chunk() might trigger a collection
      st->temp = 0;   // restore
      assert(st->sp == st->sp_max);
   }
   st->sp--;
   *(st->sp) = e;
}

static bool stack_empty(mmstack_t *st)
{
   assert(st->current);
   return (st->current->prev==NULL && 
           st->sp==st->sp_max &&
           !st->temp);
}

static int stack_depth(mmstack_t *st)
{
   int d = st->sp_max - st->sp;
   stack_chunk_t *c = st->current;
   while (c->prev) {
      d = d + STACK_ELTS_PER_CHUNK;
      c = c->prev;
   }
   return d;
}

static size_t stack_sizeof(mmstack_t *st)
{
   size_t s = sizeof(mmstack_t);
   stack_chunk_t *c = st->current;
   while (c->prev) {
      s = s + sizeof(stack_chunk_t);
      c = c->prev;
   }
   return s;
}

static stack_elem_t stack_peek(mmstack_t *st)
{
   assert(STACK_VALID(st));

   if (st->sp == st->sp_max) {
      assert(st->current->prev);
      stack_ptr_t sp = ELT_0(st->current->prev);
      return *sp;
   } else
      return *(st->sp);
}

static stack_elem_t stack_pop(mmstack_t *st)
{
   assert(STACK_VALID(st));

   if (st->sp == st->sp_max)
      _mm_pop_chunk(st);

   return *(st->sp++);
}

static inline void stack_reset(mmstack_t *st, stack_ptr_t sp)
{
   assert(STACK_VALID(st));

   while (st->sp>sp || sp>st->sp_max)
      _mm_pop_chunk(st);
   st->sp = sp;
}

static stack_elem_t stack_elt(mmstack_t *st, int i)
{
   assert(STACK_VALID(st));
   
   stack_chunk_t *c = st->current;
   stack_ptr_t sp = st->sp;
   while (sp+i >= ELT_N(c)) {
      i -= ELT_N(c) - sp;
      c = c->prev;
      if (!c) {
         debug("stack underflow\n");
         assert(c);
      }
      sp = ELT_0(c);
   }
   return sp[i];
}

/* push five chunks worth of numbers on stack,
 * index, and pop.
 */

static bool stack_works_fine(mmstack_t *st)
{
   intptr_t n = 5*STACK_ELTS_PER_CHUNK+2;
   
   assert(sizeof(stack_chunk_t) ==
          (STACK_ELTS_PER_CHUNK+1)*sizeof(stack_elem_t));
   
   for (intptr_t i = 0; i < n ; i++) {
      stack_push(st, (void *)(i+1));
   }

   /* avoid 'defined but not used' warning */
   (void)stack_peek(st);

   /* test indexing */
   for (intptr_t i = 0; i < n; i++) {
      if ((n-i) != (intptr_t)stack_elt(st, i)) {
         warn("unexpected element on stack: %" PRIdPTR " (should be %" PRIdPTR")\n",
              (intptr_t)stack_elt(st,i), (n-i));
         return false;
      }
   }

   /* test popping and stack_empty */
   while (!stack_empty(st)) {
      intptr_t i = (intptr_t)stack_pop(st);
      if (n != i) {
         warn("unexpected element on stack: %" PRIdPTR " (should be %" PRIdPTR ")\n",
                   i, n);
         return false;
      }
      n--;
      if (stack_depth(st)!=n) {
         warn("stack_depth reports wrong depth: %d (should be %" PRIdPTR ")\n",
              stack_depth(st), n);
         return false;
      }
   }
   if (n) {
      mm_printf("inconsistent element count: %" PRIdPTR " (should be 0)\n", n);
      return false;
   }
   return true;
}

/*
 * MM API
 */

#if 1
static stack_ptr_t _mm_begin_anchored(void)
{
   return _mm_transients->sp;
}

static void  _mm_end_anchored(stack_ptr_t sp)
{
   stack_reset(_mm_transients, sp);
}
#else
stack_ptr_t _mm_begin_anchored(void)
{
   return _mm_transients->sp;
}

void _mm_end_anchored(stack_ptr_t sp)
{
   stack_reset(_mm_transients, sp);
}
#endif

void mm_anchor(const void *p)
{
   if (p)
      stack_push(_mm_transients, p);
}

void mm_debug(bool e)
{
   mm_debug_enabled = e;
}


mt_t mm_regtype(const char *n, size_t s, 
                clear_func_t c, mark_func_t *m, finalize_func_t *f)
{
   if (!n || !n[0]) {
      warn("first argument invalid\n");
      abort();
   }
   if (num_profiles) {
      warn("cannot register while profiling is active\n");
      abort();
   }

   /* check if type is already registered */
   for (int t = 0; t <= types_last; t++) {
      if (0==strcmp(types[t].name, n)) {
         warn("attempt to re-register memory type\n");
         assert(s <= types[t].size);
         assert(c == types[t].clear);
         assert(m == types[t].mark);
         assert(f == types[t].finalize);
         return t;
      }
   }

   /* register new memtype */
   types_last++;
   if (types_last == types_size) {
      debug("enlarging type directory\n");
      types_size *= 2;
      types = realloc(types, types_size*sizeof(typerec_t));
      assert(types);
   }
   assert(types_last < types_size);
   assert(types_last < num_blocks);

   typerec_t *rec = &(types[types_last]);
   rec->name = strdup(n);
   VALGRIND_CHECK_MEM_IS_DEFINED(types[types_last].name, strlen(n)+1);
   rec->size = MIN_HUNKSIZE * (s/MIN_HUNKSIZE);
   while (rec->size < s)
      rec->size += MIN_HUNKSIZE;
   rec->clear = c;
   rec->mark = m;
   rec->finalize = f;
   if (rec->size > 0) {
      rec->current_a = 0;
      rec->current_amax = rec->current_a + AMAX(rec->size);
      rec->next_b = types_last;
   }
   return types_last;
}


void *mm_alloc(mt_t t)
{
   if (t == mt_undefined) {
      warn("attempt to allocate with undefined memory type\n");
      abort();
   }
   assert(TYPE_VALID(t));

   if (types[t].size==0) {
      extern void dump_types(void);
      warn("attempt to allocate variable-sized object (%s)\n\n",
           types[t].name);
      stdlog = stderr;
      dump_types();
      abort();
   }
   
   void *p = alloc_variable_sized(t, types[t].size);
   ABORT_WHEN_OOM(p);

   if (types[t].clear)
      types[t].clear(p, types[t].size);
   manage(p, t);
   return p;
}


void *mm_malloc(mt_t t, size_t s)
{
   if (t == mt_undefined) {
      warn("attempt to allocate with undefined memory type\n");
      abort();
   }
   assert(TYPE_VALID(t));
   if (s == 0) {
      warn("attempt to allocate object of size zero\n");
      abort();
   }
   
   FIX_SIZE(s);
   void *p = alloc_variable_sized(t, s);
   if (p) {
      if (types[t].clear)
         types[t].clear(p, s);
      manage(p, t);
   }
   return p;
}


void *mm_allocv(mt_t t, size_t s)
{
   void *p = mm_malloc(t, s);
   ABORT_WHEN_OOM(p);
   return p;
}


void *mm_blob(size_t s)
{
   if (s <= 256) {
      mt_t mt = mt_blob256;
      if      (s <=  8)   mt = mt_blob8;
      else if (s <= 16)   mt = mt_blob16;
      else if (s <= 32)   mt = mt_blob32;
      else if (s <= 64)   mt = mt_blob64;
      else if (s <=128)   mt = mt_blob128;
      return mm_alloc(mt);
   } else
      return mm_allocv(mt_blob, s);
}


char *mm_strdup(const char *str)
{
   char *str2 = mm_blob(strlen(str)+1);
   return strcpy(str2, str);
}


void mm_manage(const void *p)
{
   if (!ADDRESS_VALID(p)) {
      warn(" 0x%"PRIxPTR" is not a valid address\n", PPTR(p));
      abort();
   }
   if (mm_debug_enabled)
      if (mm_ismanaged(p)) {
         warn(" address 0x%"PRIxPTR" already managed\n", PPTR(p));
         abort();
      }
   add_managed(p);
   MARK_BLOB(managed[man_last]);
   mm_anchor(p);
}


void mm_notify(const void *p, bool set)
{
   assert(ADDRESS_VALID(p));

   if (!client_notify && set) {
      warn("no notification function specified\n");
      abort();
   }
   
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
         
      if (!HMAP_MANAGED(a)) {
         warn("not a managed address\n");
         abort();
      }
      if (set)
         HMAP_MARK_NOTIFY(a);
      else
         HMAP_UNMARK_NOTIFY(a);

      return;
   }

   int i = managed[man_last]==p ? man_last : find_managed(p);
   if (i<0) {
      warn("not a managed address\n");
      abort();
   }
   if (set) {
      MARK_NOTIFY(managed[i]);
   } else {
      UNMARK_NOTIFY(managed[i]);
   }
}


bool mm_ismanaged(const void *p)
{
   if (!ADDRESS_VALID(p)) {
      return false;
      
   } else {
      ptrdiff_t a = ((char *)p) - heap;
      if (a>=0 && a<heapsize)
         return HMAP_MANAGED(a);
      else if (mark_in_progress)
         return _find_managed(p) != -1;
      else {
         if (!collect_in_progress)
            update_man_k();
         return find_managed(p) != -1;
      }
   }
}


mt_t mm_typeof(const void *p)
{
   assert(ADDRESS_VALID(p));
   if (INHEAP(p))
      return blockrecs[BLOCK(p)].t;
   else {
      int i = _find_managed(p);
      assert(i>-1);
      if (BLOB(managed[i]))
         return mt_blob;
      else
         return INFO_T(managed[i]);
   }
}


size_t mm_sizeof(const void *p)
{
   assert(ADDRESS_VALID(p));
   if (INHEAP(p))
      return types[blockrecs[BLOCK(p)].t].size;
   else {
      int i = _find_managed(p);
      assert(i>-1);
      if (BLOB(managed[i]))
         return 0;
      else
         return INFO_S(managed[i]);
   }
}


void mm_root(const void *_pr)
{
   void **pr = (void **)_pr;

   /* avoid creating duplicates */
   for (int i = roots_last; i >= 0 ; i--) {
      if (roots[i] == pr) {
         debug("attempt to add existing root (ignored)\n");
         return;
      }
   }
   if ((*pr) && !mm_ismanaged(*pr)) {
      warn("root does not contain a managed address\n");
      warn("*(0x%" PRIxPTR ") = 0x%" PRIxPTR "\n", PPTR(pr), PPTR(*pr));
      abort();
   }

   roots_last++;
   if (roots_last == roots_size) {
      roots_size *= 2; 
      debug("enlarging root table to %d\n", roots_size);
      roots = realloc(roots, roots_size*sizeof(void *));
      assert(roots);
   }

   assert(roots_last < roots_size);
   roots[roots_last] = pr;
}


void mm_unroot(const void *_pr)
{
   void **pr = (void **)_pr;

   assert(roots_last>=0);
   if (roots[roots_last] == pr) {
      roots_last--;
      return;
   }

   bool r_found = false;
   for (int i = 0; i < roots_last; i++) {
      if (roots[i] == pr)
         r_found = true;
      if (r_found)
         roots[i] = roots[i+1];
   }
   
   if (r_found)
      roots_last--;
   else
      warn("attempt to unroot non-existing root\n");
}

#define WITH_TEMP_STORAGE   { \
   const void *marking_stack[stack_size]; \
   stack = marking_stack;

#define TEMP_STORAGE  \
   stack = NULL; }

static void mark(void)
{
   mark_in_progress = true;

   /* Trace live objects from root objects */
   for (int r = 0; r <= roots_last; r++) {
      if (*roots[r]) {
         if (!mm_ismanaged(*roots[r])) {
            warn("root at 0x%" PRIxPTR " is not a managed address\n",
                 PPTR(roots[r]));
            abort();
         }
         if (*roots[r]) __mm_push(*roots[r]);
      }
   }
   trace_from_stack();

   /* Mark dependencies of finalization-enabled objects */
   DO_HEAP (a, b) {
      finalize_func_t *finalize = types[blockrecs[b].t].finalize;
      mark_func_t *mark = types[blockrecs[b].t].mark;
      if (!HMAP_LIVE(a) && finalize) {
         void *p = heap + a;
         if (mark) mark(p);
         trace_from_stack();
         HMAP_UNMARK_LIVE(a);  /* break cycles */
      }
   } DO_HEAP_END;
   
   DO_MANAGED (i) {
      mt_t t = INFO_T(managed[i]);
      finalize_func_t *finalize = types[t].finalize;
      mark_func_t *mark = types[t].mark;
      if (!LIVE(managed[i]) && finalize) {
         if (mark) mark(CLRPTR(managed[i]));
         trace_from_stack();
         UNMARK_LIVE(managed[i]); /* break cycles */
      }
   } DO_MANAGED_END;

   assert(empty());
   mark_in_progress = false;
}

#ifdef MM_SNAPSHOT_GC

static int pfd_garbage[2];  /* garbage pipe for async. collect */

static int write_buf(int fd, const void *buf_, size_t n)
{
   const char *buf = buf_;
   size_t pos = 0;
   while (pos < n) {
      errno = 0;
      ssize_t w = write(fd, &(buf[pos]), n-pos);
      if (w == -1) {
         if (errno != EAGAIN)
            return errno;
      } else
         pos += w;
   }
   return 0;
}

static void abort_with_message_file(const char *msg)
{
   pid_t pid = getpid();
   pid_t ppid = getppid();
   char buf[1024];
   sprintf(buf, "libcmm.%d.msg", (int)ppid);

   int fd = open(buf, O_WRONLY | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP);
   if (fd != -1) {
      sprintf(buf, "libcmm: GC child (pid = %d) aborted for the following reason:\n%s\n",
              pid, msg);
      ssize_t w = write(fd, buf, strlen(buf));
      if (w) close(fd);
   }
   abort();
}

static void sweep(void)
{
   void *buf[NUM_TRANSFER];
   assert(!collecting_child);

   int n = 0;

   /* objects in small object heap */
   DO_HEAP(a, b) {
      if (!HMAP_LIVE(a)) {
         if (blockrecs[b].t == mt_stack_chunk) /* ignore stack chunks */
            continue;
         buf[n++] = (void *)(heap + a);
         if (n == NUM_TRANSFER) {
            int errno_ = write_buf(pfd_garbage[1], &buf, NUM_TRANSFER*sizeof(void *));
            if (errno_)
               abort_with_message_file(strerror(errno_));
            n = 0;
         }
      }
   } DO_HEAP_END;

   buf[n++] = NULL;
   if (n == NUM_TRANSFER) {
      int errno_ = write_buf(pfd_garbage[1], &buf, NUM_TRANSFER*sizeof(void *));
      if (errno_)
         abort_with_message_file(strerror(errno_));
      n = 0;
   }

   /* malloc'ed objects */
   DO_MANAGED(i) {
      if (!LIVE(managed[i])) {
         buf[n++] = (void *)i;
         if (n == NUM_TRANSFER) {
            int errno_ = write_buf(pfd_garbage[1], &buf, NUM_TRANSFER*sizeof(void *));
            if (errno_)
               abort_with_message_file(strerror(errno_));
            n = 0;
         }
      }
   } DO_MANAGED_END;
   
   if (n) {
      int errno_ = write_buf(pfd_garbage[1], &buf, n*sizeof(void *));
      if (errno_)
         abort_with_message_file(strerror(errno_));
   }
}


/* fill buffer */
static int _fetch_unreachables(void *buf)
{
   errno = 0;
   int n = read(pfd_garbage[0], buf, NUM_TRANSFER*sizeof(void *));

   if (n == -1) {
      if (errno != EAGAIN) {
         int errno_ = errno;
         warn("error reading from garbage pipe:\n%s\n", strerror(errno_));
         abort();
      }
      
   } else if (n == 0) {
      /* we're done */
      close(pfd_garbage[0]);
      collect_epilogue();
   }

   return n;
}

/* fetch addresses of unreachable objects from garbage pipe */
/* return number of objects reclaimed */
static int fetch_unreachables(void)
{
   if (gc_disabled) {
      fetch_backlog++;
      return 0;
   }

   static void *buf[NUM_TRANSFER];
   static int j, n = 0;
   static bool inheap = true;

   if (n) {
      /* don't need to disable gc here as gc is still in progress */
      if (inheap) {
         if (buf[j])
            reclaim_inheap(buf[j]);
         else
            inheap = false;

         if (++j == n) {
            n = 0;
            return 1;
         }
         
         if (inheap) {
            if (buf[j])
               reclaim_inheap(buf[j]);
            else
               inheap = false;

            if (++j == n)
               n = 0;
            return 2;
         }

      } else {
         reclaim_offheap((size_t)buf[j++]);
         if (j == n) {
            n = 0;
            return 1;
         }
         reclaim_offheap((size_t)buf[j++]);
         if (j == n) {
            n = 0;
         }
         return 2;
      }

   } else if (n == 0) {

      n = _fetch_unreachables(&buf);
      
      if (n == -1) {
         n = 0;

      } else if (n == 0) {
         inheap = true;

      } else {
         assert((n%sizeof(void *)) == 0);
         n = n/sizeof(void *);
         j = 0;
      }
   }
   return 0;
}

/* close all file descriptors except essential ones */
static void close_file_descriptors(void)
{
   assert(collect_in_progress && collecting_child==0);

   char path[128];
   sprintf(path, "/proc/%d/fd", (int)getpid());
   
   errno = 0;
   DIR *d = opendir(path);
   if (!d) {
      char *errmsg = strerror(errno);
      warn("could not open %s: %s\n", path, errmsg);
      abort();
   }
   
   int fd_err = fileno(stderr);
   int fd_log = fileno(stdlog);
   int fd_parent = pfd_garbage[1];
   errno = 0;
   struct dirent *e = readdir(d);
   while (e != NULL) {
      int fd = isdigit(e->d_name[0]) ? atoi(e->d_name) : -1;
      if (!(fd==-1 || fd==fd_err || fd==fd_log || fd==fd_parent))
         (void)close(fd); /* ignore close errors */
      errno = 0;
      e = readdir(d);
   }
   if (errno) {
      char *errmsg = strerror(errno);
      debug("error while closing file descriptors:\n%s\n", errmsg);
      /* don't abort, hopefully nothing serious */
   }
   closedir(d);
}


static void collect(void)
{
   /* set up pipe for garbage */
   errno = 0;
   if (pipe(pfd_garbage) == -1) {
      char *errmsg = strerror(errno);
      warn("could not set up pipe for GC: %s\n", errmsg);
      return;
   }

   collect_prologue();

   /* fork */
   assert(!collecting_child);
   fflush(stdout);
   if (stdlog) fflush(stdlog);
   errno = 0;
   collecting_child = fork();
   if (collecting_child == -1) {
      char *errmsg = strerror(errno);
      warn("could not spawn child for GC (%s)\n", errmsg);
      warn("trying synchronous collect\n"); /* in maybe_trigger_collect */
      //fprintf(stderr, "could not spawn child for GC: %s\n", errmsg);
      close(pfd_garbage[0]);
      close(pfd_garbage[1]);
      collecting_child = 0;
      collect_in_progress = false;
      return;

   } else if (collecting_child) {
      //mm_printf("spawned gc child %d\n", collecting_child);
      close(pfd_garbage[1]);
      int flags = fcntl(pfd_garbage[0], F_GETFL);
      fcntl(pfd_garbage[0], F_SETFL, flags | O_NONBLOCK);
      return;

   } else {
      /* first thing, change the signal set up */
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sigset_t *mask = &(sa.sa_mask);
      assert(sigfillset(mask) != -1);
      assert(sigprocmask(SIG_SETMASK, mask, NULL) != -1);

      sa.sa_handler = SIG_IGN;
      assert(sigaction(SIGHUP,  &sa, NULL) != -1);
      assert(sigaction(SIGINT,  &sa, NULL) != -1);
      assert(sigaction(SIGQUIT, &sa, NULL) != -1);
      assert(sigaction(SIGPIPE, &sa, NULL) != -1);
      assert(sigaction(SIGALRM, &sa, NULL) != -1);
      assert(sigaction(SIGPWR,  &sa, NULL) != -1);
      assert(sigaction(SIGURG,  &sa, NULL) != -1);
      assert(sigaction(SIGPOLL, &sa, NULL) != -1);

      sa.sa_handler = SIG_DFL;
      assert(sigaction(SIGTERM, &sa, NULL) != -1);
      assert(sigaction(SIGBUS,  &sa, NULL) != -1);
      assert(sigaction(SIGFPE,  &sa, NULL) != -1);
      assert(sigaction(SIGILL,  &sa, NULL) != -1);
      assert(sigaction(SIGSEGV, &sa, NULL) != -1);
      assert(sigaction(SIGSYS,  &sa, NULL) != -1);
      assert(sigaction(SIGXCPU, &sa, NULL) != -1);
      assert(sigaction(SIGXFSZ, &sa, NULL) != -1);
      
      assert(sigemptyset(mask) != -1);
      sigprocmask(SIG_SETMASK, mask, NULL);

      /* second thing, close unused file descriptors */
      close_file_descriptors();
   }

   WITH_TEMP_STORAGE {
      mark();
      sweep();
   } TEMP_STORAGE;

   close(pfd_garbage[1]);
   if (stdlog) fclose(stdlog);
   _exit(0);
}


static void mm_collect(void)
{
   if (gc_disabled || collect_in_progress) {
      collect_requested = true;
      return;

   } else
      collect();
}

#endif  /* MM_SNAPSHOT_GC */

int mm_collect_now(void)
{
   if (gc_disabled) {
      collect_requested = true;
      return 0;
   } 
#ifdef MM_SNAPSHOT_GC   
   else if (collect_in_progress) {
      int n = 0;
      while (collect_in_progress)
         n += fetch_unreachables();
      return n;
   }
#else
   assert(!collect_in_progress);
#endif

   collect_prologue();

   int n = 0;
   WITH_TEMP_STORAGE {
      mark();
      n = sweep_now();
   } TEMP_STORAGE;

   collect_epilogue();
   return n;
}


bool mm_collect_in_progress(void)
{
   return collect_in_progress;
}


bool mm_idle(void)
{
   static int ncalls = 0;

   if (collect_in_progress) {
      if (!gc_disabled) {
         int i = 100 + fetch_backlog;
         while (i>0 && collect_in_progress) {
            fetch_unreachables();
            i--;
         }
         return true;
      } else
         return false;

   } else {
      ncalls++;
      if (!collect_in_progress)
         update_man_k();
      if (ncalls<NUM_IDLE_CALLS) {
         return false;
      }
      /* create some work for ourselves */
#ifdef MM_SNAPSHOT_GC
      mm_collect();
#else
      mm_collect_now();
#endif
      ncalls = 0;
      return true;
   }
}


bool mm_begin_nogc(bool dont_block)
{
   /* block when a collect is in progress */
   if (!dont_block) {
#ifdef MM_SNAPSHOT_GC
      if (collecting_child && gc_disabled) {
         warn("deadlock (MM_NOGC while pending GC paused)\n");
         abort();
      }
      while (collecting_child)
         fetch_unreachables();
#endif
      assert(!collect_in_progress);
   }

   bool nogc = gc_disabled;
   gc_disabled = true;
   return nogc;
}


void mm_end_nogc(bool nogc)
{
   gc_disabled = nogc;
#ifdef MM_SNAPSHOT_GC
   if (collect_in_progress && !gc_disabled && fetch_backlog) {
      while (collect_in_progress && fetch_backlog>0)
         fetch_backlog -= fetch_unreachables();
   } 
#endif
   if (collect_requested && !gc_disabled) {
      mm_collect();
      if (!collect_in_progress)
         /* could not spawn child, do it synchronously */
         mm_collect_now();
   }
}

static void mark_refs(void **p)
{
   int n = mm_sizeof(p)/sizeof(void *);
   for (int i = 0; i < n; i++)
      if (p[i]) _mm_push(p[i]);
}

static void clear_refs(void **p, size_t s)
{
   memset(p, 0, s);
}

void mm_record_cstack_ptr(void)
{
   char here;
   cstack_ptr_high = &here;
}

void mm_init(int npages, notify_func_t *clnotify, FILE *log)
{
   assert(sizeof(info_t) <= MIN_HUNKSIZE);
   assert(sizeof(hunk_t) <= MIN_HUNKSIZE);
   assert((1<<HMAP_EPI_BITS) == HMAP_EPI);
   assert(sizeof(stack_chunk_t) == BLOCKSIZE);

   client_notify = clnotify;
   
   if (log) {
      mm_debug_enabled = true;
      stdlog = log;
   } else {
      mm_debug_enabled = false;
      stdlog = stderr;
   }

   if (heap) {
      warn("mm is already initialized\n");
      return;
   }

   /* allocate small-object heap */
   num_blocks = max((PAGESIZE*npages)/BLOCKSIZE, MIN_NUMBLOCKS);
   num_free_blocks = num_blocks;
   heapsize = num_blocks * BLOCKSIZE;
   hmapsize = (heapsize/MIN_HUNKSIZE)/HMAP_EPI;
   block_threshold = min(MAX_BLOCKS, num_blocks/3);
   volume_threshold = min(MAX_VOLUME, heapsize/2);
#ifdef MM_SNAPSHOT_GC
   block_threshold -= (block_threshold/3);
#endif

   blockrecs = (blockrec_t *)malloc(num_blocks * sizeof(blockrec_t));
   assert(blockrecs);
   heap = (char *) malloc(heapsize + PAGESIZE);
   VALGRIND_MAKE_MEM_NOACCESS(heap, heapsize);
   if (!heap) {
      warn("could not allocate heap\n");
      abort();
   }

   /* make sure heap is PAGESIZE-aligned */
   heap = (char *)(((uintptr_t)(heap + PAGESIZE-1)) & ~(PAGESIZE-1));
   VALGRIND_CREATE_MEMPOOL(heap, 0, 0);

   hmap = calloc(hmapsize, sizeof(unsigned int));
   if (!hmap) {
      warn("could not allocate heap map\n");
      abort();
   }

   debug("heapsize  : %6"PRIdPTR" KByte (%d %d KByte blocks)\n",
         (heapsize/(1<<10)), num_blocks, BLOCKSIZE/(1<<10));
   debug("hmapsize  : %6"PRIdPTR" KByte\n", (hmapsize*sizeof(int))/(1<<10));
   debug("threshold : %6"PRIdPTR" KByte\n", volume_threshold/(1<<10));

   /* initialize block records */
   for (int i = 0; i < num_blocks; i++) {
      blockrecs[i].t = mt_undefined;
      blockrecs[i].in_use = 0;
   }
   assert(no_marked_live());

   /* set up type directory */
   types = (typerec_t *) malloc(MIN_TYPES * sizeof(typerec_t));
   assert(types);
   types_size = MIN_TYPES;
   {
      /* register internal and pre-defined types */
      mt_t mt;
      mt_stack = MM_REGTYPE("mm_stack", sizeof(mmstack_t), 0, mark_stack, 0);
      assert(mt_stack == 0);
      mt_stack_chunk = MM_REGTYPE("mm_stack_chunk", sizeof(stack_chunk_t),
                                  clear_refs, mark_stack_chunk, 0);
      assert(mt_stack_chunk == 1);
      mt = MM_REGTYPE("blob8", 8, 0, 0, 0);
      assert(mt == mt_blob8);
      mt = MM_REGTYPE("blob16", 16, 0, 0, 0);
      assert(mt == mt_blob16);
      mt = MM_REGTYPE("blob32", 32, 0, 0, 0);
      assert(mt == mt_blob32);
      mt = MM_REGTYPE("blob64", 64, 0, 0, 0);
      assert(mt == mt_blob64);
      mt = MM_REGTYPE("blob128", 128, 0, 0, 0);
      assert(mt == mt_blob128);
      mt = MM_REGTYPE("blob256", 256, 0, 0, 0);
      assert(mt == mt_blob256);
      mt = MM_REGTYPE("blob", 0, 0, 0, 0);
      assert(mt == mt_blob);
      mt = MM_REGTYPE("refs", 0, clear_refs, mark_refs, 0);
      assert(mt == mt_refs);
   }
   assert(types_last == mt_refs);

   /* set up other bookkeeping structures */
   managed = (void *)malloc(MIN_MANAGED * sizeof(void *));
   assert(managed);
   man_size = MIN_MANAGED;

   roots = malloc(MIN_ROOTS * sizeof(void *));
   assert(roots);
   roots_size = MIN_ROOTS;

   /* set up transient object stack */
   *(mmstack_t **)&_mm_transients = make_stack();
   MM_ROOT(_mm_transients);
   assert(stack_works_fine(_mm_transients));
   assert(stack_empty(_mm_transients));

   debug("done\n");
}

/* print diagnostic info to string */
char *mm_info(int level)
{
#define PRINTBUFLEN 20000
#define BPRINTF(...) {               \
   sprintf(buf, __VA_ARGS__);        \
   buf += strlen(buf);               \
   assert((buf-buffer)<PRINTBUFLEN);\
   }

   char buffer[PRINTBUFLEN], *buf = buffer;
   
   if (level<=0)
      return NULL;

   int    total_blocks_in_use = 0;
   size_t total_memory_managed = 0;
   size_t total_memory_used_by_mm = 0;
   size_t total_objects_inheap = 0;
   size_t total_objects_offheap = 0;
   size_t total_objects_per_type_ih[types_size];
   size_t total_objects_per_type_oh[types_size];
   memset(&total_objects_per_type_ih, 0, types_size*sizeof(size_t));
   memset(&total_objects_per_type_oh, 0, types_size*sizeof(size_t));

   for (int i=0; i<num_blocks; i++)
      if (blockrecs[i].in_use)
         total_blocks_in_use++;
   
   BPRINTF("Small object heap: %.2f MByte in %d blocks (%d used)\n",
           ((double)heapsize)/(1<<20), num_blocks, total_blocks_in_use);

   DO_HEAP(a, b) {
      total_objects_inheap++;
      mt_t t = blockrecs[b].t;
      total_objects_per_type_ih[t]++;
      total_memory_managed += types[t].size;
   } DO_HEAP_END;

   if (!collect_in_progress)
      update_man_k();

   DO_MANAGED(i) {
      total_objects_offheap++;
      mt_t t  = INFO_T(managed[i]);
      total_objects_per_type_oh[t]++;
      total_memory_managed += mm_sizeof(CLRPTR(managed[i]));
      total_memory_used_by_mm += BLOB(managed[i]) ? 0 : MIN_HUNKSIZE;
   } DO_MANAGED_END;

   BPRINTF("Managed memory   : %.2f MByte in %"PRIdPTR" + %"PRIdPTR" objects\n",
           ((double)total_memory_managed)/(1<<20),
           total_objects_inheap, total_objects_offheap);

   total_memory_used_by_mm += hmapsize;
   total_memory_used_by_mm += man_size*sizeof(managed[0]);
   total_memory_used_by_mm += types_size*sizeof(typerec_t);
   total_memory_used_by_mm += num_blocks*sizeof(blockrec_t);
   total_memory_used_by_mm += stack_sizeof(_mm_transients);
   BPRINTF("Memory used by MM: %.2f MByte total\n",
           ((double)total_memory_used_by_mm)/(1<<20));
   if (level>2) {
      BPRINTF("                 : %.2f MByte for inheap array\n",
              ((double)hmapsize)/(1<<20));
      BPRINTF("                 : %.2f MByte for offheap array\n",
              ((double)man_size*sizeof(managed[0]))/(1<<20));
   }
   if (gc_disabled)
      BPRINTF("!!! Garbage collection is disabled !!!\n");
   
   if (level<=1)
      return mm_strdup(buffer);

   BPRINTF("Page size        : %d bytes\n", PAGESIZE);
   BPRINTF("Block size       : %d bytes\n", BLOCKSIZE);
   BPRINTF("GC threshold     : %d blocks / %.2f MByte\n", 
           block_threshold, (double)volume_threshold/(1<<20));
   if (mm_debug_enabled) {
      BPRINTF("Debug code       : enabled\n");
   } else {
      BPRINTF("Debug code       : disabled\n");
   }

   if (collect_in_progress)
      BPRINTF("*** GC in progress ***\n");

   int active_roots = 0;
   for (int i = 0; i<=roots_last; i++)
      if (*roots[i])
         active_roots++;

   BPRINTF("Memory roots     : %d total, %d active\n", roots_last+1, active_roots);
   BPRINTF("Transient stack  : %d objects\n", stack_depth(_mm_transients));
   if (level<=2)
      return mm_strdup(buffer);

   BPRINTF("\n");
   BPRINTF(" Memory type    | size  | # inheap (blocks) | # malloced \n");
   BPRINTF("---------------------------------------------------------\n");
   
   for (int t = 0; t <= types_last; t++) {
      int n = 0;
      for (int b = 0; b < num_blocks; b++)
         if (blockrecs[b].t == t)
            n++;
      BPRINTF(" %14s | %5"PRIdPTR" | %7"PRIdPTR"  (%6d) |    %7"PRIdPTR" \n",
              types[t].name,
              types[t].size,
              total_objects_per_type_ih[t], n,
              total_objects_per_type_oh[t]);
   }
   return mm_strdup(buffer);
}

/* initialize profile array */
int mm_prof_start(int *h)
{
   int n = types_last+1;

   if (!h)
      return n;

   if (!profile) {
      profile = malloc(n*sizeof(int));
      ABORT_WHEN_OOM(profile);
      memset(profile, 0, n*sizeof(int));
   }

   /* initialize h */
   memcpy(h, profile, n*sizeof(int));
   num_profiles++;
   return num_profiles;
}

/* update profile array */
void mm_prof_stop(int *h)
{
   if (!num_profiles) {
      warn("invalid call to mm_prof_stop ignored\n");
      return;
   }

   for (int i=0; i<=types_last; i++)
      h[i] = profile[i]-h[i];

   /* stop profiling if this closes the outermost session */
   if (--num_profiles==0) {
      free(profile);
      profile = NULL;
   }
}

/* create key for profile data */
char **mm_prof_key(void)
{
   char **k = mm_allocv(mt_refs, sizeof(void *)*(types_last+1));
   {
      const void **sp = _mm_begin_anchored();
      for (int i=0; i<=types_last; i++)
         k[i] = mm_strdup(types[i].name);
      _mm_end_anchored(sp);
   }
   return k;
}


/*
 * Debugging aides
 */

/* dump all of type t or all if t = mt_undefined */
void dump_managed(mt_t t)
{
   mm_printf("Dumping managed list (%d of %d in poplars)...\n",
             man_k+1, man_last+1);
   for (int i = 0; i <= man_last; i++) {
      mt_t ti = mm_typeof(CLRPTR(managed[i]));
      if (t==mt_undefined || t==ti)
         mm_printf("%4d : %16"PRIxPTR", %1s%1s %20s\n", 
                   i, PPTR(CLRPTR(managed[i])),
                   OBSOLETE(managed[i]) ? "o" : " ",
                   NOTIFY(managed[i]) ? "n" : " ",
                   types[ti].name);
   }
   mm_printf("\n");
   fflush(stdlog);
}

void dump_types(void)
{
   mm_printf("Dumping type registry (%d types)...\n", types_last+1);
   for (int t = 0; t <= types_last; t++) {
      int n = 0;
      for (int b = 0; b < num_blocks; b++)
         if (blockrecs[b].t == t)
            n++;
      mm_printf("%3d: %15s  %4"PRIdPTR" 0x%"PRIxPTR" 0x%"PRIxPTR"  0x%"PRIxPTR" (%d in freelist)\n",
                t,
                types[t].name,
                types[t].size,
                PPTR(types[t].clear),
                PPTR(types[t].mark),
                PPTR(types[t].finalize),
                n);
   }

   mm_printf("\n");
   fflush(stdlog);
}

void dump_roots(void)
{
   mm_printf("Dumping roots...\n");
   for (int r = 0; r <= roots_last; r++)
      mm_printf(" loc 0x%"PRIxPTR" --> 0x%"PRIxPTR"\n", 
                PPTR(roots[r]), PPTR(*(roots[r])));

   mm_printf("\n");
   fflush(stdlog);
}

void dump_stack(mmstack_t *st)
{
   mm_printf("Dumping %s stack...\n",
             st == _mm_transients ? "transient" : "finalizer");
   if (st->temp)
      mm_printf("temp = 0x%"PRIxPTR"\n\n", PPTR(st->temp));

   int n = stack_depth(st);
   for (int i = 0; i < n; i++)
      mm_printf("%3d : 0x%"PRIxPTR"\n", i, PPTR(stack_elt(st,i)));
   
   mm_printf("\n");
   fflush(stdlog);
}

void dump_stack_depth(void)
{
   mm_printf("depth of transients stack: %d\n", stack_depth(_mm_transients));
   mm_printf("\n");
   fflush(stdlog);
}

void dump_heap_stats(void)
{
   int counts[types_size];
   memset(counts, 0, sizeof(counts));
   int num_ih = 0;

   for (int i = 0; i<=man_last; i++) {
      counts[mm_typeof(CLRPTR(managed[i]))]++;
      if (INHEAP(managed[i])) num_ih++;
   }

   mm_printf("\n");
   for (int t = 0; t <= types_last; t++)
      mm_printf(" [%3d] %15s : %8d\n",
                t, types[t].name, counts[t]);
   mm_printf(" total : %d, in heap %d\n\n", man_last+1, num_ih);
}

/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
