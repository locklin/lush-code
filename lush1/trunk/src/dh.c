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
 * $Id: dh.c,v 1.18 2004-08-30 21:24:11 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "dh.h"


/* ---------------------------------------------
   UTILITIES
   --------------------------------------------- */


/* next_record --
   complete next record pointer in dhdoc. */

static dhrecord *
next_record(dhrecord *drec)
{
  dhrecord *temp_drec;
  int n, i;
  
  switch(drec->op) 
    {
      /* TYPES */
      
    case DHT_BOOL:
    case DHT_BYTE:
    case DHT_UBYTE:
    case DHT_SHORT:
    case DHT_INT:
    case DHT_FLT:
    case DHT_REAL:
    case DHT_GPTR:
    case DHT_STR:
    case DHT_OBJ:
      return (drec->end = drec+1);

    case DHT_IDX:
      if (drec[1].op != DHT_SRG)
        error(NIL,"Malformed DHDOC: "
              "IDX record without SRG record", NIL);
    case DHT_SRG:
      temp_drec = drec;
      return (temp_drec->end = next_record(drec+1));

    case DHT_LIST:
      n = 0;
      temp_drec = drec;
      drec++;
      while (drec->op != DHT_END_LIST && 
             drec->op != DHT_NIL )
        {
          drec = next_record(drec);
          n++;
        }
      if (temp_drec->ndim != n)
        error(NIL,"Malformed DHDOC: incorrect number of list elements", NIL);
      return (temp_drec->end = drec+1);
      
      /* DHDOC */

    case DHT_FUNC:
      n = 0;
      temp_drec = drec;
      drec++;
      while( drec->op != DHT_TEMPS && 
             drec->op != DHT_RETURN &&
             drec->op != DHT_END_FUNC &&
             drec->op != DHT_NIL )
        {
          drec = next_record(drec);
          n++;
        }
      /* keep a pointer at the end or arguments handy */      
      temp_drec->name = (char*)drec; 
      if(temp_drec->ndim != n) 
        {
          temp_drec->name = 0;
          error(NIL, "Malformed DHDOC: "
                "function has wrong number of arguments", NIL);
	}
      if(drec->op == DHT_TEMPS) 
        drec = next_record(drec);
      if(drec->op != DHT_RETURN)
        error(NIL, "Malformed DHDOC: "
              "function has no return value", NIL);
      drec = next_record(drec);
      if(drec->op != DHT_END_FUNC) 
        error(NIL, "Malformed DHDOC: "
              "function is not terminated", NIL);
      return (temp_drec->end = drec+1);
      
    case DHT_TEMPS:
      n = 0;
      temp_drec = drec;
      drec++;
      while( drec->op != DHT_END_TEMPS &&
             drec->op != DHT_NIL )
        {
          int m;
          switch(drec->op) 
            {
            case DHT_STR:
            case DHT_OBJ:
              drec = next_record(drec);
              break;
            case DHT_IDX:
            case DHT_SRG:
              drec->end = drec+2;
              drec++;
              drec->end = drec+1;
              drec++;
              break;
            case DHT_LIST:
              m = drec->ndim;
              drec->end = drec + m + 2;
              drec ++;
              for(i = 0; i < m; i++)
                {
                  drec->end = drec+1;
                  drec++;
                }
              if (drec->op != DHT_END_LIST)
                error(NIL,"Malformed DHDOC: "
                      "incorrect list type in temporary arguments", NIL);
              drec->end = drec+1;
              drec ++;
              break;
            default:
              error(NIL, "Malformed DHDOC: "
                    "incorrect type for function temporary arguments", NIL);
	    }
          n++;
	}
      if(temp_drec->ndim != n)
        error(NIL,"Malformed DHDOC: "
              "function has wrong number of temporary arguments", NIL);
      return (temp_drec->end = drec+1);
      
    case DHT_RETURN:	
      temp_drec = drec;
      return (temp_drec->end = next_record(drec+1));
      
      /* CLASSDOC */
      
    case DHT_CLASS:
      n = 0;
      temp_drec = drec;
      drec++;
      while (drec->op == DHT_NAME)
        {
          drec = next_record(drec);
          n++;
        }
      if (temp_drec->ndim != n)
        error(NIL,"Malformed CLASSDOC: "
              "bad number of slots in class description", NIL);
      while (drec->op == DHT_METHOD)
        {
          drec = next_record(drec);
          n++;
        }
      if (drec->op != DHT_END_CLASS)
        error(NIL, "Malformed CLASSDOC: "
              "class description contains unrecognized records", NIL);
      return (temp_drec->end = drec+1);
      
    case DHT_NAME:
      temp_drec = drec;
      return (temp_drec->end = next_record(drec+1));
      
    case DHT_METHOD:
    case DHT_REFER:
    case DHT_NIL:
      return (drec->end = drec + 1);

    default:
      error(NIL, "Malformed DHDOC or CLASSDOC: "
            " unrecognized record type", NIL);
    }
}





/* ---------------------------------------------
   DH FUNCTIONS
   --------------------------------------------- */


/* from function.c */
extern struct alloc_root cfunc_alloc;
extern void cfunc_dispose(at*);
extern void cfunc_action(at*, void (*)(at*));
extern char *cfunc_name(at*);


/* from lisp_c.c */
extern at *dh_listeval(at*, at*);  


/* dh class */
class dh_class = {
  cfunc_dispose,
  cfunc_action,
  cfunc_name,
  generic_eval,
  dh_listeval,
};


/* create a new dh function */
at *
new_dh(at *name, dhdoc_t *kdata)
{
  at *p;
  struct cfunction *cfunc;
  /* update next record pointer in dhdoc */
  if (kdata->argdata->op != DHT_FUNC)
    error(NIL,"Malformed DHDOC: "
          "Expecting a function record", NIL);
  next_record(kdata->argdata);
  /* construct lisp object */
  cfunc = allocate(&cfunc_alloc);
  cfunc->call = kdata->lispdata.call;
  cfunc->info = kdata;
  cfunc->kname = strdup(kdata->lispdata.k_name);
  LOCK(name);
  cfunc->name = name;
  p = new_extern(&dh_class, cfunc);
  p->flags |= X_FUNCTION;
  return p;
}




/* ---------------------------------------------
   DH CLASSES
   --------------------------------------------- */


static at *
make_dhclass(dhclassdoc_t *kdata)
{
  at *p;
  at *classname;
  at *superclass;
  at *keylist;
  at *defaults;
  at **klwhere;
  class *cl;
  dhrecord *drec;
  at *superlock;

  if (!kdata)
    {
      return 0;
    }
  else if ((p = kdata->lispdata.atclass))
    {
      LOCK(p);
      return p;
    }
  else
    {
      /* Make sure superclass is okay */
      superlock = 0;
      if (kdata->lispdata.ksuper)
        superlock = make_dhclass(kdata->lispdata.ksuper);
      /* fixup classdoc pointer in vtable */
      ((dhclassdoc_t**)(kdata->lispdata.vtable))[0] = kdata;
      /* update next record pointer in dhdoc */
      if (kdata->argdata->op != DHT_CLASS)
        error(NIL,"Malformed CLASSDOC: "
              "Expecting a class record", NIL);
      next_record(kdata->argdata);
      for (drec = kdata->argdata->end;
           drec && drec->op == DHT_METHOD;
           drec = drec->end)
        next_record(drec);
      /* create field lists */
      keylist = defaults = NIL;
      klwhere = &keylist;
      drec = kdata->argdata + 1;
      while (drec->op == DHT_NAME)
        {
          defaults = cons(NIL,defaults);
          *klwhere = cons(named(drec->name),NIL);
          klwhere = &((*klwhere)->Cdr);
          drec = drec->end;
        }
      /* create class object */
      classname = named(kdata->lispdata.lname);
      superclass = NIL;
      if (kdata->lispdata.ksuper)
        superclass = kdata->lispdata.ksuper->lispdata.atclass;
      if (!superclass)
        superclass = object_class.backptr;
      /* create ooclass */
      p = new_ooclass(classname, superclass, keylist, defaults);
      UNLOCK(classname);
      UNLOCK(keylist);
      UNLOCK(defaults);
      UNLOCK(superlock);
      /* store */
      cl = p->Object;
      cl->classdoc = kdata;
      cl->kname = strdup(kdata->lispdata.k_name);
      kdata->lispdata.atclass = p;
      return p;
    }
  
}


LUSHAPI at *
new_dhclass(at *name, dhclassdoc_t *kdata)
{
  at *p;
  if (strcmp(nameof(name), kdata->lispdata.lname))
    error(NIL, "Incorrect CLASSDOC: "
          "class name does not match dhclass_define", NIL);    
  if (! (p = make_dhclass(kdata)))
    error(NIL, "Internal error: make_dhclass failed", NIL);
  return p;
}



/* ---------------------------------------------
   PRIMITIVES
   --------------------------------------------- */


/* Remove extra chars added by enclose_in_string */
static char *
strclean(char *s)
{
  if (s != 0)
    while (*s && (*s==' ' || *s=='\t' || *s=='\n' || *s=='\r'))
      s += 1;
  return s;
}

/* Forward */
static at *dhinfo_record(dhrecord *drec);

/* Catenate records */
static at *
dhinfo_chain(dhrecord *drec, int n, at*(*f)(dhrecord*))
{
  at *ans = NIL;
  at **where = &ans;
  while (n-->0) 
    {
      *(where) = cons((*f)(drec), NIL);
      where = &((*where)->Cdr);
      drec = drec->end;
    }
  return ans;
}

/* Process subrecord of a temporary list */
static at *
dhinfo_record_templist(dhrecord *drec)
{
  at *p;
  int i;
  switch (drec->op) 
    {
    case DHT_LIST:
      p = NIL;
      for (i=0; i<drec->ndim; i++)
        p = cons(cons(named("unk"),NIL),p);
      return cons(named("list"), p);
    case DHT_SRG:
      p = cons(named("unk"), NIL);
      p = cons(named("srg"), cons(named("w"), cons(p, NIL)));
      return cons(named("ptr"), cons(p, NIL));
    case DHT_IDX:
      p = cons(named("unk"), NIL);
      p = cons(named("srg"), cons(named("w"), cons(p, NIL)));
      p = cons(NEW_NUMBER(drec->ndim), cons(p, NIL));
      p = cons(named("idx"), cons(named("w"), p));
      return cons(named("ptr"), cons(p, NIL));
    default:
      return dhinfo_record(drec);
    }
}

/* Process records describing temporary arguments */
static at *
dhinfo_record_temp(dhrecord *drec)
{
  at *p;
  dhclassdoc_t *cdoc;
  switch(drec->op)
    {
    case DHT_LIST:
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record_templist);
      return cons(named("list"), p);
    case DHT_IDX:
      p = dhinfo_record(drec+1);
      p = cons(named("srg"), cons(named("w"), cons(p, NIL)));
      p = cons(NEW_NUMBER(drec->ndim), cons(p, NIL));
      return cons(named("idx"), cons(named("w"), p));
    case DHT_OBJ: 
      cdoc = drec->arg;
      return cons( named("obj"),
                   cons( named(strclean(cdoc->lispdata.lname)), 
                         new_cons(cdoc->lispdata.atclass, 
                                  NIL ) ) );
    case DHT_SRG: 
      return cons(named("srg"), 
                  cons(named((drec[1].access == DHT_READ) ? "r" : "w"),
                       cons(dhinfo_record(drec+1), 
                            NIL) ) );
    case DHT_STR:
      return cons(named("str"), NIL);
    default:
      error(NIL, "Malformed dhdoc: "
            "temporary argument type cannot be pointers", NIL);
    }
}

/* Process generic records */
static at *
dhinfo_record(dhrecord *drec)
{
  at *p, *q;
  dhrecord *drec1;
  dhclassdoc_t *cdoc;
  switch (drec->op) 
    {
    case DHT_BOOL: 
      return cons(named("bool"),NIL);
    case DHT_BYTE: 
      return cons(named("byte"),NIL);
    case DHT_UBYTE: 
      return cons(named("ubyte"),NIL);
    case DHT_SHORT: 
      return cons(named("short"),NIL);
    case DHT_INT: 
      return cons(named("int"),NIL);
    case DHT_FLT: 
      return cons(named("flt"),NIL);
    case DHT_REAL: 
      return cons(named("real"),NIL);
    case DHT_NIL: 
      return cons(named("bool"),NIL);

    case DHT_GPTR:
      p = NIL;
      if (drec->name)
        p = cons(new_string(strclean(drec->name)),NIL);
      return cons(named("gptr"), p);
      
    case DHT_STR: 
      p = cons(named("str"),NIL);
      return cons(named("ptr"), cons(p, NIL));
      
    case DHT_LIST:
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      p = cons(named("list"), p);
      return cons(named("ptr"), cons(p, NIL));
      
    case DHT_OBJ: 
      cdoc = drec->arg;
      p = cons( named("obj"),
                cons( named(strclean(cdoc->lispdata.lname)), 
                      new_cons(cdoc->lispdata.atclass, 
                               NIL ) ) );
      return cons(named("ptr"), cons(p, NIL));
      
    case DHT_SRG: 
      p = cons(named("srg"), 
               cons(named((drec[1].access == DHT_READ) ? "r" : "w"),
                    cons(dhinfo_record(drec+1), 
                         NIL) ) );
      return cons(named("ptr"), cons(p, NIL));
      
    case DHT_IDX:
      drec1 = drec + 1;
      p = cons(named("srg"), 
               cons(named((drec1->access == DHT_READ) ? "r" : "w"),
                    cons(dhinfo_record(drec1 + 1), 
                         NIL) ) );
      p = cons(named("idx"),
               cons(named((drec->access == DHT_READ) ? "r" : "w"),
                    cons(NEW_NUMBER(drec->ndim),
                         cons(p, NIL) ) ) );
      return cons(named("ptr"), cons(p, NIL));
      
    case DHT_FUNC:
      if (! drec->name)
        next_record(drec);
      drec1 = (dhrecord*) drec->name;
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      q = NIL;
      if (drec1->op == DHT_TEMPS) 
        {
          q = dhinfo_chain(drec1+1, drec1->ndim, dhinfo_record_temp);
          drec1 = drec1->end;
        }
      p = cons(p, cons(q, cons(dhinfo_record(drec1 + 1), NIL)));
      return cons(named("func"), p);
      
    case DHT_NAME:
      p = dhinfo_record(drec+1);
      return cons(named(strclean(drec->name)), cons(p,NIL));

    case DHT_METHOD:
      p = dhinfo_record( ((dhdoc_t*)(drec->arg))->argdata );
      return cons(new_string(strclean(drec->name)), cons(p,NIL));

    case DHT_CLASS:
      cdoc = drec->arg;
      drec1 = drec+1;
      p = dhinfo_chain(drec1, drec->ndim, dhinfo_record);
      drec1 = drec->end;
      q = dhinfo_chain(drec1, cdoc->lispdata.nmet, dhinfo_record);
      p = cons(p, cons( q, NIL));
      q = NIL;
      if (cdoc->lispdata.ksuper)
        q = new_string(strclean(cdoc->lispdata.ksuper->lispdata.cname));
      return cons(new_string(strclean(cdoc->lispdata.cname)),
                  cons(q, p));
    default:
      return cons(named("unk"), NIL);
    }
}


DX(xdhinfo_t)
{
  at *p;
  struct cfunction *cfunc;
  dhdoc_t *dhdoc;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &dh_class))
    error(NIL,"Not a DH function", p);
  cfunc = p->Object;
  if (CONSP(cfunc->name))
    check_primitive(cfunc->name, cfunc->info);
  dhdoc = (dhdoc_t*)(cfunc->info);
  if (! dhdoc)
    error(NIL,"Internal error: dhdoc unvailable",NIL);
  return dhinfo_record(dhdoc->argdata);
}


DX(xdhinfo_c)
{
  at *p;
  struct cfunction *cfunc;
  dhdoc_t *dhdoc;
  at *cname, *mname, *kname;
  at *ctest, *mtest;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &dh_class))
    error(NIL,"Not a DH function", p);
  cfunc = p->Object;
  if (CONSP(cfunc->name))
    check_primitive(cfunc->name, cfunc->info);
  dhdoc = (dhdoc_t*)(cfunc->info);
  if (! dhdoc)
    error(NIL,"Internal error: dhdoc unvailable",NIL);
  /* collect */
  cname = new_string(strclean(dhdoc->lispdata.c_name));
  mname = new_string(strclean(dhdoc->lispdata.m_name));
  kname = new_string(strclean(dhdoc->lispdata.k_name));
  if (dhdoc->lispdata.dhtest)
    {
      dhdoc_t *dhtest = dhdoc->lispdata.dhtest;
      ctest = new_string(strclean(dhtest->lispdata.c_name));
      mtest = new_string(strclean(dhtest->lispdata.m_name));
    }
  else
    {
      ctest = new_safe_string("");
      mtest = new_safe_string("");
    }
  /* build */
  p = cons(ctest, cons(mtest, cons(kname, NIL)));
  return cons(cname, cons(mname, p));
}


DX(xclassinfo_t)
{
  at *p;
  class *cl;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &class_class))
    error(NIL,"not a class",p);
  cl = p->Object;
  if (!cl->classdoc)
    return NIL;
  if (CONSP(cl->priminame))
    check_primitive(cl->priminame, cl->classdoc);
  return dhinfo_record(cl->classdoc->argdata);
}


DX(xclassinfo_c)
{
  at *p;
  class *cl;
  dhclassdoc_t *cdoc;
  at *cname, *vname, *kname;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &class_class))
    error(NIL,"not a class",p);
  cl = p->Object;
  if (!(cdoc = cl->classdoc))
    error(NIL,"class is not compiled", p);
  if (CONSP(cl->priminame))
    check_primitive(cl->priminame, cl->classdoc);
  cname = new_string(strclean(cdoc->lispdata.cname));
  kname = new_string(strclean(cdoc->lispdata.k_name));
  vname = new_string(strclean(cdoc->lispdata.v_name));
  return cons(cname, cons(kname, cons(vname, NIL)));
}



/* ---------------------------------------------
   CLASSDOC FOR OBJECT CLASS
   --------------------------------------------- */

dhclassdoc_t Kc_object;

static void 
Cdestroy_C_object(gptr p)
{
}

struct VClass_object Vt_object = { 
  (void*)&Kc_object, 
  &Cdestroy_C_object,
};

DHCLASSDOC(Kc_object, NULL, object, "object", Vt_object, 0) =
{ 
  DH_CLASS(0, Kc_object), 
  DH_END_CLASS, 
  DH_NIL 
};



/* ---------------------------------------------
   INITIALIZATION
   --------------------------------------------- */

void 
init_dh(void)
{
  /* setup object class */
  object_class.classdoc = &Kc_object;
  Kc_object.lispdata.atclass = object_class.backptr;
  /* primitives */
  class_define("DH", &dh_class);
  dx_define("dhinfo-t",xdhinfo_t);
  dx_define("dhinfo-c",xdhinfo_c);
  dx_define("classinfo-t",xclassinfo_t);
  dx_define("classinfo-c",xclassinfo_c);
}

