/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/

#include "header.h"
#include "graphics.h"

/* ===================================================================== */
/* LISP DRIVER */
/* ===================================================================== */

static at *at_begin;
static at *at_end;
static at *at_xsize;
static at *at_ysize;
static at *at_setfont;
static at *at_clear;
static at *at_draw_line;
static at *at_draw_rect;
static at *at_draw_circle;
static at *at_fill_rect;
static at *at_fill_circle;
static at *at_draw_text;
static at *at_rect_text;
static at *at_setcolor;
static at *at_alloccolor;
static at *at_fill_polygon;
static at *at_gspecial;
static at *at_clip;
static at *at_pixel_map;
static at *at_hinton_map;
static  at *get_image;




/* Graphics calls are always enclosed between one or more
   Begin/End pair. */

void lisp_begin(info)
struct window *info;
{
  at *p;
  p = send_sgi(info->backptr,at_begin,NIL);
  UNLOCK(p);
}

void lisp_end(info)
struct window *info;
{
  at *p;
  p = send_sgi(info->backptr,at_end,NIL);
  UNLOCK(p);
}


/* Close a window */

void lisp_close(info)
struct window *info;
{
  free(info);
}


/* Return the size of a window */

int lisp_xsize(info)
struct window *info;
{
  at *p;
  p = send_sgi(info->backptr,at_xsize, NIL);
  if (p && p->ctype==XT_NUMBER)
    return (int)(p->Number);
  error(NIL,"method xsize returned",p);
}


int lisp_ysize(info)
struct window *info;
{
  at *p;
  p = send_sgi(info->backptr,at_ysize, NIL);
  if (p && p->ctype==XT_NUMBER)
    return (int)(p->Number);
  error(NIL,"method ysize returned",p);
}


/* set the font in a window */

void lisp_setfont(info,f)
struct window *info;
char *f;
{
  at *p,*q;
  q = cons(new_string(f),NIL);
  p = send_sgi(info->backptr,at_setfont,q);
  UNLOCK(q);
  UNLOCK(p);
}


/* Drawing functions */

void lisp_draw_line(info,x1,y1,x2,y2)
struct window *info;
short x1,y1;
unsigned short x2,y2;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x1),
	   cons(NEW_NUMBER(y1),
		cons(NEW_NUMBER(x2),
		     cons(NEW_NUMBER(y2),NIL))));
  p = send_sgi(info->backptr,at_draw_line,q);
  UNLOCK(q);
  UNLOCK(p);
}

void lisp_draw_rect(info,x1,y1,x2,y2)
struct window *info;
short x1,y1;
unsigned short x2,y2;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x1),
	   cons(NEW_NUMBER(y1),
		cons(NEW_NUMBER(x2),
		     cons(NEW_NUMBER(y2),NIL))));
  p = send_sgi(info->backptr,at_draw_rect,q);
  UNLOCK(q);
  UNLOCK(p);
}

void lisp_draw_circle(info,x1,y1,r)
struct window *info;
short x1,y1;
unsigned short r;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x1),
	   cons(NEW_NUMBER(y1),
		cons(NEW_NUMBER(r),NIL)));
  p = send_sgi(info->backptr,at_draw_circle,q);
  UNLOCK(q);
  UNLOCK(p);
}

void lisp_fill_rect(info,x1,y1,x2,y2)
struct window *info;
short x1,y1;
unsigned short x2,y2;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x1),
	   cons(NEW_NUMBER(y1),
		cons(NEW_NUMBER(x2),
		     cons(NEW_NUMBER(y2),NIL))));
  p = send_sgi(info->backptr,at_fill_rect,q);
  UNLOCK(q);
  UNLOCK(p);
}

void lisp_fill_circle(info,x1,y1,r)
struct window *info;
short x1,y1;
unsigned short r;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x1),
	   cons(NEW_NUMBER(y1),
		cons(NEW_NUMBER(r),NIL)));
  p = send_sgi(info->backptr,at_fill_circle,q);
  UNLOCK(q);
  UNLOCK(p);
}


void lisp_clear(info)
struct window *info;
{
  at *p;
  p = send_sgi(info->backptr,at_clear,NIL);
  UNLOCK(p);
}

void lisp_draw_text(info,x,y,s)
struct window *info;
short x,y;
char *s;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x),
	   cons(NEW_NUMBER(y),
		cons(new_string(s),NIL)));
  p = send_sgi(info->backptr,at_draw_text,q);
  UNLOCK(q);
  UNLOCK(p);
}



/***** release 2 ****/


void lisp_fill_polygon(info,points,n)
struct window *info;
short (*points)[2];
unsigned int n;
{
  int i;
  at *p,*q,**w;
  q = NIL;
  w = &q;
  for (i=0;i<n;i++) {
    *w = cons(NEW_NUMBER(points[i][0]),
	      cons(NEW_NUMBER(points[i][1]), NIL) );
    w = &((*w)->Cdr->Cdr);
  }
  p = send_sgi(info->backptr,at_fill_polygon,q);
  UNLOCK(q);
  UNLOCK(p);
}

void 
lisp_rect_text(info, x, y, s, xptr, yptr, wptr, hptr)
struct window *info;
short x, y;
int *xptr, *yptr, *wptr, *hptr;
char *s;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x),
	   cons(NEW_NUMBER(y),
		cons(new_string(s),NIL)));
  p = send_sgi(info->backptr,at_rect_text,q);

  ifn (CONSP(p) && p->Car && p->Car->ctype==XT_NUMBER)
    goto lerror;
  *xptr = (int)(p->Car->Number);
  p = p->Cdr;
  ifn (CONSP(p) && p->Car && p->Car->ctype==XT_NUMBER)
    goto lerror;
  *yptr = (int)(p->Car->Number);
  p = p->Cdr;
  ifn (CONSP(p) && p->Car && p->Car->ctype==XT_NUMBER)
    goto lerror;
  *wptr = (int)(p->Car->Number);
  p = p->Cdr;
  ifn (CONSP(p) && p->Car && p->Car->ctype==XT_NUMBER)
    goto lerror;
  *hptr = (int)(p->Car->Number);
  p = p->Cdr;
  ifn (p)
    return;
 lerror:
  error(NIL,"method rect_text returned",p);
}


void lisp_setcolor(info,x)
struct window *info;
long x;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x),NIL);
  p = send_sgi(info->backptr,at_setcolor,q);
  UNLOCK(q);
  UNLOCK(p);
}


int lisp_alloccolor(info,r,g,b)
struct window *info;
real r,g,b;
{
  at *p,*q;
  q = cons(NEW_NUMBER(r),
	   cons(NEW_NUMBER(g),
		cons(NEW_NUMBER(b),NIL)));
  p = send_sgi(info->backptr,at_alloccolor,q);
  UNLOCK(q);
  if (p && p->ctype==XT_NUMBER)
    return (int)(p->Number);
  error(NIL,"Method alloccolor returned",p);
}


void lisp_gspecial(info,s)
struct window *info;
char *s;
{
  at *p,*q;
  q = cons(new_string(s),NIL);
  p = send_sgi(info->backptr,at_gspecial,q);
  UNLOCK(q);
  UNLOCK(p);
}


void lisp_clip(info,x,y,w,h)
struct window *info;
short x,y;
unsigned short w,h;
{
  at *p,*q;
  q = cons(NEW_NUMBER(x),
	   cons(NEW_NUMBER(y),
		cons(NEW_NUMBER(w),
		     cons(NEW_NUMBER(h),NIL))));
  p = send_sgi(info->backptr,at_clip,q);
  UNLOCK(q);
  UNLOCK(p);
}



int lisp_pixel_map(info,data,x,y,w,h,sx,sy)
struct window *info;
int  *data;
short x,y;
unsigned short w,h,sx,sy;
{
  at *p,*q,*m;

  ifn (data){
    p = checksend(info->backptr->Class, at_pixel_map);
    if (!p)
      return FALSE;
    UNLOCK(p);
    return TRUE;
  }  

  /* vvvvvvvv DO NOT IMITATE THIS CLUMSY PRE-IDX.H CODE vvvvvvvv */
  {
    long *data2;
    int i,dm[2];
    /* Replace this code by a virtual pointer manipulation */
    dm[0]=h;
    dm[1]=w;
    m = I32_matrix(2,dm);
    data2 = ((struct index*)(m->Object))->st->srg.data;
    for(i=0;i<w*h;i++)
      *data2++ = *data++;
  }
  /* ^^^^^^^^ DO NOT IMITATE THIS CLUMSY PRE-IDX.H CODE ^^^^^^^^ */


  q = cons(NEW_NUMBER(x),
	   cons(NEW_NUMBER(y),
		cons(m,
		     cons(NEW_NUMBER(sx),
			  cons(NEW_NUMBER(sy), NIL ) ) ) ) );
  p = send_sgi(info->backptr,at_pixel_map,q);
  UNLOCK(p);
  UNLOCK(q);
}

int lisp_hinton_map(info,data,x,y,ncol,nlin,len,apart)
struct window *info;
int  *data;
short x,y;
unsigned short ncol,nlin,len,apart;
{
  at *p,*q,*m;
  
  ifn (data) {
    p = checksend(info->backptr->Class, at_hinton_map);
    if (!p)
      return FALSE;
    UNLOCK(p);
    return TRUE;
  }  
  
  /* vvvvvvvv DO NOT IMITATE THIS CLUMSY PRE-IDX.H CODE vvvvvvvv */
  {
    long *data2;
    int i,dm[2];
    /* Replace this code by a virtual pointer manipulation */
    dm[0]=nlin;
    dm[1]=ncol;
    m = I32_matrix(2,dm);
    data2 = ((struct index*)(m->Object))->st->srg.data;
    for(i=0;i<len;i++)
      *data2++ = *data++;
  }
  /* ^^^^^^^^ DO NOT IMITATE THIS CLUMSY PRE-IDX.H CODE ^^^^^^^^ */
  
  q = cons(NEW_NUMBER(x),
	   cons(NEW_NUMBER(y),
		cons(m,
		     cons(NEW_NUMBER(len),
			  cons(NEW_NUMBER(apart), NIL ) ) ) ) );
  p = send_sgi(info->backptr,at_hinton_map,q);
  UNLOCK(p);
  UNLOCK(q);
}




struct gdriver lisp_driver = {
    "LISP",
    lisp_begin,
    lisp_end,
    lisp_close,
    lisp_xsize,
    lisp_ysize,
    NIL, 
    lisp_setfont,
    lisp_clear,
    lisp_draw_line,
    lisp_draw_rect,
    lisp_draw_circle,
    lisp_fill_rect,
    lisp_fill_circle,
    lisp_draw_text,
    lisp_setcolor,
    lisp_alloccolor,
    lisp_fill_polygon,
    lisp_rect_text,
    lisp_gspecial,
    lisp_clip,
    NIL,
    NIL,
    NIL,
    lisp_pixel_map,
    lisp_hinton_map,
    NIL,
};



/* ===================================================================== */
/* CREATION CODE */
/* ===================================================================== */


static at 
  *lisp_window()
{
  extern struct gdriver lisp_driver;
  struct window *win;
  at *ans;

  win = calloc(sizeof(struct window),1);
  win->used = -1;
  win->gdriver = &lisp_driver;
  win->eventhandler = NIL;
  win->color = COLOR_FG;
  win->clipw = win->cliph = 0;
  win->font = new_safe_string("default");
  ans = new_extern(&window_class,win,XT_WINDOW);
  win->backptr = ans;
  return ans;
}
DX(xlisp_window)
{
  ARG_NUMBER(0);
  return lisp_window();
}



/* ===================================================================== */
/* INITIALISATION CODE */
/* ===================================================================== */


void 
init_lisp_driver()
{
  dx_define("lisp_window",xlisp_window);

  /* LISP DRIVER MESSAGES */
  at_begin = var_define("begin");
  at_end = var_define("end");
  at_xsize = var_define("xsize");
  at_ysize = var_define("ysize");
  at_setfont = var_define("font");
  at_clear = var_define("cls");
  at_draw_line = var_define("draw_line");
  at_draw_rect = var_define("draw_rect");
  at_draw_circle = var_define("draw_circle");
  at_fill_rect = var_define("fill_rect");
  at_fill_circle = var_define("fill_circle");
  at_draw_text = var_define("draw_text");
  at_rect_text = var_define("rect_text");
  at_setcolor = var_define("color");
  at_alloccolor = var_define("alloccolor");
  at_fill_polygon = var_define("fill_polygon");
  at_gspecial = var_define("gspecial");
  at_clip = var_define("clip");
  at_pixel_map = var_define("pixel_map");
  at_hinton_map = var_define("hinton_map");
}



