/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */

/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/
  

#include "header.h"
#include "graphics.h"

/****** PSDRIVER.PRO contains PostScript programs for *****

px pw INI

ps_setfont		sz (font) SF
ps_setcolor		r g b SC
			SCFG
			SCBG
			SCGRAY

ps_draw_line		x y x y DL
ps_draw_rect		w h x y DR
ps_fill_rect		w h x y FR
ps_draw_circle		x y r DC
ps_fill_circle		x y r FC

ps_clear		showpage
ps_draw_text		(text) x y DT

ps_clip                 w h x y CLIP
ps_fill_polygon		x y PSTART x y FP PEND

ps_pixel_map		nc nl apx apy x y  PM  hexa
ps_hinton_map		nc nl ap ap x y HM hexa

***********************************************************/

/* ============================  MACDRIVER STRUCTURES */

#define MAXWIN        16

static struct M_window {
    struct window lwin;
    char *filename;
    FILE *f;
    int  page,w,h;
    int  colorscript;
} wind[MAXWIN];


/* ============================  WINDOW CREATION */

struct M_window *ps_new_window(x,y,w,h,name)
int x,y;
unsigned short w,h;
char *name;
{
    register struct M_window *info;
    register int i;

    info = NULL;
    for( i=0;i<MAXWIN; i++)
	   ifn ( wind[i].lwin.used ) {
	      info = &wind[i];
	      break;
	   }
    ifn (info)
       error(NIL,"too many PS windows",NIL);

    if (w ==0 || h == 0)
       w = h = 512;

    info->filename = malloc(strlen(name)+1);
    info->w = w;
    info->h = h;
    strcpy(info->filename,name);
    info->f = NIL;
    info->page = 1;
    info->colorscript = 0;
    
    return info;
}


/* ============================  UTILITY */

static void begin(info)
struct M_window *info;
{
	char fname[150];
        File *hdrf;
	FILE *hdr;
	char c;
	
	if (!info->f) {
		if (info->filename[0]=='$' || info->filename[0]=='|')
			strcpy(fname,info->filename);
		else
			sprintf(fname,"%s.%d",info->filename,info->page);

		info->f = open_write(fname,NIL)->stream.fp;
		hdr = (hdrf=open_read("psdriver.pro",NIL))->stream.fp;
		while ((c=getc(hdr))!=(char)EOF)
			fprintf(info->f,"%c",c);
		file_close(hdrf);
		
		fprintf(info->f,"%d %d INI\n",info->w, info->h);
	}
}


/* ============================  BASIC FUNCTIONS */


/* Graphics calls are always enclosed between one or more
   Begin/End pair. */

void ps_begin(info)
struct M_window *info;
{
}

void ps_end(info)
struct M_window *info;
{
}


/* Close a window */

void ps_close(info)
struct M_window *info;
{
	void ps_clear();
	
	ps_clear(info);
	free(info->filename);
    	info->lwin.used = 0;
}


/* Return the size of a window */

int ps_xsize(info)
struct M_window *info;
{
	return info->w;
}

int ps_ysize(info)
struct M_window *info;
{
	return info->h;
}


/* set the font in a window */

void ps_setfont(info,f)
struct M_window *info;
char *f;
{
  int size;
  char font[128];
  char *s;
	
  begin(info);
  strcpy(font,f);
  s = font+strlen(f);
  while (s[-1]>='0' && s[-1]<='9')
    s--;
  if (s)
    size = atoi(s);
  else
    size = 11;
  if (s[-1]=='-')
    s--;
  *s=0;
  fprintf(info->f,"%d /%s SF\n",size,font);
}


/* Drawing functions */

void ps_draw_line(info,x1,y1,x2,y2)
struct M_window *info;
short x1,y1;
unsigned short x2,y2;
{
	begin(info);
	fprintf(info->f,"%d %d %d %d DL\n",x1,y1,x2,y2);
}

void ps_draw_rect(info,x1,y1,x2,y2)
struct M_window *info;
short x1,y1;
unsigned short x2,y2;
{
	begin(info);
	fprintf(info->f,"%d %d %d %d DR\n",x2,y2,x1,y1);
}

void ps_draw_circle(info,x1,y1,r)
struct M_window *info;
short x1,y1;
unsigned short r;
{
	begin(info);
	fprintf(info->f,"%d %d %d DC\n",x1,y1,r);
}

void ps_fill_rect(info,x1,y1,x2,y2)
struct M_window *info;
short x1,y1;
unsigned short x2,y2;
{
	begin(info);
	fprintf(info->f,"%d %d %d %d FR\n",x2,y2,x1,y1);
}

void ps_fill_circle(info,x1,y1,r)
struct M_window *info;
short x1,y1;
unsigned short r;
{
	begin(info);
	fprintf(info->f,"%d %d %d FC\n",x1,y1,r);
}


void ps_clear(info)
struct M_window *info;
{
	if (info->f) {
		fprintf(info->f,"showpage\n");
		fclose(info->f);
		info->f = NIL;
		info->page++;
	}
}

void ps_draw_text(info,x,y,s)
struct M_window *info;
short x,y;
char *s;
{
  begin(info);
  fputc('(',info->f);
  while (*s) {
    if (*s=='(' || *s==')' || *s=='\\')
      fputc('\\',info->f);
    fputc(*s,info->f);
    s++;
  }
  fprintf(info->f,") %d %d DT\n",x,y);
}



/***** release 2 ****/

void ps_clip(info, x, y, w, h)
struct M_window *info;
short x, y;
unsigned short w, h;
{
  begin(info);
  if (w == 0 && h == 0)
    fprintf(info->f,"%d %d %d %d CLIP\n",info->w,info->h,0,0);
  else 
    fprintf(info->f,"%d %d %d %d CLIP\n",w,h,x,y);
}


void ps_fill_polygon(info,points,n)
struct M_window *info;
short (*points)[2];
unsigned int n;
{
	int i;
	begin(info);
	for(i=0;i<n;i++)
		if (i==0)
			fprintf(info->f,"%d %d PSTART\n",points[i][0],points[i][1]);
		else if (i<n-1)
			fprintf(info->f,"%d %d FP\n",points[i][0],points[i][1]);
		else
			fprintf(info->f,"%d %d PEND\n",points[i][0],points[i][1]);
}


void ps_setcolor(info,x)
struct M_window *info;
long x;
{
    begin(info);
    switch(x)
    {
    case COLOR_FG:
            fprintf(info->f,"SCFG\n");
	    break;
    case COLOR_BG:
            fprintf(info->f,"SCBG\n");
	    break;
    case COLOR_GRAY:
            fprintf(info->f,"SCGRAY\n");
	    break;
    default:
	    fprintf(info->f,"%f %f %f SC\n",
		    (double)RED_256(x)/255.0,
		    (double)GREEN_256(x)/255.0,
		    (double)BLUE_256(x)/255.0 );
	    break;
    }
}


int ps_alloccolor(info,r,g,b)
struct M_window *info;
real r,g,b;
{
  return COLOR_RGB(r,g,b);
}


/***** special ****/

void ps_gspecial(info,s)
struct M_window *info;
char *s;
{
  begin(info);
  fprintf(info->f,"%s\n",s);

  /* Special codes for changing 
     the color printing strategy */

  if (!strcmp(s,"%%%PSDRIVER:COLOR4"))
    info->colorscript = 4;
  if (!strcmp(s,"%%%PSDRIVER:COLOR8"))
    info->colorscript = 8;
  if (!strcmp(s,"%%%PSDRIVER:B&W"))
    info->colorscript = 0;
}


static char *hexmap="0123456789ABCDEF";

int ps_pixel_map(info,data,x,y,w,h,sx,sy)
struct M_window *info;
int  *data;
short x,y;
unsigned short w,h,sx,sy;
{
  int i,j,xx,rr,gg,bb;
	
  ifn (data)
    return TRUE;
  
  begin(info);
  
  if (info->colorscript==8) {
    int rr,gg,bb;
    fprintf(info->f,"%d %d 8 %d %d %d %d CPM\n",w,h,sx,sy,x,y);
    for(i=j=0; i<w*h; i++, j+=6) {
      if (j==48) {
	fprintf(info->f,"\n");
	j = 0;
      }
      xx = *data++;
      rr = RED_256(xx);     
      gg = GREEN_256(xx);      
      bb = BLUE_256(xx);
      fprintf(info->f,"%c%c%c%c%c%c",
	      hexmap[ (rr&0xf0)>>4 ],
	      hexmap[ rr&0xf ],
	      hexmap[ (gg&0xf0)>>4 ],
	      hexmap[ gg&0xf ],
	      hexmap[ (bb&0xf0)>>4],
	      hexmap[ bb&0xf ]  );
    }
    while (j++<48)
      fprintf(info->f,"0");
    fprintf(info->f,"\n");

  } else if (info->colorscript==4) {

    fprintf(info->f,"%d %d 4 %d %d %d %d CPM\n",w,h,sx,sy,x,y);
    for(i=j=0; i<w*h; i++, j+=3) {
      if (j==48) {
	fprintf(info->f,"\n");
	j = 0;
      }
      xx = *data++;
      rr = RED_256(xx);     
      gg = GREEN_256(xx);      
      bb = BLUE_256(xx);
      fprintf(info->f,"%c%c%c",
	      hexmap[(rr&0xf0)>>4],
	      hexmap[(gg&0xf0)>>4],
	      hexmap[(bb&0xf0)>>4]  );
    }
    while (j++<48)
      fprintf(info->f,"0");
    fprintf(info->f,"\n");

  } else {

    fprintf(info->f,"%d %d %d %d %d %d PM\n",w,h,sx,sy,x,y);
    for(i=j=0; i<w*h; i++,j+=2) {
      if (j==48) {
	fprintf(info->f,"\n");
	j = 0;
      }
      xx = *data++;
      xx = SHADE_256(xx);
      fprintf(info->f,"%c%c",
	      hexmap[(xx&0xff)>>4],
	      hexmap[(xx&0xf)] );
    }
    while (j++<48)
      fprintf(info->f,"0");
    fprintf(info->f,"\n");

  }
}

int ps_hinton_map(info,data,x,y,ncol,nlin,len,apart)
struct M_window *info;
int  *data;
short x,y;
unsigned short ncol,nlin,len,apart;
{
  int i;
  
  ifn (data)
    return TRUE;
  
  begin(info);
  fprintf(info->f,"%d %d %d %d %d %d HM\n",ncol,nlin,apart,apart,x,y);
  for(i=0;i<len;i++) {
    fprintf(info->f,"%c%c",
	    hexmap[(*data&0xff)>>4],
	    hexmap[(*data&0xf)] );
    if ((i&0xf)==0xf)
      fprintf(info->f,"\n");
		data++;
  }
  for(;i<ncol*nlin;i++) {
    fprintf(info->f,"00");
    if (i&0xf==0xf)
      fprintf(info->f,"\n");
  }
  if (i&0xf)
    fprintf(info->f,"\n");
}



int ps_get_mask(info, red_mask, green_mask, blue_mask)
struct M_window *info;
unsigned int *red_mask, *green_mask, *blue_mask;
{
  *red_mask = 0xff0000;
  *green_mask = 0xff00;
  *blue_mask = 0xff;
  return 1;
}



struct gdriver ps_driver = {
    "PS",
    ps_begin,
    ps_end,
    ps_close,
    ps_xsize,
    ps_ysize,
    NIL,  /*** old setcolor ***/
    ps_setfont,
    ps_clear,
    ps_draw_line,
    ps_draw_rect,
    ps_draw_circle,
    ps_fill_rect,
    ps_fill_circle,
    ps_draw_text,
    ps_setcolor,
    ps_alloccolor,
    ps_fill_polygon,
    NIL,
    ps_gspecial,
    ps_clip,
    NIL,
    NIL,
    NIL,
    ps_pixel_map,
    ps_hinton_map,
    NIL,
    ps_get_mask,
};

at *ps_window(x,y,w,h,name)
int x,y,w,h;
char *name;
{
    struct M_window *info;
    register at *ans;
    info = ps_new_window(x,y,w,h,name,TRUE);
    ans = new_extern( &window_class, info, XT_WINDOW );
    info->lwin.used       = 1;
    info->lwin.font       = new_safe_string(FONT_STD);
    info->lwin.color      = COLOR_FG;
    info->lwin.gdriver    = &ps_driver;
    return ans;
}
DX(xps_window)
{
    at *ans;

    ALL_ARGS_EVAL;
    switch(arg_number)
    {
    case 0:
	       ans = ps_window(0,0,0,0,"sngraphics.ps");
	       break;
    case 1:
	       ans = ps_window(0,0,0,0,ASTRING(1));
        break;
    case 3:
	       ans = ps_window(0,0,AINTEGER(1),AINTEGER(2),ASTRING(3));
	       break;
    case 5:
	       ans = ps_window(AINTEGER(1),AINTEGER(2),
		                       AINTEGER(3),AINTEGER(4),ASTRING(5));
	       break;
    default:
	       ARG_NUMBER(-1);
    }
    return ans;
}






/* ============================  INITIALISATION */


init_ps_driver()
{
        dx_define("ps_window",xps_window);
}
