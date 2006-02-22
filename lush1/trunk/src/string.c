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
 * $Id: string.c,v 1.35 2006-02-22 22:28:06 leonb Exp $
 **********************************************************************/

#include "header.h"

#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif
#if HAVE_ICONV_H
# include <iconv.h>
# include <errno.h>
#endif


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
  st->cptr = 0;
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
  ifn (buffer = malloc(n+1))
    error(NIL, "memory exhausted", NIL);
  buffer[0] = 0;
  buffer[n] = 0;
  st->flag = STRING_ALLOCATED;
  st->start = buffer;
  st->cptr = 0;
  q = new_extern(&string_class, st);
  q->flags |= X_STRING;
  return q;
}


/*
 * new_string(s) s is a normal C string. Copy it in an allocated buffer, then
 * create a new AT and return it.
 */
at *
new_string(const char *s)
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
  if (s->cptr)
    lside_destroy_item(s->cptr);
  deallocate(&alloc_string, (struct empty_alloc *) s);
}


/*
 * string_name(p) returns the pname of extern p
 */
static char *
string_name(at *p)
{
  char *s = ((struct string *) (p->Object))->start;
  char *name = string_buffer;
#if HAVE_MBRTOWC
  int n = strlen(s);
  mbstate_t ps;
  memset(&ps, 0, sizeof(mbstate_t));
  *name++ = '\"'; 
  for(;;)
    {
      char *ind;
      int c = *(unsigned char*)s;
      wchar_t wc = 0;
      int m;
      if (name>=string_buffer+STRING_BUFFER-10)
	break;
      if (c == 0)
	break;
      if ((ind = strchr(special_string, c)))
	{
	  *name++ = '\\';
	  *name++ = aspect_string[ind - special_string];
	  s += 1;
	  continue;
	} 
      m = (int)mbrtowc(&wc, s, n, &ps);
      if (m <= 0)
	{
	  *name++ = '\\';
	  *name++ = 'x';
	  *name++ = digit_string[(c >> 4) & 15];
	  *name++ = digit_string[c & 15];
	  memset(&ps, 0, sizeof(mbstate_t));
	  s += 1;
	  continue;
	}
      if (iswprint(wc))
	{
	  memcpy(name, s, m);
	  name += m;
	}
      else if (m==1 && c<=' ')
	{
	  *name++ = '\\';
	  *name++ = '^';
	  *name++ = (char)(c | 0x40);
	}
      else
	{
	  int i;
	  for (i=0; i<m; i++)
	    if (name < string_buffer+STRING_BUFFER-10)
	      {
		c =  *(unsigned char*)(s+i);
		*name++ = '\\';
		*name++ = 'x';
		*name++ = digit_string[(c >> 4) & 15];
		*name++ = digit_string[c & 15];
	      }
	}
      s += m;
      n -= m;
    }
#else
  int c;
  *name++ = '\"'; 
  while ((c = *(unsigned char*)s)) 
    {
      char *ind;
      if (name >= string_buffer+STRING_BUFFER-10)
	break;
      if ((ind = strchr(special_string, c))) 
	{
	  *name++ = '\\';
	  *name++ = aspect_string[ind - special_string];
	} 
      else if (isascii(c) && isprint(c)) 
	{
	  *name++ = c;
	} 
      else if (c<=' ') 
	{
	  *name++ = '\\';
	  *name++ = '^';
	  *name++ = (char)(c | 0x40);
	} 
      else 
	{
	  *name++ = '\\';
	  *name++ = 'x';
	  *name++ = digit_string[(c >> 4) & 15];
	  *name++ = digit_string[c & 15];
	}
      s++;
    }
#endif
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


/* helpers to construct large strings -----------------	 */

void
large_string_init(struct large_string *ls)
{
  ls->backup = NIL;
  ls->where = &ls->backup;
  ls->p = ls->buffer;
}

void
large_string_add(struct large_string *ls, char *s, int len)
{
  if (len < 0)
    len = strlen(s);
  if (ls->p > ls->buffer)
    if (ls->p + len > ls->buffer + sizeof(ls->buffer) - 1)
      {
        *ls->p = 0;
        *ls->where = cons(new_string(ls->buffer), NIL);
        ls->where = &((*ls->where)->Cdr);
        ls->p = ls->buffer;
      }
  if (len > sizeof(ls->buffer) - 1)
    {
      at *p = new_string_bylen(len);
      memcpy(SADD(p->Object), s, len);
      *ls->where = cons(p, NIL);
      ls->where = &((*ls->where)->Cdr);
      ls->p = ls->buffer;
    }
  else 
    {
      memcpy(ls->p, s, len);
      ls->p += len;
    }
}

at *
large_string_collect(struct large_string *ls)
{
  char *r;
  at *p, *q;
  int len;
  *ls->p = 0;
  len = strlen(ls->buffer);
  for (p = ls->backup; p; p = p->Cdr)
    len += strlen(SADD(p->Car->Object));
  q = new_string_bylen(len);
  r = SADD(q->Object);
  for (p = ls->backup; p; p = p->Cdr) 
    {
      strcpy(r, SADD(p->Car->Object));
      r += strlen(r);
    }
  strcpy(r, ls->buffer);
  UNLOCK(ls->backup);
  ls->backup = NIL;
  ls->where = &ls->backup;
  ls->p = ls->buffer;
  return q;
}


/* multibyte strings ---------------------------------- */

#if HAVE_ICONV
static at *
recode(const char *s, const char *fromcode, const char *tocode)
{
  unsigned int ilen, olen;
  char *obuf, *ibuf;
  struct large_string ls;
  char buffer[512];
  at *ans = NIL;

  iconv_t conv = iconv_open(tocode, fromcode);
  if (conv)
    {
      ibuf = (char*)s;
      ilen = strlen(s);
      large_string_init(&ls);
      for(;;)
        {
          obuf = buffer;
          olen = sizeof(buffer);
          iconv(conv, &ibuf, &ilen, &obuf, &olen);
          if (obuf > buffer)
            large_string_add(&ls, buffer, obuf-buffer);
          if (ilen <= 0 || errno != E2BIG)
            break;
        }
      iconv_close(conv);
      ans = large_string_collect(&ls);
      if (ilen <= 0)
        return ans;
    }
  UNLOCK(ans);
  return NIL;
}
#endif

at* 
str_mb_to_utf8(const char *s)
{
  /* best effort conversion from locale encoding to utf8 */
#if HAVE_ICONV
  at *ans;
# if HAVE_NL_LANGINFO
  if ((ans = recode(s, nl_langinfo(CODESET), "UTF-8")))
    return ans;
# endif
  if ((ans = recode(s, "char", "UTF-8")))
    return ans;
  if ((ans = recode(s, "", "UTF-8")))
    return ans;
#endif
  return new_string(s);
}

at* 
str_utf8_to_mb(const char *s)
{
  /* best effort conversion from locale encoding from utf8 */
#if HAVE_ICONV
  at *ans;
# if HAVE_NL_LANGINFO
  if ((ans = recode(s, "UTF-8", nl_langinfo(CODESET))))
    return ans;
# endif
  if ((ans = recode(s, "UTF-8", "char")))
    return ans;
  if ((ans = recode(s, "UTF-8", "")))
    return ans;
#endif
  return new_string(s);
}

DX(xstr_locale_to_utf8)
{
  ARG_EVAL(1);
  ASTRING(1);
  return str_mb_to_utf8(SADD(APOINTER(1)->Object));
}

DX(xstr_utf8_to_locale)
{
  ARG_EVAL(1);
  ASTRING(1);
  return str_utf8_to_mb(SADD(APOINTER(1)->Object));
}


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


int 
str_index(char *s1, char *s2, int start)
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

char *
str_gptr(x)
gptr x;
{
    sprintf(string_buffer, "#$%lX", (unsigned long)(x));
    return string_buffer;
}

DX(xstr_gptr)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(str_gptr(AGPTR(1)));
}

/*------------------------ */

DX(xstr_len)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(strlen(ASTRING(1)));
}

/*------------------------ */


static at*
str_del(char *s, int n, int l)
{
  struct large_string ls;
  int len = strlen(s);
  n = (n>1) ? n-1 : 0;
  if (n > len)
    n = len;
  if (l< 0 || n+l > len)
    l = len - n;
  large_string_init(&ls);
  large_string_add(&ls, s, n);
  large_string_add(&ls, s+n+l, -1);
  return large_string_collect(&ls);
}

DX(xstr_del)
{
  int l = -1;
  ALL_ARGS_EVAL;
  if (arg_number != 2)
    {
      ARG_NUMBER(3);
      l = AINTEGER(3);
    }
  return str_del(ASTRING(1), AINTEGER(2), l);
}

/*------------------------ */

static at *
str_ins(char *s, int pos, char *what)
{
  struct large_string ls;
  int len = strlen(s);
  if (pos > len)
    pos = len;
  large_string_init(&ls);
  large_string_add(&ls, s, pos);
  large_string_add(&ls, what, -1);
  large_string_add(&ls, s + pos, -1);
  return large_string_collect(&ls);
}

DX(xstr_ins)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  return str_ins(ASTRING(1),AINTEGER(2),ASTRING(3));
}



/*------------------------ */

static at *
str_subst(char *s, char *s1, char *s2)
{
  struct large_string ls;
  int len1 = strlen(s1);
  int len2 = strlen(s2);
  char *last = s;
  large_string_init(&ls);
  while(*s) 
    {
      if ((*s == *s1) && (!strncmp(s,s1,len1)) ) {
        large_string_add(&ls, last, s - last);
        large_string_add(&ls, s2, len2);
        s += len1;
        last = s;
      } else
        s += 1;
    }
  if (s > last)
    large_string_add(&ls, last, s - last);
  return large_string_collect(&ls);
}

DX(xstrsubst)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  return str_subst(ASTRING(1),ASTRING(2),ASTRING(3));
}

/*------------------------ */



DX(xupcase)
{
  at *rr;
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
#if HAVE_MBRTOWC
  {
    char buffer[MB_LEN_MAX];
    struct large_string ls;
    mbstate_t ps1;
    mbstate_t ps2;
    int n = strlen(s);
    memset(&ps1, 0, sizeof(mbstate_t));
    memset(&ps2, 0, sizeof(mbstate_t));
    large_string_init(&ls);
    while(n > 0)
      {
	wchar_t wc = 0;
	int m = (int)mbrtowc(&wc, s, n, &ps1);
	if (m == 0)
	  break;
	if (m > 0)
	  {
	    int d = wcrtomb(buffer, towupper(wc), &ps2);
	    if (d <= 0)
	      large_string_add(&ls, s, m);
	    else
	      large_string_add(&ls, buffer, d);
	    s += m;
	    n -= m;
	  }
	else
	  {
	    memset(&ps1, 0, sizeof(mbstate_t));	 
	    memset(&ps2, 0, sizeof(mbstate_t));	 
	    large_string_add(&ls, s, 1);
	    s += 1;
	    n -= 1;
	  }
      }
    rr = large_string_collect(&ls);
  }
#else
 {
   char c, *r;
   rr = new_string_bylen(strlen(s));
   r = SADD(rr->Object);
   while ((c = *s++)) 
     *r++ = toupper((unsigned char)c);
   *r = 0;
 }
#endif
  return rr;
}

DX(xupcase1)
{
  char *s;
  at *rr;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
#if HAVE_MBRTOWC
  {
    char buffer[MB_LEN_MAX];
    struct large_string ls;
    int n = strlen(s);
    int m;
    wchar_t wc;
    mbstate_t ps1;
    mbstate_t ps2;
    memset(&ps1, 0, sizeof(mbstate_t));
    memset(&ps2, 0, sizeof(mbstate_t));
    large_string_init(&ls);
    m = (int)mbrtowc(&wc, s, n, &ps1);
    if (m > 0)
      {
	int d = wcrtomb(buffer, towupper(wc), &ps2);
	if (d > 0)
	  {
	    large_string_add(&ls, buffer, d);
	    s += m;
	    n -= m;
	  }
      }
    large_string_add(&ls, s, n);
    rr = large_string_collect(&ls);
  }
#else
  {
    char *r, c;
    rr = new_string_bylen(strlen(s));
    r = SADD(rr->Object);
    strcpy(r,s);
    if ((c = *r))
      *r =  toupper((unsigned char)c);
  }
#endif
  return rr;
}

DX(xdowncase)
{
  at *rr;
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
#if HAVE_MBRTOWC
  {
    char buffer[MB_LEN_MAX];
    struct large_string ls;
    mbstate_t ps1;
    mbstate_t ps2;
    int n = strlen(s);
    memset(&ps1, 0, sizeof(mbstate_t));
    memset(&ps2, 0, sizeof(mbstate_t));
    large_string_init(&ls);
    while(n > 0)
      {
	wchar_t wc = 0;
	int m = (int)mbrtowc(&wc, s, n, &ps1);
	if (m == 0)
	  break;
	if (m > 0)
	  {
	    int d = wcrtomb(buffer, towlower(wc), &ps2);
	    if (d <= 0)
	      large_string_add(&ls, s, m);
	    else
	      large_string_add(&ls, buffer, d);
	    s += m;
	    n -= m;
	  }
	else
	  {
	    memset(&ps1, 0, sizeof(mbstate_t));	 
	    memset(&ps2, 0, sizeof(mbstate_t));	 
	    large_string_add(&ls, s, 1);
	    s += 1;
	    n -= 1;
	  }
      }
    rr = large_string_collect(&ls);
  }
#else
  {
    char c, *r;
    rr = new_string_bylen(strlen(s));
    r = SADD(rr->Object);
    while ((c = *s++)) 
      *r++ = tolower((unsigned char)c);
    *r = 0;
  }
#endif
  return rr;
}

DX(xisprint)
{
  unsigned char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = (unsigned char*) ASTRING(1);
  if (!s || !*s)
    return NIL;
#if HAVE_MBRTOWC
  {
    int n = strlen((char*)s);
    mbstate_t ps;
    memset(&ps, 0, sizeof(mbstate_t));
    while(n > 0)
      {
	wchar_t wc = 0;
	int m = (int)mbrtowc(&wc, (char*)s, n, &ps);
	if (m == 0)
	  break;
	if (m < 0)
	  return NIL;
	if (! iswprint(wc))
	  return NIL;
	s += m;
	n -= m;
      }
  }
#else
  while (*s) {
    int c = *(unsigned char*)s;
    if (! (isascii(c) && isprint(c)))
      return NIL;
    s++;
  }
#endif
  return true();
}


/* ----------------------- */

DX(xstr_asc)
{
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
#if 0 /* Disabled for compatibility reasons */
  {
    mbstate_t ps;
    wchar_t wc = 0;
    memset(&ps, 0, sizeof(mbstate_t));
    mbrtowc(&wc, s, strlen(s), &ps);
    if (wc)
      return NEW_NUMBER(wc);
  }
  if (s[0])
    /* negative to indicate illegal sequence */
    return NEW_NUMBER((s[0] & 0xff) - 256); 
#else
  if (s[0])
    /* assume iso-8859-1 */
    return NEW_NUMBER(s[0] & 0xff);
#endif
  error(NIL,"Empty string",APOINTER(1));
}

DX(xstr_chr)
{
#if 0 /* Disabled for compatibility reasons */
  char s[MB_LEN_MAX+1];
  size_t m;
  mbstate_t ps;
  int i;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  memset(s, 0, sizeof(s));
  memset(&ps, 0, sizeof(mbstate_t));
  m = wcrtomb(s, (wchar_t)i, &ps);
  if (m==0 || m==(size_t)-1)
    error(NIL,"Out of range",APOINTER(1));
  return new_string(s);
#else
  int i;
  char s[2];
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  if (i<0 || i>255)
    error(NIL,"Out of range",APOINTER(1));
  s[0]=i;
  s[1]=0;
  return new_string(s);
#endif
}


/*------------------------ */

static at *
explode_bytes(char *s)
{
  at *p = NIL;
  at **where = &p;
  while (*s)
    {
      int code = *s;
      *where = cons(NEW_NUMBER(code & 0xff),NIL);
      where = &((*where)->Cdr);
      s += 1;
    }
  return p;
}

static at *
explode_chars(char *s)
{
#if HAVE_MBRTOWC
  at *p = NIL;
  at **where = &p;
  int n = strlen(s);
  mbstate_t ps;
  memset(&ps, 0, sizeof(mbstate_t));
  while (n > 0)
    {
      wchar_t wc = 0;
      int m = (int)mbrtowc(&wc, s, n, &ps);
      if (m == 0)
        break;
      if (m > 0)
        {
          *where = cons(NEW_NUMBER(wc),NIL);
          where = &((*where)->Cdr);
          s += m;
          n -= m;
        }
      else
        error(NIL,"Illegal characters in string",NIL);
    }
  return p;
#else
  return explode_bytes(s);
#endif
}

static at *
implode_bytes(at *p)
{
  char c;
  struct large_string ls;
  large_string_init(&ls);
  while (CONSP(p))
    {
      if (! NUMBERP(p->Car))
        error(NIL,"Number expected",p->Car);
      c = (char)(p->Car->Number);
      if (p->Car->Number != (real)(unsigned char)c)
        error(NIL,"Integer in range 0..255 expected",p->Car);
      large_string_add(&ls, &c, 1);
      p = p->Cdr;
    }
  return large_string_collect(&ls);
}

static at *
implode_chars(at *p)
{
#if HAVE_MBRTOWC
  mbstate_t ps;
  struct large_string ls;
  memset(&ps, 0, sizeof(mbstate_t));
  large_string_init(&ls);
  while (CONSP(p))
    {
      char buffer[MB_LEN_MAX];
      wchar_t wc;
      int d;
      if (! NUMBERP(p->Car))
        error(NIL,"Number expected",p->Car);
      wc = (wchar_t)(p->Car->Number);
      if (p->Car->Number != (real)wc)
        error(NIL,"Integer expected",p->Car);
      d = wcrtomb(buffer, wc, &ps);
      if (d > 0)
        large_string_add(&ls, buffer, d);
      else
        error(NIL,"Integer is out of range",p->Car);
      p = p->Cdr;
    }
  return large_string_collect(&ls);
#else
  return explode_bytes(p);
#endif
}

DX(xexplode_bytes)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return explode_bytes(ASTRING(1));
}

DX(xexplode_chars)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return explode_chars(ASTRING(1));
}

DX(ximplode_bytes)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return implode_bytes(APOINTER(1));
}

DX(ximplode_chars)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return implode_chars(APOINTER(1));
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

#define charset_zero(cset) \
   { int i; for(i=0;i<16;i++) (cset)[i]=0; }
#define charset_toggle(cset) \
   { int i; for(i=0;i<16;i++) (cset)[i]^=0xffff; }
#define charset_set(cset,i) \
   { (cset)[((unsigned char)(i))/16] |= (1<<(((unsigned char)(i))%16)); }
#define charset_tst(cset,i) \
   ( (cset)[((unsigned char)(i))/16] & (1<<(((unsigned char)(i))%16)) )

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


static void regex_alternate(bounds*, bounds, int*);

static void
regex_single(bounds *ans, bounds buf, int *rnum)
{
  bounds tmp;
  unsigned short *set;
  int toggle=0,last=0;
  unsigned char c;

  ans->beg = ans->end = buf.beg;

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
    concatc((*ans),*pat,buf);
    pat++;
    break;
    
  case '.':
    concatc((*ans),RE_ANY,buf);
    break;

  case '(':
    last = *rnum;
    *rnum = *rnum + 1;
    concatc((*ans),RE_START+last,buf);
    buf.beg += 1;
    regex_alternate(&tmp, buf,rnum);
    concatb((*ans),tmp,buf);
    if (*pat!=')') serror(1);
    concatc((*ans),RE_END+last,buf);
    pat++;
    break;
    
  case '[':
    if (ans->end>buf.beg+18) serror(0);
    set = ans->end+1;
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
    concatc((*ans),RE_RNG+last+1,buf);
    ans->end += last+1;
    break;

  case '^':
    concatc((*ans),RE_CARET,buf);
    break;
    
  case '$':
    concatc((*ans),RE_DOLLAR,buf);
    break;
    
  default:
    concatc((*ans),c,buf);
    break;
    
  }
}


static void
regex_several(bounds *ans, bounds buf, int *rnum)
{
  bounds b;
  bounds rem;

  ans->beg = buf.beg;
  ans->end = buf.beg;
  rem.beg = ans->beg+2;
  rem.end = buf.end;

  regex_single(&b, rem,rnum);

  switch (*pat) {

  case '?':
    concatc((*ans), RE_FAIL+(b.end-b.beg), buf);
    concatb((*ans), b, buf);
    pat++;
    break;

  case '+':
    concatb((*ans),b,buf);
    concatc((*ans), RE_FAIL+1, buf);
    concatc((*ans), RE_JMP+(b.beg-b.end)-2, buf);
    pat++;
    break;

  case '*':
    concatc((*ans), RE_FAIL+(b.end-b.beg)+1, buf);
    concatb((*ans),b,buf);
    concatc((*ans), RE_JMP+(b.beg-b.end)-2, buf);
    pat++;
    break;

  default:
    concatb((*ans),b,buf);
    break;
  }
}

static void
regex_catenate(bounds *ans, bounds buf, int *rnum)
{
  bounds b;
  bounds rem;

  ans->beg = buf.beg;
  ans->end = buf.beg;
  rem.beg = buf.beg;
  rem.end = buf.end;

  do {
    rem.beg = ans->end;
    regex_several(&b,rem,rnum);
    concatb((*ans),b,buf);
  } while (*pat && *pat!='|' && *pat!=')');
}

static void
regex_alternate(bounds *ans, bounds buf, int *rnum)
{
  bounds b1,b2;
  bounds rem;
  int newrnum;

  ans->beg = buf.beg;
  ans->end = buf.beg;
  rem.beg = buf.beg+1;
  rem.end = buf.end;

  newrnum = *rnum;
  regex_catenate(&b1,rem,rnum);
  if (*pat == '|') {
    pat++;
    rem.beg = b1.end+1;
    regex_alternate(&b2,rem,&newrnum);
    if (newrnum>*rnum)
      *rnum = newrnum;
    concatc((*ans), RE_FAIL+(b1.end-b1.beg+1), buf);
    concatb((*ans), b1, buf);
    concatc((*ans), RE_JMP +(b2.end-b2.beg), buf);
    concatb((*ans), b2, buf);
  } else {
    concatb((*ans), b1, buf);
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
      bounds buf, tmp;
      buf.beg = (unsigned short*) bufstart;
      buf.end = (unsigned short*) bufend;
      if (strict) 
	{
	  *buf.beg++ = RE_CARET;
	  buf.end--;
	}
      regex_alternate(&tmp,buf,rnum); 
      buf = tmp;
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
  buf = (unsigned short*) buffer;
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
    buf = (unsigned short*) buffer;
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
  char *start, *end, *s1;
  struct large_string ls;


  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  
  pat = ASTRING(1);
  str = ASTRING(2);
  dat = datstart = ASTRING(3);

  pat = regex_compile(pat,buffer,buffer+1024,0,NULL);
  if (pat)
    error(NIL,pat,APOINTER(1));
  
  large_string_init(&ls);
  do {
    if (! regex_seek(buffer,datstart,dat,regptr,reglen,10,&start,&end))
      start = end = dat + strlen(dat);
    if (end <= dat)
      start = end = dat + 1;
    if (dat < start) {
      s1 = dat;
      large_string_add(&ls, s1, start-s1);
      s1 = start;
    }
    if (start < end) {
      s1 = str;
      while (*s1) 
        {
          if (*s1 == '%') 
            {
              s1++;
              if (*s1 == '%') {
                large_string_add(&ls, s1++, 1);
              } else {
                int reg = *s1++ - '0';
                if (reg<0 || reg>9)
                  error(NIL,"bad register number",APOINTER(3));
                large_string_add(&ls, regptr[reg], reglen[reg]);
              }
            } 
          else 
            {
              char *s2 = s1;
              while (*s1 && *s1!='%')
                s1 += 1;
              large_string_add(&ls, s2, s1-s2);
            }
        }
    }
    while (*dat && dat<end)
      dat++;
  } while (*dat);
  
  return large_string_collect(&ls);
}










/***********************************************************************
  SPRINTF.C (C) LYB 1991
************************************************************************/

extern char *print_buffer;

DX(xsprintf)
{
  struct large_string ls;
  char *fmt, *buf, c;
  int i, n, ok;

  if (arg_number < 1)
    error(NIL, "At least one argument expected", NIL);

  ALL_ARGS_EVAL;
  fmt = ASTRING(1);
  large_string_init(&ls);
  i = 1;
  for(;;)
    {
      /* Copy plain string */
      if (*fmt == 0)
        break;
      buf = fmt;
      while (*fmt != 0 && *fmt != '%')
        fmt += 1;
      large_string_add(&ls, buf, fmt-buf);
      if (*fmt == 0)
        break;
      /* Copy format */
      n = 0;
      buf = print_buffer;
      ok = 0;
      c = 0;
      
      *buf++ = *fmt++;		/* copy  '%' */
      while (ok < 9) {
        c = *buf++ = *fmt++;
        switch (c) 
          {
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
      if (c != '%' && ++i > arg_number)
        goto err_printf1;
      if (c == 'l' || c == 'p')
        {
          large_string_add(&ls, pname(APOINTER(i)), -1);
        }
      else if (c == 'd') 
        {
          *buf++ = 0;
          if (ok == 9) {
            large_string_add(&ls, str_number((real) AINTEGER(i)), -1);
          } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
          } else {
            sprintf(buf, print_buffer, AINTEGER(i));
            large_string_add(&ls, buf, -1);
          }
        } 
      else if (c == 's') 
        {
          *buf++ = 0;
          if (ok == 9) {
            large_string_add(&ls, ASTRING(i), -1);
          } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
          } else {
            sprintf(buf, print_buffer, ASTRING(i));
            large_string_add(&ls, buf, -1);
          }
        } 
      else if (c == 'e' || c == 'f' || c == 'g') 
        {
          *buf++ = 0;
          if (ok == 9) {
            large_string_add(&ls, str_number(AREAL(i)), -1);
          } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
          } else {
            sprintf(buf, print_buffer, AREAL(i));
            large_string_add(&ls, buf, -1);
          }
        }
      if (c == '%')
        large_string_add(&ls, "%", 1);
    }
  if (i < arg_number)
    goto err_printf1;
  return large_string_collect(&ls);

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
  ifn(string_buffer = malloc(STRING_BUFFER))
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
  dx_define("strgptr", xstr_gptr);
  dx_define("len", xstr_len);
  dx_define("strins", xstr_ins);
  dx_define("strdel", xstr_del);
  dx_define("strsubst", xstrsubst);
  dx_define("upcase", xupcase);
  dx_define("upcase1", xupcase1);
  dx_define("downcase", xdowncase);
  dx_define("isprint", xisprint);
  dx_define("asc", xstr_asc);
  dx_define("chr", xstr_chr);
  dx_define("explode-bytes", xexplode_bytes);
  dx_define("explode-chars", xexplode_chars);
  dx_define("implode-bytes", ximplode_bytes);
  dx_define("implode-chars", ximplode_chars);
  dx_define("locale-to-utf8", xstr_locale_to_utf8);
  dx_define("utf8-to-locale", xstr_utf8_to_locale);
  dx_define("stringp", xstringp);
  dx_define("regex-match", xregex_match);
  dx_define("regex-extract", xregex_extract);
  dx_define("regex-seek", xregex_seek);
  dx_define("regex-subst", xregex_subst);
  dx_define("sprintf", xsprintf);
}
