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
	io.c

- output funcs	PRINT_STRING, PRINT_LIST, PRINT_FMT
  lisp aliases                PRINT       PRINTF

- input funcs   READ_STRING, READ_WORD, READ_LIST, SKIP_CHAR,  ASK
  lisp aliases  READ_STRING, 		  READ,    SKIP_CHAR   ASK

- conversion lisp -> string   PNAME,  FIRST_LINE

- macro chars   DMC

$Id: io.c,v 1.1.1.1 2002-04-16 17:37:38 leonb Exp $
********************************************************************** */

#include "header.h"


static at *at_pname, *at_print, *at_pprint;

char *line_buffer;
char *line_pos;
char *print_buffer;
char *pname_buffer;

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



/* --------- ALTERNATE TOUPPER TOLOWER FUNCTIONS ---------------- */

#ifdef NEED_TOLOWER
int 
tolower(int c)
{
  if (isupper(toascii((unsigned char)c)))
    return c - 'A' + 'a';
  else
    return c;
}
#endif

#ifdef NEED_TOUPPER
int 
toupper(int c)
{
  if (islower(toascii((unsigned char)c)))
    return c - 'a' + 'A';
  else
    return c;
}
#endif


/* --------- GENERAL PURPOSE ROUTINES --------- */



#define set_char_map(c,f)  char_map[(unsigned char)c]=(unsigned char)(f)
#define get_char_map(c)    (int)char_map[(unsigned char)c]

static int 
macrochp(register char *s)
{
  if (!s[1] && 
      (get_char_map(s[0]) & CHAR_MCHAR))
    return TRUE;
  else if (s[0] == '^' && !s[2] &&
	   (get_char_map(s[1]) & CHAR_CARET))
    return TRUE;
  else if (s[0] == '#' && !s[2] &&
	   (get_char_map(s[1]) & CHAR_DIEZE))
    return TRUE;
  else
    return FALSE;
}

DX(xmacrochp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  ASYMBOL(1);
  if (macrochp(nameof(APOINTER(1))))
    return true();
  else
    return NIL;
}


/*
 * test_char(c,s) does the string s match the character c ? the string s is
 * an optionnal tilde  '~', any string of characters, eventually separed by
 * '-'.
 */
static int 
test_char(char c, register char *s)
{
  register int found;
  register int old;

  ifn(s)
    return FALSE;

  if (*s == '~') {
    found = FALSE;
    s++;
  } else
    found = TRUE;

  old = 0;
  while (*s) {
    if (s[0] == '-' && old && s[1]) {
      if (c > old && c <= s[1])
	return found;
      else
	++s;
    }
    if (c == s[0])
      return found;
    old = *s++;
  }

  return !found;
}

/* --------- CHARACTER PRIMITIVES --------- */

/*
 * print_char Outputs a character on the current output file. Updates
 * context->output_tab if necessary. Handles script features.
 */
void 
print_char(register char c)
{
#if defined(WIN32)
  if (c=='\n')
    print_char('\r');
#endif
#if defined(MAC)
  if (c=='\n')
    c = '\r';
#endif
  if (context->output_file) {
    if (context->output_file == stdout)
      putc(c,stdout);
    else
      putc(c, context->output_file);
  }
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
	  test_file_error(context->output_file);
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
      fflush(error_doc.script_file);
      test_file_error(error_doc.script_file);
    }
  }
}


/*
 * read_char Reads a character on the current input stream, eventually get it
 * from  'context->input_buffer'. Updates 'context->input_tab'. If an EOF
 * occurs, insert sometimes a '\n' . Handles script features.
 */

char *prompt_string = 0;

char 
read_char(void)
{
  register char c, *s;
  
  if (context->input_file==stdin && prompt_string) {
    until(*line_pos) {
      line_pos = line_buffer;
      if (feof(stdin)) {
	line_pos[0] = (char)EOF;
	line_pos[1] = 0;
      } else {
	*line_buffer = 0;
	c = 0;
	TOPLEVEL_MACHINE;
	console_getline(prompt_string,line_buffer, LINE_BUFFER - 2);
	CHECK_MACHINE("on read");
	for (s = line_buffer; *s; s++)
	  c = *s;
	ifn (c == '\n' || c == (char) EOF) {
#if defined(WIN32) || defined(MAC)
	  putc('\r', stdout);
#endif
#if defined(UNIX) || defined(WIN32)
	  putc('\n', stdout);
#endif
	}
      }
    }
    c = *line_pos++;
  } else if (context->input_file) {
    c = getc(context->input_file);
  } else
    c = (char)EOF;
  /* Handle CR and CR-LF as line separator */
  if (c == '\r') {
    c = '\n';
    if (next_char() == '\n')
      return read_char();
  }
  /* Simulate newline when on EOF */
  if (c == (char) EOF && context->input_tab)
    c = '\n';
  /* Adjust imput tab */
  if (isprint(toascii((unsigned char)c)))
    context->input_tab++;
  else
    switch (c) {
    case '\n':
      context->input_tab = 0;
      test_file_error(context->input_file);
      break;
    case '\t':
      context->input_tab |= 0x7;
      context->input_tab++;
      break;
    }
  if (error_doc.script_file) {
    if (c == '\n') {
      putc(c, error_doc.script_file);
      test_file_error(error_doc.script_file);
      error_doc.script_mode = SCRIPT_PROMPT;
    } else {
      if (error_doc.script_mode == SCRIPT_OUTPUT 
	  && context->output_tab > 0 ) {
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
  return c;
}


/*
 * next_char get the next char and replace it in the input buffer
 */
char 
next_char(void)
{
  register char c, *s;
  
  if (context->input_file==stdin && prompt_string) {
    until(*line_pos) {
      line_pos = line_buffer;
      if (feof(stdin)) {
	line_pos[0] = (char)EOF;
	line_pos[1] = 0;
      } else {
	*line_buffer = 0;
	c = 0;
	TOPLEVEL_MACHINE;
	console_getline(prompt_string,line_buffer, LINE_BUFFER - 2);
	CHECK_MACHINE("on read");
	for (s = line_buffer; *s; s++)
	  c = *s;
	ifn (c == '\n' || c == (char) EOF)
	  putc('\n', stdout);
      }
    }
    c = *line_pos;
  } else if (context->input_file) {
    c = getc(context->input_file);
    if (c != (char)EOF)
      ungetc(c,context->input_file);
  } else
    c = (char)EOF;
  if (c == '\r')
    c = '\n';
  if (c == (char) EOF && context->input_tab)
    c = '\n';
  return c;
}



/* --------- FLUSH FUNCTIONS --------- */

/*
 * (flush)
 */


DX(xflush)
{
  if (arg_number==1) 
    {
      at *p;
      ARG_EVAL(1);
      p = APOINTER(1);
      if (p && (p->flags&C_EXTERN) && p->Class==&file_R_class) {
	if (p->Object==stdin && prompt_string)
	  line_pos = "";
      } else if (p && (p->flags&C_EXTERN) && p->Class==&file_W_class) {
	fflush(p->Object);
      } else
	error(NIL,"Not a file descriptor",p);
    }
  else
    {
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

int 
ask(char *t)
{
  register char *s;
  register int eof = 8;
  
  putc('\n', stdout);
  forever
    {
      char question[1024];
      strncpy(question,t,1000);
      strcat(question," [y/n] ?");
      console_getline(question, line_buffer, LINE_BUFFER - 2);
      CHECK_MACHINE("on read");
      line_pos = "";
      s = line_buffer;
      if (feof(stdin)) 
	{
	  if ( !eof-- )
	    return -1;
	  clearerr(stdin);
	} 
      else 
	{
	  if (*s == '\n' || *s == '\r')
	    return 0;
	  while (*s && isascii((unsigned char)*s) && isspace((unsigned char)*s))
	    s++;
	  switch (*s) 
	    {
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
  ARG_EVAL(1);
  if (ask(ASTRING(1))==1)
    return true();
  else
    return NIL;
}

/*
 * skip_char(s) skips any char matched by the string s returns the next char
 * available
 */
char 
skip_char(char *s)
{
  register char c;
  while (test_char(c = next_char(), s) && c != (char) EOF)
    read_char();
  return c;
}

DX(xskip_char)
{
  char *s, answer[2];

  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    s = ASTRING(1);
  } else
    s = " \n\r\t\f";
  answer[1] = 0;
  answer[0] = skip_char(s);
  return new_string(answer);
}


/*
 * read_string(s) reads a string whose characters match 's
 */
char *
read_string(char *s)
{
  register char *bpos;

  bpos = string_buffer;
  while (test_char(next_char(), s)) {
    if (bpos < string_buffer + STRING_BUFFER - 1) {
      if ((*bpos++ = read_char()) == (char) EOF)
	break;
    } else
      error("read", "too long string", NIL);
  }
  *bpos = 0;
  return string_buffer;
}

DX(xread_string)
{
  register char *s;
  register int i;

  s = "~\n\r\377";
  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    if (ISNUMBER(1)) {
      s = string_buffer;
      i = AINTEGER(1);
      if (i < 0 || i > STRING_BUFFER - 1)
	error("read", "out of range", APOINTER(1));

      while (i-- > 0)
	if ((*s++ = read_char()) == (char) EOF)
	  break;
      *s = 0;
      return new_string(string_buffer);
    } else
      s = ASTRING(1);
  }
  return new_string(read_string(s));
}


/*
 * read_word reads a lisp word. if the word was a quoted symbol, returns |xxx
 * if it was a string, returns "xxx" if anything else, return it
 */
char *
read_word(void)
{
  register char c, *s;

  s = string_buffer;
  while ((c = next_char(), isascii((unsigned char)c) && isspace((unsigned char)c)))
    read_char();

  if (c == '|') {
    *s++ = read_char();
    until((c = read_char()) == '|' || c == (char) EOF) {
      if (iscntrl(toascii((unsigned char)c)))
	goto errw1;
      else if (s < string_buffer + STRING_BUFFER - 1)
	*s++ = c;
      else
	goto errw2;
    }
  } else if (c == '\"' /*"*/) {   
    *s++ = read_char();
    until((c = read_char()) == '\"' /*"*/ || c == (char) EOF) {  
      if (iscntrl(toascii((unsigned char)c)))
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
      if (iscntrl(toascii((unsigned char)c)))
	goto errw1;
      else if (s < string_buffer + STRING_BUFFER - 2)
	*s++ = tolower((unsigned char)c);
      else
	goto errw2;
    }
  }
  *s = 0;
  return string_buffer;

errw1:
  sprintf(string_buffer, "illegal character : 0x%x", c & 0xff);
  error("read", string_buffer, NIL);
errw2:
  error("read", "too long string", NIL);
}



/*
 * read_list reads a regular lisp object:  list, string, symbol, or number
 * uses routines:  	read_list1 back_slash_copy
 * 
 */

static at *rl_read(register char *s);

at *
read_list(void)
{
  char *sav_prompt, *word, c;
  at *ans;
  
  sav_prompt = prompt_string;

 read_list:

  word = read_word();
  c = word[1] ? 0 : word[0];
  if (c == ';') {		/* COMMENT			 */
    read_string("~\n\r\377");
    goto read_list;
  }
  if (c == ')') {		/* EXTRA RIGHT PARENTHESIS	 */
    goto read_list;
  }
  ans = rl_read(word);

  prompt_string = sav_prompt;
  return ans;
}

DX(xread)
{
  at *answer;

  ARG_NUMBER(0);
  answer = read_list();
  if (feof(context->input_file))
    error(NIL, "End of file", NIL);
  return answer;
}



static at *
rl_string(register char *s)
{
  register char *d, *ind;

  d = string_buffer;
  while (*s) {
    if (*s == '\\' && s[1]) {
      s++;

      if ((ind = strchr(aspect_string, *s))) { /* special */
	*d++ = special_string[ind - aspect_string];
	s++;

	
      } else if (*s == 'x' || *s == 'X') {
	register int h, c;	/* hexa */
	h = 0;
	s++;
	for (c = 0; c < 2; c++) {
	  ind = strchr(digit_string, tolower((unsigned char)*s));
	  if (*s && ind) {
	    h *= 16;
	    h += (ind - digit_string);
	    s++;
	  } else
	    break;
	}
	ifn(c)
	  goto err_string;
	*d++ = h;

      } else if (*s == '^' && s[1]) {	/* control */
	*d++ = (s[1]) & (0x1f);
	s += 2;
	
      } else if (*s == '+' && s[1]) {	/* high bit */
	*d++ = (s[1]) | (0x80);
	s += 2;
	
      } else if (*s == '\n') {	/* end of line */
	s++;
	
      } else {			/* octal */
	register int h, c;

	h = 0;
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
  return new_string(string_buffer);

err_string:error(NIL, "bad backslash sequence in a string", NIL);
}

static at *
rl_mchar(register char *s)
{
  register at *q, *answer;
  at *(*sav_ptr) (at *);

  ifn (macrochp(s)) {
    if (s[0] == '^')
      error("read", "undefined caret (^) char", new_string(s));
    else if (s[0] == '#')
      error("read", "undefined dieze (#) char", new_string(s));
    else
      error("io", "internal mchar failure", NIL);
  }
  q = named(s);
  answer = var_get(q);
  sav_ptr = eval_ptr;
  eval_ptr = eval_std;
  answer = apply(answer, NIL);
  eval_ptr = sav_ptr;
  UNLOCK(q);

  return answer;
}

static at *
rl_read(register char *s)
{
  at *answer, **where, *t;
  char c;

read_again:

  c = s[1] ? 0 : s[0];

  /* DOT */
  if (c == '.')
    goto err_read1;

  /* LIST */
  if (c == '(') {
    if (prompt_string)
      prompt_string = "  ";
    while (c = next_char(), isascii((unsigned char)c) && isspace((unsigned char)c))
      read_char();
    if (next_char() == (char) EOF)
      goto err_read0;
    
    where = &answer;
    answer = NIL;
    
  read_again2:
    while (c = next_char(), isascii((unsigned char)c) && isspace((unsigned char)c))
      read_char();
    if (next_char() == (char) EOF)
      goto err_read0;
    
    s = read_word();
    c = s[1] ? 0 : s[0];
    
    switch (c) {
    case ')':
      return answer;

    case ';':
      read_string("~\n\r\377");
      goto read_again2;
      
    case '.':
      ifn(where)
	goto err_read1;
      s = read_word();
    read_again3:
      c = s[1] ? 0 : s[0];
      switch (c) {
      case '(':
      case ')':
	goto err_read1;
      case ';':
	read_string("~\n\r\377");
	s = read_word();
	goto read_again3;
      default:
	t = rl_read(s);
	if (s[0] == '#' && t) {	/* DIEZE MACROCH */
	  ifn(CONSP(t) && !t->Cdr)
	    goto err_read2;
	  *where = t->Car;
	  LOCK(t->Car);
	  UNLOCK(t);
	} else {
	  if (CONSP(t))
	    goto err_read1;
	  *where = t;
	}
	where = NIL;
	goto read_again2;
      }
      
    default:
      ifn(where)
	goto err_read1;
      
      if (s[0] == '#') {	/* DIEZE MACROCH */
	t = rl_mchar(s);
	if (t && !CONSP(t))
	  goto err_read2;
	*where = t;
      } else
	*where = cons(rl_read(s), NIL);
      
      while (CONSP(*where))
	where = &((*where)->Cdr);
      
      goto read_again2;
    }
  }

  /* BINARY TOKENS */
  if (get_char_map(s[0]) & CHAR_BINARY)
    return bread(context->input_file, NIL);

  /* MACRO CHARS */
  if (get_char_map(s[0]) & CHAR_MCHAR)
    return rl_mchar(s);

  if (s[0] == '^')
    return rl_mchar(s);

  if (s[0] == '#') {
    answer = rl_mchar(s);
    ifn(answer) {
      s = read_word();
      goto read_again;
    } else
      if (CONSP(answer) && !answer->Cdr) {
	register at *p;
	p = answer->Car;
	LOCK(p);
	UNLOCK(answer);
	return p;
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
    return new_symbol(s + 1);
  
  /* SYMBOL */
  if (s[0])
    return new_symbol(s);
  
  /* EOF */
  return NIL;

err_read0:
  error("read", "end of file", NIL);
err_read1:
  error("read", "bad dotted construction", NIL);
err_read2:
  error("read", "bad dieze (#) macro-char", NIL);
}



/* --------- MACRO-CHARS DEFINITION --------- */

char *
dmc(char *s, at *l)
{
  at *q;
  int type;
  register char c;

  if (s[0] == '^' && (c = s[1]) && !s[2]) {
    type = CHAR_CARET;
  } else if (s[0] == '#' && (c = s[1]) && !s[2]) {
    type = CHAR_DIEZE;
  } else {
    type = CHAR_MCHAR;
    c = s[0];
    if (s[1] || (get_char_map(s[0]) & CHAR_SPECIAL))
      error("dmc", "illegal macro-character", new_string(s));
  }
  if ((get_char_map(c) & CHAR_SPECIAL))
    error("dmc", "illegal macro-character", new_string(s));
  ifn((q = named(s)) && (q->flags & X_SYMBOL))
    error("dmc", "can't define this symbol", NIL);
  if (l) {
    l = new_de(NIL, l);
    var_set(q, l);
    UNLOCK(l);
    char_map[(unsigned char) c] |= type;
  } else {
    var_set(q, NIL);
    char_map[(unsigned char) c] &= ~type;
  }
  return s;
}

DY(ydmc)
{
  at *q, *l;
  register char *s;

  ifn(CONSP(ARG_LIST))
    error("dmc", "bad arguments", NIL);
  q = ARG_LIST->Car;
  l = ARG_LIST->Cdr;

  ifn(q && (q->flags & X_SYMBOL))
    error(NIL, "not a symbol", q);
  s = nameof(q);
  dmc(s, l);
  LOCK(q);
  return q;
}


/* --------- OUTPUT FUNCTIONS --------- */


/*
 * print_string s prints the string s via the print_char routine.
 */
void 
print_string(register char *s)
{
  if (s)
    while (*s)
      print_char(*s++);
}


/*
 * print_tab n prints blanks until context->output_tab >= n
 */
void 
print_tab(int n)
{
  while (context->output_tab < n) {
    print_char(' ');
    if (context->output_tab == 0)	/* WIDTH set */
      break;
  }
}

DX(xtab)
{
  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    print_tab(AINTEGER(1));
  }
  return NEW_NUMBER(context->output_tab);
}

/*
 * print_list l prints the list l:
 *  note infinite recursion avoidance
 */

void 
print_list(at *list)
{
  if (list == NIL)
  {
    print_string("()");
  }
  else if (CONSP(list)) 
  {
    at *l = list;
    at *slow = list;
    char toggle = 0;
    struct recur_elt elt;
    print_char('(');
    for(;;)
      {
        if (recur_push_ok(&elt,&print_list,l->Car))
        {
          print_list(l->Car);
          recur_pop(&elt);
        }
        else
        {
          print_string("(...)");
        }
        l = l->Cdr;
        if (l == slow)
        {
          print_string(" ...");
          l = NIL;
          break;
        }
        if(!CONSP(l))
        {
	  break;
        }
        toggle ^= 1;
        if (toggle)
          slow = slow->Cdr;
        print_char(' ');
      }
    if (l && !(l->flags&X_ZOMBIE)) {
      print_string(" . ");
      print_list(l);
    }
    print_char(')');
  }
  else
  {
    struct recur_elt elt;
    at *l = classof(list);
    if (l)
    {
      class *cl = l->Object;
      UNLOCK(l);
      l = checksend(cl,at_print);
      if (l)
      {
        if (recur_push_ok(&elt,&print_list,list))
        {
          list = letslot(list,l,NIL,-1);
          recur_pop(&elt);
          UNLOCK(l);
          UNLOCK(list);
          return;
        }
        UNLOCK(l);
      } 
    }
    print_string(pname(list));
  }
}

DX(xprint)
{
  register int i;
  register at *q;

  q = NIL;
  for (i = 1; i <= arg_number; i++) {
    ARG_EVAL(i);
    q = APOINTER(i);
    print_list(q);
    if (i<arg_number)
      print_char(' ');
  }
  print_char('\n');
  LOCK(q);
  return q;
}

DX(xprin)
{
  register int i;
  register at *q;

  q = NIL;
  for (i = 1; i <= arg_number; i++) {
    ARG_EVAL(i);
    q = APOINTER(i);
    print_list(q);
    if (i<arg_number)
      print_char(' ');
  }
  LOCK(q);
  return q;
}


/*
 * printf C'printf interface. format: %{-}{n}{.{m}}{d|s|f|e|g|l|p}  ( l for
 * list, p for pretty ) or   : %%
 */


DX(xprintf)
{
  register char *fmt, *buf, c;
  register int i, ok;

  if (arg_number < 1)
    error(NIL, "format string expected", NIL);

  ALL_ARGS_EVAL;
  fmt = ASTRING(1);

  i = 1;
  forever
  {
    if (*fmt == 0)
      break;
    buf = print_buffer;
    while (*fmt != 0 && *fmt != '%')
      *buf++ = *fmt++;
    *buf = 0;
    print_string(print_buffer);

    if (*fmt == 0)
      break;
    buf = print_buffer;
    ok = 0;

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
	    ok = 4;
	  else if (ok <= 8)
	    ok = 8;
	  else
	    goto err_printf0;
      }
    }

    *buf = 0;
    if (c != '%' && ++i > arg_number)
      goto err_printf1;

    if (c == 'l')
      print_list(APOINTER(i));
    else if (c == 'p') {
      at *ans, *args;
      args = new_cons(APOINTER(i),NIL);
      ans = apply(at_pprint,args);
      UNLOCK(ans);
      UNLOCK(args);
    } else if (c == 'd') {
      *buf++ = 0;
      if (ok == 9) {
	print_string(str_number((real) AINTEGER(i)));
      } else {
	sprintf(buf, print_buffer, AINTEGER(i));
	print_string(buf);
      }
    } else if (c == 's') {
      *buf++ = 0;
      if (ok == 9) {
	print_string(ASTRING(i));
      } else {
	sprintf(buf, print_buffer, ASTRING(i));
	print_string(buf);
      }
    } else if (c == 'e' || c == 'f' || c == 'g') {
      *buf++ = 0;
      if (ok == 9) {
	print_string(str_number(AREAL(i)));
      } else {
	sprintf(buf, print_buffer, AREAL(i));
	print_string(buf);
      }
    };

    if (c == '%')
      print_char('%');
  };

  if (i < arg_number)
    goto err_printf1;

  return NIL;

  err_printf0:
   error(NIL, "bad format string", NIL);
  err_printf1:
   error(NIL, "bad argument number", NIL);
}


/* --------- LIST TO STRING CONVERSION ROUTINES --------- */

static char *
convert(register char *s, register at *list, register char *end)
{
  register char *n, *m;
  at *p,*q=NIL;
  int mode;

  ifn(list)
    {			/* PNAME :   NIL    =>   '()'	 */
      *s++ = '(';
      *s++ = ')';
      *s = 0;
      return s;
    }
  else if (CONSP(list))
    {		        /* PNAME :   LIST   =>  '(...)'	 */
      *s++ = '(';
      if (s > end - 8 )
	goto exit_convert;
      forever
	{
	  ifn(s = convert(s, list->Car, end))
	    return 0L;
	  list = list->Cdr;
	  ifn(CONSP(list))
	    break;
	  *s++ = ' ';
	}
      if (list && !(list->flags&X_ZOMBIE)) {
	*s++ = ' ';
	*s++ = '.';
	*s++ = ' ';
	ifn(s = convert(s, list, end))
	  return 0L;
      }
      *s++ = ')';
      if (s > end - 8 )
	goto exit_convert;
      *s = 0;
      return s;
    }
  else
    {	
      n = NIL;	
    
      if (((list->flags & C_NUMBER) 
	   && (p=checksend(&number_class,at_pname)))
	  || ((list->flags & C_EXTERN)
	      && (p=checksend(list->Class,at_pname))) ) 
	{
	  q = letslot(list,p,NIL,-1);
	  ifn (q && (q->flags&X_STRING))
	    error(NIL,"pname does not return a string",q);
	  n = SADD(q->Object);
	  UNLOCK(p);
	}
      else if (list->flags & C_EXTERN) 
	{
	  n = (*(list->Class->self_name)) (list);
	}
      else if (list->flags & C_NUMBER) 
	{
	  n = str_number(list->Number);
	}
      if (n == NIL)
	error("io.c/convert", "internal error", NIL);
      
      mode = 0;
      if (list->flags & X_SYMBOL) {
	for (m = n; *m; m++)
	  if (   (isascii((unsigned char)*m) && isspace((unsigned char)*m)) 
	      || isupper(toascii((unsigned char)*m))
	      || (get_char_map(*m) & CHAR_INTERWORD) ) {
	    mode = 1;
	    break;
	  }
	ifn(*n)
	  mode = 1;
      }
      if (mode)
	*s++ = '|';
      while (*n)
	if (s < end - 8)
	  *s++ = *n++;
	else
	  goto exit_convert;
      if (mode)
	*s++ = '|';
      *s = 0;
      UNLOCK(q);
      return s;
    }
  
 exit_convert:
  *s = 0;
  strcpy(s, " ... ");
  return 0L;
}


/*
 * first_line l returns the first line of list l (80 characters max)
 */
char *
first_line(at *l)
{
  convert(pname_buffer, l, pname_buffer+60 );
  return pname_buffer;
}


/*
 * pname l returns the string image of the list l
 */
char *
pname(at *l)
{
  convert(pname_buffer, l, pname_buffer+LINE_BUFFER );
  return pname_buffer;
}

DX(xpname)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(pname(APOINTER(1)));
}



/* --------- INITIALISATION CODE --------- */

void 
init_io(void)
{
  register int i;

  ifn((line_buffer = tl_malloc(LINE_BUFFER)) &&
      (print_buffer = tl_malloc(LINE_BUFFER)) &&
      (pname_buffer = tl_malloc(LINE_BUFFER)))
    abort("not enough memory");
  line_pos = line_buffer;

  set_char_map(')', CHAR_SPECIAL);
  set_char_map('(', CHAR_SPECIAL);
  set_char_map(';', CHAR_SPECIAL);
  set_char_map('\"' /*"*/, CHAR_SPECIAL);
  set_char_map('|', CHAR_SPECIAL);
  set_char_map('^', CHAR_SPECIAL | CHAR_PREFIX);
  set_char_map('#', CHAR_SPECIAL | CHAR_PREFIX);

  for (i = 0; i < ' '; i++)
    ifn(isspace(i))
      set_char_map(i, CHAR_SHORT_CARET);

  set_char_map(0x9f, CHAR_BINARY);

  dx_define("macrochp", xmacrochp);
  dx_define("flush",xflush);
  dx_define("ask", xask);
  dx_define("skip_char", xskip_char);
  dx_define("read_string", xread_string);
  dx_define("read", xread);
  dy_define("dmc", ydmc);
  dx_define("tab", xtab);
  dx_define("print", xprint);
  dx_define("prin", xprin);
  dx_define("printf", xprintf);
  dx_define("pname", xpname);

  at_pname  = var_define("pname");
  at_print  = var_define("print");
  at_pprint  = var_define("pprint");
}
