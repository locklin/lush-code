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

/***********************************************************************
- output funcs	PRINT_STRING, PRINT_LIST, PRINT_FMT
  lisp aliases  PRINT, PRINTF
- input funcs   READ_STRING, READ_WORD, READ_LIST, SKIP_CHAR, ASK
  lisp aliases  READ_STRING, READ, SKIP_CHAR, ASK
- conversion    PNAME, FIRST_LINE
- macro chars   DMC
********************************************************************** */

#include "header.h"
#include <errno.h>

char *line_buffer;
char *line_pos;
int   line_flush_stdout;
char  print_buffer[LINE_BUFFER];
char  pname_buffer[LINE_BUFFER];

unsigned char char_map[256];

#define CHAR_NORMAL      0x0
#define CHAR_SPECIAL     0x2
#define CHAR_PREFIX      0x4
#define CHAR_SHORT_CARET 0x8
#define CHAR_MCHAR       0x10
#define CHAR_CARET       0x20
#define CHAR_DIEZE       0x40
#define CHAR_BINARY      0x80

#define CHAR_INTERWORD   (CHAR_MCHAR|CHAR_SPECIAL|\
			  CHAR_SHORT_CARET|CHAR_PREFIX|CHAR_BINARY)

#ifndef HAVE_FLOCKFILE
# define flockfile(f)    /* noop */
# define funlockfile(f)  /* noop */
# define getc_unlocked   getc
#endif

static at *at_pname, *at_print, *at_pprint;
static char xdigit[]  = "0123456789abcdef";
static char special[] = "\"\\\n\r\b\t\f\377";
static char aspect[]  = "\"\\nrbtfe";


/* --------- GENERAL PURPOSE ROUTINES --------- */

#define set_char_map(c,f)  char_map[(unsigned char)c]=(unsigned char)(f)
#define get_char_map(c)    (int)char_map[(unsigned char)c]

static bool macrochp(const char *s)
{
   if (!s[1] && (get_char_map(s[0]) & CHAR_MCHAR))
      return true;
   else if (s[0]=='^' && !s[2] && (get_char_map(s[1]) & CHAR_CARET))
      return true;
   else if (s[0]=='#' && !s[2] && (get_char_map(s[1]) & CHAR_DIEZE))
      return true;
   else
      return false;
}

DX(xmacrochp)
{
   ARG_NUMBER(1);
   ASYMBOL(1);
   if (macrochp(NAMEOF(APOINTER(1))))
      return t();
   else
      return NIL;
}


/* --------- CHARACTER PRIMITIVES --------- */

/*
 * print_char Outputs a character on the current output file. Updates
 * context->output_tab if necessary. Handles script features.
 */
void print_char(char c)
{
#if defined(WIN32)
   if (c=='\n')
      print_char('\r');
#endif
#if defined(MAC)
   if (c=='\n')
      c = '\r';
#endif
   if (context->output_file)
      putc(c, context->output_file);

   if (isprint(toascii((unsigned char)c)))
      context->output_tab++;
   else
      switch (c) {
#if defined(MAC)
      case '\r':
#endif
      case '\n':
         context->output_tab = 0;
         if (context->output_file)
            test_file_error(context->output_file, 0);
         if (line_flush_stdout && context->output_file==stdout)
            fflush(stdout);
         break;
         
      case '\b':
         context->output_tab--;
         break;
         
      case '\t':
         context->output_tab |= 0x7;
         context->output_tab++;
         break;
      }
   if (error_doc.script_file) {
      if (error_doc.script_mode != SCRIPT_OUTPUT) {
#if defined(WIN32) || defined(MAC)
         putc('\r', error_doc.script_file);
#endif
#if defined(UNIX) || defined(WIN32)
         putc('\n', error_doc.script_file);
#endif
         error_doc.script_mode = SCRIPT_OUTPUT;
      }
      putc(c, error_doc.script_file);
      if (c == '\n') {
         errno = 0;
         fflush(error_doc.script_file);
         test_file_error(error_doc.script_file, errno);
      }
   }
}


/*
 * read_char Reads a character on the current input stream, eventually get it
 * from  'context->input_buffer'. Updates 'context->input_tab'. If an EOF
 * occurs, insert sometimes a '\n' . Handles script features.
 */

char *prompt_string = 0;

static void fill_line_buffer(void)
{
   while (! *line_pos) {
      line_pos = line_buffer;
      if (feof(stdin)) {
         line_pos[0] = (char)EOF;
         line_pos[1] = 0;

      } else {
         *line_buffer = 0;
         TOPLEVEL_MACHINE;
         console_getline(prompt_string,line_buffer, LINE_BUFFER - 2);
         CHECK_MACHINE("on read");
         char c = 0;
         for (char *s = line_buffer; *s; s++)
            c = *s;
         ifn (c=='\n' || c==(char)EOF) {
#if defined(WIN32) || defined(MAC)
            putc('\r', stdout);
#endif
#if defined(UNIX) || defined(WIN32)
            putc('\n', stdout);
#endif
         }
      }
   }
}

char read_char(void)
{
   int c = EOF;
   if (context->input_string) {
      if (*context->input_string)
         c = *context->input_string++;

   } else if (context->input_file==stdin && prompt_string) {
      fill_line_buffer();
      c = *line_pos++;

   } else if (context->input_file) {
      c = getc(context->input_file);
   }
   /* Handle CR and CR-LF as line separator */
   if (c == '\r') {
      c = '\n';
      if (next_char() == '\n')
         return read_char();
   }
   /* Simulate newline when on EOF */
   if (c==EOF && context->input_tab)
      c = '\n';
   /* Adjust input tab */
   if (isprint(toascii((unsigned char)c)))
      context->input_tab++;
   else
      switch (c) {
      case '\n':
         context->input_tab = 0;
         test_file_error(context->input_file, 0);
         break;

      case '\t':
         context->input_tab |= 0x7;
         context->input_tab++;
         break;
      }
   if (error_doc.script_file) {
      if (c == '\n') {
         putc(c, error_doc.script_file);
         test_file_error(error_doc.script_file, 0);
         error_doc.script_mode = SCRIPT_PROMPT;

      } else {
         if (error_doc.script_mode==SCRIPT_OUTPUT 
             && context->output_tab>0) {
#if defined(WIN32) || defined(MAC)
            putc('\r', error_doc.script_file);
#endif
#if defined(UNIX) || defined(WIN32)
            putc('\n', error_doc.script_file);
#endif
         }
         if (error_doc.script_mode != SCRIPT_INPUT) {
            if (prompt_string)
               fputs(prompt_string, error_doc.script_file);
            else
               fputs("?  ", error_doc.script_file);
            error_doc.script_mode = SCRIPT_INPUT;
         }
         putc(c, error_doc.script_file);
      }
   }
   return (char)c;
}


/*
 * next_char get the next char and replace it in the input buffer
 */

char next_char(void)
{
   int c = EOF;
   if (context->input_string) {
      if (*context->input_string)
         c = *context->input_string;

   } else if (context->input_file==stdin && prompt_string) {
      fill_line_buffer();
      c = *line_pos;

   } else if (context->input_file) {
      c = getc(context->input_file);
      if (c != EOF)
         ungetc(c, context->input_file);
   }
   if (c == '\r')
      c = '\n';
   if (c == EOF && context->input_tab)
      c = '\n';
   return (char) c;
}


/* --------- FLUSH FUNCTIONS --------- */

/*
 * (flush)
 */


DX(xflush)
{
   if (arg_number==1) {
      at *p = APOINTER(1);
      if (RFILEP(p)) {
         if (Gptr(p)==stdin && prompt_string)
            line_pos = "";
      } else if (WFILEP(p)) {
         fflush(Gptr(p));
      } else
         RAISEFX("not a file descriptor", p);

   } else {
      ARG_NUMBER(0);
      if (context->output_file)
         fflush(context->output_file);
      if (context->input_file == stdin)
         line_pos = "";
   }
   return NIL;
}


/* --------- INPUT FUNCTIONS --------- */

/*
 * ask(s) puts a question and waits for an interactive user's answer. if the
 * answer is YES returns 1 if the answer is NO	 returns 0 if the
 * answer is EOF returns -1. (waits for eigth ones)
 * 
 * Lisp's ASK returns T or ()
 */

int ask(const char *t)
{
   int eof = 8;
  
   putc('\n', stdout);
   for (;;) {
      char question[1024];
      strncpy(question, t, 1000);
      strcat(question," [y/N] ?");
      console_getline(question, line_buffer, LINE_BUFFER - 2);
      CHECK_MACHINE("on read");
      line_pos = "";
      char *s = line_buffer;
      if (feof(stdin) || *s==(char)EOF) {
         if (! eof--)
	    return -1;
         clearerr(stdin);
      } else {
         if (*s=='\n' || *s=='\r')
	    return 0;
         while (*s && isascii((unsigned char)*s) && isspace((unsigned char)*s))
	    s++;
         switch (*s) {
         case 'o':
         case 'O':
         case 'y':
         case 'Y':
            return 1;
         case 'n':
         case 'N':
            return 0;
         }
      }
   }
}

DX(xask)
{
   ARG_NUMBER(1);
   if (ask(ASTRING(1))==1)
      return t();
   else
      return NIL;
}



/*
 * make_testchar_map(s,buf[256])
 */
static void make_testchar_map(const char *s, char *buf)
{
   memset(buf, 0, 256);
   if (s) {
      int old = -1;
      int yes = true;
      if (*s == '~') {
         memset(buf, yes, 256);
         yes = false;
         s++;
      }
      while (*s) {
         if (old>0 && s[0]=='-' && s[1]) {
            while (old < ((unsigned char*)s)[1])
               buf[old++] = yes;
            old = -1;
         } else {
            old = *(unsigned char*)s;
            buf[old] = yes;
         }
         s += 1;
      } 
   }
}


/*
 * skip_char(s) skips any char matched by the string s returns the next char
 * available
 */
char skip_char(const char *s)
{
   char map[256];
   
   make_testchar_map(s, map);
   map[255] = false;
   map['\r'] |= map['\n'];

   int c;
   if (context->input_string || (context->input_file==stdin && prompt_string)) {
      /* Standard implementation */
      while (map[(unsigned char)(c = next_char())])
         read_char();
      
   } else {
      /* Go as fast as we can */
      c = EOF;
      errno = 0;
      if (context->input_file) {
         flockfile(context->input_file);
         c = getc_unlocked(context->input_file);
         while (map[(unsigned char)c]) {
            if (isprint(toascii((unsigned char)c)))
               context->input_tab++;
            else
               switch (c) {
               case '\n':
               case '\r':
                  context->input_tab = 0;
                  break;
               case '\t':
                  context->input_tab |= 0x7;
                  context->input_tab++;
                  break;
               }
            c = getc_unlocked(context->input_file);
         }
         funlockfile(context->input_file);

         if (ferror(context->input_file)) {
#ifdef EINTR
            if (errno == EINTR)
               clearerr(context->input_file);
            else
#endif
               test_file_error(context->input_file, errno);
         }
         if (c != EOF)
            ungetc(c, context->input_file);
      }
   }
   return c;
}

DX(xskip_char)
{
   const char *s;
   if (arg_number) {
      ARG_NUMBER(1);
      s = ASTRING(1);
   } else
      s = " \n\r\t\f";

   char answer[2];
   answer[1] = 0;
   answer[0] = skip_char(s);
   return make_string(answer);
}


static at *read_string(const char *s)
{
   char map[256];
   make_testchar_map(s, map);
   map[255] = false;
   map['\r'] |= map['\n'];

   struct large_string ls;
   large_string_init(&ls);

   flockfile(context->input_file);
   while (map[(unsigned char)next_char()]) {
      char c = read_char();
      large_string_add(&ls, &c, 1);
   }
   funlockfile(context->input_file);

   return large_string_collect(&ls);
}


static at *read_string_n(int n)
{
   struct large_string ls;
   large_string_init(&ls);
   
   flockfile(context->input_file);
   for (int i=0; i<n; i++) {
      char c = read_char();
      if (c == (char)EOF) break;
      large_string_add(&ls, &c, 1);
   }
   funlockfile(context->input_file);
   
   return large_string_collect(&ls);
}


DX(xread_string)
{
   const char *s = "~\n\r\377";
   if (arg_number) {
      ARG_NUMBER(1);
      if (ISNUMBER(1))
         return read_string_n(AINTEGER(1));
      s = ASTRING(1);
   }
   return read_string(s);
}


/* 
 * Skip chars until reaching some expression 
 */
char skip_to_expr(void)
{
   for(;;) {
      char c = next_char();
      if (c == ';') {		/* COMMENT */
         skip_char("~\n\r\377");
      } else if (isascii((unsigned char)c) 
                 && isspace((unsigned char)c) ) { 
         read_char();
      } else
         return c;
   }
   return 0;
}

/*
 * read_word reads a lisp word. if the word was a quoted symbol, returns |xxx
 * if it was a string, returns "xxx" if anything else, return it
 */
static char *read_word(void)
{
   char *s = string_buffer;
   char c = skip_to_expr();
   if (c == '|') {
      *s++ = read_char();
      until((c = read_char())=='|' || c==(char)EOF) {
         if (isascii(c) && iscntrl(c))
            goto errw1;
         else if (s < string_buffer + STRING_BUFFER - 1)
            *s++ = c;
         else
            goto errw2;
      }

   } else if (c == '\"' /*"*/) {   
      *s++ = read_char();
      until((c = read_char())=='\"' || c==(char)EOF) {  
         if (isascii(c) && iscntrl(c))
            goto errw1;
         else if (s < string_buffer + STRING_BUFFER - 2)
            *s++ = c;
         else
            goto errw2;
         if (c == '\\') {
            c = *s++ = read_char();
            if (iscntrl(toascii((unsigned char)c)) && (c != '\n'))
               goto errw1;
         }
      }
   } else if (get_char_map(c) & CHAR_BINARY) {
      *s++ = c;  /* Marker for binary data */

   } else if (get_char_map(c) & CHAR_PREFIX) {
      *s++ = read_char();
      *s++ = read_char();

   } else if (get_char_map(c) & CHAR_SHORT_CARET) {
      *s++ = '^';
      *s++ = read_char() + 0x40;

   } else if (get_char_map(c) & (CHAR_MCHAR | CHAR_SPECIAL)) {
      *s++ = read_char();

   } else {
      until((c = next_char(), (get_char_map(c) & CHAR_INTERWORD) ||
             (isascii((unsigned char)c) && isspace((unsigned char)c)) ||
	   (c == (char) EOF))) {
         c = read_char();
         if (!isascii(c) || iscntrl(c))
            goto errw1;
         else if (s < string_buffer + STRING_BUFFER - 2) {
/*             if (s > string_buffer && c == '_')  */
/*                c = '-'; */
            if (! context->input_case_sensitive)
               c = tolower(c);
            *s++ = c;
         } else
            goto errw2;
      }
   }
   *s = 0;
   return string_buffer;

errw1:
   sprintf(string_buffer, "illegal character : 0x%x", c & 0xff);
   error("read", string_buffer, NIL);
errw2:
   error("read", "string too long", NIL);
}

static at *rl_utf8(long h)
{
   char ub[8];
   char *u = ub;
   if (h > 0x10ffff)
      return 0;

   else if (h > 0xffff) {
      *u++ = 0xe0 | (unsigned char)(h>>18);
      *u++ = 0x80 | (unsigned char)((h>>12)&0x3f);
      *u++ = 0x80 | (unsigned char)((h>>6)&0x3f);
      *u++ = 0x80 | (unsigned char)(h&0x3f);

   } else if (h > 0x7ff) {
      *u++ = 0xe0 | (unsigned char)(h>>12);
      *u++ = 0x80 | (unsigned char)((h>>6)&0x3f);
      *u++ = 0x80 | (unsigned char)(h&0x3f);

   } else if (h > 0x7f) {
      *u++ = 0xc0 | (unsigned char)(h>>6);
      *u++ = 0x80 | (unsigned char)(h&0x3f);
   } else {
      *u++ = (unsigned char)h;
   }
   *u++ = 0;
   return str_utf8_to_mb(ub);
}
 

/* read_list reads a regular lisp object (list, string, symbol, number) */
static at *rl_string(char *s)
{
   char *d = string_buffer;
   while (*s) {
      if (*s == '\\' && s[1]) {
         s++;
         char *ind = strchr(aspect, *s);
         if (ind) {
            *d++ = special[ind - aspect];
            s++;
            
         } else if (*s == 'x' || *s == 'X') {
            int c, h = 0;
            s++;
            for (c = 0; c < 2; c++) {
               ind = strchr(xdigit, tolower((unsigned char) *s));
               if (*s && ind) {
                  h *= 16;
                  h += (ind - xdigit);
                  s++;
               } else
                  break;
            }
            ifn(c)
               goto err_string;
            *d++ = h;
            
         } else if (*s == 'u' || *s == 'U') {
            unsigned long h = 0;
            int c = ((*s == 'u') ? 4 : 8);
            at *m;
            s++;
            for (; c > 0; c--) {
               ind = strchr(xdigit, tolower((unsigned char)*s));
               if (*s && ind) {
                  h *= 16;
                  h += (ind - xdigit);
                  s++;
               } else
                  break;
            }
            m = rl_utf8(h);
            ifn (STRINGP(m))
               goto err_string;
            strcpy(d, String(m));
            d += strlen(d);
            
         } else if (*s == '^' && s[1]) {	/* control */
            *d++ = (s[1]) & (0x1f);
            s += 2;
            
         } else if (*s == '+' && s[1]) {	/* high bit latin1*/
#if HAVE_ICONV
            at *m = rl_utf8(s[1] | 0x80);
            ifn (STRINGP(m))
               goto err_string;
            strcpy(d, String(m));
            d += strlen(d);
#else
            *d++ = (s[1]) | 0x80;
#endif
            s += 2;

         } else if (*s == '\n') {	/* end of line */
            s++;
            
         } else {			/* octal */
            int c, h = 0;
            for (c = 0; c < 3; c++)
               if (*s >= '0' && *s <= '7') {
                  h *= 8;
                  h += *s++ - '0';
               } else
                  break;
            ifn(c)
               goto err_string;
            *d++ = h;
         }
         
      } else			/* other */
         *d++ = *s++;
   }
   *d = 0;
   return make_string(string_buffer);
   
err_string:
   error(NIL, "bad backslash sequence in a string", NIL);
}

static at *rl_mchar(const char *s)
{
   ifn (macrochp(s)) {
      if (s[0] == '^')
         error("read", "undefined caret (^) char", make_string(s));
      else if (s[0] == '#')
         error("read", "undefined dieze (#) char", make_string(s));
      else
         error("io", "internal mchar failure", NIL);
   }

   at *q = named(s);
   at *p = var_get(q);
   at *(*sav_ptr) (at *) = eval_std;
   eval_ptr = eval_std;
   at *answer = apply(p, NIL);
   eval_ptr = sav_ptr;
   return answer;
}

static at *rl_read(char *s)
{
   at *answer, **where;
   
read_again1:
   
   /* DOT */
   if (s[0] == '.' && !s[1])
      goto err_read1;
   
   /* LIST */
   if (s[0] == '(' && !s[1]) {
      where = &answer;
      answer = NIL;
      
   read_again2:
      s = read_word();
      if (!s[0]) {
         goto err_read0;

      } else if (s[0] == ')' && !s[1]) {
         return answer;

      } else if (! where) {
         goto err_read1;

      } else if (s[0] == '.' && !s[1]) {
         s = read_word();
         if (! s[0])
            goto err_read0;
         else if (s[1]==')' && !s[1])
            goto err_read1;
         *where = rl_read(s);
         where = NIL;
         goto read_again2;

      } else if (s[0] == '#') {
         at *t = rl_mchar(s);
         if (t && !CONSP(t))
            goto err_read2;
         *where = t;

      } else {
         *where = new_cons(rl_read(s), NIL);
      }
      while (CONSP(*where))
         where = &Cdr(*where);
      goto read_again2;
   }
  
   /* BINARY TOKENS */
   if (get_char_map(s[0]) & CHAR_BINARY) {
      if (! context->input_string)
         return bread(context->input_file, NIL);
      error("read", "cannot (yet) read binary tokens from a string", NIL);
   }
   
   /* MACRO CHARS */
   if (get_char_map(s[0]) & CHAR_MCHAR)
      return rl_mchar(s);
  
   if (s[0] == '^')
      return rl_mchar(s);
  
   if (s[0] == '#') {
      answer = rl_mchar(s);
      if (!answer) {
         s = read_word();
         goto read_again1;
      } else if (CONSP(answer) && !Cdr(answer)) {
         return Car(answer);
      } else
         goto err_read2;
   }
  
   /* STRING */
   if (s[0] == '\"')  /*"*/
      return rl_string(s + 1);
  
   /* NUMBER */
   if ((answer = str_val(s)))	
      return answer;
  
   /* QUOTED SYMBOL */
   if (s[0] == '|')
      return named(s + 1);
   
   /* SYMBOL */
   if (s[0])
      return named(s);
  
   /* EOF */
   return NIL;
  
err_read0:
   error("read", "end of file", NIL);
err_read1:
   error("read", "bad dotted construction", NIL);
err_read2:
   error("read", "bad dieze (#) macro-char", NIL);
}

at *read_list(void)
{
   MM_ENTER;
   while (skip_to_expr() == ')')
      read_char();
   MM_RETURN(rl_read(read_word()));
}

DX(xread)
{
   ARG_NUMBER(0);
   at *answer = read_list();
   if (!answer && feof(context->input_file))
      RAISEFX("end of file", NIL);
   return answer;
}


/* --------- MACRO-CHARS DEFINITION --------- */

const char *dmc(const char *s, at *l)
{
   int type;
   char c;
   
   if (s[0] == '^' && (c = s[1]) && !s[2]) {
      type = CHAR_CARET;
   } else if (s[0] == '#' && (c = s[1]) && !s[2]) {
      type = CHAR_DIEZE;
   } else {
      type = CHAR_MCHAR;
      c = s[0];
      if (s[1] || (get_char_map(s[0]) & CHAR_SPECIAL))
         RAISEF("illegal macro-character", NEW_STRING(s));
   }
   if ((get_char_map(c) & CHAR_SPECIAL))
      RAISEF("illegal macro-character", NEW_STRING(s));

   at *q = NEW_SYMBOL(s);
   ifn (SYMBOLP(q))
      RAISEF("can't define this symbol", NIL);

   if (l) {
      l = new_de(NIL, l);
      var_set(q, l);
      char_map[(unsigned char) c] |= type;
   } else {
      var_set(q, NIL);
      char_map[(unsigned char) c] &= ~type;
   }
   return s;
}

DY(ydmc)
{
   ifn (CONSP(ARG_LIST))
      RAISEFX("bad arguments", ARG_LIST);
   at *q = Car(ARG_LIST);
   at *l = Cdr(ARG_LIST);

   ifn (SYMBOLP(q))
      RAISEF("not a symbol", q);

   dmc(NAMEOF(q), l);
   return q;
}


/* --------- OUTPUT FUNCTIONS --------- */


/*
 * print_string s prints the string s via the print_char routine.
 */
void print_string(const char *s)
{
   if (s) {
      flockfile(context->input_file);
      while (*s)
         print_char(*s++);
      funlockfile(context->input_file);
   }
}


/*
 * print_tab n prints blanks until context->output_tab >= n
 */
void print_tab(int n)
{
   flockfile(context->input_file);
   while (context->output_tab < n) {
      print_char(' ');
      if (context->output_tab == 0)	/* WIDTH set */
         break;
   }
   funlockfile(context->input_file);
}

DX(xtab)
{
   if (arg_number) {
      ARG_NUMBER(1);
      print_tab(AINTEGER(1));
   }
   return NEW_NUMBER(context->output_tab);
}

/*
 * print_list l prints the list l:
 *  note infinite recursion avoidance
 */

void print_list(at *list)
{
   MM_ENTER;
   if (list == NIL)
      print_string("()");

   else if (CONSP(list)) {
      at *l = list;
      at *slow = list;
      char toggle = 0;
      struct recur_elt elt;
      print_char('(');
      for(;;) {
         if (recur_push_ok(&elt, (void *)&print_list, Car(l))) {
            print_list(Car(l));
            recur_pop(&elt);
         } else
            print_string("(...)");

         l = Cdr(l);
         if (l == slow) {
            print_string(" ...");
            l = NIL;
            break;
         }
         ifn (CONSP(l))
            break;

         toggle ^= 1;
         if (toggle)
            slow = Cdr(slow);
         print_char(' ');
      }
      if (l && !ZOMBIEP(l)) {
         print_string(" . ");
         print_list(l);
      }
      print_char(')');

   } else {
      struct recur_elt elt;
      class_t *cl = classof(list);
      at *l = getmethod(cl, at_print);
      if (l && recur_push_ok(&elt, (void *)&print_string,list)) {
         list = send_message(NIL, list, at_print, NIL);
         recur_pop(&elt);
         MM_RETURN_VOID;
      }
      print_string(pname(list));
   }
   MM_EXIT;
}

DX(xprint)
{
   for (int i = 1; i <= arg_number; i++) {
      print_list(APOINTER(i));
      if (i<arg_number)
         print_char(' ');
   }
   print_char('\n');
   return arg_number ? APOINTER(arg_number) : NIL;
}

DX(xprin)
{
   for (int i = 1; i <= arg_number; i++) {
      print_list(APOINTER(i));
      if (i<arg_number)
         print_char(' ');
   }
   return arg_number ? APOINTER(arg_number) : NIL;
}


/*
 * printf C'printf interface. format: %{-}{n}{.{m}}{c|d|s|f|e|g|l|p}  ( l for
 * list, p for pretty ) or   : %%
 */


DX(xprintf)
{
   if (arg_number < 1)
      error(NIL, "format string expected", NIL);

   const char *fmt = ASTRING(1);

   int i = 1;
   for(;;) {
      if (*fmt == 0)
         break;

      char *buf = print_buffer;
      while (*fmt && *fmt!='%' 
             && buf<(print_buffer+ LINE_BUFFER - 10))
         *buf++ = *fmt++;
      *buf = 0;

      print_string(print_buffer);
      if (*fmt != '%')
         continue;
      buf = print_buffer;
      int n = 0;
      int ok = 0;
      char c = 0;
      
      *buf++ = *fmt++;		/* copy  '%' */
      while (ok < 9) {
         c = *buf++ = *fmt++;

         switch (c) {
         case 0:
            goto err_printf0;
         case '-':
            if (ok >= 1)
               goto err_printf0;
            else
               ok = 1;
            break;
         case '.':
            if (ok >= 5)
               goto err_printf0;
            else
               ok = 5;
            break;
         case '%':
         case 'l':
         case 'p':
            if (ok >= 1)
               goto err_printf0;
            else
               ok = 10;
            break;
         case 'd':
         case 'c':
         case 's':
            if (ok >= 5)
               goto err_printf0;
            else if (ok)
               ok = 10;
            else
               ok = 9;
            break;
         case 'f':
         case 'g':
         case 'e':
            if (ok)
               ok = 10;
            else
               ok = 9;
            break;
         default:
            if (!isdigit((unsigned char)c))
               goto err_printf0;
            if (ok <= 4)
               n = (n * 10) + (c - '0');
            if (ok <= 4)
               ok = 4;
            else if (ok <= 8)
               ok = 8;
            else
               goto err_printf0;
         }
      }
      
      *buf = 0;
      if (c!='%' && (++i)>arg_number)
         goto err_printf1;
      
      if (c == 'l')
         print_list(APOINTER(i));

/*       else if (c == 'p') { */
/*          at *args = new_cons(APOINTER(i), NIL); */
/*          apply(Value(at_pprint),args); */

      else if (c == 'p') {
         *buf++ = 0;
         at *a = APOINTER(i);
         ifn (GPTRP(a) || MPTRP(a)) AGPTR(i);
         if (ok == 9) {
            print_string(str_number_hex((double)(intptr_t)Gptr(a)));
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, Gptr(a));
            print_string(buf);
         }

      } else if (c == 'c') {
         *buf++ = 0;
         if (ok == 9) {
            print_char((char)AINTEGER(i));
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, (char)AINTEGER(i));
            print_string(buf);
         }

      } else if (c == 'd') {
         *buf++ = 0;
         if (ok == 9) {
            print_string(str_number((real) AINTEGER(i)));
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, AINTEGER(i));
            print_string(buf);
         }

      } else if (c == 's') {
         *buf++ = 0;
         if (ok == 9) {
            print_string(ASTRING(i));
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, ASTRING(i));
            print_string(buf);
         }
      } else if (c == 'e' || c == 'f' || c == 'g') {
         *buf++ = 0;
         if (ok == 9) {
            print_string(str_number(AREAL(i)));
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, AREAL(i));
            print_string(buf);
         }
      }
      if (c == '%')
         print_char('%');
   }
   if (i < arg_number)
      goto err_printf1;
   return NIL;
   
err_printf0:
   RAISEFX("bad format string", NIL);
err_printf1:
   RAISEFX("bad argument number", NIL);
   return NIL; // make compiler happy
}


/* --------- LIST TO STRING CONVERSION ROUTINES --------- */

static char *convert(char *s, at *list, char *end)
{
   if (s > end - 8 )
      goto exit_convert;

   ifn (list) {			/* PNAME :   NIL    =>   '()'	 */
      *s++ = '(';
      *s++ = ')';
      *s = 0;
      return s;

   } else if (CONSP(list)) {     /* PNAME :   LIST   =>  '(...)'	 */
      *s++ = '(';
      if (s > end - 8 )
         goto exit_convert;
      forever {
         ifn ((s = convert(s, Car(list), end)))
            return 0L;
         list = Cdr(list);
         ifn (CONSP(list))
            break;
         *s++ = ' ';
      }
      if (list && !ZOMBIEP(list)) {
         *s++ = ' ';
         *s++ = '.';
         *s++ = ' ';
         ifn ((s = convert(s, list, end)))
            return NULL;
      }
      *s++ = ')';
      if (s > end - 8 )
         goto exit_convert;
      *s = 0;
      return s;

   } else {
      
      const char *n = NULL;	
      at *p = getmethod(Class(list), at_pname); 
      if (p) {
         at *q = send_message(NIL, list, at_pname, NIL);
         ifn (STRINGP(q))
            error(NIL, "pname does not return a string", q);
         n = String(q);
         
      } else if (Class(list)->name) {
         n = Class(list)->name(list);

      }
      if (n == NIL)
         error("io.c/convert", "internal error", NIL);
      
      int mode = 0;
      if (SYMBOLP(list)) {
         for (const char *m = n; *m; m++)
            if (!isascii((unsigned char)*m) || 
                iscntrl((unsigned char)*m) ||
                ((!context->input_case_sensitive) && isupper((unsigned char)*m)) ||
                /* (m>n && *m=='_') || */
                (get_char_map(*m) & CHAR_INTERWORD) ) {
               mode = 1;
               break;
            }
         if(!*n)
            mode = 1;
         if (!mode && str_val(n))
            mode = 1;
      }
      if (mode)
         *s++ = '|';
      while (*n)
         if (s < end-8)
            *s++ = *n++;
         else
            goto exit_convert;
      if (mode)
         *s++ = '|';
      *s = 0;
      return s;
   }
exit_convert:
      *s = 0;
      strcpy(s, " ... ");
      return NULL;
}


/*
 * first_line l returns the first line of list l (70 characters max)
 */
const char *first_line(at *l)
{
   convert(pname_buffer, l, pname_buffer+70);
   return mm_strdup(pname_buffer);
}

DX(xfirst_line)
{
   ARG_NUMBER(1);
   return NEW_STRING(first_line(APOINTER(1)));
}


/*
 * pname l returns the string image of the list l
 */
const char *pname(at *l)
{
   convert(pname_buffer, l, pname_buffer+LINE_BUFFER );
   return mm_strdup(pname_buffer);
}

DX(xpname)
{
   ARG_NUMBER(1);
   return NEW_STRING(pname(APOINTER(1)));
}



/* --------- INITIALISATION CODE --------- */

void init_io(void)
{
   line_buffer = mm_allocv(mt_blob, LINE_BUFFER);
   MM_ROOT(line_buffer);
   line_pos = line_buffer;

   set_char_map(')', CHAR_SPECIAL);
   set_char_map('(', CHAR_SPECIAL);
   set_char_map(';', CHAR_SPECIAL);
   set_char_map('\"' /*"*/, CHAR_SPECIAL);
   set_char_map('|', CHAR_SPECIAL);
   set_char_map('^', CHAR_PREFIX);
   set_char_map('#', CHAR_PREFIX);

   for (int i=0; i<' '; i++)
      ifn (isspace(i))
         set_char_map(i, CHAR_SHORT_CARET);
   
   set_char_map(0x9f, CHAR_BINARY);
   
   dx_define("macrochp", xmacrochp);
   dx_define("flush",xflush);
   dx_define("ask", xask);
   dx_define("skip-char", xskip_char);
   dx_define("read-string", xread_string);
   dx_define("read", xread);
   dy_define("dmc", ydmc);
   dx_define("tab", xtab);
   dx_define("print", xprint);
   dx_define("prin", xprin);
   dx_define("printf", xprintf);
   dx_define("pname", xpname);
   dx_define("first-line", xfirst_line);

   at_pname = var_define("pname");
   at_print = var_define("print");
   at_pprint = var_define("pprint");
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
