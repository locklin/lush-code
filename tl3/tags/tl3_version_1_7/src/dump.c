/*   TL3 Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
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
 */
 
/***********************************************************************
  TL3: (C) LYB YLC 1988
  dump.c
  $Id: dump.c,v 1.1.1.1 2002-04-16 17:37:38 leonb Exp $

********************************************************************** */


#include "header.h"



/* ------ FORMAT OF A DUMP FILE ----- */


/* - 4 bytes of magic number 				    */
/* - 4 bytes of version number 				    */
/* - 256 bytes of character map (marking macro chars)       */
/* - 1 list executed before undumping everything            */
/* - 1 list of pairs (symbol . value) for the classes       */
/* - 1 list of pairs (symbol . value) for the other symbols */

#define DUMPMAGIC  0x44454d50
#define DUMPVERSION 2


/* From IO.C */
extern char char_map[];


/* --------- DUMP ------------------- */



static void 
dump(char *s, at *exec)
{
  FILE *f;
  at *atf;
  struct hash_name **j, *hn;
  at *ans;
  at **where;
  at *sym, *val;
  struct symbol *symb;
  
  /* Header */
  atf = OPEN_WRITE(s,"dump");
  f = atf->Object;
  write4(f, DUMPMAGIC);
  write4(f, DUMPVERSION);

  /* The macro character map */
  fwrite(char_map,1,256,f);
  if (ferror(f))
    test_file_error(f);

  /* The executable statement */
  bwrite(exec,f);

  /* The classes */
  ans = NIL;
  where = &ans;
  iter_hash_name(j, hn)
    if (EXTERNP(hn->named, &symbol_class))
      {
	sym = hn->named;
	symb = sym->Object;
	while (symb->next)
	  symb = symb->next;
	if (symb->valueptr) {
	  val = *(symb->valueptr);
	  if (EXTERNP(val,&class_class)) {
	    *where = cons( new_cons(sym,val), NIL);
	    where = &((*where)->Cdr);
	  }
	}
      }
  bwrite(ans,f);
  UNLOCK(ans);

  /* The rest */
  ans = NIL;
  where = &ans;
  iter_hash_name(j, hn)
    if (EXTERNP(hn->named, &symbol_class))
      {
	sym = hn->named;
	symb = sym->Object;
	while (symb->next)
	  symb = symb->next;
	if (symb->valueptr) {
	  val = *(symb->valueptr);
	  ifn (EXTERNP(val,&class_class)) {
	    *where = cons( new_cons(sym,val), NIL);
	    where = &((*where)->Cdr);
	  }
	}
      }
  bwrite(ans,f);
  UNLOCK(ans);
  UNLOCK(atf);
}


DX(xdump)
{
  ALL_ARGS_EVAL;
  if (arg_number == 1)
  {
    dump(ASTRING(1), NIL);
    return new_string(file_name);
  }
  else
  {
    ARG_NUMBER(2);
    dump(ASTRING(1), ALIST(2));
    return new_string(file_name);
  }
}





/* --------- UNDUMP ----------------- */



int 
isdump(char *s)
{
  FILE *f;
  at *atf;
  int magic;
  atf = OPEN_READ(s,"dump");
  f = atf->Object;
  magic = read4(f);
  UNLOCK(atf);
  ifn (magic==DUMPMAGIC)
    return 0;
  return 1;
}

void 
undump(char *s)
{
  at *p;
  at *sym;
  at *val;
  FILE *f;
  at *atf;
  int magic;
  int version;

  atf = OPEN_READ(s,"dump");
  f = atf->Object;

  magic = read4(f);
  version = read4(f);
  if ( magic != DUMPMAGIC )
    error(NIL,"Dump file format is incorrect",NIL);
  if ( version > DUMPVERSION )
    error(NIL,"Dump file format version is too high",NIL);

  /* The macro character map */
  fread(char_map,1,256,f);
  if (feof(f) || ferror(f))
    error(NIL,"Corrupted dump file (1)",NIL);

  /* The executable list */
  if (version >= 2)
  {
    p = bread(f, NIL);
    sym = named("file-being-loaded");
    val = new_string(s);
    symbol_push(sym,val);
    UNLOCK(val);
    val = eval(p);
    UNLOCK(val);
    UNLOCK(p);
    symbol_pop(sym);
    UNLOCK(sym);
  }

  /* The classes */
  p = bread(f, NIL);
  while (CONSP(p)) {
    ifn (CONSP(p->Car))
      error(NIL,"Corrupted dump file (2)",NIL);
    sym = p->Car->Car;
    val = p->Car->Cdr;
    p = p->Cdr;
    ifn (EXTERNP(sym,&symbol_class))
      error(NIL,"Corrupted dump file (3)",NIL);
    var_SET(sym,val);
  }
  UNLOCK(p);

  /* The rest */
  p = bread(f, NIL);
  while (CONSP(p)) {
    ifn (CONSP(p->Car))
      error(NIL,"Corrupted dump file (2)",NIL);
    sym = p->Car->Car;
    val = p->Car->Cdr;
    p = p->Cdr;
    ifn (EXTERNP(sym,&symbol_class))
      error(NIL,"Corrupted dump file (3)",NIL);
    var_SET(sym,val);
  }
  UNLOCK(p);

  /* Close file */
  UNLOCK(atf);
}

/* --------- INIT SECTION ----------- */


void 
init_dump(void)
{
  dx_define("dump",xdump);
}

	    
