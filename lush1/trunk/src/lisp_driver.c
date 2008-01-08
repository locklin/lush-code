/***********************************************************************
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

#include "header.h"
#include "graphics.h"

#define uint unsigned int
#define wptr struct window *

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
static at *at_hilite;
static at *at_pixel_map;
static at *at_hinton_map;
static at *at_draw_arc;
static at *at_fill_arc;
static at *at_get_image;
static at *at_get_mask;
static at *at_set_linestyle;

static int
lisp_check(wptr info, at *method)
{
  at *obj = info->driverdata;
  if (obj && (obj->flags & C_EXTERN)) {
    at *p = checksend(obj->Class, method);
    if (p) {
      UNLOCK(p);
      return TRUE;
    }
  }
  return FALSE;
}

static at *
lisp_send(wptr info, at *method, at *args)
{
  if (! lisp_check(info, method))
    error(NIL,"Driver does not implement mandatory method",method);
  return send_message(NIL,info->driverdata,method,args);
}

static int 
lisp_send_maybe_ext(wptr info, at *method, at *args, at **pret)
{
  if (lisp_check(info, method))
    {
      at *p = send_message(NIL,info->driverdata,method,args);
      if (pret)
        *pret = p;
      else
        UNLOCK(p);
      return TRUE;
    }
  return FALSE;
}

static int 
lisp_send_maybe(wptr info, at *method, at *args)
{
  return lisp_send_maybe_ext(info, method, args, NIL);
}


static void 
lisp_begin(wptr info)
{
  lisp_send_maybe(info,at_begin,NIL);
}

static void 
lisp_end(wptr info)
{
  lisp_send_maybe(info,at_end,NIL);
}

static void 
lisp_close(wptr info)
{
  free(info);
}

static int 
lisp_xsize(wptr info)
{
  at *p = lisp_send(info,at_xsize,NIL);
  if (NUMBERP(p))
    return (int)(p->Number);
  error(NIL,"Method xsize returned something illegal",p);
}

static int 
lisp_ysize(wptr info)
{
  at *p = lisp_send(info,at_ysize,NIL);
  if (NUMBERP(p))
    return (int)(p->Number);
  error(NIL,"Method ysize returned something illegal",p);
}

static char *
lisp_setfont(wptr info, char *f)
{
  at *q = cons(new_string(f),NIL);
  at *r = NIL;
  lisp_send_maybe_ext(info,at_setfont,q, &r);
  UNLOCK(q);
  if (EXTERNP(r, &string_class))
    {
      UNLOCK(info->font);
      info->font = r;
      return SADD(r->Object);
    }
  return 0;
}

static void 
lisp_clear(wptr info)
{
  at *p =  lisp_send(info,at_clear,NIL);
  UNLOCK(p);
}

static void
lisp_draw_line(wptr info, int x1, int y1, int x2, int y2)
{
  at *q = cons(NEW_NUMBER(x1),
	       cons(NEW_NUMBER(y1),
		    cons(NEW_NUMBER(x2),
			 cons(NEW_NUMBER(y2),NIL))));
  at *p = lisp_send(info,at_draw_line,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void
lisp_draw_rect(wptr info, int x1, int y1, uint x2, uint y2)
{
  at *q = cons(NEW_NUMBER(x1),
	       cons(NEW_NUMBER(y1),
		    cons(NEW_NUMBER(x2),
			 cons(NEW_NUMBER(y2),NIL))));
  at *p = lisp_send(info,at_draw_rect,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void
lisp_draw_circle(wptr info, int x1, int y1, uint r)
{
  at *q = cons(NEW_NUMBER(x1),
	       cons(NEW_NUMBER(y1),
		    cons(NEW_NUMBER(r),NIL)));
  at *p = lisp_send(info,at_draw_circle,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void
lisp_fill_rect(wptr info, int x1, int y1, uint x2, uint y2)
{
  at *q = cons(NEW_NUMBER(x1),
	       cons(NEW_NUMBER(y1),
		    cons(NEW_NUMBER(x2),
			 cons(NEW_NUMBER(y2),NIL))));
  at *p = lisp_send(info,at_fill_rect,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void
lisp_fill_circle(wptr info, int x1, int y1, uint r)
{
  at *q = cons(NEW_NUMBER(x1),
	       cons(NEW_NUMBER(y1),
		    cons(NEW_NUMBER(r),NIL)));
  at *p = lisp_send(info,at_fill_circle,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void 
lisp_draw_text(wptr info, int x, int y, char *s)
{
  at *q = cons(NEW_NUMBER(x),
	       cons(NEW_NUMBER(y),
		    cons(new_string(s),NIL)));
  at *p = lisp_send(info,at_draw_text,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void
lisp_rect_text(wptr info, int x, int y, char *s, 
	       int *xp, int *yp, int *wp, int *hp)
{
  at *q = cons(NEW_NUMBER(x),
	       cons(NEW_NUMBER(y),
		    cons(new_string(s),NIL)));
  at *p = lisp_send(info,at_rect_text,q);
  at *r = p;
  UNLOCK(q);
  if (CONSP(p) && NUMBERP(p->Car)) { 
    *xp = (int)(p->Car->Number); 
    p = p->Cdr; 
    if (CONSP(p) && NUMBERP(p->Car)) {
      *yp = (int)(p->Car->Number); 
      p = p->Cdr; 
      if (CONSP(p) && NUMBERP(p->Car)) {
	*wp = (int)(p->Car->Number); 
	p = p->Cdr; 
	if (CONSP(p) && NUMBERP(p->Car)) {
	  *hp = (int)(p->Car->Number); 
	  p = p->Cdr; 
	  if (! p) {
	    UNLOCK(r);
	    return;
	  }
	}
      }
    }
  }
  error(NIL,"Method rect-text returned something illegal", r);
}


static void 
lisp_setcolor(wptr info, int x)
{
  at *q = cons(NEW_NUMBER(x),NIL);
  lisp_send_maybe(info,at_setcolor,q);
  UNLOCK(q);
}

static int 
lisp_alloccolor(wptr info, double r, double g, double b)
{
  if (lisp_check(info, at_alloccolor))
    {
      at *q = cons(NEW_NUMBER(r),
		   cons(NEW_NUMBER(g),
			cons(NEW_NUMBER(b),NIL)));
      at *p = lisp_send(info, at_alloccolor, q);
      UNLOCK(q);
      if (! NUMBERP(p))
	error(NIL,"Method alloccolor returned something illegal", p);
      return (int)(p->Number);
    }
  return (((int)(r*255)<<16) | ((int)(g*255)<<8) | (int)(b*255));
}

static int
lisp_get_mask(wptr info, uint *rp, uint *gp, uint *bp)
{
  if (lisp_check(info, at_get_mask))
    {
      at *p = lisp_send(info, at_get_mask, NIL);
      at *r = p;
      if (CONSP(p) && NUMBERP(p->Car)) {
	*rp = (uint)(p->Car->Number);
	p = p->Cdr;
	if (CONSP(p) && NUMBERP(p->Car)) {
	  *gp = (uint)(p->Car->Number);
	  p = p->Cdr;
	  if (CONSP(p) && NUMBERP(p->Car)) {
	    *bp = (uint)(p->Car->Number);
	    p = p->Cdr;
	    if (! p) {
	      UNLOCK(r);
	      return 1;
	    }
	  }
	}
      }
      error(NIL,"Method get-mask returned something illegal", r);
    }
  if (lisp_check(info, at_alloccolor))
    return 0;
  *rp = 0xff0000;
  *gp = 0xff00;
  *bp = 0xff;
  return 1;
}

static void
lisp_set_linestyle(wptr info, int x)
{ 
  at *q = cons(NEW_NUMBER(x),NIL);
  lisp_send_maybe(info,at_set_linestyle,q);
  UNLOCK(q);
}

static void 
lisp_fill_polygon(wptr info, short (*points)[2], uint n)
{
  int i;
  at *p;
  at *q = NIL;
  at **w = &q;
  for (i=0;i<n;i++) {
    *w = cons(NEW_NUMBER(points[i][0]),
              cons(NEW_NUMBER(points[i][1]), NIL) );
    w = &((*w)->Cdr->Cdr);
  }
  p = lisp_send(info,at_fill_polygon,q);
  UNLOCK(q);
  UNLOCK(p);
}

static void 
lisp_gspecial(wptr info, char *s)
{
  at *q = cons(new_string(s),NIL);
  lisp_send_maybe(info,at_gspecial,q);
  UNLOCK(q);
}

static void 
lisp_clip(wptr info, int x, int y, uint w, uint h)
{
  at *q = cons(NEW_NUMBER(x),
	       cons(NEW_NUMBER(y),
		    cons(NEW_NUMBER(w),
			 cons(NEW_NUMBER(h),NIL))));
  lisp_send_maybe(info,at_clip,q);
  UNLOCK(q);
}

static void
lisp_hilite(wptr info, int code, int x, int y, int w, int h)
{
  at *q = cons(NEW_NUMBER(code),
	       cons(NEW_NUMBER(x),
		    cons(NEW_NUMBER(y),
			 cons(NEW_NUMBER(w),
			      cons(NEW_NUMBER(h),NIL)))));
  lisp_send_maybe(info,at_hilite,q);
  UNLOCK(q);
}

static at*
lisp_make_i32matrix(uint *data, int w, int h)
{
  /* would be better to avoid copying
     by direct pointer manipulation */
  at *m;
  int i;
  uint *data2;
  intg dims[2];
  dims[0] = h;
  dims[1] = w;
  m = I32_matrix(2, dims);
  data2 = ((struct index*)(m->Object))->st->srg.data;
  if (data)
    for(i=0;i<w*h;i++)
      *data2++ = *data++;
  return m;
}


static int
lisp_pixel_map(wptr info, uint *data, int x, int y, 
	       uint w, uint h, uint sx, uint sy)
{
  if (lisp_check(info, at_pixel_map))
    {
      if (data)
        {
          at *m = lisp_make_i32matrix(data, w, h);
          at *q = cons(NEW_NUMBER(x),
                       cons(NEW_NUMBER(y),
                            cons(m,
                                 cons(NEW_NUMBER(sx),
                                      cons(NEW_NUMBER(sy), NIL ) ) ) ) );
          at *p = lisp_send(info,at_pixel_map,q);
          UNLOCK(q);
          UNLOCK(p);
        }
      return TRUE;
    }
  return FALSE;
}

static int 
lisp_hinton_map(wptr info, uint *data, int x, int y,
		uint ncol, uint nlin, uint len, uint apart)
{
  if (lisp_check(info, at_hinton_map))
    {
      if (data)
        {
          at *m = lisp_make_i32matrix(data, ncol, nlin);
          at *q = cons(NEW_NUMBER(x),
                       cons(NEW_NUMBER(y),
                            cons(m,
                                 cons(NEW_NUMBER(len),
                                      cons(NEW_NUMBER(apart), NIL ) ) ) ) );
          at *p = lisp_send(info,at_hinton_map,q);
          UNLOCK(q);
          UNLOCK(p);
        }
      return TRUE;
    }
  return FALSE;
}

static void
lisp_draw_arc(wptr info, int x, int y, uint r, int from, int to)
{
  at *q = cons(NEW_NUMBER(x),
	       cons(NEW_NUMBER(y),
		    cons(NEW_NUMBER(r),
			 cons(NEW_NUMBER(from),
			      cons(NEW_NUMBER(to), NIL)))));
  at *p = lisp_send(info, at_draw_arc, q);
  UNLOCK(p);
  UNLOCK(q);
}

static void
lisp_fill_arc(wptr info, int x, int y, uint r, int from, int to)
{
  at *q = cons(NEW_NUMBER(x),
	       cons(NEW_NUMBER(y),
		    cons(NEW_NUMBER(r),
			 cons(NEW_NUMBER(from),
			      cons(NEW_NUMBER(to), NIL)))));
  at *p = lisp_send(info, at_fill_arc, q);
  UNLOCK(p);
  UNLOCK(q);
}

static void
lisp_get_image(wptr info, uint *data, int x, int y, uint w, uint h)
{
  if (lisp_check(info, at_get_image))
    {
      int i, j;
      intg d[MAXDIMS];
      at *q = cons(NEW_NUMBER(x),
		   cons(NEW_NUMBER(y),
			cons(NEW_NUMBER(w),
			     cons(NEW_NUMBER(h), NIL))));
      at *p = lisp_send(info, at_get_image, q);
      struct index *ind = easy_index_check(p, 2, d);
      if (ind==0 || d[0]!=h || d[0]!=w)
	error(NIL,"method get-image returned something illegal",p);
      for (i=0; i<h; i++) {
	d[0] = i;
	for (j=0; j<w; j++) {
	  d[1] = j;
	  *data++ = (uint) easy_index_get(ind, d);
	}
      }
      UNLOCK(q);
      UNLOCK(p);
    }
  error(NIL,"Method get-image is not implemented",NIL);
}


struct gdriver lisp_driver = {
    "LISP",
    lisp_begin,
    lisp_end,
    lisp_close,
    lisp_xsize,
    lisp_ysize,
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
    lisp_hilite,
    lisp_pixel_map,
    lisp_hinton_map,
    lisp_draw_arc,
    lisp_fill_arc,
    lisp_get_image,
    lisp_get_mask,
    lisp_set_linestyle
};



/* ===================================================================== */
/* CREATION CODE */
/* ===================================================================== */


static at *
lisp_window(at *delegate)
{
  at *ans;
  extern struct gdriver lisp_driver;
  wptr info;
  info = malloc(sizeof(struct window));
  memset(info, 0, sizeof(struct window));
  info->used = 1;
  info->font = new_safe_string(FONT_STD);
  info->color = COLOR_FG;
  info->gdriver = &lisp_driver;
  info->clipw = 0;
  info->cliph = 0;
  info->linestyle = 0;
  ans = new_extern(&window_class, info);
  LOCK(delegate);
  info->driverdata = delegate;
  info->backptr = ans;
  return ans;
}

DX(xlisp_window)
{
  at *p;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! (p && (p->flags & X_OOSTRUCT)))
    error(NIL,"Invalid delegate for graphic driver calls", p);
  return lisp_window(p);
}

DX(xlisp_window_delegate)
{
  at *p;
  struct window *w;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  if (! EXTERNP(p, &window_class))
    error(NIL,"Not a window", p);
  w = p->Object;
  if (w->gdriver != &lisp_driver)
    error(NIL,"Not a lisp-driver window",p);
  p = w->driverdata;
  LOCK(p);
  return p;
}


/* ===================================================================== */
/* INITIALISATION CODE */
/* ===================================================================== */


void 
init_lisp_driver()
{
  dx_define("lisp-window",xlisp_window);
  dx_define("lisp-window-delegate", xlisp_window_delegate);
 
  /* Here is the list of methods accepted by the lisp driver.
     The qualification 'mandatory, recommanded, optional' are 
     just informative. The action between parenthesis is what 
     happens when you do something that would invoke an
     undefined method. */
 
  at_begin = var_define("begin");               /* optional (noop) */
  at_end = var_define("end");          	        /* optional (noop) */
  at_xsize = var_define("xsize");               /* mandatory (error) */
  at_ysize = var_define("ysize");               /* mandatory (error) */
  at_setfont = var_define("font");              /* recommanded (noop) */
  at_clear = var_define("clear");               /* mandatory (error) */
  at_draw_line = var_define("draw-line");       /* mandatory (error) */
  at_draw_rect = var_define("draw-rect");       /* mandatory (error) */
  at_draw_circle = var_define("draw-circle");   /* mandatory (error) */
  at_fill_rect = var_define("fill-rect");       /* mandatory (error) */
  at_fill_circle = var_define("fill-circle");   /* mandatory (error) */
  at_draw_text = var_define("draw-text");       /* mandatory (error) */
  at_rect_text = var_define("rect-text");       /* optional (return nil) */
  at_setcolor = var_define("color");            /* recommanded (noop) */
  at_alloccolor = var_define("alloccolor");     /* optional (assume truecolor) */
  at_fill_polygon = var_define("fill-polygon"); /* recommanded (error) */
  at_gspecial = var_define("gspecial");         /* optional (noop) */
  at_clip = var_define("clip");                 /* optional (noop) */
  at_hilite = var_define("hilite");             /* optional (noop) */
  at_pixel_map = var_define("pixel-map");       /* optional (emulate with fill-rect) */
  at_hinton_map = var_define("hinton-map");     /* optional (emulate with fill-rect) */
  at_draw_arc = var_define("draw-arc");         /* recommanded (error) */
  at_fill_arc = var_define("fill-arc");         /* recommanded (error) */
  at_get_image = var_define("get-image");       /* optional (error) */
  at_get_mask = var_define("get-mask");         /* optional (assume truecolor) */
  at_set_linestyle = var_define("linestyle");   /* optional (noop) */
}
