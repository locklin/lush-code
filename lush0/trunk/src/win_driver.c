/*   Lush Lisp interpreter and development tools
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
	T-Lisp3: LYB YLC 1/88
	PSdriver.c
	Graphic driver for X11
$Id: win_driver.c,v 0.1 2001-10-31 17:31:13 profshadoko Exp $
***********************************************************************/


/* Include system header files */
#define VC_EXTRALEAN
#include <windows.h>

/* Include communication header files */
#include "win32.h"

/* Include Lush header files */
#include "header.h"
#include "graphics.h"


/* ============================  DRIVER STRUCTURES */

#define MAXWIN 32
#define PI     3.141592653589793


struct M_window {
  struct window lwin;
  int dbflag;
  /* Provided by lushcom */
  void *wid;
  CRITICAL_SECTION *lock;
  HDC hdc;
  HPALETTE hpal;
  LOGPALETTE *logpal;
  int szpal;
  /* Managed by win_driver */
  RECT hitrect;
  HPEN hpen;
  HBRUSH hbrush;
  /* For wpr_window */
  int  prw;
  int  prh;
  int  in_page;
};

static struct M_window wind[MAXWIN];



/* ============================  EVENT MANAGEMENT */


static CRITICAL_SECTION lushevent_cs;
static struct event *lushevent_buffer;
static int lushevent_evput, lushevent_evget;

#define EVBUFMASK 0x1ff
#define EVBUFSIZE 0x200



/* lushevent_test -- called by winlush thread to test presence of events */

BOOL 
lushevent_test(void)
{
  BOOL ans = TRUE;
  EnterCriticalSection(&lushevent_cs);
  if (lushevent_evput == lushevent_evget)
    ans = FALSE;
  LeaveCriticalSection(&lushevent_cs);
  return ans;
}



/* buttonevents -- lush events for button messages */

static struct {
  int code;
  char *name;
}
buttonevents[] = {
  WM_LBUTTONDOWN,    "Button1",
  WM_MBUTTONDOWN,    "Button2",
  WM_RBUTTONDOWN,    "Button3",
  WM_LBUTTONDBLCLK,  "Button1-Dbl",
  WM_MBUTTONDBLCLK,  "Button2-Dbl",
  WM_RBUTTONDBLCLK,  "Button3-Dbl",
  0
};



/* keyevents -- lush events for keydown messages */

static struct { 
  int vk;
  int event;
  char *name;
} 
keyevents[] = {
  VK_DELETE,    EVENT_FKEY,         "Delete",   
  VK_INSERT,    EVENT_FKEY,         "Insert",   
  VK_HOME,      EVENT_FKEY,         "Home",  
  VK_END,       EVENT_FKEY,         "End",   
  VK_PRIOR,     EVENT_FKEY,         "Prior",  
  VK_NEXT,      EVENT_FKEY,         "Next",  
  VK_CANCEL,    EVENT_FKEY,         "Cancel", 
  VK_PAUSE,     EVENT_FKEY,         "Break", 
  VK_UP,        EVENT_ARROW_UP,     "Up",    
  VK_DOWN,      EVENT_ARROW_DOWN,   "Down",  
  VK_LEFT,      EVENT_ARROW_LEFT,   "Left",  
  VK_RIGHT,     EVENT_ARROW_RIGHT,  "Right", 
  VK_F1,        EVENT_HELP,         "F1",    
  VK_F2,        EVENT_FKEY,         "F2",    
  VK_F3,        EVENT_FKEY,         "F3",    
  VK_F4,        EVENT_FKEY,         "F4",    
  VK_F5,        EVENT_FKEY,         "F5",    
  VK_F6,        EVENT_FKEY,         "F6",    
  VK_F7,        EVENT_FKEY,         "F7",    
  VK_F8,        EVENT_FKEY,         "F8",    
  VK_F9,        EVENT_FKEY,         "F9",    
  VK_F10,       EVENT_FKEY,         "F10",   
  VK_F11,       EVENT_FKEY,         "F11",   
  VK_F12,       EVENT_FKEY,         "F12",   
  0,0,0
};



/* translate_event -- translete win event into tlisp event */

static BOOL
translate_event(int code, int arg, int x, int y, struct event *ev)
{
  int i;
  static char *down = NULL;
  static int xd = 0;
  static int yd = 0;

  ev->desc = NULL;
  switch (code)
  {

  case WM_CLOSE:
    ev->code = EVENT_DELETE;
    ev->xd = ev->xu = 0;
    ev->yd = ev->yu = 0;
    return TRUE;

  case WM_SIZE:
    ev->code = EVENT_RESIZE;
    ev->xd = ev->xu = x;
    ev->yd = ev->yu = y;
    return TRUE;

  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    ev->code = EVENT_MOUSE_DOWN;
    xd = ev->xd = x;
    yd = ev->yd = y;
    ev->xu = (arg&0x10000000 ? -1 : 0);
    ev->yu = (arg&0x20000000 ? -1 : 0);
    for (i=0; buttonevents[i].code; i++)
      if (code==buttonevents[i].code)
      {
        down = ev->desc = buttonevents[i].name;
        return TRUE;
      }
    return FALSE;

  case WM_MOUSEMOVE:
    if (down)
    {
      ev->code = EVENT_MOUSE_DRAG;
      ev->xd = xd;
      ev->xu = x;
      ev->yd = yd;
      ev->yu = y;
      ev->desc = down;
      return TRUE;
    }
    return FALSE;

  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    ev->code = EVENT_MOUSE_UP;
    ev->xd = xd;
    ev->xu = x;
    ev->yd = yd;
    ev->yu = y;
    ev->desc = down;
    down = NULL;
    return TRUE;

  case WM_CHAR:
    ev->code = ASCII_TO_EVENT(arg);
    ev->xd = ev->xu = x;
    ev->yd = ev->yu = y;
    return TRUE;

  case WM_KEYDOWN:
    ev->xd = x;
    ev->yd = y;
    ev->xu = (arg&0x10000000 ? -1 : 0);
    ev->yu = (arg&0x20000000 ? -1 : 0);
    arg &= 0xffffff;
    for (i=0; keyevents[i].vk; i++)
      if (arg == keyevents[i].vk)
      {
        ev->code = keyevents[i].event;
        ev->desc = keyevents[i].name;
        return TRUE;
      }
    return FALSE;
  }
  return FALSE;
}


/* lushevent_add -- called by winlush thread to file new events */

BOOL 
lushevent_add(void *wid, int code, int arg, int x, int y)
{ 
  int i;
  struct event *ev;
  struct M_window *info = NULL;

  EnterCriticalSection(&lushevent_cs);

  /* Collapse message when possible */

  if (lushevent_evput!=lushevent_evget)
  {
    i = (lushevent_evput - 1) & EVBUFMASK;
    ev = &lushevent_buffer[i];
    if ((void*)ev->handler == wid)
    {
      if (code==WM_MOUSEMOVE)
      {
	if (ev->code == EVENT_MOUSE_DRAG)
	{
	  ev->xu = x;
	  ev->yu = y;
	  LeaveCriticalSection(&lushevent_cs);
	  return TRUE;
	}
	else if (ev->code==EVENT_MOUSE_DOWN && ev->xd==x && ev->yd==y)
	{
	  LeaveCriticalSection(&lushevent_cs);
	  return TRUE;
	}
      } 
      else if (code==WM_LBUTTONUP)
      {
	if (ev->code == EVENT_MOUSE_DRAG)
	{
	  ev->code = EVENT_MOUSE_UP;
	  ev->xu = x;
	  ev->yu = y;
	  LeaveCriticalSection(&lushevent_cs);
	  return TRUE;
	}
      }
    }
  }

  /* Convert windows event into Lush event */
  i = (lushevent_evput + 1) & EVBUFMASK;
  ev = &lushevent_buffer[lushevent_evput];
  if (translate_event(code, arg, x, y, ev) && i!=lushevent_evget)
  {
    ev->handler = (at*)wid;
    lushevent_evput = i;
    LeaveCriticalSection(&lushevent_cs);
    return FALSE;
  }
  /* Message not recorded */
  LeaveCriticalSection(&lushevent_cs);
  return FALSE;
}



/* transfer_events -- called to refill the Lush event queue */

static void
transfer_events(void)
{
  /* Transfer events from temp queue to Lush queue */
  EnterCriticalSection(&lushevent_cs);
  while (lushevent_evget != lushevent_evput)
  {
    int i;
    struct M_window *info = NULL;
    struct event *ev = &lushevent_buffer[lushevent_evget];
    /* search window */
    for (i=0; i<MAXWIN; i++)
      if (wind[i].lwin.used && wind[i].wid==(void*)ev->handler)
	break;
    if (i < MAXWIN)
      info = wind + i;
    /* test if an event handler is available */
    if (info && info->lwin.eventhandler)
    {
      // transfer event if an event handler is present
      enqueue_eventdesc(info->lwin.eventhandler, ev->code, 
                        ev->xd, ev->yd, ev->xu, ev->yu, ev->desc);
    }
    else if (info && ev->code == EVENT_DELETE)
    {
      /* close window if WM_CLOSE and no event handler */
      delete_at(info->lwin.backptr);
    }
    /* next event */
    lushevent_evget = (lushevent_evget + 1) & EVBUFMASK;
  }
  LeaveCriticalSection(&lushevent_cs);
}




/* kill_events -- called to when window is destroyed */

static void
kill_events(void *wid)
{
  int i;
  EnterCriticalSection(&lushevent_cs);
  i = lushevent_evget;
  while (i != lushevent_evput)
  {
    if ((void*)lushevent_buffer[i].handler == wid)
      lushevent_buffer[i].handler = NIL;
    i = (i+1) & EVBUFMASK;
  }
  LeaveCriticalSection(&lushevent_cs);
}


/* ============================  MQNQGE HIT RECTANGLE */


/* grow_hitect -- increases hit rectangle */

static void
grow_hitrect(struct M_window *info, RECT *prect)
{
  RECT clipRect;
  /* Restrict to clip region */
  if (GetClipBox(info->hdc, &clipRect) != NULLREGION)
    IntersectRect(prect, prect, &clipRect);
  /* Union with current hit rect */
  if (! IsRectEmpty(&info->hitrect))
    UnionRect(&info->hitrect, &info->hitrect, prect);
  else
    info->hitrect = *prect;
}



/* ============================  BASIC FUNCTIONS */


/* Graphics calls are always enclosed between one or more
   Begin/End pair. */

static void 
wbm_begin(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  info->dbflag++;
  if (info->hpal && info->szpal < info->logpal->palNumEntries)
  {
    int k = 0;
    int i = info->szpal;
    int m = info->logpal->palNumEntries;
    PALETTEENTRY *pe = info->logpal->palPalEntry;
    RGBQUAD rgb[256];
    while (i < m)
    {
      rgb[k].rgbRed = pe[i].peRed;
      rgb[k].rgbGreen = pe[i].peGreen;
      rgb[k].rgbBlue = pe[i].peBlue;
      rgb[k].rgbReserved = 0;
      i++;
      k++;
    }
    SetDIBColorTable(info->hdc, info->szpal, k, rgb);
    info->szpal = info->logpal->palNumEntries;
  }
}


static void 
wbm_end(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  if (--info->dbflag == 0)
  {
    GdiFlush();
    if (! IsRectEmpty(&info->hitrect))
    {
      struct msg_configwin arg;
      arg.wid = info->wid;
      arg.x = info->hitrect.left;
      arg.y = info->hitrect.top;
      arg.w = info->hitrect.right;
      arg.h = info->hitrect.bottom;
      arg.status = MSG_OK;
      lushsend(LUSH_UPDATEWIN, (LUSHARG*)&arg);
      SetRectEmpty(&info->hitrect);
    }
  }
}


/* Close a window */

static void 
wbm_close(struct window *linfo)
{
  HFONT hFont;
  struct msg_simplewin arg;
  struct M_window *info = (struct M_window*)linfo;
  // not harmful to delete stock objects
  if (info->hpen)
    DeleteObject(info->hpen);
  if (info->hbrush)
    DeleteObject(info->hbrush);
  // extract fonts
  hFont = (HFONT)SelectObject(info->hdc, GetStockObject(SYSTEM_FONT));
  if (hFont)
    DeleteObject(hFont);
  // perform the actual close
  arg.wid = info->wid;
  arg.status = MSG_OK;
  lushsend(LUSH_CLOSEWIN, (LUSHARG*)&arg);
  // clean events
  kill_events(info->wid);
  // set used
  info->lwin.used = 0;
}


/* Return the size of a window */

static int 
wbm_xsize(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  struct msg_configwin arg;
  arg.status = MSG_OK;
  arg.wid = info->wid;
  lushsend(LUSH_QUERYWIN, (LUSHARG*)&arg);
  if (arg.status & MSG_CONFIGWIN_W)
    return arg.w;
  else
    return -1;
}

static int 
wbm_ysize(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  struct msg_configwin arg;
  arg.status = MSG_OK;
  arg.wid = info->wid;
  lushsend(LUSH_QUERYWIN, (LUSHARG*)&arg);
  if (arg.status & MSG_CONFIGWIN_H)
    return arg.h;
  else
    return -1;
}




/* set the font in a window */

static char * 
setfont_cmp(char *key, char *r, char *fend)
{
  int n = strlen(key);
  if (r+n <= fend)
    if (!_strnicmp(key,r,n))
      return r + n;
  return 0;
}

static int CALLBACK 
setfont_enumproc(ENUMLOGFONT *lpelf, NEWTEXTMETRIC *lpntm, int type, LPARAM arg)
{
  LOGFONT *lplf = (LOGFONT*)arg;
  /* Mark that font family is ok */
  lplf->lfHeight = -11;
  /* Force ANSI character set if available */
  if (lpelf->elfLogFont.lfCharSet == ANSI_CHARSET)
    lplf->lfCharSet = ANSI_CHARSET;
  return 1;
}


static void 
wdc_setfont(struct window *linfo, char *f)
{
  LOGFONT lf;
  HFONT hf, hfOld;
  struct M_window *info = (struct M_window*)linfo;

  /* sets the default font mapping */
  memset(&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = -11;
  lf.lfWeight = FW_NORMAL;
  lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = FALSE;
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfOutPrecision = OUT_TT_PRECIS;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = DEFAULT_QUALITY;
  lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  strcpy(lf.lfFaceName, "Courier New");

  /* analyze font name */
  if (!_stricmp(f, FONT_STD))
  {
    /* handle default (never fails) */
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    hf = CreateFontIndirect(&lf);
    if (hf == NULL)
      hf = GetStockObject(ANSI_FIXED_FONT);
  }
  else
  {
    int nsize = 11;
    long nweight = FW_NORMAL;
    char *fend, *r, *s;

    /* PROCESS SIZE SPECIFICATION */
    fend = f + strlen(f);
    r = fend;
    while (r>f && isdigit(r[-1]))
      r -= 1;
    if (r>f && r<fend && r[-1]=='-')
    {
      /* Found size specification */
      s = r;
      nsize = 0;
      while (s < fend)
        nsize = nsize*10 + *s++ - '0';
      if (nsize<=0 || nsize>=128)
        error(NIL,"Illegal font size", NEW_NUMBER(nsize));
      fend = r-1;
    }

    /* PROCESS STYLE SPECIFICATION */
    r = fend;
    while (r>f && r[-1]!='-')
      r -= 1;
    if (r>f && r<fend)
    {
      /* Possible style specification */
      if (setfont_cmp("roman", r, fend) == fend)
      {
	fend = r-1;
      }
      else
      {
	int i;
	static struct { char *key; long weight; } wkeys[] = 
        {
	  "thin",	FW_THIN, 
	  "extralight", FW_EXTRALIGHT,
	  "ultralight", FW_ULTRALIGHT,
	  "light",	FW_LIGHT,
	  "normal",	FW_NORMAL,
	  "regular",	FW_NORMAL,
	  "medium",	FW_NORMAL,
	  "semibold",	FW_SEMIBOLD,
	  "demibold",	FW_DEMIBOLD,
	  "demi",	FW_DEMIBOLD,
	  "bold",	FW_BOLD,
	  "extrabold",	FW_EXTRABOLD,
	  "ultrabold",	FW_ULTRABOLD,
	  "heavy",	FW_HEAVY,
	  "black",	FW_BLACK,
	  0, 0 
        };

	for (i=0; wkeys[i].key; i++)
	  if ((s = setfont_cmp(wkeys[i].key, r, fend)))
	  {
	    nweight = wkeys[i].weight;
	    break;
	  }
	if (s==NULL)
	  s = r;
	if (s==fend)
	{
	  lf.lfWeight = nweight;
	  fend = r-1;
	}
	else if (s<fend && setfont_cmp("italic", s, fend)==fend)
	{
	  lf.lfItalic = TRUE;
	  lf.lfWeight = nweight;
	  fend = r-1;
	}
	else if (s<fend && setfont_cmp("oblique", s, fend)==fend)
	{
	  lf.lfItalic = TRUE;
	  lf.lfWeight = nweight;
	  fend = r-1;
	}
      }
    }

    /* PROCESS FONT FAMILY NAME */
    if (fend - f >= LF_FACESIZE)
      error(NIL,"Illegal font specification",new_string(f));
    /* Check face name specification */
    strncpy(lf.lfFaceName, f, fend-f);
    lf.lfFaceName[fend-f] = 0;
    lf.lfHeight = 0;
  again:
    EnumFontFamilies(info->hdc, lf.lfFaceName, 
                     (FONTENUMPROC)setfont_enumproc, (LPARAM)&lf);
    if (!lf.lfHeight)
    {
      /* Process Font family substitution table */
      int i;
      static struct { char *from; char *to; } facesubst[] = 
      {
        "helvetica", "Arial",
        "times",     "Times New Roman",
        "courier",   "Courier New",
        0,           0 
      };

      for (i=0; facesubst[i].from; i++)
      {
        if (! _stricmp(lf.lfFaceName, facesubst[i].from) )
        {
          strcpy(lf.lfFaceName, facesubst[i].to);
          goto again;
        }
      }
      error(NIL,"Unknown font family",new_string(f));
    }

    /* CREATE FONT */
    lf.lfHeight = -nsize;
    hf = CreateFontIndirect(&lf);
    if (hf == NULL)
      error(NIL,"Cannot create this font",new_string(f));
  }
  /* Select font */
  hfOld = SelectObject(info->hdc, hf);
  DeleteObject(hfOld);
}


/* Drawing functions */

static void 
wdc_draw_line(struct window *linfo, int x1, int y1, int x2, int y2)
{
  HPEN hpenOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  SetRect(&rectNew, min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
  InflateRect(&rectNew, 2, 2);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hpenOld = SelectObject(info->hdc, info->hpen);
  MoveToEx(info->hdc, x1, y1, NULL);
  LineTo(info->hdc, x2, y2);
  SetPixel(info->hdc, x2, y2, GetTextColor(info->hdc));
  SelectObject(info->hdc, hpenOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_draw_rect(struct window *linfo, 
	     int x1, int y1, unsigned int x2, unsigned int y2)
{
  HPEN hpenOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle (because Win95 does not do it) */
  SetRect(&rectNew, x1, y1, x1+x2, y1+y2);
  InflateRect(&rectNew, 2, 2);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hpenOld = SelectObject(info->hdc, info->hpen);
  Rectangle(info->hdc, x1, y1, x1+x2+1, y1+y2+1);
  SelectObject(info->hdc, hpenOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_draw_circle(struct window *linfo, 
	       int x1, int y1, unsigned int r)
{
  HPEN hpenOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle (because Win95 does not do it) */
  SetRect(&rectNew, x1-r, y1-r, x1+r, y1+r);
  InflateRect(&rectNew, 2, 2);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hpenOld = SelectObject(info->hdc, info->hpen);
  Ellipse(info->hdc, x1-r, y1-r, x1+r+1, y1+r+1);
  SelectObject(info->hdc, hpenOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_draw_arc(struct window *linfo, 
	    int x1, int y1, unsigned int r, int from, int to)
{
  HPEN hpenOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  const double ratio = 3.1415925359 / 180.0;
  /* calculate endpoints */
  int xstart, xend, ystart, yend;
  double rfrom = PI * from / 180;
  double rto = PI * to / 180;
  xstart = x1 + (int)(1024 * cos(rfrom));
  ystart = y1 - (int)(1024 * sin(rfrom));
  xend = x1 + (int)(1024 * cos(rto));
  yend = y1 - (int)(1024 * sin(rto));
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle (because Win95 does not do it) */
  SetRect(&rectNew, x1-r, y1-r, x1+r, y1+r);
  InflateRect(&rectNew, 2, 2);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hpenOld = SelectObject(info->hdc, info->hpen);
  Arc(info->hdc, x1-r, y1-r, x1+r+1, y1+r+1, xstart, ystart, xend, yend);
  SelectObject(info->hdc, hpenOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_fill_rect(struct window *linfo, 
	     int x1, int y1, unsigned int x2, unsigned int y2)
{
  HBRUSH hbrushOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle (because Win95 does not do it) */
  SetRect(&rectNew, x1, y1, x1+x2+1, y1+y2+1);
  grow_hitrect(info, &rectNew);
  /* Perform drawinf */
  hbrushOld = SelectObject(info->hdc, info->hbrush);
  Rectangle(info->hdc, x1, y1, x1+x2+1, y1+y2+1);
  SelectObject(info->hdc, hbrushOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void
wdc_fill_circle(struct window *linfo, 
	       int x1, int y1, unsigned int r)
{
  HBRUSH hbrushOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle (because Win95 does not do it) */
  SetRect(&rectNew, x1-r, y1-r, x1+r+1, y1+r+1);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hbrushOld = SelectObject(info->hdc, info->hbrush);
  Ellipse(info->hdc, x1-r, y1-r, x1+r+1, y1+r+1);
  SelectObject(info->hdc, hbrushOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_fill_arc(struct window *linfo, 
	    int x1, int y1, unsigned int r, int from, int to)
{
  HBRUSH hbrushOld;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* calculate endpoints */
  int xstart, xend, ystart, yend;
  double rfrom = PI * from / 180;
  double rto = PI * to / 180;
  xstart = x1 + (int)(1024 * cos(rfrom));
  ystart = y1 - (int)(1024 * sin(rfrom));
  xend = x1 + (int)(1024 * cos(rto));
  yend = y1 - (int)(1024 * sin(rto));
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  SetRect(&rectNew, x1-r, y1-r, x1+r+1, y1+r+1);
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hbrushOld = SelectObject(info->hdc, info->hbrush);
  Pie(info->hdc, x1-r, y1-r, x1+r+1, y1+r+1, xstart, ystart, xend, yend);
  SelectObject(info->hdc, hbrushOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_clear(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  RECT rect;
  rect.top = rect.left = 0;
  rect.right = rect.bottom = 32767;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  grow_hitrect(info, &rect);
  /* Perform drawing */
  FillRect(info->hdc, &rect, GetStockObject(WHITE_BRUSH));
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_draw_text(struct window *linfo, int x, int y, char *s)
{
  SIZE sz;
  TEXTMETRIC tm;
  RECT rectNew;
  struct M_window *info = (struct M_window*)linfo;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  GetTextMetrics(info->hdc, &tm);
  GetTextExtentPoint32(info->hdc, s, strlen(s), &sz);
  SetRect(&rectNew, x, y-tm.tmAscent, x+sz.cx, y+tm.tmDescent);
  InflateRect(&rectNew, 2, 2);
  grow_hitrect(info, &rectNew);
  /* Preform drawing */
  TextOut(info->hdc, x, y, s, strlen(s));
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void 
wdc_rect_text(struct window *linfo, 
	      int x, int y, char *s, 
	      int *xptr, int *yptr, int *wptr, int *hptr)
{
  SIZE sz;
  TEXTMETRIC tm;
  struct M_window *info = (struct M_window*)linfo;

  EnterCriticalSection(info->lock);
  GetTextMetrics(info->hdc, &tm);
  GetTextExtentPoint32(info->hdc, s, strlen(s), &sz);

  /* There is more metric information for outline fonts */
  if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
  {
    int nAscent = 0;
    int nDescent = 0;
    int nShift = 0;
    GLYPHMETRICS gm;
    UINT uChar;
    DWORD ok;
    /* Initialize mat2 */
    static MAT2 mat2;
    mat2.eM11.value = mat2.eM22.value = 1;
    /* Iterate over characters */
    while ((uChar = (UINT)*s++))
    {
      if (uChar==0x20 || uChar==0xA0)
        continue; // Seems that NT40 has no outline for spaces.
      ok = GetGlyphOutline(info->hdc, uChar, GGO_METRICS, &gm, 0, NULL, &mat2);
      if (ok == GDI_ERROR)
	break;
      if (gm.gmptGlyphOrigin.y - nShift > nAscent)
	nAscent = gm.gmptGlyphOrigin.y - nShift;
      if (nShift - gm.gmptGlyphOrigin.y + (int)gm.gmBlackBoxY > nDescent)
	nDescent = nShift - gm.gmptGlyphOrigin.y + (int)gm.gmBlackBoxY;
      nShift += gm.gmCellIncY;
    }
    /* Unpack information */
    if (uChar == 0)
    {
      SIZE vpExt, wExt;
      GetViewportExtEx(info->hdc, &vpExt);
      GetWindowExtEx(info->hdc, &wExt);
      LeaveCriticalSection(info->lock);
      nAscent = MulDiv(nAscent, wExt.cy, vpExt.cy);
      nDescent = MulDiv(nDescent, wExt.cy, vpExt.cy);
      *xptr = x;
      *yptr = y - nAscent;
      *wptr = sz.cx;
      *hptr = nAscent + nDescent;
      return;
    }
  }

  /* Another trial for TT fonts */
  if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
  {
    OUTLINETEXTMETRIC otm;
    if (GetOutlineTextMetrics(info->hdc, sizeof(otm), &otm))
    {
      LeaveCriticalSection(info->lock);
      *xptr = x;
      *yptr = y - otm.otmAscent;
      *wptr = sz.cx;
      *hptr = otm.otmAscent + otm.otmDescent;
      return;
    }
  }

  /* Default for non TT fonts */
  LeaveCriticalSection(info->lock);
  *xptr = x;
  *yptr = y - tm.tmAscent;
  *wptr = sz.cx;
  *hptr = tm.tmAscent + tm.tmDescent;
}


/***** release 2 ****/


static void 
wdc_fill_polygon(struct window *linfo, short (*points)[2], unsigned int n)
{
  RECT rectNew;
  HBRUSH hbrushOld;
  unsigned int i;
  struct M_window *info = (struct M_window*)linfo;
  /* allocate (static) array of points */
  static POINT *pnts = NULL;
  static unsigned int npnts = 0;
  if (npnts < n)
  {
    POINT *newpnts;
    unsigned int newsize = npnts + 16;
    while (newsize < n)
      newsize *= 2;
    newpnts = (POINT*)tl_malloc(newsize * sizeof(POINT));
    if (!newpnts)
      return;
    free(pnts);
    pnts = newpnts;
    npnts = newsize;
  }
  /* fill array of points and compute enclosing rectangle */
  pnts[0].x = points[0][0];
  pnts[0].y = points[0][1];
  SetRect(&rectNew, pnts[0].x, pnts[0].y, pnts[0].x, pnts[0].y);
  for (i=1; i<n; i++)
  {
    pnts[i].x = points[i][0];
    pnts[i].y = points[i][1];
    rectNew.left = min(rectNew.left, pnts[i].x);
    rectNew.right = max(rectNew.right, pnts[i].x);
    rectNew.top = min(rectNew.top, pnts[i].y);
    rectNew.bottom = max(rectNew.bottom, pnts[i].y);
  }
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  grow_hitrect(info, &rectNew);
  /* Perform drawing */
  hbrushOld = SelectObject(info->hdc, info->hbrush);
  Polygon(info->hdc, pnts, n);
  SelectObject(info->hdc, hbrushOld);
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
}


static void
wdc_updatepalette(struct M_window *info, COLORREF color)
{
  if (info->hpal 
      && info->logpal->palNumEntries < info->logpal->palVersion)
    {
      int ir = GetRValue(color);
      int ig = GetGValue(color);
      int ib = GetBValue(color);
      int nidx;
      EnterCriticalSection(info->lock);
      nidx = GetNearestPaletteIndex(info->hpal, color);
      if (((ir ^ info->logpal->palPalEntry[nidx].peRed) & 0xFC) ||
	  ((ig ^ info->logpal->palPalEntry[nidx].peGreen) & 0xFC) ||
	  ((ib ^ info->logpal->palPalEntry[nidx].peBlue) & 0xFC) )
      {
	nidx = info->logpal->palNumEntries;
	info->logpal->palPalEntry[nidx].peRed = ir;
	info->logpal->palPalEntry[nidx].peGreen = ig;
	info->logpal->palPalEntry[nidx].peBlue = ib;
	info->logpal->palPalEntry[nidx].peFlags = 0;
	if (ResizePalette(info->hpal, nidx+1))
	{
	  info->logpal->palNumEntries = nidx+1;
	  SetPaletteEntries(info->hpal, nidx, 1, &info->logpal->palPalEntry[nidx]);
	}
      }
      LeaveCriticalSection(info->lock);
  }
}

static void 
wdc_setcolor(struct window *linfo, int x)
{
  struct M_window *info = (struct M_window*)linfo;
  DeleteObject(info->hpen);
  DeleteObject(info->hbrush);
  info->hpen = NULL;
  info->hbrush = NULL;
  switch(x)
  {
    case COLOR_FG:
      x = RGB(0,0,0);
      info->hpen = GetStockObject(BLACK_PEN);
      info->hbrush = GetStockObject(BLACK_BRUSH);
      break;
    case COLOR_BG:
      x = RGB(255,255,255);
      info->hpen = GetStockObject(WHITE_PEN);
      info->hbrush = GetStockObject(WHITE_BRUSH);
      break;
    case COLOR_GRAY:
      x = GetSysColor(COLOR_GRAYTEXT);
      info->hpen = CreatePen(PS_SOLID,0,(COLORREF)x);
      info->hbrush = GetStockObject(GRAY_BRUSH);
      break;
    default:
      wdc_updatepalette(info, (COLORREF)x);
      info->hpen = CreatePen(PS_SOLID,0,(COLORREF)x);
      info->hbrush = CreateSolidBrush((COLORREF)x);
      break;
  }
  SetTextColor(info->hdc, (COLORREF)x);
}


static int
wdc_alloccolor(struct window *linfo, double r, double g, double b)
{
  int ir = min(255, max(0, (int)(r*255)));
  int ig = min(255, max(0, (int)(g*255)));
  int ib = min(255, max(0, (int)(b*255)));
  COLORREF color = RGB(ir, ig, ib);
  wdc_updatepalette((struct M_window*)linfo, color);
  return color;
}


static void 
wdc_clip(struct window *linfo, 
	int x, int y, unsigned int w, unsigned int h)
{
  struct M_window *info = (struct M_window*)linfo;
  EnterCriticalSection(info->lock);
  SelectClipRgn(info->hdc, NULL);
  if (w>0 || h>0)
    IntersectClipRect(info->hdc,x, y, x+w, y+h);
  LeaveCriticalSection(info->lock);
}


static int
wdc_pixel_map(struct window *linfo, unsigned int *data, 
	     int x, int y, unsigned int w, unsigned int h, 
	     unsigned int sx, unsigned int sy)
{
  struct M_window *info = (struct M_window*)linfo;
  BITMAPINFO bmi;
  RECT rectNew;
  /* Always accept optimized method */
  if (!data)
    return TRUE;
  /* Prepare the bitmapinfo structure */
  memset(&bmi, 0, sizeof(BITMAPINFO));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -(int)h;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  /* Enter critical section */
  EnterCriticalSection(info->lock);
  /* Compute hit rectangle */
  SetRect(&rectNew, x, y, x+w*sx, y+h*sy);
  grow_hitrect(info, &rectNew);
  /* Sends the DIB bits to the device */
  StretchDIBits(info->hdc,  x, y, w*sx, h*sy,  0, 0, w, h, 
		(void*)data, &bmi, DIB_RGB_COLORS, SRCCOPY );
  /* Leave critical section */
  LeaveCriticalSection(info->lock);
  return TRUE;
}



static void 
wbm_hilite(struct window *linfo, int code, int x1, int y1, int x2, int y2)
{
  struct M_window *info = (struct M_window*)linfo;
  struct msg_configwin arg;
  arg.status = code;
  arg.wid = info->wid;
  arg.x = x1;
  arg.y = y1;
  arg.w = x2;
  arg.h = y2;
  if (lushport)
    lushsend(LUSH_HILITEWIN, (LUSHARG*)&arg);
}



/* ============================  DRIVER FOR WBM WINDOW */


struct gdriver wbm_driver = {
  "WBM",
  /* release 1 */
  wbm_begin,
  wbm_end,
  wbm_close,
  wbm_xsize,
  wbm_ysize,
  wdc_setfont,
  wdc_clear,
  wdc_draw_line,
  wdc_draw_rect,
  wdc_draw_circle,
  wdc_fill_rect,
  wdc_fill_circle,
  wdc_draw_text,
  /* release 2 */
  wdc_setcolor,
  wdc_alloccolor,
  wdc_fill_polygon,
  wdc_rect_text,
  NIL, /* gspecial */
  wdc_clip,
  wbm_hilite,
  wdc_pixel_map,
  NIL, /* hinton_map */
  /* release 3 */
  wdc_draw_arc,
  wdc_fill_arc,
};




/* ============================  CREATION OF WBM WINDOW */



/* wbm_new_window
 * -- creates the main data structure
 */

static struct M_window *
wbm_new_window(int x, int y, 
               unsigned int w, unsigned int h, 
               char *name, BOOL toplevel)
{
  struct M_window *info = NULL;
  struct msg_createwin arg;
  int i;
  
  /* Locate empty window slot */
  for(i=0; i<MAXWIN; i++)
    if (! wind[i].lwin.used ) 
    {
      info = &wind[i];
      break;
    }
  if (! info)
    error("wbm_window","Too many TL windows",NIL);
  if (! lushport)
    error("wbm_window","This function requires WinLush",NIL);
  /* Perform RPC */
  arg.x = x;
  arg.y = y;
  arg.w = w;
  arg.h = h;
  arg.title = (const char *)name;
  if (toplevel)
    lushsend(LUSH_CREATETOPWIN, (LUSHARG*)&arg);
  else
    lushsend(LUSH_CREATEWIN, (LUSHARG*)&arg);
  if (arg.status != MSG_OK)
    error("wbm_window","Error while creating window",NIL);
  /* Unpack return values */
  info->dbflag = 0;
  info->szpal = 0;
  info->lwin.used = 1;
  info->wid = arg.wid;
  info->hdc = arg.hdc;
  info->lock = arg.lock;
  info->hpal = arg.hpal;
  info->logpal = arg.logpal;
  /* Hitrect is initially null */
  SetRectEmpty(&info->hitrect);
  /* Create pen and brush */
  info->hpen = GetStockObject(BLACK_PEN);
  info->hbrush = GetStockObject(BLACK_BRUSH);
  SelectObject(info->hdc, GetStockObject(NULL_PEN));
  SelectObject(info->hdc, GetStockObject(NULL_BRUSH));
  SetTextColor(info->hdc, RGB(0,0,0));
  SetTextAlign(info->hdc, TA_LEFT|TA_BASELINE);
  SetBkMode(info->hdc, TRANSPARENT);
  wdc_setfont(&info->lwin, FONT_STD);
  return info;
}



/* wbm_window
 * -- opens a wbm window
 */

static at *
wbm_window(int x, int y, int w, int h, char *name, BOOL toplevel)
{
  at *ans;
  struct M_window *info;
  info = wbm_new_window(x,y,w,h,name,toplevel);
  ans = new_extern( &window_class, info );
  info->lwin.gdriver = &wbm_driver;
  info->lwin.backptr = ans;
  info->lwin.font = new_safe_string(FONT_STD);
  info->lwin.color = COLOR_FG;
  info->lwin.eventhandler = 0;
  info->lwin.clipw = 0;
  info->lwin.cliph = 0;
  return ans;
}



/* (wbm_window <args>)
 * Returns a Windows BitMapped window
 */

DX(xwbm_window)
{
  at *ans;
  ALL_ARGS_EVAL;
  switch(arg_number)
    {
    case 0:
      ans = wbm_window(0,0,0,0,"TL Graphics",FALSE);
      break;
    case 1:
      ans = wbm_window(0,0,0,0,ASTRING(1),FALSE);
      break;
    case 3:
      ans = wbm_window(0,0,AINTEGER(1),AINTEGER(2),ASTRING(3),FALSE);
      break;
    case 5:
      ans = wbm_window(AINTEGER(1),AINTEGER(2),
		       AINTEGER(3),AINTEGER(4),ASTRING(5),FALSE);
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return ans;
}


/* (wbm_toplevel_window <args>)
 * Returns a Windows BitMapped window
 * that is not in the MDI frame
 */

DX(xwbm_toplevel_window)
{
  at *ans;
  ALL_ARGS_EVAL;
  switch(arg_number)
    {
    case 0:
      ans = wbm_window(0,0,0,0,"TL Graphics",TRUE);
      break;
    case 1:
      ans = wbm_window(0,0,0,0,ASTRING(1),TRUE);
      break;
    case 3:
      ans = wbm_window(0,0,AINTEGER(1),AINTEGER(2),ASTRING(3),TRUE);
      break;
    case 5:
      ans = wbm_window(AINTEGER(1),AINTEGER(2),
		       AINTEGER(3),AINTEGER(4),ASTRING(5),TRUE);
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
    }
  return ans;
}



/* ============================  WINDOWS SPECIFIC UTILITIES */


/* returns depth of window display */

DX(xwin_depth)
{
  HDC hdc;
  int depth;
  ARG_NUMBER(0);
  hdc = CreateIC("DISPLAY",NULL,NULL,NULL);
  depth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
  DeleteDC(hdc);
  return NEW_NUMBER(depth);
}


/* adjust location and size of window */

DX(xwin_configure)
{
  struct window *w;
  struct M_window *info;
  struct msg_configwin arg;


  ALL_ARGS_EVAL;
  w = current_window();
  if (w->gdriver == &wbm_driver)
  {
    info = (struct M_window*)w;
    arg.status = 0;
    arg.wid = info->wid;

    if (arg_number!=0)
    {
      ARG_NUMBER(5);
      if (APOINTER(1)) {
	arg.status |= MSG_CONFIGWIN_RAISE;
      }
      if (APOINTER(2)) {
        arg.x = AINTEGER(2);
        arg.status |= MSG_CONFIGWIN_X;
      }
      if (APOINTER(3)) {
        arg.y = AINTEGER(3);
        arg.status |= MSG_CONFIGWIN_Y;
      }
      if (APOINTER(4)) {
        arg.w = AINTEGER(4);
        arg.status |= MSG_CONFIGWIN_W;
      }
      if (APOINTER(5)) {
        arg.h = AINTEGER(5);
        arg.status |= MSG_CONFIGWIN_H;
      }
    }
    if (arg.status)
      lushsend(LUSH_CONFIGWIN, (LUSHARG*)&arg);
    else
      lushsend(LUSH_QUERYWIN, (LUSHARG*)&arg);
    return cons(((arg.status & MSG_CONFIGWIN_ICON) ? NIL : TLtrue()),
		cons(NEW_NUMBER(arg.w),
		     cons(NEW_NUMBER(arg.h), 
		          NIL) ) );
  }
  return NIL;
}



/* clipboard manipulation */

DX(xwin_text_to_clip)
{
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  if (OpenClipboard(NULL))
  {
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, 
                               strlen(s)+1 );
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      strcpy(pmem, s);
      GlobalUnlock(hmem);
      EmptyClipboard();
      SetClipboardData(CF_TEXT, hmem);
    }
    CloseClipboard();
  }
  LOCK(APOINTER(1));
  return APOINTER(1);
}


DX(xwin_clip_to_text)
{
  at *p;
  ARG_NUMBER(0);
  if (OpenClipboard(NULL))
  {
    HGLOBAL hmem = GetClipboardData(CF_TEXT);
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      p = new_string(pmem);
      GlobalUnlock(hmem);
    }
    CloseClipboard();
  }
  return p;
}



/* ============================  PRINTING */


static void 
open_page(struct M_window *info)
{
  if (!info->in_page)
    {
      int hres, vres;
      // Reinitialize page
      StartPage(info->hdc);
      info->in_page = 1;
      // Reset mapping mode
      SetMapMode(info->hdc, MM_ISOTROPIC);
      SetWindowOrgEx(info->hdc, 0, 0, NULL);
      SetWindowExtEx(info->hdc, info->prw, info->prh, NULL);
      hres = GetDeviceCaps(info->hdc, HORZRES);
      vres = GetDeviceCaps(info->hdc, VERTRES);
      SetViewportOrgEx(info->hdc, 0, 0, NULL);
      SetViewportExtEx(info->hdc, hres, vres, NULL);
      // Reset DC attributes
      SetTextAlign(info->hdc, TA_LEFT|TA_BASELINE);
      SetBkMode(info->hdc, TRANSPARENT);
      SelectObject(info->hdc, GetStockObject(NULL_PEN));
      SelectObject(info->hdc, GetStockObject(NULL_BRUSH));
      wdc_setfont(&info->lwin, SADD(info->lwin.font->Object));
      wdc_setcolor(&info->lwin, info->lwin.color);
      if (info->lwin.clipw || info->lwin.cliph)
        wdc_clip(&info->lwin, 
                 info->lwin.clipx, info->lwin.clipy,
                 info->lwin.clipw, info->lwin.cliph );
    }
}


static void 
close_page(struct M_window *info)
{
  if (info->in_page)
    {
      EndPage(info->hdc);
      info->in_page = 0;
    }
}


static void 
wpr_begin(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  if (info->dbflag++ == 0)
    open_page(info);
}


static void 
wpr_end(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  if (--info->dbflag == 0)
    GdiFlush();
}


/* Close a window */

static void 
wpr_close(struct window *linfo)
{
  HFONT hFont;
  struct M_window *info = (struct M_window*)linfo;
  // close current page
  close_page(info);
  // finish document
  EndDoc(info->hdc);
  // cleanup DC attributes
  if (info->hpen)
    DeleteObject(info->hpen);
  if (info->hbrush)
    DeleteObject(info->hbrush);
  hFont = (HFONT)SelectObject(info->hdc, GetStockObject(SYSTEM_FONT));
  if (hFont)
    DeleteObject(hFont);
  // remove dummy critical section
  DeleteCriticalSection(info->lock);
  free(info->lock);
  // perform the actual close
  DeleteDC(info->hdc);
  // set used
  info->lwin.used = 0;
}

static int 
wpr_xsize(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  return info->prw;
}

static int 
wpr_ysize(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  return info->prh;
}

static void 
wpr_clear(struct window *linfo)
{
  struct M_window *info = (struct M_window*)linfo;
  if (linfo->clipw || linfo->cliph)
    wdc_clear(linfo);
  else
    close_page(info);
}

static void 
wpr_gspecial(struct window *linfo, char *s)
{
}

struct gdriver wpr_driver = {
  "WPR",
  /* release 1 */
  wpr_begin,
  wpr_end,
  wpr_close,
  wpr_xsize,
  wpr_ysize,
  wdc_setfont,
  wpr_clear,
  wdc_draw_line,
  wdc_draw_rect,
  wdc_draw_circle,
  wdc_fill_rect,
  wdc_fill_circle,
  wdc_draw_text,
  /* release 2 */
  wdc_setcolor,
  wdc_alloccolor,
  wdc_fill_polygon,
  wdc_rect_text,
  wpr_gspecial,
  wdc_clip,
  NIL, /* hilite */
  wdc_pixel_map,
  NIL, /* hinton_map */
  /* release 3 */
  wdc_draw_arc,
  wdc_fill_arc,
};


/* wpr_default_printer
 * -- returns default printer or NULL
 */

static at *
wpr_default_printer(void)
{
  at *ans;
  PRINTDLG pdlg;
  DEVNAMES *lpDevNames;
  char *name;

  memset(&pdlg, 0, sizeof(pdlg));
  pdlg.lStructSize = sizeof(pdlg);
  pdlg.Flags = PD_RETURNDEFAULT;
  if (!PrintDlg(&pdlg))
    return NIL;
  if (!pdlg.hDevNames)
    return NIL;
  lpDevNames = (DEVNAMES*)GlobalLock(pdlg.hDevNames);
  name = (char*)lpDevNames + lpDevNames->wDeviceOffset;
  ans = new_string(name);
  GlobalUnlock(pdlg.hDevNames);
  return ans;
}


/* wpr_new_window
 * -- creates a new print window
 */

static struct M_window *
wpr_new_window(int x, int y, 
               unsigned int w, unsigned int h, 
               char *name)
{
  struct M_window *info = NULL;
  DOCINFO  dinf;
  int i;
  
  /* Locate empty window slot */
  for(i=0; i<MAXWIN; i++)
    if (! wind[i].lwin.used ) {
      info = &wind[i];
      break;
    }
  if (! info)
    error("wpr_window","Too many TL windows",NIL);
  /* Initialise wid and lock */
  info->wid = 0;
  info->lock = (CRITICAL_SECTION*)tl_malloc(sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(info->lock);
  /* Get printer */
  if (!name)
    error("wpr_window","No printer specified", NIL);
  info->hdc = CreateDC("WINSPOOL", name, NULL, NULL);
  if (!info->hdc)
    error("wpr_window","Unknown printer", new_string(name));
  /* Unpack return values */
  info->dbflag = 0;
  info->szpal = 0;
  info->lwin.used = 1;
  info->hpal = 0;
  info->logpal = 0;
  info->prw = (w ? w : 512);
  info->prh = (h ? h : 512);
  /* Hitrect is initially null */
  SetRectEmpty(&info->hitrect);
  /* Start Document */
  memset(&dinf,0,sizeof(dinf));
  dinf.cbSize = sizeof(dinf);
  dinf.lpszDocName = "Lush print request";
  StartDoc(info->hdc,&dinf);
  info->in_page = 0;
  /* Create pen and brush */
  info->hpen = GetStockObject(BLACK_PEN);
  info->hbrush = GetStockObject(BLACK_BRUSH);
  return info;
}



/* wpr_window
 * -- opens a wpr window (for printing)
 */

static at *
wpr_window(int x, int y, int w, int h, char *name)
{
  at *ans = NIL;
  struct M_window *info;

  info = wpr_new_window(x,y,w,h,name);
  ans = new_extern( &window_class, info );
  info->lwin.gdriver = &wpr_driver;
  info->lwin.backptr = ans;
  info->lwin.font = new_safe_string(FONT_STD);
  info->lwin.color = COLOR_FG;
  info->lwin.eventhandler = 0;
  info->lwin.clipw = 0;
  info->lwin.cliph = 0;
  return ans;
}



/* (wpr_window <args>)
 * Returns a window for printing
 */

DX(xwpr_window)
{
  at *ans;
  at *name = NIL;
  int w = 512;
  int h = 512;

  ALL_ARGS_EVAL;
  switch (arg_number)
    {
    case 1:
      name = APOINTER(1);
    case 0:
      break;
    case 3:
      name = APOINTER(3);
    case 2:
      w = AINTEGER(1);
      h = AINTEGER(2);
      break;
    case 5:
      name = APOINTER(5);
    case 4:
      AINTEGER(1);
      AINTEGER(2);
      w = AINTEGER(3);
      h = AINTEGER(4);
      break;
    default:
      ARG_NUMBER(-1);
      break;
    }
  if (name)
    {
      LOCK(name);
      if (! EXTERNP(name, &string_class)) 
        error(NIL,"String expected", name);
    }
  else 
    {
      name = wpr_default_printer();
      if (!name)
        return NIL;
    }
  ans = wpr_window(0,0,w,h,SADD(name->Object));
  UNLOCK(name);
  return ans;
}



/* (wpr-printers) return the list of printers */

DX(xwpr_printers)
{
  DWORD i;
  DWORD nNeeded;
  DWORD nReturned;
  const char *defprinter = NULL;
  at *def = NIL;
  at *ans = NIL;
  at **where = &ans;
  
  ARG_NUMBER(0);
  def = wpr_default_printer();
  if (def)
    {
      *where = cons(def,NIL);
      where = &((*where)->Cdr);
      defprinter = SADD(def->Object);
    }

  if (_osver & 0x8000)
    {
      // Windows 95
      PRINTER_INFO_5 *pi = 0;
      EnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, 
                   "", 5, NULL, 0, &nNeeded, &nReturned);
      pi = (PRINTER_INFO_5*)tl_malloc(nNeeded);
      EnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, 
                   "", 5, (LPBYTE)pi, nNeeded, &nNeeded, &nReturned);
      for (i=0; i<nReturned; i++)
        {
          char *name = pi[i].pPrinterName;
          if (pi[i].Attributes & PRINTER_ATTRIBUTE_HIDDEN)
            continue;
          if (defprinter && !strcmp(name, defprinter))
            continue;
          *where = cons(new_string(name),NIL);
          where = &((*where)->Cdr);
        }
      free(pi);
    }
  else
    {
      // Windows NT
      PRINTER_INFO_4 *pi = 0;
      EnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, 
                   "", 4, NULL, 0, &nNeeded, &nReturned);
      pi = (PRINTER_INFO_4*)tl_malloc(nNeeded);
      EnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, 
                   "", 4, (LPBYTE)pi, nNeeded, &nNeeded, &nReturned);
      for (i=0; i<nReturned; i++)
        {
          char *name = pi[i].pPrinterName;
          if (pi[i].Attributes & PRINTER_ATTRIBUTE_HIDDEN)
            continue;
          if (defprinter && !strcmp(name, defprinter))
            continue;
          *where = cons(new_string(name),NIL);
          where = &((*where)->Cdr);
        }
      free(pi);
    }
  // Return the result
  return ans;
}



/* ============================  INITIALISATION */


#ifdef __cplusplus
extern "C" 
#endif
void
init_WINdriver(void)
{
  /* Initialize event system */
  set_trigger(transfer_events, process_pending_events);
  InitializeCriticalSection(&lushevent_cs);
  lushevent_buffer = (struct event*)tl_malloc( sizeof(struct event) * EVBUFSIZE );
  lushevent_evput = lushevent_evget = 0;
  /* Initialize windriver calls */
  dx_define("wbm_window",xwbm_window);
  dx_define("wbm_toplevel_window",xwbm_toplevel_window);
  dx_define("win_depth", xwin_depth);
  dx_define("win_configure", xwin_configure);
  dx_define("win_clip_to_text", xwin_clip_to_text);
  dx_define("win_text_to_clip", xwin_text_to_clip);
  /* Print window */
  dx_define("wpr_window",xwpr_window);
  dx_define("wpr_printers",xwpr_printers);
}
