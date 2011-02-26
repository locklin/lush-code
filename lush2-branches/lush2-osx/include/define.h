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

#ifndef DEFINE_H
#define DEFINE_H

#ifdef HAVE_CONFIG_H
# include "lushconf.h"
#else
# ifdef WIN32
#  define HAVE_STRFTIME 1
#  define STDC_HEADERS  1  
#  define HAVE_STRCHR   1
#  define HAVE_MEMCPY   1
#  define HAVE_STRERROR 1
# endif
#endif

#if defined(__APPLE__) && defined(__MACH__)
#  define __MACOSX__  1
#  define _DARWIN_C_SOURCE 1
#endif


/* --------- GENERAL PURPOSE DEFINITIONS ---------- */

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE  199506L
#endif
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE    600
#endif

#if HAVE_SYS_TYPES_H 
# include <sys/types.h>
  /* Solaris defines index_t in <sys/types.h> */
# define index_t TLindex_t
#endif

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

/* untyped pointer */
typedef void* gptr;
typedef void* mptr;  /* for managed addresses */
#ifndef uchar
#  define uchar unsigned char
#endif
#define NIL 0L


/* --------- MACHINE DEPENDANT STUFF -------- */

#ifdef WIN32
# define main             lushmain
# define exit             win32_exit
# define isatty           win32_isatty
# define popen            win32_popen
# define pclose           win32_pclose
# define FMODE_TEXT(f)    win32_fmode_text(f);
# define FMODE_BINARY(f)  win32_fmode_binary(f);
# define INIT_MACHINE     init_win32()
# define TOPLEVEL_MACHINE break_attempt=0
# define CHECK_MACHINE(s) if (break_attempt) win32_user_break(s)
# define DLLEXPORT        __declspec(dllexport)
# define DLLIMPORT        __declspec(dllimport)
# if ! defined(TLAPI)
#  if defined(TL3DLL)
#   define TLAPI DLLEXPORT
#  else  /* !defined TL3DLL && !defined(_CONSOLE) */
#   define TLAPI DLLIMPORT
#  endif /* !defined(TL3DLL) */
# endif /* !defined(TLAPI) */
# if defined (_MSC_VER) && (_MSC_VER == 1100)
#  pragma optimize("p",on)
#  pragma warning(disable: 4056)
# endif /* VC50 */
#endif /* WIN32 */

#ifdef AMIGA
/* This is not up-to-date */
# define INIT_MACHINE      init_amiga()
# define CHECK_MACHINE(s)  check_amiga("Break "s)
# define putc(c,stream)    aputc(c,stream)
# define getc(stream)      agetc(stream)
#endif

#ifdef UNIX
# define INIT_MACHINE      init_unix()
# define FINI_MACHINE      fini_unix()
# define TOPLEVEL_MACHINE  toplevel_unix()
# define CHECK_MACHINE(s)  if (break_attempt) user_break(s)
# ifdef HAVE_WAITPID
#  define NEED_POPEN
# endif
# define popen             unix_popen
# define pclose            unix_pclose
# ifdef __CYGWIN32__
#  define FMODE_TEXT(f)    cygwin_fmode_text(f);
#  define FMODE_BINARY(f)  cygwin_fmode_binary(f);
# endif
#endif

#ifndef TLAPI
# define TLAPI            /**/
#endif
#ifndef LUSHAPI
# define LUSHAPI TLAPI
#endif
#ifndef INIT_MACHINE
# define INIT_MACHINE     /**/
#endif
#ifndef FINI_MACHINE
# define FINI_MACHINE     /**/
#endif
#ifndef TOPLEVEL_MACHINE
# define TOPLEVEL_MACHINE /**/
#endif
#ifndef CHECK_MACHINE
# define CHECK_MACHINE    /**/
#endif
#ifndef FMODE_TEXT
# define FMODE_TEXT(f)    /**/
#endif
#ifndef FMODE_BINARY
# define FMODE_BINARY(f)  /**/
#endif

/* --------- AUTOCONF --------- */

#if STDC_HEADERS
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# else
#  include <string.h>
# endif
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
# ifndef HAVE_MEMCPY
#  define memcpy(d,s,n) bcopy((s),(d),(n))
#  define memset(d,c,n) do{char *dd=d;int nn=n;while(nn-->0)*dd++=c;}while(0)
# endif
#endif

#ifndef STDC_HEADERS
# ifdef toupper
#  undef toupper
# endif
# ifdef tolower
#  undef tolower
# endif
# define NEED_TOUPPER
# define NEED_TOLOWER
#endif

#ifdef HAVE_WCHAR_H
# include <wchar.h>
# include <limits.h>
# ifdef HAVE_WCTYPE_H
#  include <wctype.h>
# endif
# ifndef HAVE_WINT_T
#  define wint_t int
# endif
# ifndef HAVE_MBSTATE_T
#  define mbstate_t int
# endif
# ifndef HAVE_MBRTOWC
#  define mbrtowc(p,s,n,ps) mbtowc(p,s,n)
# endif
# ifndef HAVE_WCRTOMB
#  define wcrtomb(s,w,ps) wctomb(s,w)
# endif
# ifndef HAVE_MBRLEN
#  define mbrlen(s,n,ps) mblen(s,n)
# endif
#endif

#ifndef HAVE_SIGSETJMP
# ifndef sigsetjmp
#  ifndef siglongjmp
#   ifndef sigjmp_buf
#    define sigjmp_buf jmp_buf
#    define sigsetjmp(env, arg) setjmp(env)
#    define siglongjmp(env, arg) longjmp(env,arg)
#   endif
#  endif
# endif
#endif

/* --------- GENERIC NAMES --------------- */

/* These macros may be redefined in
 * the machine dependent part, just below 
 */

#ifdef __STDC__			/* Defined by ANSI compilers */
# define name2(a,b)      _name2(a,b)
# define _name2(a,b)     a##b
# define name3(a,b,c)    _name3(a,b,c)
# define _name3(a,b,c)   a##b##c
#else
# define name2(a,b)      a/**/b
# define name3(a,b,c)    a/**/b/**/c
#endif

/* return the variable in a string */
#ifdef __STDC__
# define enclos2_in_string(a) #a
# define enclose_in_string(a) enclos2_in_string(a)
#else
# define enclose_in_string(a) "a"
#endif

/* --------- LISP CONSTANTS --------- */

#define DXSTACKSIZE   (int)4096	/* max size for the DX stack */
#define MAXARGMAPC    (int)8	/* max number of listes in MAPCAR */
#define HASHTABLESIZE (int)1024	/* symbol hashtable size */
#define STRING_BUFFER (int)4096	/* string operations buffer size */
#define LINE_BUFFER   (int)1024	/* line buffer length */
#define FILELEN       (int)1024	/* file names length */
#define DZ_STACK_SIZE (int)1000 /* stack size for DZs */
#define MINSTORAGE    (size_t)4 /* min number of elements allocated */
/* ---------------------------------- */
#endif
