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
 * $Id: dh.c,v 1.20 2005/01/17 18:23:25 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "dh.h"


/* ---------------------------------------------
   UTILITIES
   --------------------------------------------- */


/* next_record --
   complete next record pointer in dhdoc. */

static dhrecord *next_record(dhrecord *drec)
{
   dhrecord *temp_drec;
   int n;
  
   switch(drec->op) {
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
      while (drec->op != DHT_END_LIST) {
         /* && drec->op != DHT_NIL ) */
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
             drec->op != DHT_NIL ) {
         drec = next_record(drec);
         n++;
      }
      /* keep a pointer at the end or arguments handy */      
      temp_drec->name = (char*)drec; 
      if(temp_drec->ndim != n) {
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
             drec->op != DHT_NIL ) {
         int m;
         switch(drec->op) {
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
            for(int i = 0; i < m; i++) {
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
      while (drec->op == DHT_NAME) {
         drec = next_record(drec);
         n++;
      }
      if (temp_drec->ndim != n)
         error(NIL,"Malformed CLASSDOC: "
               "bad number of slots in class description", NIL);
      if (drec->op != DHT_END_CLASS)
         error(NIL, "Malformed CLASSDOC: "
               "class description contains unrecognized records", NIL);
      drec++;
      temp_drec->end = drec;
      temp_drec = drec;
      while (drec->op == DHT_METHOD)
         drec = next_record(drec);
      return temp_drec;
      
   case DHT_NAME:
      temp_drec = drec;
      return (temp_drec->end = next_record(drec+1));
      
   case DHT_METHOD:
   case DHT_REFER:
   case DHT_NIL:
      return (drec->end = drec + 1);
      
   default:
      error(NIL, "malformed DHDOC or CLASSDOC: "
            " unrecognized record type", NIL);
   }
}

void clean_dhrecord(dhrecord *drec)
{
   if (drec)
      next_record(drec);
}


/* ---------------------------------------------
   DH FUNCTIONS
   --------------------------------------------- */


/* from function.c */
extern mt_t mt_cfunction;
extern char *cfunc_name(at*);

/* from lisp_c.c */
extern at *dh_listeval(at*, at*);  


/* create a new dh function */
at *new_dh(at *name, dhdoc_t *kdata)
{
   /* update next record pointer in dhdoc */
   if (kdata->argdata->op != DHT_FUNC)
      error(NIL,"malformed DHDOC: "
            "Expecting a function record", NIL);
   next_record(kdata->argdata);
   /* construct lisp object */
   cfunction_t *cfunc = mm_alloc(mt_cfunction);
   cfunc->call = kdata->lispdata.call;
   cfunc->info = kdata;
   cfunc->kname = mm_strdup(kdata->lispdata.k_name);
   cfunc->name = name;
   return new_extern(&dh_class, cfunc);
}




/* ---------------------------------------------
   DH CLASSES
   --------------------------------------------- */


static at *make_dhclass(dhclassdoc_t *kdata)
{
   if (!kdata) 
      return 0;
   else if (kdata->lispdata.atclass)
      return kdata->lispdata.atclass;
   
   /* Make sure superclass is okay */
   at *superlock = 0;
   if (kdata->lispdata.ksuper)
      superlock = make_dhclass(kdata->lispdata.ksuper);

   /* fixup classdoc pointer in vtable */
   ((dhclassdoc_t**)(kdata->lispdata.vtable))[0] = kdata;

   /* update next record pointer in dhdoc */
   if (kdata->argdata->op != DHT_CLASS)
      error(NIL,"malformed CLASSDOC: "
            "Expecting a class record", NIL);
   next_record(kdata->argdata);

   /* create field lists */
   at *defaults = NIL;
   at *keylist = NIL;
   at **klwhere = &keylist;
   dhrecord *drec = kdata->argdata + 1;
   while (drec->op == DHT_NAME) {
      defaults = new_cons(NIL,defaults);
      *klwhere = new_cons(named(drec->name),NIL);
      klwhere = &Cdr(*klwhere);
      drec = drec->end;
   }

   /* create class object */
   at *classname = named(kdata->lispdata.lname);
   at *superclass = NIL;
   if (kdata->lispdata.ksuper)
      superclass = kdata->lispdata.ksuper->lispdata.atclass;
   if (!superclass)
      superclass = object_class->backptr;
   /* create ooclass */
   at *p = new_ooclass(classname, superclass, keylist, defaults);
   class_t *cl = Mptr(p);
   cl->classdoc = kdata;
   cl->kname = mm_strdup(kdata->lispdata.k_name);
   kdata->lispdata.atclass = p;
   return p;
}


LUSHAPI at *new_dhclass(at *name, dhclassdoc_t *kdata)
{
   if (strcmp(NAMEOF(name), kdata->lispdata.lname))
      error(NIL, "incorrect CLASSDOC: "
            "class name does not match dhclass_define", NIL);    

   at *p = make_dhclass(kdata);
   if (!p)
      error(NIL, "internal error: make_dhclass failed", NIL);
   return p;
}



/* ---------------------------------------------
   PRIMITIVES
   --------------------------------------------- */


/* Remove extra chars added by enclose_in_string */
static const char *strclean(const char *s)
{
   if (s != 0)
      while (*s && (*s==' ' || *s=='\t' || *s=='\n' || *s=='\r'))
         s += 1;
   return s;
}

/* Catenate records */
static at *dhinfo_chain(dhrecord *drec, int n, at*(*f)(dhrecord*))
{
   at *ans = NIL;
   at **where = &ans;
   while (n-->0) {
      *(where) = new_cons((*f)(drec), NIL);
      where = &Cdr(*where);
      drec = drec->end;
    }
   return ans;
}

static at *dhinfo_record(dhrecord *drec);

/* Process subrecord of a temporary list */
static at *dhinfo_record_templist(dhrecord *drec)
{
   at *p = NIL;
   switch (drec->op) {
   case DHT_LIST:
      for (int i=0; i<drec->ndim; i++)
         p = new_cons(new_cons(named("unk"),NIL),p);
      return new_cons(named("list"), p);

    case DHT_SRG:
      p = new_cons(named("unk"), NIL);
      p = new_cons(named("srg"), new_cons(named("w"), new_cons(p, NIL)));
      return new_cons(named("ptr"), new_cons(p, NIL));

   case DHT_IDX:
      p = new_cons(named("unk"), NIL);
      p = new_cons(named("srg"), new_cons(named("w"), new_cons(p, NIL)));
      p = new_cons(NEW_NUMBER(drec->ndim), new_cons(p, NIL));
      p = new_cons(named("idx"), new_cons(named("w"), p));
      return new_cons(named("ptr"), new_cons(p, NIL));
   default:
      return dhinfo_record(drec);
   }
}

/* Process records describing temporary arguments */
static at *dhinfo_record_temp(dhrecord *drec)
{
   at *p;
   dhclassdoc_t *cdoc;
   switch(drec->op) {
   case DHT_LIST:
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record_templist);
      return new_cons(named("list"), p);
   
   case DHT_IDX:
      p = dhinfo_record(drec+1);
      p = new_cons(named("srg"), new_cons(named("w"), new_cons(p, NIL)));
      p = new_cons(NEW_NUMBER(drec->ndim), new_cons(p, NIL));
      return new_cons(named("idx"), new_cons(named("w"), p));
      
   case DHT_OBJ: 
      cdoc = drec->arg;
      return new_cons(named("obj"),
                      new_cons( named(strclean(cdoc->lispdata.lname)),
                                new_cons(cdoc->lispdata.atclass,
                                         NIL)));
   case DHT_SRG: 
      return new_cons(named("srg"), 
                      new_cons(named((drec[1].access == DHT_READ) ? "r" : "w"),
                               new_cons(dhinfo_record(drec+1), 
                                        NIL)));
   case DHT_STR:
      return new_cons(named("str"), NIL);
   default:
      error(NIL, "malformed dhdoc: "
            "temporary argument type cannot be pointers", NIL);
   }
}

/* Process generic records */
static at *dhinfo_record(dhrecord *drec)
{
   at *p;
   dhrecord *drec1;
   dhclassdoc_t *cdoc;
   switch (drec->op) {
   case DHT_BOOL: 
      return new_cons(named("bool"),NIL);

   case DHT_BYTE: 
      return new_cons(named("byte"),NIL);

   case DHT_UBYTE: 
      return new_cons(named("ubyte"),NIL);

   case DHT_SHORT: 
      return new_cons(named("short"),NIL);

   case DHT_INT: 
      return new_cons(named("int"),NIL);

   case DHT_FLT: 
      return new_cons(named("flt"),NIL);

   case DHT_REAL: 
      return new_cons(named("real"),NIL);
      
   case DHT_STR: 
      return new_cons(named("str"),NIL);

   case DHT_NIL: 
      return new_cons(named("bool"),NIL);
      
   case DHT_GPTR:
      p = NIL;
      if (drec->name)
         p = new_cons(new_string(strclean(drec->name)),NIL);
      return new_cons(named("gptr"), p);
      
   case DHT_LIST:
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      p = new_cons(named("list"), p);
      return new_cons(named("ptr"), new_cons(p, NIL));
      
   case DHT_OBJ: 
      cdoc = drec->arg;
      p = new_cons(named("obj"),
                   new_cons(named(strclean(cdoc->lispdata.lname)), 
                            new_cons(cdoc->lispdata.atclass, 
                                     NIL)));
      return new_cons(named("ptr"), new_cons(p, NIL));
      
   case DHT_SRG: 
      p = new_cons(named("srg"), 
                   new_cons(named((drec[1].access == DHT_READ) ? "r" : "w"),
                            new_cons(dhinfo_record(drec+1), 
                                     NIL)));
      return new_cons(named("ptr"), new_cons(p, NIL));
      
    case DHT_IDX:
       drec1 = drec + 1;
       p = new_cons(named("srg"), 
                    new_cons(named((drec1->access == DHT_READ) ? "r" : "w"),
                             new_cons(dhinfo_record(drec1 + 1), 
                                      NIL)));
       p = new_cons(named("idx"),
                    new_cons(named((drec->access == DHT_READ) ? "r" : "w"),
                             new_cons(NEW_NUMBER(drec->ndim),
                                      new_cons(p, NIL))));
       return new_cons(named("ptr"), new_cons(p, NIL));
       
   case DHT_FUNC:
      if (! drec->name)
         next_record(drec);
      dhrecord *drec1 = (dhrecord*) drec->name;
      p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      at *q = NIL;
      if (drec1->op == DHT_TEMPS) {
         q = dhinfo_chain(drec1+1, drec1->ndim, dhinfo_record_temp);
         drec1 = drec1->end;
      }
      p = new_cons(p, new_cons(q, new_cons(dhinfo_record(drec1 + 1), NIL)));
      return new_cons(named("func"), p);
      
   case DHT_NAME:
      p = dhinfo_record(drec+1);
      return new_cons(named(strclean(drec->name)), new_cons(p,NIL));
      
   case DHT_METHOD:
      p = dhinfo_record( ((dhdoc_t*)(drec->arg))->argdata );
      return new_cons(new_string(strclean(drec->name)), new_cons(p,NIL));

   case DHT_CLASS:
      cdoc = drec->arg;
      drec1 = drec+1;
      p = dhinfo_chain(drec1, drec->ndim, dhinfo_record);
      drec1 = drec->end;
      q = dhinfo_chain(drec1, cdoc->lispdata.nmet, dhinfo_record);
      p = new_cons(p, new_cons( q, NIL));
      q = NIL;
      if (cdoc->lispdata.ksuper)
         q = new_string(strclean(cdoc->lispdata.ksuper->lispdata.cname));
      return new_cons(new_string(strclean(cdoc->lispdata.cname)),
                      new_cons(q, p));
   default:
      return new_cons(named("unk"), NIL);
   }
}


DX(xdhinfo_t)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);

   at *p = APOINTER(1);
   ifn (p && (Class(p) == &dh_class))
      error(NIL, "not a DH function", p);
   cfunction_t *cfunc = Mptr(p);
   if (CONSP(cfunc->name))
      check_primitive(cfunc->name, cfunc->info);
   dhdoc_t *dhdoc = (dhdoc_t*)(cfunc->info);
   if (! dhdoc)
      error(NIL, "internal error: dhdoc unvailable", NIL);
   return dhinfo_record(dhdoc->argdata);
}


DX(xdhinfo_c)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (p && (Class(p) == &dh_class))
      error(NIL, "not a DH function", p);

   cfunction_t *cfunc = Mptr(p);
   if (CONSP(cfunc->name))
      check_primitive(cfunc->name, cfunc->info);
   dhdoc_t *dhdoc = (dhdoc_t*)(cfunc->info);
   if (! dhdoc)
      error(NIL, "internal error: dhdoc unvailable", NIL);

   /* collect */
   at *cname = new_string(strclean(dhdoc->lispdata.c_name));
   at *mname = new_string(strclean(dhdoc->lispdata.m_name));
   at *kname = new_string(strclean(dhdoc->lispdata.k_name));
   at *ctest, *mtest;
   if (dhdoc->lispdata.dhtest) {
      dhdoc_t *dhtest = dhdoc->lispdata.dhtest;
      ctest = new_string(strclean(dhtest->lispdata.c_name));
      mtest = new_string(strclean(dhtest->lispdata.m_name));
   } else {
      ctest = null_string;
      mtest = null_string;
   }
   /* build */
   p = new_cons(ctest, new_cons(mtest, new_cons(kname, NIL)));
   return new_cons(cname, new_cons(mname, p));
}


DX(xclassinfo_t)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  at *p = APOINTER(1);
  ifn (CLASSP(p))
     error(NIL,"not a class",p);

  class_t *cl = Mptr(p);
  if (!cl->classdoc)
     return NIL;
  if (CONSP(cl->priminame))
     check_primitive(cl->priminame, cl->classdoc);
  return dhinfo_record(cl->classdoc->argdata);
}


DX(xclassinfo_c)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   ifn (CLASSP(p))
      error(NIL, "not a class", p);
   
   class_t *cl = Mptr(p);
   dhclassdoc_t *cdoc = cl->classdoc;
   if (!cdoc)
      error(NIL, "class is not compiled", p);
   if (CONSP(cl->priminame))
      check_primitive(cl->priminame, cl->classdoc);
   at *cname = new_string(strclean(cdoc->lispdata.cname));
   at *kname = new_string(strclean(cdoc->lispdata.k_name));
   at *vname = new_string(strclean(cdoc->lispdata.v_name));
   return new_cons(cname, new_cons(kname, new_cons(vname, NIL)));
}



/* ---------------------------------------------
   CLASSDOC FOR OBJECT CLASS
   --------------------------------------------- */

dhclassdoc_t Kc_object;

static void Cdestroy_C_object(gptr p)
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

class_t dh_class;

void init_dh(void)
{
   /* setup object class */
   object_class->classdoc = &Kc_object;
   Kc_object.lispdata.atclass = object_class->backptr;

   /* setting up dh_class */
   class_init(&dh_class, false);
   dh_class.name = cfunc_name;
   dh_class.listeval = dh_listeval;
   dh_class.super = &function_class;
   dh_class.atsuper = function_class.backptr;
   class_define("DH", &dh_class);

   /* primitives */
   dx_define("dhinfo-t",xdhinfo_t);
   dx_define("dhinfo-c",xdhinfo_c);
   dx_define("classinfo-t",xclassinfo_t);
   dx_define("classinfo-c",xclassinfo_c);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
