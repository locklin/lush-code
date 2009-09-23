/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

/***********************************************************************
 * $Id: messages.h,v 1.1 2005/02/10 14:49:11 leonb Exp $
 **********************************************************************/

#ifndef MESSAGES_H
#define MESSAGES_H


#include <stdarg.h>


#ifdef __cplusplus__
extern "C" { 
#if 0
}
#endif
#endif


/* Message levels */

typedef enum { LASVM_ERROR, 
	       LASVM_WARNING, 
	       LASVM_INFO, 
	       LASVM_DEBUG }  lasvm_message_t;


/* lasvm_message_level ---
   Default routine will print every message 
   more severe than this.
*/

extern lasvm_message_t lasvm_message_level;

/* lasvm_message_proc ---
   The routine to handle messages.
   The default routine will print everything
   with message level less than lasvm_message_level
   and abort when message level is LASVM_ERROR.
*/

typedef void lasvm_message_proc_t(lasvm_message_t, const char*, va_list);
extern lasvm_message_proc_t  *lasvm_message_proc;


/* lasvm_error, lasvm_warning, lasvm_info, lasvm_debug ---
   Convenience functions to display messages and signal errors.
*/
extern void lasvm_error(const char *fmt, ...);
extern void lasvm_warning(const char *fmt, ...);
extern void lasvm_info(const char *fmt, ...);
extern void lasvm_debug(const char *fmt, ...);


/* ASSERT --- 
   Macro for quickly inserting debugging checks.
*/

#ifdef NDEBUG
# define ASSERT(x) /**/
#else
# define ASSERT(x) do { if (!(x))\
                     lasvm_assertfail(__FILE__,__LINE__);\
                   } while(0)
#endif

void lasvm_assertfail(const char *file,int line);


#ifdef __cplusplus__
}
#endif
#endif
