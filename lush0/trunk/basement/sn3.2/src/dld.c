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


#include <string.h>
#include "header.h"
#include "dh.h"


at *dynlink_gone(p)
at *p;
{
    error(NIL,"this function has been unlinked",NIL);
}

at *dynlink_partial(p)
at *p;
{
    error(NIL,"this function belongs to a partially linked module",NIL);
}


#ifdef DLD

/* We now rely on the BFD implementation of DLD */
#include "dldbfd.h"



/********* Modules ********************/

/* Basic array lengths */

#define FULLNAMELENGTH FILELEN

/* States of a module */

#define EXEC_NOINIT          0	/* No initialization routine */
#define EXEC_NEVER           1	/* Never initialized (still partially linked) */
#define EXEC_OK              2	/* Initialized and fully linked */
#define EXEC_NOLONGER        3	/* Initialized, but no longer fully linked */


/* Each files that are loaded has a dynlink_module.  It contains the name,
   the init function, and a pointer to all the C objects it contains. */

struct dynlink_module {
    char filename[FULLNAMELENGTH];
    char initname[NAMELENGTH];
    void (*initfunc)();
    int   exec_state;
    short major_version;
    short minor_version;
    struct dynlink_object *c_objects;
    struct dynlink_module *next;
};

/* Each function/class visible from the lisp has a dynlink object associated
   to it.  The dynlink_ojbects form a list inside each dynlink_module. */

struct dynlink_object {
    c_object_type c_object;
    short check_depend_p;
    short class_p;
    struct dynlink_object *next;
    char name[NAMELENGTH];
    at *(*address) ();
};


static struct alloc_root dynmodule_alloc = { 
    NULL,
    NULL,
    sizeof(struct dynlink_module),
    5,
};


static struct alloc_root dynfunction_alloc = { 
    NULL,
    NULL,
    sizeof(struct dynlink_object),
    40,
};


/********* Globals ***********************/


static int dynlink_initialized = 0;
static struct dynlink_module *modules = NULL;
static struct dynlink_module *module_being_inited;
static char error_module_name[FULLNAMELENGTH];

int doing_dynlink = 0;



/********* A debugging tool... ***********************/

static void 
dynlink_object_print(f)
struct dynlink_object *f;
{
    printf("\t***** dynlink_object: ");
    printf("->address = %lx, ", (unsigned long)f->c_object.address);
    printf("->kind = ");
    switch(f->c_object.kind) {
    case COBJECT_STANDARD:
	printf("STANDARD");
	break;
    case COBJECT_DYNUNLINKED:
	printf("DYNUNLINKED");
	break;
    case COBJECT_DYNLINKED:
	printf("DYNLINKED");
	break;
    case COBJECT_DYNUNREFRED:
	printf("DYNUNREFRED");
	break;
    }
    printf("\n");
    printf("\tdepend = %lx, ", (unsigned long)f->check_depend_p);
    printf("next = %lx, ", (unsigned long)f->next);
    printf("at = %lx, ", (unsigned long)f->address);
    printf("name = %s\n", f->name);
}

static void dynlink_module_print(dm)
struct dynlink_module *dm;
{
    struct dynlink_object *f;
    printf("filename = %s, state = ", dm->filename);
    switch(dm->exec_state) {
    case EXEC_NOINIT:
	printf("NOINIT,  ");
	break;
    case EXEC_NEVER:
	printf("NEVER,   ");
	break;
    case EXEC_OK:
	printf("OK,      ");
	break;
    case EXEC_NOLONGER:
	printf("NOLONGER,");
	break;
    }
    printf("\n    init = %s (%lx), ",
           dm->initname, (unsigned long)dm->initfunc);
    printf("module = %lx, object = %lx\n", 
           (unsigned long)dm->next, (unsigned long)dm->c_objects);
    for(f = dm->c_objects; f; f = f->next)
	dynlink_object_print(f);
}

DX(xmod_status)
{
    struct dynlink_module *dm;
    
    ARG_NUMBER(0);
    for(dm = modules; dm; dm = dm->next)
	dynlink_module_print(dm);    
    return NIL;
}

/******** DLD utilities *******************/


static void 
dynlink_error(p)
at *p;
{
    dld_perror("*** DLD/BFD");
    error(NIL,"dynamic linker error",p);
}


/* The above two functions work for dynamically linked dx & dy,
 * but not for dh (cfunc.address is a kname not a func)
 */
void 
dld_check_dh(add)
void* add;
{
    if (add == dynlink_partial || add == dynlink_gone)
        ((at*(*)())(add)) ();
}

/******** Initialize DLD *****************/


static void 
dynlink_init(flag)
int flag;
{
    if (!dynlink_initialized) {
        extern char *commandname;
        static char *executablename = 0;
        if (!executablename)
        {
#ifdef VERSION
            if (!commandname)
                commandname = VERSION;
#endif
            if (!commandname)
                commandname = "sn3.1";
            executablename = dld_find_executable(commandname);
            if (!executablename)
                error(NIL,"Cannot find executable for command",NIL);
        }
        if (flag)
        {
#ifdef DEBUG
            printf("Initializing DLD, command='%s', exec='%s'\n",
                   commandname, executablename);
#endif
            if (dld_init(executablename))
                dynlink_error(NIL);
            dynlink_initialized = 1;
        }
    }
}





/*** Check executability of the modules ***/


void 
check_executability()
{
    struct dynlink_module *m;
    struct dynlink_object *f;
    int exec_p;
    for (m=modules; m; m=m->next) 
    {
#ifdef DEBUG
        printf("Checking executability of %s\n",m->filename);
#endif
        if (m->exec_state == EXEC_NOINIT)
            continue;
        exec_p = dld_function_executable_p(m->initname);
        /* Update executability state */
        switch (m->exec_state) 
        {
        case EXEC_NEVER:
            if (exec_p && m->initfunc) 
            {
                doing_dynlink = 1;
                module_being_inited = m;
                (m->initfunc)();
                module_being_inited = NULL;
                doing_dynlink = 0;
                m->exec_state = EXEC_OK;
            }
            break;
            
        case EXEC_OK:
            if (!exec_p) 
                m->exec_state = EXEC_NOLONGER;
            break;
            
        case EXEC_NOLONGER:
            if (exec_p) 
                m->exec_state = EXEC_OK;
            break;
            
        default:
            error("dld.c/check_executability","internal error",NIL);
        }
        
        /* Check dependences */
        if (m->exec_state == EXEC_OK)
        {
            for (f=m->c_objects; f; f=f->next) 
            {
                f->c_object.address = f->address;
                if (f->check_depend_p)
                    if (check_dependence(f->address, m->filename, 1)) 
                    {
                        m->exec_state = EXEC_NOLONGER;
                        break;
                    }
            }
            
        }
        
        /* Enforce non executability */
        if (m->exec_state != EXEC_OK)
        {
            for (f=m->c_objects; f; f=f->next) 
            {
                f->c_object.address = dynlink_partial;
                if (f->class_p)
                    lside_dld_partial(f->address);
            }
        }
    }
}






/******** Unload a module *****************/



static void 
dld_cfunc_unlink(f)
struct dynlink_object *f;
{
    int i;
    c_object_type *func = &(f->c_object);
    dhclassconstraint *k = (dhclassconstraint*)(f->address);            
    
    /* Tells Lisp_C that class was unloaded */
    if (f->class_p)
        lside_dld_partial(k);
    
    /* Cleans COBJECT structure */
    switch (func->kind) 
    {
    case COBJECT_DYNLINKED:
        /* tells LISP_C that possible class has been unlinked */
        if (f->class_p)
            for (i=0;i<k->lispdata.nmethods;i++) 
                UNLOCK(k->lispdata.at_methods[i]);
        /* changes function pointer to reflect this change */
        func->address = dynlink_gone;
        func->kind = COBJECT_DYNUNLINKED;
        break;
    case COBJECT_DYNUNREFRED:
        func->address = dynlink_gone;
        deallocate(&dynfunction_alloc,f);
        break;
    default:
        error("dld.c/cfunc_unlink","internal error",NIL);        
        break;
    }
#ifdef DEBUG
    printf("Marking function %s as unlinked\n",f->name);
#endif
}



int 
module_unload(name)
char *name;
{
    struct dynlink_module **mptr, *m;
    struct dynlink_object *f, *nextf;
    int status;
    
    /* initialize DLD */
    dynlink_init(TRUE);
    
    /* find the file */
    name = search_file(name,"o");
    ifn (name)
        error(NIL,"File not found",NIL);
    
    /* find it */
    for (mptr = &modules;  m = *mptr;  mptr = &(m->next)) {
        if (!strcmp(name,m->filename))
            break;
    }
    if (!m)
        return -1;
    
    /* unlink functions */
    for (f=m->c_objects; f; f=nextf) {
        nextf = f->next;
        dld_cfunc_unlink(f);
    }
    
    /* unlink the module */
#ifdef DEBUG
    printf("Unlinking module %s\n",m->filename);
#endif
    status = dld_unlink_by_file(m->filename, 1);
    
    /* deallocate module */
    *mptr = m->next;
    deallocate(&dynmodule_alloc, m);
    
    /* check executability */
    check_executability();
    
    /* check for errors */
    if (status)
#ifdef DLD_EUNDEFSYM
        if (status == DLD_EUNDEFSYM)
            printf("WARNING: Unloading %s, undefined symbol.\n", name);
        else 
#endif
            dynlink_error(new_string(name));
    
    return 0;
}


DX(xmod_unload)
{
    ARG_NUMBER(1);
    ARG_EVAL(1);
    if (module_unload(ASTRING(1)))
        error(NIL,"unknown module",APOINTER(1));
    return NIL;
}



/******** Load a module *******************/

#ifdef VERSION
static void 
get_version_info(m)
struct dynlink_module *m;
{
    char major[NAMELENGTH], minor[NAMELENGTH];
    int *majptr, *minptr;
    
    strcpy(major, "majver_");
    strcpy(minor, "minver_");
    strcat(major, &(m->initname[5]));
    strcat(minor, &(m->initname[5]));
    majptr = (int*) dld_get_symbol(major);
    minptr = (int*) dld_get_symbol(minor);
    
    if (majptr == 0 || minptr == 0)
    {
	m->major_version = -1;
	m->minor_version = -1;
    }
    else
    {
	m->major_version = *majptr;
	m->minor_version = *minptr;
    }
}
#endif /* VERSION */


int 
module_load(name)
char *name;
{
    struct dynlink_module *m;
    char *s1, *s2, *s;
    
    /* initialize DLD */
    dynlink_init(TRUE);
    
    /* attempt to unload a previous version of this module */
    module_unload(name);
    
    /* find the file */
    name = search_file(name,"o");
    ifn (name)
        error(NIL,"File not found",NIL);
    
    /* check the '.o' suffix */
    s2 = strrchr(name,'.');
    ifn (s2 && (s2[1]=='o' || s2[1]=='a') && s2[2]==0)
        error(NIL,"Filename must end with '.o' or '.a'",new_safe_string(name));
    /* allocate & initialize a struct dynlink_module */
    m = allocate(&dynmodule_alloc);
    concat_filename(NULL,name,m->filename,FULLNAMELENGTH);
    m->c_objects = NULL;
    m->next = NULL;
    m->exec_state = EXEC_NOINIT;
    m->initfunc = NULL;
    m->initname[0] = 0;
    
    /* attempt to load the module */
#ifdef DEBUG
    printf("Dynlinking module %s\n",m->filename);
#endif
    
    if (dld_link(m->filename)) { 
        strcpy(error_module_name, m->filename);
        deallocate(&dynmodule_alloc,m);
        dynlink_error(new_string(error_module_name));
    }
    
    /* compute the init function name */
    s1 = strrchr(name,'/');
    ifn (s1) 
        s1 = strrchr(name,':');
    ifn (s1)
        s1 = strrchr(name,'\\');
    ifn (s1)
        s1 = name;
    else
        s1 += 1;
    if (s2-s1<=0 || s2-s1>NAMELENGTH-8)
        error(NIL,"init function name is too long");
    strcpy(m->initname,"init_");
    s = m->initname + strlen(m->initname);
    while (s1<s2)
        *s++ = *s1++;
    *s = 0;
#ifdef DEBUG
    printf("Initfunction name is %s\n",m->initname);
#endif
    
    /* Looking for initfunction */
    m->initfunc = (void(*)()) dld_get_func(m->initname);
    if (m->initfunc) {
#ifdef VERSION
        get_version_info(m);
        if (m->major_version != maj_version() || m->minor_version != min_version())
        {
            dld_unlink_by_file(m->filename, 1);
            deallocate(&dynmodule_alloc, m);
            error(NIL, "file being loaded has wrong SN version", NIL);
        }
#endif /* VERSION */
        m->exec_state = EXEC_NEVER;
#ifdef DEBUG
        printf("Initfunction found\n");
#endif
    }
#ifdef DEBUG
    else
        printf("Initfunction not found\n");
#endif
    
    
    /* Adding thie module to the list */
    m->next = modules;
    modules = m;
    
    /* Checking executability */
    check_executability();
}



DX(xmod_load)
{
    ARG_NUMBER(1);
    ARG_EVAL(1);
    module_load(ASTRING(1));
    return new_string(file_name);
}

/******** Load a shared module ***********/

#ifdef DLOPEN
#include <dlfcn.h>

DX(xmod_dlopen)
{
    void *handle;
    int mode;
#ifdef RTLD_NOW
    mode = RTLD_NOW;
#else
    mode = 1;
#endif
    ARG_NUMBER(1);
    ARG_EVAL(1);
    dynlink_init(TRUE);
    handle = dld_dlopen(ASTRING(1), mode);
    if (!handle)
        dynlink_error(APOINTER(1));
    check_executability();
    return true();
}

#endif



/******** Alternate dx/dy define *********/



int
dld_dx_define(name,addr)
char *name;
void *addr;
{
    at *p, *symb;
    struct dynlink_object *f;
    
#ifdef DEBUG
    printf("Defining DX function %s\n",name);
#endif
    
    ifn (module_being_inited)
        error("dld.c/dld_dx_define","internal error",NIL);
    
    symb = named(name);
    
    if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
        print_string("*** Warning: Symbol ");
        print_string(name);
        print_string("is locked.");
        UNLOCK(symb);
        return;
    }
    
    f = allocate(&dynfunction_alloc);
    f->c_object.address = addr;
    f->check_depend_p = 0;
    f->class_p = 0;
    f->c_object.kind = COBJECT_DYNLINKED;
    f->address = addr;
    strncpy(f->name,name,NAMELENGTH-1);
    f->name[NAMELENGTH-1]=0;
    
    f->next = module_being_inited->c_objects;
    module_being_inited->c_objects = f;
    
    p = new_extern(&dx_class, f, XT_DX);
    
    var_set(symb,p);
    UNLOCK(symb);
    UNLOCK(p);
}



int
dld_dy_define(name,addr)
char *name;
void *addr;
{
    at *p, *symb;
    struct dynlink_object *f;
    
#ifdef DEBUG
    printf("Defining DY function %s\n",name);
#endif
    
    ifn (module_being_inited)
        error("dld.c/dld_dy_define","internal error",NIL);
    
    symb = named(name);
    
    if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
        print_string("*** Warning: Symbol ");
        print_string(name);
        print_string("is locked.");
        UNLOCK(symb);
        return;
    }
    
    f = allocate(&dynfunction_alloc);
    f->check_depend_p = 0;
    f->c_object.address = addr;
    f->c_object.kind = COBJECT_DYNLINKED;
    f->class_p = 0;
    f->address = addr;
    strncpy(f->name,name,NAMELENGTH-1);
    f->name[NAMELENGTH-1]=0;
    
    f->next = module_being_inited->c_objects;
    module_being_inited->c_objects = f;
    
    p = new_extern(&dy_class, f, XT_DY);
    
    var_set(symb,p);
    UNLOCK(symb);
    UNLOCK(p);
}

/******************************************************************************
 *
 * Load a DH
 *
 ******************************************************************************/
at *
dld_new_dh(addr, name)
void *addr;
char *name;
{
    at *p, *symb;
    struct dynlink_object *f;
    
#ifdef DEBUG
    printf("Defining DH function %s\n",name);
#endif
    
    ifn (module_being_inited)
        error("dld.c/dld_dh_define","internal error",NIL);
    
    f = allocate(&dynfunction_alloc);
    f->check_depend_p = 1;
    f->class_p = 0;
    f->c_object.address = addr;
    f->c_object.kind = COBJECT_DYNLINKED;
    f->address = addr;
    strncpy(f->name,name,NAMELENGTH-1);
    f->name[NAMELENGTH-1]=0;
    f->next = module_being_inited->c_objects;
    module_being_inited->c_objects = f;
    
    p = new_extern(&dh_class, f, XT_DH);
    return p;
}

int
dld_dh_define(name,addr)
char *name;
void *addr;
{
    at *p, *symb;
    
#ifdef DEBUG
    printf("Defining DH function %s\n",name);
#endif
    
    ifn (module_being_inited)
        error("dld.c/dld_dh_define","internal error",NIL);
    
    symb = named(name);
    
    if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
        print_string("*** Warning: Symbol ");
        print_string(name);
        print_string("is locked.");
        UNLOCK(symb);
        return;
    }
    
    p = dld_new_dh(addr,name);
    var_set(symb, p);
    UNLOCK(symb);
    UNLOCK(p);
}

/******************************************************************************
 *
 * Load dynamically a LISP class: BUG!!  Don't use!!
 *
 ******************************************************************************/
void
dld_class_define(name,cl)
char *name;
class *cl;
{
#ifdef DEBUG
    printf("Defining class %s\n",name);
#endif
    /* NOT YET IMPLEMENTED (HOW TO UNLOAD IT?) */
    module_being_inited = NULL;
    doing_dynlink = 0;
    error(NIL,"This module attempted to define a new class",NIL);
}

/******************************************************************************
 *
 * Load dynamically a C class
 *
 ******************************************************************************/
at *
dld_cclass_define(name,addr)
char *name;
void *addr;
{
    at *p, *symb;
    struct dynlink_object *f;
    
#ifdef DEBUG
    printf("Defining CCLASS function %s\n",name);
#endif
    
    /* IS THERE A PROBLEM TO UNLOAD IT?) */
    
    ifn (module_being_inited)
        error("dld.c/dld_class_define","internal error",NIL);
    
    f = allocate(&dynfunction_alloc);
    f->check_depend_p = 1;
    f->class_p = 1;
    f->c_object.address = addr;
    f->c_object.kind = COBJECT_DYNLINKED;
    f->address = addr;
    strncpy(f->name,name,NAMELENGTH-1);
    f->name[NAMELENGTH-1]=0;
    
    f->next = module_being_inited->c_objects;
    module_being_inited->c_objects = f;
    
    return new_extern(&cclass_class, f, XT_CCLASS);
}

/******* alternate cfunc_dispose ********/


static void 
dld_cfunc_action(q, action)
at *q;
void (*action)();
{
    int i;
    dhclassconstraint *k;
    struct dynlink_object *f = q->Object;
    
    if (f->c_object.kind != COBJECT_DYNUNLINKED)
        if (f->class_p != 0)
        {
            k = (dhclassconstraint*)(f->address);
            for (i=0;i<k->lispdata.nmethods;i++)
                (*action)(k->lispdata.at_methods[i]);
        }
}

static void 
dld_cfunc_dispose(p)
at *p;
{
    int i;
    struct dynlink_object *f;
    dhclassconstraint *k;
    
    f = p->Object;
    switch (f->c_object.kind) 
    {
    case COBJECT_DYNLINKED:
        if (f->class_p)
        {
            k = (dhclassconstraint*)(f->address);
            for (i=0;i<k->lispdata.nmethods;i++) 
                UNLOCK(k->lispdata.at_methods[i]);
        }
        f->c_object.kind = COBJECT_DYNUNREFRED;
        break;
    case COBJECT_DYNUNLINKED:
        f->c_object.address = dynlink_gone;
        deallocate(&dynfunction_alloc,f);
        break;
    default:
        break;
    }
}


/*********** Utilities ******************/


DX(xmod_create_reference)
{
    int i;
    char *s;
    ALL_ARGS_EVAL;
    dynlink_init(TRUE);
    for (i=1; i<=arg_number; i++)
    {
        s = ASTRING(i);
        dld_create_reference(s);
    }
    return NEW_NUMBER(dld_undefined_sym_count);
}

DX(xmod_compatibility_flag)
{
    ARG_NUMBER(1);
    ARG_EVAL(1);
    dld_compatibility_flag = ( APOINTER(1) ? 1 : 0 );
    return (dld_compatibility_flag ? true() : NIL);
}

DX(xmod_list)
{
    at *p, **where;
    struct dynlink_module *m;
    
    ARG_NUMBER(0);
    p = NIL;
    where = &p;
    for (m=modules; m; m=m->next) {
        *where = cons( new_string(m->filename), NIL);
        where = &((*where)->Cdr);
    }
    return p;
}

DX(xmod_undefined)
{
    char **dld_undefined_sym_list;
    extern int dld_undefined_sym_count;
    at *p, **where;
    int i;
    ifn (dynlink_initialized)
        return NIL;
    dld_undefined_sym_list = (char**)dld_list_undefined_sym();
    p = NIL;
    where = &p;
    for (i=0; i<dld_undefined_sym_count; i++) {
        *where = cons( new_string(dld_undefined_sym_list[i]), NIL);
        where = &((*where)->Cdr);
    }
    free(dld_undefined_sym_list);
    return p;
}




DX(xmod_inquire)
{
    struct dynlink_module *m;
    struct dynlink_object *f;
    char *name;
    at *p, **where;
    
    ARG_NUMBER(1);
    ARG_EVAL(1);
    name = ASTRING(1);
    
    /* find the module */
    name = search_file(name,"o");
    ifn (name)
        error(NIL,"Module not found",APOINTER(1));
    for (m=modules; m; m=m->next)
        if (!strcmp(name,m->filename))
            break;
    ifn(m)
        error(NIL,"Module not found",new_string(name));
    
    /* status */
    switch ( m->exec_state ) {
    default:
    case EXEC_NOINIT:
        name = "State: unknown"; 
        break;
    case EXEC_NEVER:
        name = "State: uninitialized";
        break;
    case EXEC_OK:
        name = "State: initialized, executable";
        break;
    case EXEC_NOLONGER:
        name = "State: initialized, partially linked";
        break;
    }
    
    p = cons(new_safe_string(name),NIL);
    where = &(p->Cdr);
    for (f=m->c_objects; f; f=f->next) {
        *where = cons( named(f->name), NIL);
        where = &((*where)->Cdr);
    }
    return p;
}



/*********** Initialization *************/


void init_dld()
{
    dx_define("mod-load",xmod_load);
    dx_define("mod-unload",xmod_unload);
    dx_define("mod-list",xmod_list);
    dx_define("mod-undefined",xmod_undefined);
    dx_define("mod-inquire",xmod_inquire);
    dx_define("mod-status", xmod_status);
    dx_define("mod-create-reference", xmod_create_reference);
    dx_define("mod-compatibility-flag", xmod_compatibility_flag);
    /* Plug the alternate cfunc_dispose */
    dx_class.self_dispose = dld_cfunc_dispose;
    dy_class.self_dispose = dld_cfunc_dispose;
    cclass_class.self_dispose = dld_cfunc_dispose;
    cclass_class.self_action = dld_cfunc_action;
    /* Compute executable name from current directory */
    dynlink_init(FALSE);
    /* Additional DLOPEN function */
#ifdef DLOPEN
    dx_define("mod-dlopen", xmod_dlopen);
#endif
}

#endif /* DLD */

