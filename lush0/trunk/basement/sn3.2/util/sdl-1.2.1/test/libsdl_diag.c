
/* Copyright (c) 1998  AT&T --- All Rights Reserved
 * The copyright notice above does not evidence any
 * actual or intended publication of this work.
 *
 * WARNING:
 * This code has been automatically generated
 * by the DH compiler. Exercise care while
 * modifying it.
 */
/*** PREEMPTIVE HEADERS (user defined) ***/
#include <SDL/SDL.h>

#include "header.h"
#include "dh.h"
#include "idxmac.h"
#include "idxops.h"
#include "runtime_err.h"
#include "check_func.h"

/*
 * C_FUNCTION
 */
char
C_sdl_surface_dump (L1_s)
SDL_Surface *L1_s;
{
  TRACE_PUSH ("C_sdl_surface_dump");
  {
    printf ("flags=%d\n", ((int) floor ((double) (int) ((L1_s)->flags))));
    printf ("format=%d\n", ((int) floor ((double) (int) ((L1_s)->format))));
    printf ("w=%d, h=%d\n", ((int) floor ((double) (int) ((L1_s)->w))),
	    ((int) floor ((double) (int) (L1_s)->h)));
    printf ("pitch=%d\n", ((int) floor ((double) (int) ((L1_s)->pitch))));
    printf ("pixels=%d\n", ((int) floor ((double) (int) ((L1_s)->pixels))));
    printf ("offset=%d\n", ((int) floor ((double) (int) ((L1_s)->offset))));
    printf ("hwdata=%d\n", ((int) floor ((double) (int) ((L1_s)->hwdata))));
    printf ("clip_rect x=%d, y=%d, w=%d, h=%d\n",
	    ((int) floor ((double) (int) ((L1_s)->clip_rect.x))),
	    ((int) floor ((double) (int) ((L1_s)->clip_rect.y))),
	    ((int) floor ((double) (int) ((L1_s)->clip_rect.w))),
	    ((int) floor ((double) (int) ((L1_s)->clip_rect.h))));
    printf ("unused1=%d\n", ((int) floor ((double) (int) ((L1_s)->unused1))));
    printf ("locked=%d\n", ((int) floor ((double) (int) ((L1_s)->locked))));
    printf ("map=%d\n", ((int) floor ((double) (int) ((L1_s)->map))));
    printf ("format_version=%d\n",
	    ((int) floor ((double) (int) ((L1_s)->format_version))));
    printf ("refcount=%d\n",
	    ((int) floor ((double) (int) ((L1_s)->refcount))));
    TRACE_POP ("C_sdl_surface_dump");
    return 0;
  }
}

/*
 * X_FUNCTION
 */
#ifndef NOLISP
DH (X_sdl_surface_dump)
{
  dharg ret;

  ret.dh_char = C_sdl_surface_dump (a[1].dh_gptr);
  return ret;
}
#endif

/*
 * K_RECORD
 */
#ifndef NOLISP
DHDOC (K_sdl_surface_dump, X_sdl_surface_dump, "C_sdl_surface_dump", NULL,
       NULL)
{
DH_FUNC (1), DH_GPTR, DH_RETURN, DH_BOOL, DH_END_FUNC, DH_NIL};
#endif

#ifndef NOLISP
/*
 * INIT FUNCTION
 */
void
init_libsdl_diag ()
{
  dh_define ("sdl-surface-dump", &K_sdl_surface_dump);
}

int majver_libsdl_diag = 3;
int minver_libsdl_diag = 2;
#endif
