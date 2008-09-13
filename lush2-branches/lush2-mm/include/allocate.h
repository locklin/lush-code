
#define NVALGRIND
#ifndef NVALGRIND
#  include <valgrind/memcheck.h>
# define REDZONE_SIZE 4
#else
#  define VALGRIND_MALLOCLIKE_BLOCK(...)
#  define VALGRIND_FREELIKE_BLOCK(...)
#  define VALGRIND_MAKE_MEM_NOACCESS(...)
# define REDZONE_SIZE 0
#endif

/* ALLOC.H ---------------------------------------------------- */

/*
  How to use Valgrind to investigate memory problems:
  o #undef NVALGRIND
  o recompile (with '-g -O -Wall')
*/

/* Allocation of objects of identical size */

typedef struct empty_alloc {
  int used;
  struct empty_alloc *next;
} empty_alloc_t;

typedef struct chunk_header {
  empty_alloc_t *begin;
  empty_alloc_t *end;
  /* gptr pad; */
  struct chunk_header *next;
} chunk_header_t;

typedef struct alloc_root {
  empty_alloc_t *freelist;        /* List of free elements */
  chunk_header_t *chunklist;      /* List of active chunkes */
  size_t sizeof_elem;		  /* Size of one element */
  int chunksize;		  /* Number of elems in one chunk */
} alloc_root_t;

LUSHAPI void allocate_chunk(alloc_root_t *ar);

LUSHAPI static inline gptr allocate(alloc_root_t *ar)
{
  if (ar->freelist==NULL)
    allocate_chunk(ar);
  
  empty_alloc_t *answer = ar->freelist;
  VALGRIND_MALLOCLIKE_BLOCK(answer, ar->sizeof_elem, REDZONE_SIZE, 1);
  
  ar->freelist = answer->next;
  assert(answer->used==0);
  answer->used = -1;
  return answer;
}

LUSHAPI static inline void deallocate(alloc_root_t *ar, gptr p)
{
  assert(p!=NULL);
  empty_alloc_t *elem = p;
  elem->next = ar->freelist;
  elem->used = 0;
  ar->freelist = elem;
  VALGRIND_FREELIKE_BLOCK(elem, REDZONE_SIZE);
}

