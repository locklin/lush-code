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
 * $Id: x11_driver.c,v 1.2 2002-04-24 20:55:38 leonb Exp $
 **********************************************************************/

/***********************************************************************
  Graphic driver for X11
 ********************************************************************** */

#include "config.h"
#ifndef X_DISPLAY_MISSING
/* ============================================================ */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifndef X11R3
#ifndef X11R4
#ifndef PWinGravity
#define X11R3
#else
#define X11R4
#endif
#endif
#endif
#ifdef X11R4
#include <X11/Xatom.h>
#endif

#include "header.h"
#include "graphics.h"




/* ============================  X11DRIVER STRUCTURES */

#define MAXWIN 64
#define MINDEPTH 2
#define ASYNC_EVENTS (ExposureMask|StructureNotifyMask)
#define SYNC_EVENTS  (ButtonPressMask|ButtonReleaseMask|KeyPressMask|ButtonMotionMask)

/* pointeur to the var 'display' */
static at *display;

int Xinitialised = FALSE;
at *at_event;

static struct X_def {
  long bgcolor;
  long fgcolor;
  char font[128];
  int x, y;
  unsigned int w, h;
  int depth;
  Display *dpy;
  int screen;
  Visual *visual;
  unsigned long red_mask;
  unsigned long green_mask;
  unsigned long blue_mask;
  Colormap cmap;
  int      cmapflag;
  Cursor   carrow, cwatch, ccross;
  Atom     wm_delete_window, wm_protocols;
  GC gcclear, gccopy, gcdash;
  int      gcflag;
} xdef;



static struct X_window {
  struct window lwin;
  Window win;
  GC gc;
  XFontStruct *font;
  Pixmap backwin;
  int sizx, sizy;
  int dbflag;
  int x1, y1, x2, y2;
  int resizestate, xdown, ydown;
  int hilitestate, okhilite, xhilite, yhilite, whilite, hhilite;
} xwind[MAXWIN];


static unsigned short bitmap[][16] = {
  { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,\
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
  { 0x0000, 0x5555, 0x0000, 0x5555, 0x0000, 0x5555, 0x0000, 0x5555,\
    0x0000, 0x5555, 0x0000, 0x5555, 0x0000, 0x5555, 0x0000, 0x5555},
  { 0xAAAA, 0x1111, 0xAAAA, 0x4444, 0xAAAA, 0x1111, 0xAAAA, 0x4444,\
    0xAAAA, 0x1111, 0xAAAA, 0x4444, 0xAAAA, 0x1111, 0xAAAA, 0x4444},
  { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,\
    0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555},  
  { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,\
    0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555},
  { 0x5555, 0xEEEE, 0x5555, 0xBBBB, 0x5555, 0xEEEE, 0x5555, 0xBBBB,\
    0x5555, 0xEEEE, 0x5555, 0xBBBB, 0x5555, 0xEEEE, 0x5555, 0xBBBB},
  { 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555,\
    0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555},
  { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,\
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
};

static Pixmap gray_stipple[8];
extern struct gdriver x11_driver;



/* ============================  TRIGGER */

/* From UNIX.C */
void block_async_trigger(void);
void unblock_async_trigger(void);

#define enable()   unblock_async_trigger()
#define disable()  block_async_trigger()

static void setcursor(int flag);
static void handle_sync_events(void);
static void handle_async_events(void);

static void
x11_trigger(int op)
{
  switch (op)
    {
    case OP_SYNC:
      handle_sync_events();
      return;
    case OP_ASYNC:
      handle_async_events();
      return;
    case OP_END_WAIT:
      setcursor(0);
      return;
    case OP_BEGIN_WAIT:
      setcursor(1);
      return;
    }
}




/* ============================  INITIALISATION & DEFAULTS */

static char *
search_display_name(void)
{
  register at *p;
  register char *s;
  static char dpyname[128] = "unix:0.0";
  extern char *getenv(const char *);

  p = var_get(display);
  if (p && (p->flags & X_STRING))
    strcpy(dpyname, SADD(p->Object));
  else if ((s = getenv("DISPLAY")))
    strcpy(dpyname, s);
  return dpyname;
}


static int badname = 1;

static void
x11_handler(Display *display, XErrorEvent *myerr)
{
  char msg[80];
  if (badname==0 && myerr->error_code==BadName) {
    badname = 1;
  } else {
    XGetErrorText(display, myerr->error_code, msg, 80);
    fprintf(stderr,"*** Xlib error %d : %s\n", myerr->error_code, msg);
  }
}

static void 
x11_init(void)
{
  char *dpyname;
  char *tempstr;
  int i;
  XColor xc;
  XVisualInfo vinfo;

  /* Open display */
  dpyname = search_display_name();
  xdef.dpy = XOpenDisplay(dpyname);
  ifn(xdef.dpy)
    error(NIL, "X11: Can't open display on", new_string(dpyname));

  /* Get visual */
  xdef.screen = DefaultScreen(xdef.dpy);
  if ( XMatchVisualInfo(xdef.dpy, xdef.screen, 32, TrueColor, &vinfo) || 
       XMatchVisualInfo(xdef.dpy, xdef.screen, 24, TrueColor, &vinfo) ||
       XMatchVisualInfo(xdef.dpy, xdef.screen, 16, TrueColor, &vinfo) ||
       XMatchVisualInfo(xdef.dpy, xdef.screen, 15, TrueColor, &vinfo)   )
    {
      xdef.visual = vinfo.visual;
      xdef.depth = vinfo.depth;
      xdef.red_mask = vinfo.red_mask;
      xdef.green_mask = vinfo.green_mask;
      xdef.blue_mask = vinfo.blue_mask;
      xdef.cmapflag = 1;
      xdef.cmap =  XCreateColormap (xdef.dpy, DefaultRootWindow(xdef.dpy),
                                    xdef.visual, AllocNone );
    }
  else
    {
      xdef.visual = DefaultVisual(xdef.dpy, xdef.screen);
      xdef.depth = DisplayPlanes(xdef.dpy, xdef.screen);
      xdef.red_mask = 0;
      xdef.green_mask = 0;
      xdef.blue_mask = 0;
      xdef.cmap = DefaultColormap(xdef.dpy, xdef.screen);
      xdef.cmapflag = 0;
    }
  
  /* Defaults */
  xc.red = xc.green = xc.blue = 0;
  xc.pixel = BlackPixel(xdef.dpy, xdef.screen);
  XAllocColor(xdef.dpy, xdef.cmap, &xc);
  xdef.fgcolor = xc.pixel;
  xc.red = xc.green = xc.blue = 65535;
  xc.pixel = WhitePixel(xdef.dpy, xdef.screen);
  XAllocColor(xdef.dpy, xdef.cmap, &xc);
  xdef.bgcolor = xc.pixel;
  tempstr = XGetDefault(xdef.dpy, "SN", "ReverseVideo");
  if (tempstr && !strcmp(tempstr, "on")) {
    int tmp = xdef.bgcolor;
    xdef.bgcolor = xdef.fgcolor;
    xdef.fgcolor = tmp;
  }
  strcpy(xdef.font, "fixed");
  tempstr = XGetDefault(xdef.dpy, "SN", "Font");
  if (tempstr) {
    strcpy(xdef.font, tempstr);
  }
  xdef.x = xdef.y = 0;
  xdef.w = xdef.h = 512;
  tempstr = XGetDefault(xdef.dpy, "SN", "Geometry");
  if (tempstr) {
    XParseGeometry(tempstr, &xdef.x, &xdef.y, &xdef.w, &xdef.h);
  }

  /* Get wm_delete_window atom */
#ifdef X11R4
  xdef.wm_delete_window = XInternAtom(xdef.dpy, "WM_DELETE_WINDOW", True);
  xdef.wm_protocols = XInternAtom(xdef.dpy, "WM_PROTOCOLS", True);
#endif

  /* Gray Stipple */
  for (i = 1; i < 7; i++)
    gray_stipple[i] = XCreateBitmapFromData(xdef.dpy,
					    DefaultRootWindow(xdef.dpy),
					    (char *) bitmap[i], 16, 16);
  /* Shared stuff */
  xdef.gcflag = FALSE;
  xdef.carrow = XCreateFontCursor(xdef.dpy, XC_arrow);
  xdef.cwatch = XCreateFontCursor(xdef.dpy, XC_watch);
  xdef.ccross = XCreateFontCursor(xdef.dpy, XC_crosshair);
  
  /* Error handler */
  XSetErrorHandler((void*)x11_handler);

  /* Miscellaneous */
  set_trigger(ConnectionNumber(xdef.dpy), x11_trigger, process_pending_events);
  Xinitialised = TRUE;
}

static void 
x11_finalize_xdef(Drawable d)
{
  /* Default GC */
  GC gc;
  XGCValues context;
  context.foreground = xdef.bgcolor;
  context.background = xdef.bgcolor;
  gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
  xdef.gcclear = gc;
  context.foreground = xdef.bgcolor;
  context.background = xdef.fgcolor;
  gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
  xdef.gccopy = gc;
  gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
  XSetLineAttributes(xdef.dpy, gc, 0, LineDoubleDash, CapButt, JoinMiter);
  XSetStipple(xdef.dpy, gc, gray_stipple[2]);
  xdef.gcdash = gc;
  xdef.gcflag = TRUE;
}



static int 
x11_make_window(int x, int y, int w, int h, char *name)
{
  XSizeHints hints;
#ifdef X11R4
  XWMHints wmhints;
  XClassHint clhints;
  XTextProperty xtpname;
#endif
  XEvent junk;
  XGCValues context;
  XSetWindowAttributes xswattrs;
  unsigned long xswattrs_mask;
  register int i;
  register struct X_window *info;

  if (!Xinitialised)
    x11_init();

  info = NIL;
  for (i = 0; i < MAXWIN; i++)
    ifn(xwind[i].lwin.used) {
      info = &xwind[i];
      break;
    }
  ifn(info)
    error(NIL, "too many X11 windows", NIL);

  if (x == 0 && y == 0) {
    hints.x = xdef.x + 20 * i;
    hints.y = xdef.y + 20 * i;
  } else {
    hints.x = x;
    hints.y = y;
  }

  if (w == 0 && h == 0) {
    w = xdef.w;
    h = xdef.h;
  }

  if (w == 0 || h == 0) {
    hints.width = w = 512;
    hints.height = h = 512;
  } else {
    hints.width = w;
    hints.height = h;
  }

  if (hints.x < 0)
    hints.x = DisplayWidth(xdef.dpy, xdef.screen) + hints.x - w;
  if (y < 0)
    hints.y = DisplayHeight(xdef.dpy, xdef.screen) + hints.y - h;


  /* Protect from interrupts */
  disable();

  /* Create the window */
  info->lwin.used = -1;
  xswattrs.border_pixel = xdef.fgcolor;
  xswattrs.background_pixel = xdef.bgcolor;
  xswattrs.colormap = xdef.cmap;
  xswattrs_mask = (CWBackPixel | CWBorderPixel | CWColormap);
  info->win = XCreateWindow(xdef.dpy, DefaultRootWindow(xdef.dpy),
			    hints.x, hints.y, w, h, 2, xdef.depth,
			    InputOutput, xdef.visual,
			    xswattrs_mask, &xswattrs);
  ifn(info->win) {
    enable();
    error(NIL, "Window creation failed", NIL);
  }

  /* Set properties */
  if (x || y)
    hints.flags = (USPosition | USSize);
  else
    hints.flags = (PPosition | PSize);

#ifdef X11R3
  XSetStandardProperties(xdef.dpy, info->win,
			 name, name, None,
			 NULL, 0, &hints);
#endif
#ifdef X11R4
  wmhints.input = True;
  wmhints.flags = InputHint;
  clhints.res_name = "lush";
  clhints.res_class = "TL";
  ifn (XStringListToTextProperty(&name,1,&xtpname)) {
    enable();
    error(NIL,"memory exhausted",NIL);
  }
  XSetWMProperties(xdef.dpy,info->win,&xtpname,&xtpname,
		   NULL,0, &hints, &wmhints, &clhints );
  if (xdef.wm_delete_window!=None)
    XSetWMProtocols(xdef.dpy,info->win,&xdef.wm_delete_window,1);
#endif

  XSelectInput(xdef.dpy, info->win, ExposureMask);

  /* Map the window */
  XMapRaised(xdef.dpy, info->win);
  XWindowEvent(xdef.dpy, info->win, ExposureMask, &junk);
  XSelectInput(xdef.dpy, info->win, None);
  while (XCheckWindowEvent(xdef.dpy, info->win, ExposureMask, &junk))
    /* DO NOTHING */ ;

  /* Finalize XDEF structure */
  if (!xdef.gcflag) 
    x11_finalize_xdef(info->win);
  XDefineCursor( xdef.dpy, info->win, xdef.ccross);

  /* Create and setup the GC */
  memset(&context, 0, sizeof(context));
  info->gc = XCreateGC(xdef.dpy, info->win, 0, &context);
  XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
  XSetBackground(xdef.dpy, info->gc, xdef.bgcolor);
  XSetFillStyle(xdef.dpy, info->gc, FillSolid);

  /* Misc */
  info->font = NULL;
  info->resizestate = FALSE;
  info->okhilite = FALSE;
  return i;
}



/* ============================  CURSORSUBS */

static int oldcursor = 0;

static void 
setcursor(int flag)
{
  int i;
  struct X_window *info;
  
  if (flag != oldcursor && (flag==0 || flag==1)) {
    oldcursor = flag;
    disable();
    for (i=0, info=xwind; i<MAXWIN; i++, info++)
      if (info->lwin.used)
	{
	  if (!info->lwin.eventhandler)
	    XDefineCursor(xdef.dpy,info->win,xdef.ccross);
	  else if (flag || info->okhilite)
	    XDefineCursor(xdef.dpy,info->win,xdef.carrow);
	  else
	    XDefineCursor(xdef.dpy,info->win,xdef.cwatch);
	}
    XFlush(xdef.dpy);
    enable();
  }
}


/* ============================  HILITESUBS */


static void 
unhilite(struct X_window *info)
{
  int x, y, w, h;

  x = info->xhilite;
  y = info->yhilite;
  w = info->whilite;
  h = info->hhilite;

  x -= 2;
  y -= 2;
  w += 4;
  h += 4;
  if (x<0) x=0;
  if (y<0) y=0;
  
  if (info->whilite && info->hhilite) {
    XCopyArea(xdef.dpy, info->backwin, info->win, xdef.gccopy,
	      x, y, w, h, x, y);
  }
  info->whilite = 0;
  info->hhilite = 0;
}


static void 
x11_hilite(struct window *linfo, int code, int x1, int y1, int x2, int y2)
{
  struct X_window *info = (struct X_window*)linfo;
  int x, y, w, h;

  unhilite(info);
  ifn(info->okhilite)
    return;

#define MIN(x,y) ((x<y)?(x):(y))
#define DIST(x,y) ((x<y)?((y)-(x)):((x)-(y)))

  info->xhilite = x = MIN(x1, x2);
  info->yhilite = y = MIN(y1, y2);
  info->whilite = w = DIST(x1, x2);
  info->hhilite = h = DIST(y1, y2);

#undef DIST
#undef MIN

  switch (code) {

  case HILITE_NONE:
    break;

  case HILITE_VECTOR:
    XDrawLine(xdef.dpy, info->win, xdef.gcdash, x1, y1, x2, y2);
    break;

  case HILITE_INVERT:
    XSetFunction(xdef.dpy, xdef.gccopy, GXcopyInverted);
    XCopyArea(xdef.dpy, info->backwin, info->win, xdef.gccopy, 
	      x, y, w, h, x, y);
    XSetFunction(xdef.dpy, xdef.gccopy, GXcopy);
    /*
     * This old way creates a kind of mask...
     * 
     *     XSetFillStyle(xdef.dpy, xdef.gcdash, FillStippled);
     *     XFillRectangle(xdef.dpy, info->win, xdef.gcdash, x, y, w, h);
     *     XSetFillStyle(xdef.dpy, xdef.gcdash, FillSolid);
     */
    break;

  case HILITE_RECT:
  default:
    XDrawLine(xdef.dpy, info->win, xdef.gcdash, x, y, x + w, y);
    XDrawLine(xdef.dpy, info->win, xdef.gcdash, x, y + h, x + w, y + h);
    XDrawLine(xdef.dpy, info->win, xdef.gcdash, x, y, x, y + h);
    XDrawLine(xdef.dpy, info->win, xdef.gcdash, x + w, y, x + w, y + h);
    break;
  }
  if (info->lwin.eventhandler) {
    XDefineCursor(xdef.dpy,info->win,xdef.carrow);
  }
  XFlush(xdef.dpy);
}


/* ============================  READ EVENTS FROM X11 */


static void x11_refresh(struct X_window *info);
static void x11_resize(struct X_window *info, int x, int y);


static void
handle_async_events(void)
{
  register int i;
  register struct X_window *info;
  register Window win;
  XEvent ev;

  while (XCheckMaskEvent(xdef.dpy, ASYNC_EVENTS, &ev))
    {
      win = ev.xany.window;
      for (i = 0, info = xwind; i < MAXWIN; i++, info++)
	if (xwind[i].lwin.used && win == xwind[i].win)
	  {
	    switch (ev.type) 
	      {
	      case Expose:
		if (ev.xexpose.count == 0)
		  x11_refresh(info);
		break;
		
	      case ConfigureNotify:
		x11_resize(info, ev.xconfigure.width, ev.xconfigure.height);
		info->resizestate = TRUE;
		break;
	      }
	    break;
	  }
    }
}

static void
handle_sync_events(void)
{
  register int i;
  register struct X_window *info;
  register Window win;
  XEvent ev;
  KeySym ks;
  unsigned char s[8];
  static XComposeStatus xcst;
  static Time timedown;
  static char *buttondown = NULL;

  /* search for resized windows */
  for (i = 0, info = xwind; i < MAXWIN; i++, info++)
    if (info->resizestate)
      {
	if (info->lwin.eventhandler)
	  enqueue_event(info->lwin.eventhandler, EVENT_RESIZE,
			info->sizx, info->sizy, 0,0 );
	info->resizestate = FALSE;
      }
  
  /* search for other X events */
  while (XPending(xdef.dpy)) 
    {
      XNextEvent(xdef.dpy, &ev);
      win = ev.xany.window;
      for (i = 0, info = xwind; i < MAXWIN; i++, info++)
	if (xwind[i].lwin.used && win == xwind[i].win)
	  break;
      if (i >= MAXWIN)
	continue;
      switch (ev.type) 
	{
	case MappingNotify:
	  XRefreshKeyboardMapping( (XMappingEvent*) &ev );
	  break;
	  
	case Expose:
	  if (ev.xexpose.count == 0)
	    x11_refresh(info);
	  break;
	  
	case ConfigureNotify:
	  x11_resize(info,
		     ev.xconfigure.width,
		     ev.xconfigure.height);
	  if (info->lwin.eventhandler)
	    enqueue_event(info->lwin.eventhandler,EVENT_RESIZE,
			  ev.xconfigure.width, ev.xconfigure.height,
			  0,0);
	  break;
	  
	case ButtonPress:
	  info->okhilite = TRUE;
	  info->xdown = ev.xbutton.x;
	  info->ydown = ev.xbutton.y;
          
          i = ((ev.xbutton.time - timedown) < 600);
          buttondown = NULL;
          if (ev.xbutton.button == Button1)
            buttondown = (i ? "Button1-Dbl" : "Button1");
          else if (ev.xbutton.button == Button2)
            buttondown = (i ? "Button2-Dbl" : "Button2");
          else if (ev.xbutton.button == Button3)
            buttondown = (i ? "Button3-Dbl" : "Button3");
          timedown = ev.xbutton.time;

	  if (info->lwin.eventhandler)
	    enqueue_eventdesc(info->lwin.eventhandler,EVENT_MOUSE_DOWN,
                              ev.xbutton.x, ev.xbutton.y,
			      ev.xbutton.state & ShiftMask, 
                              ev.xbutton.state & ControlMask,
                              buttondown);

	  break;
	  
	case ButtonRelease:
	  info->okhilite = FALSE;
	  unhilite(info);
	  if (info->lwin.eventhandler && !oldcursor) 
	    {
	      XDefineCursor(xdef.dpy,info->win,xdef.cwatch);
	      XFlush(xdef.dpy);
	    }
	  if (info->lwin.eventhandler)
	    enqueue_eventdesc(info->lwin.eventhandler,EVENT_MOUSE_UP,
                              info->xdown, info->ydown,
                              ev.xbutton.x, ev.xbutton.y,
                              buttondown);
	  break;
	  
	case KeyPress:
	  i = XLookupString(&ev.xkey, (char*)s, 7, &ks, &xcst);
	  if (info->lwin.eventhandler) 
	    {
              char *desc = XKeysymToString(ks);
              switch(ks) 
                {
                case XK_Up:
                  i = EVENT_ARROW_UP;
                  break;
                case XK_Right:
                  i = EVENT_ARROW_RIGHT;
                  break;
                case XK_Down:
                  i = EVENT_ARROW_DOWN;
                  break;
                case XK_Left:
                  i = EVENT_ARROW_LEFT;
                  break;
#ifdef XK_Help
                case XK_Help:
                  i = EVENT_HELP;
                  break;
#endif
                default:
                  if (i == 1 && (ev.xkey.state&Mod1Mask))
                    {
                      i = ASCII_TO_EVENT((s[0]|0x80));
                    }
                  else if (i == 1)
                    {
                      i = ASCII_TO_EVENT((s[0]));
                      if (ks == XK_Delete && s[0]==0x7f)
                        i = EVENT_FKEY;
                    }
                  else
                    {
                      if (desc && !IsModifierKey(ks))
                        i = EVENT_FKEY;
                      else
                        i = EVENT_NONE;
                    }
                  break;
                }
              if (i>=EVENT_ASCII_MIN && i<=EVENT_ASCII_MAX)
                ev.xbutton.state = 0;
              if (i!=EVENT_NONE)
		enqueue_eventdesc(info->lwin.eventhandler,i,
                                  ev.xbutton.x, ev.xbutton.y,
                                  ev.xbutton.state & ShiftMask, 
                                  ev.xbutton.state & ControlMask,
                                  desc);
	    }
	  break;
    
	case MotionNotify:
	  if (info->lwin.eventhandler)
	    enqueue_eventdesc(info->lwin.eventhandler,EVENT_MOUSE_DRAG,
                              info->xdown, info->ydown,
                              ev.xbutton.x, ev.xbutton.y,
                              buttondown);
	  break;
	  
#ifdef X11R4
	case ClientMessage:
	  if (xdef.wm_delete_window != None &&
	      ev.xclient.message_type == xdef.wm_protocols &&
	      ev.xclient.data.l[0] == xdef.wm_delete_window ) {
	    if (info->lwin.eventhandler)
	      enqueue_event(info->lwin.eventhandler,EVENT_DELETE,0,0,0,0);
	    else
	      delete_at(info->lwin.backptr);
	  }
	  break;
#endif
	}
    }
}


/* ============================  REFRESH_WINDOW */


static void 
copy_buffer(struct X_window *info)
{
  if (info->lwin.used && info->x1>=0) {
#ifdef aix
    info->x1 = (info->x1 - 2) & 0xfff0 + 2;
    info->x2 = (info->x2 + 17) & 0xfff0 - 2;
#endif
    XCopyArea(xdef.dpy, info->backwin, info->win,
	      xdef.gccopy, info->x1 - 2, info->y1 - 2,
	      info->x2 - info->x1 + 4, info->y2 - info->y1 + 4,
	      info->x1 - 2, info->y1 - 2);
    info->x1 = info->y1 = info->x2 = info->y2 = -1;
  }
}

static void 
grow_rect(struct X_window *info, int x1, int y1, int x2, int y2)
{
  int z;

  if (x2 < x1) {
    z = x1;
    x1 = x2;
    x2 = z;
  }
  if (y2 < y1) {
    z = y1;
    y1 = y2;
    y2 = z;
  }
  if (info->x1 >= 0) {
    x1 = ((info->x1 < x1) ? info->x1 : x1);
    y1 = ((info->y1 < y1) ? info->y1 : y1);
    x2 = ((info->x2 > x2) ? info->x2 : x2);
    y2 = ((info->y2 > y2) ? info->y2 : y2);
  } else {
    x2 = ((x2 < 0) ? 0 : x2);
    y2 = ((y2 < 0) ? 0 : y2);
    x1 = ((x1 > info->sizx) ? info->sizx : x1);
    y1 = ((y1 > info->sizy) ? info->sizy : y1);
  }
  info->x1 = ((x1 < 0) ? 0 : x1);
  info->y1 = ((y1 < 0) ? 0 : y1);
  info->x2 = ((x2 > info->sizx) ? info->sizx : x2);
  info->y2 = ((y2 > info->sizy) ? info->sizy : y2);
}

static void 
x11_begin(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  disable();
  if ((info->dbflag++) == 0) {
    info->x1 = info->y1 = info->x2 = info->y2 = -1;
  }
}

static void 
x11_end(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  if (--(info->dbflag) == 0) {
    copy_buffer(info);
    XSync(xdef.dpy,0);
  }
  enable();
}

static void 
x11_close(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  disable();
  XFreePixmap(xdef.dpy, info->backwin);
  XFreeGC(xdef.dpy, info->gc);
  XUndefineCursor( xdef.dpy, info->win);
  XUnmapWindow(xdef.dpy, info->win);
  XDestroyWindow(xdef.dpy, info->win);
  XFlush(xdef.dpy);
  info->lwin.used = 0;
  enable();
}

static int 
x11_xsize(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  return info->sizx;
}

static int 
x11_ysize(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  return info->sizy;
}

static void 
x11_setcolor(struct window *linfo, int xx)
{
  struct X_window *info = (struct X_window*)linfo;
  int x;

  switch (xx) {

  case COLOR_FG:
    XSetFillStyle(xdef.dpy, info->gc, FillSolid);
    XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
    break;
  case COLOR_BG:
    XSetFillStyle(xdef.dpy, info->gc, FillSolid);
    XSetForeground(xdef.dpy, info->gc, xdef.bgcolor);
    break;
  case COLOR_GRAY:
    XSetStipple(xdef.dpy, info->gc, gray_stipple[4]);
    XSetFillStyle(xdef.dpy, info->gc, FillOpaqueStippled);
    XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
    break;
  default:
    if (xdef.depth < MINDEPTH) {
      x = xx / 32;
      if (x < 1) {
	XSetFillStyle(xdef.dpy, info->gc, FillSolid);
	XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
      } else if (x > 6) {
	XSetFillStyle(xdef.dpy, info->gc, FillSolid);
	XSetForeground(xdef.dpy, info->gc, xdef.bgcolor);
      } else {
	XSetStipple(xdef.dpy, info->gc, gray_stipple[7 - x]);
	XSetFillStyle(xdef.dpy, info->gc, FillOpaqueStippled);
	XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
      }
    } else {
      if (xx < 0 || xx >= (1 << xdef.depth))
	return;
      XSetFillStyle(xdef.dpy, info->gc, FillSolid);
      XSetForeground(xdef.dpy, info->gc, xx);
    }
  }
}


#define COLORCACHE 256
static XColor colorcache[COLORCACHE];
static int whereinthecache = 0, lasthitinthecache = 0;


static int 
x11_alloccolor(struct window *linfo, double r, double g, double b)
{
  if (xdef.depth >= MINDEPTH) {
    int i;
    XColor xc;

    xc.pixel = COLOR_GRAY;
    xc.red = (long)(r*65535)&0xff00;
    xc.green = (long)(g*65535)&0xff00;
    xc.blue = (long)(b*65535)&0xff00;

    for (i = lasthitinthecache; i < whereinthecache; i++)
      if (colorcache[i].red == xc.red &&
	  colorcache[i].green == xc.green &&
	  colorcache[i].blue == xc.blue) {
	lasthitinthecache = i;
	return colorcache[i].pixel;
      }
    for (i = 0; i < lasthitinthecache; i++)
      if (colorcache[i].red == xc.red &&
	  colorcache[i].green == xc.green &&
	  colorcache[i].blue == xc.blue) {
	lasthitinthecache = i;
	return colorcache[i].pixel;
      }


    XAllocColor(xdef.dpy,xdef.cmap,&xc);

    /* If it fails, try a new colormap! */
    if (xc.pixel==COLOR_GRAY && !xdef.cmapflag) {
      xdef.cmap = XCopyColormapAndFree(xdef.dpy,xdef.cmap);
      xdef.cmapflag = 1;
      for (i=0; i<MAXWIN; i++)
	if (xwind[i].lwin.used) 
	  XSetWindowColormap(xdef.dpy, xwind[i].win, xdef.cmap);
      XAllocColor(xdef.dpy,xdef.cmap,&xc);
    }

    if (xc.pixel!=COLOR_GRAY)
      if (whereinthecache < COLORCACHE) {
	colorcache[whereinthecache].pixel = xc.pixel;
	colorcache[whereinthecache].red = r * 65535;
	colorcache[whereinthecache].green = g * 65535;
	colorcache[whereinthecache].blue = b * 65535;
	whereinthecache++;
      }
    return xc.pixel;

  } else {
    int x;

    x = 16 * (5 * r + 9 * g + 2 * b);
    if (x >= 256)
      x = 255;
    return x;
  }
}







#define FONTCACHE 8

static struct {
  char name[128];
  XFontStruct *fs;
} fontcache[FONTCACHE];

static int lastfont = 0;


static char*
psfonttoxfont(char *f)
{
  /* convert a PS name to a X... */
  char type[10];
  int size=0;
  static char copy[256];
  static char name[256];
  static char *wght[] = { "bold","demi","light","demibold","book",0 };
  char *s;
  
  if (*f=='-')
    return f;
  strcpy(copy,f);
  s = copy;
  while (*s) {
    *s = tolower((unsigned char)*s);
    s++;
  }
  f = copy+strlen(copy);
  s = strchr(copy,'-');
  if (!s) {
    strcpy(type,"medium-r");
  } else {
    *s=0;
    s++;
    for (size=0;wght[size];size++)
      if (!strncmp(s,wght[size],strlen(wght[size]))) {
	strcpy(type,wght[size]);
	strcat(type,"-");
	s+=strlen(wght[size]);
	break;
      }
    ifn (wght[size])
      strcpy(type,"medium-");
    switch (*s) {
    case 'i':
      strcat(type,"i");
      break;
    case 'o':
      strcat(type,"o");
      break;
    default:
      strcat(type,"r");
      break;
    }
  }

  size = 11;
  while (f[-1]>='0' && f[-1]<='9')
    f--;
  
  if (*f)
    size = atoi(f);
  f[0] = 0;
  if (f[-1]=='-')
    f[-1] = 0;
  sprintf(name,"-*-%s-%s-normal-*-%d-*",
	  copy, type, size );
  return name;
}

static void 
x11_setfont(struct window *linfo, char *f)
{
  struct X_window *info = (struct X_window*)linfo;
  Font fid;
  XFontStruct *xfs = 0;
  int i;
  char *xf;
  
  xf = psfonttoxfont(f);
  ifn (strcmp(f, FONT_STD)) {
    xfs = fontcache[0].fs;
    i = 0;
  } else {
    for (i = 0; i < FONTCACHE; i++)
      if (fontcache[i].fs) {
	if (strcmp(f, fontcache[i].name)==0) {
	  xfs = fontcache[i].fs;
	  break;
	}
	if (strcmp(xf, fontcache[i].name)==0) {
	  xfs = fontcache[i].fs;
	  break;
	}
      }
  }
  
  if (i >= FONTCACHE) {
    i = lastfont;
    if (++lastfont >= FONTCACHE)
      lastfont = 1;
    badname = 0;
    fid = XLoadFont(xdef.dpy,f);
    XSync(xdef.dpy,False);
    if (badname) {
      badname = 0;
      fid = XLoadFont(xdef.dpy,xf);
      XSync(xdef.dpy,False);
      if (badname)
	error(NIL, "Cannot load font", new_string(f));
      f = xf;
    }
    badname = 1;
    xfs = XQueryFont(xdef.dpy,fid);
    if (!xfs)
      error(NIL,"Problem while loading font",new_string(f));
    if (fontcache[i].fs)
      XFreeFont(xdef.dpy, fontcache[i].fs);
    strcpy(fontcache[i].name, f);
    fontcache[i].fs = xfs;
  }
  XSetFont(xdef.dpy, info->gc, xfs->fid);
  info->font = xfs;
}

static void 
x11_clear(struct window *linfo)
{
  struct X_window *info = (struct X_window*)linfo;
  XSetForeground(xdef.dpy, info->gc, xdef.bgcolor);
  XSetFillStyle(xdef.dpy, info->gc, FillSolid);
  XFillRectangle(xdef.dpy, info->backwin, info->gc,
		 0, 0, info->sizx, info->sizy);
  x11_setcolor(&info->lwin, info->lwin.color);
  grow_rect(info, 0, 0, info->sizx, info->sizy);
}

static void 
x11_draw_line(struct window *linfo, int x1, int y1, int x2, int y2)
{
  struct X_window *info = (struct X_window*)linfo;
  XDrawLine(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
  grow_rect(info, x1, y1, x2, y2);
}

static void 
x11_draw_rect(struct window *linfo,
	      int x1, int y1, unsigned int x2, unsigned int y2)
{
  struct X_window *info = (struct X_window*)linfo;
  XDrawRectangle(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
  grow_rect(info, x1, y1, x1 + x2, y1 + y2);
}

static void 
x11_draw_circle(struct window *linfo,
		int x1, int y1, unsigned int r)
{
  struct X_window *info = (struct X_window*)linfo;
  XDrawArc(xdef.dpy, info->backwin, info->gc, 
	   x1 - r, y1 - r, 2 * r, 2 * r, 0, 360 * 64);
  grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}

static void 
x11_draw_arc(struct window *linfo, 
	     int x1, int y1, unsigned int r, int from, int to)
{
  struct X_window *info = (struct X_window*)linfo;
  XDrawArc(xdef.dpy, info->backwin, info->gc, 
	   x1 - r, y1 - r, 2 * r, 2 * r, 
	   from*64, (to-from)*64 );
  grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}

static void 
x11_fill_rect(struct window *linfo, 
	      int x1, int y1, unsigned int x2, unsigned int y2)
{
  struct X_window *info = (struct X_window*)linfo;
  XFillRectangle(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
  grow_rect(info, x1, y1, x1 + x2, y1 + y2);
}

static void 
x11_fill_circle(struct window *linfo, int x1, int y1, unsigned int r)
{
  struct X_window *info = (struct X_window*)linfo;
  XFillArc(xdef.dpy, info->backwin, info->gc, 
	   x1 - r, y1 - r, 2 * r, 2 * r, 0, 360 * 64);
  grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}


static void 
x11_fill_arc(struct window *linfo, 
	     int x1, int y1, unsigned int r, int from, int to)
{
  struct X_window *info = (struct X_window*)linfo;
  XFillArc(xdef.dpy, info->backwin, info->gc, 
	   x1 - r, y1 - r, 2 * r, 2 * r,
	   from*64, (to-from)*64 );
  grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}


static void 
x11_fill_polygon(struct window *linfo, short (*points)[2], unsigned int n)
{
  struct X_window *info = (struct X_window*)linfo;
  int i, xmax, xmin, ymax, ymin;
  short (*d)[2];

  d = points;
  xmax = xmin = (*d)[0];
  ymin = ymax = (*d)[1];

#define MAX(x,y) (((x)>(y))?(x):(y))
#define MIN(x,y) (((x)<(y))?(x):(y))
  for (i = 1; i < n; i++) {
    d++;
    xmax = MAX(xmax, (*d)[0]);
    xmin = MIN(xmin, (*d)[0]);
    ymax = MAX(ymax, (*d)[1]);
    ymin = MIN(ymin, (*d)[1]);
  }
#undef MAX
#undef MIN

  XFillPolygon(xdef.dpy, info->backwin, info->gc,
	       (XPoint *) points, n, Complex, CoordModeOrigin);
  grow_rect(info, xmin, ymin, xmax, ymax);
}

static void 
x11_draw_text(struct window *linfo, int x, int y, char *s)
{
  struct X_window *info = (struct X_window*)linfo;
  int i, junk;
  XCharStruct overall;

  i = strlen(s);
  XTextExtents(info->font, s, i, &junk, &junk, &junk, &overall);
  XDrawString(xdef.dpy, info->backwin, info->gc, x, y, s, i);
  grow_rect(info, x - overall.lbearing - 5, y - overall.ascent,
	    x + overall.rbearing + 5, y + overall.descent);
}

static void 
x11_rect_text(struct window *linfo, 
	      int x, int y, char *s, 
	      int *xptr, int *yptr, int *wptr, int *hptr)
{
  struct X_window *info = (struct X_window*)linfo;
  int i, junk;
  XCharStruct overall;

  i = strlen(s);
  XTextExtents(info->font, s, i, &junk, &junk, &junk, &overall);
  *xptr = x - overall.lbearing;
  *yptr = y - overall.ascent;
  if (overall.rbearing > overall.width)
    *wptr = overall.lbearing + overall.rbearing;
  else
    *wptr = overall.lbearing + overall.width;
  *hptr = overall.ascent + overall.descent;
}



static void
error_diffusion(unsigned int *image, int w, int h)
{
  unsigned int *im = image;
  unsigned char *data;
  int i, j, err;

  for (i = 0; i < h - 1; i++) {
    (*im > 127) ? (err = *im - 255, *im = 1) : (err = *im, *im = 0);
    im[1] += (err * 7) >> 4;
    im[w] += (err * 5) >> 4;
    im[w + 1] += err >> 4;
    im++;
    for (j = 1; j < w - 1; j++) {
      (*im > 127) ? (err = *im - 255, *im = 1) : (err = *im, *im = 0);
      im[1] += (err * 7) >> 4;
      im[w - 1] += (err * 3) >> 4;
      im[w] += (err * 5) >> 4;
      im[w + 1] += err >> 4;
      im++;
    }
    (*im > 127) ? (err = *im - 255, *im = 1) : (err = *im, *im = 0);
    im[w - 1] += (err * 3) >> 4;
    im[w] += (err * 5) >> 4;
    im++;
  }
  for (j = 0; j < w - 1; j++) {
    (*im > 127) ? (err = *im - 255, *im = 1) : (err = *im, *im = 0);
    im[1] += (err * 7) >> 4;
    im++;
  }
  (*im > 127) ? (err = *im - 255, *im = 1) : (err = *im, *im = 0);

  im = image;
  data = (unsigned char *) image;

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      err <<= 1;
      if (*im++)
	err |= 1;
      if (j % 8 == 7)
	*data++ = err;
    }
    if (j % 8) {
      err <<= 8 - j % 8;
      *data++ = err;
    }
  }
}



static int 
x11_pixel_map(struct window *linfo, unsigned int *image, 
	      int x, int y, unsigned int w, unsigned int h, 
	      unsigned int sx, unsigned int sy)
{
  struct X_window *info = (struct X_window*)linfo;
  XImage *ximage;
  register int i, j, k, l;
  register unsigned int *im, *jm, *image2;
  int sx_w = sx * w;
  int sy_h = sy * h;

  union { int i; char c[sizeof(int)]; } test;
  test.i=1;
  

  if (!image) {
    if (sx * sy > 9 && xdef.depth >= MINDEPTH) /* test if ok */
      return FALSE;
    else
      return TRUE;
  }
  if (sx == 1 && sy == 1) {

    if (xdef.depth < MINDEPTH) {
      error_diffusion(image, w, h);
      ximage = XCreateImage(xdef.dpy, DefaultVisual(xdef.dpy, 0),
			    1, XYBitmap,
			    0, (void *) image, w, h, 8, 0);
      ximage->byte_order = MSBFirst;
      ximage->bitmap_bit_order = MSBFirst;
    } else {
      ximage = XCreateImage(xdef.dpy, DefaultVisual(xdef.dpy, 0),
			    xdef.depth, ZPixmap,
			    0, (void *) image, w, h, 32,
			    w * sizeof(int));

      ximage->byte_order = (test.c[0] ? LSBFirst: MSBFirst);
      ximage->bitmap_pad = sizeof(int) * 8;
      ximage->bitmap_unit = sizeof(int) * 8;
      ximage->bits_per_pixel = sizeof(int) * 8;
      ximage->bytes_per_line = w * sizeof(int);
    }

    XSetForeground(xdef.dpy, info->gc,
		   WhitePixel(xdef.dpy, xdef.screen));
    XSetBackground(xdef.dpy, info->gc,
		   BlackPixel(xdef.dpy, xdef.screen));
    XPutImage(xdef.dpy, info->backwin, info->gc,
	      ximage, 0, 0, x, y, w, h);
    XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
    XSetBackground(xdef.dpy, info->gc, xdef.bgcolor);
    x11_setcolor(&info->lwin, info->lwin.color);

    ximage->obdata = NULL;
    ximage->data = NULL;
    XDestroyImage(ximage);	/* free imchar */

  } else {
    ifn (sx>0 && sy>0)
      return FALSE;
    ifn (image2 = malloc(sx_w * sy_h * sizeof(int)))
      return FALSE;
    
    for (i = 0; i < h; i++) {
      im = image2 + sx_w * sy * i;
      for (k = 0; k < w; k++, image++) {
	register int c;
	c = *image;
	jm = im;
	for (j = 0; j < sy; j++, jm += sx_w)
	  for (l = 0; l < sx; l++)
	    jm[l] = c;
	im += sx;
      }
    }

    if (xdef.depth < MINDEPTH) {
      error_diffusion(image2, sx_w, sy_h);
      ximage = XCreateImage(xdef.dpy, DefaultVisual(xdef.dpy, 0),
			    1, XYBitmap,
			    0, (void *) image2, sx_w, sy_h, 8, 0);
      ximage->byte_order = MSBFirst;
      ximage->bitmap_bit_order = MSBFirst;
    } else {
      ximage = XCreateImage(xdef.dpy, DefaultVisual(xdef.dpy, 0),
			    xdef.depth, ZPixmap,
			    0, (void *) image2, sx_w, sy_h, 32,
			    sx_w * sizeof(int));

      ximage->byte_order = (test.c[0] ? LSBFirst: MSBFirst);
      ximage->bitmap_pad = sizeof(int) * 8;
      ximage->bitmap_unit = sizeof(int) * 8;
      ximage->bits_per_pixel = sizeof(int) * 8;
      ximage->bytes_per_line = sx_w * sizeof(int);
    }

    XSetForeground(xdef.dpy, info->gc,
		   WhitePixel(xdef.dpy, xdef.screen));
    XSetBackground(xdef.dpy, info->gc,
		   BlackPixel(xdef.dpy, xdef.screen));
    XPutImage(xdef.dpy, info->backwin, info->gc,
	      ximage, 0, 0, x, y, sx_w, sy_h);
    XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
    XSetBackground(xdef.dpy, info->gc, xdef.bgcolor);
    x11_setcolor(&info->lwin, info->lwin.color);

    XDestroyImage(ximage);	/* free imchar */
  }

  grow_rect(info, x, y, x + sx_w, y + sy_h);
  return TRUE;
}


static void 
x11_clip(struct window *linfo, int x, int y, unsigned int w, unsigned int h)
{
  struct X_window *info = (struct X_window*)linfo;
  XRectangle rect;

  if (w == 0 && h == 0)
    XSetClipMask(xdef.dpy, info->gc, None);
  else {
    rect.x = x;
    rect.y = y;
    rect.width = w;
    rect.height = h;
    XSetClipRectangles(xdef.dpy, info->gc, 0, 0, &rect, 1, Unsorted);
  }
}

void
x11_get_image(struct window *linfo, unsigned int *image, 
              int x, int y, unsigned int w, unsigned int h)
{
  register int i,j;
  register unsigned int *im;
  struct X_window *info = (struct X_window*)linfo;

  XImage *ximage;
  ximage = XGetImage(xdef.dpy, info->backwin, x, y, w, h, ~0, ZPixmap);
  im = image;
  for (i=0; i<h; i++) {
    for (j=0; j<w; j++) {
      *im++ = XGetPixel(ximage,j,i);
    }
  }
  XDestroyImage(ximage);
}


int 
x11_get_mask(struct window *linfo, 
             unsigned int *red_mask, 
             unsigned int *green_mask, 
             unsigned int *blue_mask  )
{
  if (xdef.red_mask && xdef.green_mask && xdef.blue_mask)
    {
      *red_mask = xdef.red_mask;
      *green_mask = xdef.green_mask;
      *blue_mask = xdef.blue_mask;
      return 1;
    }
  return 0;
}


struct gdriver x11_driver = {
  "X11",
  /* release 1 */
  x11_begin,
  x11_end,
  x11_close,
  x11_xsize,
  x11_ysize,
  x11_setfont,
  x11_clear,
  x11_draw_line,
  x11_draw_rect,
  x11_draw_circle,
  x11_fill_rect,
  x11_fill_circle,
  x11_draw_text,
  /* release 2 */
  x11_setcolor,
  x11_alloccolor,
  x11_fill_polygon,
  x11_rect_text,
  NIL,				/* x11_gspecial, */
  x11_clip,
  x11_hilite,
  x11_pixel_map,
  NIL,				/* x11_hinton_map */
  /* release 3 */
  x11_draw_arc,
  x11_fill_arc,
  /* import from sn3.2*/
  x11_get_image,
  x11_get_mask,
};




static void 
x11_resize(struct X_window *info, int x, int y)
{
  int w, h;
  Pixmap newbackwin;

  if (x != info->sizx || y != info->sizy) {
    w = (info->sizx < x) ? info->sizx : x;
    h = (info->sizy < y) ? info->sizy : y;
    info->sizx = x;
    info->sizy = y;
    newbackwin = XCreatePixmap(xdef.dpy,
			       DefaultRootWindow(xdef.dpy),
			       x, y, xdef.depth);
    if (newbackwin && info->backwin) {
      XFillRectangle(xdef.dpy, newbackwin,
		     xdef.gcclear, 0, 0, x, y);

      XCopyArea(xdef.dpy, info->backwin, newbackwin,
		xdef.gccopy, 0, 0, w, h, 0, 0);
      XFreePixmap(xdef.dpy, info->backwin);
    }
    if (newbackwin)
      info->backwin = newbackwin;
    XFlush(xdef.dpy);
  }
}

static void 
x11_refresh(struct X_window *info)
{

  XCopyArea(xdef.dpy, info->backwin, info->win,
	    xdef.gccopy, 0, 0, info->sizx, info->sizy, 0, 0);
  XFlush(xdef.dpy);
}



static at *
x11_window(int x, int y, unsigned int w, unsigned int h, char *name)
{
  struct X_window *info;
  int ijunk;
  unsigned int ujunk;
  at *ans;
  Window wjunk;

  info = xwind + x11_make_window(x, y, w, h, name);


  XGetGeometry(xdef.dpy, info->win, &wjunk, &ijunk, &ijunk,
	       &w, &h, &ujunk, &ujunk);
  info->sizx = w;
  info->sizy = h;
  info->dbflag = 0;
  ifn(info->backwin = XCreatePixmap(xdef.dpy,
				    DefaultRootWindow(xdef.dpy),
				    w, h, xdef.depth)) {
    enable();
    error(NIL, "can't create offscreen pixmap", NIL);
  }
  XSelectInput(xdef.dpy, info->win, SYNC_EVENTS|ASYNC_EVENTS);

  info->lwin.gdriver = &x11_driver;
  info->lwin.eventhandler = 0;
  info->lwin.clipw = 0;
  info->lwin.cliph = 0;

  ans = new_extern(&window_class, info);
  info->lwin.backptr = ans;

  x11_setcolor(&info->lwin, COLOR_FG);
  info->lwin.color = COLOR_FG;

  x11_setfont(&info->lwin, xdef.font);
  info->lwin.font = new_string(xdef.font);

  x11_clear(&info->lwin);

  enable();
  return ans;
}

DX(xx11_window)
{
  at *ans;

  ALL_ARGS_EVAL;
  switch (arg_number) {
  case 0:
    ans = x11_window(0, 0,
		     0, 0, "Graphics");
    break;
  case 1:
    ans = x11_window(0, 0,
		     0, 0, ASTRING(1));
    break;
  case 3:
    ans = x11_window(0, 0,
		     AINTEGER(1), AINTEGER(2), ASTRING(3));
    break;
  case 5:
    ans = x11_window(AINTEGER(1), AINTEGER(2),
		     AINTEGER(3), AINTEGER(4), ASTRING(5));
    break;
  default:
    ARG_NUMBER(-1);
    return NIL;
  }
  return ans;
}



/* ============================  X11 ONLY */

DX(xx11_depth)
{
  ARG_NUMBER(0);
  if (!Xinitialised)
    x11_init();
  return NEW_NUMBER(xdef.depth);
}

DX(xx11_fontname)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(psfonttoxfont(ASTRING(1)));
}

DX(xx11_configure)
{
  struct window *w;
  struct window *current_window(void);
  struct X_window *info;
  
  ALL_ARGS_EVAL;
  w = current_window();
  if (w->gdriver == &x11_driver)
    {
#ifdef X11R4      
      int mask = 0;
      XWindowChanges xwc;
      XWindowAttributes xwa;
      
      info = (struct X_window*)w;
      if (arg_number!=0)
	{
	  ARG_NUMBER(5);
	  if (APOINTER(1)) {
	    xwc.stack_mode = Above;
	    mask |= CWStackMode;
	  }
	  if (APOINTER(2)) {
	    xwc.x = AINTEGER(2);
	    mask |= CWX;
	  }
	  if (APOINTER(3)) {
	    xwc.y = AINTEGER(3);
	    mask |= CWY;
	  }
	  if (APOINTER(4)) {
	    xwc.width = AINTEGER(4);
	    mask |= CWWidth;
	  }
	  if (APOINTER(5)) {
	    xwc.height = AINTEGER(5);
	    mask |= CWHeight;
	  }
	}
      disable();
      if (mask & CWStackMode) 
	XMapRaised(xdef.dpy,info->win);
      if (mask)
	XReconfigureWMWindow(xdef.dpy,info->win,
			     xdef.screen, mask, &xwc);
      XGetWindowAttributes(xdef.dpy,info->win,&xwa);
      enable();
      return cons((xwa.map_state==IsViewable ? true() : NIL ),
		  cons(NEW_NUMBER(xwa.width),
		       cons(NEW_NUMBER(xwa.height), NIL) ) );
#endif
    }
  return NIL;
}



DX(xx11_lookup_color)
{
  char *s;
  real rgb[3];
  XColor xc1, xc2;
  int status;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  if (!Xinitialised)
    x11_init();
  disable();
  status = XLookupColor(xdef.dpy, xdef.cmap, s, &xc1, &xc2);
  enable();
  if (status) {
    rgb[0] = (real)(xc1.red)/65535.0;    
    rgb[1] = (real)(xc1.green)/65535.0;    
    rgb[2] = (real)(xc1.blue)/65535.0;
    return cons(NEW_NUMBER(rgb[0]),
		cons(NEW_NUMBER(rgb[1]),
		     cons(NEW_NUMBER(rgb[2]), NIL)));
  }
  else
    return NIL;
}



DX(xx11_text_to_clip)
{
  char *s;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  if (!Xinitialised)
    x11_init();
  disable();
  XStoreBuffer(xdef.dpy,s,strlen(s),0);
#ifdef X11R4
  XSetSelectionOwner(xdef.dpy,XA_PRIMARY,None,CurrentTime);
  /* TODO: Own CLIPBOARD (and SHELF?) selections */
#endif
  enable();
  LOCK(APOINTER(1));
  return APOINTER(1);
}



DX(xx11_clip_to_text)
{
  char *buf = NULL;
  char *s;
  at *p;
  int nbytes;

  ARG_NUMBER(0);
  if (!Xinitialised)
    x11_init();
  disable();

#ifdef X11R4
  /* Check CLIPBOARD (and SHELF?) selections */
#endif
  
  if (!buf)
    {
      buf = XFetchBuffer(xdef.dpy,&nbytes,0);
      if (nbytes==0) {
	p = NIL;
      } else {
	p = new_string_bylen(nbytes+1);
	s = SADD(p->Object);
	strcpy(s,buf);
	s[nbytes] = 0;
	XFree(buf);
      }
    }
  enable();
  return p;
}



/* ============================  INITIALISATION */


void
init_X11driver(void)
{
  display = var_define("display");
  dx_define("x11_window", xx11_window);
  dx_define("x11_fontname", xx11_fontname);
  dx_define("x11_depth", xx11_depth);
  dx_define("x11_configure", xx11_configure);
  dx_define("x11_lookup_color", xx11_lookup_color);
  dx_define("x11_clip_to_text", xx11_clip_to_text);
  dx_define("x11_text_to_clip", xx11_text_to_clip);
}


/* ============================================================ */
#endif /* X_DISPLAY_MISSING */
