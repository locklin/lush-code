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
 * $Id: graphics.c,v 1.6 2002-09-27 20:58:47 leonb Exp $
 **********************************************************************/


#include "header.h"
#include "graphics.h"

static at *at_window;


/* ------------------------------------ */
/* CLASS FUNCTIONS                      */
/* ------------------------------------ */     

static void 
window_dispose(at *p)
{
  register struct window *win;
  
  win = p->Object;
  UNLOCK(win->font);
  if (win->eventhandler)
    unprotect(p);
  UNLOCK(win->eventhandler);
  UNLOCK(win->driverdata);
  if (win->gdriver->close)
    (*win->gdriver->close) (win);
  win->used = 0;
}

static void 
window_action(at *p, void (*action)(at *))
{
  register struct window *win;
  
  win = p->Object;
  (*action) (win->font);
  (*action)(win->eventhandler);
  (*action)(win->driverdata);
}

static char *
window_name(at *p)
{
  sprintf(string_buffer, "::%s:%s:%lx",
	  nameof(p->Class->classname),
	  ((struct window *) (p->Object))->gdriver->name,
	  (long)p->Object);
  return string_buffer;
}


class window_class = {
  window_dispose,
  window_action,
  window_name,
  generic_eval,
  generic_listeval,
};



/* ------------------------------------ */
/* UTILITIES                            */
/* ------------------------------------ */     

struct window *
current_window(void)
{
  register at *w;
  register struct window *ww;
  w = var_get(at_window);
  if (!EXTERNP(w,&window_class))
    error("window", "symbol value is not a window", w);
  ww = w->Object;
  UNLOCK(w);
  return ww;
}

/* This function can be called by compiled code */
struct window *
current_window_no_error(void)
{
  register at *w;
  register struct window *ww = 0;
  w = var_get(at_window);
  if (EXTERNP(w,&window_class))
    ww = w->Object;
  UNLOCK(w);
  return ww;
}


/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 1)         */
/* ------------------------------------ */     


DX(xgdriver)
{
  register struct window *win;
  
  ARG_NUMBER(0);
  win = current_window();
  return new_safe_string(win->gdriver->name);
}

DX(xxsize)
{
  register struct window *win;
  int size;
  
  ARG_NUMBER(0);
  win = current_window();
  
  if (win->gdriver->xsize) {
    (*win->gdriver->begin) (win);
    size = (*win->gdriver->xsize) (win);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'xsize'", NIL);
  
  return NEW_NUMBER(size);
}

DX(xysize)
{
  register struct window *win;
  int size;
  
  ARG_NUMBER(0);
  win = current_window();
  
  if (win->gdriver->ysize) {
    (*win->gdriver->begin) (win);
    size = (*win->gdriver->ysize) (win);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'ysize'", NIL);
  
  return NEW_NUMBER(size);
}

DX(xfont)
{
  char *s;
  struct window *win;
  struct context mycontext;
  
  win = current_window();
  
  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    s = ASTRING(1);
    
    if (win->gdriver->setfont) {
      context_push(&mycontext);	/* can be interrupted */
      (*win->gdriver->begin) (win);
      if (sigsetjmp(context->error_jump, 1)) { 
	(*win->gdriver->end) (win);
	context_pop();
	siglongjmp(context->error_jump, -1);
      }
      (*win->gdriver->setfont) (win, s);
      (*win->gdriver->end) (win);
      context_pop();
    } else
      error(NIL, "this driver does not support 'font'", NIL);
    
    UNLOCK(win->font);
    win->font = APOINTER(1);
    LOCK(win->font);
  }
  LOCK(win->font);
  return win->font;
}

DX(xcls)
{
  register struct window *win;
  
  ARG_NUMBER(0);
  win = current_window();
  
  if (win->gdriver->clear) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->clear) (win);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'cls'", NIL);
  
  
  return NIL;
}

DX(xdraw_line)
{
  register struct window *win;
  register int x1, y1, x2, y2;
  
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  x2 = AINTEGER(3);
  y2 = AINTEGER(4);
  
  if (win->gdriver->draw_line) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_line) (win, x1, y1, x2, y2);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'draw_line'", NIL);
  
  return NIL;
}

DX(xdraw_rect)
{
  register struct window *win;
  register int x1, y1, x2, y2;
  
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  x2 = AINTEGER(3);
  y2 = AINTEGER(4);
  
  if (win->gdriver->draw_rect) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_rect) (win, x1, y1, x2, y2);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'draw-rect'", NIL);
  
  return NIL;
}

DX(xdraw_circle)
{
  register struct window *win;
  register int x1, y1, r;
  
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  r = AINTEGER(3);
  
  if (win->gdriver->draw_circle) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_circle) (win, x1, y1, r);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'draw-circle'", NIL);
  
  return NIL;
}


DX(xfill_rect)
{
  register struct window *win;
  register int x1, y1, x2, y2;
  
  ARG_NUMBER(4);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  x2 = AINTEGER(3);
  y2 = AINTEGER(4);
  
  if (win->gdriver->fill_rect) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_rect) (win, x1, y1, x2, y2);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'fill-rect'", NIL);
  
  return NIL;
}

DX(xfill_circle)
{
  register struct window *win;
  register int x1, y1, r;
  
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  r = AINTEGER(3);
  
  if (r<0)
    error(NIL,"Negative radius in a circle",APOINTER(3));

  if (win->gdriver->fill_circle) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_circle) (win, x1, y1, r);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'fill-circle'", NIL);
  
  return NIL;
}


DX(xdraw_text)
{
  register struct window *win;
  register int x1, y1;
  char *s;
  
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  s = ASTRING(3);
  
  if (win->gdriver->draw_text) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_text) (win, x1, y1, s);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'draw-text'", NIL);
  
  return NIL;
}

DX(xrect_text)
{
  register struct window *win;
  int x1, y1, w, h;
  char *s;
  
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  s = ASTRING(3);
  
  if (win->gdriver->rect_text) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->rect_text) (win, x1, y1, s, &x1, &y1, &w, &h);
    (*win->gdriver->end) (win);
  } else
    return NIL;
  
  return cons(NEW_NUMBER(x1),
	      cons(NEW_NUMBER(y1),
		   cons(NEW_NUMBER(w),
			cons(NEW_NUMBER(h), NIL))));
}


/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 2)         */
/* ------------------------------------ */     


DX(xfill_polygon)
{
  register struct window *win;
  int i, j;
  short points[250][2];
  
  ALL_ARGS_EVAL;
  win = current_window();
  if ((arg_number < 4) || (arg_number & 1))
    ARG_NUMBER(-1);
  if (arg_number > 500)
    error(NIL, "up to 250 point allowed", NIL);
  
  for (i = 1, j = 0; i <= arg_number; i += 2, j++) {
    points[j][0] = AINTEGER(i);
    points[j][1] = AINTEGER(i + 1);
  }
  
  if (win->gdriver->fill_polygon) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_polygon) (win, points, arg_number / 2);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'fill-polygon'", NIL);
  
  return NIL;
}



DX(xgspecial)
{
  register struct window *win;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  win = current_window();
  if (win->gdriver->gspecial) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->gspecial) (win, ASTRING(1));
    (*win->gdriver->end) (win);
  }
  /* do nothing */
  return NIL;
}


DX(xhilite)
{
  register struct window *win;
  int code, x, y;
  unsigned int w, h;
  
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  code = AINTEGER(1);
  x = AINTEGER(2);
  y = AINTEGER(3);
  w = AINTEGER(4);
  h = AINTEGER(5);
  
  win = current_window();
  if (win->gdriver->hilite) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->hilite) (win, code, x, y, w, h);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'hilite'", NIL);
  return NIL;
}

/*
 * Context functions. These functions allow the user to change color,
 * linestyle, font and current window.
 */


DX(xclip)
{
  register struct window *win;
  register int x, y, w, h;
  
  ALL_ARGS_EVAL;
  win = current_window();
  if (arg_number) 
    {
      if (arg_number == 1 && APOINTER(1) == NIL)
        {
          if (win->gdriver->clip)
            {
              (*win->gdriver->begin) (win);
              (*win->gdriver->clip) (win, 0, 0, 0, 0);
              (*win->gdriver->end) (win);
            }
          win->clipw = win->cliph = 0;
        } 
      else 
        {
          ARG_NUMBER(4);
          x = AINTEGER(1);
          y = AINTEGER(2);
          w = AINTEGER(3);
          h = AINTEGER(4);
          if (win->gdriver->clip)
            {
              (*win->gdriver->begin) (win);
              (*win->gdriver->clip) (win, x, y, w, h);
              (*win->gdriver->end) (win);
              win->clipx = x;
              win->clipy = y;
              win->clipw = w;
              win->cliph = h;
            }
        }
    }
  if (win->clipw || win->cliph)
    return cons(NEW_NUMBER(win->clipx),
		cons(NEW_NUMBER(win->clipy),
		     cons(NEW_NUMBER(win->clipw),
			  cons(NEW_NUMBER(win->cliph), NIL))));
  return NIL;
}


DX(xcolor)
{
  int c;
  struct window *win;
  
  win = current_window();
  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    c = AINTEGER(1);
    if (c != win->color) {
      (*win->gdriver->begin) (win);
      if (win->gdriver->setcolor)
	(*win->gdriver->setcolor) (win, c);
      else {
	(*win->gdriver->end) (win);
	error(NIL, "this driver does not support 'set-color'", NIL);
      }
      (*win->gdriver->end) (win);
      win->color = c;
    }
  }
  return NEW_NUMBER(win->color);
}

DX(xalloccolor)
{
  real r, g, b;
  struct window *win;
  int color;
  
  win = current_window();
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;
  r = AREAL(1);
  g = AREAL(2);
  b = AREAL(3);
  
  if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1)
    error(NIL, "illegal RGB values", NIL);
  
  (*win->gdriver->begin) (win);
  
  if (win->gdriver->alloccolor) {
    color = (*win->gdriver->alloccolor) (win, r, g, b);
  } else {
    (*win->gdriver->end) (win);
    error(NIL, "this driver does not support 'alloc-color'", NIL);
  }
  (*win->gdriver->end) (win);
  
  return NEW_NUMBER(color);
  
}



/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 3)         */
/* ------------------------------------ */     


DX(xdraw_arc)
{
  register struct window *win;
  register int x1, y1, r, from, to;
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  r = AINTEGER(3);
  from = AINTEGER(4);
  to = AINTEGER(5);
  
  if (r<0)
    error(NIL,"Negative radius in a circle",APOINTER(3));
  if (from<=-360 || from>=360 || from>to || (to-from)>360)
    error(NIL,"Arc limits are illegal",NIL);
  if (from==to)
    return NIL;
  if (win->gdriver->draw_arc) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_arc) (win, x1, y1, r, from, to);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'draw-arc'", NIL);
  return NIL;
}

DX(xfill_arc)
{
  register struct window *win;
  register int x1, y1, r, from, to;
  
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  win = current_window();
  x1 = AINTEGER(1);
  y1 = AINTEGER(2);
  r = AINTEGER(3);
  from = AINTEGER(4);
  to = AINTEGER(5);
  
  if (r<0)
    error(NIL,"Negative radius in a circle",APOINTER(3));
  if (from<=-360 || from>=360 || from>to || (to-from)>360)
    error(NIL,"Arc limits are illegal",NIL);
  if (from==to)
    return NIL;
  if (win->gdriver->fill_arc) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_arc) (win, x1, y1, r, from, to);
    (*win->gdriver->end) (win);
  } else
    error(NIL, "this driver does not support 'fill-arc'", NIL);
  
  return NIL;
}




/* ------------------------------------ */
/* BATCHING                             */
/* ------------------------------------ */     


static int batchcount = 0;

DY(ygraphics_batch)
{
  struct context mycontext;
  at *answer;
  struct window *win;
  
  if (! CONSP(ARG_LIST))
    error(NIL, "syntax error", NIL);
  
  win = current_window();
  context_push(&mycontext);
  (*win->gdriver->begin) (win);
  batchcount++;  
  if (sigsetjmp(context->error_jump, 1)) {
    batchcount--;
    if (win->used && EXTERNP(win->backptr,&window_class))
      (*win->gdriver->end) (win);
    context_pop();
    siglongjmp(context->error_jump, -1);
  }
  answer = progn(ARG_LIST);
  batchcount--;  
  if (win->used && EXTERNP(win->backptr,&window_class))
    (*win->gdriver->end) (win);
  context_pop();
  return answer;
}

DX(xgraphics_sync)
{
  struct window *win;
  int i;

  ARG_NUMBER(0);
  win = current_window();
  for (i=0; i<batchcount; i++)
    (*win->gdriver->end) (win);
  for (i=0; i<batchcount; i++)
    (*win->gdriver->begin) (win);
  return NEW_NUMBER(batchcount);
}


/* static structure for pixel_map, hinton_map, get_image... */

static int *image = NULL;
static unsigned int imagesize = 0;


/* ------------------------------------ */
/* GET_IMAGE                            */
/* ------------------------------------ */     



/* get a rectangular pixel area from the backstore into a matrix */
void 
get_image(int x, int y, at *p)
{
  struct window *win;
  struct index *ind;
  struct idx idx;
  int d1, d2, m1, m2;

  win = current_window();

  if (win->gdriver->get_image == NULL) {
      error(NIL,"get-image is not supported for this type of window",NIL);}

  if (!indexp(p))
    error(NIL, "not an index", p);

  ind = p->Object;
  index_write_idx(ind,&idx);

  if (idx.ndim == 1) {
    d1 = idx.dim[0];
    m1 = idx.mod[0];
    d2 = 1;
    m2 = 0;
  } else if (idx.ndim == 2) {
    d1 = idx.dim[1];
    m1 = idx.mod[1];
    d2 = idx.dim[0];
    m2 = idx.mod[0];
  } else
    error(NIL, "1D or 2D index expected", p);

  {
      int *im;
      
      if (imagesize < sizeof(int) * d1 * d2) {
	if (imagesize)
	  free(image);
	imagesize = 0;
	image = malloc(sizeof(int) * d1 * d2);
	ifn (image) {
	  index_rls_idx(ind,&idx);
	  error(NIL, "not enough memory", NIL);
	}
	imagesize = sizeof(int) * d1 * d2;
      }
      im = image;
      
      (win->gdriver->begin) (win);
      (win->gdriver->get_image) (win, image, x, y, d1, d2);
      (win->gdriver->end) (win);

      if (idx.srg->type == ST_F) 
	{			/* fast routine for flts.. */
	  int i, j;
	  flt *data;
	  int off1, off2;
	  
	  data = IDX_DATA_PTR(&idx);
	  off2 = 0;
	  
	  for (j = 0; j < d2; j++, off2 += m2) {
	    off1 = off2;
	    for (i = 0; i < d1; i++, off1 += m1) {
	      data[off1] = *im++ ;
	    }
	  }
	} 
      else
	{
	  /* generic routine */
	  int i,j;
	  int off1, off2;
	  void (*setf)(gptr,int,flt);
	  gptr data;
	  
	  data = IDX_DATA_PTR(&idx);
	  setf = storage_type_setf[idx.srg->type];
	  off2 = 0;
	  
	  for (j = 0; j < d2; j++, off2 += m2) {
	    off1 = off2;
	    for (i = 0; i < d1; i++, off1 += m1) {
	      (*setf)(data,off1,*im++);
	    }
	  }
	}
      
  }
  index_rls_idx(ind,&idx);
}

DX(xget_image)
{
  ARG_NUMBER(3);
  ALL_ARGS_EVAL;

  get_image(AINTEGER(1),AINTEGER(2),APOINTER(3));
  return NIL;
}


/* ------------------------------------ */
/* COMPLEX COMMANDS                     */
/* ------------------------------------ */     


static void 
draw_value(int x, int y, double v, double maxv, int maxs)
{
  register int size;
  struct window *win;
  void (*setcolor)(struct window *, int);
  void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
  
  win = current_window();
  
  if (!(fill_rect = win->gdriver->fill_rect))
    error(NIL, "'fill-rect' unsupported", NIL);
  
  if (!(setcolor = win->gdriver->setcolor))
    error(NIL, "'setcolor' unsupported", NIL);
  
  if (maxv == 0)
    error(NIL, "Max value is 0", NIL);
  v = v / maxv;
  if (v > 1.0)
    v = 1.0;
  if (v < -1.0)
    v = -1.0;
  size = (int)(maxs * sqrt(fabs(v)));
  x -= size / 2;
  y -= size / 2;
  (*win->gdriver->begin) (win);
  if (v < 0.0) {
    size = (int)(maxs * sqrt(-v));
    (*setcolor) (win, COLOR_FG);
    (*fill_rect) (win, x, y, size, size);
  } else if (v > 0.0) {
    size = (int)(maxs * sqrt(v));
    (*setcolor) (win, COLOR_BG);
    (*fill_rect) (win, x, y, size, size);
  }
  (*setcolor) (win, win->color);
  (*win->gdriver->end) (win);
}

DX(xdraw_value)
{
  ARG_NUMBER(5);
  ALL_ARGS_EVAL;
  draw_value(AINTEGER(1), AINTEGER(2), AREAL(3),
	     AREAL(4), AINTEGER(5));
  return NIL;
}




static void 
draw_list(int xx, int y, register at *l, int ncol, 
	  double maxv, int apart, int maxs)
{
  register struct window *win;
  register int x, size, len, nlin, ap2;
  register double v;
  register int *im;
  void (*setcolor)(struct window *, int);
  void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
  
  len = length(l);
  x = xx;
  win = current_window();
  if (maxv == 0)
    error(NIL, "Illegal scaling factor", NIL);
  if (ncol <= 0)
    error(NIL, "Negative number of columns", NIL);
  nlin = (len + ncol - 1) / ncol;
  
  if (win->gdriver->hinton_map &&
      (*win->gdriver->hinton_map) (win, NIL, xx, y, ncol, nlin, len, apart)) {
    
    /* special hinton_map call */
    
    if (imagesize != sizeof(int) * len) {
      if (imagesize)
	free(image);
      imagesize = 0;
      image = malloc(sizeof(int) * len);
      if (!image)
	error(NIL, "no memory", NIL);
      imagesize = sizeof(int) * len;
    }
    im = image;
    
    while (CONSP(l)) {
      if (! NUMBERP(l->Car))
	error(NIL, "not a number", l->Car);
      v = l->Car->Number / maxv;
      l = l->Cdr;
      if (v > 1.0)
	v = 1.0;
      if (v < -1.0)
	v = -1.0;
      if (v < 0.0) {
	*im++ = (int)(-maxs * sqrt(-v));
      } else
	*im++ = (int)(maxs * sqrt(v));
    }
    (win->gdriver->begin) (win);
    (win->gdriver->hinton_map) (win, image, xx, y, ncol, nlin, len, apart);
    (win->gdriver->end) (win);
    
  } else {
    
    /* default case */
    
    if (!(fill_rect = win->gdriver->fill_rect))
      error(NIL, "'fill-rect' unsupported", NIL);
    if (!(setcolor = win->gdriver->setcolor))
      error(NIL, "'setcolor' unsupported", NIL);
    
    (*win->gdriver->begin) (win);
    (*setcolor) (win, COLOR_GRAY);
    (*fill_rect) (win, x, y, ncol * apart, nlin * apart);
    
    nlin = ncol;
    while (CONSP(l)) {
      if (! NUMBERP(l->Car)) {
	(*setcolor) (win, win->color);
	(*win->gdriver->end) (win);
	error(NIL, "not a number", l->Car);
      }
      v = l->Car->Number / maxv;
      if (v > 1.0)
	v = 1.0;
      if (v < -1.0)
	v = -1.0;
      if (v < 0.0) {
	size = (int)(maxs * sqrt(-v));
	ap2 = (apart - size) / 2;
	(*setcolor) (win, COLOR_FG);
	(*fill_rect) (win, x + ap2, y + ap2, size, size);
      } else if (v > 0.0) {
	size = (int)(maxs * sqrt(v));
	ap2 = (apart - size) / 2;
	(*setcolor) (win, COLOR_BG);
	(*fill_rect) (win, x + ap2, y + ap2, size, size);
      }
      l = l->Cdr;
      (--nlin) ? (x += apart) : (x = xx, y += apart, nlin = ncol);
    }
    (setcolor) (win, win->color);
    (*win->gdriver->end) (win);
  }
}

DX(xdraw_list)
{
  ARG_NUMBER(7);
  ALL_ARGS_EVAL;
  draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
	    AINTEGER(4), AREAL(5), AINTEGER(6), AINTEGER(7));
  return NIL;
}



/* allocate the gray levels or the colors */

static int colors[64];

static int *
allocate_grays(void)
{
  struct window *win;
  int grayok = 0;
  win = current_window();
  if (win->gdriver->alloccolor) {
    (win->gdriver->begin) (win);
    for (grayok = 0; grayok < 64; grayok++)
      colors[grayok] = (*win->gdriver->alloccolor)
	(win, (double) grayok / 63, (double) grayok / 63, (double) grayok / 63);
    (win->gdriver->end) (win);
    return colors;
  } else
    return NIL;
}

/* fill the color table with 64 colors taken from a int-matrix */
static int *
colors_from_int_matrix(at *p)
{
  struct index *ind;
  int i=64;
  ind= easy_index_check(p,1,&i);
  if ( !(ind->st->srg.type == ST_I32) ) error(NIL,"not an int-matrix",p);
  for( i=0; i<64; i++ ) {
    colors[i] = easy_index_get(ind,&i);
  }
  return colors;
}

static int *
allocate_cube(int *cube)
{
  struct window *win;
  int r,g,b,k;
  win = current_window();
  if (win->gdriver->alloccolor) {
    (win->gdriver->begin) (win);
    k = 0;
    for (r=0; r<=255; r+=85)
      for (g=0; g<=255; g+=85)
        for (b=0; b<=255; b+=85) {
          cube[k++] =  (*win->gdriver->alloccolor)
            (win, (real)r/255.0, (real)g/255.0, (real)b/255.0 );
        }
    (win->gdriver->end) (win);
    return cube;
  }
  return NIL;
}



static void 
color_draw_list(int xx, int y, register at *l, int ncol, 
		double minv, double maxv, int apart, int *colors)
{
  register struct window *win;
  register int x, len, nlin;
  register int *im;
  register int v;
  void (*setcolor)(struct window *, int);
  void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
  
  len = length(l);
  x = xx;
  win = current_window();
  if (maxv - minv == 0)
    error(NIL, "Illegal scaling factor", NIL);
  if (ncol <= 0)
    error(NIL, "Negative number of columns", NIL);
  nlin = (len + ncol - 1) / ncol;
  
  
  if (colors && win->gdriver->pixel_map &&
      (*win->gdriver->pixel_map) (win, NIL, xx, y, ncol, nlin, apart, apart)) {
    
    /* special pixel_map call */
    
    if (imagesize < sizeof(int) * ncol * nlin) {
      if (imagesize)
	free(image);
      imagesize = 0;
      image = malloc(sizeof(int) * ncol * nlin);
      if (!image)
	error(NIL, "no memory", NIL);
      imagesize = sizeof(int) * ncol * nlin;
    }
    im = image;
    
    while (CONSP(l)) {
      if (! NUMBERP(l->Car))
	error(NIL, "not a number", l->Car);
      v = (int)(64 * (l->Car->Number - minv) / (maxv - minv));
      l = l->Cdr;
      if (v > 63)
	v = 63;
      if (v < 0)
	v = 0;
      *im++ = colors[v];
      len--;
    }
    while (len-- > 0)
      *im++ = colors[63];
    
    (win->gdriver->begin) (win);
    (win->gdriver->pixel_map) (win, image, xx, y, ncol, nlin, apart, apart);
    (win->gdriver->end) (win);
    
  } else {
    
    /* default routine */
    
    if (!(fill_rect = win->gdriver->fill_rect))
      error(NIL, "'fill-rect' unsupported", NIL);
    if (!(setcolor = win->gdriver->setcolor))
      error(NIL, "'setcolor' unsupported", NIL);
    
    (*win->gdriver->begin) (win);
    nlin = ncol;
    while (CONSP(l)) {
      if (! NUMBERP(l->Car)) {
	(*setcolor) (win, win->color);
	(*win->gdriver->end) (win);
	error(NIL, "not a number", l->Car);
      }
      v = (int)(64 * (l->Car->Number - minv) / (maxv - minv));
      if (v > 63)
	v = 63;
      if (v < 0)
	v = 0;
      if (colors)
	(*setcolor) (win, colors[v]);
      else
	(*setcolor) (win, COLOR_SHADE((real) v / 63.0));
      (*fill_rect) (win, x, y, apart, apart);
      l = l->Cdr;
      (--nlin) ? (x += apart) : (x = xx, y += apart, nlin = ncol);
    }
    (*setcolor) (win, win->color);
    (*win->gdriver->end) (win);
  }
}

DX(xgray_draw_list)
{
  ARG_NUMBER(7);
  ALL_ARGS_EVAL;
  color_draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
		  AINTEGER(4), AREAL(5), AREAL(6), AINTEGER(7),
		  allocate_grays());
  return NIL;
}


DX(xcolor_draw_list)
{
  ARG_NUMBER(8);
  ALL_ARGS_EVAL;
  color_draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
		  AINTEGER(4), AREAL(5), AREAL(6), AINTEGER(7),
		  colors_from_int_matrix(APOINTER(8)) );
  return NIL;
}


int 
color_draw_idx(int x, int y, struct idx *idx, 
               real minv, real maxv, int apartx, int aparty, 
               int *colors)
{
  struct window *win;

  int d1, d2, m1, m2;

  win = current_window_no_error();
  if(win==NIL)
      return 1;
  if (maxv - minv == 0)
      return 2;
  
  if (idx->ndim == 1) {
    d1 = idx->dim[0];
    m1 = idx->mod[0];
    d2 = 1;
    m2 = 0;
  } else if (idx->ndim == 2) {
    d1 = idx->dim[1];
    m1 = idx->mod[1];
    d2 = idx->dim[0];
    m2 = idx->mod[0];
  } else
      return 3;

  if (colors && 
      win->gdriver->pixel_map &&
      (*win->gdriver->pixel_map) (win, NIL, x, y, d1, d2, apartx, aparty)) 
    {

      /* VIA THE PIXEL MAP ROUTINE */
      
      int *im;
      
      if (imagesize < sizeof(int) * d1 * d2) {
	if (imagesize)
	  free(image);
	imagesize = 0;
	image = malloc(sizeof(int) * d1 * d2);
	ifn (image) {
            return 4;
	}
	imagesize = sizeof(int) * d1 * d2;
      }
      im = image;

      if (idx->srg->type == ST_F) 
	{			/* fast routine for flts.. */
	  flt dm, dv, w;
	  int i, j, v;
	  flt *data;
	  int off1, off2;
	  
	  dm = minv;
	  dv = maxv - minv;
	  data = IDX_DATA_PTR(idx);
	  off2 = 0;
	  
	  for (j = 0; j < d2; j++, off2 += m2) {
	    off1 = off2;
	    for (i = 0; i < d1; i++, off1 += m1) {
	      w = data[off1];
	      v = 64 * (w - dm) / dv;
	      if (v > 63)
		v = 63;
	      if (v < 0)
		v = 0;
	      *im++ = colors[v];
	    }
	  }
	} 
      else
	{
	  /* generic routine */
	  int i,j,v;
	  int off1, off2;
	  flt dm, dv, w;
	  flt (*getf)(gptr,int);
	  gptr data;
	  
	  data = IDX_DATA_PTR(idx);
	  getf = storage_type_getf[idx->srg->type];
	  dm = minv;
	  dv = maxv - minv;
	  off2 = 0;
	  
	  for (j = 0; j < d2; j++, off2 += m2) {
	    off1 = off2;
	    for (i = 0; i < d1; i++, off1 += m1) {
	      w = (*getf)(data,off1);
	      v = 64 * (w - dm) / dv;
	      if (v > 63)
		v = 63;
	      if (v < 0)
		v = 0;
	      *im++ = colors[v];
	    }
	  }
	}
      (win->gdriver->begin) (win);
      (win->gdriver->pixel_map) (win, image, x, y, d1, d2, apartx, aparty);
      (win->gdriver->end) (win);
      
    } 
  else 
    {
      
      /* VIA SMALL SQUARES WHEN NO PIXEL MAP CALL */
      
      flt dm, dv, w;
      int v, xx, i, j;
      int off1, off2;
      flt (*getf)(gptr,int);
      gptr data;
      void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
      void (*setcolor)(struct window *, int);
	
      if (!(fill_rect = win->gdriver->fill_rect)) {
          return 5;
      }
      if (!(setcolor = win->gdriver->setcolor)) {
        return 6;
      }
      
      data = IDX_DATA_PTR(idx);
      getf = storage_type_getf[idx->srg->type];
      dm = minv;
      dv = maxv - minv;
      
      (*win->gdriver->begin) (win);
      
      off2 = 0;
      for (j = 0; j < d2; j++, off2 += m2) {
	off1 = off2;
	xx = x;
	for (i = 0; i < d1; i++, off1 += m1) {
	  w = (*getf)(data,off1);
	  v = 64 * (w - dm) / dv;
	  if (v > 63)
	    v = 63;
	  if (v < 0)
	    v = 0;
	  if (colors)
	    (*setcolor) (win, colors[v]);
	  else
	    (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	  (*fill_rect) (win, xx, y, apartx, aparty);
	  xx += apartx;
	}
	y += aparty;
      }
      (*setcolor) (win, win->color);
      (*win->gdriver->end) (win);
    }

  return 0;
}

void 
color_draw_matrix(int x, int y, at *p, 
                      real minv, real maxv, int apartx, int aparty, 
                      int *colors)
{
    struct index *ind;
    struct idx idx;
    int error_flag;
    struct window *win;

    if (!indexp(p))
        error(NIL, "not an index", p);
    ind = p->Object;
    index_read_idx(ind,&idx);

    error_flag = color_draw_idx(x, y, &idx, minv, maxv, apartx, aparty, colors);

    index_rls_idx(ind,&idx);

    if(error_flag)
        switch(error_flag) {
        case 1:
            win = current_window(); /* will be an error */
        case 2:
            error(NIL, "Illegal scaling factor", NIL);
        case 3:
            error(NIL, "1D or 2D index expected", p);
        case 4:
            error(NIL, "not enough memory", NIL);
        case 5:
            error(NIL, "'fill-rect' unsupported", NIL);
        case 6:
            error(NIL, "'setcolor' unsupported", NIL);
        }
}


DX(xgray_draw_matrix)
{
  int apartx, aparty;
  at *p;
  
  ALL_ARGS_EVAL;
  if (arg_number == 5) {
    apartx = aparty = 1;
  } else {
    ARG_NUMBER(7);
    apartx = AINTEGER(6);
    aparty = AINTEGER(7);
  }
  
  p = APOINTER(3);
  if (!matrixp(p))
    error(NIL, "not a matrix", p);
  
  color_draw_matrix(AINTEGER(1), AINTEGER(2), p,
		    AREAL(4), AREAL(5),
		    apartx, aparty, 
		    allocate_grays() );
  return NIL;
}


DX(xcolor_draw_matrix)
{
  int apartx, aparty;
  at *p,*q;
  
  ALL_ARGS_EVAL;
  if (arg_number == 6) {
    apartx = aparty = 1;
    q = APOINTER(6);
  } else {
    ARG_NUMBER(8);
    apartx = AINTEGER(6);
    aparty = AINTEGER(7);
    q = APOINTER(8);
  }
  
  p = APOINTER(3);
  if (!matrixp(p))
    error(NIL, "not a matrix", p);
  
  color_draw_matrix(AINTEGER(1), AINTEGER(2), p,
		    AREAL(4), AREAL(5),
		    apartx, aparty, 
		    colors_from_int_matrix(q));
  return NIL;
}


/* -------------------------------------------- */
/*     RGB DRAW MATRIX                          */
/* -------------------------------------------- */

int 
rgb_draw_idx(int x, int y, struct idx *idx, int sx, int sy)
{
  struct window *win;
  int d1, d2, m1, m2, m3;
  unsigned int red_mask, green_mask, blue_mask;
  int truecolorp;

  /* Check window characteristics */
  win = current_window_no_error();
  if(win==NULL)
      return 1;
  if (!win->gdriver->pixel_map) {
      return 2;
  }

  /* Check matrix characteristics */
  if (idx->ndim == 3) {
    m3 = idx->mod[2];
    if (idx->dim[2] < 3) {
      return 3;
    }
  } else if (idx->ndim == 2) {
    m3 = 0;
  } else {
      return 4;
  }
  d1 = idx->dim[1];
  m1 = idx->mod[1];
  d2 = idx->dim[0];
  m2 = idx->mod[0];

  truecolorp = 0;
  if (win->gdriver->get_mask)
    truecolorp = (*win->gdriver->get_mask)(win, &red_mask, &green_mask, &blue_mask);
  
  if (!truecolorp) 
    {
      /* DISPLAY REQUIRE DITHERING */
      int i, j, zi, zj, c;
      int off1, off2, r, g, b;
      unsigned char *im;
      flt (*getf)(gptr,int);
      gptr data;
      int cube[64];
      /* Allocate color cube */
      if (!allocate_cube(cube)) {
          return 5;
      }
      if (sizeof(int)!=4 && sizeof(short)!=2) {
          return 6;
      }
      /* Check pixel image buffer */
      if (imagesize < 4 * d1 * d2 * sx * sy) {
        if (imagesize)
          free(image);
        imagesize = 0;
        image = malloc(4 * d1 * d2 * sx * sy);
        ifn (image) {
            return 7;
        }
        imagesize = 4 * d1 * d2 * sx * sy;
      }
      /* Copy image and zoom */
      im = (unsigned char*)image;
      data = IDX_DATA_PTR(idx);
      getf = storage_type_getf[idx->srg->type];
      off2 = 0;
      for (j = 0; j < d2; j++, off2 += m2) 
        for (zj=0; zj<sy; zj++) {
          off1 = off2;
          for (i = 0; i < d1; i++, off1 += m1) {
            r = (*getf)(data,off1);
            g = (*getf)(data,off1 + m3);
            b = (*getf)(data,off1 + m3 + m3);
            for (zi=0; zi<sx; zi++) {
              *(short*)im = r;
              im[2] = g;
              im[3] = b;
              im += 4;
            }
          }
        }
      d1 *= sx; sx = 1;
      d2 *= sy; sy = 1;
      /* Perform error diffusion in RGB space */
      for (c=0; c<=2; c++)
        {
          im = (unsigned char*)image;
          for (j=0; j<d2; j++)
            for (i=0; i<d1; i++)
              {
                short *im1 = (short*)im;
                /* discretize */
                zi = *im1;
                if (zi < 43) {
                  zj = 0;
                } else if (zi < 128) {
                  zj = 1; zi = zi - 85;
                } else if (zi < 213) {
                  zj = 2; zi = zi - 170;
                } else {
                  zj = 3; zi = zi - 255;
                }
                /* rotate */
                *im1 = im[2];
                im[2] = im[3];
                im[3] = zj;
                /* diffuse */
                if (i+1<d1) im1[2] += (zi*7) >> 4;
                if (j+1<d2) {
                  im1 =  im1 + d1 + d1 - 2;
                  if (i>0) im1[0] += (zi*3) >> 4;
                  im1[2] += (zi*5) >> 4;
                  if (i+1<d1) im1[4] += zi >> 4;                    
                }
                im += 4;
              }
          }
      im = (char*)image;
      for (j=0; j<d2; j++)
        for (i=0; i<d1; i++)
          {
            /* code */
            zi = (((*(short*)im)<<4)&0x30);
            zi = zi | ((im[2]<<2)&0xc) | (im[3]&0x3);
            *((int*)im) = cube[zi];
            im += sizeof(int);
          }
      /* Perform drawing */
      (*win->gdriver->begin) (win);      
      (*win->gdriver->pixel_map)(win, image, x, y, d1, d2, sx, sy);
      (*win->gdriver->end) (win);
    }      
  else if (! (*win->gdriver->pixel_map)(win, NIL, x, y, d1, d2, sx, sy))
    {
      /* ZOOM IS TOO STRONG: USE SMALL SQUARES */
      flt r, g, b;
      int v, xx, i, j;
      int off1, off2;
      gptr data;
      void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int) 
        = win->gdriver->fill_rect;
      void (*setcolor)(struct window *, int) 
        = win->gdriver->setcolor;
      int (*alloccolor)(struct window *, double, double, double) 
        = win->gdriver->alloccolor;
      flt (*getf)(gptr,int);
      
      ifn (alloccolor && setcolor && fill_rect ) {
          return 8;
      }
      /* Go looping */
      data = IDX_DATA_PTR(idx);
      getf = storage_type_getf[idx->srg->type];
      (*win->gdriver->begin) (win);      
      off2 = 0;
      for (j = 0; j < d2; j++, off2 += m2) {
	off1 = off2;
	xx = x;
	for (i = 0; i < d1; i++, off1 += m1) {
	  r = (*getf)(data,off1) / 255.0;
	  g = (*getf)(data,off1 + m3) / 255.0;
	  b = (*getf)(data,off1 + m3 + m3) / 255.0;
          v = (*alloccolor)(win, r, g, b);
          (*setcolor)(win, v);
	  (*fill_rect) (win, xx, y, sx, sy);
	  xx += sx;
	}
	y += sy;
      }
      (*setcolor)(win, win->color);
        (*win->gdriver->end) (win);
    }
  else
    {
      /* ZOOM SMALL ENOUGH: USE PIXEL MAP ROUTINE */
      int i, j;
      int off1, off2;
      /* Check pixel image buffer */
      unsigned int *im;
      if (imagesize < sizeof(int) * d1 * d2) {
        if (imagesize)
          free(image);
        imagesize = 0;
        image = malloc(sizeof(int) * d1 * d2);
        ifn (image) {
            return 7;
        }
        imagesize = sizeof(int) * d1 * d2;
      }
      im = (unsigned int*)image;
      /* Copy RGB into pixel image */
      if (idx->srg->type==ST_U8 && 
          red_mask==0xff && green_mask==0xff00 && blue_mask==0xff0000) 
        {
          unsigned char r, g, b;
          unsigned char *data = IDX_DATA_PTR(idx);
          off2 = 0;
          for (j = 0; j < d2; j++, off2 += m2) {
            off1 = off2;
            for (i = 0; i < d1; i++, off1 += m1) {
              r = data[off1];
              g = data[off1 + m3];
              b = data[off1 + m3 + m3];
              *im++ = r | ((g | (b << 8)) << 8);
            }
          }
        }
      else if (idx->srg->type==ST_U8 && 
               red_mask==0xff0000 && green_mask==0xff00 && blue_mask==0xff) 
        {
          unsigned char r, g, b;
          unsigned char *data = IDX_DATA_PTR(idx);
          off2 = 0;
          for (j = 0; j < d2; j++, off2 += m2) {
            off1 = off2;
            for (i = 0; i < d1; i++, off1 += m1) {
              r = data[off1];
              g = data[off1 + m3];
              b = data[off1 + m3 + m3];
              *im++ = b | ((g | (r << 8)) << 8);
            }
          }
        }
      else 
        {
          /* Generic transcription routine */
          flt (*getf)(gptr,int);
          flt r, g, b;
          gptr data = IDX_DATA_PTR(idx);
          getf = storage_type_getf[idx->srg->type];
          off2 = 0;
          for (j = 0; j < d2; j++, off2 += m2) {
            off1 = off2;
            for (i = 0; i < d1; i++, off1 += m1) {
              r = (*getf)(data,off1) / 255.0;
              g = (*getf)(data,off1 + m3) / 255.0;
              b = (*getf)(data,off1 + m3 + m3) / 255.0;
              *im++ = (red_mask   & (unsigned int)(r * red_mask)) 
                |     (green_mask & (unsigned int)(g * green_mask)) 
                |     (blue_mask  & (unsigned int)(b * blue_mask));
            }
          }
	}
      /* perform drawing */
      (*win->gdriver->begin) (win);      
      (*win->gdriver->pixel_map)(win, image, x, y, d1, d2, sx, sy);
      (*win->gdriver->end) (win);
    }

  /* THAT'S ALL FOLKS */
  return 0;
}


void
rgb_draw_matrix(int x, int y, at *p, int sx, int sy)
{
    struct index *ind;
    struct idx idx;
    int error_flag;
    struct window *win;

    if (!indexp(p))
        error(NIL, "not an index", p);
    ind = p->Object;
    index_read_idx(ind,&idx);

    error_flag = rgb_draw_idx(x, y, &idx, sx, sy);

    index_rls_idx(ind,&idx);

    if(error_flag)
        switch(error_flag) {
        case 1:
            win = current_window(); /* will be an error */
        case 2:
            error(NIL,"Graphic driver does not support pixel-map",NIL);
        case 3:
            error(NIL,"Last dimension shouls be three (or more)",p);
        case 4:
            error(NIL,"2D or 3D index expected",p);
        case 5:
            error(NIL, "cannot allocate enough colors", NIL);
        case 6:
            error(NIL, "machine does not support dithering", NIL);
        case 7:
          error(NIL, "not enough memory", NIL);
        case 8:
            error(NIL,"Graphic driver does not support "
                  "alloccolor, setcolor or fill_rect",p);
        }
}

DX(xrgb_draw_matrix)
{
  int x, y, sx, sy;
  at *p;

  ALL_ARGS_EVAL;
  sx = sy = 1;
  if (arg_number != 3)
    {
      ARG_NUMBER(5);
      sx = AINTEGER(4);
      sy = AINTEGER(5);
      if (sx<1 || sy<1)
        error(NIL,"Illegal zoom factor",NIL);
    }
  x = AINTEGER(1);
  y = AINTEGER(2);
  p = APOINTER(3);
  rgb_draw_matrix(x,y,p,sx,sy);
  return NIL;
}





/* ------------------------------------ */
/* OGRE SPEEDUP                         */
/* ------------------------------------ */     



static void 
getrect(at *pp, int *x, int *y, int *w, int *h)
{
  at *p;
  
  p = pp;
  if (CONSP(p) && NUMBERP(p->Car))
    {
      *x = (int)(p->Car->Number);
      p = p->Cdr;
      if (CONSP(p) && NUMBERP(p->Car))
	{
	  *y = (int)(p->Car->Number);
	  p = p->Cdr;
	  if (CONSP(p) && NUMBERP(p->Car))
	    {
	      *w = (int)(p->Car->Number);
	      p = p->Cdr;
	      if (CONSP(p) && NUMBERP(p->Car))
		{
		  *h = (int)(p->Car->Number);
		  p = p->Cdr;
		  if (! p)
		    return;
		}
	    }
	}
    }
  error(NIL,"illegal rectangle",pp);
}

static at * 
makerect(int x, int y, int w, int h)
{
  return cons(NEW_NUMBER(x),
	      cons(NEW_NUMBER(y),
		   cons(NEW_NUMBER(w),
			cons(NEW_NUMBER(h),
			     NIL))));
}


DX(xpoint_in_rect)
{
  int xx,yy,x,y,w,h;
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  xx=AINTEGER(1);
  yy=AINTEGER(2);
  getrect(ALIST(3),&x,&y,&w,&h);
  
  if ( xx>=x && yy>=y && xx<x+w && yy<y+h)
    return true();
  else
    return NIL;
}

DX(xrect_in_rect)
{
  int xx,yy,ww,hh,x,y,w,h;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  getrect(ALIST(1),&xx,&yy,&ww,&hh);
  getrect(ALIST(2),&x,&y,&w,&h);
  
  w += x;
  h += y;
  ww += xx-1;
  hh += yy-1;
  
  if ( xx>=x && yy>=y && xx<w && yy<h &&
      ww>=x && hh>=y && ww<w && hh<h  )
    return true();
  else
    return NIL;
}



DX(xcollide_rect)
{
  int x1,y1,w1,h1,x2,y2,w2,h2,x,y,w,h;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  getrect(ALIST(1),&x1,&y1,&w1,&h1);
  getrect(ALIST(2),&x2,&y2,&w2,&h2);
  
#define TLMAX(u,v) (((u)>(v))?(u):(v))
#define TLMIN(u,v) (((u)<(v))?(u):(v))
  
  x = TLMAX(x1,x2);
  y = TLMAX(y1,y2);
  w = TLMIN(x1+w1,x2+w2)-x;
  h = TLMIN(y1+h1,y2+h2)-y;
  
#undef TLMAX
#undef TLMIN
  
  if (w>0 && h>0)
    return makerect(x,y,w,h);
  else
    return NIL;
}


DX(xbounding_rect)
{
  int x1,y1,w1,h1,x2,y2,w2,h2,x,y,w,h;
  ALL_ARGS_EVAL;
  ARG_NUMBER(2);
  getrect(ALIST(1),&x1,&y1,&w1,&h1);
  getrect(ALIST(2),&x2,&y2,&w2,&h2);
  
#define TLMAX(u,v) (((u)>(v))?(u):(v))
#define TLMIN(u,v) (((u)<(v))?(u):(v))
  
  x = TLMIN(x1,x2);
  y = TLMIN(y1,y2);
  w = TLMAX(x1+w1,x2+w2)-x;
  h = TLMAX(y1+h1,y2+h2)-y;
  
#undef TLMAX
#undef TLMIN
  
  return makerect(x,y,w,h);
}




DX(xexpand_rect)
{
  int bx,by,x,y,w,h;
  ALL_ARGS_EVAL;
  ARG_NUMBER(3);
  getrect(ALIST(1),&x,&y,&w,&h);
  bx = AINTEGER(2);
  by = AINTEGER(3);
  return makerect(x-bx,y-by,w+2*bx,h+2*by);
}


DX(xdraw_round_rect)
{
  int r = 3;
  int left,top,bottom,right;
  int almosttop,almostleft,almostright,almostbottom;
  register struct window *win;
  win = current_window();
  ALL_ARGS_EVAL;
  if (arg_number==5)
    r = AINTEGER(5);
  else
    ARG_NUMBER(4);
  left = AINTEGER(1);
  top  = AINTEGER(2);
  right  = left + AINTEGER(3);
  bottom = top + AINTEGER(4);
  almostleft = left+r;
  almosttop = top+r;
  almostright = right-r;
  almostbottom = bottom-r;
  if (win->gdriver->draw_arc && win->gdriver->draw_line) 
  {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_line)(win,almostright,top,almostleft,top);
    (*win->gdriver->draw_line)(win,almostleft,bottom,almostright,bottom);
    (*win->gdriver->draw_line)(win,left,almosttop,left,almostbottom);
    (*win->gdriver->draw_line)(win,right,almostbottom,right,almosttop);
    (*win->gdriver->draw_arc)(win,almostleft,almosttop,r,90,180);
    (*win->gdriver->draw_arc)(win,almostleft,almostbottom,r,180,270);
    (*win->gdriver->draw_arc)(win,almostright,almosttop,r,0,90);
    (*win->gdriver->draw_arc)(win,almostright,almostbottom,r,270,360);
    (*win->gdriver->end) (win);  
  } 
  else if ( win->gdriver->draw_line ) 
  {
    (*win->gdriver->begin) (win);
    (*win->gdriver->draw_rect)(win,left,top,right-left,bottom-top);
    (*win->gdriver->end) (win);  
  } else
    error(NIL, "this driver does not support 'fill-polygon'", NIL);
  return NIL;
}



DX(xfill_round_rect)
{
  int r = 3;
  int left,top,bottom,right;
  int almosttop,almostleft,almostright,almostbottom;
  int almostwidth;
  register struct window *win;
  
  win = current_window();
  
  ALL_ARGS_EVAL;
  if (arg_number==5)
    r = AINTEGER(5);
  else
    ARG_NUMBER(4);
  
  left = AINTEGER(1);
  top  = AINTEGER(2);
  right  = left + AINTEGER(3);
  bottom = top + AINTEGER(4);
  
  almostleft = left+r;
  almosttop = top+r;
  almostright = right-r;
  almostbottom = bottom-r;
  almostwidth = almostright-almostleft;
  
  if (win->gdriver->fill_arc) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_rect)(win,left,almosttop,right-left,almostbottom-almosttop);
    (*win->gdriver->fill_rect)(win,almostleft,top,almostwidth,r);
    (*win->gdriver->fill_rect)(win,almostleft,almostbottom,almostwidth,r);
    (*win->gdriver->fill_arc)(win,almostleft,almosttop,r,90,180);
    (*win->gdriver->fill_arc)(win,almostleft,almostbottom-1,r,180,270);
    (*win->gdriver->fill_arc)(win,almostright-1,almosttop,r,0,90);
    (*win->gdriver->fill_arc)(win,almostright-1,almostbottom-1,r,270,360);
    (*win->gdriver->end) (win);
    
  } else if ( win->gdriver->fill_rect ) {
    (*win->gdriver->begin) (win);
    (*win->gdriver->fill_rect)(win,left,top,right-left,bottom-top);
    (*win->gdriver->end) (win);
    
  } else
    error(NIL, "this driver does not support 'fill-polygon'", NIL);
  return NIL;
}



DY(ygsave)
{
  at *ans = NIL;
  int  oldcolor;
  at   *oldfont;
  short oldx,oldy,oldw,oldh;
  struct window *win;
  struct context mycontext;
  int errorflag=0;
  
  if (! CONSP(ARG_LIST))
    error(NIL, "syntax error", NIL);
  
  win = current_window();
  oldcolor = win->color;
  oldfont = win->font;
  oldx = win->clipx;
  oldy = win->clipy;
  oldw = win->clipw;
  oldh = win->cliph;
  LOCK(oldfont);
  
  context_push(&mycontext);
  if (sigsetjmp(context->error_jump, 1))
    errorflag = 1;
  else
    ans = progn(ARG_LIST);
  context_pop();
  
  if (win->used && EXTERNP(win->backptr,&window_class))
    /* Is the window still there */
    {
      if (oldcolor!=win->color) {
	(*win->gdriver->begin) (win);
	if (win->gdriver->setcolor) {
	  (*win->gdriver->setcolor) (win, oldcolor);
	  win->color = oldcolor;
	}
	(*win->gdriver->end) (win);
      }
    
      if (oldfont!=win->font)
	if (win->gdriver->setfont && oldfont && (oldfont->flags&X_STRING)) {
	  (*win->gdriver->begin) (win);
	  (*win->gdriver->setfont)(win,SADD(oldfont->Object));
	  (*win->gdriver->end) (win);
	  UNLOCK(win->font);
	  win->font = oldfont;
	  LOCK(oldfont);
	}
      UNLOCK(oldfont);
      
      if (oldx!=win->clipx || oldy!=win->clipy || 
	  oldw!=win->clipw || oldh!=win->cliph )
	if (win->gdriver->clip) {
	  (*win->gdriver->begin) (win);
	  (*win->gdriver->clip) (win, oldx, oldy, oldw, oldh);
	  (*win->gdriver->end) (win);
	  win->clipx = oldx;
	  win->clipy = oldy;
	  win->clipw = oldw;
	  win->cliph = oldh;
	}
    }
  
  if (errorflag)
    siglongjmp(context->error_jump, -1L);
  return ans;
}


DX(xaddclip)
{
  register struct window *win;
  int x, y, w, h;
  int x1, y1, w1, h1;
  
  win = current_window();
  if (! win->gdriver->clip)
    error(NIL, "this driver does not support 'clip'", NIL);
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  getrect(ALIST(1),&x1,&y1,&w1,&h1);
  
  if (win->clipw || win->cliph) {
    
#define TLMAX(u,v) (((u)>(v))?(u):(v))
#define TLMIN(u,v) (((u)<(v))?(u):(v))
    
    x = TLMAX(x1,win->clipx);
    y = TLMAX(y1,win->clipy);
    w = TLMIN(x1+w1,win->clipx+win->clipw)-x;
    h = TLMIN(y1+h1,win->clipy+win->cliph)-y;
    
#undef TLMAX
#undef TLMIN
    
    if (w>0 && h>0) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->clip) (win, x, y, w, h);
      (*win->gdriver->end) (win);
      win->clipx = x;
      win->clipy = y;
      win->clipw = w;
      win->cliph = h;
    } else {
      int xs = (*win->gdriver->xsize)(win);
      int ys = (*win->gdriver->ysize)(win);
      (*win->gdriver->begin) (win);
      (*win->gdriver->clip) (win, xs+10,ys+10,1,1);
      (*win->gdriver->end) (win);
      win->clipx = xs+10;
      win->clipy = ys+10;
      win->clipw = 1;
      win->cliph = 1;
    }
  } else {
    (*win->gdriver->begin) (win);
    (*win->gdriver->clip) (win, x1, y1, w1, h1);
    (*win->gdriver->end) (win);
    win->clipx = x1;
    win->clipy = y1;
    win->clipw = w = w1;
    win->cliph = h = h1;
  }
  if (w>0 && h>0)
    return cons(NEW_NUMBER(win->clipx),
		cons(NEW_NUMBER(win->clipy),
		     cons(NEW_NUMBER(win->clipw),
			  cons(NEW_NUMBER(win->cliph), NIL))));
  else
    return NIL;
}


/* ------------------------------------ */
/* INITIALISATION                       */
/* ------------------------------------ */     


void 
init_graphics(void)
{
  /* WINDOW */
  class_define("WINDOW",&window_class );
  at_window = var_define("window");
  /* RELEASE 1 */
  dx_define("gdriver",xgdriver);
  dx_define("xsize",xxsize);
  dx_define("ysize",xysize);
  dx_define("font",xfont);
  dx_define("cls",xcls);
  dx_define("draw_line",xdraw_line);
  dx_define("draw_rect",xdraw_rect);
  dx_define("draw_circle",xdraw_circle);
  dx_define("fill_rect",xfill_rect);
  dx_define("fill_circle",xfill_circle);
  dx_define("draw_text",xdraw_text);
  dx_define("rect_text",xrect_text);
  /* RELEASE 2 */
  dx_define("fill_polygon",xfill_polygon);
  dx_define("gspecial",xgspecial);
  dx_define("hilite",xhilite);
  dx_define("clip",xclip);
  dx_define("color",xcolor);
  dx_define("alloccolor",xalloccolor);
  /* RELEASE 3 */
  dx_define("draw_arc",xdraw_arc);
  dx_define("fill_arc",xfill_arc);
  /* BATCHING */
  dy_define("graphics_batch",ygraphics_batch);
  dx_define("graphics_sync",xgraphics_sync);
  /* COMPLEX COMMANDS */
  dx_define("draw_value",xdraw_value);
  dx_define("draw_list",xdraw_list);
  dx_define("gray_draw_list",xgray_draw_list);
  dx_define("color_draw_list",xcolor_draw_list);
  dx_define("gray_draw_matrix",xgray_draw_matrix);
  dx_define("color_draw_matrix",xcolor_draw_matrix);
  /* dx_define("draw_pixel_matrix",xdraw_pixel_matrix); */
  dx_define("rgb_draw_matrix",xrgb_draw_matrix);  
  dx_define("xget_image",xget_image);  
  /* OGRE SPEEDUP */
  dx_define("point_in_rect",xpoint_in_rect);
  dx_define("rect_in_rect",xrect_in_rect);
  dx_define("collide_rect",xcollide_rect);
  dx_define("bounding_rect",xbounding_rect);
  dx_define("expand_rect",xexpand_rect);
  dx_define("draw_round_rect",xdraw_round_rect);
  dx_define("fill_round_rect",xfill_round_rect);
  dy_define("gsave",ygsave);
  dx_define("addclip",xaddclip);
}
