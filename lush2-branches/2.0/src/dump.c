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
#include <errno.h>
#include <sys/stat.h>

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
#define DUMPVERSION 4


/* From IO.C */
extern unsigned char char_map[];


/* --------- UTILS ------------------- */

#define CHECK(f)                                        \
   {                                                    \
      if (feof(f))                                      \
         error(NIL,"end of file during bread",NIL);     \
      else                                              \
         test_file_error(f, errno);                     \
   }

static void write32(FILE *f, int x)
{
   char c[4];
   c[0] = x>>24;
   c[1] = x>>16;
   c[2] = x>>8;
   c[3] = x;
   errno = 0;
   if (fwrite(&c, sizeof(char), 4, f) != 4)
      test_file_error(f, errno);
}

static int read32(FILE *f)
{
   uchar c[4];
   errno = 0;
   if (fread(c, sizeof(char), 4, f) != 4)
      CHECK(f);
   return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}

static int readmagic32(FILE *f)
{
   uchar c[4];
   errno = 0;
   if (fread(c, sizeof(char), 2, f) != 2)
      CHECK(f);
   if (c[0]=='#' && c[1]=='!') {
      errno = 0;
      int x = getc(f);
      while (x != '\n' && x != '\r' && x != EOF)
         x = getc(f);
      while (x == '\n' || x == '\r')
         x = getc(f);
      if (x == EOF)
         CHECK(f);
      c[0] = x;
      c[1] = getc(f);
   }
   errno = 0;
   if (fread(c+2, sizeof(char), 2, f) != 2)
      CHECK(f);
   return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}



/* --------- DUMP ------------------- */

/* return size of dump file */
static off_t dump(const char *s)
{
   /* Build the big list */
   at *ans = NIL, **where = &ans;
   
   /* 1 - the modules */
   at *p = module_list();
   at *q = p;
   while (CONSP(q)) {
      *where = new_cons(Car(q), NIL);
      where = &Cdr(*where);
      q = Cdr(q);
   }
   /* 2- the globals */
   *where = global_defs();

   /* Header */
   at *atf = OPEN_WRITE(s,"dump");
   FILE *f = Gptr(atf);
   write32(f, DUMPMAGIC);
   write32(f, DUMPVERSION);

   /* The macro character map */
   errno = 0;
   fwrite(char_map,1,256,f);
   test_file_error(f, errno);
   
   /* Write the big list */
   bool oldready = error_doc.ready_to_an_error;
   error_doc.ready_to_an_error = false;
   bwrite(ans, f, true);
   error_doc.ready_to_an_error = oldready;
   lush_delete(atf);     /* close file */

   /* get file size */
   struct stat buf;
   if (stat(s, &buf)>=0)
      if (S_ISREG(buf.st_mode))
         return buf.st_size;
   return (off_t)0;
}


DX(xdump)
{
   ARG_NUMBER(1);
   return NEW_NUMBER((size_t)dump(ASTRING(1)));
}





/* --------- UNDUMP ----------------- */



int isdump(const char *s)
{
   at *atf = OPEN_READ(s,0);
   FILE *f = Gptr(atf);
   int magic = readmagic32(f);
   if (magic != DUMPMAGIC)
      return 0;
   return 1;
}

void undump(char *s)
{
   at *atf = OPEN_READ(s,0);
   FILE *f = Gptr(atf);

   int magic = readmagic32(f);
   int version = read32(f);
   if ( magic != DUMPMAGIC )
      error(NIL, "incorrect dump file format", NIL);
   if ( version > DUMPVERSION )
      error(NIL, "dump file format version not supported", NIL);
   
   /* The macro character map */
   size_t sr = fread(char_map,1,256,f);
   if (sr < 256 || feof(f) || ferror(f))
      error(NIL, "corrupted dump file (1)",NIL);
   
   /* The unified list */
   at *val, *sym, *p = bread(f, NIL);
   while (CONSP(p)) {
      if (CONSP(Car(p))) {
         sym = Caar(p);
         val = Cdar(p);
         ifn (SYMBOLP(sym))
            error(NIL, "corrupted dump file (4)", NIL);
         var_SET(sym, val);
      } else if (SYMBOLP(Car(p)))
         var_lock(Car(p));
      val = p;
      p = Cdr(p);
      Cdr(val) = NIL;
   }
   /* define special symbols */
   at_NULL = var_get(named("NULL"));
}



/* --------- INIT SECTION ----------- */


void init_dump(void)
{
   dx_define("dump", xdump);
}


	    
/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
