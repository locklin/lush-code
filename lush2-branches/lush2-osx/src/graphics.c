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
#include "graphics.h"


static at *at_window;

void clear_window(window_t *w, size_t s)
{
   memset(w, 0, s);
}

void mark_window(window_t *w)
{
   MM_MARK(w->font);
   MM_MARK(w->eventhandler);
   MM_MARK(w->driverdata);
   MM_MARK(w->backptr);
}

static window_t *window_dispose(window_t *);

bool finalize_window(window_t *w)
{
   window_dispose(w);
   return true;
}

mt_t mt_window = mt_undefined;


/* ------------------------------------ */
/* CLASS FUNCTIONS                      */
/* ------------------------------------ */     

static window_t *window_dispose(window_t *win)
{
   if (win && win->used) {
      if (win->eventhandler)
         unprotect(win->backptr);
      win->eventhandler = NIL;
      win->driverdata = NIL;
      if (win->gdriver->close)
         (*win->gdriver->close) (win);
      win->used = 0;
      zombify(win->backptr);
      return NULL;
   }
   return win;
}

static const char *window_name(at *p)
{
   sprintf(string_buffer, "::%s:%s:%lx",
           NAMEOF(Class(p)->classname),
           ((window_t *)Mptr(p))->gdriver->name,
           (long)Mptr(p));
   return mm_strdup(string_buffer);
}


/* ------------------------------------ */
/* UTILITIES                            */
/* ------------------------------------ */     

window_t *current_window(void)
{
  at *p = var_get(at_window);
  ifn (WINDOWP(p))
     error("window", "not a window", p);
  return (window_t *)Mptr(p);
}

/* This function can be called by compiled code */
window_t *current_window_no_error(void)
{
   at *p = var_get(at_window);
   return WINDOWP(p) ? (window_t *)Mptr(p) : NULL;
}


/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 1)         */
/* ------------------------------------ */     


DX(xgdriver)
{
   ARG_NUMBER(0);
   window_t *win = current_window();
   return make_string(win->gdriver->name);
}

DX(xxsize)
{
   ARG_NUMBER(0);
   window_t *win = current_window();
  
   int size = -1;
   if (win->gdriver->xsize) {
      (*win->gdriver->begin)(win);
      size = (*win->gdriver->xsize)(win);
      (*win->gdriver->end)(win);
   } else
      RAISEFX("driver does not support 'xsize'", NIL);
   
   return NEW_NUMBER(size);
}

DX(xysize)
{
   ARG_NUMBER(0);
   window_t *win = current_window();
  
   int size = -1;
   if (win->gdriver->ysize) {
      (*win->gdriver->begin)(win);
      size = (*win->gdriver->ysize) (win);
      (*win->gdriver->end)(win);
   } else
      RAISEFX("driver does not support 'ysize'", NIL);
   
   return NEW_NUMBER(size);
}

DX(xfont)
{
   struct lush_context mycontext;
   
   window_t *win = current_window();
   at *oldfont = win->font;
   MM_ANCHOR(oldfont);
   at *q = oldfont;
   const char *r = 0;
   if (arg_number) {
      ARG_NUMBER(1);
      const char *s = ASTRING(1);
      q = str_mb_to_utf8(s);
      if (STRINGP(q))
         s = String(q);
      if (win->gdriver->setfont) {
         context_push(&mycontext);	/* can be interrupted */
         (*win->gdriver->begin)(win);
         if (sigsetjmp(context->error_jump, 1)) { 
            (*win->gdriver->end) (win);
            context_pop();
            siglongjmp(context->error_jump, -1);
         }
         r = (*win->gdriver->setfont) (win, s);
         (*win->gdriver->end) (win);
         context_pop();
      } else {
         RAISEFX("driver does not support 'font'", NIL);
      }
      if (r)
         q = win->font = str_utf8_to_mb(r);
      else
         q = NIL;
   }
   return q;
}

DX(xcls)
{
   ARG_NUMBER(0);
   window_t *win = current_window();
   
   if (win->gdriver->clear) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->clear) (win);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'cls'", NIL);

   return NIL;
}

DX(xdraw_line)
{
   ARG_NUMBER(4);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int x2 = AINTEGER(3);
   int y2 = AINTEGER(4);
  
   if (win->gdriver->draw_line) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_line) (win, x1, y1, x2, y2);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'draw_line'", NIL);
   
   return NIL;
}

DX(xdraw_rect)
{
   ARG_NUMBER(4);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int x2 = AINTEGER(3);
   int y2 = AINTEGER(4);
   
   if (win->gdriver->draw_rect) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_rect) (win, x1, y1, x2, y2);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'draw-rect'", NIL);
  
   return NIL;
}

DX(xdraw_circle)
{
   ARG_NUMBER(3);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int r = AINTEGER(3);
   
   if (win->gdriver->draw_circle) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_circle) (win, x1, y1, r);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'draw-circle'", NIL);
   
   return NIL;
}


DX(xfill_rect)
{
   ARG_NUMBER(4);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int x2 = AINTEGER(3);
   int y2 = AINTEGER(4);
   
   if (win->gdriver->fill_rect) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->fill_rect) (win, x1, y1, x2, y2);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'fill-rect'", NIL);
   
   return NIL;
}

DX(xfill_circle)
{
   ARG_NUMBER(3);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int r = AINTEGER(3);
   
   if (r<0)
      RAISEFX("radius not non-negative", APOINTER(3));
   
   if (win->gdriver->fill_circle) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->fill_circle) (win, x1, y1, r);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'fill-circle'", NIL);
  
   return NIL;
}


DX(xdraw_text)
{
   ARG_NUMBER(3);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   const char *s = ASTRING(3);
   
   if (win->gdriver->draw_text) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_text) (win, x1, y1, s);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'draw-text'", NIL);
  
   return NIL;
}

DX(xrect_text)
{
   ARG_NUMBER(3);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   const char *s = ASTRING(3);
   int w = 0;
   int h = 0;
   if (win->gdriver->rect_text)  {
      (*win->gdriver->begin) (win);
      (*win->gdriver->rect_text) (win, x1, y1, s, &x1, &y1, &w, &h);
      (*win->gdriver->end) (win);
   } 
   if (w == 0 || h == 0)
      return NIL;
   return new_cons(NEW_NUMBER(x1),
                   new_cons(NEW_NUMBER(y1),
                            new_cons(NEW_NUMBER(w),
                                     new_cons(NEW_NUMBER(h), NIL))));
}


/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 2)         */
/* ------------------------------------ */     


DX(xfill_polygon)
{
   short points[250][2];
   window_t *win = current_window();

   if ((arg_number < 4) || (arg_number & 1))
      ARG_NUMBER(-1);
   if (arg_number > 500)
      RAISEFX("too many points (max is 250)", NIL);
   
   int i, j;
   for (i = 1, j = 0; i <= arg_number; i += 2, j++) {
      points[j][0] = AINTEGER(i);
      points[j][1] = AINTEGER(i + 1);
   }
   
   if (win->gdriver->fill_polygon) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->fill_polygon) (win, points, arg_number / 2);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'fill-polygon'", NIL);
   
   return NIL;
}



DX(xgspecial)
{
   ARG_NUMBER(1);
   window_t *win = current_window();
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
   ARG_NUMBER(5);
   int code = AINTEGER(1);
   int x = AINTEGER(2);
   int y = AINTEGER(3);
   unsigned int w = AINTEGER(4);
   unsigned int h = AINTEGER(5);
   
   window_t *win = current_window();
   if (win->gdriver->hilite) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->hilite) (win, code, x, y, w, h);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'hilite'", NIL);

   return NIL;
}

/*
 * Context functions. These functions allow the user to change color,
 * linestyle, font and current window.
 */


DX(xclip)
{
   window_t *win = current_window();

   if (arg_number) {
      if (arg_number == 1 && APOINTER(1) == NIL) {
         if (win->gdriver->clip) {
            (*win->gdriver->begin) (win);
            (*win->gdriver->clip) (win, 0, 0, 0, 0);
            (*win->gdriver->end) (win);
         }
         win->clipw = win->cliph = 0;
      } else {
         ARG_NUMBER(4);
         int x = AINTEGER(1);
         int y = AINTEGER(2);
         int w = AINTEGER(3);
         int h = AINTEGER(4);
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
      return new_cons(NEW_NUMBER(win->clipx),
                      new_cons(NEW_NUMBER(win->clipy),
                               new_cons(NEW_NUMBER(win->clipw),
                                        new_cons(NEW_NUMBER(win->cliph), NIL))));
   return NIL;
}


DX(xlinestyle)
{
   window_t *win = current_window();
   if (arg_number>=1) {
      ARG_NUMBER(1);
      int ls = AINTEGER(1);
      if (win->gdriver->set_linestyle) {
         (*win->gdriver->begin) (win);
         (*win->gdriver->set_linestyle) (win, ls);
         (*win->gdriver->end) (win);
         win->linestyle = ls;
      }
   }
   return NEW_NUMBER(win->linestyle);
}

DX(xcolor)
{
   window_t *win = current_window();
   if (arg_number) {
      ARG_NUMBER(1);
      int c = AINTEGER(1);
      if (c != win->color) {
         (*win->gdriver->begin) (win);
         if (win->gdriver->setcolor)
            (*win->gdriver->setcolor) (win, c);
         else {
            (*win->gdriver->end) (win);
            RAISEFX("driver does not support 'set-color'", NIL);
         }
         (*win->gdriver->end) (win);
         win->color = c;
      }
   }
   return NEW_NUMBER(win->color);
}

DX(xalloccolor)
{
   ARG_NUMBER(3);
   window_t *win = current_window();
   double r = AREAL(1);
   double g = AREAL(2);
   double b = AREAL(3);
  
   if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1)
      RAISEFX("illegal RGB values", NIL);
   
   (*win->gdriver->begin) (win);
  
   int color;
   if (win->gdriver->alloccolor) {
      color = (*win->gdriver->alloccolor) (win, r, g, b);
   } else {
      (*win->gdriver->end) (win);
      RAISEFX("driver does not support 'alloc-color'", NIL);
   }
   (*win->gdriver->end) (win);
   
   return NEW_NUMBER(color);
}



/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 3)         */
/* ------------------------------------ */     


DX(xdraw_arc)
{
   ARG_NUMBER(5);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int r = AINTEGER(3);
   int from = AINTEGER(4);
   int to = AINTEGER(5);
   
   if (r<0)
      RAISEFX("negative radius in a circle", APOINTER(3));
   if (from<=-360 || from>=360 || from>to || (to-from)>360)
      RAISEFX("arc limits are illegal",NIL);
   if (from==to)
      return NIL;
   if (win->gdriver->draw_arc) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_arc) (win, x1, y1, r, from, to);
      (*win->gdriver->end) (win);
   } else
      RAISEFX("driver does not support 'draw-arc'", NIL);
   return NIL;
}

DX(xfill_arc)
{
   ARG_NUMBER(5);
   window_t *win = current_window();
   int x1 = AINTEGER(1);
   int y1 = AINTEGER(2);
   int r = AINTEGER(3);
   int from = AINTEGER(4);
   int to = AINTEGER(5);
  
   if (r<0)
      RAISEFX("negative radius in a circle", APOINTER(3));
   if (from<=-360 || from>=360 || from>to || (to-from)>360)
      RAISEFX("arc limits are illegal",NIL);
   if (from==to)
      return NIL;
   if (win->gdriver->fill_arc) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->fill_arc) (win, x1, y1, r, from, to);
      (*win->gdriver->end) (win);
   } else
      RAISEFX(" driver does not support 'fill-arc'", NIL);
   
   return NIL;
}


/* ------------------------------------ */
/* BATCHING                             */
/* ------------------------------------ */     


static int batchcount = 0;

DY(ygraphics_batch)
{
   if (! CONSP(ARG_LIST))
      RAISEFX("syntax error", NIL);
  
   window_t *win = current_window();

   struct lush_context mycontext;
   context_push(&mycontext);
   (*win->gdriver->begin) (win);

   batchcount++;  
   if (sigsetjmp(context->error_jump, 1)) {
      batchcount--;
      if (win->used && WINDOWP(win->backptr))
         (*win->gdriver->end) (win);
      context_pop();
      siglongjmp(context->error_jump, -1);
   }
   at *answer = progn(ARG_LIST);
   batchcount--;  

   if (win->used && WINDOWP(win->backptr))
      (*win->gdriver->end) (win);
   context_pop();
   return answer;
}

DX(xgraphics_sync)
{
   ARG_NUMBER(0);
   window_t *win = current_window();
   for (int i=0; i<batchcount; i++)
      (*win->gdriver->end) (win);
   for (int i=0; i<batchcount; i++)
      (*win->gdriver->begin) (win);
   return NEW_NUMBER(batchcount);
}


/* static structure for pixel_map, hinton_map, get_image... */

static unsigned int *image = NULL;
static unsigned int imagesize = 0;



/* ------------------------------------ */
/* COMPLEX COMMANDS                     */
/* ------------------------------------ */     


static void draw_value(int x, int y, double v, double maxv, int maxs)
{
   void (*setcolor)(struct window *, int);
   void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
   
   window_t *win = current_window();
   
   if (!(fill_rect = win->gdriver->fill_rect))
      RAISEFX("driver does not support 'fill-rect'", NIL);
   
   if (!(setcolor = win->gdriver->setcolor))
      RAISEFX("driver does not support 'setcolor'", NIL);
   
   if (maxv == 0)
      RAISEFX("max value is 0", NIL);
   v = v / maxv;
   if (v > 1.0)
      v = 1.0;
   if (v < -1.0)
      v = -1.0;
   int size = (int)(maxs * sqrt(fabs(v)));
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
   draw_value(AINTEGER(1), AINTEGER(2), AREAL(3),
              AREAL(4), AINTEGER(5));
   return NIL;
}


static void draw_list(int xx, int y, register at *l, int ncol, 
                      double maxv, int apart, int maxs)
{
   void (*setcolor)(struct window *, int);
   void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
  
   int len = length(l);
   int x = xx;
   window_t *win = current_window();
   if (maxv == 0)
      RAISEFX("scaling factor is zero", NEW_NUMBER(maxv));
   if (ncol <= 0)
      RAISEFX("invalid number of columns", NEW_NUMBER(ncol));
   int nlin = (len + ncol - 1) / ncol;
   
   if (win->gdriver->hinton_map &&
       (*win->gdriver->hinton_map) (win, NIL, xx, y, ncol, nlin, len, apart)) {
      
      /* special hinton_map call */
      if (imagesize != sizeof(int) * len) {
         if (imagesize)
            free(image);
         imagesize = 0;
         image = malloc(sizeof(int) * len);
         if (!image)
            RAISEFX("no memory", NIL);
         imagesize = sizeof(int) * len;
      }
      unsigned int *im = image;
      
      while (CONSP(l)) {
         if (! NUMBERP(Car(l)))
            RAISEFX("not a number", Car(l));
         double v = Number(Car(l)) / maxv;
         l = Cdr(l);
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
         RAISEFX("driver does not support 'fill-rect'", NIL);
      if (!(setcolor = win->gdriver->setcolor))
         RAISEFX("driver does not support 'setcolor'", NIL);
      
      (*win->gdriver->begin) (win);
      (*setcolor) (win, COLOR_GRAY);
      (*fill_rect) (win, x, y, ncol * apart, nlin * apart);
      
      nlin = ncol;
      while (CONSP(l)) {
         if (! NUMBERP(Car(l))) {
            (*setcolor) (win, win->color);
            (*win->gdriver->end) (win);
            RAISEFX("not a number", Car(l));
         }
         double v = Number(Car(l)) / maxv;
         if (v > 1.0)
            v = 1.0;
         if (v < -1.0)
            v = -1.0;
         if (v < 0.0) {
            int size = (int)(maxs * sqrt(-v));
            int ap2 = (apart - size) / 2;
            (*setcolor) (win, COLOR_FG);
            (*fill_rect) (win, x + ap2, y + ap2, size, size);
         } else if (v > 0.0) {
            int size = (int)(maxs * sqrt(v));
            int ap2 = (apart - size) / 2;
            (*setcolor) (win, COLOR_BG);
            (*fill_rect) (win, x + ap2, y + ap2, size, size);
         }
         l = Cdr(l);
         (--nlin) ? (x += apart) : (x = xx, y += apart, nlin = ncol);
      }
      (setcolor) (win, win->color);
      (*win->gdriver->end) (win);
   }
}

DX(xdraw_list)
{
   ARG_NUMBER(7);
   draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
             AINTEGER(4), AREAL(5), AINTEGER(6), AINTEGER(7));
   return NIL;
}



/* allocate the gray levels or the colors */

static int colors[64];

static int *allocate_grays(void)
{
   int grayok = 0;
   window_t *win = current_window();
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
static int *colors_from_int_matrix(at *p)
{
   ifn (INDEXP(p))
      RAISEF("not an index", p);
   index_t *ind = Mptr(p);
   ifn (IND_STTYPE(ind)==ST_INT) 
      RAISEF("not an integer array", p);
   easy_index_check(ind, SHAPE1D(64));

   int *indbase = IND_BASE_TYPED(ind, int);
   for (int i=0; i<64; i++)
      colors[i] = indbase[ind->mod[0]*i];

   return colors;
}

static int *allocate_cube(int *cube)
{
   window_t *win = current_window();
   if (win->gdriver->alloccolor) {
      (win->gdriver->begin) (win);
      int k = 0;
      for (int r=0; r<=255; r+=85)
         for (int g=0; g<=255; g+=85)
            for (int b=0; b<=255; b+=85) {
               cube[k++] =  (*win->gdriver->alloccolor)
                  (win, (real)r/255.0, (real)g/255.0, (real)b/255.0 );
            }
      (win->gdriver->end) (win);
      return cube;
   }
   return NIL;
}



static void color_draw_list(int xx, int y, at *l, int ncol, 
                            double minv, double maxv, int apart, int *colors)
{
   void (*setcolor)(struct window *, int);
   void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
   
   int len = length(l);
   int x = xx;
   window_t *win = current_window();
   if (maxv - minv == 0)
      RAISEFX("invalid scaling factor", NEW_NUMBER(0));
   if (ncol <= 0)
      RAISEFX("invalid number of columns", NEW_NUMBER(ncol));
   int nlin = (len + ncol - 1) / ncol;
  
  
   if (colors && win->gdriver->pixel_map &&
       (*win->gdriver->pixel_map) (win, NIL, xx, y, ncol, nlin, apart, apart)) {
      
      /* special pixel_map call */
      
      if (imagesize < sizeof(int) * ncol * nlin) {
         if (imagesize)
            free(image);
         imagesize = 0;
         image = malloc(sizeof(int) * ncol * nlin);
         if (!image)
            RAISEFX("no memory", NIL);
         imagesize = sizeof(int) * ncol * nlin;
      }
      unsigned int *im = image;
      
      while (CONSP(l)) {
         if (! NUMBERP(Car(l)))
            error(NIL, "not a number", Car(l));
         int v = (int)(64 * (Number(Car(l)) - minv) / (maxv - minv));
         l = Cdr(l);
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
         RAISEFX("driver does not support 'fill-rect'", NIL);
      if (!(setcolor = win->gdriver->setcolor))
         RAISEFX("driver does not support 'setcolor'", NIL);
      
      (*win->gdriver->begin) (win);
      nlin = ncol;
      while (CONSP(l)) {
         if (! NUMBERP(Car(l))) {
            (*setcolor) (win, win->color);
            (*win->gdriver->end) (win);
            RAISEFX("not a number", Car(l));
         }
         int v = (int)(64 * (Number(Car(l)) - minv) / (maxv - minv));
         if (v > 63)
            v = 63;
         if (v < 0)
            v = 0;
         if (colors)
            (*setcolor) (win, colors[v]);
         else
            (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
         (*fill_rect) (win, x, y, apart, apart);
         l = Cdr(l);
         (--nlin) ? (x += apart) : (x = xx, y += apart, nlin = ncol);
      }
      (*setcolor) (win, win->color);
      (*win->gdriver->end) (win);
   }
}

DX(xgray_draw_list)
{
   ARG_NUMBER(7);
   color_draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
                   AINTEGER(4), AREAL(5), AREAL(6), AINTEGER(7),
                   allocate_grays());
   return NIL;
}


DX(xcolor_draw_list)
{
   ARG_NUMBER(8);
   color_draw_list(AINTEGER(1), AINTEGER(2), ACONS(3),
                   AINTEGER(4), AREAL(5), AREAL(6), AINTEGER(7),
                   colors_from_int_matrix(APOINTER(8)) );
   return NIL;
}

int color_draw_idx(int x, int y, index_t *idx, 
                   real minv, real maxv, int apartx, int aparty, 
                   int *colors)
{
   window_t *win = current_window_no_error();
   if(win==NIL)
      return 1;
/*    if (maxv - minv == 0) */
/*       return 2; */

   int d1, d2, m1, m2;
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
   
   if (!colors)
      colors = allocate_grays();
   
   if (colors && win->gdriver->pixel_map &&
       (*win->gdriver->pixel_map) (win, NIL, x, y, d1, d2, apartx, aparty)) {

      /* VIA THE PIXEL MAP ROUTINE */
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
      unsigned int *im = image;
      
      if (idx->st->type == ST_FLOAT) {
         /* fast routine for flts.. */
         flt dm = minv;
         flt dv = maxv - minv;
         flt *data = IND_BASE(idx);
         int off2 = 0;
         for (int j = 0; j < d2; j++, off2 += m2) {
	    int off1 = off2;
	    for (int i = 0; i < d1; i++, off1 += m1) {
               int v = dv ? 64 * (data[off1] - dm) / dv : 32;
               if (v > 63)
                  v = 63;
               if (v < 0)
                  v = 0;
               *im++ = colors[v];
	    }
         }
      } else {
         /* generic routine */
         flt (*getf)(gptr,size_t) = storage_getf[idx->st->type];
         gptr data = IND_BASE(idx);
         flt dm = minv;
         flt dv = maxv - minv;
         int off2 = 0;
         for (int j = 0; j < d2; j++, off2 += m2) {
	    int off1 = off2;
	    for (int i = 0; i < d1; i++, off1 += m1) {
               flt w = (*getf)(data,off1);
               int v = dv ? 64 * (w - dm) / dv : 32;
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
      
   } else {
      /* VIA SMALL SQUARES WHEN NO PIXEL MAP CALL */
      void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
      void (*setcolor)(struct window *, int);
      
      if (!(fill_rect = win->gdriver->fill_rect)) {
          return 5;
      }
      if (!(setcolor = win->gdriver->setcolor)) {
         return 6;
      }
      
      gptr data = IND_BASE(idx);
      flt (*getf)(gptr,size_t) = storage_getf[idx->st->type];
      flt dm = minv;
      flt dv = maxv - minv;

      (*win->gdriver->begin) (win);
      
      int off2 = 0;
      for (int j = 0; j < d2; j++, off2 += m2) {
         int off1 = off2;
         int xx = x;
         for (int i = 0; i < d1; i++, off1 += m1) {
            flt w = (*getf)(data,off1);
            int v = dv ? 64 * (w - dm) / dv : 32;
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

int gray_draw_idx(int x, int y, index_t *idx, 
                  real minv, real maxv, int apartx, int aparty)
{
   return color_draw_idx(x, y, idx, minv, maxv, apartx, aparty, 0);
}

static void color_draw_matrix(int x, int y, at *p, 
                              real minv, real maxv, int apartx, int aparty, 
                              int *colors)
{
   ifn (INDEXP(p))
      error(NIL, "not an index", p);
   index_t *ind = Mptr(p);
   int error_flag = color_draw_idx(x, y, ind, minv, maxv, apartx, aparty, colors);
   
   if(error_flag)
      switch(error_flag) {
      case 1:
         current_window(); /* will be an error */
      case 2:
         error(NIL, "Illegal scaling factor", NIL);
      case 3:
         error(NIL, "1D or 2D index expected", p);
      case 4:
         error(NIL, "not enough memory", NIL);
      case 5:
         error(NIL, "driver does not support 'fill-rect'", NIL);
      case 6:
         error(NIL, "driver does not support 'setcolor'", NIL);
      }
}

DX(xcolor_draw_matrix)
{
   int apartx, aparty;
   at *q;
   if (arg_number == 6) {
      apartx = aparty = 1;
      q = APOINTER(6);
   } else {
      ARG_NUMBER(8);
      apartx = AINTEGER(6);
      aparty = AINTEGER(7);
      q = APOINTER(8);
   }
   
   at *p = APOINTER(3);
   ifn (INDEXP(p) && index_numericp(Mptr(p)))
      RAISEFX("not a numeric storage class", p);
   
   color_draw_matrix(AINTEGER(1), AINTEGER(2), p,
                     AREAL(4), AREAL(5),
                     apartx, aparty, 
                     colors_from_int_matrix(q));
   return NIL;
}

DX(xgray_draw_matrix)
{
   int apartx, aparty;
   if (arg_number == 5) {
      apartx = aparty = 1;
   } else {
      ARG_NUMBER(7);
      apartx = AINTEGER(6);
      aparty = AINTEGER(7);
   }
   
   at *p = APOINTER(3);
   ifn (INDEXP(p) && index_numericp(Mptr(p)))
      RAISEFX("not a numeric storage class", p);
   
   color_draw_matrix(AINTEGER(1), AINTEGER(2), p,
                     AREAL(4), AREAL(5),
                     apartx, aparty, 0);
   return NIL;
}




/* -------------------------------------------- */
/*     RGB DRAW MATRIX                          */
/* -------------------------------------------- */

int rgb_draw_idx(int x, int y, index_t *idx, int sx, int sy)
{
   /* Check window characteristics */
   window_t *win = current_window_no_error();
   if (! win)
      return 1;
   if (! win->gdriver->pixel_map)
      return 2;
      
   /* Check matrix characteristics */
   int m3;
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
   int d1 = idx->dim[1];
   int m1 = idx->mod[1];
   int d2 = idx->dim[0];
   int m2 = idx->mod[0];
   
   int truecolorp = 0;
   unsigned int red_mask, green_mask, blue_mask;
   if (win->gdriver->get_mask)
      truecolorp = (*win->gdriver->get_mask)(win, &red_mask, &green_mask, &blue_mask);
   
   if (!truecolorp) {
      /* DISPLAY REQUIRE DITHERING */

      /* Allocate color cube */
      int cube[64];
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
      unsigned char *im = (unsigned char*)image;
      gptr data = IND_BASE(idx);
      flt (*getf)(gptr,size_t) = storage_getf[idx->st->type];
      int off2 = 0;
      for (int j = 0; j < d2; j++, off2 += m2) 
         for (int zj=0; zj<sy; zj++) {
            int off1 = off2;
            for (int i = 0; i < d1; i++, off1 += m1) {
               int r = (*getf)(data,off1);
               int g = (*getf)(data,off1 + m3);
               int b = (*getf)(data,off1 + m3 + m3);
               for (int zi=0; zi<sx; zi++) {
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
      for (int c = 0; c <= 2; c++) {
         im = (unsigned char*)image;
         for (int j=0; j<d2; j++)
            for (int i=0; i<d1; i++) {
               short *im1 = (short*)im;
               /* discretize */
               short zj, zi = *im1;
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
      im = (unsigned char*)image;
      for (int j=0; j<d2; j++)
         for (int i=0; i<d1; i++) {
            /* code */
            short zi = (((*(short*)im)<<4)&0x30);
            zi = zi | ((im[2]<<2)&0xc) | (im[3]&0x3);
            *((int*)im) = cube[zi];
            im += sizeof(int);
         }
      /* Perform drawing */
      (*win->gdriver->begin) (win);      
      (*win->gdriver->pixel_map)(win, image, x, y, d1, d2, sx, sy);
      (*win->gdriver->end) (win);

   } else if (! (*win->gdriver->pixel_map)(win, NIL, x, y, d1, d2, sx, sy)) {
      /* ZOOM IS TOO STRONG: USE SMALL SQUARES */

      void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int) 
         = win->gdriver->fill_rect;
      void (*setcolor)(struct window *, int) 
         = win->gdriver->setcolor;
      int (*alloccolor)(struct window *, double, double, double) 
         = win->gdriver->alloccolor;
      
      ifn (alloccolor && setcolor && fill_rect ) {
         return 8;
      }
      /* Go looping */
      gptr data = IND_BASE(idx);
      flt (*getf)(gptr,size_t) = storage_getf[idx->st->type];
      (*win->gdriver->begin) (win);      
      int off2 = 0;
      for (int j = 0; j < d2; j++, off2 += m2) {
         int off1 = off2;
         int xx = x;
         for (int i = 0; i < d1; i++, off1 += m1) {
            flt r = (*getf)(data,off1) / 255.0;
            flt g = (*getf)(data,off1 + m3) / 255.0;
            flt b = (*getf)(data,off1 + m3 + m3) / 255.0;
            int v = (*alloccolor)(win, r, g, b);
            (*setcolor)(win, v);
            (*fill_rect) (win, xx, y, sx, sy);
            xx += sx;
         }
         y += sy;
      }
      (*setcolor)(win, win->color);
      (*win->gdriver->end) (win);

   } else {
      /* ZOOM SMALL ENOUGH: USE PIXEL MAP ROUTINE */

      /* Check pixel image buffer */
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
      unsigned int *im = (unsigned int*)image;
      /* Copy RGB into pixel image */
      if (idx->st->type==ST_UCHAR && 
          red_mask==0xff && green_mask==0xff00 && blue_mask==0xff0000) {
         unsigned char *data = IND_BASE(idx);
         int off2 = 0;
         for (int j = 0; j < d2; j++, off2 += m2) {
            int off1 = off2;
            for (int i = 0; i < d1; i++, off1 += m1) {
               uchar r = data[off1];
               uchar g = data[off1 + m3];
               uchar b = data[off1 + m3 + m3];
               *im++ = r | ((g | (b << 8)) << 8);
            }
         }
      } else if (idx->st->type==ST_UCHAR && 
                 red_mask==0xff0000 && green_mask==0xff00 && blue_mask==0xff) {
         unsigned char *data = IND_BASE(idx);
         int off2 = 0;
         for (int j = 0; j < d2; j++, off2 += m2) {
            int off1 = off2;
            for (int i = 0; i < d1; i++, off1 += m1) {
               uchar r = data[off1];
               uchar g = data[off1 + m3];
               uchar b = data[off1 + m3 + m3];
               *im++ = b | ((g | (r << 8)) << 8);
            }
         }
      } else  {
         /* Generic transcription routine */
         flt (*getf)(gptr,size_t) = storage_getf[idx->st->type];
         gptr data = IND_BASE(idx);
         int off2 = 0;
         for (int j = 0; j < d2; j++, off2 += m2) {
            int off1 = off2;
            for (int i = 0; i < d1; i++, off1 += m1) {
               flt r = (*getf)(data,off1) / 255.0;
               flt g = (*getf)(data,off1 + m3) / 255.0;
               flt b = (*getf)(data,off1 + m3 + m3) / 255.0;
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


static void rgb_draw_matrix(int x, int y, at *p, int sx, int sy)
{
    ifn (INDEXP(p))
       error(NIL, "not an index", p);
    index_t *ind = Mptr(p);
    int error_flag = rgb_draw_idx(x, y, ind, sx, sy);

    switch (error_flag) {
    case 0:
       break;
    case 1:
       current_window(); /* will be an error */
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
   int sx = 1;
   int sy = 1;
   if (arg_number != 3) {
      ARG_NUMBER(5);
      sx = AINTEGER(4);
      sy = AINTEGER(5);
      if (sx<1 || sy<1)
         RAISEFX("illegal zoom factor",NIL);
   }
   int x = AINTEGER(1);
   int y = AINTEGER(2);
   at *p = APOINTER(3);
   rgb_draw_matrix(x,y,p,sx,sy);
   return NIL;
}



/* -------------------------------------------- */
/*     RGB GRAB MATRIX                          */
/* -------------------------------------------- */

int rgb_grab_idx(int x, int y, index_t *idx)
{
   window_t *win = current_window_no_error();
   if(win==NULL)
      return 1;
   if (! win->gdriver->pixel_map)
      return 2;
   if (! win->gdriver->get_mask)
      return 21;

   unsigned int red_mask, green_mask, blue_mask;
   if (win->gdriver->get_mask)
      if (! (*win->gdriver->get_mask)(win, &red_mask, &green_mask, &blue_mask))
         return 22;
   
   int m3;
   if (idx->ndim == 3) {
      m3 = idx->mod[2];
      if (idx->dim[2] < 3) 
         return 3;
   } else if (idx->ndim == 2) {
      m3 = 0;
   } else
      return 4;
   int d1 = idx->dim[1];
   int m1 = idx->mod[1];
   int d2 = idx->dim[0];
   int m2 = idx->mod[0];
   
   if (imagesize < sizeof(int) * d1 * d2) {
      if (imagesize)
         free(image);
      imagesize = 0;
      image = malloc(sizeof(int) * d1 * d2);
      if (! image) 
         return 7;
      imagesize = sizeof(int) * d1 * d2;
   }
   unsigned int *im = image;
   (win->gdriver->begin) (win);
   (win->gdriver->get_image) (win, im, x, y, d1, d2);
   (win->gdriver->end) (win);
   
   gptr data = IND_BASE(idx);
   void (*setf)(gptr,size_t,flt) = storage_setf[idx->st->type];
   int off2 = 0;
   for (int j = 0; j < d2; j++, off2 += m2) {
      int off1 = off2;
      for (int i = 0; i < d1; i++, off1 += m1) {
         unsigned int p = *im++;
         flt r = (255 * (p & red_mask)) / (double)red_mask;
         flt g = (255 * (p & green_mask)) / (double)green_mask;
         flt b = (255 * (p & blue_mask)) / (double)blue_mask;
         if (idx->ndim>=3) {
            (*setf)(data, off1, r);
            (*setf)(data, off1+m3, g);
            (*setf)(data, off1+m3+m3, b);
         } else {
            (*setf)(data, off1, (5*r+9*g+2*b)/16.0 );
         }
      }
   }
   return 0;
}

static void rgb_grab_matrix(int x, int y, at *p)
{
   ifn (INDEXP(p))
      error(NIL, "not an index", p);
   index_t *ind = Mptr(p);
   int error_flag = rgb_grab_idx(x, y, ind);

   if(error_flag)
      switch(error_flag) {
      case 1:
         current_window(); /* will be an error */
      case 2:
         error(NIL,"Graphic driver does not support get-image",NIL);
      case 21:
         error(NIL,"Graphic driver does not support get-mask",NIL);
      case 22:
         error(NIL,"This function only works on truecolor displays",NIL);
      case 3:
         error(NIL,"Last dimension shouls be three (or more)",p);
      case 4:
         error(NIL,"2D or 3D index expected",p);
      case 7:
         error(NIL, "not enough memory", NIL);
      }
}

DX(xrgb_grab_matrix)
{
   ARG_NUMBER(3);
   at *p = APOINTER(3);
   rgb_grab_matrix(AINTEGER(1),AINTEGER(2),p);
   return p;
}


/* ------------------------------------ */
/* OGRE SPEEDUP                         */
/* ------------------------------------ */     



static void getrect(at *pp, double *x, double *y, double *w, double *h)
{
   at *p = pp;
   if (CONSP(p) && NUMBERP(Car(p))) {
      *x = Number(Car(p));
      p = Cdr(p);
      if (CONSP(p) && NUMBERP(Car(p))) {
         *y = Number(Car(p));
         p = Cdr(p);
         if (CONSP(p) && NUMBERP(Car(p))) {
            *w = Number(Car(p));
            p = Cdr(p);
            if (CONSP(p) && NUMBERP(Car(p))) {
               *h = Number(Car(p));
               p = Cdr(p);
               if (! p)
                  return;
            }
         }
      }
   }
   error(NIL,"illegal rectangle",pp);
}

static at *makerect(int x, int y, int w, int h)
{
   return new_cons(NEW_NUMBER(x),
                   new_cons(NEW_NUMBER(y),
                            new_cons(NEW_NUMBER(w),
                                     new_cons(NEW_NUMBER(h),
                                              NIL))));
}


DX(xpoint_in_rect)
{
   ARG_NUMBER(3);
   double xx = AREAL(1);
   double yy = AREAL(2);
   double x, y, w, h;
   getrect(ALIST(3),&x,&y,&w,&h);
   
   if (xx>=x && yy>=y && xx<x+w && yy<y+h)
      return t();
   else
      return NIL;
}

DX(xrect_in_rect)
{
   ARG_NUMBER(2);
   double xx,yy,ww,hh,x,y,w,h;
   getrect(ALIST(1),&xx,&yy,&ww,&hh);
   getrect(ALIST(2),&x,&y,&w,&h);
   w += x;
   h += y;
   ww += xx-1;
   hh += yy-1;
   
   if ( xx>=x && yy>=y && xx<w && yy<h &&
        ww>=x && hh>=y && ww<w && hh<h  )
      return t();
   else
      return NIL;
}


#define TLMAX(u,v) (((u)>(v))?(u):(v))
#define TLMIN(u,v) (((u)<(v))?(u):(v))
  
DX(xcollide_rect)
{
   ARG_NUMBER(2);
   double x1,y1,w1,h1,x2,y2,w2,h2;
   getrect(ALIST(1),&x1,&y1,&w1,&h1);
   getrect(ALIST(2),&x2,&y2,&w2,&h2);
   
   double x = TLMAX(x1,x2);
   double y = TLMAX(y1,y2);
   double w = TLMIN(x1+w1,x2+w2)-x;
   double h = TLMIN(y1+h1,y2+h2)-y;
  
   
   if (w>0 && h>0)
      return makerect(x,y,w,h);
   else
      return NIL;
}


DX(xbounding_rect)
{
   ARG_NUMBER(2);
   double x1,y1,w1,h1,x2,y2,w2,h2;
   getrect(ALIST(1),&x1,&y1,&w1,&h1);
   getrect(ALIST(2),&x2,&y2,&w2,&h2);

   double x = TLMIN(x1,x2);
   double y = TLMIN(y1,y2);
   double w = TLMAX(x1+w1,x2+w2)-x;
   double h = TLMAX(y1+h1,y2+h2)-y;

   return makerect(x,y,w,h);
}


DX(xexpand_rect)
{
   ARG_NUMBER(3);
   double x,y,w,h;
   getrect(ALIST(1),&x,&y,&w,&h);
   double bx = AREAL(2);
   double by = AREAL(3);

   return makerect(x-bx,y-by,w+2*bx,h+2*by);
}


DX(xdraw_round_rect)
{
   int r = 3;
   if (arg_number==5)
      r = AINTEGER(5);
   else
      ARG_NUMBER(4);

   int left = AINTEGER(1);
   int top  = AINTEGER(2);
   int right  = left + AINTEGER(3);
   int bottom = top + AINTEGER(4);
   int almostleft = left+r;
   int almosttop = top+r;
   int almostright = right-r;
   int almostbottom = bottom-r;

   window_t *win = current_window();
   if (win->gdriver->draw_arc && win->gdriver->draw_line) {
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

   } else if ( win->gdriver->draw_line ) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->draw_rect)(win,left,top,right-left,bottom-top);
      (*win->gdriver->end) (win);  
   } else
      RAISEFX("driver does not support 'fill-polygon'", NIL);
   return NIL;
}



DX(xfill_round_rect)
{
   int r = 3;
   if (arg_number==5)
      r = AINTEGER(5);
   else
      ARG_NUMBER(4);
  
   int left = AINTEGER(1);
   int top  = AINTEGER(2);
   int right  = left + AINTEGER(3);
   int bottom = top + AINTEGER(4);
   int almostleft = left+r;
   int almosttop = top+r;
   int almostright = right-r;
   int almostbottom = bottom-r;
   int almostwidth = almostright-almostleft;
  
   window_t *win = current_window();
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
      RAISEFX("driver does not support 'fill-polygon'", NIL);
   
   return NIL;
}



DY(ygsave)
{
   if (! CONSP(ARG_LIST))
      RAISEFX("syntax error", NIL);
   
   window_t *win = current_window();
   int oldcolor = win->color;
   at *oldfont = win->font;
   short oldx = win->clipx;
   short oldy = win->clipy;
   short oldw = win->clipw;
   short oldh = win->cliph;
   short oldl = win->linestyle;

   struct lush_context mycontext;
   int errorflag = 0;
   at *ans = NIL;
   MM_ANCHOR(oldfont);
   context_push(&mycontext);
   if (sigsetjmp(context->error_jump, 1))
      errorflag = 1;
   else
      ans = progn(ARG_LIST);
   context_pop();
  
   if (win->used && WINDOWP(win->backptr)) {
      if (oldcolor!=win->color) {
         (*win->gdriver->begin) (win);
         if (win->gdriver->setcolor) {
            (*win->gdriver->setcolor) (win, oldcolor);
            win->color = oldcolor;
         }
         (*win->gdriver->end) (win);
      }
      
      if (oldfont!=win->font)
         if (win->gdriver->setfont && STRINGP(oldfont)) {
            (*win->gdriver->begin) (win);
            (*win->gdriver->setfont)(win, String(oldfont));
            (*win->gdriver->end) (win);
            win->font = oldfont;
         }
      
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
      if (oldl!=win->linestyle)
         if (win->gdriver->set_linestyle) {
            (*win->gdriver->begin) (win);
            (*win->gdriver->set_linestyle) (win, oldl);
            (*win->gdriver->end) (win);
            win->linestyle = oldl;
         }
   }
   if (errorflag)
      siglongjmp(context->error_jump, -1L);
   return ans;
}


DX(xaddclip)
{
   ARG_NUMBER(1);
   window_t *win = current_window();

   double x1, y1, w1, h1;
   getrect(ALIST(1),&x1,&y1,&w1,&h1);
   
   if (win->gdriver->clip) {
      if (win->clipw || win->cliph) {
         int x = TLMAX(x1,win->clipx);
         int y = TLMAX(y1,win->clipy);
         int w = TLMIN(x1+w1,win->clipx+win->clipw)-x;
         int h = TLMIN(y1+h1,win->clipy+win->cliph)-y;
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
         win->clipw = w1;
         win->cliph = h1;
      }
   }
   if (win->clipw>0 && win->cliph>0)
      return new_cons(NEW_NUMBER(win->clipx),
                      new_cons(NEW_NUMBER(win->clipy),
                               new_cons(NEW_NUMBER(win->clipw),
                                        new_cons(NEW_NUMBER(win->cliph), NIL))));
   return NIL;
}
#undef TLMAX
#undef TLMIN

/* ------------------------------------ */
/* INITIALISATION                       */
/* ------------------------------------ */     

class_t *window_class;

void init_graphics(void)
{
   mt_window = MM_REGTYPE("window", sizeof(window_t),
                          clear_window, mark_window, finalize_window);
   /* WINDOW */
   window_class = new_builtin_class(NIL);
   window_class->dispose = (dispose_func_t *)window_dispose;
   window_class->name = window_name;
   class_define("Window", window_class);
   at_window = var_define("window");

   /* RELEASE 1 */
   dx_define("gdriver",xgdriver);
   dx_define("xsize",xxsize);
   dx_define("ysize",xysize);
   dx_define("font",xfont);
   dx_define("cls",xcls);
   dx_define("draw-line",xdraw_line);
   dx_define("draw-rect",xdraw_rect);
   dx_define("draw-circle",xdraw_circle);
   dx_define("fill-rect",xfill_rect);
   dx_define("fill-circle",xfill_circle);
   dx_define("draw-text",xdraw_text);
   dx_define("rect-text",xrect_text);
   /* RELEASE 2 */
   dx_define("fill-polygon",xfill_polygon);
   dx_define("gspecial",xgspecial);
   dx_define("hilite",xhilite);
   dx_define("clip",xclip);
   dx_define("color",xcolor);
   dx_define("alloccolor",xalloccolor);
   /* RELEASE 3 */
   dx_define("draw-arc",xdraw_arc);
   dx_define("fill-arc",xfill_arc);
   /* BATCHING */
   dy_define("graphics-batch",ygraphics_batch);
   dx_define("graphics-sync",xgraphics_sync);
   /* COMPLEX COMMANDS */
   dx_define("draw-value",xdraw_value);
   dx_define("draw-list",xdraw_list);
   dx_define("gray-draw-list",xgray_draw_list);
   dx_define("color-draw-list",xcolor_draw_list);
   dx_define("gray-draw-matrix",xgray_draw_matrix);
   dx_define("color-draw-matrix",xcolor_draw_matrix);
   dx_define("rgb-draw-matrix",xrgb_draw_matrix);  
   /* LUSH */
   dx_define("rgb-grab-matrix",xrgb_grab_matrix);  
   dx_define("linestyle",xlinestyle);
   /* OGRE SPEEDUP */
   dx_define("point-in-rect",xpoint_in_rect);
   dx_define("rect-in-rect",xrect_in_rect);
   dx_define("collide-rect",xcollide_rect);
   dx_define("bounding-rect",xbounding_rect);
   dx_define("expand-rect",xexpand_rect);
   dx_define("draw-round-rect",xdraw_round_rect);
   dx_define("fill-round-rect",xfill_round_rect);
   dy_define("gsave",ygsave);
   dx_define("addclip",xaddclip);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
