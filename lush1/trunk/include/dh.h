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
 * $Id: dh.h,v 1.3 2002-07-01 21:20:36 leonb Exp $
 **********************************************************************/
#ifndef DH_H
#define DH_H

/* SWITCHES
 * -- BASETYPE: The type of the matrix elements.     Default: flt
 * -- ST_BASETYPE: The type of the idxs.             Default: ST_F
 * -- NOLISP: Do not compile the Lisp dependent parts
 */

#ifndef DEFINE_H
#include "define.h"
#endif
#ifndef IDXMAC_H
#include "idxmac.h"
#endif
#ifndef IDXOPS_H
#include "idxops.h"
#endif

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif

#ifndef NOLISP
#ifndef HEADER_H
/* Should be replaced by minimal definitions:
 * - struct srg, struct idx, etc... 
 */
#include "header.h"
#endif
#endif

/* ----------------------------------------------- */
/* DHRECORDS                                       */
/* ----------------------------------------------- */



/* Values for the "access" field */
#define DHT_READ  1
#define DHT_WRITE 2

/* Values for the "op" field */
enum dht_type {
    DHT_NIL,
    
    DHT_FUNC,       /* function (+ arg types + temps + return ) */
    DHT_BOOL,       /* type */
    DHT_BYTE,       /* type */
    DHT_UBYTE,      /* type */
    DHT_SHORT,      /* type */
    DHT_INT,        /* type */
    DHT_STR,        /* type */
    DHT_FLT,	    /* type */
    DHT_REAL,	    /* type */
    DHT_GPTR,	    /* type */
    DHT_IDX,   	    /* type (+ srg type) */
    DHT_SRG,   	    /* type (+ base type) */
    DHT_LIST,	    /* type (+ component types + end_list */
    DHT_END_LIST,   /* type terminator */
    DHT_OBJ,        /* type */
    DHT_TEMPS,      /* temps (+ types for temps) */
    DHT_END_TEMPS,  /* temps terminator */
    DHT_RETURN,     /* return type (+ return type) */
    DHT_END_FUNC,   /* function terminator */

    DHT_CLASS,      /* class (+ fields + methods ) */
    DHT_NAME,       /* define the name/position of a field */
    DHT_METHOD,     /* define the name/constraint of a method */
    DHT_END_CLASS,  /* class terminator */
    
    DHT_LAST        /* TAG */
};


/* dhrecord --- 
 * The basic data structure for metainformation in compiled code.
 * Note: this is not a union because union can't be initialized 
 */
typedef struct s_dhrecord 
{
  enum dht_type op;	    /* Type of the record */
  short access;             /* Type of access */
  short ndim;		    /* number of dimensions/fields */
  char *name;               /* field name */
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
#define DH_BYTE \
	{DHT_BYTE}
#define DH_UBYTE \
	{DHT_UBYTE}
#define DH_SHORT \
	{DHT_SHORT}
#define DH_INT \
        {DHT_INT}
#define DH_FLT \
        {DHT_FLT}
#define DH_REAL \
        {DHT_REAL}
#define DH_GPTR \
        {DHT_GPTR}
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
        {DHT_OBJ,0,0,0,&kclass}
#define DH_TEMPS(n) \
        {DHT_TEMPS,0,n}
#define DH_END_TEMPS \
        {DHT_END_TEMPS}
#define DH_RETURN \
        {DHT_RETURN}

#define DH_CLASS(n, cl) \
        {DHT_CLASS,0,n,0,&cl}
#define DH_END_CLASS \
        {DHT_END_CLASS}#
#define DH_NAME(s,cl,sl) \
        {DHT_NAME, 0, 0, s, &(((struct name2(CClass_,cl)*)0)->sl)}
#define DH_METHOD(n,kname) \
        {DHT_METHOD,0,n,kname}



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

typedef union 
{
  char          dh_char;
  unsigned char dh_uchar;
  short         dh_short;
  int           dh_ord;
  int           dh_int;
  int           dh_bool;
  flt           dh_flt;
  real          dh_real;
  gptr		dh_gptr;
  struct idx   *dh_idx_ptr;
  struct srg   *dh_srg_ptr;
  struct srg   *dh_str_ptr;
  int          *dh_obj_ptr;
} dharg;


/* dhconstraint ---
 * Describes function to the interpretor.
 */

struct dhdoc_s
{
  dhrecord *argdata;            /* Points to the metainformation records */
  struct {
    char *c_name;		/* The C name of this DH         */
    char *m_name;		/* The macro for this DH, or NIL */
    dharg (*call)();		/* The DH call, defined by DH    */
    char *k_name;
    dhdoc_t *dhtest;
  } lispdata;
};



#ifndef NOLISP

#define DHDOC(Kname,Xname,Cnamestr,Mnamestr,Ktest) \
  static dhrecord name2(K,Kname)[];\
  dhconstraint Kname = { name2(K,Kname), \
    { Cnamestr, Mnamestr, Xname, enclose_in_string(Kname), Ktest } }; \
  static dhrecord name2(K,Kname)[] =

#define DH(Xname) \
  dharg Xname(dharg *a)

#endif /* !NOLISP */



/* ----------------------------------------------- */
/* DHCLASSES                                       */
/* ----------------------------------------------- */


/* Names associated with a class
 * -----------------------------
 *
 * struct CClass_name:  structure representing instances (macro DHCLASSDOC)
 * struct VClass_name:  structure representing vtable 
 * V_name_Rxxxxxxxx:    vtable for the class (VClass_name)
 * K_name_Rxxxxxxxx:    dhclassconstraint for the class
 *
 * C_methodname_C_name: compiled code for a method
 * K_methodname_C_name_Rxxxxxxxx: dhconstraint for a method (macro DHDOC)
 * X_methodname_C_name: stub code for a method (macro DH)
 *
 */



/* dhclassconstraint ---
 * This is the dhclass descriptor.
 */

struct dhclassdoc_s
{
  dhrecord *argdata;
  struct {
    at *atsuper;
    dhclassdoc_t *ksuper;
    char *name;
    char *v_name;
    char *k_name;
    int nmethods;
    dhdoc_t **ktable;
    void *vtable;
  } lispdata;
};

#ifndef NOLISP

#define DHCLASSDOC(Kname,superKname,nameStr,VnameStr,nmet,vtable) \
  static dhrecord name2(K,Kname)[]; \
  static dhdoc_t* name2(M,Kname)[]; \
  dhclassdoc_t Kname = { name2(K,Kname), \
   { NIL, superKname, nameStr, VnameStr, enclose_in_string(Kname), \
     nmet, name2(M,Kname), vtable } }; \
  static dhrecord name2(K,Kname)[] = 

#endif


/* ----------------------------------------------- */
/* END                                             */
/* ----------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
