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
 * $Id: module.c,v 1.6 2002-05-06 20:09:47 leonb Exp $
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

#define MODULE_USED      0x01
#define MODULE_DLD       0x02
#define MODULE_SO        0x04
#define MODULE_EXEC      0x08
#define MODULE_INIT      0x20
#define MODULE_CLASS     0x40


struct module {
  int flags;
  struct module *prev;
  struct module *next;
  dlopen_handle_t *handle;
  char *filename;
  void (*init)(void);
  at *backptr;
  at *defs;
};

static struct alloc_root module_alloc = {
  NULL, NULL, sizeof(struct module), 16 
};

static struct module root = { 
  (MODULE_USED|MODULE_INIT|MODULE_EXEC), &root, &root 
};

static struct module *current = &root;

static at *atroot;


/* ---------- THE MODULE OBJECT ----------- */

static void 
module_dispose(at *p)
{
  struct module *m = p->Object;
  UNLOCK(m->defs);
  m->defs = NIL;
  if (m->filename)
    free(m->filename);
  m->filename = 0;
  m->handle = 0;
  if (m != &root) 
    {
      m->prev->next = m->next;
      m->next->prev = m->prev;
      deallocate(&module_alloc, (struct empty_alloc *) m);
    }
}

static void 
module_action(at *p, void (*action) (at *p))
{
  struct module *m = p->Object;
  (*action)(m->defs);
}

class module_class = {
  module_dispose,
  module_action,
  generic_name,
  generic_eval,
  generic_listeval,
};


DX(xmodule_list)
{
  at *ans = NIL;
  at **where = &ans;
  struct module *m = &root;
  ARG_NUMBER(0);
  do {
    *where = new_cons(m->backptr, NIL);
    where = &((*where)->Cdr);
    m = m->next;
  } while (m != &root);
  return ans;
}

DX(xmodule_filename)
{
  at *p;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module", p);
  m = p->Object;
  if (m->filename)
    return new_string(m->filename);
  return NIL;
}

DX(xmodule_executable_p)
{
  at *p;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module", p);
  m = p->Object;
  if (m->flags & MODULE_EXEC)
    return true();
  return NIL;
}

DX(xmodule_unloadable_p)
{
  at *p;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module", p);
  m = p->Object;
  if (m->flags & (MODULE_SO|MODULE_CLASS))
    return NIL;
  return true();
}

DX(xmodule_defs)
{
  at *p;
  at *ans = NIL;
  at **where = &ans;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module", p);
  m = p->Object;
  p = m->defs;
  while (CONSP(p) && CONSP(p->Car))
    {
      LOCK(p->Car->Car);
      LOCK(p->Car->Cdr);
      *where = cons(cons(p->Car->Car,p->Car->Cdr),NIL);
      where = &((*where)->Cdr);
      p = p->Cdr;
    }
  return ans;
}



/* --------- LOCATE PRIMITIVES --------- */



at *
find_primitive(at *module, at *name)
{
  at *p = root.defs;

  if (module) {
    if (! EXTERNP(module, &module_class))
      error(NIL,"Not a module descriptor", module);
    p = ((struct module*)(module->Object))->defs;
  }
  while (CONSP(p))
    {
      if (CONSP(p->Car))
        if (p->Car->Car == name)
          {
            p = p->Car->Cdr;
            LOCK(p);
            return p;
          }
      p = p->Cdr;
    }
  return NIL;
}




/* --------- INITIALISATION CODE --------- */

static const char *program_name = 0;
static int dynlink_initialized = 0;



static void 
dynlink_error(at *p)
{
  char *err;
  char buffer[80];
  strcpy(buffer,"Dynamic linking error");
#ifdef DLDBFD
  if (dld_errno) 
    {
      strcpy(buffer,"dld/bfd error: ");
      strcat(buffer, dld_errno);
    }
#endif  
#ifdef DLOPEN
  err = dlerror();
  if (err)
    {
      strcpy(buffer,"dlopen error: ");
      strcat(buffer, err);
    }
#endif
  error(NIL, buffer, p);
}

static void
dynlink_init(void)
{
  if (! dynlink_initialized)
    {
#ifdef DLDBFD
      char *exec;
      if (!program_name)
        error(NIL,"Internal error (program_name unset)",NIL);        
      exec = dld_find_executable(program_name);      
      if (!exec)
        error(NIL,"Cannot find lush executable",NIL);
      if (dld_init(exec))
        dynlink_error(NIL);
      free(exec);
#endif
      dynlink_initialized = 1;
    }
}

static void
dynlink_hook(struct module *m, char *hookname)
{
  at *hook = named(hookname);
  at *ans = send_message(NIL, m->backptr, hook, NIL);
  UNLOCK(hook);
  UNLOCK(ans);
}







/* --------- XXX_DEFINE FUNCTIONS --------- */

static at *
module_priminame(at *name)
{
  if (current != &root)
    return new_cons(current->backptr, name);
  LOCK(name);
  return name;
}

static void
module_def(at *name, at *val)
{
  /* Check name and add definition */
  if (! EXTERNP(name, &symbol_class))
    error("module.c/module_def",
          "internal error (symbol expected)",name);
  current->defs = cons(new_cons(name,val),current->defs);
  /* Root definitions are also writtin into symbols */
  if (current == &root)
    {
      struct symbol *symb = name->Object;
      if (symb->mode == SYMBOL_LOCKED)
        error("module.c/module_def",
              "internal error (multiple definition)", name);
      var_set(name, val);
      symb->mode = SYMBOL_LOCKED;
    }
}

void 
class_define(char *name, class *cl)
{
  at *symb = new_symbol(name);
  at *classat = new_extern(&class_class,cl);
  cl->classname = symb;
  cl->priminame = module_priminame(symb);
  cl->backptr = classat;
  cl->goaway = 0;
  cl->dontdelete = 0;
  cl->slotssofar = 0;
  cl->super = 0;
  cl->atsuper = NIL;
  cl->nextclass = NIL;
  module_def(symb, classat);
  current->flags |= MODULE_CLASS;
  UNLOCK(classat);
}

void 
dx_define(char *name, at *(*addr) (int, at **))
{
  at *symb = new_symbol(name);
  at *priminame = module_priminame(symb);
  at *func = new_dx(priminame, addr);
  module_def(symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}

void 
dy_define(char *name, at *(*addr) (at *))
{
  at *symb = new_symbol(name);
  at *priminame = module_priminame(symb);
  at *func = new_dy(priminame, addr);
  module_def(symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}







/* --------- INITIALISATION CODE --------- */




void 
init_module(char *progname)
{
  class_define("MODULE",&module_class);
  /* Create root module */
  atroot = new_extern(&module_class, &root);
  root.backptr = atroot;
  protect(atroot);
  /* Functions */
  dx_define("module_list", xmodule_list);
  dx_define("module_filename", xmodule_filename);
  dx_define("module_defs", xmodule_defs);
  dx_define("module_executable_p", xmodule_executable_p);
  dx_define("module_unloadable_p", xmodule_unloadable_p);
  /* Cache program name for later use */
  program_name = progname;
}
