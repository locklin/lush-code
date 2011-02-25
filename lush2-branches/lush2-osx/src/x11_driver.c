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

/***********************************************************************
  Graphic driver for X11
 ********************************************************************** */

#ifdef HAVE_CONFIG_H
# include "lushconf.h"
#endif
#include "header.h"
#include "graphics.h"

#ifndef X_DISPLAY_MISSING
/* ============================================================ */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifndef X11RELEASE
# ifdef XlibSpecificationRelease
#  define X11RELEASE XlibSpecificationRelease
# else
#  ifdef PWinGravity
#   define X11RELEASE 4
#  else 
#   define X11RELEASE 3
#  endif
# endif
#endif

#if X11RELEASE >= 4
# include <X11/Xatom.h>
#endif

#ifndef MAKEDEPEND
# if HAVE_XFT
#  include <X11/Xft/Xft.h>
#  ifdef XFT_MAJOR
#   if XFT_MAJOR >= 2
#    define HAVE_XFT2 1
#   endif
#  endif
# endif
#endif

#include <errno.h>

#ifdef class
# undef class
#endif

typedef unsigned int   _uint;  // avoid compiler warning because of redefine
#ifdef uint
# undef uint
#endif
#define uint _uint

/* ============================  X11DRIVER STRUCTURES */

#define MAXWIN 64
#define MAXFONT 96
#define MINDEPTH 2
#define ASYNC_EVENTS (ExposureMask|StructureNotifyMask)
#define SYNC_EVENTS  (ButtonPressMask|ButtonReleaseMask|KeyPressMask|ButtonMotionMask)

#if MAXFONT <= MAXWIN
# error "MAXFONT must be greater than MAXWIN"
#endif

/* pointeur to the var 'display' */
static at *display;

int Xinitialised = false;
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
   GC       gcclear, gccopy, gcdash;
   int      gcflag;
} xdef;

static struct X_font {
   char *name1;
   char *name2;
   XFontStruct *xfs;
#if HAVE_XFT2
   XftFont *xft;
#endif
} *fontcache[MAXFONT];

struct X_window {
   window_t lwin;
   Window win;
   GC gc;
   struct X_font *font;
#if HAVE_XFT2
   XftColor xftcolor;
#endif
   Pixmap backwin;
   int sizx, sizy;
   int dbflag;
   int x1, y1, x2, y2;
   int resizestate, xdown, ydown;
   int hilitestate, okhilite, xhilite, yhilite, whilite, hhilite;
   int dpos;
   struct X_window *next;
}; //xwind[MAXWIN];

static struct X_window *xwins = NULL;

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

#define enable()   unblock_async_poll()
#define disable()  block_async_poll()

static void setcursor(int flag);
static int  handle_sync_events(void);
static void handle_async_events(void);

static void unchain_xwin(struct X_window *w)
{
   struct X_window **where = &xwins;
   while (*where) {
      if (*where == w) {
         *where = w->next;
         return;
      } else
         where = &((*where)->next);
   }
   assert(0);
}

/* ============================  INITIALISATION & DEFAULTS */

static char *search_display_name(void)
{
   static char dpyname[128] = "unix:0.0";
   extern char *getenv(const char *);

   char *s;
   at *p = var_get(display);
   if (STRINGP(p))
      strcpy(dpyname, String(p));
   else if ((s = getenv("DISPLAY")))
      strcpy(dpyname, s);
   return dpyname;
}


static int badname = 1;

static int x11_handler(Display *display, XErrorEvent *myerr)
{
   char msg[80];
   if (badname==0 && myerr->error_code==BadName) {
      badname = 1;
   } else {
      XGetErrorText(display, myerr->error_code, msg, 80);
      fprintf(stderr,"*** Xlib error %d : %s\n", myerr->error_code, msg);
   }
   return 0;
}

static void x11_bwait(void)
{
   setcursor(1);
}

static void x11_ewait(void)
{
   setcursor(0);
}

static void  x11_init(void)
{
   /* Open display */
   char *dpyname = search_display_name();
   xdef.dpy = XOpenDisplay(dpyname);
   ifn (xdef.dpy)
      error(NIL, "X11: Can't open display on", make_string(dpyname));

   /* Check if default visual is good enough or look for a better one... */
   XVisualInfo vinfo, *vinfoptr;
   xdef.screen = DefaultScreen(xdef.dpy);
   xdef.visual = DefaultVisual(xdef.dpy, xdef.screen);
   xdef.depth = DisplayPlanes(xdef.dpy, xdef.screen);
   xdef.red_mask = 0;
   xdef.green_mask = 0;
   xdef.blue_mask = 0;
   xdef.cmap = DefaultColormap(xdef.dpy, xdef.screen);
   xdef.cmapflag = 0;
   vinfo.visualid = XVisualIDFromVisual(xdef.visual);
   int i;
   vinfoptr = XGetVisualInfo(xdef.dpy, VisualIDMask, &vinfo, &i);
   if (vinfoptr) {
      vinfo = *vinfoptr;
      XFree(vinfoptr);
      if (vinfo.class == TrueColor || vinfo.class == DirectColor) {
         xdef.red_mask = vinfo.red_mask;
         xdef.green_mask = vinfo.green_mask;
         xdef.blue_mask = vinfo.blue_mask;
      }
      else if (XMatchVisualInfo(xdef.dpy, xdef.screen, 24, TrueColor, &vinfo) ||
               XMatchVisualInfo(xdef.dpy, xdef.screen, 16, TrueColor, &vinfo) ||
               XMatchVisualInfo(xdef.dpy, xdef.screen, 15, TrueColor, &vinfo) )
      {
         xdef.visual = vinfo.visual;
         xdef.depth = vinfo.depth;
         xdef.red_mask = vinfo.red_mask;
         xdef.green_mask = vinfo.green_mask;
         xdef.blue_mask = vinfo.blue_mask;
         xdef.cmapflag = 1;
         xdef.cmap = XCreateColormap(xdef.dpy, DefaultRootWindow(xdef.dpy),
                                     xdef.visual, AllocNone);
      }
   }
   
   /* Defaults */
   XColor xc;
   xc.red = xc.green = xc.blue = 0;
   xc.pixel = BlackPixel(xdef.dpy, xdef.screen);
   XAllocColor(xdef.dpy, xdef.cmap, &xc);
   xdef.fgcolor = xc.pixel;
   xc.red = xc.green = xc.blue = 65535;
   xc.pixel = WhitePixel(xdef.dpy, xdef.screen);
   XAllocColor(xdef.dpy, xdef.cmap, &xc);
   xdef.bgcolor = xc.pixel;
   char *tempstr = XGetDefault(xdef.dpy, "SN", "ReverseVideo");
   if (tempstr && !strcmp(tempstr, "on")) {
      int tmp = xdef.bgcolor;
      xdef.bgcolor = xdef.fgcolor;
      xdef.fgcolor = tmp;
   }
   strcpy(xdef.font, "-misc-fixed-medium-r-normal--10-*-*-*-*-*-iso8859-1");
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
#if X11RELEASE >= 4
   xdef.wm_delete_window = XInternAtom(xdef.dpy, "WM_DELETE_WINDOW", True);
   xdef.wm_protocols = XInternAtom(xdef.dpy, "WM_PROTOCOLS", True);
#endif

   /* Gray Stipple */
   for (int i = 1; i < 7; i++)
      gray_stipple[i] = XCreateBitmapFromData(xdef.dpy,
					    DefaultRootWindow(xdef.dpy),
					    (char *) bitmap[i], 16, 16);
  /* Shared stuff */
  xdef.gcflag = false;
  xdef.carrow = XCreateFontCursor(xdef.dpy, XC_arrow);
  xdef.cwatch = XCreateFontCursor(xdef.dpy, XC_watch);
  xdef.ccross = XCreateFontCursor(xdef.dpy, XC_crosshair);
  
  /* Error handler */
  XSetErrorHandler(x11_handler);

  /* Xft */
#if HAVE_XFT2
  XftInit(NULL);
  XftInitFtLibrary();
#endif

  /* Miscellaneous */
  Xinitialised = true;
  register_poll_functions(handle_sync_events, handle_async_events,
                          x11_bwait, x11_ewait, ConnectionNumber(xdef.dpy));
}


static void x11_finalize_xdef(Drawable d)
{
   /* Default GC */
   XGCValues context;
   context.foreground = xdef.bgcolor;
   context.background = xdef.bgcolor;
   GC gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
   xdef.gcclear = gc;
   context.foreground = xdef.bgcolor;
   context.background = xdef.fgcolor;
   gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
   xdef.gccopy = gc;
   gc = XCreateGC(xdef.dpy, d, GCForeground | GCBackground, &context);
   XSetLineAttributes(xdef.dpy, gc, 0, LineDoubleDash, CapButt, JoinMiter);
   XSetStipple(xdef.dpy, gc, gray_stipple[2]);
   xdef.gcdash = gc;
   xdef.gcflag = true;
}


static struct X_window *x11_make_window(int x, int y, int w, int h, const char *name)
{
   XSizeHints hints;
#if X11RELEASE >= 4
   XWMHints wmhints;
   XClassHint clhints;
   XTextProperty xtpname;
#endif
   XEvent junk;
   XGCValues context;
   XSetWindowAttributes xswattrs;
   unsigned long xswattrs_mask;
   
   if (!Xinitialised)
      x11_init();

   extern mt_t mt_window; // defined in graphics.c
   struct X_window *info = mm_allocv(mt_window, sizeof(struct X_window));

   static int i = 0;
   i = (i+1) % MAXWIN;

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
   ifn (info->win) {
      enable();
      error(NIL, "Window creation failed", NIL);
   }

   /* Set properties */
   if (x || y)
      hints.flags = (USPosition | USSize);
   else
      hints.flags = (PPosition | PSize);

#if X11RELEASE < 4
   XSetStandardProperties(xdef.dpy, info->win,
                          name, name, None,
                          NULL, 0, &hints);
#else
   wmhints.input = True;
   wmhints.flags = InputHint;
   clhints.res_name = "lush";
   clhints.res_class = "Lush";
   char **list = (char **)&name;
# if X11RELEASE >= 6
   if (XmbTextListToTextProperty(xdef.dpy, list, 1, XStdICCTextStyle, &xtpname))
#  if X_HAVE_UTF8_STRING
      if (XmbTextListToTextProperty(xdef.dpy, list, 1, XUTF8StringStyle, &xtpname))
#  endif
         if (XmbTextListToTextProperty(xdef.dpy, list, 1, XTextStyle, &xtpname))
# else
            if (XStringListToTextProperty(list, 1, &xtpname))
# endif
            {
               enable();
               error(NIL,"memory exhausted",NIL);
            }
   XSetWMProperties(xdef.dpy,info->win,&xtpname,&xtpname,
                    NULL,0, &hints, &wmhints, &clhints );
   if (xtpname.value)
      XFree(xtpname.value);
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
   info->resizestate = false;
   info->okhilite = false;
   return info;
}



/* ============================  CURSORSUBS */

static int oldcursor = 0;

static void setcursor(int flag)
{
   if (flag != oldcursor && (flag==0 || flag==1)) {
      oldcursor = flag;
      disable();
      struct X_window *info = xwins;
      for (; info; info = info->next)
         if (info->lwin.used) {
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


static void unhilite(struct X_window *info)
{
   int x = info->xhilite;
   int y = info->yhilite;
   int w = info->whilite;
   int h = info->hhilite;

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


static void x11_hilite(window_t *linfo, int code, int x1, int y1, int x2, int y2)
{
   struct X_window *info = (struct X_window*)linfo;

   unhilite(info);
   ifn(info->okhilite)
      return;

#define MIN(x,y) ((x<y)?(x):(y))
#define DIST(x,y) ((x<y)?((y)-(x)):((x)-(y)))

   int x = info->xhilite = MIN(x1, x2);
   int y = info->yhilite = MIN(y1, y2);
   int w = info->whilite = DIST(x1, x2);
   int h = info->hhilite = DIST(y1, y2);
   
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


static void handle_async_events(void)
{
   XEvent ev;
   while (XCheckMaskEvent(xdef.dpy, ASYNC_EVENTS, &ev)) {
      Window win = ev.xany.window;
      struct X_window *info = xwins;
      for(; info; info = info->next)
         if (info->lwin.used && win==info->win) {
            switch (ev.type) {
            case Expose:
               if (ev.xexpose.count == 0)
                  x11_refresh(info);
               break;
               
            case ConfigureNotify:
               x11_resize(info, ev.xconfigure.width, ev.xconfigure.height);
               info->resizestate = true;
               break;
            }
            break;
         }
   }
   errno = 0;
}

static int handle_sync_events(void)
{
   KeySym ks;
   unsigned char s[8];
   static XComposeStatus xcst;
   static Time timedown;
   static char *buttondown = NULL;
   
   /* search for resized windows */
   struct X_window *info = xwins;
   for (; info; info = info->next)
      if (info->lwin.used && info->resizestate) {
         if (info->lwin.eventhandler)
            enqueue_event(info->lwin.eventhandler, EVENT_RESIZE,
                          info->sizx, info->sizy, 0,0 );
         info->resizestate = false;
      }
   
   /* search for other X events */
   while (XPending(xdef.dpy)) {
      XEvent ev;
      XNextEvent(xdef.dpy, &ev);
      Window win = ev.xany.window;

      for (info = xwins; info; info = info->next)
         if (info->lwin.used && win==info->win)
            break;
      if (!info)
         continue;
      if (ZOMBIEP(info->lwin.eventhandler))
         info->lwin.eventhandler = NIL;

      switch (ev.type) {
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
         info->okhilite = true;
         info->xdown = ev.xbutton.x;
         info->ydown = ev.xbutton.y;
         
         int i = ((ev.xbutton.time - timedown) < 600);
         buttondown = "n/a";
         if (ev.xbutton.button == Button1)
            buttondown = (i ? "Button1-Dbl" : "Button1");
         else if (ev.xbutton.button == Button2)
            buttondown = (i ? "Button2-Dbl" : "Button2");
         else if (ev.xbutton.button == Button3)
            buttondown = (i ? "Button3-Dbl" : "Button3");
         else if (ev.xbutton.button == Button4)
            buttondown = "Button4";
         else if (ev.xbutton.button == Button5)
            buttondown = "Button5";
         timedown = ev.xbutton.time;
         
         if (info->lwin.eventhandler)
	    enqueue_eventdesc(info->lwin.eventhandler,EVENT_MOUSE_DOWN,
                              ev.xbutton.x, ev.xbutton.y,
			      ev.xbutton.state & ShiftMask, 
                              ev.xbutton.state & ControlMask,
                              buttondown);

         break;
	  
      case ButtonRelease:
         info->okhilite = false;
         unhilite(info);
         if (info->lwin.eventhandler && !oldcursor) {
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
      {
         int i = XLookupString(&ev.xkey, (char*)s, 7, &ks, &xcst);
         if (info->lwin.eventhandler) {
            char *desc = XKeysymToString(ks);
            switch(ks) {
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
               if (i == 1 && (ev.xkey.state&Mod1Mask)) {
                  i = ASCII_TO_EVENT((s[0]|0x80));

               } else if (i == 1) {
                  i = ASCII_TO_EVENT((s[0]));
                  if (ks == XK_Delete && s[0]==0x7f)
                     i = EVENT_FKEY;

               } else {
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
      }
      case MotionNotify:
         if (info->lwin.eventhandler)
	    enqueue_eventdesc(info->lwin.eventhandler,EVENT_MOUSE_DRAG,
                              info->xdown, info->ydown,
                              ev.xbutton.x, ev.xbutton.y,
                              buttondown);
         break;
	  
#if X11RELEASE >= 4
      case ClientMessage:
         if (xdef.wm_delete_window != None &&
             ev.xclient.message_type == xdef.wm_protocols &&
             ev.xclient.data.l[0] == xdef.wm_delete_window ) {
	    if (info->lwin.eventhandler)
               enqueue_event(info->lwin.eventhandler,EVENT_DELETE,0,0,0,0);
	    else
               lush_delete(info->lwin.backptr);
         }
         break;
#endif
      }
   }
   errno = 0;
   return 0;
}


/* ============================  REFRESH_WINDOW */


static void copy_buffer(struct X_window *info)
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

static void grow_rect(struct X_window *info, int x1, int y1, int x2, int y2)
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

static void x11_begin(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   disable();
   if ((info->dbflag++) == 0) {
      info->x1 = info->y1 = info->x2 = info->y2 = -1;
   }
}

static void x11_end(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   if (--(info->dbflag) == 0) {
      copy_buffer(info);
      XSync(xdef.dpy,0);
   }
   enable();
}

static void x11_close(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   disable();
   XFreePixmap(xdef.dpy, info->backwin);
   XFreeGC(xdef.dpy, info->gc);
   XUndefineCursor( xdef.dpy, info->win);
   XUnmapWindow(xdef.dpy, info->win);
   XDestroyWindow(xdef.dpy, info->win);
   XSync(xdef.dpy, True);
   errno = 0; /* call to XSync may change errno */

   unchain_xwin(info);
   info->lwin.used = 0;
   enable();
}

static int x11_xsize(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   return info->sizx;
}

static int x11_ysize(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   return info->sizy;
}

static int x11_alloccolor(window_t *linfo, double r, double g, double b)
{
   return COLOR_RGB(r, g, b);
}

static long colorcube[6][6][6];
static char colorready[6][6][6];

static long alloc_from_cube(int r, int g, int b)
{
   XColor xc;
   r = (r + 26) / 51;
   g = (g + 26) / 51;
   b = (b + 26) / 51;
   if (colorready[b][g][r])
      return colorcube[b][g][r];
   xc.red = r * 13107;
   xc.green = g * 13107;
   xc.blue = b * 13107;
   xc.pixel = xdef.fgcolor;
   XAllocColor(xdef.dpy, xdef.cmap, &xc);
   colorcube[b][g][r] = xc.pixel;
   colorready[b][g][r] = 1;
   return xc.pixel;
}

static void x11_setcolor(window_t *linfo, int c)
{
   struct X_window *info = (struct X_window*)linfo;
   int r, g, b;
   long x;
   
   switch (c) {
   case COLOR_FG:
      r = g = b = 0;
      x = xdef.fgcolor;
      XSetFillStyle(xdef.dpy, info->gc, FillSolid);
      XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
      break;

   case COLOR_BG:
      r = g = b = 255;
      x = xdef.bgcolor;
      XSetFillStyle(xdef.dpy, info->gc, FillSolid);
      XSetForeground(xdef.dpy, info->gc, xdef.bgcolor);
      break;
      
   case COLOR_GRAY:
      r = g = b = 128;
      x = alloc_from_cube(128,128,128);
      XSetStipple(xdef.dpy, info->gc, gray_stipple[4]);
      XSetFillStyle(xdef.dpy, info->gc, FillOpaqueStippled);
      XSetForeground(xdef.dpy, info->gc, xdef.fgcolor);
      break;

   default:
      r = RED_256(c);
      g = GREEN_256(c);
      b = BLUE_256(c);
      if (xdef.red_mask && xdef.green_mask && xdef.blue_mask) {
         x = (((int)(xdef.red_mask * ((double)r/255.0))) & xdef.red_mask) 
            | (((int)(xdef.green_mask * ((double)g/255.0))) & xdef.green_mask) 
            | (((int)(xdef.blue_mask * ((double)b/255.0))) & xdef.blue_mask);
         XSetFillStyle(xdef.dpy, info->gc, FillSolid);
         XSetForeground(xdef.dpy, info->gc, x);

      } else if (xdef.depth >= MINDEPTH) {
         x = alloc_from_cube(r, g, b);
         r = (r + 26) / 51;  r *= 51;
         g = (g + 26) / 51;  g *= 51;
         b = (b + 26) / 51;  b *= 51;
         XSetFillStyle(xdef.dpy, info->gc, FillSolid);
         XSetForeground(xdef.dpy, info->gc, x);

      } else {
        x = SHADE_256(c) / 32;
        r = g = b = x * 32;
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
        x = alloc_from_cube(r, g, b);
      }
      break;
   }
#if HAVE_XFT2
   info->xftcolor.color.red   = (r << 8) | 0x7f;
   info->xftcolor.color.green = (g << 8) | 0x7f;
   info->xftcolor.color.blue  = (b << 8) | 0x7f;
   info->xftcolor.color.alpha = 0xffff;
   info->xftcolor.pixel = x;
#endif
}


static char *fontweights[] = {
   "thin", "extralight", "ultralight", "light", "book",
   "regular", "normal", "medium", "demibold", "semibold",
   "bold", "extrabold", "black", "heavy", 0
};

static char *fontslants[] = {
   "roman", "italic", "oblique", 0
};

static char *fontwidths[] = {
   "ultracondensed",  "extracondensed", "condensed",
   "semicondensed", "normal", "semiexpanded", "expanded",
   "extraexpanded", "ultraexpanded", 0
};

static int compare(const char *s, const char *q)
{
   while (*s && *q) {
      int c1 = *s;
      int c2 = *q;
      if (isascii(c1))
         c1 = tolower(c1);
      if (isascii(c2))
         c2 = tolower(c2);
      if (c1 != c2)
         return 0;
      s++;
      q++;
   }
   if (*q)
      return 0;
   return 1;
}

static int parse_psfont(const char *name, char *family, int *size,
                        char **weight, char **slant, char **width)
{
   const char *s = name;
   char *d = family;
   while (*s && (!isascii(*s) || isalpha(*s) || *s==' ')) {
      int c = *s++;
      if (d - family < 128)
         *d++ = tolower(c);
   }
   *d = 0;
   if (*s == '-') {
      s++;
      for(;;) {
         char **q;
         for (q = fontweights; *q; q++)
            if (compare(s, *q)) {
               *weight = *q;
               s += strlen(*q);
               continue;
            }
         for (q = fontslants; *q; q++)
            if (compare(s, *q)) {
               *slant = *q;
               s += strlen(*q);
               continue;
            }
         for (q = fontwidths; *q; q++)
            if (compare(s, *q)) {
               *width = *q;
               s += strlen(*q);
               continue;
            }
         break;
      }
   }
   while (*s && (!isascii(*s) || isalpha(*s) || *s==' ' || *s=='-'))
      s++;
   if (*s>='0' && *s<='9')
      *size = atoi(s);
   while (*s && isascii(*s) && isdigit(*s))
      s++;
   if (*s)
      return 0;
   return 1;
}

static char fname_buffer[256];

static const char *psfonttoxfont(const char *f)
{
   char family[256];
   int size = 11;
   char *weight = "medium";
   char *slant = "r";
   char *width = "normal";
   if (*f == '-' || *f == ':')
      return f;
   if (! parse_psfont(f, family, &size, &weight, &slant, &width))
      return f;
   if (!family[0])
      strcpy(family,"helvetica");
   sprintf(fname_buffer,"-*-%s-%s-%c-%s-*-%d-*-*-*-*-*-iso8859-1",
           family, weight, slant[0], width, size);
   return fname_buffer;
}

#if HAVE_XFT2
static const char *psfonttoxftfont(const char *f)
{
   /* convert a PS name to a XFT name */
   char *d;
   char family[256];
   int size = 11;
   char *weight = 0;
   char *slant = 0;
   char *width = 0;
   if (*f == '-' || *f == ':')
      return f;
   if (! parse_psfont(f, family, &size, &weight, &slant, &width))
      return f;
   fname_buffer[0] = 0;
   if (family[0]) 
      sprintf(fname_buffer, ":family=%s", family);
   d = fname_buffer + strlen(fname_buffer);
   sprintf(d, ":pixelsize=%d", size);
   d = d + strlen(d);
   if (weight) sprintf(d, ":%s", weight);
   d = d + strlen(d);
   if (slant) sprintf(d, ":%s", slant);
   d = d + strlen(d);
   if (width) sprintf(d, ":%s", width);
   return fname_buffer;
}
#endif

#if HAVE_XFT2
static char *fcpatterntoxftfont(FcPattern *p)
{
   FcValue val;
   char *s, *d;
   char *n = fname_buffer;
   int i;
   for (i=0; i<16; i++)
      if (FcPatternGet(p,"family",i,&val) == FcResultMatch && 
          val.type == FcTypeString) {
         if (n + strlen((char*)val.u.s)+16 > fname_buffer+sizeof(fname_buffer)) 
            return 0;
         sprintf(n, (i) ? ",%s" : ":family=%s", val.u.s);
         n = n + strlen(n);
      }
   for (i=0; i<16; i++)
      if (FcPatternGet(p,"size",i,&val) == FcResultMatch 
          && val.type == FcTypeDouble) {
         if (n + 24 > fname_buffer+sizeof(fname_buffer)) 
            return 0;
         sprintf(n, (i) ? ",%f" : ":size=%f", val.u.d);
         n = n + strlen(n);
      }
   s = d = (char*)FcNameUnparse(p);
   if (!s) return 0;
   while (*d && (*d != ':')) d++;
   if (n + strlen(d) + 2 > fname_buffer+sizeof(fname_buffer)) 
      return 0;
   strcpy(n, d);
   free(s);
   return fname_buffer;
}
#endif

static void delfont(struct X_font *fc)
{
   if (fc) {
      if (fc->xfs)
         XFreeFont(xdef.dpy, fc->xfs);
#if HAVE_XFT2
      if (fc->xft)
         XftFontClose(xdef.dpy, fc->xft);
#endif
      if (fc->name1)
         free(fc->name1);
      if (fc->name2)
         free(fc->name2);
      free(fc);
   }
}

static struct X_font *getfont(const char *name, int xft)
{
   int i;
   struct X_font *fc;
   for (i=0; i<MAXFONT; i++)
      if (fontcache[i])
         if ((fontcache[i]->name1 && !strcmp(name, fontcache[i]->name1)) ||
             (fontcache[i]->name2 && !strcmp(name, fontcache[i]->name2)) )
            break;
   /* not found */
   if (i>= MAXFONT) {
      /* prepare */
      i = MAXFONT-1;
      fc = fontcache[i];
      delfont(fc);
      fontcache[i] = 0;
      if (strlen(name)>255) 
         return 0;
      if (! (fc = malloc(sizeof(struct X_font))))
         return 0;
      memset(fc, 0, sizeof(struct X_font));
      fc->name1 = strdup(name);
      /* load */
#if HAVE_XFT2
      if (xft) {
         FcResult result;
         FcPattern *pattern = 0;
         FcPattern *newpattern = 0;
         pattern = FcNameParse((FcChar8*)name);
         if (pattern)
            newpattern = XftFontMatch(xdef.dpy, xdef.screen, pattern, &result);
         if (newpattern) {
            const char *s;
            FcValue val;
            if (FcPatternGet(newpattern,"family", 0, &val) == FcResultMatch 
                && val.type == FcTypeString )
            {
               FcPatternDel(pattern, "family");
               FcPatternAdd(pattern, "family", val, 0);
            }
            if ((s = fcpatterntoxftfont(pattern)))
               fc->name2 = strdup(s);
            fc->xft = XftFontOpenPattern(xdef.dpy, newpattern);
            if (fc->xft)
               newpattern = 0;
         }
         if (pattern)
            FcPatternDestroy(pattern);
         if (newpattern)
            FcPatternDestroy(newpattern);
      } else
#endif
      {
         Font fid;
         badname = 0;
         fid = XLoadFont(xdef.dpy, name);
         XSync(xdef.dpy, False);
         if (! badname)
            fc->xfs = XQueryFont(xdef.dpy,fid);
      }
      /* check */
#if HAVE_XFT2
      if (!fc->xft)
#endif
         if (! fc->xfs)
         {
            delfont(fc);
            return 0;
         }
      /* install */
      fontcache[i] = fc;
   }
   /* move to front */
   fc = fontcache[i];
   while (--i >= 0)
      fontcache[i+1] = fontcache[i];
   fontcache[0] = fc;
   return fc;
}

static const char *x11_setfont(window_t *linfo, const char *f)
{
   struct X_window *info = (struct X_window*)linfo;
   struct X_font *fc = 0;
   char *r = 0;
   if (!strcmp(f, "default"))
      f = xdef.font;
   if (f[0]=='-')
      fc = getfont(f, 0);
#if HAVE_XFT2
   else if (f[0]==':')
      fc = getfont(f, 1);
#endif
   else {
#if HAVE_XFT2
      if  (!fc)
         fc = getfont(psfonttoxftfont(f), 1);
      if (!fc)
         fc = getfont(f, 1);
#endif
      if (!fc)
         fc = getfont(psfonttoxfont(f), 0);
      if (!fc)
         fc = getfont(f, 0);
   }
   if (fc && fc->name2)
      r = fc->name2;
   else if (fc && fc->name1)
      r = fc->name1;
#if HAVE_XFT2
   if (! fc)
      fc = getfont(xdef.font, 1);
#endif
   if (! fc)
      fc = getfont(xdef.font, 0);
   if (! fc)
      fc = getfont("fixed", 0);
   if (fc) {
      info->font = fc;
      if (fc->xfs)
         XSetFont(xdef.dpy, info->gc, fc->xfs->fid);
   }
   return r;
}

static void x11_linestyle(window_t *linfo, int ls)
{
   static char dotted[] = {1,2};
   static char dashed[] = {5,3};
   static char dotdashed[] = {7,2,1,2};
   struct X_window *info = (struct X_window*)linfo;
   int dpos;
   if (ls >= 0)
      info->dpos = 0;
   else
      ls = linfo->linestyle;
   switch (ls) {
   case 1:
      dpos = info->dpos % 3;
      XSetDashes(xdef.dpy, info->gc, dpos, dotted, 2);
      break;
   case 2:
      dpos = info->dpos % 8;
      XSetDashes(xdef.dpy, info->gc, dpos, dashed, 2);
      break;
   case 3:
      dpos = info->dpos % 12;
      XSetDashes(xdef.dpy, info->gc, dpos, dotdashed, 4);
      break;
   }
   if (ls)
      XSetLineAttributes(xdef.dpy, info->gc, 0, 
                         LineOnOffDash, CapButt, JoinMiter);
   else
      XSetLineAttributes(xdef.dpy, info->gc, 0, 
                         LineSolid, CapButt, JoinMiter);
}

static void x11_clear(window_t *linfo)
{
   struct X_window *info = (struct X_window*)linfo;
   XSetForeground(xdef.dpy, info->gc, xdef.bgcolor);
   XSetFillStyle(xdef.dpy, info->gc, FillSolid);
   XFillRectangle(xdef.dpy, info->backwin, info->gc,
                  0, 0, info->sizx, info->sizy);
   x11_setcolor(&info->lwin, info->lwin.color);
   grow_rect(info, 0, 0, info->sizx, info->sizy);
}

static void x11_draw_line(window_t *linfo, int x1, int y1, int x2, int y2)
{
   struct X_window *info = (struct X_window*)linfo;
   XDrawLine(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
   grow_rect(info, x1, y1, x2, y2);
   /* adjust dash starting point */
   if (info->lwin.linestyle) {
      int dx = abs(x2-x1);
      int dy = abs(y2-y1);
      info->dpos += ((dx>dy) ? dx : dy);
      x11_linestyle(linfo, -1);
   }
}

static void x11_draw_rect(window_t *linfo,
                          int x1, int y1, unsigned int x2, unsigned int y2)
{
   struct X_window *info = (struct X_window*)linfo;
   XDrawRectangle(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
   grow_rect(info, x1, y1, x1 + x2, y1 + y2);
}

static void x11_draw_circle(window_t *linfo,
                            int x1, int y1, unsigned int r)
{
   struct X_window *info = (struct X_window*)linfo;
   XDrawArc(xdef.dpy, info->backwin, info->gc, 
            x1 - r, y1 - r, 2 * r, 2 * r, 0, 360 * 64);
   grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}

static void x11_draw_arc(window_t *linfo, 
                         int x1, int y1, unsigned int r, int from, int to)
{
   struct X_window *info = (struct X_window*)linfo;
   XDrawArc(xdef.dpy, info->backwin, info->gc, 
            x1 - r, y1 - r, 2 * r, 2 * r, 
            from*64, (to-from)*64 );
   grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}

static void x11_fill_rect(window_t *linfo, 
                          int x1, int y1, unsigned int x2, unsigned int y2)
{
   struct X_window *info = (struct X_window*)linfo;
   XFillRectangle(xdef.dpy, info->backwin, info->gc, x1, y1, x2, y2);
   grow_rect(info, x1, y1, x1 + x2, y1 + y2);
}

static void x11_fill_circle(window_t *linfo, int x1, int y1, unsigned int r)
{
   struct X_window *info = (struct X_window*)linfo;
   XFillArc(xdef.dpy, info->backwin, info->gc, 
            x1 - r, y1 - r, 2 * r, 2 * r, 0, 360 * 64);
   grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}


static void x11_fill_arc(window_t *linfo, 
                         int x1, int y1, unsigned int r, int from, int to)
{
   struct X_window *info = (struct X_window*)linfo;
   XFillArc(xdef.dpy, info->backwin, info->gc, 
            x1 - r, y1 - r, 2 * r, 2 * r,
            from*64, (to-from)*64 );
   grow_rect(info, x1 - r, y1 - r, x1 + r, y1 + r);
}


static void x11_fill_polygon(window_t *linfo, short (*points)[2], unsigned int n)
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

static void x11_draw_text(window_t *linfo, int x, int y, const char *s)
{
   struct X_window *info = (struct X_window*)linfo;
   if (!s[0]) return;
#if HAVE_XFT2
   if (info->font->xft) {
      XGlyphInfo extents;
      Pixmap pmap;
      XftDraw *draw;
      at *utf8q = str_mb_to_utf8(s);
      const FcChar8 *utf8 = (FcChar8*)String(utf8q);
      unsigned int utf8l = strlen((char*)utf8);
      XftTextExtentsUtf8(xdef.dpy, info->font->xft, 
                         utf8, utf8l, &extents);
      if (extents.width<=0 || extents.height<=0)
        return;
      pmap = XCreatePixmap(xdef.dpy, DefaultRootWindow(xdef.dpy),
                           extents.width, extents.height, xdef.depth);
      draw = XftDrawCreate(xdef.dpy, pmap, xdef.visual, xdef.cmap);
      if (draw) {
         XCopyArea(xdef.dpy, info->backwin, pmap,
                   xdef.gccopy, x - extents.x, y - extents.y,
                    extents.width, extents.height, 0, 0);
         XftDrawStringUtf8(draw, &info->xftcolor, info->font->xft, 
                           extents.x, extents.y, utf8, utf8l);
         XCopyArea(xdef.dpy, pmap, info->backwin,
                   info->gc, 0, 0, extents.width, extents.height,
                   x - extents.x, y - extents.y);
         XftDrawDestroy(draw);
         XFreePixmap(xdef.dpy, pmap);
      }
      grow_rect(info, x - extents.x, y - extents.y, 
                x - extents.x + extents.width, 
                y - extents.y + extents.height);
   } else
#endif
   {
      int junk;
      int len = strlen(s);
      XCharStruct overall;
      XTextExtents(info->font->xfs, s, len, 
                   &junk, &junk, &junk, &overall);
      XDrawString(xdef.dpy, info->backwin, info->gc, 
                  x, y, s, len);
      grow_rect(info, x - overall.lbearing - 5, y - overall.ascent,
                x + overall.rbearing + 5, y + overall.descent);
   }
}

static void x11_rect_text(window_t *linfo, 
                          int x, int y, const char *s, 
                          int *xptr, int *yptr, int *wptr, int *hptr)
{
   struct X_window *info = (struct X_window*)linfo;
#if HAVE_XFT2
   if (info->font->xft) {
      XGlyphInfo extents;
      at *utf8q = str_mb_to_utf8(s);
      const FcChar8 *utf8 = (FcChar8*)String(utf8q);
      uint utf8l = strlen((char*)utf8);
      XftTextExtentsUtf8(xdef.dpy, info->font->xft, 
                         utf8, utf8l, &extents);
      int lb = extents.x;
      int rb = extents.width - extents.x;
      if (lb < 0) lb = 0;
      if (rb < 0) rb = 0;
      *xptr = x - lb;
      *yptr = y - extents.y;
      if (rb > extents.xOff)
         *wptr = lb + rb;
      else
         *wptr = lb + extents.xOff;
      *hptr = extents.height;
   } else
#endif
   {
      int junk;
      int len = strlen(s);
      XCharStruct overall;
      XTextExtents(info->font->xfs, s, len, 
                   &junk, &junk, &junk, &overall);
      *xptr = x - overall.lbearing;
      *yptr = y - overall.ascent;
      if (overall.rbearing > overall.width)
         *wptr = overall.lbearing + overall.rbearing;
      else
         *wptr = overall.lbearing + overall.width;
      *hptr = overall.ascent + overall.descent;
   }
}



static void error_diffusion(unsigned int *image, int w, int h)
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



static int x11_pixel_map(window_t *linfo, uint *image, 
                         int x, int y, uint w, uint h, 
                         uint sx, uint sy)
{
   struct X_window *info = (struct X_window*)linfo;
   XImage *ximage;
   int i, j, k, l;
   uint *im, *jm, *image2;
   int sx_w = sx * w;
   int sy_h = sy * h;
   
   union { int i; char c[sizeof(int)]; } test;
   test.i=1;
   
   if (!image) {
      if (sx * sy > 36 && xdef.depth >= MINDEPTH) /* test if ok */
         return false;
      else
         return true;
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
         return false;
      ifn (image2 = malloc(sx_w * sy_h * sizeof(int)))
         return false;
      
      for (i = 0; i < h; i++) {
         im = image2 + sx_w * sy * i;
         for (k = 0; k < w; k++, image++) {
            int c = *image;
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
   return true;
}


static void x11_clip(window_t *linfo, int x, int y, unsigned int w, unsigned int h)
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

void x11_get_image(window_t *linfo, unsigned int *image, 
                   int x, int y, unsigned int w, unsigned int h)
{
   int i,j;
   int mod = w;
   unsigned int *im;
   struct X_window *info = (struct X_window*)linfo;
   XImage *ximage;
   
   memset(image, 0, sizeof(unsigned int) * w * h);
   if (x < 0) { image -= x; x = 0; }
   if (y < 0) { image -= y * mod; y = 0; }
   if (x + w > info->sizx) { w = info->sizx - x; }
   if (y + h > info->sizy) { h = info->sizy - y; }
   ximage = XGetImage(xdef.dpy, info->backwin, x, y, w, h, ~0, ZPixmap);
   for (i=0; i<h; i++) {
      im = image;
      for (j=0; j<w; j++) 
         *im++ = XGetPixel(ximage,j,i);
      image += mod;
   }
   XDestroyImage(ximage);
}


int x11_get_mask(window_t *linfo, uint *red_mask, 
                 uint *green_mask, uint *blue_mask)
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
   x11_linestyle,
};


static void x11_resize(struct X_window *info, int x, int y)
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

static void x11_refresh(struct X_window *info)
{
   
   XCopyArea(xdef.dpy, info->backwin, info->win,
             xdef.gccopy, 0, 0, info->sizx, info->sizy, 0, 0);
   XFlush(xdef.dpy);
}

static at *x11_window(int x, int y, uint w, uint h, const char *name)
{
   int ijunk;
   uint ujunk;
   Window wjunk;
   struct X_window *info = x11_make_window(x, y, w, h, name);
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
   info->lwin.linestyle = 0;
   
   at *ans = new_at(window_class, info);
   info->lwin.backptr = ans;
   
   x11_setcolor(&info->lwin, COLOR_FG);
   info->lwin.color = COLOR_FG;
   
   x11_setfont(&info->lwin, xdef.font);
   info->lwin.font = make_string(xdef.font);
   
   x11_clear(&info->lwin);

   info->next = xwins;
   xwins = info;
   enable();
   return ans;
}

DX(xx11_window)
{
   at *ans;
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


DX(xx11_id)
{
   if (arg_number > 1)
      ARG_NUMBER(-1);
   
   window_t *w;
   if (arg_number==0) {
      window_t *current_window(void);
      w = current_window();
      
   } else {
      at *p = APOINTER(1);
      ifn (WINDOWP(p))
         RAISEF("not a window", p);
      w = (window_t *)Mptr(p);
   }
   if (w->gdriver == &x11_driver) {
      struct X_window *info = (struct X_window *)w;
      return NEW_NUMBER(info->win);
   }
   return NIL;
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
#if HAVE_XFT2
   return make_string(psfonttoxftfont(ASTRING(1)));
#else
   return make_string(psfonttoxfont(ASTRING(1)));
#endif
}

DX(xx11_configure)
{
   window_t *current_window(void);
   window_t *w = current_window();
   if (w->gdriver == &x11_driver) {
#if X11RELEASE >= 4      
      int mask = 0;
      XWindowChanges xwc;
      XWindowAttributes xwa;
      const char *name = NULL;
      struct X_window *info = (struct X_window*)w;
      if (arg_number!=0) {
         if (arg_number>6 || arg_number<5)
	    ARG_NUMBER(-1);
         
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
         if (arg_number>5 && APOINTER(6)) {
	    name = ASTRING(6);
         }
      }
      disable();
      if (mask & CWStackMode) 
         XMapRaised(xdef.dpy,info->win);
      if (mask)
         XReconfigureWMWindow(xdef.dpy,info->win,
                              xdef.screen, mask, &xwc);
      XGetWindowAttributes(xdef.dpy,info->win,&xwa);
      if (name) {
         XTextProperty xtpname;
         char **list = (char **)&name;
# if X11RELEASE >= 6
         if (XmbTextListToTextProperty(xdef.dpy, list, 1, XStdICCTextStyle, &xtpname))
#  if X_HAVE_UTF8_STRING
            if (XmbTextListToTextProperty(xdef.dpy, list, 1, XUTF8StringStyle, &xtpname))
#  endif
               if (XmbTextListToTextProperty(xdef.dpy, list, 1, XTextStyle, &xtpname))
# else
                  if (XStringListToTextProperty(list, 1, &xtpname))
# endif
                  {
                     enable();
                     error(NIL, "memory exhausted", NIL);
                  }
         XSetWMName(xdef.dpy, info->win, &xtpname);
         if (xtpname.value)
            XFree(xtpname.value);
         
      }
      enable();
      return new_cons((xwa.map_state==IsViewable ? t() : NIL ),
                      new_cons(NEW_NUMBER(xwa.width),
                               new_cons(NEW_NUMBER(xwa.height), NIL) ) );
#endif
   }
   return NIL;
}

DX(xx11_iconify)
{
   if (arg_number > 1)
      ARG_NUMBER(-1);
   window_t *w;
   if (arg_number==0) {
      window_t *current_window(void);
      w = current_window();
      
   } else {
      at *p = APOINTER(1);
      ifn (WINDOWP(p))
         RAISEF("not a window", p);
      w = (window_t *)Mptr(p);
   }
   if (w->gdriver == &x11_driver) {
      struct X_window *info = (struct X_window *)w;
      disable();
      XIconifyWindow(xdef.dpy, info->win, xdef.screen);
      enable();
   }
   return NIL;
}

DX(xx11_lookup_color)
{
   real rgb[3];
   XColor xc1, xc2;
   int status;
  
   ARG_NUMBER(1);
   const char *s = ASTRING(1);
   if (!Xinitialised)
      x11_init();
   disable();
   status = XLookupColor(xdef.dpy, xdef.cmap, s, &xc1, &xc2);
   enable();
   if (status) {
      rgb[0] = (real)(xc1.red)/65535.0;    
      rgb[1] = (real)(xc1.green)/65535.0;    
      rgb[2] = (real)(xc1.blue)/65535.0;
      return new_cons(NEW_NUMBER(rgb[0]),
                      new_cons(NEW_NUMBER(rgb[1]),
                               new_cons(NEW_NUMBER(rgb[2]), NIL)));
   }
   else
      return NIL;
}



DX(xx11_text_to_clip)
{
   ARG_NUMBER(1);
   const char *s = ASTRING(1);
   if (!Xinitialised)
      x11_init();
   disable();
   XStoreBuffer(xdef.dpy,s,strlen(s),0);
#if X11RELEASE >= 4
   XSetSelectionOwner(xdef.dpy,XA_PRIMARY,None,CurrentTime);
   /* TODO: Own CLIPBOARD (and SHELF?) selections */
#endif
   enable();
   return APOINTER(1);
}



DX(xx11_clip_to_text)
{
   ARG_NUMBER(0);
   if (!Xinitialised)
      x11_init();
   disable();

   char *buf = NULL;
#if X11RELEASE >= 4
   /* Check CLIPBOARD (and SHELF?) selections */
#endif

   at *p = NIL;
   if (!buf) {
      int nbytes;
      buf = XFetchBuffer(xdef.dpy,&nbytes,0);
      if (nbytes==0) {
         p = NIL;
      } else {
         p = make_string_of_length(nbytes);
         char *s = (char *)String(p);
         strncpy(s, buf, nbytes);
         s[nbytes] = 0;
         XFree(buf);
      }
   }
   enable();
   return p;
}



/* ============================  INITIALISATION */


void init_x11_driver(void)
{
   display = var_define("display");
   dx_define("x11-window", xx11_window);
   dx_define("x11-id", xx11_id);
   dx_define("x11-fontname", xx11_fontname);
   dx_define("x11-depth", xx11_depth);
   dx_define("x11-configure", xx11_configure);
   dx_define("x11-iconify", xx11_iconify);
   dx_define("x11-lookup-color", xx11_lookup_color);
   dx_define("x11-clip-to-text", xx11_clip_to_text);
   dx_define("x11-text-to-clip", xx11_text_to_clip);
}


/* ============================================================ */
#endif /* X_DISPLAY_MISSING */


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
