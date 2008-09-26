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
#define _XOPEN_SOURCE    500

#include "mm.h"

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
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>

#define max(x,y)        ((x)<(y) ? (y) : (x))
#define min(x,y)        ((x)<(y) ? (x) : (y))
#define mm_printf(...)  fprintf(stdlog, __VA_ARGS__)
#define debug(...)      mm_debug ? (\
        fprintf(stdlog, "mm(%s): ", __func__), \
        fprintf(stdlog, __VA_ARGS__), 0) : 1

#define warn(...)      \
        fprintf(stderr, "mm(%s): ", __func__), \
        fprintf(stderr, __VA_ARGS__), fflush(stderr), 0

#define PPTR(p)         ((uintptr_t)(p))

#ifndef PIPE_BUF
#  define PIPE_BUF      512
#endif

#define PAGEBITS        12
#define BLOCKBITS       (PAGEBITS+1)
#if defined  PAGESIZE && (PAGESIZE != (1<<PAGEBITS))
#  error "definitions related to PAGESIZE need to be updated"
#elif !defined PAGESIZE
#  define PAGESIZE      (1<<PAGEBITS)
#endif
#define BLOCKSIZE       (1<<BLOCKBITS)

#define ALIGN_NUM_BITS  3
#define MIN_HUNKSIZE    (1<<ALIGN_NUM_BITS)
#define MIN_STRING      MIN_HUNKSIZE
#define MIN_NUMBLOCKS   0x100
#define MIN_TYPES       0x100
#define MIN_MANAGED     0x40000
#define MIN_ROOTS       0x100
#define MIN_STACK       0x1000
#define MAX_VOLUME      0x100000    /* max volume threshold */
#define MAN_K_UPDS      100
#define NUM_TRANSFER    (PIPE_BUF/sizeof(void *))

#define MM_SIZE_MAX     (UINT32_MAX*MIN_HUNKSIZE) /* checked in alloc_variable_sized */

#define HMAP_NUM_BITS   4

#if !defined INT_MAX
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
static int        num_blocks;
static blockrec_t *blockrecs = NULL;
static char      *heap = NULL;
static unsigned int *restrict hmap = NULL; /* bits for heap objects */

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

/* the marking stack */
static const void * *restrict stack = NULL;
static int        stack_size = MIN_STACK;
static int        stack_last = -1;
static bool       stack_overflowed  = false;
static bool       stack_overflowed2 = false;

/* other state variables */
static int        num_allocs = 0;
static int        num_collects = 0;
static size_t     vol_allocs = 0;
static bool       gc_disabled = false;
static bool       collect_in_progress = false;
static bool       mark_in_progress = false;
static bool       collect_requested = false;
static pid_t      collecting_child = 0;
static bool       mm_debug = false;
static notify_func_t *client_notify = NULL;
static mt_t       marking_type = mt_undefined;
static const void *marking_object = NULL;
static FILE *     stdlog = NULL;
static int        pfd_garbage[2]; /* garbage pipe for async. collect */

/* transient object stack */
typedef const void    *stack_elem_t; 
typedef stack_elem_t  *stack_ptr_t;
typedef struct stack   mmstack_t;      /* stack_t define in sigstack.h */

static mmstack_t   *make_stack(void);
static void         stack_push(mmstack_t *, stack_elem_t);
static stack_elem_t stack_peek(mmstack_t *);
static stack_elem_t stack_pop(mmstack_t *);
static bool         stack_empty(mmstack_t *);
static void         stack_reset(mmstack_t *, stack_ptr_t);
static stack_elem_t stack_elt(mmstack_t *, int);

static mt_t       mt_stack;
static mt_t       mt_stack_chunk;
static mmstack_t *const transients;
static bool       anchor_transients = true;

/*
 * MM's little helpers
 */

#define BITL               1
#define BITN               2
#define BITO               4
#define BITM               8
#define MARKED_LIVE(p)     ((void *)((uintptr_t)(p) | BITL))
#define MARK_LIVE(p)       { p = MARKED_LIVE(p); }
#define UNMARK_LIVE(p)     { p = (void *)((uintptr_t)(p) & ~BITL); }
#define MARKED_NOTIFY(p)   ((void *)((uintptr_t)(p) | BITN))
#define MARK_NOTIFY(p)     { p = MARKED_NOTIFY(p); }
#define UNMARK_NOTIFY(p)   { p = (void *)((uintptr_t)(p) & ~BITN); }
#define MARKED_OBSOLETE(p) ((void *)((uintptr_t)(p) | BITO))
#define MARK_OBSOLETE(p)   { p = MARKED_OBSOLETE(p); }
#define UNMARK_OBSOLETE(p) { p = (void *)((uintptr_t)(p) & ~BITO); }

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
#define CLRPTR2(p)         ((void *)((((uintptr_t)(p)) & ~3)))
#define ADDRESS_VALID(p)   (!LBITS(p))
#define TYPE_VALID(t)      ((t)>=0 && (t)<=types_last)
#define INDEX_VALID(i)     ((i)>=0 && (i)<=man_last)

#define LIVE(p)            (((uintptr_t)(p)) & BITL)
#define NOTIFY(p)          (((uintptr_t)(p)) & BITN)
#define OBSOLETE(p)        (((uintptr_t)(p)) & BITO)

#define INHEAP(p)          (((char *)(p) >= heap) && ((char *)(p) < (heap + heapsize)))
#define BLOCK(p)           (((ptrdiff_t)((char *)(p) - heap))>>BLOCKBITS)
#define BLOCK_ADDR(p)      (heap + BLOCKSIZE*BLOCK(p))
#define BLOCKA(a)          (((uintptr_t)((char *)(a)))>>BLOCKBITS)
#define INFO_S(p)          (((info_t *)(unseal((char *)p)))->nh*MIN_HUNKSIZE)
#define INFO_T(p)          (((info_t *)(unseal((char *)p)))->t)
#define MM_SIZEOF(p)       \
   (INHEAP(p) ? types[blockrecs[BLOCK(p)].t].size : INFO_S(p))
#define MM_TYPEOF(p)       \
   (INHEAP(p) ? blockrecs[BLOCK(p)].t : INFO_T(p))

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
/* a cleared address (use CLRPTR2 to clear)             */ 
#define DO_MANAGED(i) { \
  int __lasti = collect_in_progress ? man_k : man_last; \
  for (int i = 0; i <= __lasti; i++) { \
     if (!OBSOLETE(managed[i])) {

#define DO_MANAGED_END }}} 

#define DISABLE_GC \
   bool __nogc = gc_disabled; \
   gc_disabled = true;

#define ENABLE_GC gc_disabled = __nogc;

static int find_managed(const void *p);

static bool obsolete(const void *p)
{
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      return !HMAP_MANAGED(a);
   }
   int i = find_managed(MARKED_OBSOLETE(p));
   return i != -1;
}

/*
static bool isroot(const void *p)
{
   for (int i = 0; i <= roots_last; i++)
      if (*roots[i]==p)
         return true;
   return false;
}
*/

static inline void *seal(const char *p)
{
   assert(!LBITS(p));
   VALGRIND_MAKE_MEM_NOACCESS(p, MIN_HUNKSIZE);
   p += MIN_HUNKSIZE;
   return (void *)p;
}

static inline void *unseal(const char *p)
{
   assert(!LBITS(p));
   p -= MIN_HUNKSIZE;
   VALGRIND_MAKE_MEM_DEFINED(p, MIN_HUNKSIZE);
   return (void *)p;
}

static inline finalize_func_t *finalizeof(const void *p)
{
   assert(ADDRESS_VALID(p));
   mt_t t = MM_TYPEOF(p);
   return types[t].finalize;
}

static int num_free_blocks(void)
{
   int n = 0;
   for (int b=0; b < num_blocks; b++)
     if (blockrecs[b].t == mt_undefined)
        n++;
   return n;
}

static inline bool live(const void *p)
{
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(HMAP_MANAGED(a));
      return HMAP_LIVE(a);
   }
   int i = find_managed(p);
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
   int i = find_managed(p);
   assert(i != -1);
   MARK_LIVE(managed[i]);
}

static void maybe_trigger_collect(size_t s)
{
   num_allocs += 1;
   vol_allocs += s;
   if (gc_disabled || collect_in_progress)
      return;

   if ((vol_allocs >= volume_threshold) || collect_requested) {
      mm_collect();
      if (!collect_in_progress)
         /* could not spawn child, do it synchronously */
         mm_collect_now();
      num_allocs = 0;
      vol_allocs = 0;
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
   while (HMAP_MANAGED(a) && a <= tr->current_amax)
      a += s;
   if (a <= tr->current_amax) {
      tr->current_a = a;
      return true;
   }

 search_for_block:
   /* search for another block with free hunks */
   ;
   int b = tr->next_b;
   int orig_b = (b-1) % num_blocks;

   while (b != orig_b) {
      if (blockrecs[b].t == mt_undefined) {
         assert(blockrecs[b].in_use == 0);
         blockrecs[b].t = t;
         VALGRIND_CREATE_BLOCK(heap + a, BLOCKSIZE, types[t].name);
         tr->current_a = a = b*BLOCKSIZE;
         tr->current_amax = a + AMAX(s);
         tr->next_b = b + 1;
         return true;

      } else if (blockrecs[b].t==t && blockrecs[b].in_use<(BLOCKSIZE/s)) {
         a = b*BLOCKSIZE;
         tr->current_amax = a + AMAX(s);
         tr->next_b = b + 1;
         goto search_in_block;
      }
      b = (b+1) % num_blocks;
   }

   /* no free hunk found */
   assert(b == orig_b);
   tr->current_a = 0;
   tr->current_amax = AMAX(s);
   tr->next_b = 1;
   return false;
}

static inline bool update_current_a(mt_t t)
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
   
static int fetch_unreachables(void);

/* allocate with malloc */
static void *alloc_variable_sized(mt_t t, size_t s)
{
   /* make sure s is a multiple of MIN_HUNKSIZE */
   if (s % MIN_HUNKSIZE)
      s = ((s>>ALIGN_NUM_BITS)+1)<<ALIGN_NUM_BITS;

   if (s > MM_SIZE_MAX) {
      warn("size exceeds MM_SIZE_MAX\n");
      abort();
   }
  
   void *p = malloc(s + MIN_HUNKSIZE);  // + space for info
   assert(!LBITS(p));

   if (p) {
      info_t *info = p;
      info->t = t;
      info->nh = s/MIN_HUNKSIZE;
      
      if (collecting_child && !gc_disabled)
         fetch_unreachables();
      maybe_trigger_collect(s);

      return seal(p);

   } else
      return NULL;
}

/* allocate from small-object heap if possible */
static void *alloc_fixed_size(mt_t t)
{
   size_t s = types[t].size;
   if (BLOCKSIZE<(2*s))
      return alloc_variable_sized(t, s);
   
   if (!update_current_a(t)) {
      static bool warned = false;
      if (!warned) {
         /* warn once */
         warn("no free block found.\n");
         warn("consider re-compiling with larger heap size.\n");
         warned = true;
      }
      return alloc_variable_sized(t, s);
   }   

   void *p = heap + types[t].current_a;
   VALGRIND_MEMPOOL_ALLOC(heap, p, s);
   blockrecs[BLOCKA(types[t].current_a)].in_use++;
   
   if (collecting_child && !gc_disabled)
      fetch_unreachables();
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
            warn("address 0x%x (in block %d) is marked live\n", PPTR(heap + a), b);
            return false;
         }
      }
   }

   for (int i = 0; i <= man_last; i++) {
      if (LIVE(managed[i])) {
         void *p = CLRPTR2(managed[i]);
         warn("address 0x%x is marked live\n", PPTR(p));
         return false;
      }
   }

   return true;
}


/* 
 * Sort managed array in-place using poplar sort.
 * Information Processing Letters 39, pp 269-276, 1991.
 */

#define MAXPOPLAR 31
static int   poplar_roots[MAXPOPLAR+2] = {-1};
static bool  poplar_sorted[MAXPOPLAR];

#define M(p,q) (((p)+(q))/2)
#define A(q)   (managed[q])

#define SWAP(i,j)             \
{                             \
   void *__p = managed[i];    \
   managed[i] = managed[j];   \
   managed[j] = __p;          \
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

/* increment man_k while maintaining the poplar invariant */
/* only update if at least n updates could be done        */
static void update_man_k(int n)
{
   assert(!collect_in_progress);
#  define r poplar_roots

   if (man_last - man_k < n)
      return;

   while (man_k < man_last) {
      man_k++;
      n--;
      if ((man_t>=2) && (man_k-1+r[man_t-2] == 2*r[man_t-1])) {
         r[--man_t] = man_k;
         sift(r[man_t-1], man_k);
         poplar_sorted[man_t-1] = false;
      } else {
         r[++man_t] = man_k;
         poplar_sorted[man_t-1] = true;
      }
      if (n==0)
         break;
   }
#  undef r
}

/* sort a single poplar using poplar sort ;-) */
static void sort_poplar(int n)
{
   assert(!collect_in_progress);
   
   if (poplar_sorted[n]) return;

   int r[MAXPOPLAR+1], t = 1;
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

static bool poplar_roots_ok(void)
{
   for (int n = 1; n < man_t; n++) {
      assert(-1<=poplar_roots[n] && poplar_roots[n]<=man_k);
      assert(poplar_roots[n] < poplar_roots[n+1]);
   }
   assert(poplar_roots[man_t]==man_k);
   return true;
}

/* Remove obsolete entries from managed. */
static void compact_managed(void)
{
   assert(!collect_in_progress);

   if (man_is_compact) return;

   int j = 0;
   for (int i=0; i<=man_last; i++) {
      if (!OBSOLETE(managed[i]))
         managed[j++] = managed[i];
   }
   j--;

   /* rebuild poplars */
   man_last = j;
   man_k = -1;
   man_t = 0;
   update_man_k(-1);
   assert(poplar_roots_ok());

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
   if (CLRPTR2(managed[l]) == p)
      return l;

   else if (l<man_k && (CLRPTR2(managed[l+1]) == p))
      return l+1;

   else 
      return -1;
}

static int find_managed(const void *p)
{
   for (int n = 0; n < man_t; n++) {
      if (!poplar_sorted[n]) sort_poplar(n);
      int i = bsearch_managed(p, poplar_roots[n]+1, poplar_roots[n+1]+1);
      if (i >= 0)
         return i;
   }
   
   if (man_k < man_last)
      /* do linear search on excess part */
      for (int i = man_last; i > man_k; i--) {
         if (CLRPTR2(managed[i]) == p)
            return i;
      }
   return -1;
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
      assert(managed);
      /* XXX: try mm_collect_now before aborting? */
   }
   assert(man_last < man_size);
   managed[man_last] = (void *)p;
   
   if (!collect_in_progress)
      update_man_k(MAN_K_UPDS);
   
}

static void manage(const void *p)
{
   assert(ADDRESS_VALID(p));
   
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(!HMAP_MANAGED(a));
      HMAP_MARK_MANAGED(a);
      if (mark_in_progress && !collecting_child)
         HMAP_MARK_LIVE(a);

   } else
      add_managed(p);
   
   if (anchor_transients)
      stack_push(transients, p);
}

static void unmanage(const void *p)
{
   assert(ADDRESS_VALID(p));
   
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
      assert(HMAP_MANAGED(a));
      if (HMAP_NOTIFY(a)) {
         HMAP_UNMARK_NOTIFY(a);
         client_notify((void *)p);
      }
      HMAP_UNMARK_MANAGED(a);

   } else {
      int i = find_managed(p);
      assert(!OBSOLETE(managed[i]));
   
      if (NOTIFY(managed[i])) {
         UNMARK_NOTIFY(managed[i]);
         client_notify(managed[i]);
      }

      MARK_OBSOLETE(managed[i]);
      man_is_compact = false;
   }
}

static void collect_prologue(void)
{
   assert(!collect_in_progress);
   assert(!collecting_child);
   assert(!stack_overflowed2);
   if (mm_debug)
      assert(no_marked_live());
   
   /* prepare managed array and poplar data */
   update_man_k(-1);
   for (int n = 0; n < man_t; n++)
      sort_poplar(n);
   assert(man_k == man_last);

   collect_in_progress = true;
   num_collects += 1;
   debug("%dth collect after %d allocations:\n",
         num_collects, num_allocs);
   debug("mean alloc %.2f bytes, %d free blocks)\n",
         ((double)vol_allocs)/num_allocs, num_free_blocks());
}

static void collect_epilogue(void)
{
   assert(collect_in_progress);

   if (stack_overflowed2) {
      /* only effective with synchronous collect */
      stack_size *= 2;
      debug("enlarging marking stack to %d\n", stack_size);
      stack_overflowed2 = false;
   }

   marking_type = mt_undefined;
   marking_object = NULL;
   collect_requested = false;

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
#ifdef WCOREDUMP
         else if (WCOREDUMP(status))
            warn("coredumped\n");
#endif
         else
            warn("cause unknown (status = %d)\n", status);
         abort();
      }
      //mm_printf("reaped gc child %d\n", collecting_child);
      collecting_child = 0;
   }
   collect_in_progress = false;
   compact_managed();
}

/*
 * Push address onto marking stack and mark it if requested.
 */
static inline void push(const void *p, bool push_and_mark)
{
   if (live(p)) return;
   
   if (push_and_mark)
      mark_live(p);
    
   assert(stack_last < stack_size);
   stack_last++;
   if (stack_last == stack_size) {
      stack_overflowed = true;
      stack_last--;
   } else {
      stack[stack_last] = p;
   }
}

static inline const void *pop(void)
{
   return ((stack_last<0) ? NULL : stack[stack_last--]);
}

static inline bool empty(void)
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
         void *p = CLRPTR2(managed[i]);
         mt_t t  = INFO_T(p);
         if (types[t].mark)
            types[t].mark(p);
      }
   } DO_MANAGED_END;
}

void mm_mark(const void *p)
{
   if (!p) return;
   
   if (mm_debug) {
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
   push(p, true);
}


/* trace starting from objects currently in the stack */
static void trace_from_stack(void)
{
   assert(!stack_overflowed);

process_stack:
   while (!empty()) {
      const void *p = marking_object = pop();
      mt_t t = marking_type = MM_TYPEOF(p);
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
      debug("finalizer of object 0x%"PRIxPTR" (%s) caused an error:\n%s\n",
            PPTR(q), types[MM_TYPEOF(q)].name, errmsg);
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

   unmanage(q);
   VALGRIND_MEMPOOL_FREE(heap, q);
   
   assert(blockrecs[b].in_use > 0);
   blockrecs[b].in_use--;      
   if (blockrecs[b].in_use == 0) {
      blockrecs[b].t = mt_undefined;
      VALGRIND_DISCARD((block_t *)BLOCK_ADDR(q));
   }
   if (b < types[t].next_b)
      types[t].next_b = b;
}

static void reclaim_offheap(void *q)
{
   info_t *info_q = unseal(q); 
   finalize_func_t *f = types[info_q->t].finalize;
   if (f)
      if (!run_finalizer(f, q))
         return;

   unmanage(q);
   free(info_q);
}

/*
static inline void reclaim(void *q)
{
   if (INHEAP(q))
      reclaim_inheap(q);
   else
      reclaim_offheap(q);
}
*/

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
         reclaim_offheap(CLRPTR2(managed[i]));
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

#define STACK_ELTS_PER_CHUNK  255

typedef struct stack_chunk {
   struct stack_chunk  *prev;
   stack_elem_t        elems[STACK_ELTS_PER_CHUNK];
} stack_chunk_t;

struct stack {
   stack_ptr_t     sp;
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

   MM_MARK(st->temp);

   /* clear unused entries in current chunk */
   int n = st->sp - CURRENT_0(st);
   memset(st->current->elems, 0, n * sizeof(stack_elem_t));

   MM_MARK(st->current);
}

static void mark_stack_chunk(stack_chunk_t *c)
{
   MM_MARK(c->prev);
   for (int i = STACK_ELTS_PER_CHUNK-1; i >= 0; i--) {
      if (c->elems[i])
         mm_mark(c->elems[i]);
      else
         break;
   }
 }

static void add_chunk(mmstack_t *st)
{
   anchor_transients = false;
   DISABLE_GC;

   stack_chunk_t *c = mm_alloc(mt_stack_chunk);
   assert(c);
   c->prev = st->current;
   st->current = c;
   st->sp = ELT_N(c);
   
   ENABLE_GC;
   anchor_transients = true;
}


static void pop_chunk(mmstack_t *st)
{
   st->current = st->current->prev;
   if (!st->current) {
      warn("stack underflow\n");
      abort();
   }
   st->sp = ELT_0(st->current);
}


static mmstack_t *make_stack(void)
{
   mmstack_t stack = {0, 0, 0};
   add_chunk(&stack);
   
   anchor_transients = false;

   mmstack_t *st = mm_malloc(sizeof(mmstack_t));
   mm_type(st, mt_stack);
   memcpy(st, &stack, sizeof(mmstack_t));

   anchor_transients = true;

   return st;
}

static void stack_push(mmstack_t *st, stack_elem_t e)
{
   assert(e != 0);
   if (st->sp == CURRENT_0(st)) {
      st->temp = e;   // save
      add_chunk(st);  // add_chunk() might trigger a collection
      st->temp = 0;   // restore
      assert(st->sp == CURRENT_N(st));
   }
   st->sp--;
   *(st->sp) = e;
}

static bool stack_empty(mmstack_t *st)
{
   assert(st->current);
   return (st->current->prev==NULL && 
           st->sp==CURRENT_N(st) &&
           !st->temp);
}

static int stack_depth(mmstack_t *st)
{
   int d = CURRENT_N(st) - st->sp;
   stack_chunk_t *c = st->current;
   while (c->prev) {
      d = d + STACK_ELTS_PER_CHUNK;
      c = c->prev;
   }
   return d;
}

static stack_elem_t stack_peek(mmstack_t *st)
{
   assert(STACK_VALID(st));

   if (st->sp == CURRENT_N(st)) {
      assert(st->current->prev);
      stack_ptr_t sp = ELT_0(st->current->prev);
      return *sp;
   } else
      return *(st->sp);
}

static stack_elem_t stack_pop(mmstack_t *st)
{
   assert(STACK_VALID(st));

   if (st->sp == CURRENT_N(st))
      pop_chunk(st);

   return *(st->sp++);
}

static inline void stack_reset(mmstack_t *st, stack_ptr_t sp)
{
   assert(STACK_VALID(st));

   while (!(st->sp <= sp && sp <= CURRENT_N(st)))
      pop_chunk(st);
   st->sp = sp;

   assert(STACK_VALID(st));
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

/* push two chunks worth of numbers on stack,
 * index, and pop.
 */

static bool stack_works_fine(mmstack_t *st)
{
   intptr_t n = 2*STACK_ELTS_PER_CHUNK+2;
   
   assert(sizeof(stack_chunk_t) ==
          (STACK_ELTS_PER_CHUNK+1)*sizeof(stack_elem_t));
   
   for (intptr_t i = 0; i < n ; i++) {
      stack_push(st, (void *)(i+1));
   }

   /* avoid 'defined but not used' warning */
   (void)stack_peek(st);
   (void)obsolete(st);

   /* test indexing */
   for (intptr_t i = 0; i < n; i++) {
      if ((n-i) != (intptr_t)stack_elt(st, i)) {
         mm_printf("unexpected element on stack: %" PRIdPTR " (should be %" PRIdPTR")\n",
                   (intptr_t)stack_elt(st,i), (n-i));
         return false;
      }
   }

   /* test popping and stack_empty */
   n--;
   while (!stack_empty(st)) {
      intptr_t i = (intptr_t)stack_pop(st)-1;
      if (n != i) {
         mm_printf("unexpected element on stack: %" PRIdPTR " (should be %" PRIdPTR ")\n",
                   i, n);
         return false;
      }
      n--;
   }
   return true;
}

/*
 * MM API
 */

stack_ptr_t mm_begin_anchored(void)
{
   return transients->sp;
}

void mm_end_anchored(stack_ptr_t sp)
{
   stack_reset(transients, sp);
}

void mm_anchor(void *p)
{
   if (p)
      stack_push(transients, p);
}

mt_t mm_regtype(const char *n, size_t s, 
                clear_func_t c, mark_func_t *m, finalize_func_t *f)
{
   if (!n || !n[0]) {
      warn("first argument invalid\n");
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
   
   void *p = alloc_fixed_size(t);
   if (!p) {
      warn("allocation failed\n");
      abort();
   }
   if (types[t].clear)
      types[t].clear(p);
   manage(p);
   return p;
}


void *mm_allocv(mt_t t, size_t s)
{
   if (t == mt_undefined) {
      warn("attempt to allocate with undefined memory type\n");
      abort();
   }
   assert(TYPE_VALID(t));

   void *p = alloc_variable_sized(t, s);
   if (!p) {
      warn("allocation failed\n");
      abort();
   }
   if (types[t].clear)
      types[t].clear(p);
   manage(p);
   return p;
}


void *mm_malloc(size_t s)
{
   void *p = alloc_variable_sized(mt_blob, s);
   if (p) manage(p);
   return p;
}


void *mm_calloc(size_t n, size_t s)
{
   void *p = alloc_variable_sized(mt_blob, s);
   if (p) {
      memset(p, 0, mm_sizeof(p));
      manage(p);
   }
   return p;
}


void *mm_realloc(void *q, size_t s)
{
   if (q == NULL)
      return mm_malloc(s);

   assert(ADDRESS_VALID(q));
   info_t *info_q = unseal(q);
   mt_t t = info_q->t;
   if (INHEAP(q)) {
      warn("address was not obtained with mm_malloc\n");
      abort();
   }

   assert(TYPE_VALID(t));
   if (s < types[t].size) {
      warn("new size is smaller than size of memory type '%s'\n",
           types[t].name);
      abort();
   }  

   void *r = alloc_variable_sized(t, s);
   if (r) {
      memcpy(r, q, info_q->nh*MIN_HUNKSIZE);

      /* the new address r inherits q's notify flag     */
      /* notifiers and finalizers should not run for q  */
      /* so we clear q's notify flag and set its memory */
      /* type to mt_blob                                */
      manage(r);
      int i = find_managed(q);
      assert(i != -1);
      assert(managed[man_last] == r);
      if (NOTIFY(managed[i])) {
         MARK_NOTIFY(managed[man_last]);
         UNMARK_NOTIFY(managed[i]);
      }
      info_q->t = mt_blob; 
   }
   return r;
}


char *mm_strdup(const char *s)
{
   size_t ss = strlen(s);
   char  *s2;

   if (ss < MIN_STRING)
      s2 = mm_alloc(mt_string);
   else
      s2 = mm_allocv(mt_string, ss+1);

   if (s2)
      memcpy(s2, s, ss+1);

   return s2;
}


size_t mm_strlen(const char *s)
{
   if (MM_TYPEOF(s) != mt_string)
      return strlen(s);
   
   size_t l = MM_SIZEOF(s) - MIN_HUNKSIZE;
   l += strlen(s + l);
   assert(l == strlen(s));
   return l;
}


void mm_type(const void *p, mt_t t)
{
   assert(ADDRESS_VALID(p));
   assert(TYPE_VALID(t));

   if (INHEAP(p)) {
      warn("address was not obtained with mm_malloc\n");
      abort();
   }

   int i = managed[man_last]==p ? man_last : find_managed(p);
   if (i<0) {
      warn("(mm_type) 0x%" PRIxPTR " not a managed address\n", PPTR(p));
      abort();
   }
   
#ifdef NVALGRIND
   mt_t pt = INFO_T(p);
   if (t!=pt)
      debug("changing memory type of address %" PRIxPTR " from '%s' to '%s'\n",
            PPTR(p), types[pt].name, types[t].name);
#endif
   if (types[t].size > INFO_S(p)) {
      warn("Size of new memory type may not be larger than size of old memory type.\n"); 
      abort();
   }
   info_t *info = unseal(p);
   info->t = t;
   seal((char *)info);
}


void mm_notify(const void *p, bool set)
{
   assert(ADDRESS_VALID(p));

   if (!client_notify && set) {
      warn("(mm_notify) no notification function specified\n");
      abort();
   }
   
   ptrdiff_t a = ((char *)p) - heap;
   if (a>=0 && a<heapsize) {
         
      if (!HMAP_MANAGED(a)) {
         warn("(mm_notify) not a managed address\n");
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
      warn("(mm_notify) not a managed address\n");
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
      else
         return find_managed(p) != -1;
   }
}


mt_t mm_typeof(const void *p)
{
   assert(ADDRESS_VALID(p));
   mt_t t = MM_TYPEOF(p);
   return t;
}


size_t mm_sizeof(const void *p)
{
   assert(ADDRESS_VALID(p));
   return MM_SIZEOF(p);
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
         push(*roots[r], true);
      }
   }
   trace_from_stack();

   /* Mark dependencies of finalization-enabled objects */
   DO_HEAP (a, b) {
      finalize_func_t *finalize = types[blockrecs[b].t].finalize;
      if (!HMAP_LIVE(a) && finalize) {
         push((void *)(heap + a), false);
         trace_from_stack();
         HMAP_UNMARK_LIVE(a);  /* break cycles */
      }
   } DO_HEAP_END;
   
   DO_MANAGED (i) {
      void *p = CLRPTR(managed[i]);
      finalize_func_t *finalize = types[INFO_T(p)].finalize;
      if (!LIVE(managed[i]) && finalize) {
         push(p, false);
         trace_from_stack();
         UNMARK_LIVE(managed[i]); /* break cycles */
      }
   } DO_MANAGED_END;

   assert(empty());
   mark_in_progress = false;
}

static void sweep(void)
{
   void *buf[NUM_TRANSFER];
   assert(!collecting_child);

   int n = 0;

   /* objects in small object heap */
   DO_HEAP(a, b) {
      if (!HMAP_LIVE(a)) {
         buf[n++] = (void *)(heap + a);
         if (n == NUM_TRANSFER) {
            write(pfd_garbage[1], &buf, NUM_TRANSFER*sizeof(void *));
            n = 0;
         }
      }
   } DO_HEAP_END;

   buf[n++] = NULL;

   /* malloc'ed objects */
   DO_MANAGED(i) {
      if (!LIVE(managed[i])) {
         buf[n++] = CLRPTR2(managed[i]);
         if (n == NUM_TRANSFER) {
            write(pfd_garbage[1], &buf, NUM_TRANSFER*sizeof(void *));
            n = 0;
         }
      }
   } DO_MANAGED_END;
   if (n)
      write(pfd_garbage[1], &buf, n*sizeof(void *));
}

/* fetch addresses of unreachable objects from garbage pipe */
/* return number of objects reclaimed                       */
static int fetch_unreachables(void)
{
   static bool inheap = true;
   assert(collecting_child && !gc_disabled);
   
   errno = 0;
   void *buf[NUM_TRANSFER];
   int n = read(pfd_garbage[0], &buf, NUM_TRANSFER*sizeof(void *));

   if (n == -1) {
      if (errno != EAGAIN) {
         char *errmsg = strerror(errno);
         warn("error reading from garbage pipe\n");
         warn(errmsg);
         abort();
      } else
         return 0;

   } else if (n == 0) {
      /* we're done */
      close(pfd_garbage[0]);
      collect_epilogue();
      inheap = true;
      return 0;

   } else {
      /* don't need to disable gc here as gc is still
       * in progress
       */
      assert((n%sizeof(void *)) == 0);
      n = n/sizeof(void *);
      int j = 0;
      if (!inheap)
         goto reclaim_offheap;

      for (; j < n; j++) {
         if (buf[j])
            reclaim_inheap(buf[j]);
         else {
            inheap = false;
            break;
         }
      }
      j++;
   reclaim_offheap:
      for (; j < n; j++) {
         reclaim_offheap(buf[j]);
      }
      return n;
   }
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
      warn("could not open %s:\n%s\n", path, errmsg);
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
      warn("could not set up pipe for GC:\n%s\n", errmsg);
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
      warn("could not spawn child for GC:\n%s\n", errmsg);
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


void mm_collect(void)
{
   if (gc_disabled || collect_in_progress) {
      collect_requested = true;
      return;

   } else
      collect();
}


int mm_collect_now(void)
{
   if (gc_disabled || collect_in_progress) {
      collect_requested = true;
      return 0;
   }
   
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
      if (!gc_disabled)
         return fetch_unreachables()>0;
      else
         return false;

   } else if ((man_last - man_k) > 10*MAN_K_UPDS) {
      update_man_k(-1);
      return true;

   } else {
      ncalls++;
      if (ncalls<10) {
         return false;
      }
      /* create some work for ourselves */
      mm_collect();
      ncalls = 0;
      return true;
   }
}


bool mm_begin_nogc(bool dont_block)
{
   /* block when a collect is in progress */
   if (!dont_block) {
      if (collecting_child && gc_disabled) {
         warn("deadlock (MM_NOGC while pending GC paused)\n");
         abort();
      }
      while (collecting_child)
         fetch_unreachables();
      assert(!collect_in_progress);
   }

   bool nogc = gc_disabled;
   gc_disabled = true;
   return nogc;
}

void mm_end_nogc(bool nogc)
{
   gc_disabled = nogc;
   if (collect_requested && !gc_disabled)
      mm_collect();
}

static void mark_refs(void **p)
{
   int n = mm_sizeof(p)/sizeof(void *);
   for (int i = 0; i < n; i++)
      MM_MARK(p[i]);
}

static void clear_refs(void **p)
{
   memset(p, 0, mm_sizeof(p));
}


void mm_init(int npages, notify_func_t *clnotify, FILE *log)
{
   assert(sizeof(info_t) <= MIN_HUNKSIZE);
   assert(sizeof(hunk_t) <= MIN_HUNKSIZE);
   assert((1<<HMAP_EPI_BITS) == HMAP_EPI);

   client_notify = clnotify;
   
   if (log) {
      mm_debug = true;
      stdlog = log;
   } else {
      mm_debug = false;
      stdlog = stderr;
   }

   if (heap) {
      warn("mm is already initialized\n");
      return;
   }

   /* check that we have a proc file system */
   {
      errno = 0;
      DIR *d = opendir("/proc");
      if (!d) {
         char *errmsg = strerror(errno);
         warn("can't find proc filesystem:\n%s\n", errmsg);
         abort();
      }
      closedir(d);
   }

   /* allocate small-object heap */
   num_blocks = max((PAGESIZE*npages)/BLOCKSIZE, MIN_NUMBLOCKS);
   heapsize = num_blocks * BLOCKSIZE;
   hmapsize = (heapsize/MIN_HUNKSIZE)/HMAP_EPI;
   volume_threshold = min(MAX_VOLUME, heapsize/20);

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
   assert(mt_blob == MM_REGTYPE("blob", 0, 0, 0, 0));
   assert(mt_refs == MM_REGTYPE("refs", 0, clear_refs, mark_refs, 0));
   assert(mt_string == MM_REGTYPE("string", MIN_STRING, 0, 0, 0));
   assert(types_last == 2);

   /* set up other bookkeeping structures */
   managed = (void *)malloc(MIN_MANAGED * sizeof(void *));
   assert(managed);
   man_size = MIN_MANAGED;

   roots = malloc(MIN_ROOTS * sizeof(void *));
   assert(roots);
   roots_size = MIN_ROOTS;

   /* initialize transient object stack */
   mt_stack = MM_REGTYPE("mm_stack", sizeof(mmstack_t), 0, mark_stack, 0);
   mt_stack_chunk = MM_REGTYPE("mm_stack_chunk", sizeof(stack_chunk_t),
                               clear_refs, mark_stack_chunk, 0);
   *(mmstack_t **)&transients = make_stack();
   MM_ROOT(transients);
   assert(stack_works_fine(transients));
   assert(stack_empty(transients));

   debug("done\n");
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
      if (!OBSOLETE(managed[i])) {
         mt_t ti = mm_typeof(CLRPTR(managed[i]));
         if (t==mt_undefined || t==ti)
            mm_printf("%4d : %16"PRIxPTR", %4d  %8s %1s%1s%1s %20s\n", 
                      i, PPTR(CLRPTR(managed[i])),
                      INHEAP(managed[i]) ? (int)BLOCK(managed[i]) : -1,
                      INHEAP(managed[i]) ? "inheap" : "offheap",
                      LIVE(managed[i]) ? "l" : " ",
                      OBSOLETE(managed[i]) ? "o" : " ",
                      NOTIFY(managed[i]) ? "n" : " ",
                      types[ti].name);
      }
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
             st == transients ? "transient" : "finalizer");
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
   mm_printf("depth of transients stack: %d\n", stack_depth(transients));
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
