/*   TL3 Lisp interpreter and development tools
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
        TL3:    L.Y. Bottou   --   Y. LeCun
        define.h
        General Use definitions
	$Id: define.h,v 1.1.1.1 2002-04-16 17:37:32 leonb Exp $
********************************************************************** */

#ifndef DEFINE_H
#define DEFINE_H

/* --------- GENERAL PURPOSE DEFINITIONS ---------- */

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

/* untyped pointer */
#define ptr void*
#define NIL 0L

/* boolean constants */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* --------- UNFORTUNATE NAMES -------- */

#define abort     TLabort
#define error     TLerror
#define class     TLclass
#define true      TLtrue
#define basename  TLbasename
#define dirname   TLdirname

/* --------- MACHINE DEPENDANT STUFF -------- */

#ifdef WIN32
#define main             tl3main
#define exit             win32_exit
#define isatty           win32_isatty
#define popen            win32_popen
#define pclose           win32_pclose
#define FMODE_TEXT(f)    win32_fmode_text(f);
#define FMODE_BINARY(f)  win32_fmode_binary(f);
#define INIT_MACHINE     init_win32()
#define TOPLEVEL_MACHINE break_attempt=0
#define CHECK_MACHINE(s) if (break_attempt) win32_user_break(s)
#define DLLEXPORT        __declspec(dllexport)
#define DLLIMPORT        __declspec(dllimport)
#if !defined(TLAPI)
#if defined(TL3DLL)
#define TLAPI DLLEXPORT
#else  /* !defined TL3DLL && !defined(_CONSOLE) */
#define TLAPI DLLIMPORT
#endif /* !defined(TL3DLL) */
#endif /* !defined(TLAPI) */
/* Optimization bug with VC5.0 */
#if defined (_MSC_VER) && (_MSC_VER == 1100)
#pragma optimize("p",on)
#pragma warning(disable: 4056)
#endif /* VC50 */
#endif /* WIN32 */

#ifdef AMIGA
#define INIT_MACHINE      init_amiga()
#define CHECK_MACHINE(s)  check_amiga("Break "s)
#define putc(c,stream)    aputc(c,stream)
#define getc(stream)      agetc(stream)
/* This is not up-to-date */
#endif

#ifdef MAC
/* This is not up-to-date */
#endif

#ifndef WIN32
#ifndef AMIGA
#ifndef MAC
#define UNIX
#endif
#endif
#endif

#ifdef UNIX
#define INIT_MACHINE      init_unix()
#define TOPLEVEL_MACHINE  break_attempt=0
#define CHECK_MACHINE(s)  if (break_attempt) user_break(s)
#endif

#ifndef TLAPI
#define TLAPI            /**/
#endif
#ifndef INIT_MACHINE
#define INIT_MACHINE     /**/
#endif
#ifndef TOPLEVEL_MACHINE
#define TOPLEVEL_MACHINE /**/
#endif
#ifndef CHECK_MACHINE
#define CHECK_MACHINE    /**/
#endif
#ifndef FMODE_TEXT
#define FMODE_TEXT(f)    /**/
#endif
#ifndef FMODE_BINARY
#define FMODE_BINARY(f)  /**/
#endif

/* --------- AUTOCONF --------- */

#ifdef WIN32
#define HAVE_STRFTIME 1
#define STDC_HEADERS  1  
#define HAVE_STRCHR   1
#define HAVE_MEMCPY   1
#define HAVE_STRERROR 1
#endif

#ifdef UNIX
#include "config.h"
#endif

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
#ifdef toupper
#undef toupper
#endif
#ifdef tolower
#undef tolower
#endif
#define NEED_TOUPPER 1
#define NEED_TOLOWER 1
#endif

#ifndef HAVE_SIGSETJMP
#ifndef sigsetjmp
#ifndef siglongjmp
#ifndef sigjmp_buf
#define sigjmp_buf jmp_buf
#define sigsetjmp(env, arg) setjmp(env)
#define siglongjmp(env, arg) longjmp(env,arg)
#endif
#endif
#endif
#endif

/* --------- LISP CONSTANTS --------- */

#define DXSTACKSIZE   (int)3000	/* max size for the DX stack */
#define MAXARGMAPC    (int)8	/* max number of listes in MAPCAR */
#define CONSCHUNKSIZE (int)2048	/* minimal cons allocation unit */
#define HASHTABLESIZE (int)1024	/* symbol hashtable size */
#define NAMELENGTH    (int)40	/* symbol names length */
#define STRING_BUFFER (int)4096	/* string operations buffer size */
#define LINE_BUFFER   (int)1024	/* line buffer length */
#define FILELEN       (int)1024	/* file names length */
#define PATHLEN       (int)4096	/* path buffer length */
#define PATHNUMBER    (int)64	/* path items number */
#define PRODNUMBER    (int)16	/* number of seperate NSA products */

/* ---------------------------------- */
#endif
