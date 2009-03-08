
/* private definitions */

struct mm_stack {
   const void  **sp;
   const void  **sp_max;
};

static inline const void **_mm_begin_anchored(void)
{
   extern struct mm_stack *const _mm_transients;
   return _mm_transients->sp;
}

extern void _mm_pop_chunk(struct mm_stack *);

#define st _mm_transients
static inline void _mm_end_anchored(const void **sp)
{
   extern struct mm_stack *const _mm_transients;
   while (st->sp>sp || sp>st->sp_max)
      _mm_pop_chunk(st);
   st->sp = sp;
}
#undef st


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
