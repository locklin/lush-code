/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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

/*
 * Every managed object has an associated memtype.
 * The mark function knows about references in an
 * object and marks the object and all its children.
 * The finalize function knows how to properly dispose
 * an object.
 */

#ifndef MM_INCLUDED
#define MM_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define NVALGRIND
#define MM_SIZE_MAX    (UINT32_MAX*MIN_HUNKSIZE)



typedef void clear_func_t(void *, size_t);
typedef void mark_func_t(const void *);
typedef bool finalize_func_t(void *);

typedef void notify_func_t(void *);

typedef short mt_t;

/* pre-defined memory types */
enum mt {
   mt_undefined = -1,
   /* 0 & 1 for internal use */
   mt_blob8     =  2,
   mt_blob16    =  3,
   mt_blob32    =  4,
   mt_blob64    =  5,
   mt_blob128   =  6,
   mt_blob256   =  7,
   mt_blob      =  8,
   mt_refs      =  9,
};

/* MM administration */
void    mm_init(int, notify_func_t *, FILE *); // initialize manager
void    mm_debug(bool);                  // enable/disable debug code
mt_t    mm_regtype(const char *, size_t, clear_func_t, mark_func_t *, finalize_func_t *);
void    mm_root(const void *);           // add a root location
void    mm_unroot(const void *);         // remove a root location
bool    mm_idle(void);                   // do work, return true when more work

/* garbage collection */
int     mm_collect_now(void);            // trigger garbage collection
bool    mm_collect_in_progress(void);    // true if gc is under way

/* allocation functions */
void   *mm_alloc(mt_t);                  // allocate fixed-size object
void   *mm_allocv(mt_t, size_t);         // allocate variable-sized object
void   *mm_malloc(mt_t, size_t);         // allocate variable-sized object
void   *mm_blob(size_t);                 // allocate blob of size
char   *mm_strdup(const char *);         // create managed copy of string

/* properties of managed objects */
bool    mm_ismanaged(const void *);      // true if managed object
void    mm_manage(const void *);         // manage malloc'ed address 
void    mm_notify(const void *, bool);   // set or unset notify flag
size_t  mm_sizeof(const void *);         // size of object
mt_t    mm_typeof(const void *);         // type of object

/* diagnostics */
char   *mm_info(int);                    // diagnostic message in managed string
int     mm_prof_start(int *);            // start profiling, initialize histogram
void    mm_prof_stop(int *);             // stop profiling and write data
char  **mm_prof_key(void);               // make key for profile data

/* MACROS */
#define MM_INIT(n,nf,f)         { mm_record_cstack_ptr(); mm_init(n,nf,f); }

#define MM_MARK(p)              { if (p) _mm_mark(p); }
#define MM_REGTYPE(n,s,c,m,f)   mm_regtype(n, s, (clear_func_t *)c, (mark_func_t *)m, (finalize_func_t *)f)
#define MM_ROOT(p)              mm_root(&p)
#define MM_UNROOT(p)            mm_unroot(&p)

#define MM_ENTER                const void **__mm_stack_pointer = _mm_begin_anchored()
#define MM_EXIT                 _mm_end_anchored(__mm_stack_pointer)
#define MM_ANCHOR(p)            { if (p) _mm_anchor(p); }
#define MM_RETURN(p)            { void *pp = p; MM_EXIT; MM_ANCHOR(pp); return pp; }
#define MM_RETURN_VOID          MM_EXIT; return

#define MM_NOGC                 bool __mm_nogc = mm_begin_nogc(false)
#define MM_NOGC_END             mm_end_nogc(__mm_nogc)
#define MM_PAUSEGC              bool __mm_pausegc = mm_begin_nogc(true)
#define MM_PAUSEGC_END          mm_end_nogc(__mm_pausegc)

void    mm_record_cstack_ptr(void);
void    mm_anchor(const void *);
bool    mm_begin_nogc(bool);
void    mm_end_nogc(bool);

#ifndef MM_INTERNAL
#include "cmm_private.h"
#endif

#endif /* MM_INCLUDED */


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
