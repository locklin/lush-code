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
 * $Id: graphics.h,v 1.6 2002-08-02 20:31:42 leonb Exp $
 **********************************************************************/

#ifndef GRAPHICS_H
#define GRAPHICS_H
#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
}
#endif
#endif


/* ---------------------------------------- */
/* WINDOWS AS LISP OBJECTS                  */
/* ---------------------------------------- */
   
struct window {
  short used;
  short pad;
  at *backptr;
  struct gdriver *gdriver;
  at *font;
  at *eventhandler;
  int color;
  short clipx, clipy, clipw, cliph;
};


/* Standard font */

#define FONT_STD    "default"

/* Standard colors */

#define COLOR_FG    (-1L)
#define COLOR_BG    (-2L)
#define COLOR_GRAY  (-3L)

/* Colors definitions for OLDsetcolor */

#define COLOR_FAST       (-10000L)
#define COLOR_RGB(r,g,b) (65536*(int)(255*r)+256*(int)(255*g)+(int)(255*b))
#define COLOR_SHADE(x)   ((65536+256+1)*(int)(255*x))
#define RED_256(c)       ((c & 0xff0000L)>>16)
#define GREEN_256(c)     ((c & 0x00ff00L)>>8)
#define BLUE_256(c)      ((c & 0x0000ffL))
#define SHADE_256(c)     ((5*RED_256(c)+9*GREEN_256(c)+2*BLUE_256(c))/16)


/* Functions and Definitions */

extern LUSHAPI TLclass window_class;
LUSHAPI struct window *current_window(void);
LUSHAPI struct window *current_window_no_error(void);


/* ---------------------------------------- */
/* EVENTS                                   */
/* ---------------------------------------- */

struct event {
  at *handler;
  int code;
  int xd,yd,xu,yu;
  char *desc;
};

/* Standard events */

#define EVENT_NONE        (-1L)
#define EVENT_ASCII_MIN   (0L)
#define ASCII_TO_EVENT(i) ((int)(unsigned)i)
#define EVENT_TO_ASCII(i) ((char)i)
#define EVENT_ASCII_MAX   (255L)

#define EVENT_MOUSE_DOWN  (1001L)
#define EVENT_MOUSE_UP    (1002L)
#define EVENT_MOUSE_DRAG  (1003L)

#define EVENT_ARROW_UP    (1011L)
#define EVENT_ARROW_RIGHT (1012L)
#define EVENT_ARROW_DOWN  (1013L)
#define EVENT_ARROW_LEFT  (1014L)
#define EVENT_FKEY        (1015L)

#define EVENT_RESIZE      (2001L)
#define EVENT_HELP        (2002L)
#define EVENT_DELETE      (2003L)
#define EVENT_SENDEVENT   (2004L)

#define EVENT_ALARM       (3001L)

#define EVENT_GLEXPOSE    (4001L)
#define EVENT_EXPOSE      (4001L)


/* Hilites */

#define HILITE_NONE       (0)
#define HILITE_INVERT     (1)
#define HILITE_VECTOR     (2)
#define HILITE_RECT       (3)



/* ---------------------------------------- */
/* GRAPHIC FUNCTIONS                        */
/* ---------------------------------------- */

LUSHAPI int  rgb_draw_idx(int x, int y, struct idx *idx, int sx, int sy);

LUSHAPI void rgb_draw_matrix(int x, int y, at *p, int sx, int sy);

LUSHAPI int  color_draw_idx(int x, int y, struct idx *idx, real minv, real maxv, 
                            int apartx, int aparty, int *colors);

LUSHAPI void color_draw_matrix(int x, int y, at *p, real minv, real maxv, 
                               int apartx, int aparty, int *colors);



/* ---------------------------------------- */
/* DRIVERS                                  */
/* ---------------------------------------- */

#define uint unsigned int 
#define wptr struct window *

struct gdriver {
  /**** gdriver name *** */
  char *name;
  /**** initial routines *** */
  void (*begin) (wptr);		/* begin(device) & end(device) */
  void (*end) (wptr);		/* encapsulate graphics calls */
  void (*close) (wptr);
  int (*xsize) (wptr);
  int (*ysize) (wptr);
  void (*setfont) (wptr, char *);
  void (*clear) (wptr);
  void (*draw_line) (wptr, int, int, int, int);
  void (*draw_rect) (wptr, int, int, uint, uint);
  void (*draw_circle) (wptr, int, int, uint);
  void (*fill_rect) (wptr, int, int, uint, uint);
  void (*fill_circle) (wptr, int, int, uint);
  void (*draw_text) (wptr, int, int, char *);
  /**** routines added for release 2 ****/
  void (*setcolor) (wptr, int);
  int  (*alloccolor) (wptr, double, double, double);
  void (*fill_polygon) (wptr, short (*points)[2], uint);
  void (*rect_text) (wptr, int, int, char*, int*, int*, int*, int*);
  void (*gspecial) (wptr, char*);
  void (*clip) (wptr, int, int, uint, uint);
  void (*hilite) (wptr, int, int, int, int, int);
  int (*pixel_map) (wptr, uint*, int, int, uint, uint, uint, uint);
  int (*hinton_map) (wptr, uint*, int, int, uint, uint, uint, uint);
  /**** routines added for release 3 ****/
  void (*draw_arc) (wptr, int, int, uint, int, int);
  void (*fill_arc) (wptr, int, int, uint, int, int);
  /**** routines added for lush ****/
  void (*get_image) (wptr, uint*, int, int, uint, uint);
  int (*get_mask)(wptr, uint*, uint*, uint*);
};

#undef uint
#undef wptr



/* -------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* GRAPHICS_H */



