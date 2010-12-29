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

#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif
#if HAVE_ICONV_H
# include <iconv.h>
# include <errno.h>
#endif

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
      singletons[c] = NEW_STRING(ss);
   }
}


at *make_string_of_length(size_t n)
{
   char *s = mm_blob(n+1);
   s[0] = s[n] = '\0';
   return NEW_STRING(s);
}


/* make a managed string for NUL-terminated string s */
at *make_string(const char *s)
{
   if (!s || !*s) {
      return null_string;

   } else if (!s[1] && ((uchar)s[0])<NUM_SINGLETONS) {
      return singletons[(int)s[0]];
     
   } else {
      char *sd = mm_strdup(s);
      assert(sd);
      return NEW_STRING(sd);
   }
}

static at *string_listeval (at *p, at *q)
{
   at *qi = eval_arglist(Cdr(q));
   ifn (LASTCONSP(qi))
      error(NIL, "one argument expected", NIL);
   ifn (NUMBERP(Car(qi)))
      error(NIL, "not a number", Car(qi));
   
   char *s = Mptr(p);
   assert(s);
   int n = strlen(s);
   int i = (int)Number(Car(qi));
   i = (i < 0) ? n + i : i;
   if (i<0 || i>=n)
      error(NIL, "not a valid index value", Car(qi));
   return NEW_NUMBER(s[i]);
}
     

/*
 * string_name(p) returns the pname of extern p
 */
static const char *string_name(at *p)
{
   const char *s = String(p);
   char *name = string_buffer;
#if HAVE_MBRTOWC
   int n = strlen(s);
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
   return mm_strdup(string_buffer);
}


/* string_compare
 * -- compare two strings
 */

static int string_compare(at *p, at *q, int order)
{
   return strcmp(String(p),String(q));
}


/* string hash
 * -- return hashcode for string
 */

static unsigned long string_hash(at *p)
{
   unsigned long x = 0x12345678;
   const char *s = String(p);
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

void large_string_add(large_string_t *ls, const char *s, int len)
{
   if (len < 0)
      len = strlen(s);
   if (ls->p > ls->buffer)
      if (ls->p + len > ls->buffer + sizeof(ls->buffer)-1) {
         *ls->p = 0;
         *ls->where = new_cons(make_string(ls->buffer), NIL);
         ls->where = &Cdr(*ls->where);
         ls->p = ls->buffer;
      }
   if (len > sizeof(ls->buffer)-1) {
      at *p = make_string_of_length(len);
      char *ps = (char *)String(p);
      memcpy(ps, s, len);
      *ls->where = new_cons(p, NIL);
      ls->where = &Cdr(*ls->where);
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
   for (at *p = ls->backup; p; p = Cdr(p))
      len += strlen(String(Car(p)));
   
   at *q = make_string_of_length(len);
   char *r = (char *)String(q);
   for (at *p = ls->backup; p; p = Cdr(p)) {
      strcpy(r, String(Car(p)));
      r += strlen(r);
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
      const char *ibuf = s;
      size_t ilen = strlen(s);
      large_string_init(ls);
      for(;;) {
         char *obuf = buffer;
         size_t olen = sizeof(buffer);
         iconv(conv, (char **)&ibuf, &ilen, &obuf, &olen);
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
   return NEW_STRING(s);
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
   return NEW_STRING(s);
}

DX(xstr_locale_to_utf8)
{
   return str_mb_to_utf8(ASTRING(1));
}

DX(xstr_utf8_to_locale)
{
   return str_utf8_to_mb(ASTRING(1));
}


/* operations on strings ------------------------------	 */


DX(xstr_left)
{
   ARG_NUMBER(2);
   const char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = strlen(s);
 
   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;

   at *p = make_string_of_length(n);
   char *a = (char *)String(p);
   strncpy(a,s,n);
   a[n] = 0;
   return p;
}

DX(xleft)
{
#ifndef NOWARN_DEPRECATED
  static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'left' is deprecated, use 'str-left'\n");
      warned = true;
   }
#endif
   ARG_NUMBER(2);
   const char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = strlen(s);
 
   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;

   at *p = make_string_of_length(n);
   char *a = (char *)String(p);
   strncpy(a,s,n);
   a[n] = 0;
   return p;
}

/*------------------------ */


DX(xstr_right)
{
   ARG_NUMBER(2);
   const char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = strlen(s);

   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;
   return make_string(s+l-n);
}

DX(xright)
{
#ifndef NOWARN_DEPRECATED
  static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'right' is deprecated, use 'str-right'\n");
      warned = true;
   }
#endif
   ARG_NUMBER(2);
   const char *s = ASTRING(1);
   int n = AINTEGER(2);
   int l = strlen(s);

   n = (n < 0) ? l+n : n;
   if (n < 0)
      RAISEFX(badarg, NEW_NUMBER(n));
   if (n > l)
      n = l;
   return make_string(s+l-n);
}

/*------------------------ */

DX(xsubstring)
{
   ARG_NUMBER(3);
   const char *s = ASTRING(1);
   int n = AINTEGER(2);
   int m = AINTEGER(3);
   int l = strlen(s);

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
   at *p = make_string_of_length(l);
   char *a = (char *)String(p);
   strncpy(a, s+n, l);
   a[l] = 0;
   return p;
}


DX(xstr_mid)
{
   if (arg_number == 2) {
      const char *s = ASTRING(1);
      int n = AINTEGER(2);
      int l = strlen(s);
      if (n < 0)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (n >= l)
         return null_string;
      else
         return make_string(s+n);

   } else {
      ARG_NUMBER(3);
      const char *s = ASTRING(1);
      int n = AINTEGER(2);
      int m = AINTEGER(3);
      if (n < 0)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (m < 0)
         RAISEFX(badarg, NEW_NUMBER(m));
      int l = strlen(s)-n;
      if (m > l)
         m = l;
      if (m < 1)
         return null_string;

      at *p = make_string_of_length(m);
      char *a = (char *)String(p);
      strncpy(a,s+n,m);
      a[m] = 0;
      return p;
   }
}

/* obsolete version */
DX(xmid)
{
#ifndef NOWARN_DEPRECATED
  static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'mid' is deprecated, use 'str-mid'\n");
      warned = true;
   }
#endif
   if (arg_number == 2) {
      const char *s = ASTRING(1);
      int n = AINTEGER(2);
      int l = strlen(s);
      if (n < 1)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (n > l)
         return null_string;
      else
         return make_string(s+n-1);

   } else {
      ARG_NUMBER(3);
      const char *s = ASTRING(1);
      int n = AINTEGER(2);
      int m = AINTEGER(3);
      if (n < 1)
         RAISEFX(badarg, NEW_NUMBER(n));	
      if (m < 0)
         RAISEFX(badarg, NEW_NUMBER(m));
      int l = strlen(s)-(n-1);
      if (m > l)
         m = l;
      if (m < 1)
         return null_string;

      at *p = make_string_of_length(m);
      char *a = (char *)String(p);
      strncpy(a,s+(n-1),m);
      a[m] = 0;
      return p;
   }
}

/*------------------------ */


DX(xstr_cat)
{
   int length = 0;
   for (int i=1; i<=arg_number; i++)
      length += (int)strlen(ASTRING(i));

   at *p = make_string_of_length(length);
   char *here = (char *)String(p);
   for (int i=1; i<=arg_number; i++) {
      const char *s = ASTRING(i);
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


static int str_index(const char *s1, const char *s2, int start)
{
   int indx = 1;
   while (*s2) {
      if (--start <= 0) {
         const char *sa = s2;
         const char *sb = s1;
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

DX(xindex)
{
#ifndef NOWARN_DEPRECATED
   static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'index' is deprecated, use 'str-find'\n");
      warned = true;
   }
#endif
   int start = 1;
   if (arg_number == 3)
      start = AINTEGER(3);
   else
      ARG_NUMBER(2);

   const char *s = ASTRING(1);
   if ((start = str_index(s, ASTRING(2), start)))
      return NEW_NUMBER(start);
   else
      return NIL;
}

int str_find(const char *s1, const char *s2, int start)
{
   int indx = 0;
   while (*s2) {
      if (start-- <= 0) {
         const char *sa = s2;
         const char *sb = s1;
         while (*sb && *sb == *sa++)
            sb++;
         if (*sb == 0)
            return indx;
      }
      indx++;
      s2++;
   }
   return -1;
}

DX(xstr_find)
{
   int start = 0;
   if (arg_number == 3)
      start = AINTEGER(3);
   else
      ARG_NUMBER(2);

   const char *s = ASTRING(1);
   if ((start = str_find(s, ASTRING(2), start)) != -1)
      return NEW_NUMBER(start);
   else
      return NIL;
}


/*------------------------ */

/* sscanf support for hex numbers ("0x800") seems to be missing */
/* in earlier glibc versions. Is this a special glibc feature?  */

static at *str_val_hex(const char *s)
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

at* str_val(const char *s) 
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
   return str_val(ASTRING(1));
}

/*------------------------ */

static char *nanlit = "NAN";
static char *inflit = "INFINITY";
static char *ninflit = "-INFINITY";

const char *str_number(double x)
{
   char *s, *t;
   
   if (isnan((real)x))
      return mm_strdup(nanlit);
   if (isinf((real)x))
      return mm_strdup((x>0 ? inflit : ninflit));
  
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
   return mm_strdup(string_buffer);
}

DX(xstr_number)
{
   ARG_NUMBER(1);
   return NEW_STRING(str_number(AREAL(1)));
}

/*------------------------ */

const char *str_number_hex(double x)
{
   int ix = (int)floor(x);
   
   if (isnan((real)x))
      return mm_strdup(nanlit);
   if (isinf((real)x))
      return mm_strdup((x>0 ? inflit : ninflit));
   if (ix == 0)
      return mm_strdup("0");
   
   sprintf(string_buffer, "0x%x", ix);
   return mm_strdup(string_buffer);
}

DX(xstr_number_hex)
{
   ARG_NUMBER(1);
   return NEW_STRING(str_number_hex(AREAL(1)));
}

/*------------------------ */

DX(xstr_len)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(strlen(ASTRING(1)));
}

/*------------------------ */


static at *str_del(const char *s, int n, int l)
{
   MM_ENTER;
   struct large_string ls;
   int len = strlen(s);
   n = (n<0) ? 0: n;
   if (n >= len)
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
   if (arg_number != 2) {
      ARG_NUMBER(3);
      l = AINTEGER(3);
   }
   return str_del(ASTRING(1), AINTEGER(2), l);
}


static at *strdel(const char *s, int n, int l)
{
   MM_ENTER;
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
   MM_RETURN(large_string_collect(&ls));
}


DX(xstrdel)
{
#ifndef NOWARN_DEPRECATED
   static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'strdel' is deprecated, use 'str-del'\n");
      warned = true;
   }
#endif
   int l = -1;
   if (arg_number != 2) {
      ARG_NUMBER(3);
      l = AINTEGER(3);
   }
   return strdel(ASTRING(1), AINTEGER(2), l);
}


/*------------------------ */

static at *str_insert(const char *s, int pos, const char *what)
{
   MM_ENTER;
   struct large_string ls;
   int len = strlen(s);
   if (pos > len)
      pos = len;
   large_string_init(&ls);
   large_string_add(&ls, s, pos);
   large_string_add(&ls, what, -1);
   large_string_add(&ls, s + pos, -1);
   MM_RETURN(large_string_collect(&ls));
}

DX(xstr_insert)
{
   ARG_NUMBER(3);
   return str_insert(ASTRING(1),AINTEGER(2),ASTRING(3));
}

DX(xstrins)
{
#ifndef NOWARN_DEPRECATED
  static bool warned = false;
   if (!warned) {
      fprintf(stderr, "*** Warning: 'strins' is deprecated, use 'str-insert'\n");
      warned = true;
   }
#endif
   ARG_NUMBER(3);
   return str_insert(ASTRING(1),AINTEGER(2),ASTRING(3));
}


/*------------------------ */

static at *str_subst(const char *s, const char *s1, const char *s2)
{
   MM_ENTER;
   struct large_string ls;
   int len1 = strlen(s1);
   int len2 = strlen(s2);
   const char *last = s;

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

DX(xstr_subst)
{
   ARG_NUMBER(3);
   return str_subst(ASTRING(1),ASTRING(2),ASTRING(3));
}

/*------------------------ */



DX(xupcase)
{
   ARG_NUMBER(1);
   const char *s = ASTRING(1);
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
    rr = make_string_of_length(strlen(s));
    r = (char *)String(rr);
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
   const char *s = ASTRING(1);
   at *rr = NIL;
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
      rr = make_string_of_length(strlen(s));
      r = String(rr);
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
   const char *s = ASTRING(1);
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
     rr = make_string_of_length(strlen(s));
     r = String(rr);
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
   uchar *s = (unsigned char*) ASTRING(1);
   if (!s || !*s)
      return NIL;
#if HAVE_MBRTOWC
   {
      int n = strlen((char*)s);
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
   const char *s = ASTRING(1);
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
   RAISEFX("empty string",APOINTER(1));
   return NIL; // make compiler happy
}

DX(xstr_chr)
{
#if 0 /* Disabled for compatibility reasons */
   char s[MB_LEN_MAX+1];
   mbstate_t ps;
   ARG_NUMBER(1);
   int i = AINTEGER(1);
   memset(s, 0, sizeof(s));
   memset(&ps, 0, sizeof(mbstate_t));
   size_t m = wcrtomb(s, (wchar_t)i, &ps);
   if (m==0 || m==(size_t)-1)
      RAISEFX("out of range", APOINTER(1));
   return make_string(s);
#else
   ARG_NUMBER(1);
   int i = AINTEGER(1);
   if (i<0 || i>255)
      error(NIL,"out of range", APOINTER(1));
   char s[2];
   s[0]=i;
   s[1]=0;
   return make_string(s);
#endif
}

/*------------------------ */

static at *explode_bytes(const char *s)
{
   at *p = NIL;
   at **where = &p;
   while (*s) {
      int code = *s;
      *where = new_cons(NEW_NUMBER(code & 0xff),NIL);
      where = &Cdr(*where);
      s += 1;
   }
   return p;
}

static at *explode_chars(const char *s)
{
#if HAVE_MBRTOWC
   at *p = NIL;
   at **where = &p;
   int n = strlen(s);
   mbstate_t ps;
   memset(&ps, 0, sizeof(mbstate_t));
   while (n > 0) {
      wchar_t wc = 0;
      int m = (int)mbrtowc(&wc, s, n, &ps);
      if (m == 0)
         break;
      if (m > 0) {
         *where = new_cons(NEW_NUMBER(wc),NIL);
         where = &Cdr(*where);
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
      if (! NUMBERP(Car(p)))
         RAISEF("number expected", Car(p));
      char c = (char)Number(Car(p));
      if (! c)
         break;
      if (Number(Car(p)) != (real)(unsigned char)c)
         RAISEF("integer in range 0..255 expected", Car(p));
      large_string_add(&ls, &c, 1);
      p = Cdr(p);
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

      if (! NUMBERP(Car(p)))
         RAISEF("number expected", Car(p));
      wc = (wchar_t)Number(Car(p));
      if (! wc)
         break;
      if (Number(Car(p)) != (real)wc)
         RAISEF("integer expected", Car(p));
      int d = wcrtomb(buffer, wc, &ps);
      if (d > 0)
         large_string_add(&ls, buffer, d);
      else
         error(NIL,"Integer is out of range", Car(p));
      p = Cdr(p);
   }
   MM_RETURN(large_string_collect(&ls));
#else
   return explode_bytes(p);
#endif
}

DX(xexplode_bytes)
{
   ARG_NUMBER(1);
   return explode_bytes(ASTRING(1));
}

DX(xexplode_chars)
{
   ARG_NUMBER(1);
   return explode_chars(ASTRING(1));
}

DX(ximplode_bytes)
{
   ARG_NUMBER(1);
   return implode_bytes(APOINTER(1));
}

DX(ximplode_chars)
{
   ARG_NUMBER(1);
   return implode_chars(APOINTER(1));
}




/*------------------------ */



DX(xstringp)
{
   ARG_NUMBER(1);
   at *p = APOINTER(1);
   if (STRINGP(p))
      return p;
   else
      return NIL;
}

DX(xvector_to_string)
{
   ARG_NUMBER(1);
   index_t *ind = as_contiguous_array(AINDEX(1));
   ifn ((IND_STTYPE(ind)==ST_INT) && (IND_NDIMS(ind)==1))
      RAISEF("int vector expected", APOINTER(1));
   
   /* check that these are all valid ASCII codes */
   int *vp = IND_BASE_TYPED(ind, int);
   for (int i=0; i<IND_DIM(ind,0); i++)
      if (!isascii(vp[i])) {
         fprintf(stderr, "*** Warning: not all values are character codes\n");
         break;
      }
 
   at *p = make_string_of_length(IND_DIM(ind, 0));
   unsigned char *s = (unsigned char *)String(p);
   for (int i=0; i<IND_DIM(ind,0); i++)
      s[i] = (unsigned char)vp[i];
   return p;
}

DX(xstring_to_vector)
{
   ARG_NUMBER(1);
   const char *s = ASTRING(1);
   int n = strlen(s);
   storage_t *st = new_storage_managed(ST_INT, n, NIL);
   int *stp = st->data;
   for (int i=0; i<n; i++)
      stp[i] = (int)s[i];
   return new_index(st, NULL)->backptr;
}

/***********************************************************************
  SPRINTF.C (C) LYB 1991
************************************************************************/

extern char print_buffer[];

DX(xsprintf)
{
   MM_ENTER;
   
   if (arg_number < 1)
      error(NIL, "At least one argument expected", NIL);
   
   const char *fmt = ASTRING(1);

   struct large_string ls;
   large_string_init(&ls);
   int i = 1;
   for(;;)
   {
      /* Copy plain string */
      if (*fmt == 0)
         break;
      {
         const char *tmp = fmt;
         while (*fmt != 0 && *fmt != '%')
            fmt += 1;
         large_string_add(&ls, tmp, fmt-tmp);
      }
      if (*fmt == 0)
         break;
      /* Copy format */
      int n = 0;
      char *buf = print_buffer;
      int ok = 0;
      char c = 0;
      
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
      if (c == 'l') {
         large_string_add(&ls, pname(APOINTER(i)), -1);

      } else if (c == 'p') {
         *buf++ = 0;
         at *a = APOINTER(i);
         ifn (GPTRP(a) || MPTRP(a)) AGPTR(i);
         if (ok == 9) {
            large_string_add(&ls, str_number_hex((double)(intptr_t)Gptr(a)), -1);
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, Gptr(a));
            large_string_add(&ls, buf, -1);
         }

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
         double d = AREAL(i);
         if (ok == 9 || isnan(d) || isinf(d)) {
            large_string_add(&ls, str_number(d), -1);
         } else if (n > print_buffer + LINE_BUFFER - buf - 1) {
            goto err_printf0;
         } else {
            sprintf(buf, print_buffer, d);
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

class_t *string_class;

void init_string(void)
{
   /* set up string_class */
   string_class = new_builtin_class(NIL);
   string_class->listeval = string_listeval;
   string_class->name = string_name;
   string_class->compare = string_compare;
   string_class->hash = string_hash;
   class_define("String", string_class);

   /* cache some ubiquitous strings */
   make_singletons();
   singletons[0] = null_string = make_string_of_length(0);
   MM_ROOT(singletons);

   dx_define("str-left", xstr_left);
   dx_define("str-right", xstr_right);
   dx_define("str-mid", xstr_mid);
   dx_define("substring", xsubstring);
   dx_define("str-cat", xstr_cat);
   dx_define("str-find", xstr_find);
   dx_define("str-val", xstr_val);
   dx_define("str", xstr_number);
   dx_define("strhex", xstr_number_hex);
   dx_define("str-len", xstr_len);
   dx_define("str-insert", xstr_insert);
   dx_define("str-del", xstr_del);
   dx_define("str-subst", xstr_subst);
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
   dx_define("string-to-vector", xstring_to_vector);
   dx_define("sprintf", xsprintf);

   /* deprecated functions */
   dx_define("mid", xmid);  
   dx_define("strdel", xstrdel);
   dx_define("index", xindex);
   dx_define("strins", xstrins);
   dx_define("left", xleft);
   dx_define("right", xright);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
