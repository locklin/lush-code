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
  TL3: LYB YLC 1/88
  graphics.c
  Device independant graphics functions
  $Id: graphics.c,v 1.1.1.1 2002-04-16 17:37:38 leonb Exp $
  ***********************************************************************/


#include "header.h"
#include "graphics.h"

static at *window;
static at *at_event;
static at *at_idle;


/* ------------------------------------ */
/* EVENT MANAGEMENT                     */
/* ------------------------------------ */     


static struct event *event_buffer;
static int event_put, event_get;

#define EVBUFMASK 0xff
#define EVBUFSIZE 256

static int   evshift;
static int   evctrl;
static char *evdesc = NULL;



static at * 
event_to_list(int event, int xdown, int ydown, int xup, int yup, char *desc)
{
  if (event == EVENT_NONE)
    return NIL;
  
  
  /* events that do not update evshift and evcontrol */

  evdesc = desc;
  if (event == EVENT_MOUSE_UP) 
    return (cons(named("mouse_up"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown),
			   cons(NEW_NUMBER(xup),
				cons(NEW_NUMBER(yup), NIL))))));
    
  if (event == EVENT_MOUSE_DRAG) 
    return (cons(named("mouse_drag"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown),
			   cons(NEW_NUMBER(xup),
				cons(NEW_NUMBER(yup), NIL))))));

  if (event == EVENT_RESIZE)
    return (cons(named("resize"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));

  if (event == EVENT_DELETE) 
    return (cons(named("delete"),NIL));
    
  if (event == EVENT_SENDEVENT)
    return (cons(named("sendevent"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown),NIL))));

  if (event >= EVENT_ASCII_MIN && event <= EVENT_ASCII_MAX) {
    static char keyevent[2];
    keyevent[0] = EVENT_TO_ASCII(event);
    keyevent[1] = 0;
    evdesc = keyevent;
    return (cons(new_string(keyevent),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
  }


  /* events that update evshift and evcontrol */

  evshift = (xup ? 1 : 0);
  evctrl =  (yup ? 1 : 0);
  if (event == EVENT_MOUSE_DOWN)
    return (cons(named("mouse_down"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
    
  if (event == EVENT_HELP)
    return (cons(named("help"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));

  if (event == EVENT_ARROW_UP)
    return (cons(named("arrow_up"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
    
  if (event == EVENT_ARROW_RIGHT)
    return (cons(named("arrow_right"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
    
  if (event == EVENT_ARROW_DOWN)
    return (cons(named("arrow_down"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
    
  if (event == EVENT_ARROW_LEFT)
    return (cons(named("arrow_left"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
    
  if (event == EVENT_FKEY)
    return (cons(named("fkey"),
                 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));

  evshift = evctrl = 0;
  return (cons(NEW_NUMBER(event),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
}




void 
process_pending_events(void)
{
  int old_event_get;
  at *q;
  
again:
  call_sync_trigger();
  while (event_get != event_put) 
    {
      old_event_get = event_get;
      /* CALLS EVENT-HOOK */
      q = var_get(at_event);
      if (! (q && (q->flags&X_FUNCTION))) {
	UNLOCK(q);
      } else {
	UNLOCK(q);
	q = apply(at_event,NIL);
	UNLOCK(q);
      }
      if (old_event_get == event_get) 
	return;
    }
  call_sync_trigger();
  if (event_get != event_put) 
    goto again;
  
  /* CALLS IDLE-HOOK */
  q = var_get(at_idle);
  if (! (q && (q->flags&X_FUNCTION))) {
    UNLOCK(q);
  } else {
    UNLOCK(q);
    q = apply(at_idle,NIL);
    UNLOCK(q);
  }
  call_sync_trigger();
  if (event_get != event_put) 
    goto again;
}



void
enqueue_event(at *handler, int event, 
              int xd, int yd, int xu, int yu)
{
  enqueue_eventdesc(handler,event,xd,yd,xu,yu,NULL);
}



void
enqueue_eventdesc(at *handler, int event, 
                  int xd, int yd, int xu, int yu, char *desc)
{
  int last_event_put;
  int new_event_put;
  struct event *ev;
  
  last_event_put = (event_put-1)&EVBUFMASK;
  ev = &event_buffer[last_event_put];
  
  if (   last_event_put != event_get
      && event_put != event_get
      && event == EVENT_MOUSE_DRAG
      && ev->code == EVENT_MOUSE_DRAG
      && ev->handler == handler ) 
    {
      ev = &event_buffer[last_event_put];
      new_event_put = event_put;
    }
  else
    {
      ev = &event_buffer[event_put];
      new_event_put = (event_put+1)&EVBUFMASK;
    }
  
  if (new_event_put == event_get) /* no room */
    return;
  event_put = new_event_put;
  ev->handler = handler;
  ev->code = event;
  ev->xd = xd;
  ev->yd = yd;
  ev->xu = xu;
  ev->yu = yu;
  ev->desc = desc;
}



static void
remove_one_event(int ev)
{
  int next_ev;
  
  while (ev!=event_get) {
    next_ev = (ev-1)&EVBUFMASK;
    event_buffer[ev] = event_buffer[next_ev];
    ev = next_ev;
  }
  event_get = (event_get+1)&EVBUFMASK;
}


static void
remove_events(at *h)
{
  int ev;
  
  ev = event_get;
  while (ev != event_put) {
    if (event_buffer[ev].handler==h)
      remove_one_event(ev);
    ev = (ev+1)&EVBUFMASK;
  }
}





/* ------------------------------------ */
/* CLASS FUNCTIONS                      */
/* ------------------------------------ */     

static void 
window_dispose(at *p)
{
  register struct window *win;
  
  win = p->Object;
  UNLOCK(win->font);
  if (win->eventhandler) {
    remove_events(win->eventhandler);
    unprotect(p);
  }
  UNLOCK(win->eventhandler);
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

static at *window;

struct window *
current_window(void)
{
  register at *w;
  register struct window *ww;
  
  w = var_get(window);
  if (EXTERNP(w,&window_class))
    ww = w->Object;
  else
    error("window", "symbol value is not a window", w);
  
  UNLOCK(w);
  return ww;
}




/* ------------------------------------ */
/* EVENT USER FUNCTIONS                 */
/* ------------------------------------ */     



/* Set the event recipient? */

DX(xseteventhandler)
{
  register struct window *win;
  register at *w,*p,*q;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  
  w = APOINTER(1);
  if (EXTERNP(w,&window_class))
    win = w->Object;
  else
    error("window", "symbol value is not a window", w);
  
  if ((p = win->eventhandler)) {
    win->eventhandler = NIL;
    remove_events(p);
    /* interactive windows shall not dissappear on gc call */
    unprotect(w);
  }
  q = APOINTER(2);
  if ((win->eventhandler = q)) {
    protect(w);
  }
  LOCK(q);
  return p;
}

/* Calls event hook ... */

DX(xprocess_pending_events)
{
  ARG_NUMBER(0);
  process_pending_events();
  return NIL;
}


/* Event subs */


DX(xsendevent)
{
  struct window *win;
  int x,y;
  
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  x = AINTEGER(1);
  y = AINTEGER(2);
  win = current_window();
  (*win->gdriver->begin) (win);
  (*win->gdriver->end) (win);
  if (!win->eventhandler)
    error(NIL,"No event handler associated to the current window",NIL);
  enqueue_event(win->eventhandler,EVENT_SENDEVENT,x,y,x,y);
  return NIL;
}



DX(xtestevent)
{
  register struct window *win;
  at *check;
  struct event *p;
  int ev;
  
  ARG_NUMBER(0);
  win = current_window();
  check = win->eventhandler;           
  if (check)
    {
      call_sync_trigger();
      ev = event_get;
      while (ev != event_put) 
	{
	  p = &event_buffer[ev];
	  if (p->handler == check)
	    return event_to_list(p->code,p->xd,p->yd,p->xu,p->yu,p->desc);
	  ev = (ev+1)&EVBUFMASK;
	}
    }
  return NIL;
}


DX(xcheckevent)
{
  at *check;
  if (arg_number==0) 
    {				/* (checkevent) return EVENT for this WINDOW */
      struct window *win = current_window();	
      check = win->eventhandler;           
      if (!check)
	return NIL;
    }
  else 
    {				/* (checkevent handler) returns EVENT */
      ARG_NUMBER(1);
      ARG_EVAL(1);
      check = APOINTER(1);	
    }
  call_sync_trigger();
  if (!check) 
    {				/* (checkevent ()) return HANDLER */
      if (event_get != event_put)
	check = event_buffer[event_get].handler;
      LOCK(check);
      return check;
    }
  else 
    {
      int ev = event_get;
      int event = EVENT_NONE;
      int xdown = 0, ydown = 0, xup = 0, yup = 0;
      char *desc = NULL;
      while (ev != event_put) {
	struct event *p = &event_buffer[ev];
	if (p->handler == check) {
	  event = p->code;
	  xdown = p->xd;
	  xup   = p->xu;
	  ydown = p->yd;
	  yup   = p->yu;
          desc  = p->desc;
	  remove_one_event(ev);
	  break;
	}
	ev = (ev+1)&EVBUFMASK;
      }
      return event_to_list(event,xdown,ydown,xup,yup,desc);
    }
}


DX(xwaitevent)
{
  at *q;
  ARG_NUMBER(0);
  call_sync_trigger();
  if (event_get == event_put)
    {
      q = var_get(at_idle); 	/* CALLS IDLE_HOOK */
      if (! (q && (q->flags&X_FUNCTION))) {
	UNLOCK(q);
      } else {
	UNLOCK(q);
	q = apply(at_idle,NIL);
	UNLOCK(q);
      }
      while(event_get == event_put) {
	wait_trigger();
	call_sync_trigger();
#if defined(UNIX) || defined(WIN32)
	if (break_attempt)
	  break;
#endif
      }
      CHECK_MACHINE("on");
    }
  q = event_buffer[event_get].handler;
  LOCK(q);
  return q;
}



DX(xeventinfo)
{
  ARG_NUMBER(0);
  return cons(new_string(evdesc ? evdesc : "None"),
              cons((evshift ? true() : NIL),
                   cons((evctrl ? true() : NIL),
                        NIL )));
}


/* ------------------------------------ */
/* DRIVER FUNCTIONS (RELEASE 1)         */
/* ------------------------------------ */     



/* What is the current driver? */

DX(xgdriver)
{
  register struct window *win;
  
  ARG_NUMBER(0);
  win = current_window();
  return new_safe_string(win->gdriver->name);
}

/* User functions: minimal set */


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
  
  win = current_window();
  if (!win->gdriver->clip)
    error(NIL, "this driver does not support 'clip'", NIL);
  
  if (arg_number) {
    ALL_ARGS_EVAL;
    if (arg_number == 1 && !APOINTER(1)) {
      (*win->gdriver->begin) (win);
      (*win->gdriver->clip) (win, 0, 0, 0, 0);
      (*win->gdriver->end) (win);
      win->clipw = win->cliph = 0;
    } else {
      ARG_NUMBER(4);
      x = AINTEGER(1);
      y = AINTEGER(2);
      w = AINTEGER(3);
      h = AINTEGER(4);
      (*win->gdriver->begin) (win);
      (*win->gdriver->clip) (win, x, y, w, h);
      (*win->gdriver->end) (win);
      win->clipx = x;
      win->clipy = y;
      win->clipw = w;
      win->cliph = h;
    }
  }
  if (win->clipw || win->cliph)
    return cons(NEW_NUMBER(win->clipx),
		cons(NEW_NUMBER(win->clipy),
		     cons(NEW_NUMBER(win->clipw),
			  cons(NEW_NUMBER(win->cliph), NIL))));
  else
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




/* ------------------------------------ */
/* COMPLEX COMMANDS                     */
/* ------------------------------------ */     


/* static structure for pixel_map and hinton_map... */

static int *image = NULL;
static unsigned int imagesize = 0;

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
      image = tl_malloc(sizeof(int) * len);
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

static int *
pixels_from_imatrix(at *p)
{
  struct array *arr;
  int i;
  int *f;

  if (! EXTERNP(p,&imatrix_class))
    error(NIL,"Not an imatrix",p);
  arr = p->Object;
  if (arr->ndim!=1 || arr->dim[0]!=64)
    error(NIL,"Not a 64 elements vector",p);

  f = arr->data;
  for( i=0; i<64; i++ ) {
    colors[i] = *f;
    f += arr->modulo[0];
  }
  return colors;
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
      image = tl_malloc(sizeof(int) * ncol * nlin);
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
		  pixels_from_imatrix(APOINTER(8)) );
  return NIL;
}




static void 
color_draw_matrix(int xx, int y, register at *p, 
		  double minv, double maxv, 
		  int apartx, int aparty, int *colors)
{
  register struct window *win;
  register int x, i, j, v;
  float w, dm, dv;
  struct array *mat;
  int d1, d2, m1, m2, mode;
  int *im;
  void (*setcolor)(struct window *, int);
  void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);
  
  /* check for a matrix */
  if (p->Class == &pmatrix_class)
    mode = 1;
  else if (p->Class == &matrix_class)
    mode = 2;
  else if (p->Class == &dmatrix_class)
    mode = 3;
  else if (p->Class == &imatrix_class)
    mode = 4;
  else if (p->Class == &bmatrix_class)
    mode = 5;
  else if (p->Class == &smatrix_class)
    mode = 6;
  else
    error(NIL, "cannot display this array", p);
  
  mat = p->Object;
  if (mat->ndim == 1) {
    d1 = mat->dim[0];
    m1 = mat->modulo[0];
    d2 = 1;
    m2 = 0;
  } else if (mat->ndim == 2) {
    d1 = mat->dim[1];
    m1 = mat->modulo[1];
    d2 = mat->dim[0];
    m2 = mat->modulo[0];
  } else
    error(NIL, "1d or 2d matrix expected", p);
  
  
  win = current_window();
  if (maxv - minv == 0)
    error(NIL, "Illegal scaling factor", NIL);
  
  dm = (float)(minv);
  dv = (float)(maxv - minv);
  
  if (colors && win->gdriver->pixel_map &&
      (*win->gdriver->pixel_map) (win, NIL, xx, y, d1, d2, apartx, aparty)) {
    
    /* if special routine */
    
    if (imagesize < sizeof(int) * d1 * d2) {
      if (imagesize)
	free(image);
      imagesize = 0;
      image = tl_malloc(sizeof(int) * d1 * d2);
      if (!image)
	error(NIL, "no memory", NIL);
      imagesize = sizeof(int) * d1 * d2;
    }
    im = image;
    
    switch (mode) {
      
    case 1:			/* packed matrix */
      {
	register unsigned char *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)unpack(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;

    case 2:			/* matrix of flt */
      {
	register flt *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)Ftor(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;


    case 3:			/* matrix of reals */
      {
	register real *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;

    case 4:			/* matrix of integers */
      {
	register int *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;

    case 5:			/* matrix of bytes */
      {
	register unsigned char *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;

    case 6:			/* matrix of shorts */
      {
	register short *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    *im++ = colors[v];
	  }
	}
      }
      break;
      
    default:
      error("graphics.c/gray-draw-matrix","internal error: unhandled format",NIL);
      
    }

    (win->gdriver->begin) (win);
    (win->gdriver->pixel_map) (win, image, xx, y, d1, d2, apartx, aparty);
    (win->gdriver->end) (win);
    
    
  } else {
    
    /* default routine */
    
    /* check for functions */
    
    if (!(fill_rect = win->gdriver->fill_rect))
      error(NIL, "'fill-rect' unsupported", NIL);
    if (!(setcolor = win->gdriver->setcolor))
      error(NIL, "'setcolor' unsupported", NIL);
    
    (*win->gdriver->begin) (win);
    
    switch(mode) {

    case 1:
      {
	register unsigned char *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)unpack(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    case 2:
      {
	register flt *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)Ftor(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    case 3:
      {
	register real *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    case 4:
      {
	register int *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    case 5:
      {
	register unsigned char *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    case 6:
      {
	register short *p1, *p2;
	p2 = mat->data;
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  x = xx;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    w = (float)(*p1);
	    v = (int)(64 * (w - dm) / dv);
	    if (v > 63)
	      v = 63;
	    if (v < 0)
	      v = 0;
	    if (colors)
	      (*setcolor) (win, colors[v]);
	    else
	      (*setcolor) (win, COLOR_SHADE((real) v / 63.0));
	    (*fill_rect) (win, x, y, apartx, aparty);
	    x += apartx;
	  }
	  y += aparty;
	}
      }
      break;

    default:
      error("graphics.c/gray-draw-matrix","internal error: unhandled format",NIL);
      
    }
    
    (*setcolor) (win, win->color);
    (*win->gdriver->end) (win);
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
		    pixels_from_imatrix(q));
  return NIL;
}






static void 
draw_pixel_matrix(int xx, int y, at *p, int apartx, int aparty, 
		  int *colors, int ncolors)
{
  register struct window *win;
  register int x, i, j;
  struct array *mat;
  int d1, d2, m1, m2;
  int *im, *p1, *p2;
  void (*setcolor)(struct window *, int);
  void (*fill_rect)(struct window *, int, int, unsigned int, unsigned int);

  if  (! EXTERNP(p, &imatrix_class))
    error(NIL,"Not an imatrix",p);

  mat = p->Object;
  if (mat->ndim == 1) {
    d1 = mat->dim[0];
    m1 = mat->modulo[0];
    d2 = 1;
    m2 = 0;
  } else if (mat->ndim == 2) {
    d1 = mat->dim[1];
    m1 = mat->modulo[1];
    d2 = mat->dim[0];
    m2 = mat->modulo[0];
  } else
    error(NIL, "1d or 2d matrix expected", p);

  win = current_window();
  
  if (win->gdriver->pixel_map &&
      (*win->gdriver->pixel_map) (win, NIL, xx, y, d1, d2, apartx, aparty)) {
    
    /* if special routine */
    
    if (imagesize < sizeof(int) * d1 * d2) {
      if (imagesize)
	free(image);
      imagesize = 0;
      image = tl_malloc(sizeof(int) * d1 * d2);
      if (!image)
	error(NIL, "no memory", NIL);
      imagesize = sizeof(int) * d1 * d2;
    }
    im = image;
    p2 = mat->data;
    if (colors)
      {
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1) {
	    int c = *p1;
	    *im++ = ((c>=ncolors) ? colors[0] : colors[c]);
	  }
	} 
      }
    else
      {
	for (j = 0; j < d2; j++, p2 += m2) {
	  p1 = p2;
	  for (i = 0; i < d1; i++, p1 += m1)
	    *im++ = *p1;
	}
      }
    (win->gdriver->begin) (win);
    (win->gdriver->pixel_map) (win, image, xx, y, d1, d2, apartx, aparty);
    (win->gdriver->end) (win);
    
  } else {
    
    /* default routine */
    
    /* check for functions */
    
    if (!(fill_rect = win->gdriver->fill_rect))
      error(NIL, "'fill-rect' unsupported", NIL);
    if (!(setcolor = win->gdriver->setcolor))
      error(NIL, "'setcolor' unsupported", NIL);
    
    (*win->gdriver->begin) (win);
    
    p2 = mat->data;
    for (j = 0; j < d2; j++, p2 += m2) {
      p1 = p2;
      x = xx;
      for (i = 0; i < d1; i++, p1 += m1) {
	int c = *p1;
	if (colors)
	  (*setcolor) (win, ((c>=ncolors) ? colors[0] : colors[c]));
	else
	  (*setcolor) (win, c);
	(*fill_rect) (win, x, y, apartx, aparty);
	x += apartx;
      }
      y += aparty;
    }
    (*setcolor) (win, win->color);
    (*win->gdriver->end) (win);
  }
}



DX(xdraw_pixel_matrix)
{
  int sx,sy;
  int *colors, ncolors;
  ALL_ARGS_EVAL;
  if (arg_number==6) {
    sx = AINTEGER(4);
    sy = AINTEGER(5);
    ncolors = 0;
    colors = 0;
    if (APOINTER(6))
      colors = get_nr0_ivector(APOINTER(6),&ncolors);
  } else if (arg_number==5) {
    sx = AINTEGER(4);
    sy = AINTEGER(5);
    colors = 0;
    ncolors = 0;
  } else if (arg_number==3) {
    sx = sy = 1;
    colors = 0;
    ncolors = 0;
  } else {
    ARG_NUMBER(-1);
    return NIL;
  }
  draw_pixel_matrix(AINTEGER(1),AINTEGER(2),APOINTER(3), 
		    sx, sy, colors, ncolors);
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
  event_buffer = tl_malloc( sizeof(struct event)*EVBUFSIZE );
  event_put = event_get = 0;
  if  (! event_buffer)
    error(NIL,"Memory exhausted",NIL);

  class_define("WINDOW",&window_class );
  
  window = var_define("window");
  at_event  = var_define("event-hook");
  at_idle  = var_define("idle-hook");

  /* EVENTS */
  dx_define("set_event_handler",xseteventhandler);
  dx_define("process_pending_events",xprocess_pending_events);
  dx_define("sendevent",xsendevent);
  dx_define("testevent",xtestevent);
  dx_define("checkevent",xcheckevent);
  dx_define("waitevent",xwaitevent);
  dx_define("eventinfo",xeventinfo);
	
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
  dx_define("draw_pixel_matrix",xdraw_pixel_matrix);

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
