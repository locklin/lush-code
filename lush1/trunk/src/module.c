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
 * $Id: module.c,v 1.3 2002-05-03 18:46:39 leonb Exp $
 **********************************************************************/


#include "header.h"


/* ------ DLOPEN, BFD, ETC. -------- */

#if HAVE_LIBBFD
# define DLDBFD 1
# include "dldbfd.h"
#endif

#if HAVE_DLFCN_H
#if HAVE_DLOPEN
#include <dlfcn.h>
#define DLOPEN 1
typedef void* dlopen_handle_t;
#endif
#endif

#if HAVE_DL_H 
#if HAVE_LIBDLD
#include <dl.h>
#define DLOPEN 1
typedef shl_t dlopen_handle_t
#endif
#endif

#ifdef WIN32
#define DLOPEN 1
typedef void* dlopen_handle_t;
#endif

#if DLOPEN
#define RTLD_SPECIAL -1
#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif
#endif

#ifndef DLDBFD
#ifdef DLOPEN
#ifdef HAVE_DL_H

/* DLOPEN emulation under HP/UX */
static dlopen_handle_t dlopen(char *soname, int mode)
{ 
  return shl_load(soname,BIND_IMMEDIATE|BIND_NONFATAL|
		  BIND_NOSTART|BIND_VERBOSE, 0L ); 
}
static void dlclose(dlopen_handle_t hndl)
{ 
  shl_unload(hndl); 
}
static void* dlsym(dlopen_handle_t hndl, char *sym)
{ 
  void *addr = 0;
  if (shl_findsym(&hndl,sym,TYPE_PROCEDURE,&addr) >= 0)
    return addr;
  return 0;
}
static char* dlerror(void)
{
  return "Function shl_load() has failed";
}

#endif
#endif
#endif






/* ------ THE MODULE DATA STRUCTURE -------- */

#define MODULE_ROOT      0x01
#define MODULE_DLD       0x02
#define MODULE_SO        0x04
#define MODULE_EXEC      0x08
#define MODULE_CAN_INIT  0x10
#define MODULE_INIT      0x20


struct module {
  short used;
  short flags;
  struct module *prev;
  struct module *next;
  dlopen_handle_t *handle;
  const char *name;
  at *defs;
};

static struct module root = { 1, MODULE_ROOT|MODULE_INIT, &root, &root };
static struct module *current = &root;


/* ---------- THE MODULE OBJECT ----------- */



/* --------- XXX_DEFINE FUNCTIONS --------- */


class *rootclasslist = 0;

void 
class_define(char *name, class *cl)
{
  at *symb;
  at *classat;
  symb = new_symbol(name);
  classat = new_extern(&class_class,cl);
  cl->classname = symb;
  cl->backptr = classat;
  cl->goaway = 0;
  cl->dontdelete = 0;
  cl->slotssofar = 0;
  cl->super = 0;
  cl->atsuper = NIL;
  cl->nextclass = rootclasslist;
  rootclasslist = cl;
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, classat);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
  }
  UNLOCK(classat);
}

void 
dx_define(char *name, at *(*addr) (int, at **))
{
  at *func, *symb;

  func = new_dx(addr);
  symb = new_symbol(name);
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, func);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
    ((struct function*) (func->Object))->evaluable_list = symb;
  }
  UNLOCK(func);
  UNLOCK(symb)
}

void 
dy_define(char *name, at *(*addr) (at *))
{
  at *func, *symb;

  func = new_dy(addr);
  symb = new_symbol(name);
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, func);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
    ((struct function*) (func->Object))->evaluable_list = symb;
  }
  UNLOCK(func);
  UNLOCK(symb)
}




/* --------- INITIALISATION CODE --------- */

void 
init_module(void)
{
}
