/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/
  

#ifndef DEFINE_H
#define DEFINE_H

/* --------- MACHINE TYPE --------- */

#ifdef SOLARIS86
#define SOLARIS
#endif
#ifdef SOLARIS
#define SGI_OR_SOLARIS

/*#define SGI_OR_SOLARIS_OR_ALPHA*/
/* since we also need _OR_LINUX
/* I decided to remove this and 
/* replace the #ifdef SGI_OR_SOLARIS_OR_ALPHA_OR_LINUX by #ifndef SUNOS */
/* -- Yoshua Bengio */

#endif
#ifdef SGI
#define SGI_OR_ALPHA
#define SGI_OR_SOLARIS

/* #define SGI_OR_SOLARIS_OR_ALPHA */ 

#endif
#ifdef ALPHA
#define SGI_OR_ALPHA
/*#define SGI_OR_SOLARIS_OR_ALPHA*/
#endif
#ifdef LINUX86OLD
#define LINUX
#endif

/* #define MAC */
#define UNIX
/* #define AMIGA */
/* #define MSDOS */

/* --------- GENERAL PURPOSE DEFINITIONS ---------- */

#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* untyped pointer */
#define ptr void*

#define bool char

#define NIL     0L

#ifdef TRUE
#undef TRUE
#undef FALSE
#endif

#define TRUE    (int) 1
#define FALSE   (int) 0


/* --------- GENERIC NAMES --------------- */

/* These macros may be redefined in
 * the machine dependent part, just below 
 */

#ifdef __STDC__			/* Defined by ANSI compilers */
/* ANSI C method */
#define name2(a,b)      _name2(a,b)
#define _name2(a,b)     a##b
#define name3(a,b,c)    _name3(a,b,c)
#define _name3(a,b,c)   a##b##c
#else
#ifdef __BSDCPP__		/* Specified in the Makefile */
/* Berkeley CPP method */
#define name2(a,b)      a\
b
#define name3(a,b,c)    a\
b\
c
#else				/* Most common case */
/* Null comment method */
#define name2(a,b)      a/**/b
#define name3(a,b,c)    a/**/b/**/c
#endif
#endif

/* return the variable in a string 
   (account for the difference between Ansi and K&R cpp) */
#ifdef __STDC__
#define enclose_in_string(a) #a
#else
#define enclose_in_string(a) "a"
#endif

/* --------- LISP CONSTANTS --------- */

#define DXSTACKSIZE   (int)8000	/* max size for the DX stack (min 500) */
#define MAXARGMAPC    (int)8	/* max number of listes in MAPCAR & Co */
#define CONSCHUNKSIZE (int)4000	/* minimal cons allocation unit */
#define HASHTABLESIZE (int)256	/* hash_coding cardinality */
#define NAMELENGTH    (int)80	/* symbol names   length */
#define STRING_BUFFER (int)4096	/* string operations buffer size */
#define LINE_BUFFER   (int)4096	/* line buffer length */
#define FILELEN       (int)256	/* file names length */
#define PATHLEN       (int)1024	/* path buffer length */
#define PATHNUMBER    (int)16	/* path items number */
#define HASHMETHODS   (int)31   /* method hashtable */


/* --------- NUMERIC DEFINITIONS --------- */

/*
 * See files       - FLTLIB.H - FLTLIB.C
 */


/* --------- MACHINE DEPENDANT STUFFS -------- */

#ifdef AMIGA
#define DEFAULT_STARTUP         "stdenv.sn"
#define DEFAULT_MACROS          "sysenv.sn"
#define SN_DIR                  "sn2:"
#define SN_LIB                  "lib/"
#define INIT_MACHINE            INIT_amiga()
#define TOPLEVEL_MACHINE        ;
#define CHECK_MACHINE(s)        check_amiga(breakdebug)
#define BYEBYE_MACHINE		;
#define putc(c,stream)          aputc(c,stream)
#define getc(stream)            agetc(stream)
#define GETLINE(buf,size)       fgets(buf,size-2,stdin)
extern ptr malloc();
extern ptr calloc();
#endif

#ifdef MAC
/* To be copied and updated from sn2.2 */
#endif

#ifdef UNIX

#define DEFAULT_STARTUP    "stdenv"
#define DEFAULT_MACROS     "sysenv"
#define SN_DIR             getenv("SN3DIR")
#define SN_LIB             "/lib/"
#define INIT_MACHINE       INIT_unix()
#define TOPLEVEL_MACHINE   break_attempt = 0;
#define BYEBYE_MACHINE	   BYEBYE_unix()
#define CHECK_MACHINE(s)   if (break_attempt) error(breakdebug,"", NIL)
#define GETLINE(buf,size)  fgets(buf,size-2,stdin)
extern ptr malloc();
extern ptr calloc();
extern int break_attempt;

/* THESE THINGS ARE SAFER ANYWAY */
#define HASPIPES
#define class  myclass
#define fread  myfread
#define fwrite myfwrite
#define abort myabort
#undef toupper
#undef tolower
#define ALT_TOUPPER
#define toupper mytoupper
#define tolower mytolower

#ifdef SUNOS
#ifndef __GNUC__
#undef alloca
#include <alloca.h>
#endif
#define PARANOIAC_IEEE
#define FORK fork
#define MYPOPEN
#define popen mypopen
#define pclose mypclose
#ifdef SUNDRIVER
#undef GETLINE
#undef CHECK_MACHINE
#define GETLINE(buf,size) sun_getline(buf,size-2)
#define CHECK_MACHINE(s) sun_check_on_the_fly(s);
#endif
#endif

#ifndef SUNOS
#define PARANOIAC_IEEE
#define FORK                           fork
#endif

/* We declare alloca to "ptr",
 * only if it has not been #defined before.
 * Usually, builtin allocas are defined as __builtin_alloca.
 * Se we can expect it to work.
 * ALWAYS CAST THE OUTPUT OF ALLOCA
 */

#ifdef SUNOS
#ifndef alloca
ptr alloca();
#endif
#else
/* there is a alloca function known by cc on SGI machines */
#include <alloca.h>
#endif

#ifndef SUNOS
#define bzero(s,n)                     memset(s,0,n)
#define NOGETWD
#endif

#ifdef ALPHA
#define abort myabort
#endif

#ifdef APOLLO
/* PLEASE, CHECK */
#define FORK                           vfork
#ifdef GPRDRIVER
#undef GETLINE
#undef CHECK_MACHINE
#define GETLINE(buf,size)              apollo_getline(buf,size-2)
#define CHECK_MACHINE(s)               apollo_check_on_the_fly(s);
#endif
#endif

#ifdef SONY
/* PLEASE, CHECK */
#ifdef mips
#undef ptr
#define ptr char*
#endif
#define BLORT
#define FORK                           vfork
#endif

#ifdef MIPS
/* PLEASE, CHECK */
#define FORK                           vfork
#define index strchr
#define rindex strrchr
#define NO_IN_FLUSH
#define STANDARD_SYS
#endif

#ifdef VAX
/* PLEASE, CHECK */
#undef ptr
#define ptr char*
#define FORK				vfork
#define NOGETWD
#define HILOBYTE
#define NO_IN_FLUSH
#endif

#ifdef IBM6000
/* PLEASE, CHECK */
#define FORK				vfork
#define abort				myabort
#define HILOBYTE
#define NO_IN_FLUSH
#endif

#ifdef HP
/* PLEASE, CHECK */
/* TODO: Include BSDSIGNAL, SYSVSIGNAL in unix.c */
#define index strchr
#define rindex strrchr
#define NOGETWD
#endif

#ifdef SVR4
/* PLEASE, CHECK */
/* TODO: INTEGRATE STREAMS X11 CODE */
#define ASYNC_STREAMS
#endif

#ifdef DPX2000
/* PLEASE, CHECK */
#define bzero(s,n)                     memset(s,0,n)
#define FORK                           fork
#define NOGETWD
/* TODO: INTEGRATE CLUMSY ALARM X11 CODE */
#define ASYNC_ALARM
#undef GETLINE
#define GETLINE(buf,size)              alarm_getline(buf,size-2)
#endif

#endif /* UNIX */


#endif /* DEFINE_H */
