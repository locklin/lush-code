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
 * $Id: messages.c,v 1.2 2004-08-30 23:24:30 leonb Exp $
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "messages.h"

mysvm_message_t mysvm_message_level = MYSVM_INFO;

mysvm_message_proc_t  *mysvm_message_proc = 0;


static void 
defaultproc(mysvm_message_t level, const char *fmt, va_list ap)
{
  if (level <= mysvm_message_level)
    vprintf(fmt, ap);
#ifdef LUSH
  extern void run_time_error(const char *s);
  if (level <= MYSVM_ERROR)
    run_time_error("mysvm error");
#endif
  if (level <= MYSVM_ERROR)
    abort();
}

void 
mysvm_error(const char *fmt, ...) 
{ 
  mysvm_message_proc_t *f = mysvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(MYSVM_ERROR,fmt,ap);
  va_end(ap); 
  abort();
}

void 
mysvm_warning(const char *fmt, ...) 
{ 
  mysvm_message_proc_t *f = mysvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(MYSVM_WARNING,fmt,ap);
  va_end(ap); 
}

void 
mysvm_info(const char *fmt, ...) 
{ 
  mysvm_message_proc_t *f = mysvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(MYSVM_INFO,fmt,ap);
  va_end(ap); 
}

void 
mysvm_debug(const char *fmt, ...) 
{ 
  mysvm_message_proc_t *f = mysvm_message_proc;
  va_list ap; 
  va_start(ap,fmt);
  if (! f) 
    f = defaultproc;
  (*f)(MYSVM_DEBUG,fmt,ap);
  va_end(ap); 
}

void 
mysvm_assertfail(const char *file,int line)
{
  mysvm_error("Assertion failed: %s:%d\n", file, line);
}

