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
 * $Id: module.c,v 1.2 2002-05-01 18:32:46 leonb Exp $
 **********************************************************************/


#include "header.h"


/* --------- XXX_DEFINE FUNCTIONS --------- */


class *rootclasslist = 0;

void 
class_define(char *name, class *cl)
{
  at *symb;
  at *classat;
  symb = new_symbol(name);
  classat = new_extern(&class_class,cl);
  cl->classname = symb;
  cl->backptr = classat;
  cl->goaway = 0;
  cl->dontdelete = 0;
  cl->slotssofar = 0;
  cl->super = 0;
  cl->atsuper = NIL;
  cl->nextclass = rootclasslist;
  rootclasslist = cl;
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, classat);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
  }
  UNLOCK(classat);
}

void 
dx_define(char *name, at *(*addr) (int, at **))
{
  at *func, *symb;

  func = new_dx(addr);
  symb = new_symbol(name);
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, func);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
    ((struct function*) (func->Object))->evaluable_list = symb;
  }
  UNLOCK(func);
  UNLOCK(symb)
}

void 
dy_define(char *name, at *(*addr) (at *))
{
  at *func, *symb;

  func = new_dy(addr);
  symb = new_symbol(name);
  if (((struct symbol *) (symb->Object))->mode == SYMBOL_LOCKED) {
    fprintf(stderr, "init: attempt to redefine symbol '%s'\n", name);
  } else {
    var_set(symb, func);
    ((struct symbol *) (symb->Object))->mode = SYMBOL_LOCKED;
    ((struct function*) (func->Object))->evaluable_list = symb;
  }
  UNLOCK(func);
  UNLOCK(symb)
}




/* --------- INITIALISATION CODE --------- */

void 
init_module(void)
{
}
