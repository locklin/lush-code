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
 * $Id: event.c,v 1.15 2002-11-06 21:09:42 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "graphics.h"

static at *at_handle;
static at *head;
static at *tail;

/* From UNIX.C */
extern void os_block_async_poll(void);
extern void os_unblock_async_poll(void);
extern void os_setup_async_poll(int*fds, int nfds, void(*apoll)(void));
extern int  os_wait(int nfds, int* fds, int console, unsigned long ms);
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

static void
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
ev_finalize(at *handler, void *arg)
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
ev_add(at *handler, at *event, const char *desc, int mods)
{
  if (handler && event)
    {
      at *p;
      at *d = NIL;
      if (mods == (unsigned char)mods)
        d = NEW_NUMBER(mods);
      if (desc && d)
        d = cons(new_gptr((gptr)desc), d);
      else if (desc)
        d = new_gptr((gptr)desc);
      LOCK(event);
      p = cons(new_gptr(handler),cons(d,event));
      add_finalizer(handler, ev_finalize, 0);
      tail->Cdr = cons(p,NIL);
      tail = tail->Cdr;
    }
}

static void *
ev_peek(void)
{
  at *pp = head;
  void *hndl;
  if (CONSP(pp))
    {
      while (CONSP(pp->Cdr))
        {
          at *p = pp->Cdr;
          if ((hndl = ev_hndl(p->Car)))
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

static const char *evdesc = 0;
static int evmods = 0;

void
ev_parsedesc(at *desc)
{
  if (CONSP(desc)) {
    ev_parsedesc(desc->Car);
    ev_parsedesc(desc->Cdr);
  } else if (GPTRP(desc))
    evdesc = (const char *)(desc->Gptr);
  else if (NUMBERP(desc))
    evmods = (unsigned char)(desc->Number);
}


/* event_add --
   Add a new event targeted to a specific handler.
*/
void
event_add(at *handler, at *event)
{
  if (! handler)
    error(NIL,"Illegal null event handler",NIL);
  if (! CONSP(event))
    error(NIL,"Illegal event expression (list expected)", event);
  ev_add(handler, event, NULL, -1);
}

/* event_peek --
   Return null when the queue is empty.
   Otherwise return the handler associated 
   with the next available event.
*/
at *
event_peek(void)
{
  at *handler = ev_peek();
  LOCK(handler);
  return handler;
}

/* event_get --
   Return the next event associated with the specified handler.
   The event is removed from the queue if <remove> is non zero.
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
          if (hndl == 0)
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
  if (t1->msec < t2->msec)
    return -1;
  if (t1->msec > t2->msec)
    return +1;
  return 0;
}

static void
evtime_add(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
  r->sec = t1->sec + t2->sec;
  r->msec = t1->msec + t2->msec;
  while (r->msec > 1000)
    {
      r->sec += 1;
      r->msec -= 1000;
    }
}

static void
evtime_sub(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
  r->sec = t1->sec - t2->sec - 1;
  r->msec = t1->msec + 1000 - t2->msec;
  while (r->msec > 1000)
    {
      r->sec += 1;
      r->msec -= 1000;
    }
}

static void
ti_finalize(at *handler, void *arg)
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
  if (! handler)
    error(NIL,"Illegal null event handler",NIL);
  if (delay < 0)
    error(NIL,"Illegal timer delay",NEW_NUMBER(delay));
  if (period < 0)
    error(NIL,"Illegal timer interval",NEW_NUMBER(period));
  if (period > 0 && period < 20)
    period = 20;
  if (handler && delay>=0)
    {
      add_finalizer(handler, ti_finalize, 0);
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
      struct event_timer *ti = timers;
      at *p = cons(named("timer"), cons(new_gptr(ti), NIL));
      timers = ti->next;
      event_add(ti->handler, p);
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
          free(ti);
        }
    }
  if (timers)
    {
      evtime_t diff;
      evtime_sub(&timers->date, &now, &diff);
      if (diff.sec < 24*3600)
        return diff.sec * 1000 + diff.msec;
    }
  return 24*3600*1000;
}




/* ------------------------------------ */
/* POLL FUNCTIONS                       */
/* ------------------------------------ */     



struct poll_functions 
{
  struct poll_functions *next;
  int fd;
  int  (*spoll)(void);
  void (*apoll)(void);
  void (*bwait)(void);
  void (*ewait)(void);
};

static struct poll_functions *sources = 0;
static int async_block = 0;
static int waiting = 0;

static int
call_spoll(void)
{
  int timeout = 24*3600*1000;
  struct poll_functions *src;
  block_async_poll();
  for (src=sources; src; src=src->next)
    if (src->spoll)
      {
        int ms = (*src->spoll)();
        if (ms>0 && ms<timeout)
          timeout = ms;
      }
  unblock_async_poll();
  return timeout;
}

static void
call_apoll(void)
{
  struct poll_functions *src;
  if (async_block > 0) return;
  for (src=sources; src; src=src->next)
    if (src->apoll && src->fd>0)
      (*src->apoll)();
}

static void
call_bwait(void)
{
  struct poll_functions *src;  
  if (waiting) return;
  for (src=sources; src; src=src->next)
    if (src->bwait)
      (*src->bwait)();
  waiting = 1;
}

static void
call_ewait(void)
{
  struct poll_functions *src;
  if (!waiting) return;
  for (src=sources; src; src=src->next)
    if (src->ewait)
      (*src->ewait)();
  waiting = 0;
}

#define MAXFDS 32
static int sourcefds[MAXFDS];

static void
async_poll_setup(void)
{
  struct poll_functions *src;
  int n = 0;
  for (src=sources; src; src=src->next)
    if (src->apoll && src->fd>0 && n<MAXFDS)
      sourcefds[n++] = src->fd;
  os_setup_async_poll(sourcefds, n, call_apoll);
}


/* unregister_poll_functions --
   Unregistes a previously registered event source.
   Argument handle is the handle returned by the
   registering function below.
*/
void
unregister_poll_functions(void *handle)
{
  struct poll_functions **p = &sources;
  struct poll_functions *src;
  while ((src = *p) && ((void*)src!=handle))
    p = &src->next;
  if (!src)
    return;
  *p = src->next;
  free(src);
  async_poll_setup();
}

/* register_poll_functions --
   Modules that produce events must register an event source.
   All arguments are optional.
   
   Function spoll() is called whenever lush checks the
   event queue for events. This function may file new
   events by calling event_add().  It returns the maximum
   number of milliseconds to wait before calling it again.
   
   Function apoll() might be called anytime to let the
   source check its event queue.  This function is called
   from signal handlers and should not do much.
   It cannot call event_add() but can set internal data
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
register_poll_functions(int  (*spoll)(void),
                        void (*apoll)(void),
                        void (*bwait)(void),
                        void (*ewait)(void),
                        int fd )
     
{
  struct poll_functions *src;
  src = malloc(sizeof(struct poll_functions));
  if (! src)
    error(NIL,"Out of memory", NIL);
  src->next = sources;
  src->fd = fd;
  src->spoll = spoll;
  src->apoll = apoll;
  src->bwait = bwait;
  src->ewait = ewait;
  sources = src;
  async_poll_setup();
  return src;
}


/* block_async_event/unblock_async_event --
   Blocks/unblocks calls to the asynchronous polling
   function of registered event sources.
*/
void
block_async_poll(void)
{
  if (! async_block++)
    os_block_async_poll();
}

void
unblock_async_poll(void)
{
  call_ewait();
  if (! --async_block)
    os_unblock_async_poll();    
}


/* ------------------------------------ */
/* PROCESSING EVENTS                    */
/* ------------------------------------ */     

static void
call_hook(at *hook, at *args)
{
  if (EXTERNP(hook, &symbol_class))
    {
      at *ans = NIL;
      at *fun = var_get(hook);
      if (fun && error_doc.ready_to_an_error)
        if (fun->flags & X_FUNCTION)
          ans = apply(hook, args);
      UNLOCK(fun);
      UNLOCK(ans);
    }
}


/* event_wait --
   Waits until events become available.
   Returns handler of first available event.
   Also wait for console input if <console> is true.
*/
at *
event_wait(int console)
{
  at *hndl = 0;
  int cinput = 0;
  int toggle = 1;
  block_async_poll();
  for (;;)
    {
      int n, ms1, ms2;
      struct poll_functions *src;
      if ((hndl = ev_peek()))
        break;
      ms1 = call_spoll();
      if ((hndl = ev_peek()))
        break;
      /* Check for console input */
      hndl = NIL;
      if (console && cinput)
        break;
      /* Check timer every other time */
      ms2 = 0;
      if (console)
        toggle ^= 1;
      if (toggle || !console)
        {
          ms2 = timer_fire();
          if ((hndl = ev_peek()))
            break;
        }
      /* Really wait */
      n = 0;
      for (src=sources; src; src=src->next)
        if (src->fd>0 && n<MAXFDS)
          sourcefds[n++] = src->fd;
      call_bwait();
      cinput = os_wait(n, sourcefds, console, 
                       (ms1<ms2) ? ms1 : ms2 );
    }
  unblock_async_poll();
  LOCK(hndl);
  return hndl;
}

/* process_pending_events --
   Process currently pending events
   by calling event-hook and event-idle
   until no events are left. */
void 
process_pending_events(void)
{
  at *hndl;
  at *event;
  int timer_fired = 0;
  call_spoll();
  hndl = ev_peek();
  while (hndl)
    {
      while (hndl)
        {
          /* Call the handler method <handle> */
          LOCK(hndl);
          event = event_get(hndl, TRUE);
          if (CONSP(event))
            {
              at *cl = classof(hndl);
              if (EXTERNP(cl, &class_class))
                {
                  at *m = checksend(cl->Object, at_handle);
                  if (m)
                    {
                      at *args = new_cons(event,NIL);
                      UNLOCK(m);
                      argeval_ptr = eval_nothing;
                      m = send_message(NIL,hndl,at_handle,args);
                      argeval_ptr = eval_std;
                      UNLOCK(args);
                    }
                  UNLOCK(m);
                }
              UNLOCK(cl);
            }
          UNLOCK(event);
          UNLOCK(hndl);
          /* Check for more events */
          call_spoll();
          hndl = ev_peek();
        }
      /* Check for timer events */
      if (! timer_fired)
        timer_fire();
      timer_fired = 1;
      hndl = ev_peek();
    }
}





/* ------------------------------------ */
/* COMPATIBILIY FOR GRAPHICS STUFF      */
/* ------------------------------------ */     


static at *
two_integers(int i1, int i2)
{
  return cons(NEW_NUMBER(i1), cons(NEW_NUMBER(i2), NIL));
}

static at *
four_integers(int i1, int i2, int i3, int i4)
{
  return cons(NEW_NUMBER(i1), cons(NEW_NUMBER(i2), two_integers(i3, i4)));
} 

static at * 
event_to_list(int event, int xd, int yd, int xu, int yu, int *pmods)
{
  *pmods = -1;
  /* events that do not update evshift and evcontrol */
  if (event == EVENT_MOUSE_UP) 
    return cons(named("mouse-up"), four_integers(xd,yd,xu,yu));
  if (event == EVENT_MOUSE_DRAG) 
    return cons(named("mouse-drag"), four_integers(xd,yd,xu,yu));
  if (event == EVENT_RESIZE)
    return cons(named("resize"), two_integers(xd,yd));
  if (event == EVENT_DELETE) 
    return cons(named("delete"),NIL);
  if (event == EVENT_SENDEVENT)
    return cons(named("sendevent"), two_integers(xd,yd));
  if (event == EVENT_EXPOSE)
    return cons(named("expose"), two_integers(xd,yd));
  if (event == EVENT_GLEXPOSE)
    return cons(named("glexpose"), two_integers(xd,yd));
  if (event >= EVENT_ASCII_MIN && event <= EVENT_ASCII_MAX) 
    {
      char keyevent[2];
      keyevent[0] = EVENT_TO_ASCII(event);
      keyevent[1] = 0;
      return cons(new_string(keyevent), two_integers(xd,yd));
    }
  /* events that update evshift and evcontrol */
  *pmods = 0;
  if (xu) 
    *pmods |= 1;  /* shift */
  if (yu) 
    *pmods |= 2;  /* ctrl  */
  if (event == EVENT_MOUSE_DOWN)
    return cons(named("mouse-down"), two_integers(xd,yd)); 
  if (event == EVENT_HELP)
    return cons(named("help"), two_integers(xd,yd)); 
  if (event == EVENT_ARROW_UP)
    return cons(named("arrow-up"), two_integers(xd,yd)); 
  if (event == EVENT_ARROW_RIGHT)
    return cons(named("arrow-right"), two_integers(xd,yd)); 
  if (event == EVENT_ARROW_DOWN)
    return cons(named("arrow-down"), two_integers(xd,yd)); 
  if (event == EVENT_ARROW_LEFT)
    return cons(named("arrow-left"), two_integers(xd,yd)); 
  if (event == EVENT_FKEY)
    return cons(named("fkey"), two_integers(xd,yd)); 
  /* default */
  return NIL;
}

void
enqueue_eventdesc(at *handler, int event, 
                  int xd, int yd, int xu, int yu, char *desc)
{
  int mods = -1;
  at *ev = event_to_list(event, xd, yd, xu, yu, &mods);
  if (!ev) return;
  ev_add(handler, ev, desc, mods);
  UNLOCK(ev);
}

void
enqueue_event(at *handler, int event, 
              int xd, int yd, int xu, int yu)
{
  enqueue_eventdesc(handler, event, xd, yd, xu, yu, NULL);
}



/* ------------------------------------ */
/* EVENT MANAGEMENT                     */
/* ------------------------------------ */     


static at *
current_window_handler(void)
{
  struct window *win = current_window();
  (*win->gdriver->begin) (win);
  (*win->gdriver->end) (win);
  if (!win->eventhandler)
    error(NIL,"No event handler for the current window",NIL);
  LOCK(win->eventhandler);
  return win->eventhandler;
}


/* Call event hooks */
DX(xprocess_pending_events)
{
  ARG_NUMBER(0);
  process_pending_events();
  return NIL;
}

/* Set the event handler for a window */
DX(xseteventhandler)
{
  struct window *win;
  at *w,*p,*q;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  w = APOINTER(1);
  q = APOINTER(2);
  if (!EXTERNP(w,&window_class))
    error(NIL, "not a window", w);
  win = w->Object;
  if ((p = win->eventhandler)) 
    {
      win->eventhandler = NIL;
      unprotect(w);
    }
  if ((win->eventhandler = q))
    {
      LOCK(q);
      protect(w);
    }
  return p;
}

/* Send events */
DX(xsendevent)
{
  at *p1, *p2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  p1 = APOINTER(1);
  p2 = APOINTER(2);
  if (NUMBERP(p1) && NUMBERP(p2))
    {
      /* Old syntax */
      at *event = cons(named("sendevent"), cons(p1, cons(p2, NIL)));
      at *handler = current_window_handler();
      event_add(handler, event);
      UNLOCK(handler);
      UNLOCK(event);
    }
  else if (p1 && p2)
    {
      /* New syntax */
      event_add(p1, p2);
    }
  return NIL;
}


static at *
test_event_sub(int arg_number, at **arg_array, int remove)
{
  /* Validate parameters */
  at *r = NIL;
  at *handler = NIL;
  if (arg_number == 0)
    handler = current_window_handler();
  else if (arg_number == 1) {
    ARG_EVAL(1);
    handler = APOINTER(1);
    LOCK(handler);
  } else 
    ARG_NUMBER(-1);
  /* Perform */
  call_spoll();
  if (handler)
    r = event_get(handler, remove);
  else
    r = event_peek();
  UNLOCK(handler);
  return r;
}

/* Test presence of events */
DX(xtestevent)
{
  return test_event_sub(arg_number, arg_array, FALSE);
}

/* Get events */
DX(xcheckevent)
{
  return test_event_sub(arg_number, arg_array, TRUE);
}

/* Get event info */
DX(xeventinfo)
{
  ARG_NUMBER(0);
  return cons(new_string((char*)((evdesc) ? evdesc : "n/a")),
              cons( ((evmods & 1) ? true() : NIL),
                    cons( ((evmods & 2) ? true() : NIL), 
                          NIL ) ) );
}

/* Wait for events */
DX(xwaitevent)
{
  ARG_NUMBER(0);
  return event_wait(FALSE);
}

/* Create a timer */
DX(xcreate_timer)
{
  ALL_ARGS_EVAL;
  if (arg_number == 2)
    return new_gptr(timer_add(APOINTER(1),AINTEGER(2),0));
  ARG_NUMBER(3);
  return new_gptr(timer_add(APOINTER(1),AINTEGER(2),AINTEGER(3)));  
}

/* Destroy a timer */
DX(xkill_timer)
{
  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  timer_del(AGPTR(1));
  return NIL;
}

/* Sleep for specified time (seconds) */
DX(xsleep)
{
  at *q;
  at *handler;
  int delay;

  ALL_ARGS_EVAL;
  ARG_NUMBER(1);
  delay = (int)(1000 * AREAL(1));
  /* Setup timer with dummy handler */
  handler = cons(NIL,NIL);
  timer_add(handler, delay, 0);
  /* Loop until getting this event */
  q = NIL;
  while (! q)
    {
      q = event_wait(FALSE);
      UNLOCK(q);
      q = NIL;
      q = event_get(handler, TRUE);
    }
  UNLOCK(q);
  UNLOCK(handler);
  q = APOINTER(1);
  LOCK(q);
  return q;
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
  /* EVENTS FUNCTION */
  at_handle = var_define("handle");
  dx_define("set-event-handler",xseteventhandler);
  dx_define("process-pending-events",xprocess_pending_events);
  dx_define("sendevent",xsendevent);
  dx_define("testevent",xtestevent);
  dx_define("checkevent",xcheckevent);
  dx_define("waitevent",xwaitevent);
  dx_define("eventinfo",xeventinfo);
  /* TIMER FUNCTIONS */
  dx_define("create-timer", xcreate_timer);
  dx_define("kill-timer", xkill_timer);
  dx_define("sleep", xsleep);
}
