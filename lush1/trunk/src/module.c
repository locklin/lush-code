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
 * $Id: module.c,v 1.30 2003-03-04 18:18:12 leonb Exp $
 **********************************************************************/



#include "header.h"
#include "dh.h"


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
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
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

#ifndef DLOPEN
typedef void* dlopen_handle_t;
#endif

/* ------ THE MODULE DATA STRUCTURE -------- */

#define MODULE_USED       0x01
#define MODULE_DLD        0x02
#define MODULE_SO         0x04
#define MODULE_EXEC       0x08
#define MODULE_INIT       0x20
#define MODULE_CLASS      0x40
#define MODULE_STICKY     0x80


struct module {
  int flags;
  struct module *prev;
  struct module *next;
  dlopen_handle_t *handle;
  char *filename;
  char *initname;
  void *initaddr;
  at *backptr;
  at *defs;
  at *hook;
};

static struct alloc_root module_alloc = {
  NULL, NULL, sizeof(struct module), 16 
};

static struct module root = { 
  (MODULE_USED|MODULE_STICKY|MODULE_INIT|MODULE_EXEC), &root, &root 
};

static struct module *current = &root;

static at *atroot;

static int check_executability = FALSE;
static void check_exec();

/* ---------- THE MODULE OBJECT ----------- */

static char *module_maybe_unload(struct module *m);

static void 
module_dispose(at *p)
{
  struct module *m;
  m = p->Object;
  /* Try to unload */
  if (m->prev && m->next && m!=&root)
    module_maybe_unload(m);
  /* Unlock dependencies */
  UNLOCK(m->defs);
  m->defs = NIL;
  UNLOCK(m->hook);
  m->hook = NIL;
  m->backptr = NIL;
  if (m == &root)
    return;
  /* Attempt to unload */
  if (m->prev && m->next)
    m->prev->next = m->next;
  if (m->prev && m->next)
    m->next->prev = m->prev;    
  /* Free memory */
  if (m->filename)
    free(m->filename);
  if (m->initname)
    free(m->initname);
  m->filename = 0;
  m->initname = 0;
  m->initaddr = 0;
  m->handle = 0;
  deallocate(&module_alloc, (struct empty_alloc *) m);
}

static void 
module_action(at *p, void (*action) (at *p))
{
  struct module *m = p->Object;
  (*action)(m->defs);
  (*action)(m->hook);
}

static void
module_serialize(at **pp, int code)
{
  char *fname = 0;
  char *aname = 0;
  struct module *m = 0;
  at *junk = 0;
  
  if (code != SRZ_READ)
    {
      m = (*pp)->Object;
      fname = "";
      if (m != &root)
        if (! (fname = relative_fname(lushdir_name, m->filename)))
          fname = m->filename;
    }
  serialize_string(&fname, code, -1);
  if (code == SRZ_READ)
    {
      m = &root;
      if (fname[0] == 0)
        {
        already:
          LOCK(m->backptr);
          *pp = m->backptr;
          UNLOCK(junk);
          serialize_atstar(&junk, code); 
          UNLOCK(junk);
          return;
        }
      junk = new_string(concat_fname(lushdir_name, fname));
      aname = SADD(junk->Object);
      while ( (m = m->next) != &root)
        if (!strcmp(aname, m->filename))
          goto already;
      (*pp) = module_load(aname, NIL);
      m = (*pp)->Object;
      free(fname);
      check_exec();
    }
  serialize_atstar(&m->hook, code);  
  UNLOCK(junk);
}

class module_class = {
  module_dispose,
  module_action,
  generic_name,
  generic_eval,
  generic_listeval,
  module_serialize,
};

at *
module_list(void)
{
  at *ans = NIL;
  at **where = &ans;
  struct module *m = &root;
  do {
    *where = new_cons(m->backptr, NIL);
    where = &((*where)->Cdr);
    m = m->next;
  } while (m != &root);
  return ans;
}

DX(xmodule_list)
{
  ARG_NUMBER(0);
  return module_list();
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

DX(xmodule_init_function)
{
  at *p;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module", p);
  m = p->Object;
  if (m->initname)
    return new_string(m->initname);
  if (m == &root)
    return new_safe_string("init_lush");
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
  if (m->flags & MODULE_STICKY)
     return NIL;
     return true();
}

DX(xmodule_defs)
{
  at *p;
  at *ans = NIL;
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
      ans = cons(new_cons(p->Car->Car,p->Car->Cdr),ans);
      p = p->Cdr;
    }
  return ans;
}



/* --------- DYNLINK UTILITY FUNCTIONS --------- */



static int dynlink_initialized = 0;

static void 
dynlink_error(at *p)
{
  char *err;
  char buffer[80];
  strcpy(buffer,"Dynamic linking error");
#ifdef DLDBFD
  if ((err = dld_errno))
    {
      strcpy(buffer,"dld/bfd error\n*** ");
      strcat(buffer, err);
      error(NIL, buffer, p);
    }
#endif  
#ifdef DLOPEN
  if ((err = dlerror()))
    {
      strcpy(buffer,"dlopen error\n*** ");
      strcat(buffer, err);
      error(NIL, buffer, p);
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
      if (!root.filename)
        error(NIL,"Internal error (program_name unset)",NIL);        
      if (dld_init(root.filename))
        dynlink_error(NIL);
#endif
      dynlink_initialized = 1;
    }
}

static void
dynlink_hook(struct module *m, char *hookname)
{
  if (m->hook && error_doc.ready_to_an_error)
    {
      at *arg = cons(named(hookname), new_cons(m->backptr, NIL));
      at *ans = apply(m->hook, arg);
      UNLOCK(arg);
      UNLOCK(ans);
    }
}




/* --------- EXECUTABILITY CHECK --------- */



static void
update_exec_flag(struct module *m)
{
  int oldstate = (m->flags & MODULE_EXEC);
  int newstate = TRUE;
#ifdef DLDBFD
  if (m->flags & MODULE_DLD)
    {
      newstate = FALSE;
      if (m->initname)
        newstate = dld_function_executable_p(m->initname);
    }
#endif
  m->flags &= ~MODULE_EXEC;
  if (newstate)
    m->flags |= MODULE_EXEC;
  if (m->defs && newstate != oldstate)
    dynlink_hook(m, "exec");
}

static void
update_init_flag(struct module *m)
{
  int status;
  
  if (m->flags & MODULE_INIT) 
    return;
  if (! (m->flags & MODULE_EXEC))
    return;
  if (! (m->initaddr && m->initname))
    return;
  
  /* Execute init function */
  if (! strcmp(m->initname, "init_user_dll"))
    {
      /* TL3 style init function */
      int (*call)(int, int) = m->initaddr;
      current = m;
      status = (*call)(TLOPEN_MAJOR, TLOPEN_MINOR);
      current = &root;
    }
  else
    {
      /* SN3 style init function */
      int *majptr, *minptr, maj, min;
      strcpy(string_buffer, "majver_");
      strcat(string_buffer, m->initname + 5);
      majptr = (int*) dld_get_symbol(string_buffer);
      strcpy(string_buffer, "minver_");
      strcat(string_buffer, m->initname + 5);
      minptr = (int*) dld_get_symbol(string_buffer);
      maj = ((majptr) ? *majptr : TLOPEN_MAJOR);
      min = ((minptr) ? *minptr : TLOPEN_MINOR);
      status = -2;
      if (maj == TLOPEN_MAJOR && min >= TLOPEN_MINOR)
        {
          void (*call)(void) = m->initaddr;
          current = m;
          (*call)();
          current = &root;
          status = 0;
        }
    }
  /* Mark new status */
  if (status >= 0)
    {
      m->flags |= MODULE_INIT;
      dynlink_hook(m, "init");
    }
  else 
    {
      m->initaddr = 0;
      if (status == -2)
        fprintf(stderr,
                "*** WARNING: Module %s\n"
                "***   Initialization failed because of mismatched version number\n",
                m->filename );
      else if (status == -1)
        fprintf(stderr,
                "*** WARNING: Module %s\n"
                "***   Initialization failed\n",
                m->filename );
    }
}

static void 
check_exec()
{
  if (check_executability)
    {
      struct module *m;
      check_executability = FALSE;
      for (m=root.next; m!=&root; m=m->next)
        update_exec_flag(m);
      for (m=root.next; m!=&root; m=m->next)
        if (! (m->flags & MODULE_INIT))
          update_init_flag(m);
    }
}

void
check_primitive(at *prim)
{
  if (CONSP(prim) && EXTERNP(prim->Car, &module_class))
    {
      struct module *m = prim->Car->Object;
      if (check_executability)
        check_exec(TRUE);
      if (! (m->flags & MODULE_EXEC))
        error(NIL,"Function belongs to a partially linked module", 
              prim->Cdr);
    }
}



/* --------- MODULE_UNLOAD --------- */


DX(xmodule_never_unload)
{
  at *p;
  struct module *m;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module",p);
  m = p->Object;
  m->flags |= MODULE_STICKY;
  LOCK(p);
  return p;
}


DX(xmodule_depends)
{
  at *p;
  struct module *m, *mc;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &module_class))
    error(NIL,"Not a module",p);
  m = p->Object;
  p = NIL;
  if (m == &root)
    return NIL;
  /* Simulate unlink */
  if (dld_simulate_unlink_by_file(m->filename) < 0)
    dynlink_error(NIL);
  for (mc = root.next; mc != &root; mc = mc->next)
    if (mc->initname && mc->defs)
      if (! dld_function_executable_p(mc->initname))
        {
          LOCK(mc->backptr);
          p = cons(mc->backptr, p);
        }
  /* Reset everything as it should be */
  dld_simulate_unlink_by_file(0);
  /* Return */
  return p;
}


static void
cleanup_module(struct module *m)
{
  at *p;
  at *classes = 0;
  struct module *mc;
  
  extern void delete_at_special(at *, int);
  
  /* 1 --- No cleanup when not ready for errors */
  if (! error_doc.ready_to_an_error)
    return;
  
  /* 2 --- Collect impacted classes */
  if (dld_simulate_unlink_by_file(m->filename) < 0)
    dynlink_error(NIL);
  for (mc = root.next; mc != &root; mc = mc->next)
    if (mc->initname && mc->defs && (mc->flags & MODULE_CLASS))
      if (! dld_function_executable_p(mc->initname))
        {
          /* This module defines classes that become non executable.
             Collect impacted classes. */
          p = mc->defs;
          while (CONSP(p))
            {
              if (CONSP(p->Car)) {
                at *q = p->Car->Car;
                if (q && !(q->flags & X_FUNCTION)) {
                  LOCK(q);
                  classes = cons(q, classes);
                }
              }
              p = p->Cdr;
            }
        }
  dld_simulate_unlink_by_file(0);
  
  /* 3 --- Zap instances of impacted classes */
  p = classes;
  while (CONSP(p))
    {
      at *q = p->Car;
      if (EXTERNP(q, &class_class))
        {
          int n;
          class *cl = q->Object;
          /* Mark cside objects as unlinked */
          if (cl->classdoc)
            {
              n = lside_mark_unlinked(cl->classdoc);
              if (n > 0)
                fprintf(stderr,"*** WARNING: "
                        "marked %d instances of class %s as unlinked\n", 
                        n, pname(q));
            }
          if (! cl->goaway)
            {
              n = 0;
              begin_iter_at(x) 
                {
                  if (EXTERNP(x, cl)) 
                    {
                      delete_at(x);
                      n += 1;
                    }
                }
              end_iter_at(x);
              if (n > 0)
                fprintf(stderr,"*** WARNING: "
                        "destroyed %d instances of class %s\n", n, pname(q));
            }
        }
      p = p->Cdr;
    }
  UNLOCK(classes);
  
  /* 4 --- Zap primitives defined by this module. */
  if (m->defs)
    for (p = m->defs; CONSP(p); p = p->Cdr)
      if (CONSP(p->Car))
        {
          at *q = p->Car->Car;
          if (EXTERNP(q, &class_class))
            {
              class *cl = q->Object;
              cl->classdoc = 0;
              if (! cl->goaway)
                delete_at_special(q, FALSE);
            }
          else
            {
              delete_at(q);
            }
        }
  UNLOCK(m->defs);
  m->defs = NIL;
}


static char *
module_maybe_unload(struct module *m)
{
  if (m == &root)
    return "Cannot unload root module";
  if (! (m->flags & MODULE_DLD))
    return "Cannot unload module of this type";
  if (m->flags & MODULE_STICKY)
    return "Cannot unload this module";
  /* Advertize */
  dynlink_hook(m, "unlink");
  /* Cleanup definitions */
  cleanup_module(m);
  /* Unload module */
#if DLOPEN
  if (m->flags & MODULE_SO)
    if (dlclose(m->handle))
      dynlink_error(new_string(m->filename));
#endif
#if DLDBFD
  if (m->flags & MODULE_DLD)
    if (dld_unlink_by_file(m->filename, 1))
      dynlink_error(new_string(m->filename));
#endif
  check_executability = TRUE;
  /* Remove the module from the chain */
  m->prev->next = m->next;
  m->next->prev = m->prev;
  m->next = m->prev = 0;
  unprotect(m->backptr);
  return NULL;
}

void
module_unload(at *atmodule)
{
  char *err;
  if (! EXTERNP(atmodule, &module_class))
    error(NIL,"Not a module", atmodule);
  if ((err = module_maybe_unload(atmodule->Object)))
    error(NIL, err, atmodule);
}

DX(xmodule_unload)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  module_unload(APOINTER(1));
  check_exec(TRUE);
  return NIL;
}









/* --------- MODULE_LOAD --------- */




at *
module_load(char *filename, at *hook)
{
  char *l;
  int len;
  at *ans;
  struct module *m;
  int dlopen;
  dlopen_handle_t handle = 0;
  /* Check that file exists */
  filename = concat_fname(NULL, filename);
  ans = new_string(filename);
  filename = SADD(ans->Object);
  if (! filep(filename))
    error(NIL,"file not found",new_string(filename));
  /* Check if the file extension indicates a DLL */
  dlopen = 0;
  l = filename + strlen(filename);
  while (l>filename && isdigit(l[-1]))
    {
      while (l>filename && isdigit(l[-1]))
        l -= 1;
      if (l>filename && l[-1]=='.')
        l -= 1;
    }
  if (l[0]==0 || l[0]=='.')
    {
      len = strlen(EXT_DLL);
      if (len>0 && !strncmp(EXT_DLL, l-len, len))
        dlopen = 1;
    }
  /* Initialize */
  dynlink_init();
  /* Are we reloading an existing module */
  for (m = root.next; m != &root; m = m->next)
    if (!strcmp(filename, m->filename))
      break;
  if (m != &root)
    if (module_maybe_unload(m))
      {
        LOCK(m->backptr);
        return m->backptr;
      }
  /* Allocate */
  m = allocate(&module_alloc);
  m->flags = MODULE_USED ;
  m->prev = 0;
  m->next = 0;
  m->handle = handle;
  m->filename = filename = strdup(filename);
  m->initname = 0;
  m->initaddr = 0;
  m->defs = 0;
  m->hook = hook;
  m->backptr = 0;
  UNLOCK(ans);
  ans = new_extern(&module_class, m);
  m->backptr = ans;
  LOCK(hook);
  if (! m->filename) 
    error(NIL,"out of memory", NIL);
  /* Load the file */
  if (dlopen)
    {
#if DLDBFD && DLOPEN
      if (! (handle = dld_dlopen(m->filename, RTLD_NOW|RTLD_GLOBAL)))
        dynlink_error(new_string(m->filename));
#elif DLOPEN
      if (! (handle = dlopen(m->filename, RTLD_NOW|RTLD_GLOBAL)))
        dynlink_error(new_string(m->filename));
#else
      error(NIL,"Dynlinking this file is not supported (need dlopen)", 
            new_string(m->filename));
#endif
      m->flags |= MODULE_SO | MODULE_STICKY;
    }
  else
    {
#if DLDBFD
      if (dld_link(m->filename))
        dynlink_error(new_string(m->filename));
#else
      error(NIL,"Dynlinking this file is not supported (need bfd)", 
            new_string(m->filename));
#endif      
      m->flags |= MODULE_DLD;
    }
  /* Search init function */
#if DLOPEN
  if (dlopen)
    {
      strcpy(string_buffer, "init_user_dll");
      m->initaddr = dlsym(handle, string_buffer);
    }
#endif
#if DLDBFD
  if (! m->initaddr)
    {
      strcpy(string_buffer, "init_");
      strcat(string_buffer, basename(m->filename, 0));
      if ((l = strchr(string_buffer, '.')))
        l[0] = 0;
      m->initaddr = dld_get_func(string_buffer);
    }
#endif
  if (m->initaddr)
    {
      m->initname = strdup(string_buffer);
      if (! m->initname)
        error(NIL,"Out of memory", NIL);
    }
  /* Terminate */
  protect(ans);
  m->prev = root.prev;
  m->next = &root;
  m->prev->next = m;
  m->next->prev = m;
  check_executability = TRUE;
  return ans;
}

DX(xmodule_load)
{
  at *ans;
  at *hook = NIL;
  ALL_ARGS_EVAL;
  if (arg_number != 1)
    ARG_NUMBER(2);
  if (arg_number >= 2)
    hook = APOINTER(2);
  if (hook && !(hook->flags & X_FUNCTION))
    error(NIL,"Not a function", hook);
  ans = module_load(ASTRING(1), hook);
  check_exec(TRUE);
  return ans;
}






/* --------- SN3 FUNCTIONS --------- */



DX(xmod_create_reference)
{
  int i;
  ALL_ARGS_EVAL;
#ifdef DLDBFD
  dynlink_init();
  for (i=1; i<=arg_number; i++)
    dld_create_reference(ASTRING(1));
  return NEW_NUMBER(dld_undefined_sym_count);
#else
  return NIL;
#endif
}

DX(xmod_compatibility_flag)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
#ifdef DLDBFD
  dld_compatibility_flag = ( APOINTER(1) ? 1 : 0 );
  return (dld_compatibility_flag ? true() : NIL);
#else
  return NIL;
#endif
}

DX(xmod_undefined)
{
  at *p = NIL;
  at **where = &p;
#ifdef DLDBFD
  if (dynlink_initialized)
    {
      int i;
      char ** dld_undefined_sym_list = (char**)dld_list_undefined_sym();
      p = NIL;
      where = &p;
      for (i=0; i<dld_undefined_sym_count; i++) {
        *where = cons( new_string(dld_undefined_sym_list[i]), NIL);
        where = &((*where)->Cdr);
      }
      free(dld_undefined_sym_list);
    }
#endif
  return p;
}






/* --------- LOCATE PRIMITIVES --------- */



at *
find_primitive(at *module, at *name)
{
  at *q = 0;
  at *p = root.defs;
  at *clname = 0;

  /* Missing MODULE means root. */
  if (module) 
    {
      if (! EXTERNP(module, &module_class))
        error(NIL,"Not a module descriptor", module);
      p = ((struct module*)(module->Object))->defs;
    }
  /* Name can be NAME or (CLASSNAME . NAME) */
  if (CONSP(name))
    {
      clname = name->Car;
      name = name->Cdr;
    }
  /* Iterate over module definitions */
  while (CONSP(p))
    {
      q = p->Car;
      if (CONSP(q))
        {
          if (clname)
            {
              /* Looking for a method:
                 - names must match, 
                 - clname must match class. */
              if ( CONSP(q->Cdr) &&
                   q->Cdr->Cdr == name &&
                   EXTERNP(q->Cdr->Car,&class_class) &&
                   ((class*)(q->Cdr->Car->Object))->classname == clname )
                break;
            } 
          else
            {
              /* Looking for a function: 
                 - names must match. */
              if (name == q->Cdr)
                break;
            }
        }
      p = p->Cdr;
      q = 0;
    }
  if (q)
    {
      LOCK(q->Car);
      return q->Car;
    }
  return NIL;
}



/* --------- XXX_DEFINE FUNCTIONS --------- */




static at *
module_priminame(at *name)
{
  /* NAME for root module.
   * (MODULE . NAME) for other modules. 
   */ 
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
  current->defs = cons(new_cons(val, name),current->defs);
  /* Root definitions are also written into symbols */
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

static at *
module_method_priminame(class *cl, at *name)
{
  /* (CLASSNAME . NAME) for root module.
   * (MODULE CLASSNAME . NAME) for other modules. 
   */ 
  at *n = new_cons(cl->classname, name);
  if (current == &root)
    return n;
  LOCK(current->backptr);
  return cons(current->backptr, n);
}

static void
module_method_def(class *cl, at *name, at *val)
{
  /* Check name and add definition */
  if (! EXTERNP(name, &symbol_class))
    error("module.c/module_def",
          "internal error (symbol expected)",name);
  if (! cl->backptr)
    error("module.c/module_def",
          "defining method before class_define",name);
  LOCK(val);
  LOCK(name);
  LOCK(cl->backptr);
  current->defs = cons(cons(val, cons(cl->backptr, name)), current->defs);
  /* Root definitions are also written into classes */
  if (current == &root)
    putmethod(cl, name, val);
}


void 
class_define(char *name, class *cl)
{
  at *symb;
  at *classat;
  
  symb = new_symbol(name);
  classat = new_extern(&class_class,cl);
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

void 
dxmethod_define(class *cl, char *name, at *(*addr) (int, at **))
{
  at *symb = new_symbol(name);
  at *priminame = module_method_priminame(cl, symb);
  at *func = new_dx(priminame, addr);
  module_method_def(cl, symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}


void 
dymethod_define(class *cl, char *name, at *(*addr) (at *))
{
  at *symb = new_symbol(name);
  at *priminame = module_method_priminame(cl, symb);
  at *func = new_dy(priminame, addr);
  module_method_def(cl, symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}


void 
dhclass_define(char *name, dhclassdoc_t *kclass)
{
  at *symb;
  at *classat;
  class *cl;
  
  symb = new_symbol(name);
  classat = new_dhclass(symb, kclass);
  cl = classat->Object;
  UNLOCK(cl->priminame);
  cl->priminame = module_priminame(symb); 
  module_def(symb, classat);
  current->flags |= MODULE_CLASS;
  UNLOCK(classat);
  UNLOCK(symb);
}


void 
dh_define(char *name, dhdoc_t *kname)
{
  at *symb = new_symbol(name);
  at *priminame = module_priminame(symb);
  at *func = new_dh(priminame, kname);
  module_def(symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}

void 
dhmethod_define(dhclassdoc_t *kclass, char *name, dhdoc_t *kname)
{
  at *symb, *priminame, *func;
  dhrecord *drec;
  class *cl;

  symb = new_symbol(name);
  if (! kclass->lispdata.atclass)
    error(NIL,"Internal: dhmethod_define called before dhclass_define", symb);
  for (drec = kclass->argdata;
       drec->op != DHT_END_CLASS && drec->op != DHT_NIL;
       drec = drec->end)
    if (drec->op == DHT_METHOD)
      if (drec->arg == kname)
        break;
  if (drec->arg != kname)
    error(NIL,"Internal: method is not listed in classdoc", symb);
  cl = kclass->lispdata.atclass->Object;
  priminame = module_method_priminame(cl, symb);
  func = new_dh(priminame, kname);
  module_method_def(cl, symb, func);
  UNLOCK(func);
  UNLOCK(symb);
  UNLOCK(priminame);
}




/* --------- INITIALISATION CODE --------- */




void 
init_module(char *progname)
{
  class_define("MODULE",&module_class);
  module_class.dontdelete = TRUE;

  /* Create root module */
#ifdef DLDBFD
  root.filename = dld_find_executable(progname);
#else
  root.filename = progname;
#endif  
  atroot = new_extern(&module_class, &root);
  root.backptr = atroot;
  protect(atroot);

  /* Functions */
  dx_define("module-list", xmodule_list);
  dx_define("module-filename", xmodule_filename);
  dx_define("module-defs", xmodule_defs);
  dx_define("module-initname", xmodule_init_function);
  dx_define("module-executable-p", xmodule_executable_p);
  dx_define("module-unloadable-p", xmodule_unloadable_p);
  dx_define("module-load", xmodule_load);
  dx_define("module-unload", xmodule_unload);
  dx_define("module-depends", xmodule_depends);
  dx_define("module-never-unload", xmodule_never_unload);
  
  /* SN3 functions */
  dx_define("mod-create-reference", xmod_create_reference);
  dx_define("mod-compatibility-flag", xmod_compatibility_flag);
  dx_define("mod-undefined",xmod_undefined);
}


