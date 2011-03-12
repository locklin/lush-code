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
  This file contains functions for handling date/time
********************************************************************** */


/* ----------------------------------------
 * Declarations
 */
#include "header.h"

#ifdef TIME_WITH_SYS_TIME
# include <sys/times.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#define MINDATE 0
#define MAXDATE 6

struct date {
   unsigned char from, to;
   unsigned char x[MAXDATE];
   char str[24];
};

#define DATEP(x)  ((x)&&(Class(x) == date_class))
 
/* ----------------------------------------
 * Ansi conversion strings
 */


static char *ansidatenames[] = {
   "year",
   "month",
   "day",
   "hour",
   "minute",
   "second",
};


/* ----------------------------------------
 * date check
 */


static int date_check(struct date *dt)
{
   static char mlen[] = {31,29,31,30,31,30,31,31,30,31,30,31};
   
   int m = 1;
   if (dt->from<=DATE_DAY && dt->to>=DATE_DAY) {
      int d = dt->x[DATE_DAY];
      if (dt->from<=DATE_MONTH) {
         m = dt->x[DATE_MONTH];
         if (d>mlen[m-1])
	    return -1;
      }
      if (dt->from<=DATE_YEAR) {
         int y = dt->x[DATE_YEAR];
         if (m==2 && d==29)
	    if (!((y%4==0)&&((y%100!=0)||(y%400==100))))
               return -1;
      }
   }
   return 0;
}

/* ----------------------------------------
 * DATE -> STRING
 */

static int str_date_imp(struct date *d, char *s)
{
   char buffer[8];
   if( ! s )
      s = d->str; /* inside buffer */
   for (int i=d->from; i<=d->to; i++) {
      switch(i) {
      case DATE_YEAR:
         sprintf(buffer," %04d",1900 + d->x[i]);
         break;
      case DATE_MONTH:
      case DATE_DAY:
         sprintf(buffer,"-%02d",d->x[i]);
         break;
      case DATE_HOUR:
         sprintf(buffer," %02d",d->x[i]);
         break;
      case DATE_MINUTE:
      case DATE_SECOND:
         sprintf(buffer,":%02d",d->x[i]);
         break;
      }
      char *r = buffer;
      if (i==d->from)
         r++;
      while (*r)
         *s++ = *r++;
   }
   *s = 0;
   return 0;
}


/* ----------------------------------------
 * STRING -> DATE
 */


static int new_date_imp(const char *s, struct date *d, int from, int to)
{
   if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to)
      error(NIL, "illegal date range specification", NIL);
   d->from = from;
   d->to = to;

   char c = 0;
   while (*s==' ')
      s++;
   for (int i=from; i<=to; i++) {
      if (*s<'0' || *s>'9') 
         return -1;
      int r = 0;
      while (*s>='0' && *s<='9')
         r = r*10 + (*s++) - '0';
      switch(i) {
      case DATE_YEAR:
         if (r<1900 || r>1900+255) 
	    return -1;
         d->x[i] = r-1900;
         c = '-';
         break;
      case DATE_MONTH:
         if (r<1 || r>12) 
	    return -1;
         d->x[i] = r;
         c = '-';
         break;
      case DATE_DAY:
         if (r<1 || r>31) 
            return -1;
         d->x[i] = r;
         c = ' ';
         break;
      case DATE_HOUR:
         if (r<0 || r>23) 
	    return -1;
         d->x[i] = r;
         c = ':';
         break;
      case DATE_MINUTE:
         if (r<0 || r>59) 
	    return -1;
         d->x[i] = r;
         c = ':';
         break;
      case DATE_SECOND:
	  if (r<0 || r>59) 
             return -1;
	  d->x[i] = r;
	  c = ' ';
	  break;
      }
      if (i<d->to)
         if (*s++!=c)
            return -1;
      if (c==' ')
         while (*s==' ')
            s++;
   }
   if (*s)
      return -1;
   if (date_check(d)<0)
      return -1;
   return 0;
}


/* ----------------------------------------
 * EXTEND
 */


static void time_imp(time_t *clock, struct date *d)
{
   struct tm *tm = localtime(clock);
   d->from = DATE_YEAR;
   d->to = DATE_SECOND;
   d->x[DATE_YEAR] = tm->tm_year;
   d->x[DATE_MONTH] = 1+tm->tm_mon;
   d->x[DATE_DAY] = tm->tm_mday;
   d->x[DATE_HOUR] = tm->tm_hour;
   d->x[DATE_MINUTE] = tm->tm_min;
   d->x[DATE_SECOND] = tm->tm_sec;
}

static void now_imp(struct date *d)
{
   time_t clock;
   time(&clock);
   time_imp(&clock, d);
}

static void date_extend(struct date *d, struct date *nd, int nfrom, int nto)
{
   if (nfrom<d->from) 
      now_imp(nd);
   nd->from = nfrom;
   nd->to = nto;
   for (int i=d->from; i<MAXDATE; i++) {
      if (i<= d->to)
         nd->x[i] = d->x[i];
      else
         nd->x[i] = 0;
   }
   if (d->to < DATE_DAY) {
      nd->x[DATE_DAY] = 1;
      if (d->to < DATE_MONTH)
         nd->x[DATE_MONTH] = 1;
   }
}




/* ----------------------------------------
 * Class DATE
 */

static const char *date_name(at *p)
{
   struct date *d = Mptr(p);
   strcpy(string_buffer,"::DATE:");
   strcat(string_buffer, ansidatenames[(int)(d->from)]);
   strcat(string_buffer,"-");
   strcat(string_buffer, ansidatenames[(int)(d->to)]);
   strcat(string_buffer,":(");
   str_date_imp(d, string_buffer+strlen(string_buffer));
   strcat(string_buffer,")");
   return mm_strdup(string_buffer);
}

static at *copy_date(struct date *d);

static void date_serialize(at **pp, int code)
{
   void *p = 0;
   
   if (code == SRZ_READ) {
      serialize_chars(&p,code,-1);
      *pp = copy_date(p);
      free(p);

   } else {
      p = Mptr(*pp);
      serialize_chars(&p, code, sizeof(struct date));
   }
}

static int date_compare(at *p, at *q, int order)
{
   struct date *d1 = Mptr(p);
   struct date *d2 = Mptr(q);
   if (d1->from != d2->from) {
      if (order)
         error(NIL, "cannot rank incompatible dates", NIL);
      return 1;
   }
   for (int i=d1->from; i<MAXDATE; i++) {
      int n1 = 0;
      int n2 = 0;
      if (i <= d1->to)
         n1 = d1->x[i];
      if (i <= d2->to)
         n2 = d2->x[i];
      if (n1 < n2)
         return -1;
      if (n1 > n2)
         return 1;
   }
   return 0;
}

static unsigned long date_hash(at *p)
{
   unsigned long x = 0x01020304;
   struct date *d = Mptr(p);
   x ^= d->from;
   for (int i=d->from; i<MAXDATE; i++) {
      int n = 0;
      if (i <= d->to)
         n = d->x[i];
      x = (x<<6) | ((x&0xfc000000)>>26);
      x ^= n;
   }
   return x;
}



/* ----------------------------------------
 * utilities
 */


static struct date *get_date(at *p)
{
   ifn (DATEP(p))
      error(NIL, "not a date", p);
   return Mptr(p);
}

static at *copy_date(struct date *d)
{
   if (date_check(d)<0)
      error(NIL, "invalid date", NIL);

   struct date *nd = mm_blob(sizeof(struct date));
   *nd = *d;
   return new_at(date_class, nd);
}


char *str_date(at *p, int *pfrom, int *pto)
{
   struct date *d = get_date(p);
   if (pfrom)
      *pfrom = d->from;
   if (pto)
      *pto = d->to;
   str_date_imp(d,0);
   return d->str;
}


at *new_date(char *s, int from, int to)
{
   struct date buf;
   if (new_date_imp(s, &buf, from, to) == -1)
      error(NIL, "Illegal date", make_string(s));
   return copy_date(&buf);
}
  
at *new_date_from_time(void *clock, int from, int to)
{
   struct date buf;
   time_imp(clock, &buf);
   buf.from = from;
   buf.to = to;
   return copy_date(&buf);
}



/* ----------------------------------------
 * DATE_TO_DAY
 */

static void date_to_n(struct date *dt, int *ndp, int *nsp)
{
   struct date buf;
   static int month[12] = {
      0,
      31,
      31+29,
      31+29+31,
      31+29+31+30,
      31+29+31+30+31,
      31+29+31+30+31+30,
      31+29+31+30+31+30+31,
      31+29+31+30+31+30+31+31,
      31+29+31+30+31+30+31+31+30,
      31+29+31+30+31+30+31+31+30+31,
      31+29+31+30+31+30+31+31+30+31+30
   };

   date_extend(dt,&buf,DATE_YEAR,DATE_SECOND);
  
   if (ndp) {
      int y = buf.x[DATE_YEAR];
      int m = buf.x[DATE_MONTH];
      int d = buf.x[DATE_DAY];
      int leap = (y+3)/4 - (y+99)/100 + (y+299)/400;
      *ndp = y*365 + leap - 25567 + month[m-1] + d-1;
      if ((m>2) && !((y%4==0) && ((y%100!=0) || (y%400==100))))
         *ndp -= 1;
   }
   if (nsp) {
      int h = buf.x[DATE_HOUR];
      int m = buf.x[DATE_MINUTE];
      int s = buf.x[DATE_SECOND];
      *nsp = s+60*(m+60*h);
   }
}


static int date_to_yday(struct date *d)
{
   int nd1,nd2;
   struct date buf;
   buf = *d;
   date_to_n(&buf,&nd1,0);
   buf.to = DATE_YEAR;
   date_to_n(&buf,&nd2,0);
   return nd1 - nd2;
}

static int date_to_wday(struct date *d)
{
   int r;
   date_to_n(d,&r,0);
   return (r+4)%7;
}

static real date_to_day(struct date *d)
{
   int r,z;
   struct date buf;
   date_extend(d,&buf,DATE_YEAR,DATE_SECOND);
   date_to_n(&buf,&r,&z);
   return (double)r + (double)z/(60*60*24);
}


/* ----------------------------------------
 * DAY_TO_DATE
 */

static void n_to_date(int nd, int ns, int from, int to, struct date *d)
{
   static char mlen[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
  
   int h = ns/3600/24;
   nd += h;
   ns -= h*3600*24;
   nd = nd+25567;
   if (nd<0)
      error(NIL, "day number falls before 1900", NEW_NUMBER(nd));
   int ly, y = nd/366;
   nd = nd - (365*y + (y+3)/4 - (y+99)/100 + (y+299)/400);
   for(;;) {
      ly = (((y%4==0) && ((y%100!=0) || (y%400==100))) ? 366 : 365);
      if (nd<ly) break;
      y += 1;
      nd -= ly;
   }
   if (y>255)
      error(NIL,"day number falls after year 2155", NEW_NUMBER(nd));
   
   int m = 1;
   for(; m<=12; m++) {
      if (m!=2)
         h = mlen[m-1];
      else if (ly==366)
         h = 29;
      else
         h = 28;
      if (nd<h)
         break;
      nd = nd - h;
   }
   d->x[DATE_YEAR] = y;
   d->x[DATE_MONTH] = m;
   d->x[DATE_DAY] = 1+nd;

   if (ns<0 || ns>=60*60*24)
      error(NIL, "second number is illegal", NEW_NUMBER(ns));
   
   d->x[DATE_HOUR] = h = ns/3600;
   d->x[DATE_MINUTE] = m = (ns-3600*h)/60;
   d->x[DATE_SECOND] = ns-60*(m+60*h);
   d->from = from;
   d->to = to;
}


static void day_to_date(double day, struct date *d)
{
   int nd = (int)floor(day);
   int ns = (int)(3600*24*(day-nd));
   n_to_date(nd,ns,DATE_YEAR,DATE_SECOND,d);
}


/* ----------------------------------------
 * Elementary DX
 */


DX(xdate_to_day)
{
   ARG_NUMBER(1);
   return NEW_NUMBER(date_to_day(get_date(APOINTER(1))));
}

DX(xday_to_date)
{
   struct date buf;
   buf.from = DATE_YEAR;
   buf.to = DATE_SECOND;
   ARG_NUMBER(1);
   day_to_date(AREAL(1),&buf);
   return copy_date(&buf);
}


DX(xnow)
{
   struct date buf;
   now_imp(&buf);
   return copy_date(&buf);
}

DX(xtoday)
{
   struct date buf;
   now_imp(&buf);
   buf.to = DATE_DAY;
   return copy_date(&buf);
}








/* ----------------------------------------
 * DX for date<->itemnumber conversion
 */


DX(xsplit_date)
{
   at *p = NIL;
   at **pp = &p;
   
   ARG_NUMBER(1);
   struct date *d = get_date(APOINTER(1));
   for (int i=d->from; i<=d->to; i++) {
      *pp = new_cons(new_cons(named(ansidatenames[i]),NEW_NUMBER(d->x[i])),NIL);
      pp = &Cdr(*pp);
   }

   if (d->from<=DATE_YEAR && d->to>=DATE_DAY) {
      int wday = date_to_wday(d);
      int yday = date_to_yday(d);
      *pp = new_cons(new_cons(named("wday"), NEW_NUMBER(wday)),NIL);
      pp = &Cdr(*pp);
      *pp = new_cons(new_cons(named("yday"), NEW_NUMBER(yday)),NIL);
      pp = &Cdr(*pp);
   }
   return p;
}


/* ----------------------------------------
 * DX for date extension 
 */


DX(xdate_type)
{
   ARG_NUMBER(1);
   struct date *d = get_date(APOINTER(1));
   return new_cons(named(ansidatenames[(int)(d->from)]),
                   new_cons(named(ansidatenames[(int)(d->to)]),
                            NIL));
}


DX(xdate_extend)
{
   ARG_NUMBER(3);
   ASYMBOL(2);
   ASYMBOL(3);

   struct date *d = get_date(APOINTER(1));
   int from = -1;
   int to = -1;
   for (int i=DATE_YEAR; i<=DATE_SECOND; i++) {
      if (!strcmp(ansidatenames[i], NAMEOF(APOINTER(2))))
         from = i;
      if (!strcmp(ansidatenames[i], NAMEOF(APOINTER(3))))
         to = i;
   }
   if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to )
      error(NIL,"illegal date range specification",NIL);
   
   struct date buf;
   date_extend(d,&buf,from,to);
   return copy_date(&buf);
}


/* ----------------------------------------
 * DX for date<->string conversion
 */


#ifndef HAVE_STRFTIME
static char *strftime(char *buffer, int buflen, char *fmt, struct tm *tm)
{
   /* We may have to write this some day*/
   error(NIL,"Function strftime is no implemented",NIL);
}
#endif

DX(xdate_to_string)
{
   static struct tm tm;

   if (arg_number==1) {
      return make_string(str_date(APOINTER(1),NULL,NULL));

   } else {
      ARG_NUMBER(2);
      struct date buf, *d = get_date(APOINTER(1));
      const char *format = ASTRING(2);
      date_extend(d,&buf,DATE_YEAR,DATE_SECOND);
      tm.tm_year = buf.x[DATE_YEAR];
      tm.tm_mon = buf.x[DATE_MONTH] - 1;
      tm.tm_mday = buf.x[DATE_DAY];
      tm.tm_hour = buf.x[DATE_HOUR];
      tm.tm_min = buf.x[DATE_MINUTE];
      tm.tm_sec = buf.x[DATE_SECOND];

      int status = strftime(string_buffer,STRING_BUFFER,format,&tm);
      if (!status)
         RAISEFX("string too long",NIL);
      return make_string(string_buffer);
   }
}


DX(xstring_to_date)
{
   int from = DATE_YEAR;
   int to = DATE_SECOND;

   if (arg_number!=1) {
      ARG_NUMBER(3);
      ASYMBOL(2);
      ASYMBOL(3);
      for (int i=DATE_YEAR; i<=DATE_SECOND; i++) {
         if (!strcmp(ansidatenames[i], NAMEOF(APOINTER(2))))
            from = i;
         if (!strcmp(ansidatenames[i], NAMEOF(APOINTER(3))))
            to = i;
      }
   }
   if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to )
      RAISEFX("illegal date range specification", NIL);

   struct date buf;
   if (new_date_imp(ASTRING(1),&buf,from,to) < 0)
      return NIL;
   return copy_date(&buf);
}


/* ----------------------------------------
 * Add month or year
 */

DX(xdate_add_second)
{
   ARG_NUMBER(2);
   struct date buf, *d = get_date(APOINTER(1));
   int nd, ns, add = AINTEGER(2);
   date_to_n(d,&nd,&ns);
   n_to_date(nd,ns+add,d->from,d->to,&buf);
   return copy_date(&buf);
}

DX(xdate_add_minute)
{
   ARG_NUMBER(2);
   struct date buf, *d = get_date(APOINTER(1));
   int nd, ns, add = AINTEGER(2);
   date_to_n(d,&nd,&ns);
   n_to_date(nd,ns+60*add,d->from,d->to,&buf);
   return copy_date(&buf);
}

DX(xdate_add_hour)
{
   ARG_NUMBER(2);
   struct date buf, *d = get_date(APOINTER(1));
   int nd, ns, add = AINTEGER(2);
   date_to_n(d,&nd,&ns);
   n_to_date(nd,ns+3600*add,d->from,d->to,&buf);
   return copy_date(&buf);
}


DX(xdate_add_day)
{
   ARG_NUMBER(2);
   struct date buf, *d = get_date(APOINTER(1));
   int nd, ns, add = AINTEGER(2);
   date_to_n(d,&nd,&ns);
   n_to_date(nd+add,ns,d->from,d->to,&buf);
   return copy_date(&buf);
}



DX(xdate_add_month)
{
   uchar force_last_day_of_month = 0;
   uchar loosely = 0;
   uchar noerror = 0;
   
   static unsigned char mlen[] = 
      {31,28,31,30,31,30,31,31,30,31,30,31};

   at *opt;
   switch (arg_number) {
   case 2:
      opt = NIL;
      break;
   default:
      ARG_NUMBER (3);
      opt = APOINTER(3);
   }

   struct date buf, *d = get_date(APOINTER(1));
   buf = *d;
   int add = AINTEGER(2);
   if (opt) {
      const char *s = pname(opt);
      if (!strcmp(s, "noerror")) 
         noerror = 1; 
      else if (!strcmp(s, "no-error")) 
         noerror = 1; 
      else if (!strcmp(s, "end-of-month")) 
         force_last_day_of_month = 1;
      else 
         loosely = 1; 
   }

   int m = 0, y = 0;
   if (d->to >= DATE_MONTH) {
      if (d->from == DATE_MONTH) {
         m = buf.x[DATE_MONTH] - 1 + add;
         m = (m % 12) + 1;
         if (m < 1) 
            m += 12;
         buf.x[DATE_MONTH] = m;

      } else {
         y = buf.x[DATE_YEAR];
         m = 12*y + buf.x[DATE_MONTH] - 1 + add;
         if (m<0 || m>=256*12)
            error(NIL,"date out of range",NIL);
         y = m/12;
         m = m - (12*y) + 1;
         buf.x[DATE_MONTH] = m;
         buf.x[DATE_YEAR] = y;
      }
      if (d->to >= DATE_DAY) {
         int day = buf.x[DATE_DAY];
         int last_day_of_month = mlen[m-1];
         if ((m==2) && (d->from <= DATE_YEAR))
            if ((y%4==0)&&((y%100!=0)||(y%400==100)))
               last_day_of_month += 1;
         if (force_last_day_of_month)
            buf.x[DATE_DAY] = last_day_of_month;
         if (loosely && day>last_day_of_month)
            buf.x[DATE_DAY] = last_day_of_month;
         if (noerror && day>last_day_of_month)
            return NIL;
      }
   }
   return copy_date(&buf);
}

DX(xdate_add_year)
{
   uchar loosely = 0;
   uchar noerror = 0;
   
   at *opt;
   switch (arg_number) {
   case 2:
      opt = NIL;
      break;
   default:
      ARG_NUMBER (3);
      opt = APOINTER(3);
   }

   struct date buf, *d = get_date(APOINTER(1));
   buf = *d;
   int y, add = AINTEGER(2); 
   if (opt) {
      const char *s = pname(opt);
      if (!strcmp(s,"noerror")) 
         noerror = 1; 
      else if (!strcmp(s,"no-error")) 
         noerror = 1; 
      else 
         loosely = 1; 
   }

   if (d->from<=DATE_YEAR) {
      y = buf.x[DATE_YEAR] + add;
      if (y<0 || y>=256)
         error(NIL,"date out of range",NIL);
      buf.x[DATE_YEAR] = y;
      
      if (d->to >= DATE_DAY) {
         if (buf.x[DATE_MONTH]==2 && buf.x[DATE_DAY]==29)
            if (!((y%4==0)&&((y%100!=0)||(y%400==100)))) {
               if (noerror)
                  return NIL;
               else if (loosely)
                  buf.x[DATE_DAY] = 28;
            } 
      }
   }
   return copy_date(&buf);
}


/* ----------------------------------------
 * Date to cycle
 */



static void makecycle(double after, double before, double here, float *m)
{
   double ratio = (here-before)/(after-before);
   ratio *= 6.283185308;
   m[0] = (float)sin(ratio);
   m[1] = (float)cos(ratio);
}

#define FLAGLINEAR 	1
#define FLAGYEAR	2
#define FLAGMONTH	4
#define FLAGWEEK	8
#define FLAGDAY		16

int tl_date_code(at *dt, int flag, float *m)
{
   
   struct date buf, *d = get_date(dt);
   int nd, ns;
   date_to_n(d,&nd,&ns);
   double now = (double)nd + (double)ns/86400;
   int wday = (nd+4)%7;
   int count = 0;
   
   if (flag & FLAGLINEAR) {
      m[count] = (float)now;
      count++;
   }
  
   double after, before;
   if (flag & FLAGDAY) {
      if  ((d->from <= DATE_HOUR) && (d->to>=DATE_HOUR)) {
         before = 0;
         after = 86400;
         makecycle(after, before, (double)ns, &m[count]);
         count += 2;
      } else {
         m[count++] = NAN;
         m[count++] = NAN;
      }
   }
  
   if (flag & FLAGWEEK) {
      if ((d->from<=DATE_YEAR) && (d->to>=DATE_DAY)) {
         before = 0;
         after = 86400*7;
         makecycle(after,before, (double)(ns+86400*wday),&m[count]);
         count += 2;
      } else {
         m[count++] = NAN;
         m[count++] = NAN;
      }
   }
  
   buf = *d;
   if (buf.to > DATE_MONTH) 
      buf.to = DATE_MONTH;
   
   if (flag&FLAGMONTH) {
      if ((d->from<=DATE_DAY) && (d->to>=DATE_DAY)) {
         before = date_to_day(&buf);
         buf.x[DATE_MONTH] += 1;
         if (buf.x[DATE_MONTH]<=12) {
	    after = date_to_day(&buf);
         } else {
	    buf.x[DATE_MONTH] -= 12;
	    buf.x[DATE_YEAR] += 1;
	    after = date_to_day(&buf);
	    buf.x[DATE_YEAR] -= 1;
         }
         makecycle(after,before,now,&m[count]);
         count += 2;
      } else {
         m[count++] = NAN;
         m[count++] = NAN;
      }
   }
   
   if (buf.to > DATE_YEAR) 
      buf.to = DATE_YEAR;
   
   if (flag & FLAGYEAR) {
      if ((d->from<=DATE_MONTH) && (d->to>=DATE_MONTH)) {
         before = date_to_day(&buf);
         buf.x[DATE_YEAR] += 1;
         after = date_to_day(&buf);
         makecycle(after,before,now,&m[count]);
         count += 2;
      } else {
         m[count++] = NAN;
         m[count++] = NAN;
      }
   }
   return count;
}


DX(xdate_code)
{
   ARG_NUMBER(2);
   float m[10];
   int count = tl_date_code(APOINTER(1),AINTEGER(2),m);
   at *p = NIL;
   at **pp = &p;
   for (int i=0; i<count; i++) {
      *pp = new_cons(NEW_NUMBER(m[i]),NIL);
      pp = &Cdr(*pp);
   }
   return p;
}


/* --------- INITIALISATION CODE --------- */


class_t *date_class;

void init_date(void)
{
   /* setting up date_class */
   date_class = new_builtin_class(NIL);
   date_class->name = date_name;
   date_class->serialize = date_serialize;
   date_class->compare = date_compare;
   date_class->hash = date_hash;
   class_define("Date", date_class);
  
   dx_define("date-to-day",xdate_to_day);
   dx_define("day-to-date",xday_to_date);
   dx_define("today",xtoday);
   dx_define("now",xnow);
   dx_define("split-date",xsplit_date);
   dx_define("date-type",xdate_type); 
   dx_define("date-extend",xdate_extend);
   dx_define("date-to-string",xdate_to_string);
   dx_define("string-to-date",xstring_to_date);
   dx_define("date-add-second",xdate_add_second);
   dx_define("date-add-minute",xdate_add_minute);
   dx_define("date-add-hour",xdate_add_hour);
   dx_define("date-add-day",xdate_add_day);
   dx_define("date-add-month",xdate_add_month);
   dx_define("date-add-year",xdate_add_year);
   dx_define("date-code",xdate_code);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
