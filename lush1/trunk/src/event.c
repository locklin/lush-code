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
 * $Id: event.c,v 1.1 2002-08-02 15:08:49 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "graphics.h"

static at *at_event;
static at *at_idle;
static at *head;
static at *tail;

/* From UNIX.C */
extern void os_block_async_poll(void);
extern void os_unblock_async_poll(void);
extern void os_setup_async_poll(int*fds, int nfds, void(*f)(void));
extern int  os_wait(int* fds, int nfds, int console, unsigned long ms);
extern void os_curtime(int *sec, int *msec);


/* ------------------------------------ */
/* EVENT QUEUE                          */
/* ------------------------------------ */     


static void *
ev_hndl(at *q)
{
  if ( CONSP(q) && GPTRP(q->Car) &&
       CONSP(q->Cdr) && q->Cdr->Cdr )
    return q->Car->Gptr;
  return 0;
}

static void *
ev_remove(at *pp, at *p)
{
  if (pp->Cdr != p)
    error(NIL,"Internal error", NIL);
  pp->Cdr = p->Cdr;
  p->Cdr = NIL;
  if (tail == p)
    tail = pp;
  UNLOCK(p);
}

static void
ev_finalizer(at *handler, void *arg)
{
  at *pp = head;
  if (CONSP(pp))
    {
      at *p = pp->Cdr;
      while (CONSP(p)) 
        {
          void *hndl = ev_hndl(p->Car);
          if ( hndl == 0 || hndl == (gptr)handler ) 
            ev_remove(pp, p);
          else 
            pp = p;
          p = pp->Cdr;
        }
    }
}


void
ev_add(at *handler, at *event, 
       const char *desc, int mods)
{
  if (handler && event)
    {
      at *d = ((desc)?cons(new_gptr(desc),NEW_NUMBER(mods)):NIL);
      at *p = cons(new_gptr(handler),cons(d,event));
      LOCK(event);
      add_finalizer(handler, event_finalizer, 0);
      tail->Cdr = cons(p,NIL);
      tail = tail->Cdr;
    }
}

static const char *evdesc = 0;
static const int evmods = 0;

void
ev_parsedesc(at *desc)
{
  if (CONSP(desc))
    { 
      if (GPTRP(desc->Car))
        evdesc = (const char *)(desc->Csr->Gptr);
      if (NUMBERP(desc->Cdr))
        evmods = (int)(desc->Cdr->Number);
    }
}


/* event_add --
   Add a new event targeted to a specific handler.
*/
void
event_add(at *handler, at *event)
{
  ev_add(handler, event, NULL, 0);
}

/* event_peek --
   Return null when the queue is empty.
   Otherwise return the handler associated 
   with the next available event.
*/
void *
event_peek(void)
{
  at *pp = head;
  if (CONSP(pp))
    {
      while (CONSP(pp->Cdr))
        {
          at *p = pp->Cdr;
          void *hdnl = ev_hdnl(p->Car);
          if (hdnl)
            return hndl;
          ev_remove(pp, p);
        }
      if (pp->Cdr)
        {
          at *p = pp->Cdr;
          pp->Cdr = NIL;
          UNLOCK(p);
        }
    }
  return 0;
}


/* event_get --
   Return the next event associated with the specified handler.
   The event is removed from the queue if <remove> is non zero.
   Argument pdesc optionally provides a location
   for storing the additionnal description string pointer.
*/
at * 
event_get(void *handler, int remove)
{
  at *pp = head;
  if (CONSP(pp))
    {
      while (CONSP(pp->Cdr))
        {
          at *p = pp->Cdr;
          void *hndl = ev_hndl(p->Car);
          if (hndl == handler)
            {
              at *event = p->Car->Cdr->Cdr;
              ev_parsedesc(p->Car->Cdr->Car);
              LOCK(event);
              if (remove)
                ev_remove(pp, p);
              return event;
            }
          if (hdnl == 0)
            {
              ev_remove(pp, p);
              continue;
            }
          pp = p;
        }
      if (pp->Cdr)
        {
          at *p = pp->Cdr;
          pp->Cdr = NIL;
          UNLOCK(p);
        }
    }
  return NIL;
}


/* ------------------------------------ */
/* TIMERS                               */
/* ------------------------------------ */     


typedef struct evtime {
  int sec;
  int msec;
} evtime_t;

struct event_timer
{
  struct event_timer *next;
  evtime_t date;
  evtime_t period;
  at *handler;
};

static struct event_timer *timers = 0;

static void
evtime_now(evtime_t *r)
{
  os_curtime(&r->sec, &r->msec);
}

static int
evtime_cmp(evtime_t *t1, evtime_t *t2)
{
  if (t1->sec < t2->sec)
    return -1;
  if (t1->sec > t2->sec)
    return +1;
  if (t1->usec < t2->usec)
    return -1;
  if (t1->usec > t2->usec)
    return +1;
  return 0;
}

static void
evtime_add(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
  r->sec = t1->sec + t2->sec;
  r->usec = t1->usec + t2->usec;
  while (r->usec > 1000)
    {
      r->sec += 1;
      r->usec -= 1000;
    }
}

static void
evtime_sub(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
  r->sec = t1->sec - t2->sec - 1;
  r->usec = t1->usec + 1000 - t2->usec;
  while (r->usec > 1000)
    {
      r->sec += 1;
      r->usec -= 1000;
    }
}

static void
ti_finalizer(at *handler, void *arg)
{
  struct event_timer **p = &timers;
  struct event_timer *t;
  while ((t = *p))
    if (t->handler == handler) {
      *p = t->next;
      free(t);
    } else {
      p = &(t->next);
    }
}

static void
ti_insert(struct event_timer *ti)
{
  struct event_timer **p;
  struct event_timer *t;
  for (p=&timers; (t = *p); p=&(t->next))
    if (evtime_cmp(&ti->date, &t->date)<0)
      break;
  ti->next = t;
  *p = ti;
}

/* timer_add --
   Add a timer targeted to the specified handler
   firing after delay milliseconds and every period
   milliseconds after that.  Specifying period equal 
   to zero sets a one shot timer.
*/
void *
timer_add(at *handler, int delay, int period)
{
  struct event_timer *ti;
  evtime_t now, add;
  if (delay < 0 && period > 0)
    {
      int n = (period-delay)/period;
      delay += period * n;
    }
  if (handler && delay>=0)
    {
      add_finalizer(handler, ti_finalizer, 0);
      if (! (ti = malloc(sizeof(struct event_timer))))
        error(NIL,"Out of memory", NIL);
      evtime_now(&now);
      add.sec = delay/1000;
      add.msec = delay%1000;
      evtime_add(&now, &add, &ti->date);
      ti->period.sec = period/1000;
      ti->period.msec = period%1000;
      ti->handler = handler;
      ti->next = 0;
      ti_insert(ti);
      return ti;
    }
  return 0;
}

/* timer_del --
   Remove a timer using the handle 
   returned by timer_add().
*/
void
timer_del(void *handle)
{
  struct event_timer **p;
  struct event_timer *t;
  for (p=&timers; (t = *p); p=&(t->next))
    if (t == handle)
      {
        *p = t->next;
        free(t);
        return;
      }
}


/* timer_fire --
   Sends all current timer events.
   Returns number of milliseconds until
   next timer event (or a large number)
*/
int
timer_fire(void)
{
  evtime_t now;
  evtime_now(&now);
  while (timers && evtime_cmp(&now,&timers->date)>=0)
    {
      struct timer_event *ti = timers;
      p = cons(named("timer"), cons(new_gptr(ti), NIL));
      timers = ti->next;
      event_add(ti->handler, p, "Timer");
      UNLOCK(p);
      if (ti->period.sec>0 || ti->period.msec>0)
        {
          /* Periodic timer shoot only once per call */
          while(evtime_cmp(&now,&ti->date) >= 0)
            evtime_add(&ti->date,&ti->period,&ti->date);
          ti_insert(ti);
        }
      else
        {
          /* One shot timer */
          unprotect(t->info);
          free(t);
        }
    }
  if (timers)
    {
      evtime diff;
      evtime_sub(&timers->date, &now, &diff);
      if (diff.sec < 24*3600)
        return diff.sec * 1000 + diff.usec;
    }
  return 24*3600*1000;
}




/* ------------------------------------ */
/* EVENT SOURCE                         */
/* ------------------------------------ */     



struct event_source 
{
  struct event_source *next;
  int fd;
  void (*spoll)(void);
  void (*apoll)(void);
  void (*bwait)(void);
  void (*ewait)(void);
};

static struct event_source *sources = 0;
static int async_block = 0;
static int waiting = 0;

static void
call_spoll(void)
{
  struct event_source *src;
  for (src=sources; src; src=src->next)
    if (src->spoll)
      (*src->spoll)();
}

static void
call_apoll(void)
{
  struct event_source *src;
  if (async_block > 0) return;
  for (src=sources; src; src=src->next)
    if (src->apoll && src->fd>0)
      (*src->apoll)();
}

static void
call_bwait(void)
{
  struct event_source *src;  
  if (waiting) return;
  for (src=sources; src; src=src->next)
    if (src->bwait)
      (*src->bwait)();
  waiting = 1;
}

static void
call_ewait(void)
{
  struct event_source *src;
  if (!waiting) return;
  for (src=sources; src; src=src->next)
    if (src->ewait)
      (*src->ewait)();
  waiting = 0;
}

#define MAXFDS 16
static int sourcefds[MAXFDS];

static void
source_setup(void)
{
  int n = 0;;
  for (src=sources; src; src=src->next)
    if (src->apoll && src->fd>0 && n<MAXFDS)
      sourcefds[n++] = src->fd;
  os_setup_async_poll(fds, n, call_apoll);
  free(fds);
}


/* unregister_event_source --
   Unregistes a previously registered event source.
   Argument handle is the handle returned by the
   registering function below.
*/
void
unregister_event_source(void *handle)
{
  struct event_source **p = &sources;
  struct event_source *src;
  while ((src = *p) && ((void*)src!=handle))
    p = &src->next;
  if (!src)
    return;
  *p = src->next;
  free(src);
  source_setup();
}

/* register_event_source --
   Modules that produce events must register an event source.
   All arguments are optional.
   
   Function spoll() is called whenever lush checks the
   event queue for events. This function may file new
   events by calling event_add().
   
   Function apoll() might be called anytime to let the
   source check its event queue.  This function should
   never call event_add() but can set internal data
   structures within the source.

   Function bwait() is called on all sources whenever
   lush is waiting for user events.  Function ewait()
   is called whenever lush stops waiting for user events.
   These function are usuful to present the user
   with some feedback such as a hourglass cursor.
   
   Argument fd is a unix file descriptor (or another
   operating system dependent handle) that will be
   used to check for the availability of events.
*/

void *
register_event_source(void (*spoll)(void),
                      void (*apoll)(void),
                      void (*bwait)(void),
                      void (*ewait)(void),
                      int fd )
     
{
  struct event_source *src;
  src = malloc(sizeof(struct event_source));
  if (! src)
    error(NIL,"Out of memory", NIL);
  src->next = sources;
  src->fd = fd;
  src->spoll = spoll;
  src->apoll = apoll;
  src->bwait = bwait;
  src->ewait = ewait;
  sources = src;
  source_setup();
}


/* block_async_event/unblock_async_event --
   Blocks/unblocks calls to the asynchronous polling
   function of registered event sources.
*/
void
block_async_poll(void)
{
  if (! async_block++)
    os_block_async_poll(void);
}

void
unblock_async_poll(void)
{
  call_ewait();
  if (! --async_block)
    os_unblock_async_poll(void);    
}


/* ------------------------------------ */
/* WAITING                              */
/* ------------------------------------ */     


/* wait_for_event --
   Waits until events become available.
   Returns handler of first available event.
   Also wait for console input if <console> is true.
*/
at *
wait_for_event(int console)
{
  at *hndl = 0;
  block_async_poll();
  for (;;)
    {
      int n, ms;
      if ((hndl = event_peek()))
        break;
      call_spoll();
      if ((hndl = event_peek()))
        break;
      ms = timer_fire();
      if ((hndl = event_peek()))
        break;
      /* We should really wait */
      n = 0;
      for (src=sources; src; src=src->next)
        if (src->fd>0 && n<MAXFDS)
          sourcefds[n++] = src->fd;
        else
          ms = 250;
      call_bwait();
      ms = os_wait(n, sourcefds, console, ms);
      hndl = NIL;
      if (console && ms)
        break;
    }
  unblock_async_poll();
  LOCK(hndl);
  return hndl;
}


*********************************************************


/* ------------------------------------ */
/* COMPATIBILIY FOR EVENT_ADD           */
/* ------------------------------------ */     


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
  if (event == EVENT_ALARM) 
    return (cons(named("alarm"),
		 cons(NEW_NUMBER(xdown),
		      cons(NEW_NUMBER(ydown), NIL))));
  if (event == EVENT_EXPOSE)
    return cons(named("expose"),
                cons(NEW_NUMBER(xdown),
                     cons(NEW_NUMBER(ydown),
                          cons(NEW_NUMBER(xup),
                               cons(NEW_NUMBER(yup), NIL))) ) );
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





***********************************************************************
















/* ------------------------------------ */
/* EVENT MANAGEMENT                     */
/* ------------------------------------ */     




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



/* ------------------------------------ */
/* EVENT USER FUNCTIONS                 */
/* ------------------------------------ */     



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
/* INITIALISATION                       */
/* ------------------------------------ */     


void 
init_event(void)
{
  /* EVENT QUEUE */
  head = tail = cons(NIL,NIL);
  protect(head);
  /* EVENT HOOKS */
  at_event  = var_define("event-hook");
  at_idle  = var_define("idle-hook");
  /* EVENTS FUNCTION */
  dx_define("process_pending_events",xprocess_pending_events);
  dx_define("sendevent",xsendevent);
  dx_define("testevent",xtestevent);
  dx_define("checkevent",xcheckevent);
  dx_define("waitevent",xwaitevent);
  dx_define("eventinfo",xeventinfo);
	
}
