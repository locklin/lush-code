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

#include "header.h"
#include "dh.h"

extern at *at_true;
static at *dhinfo_record(), *dhinfo_record_temp();
extern at *new_storage(), *dld_new_dh(), *dld_cclass_define();
extern dhrecord *next_record();
extern storage_to_dht[];
extern dht_to_storage[];

extern void class_dispose();
extern void class_action();
extern char *class_name();
extern dynlink_gone(), dynlink_partial();




/* ---------- The compiled class descriptor ------------ */

extern at *dh_listeval();

class dh_class = {
  generic_dispose,
  generic_action,
  generic_name,
  generic_eval,
  dh_listeval,
  XT_DH,
};

void 
cclass_dispose(q)
at *q;
{
    int i;
    union function *cfunc;
    dhclassconstraint *k;
    cfunc = q->Object;
    k = cfunc->cfunc.address;
    for (i=0;i<k->lispdata.nmethods;i++) 
	UNLOCK(k->lispdata.at_methods[i]);
}

void 
cclass_action(q, action)
at *q;
void (*action)();
{
    int i;
    union function *cfunc;
    dhclassconstraint *k;
    cfunc = q->Object;
    k = cfunc->cfunc.address;
    for (i=0;i<k->lispdata.nmethods;i++)
	(*action)(k->lispdata.at_methods[i]);
}

class cclass_class = {
  cclass_dispose,
  cclass_action,
  generic_name,
  generic_eval,
  generic_listeval,
  XT_CCLASS,
};

/* ------ The dh_define stuff -------- */

at *
new_dh(addr)
at *(*addr) ();
{
  extern struct alloc_root function_alloc;
  register union function *cfunc;
  register at *p;

  cfunc = allocate(&function_alloc);
  cfunc->cfunc.address = addr;
  cfunc->cfunc.kind = COBJECT_STANDARD;
  p = new_extern(&dh_class, cfunc, XT_DH);
  return p;
}

at *
new_cclass(addr)
at *(*addr) ();
{
    register union function *f;
    register at *p;
    
    f = malloc(sizeof(union function));
    if (!f)
        error(NIL,"Out of memory",NIL);
    f->cfunc.address = addr;
    f->cfunc.kind = COBJECT_STANDARD;
    p = new_extern(&cclass_class, f, XT_CCLASS);
    return p;
}


/* --------- Patrice's debug stuff --------- */



/* atprint -- debug function to display an at object */

void
atprint(p)
at *p;
{
    static int tab = 0;
    int i;
    at *q;

#define nspace(a) { int i, n=(a); for(i=0;i<n;i++) printf(" ");}

    nspace(tab);
    if(!p) {
	printf("NIL\n");
	return;
    }
    printf("%8X (%d) ", p, p->count);
    switch((enum ctypes) (p->ctype)) {
    case XT_INDEX:
	printf("INDEX, ndim: %d, offset: %d, flags: %X, dims:", 
	       ((struct index *)(p->Object))->ndim,
	       ((struct index *)(p->Object))->offset,
	       ((struct index *)(p->Object))->flags);
        for(i=0;i<((struct index *)(p->Object))->ndim;i++)
	    printf(" %d", ((struct index *)(p->Object))->dim[i]);
	printf(" mods:");
	for(i=0;i<((struct index *)(p->Object))->ndim;i++)
	    printf(" %d", ((struct index *)(p->Object))->mod[i]);
	printf("\n");
	tab += 3;
	atprint(((struct index *)(p->Object))->atst);
	tab -= 3;
	break;
    case XT_OBJECT:
	printf("OBJECT(Class %s: %8X (%d)):\n", 
	       pname(((struct class *)(p->Slots->class->Object))->classname),
	       p->Slots->class, p->Slots->class->count);
	tab += 3;
	for(i=p->Slots->size-1; i>=0;i--) {
	    nspace(tab);
	    printf("%s:\n", pname(p->Slots->slots[i].symb));
	    atprint(p->Slots->slots[i].val);
	}
	tab -= 3;
	break;
    case XT_NUMBER:
	printf("NUMBER = %f\n", (flt) p->Number);
	break;
    case XT_GPTR:
	printf("GPTR = %8X\n", p->Gptr);
	break;
    case XT_CONS:
	printf("LIST(%d):\n", length(p));
	q = p;
	tab += 3;
	while(q) {
	    atprint(q->Car);
	    q = q->Cdr;
	}
	tab -= 3;
	break;
    case XT_STRING:
	printf("STR = %s\n", ((struct string *) (p->Object))->start);
	break;
    case XT_SYMBOL:
	printf("SYMBOL = %s\n", ((struct symbol *) p->Object)->name->name);
	break;
    case XT_CCLASS:
        printf("CCLASS = (%d, %d)\n", 
               ((union function *)p->Object)->cfunc.address,
               ((union function *)p->Object)->cfunc.kind);
        break;
    case XT_ATSTORAGE: 
    case XT_PSTORAGE: 
    case XT_FSTORAGE: 
    case XT_DSTORAGE:
    case XT_I32STORAGE: 
    case XT_I16STORAGE: 
    case XT_I8STORAGE: 
    case XT_U8STORAGE:
	printf("SRG, st: %X, flags: %X, type: %X, data: %8X, size: %d ", 
	       (p->Object),
	       ((struct storage *)(p->Object))->srg.flags,
	       ((struct storage *)(p->Object))->srg.type,
	       ((struct storage *)(p->Object))->srg.data,
	       ((struct storage *)(p->Object))->srg.size);
	switch(((struct storage *)(p->Object))->srg.type) {
	case ST_AT: 
            printf("AT\n"); break;
	case ST_F:
            printf("flt\n"); break;
	case ST_D: 
            printf("real\n"); break;
	case ST_I32:
            printf("int\n"); break;
	case ST_I16:
            printf("short\n"); break;
	case ST_I8:
            printf("char\n"); break;
	case ST_U8:
            printf("uchar\n"); break;
	case ST_P:
            printf("packed\n"); break;
	}
	printf("\n");
	break;
    default:
	printf("UNKNOWN (ctype=%d)\n", p->ctype);
        break;
    }
#undef nspace
}

DX(xatprint)
{
    ARG_NUMBER(1);
    ARG_EVAL(1);
    atprint(APOINTER(1));
    return NIL;
}



/******************************************************************************
 *
 *  next_record : find the next record (recursively)
 *                and set drec->end for the current record
 *
******************************************************************************/
dhrecord *
next_record(drec)
    dhrecord *drec;
{
    dhrecord *temp_drec, *temp_drec2;
    enum dht_type ob_type;
    int n, i;

    switch(drec->op) {

      case DHT_BOOL:
      case DHT_BYTE:
      case DHT_UBYTE:
      case DHT_SHORT:
      case DHT_INT:
      case DHT_FLT:
      case DHT_REAL:
      case DHT_GPTR:
      case DHT_NIL:
      case DHT_STR:
	return (drec->end = drec+1);

      case DHT_IDX:
      case DHT_RETURN:	
      case DHT_SRG:
	temp_drec = drec;
	return (temp_drec->end = next_record(drec+1));

      case DHT_FUNC:
	n = 0;
	temp_drec = drec;
	drec++;
	while(drec->op != DHT_TEMPS && drec->op != DHT_RETURN) {
	    drec = next_record(drec);
	    n++;
	}
	/* hack to keep a pointer at the end or arguments handy */
	temp_drec->name = (char *) drec; 
	if(temp_drec->ndim != n) {
	    temp_drec->name = 0;
	    error(NIL, "bad number of argument in function DHDOC", NIL);
	}
	if(drec->op == DHT_TEMPS) 
	    drec = next_record(drec);
	if(drec->op != DHT_RETURN)
	    error(NIL, "function DHDOC has no return value", NIL);
	drec = next_record(drec);

	if(drec->op != DHT_END_FUNC) 
	    error(NIL, "function DHDOC has no end", NIL);
	return (temp_drec->end = drec+1);
	
      case DHT_OBJ:
	return (drec->end = drec+1);

      case DHT_TEMPS:
	temp_drec = drec;
	drec++;
	n = 0;
	while(drec->op != DHT_END_TEMPS) {
	    switch(drec->op) {
	      case DHT_STR:
	      case DHT_OBJ:
		drec->end = drec+1;
		drec++;
		break;
	      case DHT_LIST:
		i = drec->ndim-1;
		drec->end = drec+drec->ndim+2;
		drec++;
		for(;i>=0;i--) {
		    drec->end = drec+1;
		    drec++;
		}
		if(drec->op != DHT_END_LIST)
		    error(NIL,"inconsistent def of list in temp DHDOC", NIL);
		drec->end = drec+1;
		drec += 1;
		break;
	      case DHT_IDX:
	      case DHT_SRG:
		drec->end = drec+2;
		drec++;
		drec->end = drec+1;
		++drec;
		break;
	      default:
		error(NIL, "strange DHDOC TEMPS", NIL);
	    }
	    n++;
	}
	if(temp_drec->ndim != n)
	    error(NIL,"bad number of fields for list/object/temps DHDOC", NIL);
	return (temp_drec->end = drec+1);

      case DHT_LIST:
      case DHT_DEPENDS:
	switch(drec->op) {
	  case DHT_LIST:
	    ob_type = DHT_END_LIST;
	    break;
	  case DHT_DEPENDS:
	    ob_type = DHT_END_DEPENDS;
	    break;
	}
	temp_drec = drec;
	drec++;
	n = 0;
	while(drec->op != ob_type) {
	    drec = next_record(drec);
	    n++;
	}
	if(temp_drec->ndim != n)
	    error(NIL,"bad number of fields for list/object/temps DHDOC", NIL);
	return (temp_drec->end = drec+1);

      case DHT_CLASS:
	temp_drec = drec;
        drec->access=1;
	n = drec->ndim-1;
	drec++;
	for(; n>=0; n--) {
	    if(drec->op != DHT_NAME)
		error(NIL,"Slot name expected in Class DHDOC", NIL);
	    /* compute the offset of the slot in object structure */
	    if(drec->hashcode==0) 
		drec->hashcode = (long) ((char *) (drec->end) - 
					 (char *)(temp_drec->end)); 
	    drec->end = drec+1;
	    drec++;
	    drec = next_record(drec);
	}
	if (drec->op!=DHT_END_CLASS)
	    error(NIL,"strange Class DHDOC", NIL);
	return (temp_drec->end = drec+1);
        
    default:
	error(NIL, "strange DHDOC", NIL);
    }
}
	
/*****************************************************************************
 *
 *  HASH RECORD
 *
 ******************************************************************************/


static long 
hash_dhrecord(drec,end)
dhrecord *drec, *end;
{
    char *s;
    long hash = 0;

    while (drec < end)
    {
        switch (drec->op)
        {
        case DHT_OBJ:
            s = ((dhclassconstraint*)(drec->name))->lispdata.name;
            hash ^= hash_function(s, strlen(s));
            break;
        case DHT_NAME:
            s = drec->name;
            hash ^= hash_function(s, strlen(s));
            break;
        default:
            hash ^= ((drec->op) ^ (drec->access<<8) ^ (drec->ndim<<16));
            break;
        case DHT_DH_REFER:
            break;
        }
        drec++;
        if (hash & 0x80000000)
            hash = (hash<<1) + 1;
        else
            hash = (hash<<1);
    }
    return hash;
}


/*****************************************************************************
 *
 *  Define a DH
 *
 ******************************************************************************/
void 
dh_define(name, addr)
    dhconstraint *addr;
    char *name;
{
    register at *func, *symb;
    dhrecord *drec;

#ifdef DLD
    if (doing_dynlink) {
	dld_dh_define(name,addr);
	drec = addr->argdata;
	next_record(drec);
	addr->hashcode = hash_dhrecord(drec, drec->end);
	return;
    }
#endif
    
    func = new_dh(addr);
    next_record(addr->argdata);
    symb = new_symbol(name);
    if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
	sprintf(string_buffer, "symbol already exists: %s", name);
	error("function/dx_define", string_buffer, NIL);
    }
    var_set(symb, func);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
    UNLOCK(func);
    UNLOCK(symb);
}

/*****************************************************************************
 *
 *  Define a CClass
 *
 ******************************************************************************/
void 
cclass_define(name, addr)
    dhclassconstraint *addr;
    char *name;
{
    register at *func, *symb, *ans, *p;
    dhrecord *drec;
    dhconstraint **Kmethod_table, **Ksupermethod_table;
    dhclassconstraint *super;
    int i;
    method_ptr *mt, *super_mt;
    Kmethod_table = addr->lispdata.DH_method_table;
    drec = addr->argdata;
    next_record(drec);  /* set drec-end */

    if (!addr->lispdata.super->argdata->access) {
       fprintf(stderr,"Superclass %s for class %s not yet defined\n",
          addr->lispdata.supername, addr->lispdata.name);
       error(NIL,"Internal Error: modules not initialized in right order",NIL);
    }
#ifdef DLD
    if (doing_dynlink) {
	func = dld_cclass_define(name,addr);
	/* Get methods from super class.  Set i to the correct values  */
	if(addr->lispdata.super) {
	    super = addr->lispdata.super;
	    mt = addr->lispdata.method_table;
	    super_mt = super->lispdata.method_table;
	    for(i=0;i<super->lispdata.nmethods;i++) {
		if(addr->lispdata.method_table[i] == 
		   super->lispdata.method_table[i]) {
		    /* method IS NOT a redefinition of superclass method */
		    addr->lispdata.at_methods[i] = super->lispdata.at_methods[i];
		    LOCK(super->lispdata.at_methods[i]);
		} else {
		    /* If method IS a redefinition of superclass method */
		    next_record(Kmethod_table[i]->argdata);
		    addr->lispdata.at_methods[i] = 
			dld_new_dh(Kmethod_table[i], 
                                   Kmethod_table[i]->lispdata.c_name);
                    Kmethod_table[i]->hashcode =
                        /* hash note: negative size: we only hash the size */
                        hash_dhrecord(Kmethod_table[i]->argdata,
                                      Kmethod_table[i]->argdata->end);
		}
	    }
	}
	/* Add the new methods which are not redefinitions */
	for(;i<addr->lispdata.nmethods;i++) {
            next_record(Kmethod_table[i]->argdata);
            addr->lispdata.at_methods[i] = 
                dld_new_dh(Kmethod_table[i], 
                           Kmethod_table[i]->lispdata.c_name);
            Kmethod_table[i]->hashcode =
                /* note: negative size: we only hash the size */
                hash_dhrecord(Kmethod_table[i]->argdata,
                              Kmethod_table[i]->argdata->end);
        }
        /* Compute hashcode for the class and virtual table */
        addr->hashcode = hash_dhrecord(drec, drec->end);
        /* hash note: negative size: we only hash the size */
        for (i=0; i<addr->lispdata.nmethods; i++) {
            char *s = addr->lispdata.Lisp_methods[i];
            addr->hashcode ^= hash_function(s, strlen(s));
        }
        /* Execute Lisp code to build the class */
	p = named("cclass_to_class");
        func = cons(func,NIL);
	ans = apply(p,func);
	UNLOCK(func);
	UNLOCK(p);
	UNLOCK(ans);
	return;
    }
#endif

    /* Create the cclass object */
    func = new_cclass(addr);

    /* Get methods from super class.  Set i to the correct values  */
    if(addr->lispdata.super) {
	super = addr->lispdata.super;
	mt = addr->lispdata.method_table;
	super_mt = super->lispdata.method_table;
	for(i=0;i<super->lispdata.nmethods;i++) {
	    if(addr->lispdata.method_table[i] == 
	       super->lispdata.method_table[i]) {
		/* method IS NOT a redefinition of superclass method */
		addr->lispdata.at_methods[i] = super->lispdata.at_methods[i];
		LOCK(super->lispdata.at_methods[i]);
	    } else {
		/* If method IS a redefinition of superclass method */
		next_record(((addr->lispdata.DH_method_table)[i])->argdata);
		addr->lispdata.at_methods[i] = new_dh(Kmethod_table[i]);
	    }
	}
    }
    /* Add the new methods which are not redefinitions */
    for(;i<addr->lispdata.nmethods;i++) {
	next_record(((addr->lispdata.DH_method_table)[i])->argdata);
	addr->lispdata.at_methods[i] = new_dh(Kmethod_table[i]);
    }
    /* ARG: execute lisp code to build the class */
    p = named("cclass_to_class");
    func = cons(func,NIL);
    ans = apply(p,func);
    UNLOCK(func);
    UNLOCK(p);
    UNLOCK(ans);
    return;
}

/*****************************************************************************
 *
 *  Check dependesnce
 *
******************************************************************************/
int 
check_dependence(dhdoc, filename, verbose)
dhconstraint *dhdoc;
char *filename;
int verbose;
{
    dhrecord *drec;
    drec = next_record(dhdoc->argdata);
    if(drec->op == DHT_DEPENDS) {
	drec++;
	while(drec->op != DHT_END_DEPENDS) 
        {
	    if(drec->hashcode != ((dhconstraint *) drec->name)->hashcode) 
            {
                if (verbose)
                {
                    if (((dhconstraint*)(drec->name))->argdata->op == DHT_FUNC)
                        printf("*** dld warning: function <%s> ", 
                               ((dhconstraint*)(drec->name))->lispdata.c_name);
                    else
                        printf("*** dld warning: class <%s> ", 
                               ((dhclassconstraint*)(drec->name))->lispdata.name + 8 );
                    
                    printf("is inconsistent with previously loaded ");
                    if (((dhconstraint*)(dhdoc))->argdata->op == DHT_FUNC)
                        printf("function <%s>.\n", 
                               ((dhconstraint*)(dhdoc))->lispdata.c_name);
                    else
                        printf("class <%s>.\n", 
                               ((dhclassconstraint*)(dhdoc))->lispdata.name + 8 );
                    printf("***\007    You must recompile file \"%s\".\n", filename);
                }
		return 1;
	    }
	    drec++;
	}
    }
    return 0;
}


/******************************************************************************
 *
 *  Clean strings created by "enclose_in_string"
 *
******************************************************************************/

static char *
strclean(s)
char *s;
{
    if (s != 0)
        while (*s && (*s==' ' || *s=='\t' || *s=='\n' || *s=='\r'))
            s += 1;
    return s;
}


/******************************************************************************
 *
 *  Recursive build the type description
 *
******************************************************************************/

static at *
dhinfo_chain(drec,n)
    dhrecord *drec;
    int n;
{
    at *ans, **where;
    
    ans = NIL;
    where = &ans;
    while (n-->0) {
	*(where) = cons(dhinfo_record(drec) , NIL);
	drec = drec->end;
	where = &((*where)->Cdr);
    }
    return ans;
}

static at *
dhinfo_chain_temp(drec,n)
    dhrecord *drec;
    int n;
{
    at *ans, **where;
    
    ans = NIL;
    where = &ans;
    while (n-->0) {
	*(where) = cons(dhinfo_record_temp(drec) , NIL);
	drec = drec->end;
	where = &((*where)->Cdr);
    }
    return ans;
}

static at *
dhinfo_record_list(drec)
dhrecord *drec;
{
    at *ans, **where;
    int n;
    
    switch (drec->op) {
      case DHT_LIST:
	/* No recursion on list, substitude it to a record */
	ans = NIL;
	where = &ans;
	n = drec->ndim;
	while (n-->0) {
	    *(where) = cons(cons(named("unk"), NIL), NIL);
	    where = &((*where)->Cdr);
	}
	return cons(named("list"), ans);
      default:
	return dhinfo_record(drec);
    }
}

static at *
dhinfo_chain_list(drec,n)
dhrecord *drec;
int n;
{
    at *ans, **where;
    
    ans = NIL;
    where = &ans;
    while (n-->0) {
	*(where) = cons(dhinfo_record_list(drec) , NIL);
	drec = drec->end;
	where = &((*where)->Cdr);
    }
    return ans;
}

static at *
dhinfo_record_temp(drec)
dhrecord *drec;
{
    at *ans;
    
    switch (drec->op) {
      case DHT_STR:
	return cons(named("str"), NIL);
      case DHT_IDX: 
	return cons(named("idx"),
		    cons(named("w"),
			 cons(NEW_NUMBER(drec->ndim),
			      cons(cons(named("srg"),
					cons(named("w"),
					     cons(dhinfo_record(drec+1), NIL))),
				   NIL ))));
      case DHT_SRG:
	return cons(named("srg"),
		    cons(named("w"),
			 cons(dhinfo_record(drec+1),NIL)));
      case DHT_LIST:
	return cons(named("list"),
		    dhinfo_chain_list(drec+1, drec->ndim), NIL);
      case DHT_OBJ:
	return cons(named("obj"),
		    cons(new_string(strclean(((dhclassconstraint *) 
                                             drec->name)->lispdata.name)),
                         NIL));
      default: 
	error(NIL, "Cannot construct dhinfo for temps: unknown type", NIL);
	/* return cons(named("unk"), NIL);*/
    }
}

static at *
dhinfo_record(drec)
dhrecord *drec;
{
    at *ans;
    
    switch (drec->op) {
      case DHT_BOOL: return cons(named("bool"),NIL);
      case DHT_BYTE: return cons(named("byte"),NIL);
      case DHT_UBYTE: return cons(named("ubyte"),NIL);
      case DHT_SHORT: return cons(named("short"),NIL);
      case DHT_INT: return cons(named("int"),NIL);
      case DHT_FLT: return cons(named("flt"),NIL);
      case DHT_REAL: return cons(named("real"),NIL);
      case DHT_GPTR: return cons(named("gptr"),NIL);
      case DHT_NIL: return cons(named("bool"),NIL);
      case DHT_STR: return cons(named("str"),NIL);
      case DHT_NAME: return new_string(strclean(drec->name));
      case DHT_OBJ: 
          return cons(named("ptr"), 
                      cons(cons(named("obj"), 
                                cons(new_string(strclean(((dhclassconstraint *) 
                                                          drec->name)->lispdata.name)),
                                   NIL)),
                           NIL));
      case DHT_LIST:
          return cons(named("list"),
                      dhinfo_chain(drec+1, drec->ndim));
      case DHT_IDX: 
          return cons(named("ptr"), 
                 cons( cons(named("idx"),
                       cons(named((drec->access == 0) ? "r" : "w"),
		       cons(NEW_NUMBER(drec->ndim),
	               cons(cons(named("srg"), 
			    cons(named(((drec+1)->access == 0) ? "r" : "w"),
			    cons(dhinfo_record(drec+2), 
                                 NIL))), 
                            NIL)))),
                      NIL));
      case DHT_SRG: 
	return cons(named("ptr"), 
	       cons( cons(named("srg"), 
 		     cons(named((drec->access == 0) ? "r" : "w"),
	             cons(dhinfo_record(drec+1), 
                          NIL))), 
                    NIL));
      case DHT_FUNC: {
	  dhrecord *drec_temp = (dhrecord *) (drec->name), *drec_return;
	  drec_return = drec_temp;
	  if(drec_temp->op == DHT_TEMPS)
	      drec_return = drec_temp->end;
	  return cons(named("func"), 
		 cons(dhinfo_chain(drec+1, drec->ndim),
		 cons((drec_temp->op == DHT_TEMPS) ?
		      dhinfo_chain_temp(drec_temp+1, drec_temp->ndim) : NULL,
		 cons(dhinfo_record(drec_return+1), 
                      NIL))));
      }
      default: return cons(named("unk"), NIL);
    }
}


DX(xdhinfo_t)
{
  at *p;
  union function *cfunc;
  dhconstraint *dhdoc;
  char *err;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  ifn (p && p->ctype==XT_DH)
      error(NIL,"Not a DH",p);
      
  switch(p->ctype) {
    case XT_DH:
      cfunc = p->Object;
      dhdoc = cfunc->cfunc.address;
      if(dhdoc== (dhconstraint *) NIL)
	  error(NIL, "DH was never loaded", p);
      if(dhdoc== (dhconstraint *) dynlink_gone)
	  error(NIL, "This DH has been unlinked", p);
      if(dhdoc== (dhconstraint *) dynlink_partial)
	  error(NIL, "This DH belong to a partially linked module", p);
      return dhinfo_record(dhdoc->argdata);
  }
}


DX(xdhinfo_c)
{
    at *p;
    union function *cfunc;
    dhconstraint *dhdoc;
    char *err;
    char hashcode[9];
    
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    ifn (p && p->ctype==XT_DH)
	error(NIL,"Not a DH",p);
    cfunc = p->Object;
    dhdoc = cfunc->cfunc.address;
    if(dhdoc== (dhconstraint *) NIL)
	error(NIL, "DH was never loaded", p);
    if(dhdoc== (dhconstraint *) dynlink_gone)
	error(NIL, "This DH has been unlinked", p);
    if(dhdoc== (dhconstraint *) dynlink_partial)
	error(NIL, "This DH belong to a partially linked module", p);
    sprintf(hashcode, "%X", dhdoc->hashcode);
    
    return cons(new_string(strclean(dhdoc->lispdata.c_name)),
	   cons(new_string(strclean(dhdoc->lispdata.m_name)),
	   cons(new_string(strclean((dhdoc->lispdata.test_name &&
                                     dhdoc->lispdata.test_name->lispdata.c_name) 
                                    ? dhdoc->lispdata.test_name->lispdata.c_name 
                                    : "")),
           cons(new_string(strclean((dhdoc->lispdata.test_name &&
                                     dhdoc->lispdata.test_name->lispdata.m_name) 
                                    ? dhdoc->lispdata.test_name->lispdata.m_name
                                    : "")),
           cons(new_string(strclean(dhdoc->lispdata.k_name)), 
	   cons(new_string(hashcode), 
                NIL))))));
}



DX(xdh_gptr)
{
#ifndef DLD
    error(NIL,"not supported without dynamic loader",NIL);
#else

    at *p;
    union function *cfunc;
    dhconstraint *dhdoc;
    char *err;
    char hashcode[9];
    extern void * dld_get_func();

    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    ifn (p && p->ctype==XT_DH)
	error(NIL,"Not a DH",p);
    cfunc = p->Object;
    dhdoc = cfunc->cfunc.address;
    if(dhdoc== (dhconstraint *) NIL)
	error(NIL, "DH was never loaded", p);
    if(dhdoc== (dhconstraint *) dynlink_gone)
	error(NIL, "This DH has been unlinked", p);
    if(dhdoc== (dhconstraint *) dynlink_partial)
	error(NIL, "This DH belong to a partially linked module", p);

    return NEW_GPTR(dld_get_func(dhdoc->lispdata.c_name));
#endif
}

/******************************************************************************
 *
 *  Class info
 *
******************************************************************************/

static at *
classinfo_chain(drec,n)
    dhrecord *drec;
    int n;
{
    at *ans, **where, *temp;
    ans = NIL;
    where = &ans;
    while (n-->0) {
	temp = dhinfo_record(drec);
	drec = drec->end;
	*(where) = cons(cons(temp , dhinfo_record(drec)), NIL);
	drec = drec->end;
	where = &((*where)->Cdr);
    }
    return ans;
}

static at *
classinfo_record(d)
    dhclassconstraint *d;
{
    at *ans, **where, *lisp_names;
    dhrecord *drec, *drec_temp;
    int i,j, nmdethod, n;
    dhconstraint **Kmethod_table;
    char **methods_name;
    
    drec = d->argdata;
    drec_temp = drec;
    if(drec->op != DHT_CLASS) 
	error(NIL, "This CLASSDHDOC doesn't is weird", NIL);
    n = d->lispdata.nmethods;
    Kmethod_table = d->lispdata.DH_method_table;
    methods_name = (d->lispdata.Lisp_methods);

    ans = NIL;
    where = &ans;
    i = 0;
    while (n-->0) {
	*(where) = cons(cons(new_string(strclean((Kmethod_table[i])->lispdata.c_name)),
			     dhinfo_record((Kmethod_table[i])->argdata)), NIL);
	where = &((*where)->Cdr);
	i++;
    }

    lisp_names = NIL;
    where = &lisp_names;
    n = d->lispdata.nmethods;
    i = 0;
    while (n-->0) {
	*(where) = cons(new_string(strclean(methods_name[i])), NIL);
	where = &((*where)->Cdr);
	i++;
    }
    return cons(new_string(strclean(d->lispdata.name)), 
           cons(new_string(strclean(d->lispdata.super ? d->lispdata.supername : "")), 
           cons(classinfo_chain(drec+1, (drec->ndim)), 
           cons(ans, 
           cons(lisp_names, 
                NIL)))));
}

DX(xclassinfo_t)
{
  at *p;
  union function *cfunc;
  dhclassconstraint *dhdoc;
  char *err;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  ifn (p && p->ctype==XT_CCLASS)
      error(NIL,"Not a Cclass",p);
  cfunc = p->Object;
  dhdoc = cfunc->cfunc.address;
  if(dhdoc== (dhclassconstraint *) NIL)
      error(NIL, "Class was never loaded", p);
  if(dhdoc== (dhclassconstraint *) dynlink_gone)
      error(NIL, "This Class has been unlinked", p);
  if(dhdoc== (dhclassconstraint *) dynlink_partial)
      error(NIL, "This Class belong to a partially linked module", p);
  return classinfo_record(dhdoc);
}


DX(xclassinfo_c)
{
    at *p, *ans, **where;
    union function *cfunc;
    dhclassconstraint *dhdoc;
    dhconstraint **Kmethod_table;    
    char *err;
    int i;
    char hashcode[9];
    
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    ifn (p && p->ctype==XT_CCLASS)
	error(NIL,"Not a Cclass",p);
    cfunc = p->Object;
    dhdoc = cfunc->cfunc.address;
    if(dhdoc== (dhclassconstraint *) NIL)
	error(NIL, "Class was never loaded", p);
    if(dhdoc== (dhclassconstraint *) dynlink_gone)
	error(NIL, "This Class has been unlinked", p);
    if(dhdoc== (dhclassconstraint *) dynlink_partial)
	error(NIL, "This Class belong to a partially linked module", p);
    sprintf(hashcode, "%X", dhdoc->hashcode);

    Kmethod_table = dhdoc->lispdata.DH_method_table;
    ans = NIL;
    where = &ans;
    for (i=0;i<dhdoc->lispdata.nmethods; i++) {
	*where = cons(dhdoc->lispdata.at_methods[i], NIL);
	LOCK(dhdoc->lispdata.at_methods[i]);
	where = &((*where)->Cdr);
    }
    return /* continued */
        cons(new_string(strclean(dhdoc->lispdata.name)),
        cons(new_string(strclean(dhdoc->lispdata.super?dhdoc->lispdata.supername:"")),
        cons(new_string(strclean(dhdoc->lispdata.method_table_name)),
        cons(new_string(strclean(dhdoc->lispdata.DH_method_table_name)),
	cons(ans, 
	cons(new_string(hashcode), 
             NIL))))));
}

/******************************************************************************
 *
 *  Fast convertion of storage type to dht type
 *
******************************************************************************/
int storage_to_dht[ST_LAST];
int dht_to_storage[DHT_LAST];

init_storage_to_dht() 
{
    int i;
    
    for(i=0;i<ST_LAST;i++)
        storage_to_dht[i] = -1;
    storage_to_dht[ST_F] = DHT_FLT;
    storage_to_dht[ST_D] = DHT_REAL;
    storage_to_dht[ST_I32] = DHT_INT;
    storage_to_dht[ST_I16] = DHT_SHORT;
    storage_to_dht[ST_I8] = DHT_BYTE;
    storage_to_dht[ST_U8] = DHT_UBYTE;
    storage_to_dht[ST_GPTR] = DHT_GPTR;

    for(i=0;i<DHT_LAST;i++)
        dht_to_storage[i] = -1;
    dht_to_storage[DHT_FLT] = ST_F;
    dht_to_storage[DHT_REAL] = ST_D;
    dht_to_storage[DHT_INT] = ST_I32;
    dht_to_storage[DHT_BYTE] = ST_I8;
    dht_to_storage[DHT_UBYTE] = ST_U8;
    dht_to_storage[DHT_SHORT] = ST_I16;
    dht_to_storage[DHT_GPTR] = ST_GPTR;
}

/* -------- The init function -------- */

void 
init_dh()
{
  init_storage_to_dht();

  class_define("DH", &dh_class);
  class_define("Cclass", &cclass_class);

  dx_define("atprint", xatprint);

  dx_define("dhinfo-t",xdhinfo_t);
  dx_define("dhinfo-c",xdhinfo_c);
  dx_define("dhgptr", xdh_gptr);
  dx_define("classinfo-t",xclassinfo_t);
  dx_define("classinfo-c",xclassinfo_c);

}
