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

/****** PSDRIVER.PRO contains PostScript programs for *****

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
ps_fill_polygon		x y PSTART x y FP PEND
ps_pixel_map		nc nl apx apy x y  PM  hexa
ps_hinton_map		nc nl ap ap x y HM hexa

***********************************************************/

/* ============================  DRIVER STRUCTURES */

#define MAXWIN        16
#define PAGEHEIGHT    (10*72)
#define PAGEWIDTH     (14*72/2)
#define PAGEOFFSET    (36)


struct M_window {
   struct window lwin;
   char *filename;
   FILE *f;
   int  page,w,h;
   int  pageflag;
   double pagescale;
};


/* ============================  WINDOW CREATION */

static struct M_window *ps_new_window(int x, int y, unsigned int w, unsigned int h, const char *name)
{
   extern mt_t mt_window; // defined in graphics.c

   struct M_window *info = mm_allocv(mt_window, sizeof(struct M_window));
   assert(info);
   if (w ==0 || h == 0)
      w = h = 512;
   info->filename = strdup(name); /* info->filename is not a maanged string */
   info->w = w;
   info->h = h;
   info->f = NIL;
   info->page = 0;
   info->pageflag = 1;
   info->pagescale = 1.0;
   return info;
}


/* ============================  UTILITY */


static void make_prolog(struct M_window *info)
{
   while (info->w*info->pagescale > PAGEWIDTH ||
          info->h*info->pagescale > PAGEHEIGHT)
      info->pagescale *= 0.72;
  
   info->f = open_write(info->filename,"ps");
   FMODE_TEXT(info->f);
   fprintf(info->f,"%%!PS-Adobe-2.0 EPSF-2.0\n");
   fprintf(info->f,"%%%%Title: %s\n",info->filename);
   
   fprintf(info->f,"%%%%BoundingBox: %d %d %d %d\n",
           PAGEOFFSET , PAGEHEIGHT+PAGEOFFSET-(int)(info->h*info->pagescale),
           PAGEOFFSET+(int)(info->w*info->pagescale), PAGEOFFSET+PAGEHEIGHT );
   
   FILE *hdr = open_read("psdriver.pro",NIL);
   int maybecrlf = 0;
   char c;
   while ((c=getc(hdr)) != (char)EOF) {
    if (c=='\r') {
       maybecrlf = 1;
       fputc('\n', info->f);

    } else if (c=='\n') {
       if (!maybecrlf)
          fputc('\n', info->f);
       maybecrlf = 0;

    } else {
       fputc(c, info->f);
       maybecrlf = 0;
    }
   }
   file_close(hdr);
}

static void open_page(struct M_window *info)
{
   fprintf(info->f,"%%%%Page: %d %d\n",
           info->page,info->page);
   fprintf(info->f,"%f %f %d %d BEGINPAGE\n",
           info->pagescale, - info->pagescale,
           PAGEOFFSET, PAGEHEIGHT+PAGEOFFSET );
   info->pageflag = 0;
}


static void close_page(struct M_window *info)
{
   fprintf(info->f,"ENDPAGE\n\n");
   info->pageflag = 1;
}


static void begin(struct M_window *info)
{
   if (!info->f)
      make_prolog(info);
   if (info->pageflag) {
      info->page += 1;
      open_page(info);
   }
}


/* ============================  BASIC FUNCTIONS */


/* Graphics calls are always enclosed between one or more
   Begin/End pair. */

static void ps_begin(struct window *linfo)
{
}

static void ps_end(struct window *linfo)
{
}


/* Close a window */

static void ps_close(struct window *linfo)
{
   struct M_window *info = (struct M_window*)linfo;
   if (info->f) {
      if (info->pageflag == 0)
         close_page(info);
      fprintf(info->f,"%%%%Trailer\n");
      fprintf(info->f,"%%%%Pages: %d %d\n",info->page,1);
      if (info->f!=stdin && info->f!=stdout && info->f!=stderr)
         if (pclose(info->f) < 0)
            fclose(info->f);
   }
   free(info->filename);
   info->lwin.used = 0;
}


/* Return the size of a window */

static int ps_xsize(struct window *linfo)
{
   struct M_window *info = (struct M_window*)linfo;
   return info->w;
}

static int ps_ysize(struct window *linfo)
{
   struct M_window *info = (struct M_window*)linfo;
   return info->h;
}


/* set the font in a window */

static const char *ps_setfont(struct window *linfo, const char *f)
{
   struct M_window *info = (struct M_window*)linfo;
   int size;
   char font[128];
   char *s;
   if (!strcmp(f, FONT_STD))
      f = mm_strdup("Helvetica-11");
   begin(info);
   strncpy(font,f,126);
   s = font+strlen(f);
   while (s[-1]>='0' && s[-1]<='9' && s>font)
      s--;
   if (s)
      size = atoi(s);
   else
      size = 11;
   if (s[-1]=='-' && s>font)
      s--;
   *s=0;
   fprintf(info->f,"%d /%s SF\n",size,font);
   return f;
}


/* Drawing functions */

static void ps_draw_line(struct window *linfo, int x1, int y1, int x2, int y2)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d %d DL\n",x1,y1,x2,y2);
}

static void ps_draw_rect(struct window *linfo, 
                         int x1, int y1, unsigned int x2, unsigned int y2)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d %d DR\n",x2,y2,x1,y1);
}

static void ps_draw_circle(struct window *linfo, 
                           int x1, int y1, unsigned int r)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d DC\n",x1,y1,r);
}

static void ps_draw_arc(struct window *linfo, 
                        int x1, int y1, unsigned int r, int from, int to)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d %d %d DA\n",x1,y1,r,from,to);
}

static void ps_fill_rect(struct window *linfo, 
                         int x1, int y1, unsigned int x2, unsigned int y2)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d %d FR\n",x2,y2,x1,y1);
}

static void ps_fill_circle(struct window *linfo, 
                           int x1, int y1, unsigned int r)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d FC\n",x1,y1,r);
}

static void ps_fill_arc(struct window *linfo, 
                        int x1, int y1, unsigned int r, int from, int to)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%d %d %d %d %d FA\n",x1,y1,r,from,to);
}


static void ps_draw_text(struct window *linfo, int x, int y, const char *s)
{
   struct M_window *info = (struct M_window*)linfo;
   unsigned char c;
   
   begin(info);
   fprintf(info->f,"(");
   while (*s) {
      c = *s++;
      if (c>127 || c<32 || c==127)
       fprintf(info->f,"\\%03o",c);
      else if (c==')' || c=='(' || c=='\\') 
         fprintf(info->f,"\\%c",c);
      else
         fprintf(info->f,"%c",c);
   }
   fprintf(info->f,") %d %d DT\n",x,y);
}



/***** release 2 ****/


static void ps_fill_polygon(struct window *linfo, short (*points)[2], unsigned int n)
{
   struct M_window *info = (struct M_window*)linfo;
   unsigned int i;
   begin(info);
   for(i=0;i<n;i++)
      if (i==0)
         fprintf(info->f,"%d %d PSTART\n",points[i][0],points[i][1]);
      else if (i<n-1)
         fprintf(info->f,"%d %d FP\n",points[i][0],points[i][1]);
      else
         fprintf(info->f,"%d %d PEND\n",points[i][0],points[i][1]);
}


static void ps_setcolor(struct window *linfo, int x)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   switch(x) {

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
              (x&0xff0000)/16777216.0, (x&0xff00)/65536.0, (x&0xff)/256.0);
      break;
   }
}


static int ps_alloccolor(struct window *linfo, double r, double g, double b)
{
   return COLOR_RGB(r,g,b);
}




/***** special ****/

static void ps_gspecial(struct window *linfo, const char *s)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   fprintf(info->f,"%s\n",s);
}


static void ps_clip(struct window *linfo, 
                    int x, int y, unsigned int w, unsigned int h)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   if (w==0 && h==0)
      fprintf(info->f,"%d %d 0 0 CLIP\n", info->w, info->h);
   else
      fprintf(info->f,"%d %d %d %d CLIP\n",w,h,x,y);
}

static void ps_set_linestyle(struct window *linfo, int ls)
{
   struct M_window *info = (struct M_window*)linfo;
   begin(info);
   if (ls>=0 && ls<4)
      fprintf(info->f,"L%d\n",ls);
}


static char *hexmap="0123456789ABCDEF";

static int ps_pixel_map(struct window *linfo, unsigned int *data, 
                        int x, int y, unsigned int w, unsigned int h, 
                        unsigned int sx, unsigned int sy)
{
   struct M_window *info = (struct M_window*)linfo;
   int color = false;
   int size = w * h;
   int i;
   ifn (data)
      return true;
   /* Check if we want colors */
   for (i=0; i<size && !color; i++) {
      unsigned int c = data[i];
      if ( ( c ^ (c>>8) ) & 0xffff )
         color = true;
   }
   /* Display */
   begin(info);
   if (color) {
      /* Color */
      fprintf(info->f,"%d %d 8 %d %d %d %d CPM\n",w,h,sx,sy,x,y);
      for(i=0; i<size; i++) {
        fprintf(info->f,"%06x", data[i]);
        if ((i&0x7)==0x7)
           fprintf(info->f,"\n");
      }
      if (i&0x7)
         fprintf(info->f,"\n");

   } else {
      /* Gray scale */
      fprintf(info->f,"%d %d %d %d %d %d PM\n",w,h,sx,sy,x,y);
      for(i=0; i<size; i++) {
         fprintf(info->f,"%02x", data[i] & 0xff);
         if ((i&0xf)==0xf)
            fprintf(info->f,"\n");
      }
      if (i&0xf)
         fprintf(info->f,"\n");
   }
   return true;
}

static int ps_hinton_map(struct window *linfo, unsigned int *data, 
                         int x, int y, unsigned int ncol, unsigned int nlin, 
                         unsigned int len, unsigned int apart)
{
   struct M_window *info = (struct M_window*)linfo;
   unsigned int i;
   
   ifn (data)
      return true;
   
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
      if ((i&0xf)==0xf)
         fprintf(info->f,"\n");
   }
   if (i&0xf)
      fprintf(info->f,"\n");
   return true;
}





/********** clear *************/

static void ps_clear(struct window *linfo)
{
   struct M_window *info = (struct M_window*)linfo;
   if (info->f) 
      if (info->pageflag == 0) {
         if (linfo->clipw || linfo->cliph) {
	    /* clip is set: clear clip area */
	    ps_setcolor(linfo, COLOR_BG);
	    ps_fill_rect(linfo,
                         linfo->clipx, linfo->clipy,
                         linfo->clipw, linfo->cliph);
	    ps_setcolor(linfo, linfo->color);
         } else {
	    /* clip not set: go to next page */
	    close_page(info);
         }
      }
}




/********** get_mask *********/

int ps_get_mask(struct window *linfo, 
                unsigned int *red_mask, 
                unsigned int *green_mask, 
                unsigned int *blue_mask  )
{
   *red_mask = 0xff0000;
   *green_mask = 0xff00;
   *blue_mask = 0xff;
   return 1;
}


/********** driver definition *********/


struct gdriver ps_driver = {
   "PS",
   /* release 1 */
   ps_begin,
   ps_end,
   ps_close,
   ps_xsize,
   ps_ysize,
   ps_setfont,
   ps_clear,
   ps_draw_line,
   ps_draw_rect,
   ps_draw_circle,
   ps_fill_rect,
   ps_fill_circle,
   ps_draw_text,
   /* release 2 */
   ps_setcolor,
   ps_alloccolor,
   ps_fill_polygon,
   NIL,
   ps_gspecial,
   ps_clip,
   NIL,
   ps_pixel_map,
   ps_hinton_map,
   /* release 2 */
   ps_draw_arc,
   ps_fill_arc,
   /* import from sn3.2 */
   NIL,
   ps_get_mask,
   ps_set_linestyle,
};


static at *ps_window(int x, int y, int w, int h, const char *name)
{
   struct M_window *info = ps_new_window(x,y,w,h,name);
   at *ans = new_at(window_class, info );
   info->lwin.used = 1;
   info->lwin.font = make_string(FONT_STD);
   info->lwin.color = COLOR_FG;
   info->lwin.gdriver = &ps_driver;
   info->lwin.clipw = 0;
   info->lwin.cliph = 0;
   info->lwin.linestyle = 0;
   info->lwin.backptr = ans;
   return ans;
}

DX(xps_window)
{
   const char *name = mm_strdup("lush_output.ps");
   int w = 512;
   int h = 512;
   
   switch (arg_number) {
   case 1:
      if (APOINTER(1))
         name = ASTRING(1);
   case 0:
      break;
   case 3:
      if (APOINTER(3))
         name = ASTRING(3);
   case 2:
      w = AINTEGER(1);
      h = AINTEGER(2);
      break;
   case 5:
      if (APOINTER(5))
         name = ASTRING(5);
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
   return ps_window(0,0,w,h,name);
}





/* ============================  INITIALISATION */


void init_ps_driver(void)
{
   dx_define("ps-window",xps_window);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
