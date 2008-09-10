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
 * $Id: string.c,v 1.40 2007/08/03 15:42:30 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "mm.h"

#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif
#if HAVE_ICONV_H
# include <iconv.h>
# include <errno.h>
#endif

typedef unsigned char   uchar;

#define NUM_SINGLETONS 128

char string_buffer[STRING_BUFFER];
at   *null_string = NIL;

static at  **singletons = NULL;
static char  xdigit[]  = "0123456789abcdef";
static char  special[] = "\"\\\n\r\b\t\f\377";
static char  aspect[]  = "\"\\nrbtfe";

static char *badarg = "argument out of range";

static void make_singletons(void)
{
   if (singletons) return;
   singletons = mm_allocv(mt_refs, NUM_SINGLETONS*sizeof(at *));

   char s[2] = " ";
   for (int c = 1; c < NUM_SINGLETONS; c++) {
      s[0] = (char)c;
      char *ss = (char *)mm_strdup(s);
      assert(ss);
      singletons[c] = new_extern(&string_class, ss);
   }
}

/*
 * new_string_bylen(n)
 * returns a string for length n
 */
at *new_string_bylen(int n)
{
   assert(n>=0);

   char *s = mm_allocv(mt_blob, n+1);
   ifn (s)
      error(NIL, "memory exhausted", NIL);
   
   s[0] = s[n] = 0;
   return new_extern(&string_class, s);
}

/* s is a nul-terminated string (a "C string") */
at *new_string(const char *s)
{
   if (!s || !*s) {
      return null_string;

   } else if (!s[1] && ((uchar)s[0])<NUM_SINGLETONS) {
      return singletons[(int)s[0]];
     
   } else {
      char *sd = mm_strdup(s);
      assert(sd);
      return new_extern(&string_class, sd);
   }
}


static at *string_listeval (at *p, at *q)
{
   at *qi = eval_a_list(q->Cdr);
   ifn (LASTCONSP(qi))
      error(NIL, "one argument expected", NIL);
   ifn (NUMBERP(qi->Car))
      error(NIL, "not a number", qi->Car);
   
   char *s = p->Object;
   assert(s);
   int n = mm_strlen(s);
   int i = (int)Number(qi->Car);
   i = (i < 0) ? n + i : i;
   if (i<0 || i>=n)
      error(NIL, "not a valid index value", qi->Car);
   return NEW_NUMBER(s[i]);
}
     

/*
 * string_name(p) returns the pname of extern p
 */
static char *string_name(at *p)
{
   char *s = p->Object;
   char *name = string_buffer;
#if HAVE_MBRTOWC
   int n = mm_strlen(s);
   mbstate_t ps;
   memset(&ps, 0, sizeof(mbstate_t));
   *name++ = '\"'; 
   for(;;) {
      char *ind;
      int c = *(unsigned char*)s;
      wchar_t wc = 0;
      if (name >= (string_buffer+STRING_BUFFER-10))
         break;
      if (c == 0)
         break;
      if ((ind = strchr(special, c))) {
         *name++ = '\\';
         *name++ = aspect[ind - special];
         s += 1;
         continue;
      } 
      int m = (int)mbrtowc(&wc, s, n, &ps);
      if (m <= 0) {
         *name++ = '\\';
         *name++ = 'x';
         *name++ = xdigit[(c >> 4) & 15];
         *name++ = xdigit[c & 15];
         memset(&ps, 0, sizeof(mbstate_t));
         s += 1;
         continue;
      }
      if (iswprint(wc)) {
         memcpy(name, s, m);
         name += m;

      } else if (m==1 && c<=' ') {
         *name++ = '\\';
         *name++ = '^';
         *name++ = (char)(c | 0x40);

      } else {
         for (int i=0; i<m; i++)
	    if (name < string_buffer+STRING_BUFFER-10) {
               c =  *(unsigned char*)(s+i);
               *name++ = '\\';
               *name++ = 'x';
               *name++ = xdigit[(c >> 4) & 15];
               *name++ = xdigit[c & 15];
            }
      }
      s += m;
      n -= m;
   }
#else
   int c;
   *name++ = '\"'; 
   while ((c = *(unsigned char*)s)) {
      if (name >= string_buffer+STRING_BUFFER-10)
         break;
      char *ind = strchr(special, c);
      if (ind) {
         *name++ = '\\';
         *name++ = aspect[ind - special];

      } else if (isascii(c) && isprint(c)) {
         *name++ = c;
         
      } else if (c <= ' ') {
         *name++ = '\\';
         *name++ = '^';
         *name++ = (char)(c | 0x40);

      } else {
         *name++ = '\\';
         *name++ = 'x';
         *name++ = xdigit[(c >> 4) & 15];
         *name++ = xdigit[c & 15];
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

static int string_compare(at *p, at *q, int order)
{
   return strcmp(SADD(p->Object),SADD(q->Object));
}


/* string hash
 * -- return hashcode for string
 */

static unsigned long string_hash(at *p)
{
   unsigned long x = 0x12345678;
   char *s = SADD(p->Object);
   while (*s) {
      x = (x<<6) | ((x&0xfc000000)>>26);
      x ^= (*s);
      s++;
   }
   return x;
}


/* helpers to construct large strings -----------------	 */

void large_string_init(large_string_t *ls)
{
   ls->backup = NIL;
   ls->where = &ls->backup;
   ls->p = ls->buffer;
}

void large_string_add(large_string_t *ls, char *s, int len)
{
   if (len < 0)
      len = strlen(s);
   if (ls->p > ls->buffer)
      if (ls->p + len > ls->buffer + sizeof(ls->buffer)-1) {
         *ls->p = 0;
         *ls->where = new_cons(new_string(ls->buffer), NIL);
         ls->where = &((*ls->where)->Cdr);
         ls->p = ls->buffer;
      }
   if (len > sizeof(ls->buffer)-1) {
      at *p = new_string_bylen(len);
      memcpy(SADD(p->Object), s, len);
      *ls->where = new_cons(p, NIL);
      ls->where = &((*ls->where)->Cdr);
      ls->p = ls->buffer;
   } else {
      memcpy(ls->p, s, len);
      ls->p += len;
   }
}

at *large_string_collect(large_string_t *ls)
{
   *ls->p = 0;
   int len = strlen(ls->buffer);
   for (at *p = ls->backup; p; p = p->Cdr)
      len += mm_strlen(SADD(p->Car->Object));
   
   at *q = new_string_bylen(len);
   char *r = SADD(q->Object);
   for (at *p = ls->backup; p; p = p->Cdr) {
      strcpy(r, SADD(p->Car->Object));
      r += mm_strlen(r);
   }
   strcpy(r, ls->buffer);
   large_string_init(ls);
   return q;
}


/* multibyte strings ---------------------------------- */

#if HAVE_ICONV
static at *recode(const char *s, const char *fromcode, const char *tocode)
{
   MM_ENTER;
   large_string_t lstring, *ls = &lstring;
   char buffer[512];
   
   iconv_t conv = iconv_open(tocode, fromcode);

   if (conv) {
      char *ibuf = (char*)s;
      size_t ilen = strlen(s);
      large_string_init(ls);
      for(;;) {
         char *obuf = buffer;
         size_t olen = sizeof(buffer);
         iconv(conv, &ibuf, &ilen, &obuf, &olen);
         if (obuf > buffer)
            large_string_add(ls, buffer, obuf-buffer);
         if (ilen==0 || errno!=E2BIG)
            break;
      }
      iconv_close(conv);
      if (ilen == 0)
         MM_RETURN(large_string_collect(ls));
   }
   MM_EXIT;
   return NIL;
}
#endif

at* str_mb_to_utf8(const char *s)
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

at* str_utf8_to_mb(const char *s)
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
   return str_mb_to_utf8(ASTRING(1));
}

DX(xstr_utf8_to_locale)
{
   ARG_EVAL(1);
   return str_utf8_to_mb(ASTRING(1));
}


/* operations on strings ------------------------------	 */


DX(xstr_left)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;

   char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = mm_strlen(s);
 
   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;

   at *p = new_string_bylen(n);
   char *a = SADD(p->Object);
   strncpy(a,s,n);
   a[n] = 0;
   return p;
}


/*------------------------ */


DX(xstr_right)
{
   ARG_NUMBER(2);
   ALL_ARGS_EVAL;
   
   char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = mm_strlen(s);

   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;
   return new_string(s+l-n);
}


/*------------------------ */

DX(xsubstring)
{
   ARG_NUMBER(3);
   ALL_ARGS_EVAL;

   char *s = ASTRING(1);
   int n = AINTEGER(2);
   int m = AINTEGER(3);
   int l = mm_strlen(s);

   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));	
   if (n > l)
      n = l;
  
   m = (m < 0) ? l+m : m;
   if (m < 0)
      RAISEFX(badarg, NEW_NUMBER(m));
   if (m > l)
      m = l;
   if (m <= n)
      return null_string;

   /* create new string of length m-n+1 */
   l = m-n;
   at *p = new_string_bylen(l);
   char *a = SADD(p->Object);
   strncpy(a, s+n, l);
   a[l] = 0;
   return p;
}


DX(xstr_mid)
{
   char *s, *a;
   at *p;
   int n,m,l;
   
   ALL_ARGS_EVAL;
   if (arg_number == 2) {
      s = ASTRING(1);
      n = AINTEGER(2);
      l = mm_strlen(s);
      if (n < 1)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (n > l)
         return null_string;
      else
         return new_string(s+n-1);

   } else {
      ARG_NUMBER(3);
      s = ASTRING(1);
      n = AINTEGER(2);
      m = AINTEGER(3);
      if (n < 1)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (m < 0)
         RAISEFX(badarg, NEW_NUMBER(m));
      l = mm_strlen(s)-(n-1);
      if (m > l)
         m = l;
      if (m < 1)
         return null_string;

      p = new_string_bylen(m);
      a = SADD(p->Object);
      strncpy(a,s+(n-1),m);
      a[m] = 0;
      return p;
   }
}

/*------------------------ */


DX(xstr_concat)
{
   ALL_ARGS_EVAL;

   int length = 0;
   for (int i=1; i<=arg_number; i++)
      length += (int)mm_strlen(ASTRING(i));

   at *p = new_string_bylen(length);
   char *here = SADD(p->Object);
   for (int i=1; i<=arg_number; i++) {
      char *s = ASTRING(i);
      while (*s) {
         length--;
         *here++ = *s++;
      }
   }
   *here = 0;
   assert(length==0);
   return p;
}


/*------------------------ */


int str_index(char *s1, char *s2, int start)
{
   int indx = 1;
   while (*s2) {
      if (--start <= 0) {
         char *sa = s2;
         char *sb = s1;
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
   ALL_ARGS_EVAL;

   int start = 1;
   if (arg_number == 3)
      start = AINTEGER(3);
   else
      ARG_NUMBER(2);

   char *s = ASTRING(1);
   if ((start = str_index(s, ASTRING(2), start)))
      return NEW_NUMBER(start);
   else
      return NIL;
}


/*------------------------ */

/* sscanf support for hex numbers ("0x800") seems to be missing */
/* in earlier glibc versions. Is this a special glibc feature?  */

static at *str_val_hex(char *s)
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
   while (*s) {
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

at* str_val(char *s) 
{
   at *p = str_val_hex(s);
   ifn (p) {
      double d;
      char buff[2];
      int n = sscanf(s, " %lf %1s", &d, buff);
      if ((n==EOF) || (n!=1))
         p = NIL;
      else 
         p = NEW_NUMBER(d);
   }
   return p;
}

/* static at * */
/* str_val_float(char *s) */
/* { */
/*   int flag; */
/*   char *h; */
/*   while (isspace((unsigned char)*s)) */
/*     s++; */
/*   h = s; */
/*   if (isdigit((unsigned char)*s) || */
/*       (*s == '+' || *s == '-' || *s == '.')) { */
/*     flag = 0; */
/*     while (isdigit((unsigned char)*s) || *s == '+' || *s == '-' */
/* 	   || *s == '.' || *s == 'e' || *s == 'E') { */
/*       if (isdigit((unsigned char)*s)) */
/* 	flag |= 0x1; */
/*       if (*s == '+' || *s == '-') { */
/* 	if (flag & 0x3) */
/* 	  goto fin; */
/* 	else */
/* 	  flag |= 0x2; */
/*       } */
/*       if (*s == '.') { */
/* 	if (flag & 0xc) */
/* 	  goto fin; */
/* 	else */
/* 	  flag |= 0x4; */
/*       } */
/*       if (*s == 'e' || *s == 'E') { */
/* 	if (flag & 0x8) */
/* 	  goto fin; */
/* 	else { */
/* 	  flag &= 0xc; */
/* 	  flag |= 0x8; */
/* 	}; */
/*       } */
/*       s++; */
/*     } */
/*     while (isspace((unsigned char)*s)) */
/*       s++; */
/*     ifn(*s == 0 && (flag & 0x1)) */
/*       goto fin; */
/*     return NEW_NUMBER(atof(h)); */
/*   } */
/*  fin: */
/*   return NIL; */
/* } */

/* at *str_val(char *s) */
/* { */
/*   at *p = str_val_hex(s); */
/*   if (!p) */
/*     p = str_val_float(s); */
/*   return p; */
/* } */

DX(xstr_val)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   return str_val(ASTRING(1));
}

/*------------------------ */

static char *nanlit = "NAN";
static char *inflit = "INF";
static char *ninflit = "-INF";

char *str_number(double x)
{
   char *s, *t;
   
   if (isnanD((real)x))
      return nanlit;
   if (isinfD((real)x))
      return (x>0 ? inflit : ninflit);
  
   real y = fabs(x);
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

char *str_number_hex(double x)
{
   int ix = (int)floor(x);
   
   if (isnanD((real)x))
      return nanlit;
   if (isinfD((real)x))
      return (x>0 ? inflit : ninflit);
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

char *str_gptr(gptr x)
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
   return NEW_NUMBER(mm_strlen(ASTRING(1)));
}

/*------------------------ */


static at *str_del(char *s, int n, int l)
{
   MM_ENTER;
   struct large_string ls;
   int len = mm_strlen(s);
   n = (n>1) ? n-1 : 0;
   if (n > len)
      n = len;
   if (l< 0 || n+l > len)
      l = len - n;
   large_string_init(&ls);
   large_string_add(&ls, s, n);
   large_string_add(&ls, s+n+l, -1);
   MM_RETURN(large_string_collect(&ls));
}

DX(xstr_del)
{
   int l = -1;
   ALL_ARGS_EVAL;
   if (arg_number != 2) {
      ARG_NUMBER(3);
      l = AINTEGER(3);
   }
   return str_del(ASTRING(1), AINTEGER(2), l);
}

/*------------------------ */

static at *str_ins(char *s, int pos, char *what)
{
   MM_ENTER;
   struct large_string ls;
   int len = mm_strlen(s);
   if (pos > len)
      pos = len;
   large_string_init(&ls);
   large_string_add(&ls, s, pos);
   large_string_add(&ls, what, -1);
   large_string_add(&ls, s + pos, -1);
   MM_RETURN(large_string_collect(&ls));
}

DX(xstr_ins)
{
   ARG_NUMBER(3);
   ALL_ARGS_EVAL;
   return str_ins(ASTRING(1),AINTEGER(2),ASTRING(3));
}



/*------------------------ */

static at *str_subst(char *s, char *s1, char *s2)
{
   MM_ENTER;
   struct large_string ls;
   int len1 = mm_strlen(s1);
   int len2 = mm_strlen(s2);
   char *last = s;

   large_string_init(&ls);
   while(*s) {
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
   MM_RETURN(large_string_collect(&ls));
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
   ARG_NUMBER(1);
   ARG_EVAL(1);

   char *s = ASTRING(1);
   at *rr = NIL;

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
      while(n > 0) {
         wchar_t wc = 0;
         int m = (int)mbrtowc(&wc, s, n, &ps1);
         if (m == 0)
            break;
         if (m > 0) {
	    int d = wcrtomb(buffer, towupper(wc), &ps2);
	    if (d <= 0)
               large_string_add(&ls, s, m);
	    else
               large_string_add(&ls, buffer, d);
	    s += m;
	    n -= m;
         } else {
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
    rr = new_string_bylen(mm_strlen(s));
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
   ARG_NUMBER(1);
   ARG_EVAL(1);
   char *s = ASTRING(1);
   at *rr = NIL;
#if HAVE_MBRTOWC
   {
      char buffer[MB_LEN_MAX];
      struct large_string ls;
      int n = mm_strlen(s);
      int m;
      wchar_t wc;
      mbstate_t ps1;
      mbstate_t ps2;
      memset(&ps1, 0, sizeof(mbstate_t));
      memset(&ps2, 0, sizeof(mbstate_t));
      large_string_init(&ls);
      m = (int)mbrtowc(&wc, s, n, &ps1);
      if (m > 0) {
         int d = wcrtomb(buffer, towupper(wc), &ps2);
         if (d > 0) {
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
      rr = new_string_bylen(mm_strlen(s));
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
   ARG_NUMBER(1);
   ARG_EVAL(1);
   char *s = ASTRING(1);
   at *rr = NIL;
#if HAVE_MBRTOWC
   {
      char buffer[MB_LEN_MAX];
      struct large_string ls;
      mbstate_t ps1;
      mbstate_t ps2;
      int n = mm_strlen(s);
      memset(&ps1, 0, sizeof(mbstate_t));
      memset(&ps2, 0, sizeof(mbstate_t));
      large_string_init(&ls);
      while(n > 0) {
         wchar_t wc = 0;
         int m = (int)mbrtowc(&wc, s, n, &ps1);
         if (m == 0)
            break;
         if (m > 0) {
            int d = wcrtomb(buffer, towlower(wc), &ps2);
            if (d <= 0)
               large_string_add(&ls, s, m);
            else
               large_string_add(&ls, buffer, d);
            s += m;
            n -= m;
         } else {
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
     rr = new_string_bylen(mm_strlen(s));
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
   ARG_NUMBER(1);
   ARG_EVAL(1);
   uchar *s = (unsigned char*) ASTRING(1);
   if (!s || !*s)
      return NIL;
#if HAVE_MBRTOWC
   {
      int n = mm_strlen((char*)s);
      mbstate_t ps;
      memset(&ps, 0, sizeof(mbstate_t));
      while(n > 0) {
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
   return t();
}


/* ----------------------- */

DX(xstr_asc)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   char *s = ASTRING(1);
#if 0 /* Disabled for compatibility reasons */
   {
      mbstate_t ps;
      wchar_t wc = 0;
      memset(&ps, 0, sizeof(mbstate_t));
      mbrtowc(&wc, s, mm_strlen(s), &ps);
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
   RAISEFX("empty string",APOINTER(1));
}

DX(xstr_chr)
{
#if 0 /* Disabled for compatibility reasons */
   char s[MB_LEN_MAX+1];
   mbstate_t ps;
   ARG_NUMBER(1);
   ARG_EVAL(1);
   int i = AINTEGER(1);
   memset(s, 0, sizeof(s));
   memset(&ps, 0, sizeof(mbstate_t));
   size_t m = wcrtomb(s, (wchar_t)i, &ps);
   if (m==0 || m==(size_t)-1)
      RAISEFX("out of range", APOINTER(1));
   return new_string(s);
#else
   ARG_NUMBER(1);
   ARG_EVAL(1);
   int i = AINTEGER(1);
   if (i<0 || i>255)
      error(NIL,"out of range", APOINTER(1));
   char s[2];
   s[0]=i;
   s[1]=0;
   return new_string(s);
#endif
}

/*------------------------ */

static at *explode_bytes(char *s)
{
   at *p = NIL;
   at **where = &p;
   while (*s) {
      int code = *s;
      *where = new_cons(NEW_NUMBER(code & 0xff),NIL);
      where = &((*where)->Cdr);
      s += 1;
   }
   return p;
}

static at *explode_chars(char *s)
{
#if HAVE_MBRTOWC
   at *p = NIL;
   at **where = &p;
   int n = mm_strlen(s);
   mbstate_t ps;
   memset(&ps, 0, sizeof(mbstate_t));
   while (n > 0) {
      wchar_t wc = 0;
      int m = (int)mbrtowc(&wc, s, n, &ps);
      if (m == 0)
         break;
      if (m > 0) {
         *where = new_cons(NEW_NUMBER(wc),NIL);
         where = &((*where)->Cdr);
         s += m;
         n -= m;
      } else
         RAISEF("Illegal characters in string",NIL);
   }
   return p;
#else
   return explode_bytes(s);
#endif
}

static at *implode_bytes(at *p)
{
   MM_ENTER;
   struct large_string ls;
   large_string_init(&ls);
   
   while (CONSP(p)) {
      if (! NUMBERP(p->Car))
         RAISEF("number expected",p->Car);
      char c = (char)Number(p->Car);
      if (! c)
         break;
      if (Number(p->Car) != (real)(unsigned char)c)
         RAISEF("integer in range 0..255 expected",p->Car);
      large_string_add(&ls, &c, 1);
      p = p->Cdr;
   }
   MM_RETURN(large_string_collect(&ls));
}

static at *implode_chars(at *p)
{
#if HAVE_MBRTOWC
   MM_ENTER;
   mbstate_t ps;
   struct large_string ls;
   memset(&ps, 0, sizeof(mbstate_t));
   large_string_init(&ls);
   while (CONSP(p)) {
      char buffer[MB_LEN_MAX];
      wchar_t wc;

      if (! NUMBERP(p->Car))
         RAISEF("number expected", p->Car);
      wc = (wchar_t)Number(p->Car);
      if (! wc)
         break;
      if (Number(p->Car) != (real)wc)
         RAISEF("integer expected", p->Car);
      int d = wcrtomb(buffer, wc, &ps);
      if (d > 0)
         large_string_add(&ls, buffer, d);
      else
         error(NIL,"Integer is out of range",p->Car);
      p = p->Cdr;
   }
   MM_RETURN(large_string_collect(&ls));
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
   ARG_NUMBER(1);
   ARG_EVAL(1);
   at *p = APOINTER(1);
   if (STRINGP(p))
      return p;
   else
      return NIL;
}

DX(xvector_to_string)
{
   ARG_NUMBER(1);
   ARG_EVAL(1);
   index_t *ind = AINDEX(1);
   ifn ((IND_STTYPE(ind)==ST_UBYTE) && (IND_NDIMS(ind)==1))
      RAISEF("ubyte vector expected", APOINTER(1));

   ind = as_contiguous_array(ind);
   at *p = new_string_bylen(IND_DIM(ind, 0));
   char *s = SADD(p->Object);
   memcpy(s, IND_BASE(ind), IND_DIM(ind, 0));
   
   //delete_index(ind);
   return p;
}

/***********************************************************************
  SPRINTF.C (C) LYB 1991
************************************************************************/

extern char print_buffer[];

DX(xsprintf)
{
   MM_ENTER;
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
      if (c == 'l' || c == 'p') {
         large_string_add(&ls, pname(APOINTER(i)), -1);

      } else if (c == 'd') {
         *buf++ = 0;
         if (ok == 9) {
            large_string_add(&ls, str_number((real) AINTEGER(i)), -1);
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, AINTEGER(i));
            large_string_add(&ls, buf, -1);
         }

      } else if (c == 's') {
         *buf++ = 0;
         if (ok == 9) {
            large_string_add(&ls, ASTRING(i), -1);
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, ASTRING(i));
            large_string_add(&ls, buf, -1);
         }

      } else if (c == 'e' || c == 'f' || c == 'g') {
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
   MM_RETURN(large_string_collect(&ls));
   
err_printf0:
   MM_EXIT;
   error(NIL, "bad format string", NIL);
err_printf1:
   MM_EXIT;
   error(NIL, "bad argument number", NIL);
}







/***********************************************************************
  STRING.C (C) /// initialisation ////
************************************************************************/

class_t string_class;

void init_string(void)
{
   /* set up string_class */
   class_init(&string_class, false);
   string_class.listeval = string_listeval;
   string_class.name = string_name;
   string_class.compare = string_compare;
   string_class.hash = string_hash;
   class_define("STRING", &string_class);

   /* cache some ubiquitous strings */
   make_singletons();
   singletons[0] = null_string = new_string_bylen(0);
   MM_ROOT(singletons);

   dx_define("left", xstr_left);
   dx_define("right", xstr_right);
   dx_define("mid", xstr_mid);
   dx_define("substring", xsubstring);
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
   dx_define("vector-to-string", xvector_to_string);
   dx_define("sprintf", xsprintf);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
