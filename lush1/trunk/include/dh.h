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
 * $Id: dh.h,v 1.2 2002-06-27 19:43:13 leonb Exp $
 **********************************************************************/
#ifndef DH_H
#define DH_H

/* SWITCHES
 * -- BASETYPE: The type of the matrix elements.     Default: flt
 * -- ST_BASETYPE: The type of the idxs.             Default: ST_F
 * -- NOLISP: Do not compile the Lisp dependent parts
 */

#ifndef BASETYPE
#define BASETYPE flt
#define ST_BASETYPE  ST_F
#endif

#ifndef NOLISP
#if BASETYPE != flt
#error "BASETYPE must be flt unless NOLISP is defined"
#endif
#endif

#ifndef NOLISP			/* !defined(NOLISP) */
#ifndef HEADER_H
#include "header.h"
#endif
#else				/* defined(NOLISP) */
#ifndef IDX_H
#include "idx.h"
#endif
#endif				/* defined(NOLISP) */

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif


/* ----------------------------------------------- */
/* DHRECORDS                                       */
/* ----------------------------------------------- */



/* Values for the "access" field */
#define DHT_READ 0
#define DHT_WRITE 1

/* Values for the "op" field */
enum dht_type {
    DHT_NIL,
    DHT_BOOL,
    DHT_BYTE,
    DHT_UBYTE,
    DHT_SHORT,
    DHT_INT,
    DHT_STR,
    DHT_FLT,	    /* flt number */
    DHT_REAL,	    /* real */
    DHT_GPTR,	    /* generic pointer */
    DHT_IDX,   	    /* idx */
    DHT_SRG,   	    /* idx */
    DHT_LIST,	    /* list */
    DHT_END_LIST,   /* end of list */
    DHT_OBJ,        /* object */
    DHT_END_OBJ,    /* end of object */
    DHT_FUNC,       /* function */
    DHT_END_FUNC,  
    DHT_TEMPS,      /* begining of list of temps */
    DHT_END_TEMPS,  
    DHT_RETURN,     /* head of result section */
    DHT_DEPENDS,    /* begin list of dependencies */
    DHT_DH_REFER,   /* reference to a DH */
    DHT_CLASS_REFER,/* reference to a CLASS */
    DHT_GLOB_REFER, /* reference to a global */
    DHT_END_DEPENDS,
    DHT_REFER_TO,
    DHT_CLASS,      /* class */
    DHT_END_CLASS,
    DHT_METHODS,
    DHT_END_METHODS,
    DHT_NAME,       /* name (for a class field) */
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
  short ndim;		    /* Number of dimensions(idx)/fields(object) */
  char *name;               /* field name (used also in FUNC for end of
                               args, and in DH_CLASS to hold K_class)*/
  long hashcode;            /* to put hash code.  In DH_NAME struct, also 
                               holds the slot offset (which is updated in 
			       next-record) */
  struct s_dhrecord *end;   /* point on the next dhrecord.  This is updated
                               in next record */
} dhrecord;

/* Macros for constructing dhrecords */

#define DH_NIL \
	{DHT_NIL}		/* Nothing (end mark, usually) */
#define DH_BOOL \
	{DHT_BOOL}		/* Argument of type BASETYPE */	
#define DH_BYTE \
	{DHT_BYTE}		/* Argument of type BASETYPE */	
#define DH_UBYTE \
	{DHT_UBYTE}		/* Argument of type BASETYPE */	
#define DH_SHORT \
	{DHT_SHORT}		/* Argument of type BASETYPE */	
#define DH_INT \
        {DHT_INT}		/* Argument of type BASETYPE */	
#define DH_FLT \
        {DHT_FLT}		/* Argument of type BASETYPE */	
#define DH_REAL \
        {DHT_REAL}		/* Argument of type BASETYPE */	
#define DH_GPTR \
        {DHT_GPTR}		/* Argument is a generic pointer */	
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
#define DH_RETURN \
        {DHT_RETURN}
#define DH_FUNC(n) \
        {DHT_FUNC, DHT_READ, n}
#define DH_END_FUNC \
        {DHT_END_FUNC}
#define DH_CLASS(n,cl) \
        {DHT_CLASS,0,n, (char *) &cl, 0,\
        /* address of sample (will be erased by next_record) \
	   This is used to compute offset of slots when converting \
	   C object to Lisp object */ \
	(struct s_dhrecord *) &(name2(cl,_sample_))}
#define DH_END_CLASS \
        {DHT_END_CLASS}
#define DH_METHODS(n) \
        {DHT_METHODS,0,n}
#define DH_END_METHODS \
        {DHT_END_METHODS}
#define DH_OBJ(class) \
        {DHT_OBJ, 0, 0, (char *)  &class}
#define DH_NAME(s,cl,sl) \
        {DHT_NAME, 0, 0, s, 0,\
	   /* addres of slot in sample (will be erased by next_record) \
	      This is used to compute offset of slots when converting \
	      C object to Lisp object */ \
	   (struct s_dhrecord *) &(name2(cl,_sample_).sl)} 
#define DH_TEMPS(n) \
        {DHT_TEMPS,0,n}
#define DH_END_TEMPS \
        {DHT_END_TEMPS}
#define DH_DEPENDS(n) \
        {DHT_DEPENDS,0,n}
#define DH_REFER(na,key) \
        {DHT_DH_REFER, 0, 0, (char *) (na), (key)}
#define CLASS_REFER(na,key) \
        {DHT_CLASS_REFER, 0, 0, (char *) na, key}
#define GLOB_REFER(na,key) \
        {DHT_GLOB_REFER, 0, 0, (char *) na, key}
#define DH_END_DEPENDS \
        {DHT_END_DEPENDS}



/* ----------------------------------------------- */
/* DHFUNCTIONS                                     */
/* ----------------------------------------------- */


/* dharg ---
 * Variant datatype for passing dh arguments.
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
 * This is the dh function descriptor.
 */

typedef struct dhconstraint 
{
  dhrecord *argdata;            /* Points to the metainformation records */
  long hashcode;                /* Hashed prototype */
  struct {
    char *c_name;		/* The C name of this DH         */
    char *m_name;		/* The macro for this DH, or NIL */
    dharg (*call)();		/* The DH call, defined by DH    */
    char *k_name;
    struct dhconstraint *test_name;
  } lispdata;
} dhconstraint;



/*  Macros for defining a DH function
 *  ---------------------------------  
 *   #ifndef NOLISP 
 *   DH(Xmydh)  
 *   { 
 *     Cmydh( &a[1].dh_idx, &a[2].dh_idx, a[3].dh_ord );
 *     // or Mmydh( a[1].dh_idx, a[2].dh_idx, a[3].dh_ord, BASETYPE );
 *   };
 *
 *   DHDOC( Kmydh, Xmydh,       // identifiers
 *          "Cfunc-name",       // the name of the C function
 *          "Macro-name",       // the name of the macro (or NULL)
 *          NULL,               // pointer on dhdoc of test
 *   {
 *      DH_FUNC(3),               // number of arguments
 *       DH_IDX (DHT_READ, 1),    // arg1  ((-idx1- (-flt-)))
 *        DH_SRG (DHT_READ),
 *         DH_FLT,
 *       DH_IDX (DHT_READ, 1),    // arg2  ((-idx1- (-flt-)))
 *        DH_SRG (DHT_READ),
 *         DH_FLT,
 *       DH_IDX (DHT_READ, 2),    // arg3  ((-idx1- (-flt-)))
 *        DH_SRG (DHT_WRITE),
 *         DH_FLT,
 *       DH_RETURN,               // return value
 *        DH_BOOL,
 *      DH_END_FUNC,
 *      DH_NIL,
 *   };
 *   #endif 
 */

#ifdef NOLISP

#define DHDOC(Kname,Xname,Cstr,Mstr, Ktest)\
  dhrecord name2(__,Kname)[];	     	 	\
  dhconstraint Kname = {  			\
    name2(__,Kname), 0L, \
    {Cstr, Mstr, 0L,enclose_in_string(Kname), Ktest} \
  }; \
  dhrecord name2(__,Kname)[] =

#else /* !NOLISP */

#define DHDOC(Kname,Xname,Cstr,Mstr, Ktest) \
  dhrecord name2(__,Kname)[];\
  dhconstraint Kname = { \
    name2(__,Kname), 0L, \
    { Cstr, Mstr, Xname, enclose_in_string(Kname), Ktest }, \
  }; \
  dhrecord name2(__,Kname)[] =

#define DH(Xname) \
  dharg Xname(dharg *a)

#endif /* !NOLISP */



/* ----------------------------------------------- */
/* DHCLASSES                                       */
/* ----------------------------------------------- */


/* dhclassconstraint ---
 * This is the dhclass descriptor.
 */

typedef struct dhclassconstraint 
{
  dhrecord *argdata;          /* Points to metainformation records */
  long hashcode;	      /* Hashed prototype */
  struct {
    /* Warning: the name is the only way to get from the C class to the
       LISP class.  Don't change the way the name is made without 
       checking dh.c (build_at_return) */
    char *name; 
    struct dhclassconstraint *super;
    char *supername;
    void *method_table;
    char *method_table_name;
    dhconstraint **DH_method_table;
    char *DH_method_table_name;
    int nmethods;
#ifndef NOLISP
    at **at_methods;
    char **Lisp_methods;
#endif
    int size;
  } lispdata;
} dhclassconstraint;


/* Macros for defining dhclasses.
 * ------------------------------
 * Watch out, you cannot leave blanks between the arguments in the 
 * call to the DHDOC (otherwise the strings get garbled).
 *
 * CCLASSDOC (K_Class_object,            // The C name of this class 
 *            NULL,                      // Pointer to the super class 
 *	      T_Class_object,            // C Name of table of method 
 *	      DH_Class_object,           // C Name of table of DH method 
 *            2,                         // The number of methods
 *            Lisp_met,                  // pointer to a table of lisp names
 *            struct_class,              // structure (a sizeof will be done)
 *            )
 * {
 *     DH_CLASS(2),
 *      DH_NAME("slot_1"),
 *       DH_INT,
 *      DH_NAME("slot_2"),
 *       DH_REAL,
 *      DH_METHODS(2),
 *       DH_FUNC(0),
 *        DH_RETURN,
 *         DH_BOOL,
 *       DH_END_FUNC,
 *       DH_FUNC(0),
 *        DH_RETURN,
 *         DH_BOOL,
 *       DH_END_FUNC,
 *      DH_END_METHODS,
 *     DH_END_CLASS,
 *     DH_NIL
 * };
 */

#ifdef NOLISP

#define CCLASSDOC(Kname,Supername,Table,DH_table, nmet, lisp_met, sample) \
  dhrecord name2(__,Kname)[];                   \
  dhclassconstraint Kname = {                        \
    name2(__,Kname), 0L, \
    { enclose_in_string(Kname), /* see comment in struct definition */\
      (Supername == 0) ? 0 : &Supername, enclose_in_string(Supername), \
      Table, enclose_in_string(Table), \
      NULL, NULL, nmet, sizeof(sample)\
    }, \
  }; \
  dhrecord name2(__,Kname)[] =

#else /* !NOLISP */

#define CCLASSDOC(Kname,Supername,Table,DH_table, nmet, lisp_met, sample) \
  dhrecord name2(__,Kname)[];                   \
  sample name2(Kname,_sample_); \
  at *name2(AT_,Kname)[(nmet==0) ? 1 : nmet]; \
  dhclassconstraint Kname = {                        \
    name2(__,Kname), 0L, \
    { enclose_in_string(Kname), /* see comment in struct definition */ \
	  &Supername, enclose_in_string(Supername), \
      Table, enclose_in_string(Table), \
      DH_table, enclose_in_string(DH_table), \
      nmet, name2(AT_,Kname), lisp_met, sizeof(sample)\
    }, \
  }; \
  dhrecord name2(__,Kname)[] =

#endif /* !NOLISP */


/* ----------------------------------------------- */
/* END                                             */
/* ----------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
