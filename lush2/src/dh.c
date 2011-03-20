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

/* ---------------------------------------------
   UTILITIES
   --------------------------------------------- */

#define SIZEOF_COBJECT (sizeof(struct CClass_object) + MIN_NUM_SLOTS*sizeof(mptr))
#define SIZEOF_COBJECT2 (sizeof(struct CClass_object) + 2*MIN_NUM_SLOTS*sizeof(mptr))


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
   case DHT_FLOAT:
   case DHT_DOUBLE:
   case DHT_GPTR:
   case DHT_MPTR:
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

/* create a new dh function */
at *new_dh(at *name, dhdoc_t *kdata)
{
   /* from function.c */
   extern mt_t mt_cfunction;

   /* update next record pointer in dhdoc */
   if (kdata->argdata->op != DHT_FUNC)
      error(NIL,"malformed DHDOC: "
            "Expecting a function record", NIL);
   next_record(kdata->argdata);
   /* construct lisp object */
   cfunction_t *cfunc = mm_alloc(mt_cfunction);
   cfunc->call = (void *(*)())kdata->lispdata.call;
   cfunc->info = kdata;
   cfunc->kname = mm_strdup(kdata->lispdata.k_name);
   cfunc->name = name;
   return new_at(dh_class, cfunc);
}


int dht_from_cname(symbol_t *s)
{
   const char *name = nameof(s);
   ifn (strlen(name)>2)
      goto invalid_error;

   switch (tolower(name[0])) {
   case 'd': 
      if (strcmp(name, "double"))
         goto invalid_error;
      else
         return DHT_DOUBLE;
      
   case 'f':
      if (strcmp(name, "float"))
         goto invalid_error;
      else
         return DHT_FLOAT;

   case 'b':
      if (strcmp(name, "bool")==0)
         return DHT_BOOL;
      else if (strcmp(name, "byte")==0) {
         fprintf(stderr, "*** Warning: deprecated name 'byte' (use 'char')\n");
         return DHT_CHAR;
      } else
         goto invalid_error;

   case 'c':
      if (strcmp(name, "char")==0)
         return DHT_CHAR;
      else if (strcmp(name, "class")==0)
         return DHT_CLASS;
      else
         goto invalid_error;
      
   case 'i':
      if (strcmp(name, "int")==0)
         return DHT_INT;
      else if (strcmp(name, "idx")==0)
         return DHT_IDX;
      else if (strcmp(name, "index")==0)
         return DHT_INDEX;
      else
         goto invalid_error;
                 
   case 's': 
      if (strcmp(name, "str")==0)
         return DHT_STR;
      else if (strcmp(name, "short")==0)
         return DHT_SHORT;
      else if (strcmp(name, "srg")==0)
         return DHT_STORAGE;
      else if (strcmp(name, "storage")==0)
         return DHT_STORAGE;
      else
         goto invalid_error;

   case 'u':
      if (strcmp(name, "uchar")==0)
         return DHT_UCHAR;
      else if (strcmp(name, "ubyte")==0) {
         fprintf(stderr, "*** Warning: deprecated name 'ubyte' (use 'uchar')\n");
         return DHT_UCHAR;
      } else
         goto invalid_error;
      
   case 'g': 
      if (strcmp(name, "gptr"))
         goto invalid_error;
      else
         return DHT_GPTR;

   case 'o':
      if (strcmp(name, "obj")==0)
         return DHT_OBJECT;
      else if (strcmp(name, "object")==0)
         return DHT_OBJECT;
      else
         goto invalid_error;
   }
invalid_error:
   error(NIL, "not a type name", named(name));
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
   class_t *cl = new_ooclass(classname, superclass, keylist, defaults);
   cl->has_compiled_part = true;
   cl->classdoc = kdata;
   cl->kname = mm_strdup(kdata->lispdata.k_name);
   cl->num_cslots += length(keylist);
   kdata->lispdata.atclass = cl->backptr;
   return cl->backptr;
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
   switch (drec->op) {
   case DHT_BOOL: 
      return new_cons(named("bool"),NIL);

   case DHT_CHAR: 
      return new_cons(named("char"),NIL);

   case DHT_UCHAR: 
      return new_cons(named("uchar"),NIL);

   case DHT_SHORT: 
      return new_cons(named("short"),NIL);

   case DHT_INT: 
      return new_cons(named("int"),NIL);

   case DHT_FLOAT: 
      return new_cons(named("float"),NIL);

   case DHT_DOUBLE: 
      return new_cons(named("double"),NIL);
      
   case DHT_STR: 
      return new_cons(named("str"),NIL);

   case DHT_NIL: 
      return new_cons(named("bool"),NIL);
      
   case DHT_GPTR:
   {
      at *p = NIL;
      if (drec->name)
         p = new_cons(make_string(strclean(drec->name)),NIL);
      return new_cons(named("gptr"), p);
   }
   case DHT_MPTR:
   {
      at *p = NIL;
      if (drec->name)
         p = new_cons(make_string(strclean(drec->name)),NIL);
      return new_cons(named("mptr"), p);
   }
   case DHT_LIST:
   {
      at *p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      p = new_cons(named("list"), p);
      return new_cons(named("ptr"), new_cons(p, NIL));
   }
   case DHT_OBJ:
   {
      dhclassdoc_t *cdoc = drec->arg;
      at *p = new_cons(named("obj"),
                   new_cons(named(strclean(cdoc->lispdata.lname)), 
                            new_cons(cdoc->lispdata.atclass, 
                                     NIL)));
      return new_cons(named("ptr"), new_cons(p, NIL));
   }
   case DHT_SRG:
   {
      at *p = new_cons(named("srg"), 
                   new_cons(named((drec[1].access == DHT_READ) ? "r" : "w"),
                            new_cons(dhinfo_record(drec+1), 
                                     NIL)));
      return new_cons(named("ptr"), new_cons(p, NIL));
   }
   case DHT_IDX:
   {
      dhrecord *drec1 = drec + 1;
      at *p = new_cons(named("srg"), 
                   new_cons(named((drec1->access == DHT_READ) ? "r" : "w"),
                            new_cons(dhinfo_record(drec1 + 1), 
                                     NIL)));
      p = new_cons(named("idx"),
                   new_cons(named((drec->access == DHT_READ) ? "r" : "w"),
                            new_cons(NEW_NUMBER(drec->ndim),
                                     new_cons(p, NIL))));
      return new_cons(named("ptr"), new_cons(p, NIL));
   }
   case DHT_FUNC:
   {
      if (! drec->name)
         next_record(drec);
      dhrecord *drec1 = (dhrecord*) drec->name;
      at *p = dhinfo_chain(drec+1, drec->ndim, dhinfo_record);
      at *q = NIL;
      if (drec1->op == DHT_TEMPS) {
         q = dhinfo_chain(drec1+1, drec1->ndim, dhinfo_record_temp);
         drec1 = drec1->end;
      }
      p = new_cons(p, new_cons(q, new_cons(dhinfo_record(drec1 + 1), NIL)));
      return new_cons(named("func"), p);
   }  
   case DHT_NAME:
   {
      at *p = dhinfo_record(drec+1);
      return new_cons(named(strclean(drec->name)), new_cons(p,NIL));
   }
   case DHT_METHOD:
   {
      at *p = dhinfo_record( ((dhdoc_t*)(drec->arg))->argdata );
      return new_cons(make_string(strclean(drec->name)), new_cons(p,NIL));
   }
   case DHT_CLASS:
   {
      dhclassdoc_t *cdoc = drec->arg;
      dhrecord *drec1 = drec+1;
      at *p = dhinfo_chain(drec1, drec->ndim, dhinfo_record);
      drec1 = drec->end;
      at *q = dhinfo_chain(drec1, cdoc->lispdata.nmet, dhinfo_record);
      p = new_cons(p, new_cons( q, NIL));
      q = NIL;
      if (cdoc->lispdata.ksuper)
         q = make_string(strclean(cdoc->lispdata.ksuper->lispdata.cname));
      return new_cons(make_string(strclean(cdoc->lispdata.cname)),
                      new_cons(q, p));
   }
   default:
      return new_cons(named("unk"), NIL);
   }
}

/* check that two dhrecords match */
static bool dhinfo_match(dhrecord *rec1, dhrecord *rec2)
{
   if (rec1 == rec2)
      return true;
   if (!rec1 || !rec2)
      return false;

   dhrecord *next1 = next_record(rec1);
   dhrecord *next2 = next_record(rec2);
   
   while (rec1 != next1 && rec2 != next2) {
      if (rec1->op != rec2->op)
         return false;
 
      switch (rec1->op) {

      case DHT_IDX:
         if (rec1->ndim != rec2->ndim)
            return false;
         if (rec1->access != rec2->access) // we might need to refine this
            return false;
         break;

      case DHT_OBJ:
      {
         dhclassdoc_t *cdoc1 = rec1->arg;
         dhclassdoc_t *cdoc2 = rec2->arg;
         while (cdoc2) {
            if (strcmp(cdoc1->lispdata.cname, cdoc2->lispdata.cname) != 0)
               return false;
            cdoc2 = cdoc2->lispdata.ksuper;
         }
         return false;
      }
      break;
      
      default:
         break;
      }

      rec1++;
      rec2++;
   }
   return true;
}


DX(xdhinfo_t)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   ifn (p && (Class(p) == dh_class))
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
   at *p = APOINTER(1);
   ifn (p && (Class(p) == dh_class))
      error(NIL, "not a DH function", p);

   cfunction_t *cfunc = Mptr(p);
   if (CONSP(cfunc->name))
      check_primitive(cfunc->name, cfunc->info);
   dhdoc_t *dhdoc = (dhdoc_t*)(cfunc->info);
   if (! dhdoc)
      error(NIL, "internal error: dhdoc unvailable", NIL);

   /* collect */
   at *cname = make_string(strclean(dhdoc->lispdata.c_name));
   at *mname = make_string(strclean(dhdoc->lispdata.m_name));
   at *kname = make_string(strclean(dhdoc->lispdata.k_name));
   at *ctest, *mtest;
   if (dhdoc->lispdata.dhtest) {
      dhdoc_t *dhtest = dhdoc->lispdata.dhtest;
      ctest = make_string(strclean(dhtest->lispdata.c_name));
      mtest = make_string(strclean(dhtest->lispdata.m_name));
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
   at *p = APOINTER(1);
   ifn (CLASSP(p))
      error(NIL, "not a class", p);
   
   class_t *cl = Mptr(p);
   dhclassdoc_t *cdoc = cl->classdoc;
   if (!cdoc)
      error(NIL, "class is not compiled", p);
   if (CONSP(cl->priminame))
      check_primitive(cl->priminame, cl->classdoc);
   at *cname = make_string(strclean(cdoc->lispdata.cname));
   at *kname = make_string(strclean(cdoc->lispdata.k_name));
   at *vname = make_string(strclean(cdoc->lispdata.v_name));
   return new_cons(cname, new_cons(kname, new_cons(vname, NIL)));
}


/* lush_error -- called by compiled code when an error occurs */

bool in_compiled_code;
static jmp_buf lush_error_jump;

void lush_error(const char *s)
{
   if (in_compiled_code) {
      printf("\n\n*** lush runtime error: %s\007\007\n",s);
      print_dh_trace_stack();
      dh_trace_root = 0;
      longjmp(lush_error_jump, -1);
      
   } else {
      dh_trace_root = 0;
      error(NIL, s, NIL);
   }
}

static inline void at_to_dharg(at *at_obj, dharg *arg, dhrecord *drec, at *errctx)
{
   switch(drec->op) {
   case DHT_FLOAT:
      if (NUMBERP(at_obj))
         arg->dh_float = (float)Number(at_obj); 
      else
         error(NIL, "invalid argument (float expected)",at_obj);
      break;

   case DHT_DOUBLE:
      if (NUMBERP(at_obj))
         arg->dh_double = Number(at_obj); 
      else
         error(NIL, "invalid argument (double expected)",at_obj);
      break;
  
   case DHT_INT:
      if (NUMBERP(at_obj) && (Number(at_obj) == (int)Number(at_obj)))
         arg->dh_int = (int)Number(at_obj);
      else
         error(NIL, "invalid argument (int expected)",at_obj);
      break;
      
   case DHT_SHORT:
      if (NUMBERP(at_obj) && (Number(at_obj) == (short)Number(at_obj)))
         arg->dh_short = (short)Number(at_obj); 
      else
         error(NIL, "invalid argument (short expected)",at_obj);
      break;
  
   case DHT_CHAR:
      if (NUMBERP(at_obj) && (Number(at_obj) == (char)(Number(at_obj))))
         arg->dh_char = (char)Number(at_obj); 
      else
         error(NIL, "invalid argument (char expected)",at_obj);
    break;
  
  case DHT_UCHAR:
     if (NUMBERP(at_obj) && (Number(at_obj) == (unsigned char)(Number(at_obj))) )
        arg->dh_uchar = (unsigned char)Number(at_obj); 
     else
        error(NIL, "invalid argument (uchar expected)",at_obj);
     break;
  
   case DHT_GPTR:
      if (GPTRP(at_obj))
         arg->dh_gptr = Gptr(at_obj);             
      else if (MPTRP(at_obj))
         arg->dh_gptr = Mptr(at_obj);
      else
         error(NIL, "invalid argument (gptr expected)",at_obj);
      break;
      
   case DHT_MPTR:
      if (MPTRP(at_obj))
         arg->dh_mptr = Mptr(at_obj);             
      else
         error(NIL, "invalid argument (mptr expected)",at_obj);
      break;
      
   case DHT_NIL:
      if (!at_obj)
         arg->dh_bool = false;
      else
         error(NIL, "invalid argument (nil expected)",at_obj);
      break;
  
   case DHT_BOOL:
      if (at_obj == at_t)
         arg->dh_bool = true;
      else if (at_obj == NIL)
         arg->dh_bool = false;
      else
         error(NIL, "invalid argument (bool expected)", at_obj);
      break;
  
   case DHT_SRG:
      /* Here we make an exception to the general pattern and  */
      /* accept a compatible index when a storage is expected. */
      /* For this to make sense we require that the index is   */
      /* contiguous and has a zero offset.                     */
   {
      storage_t *st = NULL;
      if (STORAGEP(at_obj)) 
         st = (storage_t *)Mptr(at_obj);
      else if (INDEXP(at_obj)) {
         index_t *ind = Mptr(at_obj);
         if (ind->offset)
            error(NIL, "invalid index for storage argument (nonzero offset)", at_obj);
         ifn (index_contiguousp(ind))
            error(NIL, "invalid index for storage argument (not contiguous)", at_obj);
         st = new_storage(ind->st->type);
         st->data = ind->st->data;
         st->size = index_nelems(ind); /* ! */
         st->kind = ind->st->kind;
         st->isreadonly = ind->st->isreadonly;
      }
      if (st) {
         if (st->type != (drec+1)->op)
            error(NIL, "invald storage argument (wrong type)", at_obj);
         if (drec->access == DHT_WRITE)
            get_write_permit(st);
         arg->dh_srg_ptr = st;
      } else
         error(NIL, "invalid argument (index or storage expected)",at_obj);
      break;
   }
  case DHT_IDX:
     if (INDEXP(at_obj)) {
        /* check type and access */
        index_t *ind = Mptr(at_obj);
        if (ind->ndim != drec->ndim)
           error(NIL, "invalid index argument (wrong number of dimensions)", at_obj);
        if (ind->st->type != (drec+2)->op)
           error(NIL, "invalid index argument (wrong storage type)", at_obj);
        if ((ind->st->isreadonly) && (drec->access == DHT_WRITE))
           error(NIL, "invalid index argument (read-only storage)", at_obj);

        arg->dh_idx_ptr = Mptr(at_obj);
     } else
        error(NIL, "invalid argument (index expected)",at_obj);
     break;
        
   case DHT_OBJ:
      if (OBJECTP(at_obj)) {
         const class_t *cl = Class(at_obj);
         object_t *obj = Mptr(at_obj);
         ifn (cl->has_compiled_part)
            error(NIL, "not an instance of a compiled class", at_obj);
         ifn (cl->live)
            error(NIL, "class or a superclass has become non-executable", at_obj);

         assert(obj->cptr);
         for (; cl; cl=cl->super) {
            dhclassdoc_t *cdoc = cl->classdoc;
            if ((cdoc) && (cdoc == (dhclassdoc_t*)(drec->arg)))
               break;
         }
         if (cl==NULL)
            error(NIL, "invalid argument (object has wrong type)", at_obj);
         arg->dh_obj_ptr = obj->cptr;
      } else
         error(NIL, "invalid argument (object expected)", at_obj);
      break;
    
   case DHT_STR:
      if (STRINGP(at_obj))
         arg->dh_str_ptr = (char *)String(at_obj);
      else
         error(NIL, "invalid argument (string expected)", at_obj);
      break;

   case DHT_LIST:
      /* 
       * We write list elements into a storage. This is an exception
       * to the general rule. Strings and compiled lists are immutable!
       */
      if (CONSP(at_obj)) {
         if (length(at_obj) != drec->ndim)
            error(NIL, "invalid argument (list has wrong length)", at_obj);

         /* C side representation is a storage */
         storage_t *st = new_storage(DHT_UCHAR);
         storage_alloc(st, drec->ndim*(sizeof(dharg)), 0);
         dharg *larg = st->data;
         at *p = at_obj;
         int i = drec->ndim;
         drec++;
         while (--i >= 0) {
            at_to_dharg(Car(p), larg, drec, at_obj);
            drec = drec->end;
            larg += 1;
            p = Cdr(p);
         }
         arg->dh_srg_ptr = st;
         
      } else
         error(NIL, "invalid argument (string expected)", at_obj);
      break;

   case DHT_FUNC:
      if (at_obj && Class(at_obj) == dh_class) {

         cfunction_t *cfunc = Gptr(at_obj);
         if (CONSP(cfunc->name))
            check_primitive(cfunc->name, cfunc->info);
         dhdoc_t *dhdoc = cfunc->info;
         if (dhdoc) {
            if (!dhinfo_match(dhdoc->argdata, drec))
               error(NIL, "invalid argument (wrong function signature)", at_obj);

            // get the function pointer to the C function
            assert(MODULEP(Car(cfunc->name)));
            struct module *m = Gptr(Car(cfunc->name));
            extern void *dynlink_symbol(struct module *, const char *, int, int);
            void *q = dynlink_symbol(m, dhdoc->lispdata.c_name, 1, 1);
            ifn (q)
               error(NIL, "could not find function pointer", at_obj);
            arg->dh_func_gptr = (gptr (*)())q;
         } else
            error(NIL, "internal error: dhdoc unvailable", NIL);
      } else 
         error(NIL, "invalid argument (not a DH function)", at_obj);

      break;
      
   default:
      error(NIL, "internal error (unknown op in DHDOC)", at_obj);
   }
}

index_t *new_empty_index(int r)
{
   index_t *ind = new_index(NULL, NULL);
   IND_NDIMS(ind) = r;
   return ind;
}

static void make_temporary(dhrecord *drec, dharg *arg)
{
   switch (drec->op) {
   case DHT_SRG:
      arg->dh_srg_ptr = new_storage((drec+1)->op);
      break;

   case DHT_IDX:
   {
      index_t *ind = new_index(NULL, NULL);
      IND_NDIMS(ind) = drec->ndim;
      arg->dh_idx_ptr = ind;
      break;
   }
   case DHT_OBJ:
   {
      arg->dh_obj_ptr = new_cobject((dhclassdoc_t*)(drec->arg));
      break;
   }  
   case DHT_STR:
      error(NIL, "string temporary (should never get here)", NIL);
      break;

   case DHT_LIST:
      arg->dh_srg_ptr = new_storage(DHT_UCHAR);
      storage_alloc(arg->dh_srg_ptr, drec->ndim*(sizeof(dharg)), 0);
      break;

   default:
      error(NIL, "internal error (unsupported type for temporary)", dhinfo_record(drec));
   }
}

static inline at *dharg_to_at(dharg *arg, dhrecord *drec)
{
   switch(drec->op) {
   case DHT_NIL:
      return NIL;
      
   case DHT_INT:
      return NEW_NUMBER(arg->dh_int);
    
   case DHT_DOUBLE:
      return NEW_NUMBER(arg->dh_double);

   case DHT_FLOAT:
      return NEW_NUMBER(arg->dh_float);

   case DHT_GPTR:
      return (arg->dh_gptr) ? NEW_GPTR(arg->dh_gptr) : at_NULL;

   case DHT_BOOL:
      return NEW_BOOL(arg->dh_bool);
      
   case DHT_CHAR:
      return NEW_NUMBER(arg->dh_char);
    
   case DHT_UCHAR:
      return NEW_NUMBER(arg->dh_uchar);
      
   case DHT_SHORT:
      return NEW_NUMBER(arg->dh_short);
    
   case DHT_STR:
   {
      assert(arg->dh_str_ptr);
      return NEW_STRING(arg->dh_str_ptr);
   }
   case DHT_SRG:
   {
      assert(arg->dh_srg_ptr);
      return arg->dh_srg_ptr->backptr;
   }
   case DHT_IDX:
   {
      assert(arg->dh_idx_ptr);
      return arg->dh_idx_ptr->backptr;
   }
   case DHT_OBJ:
   {
      assert(arg->dh_obj_ptr);
      return object_from_cobject(arg->dh_obj_ptr)->backptr;
   }
   case DHT_LIST:
   {
      if (arg->dh_srg_ptr==0)
         return NIL;
      storage_t *st = arg->dh_srg_ptr;
      arg = (dharg *)st->data;
      int ndim = drec->ndim;
      if (ndim * sizeof(dharg) != st->size)
         error(NIL,"internal error (list size mismatch)", NIL);

      at *p = NIL;
      at **where = &p;
      drec++;
      for (int i=0; i<ndim; i++) {
         *where = new_cons(dharg_to_at(arg,drec),NIL);
         arg += 1;
         drec = drec->end;
         where = &Cdr(*where);
      }
      return p;
   }
   default:
      error(NIL, "internal error (unknown op in DHDOC)", dhinfo_record(drec));
   }
}

/* dh_listeval -- calls a compiled function */
static at *dh_listeval(at *p, at *q)
{
   extern at **dx_sp; /* in function.c */

#define MAXARGS 256
   dharg _args[MAXARGS+1];
   dharg *args = _args+1;

   MM_ENTER;

   /* Find and check the DHDOC */
   struct cfunction *cfunc = Mptr(p);
   if (CONSP(cfunc->name))
      check_primitive(cfunc->name, cfunc->info);

   dhdoc_t *kname = cfunc->info;
   dhrecord *drec = kname->argdata;
   if (drec->op != DHT_FUNC)
      error(NIL, "not a function", p);
  
   /* Count the arguments */
   int nargs = drec->ndim;
   int ntemps = ((dhrecord *) drec->name)->ndim;
   if (nargs + ntemps > MAXARGS)
      error(NIL,"too many arguments and temporaries", NIL);
  
   at **arg_pos = eval_arglist_dx(Cdr(q));
   if ((int)(dx_sp - arg_pos) != nargs)
      need_error(0, nargs, NIL);
   arg_pos++;  /* zero-based argument indexing below */
   
   /* Make compiled version of the arguments */
   drec++;
   for (int i=0; i<nargs; i++) {
      if (!arg_pos[i] && drec->op!=DHT_BOOL)
         error(NIL, "illegal nil argument, expected", dhinfo_record(drec));
      at_to_dharg(arg_pos[i], &args[i], drec, NIL);
      drec = drec->end;
   }

   /* Prepare temporaries */
   if (ntemps != 0) {
      fprintf(stderr, "*** Warning: function %s takes hidden arguments\n",
              pname(p));
      drec++;
      for(int i=nargs; i<nargs+ntemps; i++) {
         make_temporary(drec, &args[i]);
         drec = drec->end;
      }
      drec++;
   }
    
   /* Prepare environment */
/*    if (in_compiled_code) */
/*       fprintf(stderr, "*** Warning: reentrant call to compiled code\n"); */
   int errflag = setjmp(lush_error_jump);
   dh_trace_root = 0;
   
   /* Call compiled code */
   dharg funcret;
   if (!errflag) {
      in_compiled_code = true;
      /* Call the test function if it exists */
      if (kname->lispdata.dhtest)
         (*kname->lispdata.dhtest->lispdata.call)(_args);
      /* Call to the function */
      funcret = (*kname->lispdata.call)(_args);
   }
   
   /* Prepare for the update */
   in_compiled_code = false;
   dh_trace_root = 0;
    
   /* Build return value */
   at *atfuncret = NIL;
   if (!errflag) {
      if (drec->op != DHT_RETURN)
         error(NIL, "internal error (cannot find DHT_RETURN in DHDOC", NIL);
      atfuncret = dharg_to_at(&funcret, drec+1);
   }
  
   /* return */
   if (errflag)
      error(NIL,"Run-time error in compiled code",NIL);

   dx_sp = arg_pos-1;

   MM_RETURN(atfuncret);
#undef MAXARGS
}


/* ---------------------------------------------
   CLASSDOC FOR OBJECT CLASS
   --------------------------------------------- */

dhclassdoc_t Kc_object;

static void Cdestroy_C_object(struct CClass_object *cobj)
{
   if (cobj->__lptr)
      cobj->__lptr->cptr = NULL;
}

struct VClass_object Vt_object = { 
   (void*)&Kc_object,
   &Cdestroy_C_object,
   NULL
};

DHCLASSDOC(Kc_object, NULL, object, "object", Vt_object, 0) =
{ 
   DH_CLASS(0, Kc_object), 
   DH_END_CLASS, 
   DH_NIL
};

/* cobject memory type */

static void clear_cobject(void *p, size_t n)
{
   memset(p, 0, n);
}

/* if Vtbl is NULL, object has been destructed */

static void mark_cobject(struct CClass_object *cobj)
{
   if (cobj->Vtbl) {
      at *p = cobj->Vtbl->Cdoc->lispdata.atclass;
      if (p) {
         MM_MARK(Mptr(p));
         if (cobj->Vtbl->__mark)
            cobj->Vtbl->__mark(cobj);
      }
   }
}

static bool finalize_cobject(struct CClass_object *cobj)
{
   if (cobj->Vtbl) {
      at *p = cobj->Vtbl->Cdoc->lispdata.atclass;
      if (p)
         cobj->Vtbl->Cdestroy(cobj);
   }
   return true;
}

static mt_t mt_cobject = mt_undefined;
static mt_t mt_cobject2 = mt_undefined;


struct CClass_object *new_cobject(dhclassdoc_t *cdoc)
{
   class_t *cl = Mptr(cdoc->lispdata.atclass);
   ifn (cl->live)
      error(NIL, "class is obsolete", cl->classname);
   struct CClass_object *cobj = NULL;
   if (cdoc->lispdata.size <= SIZEOF_COBJECT)
      cobj = mm_alloc(mt_cobject);
   else
      cobj = mm_allocv(mt_cobject2, cdoc->lispdata.size);
   cobj->Vtbl = cdoc->lispdata.vtable;
   cobj->__lptr = NULL;
   return cobj;
}


int test_obj_class(void *obj, void *classvtable)
{
   if (obj) {

      struct VClass_object *vtable = *(struct VClass_object**)obj;
      while (vtable && vtable != classvtable) {
         /* This is tricky because Cdoc contains
            different things in the NOLISP case. */
#ifndef NOLISP
         dhclassdoc_t *cdoc = (dhclassdoc_t*)(vtable->Cdoc);
         if (! cdoc)
	    lush_error("Found null Cdoc in virtual table");
         if (vtable != cdoc->lispdata.vtable)
	    lush_error("Found improper Cdoc in virtual table");
         cdoc = cdoc->lispdata.ksuper;
         vtable = (cdoc) ? cdoc->lispdata.vtable : 0;
#else
         vtable = (struct VClass_object*)(vtable->Cdoc);
#endif
      }
      if (vtable && vtable == classvtable)
         return 1;
   }
  return 0;
}

void check_obj_class(void *obj, void *classvtable)
{
   struct CClass_object *cobj = obj;
   if (! obj)
      lush_error("Attempt to cast NULL-pointer to object");
   if (! cobj->Vtbl)
      lush_error("Attempt to cast zombie to object");
   if (! test_obj_class(obj, classvtable)) {
      lush_error("Illegal object cast");
   }
}


/* ---------------------------------------------
   INITIALIZATION
   --------------------------------------------- */

class_t *dh_class;

void init_dh(void)
{
   /* check the mapping is as we think it is */
   assert(ST_BOOL  == DHT_BOOL);
   assert(ST_CHAR  == DHT_CHAR);
   assert(ST_UCHAR == DHT_UCHAR);
   assert(ST_SHORT == DHT_SHORT);
   assert(ST_INT   == DHT_INT);
   assert(ST_FLOAT == DHT_FLOAT);
   assert(ST_DOUBLE== DHT_DOUBLE);
   assert(ST_GPTR  == DHT_GPTR);
   assert(ST_MPTR  == DHT_MPTR);

   mt_cobject = MM_REGTYPE("cobject", SIZEOF_COBJECT,
                           clear_cobject, mark_cobject, finalize_cobject);
   mt_cobject2 = MM_REGTYPE("cobject2", SIZEOF_COBJECT2,
                           clear_cobject, mark_cobject, finalize_cobject);
   /* setup object class */
   object_class->classdoc = &Kc_object;
   Kc_object.lispdata.atclass = object_class->backptr;

   /* setting up dh_class */
   extern const char *func_name(at*);
   dh_class = new_builtin_class(function_class);
   dh_class->name = func_name;
   dh_class->listeval = dh_listeval;
   class_define("DH", dh_class);

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
