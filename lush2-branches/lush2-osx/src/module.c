/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann LeCun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as 
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *  MA  02110-1301  USA
 *
 ***********************************************************************/

#include "header.h"
#include "dh.h"


/* ------- DLDBFD/NSBUNDLE HEADERS ------- */

#if HAVE_LIBBFD
# define DLDBFD 1
# include "dldbfd.h"
#elif HAVE_NSLINKMODULE
# define NSBUNDLE 1
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# include <mach-o/dyld.h>
#endif



/* ------- DLOPEN HEADERS ------- */

#if HAVE_DLFCN_H
# if HAVE_DLOPEN
#  include <dlfcn.h>
#  define DLOPEN 1
#  define dlopen_handle_t void*
# endif
#endif

#if HAVE_DL_H 
# if HAVE_LIBDLD
#  include <dl.h>
#  define DLOPEN 1
#  define DLOPENSHL 1
#  define dlopen_handle_t shl_t
# endif
#endif

#if DLOPEN
# define RTLD_SPECIAL -1
# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif
# ifndef RTLD_LAZY
#  define RTLD_LAZY 1
# endif
# ifndef RTLD_GLOBAL
#  define RTLD_GLOBAL 0
# endif
#endif



/* ------- NSBUNDLE HELPERS ------- */

#if NSBUNDLE 

/* Begin Mac OS X specific code */

typedef struct nsbundle_s {
   char *name;
   struct nsbundle_s *prev;
   struct nsbundle_s *next;
   NSObjectFileImage nsimage;
   NSModule nsmodule;
   int executable;
   int loadrank;
   int recurse;
} nsbundle_t;

static const char *nsbundle_error;
static nsbundle_t  nsbundle_head;

static int
nsbundle_init(void)
{
  nsbundle_head.prev = &nsbundle_head;
  nsbundle_head.next = &nsbundle_head;
  return 0;
}

static struct nsbundle_sym_s {
  struct nsbundle_sym_s *left;
  struct nsbundle_sym_s *right;
  char *name;
  nsbundle_t *def;
} *nsbundle_symtable;

static nsbundle_t *
nsbundle_hget(const char *sname)
{
  struct nsbundle_sym_s *p = nsbundle_symtable;
  while (p)
    {
      int s = strcmp(sname,p->name);
      if (s < 0) 
        p = p->left;
      else if (s > 0)
        p = p->right;
      else 
        return p->def;
    }
  return 0;
}

static void
nsbundle_hset(const char *sname, nsbundle_t *mark)
{
  struct nsbundle_sym_s **pp = &nsbundle_symtable;
  struct nsbundle_sym_s *p;
  while ((p = *pp))
    {
      int s = strcmp(sname,p->name);
      if (s < 0) 
        pp = &p->left;
      else if (s > 0)
        pp = &p->right;
      else 
        break;
    }
  if (! p)
    {
      p = malloc(sizeof(struct nsbundle_sym_s));
      p->name = strdup(sname);
      p->left = p->right = 0;
      *pp = p;
    }
  p->def = mark;
}

static int
nsbundle_symmark(nsbundle_t *bundle, nsbundle_t *mark)
{
  NSObjectFileImage nsimg = bundle->nsimage;
  int ns = NSSymbolDefinitionCountInObjectFileImage(nsimg);
  while (--ns >= 0)
    {
      const char *sname = NSSymbolDefinitionNameInObjectFileImage(nsimg, ns);
      nsbundle_t *old = nsbundle_hget(sname);
      if (old && mark && old!=mark)
	{
          static char buffer[512];
          sprintf(buffer,"duplicate definition of symbol '%s'", sname);
          nsbundle_error = buffer;
          return -1;
	}
      if (old==0 || old==bundle)
	nsbundle_hset(sname, mark);
    }
  return 0;
}

static int
nsbundle_exec(nsbundle_t *bundle)
{
  NSObjectFileImage nsimg = bundle->nsimage;
  int savedexecutable = bundle->executable;
  if (bundle->recurse)
    {
      nsbundle_error = 
        "MacOS X loader no longer handles circular dependencies in object files.\n"
        "*** Use 'ld -r' to collect these object files into a single image\n***";
      bundle->executable = -1;
    }
  else if (bundle->nsimage && bundle->executable>=0)
    {
      bundle->recurse = 1;
      int ns = NSSymbolReferenceCountInObjectFileImage(nsimg);
      while (--ns>=0)
	{
	  const char *sname = NSSymbolReferenceNameInObjectFileImage(nsimg, ns, NULL);
	  nsbundle_t *def = nsbundle_hget(sname);
	  if (def == &nsbundle_head) 
            {
              bundle->executable = -1;
            }
          else if (def && def != bundle)
            {
              if (nsbundle_exec(def))
                savedexecutable = -2;
              if (def->executable < 0)
                bundle->executable = def->executable;
              else if (def->executable >= bundle->executable)
                bundle->executable = 1 + def->executable;
            }
          else if (!def && !NSIsSymbolNameDefined(sname))
            bundle->executable = -1;
	  if (bundle->executable < 0)
	    break;
	}
      bundle->recurse = 0;
    }
  return bundle->executable != savedexecutable;
}

static int
nsbundle_exec_all_but(nsbundle_t *but)
{
  int again = 1;
  nsbundle_t *bundle;
  nsbundle_error = 0;
  for (bundle = nsbundle_head.next; 
       bundle != &nsbundle_head; 
       bundle=bundle->next)
    {
      bundle->recurse = 0;
      bundle->executable = -1;
      if (bundle!=but && bundle->nsimage)
        bundle->executable = 0;
    }
  while (again)
    {
      again = 0;
      for (bundle = nsbundle_head.next; 
           bundle != &nsbundle_head; 
           bundle=bundle->next)
        if (nsbundle_exec(bundle))
          again = 1;
    }
  if (nsbundle_error)
    return -1;
  return 0;
}

static int
nsbundle_update(void)
{
  nsbundle_t *bundle, *target;
  int again;
  /* attempt to unload */
  again = 1;
  while (again)
    {
      again = 0;
      target = 0;
      for (bundle = nsbundle_head.next; 
           bundle != &nsbundle_head; 
           bundle=bundle->next)
        {
          if (bundle->nsmodule && bundle->executable<0)
            if (!target || bundle->loadrank>target->loadrank)
              target = bundle;
        }
      if (target)
        {
          again = 1;
          NSUnLinkModule(target->nsmodule, NSUNLINKMODULE_OPTION_NONE);
          target->nsmodule = 0;
        }
    }
  /* attempt to load */
  again = 1;
  while (again)
    {
      again = 0;
      target = 0;
      for (bundle = nsbundle_head.next; 
           bundle != &nsbundle_head; 
           bundle=bundle->next)
        {
          if (bundle->executable>=0 && !bundle->nsmodule)
            if (!target || bundle->executable < target->executable)
              target = bundle;
        }
      if (target)
        {
          again = 1;
          target->nsmodule = 
            NSLinkModule(target->nsimage, target->name, 
                         NSLINKMODULE_OPTION_BINDNOW);
          target->loadrank = target->executable;
        }
    }
  return 0;
}

static int
nsbundle_unload(nsbundle_t *bundle)
{
  if (nsbundle_exec_all_but(bundle) < 0 ||
      nsbundle_update() < 0)
    return -1;
  if (bundle->prev)
    bundle->prev->next = bundle->next;
  if (bundle->next)
    bundle->next->prev = bundle->prev;
  if (bundle->nsimage)
    nsbundle_symmark(bundle, NULL);
  if (bundle->nsimage)
    NSDestroyObjectFileImage(bundle->nsimage);
  if (bundle->name)
    remove(bundle->name);
  if (bundle->name)
    free(bundle->name);
  memset(bundle, 0, sizeof(nsbundle_t));
  return 0;
}

static int 
nsbundle_load(const char *fname, nsbundle_t *bundle)
{
  int fnamelen = strlen(fname);
  char *cmd = 0;
  memset(bundle, 0, sizeof(nsbundle_t));
  bundle->prev = &nsbundle_head;
  bundle->next = nsbundle_head.next;
  bundle->prev->next = bundle;
  bundle->next->prev = bundle;
  nsbundle_error = "out of memory";
  if ((cmd = malloc(fnamelen + 256)) && (bundle->name = malloc(256)))
    {
#if DEBUGNAMES
      strcpy(bundle->name, fname);
      strcat(bundle->name, ".bundle");
#else
      strcpy(bundle->name, tmpname("/tmp","bundle"));
#endif
      sprintf(cmd, 
	      "ld -bundle -flat_namespace -undefined suppress /usr/lib/bundle1.o \"%s\" -o \"%s\"", 
	      fname, bundle->name);
      nsbundle_error = "Cannot create bundle from object file";
      if (system(cmd) == 0)
	{
	  NSObjectFileImageReturnCode ret;
	  nsbundle_error = "Cannot load bundle";
	  ret = NSCreateObjectFileImageFromFile(bundle->name, &bundle->nsimage);
	  if ((ret == NSObjectFileImageSuccess) &&
	      (nsbundle_symmark(bundle, bundle) >= 0) &&
              (nsbundle_exec_all_but(NULL) >= 0) &&
	      (nsbundle_update() >= 0) )
	    nsbundle_error = 0;
	}
    }
  if (cmd) 
    free(cmd);
  if (! nsbundle_error)
    return 0;
  if (bundle->nsimage)
    nsbundle_symmark(bundle, NULL);
  if (bundle->nsimage)
    NSDestroyObjectFileImage(bundle->nsimage);
  if (bundle->name)
    free(bundle->name);
  if (bundle->prev)
    bundle->prev->next = bundle->next;
  if (bundle->next)
    bundle->next->prev = bundle->prev;
  memset(bundle, 0, sizeof(nsbundle_t));
  return -1;
}

static void *
nsbundle_lookup(const char *sname, int exist)
{
  void *addr = 0;
  char *usname = malloc(strlen(sname)+2);
  if (usname)
    {
      strcpy(usname, "_");
      strcat(usname, sname);
      if (NSIsSymbolNameDefined(usname))
	{
	  NSSymbol symbol = NSLookupAndBindSymbol(usname);
	  addr = NSAddressOfSymbol(symbol);
	}
      else if (exist)
	{
	  nsbundle_t *def = nsbundle_hget(usname);
	  if (def && def!=&nsbundle_head)
	    addr = (void*)(~0);
	}
      free(usname);
    }
  return addr;
}

/* End of Mac OS X specific code */

#endif




/* ------- DLOPEN HELPERS ------- */

#if DLOPENSHL
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



/* ------ THE MODULE DATA STRUCTURE -------- */

#define MODULE_USED       0x01
#define MODULE_O          0x02
#define MODULE_SO         0x04
#define MODULE_EXEC       0x08
#define MODULE_INIT       0x20
#define MODULE_CLASS      0x40
#define MODULE_STICKY     0x80


struct module {
   int flags;
   struct module *prev;
   struct module *next;
   const char *filename;
   const char *initname;
   void *initaddr;
#if NSBUNDLE
   nsbundle_t bundle;
#endif
#if DLOPEN
   dlopen_handle_t *handle;
#endif
   at *backptr;
   at *defs;
   at *hook;
};

typedef struct module module_t;

static module_t *root;
static module_t *current;

static at *atroot;

static int check_executability = false;
static void check_exec();

static void clear_module(module_t *m, size_t _)
{
   m->prev = 0;
   m->next = 0;
   m->filename = 0;
   m->initname = 0;
#if NSBUNDLE
   m->bundle.name = 0;
   m->bundle.nsimage = 0;
   m->bundle.nsmodule = 0;
#endif
#if DLOPEN
   m->handle = 0;
#endif
   m->backptr = 0;
   m->defs = 0;
   m->hook = 0;
}

static void mark_module(module_t *m)
{
   MM_MARK(m->prev);
   MM_MARK(m->next);
   MM_MARK(m->filename);
   MM_MARK(m->initname);
   MM_MARK(m->backptr);
   MM_MARK(m->defs);
   MM_MARK(m->hook);
}

static bool finalize_module(module_t *m)
{
   module_class->dispose(m);
   return true;
}

static mt_t mt_module = mt_undefined;

/* ---------- THE MODULE OBJECT ----------- */

static at *new_module(const char *filename, at *hook)
{
   module_t *m = mm_alloc(mt_module);
   m->flags = 0;
   m->initaddr = 0;
   m->filename = filename; // filename must be managed
   m->hook = hook;
   at *ans = new_at(module_class, m);
   m->backptr = ans;
   return ans;
}

static char *module_maybe_unload(module_t *m);

static module_t *module_dispose(module_t *m)
{
   /* Don't do anything if already disposed */
   if (!m->backptr)
      return NULL;
   else
      zombify(m->backptr);

   /* Try to unload */
   if (m->prev && m->next && m!=root)
      module_maybe_unload(m);

   /* Unlock dependencies */
   m->defs = NIL;
   m->backptr = NIL;
   m->hook = NIL;
   if (m == root)
      return NULL;
   
   /* unlink */
   if (m->prev && m->next)
      m->prev->next = m->next;
   if (m->prev && m->next)
      m->next->prev = m->prev;    

   m->filename = 0;
   m->initname = 0;
   m->initaddr = 0;
#if DLOPEN
   m->handle = 0;
#endif
   return NULL;
}


static void module_serialize(at **pp, int code)
{
   module_t *m = NULL;
   char *fname = NULL;

   if (code != SRZ_READ) {
      m = Mptr(*pp);
      fname = "";
      if (m != root) {
         fname = (char *)relative_fname(lushdir, m->filename);
         if (!fname)
            fname = (char *)m->filename;
      }
   }
   
   serialize_string(&fname, code, -1);
   
   if (code == SRZ_READ) {
      at *junk = 0;
      m = root;
      
      if (fname[0] == 0) {
      already:
         *pp = m->backptr;
         serialize_atstar(&junk, code); 
         return;
      }
      const char *aname = concat_fname(lushdir, fname);
      while ((m = m->next) != root)
         if (!strcmp(aname, m->filename))
            goto already;
      (*pp) = module_load(aname, NIL);
      m = Mptr(*pp);
      check_exec();
   }
   serialize_atstar(&(m->hook), code);  
}

at *module_list(void)
{
   at *ans = NIL;
   at **where = &ans;
   module_t *m = root;
   do {
      *where = new_cons(m->backptr, NIL);
      where = &Cdr(*where);
      m = m->next;
   } while (m != root);
   return ans;
}

DX(xmodule_list)
{
   ARG_NUMBER(0);
   return module_list();
}

DX(xmodule_filename)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module", p);
   
   module_t *m = Mptr(p);
   if (m->filename)
      return NEW_STRING(m->filename);
   return NIL;
}

DX(xmodule_init_function)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module", p);

   module_t *m = Mptr(p);
   if (m->initname)
      return NEW_STRING(m->initname);
   if (m == root)
      return make_string("init_lush");
   return NIL;
}

DX(xmodule_executable_p)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module", p);

   module_t *m = Mptr(p);
   if (m->flags & MODULE_EXEC)
      return t();
   return NIL;
}

DX(xmodule_unloadable_p)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module", p);

   module_t *m = Mptr(p);
   if (m->flags & MODULE_STICKY)
      return NIL;
   return t();
}

DX(xmodule_defs)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      error(NIL,"Not a module", p);

   module_t *m = Mptr(p);
   p = m->defs;
   at *ans = NIL;
   while (CONSP(p) && CONSP(Car(p))) {
      ans = new_cons(new_cons(Caar(p), Cdar(p)), ans);
      p = Cdr(p);
   }
   return ans;
}



/* --------- DYNLINK UTILITY FUNCTIONS --------- */



static int dynlink_initialized = 0;

static void dynlink_error(at *p)
{
   const char *err;
   char buffer[256];
   strcpy(buffer,"Dynamic linking error");
#if DLDBFD
   if ((err = dld_errno)) {
      strcpy(buffer,"dld/bfd error\n*** ");
      strcat(buffer, err);
      error(NIL, buffer, p);
   }
#endif  
#if NSBUNDLE
   if ((err = nsbundle_error)) {
      strcpy(buffer,"nsbundle error\n*** ");
      strcat(buffer, nsbundle_error);
      error(NIL, buffer, p);
   }
#endif
#if DLOPEN
   if ((err = dlerror())) {
      strcpy(buffer,"dlopen error\n*** ");
      strcat(buffer, err);
      error(NIL, buffer, p);
   }
#endif
   error(NIL, buffer, p);
}

static void dynlink_init(void)
{
   if (! dynlink_initialized) {
#if DLDBFD
      if (!root->filename)
         error(NIL,"Internal error (program_name unset)",NIL);        
      if (dld_init(root->filename))
         dynlink_error(NIL);
#endif
#if NSBUNDLE
      nsbundle_init();
#endif
      dynlink_initialized = 1;
   }
}

static void dynlink_hook(module_t *m, char *hookname)
{
   if (m->hook && error_doc.ready_to_an_error) {
      at *arg = new_cons(named(hookname), new_cons(m->backptr, NIL));
      apply(m->hook, arg);
   }
}

void *dynlink_symbol(module_t *m, const char *sname, int func, int exist)
{
#if DLDBFD
   if (func)
      return dld_get_func(sname);
   else
      return dld_get_symbol(sname);
#elif NSBUNDLE
   return nsbundle_lookup(sname, exist);
#elif DLOPEN
   return dlsym(m->handle, sname);
#else
   return NULL;
#endif
}


/* --------- CLEANUP DANGLING PRIMITIVES AND OBJECTS --------- */


static at *module_class_defs(at *l, module_t *mc)
{
   at *p = mc->defs;
   at *pcls = l;
   while (CONSP(p)) {
      if (CONSP(Car(p))) {
         at *q = Caar(p);
         if (CLASSP(q)) {
            pcls = new_cons(q, pcls);
         }
      }
      p = Cdr(p);
   }
   return pcls;
}

static void cleanup_module(module_t *m)
{
   /* 1 --- No cleanup when not ready for errors */
   if (! error_doc.ready_to_an_error)
      return;
  
   /* 2 --- Collect impacted classes */
   MM_ENTER;
   at *classes = 0;
#if DLDBFD
   {
      if (dld_simulate_unlink_by_file(m->filename) < 0)
         dynlink_error(NIL);
      for (module_t *mc = root->next; mc != root; mc = mc->next)
         if (mc->initname && mc->defs && (mc->flags & MODULE_CLASS))
            if (! dld_function_executable_p(mc->initname))
               classes = module_class_defs(classes, mc);
      dld_simulate_unlink_by_file(0);
   }
#endif
#if NSBUNDLE
   {
      nsbundle_exec_all_but(&m->bundle);
      for (module_t *mc = root->next; mc != root; mc = mc->next)
         if (mc->initname && mc->defs)
            if (mc == m || mc->bundle.executable < 0)
               classes = module_class_defs(classes, mc);
      nsbundle_exec_all_but(NULL);
   }
#endif
  
   /* 3 --- Mark impacted classes as dead --- */
   for (at *p = classes; CONSP(p); p = Cdr(p)) {
      at *q = Car(p);
      if (CLASSP(q)) {
         class_t *cl = Mptr(q);
         //fprintf(stderr,"*** Warning: unlinking compiled class %s and subclasses\n", pname(q));
         cl->live = false;
         zombify_subclasses(cl);
      }
   }

   /* 4 --- Zap primitives defined by this module. */
   if (m->defs)
      for (at *p = m->defs; CONSP(p); p = Cdr(p))
         if (CONSP(Car(p)) && Caar(p)) {
            at *q = Caar(p);
            if (CLASSP(q))
               zombify(q);

            /* temporarily enable deleting objects of this class */
            bool dontdelete = Class(q)->dontdelete;
            class_t *cl = (class_t *)Class(q);
            cl->dontdelete = false;
            lush_delete(q);
            cl->dontdelete = dontdelete;
         }
   m->defs = NIL;
   MM_EXIT;
}


/* --------- EXECUTABILITY CHECK --------- */


static void update_exec_flag(module_t *m)
{
   MM_ENTER;
   int oldstate = (m->flags & MODULE_EXEC);
   int newstate = MODULE_EXEC;
#if DLDBFD
   if (m->flags & MODULE_O) {
      newstate = 0;
      if (m->initname)
         if (dld_function_executable_p(m->initname))
            newstate = MODULE_EXEC;
   }
#endif
#if NSBUNDLE 
   if (m->flags & MODULE_O) {
      newstate = 0;
      if (m->initname && m->bundle.executable>=0)
         newstate = MODULE_EXEC;
      if (m->defs && !newstate)
         m->flags &= ~MODULE_INIT;
   }
#endif
   m->flags &= ~MODULE_EXEC;
   if (newstate)
      m->flags |= MODULE_EXEC;
   if (m->defs && newstate != oldstate) {
      extern void clean_dhrecord(dhrecord *);

      /* Refresh function/class pointers */
      at *p = m->defs;
      while (p) {
         if (CONSP(Car(p))) {
            at *q = Caar(p);
            if (FUNCTIONP(q)) {
               dhdoc_t *kdoc = 0;
               struct cfunction *cfunc = Mptr(q);
               if (!newstate) {
                  kdoc = cfunc->info = 0;
               } else if (Class(q)==dh_class && cfunc->kname) {
                  kdoc = cfunc->info = dynlink_symbol(m, cfunc->kname, 0, 0);
                  cfunc->call = (void *(*)())kdoc->lispdata.call;
               } else if (cfunc->kname) {
		  cfunc->call = (void *(*)())dynlink_symbol(m, cfunc->kname, 1, 0);
		  kdoc = cfunc->info = (void *)cfunc->call;
               }
               if (kdoc)
                  clean_dhrecord(kdoc->argdata);
            } else if (CLASSP(q)) {
               class_t *cl = Mptr(q);
               if (! newstate)
                  cl->classdoc = 0;
               else if (cl->kname && !cl->classdoc) {
                  dhclassdoc_t *kdata = dynlink_symbol(m, cl->kname, 0, 0);
                  if (kdata && kdata->lispdata.vtable)
                     ((dhclassdoc_t**)(kdata->lispdata.vtable))[0] = kdata;
                  if (kdata)
                     kdata->lispdata.atclass = q;
                  cl->classdoc = kdata;
                  if (kdata)
                     clean_dhrecord(kdata->argdata);
               }
            }
         }
         p = Cdr(p);
      }
   }
   /* Call hook */
   dynlink_hook(m, "exec");
   MM_EXIT;
}

static void update_init_flag(module_t *m)
{
   int status;
   
   if (m->flags & MODULE_INIT) 
      return;
   if (! (m->flags & MODULE_EXEC))
      return;
   if (! (m->initaddr && m->initname))
      return;
   
   /* Simulate unlink if we already have definitions */
   if (m->defs) {
      dynlink_hook(m, "unlink");
      cleanup_module(m);
   }
   /* Refresh init function pointer */
   if (m->initname)
      m->initaddr = dynlink_symbol(m, m->initname, 1, 0);
   /* Call init function */
   if (! strcmp(m->initname, "init_user_dll")) {
      /* TL3 style init function */
      int (*call)(int,int) = (int(*)(int,int)) m->initaddr;
      current = m;
      status = (*call)(TLOPEN_MAJOR, TLOPEN_MINOR);
      current = root;
   } else {
      /* SN3 style init function */
      int *majptr, *minptr, maj, min;
      strcpy(string_buffer, "majver_");
      strcat(string_buffer, m->initname + 5);
      majptr = (int*)dynlink_symbol(m, string_buffer, 0, 0);
      strcpy(string_buffer, "minver_");
      strcat(string_buffer, m->initname + 5);
      minptr = (int*)dynlink_symbol(m, string_buffer, 0, 0);
      maj = ((majptr) ? *majptr : TLOPEN_MAJOR);
      min = ((minptr) ? *minptr : TLOPEN_MINOR);
      status = -2;
      if (maj == TLOPEN_MAJOR && min >= TLOPEN_MINOR) {
         void (*call)(void) = (void(*)()) m->initaddr;
         current = m;
         (*call)();
         current = root;
         status = 0;
      }
   }
   /* Mark new status */
   if (status >= 0) {
      m->flags |= MODULE_INIT;
      dynlink_hook(m, "init");

   } else {
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

static void check_exec(void)
{
   if (check_executability) {
      check_executability = false;
      for (module_t *m = root->next; m != root; m = m->next)
         update_exec_flag(m);
      for (module_t *m = root->next; m != root; m = m->next)
         ifn (m->flags & MODULE_INIT)
            update_init_flag(m);
   }
}

void check_primitive(at *prim, void *info)
{
   if (CONSP(prim) && MODULEP(Car(prim))) {
      module_t *m = Mptr(Car(prim));
      if (check_executability)
         check_exec();
      if (!info || !(m->flags & MODULE_EXEC))
         error(NIL,"function belongs to a partially linked module", Cdr(prim));
   }
}



/* --------- MODULE_UNLOAD --------- */


DX(xmodule_never_unload)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module",p);
   module_t *m = Mptr(p);
   m->flags |= MODULE_STICKY;
   return p;
}


DX(xmodule_depends)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (MODULEP(p))
      RAISEFX("not a module",p);
   module_t *m = Mptr(p);
   p = NIL;
   if (m == root)
      return NIL;
#if DLDBFD
   {
      if (m->flags & MODULE_SO)
         return NIL;
      /* Simulate unlink */
      if (dld_simulate_unlink_by_file(m->filename) < 0)
         dynlink_error(NIL);
      for (module_t *mc = root->next; mc != root; mc = mc->next)
         if (mc->initname && mc->defs)
            if (! dld_function_executable_p(mc->initname))
               p = new_cons(mc->backptr, p);
      /* Reset everything as it should be */
      dld_simulate_unlink_by_file(0);
   }
#endif
#if NSBUNDLE
   {
      /* Simulate unlink */
      nsbundle_exec_all_but(&m->bundle);
      for (module_t *mc = root->next; mc != root; mc = mc->next)
         if (mc->initname && mc->defs)
            if (mc->bundle.executable < 0)
               p = new_cons(mc->backptr, p);
      /* Reset everything as it should be */
      nsbundle_exec_all_but(NULL);
   }
#endif
   return p;
}


static char *module_maybe_unload(module_t *m)
{
   if (m == root)
      return "Cannot unload root module";
   if (! (m->flags & MODULE_O))
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
         dynlink_error(NEW_STRING(m->filename));
#endif
#if DLDBFD
   if (m->flags & MODULE_O)
      if (dld_unlink_by_file(m->filename, 1))
         dynlink_error(NEW_STRING(m->filename));
#endif
#if NSBUNDLE
   if (m->flags & MODULE_O)
      if (nsbundle_unload(&m->bundle) < 0)
         dynlink_error(NEW_STRING(m->filename));
   if (m->flags & MODULE_SO) 
      if (nsbundle_update() < 0)
         dynlink_error(NEW_STRING(m->filename));
#endif
   check_executability = true;
   /* Remove the module from the chain */
   m->prev->next = m->next;
   m->next->prev = m->prev;
   m->next = m->prev = 0;
   return NULL;
}

void module_unload(at *atmodule)
{
   char *err;
   ifn (MODULEP(atmodule))
      RAISEF("not a module", atmodule);
   if ((err = module_maybe_unload(Mptr(atmodule))))
      RAISEF(err, atmodule);
   check_exec();
}

DX(xmodule_unload)
{
   ARG_NUMBER(1);
   module_unload(APOINTER(1));
   return NIL;
}



/* --------- MODULE_LOAD --------- */

at *module_load(const char *file, at *hook)
{
#if DLOPEN
   dlopen_handle_t handle = 0;
#endif
   /* Check that file exists */
   const char *filename = concat_fname(NULL, file);
   if (! filep(filename))
      RAISEF("file not found", NEW_STRING(filename));

   /* Check if the file extension indicates a DLL */
   const char *l = filename + strlen(filename);
   while (l>filename && isdigit((unsigned int)l[-1])) {
      while (l>filename && isdigit((unsigned int)l[-1]))
         l -= 1;
      if (l>filename && l[-1]=='.')
         l -= 1;
   }
   int dlopenp = 0;
   if (l[0]==0 || l[0]=='.') {
      int len = strlen(EXT_DLL);
      if (len>0 && l>filename+len && !strncmp(EXT_DLL, l-len, len))
         dlopenp = 1;
      else if (l>filename-3 && !strncmp(".so", l-3, 3))
         dlopenp = 1;
   }

   /* Initialize */
   dynlink_init();

   /* Are we reloading an existing module? */
   module_t *m;
   for (m = root->next; m != root; m = m->next)
      if (!strcmp(filename, m->filename))
         break;
   if (m != root)
      if (module_maybe_unload(m))
         return m->backptr;
   check_exec();

   /* Allocate */
   at *ans = new_module(filename, hook);
   m = Mptr(ans);
   m->flags = MODULE_USED;
#if DLOPEN
   m->handle = handle;
#endif

   /* Load the file */
   if (dlopenp) {
#if DLOPEN
# if DLDBFD
      if (! (handle = dld_dlopen(m->filename, RTLD_NOW|RTLD_GLOBAL)))
         dynlink_error(NEW_STRING(m->filename));
# else
      if (! (handle = dlopen(m->filename, RTLD_NOW|RTLD_GLOBAL)))
         dynlink_error(NEW_STRING(m->filename));
# endif
# if NSBUNDLE
      if (nsbundle_exec_all_but(NULL) < 0 || nsbundle_update() < 0)
         dynlink_error(NEW_STRING(m->filename));
# endif
#else
      RAISEF("dynlinking this file is not supported (dlopen)",
             NEW_STRING(m->filename));
#endif
      m->flags |= MODULE_SO | MODULE_STICKY;
      
   } else {
#if DLDBFD
      if (dld_link(m->filename))
         dynlink_error(NEW_STRING(m->filename));
#elif NSBUNDLE
      if (nsbundle_load(m->filename, &m->bundle))
         dynlink_error(NEW_STRING(m->filename));
#else
      RAISEF("dynlinking this file is not supported (bfd)", 
             NEW_STRING(m->filename));
#endif      
      m->flags |= MODULE_O;
   }
   /* Search init function */
#if DLOPEN
   if (dlopenp) {
      strcpy(string_buffer, "init_user_dll");
      m->initaddr = dlsym(handle, string_buffer);
   }
#endif
   if (! m->initaddr) {
      strcpy(string_buffer, "init_");
      strcat(string_buffer, lush_basename(m->filename, 0));
      char *s = strchr(string_buffer, '.'); 
      if (s) s[0] = 0;
      m->initaddr = dynlink_symbol(m, string_buffer, 1, 1);
   }
   if (m->initaddr) {
      m->initname = mm_strdup(string_buffer);
      if (! m->initname)
         RAISEF("out of memory", NIL);
   }
   /* Terminate */
   m->prev = root->prev;
   m->next = root;
   m->prev->next = m;
   m->next->prev = m;
   check_executability = true;
   check_exec();
   return ans;
}

DX(xmodule_load)
{
   if (arg_number != 1)
      ARG_NUMBER(2);
   at *hook = NIL;
   if (arg_number >= 2)
      hook = APOINTER(2);
   ifn (FUNCTIONP(hook))
      RAISEF("not a function", hook);
   return module_load(ASTRING(1), hook);
}



/* --------- SN3 FUNCTIONS --------- */



DX(xmod_create_reference)
{
#if DLDBFD
   dynlink_init();
   for (int i=1; i<=arg_number; i++)
      dld_create_reference(ASTRING(1));
   return NEW_NUMBER(dld_undefined_sym_count);
#else
   return NIL;
#endif
}

DX(xmod_compatibility_flag)
{
   ARG_NUMBER(1);
#if DLDBFD
   dld_compatibility_flag = ( APOINTER(1) ? 1 : 0 );
   return (dld_compatibility_flag ? t() : NIL);
#else
   return NIL;
#endif
}

DX(xmod_undefined)
{
   at *p = NIL;
   if (dynlink_initialized) {
#if DLDBFD
      at **where = &p;
      char ** dld_undefined_sym_list = (char**)dld_list_undefined_sym();
      p = NIL;
      where = &p;
      for (int i=0; i<dld_undefined_sym_count; i++) {
         *where = new_cons(make_string(dld_undefined_sym_list[i]), NIL);
         where = &Cdr(*where);
      }
      free(dld_undefined_sym_list);
#endif
#if NSBUNDLE
      at **where = &p;
      nsbundle_t *bundle;
      for (bundle = nsbundle_head.next; 
	   bundle != &nsbundle_head; 
	   bundle=bundle->next) {
         NSObjectFileImage nsimg = bundle->nsimage;
         int ns = NSSymbolReferenceCountInObjectFileImage(nsimg);
         while (--ns >= 0) {
            const char *sname = NSSymbolReferenceNameInObjectFileImage(nsimg, ns, NULL);
            nsbundle_t *def = nsbundle_hget(sname);
            if (def==&nsbundle_head || (!def && !NSIsSymbolNameDefined(sname))) {
               if (sname[0]=='_') sname += 1;
               *where = new_cons( NEW_STRING((char*)sname), NIL);
               where = &Cdr(*where);
            }
         }
      }
#endif
   }
   return p;
}






/* --------- LOCATE PRIMITIVES --------- */



at *find_primitive(at *module, at *name)
{
  at *p = root->defs;
  at *clname = NIL;

  /* Missing MODULE means root. */
  if (module) {
     ifn (MODULEP(module))
        RAISEF("not a module descriptor", module);
     p = ((module_t *)Mptr(module))->defs;
  }
  /* Name can be NAME or (CLASSNAME . NAME) */
  if (CONSP(name)) {
     clname = Car(name);
     name = Cdr(name);
  }
  /* Iterate over module definitions */
  at *q = NIL;
  while (CONSP(p)) {
     q = Car(p);
     if (CONSP(q)) {
        if (clname) {
           /* Looking for a method:
              - names must match, 
              - clname must match class. */
           if (CONSP(Cdr(q)) && Cddr(q) == name && 
               CLASSP(Cadr(q)) && ((class_t *)Mptr(Cadr(q)))->classname == clname)
              break;
        } else {
           /* Looking for a function: 
              - names must match. */
           if (name == Cdr(q))
              break;
        }
     }
     p = Cdr(p);
     q = 0;
  }
  if (q) {
     return Car(q);
  }
  return NIL;
}



/* --------- XXX_DEFINE FUNCTIONS --------- */

static at *module_priminame(at *name)
{
   /* NAME for root module.
    * (MODULE . NAME) for other modules. 
    */ 
   if (current != root)
      return new_cons(current->backptr, name);
   return name;
}

static void module_def(at *name, at *val)
{
   /* Check name and add definition */
   ifn (SYMBOLP(name))
      RAISEF("internal error (symbol expected)",name);
   current->defs = new_cons(new_cons(val, name), current->defs);
   /* Root definitions are also written into symbols */
   if (current == root) {
      if (symbol_locked_p(Symbol(name)))
         RAISEF("internal error (multiple definition)", name);
      var_set(name, val);
      var_lock(name);
   }
}

static at *module_method_priminame(class_t *cl, at *name)
{
   /* (CLASSNAME . NAME) for root module.
    * (MODULE CLASSNAME . NAME) for other modules. 
    */ 
   at *n = new_cons(cl->classname, name);
   if (current == root)
      return n;
   return new_cons(current->backptr, n);
}

static void module_method_def(class_t *cl, at *name, at *val)
{
   /* Check name and add definition */
   ifn (SYMBOLP(name))
      RAISEF("internal error (symbol expected)",name);
   if (! cl->backptr)
      RAISEF("defining method before class_define",name);
   current->defs = new_cons(new_cons(val, new_cons(cl->backptr, name)), current->defs);
   /* Root definitions are also written into classes */
   if (current == root)
      putmethod(cl, name, val);
}


void class_define(const char *name, class_t *cl)
{
   at *symb = NEW_SYMBOL(name);
   cl->classname = symb;
   cl->priminame = module_priminame(symb);
   module_def(symb, cl->backptr);
   current->flags |= MODULE_CLASS;
}

void dx_define(const char *name, at *(*addr) (int, at **))
{
   at *symb = NEW_SYMBOL(name);
   at *priminame = module_priminame(symb);
   at *func = new_dx(priminame, addr);
   module_def(symb, func);
}

void dy_define(const char *name, at *(*addr) (at *))
{
   at *symb = NEW_SYMBOL(name);
   at *priminame = module_priminame(symb);
   at *func = new_dy(priminame, addr);
   module_def(symb, func);
}

void dxmethod_define(class_t *cl, const char *name, at *(*addr) (int, at **))
{
   at *symb = NEW_SYMBOL(name);
   at *priminame = module_method_priminame(cl, symb);
   at *func = new_dx(priminame, addr);
   module_method_def(cl, symb, func);
}


void dymethod_define(class_t *cl, const char *name, at *(*addr) (at *))
{
   at *symb = NEW_SYMBOL(name);
   at *priminame = module_method_priminame(cl, symb);
   at *func = new_dy(priminame, addr);
   module_method_def(cl, symb, func);
}


void dhclass_define(const char *name, dhclassdoc_t *kclass)
{
   at *symb = NEW_SYMBOL(name);
   at *classat = new_dhclass(symb, kclass);
   class_t *cl = Mptr(classat);
   cl->priminame = module_priminame(symb); 
   module_def(symb, classat);
   current->flags |= MODULE_CLASS;
}


void dh_define(const char *name, dhdoc_t *kname)
{
   at *symb = NEW_SYMBOL(name);
   at *priminame = module_priminame(symb);
   at *func = new_dh(priminame, kname);
   module_def(symb, func);
}

void dhmethod_define(dhclassdoc_t *kclass, const char *name, dhdoc_t *kname)
{
   at *symb = NEW_SYMBOL(name);
   if (! kclass->lispdata.atclass)
      error(NIL,"internal: dhmethod_define called before dhclass_define", symb);
   
   dhrecord *drec;
   for (drec = kclass->argdata;
        drec->op != DHT_END_CLASS && drec->op != DHT_NIL;
        drec = drec->end)
      if (drec->op == DHT_METHOD)
         if (drec->arg == kname)
          break;
   if (drec->arg != kname)
      error(NIL,"internal: method is not listed in classdoc", symb);
   class_t *cl = Mptr(kclass->lispdata.atclass);
   at *priminame = module_method_priminame(cl, symb);
   at *func = new_dh(priminame, kname);
   module_method_def(cl, symb, func);
}


/* --------- INITIALISATION CODE --------- */

void pre_init_module(void)
{
   if (mt_module == mt_undefined)
      mt_module = MM_REGTYPE("module", sizeof(module_t),
                             clear_module, mark_module, finalize_module);
   /* create root module */
   if (!root) {
      root = mm_alloc(mt_module);
      root->flags = MODULE_USED | MODULE_STICKY | MODULE_INIT | MODULE_EXEC;
      root->next = root;
      root->prev = root;
      current = root;
      MM_ROOT(root);
      MM_ROOT(current);
   }
}


class_t *module_class;

void init_module(char *progname)
{
   pre_init_module();

   /* set up module_class */
   module_class = new_builtin_class(NIL);
   module_class->dispose = (dispose_func_t *)module_dispose;
   module_class->serialize = module_serialize;
   module_class->dontdelete = true;
   class_define("Module", module_class);

#if DLDBFD
   char *filename = dld_find_executable(progname);
   root->filename = mm_strdup(filename);
   free(filename);
#else
   root->filename = mm_strdup(progname);
#endif  
   atroot = new_at(module_class, root);
   root->backptr = atroot;

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


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */

