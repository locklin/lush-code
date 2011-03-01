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

#ifdef HAVE_CONFIG_H
# include "lushconf.h"
#endif

/* ---------------------------------------- */
/* INCLUDES */

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE    600
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#if HAVE_LIMITS_H
# include <limits.h>
#endif

#include <bfd.h>
#include "dldbfd.h"

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif

/* ---------------------------------------- */
/* GCC OPTIMISATIONS */

#ifdef __GNUC__
#if ( __GNUC__ * 1000 + __GNUC_MINOR__ >= 2005 )
#define noreturn __attribute__ ((noreturn));
#else
#define noreturn /**/
#endif
#else /* !GNUC */
#define inline /**/
#define noreturn /**/
#endif

/* ---------------------------------------- */
/* MISSING PROTOTYPES (watch) */


#ifndef SEC_LINK_DUPLICATES_SAME_CONTENTS  /* bfd<=2.7 */
# define bfd_alloc bfd_alloc_by_size
# define bfd_size_type size_t
#endif

#ifdef bfd_get_section_size_before_reloc   /* bfd<2.15 */
# define dldbfd_section_rawsize(p) ((p)->_raw_size)
# define dldbfd_section_size(p) ((p)->_cooked_size)
#else
# define dldbfd_section_rawsize(p) ((p)->rawsize ? (p)->rawsize : (p)->size)
# define dldbfd_section_size(p) ((p)->size)
#endif

#if HAVE_BFD_HASH_TABLE_INIT_WANTS_2_ARGS
# define my_bfd_hash_table_init(a,b,c) bfd_hash_table_init(a,b)
#else
# define my_bfd_hash_table_init(a,b,c) bfd_hash_table_init(a,b,c)
#endif


void *bfd_alloc(bfd *abfd, bfd_size_type wanted);
unsigned int bfd_log2 (bfd_vma x);

/* Conversion between bfd_vma and pointers.
 * These casts remove warnings with 64 bit enabled bfd. */
#define vmaptr(x) ((void*)(unsigned long)(x))
#define ptrvma(x) ((bfd_vma)(unsigned long)(x))


/* ---------------------------------------- */
/* DLOPEN */

#if HAVE_DLFCN_H
#if HAVE_DLOPEN
# include <dlfcn.h>
# define DLOPEN 1
typedef void* dlopen_handle_t;
#endif
#endif

#if HAVE_DL_H 
#if HAVE_LIBDLD
# include <dl.h>
# define DLOPEN 1
typedef shl_t dlopen_handle_t
#endif
#endif

#ifdef WIN32
# define DLOPEN 1
typedef void* dlopen_handle_t;
#endif

#if DLOPEN
# ifndef RTLD_LAZY
#  define RTLD_LAZY 1
# endif
# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif
#endif

#if DLOPEN 
#if HAVE_DL_H
/* DLOPEN emulation under HP/UX */
static dlopen_handle_t 
dlopen(char *soname, int mode)
{ 
  return shl_load(soname,BIND_IMMEDIATE|BIND_NONFATAL|
		  BIND_NOSTART|BIND_VERBOSE, 0L ); 
}
static void 
dlclose(dlopen_handle_t hndl)
{ 
  shl_unload(hndl); 
}
static void* 
dlsym(dlopen_handle_t hndl, char *sym)
{ 
  void *addr = 0;
  if (shl_findsym(&hndl,sym,TYPE_PROCEDURE,&addr) >= 0)
    return addr;
  return 0;
}
static char* 
dlerror(void)
{
  return "Function shl_load() has failed";
}
#endif
#endif




/* ---------------------------------------- */
/* Should be implement DLD3.x bugs? */

int dld_compatibility_flag = 0;



/* ---------------------------------------- */
/* ERROR MANAGEMENT */



/* TRY/CATCH/END_CATCH/THROW -- setjmp based exception handling */

typedef struct { jmp_buf jmp; } __ex_except;
static __ex_except *__ex_main = 0;
static const char * __ex_msg = 0;
static void THROW(const char *) noreturn;

static void 
THROW(const char *n)
{
  __ex_msg = n;
  if (__ex_main)
    longjmp(__ex_main->jmp, 1);
  fprintf(stderr,"DLD/BFD unhandled exception '%s'\n",n);
  for(;;) /*noreturn*/;
}

#define TRY do { __ex_except __ex_new; \
  __ex_except *__ex_old = __ex_main; \
  if (!setjmp(__ex_new.jmp)) { __ex_main = &__ex_new;

#define CATCH(n) __ex_main = __ex_old; } else { \
  const char *n = __ex_msg; __ex_main = __ex_old; 

#define END_CATCH } } while(0)

#define MKSTR_(d) #d
#define MKSTR(d) MKSTR_(d)
#define ASSERT(expr) if (!(expr)) \
  THROW("Assertion failed: " __FILE__ ":" MKSTR(__LINE__))
#define ASSERT_BFD(expr) if (!(expr)) \
  THROW(bfd_errmsg(bfd_get_error()))


/* error_buffer -- for intermediate strings */

static char error_buffer[256];


/* dld_errno -- Contains dld error */

const char *dld_errno = 0;


/* dld_perror -- Prints out an error message for current error */

void
dld_perror (const char *str)
{
    if (str) 
        fprintf (stderr, "%s: ", str);
    if (dld_errno)
        fprintf(stderr, "%s.\n", dld_errno);
    else
        fprintf(stderr, "Unknown error.\n");
}

/* xmalloc -- malloc and check */

static void *
xmalloc(size_t n)
{
    void *ans = (void*)malloc(n);
    if (!ans) 
        THROW(bfd_errmsg(bfd_error_no_memory));
    return ans;
}

/* xballoc -- alloc in the bfd obstack and check */

static void *
xballoc(bfd *abfd, size_t n)
{
    void *ans = (void*)bfd_alloc(abfd, n);
    if (!ans)
        THROW(bfd_errmsg(bfd_error_no_memory));
    return ans;
}



/* ---------------------------------------- */
/* BFD ACCESS FUNCTIONS */



/* is_bfd_symbol_defined -- check if bfd synmbol is defined */

static inline bfd_vma
is_bfd_symbol_defined(asymbol *sym)
{
    /* Iterate over indirect symbols */
    while ((sym->flags & BSF_INDIRECT) || bfd_is_ind_section(sym->section))
      sym = (asymbol*)vmaptr(sym->value);
    /* Check that symbol is indeed defined */
    return (!bfd_is_und_section(sym->section) &&
            !bfd_is_com_section(sym->section) &&
#ifdef BSF_DEBUGGING_RELOC
            !(sym->flags & BSF_DEBUGGING_RELOC) &&
#else
            !(sym->flags & BSF_DEBUGGING) &&
#endif
            !(sym->flags & BSF_WARNING) );
}


/* value_of_bfd_symbol -- return the value of a bfd symbol */

static inline bfd_vma
value_of_bfd_symbol(asymbol *sym)
{
    /* Iterate over indirect symbols */
    while ((sym->flags & BSF_INDIRECT) || bfd_is_ind_section(sym->section))
        sym = (asymbol*)vmaptr(sym->value);
    /* Check that symbol is indeed defined */
    ASSERT(!bfd_is_com_section(sym->section) &&
           !bfd_is_und_section(sym->section) &&
           !(sym->flags & BSF_WARNING) );
#ifdef BSF_DEBUGGING_RELOC
    ASSERT(!(sym->flags & BSF_DEBUGGING_RELOC));
#else
    ASSERT(!(sym->flags & BSF_DEBUGGING));
#endif
    /* Return value of symbol */
    return sym->section->vma + sym->value;
}


/* drop_leading_char -- remove target specific leading character */

static inline const char *
drop_leading_char(bfd *abfd, const char *id)
{
    char lead;
    if (abfd)
    {
        /* check for bfd dependent leading character */
        lead = bfd_get_symbol_leading_char(abfd);
        if (!lead)
            return id;
        else if (id[0]==lead)
            return id+1;
    }
    /* add improbable character to asm symbol names */
    error_buffer[0] = '\007';
    strncpy(error_buffer+1, id, sizeof(error_buffer)-2);
    error_buffer[sizeof(error_buffer)-1] = 0;
    return error_buffer;
}






/* ---------------------------------------- */
/* FILE LISTS */


/* module_list -- manages list of file entries */

typedef struct module_list {
    struct module_list *next;
    struct module_entry *entry;
} module_list;


/* free_list_of_module_lists -- free list of module_list objects */

module_list *free_list_of_module_lists = 0;


/* insert_entry_into_list -- insert an entry into a file list */

static int
insert_entry_into_list(module_list **here, struct module_entry *ent)
{
    module_list *p;
    for (p=*here; p; p=p->next)
        if (p->entry == ent)
            return FALSE;
    if (!free_list_of_module_lists) 
    {
        int nobj = 510;
        p = xmalloc(nobj * sizeof(module_list));
        while (nobj-- > 0) {
            p->next = free_list_of_module_lists;
            free_list_of_module_lists = p;
            p++;
        }
    }
    p = free_list_of_module_lists;
    free_list_of_module_lists = p->next;
    p->next = *here;
    p->entry = ent;
    *here = p;
    return TRUE;
}


/* remove_entry_from_list -- remove an entry from a file list */

static int
remove_entry_from_list(module_list **here, struct module_entry *ent)
{
    module_list *p;
    while (*here)
    {
        p = *here;
        if (p->entry == ent)
        {
            *here = p->next;
            p->next = free_list_of_module_lists;
            free_list_of_module_lists = p;
            return TRUE;
        }
        here = &p->next;
    }
    return FALSE;
}



/* ---------------------------------------- */
/* FILE ENTRIES */


/* module_entry -- structure associated to each module */

typedef struct module_entry {
    const char *filename;       /* filename for this module */
    struct module_entry *next;  /* next file entry */
    /* BFD stuff */
    bfd *abfd;                  /* bfd descriptor */
    int symbol_count;           /* number of symbols in this file */
    asymbol **symbols;          /* canonical symbols */
    /* DLD stuff */
    int undefined_symbol_count; /* number of undefined symbols in this file */
    module_list *useds;         /* files defining symbols ref'd by this file */
    module_list *users;         /* files referencing symbols def'd by this file */
    void *exec;                 /* area for executable program */
    struct module_entry *mods;  /* for archives */
    signed char executable_flag;/* >0 if module is executable */
    int symdone_flag;       /* symbols have been put into global table */
    int archive_flag;       /* module is an archive */
    int relocated_flag;     /* relocation has been performed */
} module_entry;


/* dld_modules -- list of file entries currently loaded */

static module_entry *dld_modules = 0;



/* create_module_entry -- create a module entry and chain it */

static module_entry *
create_module_entry(bfd *abfd, module_entry **here, int archivep)
{
    module_entry *new;
    long storage_needed;
    asection *p;
    int i;

    /* Heuristically probe bfd version */
#ifndef NO_BFD_VERSION_ANALYSIS
    {
      const struct bfd_target *tgt = abfd->xvec;
      if (tgt->alternative_target && 
          tgt->alternative_target->alternative_target != tgt)
        THROW("Snafu in bfd target. Possible mismatch between libbfd and bfd.h");
      for (p=abfd->sections; p; p=p->next)
        if (p->owner != abfd)
          THROW("Snafu in bfd section. Possible mismatch between libbfd and bfd.h");
    }
#endif

    /* Fix section flags (GCC2.7.2 on MIPS) */
    if (bfd_get_flavour(abfd) == bfd_target_elf_flavour &&
        bfd_get_arch(abfd) == bfd_arch_mips )
      {
        for (p=abfd->sections; p; p=p->next)
          {
            if (!(strcmp(p->name, ".rodata")) &&
                !(p->flags & (SEC_LOAD|SEC_ALLOC)) )
              {
                p->flags |= SEC_ALLOC|SEC_READONLY|SEC_LOAD|SEC_DATA;
              }
            if (!(strcmp(p->name, ".reginfo")) &&
                 (p->flags & (SEC_LOAD|SEC_ALLOC|SEC_RELOC)) )
              {
                p->flags &= ~(SEC_LOAD|SEC_ALLOC|SEC_RELOC); 
                p->flags |=  (SEC_READONLY|SEC_DEBUGGING);
              }
          }
      }

    /* Initialize */
    new = xballoc(abfd, sizeof(module_entry));
    memset(new, 0, sizeof(module_entry));
    new->filename = abfd->filename;
    new->abfd = abfd;
    new->exec = 0;
    
    /* Load extra information from object files */
    if (!archivep)
    {
        /* Read symbols */
        storage_needed = bfd_get_symtab_upper_bound(abfd);
        ASSERT_BFD(storage_needed>=0);
        if (storage_needed > 0) 
        {
            new->symbols = xballoc(abfd, storage_needed);
            new->symbol_count = bfd_canonicalize_symtab(abfd, new->symbols);
            ASSERT_BFD(new->symbol_count>0);
        }
        /* Remove flags BSF_KEEP on all symbols */
        for (i=0; i<new->symbol_count; i++) 
          {
            asymbol *sym = new->symbols[i];
            sym->flags &= ~BSF_KEEP;
          }
        /* Load relocations */
        for (p=abfd->sections; p; p=p->next)
        {
            /* Set output section and initialize size */
	    p->output_section = p;
	    p->output_offset = 0;
	    dldbfd_section_size(p) = dldbfd_section_rawsize(p);
            /* Read relocs */
            if (p->flags & SEC_RELOC)
            {
                arelent **reloc;
                int reloc_count;
                storage_needed = bfd_get_reloc_upper_bound(abfd, p);
                ASSERT_BFD(storage_needed>0);
                reloc = xballoc(abfd, storage_needed);
                reloc_count = bfd_canonicalize_reloc(abfd, p, reloc, new->symbols);
                ASSERT_BFD(reloc_count>0);
                ASSERT(p->reloc_count==0 || p->reloc_count==reloc_count);
                /* Keep relocation table in field orelocation (unused in input bfd) */
                p->reloc_count = reloc_count;
                p->orelocation = reloc; 
                /* Mark symbols used by relocations with flag BSF_KEEP */
                for (i=0; i<reloc_count; i++)
                  {
                    arelent *rel = reloc[i];
                    if (rel->sym_ptr_ptr && *rel->sym_ptr_ptr)
                      (*rel->sym_ptr_ptr)->flags |= BSF_KEEP;
                  }
            }
        }

        /* Discard unused symbols and count undefined symbols */
        for (i=0; i<new->symbol_count; i++) 
          {
            asymbol *sym = new->symbols[i];
            if (sym->flags & BSF_LOCAL)
              continue;
            if (!sym->name || !sym->name[0])
              continue;
            if (bfd_is_und_section(sym->section))
            {
              if (sym->flags & BSF_KEEP)
                {
                  /* This is a really useful undefined symbol */
                  sym->value = 0;
                  new->undefined_symbol_count += 1;
                }
              else
                {
                  /* Hide undefined symbols that are not useful */
                  sym->flags |= BSF_LOCAL;
                  sym->section = bfd_abs_section_ptr;
                }
            }
          }
    }
    
    /* Link module */
    new->next = *here;
    *here = new;
    return new;
}

/* create_file_entry : create an entry for a module in the main chain */

static module_entry *
create_file_entry(const char *fname)
{
  module_entry *ret;
  bfd *abfd = bfd_openr(fname, "default");
  TRY
  {
    ASSERT_BFD(abfd);
    if (bfd_check_format(abfd, bfd_object)) 
      {
        ret = create_module_entry(abfd, &dld_modules, FALSE);
        ret->archive_flag = FALSE;
      }
    else if (bfd_check_format(abfd, bfd_archive)) 
      {
        ret = create_module_entry(abfd, &dld_modules, TRUE);
        ret->archive_flag = TRUE;
      }
    else
      THROW(bfd_errmsg(bfd_error_wrong_format));
  }
  CATCH(n)
  {
    if (abfd)
      bfd_close(abfd);
    THROW(n);
  }
  END_CATCH;
  return ret;
}



/* ---------------------------------------- */
/* GLOBAL SYMBOL HASH TABLE */



/* dld_undefined_sym_count -- Number of undefined symbols
 *                            i.e: Number of symbols with flag DLD_REFD 
 *                                 and without flag DLDF_DEFD.
 */

int dld_undefined_sym_count = 0;


/* symbol_entry -- type for elements of the symbol hash table */

typedef struct symbol_entry {
    struct bfd_hash_entry root;
    module_entry *defined_by;
    bfd_vma definition;
    short flags;
    short refcount;
} symbol_entry;

#define DLDF_DEFD       1   /* symbol has been defined */
#define DLDF_REFD       2   /* symbol has been referenced */
#define DLDF_BFD        4   /* defintion points to a BFD symbol */
#define DLDF_FUNCTION   8   /* symbol is a function pointer */
#define DLDF_ALLOC     16   /* data has been allocated for this symbol */   
#define DLDF_INDIRECT  32   /* definition refer to other entry */


/* value_of_symbol -- return the value of a defined symbol */

static inline bfd_vma
value_of_symbol(symbol_entry *hsym)
{
    ASSERT(hsym->flags & DLDF_DEFD);
    if (hsym->flags & DLDF_INDIRECT)
        return value_of_symbol((symbol_entry*)vmaptr(hsym->definition));
    if (hsym->flags & DLDF_BFD)
        return value_of_bfd_symbol((asymbol*)vmaptr(hsym->definition));
    return hsym->definition;
}


/* init_symbol_entry -- initializes a hash table component */

static struct bfd_hash_entry *
init_symbol_entry(struct bfd_hash_entry *entry, 
                  struct bfd_hash_table *table,
                  const char *string)
{
    symbol_entry *ret = (symbol_entry*)entry;
    if (!ret) 
    {
        ret = (symbol_entry*)bfd_hash_allocate(table, sizeof(symbol_entry));
        if (!ret)
            THROW(bfd_errmsg(bfd_error_no_memory));
    }
    memset(ret, 0, sizeof(symbol_entry));
    return bfd_hash_newfunc((struct bfd_hash_entry*)ret, table, string);
}


/* global_symbol_table -- the global symbol hash table */

static struct bfd_hash_table global_symbol_table;


/* global_dlopen_handle -- dlopen handle to main executable */
#ifdef DLOPEN
static dlopen_handle_t *global_dlopen_handle = 0;
#endif

/* init_global_symbol_table -- initializes the global symbol table */

static void
init_global_symbol_table(void)
{
    static int initialized = 0;
    ASSERT(!initialized);
    my_bfd_hash_table_init(&global_symbol_table, init_symbol_entry, sizeof(symbol_entry));
    initialized = 1;
}


/* lookup_symbol -- retrieves an symbol in the global table */

static inline symbol_entry *
lookup_symbol(const char *id)
{
    return (symbol_entry*)bfd_hash_lookup(&global_symbol_table, 
                                          id, FALSE, FALSE);
}


/* insert_symbol -- insert a symbol in the global table */

static inline symbol_entry *
insert_symbol(const char *id)
{
    return (symbol_entry*)bfd_hash_lookup(&global_symbol_table,
                                          id, TRUE, TRUE);
}




/* ---------------------------------------- */
/* COMMON SYMBOL ALLOCATION */

/* Common symbols are handled as BSS entries
 * We just append them to the BSS section.
 */


/* align_object_location -- computes natural alignment for an object */

static void
align_object_location(bfd_vma *location, int size)
{
    bfd_vma mask;
    int power = bfd_log2(size);
    if (power > 4)
        power = 4;
    mask = (1L<<power) - 1L;
    *location = (*location + mask) & ~mask;
}

/* handle_common_symbols -- append common symbols to BSS section */

static int
handle_common_symbols(module_entry *ent)
{
    int i;
    asection *sbss = 0;
    int ncommon = 0;
    bfd_vma scommon = 0;
    symbol_entry *hsym;
    asymbol *sym;
    symvalue size;    
    
    /* Compute size of common section */
    for (i=0; i<ent->symbol_count; i++)
    {
        sym = ent->symbols[i];
        if (bfd_is_com_section(sym->section)) 
        {
            ncommon += 1;
            /* Test if common symbol has already been defined */
            hsym = lookup_symbol(drop_leading_char(ent->abfd, sym->name));
            if (hsym && (hsym->flags & DLDF_DEFD))
            {
                if (hsym->flags & DLDF_FUNCTION)
                {
                    sprintf(error_buffer,
                            "Common reference to already defined function '%s'",
                            sym->name );
                    THROW(error_buffer);
                }
                if (! dld_compatibility_flag)
                {
                    asymbol *csym;
                    if (!(hsym->flags & DLDF_BFD) ||
                        !(csym = vmaptr(hsym->definition)) ||
                        !(csym->flags & BSF_OLD_COMMON) )
                    {
                        sprintf(error_buffer,
                                "Common reference to a non common symbol '%s'",
                                sym->name );
                        THROW(error_buffer);
                    }
                    if ((symvalue)csym->udata.i != sym->value)
                    {
                        sprintf(error_buffer,
                                "Common references to '%s' have different sizes",
                                sym->name );
                        THROW(error_buffer);
                    }
                }
            }
            else
            {
                align_object_location(&scommon, sym->value);
                scommon += sym->value;
            }
        }
    }
    /* Retrieve bss section */
    if (scommon > 0)
    {
        sbss = bfd_get_section_by_name(ent->abfd, ".bss");
        if (! sbss) 
        {
            sbss = bfd_make_section(ent->abfd, ".bss");
            ASSERT_BFD(sbss);
            sbss->flags = SEC_ALLOC;
        }
        ASSERT(sbss->flags & SEC_ALLOC);
        scommon = dldbfd_section_rawsize(sbss);
    }
    /* Change common symbols into BSS or UND symbols */
    if (ncommon > 0)
    {
        for (i=0; i<ent->symbol_count; i++)
        {
            sym = ent->symbols[i];
            if (bfd_is_com_section(sym->section)) 
            {
                /* Test if common symbol has already been defined */
                hsym = lookup_symbol(drop_leading_char(ent->abfd, sym->name));
                if (hsym && (hsym->flags & DLDF_DEFD))
                {
                    /* Transform already defined common symbol into reference */
                    sym->section = bfd_und_section_ptr;
                    sym->value = 0;
                    sym->flags &= ~(BSF_GLOBAL|BSF_WEAK|BSF_INDIRECT);
                }
                else
                {
                    /* Append common symbol to BSS section */
                    ASSERT(sbss);
                    size = sym->value;
                    align_object_location(&scommon, size);
                    sym->value = scommon;
                    sym->section = sbss;
                    sym->flags |= (BSF_GLOBAL|BSF_OLD_COMMON);
                    sym->udata.i = size; /* store size there */
                    scommon += size;
                }
            }
        }
    }
    /* Cleanup bss section */
    if (sbss)
    {
        bfd_set_section_size(ent->abfd, sbss, scommon);
        /* Take no risk with alignment */
        if (sbss->alignment_power < 4)
            sbss->alignment_power = 4;
    }
    /* Return number of common symbols */
    return ncommon;
}



/* ---------------------------------------- */
/* PATCHING BFD FOR SPECIAL ARCHS */

#ifdef __GNUC__
static void write_const_pointer(void **pointer, void *value)
     __attribute__ ((unused));
#endif

static void
write_const_pointer(void **pointer, void *value)
{
#if HAVE_MPROTECT
  int pagesize = sysconf(_SC_PAGESIZE);
  bfd_vma start = ptrvma(pointer) & ~(pagesize-1);
  mprotect( vmaptr(start), pagesize, PROT_READ|PROT_WRITE);
#endif
  *pointer = value;
}


/* ---------------------------------------- */
/* SPECIAL PROCESSING FOR MIPS-ELF */

#ifdef __mips__

/* ELF MIPS use the pic model by default.
 * Most of BFD relocation code does not deal with PIC stuff properly.
 * So we need to reimplement minimal support for GOT.
 * The following code:
 * - patches the howtos for PIC related relocs.
 * - appends a .got section to each object module.
 */

/* mipself_old_{lo16,hi16}_reloc --
   pointer to old reloc functions */

static bfd_reloc_status_type
(*mipself_old_lo16_reloc) (bfd *abfd,
                           arelent *reloc_entry,
                           asymbol *symbol,
                           PTR data,
                           asection *input_section,
                           bfd *output_bfd,
                           char **error_message) = NULL;

static bfd_reloc_status_type
(*mipself_old_hi16_reloc) (bfd *abfd,
                           arelent *reloc_entry,
                           asymbol *symbol,
                           PTR data,
                           asection *input_section,
                           bfd *output_bfd,
                           char **error_message) = NULL;



/* mipself_gpdisp_hiaddr, mipself_gpdisp_value --
   static variables for setting gp_disp */

static bfd_vma mipself_gpdisp_value = 0;
static bfd_vma mipself_gpdisp_hiaddr = 0;


/* mipself_new_{lo16,hi16,gprel32}_reloc --
   new reloc functions patching the bogus BFD functions */

static bfd_reloc_status_type
mipself_new_hi16_reloc (bfd *abfd,
                        arelent *reloc_entry,
                        asymbol *symbol,
                        PTR data,
                        asection *input_section,
                        bfd *output_bfd,
                        char **error_message)
{
  if (output_bfd)
    return bfd_reloc_notsupported;    
  /* Test if symbol is '_gp_disp' because it changes everything */
  if (!strcmp(symbol->name, "_gp_disp"))
    {
      mipself_gpdisp_hiaddr = ptrvma(data) + reloc_entry->address;
      mipself_gpdisp_value = symbol->section->vma + symbol->value;
      /* Assume no addend for simplicity*/
      if (reloc_entry->addend)
        return bfd_reloc_notsupported;
      return bfd_reloc_ok;
    }
  /* Regular procedure */
  return (*mipself_old_hi16_reloc)(abfd, reloc_entry, symbol,
                                   data, input_section, 
                                   output_bfd, error_message);
}

static bfd_reloc_status_type
mipself_new_lo16_reloc (bfd *abfd,
                        arelent *reloc_entry,
                        asymbol *symbol,
                        PTR data,
                        asection *input_section,
                        bfd *output_bfd,
                        char **error_message)
{
  if (output_bfd)
    return bfd_reloc_notsupported;    
  /* Test if symbol is '_gp_disp' because it changes everything */
  if(!strcmp(symbol->name, "_gp_disp"))
    {
      int loinsn, hiinsn;
      bfd_vma loaddr = ptrvma(data) + reloc_entry->address;
      bfd_vma value = symbol->section->vma + symbol->value;
      ASSERT_BFD(value == mipself_gpdisp_value);
      /* Assume no addend for simplicity*/
      if (reloc_entry->addend)
        return bfd_reloc_notsupported;
      /* Get instructions */
      loinsn = bfd_get_32(abfd, (bfd_byte*)vmaptr(loaddr));
      hiinsn = bfd_get_32(abfd, (bfd_byte*)vmaptr(mipself_gpdisp_hiaddr));
      if ((loinsn & 0xffff) || (hiinsn & 0xffff))
        return bfd_reloc_notsupported;
      /* Patch instructions (taking care of sign extension on the low 16 bits) */
      value = value - mipself_gpdisp_hiaddr;
      loinsn |= (value & 0xffff);
      value = (value>>16) + (loinsn&0x8000 ? 1 : 0); 
      hiinsn |= (value & 0xffff);
      bfd_put_32 (abfd, hiinsn, (bfd_byte*)vmaptr(mipself_gpdisp_hiaddr));
      bfd_put_32 (abfd, loinsn, (bfd_byte*)vmaptr(loaddr));
      return bfd_reloc_ok;
    }
  /* Regular procedure */
  return (*mipself_old_lo16_reloc)(abfd, reloc_entry, symbol,
                                   data, input_section, 
                                   output_bfd, error_message);
}

static bfd_reloc_status_type
mipself_new_call16_reloc (bfd *abfd, 
                         arelent *reloc_entry,
                         asymbol *symbol,
                         PTR data,
                         asection *input_section,
                         bfd *output_bfd,
                         char **error_message)
{
  /* This should only happen with GPDISP symbol */
  int insn;
  bfd_byte *addr;
  /* Plug GOT offset into low 16 bits of instruction */
  addr = (bfd_byte*)(data) + reloc_entry->address;
  insn = bfd_get_32(abfd, addr);
  if (insn & 0x0000ffff)
    return bfd_reloc_notsupported;    
  if (reloc_entry->addend != (reloc_entry->addend & 0x7fff))
    return bfd_reloc_notsupported;        
  insn |= reloc_entry->addend;
  bfd_put_32(abfd, insn, addr);
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
mipself_new_got16_reloc (bfd *abfd, 
                         arelent *reloc_entry,
                         asymbol *symbol,
                         PTR data,
                         asection *input_section,
                         bfd *output_bfd,
                         char **error_message)
{
  int insn;
  bfd_byte *addr;
  /* If symbol is '_gp_disp' same as CALL16 */
  if (!strcmp(symbol->name,"_gp_disp"))
    return mipself_new_call16_reloc(abfd, reloc_entry, symbol,
                                    data, input_section, 
                                    output_bfd, error_message);
  /* We have now a brain damaged GOT16/LO16 sequence 
     These happen when a GOT16 reloc points to a section symbol.
     This is used to access static objects (PIC) by first giving an
     offset to a GOT entry pointing the right 64kb page (GOT16) and
     a 16 bit displacement (LO16). We prefer patching the code
     into a non PIC sequence similar to HI16/LO16 sequences. */
  addr = (bfd_byte*)(data) + reloc_entry->address;
  insn = bfd_get_32(abfd, addr);
  /* Transform 'LW Rn, off(GP)' into 'LUI Rn, off' */
  if ((insn & 0xffe00000) != 0x8f800000)
    return bfd_reloc_notsupported;    
  insn &= ~0xffe00000;
  insn |=  0x3c000000;
  bfd_put_32(abfd, insn, addr);
  /* We can then process it as HI16 */
  return (*mipself_old_hi16_reloc)(abfd, reloc_entry, symbol,
                                   data, input_section, 
                                   output_bfd, error_message);
}


/* mipself_install_patches -- install patches into BFD */

static void
mipself_install_patches(bfd *abfd)
{
  reloc_howto_type *howto;
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_HI16_S);
  ASSERT_BFD(howto);
  if (howto->special_function != mipself_new_hi16_reloc)
    {
      mipself_old_hi16_reloc = howto->special_function;
      write_const_pointer((void**)&howto->special_function, mipself_new_hi16_reloc);
      ASSERT_BFD(mipself_old_hi16_reloc);
    }
  /* LO16 */
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_LO16);
  ASSERT_BFD(howto);
  if (howto->special_function != mipself_new_lo16_reloc)
    {
      mipself_old_lo16_reloc = howto->special_function;
      write_const_pointer((void**)&howto->special_function, mipself_new_lo16_reloc);
      ASSERT_BFD(mipself_old_lo16_reloc);
    }
  /* GOT16 */
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_MIPS_GOT16);
  ASSERT_BFD(howto);
  write_const_pointer((void**)&howto->special_function, mipself_new_got16_reloc);  
  /* CALL16 */
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_MIPS_CALL16);
  ASSERT_BFD(howto);
  write_const_pointer((void**)&howto->special_function, mipself_new_call16_reloc);
}


/* mipself_got_info -- structure containing GOT information */

typedef struct mipself_got_info {
  int gotsize;
  asection *sgot;
  struct bfd_hash_table got_table;
  reloc_howto_type *reloc_got16;
  reloc_howto_type *reloc_call16;
  reloc_howto_type *reloc_32;       /* for relocs in got */
} mipself_got_info;


/* mipself_got_entry -- structure for GOT entries */

typedef struct mipself_got_entry {
  struct bfd_hash_entry root;
  arelent *reloc;
  bfd_vma offset;
  int index;
} mipself_got_entry;


/* mipself_init_got_entry -- initialize got hash table component */

static struct bfd_hash_entry *
mipself_init_got_entry(struct bfd_hash_entry *entry,
                       struct bfd_hash_table *table,
                       const char *string)
{
  mipself_got_entry *ret = (mipself_got_entry*)entry;
  if (!ret)
    {
      ret = (mipself_got_entry*)
        bfd_hash_allocate(table, sizeof(mipself_got_entry));
      if (!ret)
        THROW(bfd_errmsg(bfd_error_no_memory));
    }
  memset(ret, 0, sizeof(mipself_got_entry));
  return bfd_hash_newfunc((struct bfd_hash_entry*)ret, table, string);
}


/* mipself_init_got_info -- initialize info about got */

static void
mipself_init_got_info(module_entry *ent, 
                      mipself_got_info *info)
{
  /* Create GOT hash table */
  info->sgot = NULL;
  info->gotsize = 0;
  my_bfd_hash_table_init(&info->got_table, mipself_init_got_entry, sizeof(mipself_got_entry));
  /* Identify pertinent howto information */
  info->reloc_got16 = bfd_reloc_type_lookup(ent->abfd,BFD_RELOC_MIPS_GOT16);
  info->reloc_call16 = bfd_reloc_type_lookup(ent->abfd,BFD_RELOC_MIPS_CALL16);
  info->reloc_32 = bfd_reloc_type_lookup(ent->abfd,BFD_RELOC_32);
}


/* mipself_free_got_info -- free got information */

static void
mipself_free_got_info(module_entry *ent, 
                      mipself_got_info *info)
{
  bfd_hash_table_free(&info->got_table);
}


/* mipself_got_traverse -- got table traversal function */

static int
mipself_got_traverse(struct bfd_hash_entry *ent, void *gcookie)
{
  mipself_got_entry *gotent = (mipself_got_entry*)ent;
  asection *sgot = (asection*)gcookie;
  sgot->orelocation[gotent->index] = gotent->reloc;
  return TRUE;
}


/* mipself_create_got -- create a section for GOT */

static asection *
mipself_create_got(module_entry *ent, 
                   mipself_got_info *info)
{
  asection *sgot;
  bfd_vma offset;
  asection *p;
  bfd *abfd;
  int i;
  /* Check that there are no GOT section */
  sgot = bfd_get_section_by_name(ent->abfd, ".got");
  ASSERT_BFD(!sgot);
  /* Compute GOT hash table */
  offset = 0;
  abfd = ent->abfd;
  for (p=abfd->sections; p; p=p->next)
    if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC))
      {
        ASSERT(p->orelocation);
        for (i=0; i<p->reloc_count; i++)
          {
            arelent *reloc = p->orelocation[i];
            asymbol *symbol = *(reloc->sym_ptr_ptr);
            if ((reloc->howto==info->reloc_call16) 
                || ((reloc->howto==info->reloc_got16) 
                    && !(symbol->flags & BSF_SECTION_SYM) ) )
              {
                /* Search entry in GOT table */
                const char *name = (*reloc->sym_ptr_ptr)->name;
                mipself_got_entry *gotent = (mipself_got_entry*)
                  bfd_hash_lookup(&info->got_table, name, TRUE, TRUE );
                /* Fill entry */
                if (!gotent->reloc)
                  {
                    /* Compute offset */
                    gotent->index = info->gotsize;
                    gotent->offset = offset;
                    /* Prepare relocation entry */
                    gotent->reloc = xballoc(ent->abfd, sizeof(arelent));
                    gotent->reloc->sym_ptr_ptr = reloc->sym_ptr_ptr;
                    gotent->reloc->address = offset;
                    gotent->reloc->addend = 0;
                    gotent->reloc->howto = info->reloc_32;
                    /* Increment GOT offset */
                    offset += sizeof(void*);
                    info->gotsize += 1;
                  }
              }
          }
      }
  /* Check GOT size */
  if (offset >= 0x8000)
    THROW("Too many entries in GOT (GOT full, merci MIPS)");
  /* Create GOT if necessary */
  if (info->gotsize > 0)
    {
      sgot = bfd_make_section(ent->abfd, ".got");
      ASSERT_BFD(sgot);
      sgot->flags = SEC_ALLOC|SEC_RELOC;
      dldbfd_section_rawsize(sgot) = 0;
      dldbfd_section_size(sgot) = offset;
      if (sgot->alignment_power < 4)
        sgot->alignment_power = 4;
      /* Set GOT relocations */
      sgot->orelocation = xballoc(ent->abfd, info->gotsize*sizeof(void*));
      bfd_hash_traverse(&info->got_table, (void*)mipself_got_traverse, sgot);
      sgot->reloc_count = info->gotsize;
    }
  /* return GOT section if any */
  info->sgot = sgot;
  return sgot;
}


/* mipself_fix_relocs -- patches the relocation for GOT */

static void 
mipself_fix_relocs(module_entry *ent, 
                   mipself_got_info *info)
{
  int i;
  asymbol *gpdisp = NULL;
  asymbol **gpdisp_ptr = NULL;
  asection *p;
  bfd *abfd;

  /* Locate and patch gpdisp symbol */
  for (i=0; i<ent->symbol_count; i++)
    if (!strcmp(ent->symbols[i]->name, "_gp_disp"))
      {
        gpdisp = ent->symbols[i];
        if (bfd_is_und_section(gpdisp->section))
          ent->undefined_symbol_count -= 1;
        gpdisp->value = 0;
        gpdisp->flags = BSF_LOCAL|BSF_SECTION_SYM;
        if (info->sgot)
          gpdisp->section = info->sgot;
        else
          gpdisp->section =  bfd_abs_section_ptr;
        gpdisp->udata.p = NULL;
        break;
      }
  if (!info->sgot)
    return;
  if (!gpdisp)
    THROW("Found GOT relocations but no symbol '_gp_disp'");
  
  /* Prepare pointer to gpdisp to patch got relocs */
  gpdisp_ptr = xballoc(ent->abfd, sizeof(asymbol*));
  *gpdisp_ptr = gpdisp;
  
  /* Iterate on sections and relocations */
  abfd = ent->abfd;
  for (p=abfd->sections; p; p=p->next)
    {
      if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC))
        {
          ASSERT(p->orelocation);
          for (i=0; i<p->reloc_count; i++)
            {
              arelent *reloc = p->orelocation[i];
              asymbol *symbol = *reloc->sym_ptr_ptr;
              if ((reloc->howto==info->reloc_call16) 
                  || ((reloc->howto==info->reloc_got16) 
                      && !(symbol->flags & BSF_SECTION_SYM) ) )
                {
                  /* Find GOT entry */
                  mipself_got_entry *gotent = (mipself_got_entry*)
                    bfd_hash_lookup(&info->got_table, symbol->name, FALSE, FALSE );
                  ASSERT_BFD(gotent);
                  /* Addend should be null */
                  if (reloc->addend)
                    THROW("Non zero addend in GOT16/CALL16 relocation");
                  /* GOT16 and CALL16 relocations will refer
                   * to the GPDISP symbol and have the GOT index
                   * in the addend */
                  reloc->addend = gotent->offset;
                  reloc->sym_ptr_ptr = gpdisp_ptr;
                }
            } 
        }
    }
}

/* handle_mipself -- special processing for mips elf */

static void
handle_mipself_code(module_entry *ent)
{
  /* MIPSELF case */
  if (bfd_get_flavour(ent->abfd) == bfd_target_elf_flavour &&
      bfd_get_arch(ent->abfd) == bfd_arch_mips )
    {
      mipself_got_info got_info;
      /* Install BFD patches */
      mipself_install_patches(ent->abfd);
      /* Create GOT section */
      mipself_init_got_info(ent, &got_info);
      TRY
        {
          mipself_create_got(ent, &got_info);
          mipself_fix_relocs(ent, &got_info);
        }
      CATCH(n)
        {
          mipself_free_got_info(ent, &got_info);
          THROW(n);
        }
      END_CATCH;
      mipself_free_got_info(ent, &got_info);
    }
}
   
#endif


/* ---------------------------------------- */
/* SPECIAL PROCESSING FOR ALPHA 
   Return courtesy from GCL.
   Untested */

#ifdef __alpha__

static asymbol *alphaelf_gpdisp = NULL;

static unsigned long alphaelf_gp(bfd *abfd)
{
  extern unsigned long _bfd_set_gp_value(bfd*,unsigned long);
  unsigned long gp = alphaelf_gpdisp->section->output_section->vma;
  gp += alphaelf_gpdisp->value;
  _bfd_set_gp_value(abfd, gp);
  return gp;
}

static bfd_reloc_status_type
(*alphaelf_old_gpdisp_reloc)(bfd *abfd, 
			     arelent *reloc_entry,
			     asymbol *symbol,
			     PTR data,
			     asection *input_section,
			     bfd *output_bfd,
			     char **error_message);

static bfd_reloc_status_type
alphaelf_new_gpdisp_reloc (bfd *abfd, 
                           arelent *reloc_entry,
                           asymbol *symbol,
                           PTR data,
                           asection *input_section,
                           bfd *output_bfd,
                           char **error_message) 
{
  alphaelf_gp(abfd);
  return alphaelf_old_gpdisp_reloc(abfd,reloc_entry,
				   symbol,data,input_section,
				   output_bfd,error_message);
}

static bfd_reloc_status_type
alphaelf_new_literal_reloc (bfd *abfd, 
			    arelent *reloc_entry,
			    asymbol *symbol,
			    PTR data,
			    asection *input_section,
			    bfd *output_bfd,
			    char **error_message) 
{
  int insn;
  bfd_byte *addr;
  addr = (bfd_byte*)(data) + reloc_entry->address;
  insn = bfd_get_32(abfd, addr);
  if (insn & 0x0000ffff)
    return bfd_reloc_notsupported;    
  if (reloc_entry->addend != (reloc_entry->addend & 0x7fff))
    return bfd_reloc_notsupported;        
  /* Plug GOT offset into low 16 bits of instruction */
  insn |= reloc_entry->addend;
  bfd_put_32(abfd, insn, addr);
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
alphaelf_new_hi16_reloc (bfd *abfd, 
                         arelent *reloc_entry,
                         asymbol *symbol,
                         PTR data,
                         asection *input_section,
                         bfd *output_bfd,
                         char **error_message) 
{
  int loinsn, hiinsn;
  bfd_vma addr = ptrvma(data) + reloc_entry->address;
  bfd_vma value = symbol->section->vma + symbol->value + reloc_entry->addend;
  hiinsn = bfd_get_32(abfd, (bfd_byte*)vmaptr(addr));
  if ((hiinsn & 0xffff))
    return bfd_reloc_notsupported;
  value = value - alphaelf_gp(abfd);
  loinsn = (value & 0xffff);
  value = (value>>16) + (loinsn&0x8000 ? 1 : 0); 
  hiinsn |= (value & 0xffff);
  bfd_put_32 (abfd, hiinsn, (bfd_byte*)vmaptr(addr));
  return bfd_reloc_ok;

}

static bfd_reloc_status_type
alphaelf_new_lo16_reloc (bfd *abfd, 
                         arelent *reloc_entry,
                         asymbol *symbol,
                         PTR data,
                         asection *input_section,
                         bfd *output_bfd,
                         char **error_message) 
{
  int insn;
  bfd_vma addr = ptrvma(data) + reloc_entry->address;
  bfd_vma value = symbol->section->vma + symbol->value + reloc_entry->addend;
  insn = bfd_get_32(abfd, (bfd_byte*)vmaptr(addr));
  if ((insn & 0xffff))
    return bfd_reloc_notsupported;
  value = value - alphaelf_gp(abfd);
  insn |= (value & 0xffff);
  bfd_put_32 (abfd, insn, (bfd_byte*)vmaptr(addr));
  return bfd_reloc_ok;
}

static void
alphaelf_install_patches(bfd *abfd) 
{
  reloc_howto_type *howto;
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_ALPHA_GPDISP);
  ASSERT_BFD(howto);
  if (howto->special_function != alphaelf_new_gpdisp_reloc) 
    {
      alphaelf_old_gpdisp_reloc=howto->special_function;
      ASSERT_BFD(alphaelf_old_gpdisp_reloc);
      write_const_pointer((void**)&howto->special_function, alphaelf_new_gpdisp_reloc);
    }
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_ALPHA_GPREL_HI16);
  ASSERT_BFD(howto);
  if (howto->special_function != alphaelf_new_hi16_reloc)
    write_const_pointer((void**)&howto->special_function, alphaelf_new_hi16_reloc);
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_ALPHA_GPREL_LO16);
  ASSERT_BFD(howto);
  if (howto->special_function != alphaelf_new_lo16_reloc)
    write_const_pointer((void**)&howto->special_function, alphaelf_new_lo16_reloc);
  howto = bfd_reloc_type_lookup(abfd,BFD_RELOC_ALPHA_ELF_LITERAL);
  ASSERT_BFD(howto);
  if (howto->special_function != alphaelf_new_literal_reloc)
    write_const_pointer((void**)&howto->special_function, alphaelf_new_literal_reloc);
}

typedef struct alphaelf_got_info {
  int gotsize;
  asection *sgot;
  struct bfd_hash_table got_table;
  reloc_howto_type *reloc_literal;
  reloc_howto_type *reloc_64;       /* for relocs in got */
} alphaelf_got_info;

typedef struct alphaelf_got_entry {
  struct bfd_hash_entry root;
  arelent *reloc;
  bfd_vma offset;
  int index;
} alphaelf_got_entry;

static struct bfd_hash_entry *
alphaelf_init_got_entry(struct bfd_hash_entry *entry,
			struct bfd_hash_table *table,
			const char *string) 
{
  alphaelf_got_entry *ret = (alphaelf_got_entry*)entry;
  if (!ret) 
    {
      ret = (alphaelf_got_entry*)
	bfd_hash_allocate(table, sizeof(alphaelf_got_entry));
      if (!ret)
	THROW(bfd_errmsg(bfd_error_no_memory));
    }
  memset(ret, 0, sizeof(alphaelf_got_entry));
  return bfd_hash_newfunc((struct bfd_hash_entry*)ret, table, string);
}

static void
alphaelf_init_got_info(module_entry *ent, 
		       alphaelf_got_info *info) 
{
  info->sgot = NULL;
  info->gotsize = 0;
  my_bfd_hash_table_init(&info->got_table, alphaelf_init_got_entry, sizeof(alphaelf_got_entry));
  info->reloc_literal = bfd_reloc_type_lookup(ent->abfd,BFD_RELOC_ALPHA_ELF_LITERAL);
  info->reloc_64 = bfd_reloc_type_lookup(ent->abfd,BFD_RELOC_64);
}

static void
alphaelf_free_got_info(alphaelf_got_info *info) 
{
  bfd_hash_table_free(&info->got_table);
}

static int
alphaelf_got_traverse(struct bfd_hash_entry *ent, void *gcookie) 
{
  alphaelf_got_entry *gotent = (alphaelf_got_entry*)ent;
  asection *sgot = (asection*)gcookie;
  sgot->relocation[gotent->index] = *gotent->reloc;
  return TRUE;
}

static const char *
alphaelf_msymbol_name(bfd *abfd, const asymbol *s, unsigned long a) 
{
  char *n;
  if (!(s->flags & BSF_SECTION_SYM) && !a)
    return s->name;
  n = xballoc(abfd, strlen(s->name)+18);
  sprintf(n,"%s+%016lx", s->name, a);
  return n;
}

static asection *
alphaelf_create_got(module_entry *ent, 
		    alphaelf_got_info *info) 
{
  asection *sgot;
  bfd_vma offset;
  asection *p;
  bfd *abfd;
  int i;
  /* Check that there are no GOT section */
  sgot = bfd_get_section_by_name(ent->abfd, ".got");
  ASSERT_BFD(!sgot);
  /* Compute GOT hash table */
  offset = 0;
  abfd = ent->abfd;
  for (p=abfd->sections; p; p=p->next)
    if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC)) 
      {
	ASSERT(p->orelocation);
	for (i=0; i<p->reloc_count; i++) 
	  {
	    arelent *reloc = p->orelocation[i];
	    if ((reloc->howto == info->reloc_literal)) 
	      {
		const char *name = alphaelf_msymbol_name(abfd,*reloc->sym_ptr_ptr,reloc->addend);
		alphaelf_got_entry *gotent = (alphaelf_got_entry*)
		  bfd_hash_lookup(&info->got_table, name, TRUE, TRUE );
		if (!gotent->reloc) {
		  gotent->index = info->gotsize;
		  gotent->offset = offset;
		  gotent->reloc = xballoc(ent->abfd, sizeof(arelent));
		  gotent->reloc->sym_ptr_ptr = reloc->sym_ptr_ptr;
		  gotent->reloc->address = offset;
		  gotent->reloc->addend = reloc->addend;
		  gotent->reloc->howto = info->reloc_64;
		  offset += sizeof(void*);
		  info->gotsize += 1;
		}
	      }
	  }
      }
  if (offset >= 0x8000)
    THROW("Too many entries in GOT (GOT full, merci ALPHA)");
  if (info->gotsize > 0) 
    {
      sgot = bfd_make_section(ent->abfd, ".got");
      ASSERT(sgot);
      sgot->flags = SEC_ALLOC|SEC_RELOC|SEC_LOAD;
      dldbfd_section_rawsize(sgot) = 0;
      dldbfd_section_size(sgot) = offset;
      if (sgot->alignment_power < 4)
	sgot->alignment_power = 4;      /* Set GOT relocations */
      sgot->relocation = xballoc(abfd, info->gotsize*sizeof(*info->sgot->relocation));
      bfd_hash_traverse(&info->got_table, (void*)alphaelf_got_traverse, sgot);
      sgot->reloc_count = info->gotsize;
    }
  info->sgot = sgot;
  return sgot;

}

static void 
alphaelf_fix_relocs(module_entry *ent, 
		    alphaelf_got_info *info) 
{
  int i;
  asymbol *gpdisp = NULL;
  asymbol **gpdisp_ptr = NULL;
  asection *p;
  bfd *abfd = ent->abfd;
  /* Make a gpdisp symbol at the top of the .got section */
  gpdisp = xballoc(abfd, sizeof(asymbol));
  gpdisp->value = 0;
  gpdisp->name= "gpdisp";
  gpdisp->flags = BSF_LOCAL|BSF_SECTION_SYM;
  gpdisp->section = info->sgot ? info->sgot : bfd_abs_section_ptr;
  gpdisp->udata.p = NULL;
  alphaelf_gpdisp = gpdisp;
  if (!info->sgot)
    return;
  
  /* Prepare pointer to gpdisp to patch got relocs */
  gpdisp_ptr = xballoc(abfd, sizeof(asymbol*));
  *gpdisp_ptr = gpdisp;
  
  /* Iterate on sections and relocations */
  for (p=abfd->sections; p; p=p->next)
    if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC)) 
      {
	ASSERT_BFD(p->orelocation);
	for (i=0; i<p->reloc_count; i++) 
	  {
	    arelent *reloc = p->orelocation[i];
	    if (reloc->howto==info->reloc_literal) 
	      {
		const char *name = alphaelf_msymbol_name(abfd,*reloc->sym_ptr_ptr,reloc->addend);
		alphaelf_got_entry *gotent = 
		  (alphaelf_got_entry*) bfd_hash_lookup(&info->got_table, name, FALSE, FALSE );
		ASSERT(gotent);
		/* ELF_LITERAL relocations will refer
		 * to the GPDISP symbol and have the GOT index
		 * in the addend */
		reloc->addend = gotent->offset;
		reloc->sym_ptr_ptr = gpdisp_ptr;
	      }
	  } 
      }
}


/* handle_alphaelf -- special processing for alpha elf */

static void
handle_alphaelf_code(module_entry *ent)
{
  /* ALPHAELF case */
  if (bfd_get_flavour(ent->abfd) == bfd_target_elf_flavour &&
      bfd_get_arch(ent->abfd) == bfd_arch_alpha )
    {
      alphaelf_got_info got_info;
      alphaelf_install_patches(ent->abfd);
      alphaelf_init_got_info(ent, &got_info);
      TRY
        {
	  alphaelf_create_got(ent, &got_info);
	  alphaelf_fix_relocs(ent, &got_info);
	}
      CATCH(n)
	{
	  alphaelf_free_got_info(&got_info);
	  THROW(n);
	}
      END_CATCH;
      alphaelf_free_got_info(&got_info);
    }
}

#endif

/* ---------------------------------------- */
/* REWRITES */


/* perform_rewrites -- calls all special case routines */

static void
perform_rewrites(module_entry *ent)
{
  handle_common_symbols(ent);
#ifdef __mips__
  handle_mipself_code(ent);
#endif
#ifdef __alpha__
  handle_alphaelf_code(ent);
#endif
}


/* ---------------------------------------- */
/* MEMORY ALLOCATOR IN LOW MEMORY 
   The default X86-64 code expects to live in the low 2GB of memory. 
   This is implemented using the MAP_32BIT mmap flag.
*/

#ifdef __x86_64__
# ifdef MAP_32BIT
#  define DLD_ALLOCATE_DEFINED
#  define DLD_DEALLOCATE_DEFINED

typedef struct a_arena_s {
  struct a_arena_s *next;
  size_t size;
} a_arena_t;

#define A_ALIGN(x)   (((x)+0xF)&~0xF)
#define A_CHUNKSIZE  (0x400000)

static a_arena_t *arenas = 0;

static void *
a_chunk(size_t sz)
{
  static int fd = -1;
  int prot = PROT_READ|PROT_WRITE|PROT_EXEC;
  int flag = MAP_PRIVATE;
#ifdef MAP_32BIT
  flag |= MAP_32BIT;
#endif
#ifdef MAP_ANONYMOUS
  flag |= MAP_ANONYMOUS;
#else
  if (fd < 0)
    fd = open("/dev/zero", O_RDWR);
  if (fd < 0)
    return NULL;
#endif
  return mmap(0, sz, prot, flag, fd, 0);
}

static void*
dld_allocate(size_t s, int perm)
{
  a_arena_t *a, **ap;
  /* round s */
  s = A_ALIGN(s) + A_ALIGN(sizeof(a_arena_t));
  /* search */
  ap = &arenas;
  while ((a = *ap) && (a->size < s))
    ap = &(a->next);
  if (!a)
    {
      /* get a new chunk */
      size_t cs = A_CHUNKSIZE;
      while (cs < s)
	cs += A_CHUNKSIZE;
      a = a_chunk(cs);
      if (!a)
        THROW(bfd_errmsg(bfd_error_no_memory));
      ap = &arenas;
      while(*ap && a>=*ap)
	ap = &((*ap)->next);
      a->size = cs;
      a->next = *ap;
      *ap = a;
    }
  if (a->size < s + A_ALIGN(sizeof(a_arena_t)))
    s = a->size;
  if (s < a->size)
    {
      if (perm)
	{
	  a_arena_t *b = a;
	  b->size = a->size - s;
	  a = (a_arena_t*)((char*)b + b->size);
	  a->size = s;
	  a->next = b->next;
	  b->next = a;
	  ap = &(b->next);
	}
      else
	{
	  a_arena_t *b = (a_arena_t*)((char*)a + s);
	  b->next = a->next;
	  b->size = a->size - s;
	  a->size = s;
	  a->next = b;
	}
    }
  *ap = a->next;
  a->next = (a_arena_t*)((char*)a + A_ALIGN(sizeof(a_arena_t)));
  return a->next;
}

static void
dld_deallocate(void *x)
{
  if (x) 
    {
      a_arena_t *a, **ap;
      a = (a_arena_t*)((char*)x - A_ALIGN(sizeof(a_arena_t)));
      if (a->next != x)
	THROW(bfd_errmsg(bfd_error_no_memory));
      /* insert */
      ap = &arenas;
      while (*ap && a>=*ap)
	ap = &((*ap)->next); 
      a->next = *ap;
      *ap = a;
      /* compact */
      a = arenas;
      while (a)
	{
	  while ((void**)a + a->size == (void**)a->next)
	    {
	      a->size += a->next->size;
	      a->next = a->next->next;
	    }
	  a = a->next;
	}
    }
}

# endif
#endif


/* ---------------------------------------- */
/* DEFAULT MEMORY ALLOCATOR */

#ifndef DLD_ALLOCATE_DEFINED
static void * 
dld_allocate(size_t s, int perm)
{
  return xmalloc(s);
}
#endif

#ifndef DLD_DEALLOCATE_DEFINED
static void
dld_deallocate(void *x)
{
  if (x) free(x);
}
#endif


/* ---------------------------------------- */
/* ALLOCATE AND LOAD DATA */


/* allocate_memory_for_object -- assigns addresses to the various sections */

static void
allocate_memory_for_object(module_entry *ent)
{
    asection *p;
    bfd_vma offset;
    bfd_vma mask;
    bfd *abfd;
    /* Compute offset of all sections */
    offset = 0;
    abfd = ent->abfd;
    for (p=abfd->sections; p; p=p->next)
      if (p->flags & SEC_ALLOC)
        {
	  mask = (1L << p->alignment_power) - 1L;
	  offset = (offset + mask) & ~mask;
	  p->vma = offset;
	  offset += dldbfd_section_size(p);
        }
    /* Allocate memory for all these sections */
    ent->exec = dld_allocate((size_t)offset, 0);
    memset(ent->exec, 0, (size_t)offset);
    /* Update all vma in the sections */
    for (p=abfd->sections; p; p=p->next)
      if (p->flags & SEC_ALLOC)
	p->vma = p->vma + ptrvma(ent->exec);
}


/* load_object_file_data -- loads the object file in core */

static void
load_object_file_data(module_entry *ent)
{
    bfd *abfd;
    asection *p;
    bfd_size_type size;
    int ok;
    
    abfd = ent->abfd;
    for (p=abfd->sections; p; p=p->next)
        if (p->flags & SEC_LOAD)
        {
            ASSERT(p->flags & SEC_ALLOC);
            size = dldbfd_section_rawsize(p);
            ok = bfd_get_section_contents(abfd, p, vmaptr(p->vma), 0, size);
            ASSERT_BFD(ok);
        }
}


/* ---------------------------------------- */
/* UPDATE SYMBOL TABLE */


/* check_multiple_definitions -- check for multiple definition of symbol */

static void
check_multiple_definitions(module_entry *module)
{
    int i;
    asymbol *sym;
    symbol_entry *hsym;
    const char *name;
    for (i=0; i<module->symbol_count; i++)
    {
        sym = module->symbols[i];
        if (sym->flags & BSF_LOCAL)
            continue;
        if (!sym->name || !sym->name[0])
            continue;
        if (sym->flags & (BSF_GLOBAL|BSF_INDIRECT))
        {
            if (sym->flags & BSF_WEAK)
              continue;
#ifdef SEC_LINK_ONCE
            if (sym->section->flags & SEC_LINK_ONCE)
              continue;
#endif
            name = drop_leading_char(module->abfd,sym->name);
            hsym = lookup_symbol(name);
            if (hsym && (hsym->flags & DLDF_DEFD))
              {
                module_entry *defby = hsym->defined_by;
                const char *defby_name = "main program";
                if (defby && !defby->archive_flag)
                  {
                    char *s;
                    defby_name = defby->filename;
                    s = strrchr(defby_name, '/');
                    if (s && s[1])
                      defby_name = s+1;
                  }
                sprintf(error_buffer,"Symbol '%s' already defined by %s", 
                        name, defby_name );
                THROW(error_buffer);
              }
        }
    }
}


/* insert_module_symbols -- insert module symbols into global table */

static void
insert_module_symbols(module_entry *module)
{
    int i;
    asymbol *sym;
    symbol_entry *hsym, *isym;
    const char *name;

    for (i=0; i<module->symbol_count; i++)
    {
        sym = module->symbols[i];
        /* skip local and unnamed symbols */
        if (sym->flags & BSF_LOCAL)
            continue;
        if (!sym->name || !sym->name[0])
            continue;
        if (bfd_is_und_section(sym->section))
        {
            /* process undefined symbol */
            name = drop_leading_char(module->abfd,sym->name);
            hsym = insert_symbol(name);
#ifdef DLOPEN
            /* new symbol may be defined by shared libs */
            if (!(hsym->flags & (DLDF_DEFD|DLDF_REFD)) && global_dlopen_handle)
              {
                void *addr = dlsym(global_dlopen_handle, name);
                if (!addr && sym->name != name)
                  addr = dlsym(global_dlopen_handle, sym->name);
                if (addr)
                  {
                    hsym->flags |= DLDF_DEFD;
                    hsym->definition = ptrvma(addr);
                  }
              }
#endif
            if (! (hsym->flags & DLDF_DEFD))
              if (! (hsym->flags & DLDF_REFD))
                dld_undefined_sym_count += 1;
            hsym->flags |= DLDF_REFD;
            hsym->refcount += 1;
        }
        else if (sym->flags & (BSF_GLOBAL|BSF_WEAK|BSF_INDIRECT))
        {
            name = drop_leading_char(module->abfd,sym->name);
            hsym = insert_symbol(name);
            /* Skip if symbol is already defined */
            if (hsym->flags & DLDF_DEFD)
                continue;
            /* Process defined symbol */
            hsym->defined_by = module;
            if (hsym->flags & DLDF_REFD)
                dld_undefined_sym_count -= 1;                
            
            if (is_bfd_symbol_defined(sym))
            {
                /* Regular BFD symbol */
                hsym->flags |= (DLDF_DEFD|DLDF_BFD);
                hsym->definition = ptrvma(sym);
                if ((sym->flags & BSF_FUNCTION) || 
                    (sym->section->flags & SEC_CODE) )
                    hsym->flags |= DLDF_FUNCTION;
            }
            else
            {
                /* Indirect symbol pointing to an undefined symbol */
                while ((sym->flags & BSF_INDIRECT) 
                       || bfd_is_ind_section(sym->section) )
                    sym = (asymbol*)vmaptr(sym->value);
                if (sym == module->symbols[i] || 
                    !bfd_is_und_section(sym->section) ||
                    !sym->name || !sym->name[0] )
                {
                    sprintf(error_buffer, "Strange error for symbol '%s'", 
                            module->symbols[i]->name );
                    THROW(error_buffer);
                }
                hsym->flags |= (DLDF_DEFD|DLDF_INDIRECT);
                isym = insert_symbol(drop_leading_char(module->abfd,sym->name));
                hsym->definition = ptrvma(isym);
            }
        }
    }
    ASSERT(dld_undefined_sym_count >= 0);
    module->symdone_flag = TRUE;
}


/* remove_module_symbols -- remove module symbols from global table */

static void
remove_module_symbols(module_entry *module)
{
    int i;
    asymbol *sym;
    symbol_entry *hsym;
    const char *name;
    
    for (i=0; i<module->symbol_count; i++)
    {
        sym = module->symbols[i];
        /* skip local and unnamed symbols */
        if (sym->flags & BSF_LOCAL)
            continue;
        if (!sym->name || !sym->name[0])
            continue;
        if (bfd_is_und_section(sym->section))
        {
            /* process undefined_symbol */
            name = drop_leading_char(module->abfd,sym->name);
            hsym = lookup_symbol(name);
            if (hsym)
            {
                ASSERT(hsym->refcount > 0);
                ASSERT(hsym->flags & DLDF_REFD);
                hsym->refcount -= 1;
                if (hsym->refcount == 0) 
                {
                    hsym->flags &= ~DLDF_REFD;
                    if (! (hsym->flags & DLDF_DEFD))
                        dld_undefined_sym_count -= 1;
                }
            }
        }
        else if (sym->flags & (BSF_GLOBAL|BSF_INDIRECT|BSF_WEAK))
        {
            /* process defined_symbol */
            name = drop_leading_char(module->abfd,sym->name);
            hsym = lookup_symbol(name);
            if (hsym && (hsym->flags & DLDF_DEFD))
            {
                if (hsym->defined_by == module)
                {
                    hsym->flags &= ~(DLDF_BFD|DLDF_DEFD|DLDF_FUNCTION|DLDF_INDIRECT);
                    hsym->definition = 0;
                    hsym->defined_by = 0;
                    if (hsym->refcount > 0) 
                        dld_undefined_sym_count += 1;
                    else
                        hsym->flags &= ~DLDF_REFD;
                }
            }
        }
    }
    ASSERT(dld_undefined_sym_count >= 0);
    module->symdone_flag = FALSE;
}



/* ---------------------------------------- */
/* RESOLVE SYMBOLS */

/* resolve_module_symbols -- searches definition of undefined symbol of a module */

static void
resolve_module_symbols(module_entry *module)
{
  int i;
  bfd *abfd;
  asymbol *bsym;
  symbol_entry *hsym;
  
  abfd = module->abfd;
  module->undefined_symbol_count = 0;
  for (i=0; i<module->symbol_count; i++)
    {
      bsym = module->symbols[i];
      if (bsym->flags & BSF_LOCAL)
        continue;
      if (!bsym->name || !bsym->name[0])
        continue;
      if (bfd_is_und_section(bsym->section))
        {
          hsym = lookup_symbol(drop_leading_char(abfd,bsym->name));
          ASSERT(hsym);
          bsym->value = 0;
          if (hsym->flags & DLDF_DEFD)
            {
              bsym->value = ptrvma(hsym);
              if (hsym->defined_by)
                {
                  insert_entry_into_list(&module->useds, hsym->defined_by);
                  insert_entry_into_list(&hsym->defined_by->users, module);
                }
            }
          else
            {
              module->undefined_symbol_count += 1;
            }
        }
    }
}



/* resolve_newly_defined_symbols -- resolve modules yet having undefined entries */

static void
resolve_newly_defined_symbols(module_entry *chain)
{
    while (chain)
    {
        if (chain->archive_flag)
            resolve_newly_defined_symbols(chain->mods);
        else if (chain->undefined_symbol_count > 0)
            resolve_module_symbols(chain);
        chain = chain->next;
    }
}




/* ---------------------------------------- */
/* PERFORM RELOCATION */


/* update_instruction_cache -- update the instruction cache,
   either by clearing the relevant cache lines,
   or by synchronizing with the data cache */

static void
update_instruction_cache(bfd_vma s, bfd_size_type l)
{
  /* ----- POWER PC ----- */
#if defined(__ppc__) || defined(__PPC__)
# ifndef CACHELINESIZE
#  define CACHELINESIZE 16
# endif
  char *start = vmaptr(s & ~(CACHELINESIZE-1));
  char *end = vmaptr((s+l+CACHELINESIZE-1) & ~(CACHELINESIZE-1));
  char *p;  
# ifdef __GNUC__
  for (p = start; p < end; p += CACHELINESIZE)
    { __asm__ __volatile__ ("dcbf 0,%0" : : "r" ((long)p) ); }
  __asm__ __volatile__ ("sync");
# else
#  error "GCC-dependent code here."
# endif
#endif /* __ppc__ */
}



/* apply_relocations -- perform relocation on a fully linked module */

static void
apply_relocations(module_entry *module, int externalp)
{
    /* Flag externalp decides if we apply the internal relocations only
     * (i.e. those depending on symbols defined within this module) or
     * the external relocations only (i.e. those depending on symbols
     * defined by other modules).
     *
     * Internal relocation are applied immeditly after loading.
     * External relocation are applied/undone when the module
     * becomes executable/non-executable.
     */
    
    bfd *abfd;
    asection *p;
    int i;
    arelent *reloc;
    asymbol *bsym;
    symbol_entry *hsym;
    bfd_vma value;
    arelent dummy_reloc;
    asymbol *dummy_symbol_ptr;
    asymbol dummy_symbol;
    char *dummy_string;
    bfd_reloc_status_type status;
    
    abfd = module->abfd;
    /* Iterate over relocatable sections */
    for (p=abfd->sections; p; p=p->next)
      if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC))
        {
            ASSERT(p->vma > 0);
            ASSERT(p->orelocation);
            for (i=0; i<p->reloc_count; i++) 
              {
                value = 0;
                reloc = p->orelocation[i];
                /* A relocation without a symbol is worthless */
                ASSERT(reloc->sym_ptr_ptr);
                bsym = *reloc->sym_ptr_ptr;
                ASSERT(bsym);
                /* Get value of symbol */
                if (externalp)
                {
                    /* external relocations *only* are performed */
                    if (is_bfd_symbol_defined(bsym))
                        continue;
                    hsym = (symbol_entry*)vmaptr(bsym->value);
                    ASSERT(hsym);
                    value = value_of_symbol(hsym);
                }
                else
                {
                    /* internal relocations *only* are performed */
                    if (! is_bfd_symbol_defined(bsym))
                        continue;
                    value = value_of_bfd_symbol(bsym);
		    hsym = 0;
                }
                
                /* Prepare relocation info */
                dummy_reloc = *reloc;
                dummy_reloc.sym_ptr_ptr = &dummy_symbol_ptr;
                dummy_symbol_ptr = &dummy_symbol;
                dummy_symbol.name = bsym->name;
                dummy_symbol.value = value;
                dummy_symbol.flags = BSF_GLOBAL;
                dummy_symbol.section = bfd_abs_section_ptr;
                dummy_symbol.the_bfd = NULL;
                dummy_symbol.udata.p = NULL;
                dummy_symbol.udata.i = 0;
                /* Perform relocation */
                status = bfd_perform_relocation(abfd, &dummy_reloc, 
                                                (bfd_byte*)vmaptr(p->vma), 
                                                p, NULL, &dummy_string );
		/* Create trampoline and retry */
		if (externalp && status == bfd_reloc_overflow)
		  if ((hsym->flags & DLDF_DEFD) && !( hsym->flags & DLDF_ALLOC))
		    {
		      void **stub = 0;
		      bfd_byte *data = vmaptr(p->vma);
		      data += reloc->address;
#ifdef __x86_64__
		      bfd_byte opcode = data[-1];
		      if ((opcode==0xe8) || (opcode==0xe9))
			{
			  stub = dld_allocate(2*sizeof(void*), 1);
			  stub[0] = vmaptr(0x0225ff);
			  stub[1] = vmaptr(value);
			}
                      else
                        {
                          fprintf(stderr,
                                  "dldbfd: x86_64 relocation overflow (%s)\n"
                                  "  This happens when accessing data variables\n"
                                  "  located in a shared object loaded via dlopen\n"
                                  "  Instead of\n"
                                  "     extern int remotevar;\n"
                                  "     if (remotevar = 15) ....\n"
                                  "  You can do:\n"
                                  "     extern int remotevar;\n"
                                  "     static int* volatile premotevar = &remotevar\n"
                                  "     if (*premotevar = 15) ....\n",
                                  dummy_symbol.name );
                        }
#endif
		      if (stub)
			{
			  hsym->definition = ptrvma(stub);
			  hsym->flags |= DLDF_ALLOC;
			  value = value_of_symbol(hsym);
			  dummy_reloc = *reloc;
			  dummy_reloc.sym_ptr_ptr = &dummy_symbol_ptr;
			  dummy_symbol.value = value;
			  bfd_get_section_contents(abfd, p, data, reloc->address, 
						   bfd_get_reloc_size(reloc->howto) );
			  status = bfd_perform_relocation(abfd, &dummy_reloc, 
							  (bfd_byte*)vmaptr(p->vma), 
							  p, NULL, &dummy_string );
			}
		    }
                /* Analyze return code */
                switch (status)
                {
                case bfd_reloc_ok:
                  break;
                case bfd_reloc_dangerous:
                  sprintf(error_buffer,"Dangerous relocation: %s", dummy_string);
                  THROW(error_buffer);
                case bfd_reloc_overflow:
                  THROW("Overflow during relocation");
                case bfd_reloc_notsupported:
                  THROW("Unsupported relocation");                  
                default:
                  THROW("Corrupted relocation table");
                }
              }
            /* Update I-cache */
            update_instruction_cache(p->vma, bfd_section_size(abfd, p));
        }
    /* Mark module as relocated */
    if (externalp)
      module->relocated_flag = TRUE;
}


/* undo_relocations -- restores raw data for external relocations */

static void
undo_relocations(module_entry *module)
{
    bfd *abfd;
    asection *p;
    int i;
    arelent *reloc;
    asymbol *bsym;
    
    abfd = module->abfd;
    /* Iterate over relocatable sections */
    for (p=abfd->sections; p; p=p->next)
    {
      if ((p->flags & SEC_ALLOC) && (p->flags & SEC_RELOC))
        {
          ASSERT(p->vma > 0);
            ASSERT(p->orelocation);
            for (i=0; i<p->reloc_count; i++) 
              {
                /* Check that relocation is external */
                reloc = p->orelocation[i];
                ASSERT(reloc->sym_ptr_ptr);
                bsym = *reloc->sym_ptr_ptr;
                ASSERT(bsym);
                if (is_bfd_symbol_defined(bsym))
                  continue;
                /* Clear */
                memset(vmaptr(p->vma + reloc->address), 0,
                       bfd_get_reloc_size(reloc->howto) );
                /* Attempt to load initial data */
                bfd_get_section_contents(abfd, p,
                                         vmaptr(p->vma + reloc->address), 
                                         reloc->address, 
                                         bfd_get_reloc_size(reloc->howto) );
              }
        }
    }
    /* Done */
    module->relocated_flag = FALSE;
}



/* perform_all_relocations -- relocate everything which needs relocation */

static void
perform_all_relocations(module_entry *chain)
{
    while (chain)
    {
        if (chain->archive_flag)
            perform_all_relocations(chain->mods);
        else if (chain->undefined_symbol_count==0 && !chain->relocated_flag)
            apply_relocations(chain, TRUE);
        else if (chain->undefined_symbol_count>0 && chain->relocated_flag)
            undo_relocations(chain);
        chain = chain->next;
    }
}


/* ---------------------------------------- */
/* CHECK EXECUTABILITY */



/* clear_all_executable_flags -- sets executable flag to -1 on a chain of modules */

static void 
clear_all_executable_flags(module_entry *chain)
{
    while (chain)
    {
        if (chain->archive_flag)
            clear_all_executable_flags(chain->mods);
        chain->executable_flag = -1; 
        chain = chain->next;
    }
}


/* compute_executable_flag -- computes executable flag for a module */

static int
compute_executable_flag(module_entry *module)
{
    module_list *p;
    if (module->executable_flag >= 0)
        return module->executable_flag;        
    /* Check undefined symbols */
    module->executable_flag = 0;
    if (module->undefined_symbol_count > 0)
        goto exit;
    /* Reproduce DLD bugs */
    module->executable_flag = 1;
    if (dld_compatibility_flag)
        goto exit;
    /* Check referenced modules */
    for (p=module->useds; p; p=p->next)
        if (! compute_executable_flag(p->entry))
        {
            module->executable_flag = 0;
            break;
        }
    
    /* Setup memory protection */
 exit:
#if HAVE_MPROTECT
    if (module->executable_flag > 0)
      {
        asection *p;
        bfd *abfd = module->abfd;
        int pagesize = sysconf(_SC_PAGESIZE);
        /* Call mprotect */
        for (p=abfd->sections; p; p=p->next)
          if ((p->flags & SEC_LOAD) && (p->flags & SEC_CODE))
            {
              int status;
              bfd_vma start = p->vma;
              bfd_vma end = start + dldbfd_section_rawsize(p);
              /* Assume PAGESIZE is a power of two */
              start = (start) & ~(pagesize-1);
              end = (end + pagesize - 1) & ~(pagesize-1);
              status = mprotect(vmaptr(start), (size_t)end-start, 
                                PROT_READ|PROT_WRITE|PROT_EXEC);
#ifdef DEBUG
              printf("[%x,%x[ protected with status %d -- {%s:%s}\n", 
                     start, end, status, module->filename, p->name );
#endif
            }
      }
#endif
    /* Return */
    return module->executable_flag;
}



/* compute_all_executable_flags -- computes executable flag for a chain of module */

static void 
compute_all_executable_flags(module_entry *chain)
{
    while (chain)
    {
        if (chain->archive_flag)
            compute_all_executable_flags(chain->mods);
        else
            compute_executable_flag(chain);
        chain = chain->next;
    }
}


/* ---------------------------------------- */
/* UNLOADING */


/* remove_module_entry -- unlink and free a module entry */

static void
remove_module_entry(module_entry *module, module_entry **chain)
{
    module_list *p;
    int symdone;

    /* Process archive modules */
    if (module->archive_flag)
        while (module->mods)
            remove_module_entry(module->mods, &module->mods);

    /* Process this module */
    symdone = module->symdone_flag;
    if (symdone)
    {
        /* Update symbol table */
        remove_module_symbols(module);
        /* Cleanup module lists in other modules */
        for (p=module->users; p; p=p->next) 
            remove_entry_from_list( &p->entry->useds, module);
        for (p=module->useds; p; p=p->next) 
            remove_entry_from_list( &p->entry->users, module);            
        /* Recompute symbol resolution of modules using this module */
        for (p=module->users; p; p=p->next) 
            resolve_module_symbols(p->entry);
        /* Clear module lists */
        while ((p = module->useds)) 
            remove_entry_from_list( &module->useds, p->entry);
        while ((p=module->users)) 
            remove_entry_from_list( &module->users, p->entry);
    }
    /* Check module lists */
    ASSERT(module->useds==0);
    ASSERT(module->users==0);
    /* Remove module from module chain */
    while ((*chain) && (*chain!=module))
        chain=&((*chain)->next);
    ASSERT(*chain == module);\
    *chain = module->next;
    /* Adjust relocation */
    if (symdone)
    {
        perform_all_relocations(dld_modules);
        /* Adjust executability */
        clear_all_executable_flags(dld_modules);
        compute_all_executable_flags(dld_modules);        
    }
    /* Erase bfd and free memory */
    dld_deallocate(module->exec);
    module->exec = 0;
    bfd_close(module->abfd);
}



/* ---------------------------------------- */
/* LOADING ARCHIVE MEMBERS */



/* archive_entry -- archive symbol hash table */

typedef struct archive_entry {
    struct bfd_hash_entry root;
    struct archive_chain *first;
} archive_entry;


/* archive_chain -- Chain of entries associated to each symbol in the archive map */

typedef struct archive_chain {
    struct archive_chain *next;
    symindex indx;
    bfd *eltbfd;
} archive_chain;



/* init_archive_entry -- initialize an archive symbol cache entry */

static struct bfd_hash_entry *
init_archive_entry(struct bfd_hash_entry *entry, 
                   struct bfd_hash_table *table,
                   const char *string)
{
    if (!entry)
        entry = bfd_hash_allocate(table, sizeof(archive_entry));
    if (!entry)
        THROW("out of memory");
    memset(entry, 0, sizeof(archive_entry));
    return bfd_hash_newfunc(entry, table, string);
}



/* archive_cookie -- structure used for symbol table traversal */

typedef struct chain_of_bfd {
    struct chain_of_bfd *next;
    bfd *abfd;
} chain_of_bfd;

typedef struct archive_cookie {
    struct bfd_hash_table *armap;
    struct chain_of_bfd *aropen;
    module_entry *armod;
    int again;
} archive_cookie;


/* bfdchain_add -- Add function to chain of open bfd */

static int
bfdchain_add(archive_cookie *cookie, bfd *element)
{
    chain_of_bfd **bfdchn;
    for (bfdchn=&(cookie->aropen); *bfdchn; bfdchn=&((*bfdchn)->next))
        if ((*bfdchn)->abfd == element)
            return FALSE;
    *bfdchn = bfd_hash_allocate(cookie->armap, sizeof(chain_of_bfd));
    if (! *bfdchn)
        THROW(bfd_errmsg(bfd_error_no_memory));
    (*bfdchn)->abfd = element;
    (*bfdchn)->next = 0;
    return TRUE;
}


/* arlink_traverse -- Traversal function for linking archives */

static int
arlink_traverse(struct bfd_hash_entry *gsym, void *gcookie)
{
    symbol_entry *hsym = (symbol_entry*)gsym;
    archive_cookie *cookie = gcookie;
    archive_entry *asym;
    archive_chain *achn;
    module_entry *armodule;
    bfd *element;

    /* Continue if symbol is defined or unreferenced */
    if (! (hsym->flags & DLDF_REFD))
        return TRUE;
    if (hsym->flags & DLDF_DEFD)
        return TRUE;

    /* Continue if symbol is not in archive map */
    asym = (archive_entry*)
        bfd_hash_lookup(cookie->armap, hsym->root.string, FALSE, FALSE);
    if (!asym)
        return TRUE;
    
    /* Search chain */
    for (achn=asym->first; achn; achn=achn->next)
    {
        /* Get archive element and add it into the open bfd chain */
        if (! (element = achn->eltbfd))
        {
            element = bfd_get_elt_at_index(cookie->armod->abfd, achn->indx);
            bfdchain_add(cookie, element);
        }
        /* Slot archive_pass contains -1 if this element has been linked */
        if (element->archive_pass >= 0
            && bfd_check_format(element, bfd_object) )
        {
            /* Include element in archive module chain */
            armodule = create_module_entry(element, &cookie->armod->mods, FALSE);
            armodule->filename = xballoc(element, strlen(element->filename)
                                         + strlen(cookie->armod->filename) + 3);
            sprintf((char*)armodule->filename, "%s(%s)", 
                    cookie->armod->filename, element->filename );
            ASSERT(!armodule->archive_flag);
            element->archive_pass = -1;
            /* Update global symbol table */
	    perform_rewrites(armodule);
            check_multiple_definitions(armodule);
            allocate_memory_for_object(armodule);
            load_object_file_data(armodule);
            apply_relocations(armodule,FALSE);
            insert_module_symbols(armodule);
            /* Loop again */
            cookie->again = TRUE;
            break;
        }
    }
    /* Stop traversal if there are no more undefined symbols */
    return (dld_undefined_sym_count > 0);
}



/* link_archive_members -- link needed modules from an archive */

static void
link_archive_members(module_entry *module)
{
    bfd *dbfd = 0;
    bfd *abfd = module->abfd;
    struct bfd_hash_table archive_map;
    const char *name;
    archive_cookie cookie;
    archive_entry *are;
    archive_chain *arc;
    archive_chain **arcpp;
    symindex indx;
    carsym *asym;
    chain_of_bfd *chain;

    /* Initialize archive symbol table */
    if (! my_bfd_hash_table_init(&archive_map, init_archive_entry, sizeof(archive_entry)))
        THROW(bfd_errmsg(bfd_error_no_memory));        
    cookie.armap = &archive_map;
    cookie.armod = module;
    cookie.aropen = 0;
    
    TRY
    {
        /* Check if archive has a symbol table */
        if (bfd_has_map(abfd))
        {
            /* Fill hash table using archive symbol table */
            for(indx = bfd_get_next_mapent(abfd, BFD_NO_MORE_SYMBOLS, &asym);
                indx != BFD_NO_MORE_SYMBOLS; 
                indx = bfd_get_next_mapent(abfd, indx, &asym) )
            {
                /* First object file will define leading character */
                if (!dbfd)
                {
                    dbfd = bfd_get_elt_at_index(abfd, indx);
                    ASSERT(dbfd);
                    bfdchain_add(&cookie, dbfd);
                }
                /* Remove leading character */
                name = drop_leading_char(dbfd, asym->name);
                /* Insert symbol name into hash table */
                are = (archive_entry*)
                    bfd_hash_lookup(&archive_map, name, TRUE, FALSE );
                arc = (archive_chain*)
                    bfd_hash_allocate(&archive_map, sizeof(archive_chain) );
                if (!arc)
                    THROW(bfd_errmsg(bfd_error_no_memory));
                /* Add new component at the end of the chain */
                arc->eltbfd = 0;
                arc->indx = indx;
                arc->next = 0;
                arcpp = &are->first;
                while (*arcpp)
                    arcpp = &((*arcpp)->next);
                *arcpp = arc;
            }
        }
        else
        {
            /* Fill hash table by looking all components */
            while ((dbfd = bfd_openr_next_archived_file(abfd, dbfd)))
            {
                if (bfd_check_format(dbfd, bfd_object))
                {
                    void *minptr;
                    int  i;
                    unsigned int minsize;
                    long symcount;
                    asymbol *nsym = bfd_make_empty_symbol(dbfd);
                    /* Register open bfd */
                    bfdchain_add(&cookie, dbfd);
                    /* Read minisymbols */
                    symcount = bfd_read_minisymbols(dbfd, FALSE, &minptr, &minsize);
                    for (i=0; i<symcount; i++)
                    {
                        asymbol *sym;
                        sym = bfd_minisymbol_to_symbol(dbfd, FALSE, minptr, nsym);
                        if (sym->flags & (BSF_GLOBAL|BSF_WEAK|BSF_INDIRECT))
                        {
                            /* Remove leading character */
                            name = drop_leading_char(dbfd, sym->name);
                            /* Insert symbol name into hash table */
                            are = (archive_entry*)
                                bfd_hash_lookup(&archive_map, name, TRUE, FALSE );
                            arc = (archive_chain*)
                                bfd_hash_allocate(&archive_map, sizeof(archive_chain));
                            if (!arc)
                                THROW(bfd_errmsg(bfd_error_no_memory));
                            /* Add new component at the end of the chain */
                            arc->eltbfd = dbfd;
                            arc->indx = 0;
                            arc->next = 0;
                            arcpp = &are->first;
                            while (*arcpp)
                                arcpp = &((*arcpp)->next);
                            *arcpp = arc;
                        }
                        minptr = (void*)((char*)minptr + minsize);
                    }
                }
                else
                {
                    bfd_close(dbfd);
                }
            }
        }
        /* Traverse global symbol table for undefined symbols */
        cookie.again = TRUE;
        while (cookie.again && dld_undefined_sym_count>0)
        {
            cookie.again = FALSE;
            bfd_hash_traverse(&global_symbol_table, arlink_traverse, &cookie);
        }
    }
    CATCH(n)
    {
        /* Close all unused bfds if an error occurs */
        for (chain=cookie.aropen; chain; chain=chain->next)
            if (chain->abfd->archive_pass >= 0)
                bfd_close(chain->abfd);
        /* Free archive map if an error occurs */
        bfd_hash_table_free(&archive_map);
        /* Propagate exception */
        THROW(n);
    }
    END_CATCH;

    /* Close all unused bfds if an error occurs */
    for (chain=cookie.aropen; chain; chain=chain->next)
        if (chain->abfd->archive_pass >= 0)
            bfd_close(chain->abfd);
    /* Free archive map */
    bfd_hash_table_free(&archive_map);
}





/* ---------------------------------------- */
/* DLD_LINK */

/* dld_link -- reads and links a module */

int 
dld_link (const char *oname)
{ 
    module_entry *module = 0;

    /* Check that module has not been loaded */
    for (module=dld_modules; module; module=module->next)
      if (! strcmp(module->filename, oname))
        {
          dld_errno = "Module is already loaded";
          return -1;
        }
    
    /* Create file entry */
    TRY
    {
        module = create_file_entry(oname);
        if (module->archive_flag)
        {
            if (dld_undefined_sym_count > 0)
                link_archive_members(module);
        }
        else
        {
	    perform_rewrites(module);
            check_multiple_definitions(module);
            allocate_memory_for_object(module);
            load_object_file_data(module);
            apply_relocations(module,FALSE);
            insert_module_symbols(module);
        }
        resolve_newly_defined_symbols(dld_modules);
        perform_all_relocations(dld_modules);
        clear_all_executable_flags(dld_modules);
        compute_all_executable_flags(dld_modules);        
    }
    CATCH(n)
    {
        if (module)
            remove_module_entry(module, &dld_modules);
        dld_errno = n;
        return -1;
    }
    END_CATCH;
    return 0;
}


/* ---------------------------------------- */
/* DLD_INIT */



/* define_symbol_of_main_program -- insert the symbol of the main program */

static void
define_symbol_of_main_program(const char *exec)
{
    bfd *abfd = 0;
    asymbol **symbols;
    long storage_needed;
    int symbol_count;
    asection *p;
    int i;
    asymbol *sym;
    symbol_entry *hsym;

    /* Read symbols of main program */
    abfd = bfd_openr(exec,"default");
#if defined(__CYGWIN32__) || defined(WIN32)
    if (! abfd)
      {
        char *l = xmalloc(strlen(exec)+8);
        strcpy(l, exec);
        strcat(l, ".exe");
        abfd = bfd_openr(l, "default");
        free(l);
      }
#endif
    TRY
    {
        ASSERT_BFD(abfd);
        if (! bfd_check_format(abfd, bfd_object))
            THROW("Cannot read symbols of main program");
        storage_needed = bfd_get_symtab_upper_bound(abfd);
        ASSERT_BFD(storage_needed>=0);
        if (storage_needed > 0)
          {
            symbols = xballoc(abfd, storage_needed);
            symbol_count = bfd_canonicalize_symtab(abfd, symbols);
          }
        else
          {
            storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
            if (storage_needed <= 0)
              THROW("Cannot read main program symbols");
            symbols = xballoc(abfd, storage_needed);
            symbol_count = bfd_canonicalize_dynamic_symtab(abfd, symbols);
          }
#if DLOPEN
	if (! global_dlopen_handle)
#endif
	  if (symbol_count <= 0)
	    THROW("Cannot read symbols of main program");
        /* Check that object is fully relocated */
        for (p=abfd->sections; p; p=p->next)
            ASSERT(!(p->flags & SEC_RELOC));
        /* Iterate over symbols */
        for (i=0; i<symbol_count; i++)
        {
            sym = symbols[i];
            if (sym->name && sym->name[0] &&
                (sym->flags & (BSF_GLOBAL|BSF_INDIRECT|BSF_WEAK)) )
            {
                /* Process indirect symbols */
                while (bfd_is_ind_section(sym->section) 
                       || (sym->flags & BSF_INDIRECT) )
                    sym = (asymbol*)vmaptr(sym->value);
                /* Continue if symbol is undefined (should not happen) */
                if (!is_bfd_symbol_defined(sym))
                    continue;
                /* Continue if symbol is already defined (cygwin) */
                hsym = insert_symbol(drop_leading_char(abfd,sym->name));
                if (hsym->flags & DLDF_DEFD)
		  continue;
                /* Fill symbol entry in global hash table */
                hsym->flags = DLDF_DEFD;
                hsym->definition = value_of_bfd_symbol(sym);
                if ((sym->flags & BSF_FUNCTION) ||
                    (sym->section->flags & SEC_CODE))
                    hsym->flags |= DLDF_FUNCTION;
            }
#if defined(__GNUC__) && (__GNUC__ >= 3)
	    if (sym->name && sym->name[0] &&
		!strcmp("__dso_handle",drop_leading_char(abfd,sym->name)) 
		/* This is the __dso_handle thing for gcc >= 3 */ )
	      {
	        if (!is_bfd_symbol_defined(sym)) 
		  continue;
                hsym = insert_symbol("__dso_handle");
                if (hsym->flags & DLDF_DEFD) 
		  continue;
		hsym->flags = DLDF_DEFD;
		hsym->definition = value_of_bfd_symbol(sym);
	      }
#endif
        }
        /* Close everything */
        bfd_close(abfd);
    }
    CATCH(n)
    {
        if (abfd)
            bfd_close(abfd);
        THROW(n);
    }
    END_CATCH;
}



/* Some g++ versions need this symbol. */

#if defined(__GNUC__) && (__GNUC__ < 3)
void 
__pure_virtual(void) 
{ 
  extern void lush_error();
  lush_error("Pure virtual function called"); 
}
#endif



/* dld_init -- initializes DLD/BFD */

int 
dld_init (const char *exec) 
{ 
    TRY
    {
        bfd_init();
        bfd_set_error_program_name("*** DLD/BFD");
#if DLOPEN
        /* initialize global dlopen handle */
        global_dlopen_handle = dlopen(0, RTLD_LAZY);
#endif
        /* load symbol table of current program */
        init_global_symbol_table();
        define_symbol_of_main_program(exec);
    }
    CATCH(n)
    {
        dld_errno = n;
        return -1;
    }
    END_CATCH;
    return 0;
}


/* ---------------------------------------- */
/* DLD_DLOPEN */


/* dld_dlopen -- replacement for DLOPEN compatible with DLD */

void *
dld_dlopen(const char *path, int mode)
{
#ifdef DLOPEN
    bfd *abfd = 0;
    dlopen_handle_t *handle = 0;
    asymbol **symbols;
    long storage_needed;
    int symbol_count;
    asection *p;
    int i;
    asymbol *sym;
    symbol_entry *hsym;
    char *err;
    
    /* Obtain BFD for the library */
    abfd = bfd_openr(path,"default");
    /* Perform DLOPEN */
    handle = dlopen(path, mode);
    err = (char*)dlerror();
    TRY
    {
        /* Check BFD and HANDLE */
        ASSERT_BFD(abfd);
        if (! bfd_check_format(abfd, bfd_object))
            THROW("Cannot read symbols of shared library");
        if (!handle)
          {
            if (err)
              THROW(err);
            else
              THROW("Unknown error in DLOPEN");
          }
        /* Read symbols */
        storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
        ASSERT_BFD(storage_needed>=0);
        if (storage_needed==0)
            THROW("Cannot load dynamic symbols");
        symbols = xballoc(abfd, storage_needed);
        symbol_count = bfd_canonicalize_dynamic_symtab(abfd, symbols);
        ASSERT_BFD(symbol_count>0);
        /* Check that object is fully relocated */
        for (p=abfd->sections; p; p=p->next)
            ASSERT(!(p->flags & SEC_RELOC));
        /* Iterate over symbols */
        for (i=0; i<symbol_count; i++)
	  {
	    sym = symbols[i];
            if (! (sym->name && sym->name[0]))
              continue;
            if (! (sym->flags & (BSF_LOCAL|BSF_SECTION_SYM|
                                 BSF_DEBUGGING|BSF_WARNING)))
              {
                const char *name = drop_leading_char(abfd,sym->name);
                void *addr = 0;
		if (global_dlopen_handle)
		  addr = dlsym(global_dlopen_handle, name);
		if (global_dlopen_handle && !addr)
		  addr = dlsym(global_dlopen_handle, sym->name);
		if (!addr)
		  addr = dlsym(handle, name);
                if (!addr && name!=sym->name)
                  addr = dlsym(handle, sym->name);
                if (addr)
                  {
                    /* Insert entry into global hash table */
                    hsym = insert_symbol(name);
                    if (! (hsym->flags & DLDF_DEFD))
                      {
                        if (hsym->flags & DLDF_REFD)
                          dld_undefined_sym_count -= 1;
                        hsym->flags &= DLDF_REFD;
                        hsym->flags |= DLDF_DEFD;
                        hsym->definition = ptrvma(addr);
                        if ((sym->flags & BSF_FUNCTION)
                            || (sym->section->flags & SEC_CODE) )
                          hsym->flags |= DLDF_FUNCTION;
                      }
                  }
              }
        }
        /* Close everything */
        bfd_close(abfd);
        abfd = 0;
        /* Resolve everything */
        resolve_newly_defined_symbols(dld_modules);
        perform_all_relocations(dld_modules);
        clear_all_executable_flags(dld_modules);
        compute_all_executable_flags(dld_modules);        
        return handle;
    }
    CATCH(n)
    {
        if (abfd)
            bfd_close(abfd);
        if (handle)
            dlclose(handle);
        dld_errno = n;
        return NULL;
    }
    END_CATCH;
#else
    dld_errno = "DLOPEN is not supported on this platform";
    return NULL;
#endif
}




/* ---------------------------------------- */
/* FIND EXECUTABLE */


/* dld_find_executable -- searches executable thru exec path */

char *
dld_find_executable (const char *file)
{
    char *search;
    struct stat st;
    char *p;
    
    if (strchr(file, '/'))
      return strdup(file);
    if (((search = (char *) getenv("DLDPATH")) == 0) &&
	((search = (char *) getenv("PATH")) == 0))
      search = ".";
    p = search;
    while (*p) 
      {
	char  name[MAXPATHLEN];
	register char *next;
	next = name;
	while (*p && *p != ':') *next++ = *p++;
	*next = 0;
	if (*p) p++;
	if (name[0] == '.' && name[1] == 0) {
          errno = 0;
          if (!getcwd (name, sizeof(name))) {
            fprintf(stderr, "dld_find_executable: getcwd failed (%s)\n", strerror(errno));
            abort();
          }
        }

	strcat (name, "/");
	strcat (name, file);
	if (access (name, X_OK) == 0) 
          if (stat(name, &st) == 0) 
            if (S_ISREG(st.st_mode))
              return strdup(name);
      }
    return 0;
}



/* ---------------------------------------- */
/* LIST UNDEFINED SYMBOLS */


/* structure passed when traversing */

struct list_undefined_cookie {
    const char **syms;
    int current;
    int total;
};

/* function called when traversing */

static int
list_undefined_traverse(struct bfd_hash_entry *gsym, void *gcookie)
{
    symbol_entry *hsym = (symbol_entry*)gsym;
    struct list_undefined_cookie *cookie = gcookie;
    if (hsym && (hsym->flags & DLDF_REFD) && !(hsym->flags & DLDF_DEFD))
    {
        int n = cookie->current;
        if (n < cookie->total) 
        {
            cookie->syms[n] = hsym->root.string;
            if (hsym->root.string[0] == '\007')
                cookie->syms[n] += 1;
        }
        cookie->current = n+1;
    }
    return TRUE;
}


/* return a list of undefined symbols */

const char **
dld_list_undefined_sym (void)
{
    struct list_undefined_cookie cookie;
    TRY 
    {
        cookie.syms = xmalloc( (dld_undefined_sym_count+1)*sizeof(char*) );
        cookie.current = 0;
        cookie.total = dld_undefined_sym_count;
        bfd_hash_traverse(&global_symbol_table, list_undefined_traverse, &cookie);
        ASSERT(cookie.current = cookie.total);
        cookie.syms[cookie.current] = 0;
    }
    CATCH(n)
    {
        dld_errno = n;
        return 0;
    }
    END_CATCH;
    return cookie.syms;
}




/* ---------------------------------------- */
/* ACCESS TO SYMBOL TABLE */


/* dld_get_symbol -- obtain address of data symbol */

void *
dld_get_symbol (const char *id)
{
    symbol_entry *hsym;
    void *ret = 0;
    TRY
      {
        hsym = lookup_symbol(id);
        if (hsym && (hsym->flags & DLDF_DEFD))
          {
            while (hsym->flags & DLDF_INDIRECT)
              hsym = (symbol_entry*)vmaptr(hsym->definition);
            if ( !(hsym->flags & DLDF_FUNCTION))
              ret = vmaptr(value_of_symbol(hsym));
          }
      }
    CATCH(n)
      {
        dld_errno = n;
        return 0;
      }
    END_CATCH;
    return ret;
}


/* dld_get_func -- obtain address of text symbol */

void *
dld_get_func (const char *id)
{
    symbol_entry *hsym;
    void *ret = 0;
    TRY
    {
        hsym = lookup_symbol(id);
        if (hsym && (hsym->flags & DLDF_DEFD))
        {
            while (hsym->flags & DLDF_INDIRECT)
                hsym = (symbol_entry*)vmaptr(hsym->definition);
            if (hsym->flags & DLDF_FUNCTION)
                ret = vmaptr(value_of_symbol(hsym));
        }
    }
    CATCH(n)
    {
        dld_errno = n;
        return 0;
    }
    END_CATCH;
    return ret;
}


/* dld_get_bare_symbol -- obtain address of symbol without leading character (_) */

void *
dld_get_bare_symbol (const char *id)
{
    symbol_entry *hsym;
    void *ret = 0;
    TRY
    {
        hsym = lookup_symbol(drop_leading_char(NULL,id));
        if (hsym && (hsym->flags & DLDF_DEFD))
            ret = vmaptr(value_of_symbol(hsym));
    }
    CATCH(n)
    {
        dld_errno = n;
        return 0;
    }
    END_CATCH;
    return ret;
}



/* ---------------------------------------- */
/* MODULE UNLOAD */


/* dld_unlink_by_file -- unload a module by filename */

int 
dld_unlink_by_file (const char *oname, int hard)
{
    module_entry *module, *sub;
    TRY
    {
        /* Searches module with that name */
        for (module=dld_modules; module; module=module->next)
            if (! strcmp(module->filename, oname))
                break;
        if (!module)
            THROW("Module not found");
        /* Check if module is used */
        if (hard==0)
        {
            if (!module->users)
            {
                hard = 1;
                if (module->archive_flag) 
                    for (sub=module->mods; sub; sub=sub->next)
                        if (sub->users)
                            hard = 0;
            }
        }
        /* Actually unloads module */
        if (hard)
            remove_module_entry(module, &dld_modules);
    }
    CATCH(n)
    {
        dld_errno = n;
        return -1;
    }
    END_CATCH;
    return 0;
}


/* dld_unlink_by_symbol -- unload the module defining a given symbol */

int 
dld_unlink_by_symbol (const char *id, int hard)
{
    module_entry *module, *sub;
    symbol_entry *hsym;
    TRY
      {
        /* Look for symbol */
        hsym = lookup_symbol(id);
        if (!hsym || !(hsym->flags & DLDF_DEFD))
          THROW("Undefined symbol");
        module = hsym->defined_by;
        if (! module)
          THROW("Symbol does not belong to a dynamically loaded module");
        /* Check if module is used */
        if (hard==0)
          {
            if (!module->users)
              {
                hard = 1;
                if (module->archive_flag) 
                  for (sub=module->mods; sub; sub=sub->next)
                    if (sub->users)
                      hard = 0;
              }
          }
        /* Actually unloads module */
        if (hard)
          remove_module_entry(module, &dld_modules);
      }
    CATCH(n)
      {
        dld_errno = n;
        return -1;
      }
    END_CATCH;
    return 0;
}



/* ---------------------------------------- */
/* CHECK EXECUTABILITY */


int 
dld_simulate_unlink_by_file(const char *oname)
{
  TRY
    {
      module_entry *module=0;
      if (oname)
        for (module=dld_modules; module; module=module->next)
          if (! strcmp(module->filename, oname))
            break;
      if (oname && !module)
        THROW("Module not found");
      clear_all_executable_flags(dld_modules);
      if (module) {
        module_entry *sub;
        module->executable_flag = 0;
        if (module->archive_flag)
          for (sub = module->mods; sub; sub = sub->next)
            module->executable_flag = 0;
      }
      compute_all_executable_flags(dld_modules);        
    }
  CATCH(n)
    {
      dld_errno = n;
      return -1;
    }
  END_CATCH;
  return 0;
}


int
dld_function_executable_p (const char *id)
{
    int ret;
    symbol_entry *hsym;
    module_entry *module;
    
    TRY
    {
        ret = 0;
        hsym = lookup_symbol(id);
        if (!hsym || !(hsym->flags & DLDF_DEFD))
            THROW("Undefined symbol");
        while (hsym->flags & DLDF_INDIRECT)
            hsym = (symbol_entry*)vmaptr(hsym->definition);
        if (!(hsym->flags & DLDF_FUNCTION))
            THROW("Symbol does not point to a function");
        ret = 1;
        if (hsym->flags & DLDF_BFD)
        {
            module = hsym->defined_by;
            ASSERT(module);
            ret = (module->executable_flag > 0);
        }
    }
    CATCH(n)
    {
        dld_errno = n;
        return 0;
    }
    END_CATCH;
    return ret;
}



/* ---------------------------------------- */
/* UNTESTED IMPLEMENTATION OF DLD3 FEATURES */


/* dld_create_reference -- fake an undefined symbol to force library loads */

int
dld_create_reference (const char *id)
{
    symbol_entry *hsym;
    TRY
    {
        hsym = insert_symbol(id);
        if (! (hsym->flags & DLDF_DEFD))
            if (! (hsym->flags & DLDF_REFD))
                dld_undefined_sym_count += 1;
        hsym->flags |= DLDF_REFD;
        /* The fake reference does not count in REFCOUNT.
         * Flag REFD is set but REFCOUNT may be zero.
         * When unloading a file defining this symbol, this condition is checked 
         * and flags REFD is cleared, therefore implementing DLD semantics.
         */
    }
    CATCH(n)
    {
        dld_errno = n;
        return -1;
    }
    END_CATCH;
    return 0;
}


/* dld_define_sym -- define a symbol pointing to private memory */

int 
dld_define_sym (const char *id, size_t size)
{
    symbol_entry *hsym;
    TRY
    {
        hsym = insert_symbol(id);
        if (hsym->flags & DLDF_DEFD)
            THROW("Cannot redefine an already defined symbol");
        hsym->definition = ptrvma(dld_allocate(size, 1));
        hsym->flags |= (DLDF_DEFD|DLDF_ALLOC);
        /* Adjust undefined symbol count if symbol was undefined */
        if (hsym->flags & DLDF_REFD)
            dld_undefined_sym_count -= 1;            
        /* Relink if symbol was actually referenced */
        if (hsym->refcount > 0)
        {
            resolve_newly_defined_symbols(dld_modules);
            perform_all_relocations(dld_modules);
            clear_all_executable_flags(dld_modules);
            compute_all_executable_flags(dld_modules);        
        }
    }
    CATCH(n)
    {
        dld_errno = n;
        return -1;
    }
    END_CATCH;
    return 0;
}


/* dld_remove_defined_symbol -- removes a symbol definition */

void
dld_remove_defined_symbol (const char *id)
{
    symbol_entry *hsym;
    TRY
    {
        hsym = lookup_symbol(id);
        if (hsym && (hsym->flags & DLDF_DEFD))
        {
            if (hsym->flags & DLDF_ALLOC)
                dld_deallocate(vmaptr(hsym->definition));
            hsym->flags &= ~(DLDF_FUNCTION|DLDF_ALLOC|DLDF_INDIRECT);
            hsym->flags &= ~(DLDF_BFD|DLDF_INDIRECT|DLDF_DEFD);
            hsym->definition = 0;
            hsym->defined_by = 0;
            if (hsym->refcount > 0) 
	      dld_undefined_sym_count += 1;
            else
	      hsym->flags &= ~DLDF_REFD;
        }
    }
    CATCH(n)
    {
        dld_errno = n;
    }
    END_CATCH;
}




/* -------------- */
/* DEBUGGING HELP */

/* dld_print_gdb_commands -- prints gdb commands to load symbol tables */

void 
dld_print_gdb_commands(int detail)
{
  module_entry *module=0;
  for (module = dld_modules; module; module=module->next)
    {
	bfd *abfd = module->abfd;
	if (bfd_check_format(abfd, bfd_object))
	  {
	    asection *p;
	    for (p=abfd->sections; p; p=p->next)
	      if (p->flags & SEC_CODE)
		if (p->flags & SEC_ALLOC)
		  break;
	    if (! p)
	      continue;
	    printf("\tadd-symbol-file %s %p", module->filename, vmaptr(p->vma));
	    if (detail)
	      for (p=abfd->sections; p; p=p->next)
		if (p->flags & SEC_ALLOC)
		  printf(" -s %s %p", p->name, vmaptr(p->vma));
	    printf("\n");
	  } 
    }
}
