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
 * $Id: date.c,v 1.2 2002-04-24 21:12:57 leonb Exp $
 **********************************************************************/

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


static int
date_check(struct date *dt)
{
  int y,m,d;
  static char mlen[] = {31,29,31,30,31,30,31,31,30,31,30,31};
  m = 1;
  if (dt->from<=DATE_DAY && dt->to>=DATE_DAY)
    {
      d = dt->x[DATE_DAY];
      if (dt->from<=DATE_MONTH) 
	{
	  m = dt->x[DATE_MONTH];
	  if (d>mlen[m-1])
	    return -1;
	}
      if (dt->from<=DATE_YEAR) 
	{
	  y = dt->x[DATE_YEAR];
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

static int 
str_date_imp(struct date *d, char *s)
{
  char buffer[8];
  char *r;
  int i;
  if( ! s )
    s = d->str; /* inside buffer */
  for (i=d->from; i<=d->to; i++)
    {
      switch(i)
	{
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
      r = buffer;
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


static int
new_date_imp(char *s, struct date *d, int from, int to)
{
  int i,r;
  char c = 0;
  if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to)
    error(NIL,"illegal date range specification",NIL);
  d->from = from;
  d->to = to;
  while (*s==' ')
    s++;
  for (i=from; i<=to; i++)
    {
      if (*s<'0' || *s>'9') 
	return -1;
      r = 0;
      while (*s>='0' && *s<='9')
	r = r*10 + (*s++) - '0';
      switch(i)
	{
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


static void
now_imp(struct date *d)
{
  time_t clock;
  struct tm *tm;
  time(&clock);
  tm = localtime(&clock);
  d->from = DATE_YEAR;
  d->to = DATE_SECOND;
  d->x[DATE_YEAR] = tm->tm_year;
  d->x[DATE_MONTH] = 1+tm->tm_mon;
  d->x[DATE_DAY] = tm->tm_mday;
  d->x[DATE_HOUR] = tm->tm_hour;
  d->x[DATE_MINUTE] = tm->tm_min;
  d->x[DATE_SECOND] = tm->tm_sec;
}


static void
date_extend(struct date *d, struct date *nd, int nfrom, int nto)
{
  int i;  
  if (nfrom<d->from) 
    now_imp(nd);
  nd->from = nfrom;
  nd->to = nto;
  for (i=d->from; i<MAXDATE; i++)
    {
      if (i<= d->to)
	nd->x[i] = d->x[i];
      else
	nd->x[i] = 0;
    }
  if (d->to < DATE_DAY)
    {
      nd->x[DATE_DAY] = 1;
      if (d->to < DATE_MONTH)
	nd->x[DATE_MONTH] = 1;
    }
}




/* ----------------------------------------
 * Class DATE
 */



static struct alloc_root date_alloc = {
  NULL,
  NULL,
  sizeof(struct date),
  100 
  };

static void
date_dispose(at *p)
{
  deallocate(&date_alloc,p->Object);
}


static char *
date_name(at *p)
{
  struct date *d = p->Object;
  strcpy(string_buffer,"::DATE:");
  strcat(string_buffer, ansidatenames[(int)(d->from)]);
  strcat(string_buffer,"-");
  strcat(string_buffer, ansidatenames[(int)(d->to)]);
  strcat(string_buffer,":(");
  str_date_imp(d, string_buffer+strlen(string_buffer));
  strcat(string_buffer,")");
  return string_buffer;
}

static void
date_serialize(at **pp, int code)
{
  static at *make_date(struct date *d);
  struct date *d;

  if (code == SRZ_READ)
  {
      serialize_chars((void**)&d,code,-1);
      *pp = make_date(d);
  }
  else
  {
      d = (*pp)->Object;
      serialize_chars((void**)&d,code,sizeof(struct date));
  }
}

static int 
date_compare(at *p, at *q, int order)
{
  int i,n1,n2;
  struct date *d1 = p->Object;
  struct date *d2 = q->Object;
  if (d1->from != d2->from) {
    if (order)
      error(NIL,"Cannot rank incompatible dates",NIL);
    return 1;
  }
  for (i=d1->from; i<MAXDATE; i++)
  {
    n1 = n2 = 0;
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

static unsigned long
date_hash(at *p)
{
  int i, n;
  unsigned long x = 0x01020304;
  struct date *d = p->Object;
  x ^= d->from;
  for (i=d->from; i<MAXDATE; i++)
  {
    n = 0;
    if (i <= d->to)
      n = d->x[i];
    x = (x<<6) | ((x&0xfc000000)>>26);
    x ^= n;
  }
  return x;
}


class date_class = {
  date_dispose,
  generic_action,
  date_name,
  generic_eval,
  generic_listeval,
  date_serialize,
  date_compare,
  date_hash
};



/* ----------------------------------------
 * utilities
 */


static struct date *
get_date(at *p)
{
  ifn (EXTERNP(p,&date_class))
    error(NIL,"Not a date",p);
  return p->Object;
}

static at *
make_date(struct date *d)
{
  struct date *nd;
  if (date_check(d)<0)
    error(NIL,"Illegal date",NIL);
  nd = allocate(&date_alloc);
  *nd = *d;
  return new_extern(&date_class,nd);
}


char *
str_date(at *p, int *pfrom, int *pto)
{
  struct date *d;
  d = get_date(p);
  if (pfrom)
    *pfrom = d->from;
  if (pto)
    *pto = d->to;
  str_date_imp(d,0);
  return d->str;
}


at *
new_date(char *s, int from, int to)
{
  struct date buf;
  if ( new_date_imp(s,&buf,from,to) == -1)
    error(NIL,"Illegal date",new_string(s));
  return make_date(&buf);
}
  


/* ----------------------------------------
 * DATE_TO_DAY
 */

static void
date_to_n(struct date *dt, int *ndp, int *nsp)
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
  
  if (ndp)
    {
      int y,m,d,leap;
      y = buf.x[DATE_YEAR];
      m = buf.x[DATE_MONTH];
      d = buf.x[DATE_DAY];
      leap = (y+3)/4 - (y+99)/100 + (y+299)/400;
      *ndp = y*365 + leap - 25567 + month[m-1] + d-1;
      if ((m>2) && !((y%4==0) && ((y%100!=0) || (y%400==100))))
	*ndp -= 1;
    }
  if (nsp)
    {
      int h,m,s;
      h = buf.x[DATE_HOUR];
      m = buf.x[DATE_MINUTE];
      s = buf.x[DATE_SECOND];
      *nsp = s+60*(m+60*h);
    }
}


static int
date_to_yday(struct date *d)
{
  int nd1,nd2;
  struct date buf;
  buf = *d;
  date_to_n(&buf,&nd1,0);
  buf.to = DATE_YEAR;
  date_to_n(&buf,&nd2,0);
  return nd1 - nd2;
}

static int
date_to_wday(struct date *d)
{
  int r;
  date_to_n(d,&r,0);
  return (r+4)%7;
}

static real
date_to_day(struct date *d)
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

static void 
n_to_date(int nd, int ns, int from, int to, struct date *d)
{
  int h,y,m,ly;
  static char mlen[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
  
  h = ns/3600/24;
  nd += h;
  ns -= h*3600*24;
  nd = nd+25567;
  if (nd<0)
    error(NIL,"Day number falls before 1900",NEW_NUMBER(nd));
  y = nd/366;
  nd = nd - (365*y + (y+3)/4 - (y+99)/100 + (y+299)/400);
  for(;;) {
    ly = (((y%4==0) && ((y%100!=0) || (y%400==100))) ? 366 : 365);
    if (nd<ly) break;
    y += 1;
    nd -= ly;
  }
  if (y>255)
    error(NIL,"Day number falls after year 2155",NEW_NUMBER(nd));

  for(m=1;m<=12;m++)
    {
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
    error(NIL,"Second number is illegal",NEW_NUMBER(ns));

  d->x[DATE_HOUR] = h = ns/3600;
  d->x[DATE_MINUTE] = m = (ns-3600*h)/60;
  d->x[DATE_SECOND] = ns-60*(m+60*h);
  d->from = from;
  d->to = to;
}


static void 
day_to_date(double day, struct date *d)
{
  int nd,ns;
  nd = (int)floor(day);
  ns = (int)(3600*24*(day-nd));
  n_to_date(nd,ns,DATE_YEAR,DATE_SECOND,d);
}


/* ----------------------------------------
 * Elementary DX
 */


DX(xdate_to_day)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(date_to_day(get_date(APOINTER(1))));
}

DX(xday_to_date)
{
  struct date buf;
  buf.from = DATE_YEAR;
  buf.to = DATE_SECOND;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  day_to_date(AREAL(1),&buf);
  return make_date(&buf);
}


DX(xnow)
{
  struct date buf;
  now_imp(&buf);
  return make_date(&buf);
}

DX(xtoday)
{
  struct date buf;
  now_imp(&buf);
  buf.to = DATE_DAY;
  return make_date(&buf);
}








/* ----------------------------------------
 * DX for date<->itemnumber conversion
 */


DX(xsplit_date)
{
  int i;
  struct date *d;
  at *p = NIL;
  at **pp = &p;

  ARG_NUMBER(1);
  ALL_ARGS_EVAL;

  d = get_date(APOINTER(1));
  for (i=d->from; i<=d->to; i++)
    {
      *pp = cons(cons(named(ansidatenames[i]),NEW_NUMBER(d->x[i])),NIL);
      pp = &((*pp)->Cdr);
    }

  if (d->from<=DATE_YEAR && d->to>=DATE_DAY)
    {
      int wday = date_to_wday(d);
      int yday = date_to_yday(d);
      *pp = cons(cons(named("wday"), NEW_NUMBER(wday)),NIL);
      pp = &((*pp)->Cdr);
      *pp = cons(cons(named("yday"), NEW_NUMBER(yday)),NIL);
      pp = &((*pp)->Cdr);
    }
  return p;
}


/* ----------------------------------------
 * DX for date extension 
 */


DX(xdate_type)
{
  struct date *d;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  d = get_date(APOINTER(1));
  return cons(named(ansidatenames[(int)(d->from)]),
	      cons(named(ansidatenames[(int)(d->to)]),
		   NIL));
}


DX(xdate_extend)
{
  struct date *d, buf;
  int from, to, i;
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  ASYMBOL(2);
  ASYMBOL(3);
  d = get_date(APOINTER(1));
  from = to = -1;
  for (i=DATE_YEAR; i<=DATE_SECOND; i++) {
    if (!strcmp(ansidatenames[i],nameof(APOINTER(2))))
      from = i;
    if (!strcmp(ansidatenames[i],nameof(APOINTER(3))))
      to = i;
  }
  if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to )
    error(NIL,"illegal date range specification",NIL);
  date_extend(d,&buf,from,to);
  return make_date(&buf);
}


/* ----------------------------------------
 * DX for date<->string conversion
 */


#ifndef HAVE_STRFTIME
static char *
strftime(char *buffer, int buflen, char *fmt, struct tm *tm)
{
  /* We may have to write this some day*/
  error(NIL,"Function strftime is no implemented",NIL);
}
#endif

DX(xdate_to_string)
{
  struct date *d, buf;
  static struct tm tm;
  int status;
  char *format;
  ALL_ARGS_EVAL;
  if (arg_number==1) {
    return new_string(str_date(APOINTER(1),NULL,NULL));
  } else {
    ARG_NUMBER(2);
    d = get_date(APOINTER(1));
    format = ASTRING(2);
    date_extend(d,&buf,DATE_YEAR,DATE_SECOND);
    tm.tm_year = buf.x[DATE_YEAR];
    tm.tm_mon = buf.x[DATE_MONTH] - 1;
    tm.tm_mday = buf.x[DATE_DAY];
    tm.tm_hour = buf.x[DATE_HOUR];
    tm.tm_min = buf.x[DATE_MINUTE];
    tm.tm_sec = buf.x[DATE_SECOND];
    status = strftime(string_buffer,STRING_BUFFER,format,&tm);
    if (status)
      return new_string(string_buffer);
    else
      error(NIL,"Too long string",NIL);
  }
}


DX(xstring_to_date)
{
  struct date buf;
  int from, to, i;
  
  ALL_ARGS_EVAL;
  from = DATE_YEAR;
  to = DATE_SECOND;
  if (arg_number!=1)
    {
      ARG_NUMBER(3);
      ASYMBOL(2);
      ASYMBOL(3);
      for (i=DATE_YEAR; i<=DATE_SECOND; i++) {
	if (!strcmp(ansidatenames[i],nameof(APOINTER(2))))
	  from = i;
	if (!strcmp(ansidatenames[i],nameof(APOINTER(3))))
	  to = i;
      }
    }
  if (from<0 || to<0 || from>DATE_SECOND || to>DATE_SECOND || from>to )
    error(NIL,"illegal date range specification",NIL);
  if (new_date_imp(ASTRING(1),&buf,from,to) < 0)
    return NIL;
  return make_date(&buf);
}


/* ----------------------------------------
 * Add month or year
 */

DX(xdate_add_second)
{
  struct date *d, buf;
  int nd,ns,add;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  d = get_date(APOINTER(1));
  add = AINTEGER(2);
  date_to_n(d,&nd,&ns);
  n_to_date(nd,ns+add,d->from,d->to,&buf);
  return make_date(&buf);
}

DX(xdate_add_minute)
{
  struct date *d, buf;
  int nd,ns,add;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  d = get_date(APOINTER(1));
  add = AINTEGER(2);
  date_to_n(d,&nd,&ns);
  n_to_date(nd,ns+60*add,d->from,d->to,&buf);
  return make_date(&buf);
}

DX(xdate_add_hour)
{
  struct date *d, buf;
  int nd,ns,add;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  d = get_date(APOINTER(1));
  add = AINTEGER(2);
  date_to_n(d,&nd,&ns);
  n_to_date(nd,ns+3600*add,d->from,d->to,&buf);
  return make_date(&buf);
}


DX(xdate_add_day)
{
  struct date *d, buf;
  int nd,ns,add;

  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  d = get_date(APOINTER(1));
  add = AINTEGER(2);
  date_to_n(d,&nd,&ns);
  n_to_date(nd+add,ns,d->from,d->to,&buf);
  return make_date(&buf);
}



DX(xdate_add_month)
{
  at *opt;
  struct date *d, buf;
  int add, m=0, y=0;
  unsigned char force_last_day_of_month = 0;
  unsigned char loosely = 0;
  unsigned char noerror = 0;

  static unsigned char mlen[] = 
    {31,28,31,30,31,30,31,31,30,31,30,31};

  ALL_ARGS_EVAL;
  switch (arg_number) 
  {
  case 2:
    opt = NIL;
    break;
  default:
    ARG_NUMBER (3);
    opt = APOINTER(3);
  }

  d = get_date(APOINTER(1));
  buf = *d;
  add = AINTEGER(2);
  if (opt) 
  {
    char *s = pname(opt);
    if (!strcmp(s,"noerror")) 
      noerror = 1; 
    else if (!strcmp(s,"no-error")) 
      noerror = 1; 
    else if (!strcmp(s,"end-of-month")) 
      force_last_day_of_month = 1;
    else 
      loosely = 1; 
  }
  
  if (d->to >= DATE_MONTH)
  {
    if (d->from == DATE_MONTH)
    {
      m = buf.x[DATE_MONTH] - 1 + add;
      m = (m % 12) + 1;
      if (m < 1) 
        m += 12;
      buf.x[DATE_MONTH] = m;
    }
    else
    {
      y = buf.x[DATE_YEAR];
      m = 12*y + buf.x[DATE_MONTH] - 1 + add;
      if (m<0 || m>=256*12)
	error(NIL,"date out of range",NIL);
      y = m/12;
      m = m - (12*y) + 1;
      buf.x[DATE_MONTH] = m;
      buf.x[DATE_YEAR] = y;
    }
    if (d->to >= DATE_DAY)
    {
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
  return make_date(&buf);
}

DX(xdate_add_year)
{
  at *opt;
  struct date *d, buf;
  int add, y;
  unsigned char loosely = 0;
  unsigned char noerror = 0;

  ALL_ARGS_EVAL;
  switch (arg_number) 
  {
  case 2:
    opt = NIL;
    break;
  default:
    ARG_NUMBER (3);
    opt = APOINTER(3);
  }

  d = get_date(APOINTER(1));
  buf = *d;
  add = AINTEGER(2); 
  if (opt) 
  {
    char *s = pname(opt);
    if (!strcmp(s,"noerror")) 
      noerror = 1; 
    else if (!strcmp(s,"no-error")) 
      noerror = 1; 
    else 
      loosely = 1; 
  }

  if (d->from<=DATE_YEAR)
  {
    y = buf.x[DATE_YEAR] + add;
    if (y<0 || y>=256)
      error(NIL,"date out of range",NIL);
    buf.x[DATE_YEAR] = y;

    if (d->to >= DATE_DAY)
    {
      if (buf.x[DATE_MONTH]==2 && buf.x[DATE_DAY]==29)
        if (!((y%4==0)&&((y%100!=0)||(y%400==100))))
        {
          if (noerror)
            return NIL;
          else if (loosely)
            buf.x[DATE_DAY] = 28;
        } 
    }
  }
  return make_date(&buf);
}


/* ----------------------------------------
 * Date to cycle
 */



static void
makecycle(double after, double before, double here, float *m)
{
  double ratio;
  double c,s;
  ratio = (here-before)/(after-before);
#ifdef HAVE_SINCOS
  sincos( 6.283185308*ratio, &s, &c );
#else
  ratio *= 6.283185308;
  s = sin(ratio);
  c = cos(ratio);
#endif
  m[0] = (float)c;
  m[1] = (float)s;
}

#define FLAGLINEAR 	1
#define FLAGYEAR	2
#define FLAGMONTH	4
#define FLAGWEEK	8
#define FLAGDAY		16

int 
tl_date_code(at *dt, int flag, float *m)
{
  int count, nd, ns, wday;
  struct date *d, buf;
  double now, after, before;
  
  d = get_date(dt);
  date_to_n(d,&nd,&ns);
  now = (double)nd + (double)ns/86400;
  wday = (nd+4)%7;
  count = 0;
  
  if (flag&FLAGLINEAR)
    {
      m[count] = (float)now;
      count++;
    }
  
  if (flag&FLAGDAY)
    {
      if  ((d->from <= DATE_HOUR) && (d->to>=DATE_HOUR))
	{
	  before = 0;
	  after = 86400;
	  makecycle(after, before, (double)ns, &m[count]);
	  count += 2;
	}
      else
	{
	  m[count++] = getnanF();
	  m[count++] = getnanF();
	}
    }
  
  if (flag&FLAGWEEK)
    {
      if ((d->from<=DATE_YEAR) && (d->to>=DATE_DAY))
	{
	  before = 0;
	  after = 86400*7;
	  makecycle(after,before, (double)(ns+86400*wday),&m[count]);
	  count += 2;
	}
      else
	{
	  m[count++] = getnanF();
	  m[count++] = getnanF();
	}
    }
  
  buf = *d;
  if (buf.to > DATE_MONTH) 
    buf.to = DATE_MONTH;
  
  if (flag&FLAGMONTH)
    {
      if ((d->from<=DATE_DAY) && (d->to>=DATE_DAY))
	{
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
	}
      else
	{
	  m[count++] = getnanF();
	  m[count++] = getnanF();
	}
    }
      
  if (buf.to > DATE_YEAR) 
    buf.to = DATE_YEAR;

  if (flag & FLAGYEAR)
    {
      if ((d->from<=DATE_MONTH) && (d->to>=DATE_MONTH))
	{
	  before = date_to_day(&buf);
	  buf.x[DATE_YEAR] += 1;
	  after = date_to_day(&buf);
	  makecycle(after,before,now,&m[count]);
	  count += 2;
	}
      else
	{
	  m[count++] = getnanF();
	  m[count++] = getnanF();
	}
    }
  return count;
}


DX(xdate_code)
{
  float m[10];
  int i,count;
  at *p = NIL;
  at **pp = &p;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  count = tl_date_code(APOINTER(1),AINTEGER(2),m);
  for (i=0; i<count; i++)
    {
      *pp = cons(NEW_NUMBER(m[i]),NIL);
      pp = &((*pp)->Cdr);
    }
  return p;
}


/* --------- INITIALISATION CODE --------- */



void init_date(void)
{
  class_define("DATE", &date_class);
  
  dx_define("date_to_day",xdate_to_day);
  dx_define("day_to_date",xday_to_date);
  dx_define("today",xtoday);
  dx_define("now",xnow);
  dx_define("split_date",xsplit_date);
  dx_define("date_type",xdate_type); 
  dx_define("date_extend",xdate_extend);
  dx_define("date_to_string",xdate_to_string);
  dx_define("string_to_date",xstring_to_date);
  dx_define("date_add_second",xdate_add_second);
  dx_define("date_add_minute",xdate_add_minute);
  dx_define("date_add_hour",xdate_add_hour);
  dx_define("date_add_day",xdate_add_day);
  dx_define("date_add_month",xdate_add_month);
  dx_define("date_add_year",xdate_add_year);
  dx_define("date_code",xdate_code);
}

