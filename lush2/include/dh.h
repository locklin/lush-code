/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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

#ifndef DH_H
#define DH_H

#ifndef DEFINE_H
#include "define.h"
#endif
#ifndef IDXMAC_H
#include "idxmac.h"
#endif
#ifndef IDXOPS_H
#include "idxops.h"
#endif

#ifndef NOLISP
#ifndef HEADER_H
/* Should be replaced by minimal definitions:
 * - struct idx, etc... 
 */
#include "header.h"
#endif
#endif

/* objects with less or equal MIN_NUM_SLOTS slots will
 * be allocated via mm_alloc.
 */
#define MIN_NUM_SLOTS  6


/* ----------------------------------------------- */
/* C++                                             */
/* ----------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif

#ifdef __cplusplus
#define extern_c extern "C"
#define staticref_c extern
#define staticdef_c 
#else
#define extern_c extern
#define staticref_c static
#define staticdef_c static
#endif


/* ----------------------------------------------- */
/* DHRECORDS                                       */
/* ----------------------------------------------- */



/* Values for the "access" field */
#define DHT_READ  1
#define DHT_WRITE 2

/* Values for the "op" field */
enum dht_type {
    DHT_NIL = -1,
    
    /* scalar types */
    DHT_BOOL,       /* type */
    DHT_CHAR,       /* type */
    DHT_UCHAR,      /* type */
    DHT_SHORT,      /* type */
    DHT_INT,        /* type */
    DHT_FLOAT,	    /* type */
    DHT_DOUBLE,	    /* type */
    DHT_GPTR,	    /* type */
    DHT_MPTR,
    DHT_AT,         /* currently not used */
    DHT_STR,        /* also a pointer */

    /* structured types */
    DHT_INDEX,      /* array of any rank */
    DHT_IDX,   	    /* array of specific rank */
    DHT_SRG,   	    /* type (+ base type) */
    DHT_LIST,	    /* type (+ component types + end_list */
    DHT_END_LIST,   /* type terminator */
    DHT_OBJ,        /* type */

    /* functions and classes */
    DHT_FUNC,       /* function (+ arg types + temps + return ) */
    DHT_TEMPS,      /* temps (+ types for temps) */
    DHT_END_TEMPS,  /* temps terminator */
    DHT_RETURN,     /* return type (+ return type) */
    DHT_END_FUNC,   /* function terminator */

    DHT_CLASS,      /* class (+ fields + methods ) */
    DHT_NAME,       /* define the name/position of a field */
    DHT_METHOD,     /* define the name/constraint of a method */
    DHT_END_CLASS,  /* class terminator */

    DHT_REFER,      /* specify dependency */

    DHT_LAST        /* TAG */
};

#define DHT_BYTE    DHT_CHAR
#define DHT_UBYTE   DHT_UCHAR
#define DHT_REAL    DHT_DOUBLE
#define DHT_FLT     DHT_FLOAT
#define DHT_OBJECT  DHT_OBJ
#define DHT_STORAGE DHT_SRG

/* dhrecord --- 
 * The basic data structure for metainformation in compiled code.
 * Note: this is not a union because union can't be initialized 
 */
typedef struct s_dhrecord 
{
  enum dht_type op;	    /* Type of the record */
  short access;             /* Type of access */
  short ndim;		    /* number of dimensions/fields */
  const char *name;         /* field name */
  void *arg;                /* field argument */
  struct s_dhrecord *end;   /* point on the next dhrecord. */
} dhrecord;

/* Macros for constructing dhrecords */

#define DH_NIL \
	{DHT_NIL}		/* Nothing (end mark, usually) */
#define DH_FUNC(n) \
        {DHT_FUNC, DHT_READ, n}
#define DH_END_FUNC \
        {DHT_END_FUNC}
#define DH_BOOL \
	{DHT_BOOL}
#define DH_CHAR \
	{DHT_CHAR}
#define DH_UCHAR \
	{DHT_UCHAR}
#define DH_SHORT \
	{DHT_SHORT}
#define DH_INT \
        {DHT_INT}
#define DH_FLOAT \
        {DHT_FLOAT}
#define DH_DOUBLE \
        {DHT_DOUBLE}
#define DH_GPTR(s) \
        {DHT_GPTR,0,0,s}
#define DH_MPTR(s) \
        {DHT_MPTR,0,0,s}
#define DH_STR \
        {DHT_STR}		
#define DH_LIST(n) \
        {DHT_LIST,0,n}
#define DH_END_LIST \
        {DHT_END_LIST}
#define DH_IDX(k,n) \
        {DHT_IDX,k,n}
#define DH_SRG(k) \
        {DHT_SRG,k}
#define DH_OBJ(kclass) \
        {DHT_OBJ,0,0,0,(void*)&kclass}
#define DH_TEMPS(n) \
        {DHT_TEMPS,0,n}
#define DH_END_TEMPS \
        {DHT_END_TEMPS}
#define DH_RETURN \
        {DHT_RETURN}

#define DH_CLASS(n, cl) \
        {DHT_CLASS,0,n,0,(void*)&cl}
#define DH_END_CLASS \
        {DHT_END_CLASS}
#define DH_NAME(s,cl,sl) \
        {DHT_NAME,0,0,s,(void*)&(((struct name2(CClass_,cl)*)0)->sl)}
#define DH_METHOD(s,kname) \
        {DHT_METHOD,0,0,s,(void*)&kname}

#define DH_REFER(kname) \
        {DHT_REFER,0,0,0,(void*)&kname}

/* ----------------------------------------------- */
/* DHFUNCTIONS                                     */
/* ----------------------------------------------- */


/*  Names associated with a DH function
 *  ---------------------------------  
 * 
 *   C_name: name of compiled function.
 *   K_name_Rxxxxxxxx: name of dhconstraint information (macro DHDOC).
 *   X_name: stub code for a compiled function (macro DH).
 *   M_name (optional): name of macro implememting the function.
 *   K_test (optional): dhconstraint information for test function.
 */


/* dharg ---
 * Variant datatype for passing args to the Xname function
 */

struct CClass_object;

typedef union 
{
  char          dh_char;
  unsigned char dh_uchar;
  short         dh_short;
  int           dh_int;
  bool          dh_bool;
  float         dh_float;
  double        dh_double;
  gptr		dh_gptr;
  mptr          dh_mptr;
  index_t      *dh_idx_ptr;
  storage_t    *dh_srg_ptr;
  char         *dh_str_ptr;
  struct CClass_object *dh_obj_ptr;

  // function types: function returning X
  char          (*dh_func_char)();
  unsigned char (*dh_func_uchar)();
  short         (*dh_func_short)();
  int           (*dh_func_int)();
  bool          (*dh_func_bool)();
  float         (*dh_func_float)();
  double        (*dh_func_double)();
  gptr		(*dh_func_gptr)();
  mptr          (*dh_func_mptr)();
  index_t      *(*dh_func_idx_ptr)();
  storage_t    *(*dh_func_srg_ptr)();
  char         *(*dh_func_str_ptr)();
  struct CClass_object *(*dh_func_obj_ptr)();
} dharg;


/* dhconstraint ---
 * Describes function to the interpretor.
 */

struct dhdoc_s
{
  dhrecord *argdata;            /* points to the metainformation records */
  struct {
    const char *c_name;		/* string with the C_name */
    const char *m_name;		/* string with the M_name or nil */
    dharg (*call)(dharg *);	/* pointer to the X_name function */
    const char *k_name;         /* string with the K_name_Rxxxxxxxx */
    dhdoc_t *dhtest;            /* pointer to the dhdoc for the testfunc */ 
  } lispdata;
};



#ifndef NOLISP

#define DHDOC(Kname,Xname,Cnamestr,Mnamestr,Ktest) \
  staticref_c dhrecord name2(K,Kname)[]; \
  extern_c dhdoc_t Kname; \
  dhdoc_t Kname = { name2(K,Kname), \
    { Cnamestr, Mnamestr, Xname, enclose_in_string(Kname), Ktest } }; \
  staticdef_c dhrecord name2(K,Kname)[]

#define DH(Xname) \
  static dharg Xname(dharg *a)

#define DHCALL(cf) ((dharg (*)(dharg *))cf->call)

#endif /* !NOLISP */



/* ----------------------------------------------- */
/* DHCLASSES                                       */
/* ----------------------------------------------- */


/* Names associated with a class
 * -----------------------------
 *
 * struct CClass_name:  structure representing instances (macro DHCLASSDOC)
 * struct VClass_name:  structure representing vtable 
 * Vt_name_Rxxxxxxxx:   vtable for the class (VClass_name)
 * Kc_name_Rxxxxxxxx:   dhclassdoc for the class
 *
 * C_methodname_C_name: compiled code for a method
 * K_methodname_C_name_Rxxxxxxxx: dhconstraint for a method (macro DHDOC)
 * X_methodname_C_name: stub code for a method (macro DH)
 *
 */

struct dhclassdoc_s;

struct VClass_object 
{
  struct dhclassdoc_s *Cdoc;
  void (*Cdestroy)(struct CClass_object *);
  void (*__mark)(struct CClass_object *);
};

struct CClass_object {
  struct VClass_object *Vtbl;
  object_t             *__lptr;
};



/* dhclassconstraint ---
 * This is the dhclass descriptor.
 */

struct dhclassdoc_s
{
  dhrecord *argdata;            /* points to the metainformation records */
  struct {
    dhclassdoc_t *ksuper;       /* dhclassdoc for the superclass */
    const char *lname;          /* string with the lisp class name */
    const char *cname;          /* string with the c class name 
                                   (prepend CClass_ or VClass_) */
    const char *v_name;         /* string with the name of the vtable 
                                   (V_name_Rxxxxxxxx) */
    const char *k_name;         /* string with the name of the classdoc 
                                   (K_name_Rxxxxxxxx) */
    int size;                   /* data size */
    int nmet;                   /* number of methods */
    struct VClass_object *vtable;  /* virtual table pointer */
    
#ifndef NOLISP
    at *atclass;                /* lisp object for this class */
#endif
  } lispdata;
};

#ifndef NOLISP

#define DHCLASSDOC(Kname,superKname,Cname,LnameStr,Vname,nmet) \
  staticref_c dhrecord name2(K,Kname)[];                       \
  extern_c dhclassdoc_t Kname; \
  dhclassdoc_t Kname = { name2(K,Kname), \
   { superKname, LnameStr, enclose_in_string(Cname), \
     enclose_in_string(Vname), enclose_in_string(Kname), \
     sizeof(struct name2(CClass_,Cname)), nmet, \
     (struct VClass_object *)(void *)&Vname } };        \
  staticdef_c dhrecord name2(K,Kname)[]

#endif


extern LUSHAPI class_t *dh_class;
extern LUSHAPI bool in_compiled_code;

LUSHAPI at  *new_dh(at *name, dhdoc_t *kdata);
LUSHAPI at  *new_dhclass(at *name, dhclassdoc_t *kdata);
LUSHAPI enum dht_type dht_from_cname(symbol_t *);
LUSHAPI void lush_error(const char *s);
LUSHAPI struct CClass_object *new_cobject(dhclassdoc_t *cdoc);
LUSHAPI index_t *new_empty_index(int);
LUSHAPI void check_obj_class(void *obj, void *classvtable);
LUSHAPI int  test_obj_class(void *obj, void *classvtable);


#define run_time_error lush_error


/* ----------------------------------------------- */
/* END                                             */
/* ----------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif


/* -------------------------------------------------------------
   Local Variables:
   c-font-lock-extra-types: (
     "FILE" "\\sw+_t" "at" "gptr" "real" "flt" "intg" )
   End:
   ------------------------------------------------------------- */
