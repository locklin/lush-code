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
 * $Id: messages.c,v 1.1 2005/02/10 14:49:11 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "messages.h"

lasvm_message_t lasvm_message_level = LASVM_INFO;

lasvm_message_proc_t  *lasvm_message_proc = 0;


static void 
defaultproc(lasvm_message_t level, const char *fmt, va_list ap)
{
  if (level <= lasvm_message_level)
    vprintf(fmt, ap);
#ifdef LUSH
  if (level <= LASVM_ERROR)
    {
      extern void run_time_error(const char *s);
      run_time_error("lasvm error");
    }
#endif
  if (level <= LASVM_ERROR)
    abort();
}

void 
lasvm_error(const char *fmt, ...) 
{ 
  lasvm_message_proc_t *f = lasvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(LASVM_ERROR,fmt,ap);
  va_end(ap); 
  abort();
}

void 
lasvm_warning(const char *fmt, ...) 
{ 
  lasvm_message_proc_t *f = lasvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(LASVM_WARNING,fmt,ap);
  va_end(ap); 
}

void 
lasvm_info(const char *fmt, ...) 
{ 
  lasvm_message_proc_t *f = lasvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(LASVM_INFO,fmt,ap);
  va_end(ap); 
}

void 
lasvm_debug(const char *fmt, ...) 
{ 
  lasvm_message_proc_t *f = lasvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(LASVM_DEBUG,fmt,ap);
  va_end(ap); 
}

void 
lasvm_assertfail(const char *file,int line)
{
  lasvm_error("Assertion failed: %s:%d\n", file, line);
}

