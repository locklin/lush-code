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
 * $Id: dump.c,v 1.7 2003-01-14 23:37:16 leonb Exp $
 **********************************************************************/


#include "header.h"

/* Format of a dump file: 
 *  + 4 bytes of magic number
 *  + 4 bytes of version number
 *  + 256 bytes of character map (marking macro chars)
 *  + 1 big list containing either
 *      - pairs (symbol . value) defining a variable.
 *      - symbols that must be locked.
 *      - modules that must be loaded.
 */

#define DUMPMAGIC  0x44454d50
#define DUMPVERSION 3

/* From IO.C */
extern char char_map[];


/* --------- UTILS ------------------- */


static void
check(FILE *f)
{
  if (feof(f))
    error(NIL,"End of file during bread",NIL);
  if (ferror(f))
    test_file_error(NULL);
}

static void
write32(FILE *f, int x)
{
  char c[4];
  c[0] = x>>24;
  c[1] = x>>16;
  c[2] = x>>8;
  c[3] = x;
  if (fwrite(&c, sizeof(char), 4, f) != 4)
    test_file_error(f);
}

static int
read32(FILE *f)
{
  unsigned char c[4];
  if (fread(c, sizeof(char), 4, f) != 4)
    check(f);
  return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}

static int
readmagic32(FILE *f)
{
  unsigned char c[4];
  if (fread(c, sizeof(char), 2, f) != 2)
    check(f);
  if (c[0]=='#' && c[1]=='!')
    {
      int x = getc(f);
      while (x != '\n' && x != '\r' && x != EOF)
        x = getc(f);
      while (x == '\n' || x == '\r')
        x = getc(f);
      if (x == EOF) 
        check(f);
      c[0] = x;
      c[1] = getc(f);
    }
  if (fread(c+2, sizeof(char), 2, f) != 2)
    check(f);
  return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}



/* --------- DUMP ------------------- */


static void 
dump(char *s)
{
  FILE *f;
  at *atf;
  struct hash_name **j, *hn;
  at *ans, *p, *q;
  at **where;
  at *sym, *val;
  struct symbol *symb;
  /* Header */
  atf = OPEN_WRITE(s,"dump");
  f = atf->Object;
  write32(f, DUMPMAGIC);
  write32(f, DUMPVERSION);
  /* The macro character map */
  fwrite(char_map,1,256,f);
  if (ferror(f))
    test_file_error(f);
  /* Building the big list */
  ans = NIL;
  where = &ans;
  /* 1- the modules */
  p = q = module_list();
  while (CONSP(q)) {
    *where = new_cons(q->Car, NIL);
    where = &((*where)->Cdr);
    q = q->Cdr;
  }
  UNLOCK(p);
  /* 2- the globals */
  iter_hash_name(j, hn)
    if (EXTERNP(hn->named, &symbol_class))
      {
	sym = hn->named;
	symb = sym->Object;
	while (symb->next)
	  symb = symb->next;
	if (symb->valueptr)
          {
            val = *(symb->valueptr);
            *where = cons( new_cons(sym,val), NIL);
            where = &((*where)->Cdr);
            /* locked? */
            if (symb->mode == SYMBOL_LOCKED)
              {
                *where = new_cons(sym,NIL);
                where = &((*where)->Cdr);
              }
          }
      }
  /* Write the big list */
  bwrite(ans, f, TRUE);
  UNLOCK(ans);
}


DX(xdump)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  dump(ASTRING(1));
  return new_string(file_name);
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
  magic = readmagic32(f);
  UNLOCK(atf);
  if (magic != DUMPMAGIC)
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

  magic = readmagic32(f);
  version = read32(f);
  if ( magic != DUMPMAGIC )
    error(NIL,"Dump file format is incorrect",NIL);
  if ( version > DUMPVERSION )
    error(NIL,"Dump file format version is too high",NIL);

  /* The macro character map */
  fread(char_map,1,256,f);
  if (feof(f) || ferror(f))
    error(NIL,"Corrupted dump file (1)",NIL);

  /* The executable list */
  if (version == 2)
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

  if (version <= 2)
    {
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
    }
  
  /* The unified list */
  p = bread(f, NIL);
  while (CONSP(p)) 
    {
      if (CONSP(p->Car)) {
        sym = p->Car->Car;
        val = p->Car->Cdr;
        ifn (EXTERNP(sym,&symbol_class))
          error(NIL,"Corrupted dump file (4)",NIL);
        var_SET(sym,val);
      } else if (EXTERNP(p->Car,&symbol_class))
        var_lock(p->Car);
      val = p;
      p = p->Cdr;
      val->Cdr = NIL;
      UNLOCK(val);
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

	    
