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
  string.c
  class STRING -->   	STRING_DISPOSE  STRING_NAME
  string creation -->	NEW_STRING  NEW_SAFE_STRING
  string manipulation --> LEFT$ RIGHT$ MID$ CONCAT$ VAL$ STR$
  string test -->		STRINGP
  $Id: string.c,v 1.2 2008-05-30 14:26:27 leonb Exp $
********************************************************************** */

#include "header.h"

struct alloc_root alloc_string = {
  NULL,
  NULL,
  sizeof(struct string),
  200
};


char null_string[] = "";
char digit_string[] = "0123456789abcdef";
char special_string[] = "\"\\\n\r\b\t\f\377";
char aspect_string[] = "\"\\nrbtfe";
char *string_buffer;

static char *toolong = "String too long";
static char *badarg = "Argument out of range";

/*
 * new_safe_string(s) s is a normal C string. Don't copy it in an allocated
 * buffer, but create a new EXTERN at and return it. Use NEW_SAFE_STRING
 * whenever you're sure that your string will not be destroyed soon. Else use
 * NEW_STRING.
 */

at *
new_safe_string(char *s)
{
  register struct string *st;
  register at *q;

  if (!s || !*s)
    s = null_string;

  st = allocate(&alloc_string);

  st->flag = STRING_SAFE;
  st->start = s;
  q = new_extern(&string_class, st);
  q->flags |= X_STRING;

  return q;
}



/*
 * new_string_bylen(n)
 * returns a string for length n
 */

at *
new_string_bylen(int n)
{
  at *q;
  char *buffer;
  struct string *st;

  st = allocate(&alloc_string);
  ifn (buffer = tl_malloc(n+1))
    error(NIL, "memory exhausted", NIL);
  buffer[0] = 0;
  buffer[n] = 0;
  st->flag = STRING_ALLOCATED;
  st->start = buffer;
  q = new_extern(&string_class, st);
  q->flags |= X_STRING;
  return q;
}


/*
 * new_string(s) s is a normal C string. Copy it in an allocated buffer, then
 * create a new AT and return it.
 */
at *
new_string(char *s)
{
  register struct string *st;
  register at *q;

  if (!s || !*s)
    return new_safe_string(null_string);
  q = new_string_bylen(strlen(s));
  st = q->Object;
  strcpy(st->start, s);
  return q;
}




/*
 * string_dispose(p) deallocate the string structure, and the buffer, if
 * necessary.
 */
static void 
string_dispose(at *p)
{
  register struct string *s;

  s = p->Object;
  if (s->flag == STRING_ALLOCATED)
    free(s->start);
  deallocate(&alloc_string, (struct empty_alloc *) s);
}


/*
 * string_name(p) returns the pname of extern p
 */
static char *
string_name(at *p)
{
  char *s, *ind, *name;
  unsigned char c;
  
  s = ((struct string *) (p->Object))->start;
  name = string_buffer;

  *name++ = '\"';  /*"*/
  while ((c = *s)) {
    if (name<string_buffer+STRING_BUFFER-10) {
      if ((ind = strchr(special_string, c))) {
	*name++ = '\\';
	*name++ = aspect_string[ind - special_string];
      } else if (isprint(toascii((unsigned char)c))) {
	*name++ = c;
      } else if (c<=' ') {
	*name++ = '\\';
	*name++ = '^';
	*name++ = c | 0x40;
      } else {
	*name++ = '\\';
	*name++ = 'x';
	*name++ = digit_string[(c >> 4) & 15];
	*name++ = digit_string[c & 15];
      }
    }
    s++;
  }
  *name++ = '\"';  /*"*/
  *name++ = 0;
  return string_buffer;
}


/* string_compare
 * -- compare two strings
 */

static int 
string_compare(at *p, at *q, int order)
{
  return strcmp(SADD(p->Object),SADD(q->Object));
}


/* string hash
 * -- return hashcode for string
 */

static unsigned long
string_hash(at *p)
{
  unsigned long x = 0x12345678;
  char *s = SADD(p->Object);
  while (*s)
  {
    x = (x<<6) | ((x&0xfc000000)>>26);
    x ^= (*s);
    s++;
  }
  return x;
}



class string_class =
{
  string_dispose,
  generic_action,
  string_name,
  generic_eval,
  generic_listeval,
  generic_serialize,
  string_compare,
  string_hash
};


/* operations on strings ------------------------------	 */


DX(xstr_left)
{
  char *s,*a;
  int n,l;
  at *p;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  s = ASTRING(1);
  n = AINTEGER(2);
  
  if (n < 0)
    error(NIL, badarg, NEW_NUMBER(n));
  l = strlen(s);
  if (n > l)
    n = l;
  p = new_string_bylen(n+1);
  a = SADD(p->Object);
  strncpy(a,s,n);
  a[n] = 0;
  return p;
}


/*------------------------ */


DX(xstr_right)
{
  char *s;
  int n,l;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  s = ASTRING(1);
  n = AINTEGER(2);
  
  if (n < 0)
    error(NIL, badarg, NEW_NUMBER(n));
  l = strlen(s);
  if (n > l)
    n = l;
  return new_string(s+l-n);
}


/*------------------------ */


DX(xstr_mid)
{
  char *s, *a;
  at *p;
  int n,m,l;

  ALL_ARGS_EVAL;
  if (arg_number == 2)
    {
      s = ASTRING(1);
      n = AINTEGER(2);
      l = strlen(s);
      if (n < 1)
	error(NIL, badarg, NEW_NUMBER(n));	
      if (n > l)
	return new_safe_string(null_string);
      else
	return new_string(s+n-1);
    }
  else 
    {
      ARG_NUMBER(3);
      s = ASTRING(1);
      n = AINTEGER(2);
      m = AINTEGER(3);
      if (n < 1)
	error(NIL, badarg, NEW_NUMBER(n));	
      if (m < 0)
	error(NIL, badarg, NEW_NUMBER(m));
      l = strlen(s)-(n-1);
      if (m > l)
	m = l;
      if (m < 1)
	return new_safe_string(null_string);
      p = new_string_bylen(m+1);
      a = SADD(p->Object);
      strncpy(a,s+(n-1),m);
      a[m] = 0;
      return p;
    }
}


/*------------------------ */


DX(xstr_concat)
{
  int i;
  int length = 0;
  char *here, *s;
  at *p;
  
  ALL_ARGS_EVAL;
  for (i=1; i<=arg_number; i++) {
    s = ASTRING(i);
    length += strlen(s);
  }
  p = new_string_bylen(length+1);
  here = SADD(p->Object);
  for (i=1; i<=arg_number; i++) {
    s = ASTRING(i);
    while (*s) {
      length--;
      *here++ = *s++;
    }
  }
  *here = 0;
  if (length !=0)
    error(NIL,"Internal error: bad length",NIL);
  return p;
}


/*------------------------ */


static int 
str_index(char *s1, register char *s2, register int start)
{
  register int indx;
  register char *sa, *sb;

  indx = 1;
  while (*s2) {
    if (--start <= 0) {
      sa = s2;
      sb = s1;
      while (*sb && *sb == *sa++)
	sb++;
      if (*sb == 0)
	return indx;
    }
    indx++;
    s2++;
  }
  return 0;
}

DX(xstr_index)
{
  register int start;
  register char *s;

  start = 1;
  ALL_ARGS_EVAL;
  if (arg_number == 3)
    start = AINTEGER(3);
  else
    ARG_NUMBER(2);
  s = ASTRING(1);
  if ((start = str_index(s, ASTRING(2), start)))
    return NEW_NUMBER(start);
  else
    return NIL;
}


/*------------------------ */


static at *
str_val_hex(char *s)
{
  int x = 0;
  int flag =0;
  while (isspace((unsigned char)*s))
    s++;
  if (*s=='-')
    flag = 1;
  if (*s=='+' || *s=='-')
    s++;
  if (!(s[0]=='0' && (s[1]=='x'||s[1]=='X') && s[2]))
    return NIL;
  s += 2;
  while (*s)
  {
    if (x&0xf0000000)
      return NIL;
    x <<= 4;
    if (*s>='0' && *s<='9')
      x += (*s - '0');
    else if (*s>='a' && *s<='f')
      x += (*s - 'a' + 10);
    else if (*s>='A' && *s<='F')
      x += (*s - 'A' + 10);
    else
      return NIL;
    s++;
  }
  return NEW_NUMBER(flag ? -x : x);
}


static at *
str_val_float(char *s)
{
  int flag;
  char *h;
  while (isspace((unsigned char)*s))
    s++;
  h = s;
  if (isdigit((unsigned char)*s) ||
      (*s == '+' || *s == '-' || *s == '.')) {
    flag = 0;
    while (isdigit((unsigned char)*s) || *s == '+' || *s == '-'
	   || *s == '.' || *s == 'e' || *s == 'E') {
      if (isdigit((unsigned char)*s))
	flag |= 0x1;
      if (*s == '+' || *s == '-') {
	if (flag & 0x3)
	  goto fin;
	else
	  flag |= 0x2;
      }
      if (*s == '.') {
	if (flag & 0xc)
	  goto fin;
	else
	  flag |= 0x4;
      }
      if (*s == 'e' || *s == 'E') {
	if (flag & 0x8)
	  goto fin;
	else {
	  flag &= 0xc;
	  flag |= 0x8;
	};
      }
      s++;
    }
    while (isspace((unsigned char)*s))
      s++;
    ifn(*s == 0 && (flag & 0x1))
      goto fin;
    return NEW_NUMBER(atof(h));
  }
fin:
  return NIL;
}



at *
str_val(char *s)
{
  at *p;
  p = str_val_hex(s);
  if (!p)
    p = str_val_float(s);
  return p;
}
  


DX(xstr_val)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return str_val(ASTRING(1));
}


/*------------------------ */

char *
str_number(double x)
{
  real y;
  register char *s, *t;
  
  if (isnanD((real)x))
    return "Nan";
  if (isinfD((real)x))
    return (x>0 ? "Inf" : "-Inf");
  
  y = fabs(x);
  if (y<1e-3 || y>1e10)
    sprintf(string_buffer, "%g", (double) (x));
  else
    sprintf(string_buffer, "%.4f", (double) (x));
  
  for (s = string_buffer; *s != 0; s++)
    if (*s == '.')
      break;
  if (*s == '.') {
    for (t = s + 1; isdigit((unsigned char)*t); t++)
      if (*t != '0')
	s = t + 1;
    until(*t == 0)
      * s++ = *t++;
    *s = 0;
  }
  return string_buffer;
}

DX(xstr_number)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(str_number(AREAL(1)));
}

/*------------------------ */

char *
str_number_hex(double x)
{
  int ix = (int)floor(x);
  
  if (isnanD((real)x))
    return "Nan";
  if (isinfD((real)x))
    return (x>0 ? "Inf" : "-Inf");
  if (ix == 0)
    return "0";

  sprintf(string_buffer, "0x%x", ix);
  return string_buffer;
}

DX(xstr_number_hex)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(str_number_hex(AREAL(1)));
}

/*------------------------ */


DX(xstr_len)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(strlen(ASTRING(1)));
}

/*------------------------ */


static char *
str_del(char *s, int n, int l)
{
  register char *t;

  if (n < 1)
    error(NIL, badarg, NEW_NUMBER(n));
  if (l < 0)
    error(NIL, badarg, NEW_NUMBER(l));

  t = string_buffer;
  while (*s && --n>0)  {
    if (t>string_buffer+STRING_BUFFER-10)
      error(NIL,toolong,NIL);
    *t++ = *s++;
  }
  while (*s && l-->0)
    s++;
  while (*s) {
    if (t>string_buffer+STRING_BUFFER-10)
      error(NIL,toolong,NIL);
    else
      *t++ = *s++;
  }
  *t=0;
  return string_buffer;
}
DX(xstr_del)
{
  register int l;

  ALL_ARGS_EVAL;
  if (arg_number == 2)
    l = 32767;
  else {
    ARG_NUMBER(3);
    l = AINTEGER(3);
  }
  return new_string(str_del(ASTRING(1), AINTEGER(2), l));
}

/*------------------------ */

static char *
str_ins(char *s, int pos, char *what)
{
  char *t = string_buffer;

  if (pos<0)
    error(NIL,badarg,NEW_NUMBER(pos));

  while (*s && pos-->0) {
    if (t>string_buffer+STRING_BUFFER-10)
      error(NIL,toolong,NIL);
    *t++ = *s++;
  }
  while (*what) {
    if (t>string_buffer+STRING_BUFFER-10)
      error(NIL,toolong,NIL);
    *t++ = *what++;
  }
  while (*s) {
    if (t>string_buffer+STRING_BUFFER-10)
      error(NIL,toolong,NIL);
    *t++ = *s++;
  }
  *t=0;
  return string_buffer;
}
DX(xstr_ins)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  return new_string( str_ins(ASTRING(1),AINTEGER(2),ASTRING(3)));
}



/*------------------------ */

static char *
str_subst(char *s, char *s1, char *s2)
{
  char *t = string_buffer;
  int len1 = strlen(s1);
  int len2 = strlen(s2);
  
  while(*s) {
    if ((*s==*s1) && (!strncmp(s,s1,len1)) ) {
      if (t>string_buffer+STRING_BUFFER-10-len2)
	error(NIL,toolong,NIL);
      strcpy(t,s2);
      t += len2;
      s += len1;
    } else {
      if (t>string_buffer+STRING_BUFFER-10)
	error(NIL,toolong,NIL);
      *t++ = *s++;
    }
  }
  *t = 0;
  return string_buffer;
}
DX(xstrsubst)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  return new_string( str_subst(ASTRING(1),ASTRING(2),ASTRING(3)) );
}

/*------------------------ */


DX(xupcase)
{
  char *s,*r,c;
  at *rr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  rr = new_string_bylen(strlen(s));
  r = SADD(rr->Object);
  while ((c = *s++)) {
    *r++ = toupper((unsigned char)c);
  }
  *r = 0;
  return rr;
}

DX(xupcase1)
{
  char *r;
  at *rr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  rr = new_safe_string(strdup(ASTRING(1)));
  r = SADD(rr->Object);
  if (*r) {
    *r = toupper((unsigned char)*r);
  }
  return rr;
}

DX(xdowncase)
{
  char *s,*r,c;
  at *rr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  rr = new_string_bylen(strlen(s));
  r = SADD(rr->Object);
  while ((c = *s++)) {
    *r++ = tolower((unsigned char)c);
  }
  *r = 0;
  return rr;
}


/* ----------------------- */

DX(xstr_asc)
{
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  if (s[0])
    return NEW_NUMBER(s[0]);
  else
    error(NIL,"Empty string",APOINTER(1));
}

DX(xstr_chr)
{
  int i;
  char s[2];
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  if (i<0 || i>255)
    error(NIL,"Ascii code out of range",APOINTER(1));
  s[0]=i;
  s[1]=0;
  return new_string(s);
}

DX(xisprint)
{
  unsigned char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = (unsigned char*)ASTRING(1);
  if (!s || !*s)
    return NIL;
  while (*s) {
    if ( ((*s)&0x7f)<0x20 || ((*s)&0x7f)==0x7f )
      return NIL;
    s++;
  }
  return true();
}

/*------------------------ */



DX(xstringp)
{
  register at *p;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (p && (p->flags & X_STRING)) {
    LOCK(p);
    return p;
  } else
    return NIL;
}


/***********************************************************************
  REGEX.C (C) LYB 1991
  A full regular expression package.
************************************************************************/

/*
 * This is the regular expression grammar...
 * 
 * regex.single		:   LIT
 * 			|   RANGE
 * 			|   '.'
 * 	  		|   '(' regex.alternate ')'
 * 			;
 * 
 * regex.several	:   regex.single
 * 			|   regex.single '+'
 * 			|   regex.single '*'
 * 			|   regex.single '?'
 * 			;
 * 
 * regex.catenate	:   regex.several
 * 			|   regex.catenate regex.several
 * 			;
 * 
 * regex.alternate	:   regex.catenate
 * 			|   regex.catenate '|' regex.alternate
 * 			;
 * 
 * 
 * regex		:   regex.alternate
 * 			;
 */

/* ----------- declarations ------------ */


static sigjmp_buf rejmpbuf;
static char  *pat, *dat, *datstart;
static unsigned short *buf;

#define RE_LIT      0x0000
#define RE_RNG      0x1000
#define RE_ANY      0x2000
#define RE_JMP      0x3800
#define RE_FAIL     0x4800
#define RE_START    0x5000
#define RE_END      0x6000
#define RE_CARET    0x7000
#define RE_DOLLAR   0x8000

#define charset_zero(cset)     { int i; for(i=0;i<16;i++) (cset)[i]=0; }
#define charset_toggle(cset)   { int i; for(i=0;i<16;i++) (cset)[i]^=0xffff; }
#define charset_set(cset,i)    { (cset)[i/16] |= (1<<(i%16)); }
#define charset_tst(cset,i)    ( (cset)[i/16] & (1<<(i%16)) )

typedef struct bounds {
  unsigned short *beg, *end; } bounds;

static char *err[] = {
  "Regex is too complex",
  "Unbalanced () in regex",
  "Unbalanced [] in regex",
  "Bad badslash escape in regex",
  "Regex syntax error",
};

#define serror(n)           { dat=err[n];siglongjmp(rejmpbuf,-1); }
#define concatc(b1,c,in)    { if (b1.end>in.end-4) serror(0);\
                              *b1.end++ = c; }
#define concatb(b1,b2,in)   { unsigned short *s=b2.beg;\
                              if (b1.end+(b2.end-b2.beg)>in.end-4) serror(0);\
			      if (s==b1.end) b1.end=b2.end; else \
                              while(s<b2.end) *b1.end++ = *s++; }


/* ----------- private routines ------------ */


static bounds regex_alternate(bounds buf, int *rnum);

static bounds
regex_single(bounds buf, int *rnum)
{
  bounds ans,tmp;
  unsigned short *set;
  int toggle=0,last=0;
  unsigned char c;

  ans.beg = ans.end = buf.beg;

  switch (c = *pat++) {

  case 0:
    pat--;
  case '|':
    serror(4);
  case ']':
    serror(2);
  case ')':
    serror(1);

  case '\\':
    if (!*pat) serror(3);
    concatc(ans,*pat,buf);
    pat++;
    return ans;
    
  case '.':
    concatc(ans,RE_ANY,buf);
    return ans;

  case '(':
    last = *rnum;
    *rnum = *rnum + 1;
    concatc(ans,RE_START+last,buf);
    buf.beg += 1;
    tmp = regex_alternate(buf,rnum);
    concatb(ans,tmp,buf);
    if (*pat!=')') serror(1);
    concatc(ans,RE_END+last,buf);
    pat++;
    return ans;
    
  case '[':
    if (ans.end>buf.beg+18) serror(0);
    set = ans.end+1;
    charset_zero(set);
    toggle = last = 0;
    if (*pat=='^')  { toggle=1; pat++; }
    if (*pat==']')  { charset_set(set,']'); pat++; }
    while(*pat!=']') {
      if (!*pat) serror(2);
      if (*pat=='-' && last && pat[1] && pat[1]!=']') {
	pat++;
	while(last<=*pat) {
	  charset_set(set,last);
	  last++;
	}
      } else {
	charset_set(set,*pat);
	last = *pat;
      }
      pat++;
    }
    pat++;
    if (toggle)
      charset_toggle(set);
    last = 15;
    while (set[last]==0 && last>0) 
      last--;
    concatc(ans,RE_RNG+last+1,buf);
    ans.end += last+1;
    return ans;

  case '^':
    concatc(ans,RE_CARET,buf);
    return ans;
    
  case '$':
    concatc(ans,RE_DOLLAR,buf);
    return ans;
    
  default:
    concatc(ans,c,buf);
    return ans;
    
  }
}


static bounds
regex_several(bounds buf, int *rnum)
{
  bounds b;
  bounds ans,rem;

  ans.beg = buf.beg;
  ans.end = buf.beg;
  rem.beg = ans.beg+2;
  rem.end = buf.end;

  b = regex_single(rem,rnum);

  switch (*pat) {

  case '?':
    concatc(ans, RE_FAIL+(b.end-b.beg), buf);
    concatb(ans, b, buf);
    pat++;
    return ans;

  case '+':
    concatb(ans,b,buf);
    concatc(ans, RE_FAIL+1, buf);
    concatc(ans, RE_JMP+(b.beg-b.end)-2, buf);
    pat++;
    return ans;

  case '*':
    concatc(ans, RE_FAIL+(b.end-b.beg)+1, buf);
    concatb(ans,b,buf);
    concatc(ans, RE_JMP+(b.beg-b.end)-2, buf);
    pat++;
    return ans;

  default:
    concatb(ans,b,buf);
    return ans;
  }
}

static bounds
regex_catenate(bounds buf, int *rnum)
{
  bounds b;
  bounds ans,rem;

  ans.beg = buf.beg;
  ans.end = buf.beg;
  rem.beg = buf.beg;
  rem.end = buf.end;

  do {
    rem.beg = ans.end;
    b = regex_several(rem,rnum);
    concatb(ans,b,buf);
  } while (*pat && *pat!='|' && *pat!=')');
  return ans;
}

static bounds
regex_alternate(bounds buf, int *rnum)
{
  bounds b1,b2;
  bounds ans,rem;
  int newrnum;

  ans.beg = buf.beg;
  ans.end = buf.beg;
  rem.beg = buf.beg+1;
  rem.end = buf.end;

  newrnum = *rnum;
  b1 = regex_catenate(rem,rnum);
  if (*pat == '|') {
    pat++;
    rem.beg = b1.end+1;
    b2 = regex_alternate(rem,&newrnum);
    if (newrnum>*rnum)
      *rnum = newrnum;
    concatc(ans, RE_FAIL+(b1.end-b1.beg+1), buf);
    concatb(ans, b1, buf);
    concatc(ans, RE_JMP +(b2.end-b2.beg), buf);
    concatb(ans, b2, buf);
    return ans;
  } else {
    concatb(ans, b1, buf);
    return ans;
  }
}



static int
regex_execute(char **regsptr, int *regslen, int nregs)
{
  unsigned short c;
  unsigned short *buffail;
  char  *datfail;
  while ((c = *buf++)) {
#ifdef DEBUG_REGEX
    printf("%04x (%02x)\n",(int)(c)&0xffff,(dat?*dat:0)); 
#endif
    switch (c&0xf000) {

    case RE_CARET:
      if (dat != datstart)
	return 0;
      break;

    case RE_DOLLAR:
      if (*dat)
	return 0;
      break;

    case RE_LIT:
      if (!*dat)
	return 0;
      if (*dat++ != (char)(c&0x00ff))
	return 0;
      break;

    case RE_RNG:
      if (!*dat)
	return 0;
      c -= RE_RNG;
      if (*dat>=c*16)
	return 0;
      if (!charset_tst( buf, *dat))
	return 0;
      buf += c;
      dat++;
      break;

    case RE_ANY:
      if (!*dat)
	return 0;
      dat++;
      break;

    case RE_JMP&0xf000:
      if ((*dat==0) && (c<RE_JMP)) /* never jump backwards if end of data */
	break;
      buf = buf + c - RE_JMP;
      break;
      
    case RE_FAIL&0xf000:
      buffail = buf + c - RE_FAIL;
      datfail = dat;
      if (!regex_execute(regsptr,regslen,nregs)) {
#ifdef DEBUG_REGEX
	printf("fail\n");
#endif
	buf = buffail;
	dat = datfail;
      }
      break;

    case RE_START:
      c &= 0x00ff;
      if (c<nregs)
	regsptr[c] = dat;
      break;

    case RE_END:
      c &= 0x00ff;
      if (c<nregs) {
	if (dat)
	  regslen[c] = dat-regsptr[c];
	else if (regsptr[c])
	  regslen[c] = strlen(regsptr[c]);
	else
	  regslen[c] = 0;
      }
      break;
    }
  }
  buf--;
  return 1;
}



/* ----------- public routines ------------ */


char *
regex_compile(char *pattern, 
	      short int *bufstart, short int *bufend, 
	      int strict, int *rnum)
{
  int regnum = 0;
  if (!rnum) 
    rnum = &regnum;
  *rnum = 0;
  dat = 0L;
  pat = pattern;
  if (sigsetjmp(rejmpbuf, 1))
    {
      return dat;
    }
  else 
    {
      bounds buf;
      buf.beg = (unsigned short*)bufstart;
      buf.end = (unsigned short*)bufend;
      if (strict) 
	{
	  *buf.beg++ = RE_CARET;
	  buf.end--;
	}
      buf = regex_alternate(buf,rnum); 
      if (strict) 
	*buf.end++ = RE_DOLLAR;
      *buf.end = 0;
      return 0L;
    }
}


int 
regex_exec(short int *buffer, char *string, 
	   char **regptr, int *reglen, int nregs)
{
  int c;
  
  for(c=0;c<nregs;c++)
    reglen[c] = 0;
  dat = datstart = string;
  buf = (unsigned short*)buffer;
  return regex_execute(regptr,reglen,nregs);
}


int 
regex_seek(short int *buffer, char *string, char *seekstart, 
	   char **regptr, int *reglen, int nregs, char **start, char **end)
{
  int c;
  
  datstart = string;
  while (*seekstart) {
    for(c=0;c<nregs;c++)
      reglen[c] = 0;
    dat = seekstart;
    buf = (unsigned short*)buffer;
    if (regex_execute(regptr,reglen,nregs)) {
      *start = seekstart;
      *end = dat;
      return 1;
    }
    seekstart++;
  }
  return 0;
}


/* -------- undefines ------------ */

#undef RE_LIT
#undef RE_RNG
#undef RE_ANY
#undef RE_JMP
#undef RE_FAIL
#undef RE_START
#undef RE_END
#undef charset_zero
#undef charset_toggle
#undef charset_set
#undef charset_tst
#undef serror
#undef concatc
#undef concatb

/* ---------- lisp interface ------- */


DX(xregex_match)
{
  char *pat, *dat;
  short buffer[1024];

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  
  pat = ASTRING(1);
  dat = ASTRING(2);

  pat = regex_compile(pat,buffer,buffer+1024,1,NULL);
  if (pat)
    error(NIL,pat,APOINTER(1));

  if (regex_exec(buffer,dat,NULL,NULL,0))
    return true();  
  else
    return NIL;
}


DX(xregex_extract)
{
  char *pat, *dat;
  short buffer[1024];
  at *ans=NIL;
  at **where = &ans;
  char **regptr;
  int *reglen;
  int regnum = 0;
  int i;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  pat = ASTRING(1);
  dat = ASTRING(2);
  pat = regex_compile(pat,buffer,buffer+1024,1,&regnum);
  if (pat)
    error(NIL,pat,APOINTER(1));

  regptr = malloc((regnum+1)*sizeof(char*));
  reglen = malloc((regnum+1)*sizeof(int));
  if (!regptr || !reglen)
    error(NIL,"out of memory",NIL);

  if (regex_exec(buffer,dat,regptr,reglen,regnum))
    {
      for (i=0; i<regnum; i++) 
	{
	  at *str = new_string_bylen(reglen[i]);
	  strncpy(SADD(str->Object), regptr[i], reglen[i]);
	  *where = cons(str,NIL);
	  where = &((*where)->Cdr);
	}
      if (!ans)
	*where = new_cons(APOINTER(2),NIL);
    }
  free(regptr);
  free(reglen);
  return ans;
}




DX(xregex_seek)
{
  char *pat, *dat,*datstart, *start,*end;
  int n;
  short buffer[1024];

  ALL_ARGS_EVAL;
  if (arg_number==3)
    n = AINTEGER(3);
  else {
    n = 1;
    ARG_NUMBER(2);
  }
  pat = ASTRING(1);
  dat = datstart = ASTRING(2);
  while (--n>0 && *dat)
    dat++;

  pat = regex_compile(pat,buffer,buffer+1024,0,NULL);
  if (pat)
    error(NIL,pat,APOINTER(1));
  
  if (regex_seek(buffer,datstart,dat,NULL,NULL,0,&start,&end)) {
    dat = ASTRING(2);
    return cons(NEW_NUMBER(1+start-dat),
		cons(NEW_NUMBER(end-start),
		     NIL ) );
  }
  return NIL;
}



DX(xregex_subst)
{
  char *pat, *dat, *datstart, *str; 
  short buffer[1024];
  char *regptr[10];
  int  reglen[10];
  int reg,i;
  char *start,*end, *s1, *s2;


  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  
  pat = ASTRING(1);
  str = ASTRING(2);
  dat = datstart = ASTRING(3);

  pat = regex_compile(pat,buffer,buffer+1024,0,NULL);
  if (pat)
    error(NIL,pat,APOINTER(1));
  
  s2 = string_buffer;
  do {
    if (! regex_seek(buffer,datstart,dat,regptr,reglen,10,&start,&end))
      start = end = dat+strlen(dat);
    if (dat<start) {
      s1 = dat;
      while (s1<start)
	if (s2<string_buffer+STRING_BUFFER-2)
	  *s2++ = *s1++;
	else
	  error(NIL,toolong,NIL);
    }
    if (start<end) {
      s1 = str;
      while (*s1) {
	if (s2>string_buffer+STRING_BUFFER-2)
	  error(NIL,toolong,NIL);
	if (*s1=='%') {
	  s1++;
	  if (*s1=='%')
	    *s2++ = *s1++;
	  else {
	    reg = *s1++ - '0';
	    if (reg<0 || reg>9)
	      error(NIL,"bad register number",APOINTER(3));
	    if (s2>string_buffer+STRING_BUFFER-2-reglen[reg])
	      error(NIL,toolong,NIL);
	    for(i=0; i<reglen[reg] && regptr[reg][i]; i++)
	      *s2++ = regptr[reg][i];
	  }
	} else {
	  *s2++ = *s1++;
	}
      }
    }
    while (*dat && dat<end)
      dat++;
  } while (*dat);
  *s2=0;
  return new_string(string_buffer);
}










/***********************************************************************
  SPRINTF.C (C) LYB 1991
************************************************************************/



#define GPRINT_BUFFER 1024
static char *gprint_buffer;
extern char *print_buffer;

static char *
gbcopy(register char *gb, register char *s)
{
  while (*s && gb < gprint_buffer + GPRINT_BUFFER - 1)
    *gb++ = *s++;
  *gb = 0;
  return gb;
}

DX(xsprintf)
{
  register char *fmt, *buf, *gbuf, c;
  register int i, ok;

  if (arg_number < 1)
    error(NIL, "At least one argument expected", NIL);

  ALL_ARGS_EVAL;
  fmt = ASTRING(1);

  i = 1;
  gbuf = gprint_buffer;
  forever
  {
    if (*fmt == 0)
      break;
    buf = print_buffer;
    while (*fmt != 0 && *fmt != '%')
      *buf++ = *fmt++;
    *buf = 0;
    gbuf = gbcopy(gbuf, print_buffer);

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

    if (c == 'l' || c == 'p')
      gbuf = gbcopy(gbuf, pname(APOINTER(i)));

    else if (c == 'd') {
      *buf++ = 0;
      if (ok == 9) {
	gbuf = gbcopy(gbuf, str_number((real) AINTEGER(i)));
      } else {
	sprintf(buf, print_buffer, AINTEGER(i));
	gbuf = gbcopy(gbuf, buf);
      }
    } else if (c == 's') {
      *buf++ = 0;
      if (ok == 9) {
	gbuf = gbcopy(gbuf, ASTRING(i));
      } else {
	sprintf(buf, print_buffer, ASTRING(i));
	gbuf = gbcopy(gbuf, buf);
      }
    } else if (c == 'e' || c == 'f' || c == 'g') {
      *buf++ = 0;
      if (ok == 9) {
	gbuf = gbcopy(gbuf, str_number(AREAL(i)));
      } else {
	sprintf(buf, print_buffer, AREAL(i));
	gbuf = gbcopy(gbuf, buf);
      }
    };

    if (c == '%')
      gbuf = gbcopy(gbuf, "%");
  };

  if (i < arg_number)
    goto err_printf1;

  return new_string(gprint_buffer);

 err_printf0:
  error(NIL, "bad format string", NIL);
 err_printf1:
  error(NIL, "bad argument number", NIL);
}







/***********************************************************************
  STRING.C (C) /// initialisation ////
************************************************************************/

void 
init_string(void)
{
  ifn(gprint_buffer = tl_malloc(GPRINT_BUFFER))
  abort("Not enough memory");
  ifn(string_buffer = tl_malloc(STRING_BUFFER))
    abort("Not enough memory");

  class_define("STRING",&string_class );

  dx_define("left", xstr_left);
  dx_define("right", xstr_right);
  dx_define("mid", xstr_mid);
  dx_define("concat", xstr_concat);
  dx_define("index", xstr_index);
  dx_define("val", xstr_val);
  dx_define("str", xstr_number);
  dx_define("strhex", xstr_number_hex);
  dx_define("len", xstr_len);
  dx_define("strins", xstr_ins);
  dx_define("strdel", xstr_del);
  dx_define("strsubst", xstrsubst);
  dx_define("upcase", xupcase);
  dx_define("upcase1", xupcase1);
  dx_define("downcase", xdowncase);
  dx_define("asc", xstr_asc);
  dx_define("chr", xstr_chr);
  dx_define("isprint", xisprint);
  dx_define("stringp", xstringp);
  dx_define("regex_match", xregex_match);
  dx_define("regex_extract", xregex_extract);
  dx_define("regex_seek", xregex_seek);
  dx_define("regex_subst", xregex_subst);
  dx_define("sprintf", xsprintf);
}
