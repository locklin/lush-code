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

static at *at_handle = NIL;
static at *head = NIL;
static at *tail = NIL;

/* From UNIX.C */
extern void os_block_async_poll(void);
extern void os_unblock_async_poll(void);
extern void os_setup_async_poll(int*fds, int nfds, void(*apoll)(void));
extern int  os_wait(int nfds, int* fds, int console, unsigned long ms);
extern void os_curtime(int *sec, int *msec);


/* ------------------------------------ */
/* EVENT QUEUE                          */
/* ------------------------------------ */     


static void *ev_hndl(at *q)
{
   if (CONSP(q) && GPTRP(Car(q)) && CONSP(Cdr(q)) && Cddr(q))
      return Gptr(Car(q));
   return 0;
}

static void ev_remove(at *pp, at *p)
{
   if (Cdr(pp) != p)
      error(NIL, "internal error", NIL);
   Cdr(pp) = Cdr(p);
   Cdr(p) = NIL;
   if (tail == p)
      tail = pp;
}

static void ev_notify(at *handler, void *_)
{
   at *pp = head;
   if (CONSP(pp)) {
      at *p = Cdr(pp);
      while (CONSP(p)) {
         void *hndl = ev_hndl(Car(p));
         if (hndl==0 || hndl==(gptr)handler) 
            ev_remove(pp, p);
         else
            pp = p;
         p = Cdr(pp);
      }
   }
}

void ev_add(at *handler, at *event, const char *desc, int mods)
{
   MM_ENTER;
   if (handler && event) {
      at *d = NIL;
      if (mods == (unsigned char)mods)
         d = NEW_NUMBER(mods);
      if (desc && d) {
         gptr p = (gptr)desc;
         d = new_cons(NEW_GPTR(p), d);
      } else if (desc) {
         gptr p = (gptr)desc;
         d = NEW_GPTR(p);
      }
      at *p = new_cons(NEW_GPTR(handler), new_cons(d, event));
      add_notifier(handler, (wr_notify_func_t *)ev_notify, 0);
      Cdr(tail) = new_cons(p,NIL);
      tail = Cdr(tail);
   }
   MM_EXIT;
}

static void *ev_peek(void)
{
   at *pp = head;
   if (CONSP(pp)) {
      while (CONSP(Cdr(pp))) {
         at *p = Cdr(pp);
         void *hndl = ev_hndl(Car(p));
         if (hndl)
            return hndl;
         ev_remove(pp, p);
      }
      if (Cdr(pp))
         Cdr(pp) = NIL;
   }
   return NULL;
}

static const char *evdesc = 0;
static int evmods = 0;

void ev_parsedesc(at *desc)
{
   if (CONSP(desc)) {
      ev_parsedesc(Car(desc));
      ev_parsedesc(Cdr(desc));
   } else if (GPTRP(desc))
      evdesc = (const char *)String(desc);
   else if (NUMBERP(desc))
      evmods = (unsigned char)Number(desc);
}


/* event_add --
 * Add a new event targeted to a specific handler.
 */
void event_add(at *handler, at *event)
{
   if (!handler)
      RAISEF("invalid event handler", handler);
   if (!CONSP(event))
      RAISEF("invalid event (not a list expected)", event);
   ev_add(handler, event, NULL, -1);
}

/* event_peek --
 * Return null when the queue is empty.
 * Otherwise return the handler associated 
 * with the next available event.
 */
at *event_peek(void)
{
   return ev_peek();
}

/* event_get --
 * Return the next event associated with the specified handler.
 * The event is removed from the queue if <remove> is true.
 */
at *event_get(void *handler, bool remove)
{
   at *pp = head;
   if (CONSP(pp)) {
      while (CONSP(Cdr(pp))) {
         at *p = Cdr(pp);
         void *hndl = ev_hndl(Car(p));

         if (hndl == handler) {
            at *event = Cdr(Cdar(p));
            ev_parsedesc(Car(Cdar(p)));
            if (remove)
               ev_remove(pp, p);
            return event;
         }
         if (!hndl) {
            ev_remove(pp, p);
            continue;
         }
         pp = p;
      }
      if (Cdr(pp))
         Cdr(pp) = NIL;
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

typedef struct event_timer {
   struct event_timer *next;
   evtime_t date;
   evtime_t period;
   at *handler;
} event_timer_t;

static event_timer_t *timers = 0;

static void clear_event_timer(event_timer_t *et, size_t _)
{
   et->next = NULL;
   et->handler = NULL;
}

static void mark_event_timer(event_timer_t *et)
{
   MM_MARK(et->next);
   MM_MARK(et->handler);
}

static mt_t mt_event_timer = mt_undefined;


static void evtime_now(evtime_t *r)
{
   os_curtime(&r->sec, &r->msec);
}

static int evtime_cmp(evtime_t *t1, evtime_t *t2)
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

static void evtime_add(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
   r->sec = t1->sec + t2->sec;
   r->msec = t1->msec + t2->msec;
   while (r->msec > 1000) {
      r->sec += 1;
      r->msec -= 1000;
   }
}

static void evtime_sub(evtime_t *t1, evtime_t *t2, evtime_t *r)
{
   r->sec = t1->sec - t2->sec - 1;
   r->msec = t1->msec + 1000 - t2->msec;
   while (r->msec > 1000) {
      r->sec += 1;
      r->msec -= 1000;
   }
}

static void ti_notify(at *handler, void *_)
{
  event_timer_t *et, **p = &timers;
  while ((et = *p)) {
     if (et->handler == handler)
        *p = et->next;
     else
        p = &(et->next);
  }
}

static void ti_insert(event_timer_t *ti)
{
   event_timer_t *et, **p;
   for (p = &timers; (et = *p); p = &(et->next))
      if (evtime_cmp(&ti->date, &et->date)<0)
         break;
   ti->next = et;
   *p = ti;
}


static void *timer_add_sub(at *handler, int sec, int msec, int period)
{
   if (handler) {
      add_notifier(handler, (wr_notify_func_t *)ti_notify, 0);
      event_timer_t *et = mm_alloc(mt_event_timer);
      assert(et);
      et->date.sec = sec;
      et->date.msec = msec;
      et->period.sec = period/1000;
      et->period.msec = period%1000;
      et->handler = handler;
      et->next = 0;
      ti_insert(et);
      return et;
   }
  return 0;
}

/* timer_add --
 * Add a timer targeted to the specified handler
 * firing after delay milliseconds and every period
 * milliseconds after that.  Specifying period equal 
 * to zero sets a one shot timer.
 */
void *timer_add(at *handler, int delay, int period)
{
   evtime_t now, add;
   evtime_now(&now);
   if (!handler)
      RAISEF("invalid event handler", handler);
   if (delay < 0)
      RAISEF("invalid timer delay", NEW_NUMBER(delay));
   if (period < 0)
      RAISEF("invalid timer interval", NEW_NUMBER(period));
   if (period > 0 && period < 20)
      period = 20;
   add.sec = delay/1000;
   add.msec = delay%1000;
   evtime_add(&now, &add, &add);
   return timer_add_sub(handler, add.sec, add.msec, period);
}

/* timer_abs --
 * Add a timer targeted to the specified handler
 * firing at the specified date.
 */
void *timer_abs(at *handler, double date)
{
   int sec = (int)date;
   int msec = (date - sec) * 1000;
   if (!handler)
      RAISEF("invalid event handler", handler);
   return timer_add_sub(handler, sec, msec, 0);
}

/* timer_del --
 * Remove a timer using the handle 
 * returned by timer_add().
 */
void timer_del(void *handle)
{
   event_timer_t *et, **p;
   for (p=&timers; (et = *p); p=&(et->next))
      if (et == handle) {
         *p = et->next;
         break;
      }
}


/* timer_fire --
 * Sends all current timer events.
 * Returns number of milliseconds until
 * next timer event (or a large number)
 */
int timer_fire(void)
{
   evtime_t now;
   evtime_now(&now);
   while (timers && evtime_cmp(&now,&timers->date)>=0) {
      event_timer_t *ti = timers;
      at *p = new_cons(named("timer"), 
                       new_cons(NEW_GPTR(ti), NIL));
      timers = ti->next;
      event_add(ti->handler, p);
      
      if (ti->period.sec>0 || ti->period.msec>0) {
         /* Periodic timer shoot only once per call */
         while (evtime_cmp(&now,&ti->date) >= 0)
            evtime_add(&ti->date,&ti->period,&ti->date);
         ti_insert(ti);

      }
   }
   
   if (timers) {
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

typedef struct poll_functions 
{
   struct poll_functions *next;
   int fd;
   int  (*spoll)(void);
   void (*apoll)(void);
   void (*bwait)(void);
   void (*ewait)(void);
} poll_functions_t;

static poll_functions_t *sources = 0;
static int async_block = 0;
static int waiting = 0;

static int call_spoll(void)
{
   int timeout = 24*3600*1000;
   block_async_poll();
   for (poll_functions_t *src=sources; src; src=src->next)
      if (src->spoll) {
         int ms = (*src->spoll)();
         if (ms>0 && ms<timeout)
            timeout = ms;
      }
   unblock_async_poll();
   return timeout;
}

static void call_apoll(void)
{
   if (async_block > 0) return;
   for (poll_functions_t *src=sources; src; src=src->next)
      if (src->apoll && src->fd>0)
         (*src->apoll)();
}

static void call_bwait(void)
{
   if (waiting) return;
   for (poll_functions_t *src=sources; src; src=src->next)
      if (src->bwait)
         (*src->bwait)();
   waiting = 1;
}

static void call_ewait(void)
{
   if (!waiting) return;
   for (poll_functions_t *src=sources; src; src=src->next)
      if (src->ewait)
         (*src->ewait)();
   waiting = 0;
}

#define MAXFDS 32
static int sourcefds[MAXFDS];

static void async_poll_setup(void)
{
   int n = 0;
   for (poll_functions_t *src=sources; src; src=src->next)
      if (src->apoll && src->fd>0 && n<MAXFDS)
         sourcefds[n++] = src->fd;
   os_setup_async_poll(sourcefds, n, call_apoll);
}


/* unregister_poll_functions --
 * Unregisters a previously registered event source.
 * Argument handle is the handle returned by the
 * registering function below.
 */
void unregister_poll_functions(void *handle)
{
   poll_functions_t *src, **p = &sources;
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
   lush starts waiting for user events.  Function ewait()
   is called whenever lush stops waiting for user events.
   These function are usuful to present the user
   with some feedback such as a hourglass cursor.
   
   Argument fd is a unix file descriptor (or another
   operating system dependent handle) that will be
   used to check for the availability of events.
*/

void *register_poll_functions(int  (*spoll)(void),
                              void (*apoll)(void),
                              void (*bwait)(void),
                              void (*ewait)(void),
                              int fd)

{
   poll_functions_t *src = malloc(sizeof(struct poll_functions));
   assert(src);
   src->fd = fd;
   src->spoll = spoll;
   src->apoll = apoll;
   src->bwait = bwait;
   src->ewait = ewait;
   src->next = sources;
   sources = src;
   async_poll_setup();
   return src;
}


/* block_async_event/unblock_async_event --
 * Blocks/unblocks calls to the asynchronous polling
 * function of registered event sources.
 */
void block_async_poll(void)
{
   if (!async_block++)
      os_block_async_poll();
}

void unblock_async_poll(void)
{
   call_ewait();
   if (! --async_block)
      os_unblock_async_poll();    
}


/* ------------------------------------ */
/* PROCESSING EVENTS                    */
/* ------------------------------------ */     

/* event_wait --
 * Waits until events become available.
 * Returns handler of first available event.
 * Also wait for console input if <console> is true.
 */
at *event_wait(bool console)
{
   at *hndl = 0;
   int cinput = 0;
   int toggle = 1;
   block_async_poll();
   for (;;) {
      if ((hndl = ev_peek()))
         break;
      int ms1 = call_spoll();
      if ((hndl = ev_peek()))
         break;
      /* Check for console input */
      hndl = NIL;
      if (console && cinput)
         break;
      /* Check timer every other time */
      int ms2 = 0;
      if (console)
         toggle ^= 1;
      if (toggle || !console) {
         ms2 = timer_fire();
         if ((hndl = ev_peek()))
            break;
      }
      /* Really wait */
      int n = 0;
      for (poll_functions_t *src=sources; src; src=src->next)
         if (src->fd>0 && n<MAXFDS)
            sourcefds[n++] = src->fd;
      call_bwait();
      cinput = os_wait(n, sourcefds, console, 
                       (ms1<ms2) ? ms1 : ms2 );
    }
   unblock_async_poll();
   return hndl;
}

/* process_pending_events --
 * Process currently pending events
 * by calling event-hook and event-idle
 * until no events are left.
 */
void process_pending_events(void)
{
   MM_ENTER;
   int timer_fired = 0;
   call_spoll();
   at *hndl = ev_peek();
   for(;;) {
      while (hndl) {
         /* Call the handler method <handle> */
         at *event = event_get(hndl, true);
         if (CONSP(event)) {
            class_t *cl = classof(hndl);
            at *m = getmethod(cl, at_handle);
            if (m) {
               at *args = new_cons(quote(event), NIL);
               send_message(NIL, hndl, at_handle, args);
            }
         }
         /* Check for more events */
         call_spoll();
         hndl = ev_peek();
      }

      /* Check for timer events */
      if (timer_fired)
         break;
      timer_fire();
      timer_fired = 1;
      hndl = ev_peek();
   }
   MM_EXIT;
}


/* ------------------------------------ */
/* COMPATIBILIY FOR GRAPHICS STUFF      */
/* ------------------------------------ */     


static at *two_integers(int i1, int i2)
{
   return new_cons(NEW_NUMBER(i1), new_cons(NEW_NUMBER(i2), NIL));
}

static at *four_integers(int i1, int i2, int i3, int i4)
{
   return new_cons(NEW_NUMBER(i1), new_cons(NEW_NUMBER(i2), two_integers(i3, i4)));
} 

static at *event_to_list(int event, int xd, int yd, int xu, int yu, int *pmods)
{
   *pmods = -1;
   /* events that do not update evshift and evcontrol */
   if (event == EVENT_MOUSE_UP) 
      return new_cons(named("mouse-up"), four_integers(xd,yd,xu,yu));
   if (event == EVENT_MOUSE_DRAG) 
      return new_cons(named("mouse-drag"), four_integers(xd,yd,xu,yu));
   if (event == EVENT_RESIZE)
      return new_cons(named("resize"), two_integers(xd,yd));
   if (event == EVENT_DELETE) 
      return new_cons(named("delete"),NIL);
   if (event == EVENT_SENDEVENT)
      return new_cons(named("sendevent"), two_integers(xd,yd));
   if (event == EVENT_EXPOSE)
      return new_cons(named("expose"), two_integers(xd,yd));
   if (event == EVENT_GLEXPOSE)
      return new_cons(named("glexpose"), two_integers(xd,yd));
   if (event >= EVENT_ASCII_MIN && event <= EVENT_ASCII_MAX) {
      char keyevent[2];
      keyevent[0] = EVENT_TO_ASCII(event);
      keyevent[1] = 0;
      return new_cons(make_string(keyevent), two_integers(xd,yd));
   }
   /* events that update evshift and evcontrol */
   *pmods = 0;
   if (xu) 
      *pmods |= 1;  /* shift */
   if (yu) 
      *pmods |= 2;  /* ctrl  */
   if (event == EVENT_MOUSE_DOWN)
      return new_cons(named("mouse-down"), two_integers(xd,yd)); 
   if (event == EVENT_HELP)
      return new_cons(named("help"), two_integers(xd,yd)); 
   if (event == EVENT_ARROW_UP)
      return new_cons(named("arrow-up"), two_integers(xd,yd)); 
   if (event == EVENT_ARROW_RIGHT)
      return new_cons(named("arrow-right"), two_integers(xd,yd)); 
   if (event == EVENT_ARROW_DOWN)
      return new_cons(named("arrow-down"), two_integers(xd,yd)); 
   if (event == EVENT_ARROW_LEFT)
      return new_cons(named("arrow-left"), two_integers(xd,yd)); 
   if (event == EVENT_FKEY)
      return new_cons(named("fkey"), two_integers(xd,yd)); 
   /* default */
   return NIL;
}

void enqueue_eventdesc(at *handler, int event, 
                       int xd, int yd, int xu, int yu, char *desc)
{
   int mods = -1;
   at *ev = event_to_list(event, xd, yd, xu, yu, &mods);
   if (!ev) return;
   ev_add(handler, ev, desc, mods);
}

void enqueue_event(at *handler, int event, 
                   int xd, int yd, int xu, int yu)
{
   enqueue_eventdesc(handler, event, xd, yd, xu, yu, NULL);
}



/* ------------------------------------ */
/* EVENT MANAGEMENT                     */
/* ------------------------------------ */     


static at *current_window_handler(void)
{
   struct window *win = current_window();
   (*win->gdriver->begin) (win);
   (*win->gdriver->end) (win);
   if (!win->eventhandler)
      RAISEF("no event handler for current window", NIL);
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
   ARG_NUMBER(2);
   at *w = APOINTER(1);
   at *q = APOINTER(2);
   ifn (WINDOWP(w))
      RAISEFX("not a window", w);
   
   struct window *win = Mptr(w);
   at *p = win->eventhandler;
   if (p) {
      win->eventhandler = NIL;
      unprotect(w);
   }
   if (q) {
      win->eventhandler = q;
      protect(w);
   }
   return p;
}

/* Send events */
DX(xsendevent)
{
   ARG_NUMBER(2);
   at *p1 = APOINTER(1);
   at *p2 = APOINTER(2);
   if (NUMBERP(p1) && NUMBERP(p2)) {
      /* Old syntax */
      at *event = new_cons(named("sendevent"), 
                           new_cons(p1, new_cons(p2, NIL)));
      at *handler = current_window_handler();
      event_add(handler, event);
   } else if (p1 && p2) {
      /* New syntax */
      event_add(p1, p2);
   }
   return NIL;
}


static at *test_event_sub(int arg_number, at **arg_array, int remove)
{
   /* Validate parameters */
   at *handler = NIL;
   if (arg_number == 0)
      handler = current_window_handler();
   else if (arg_number == 1)
      handler = APOINTER(1);
   else 
      ARG_NUMBER(-1);

   /* Perform */
   call_spoll();
   at *r = handler ? event_get(handler, remove) : event_peek();
   return r;
}

/* Test presence of events */
DX(xtestevent)
{
   return test_event_sub(arg_number, arg_array, false);
}

/* Get events */
DX(xcheckevent)
{
   return test_event_sub(arg_number, arg_array, true);
}

/* Get event info */
DX(xeventinfo)
{
   ARG_NUMBER(0);
   return new_cons(make_string((char*)((evdesc) ? evdesc : "n/a")),
                   new_cons(((evmods & 1) ? t() : NIL),
                            new_cons(((evmods & 2) ? t() : NIL), 
                                     NIL)));
}

/* Wait for events */
DX(xwaitevent)
{
   ARG_NUMBER(0);
   return event_wait(false);
}

/* Create a timer */
DX(xcreate_timer)
{
   if (arg_number == 2)
      return NEW_GPTR(timer_add(APOINTER(1),AINTEGER(2),0));
   ARG_NUMBER(3);
   return NEW_GPTR(timer_add(APOINTER(1), AINTEGER(2), AINTEGER(3)));  
}

/* Create a timer to a specific date */
DX(xcreate_timer_absolute)
{
   ARG_NUMBER(2);
   return NEW_GPTR(timer_abs(APOINTER(1),AREAL(2)));
}

/* Destroy a timer */
DX(xkill_timer)
{
   ARG_NUMBER(1);
   timer_del(AGPTR(1));
   return NIL;
}

/* Sleep for specified time (seconds) */
DX(xsleep)
{
   ARG_NUMBER(1);
   int delay = (int)(1000 * AREAL(1));
   os_wait(0, NULL, false, delay);
   return APOINTER(1);
}


/* ------------------------------------ */
/* INITIALISATION                       */
/* ------------------------------------ */     

void init_event(void)
{
   mt_event_timer =
      MM_REGTYPE("event_timer", sizeof(event_timer_t),
                 clear_event_timer, mark_event_timer, 0);

   MM_ROOT(timers);
   
   /* set up event queue */
   MM_ROOT(head);
   head = tail = new_cons(NIL, NIL);
   
   /* EVENTS FUNCTION */
   at_handle = var_define("handle");
   dx_define("set-event-handler", xseteventhandler);
   dx_define("process-pending-events", xprocess_pending_events);
   dx_define("sendevent", xsendevent);
   dx_define("testevent", xtestevent);
   dx_define("checkevent", xcheckevent);
   dx_define("waitevent", xwaitevent);
   dx_define("eventinfo", xeventinfo);

   /* TIMER FUNCTIONS */
   dx_define("create-timer", xcreate_timer);
   dx_define("create-timer-absolute", xcreate_timer_absolute);
   dx_define("kill-timer", xkill_timer);
   dx_define("sleep", xsleep);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
