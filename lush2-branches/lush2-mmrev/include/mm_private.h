
/* private definitions */

struct mm_stack {
   const void  **sp;
   const void  **sp_min;
   const void  **sp_max;
};

extern struct mm_stack *const _mm_transients;

#define st _mm_transients

static inline void _mm_anchor(const void *p)
{
   if (st->sp > st->sp_min)
      *(st->sp--) = p;
   else
      mm_anchor(p);
}

static inline const void **_mm_begin_anchored(void)
{
   return _mm_transients->sp;
}

static inline void _mm_end_anchored(const void **sp)
{
   extern void _mm_pop_chunk(struct mm_stack *);

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
