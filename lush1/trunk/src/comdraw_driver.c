/***********************************************************************
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

/* COMDRAW Driver for LUSH. */
/* Secil Ugurel, Aug 2002   */


#include "header.h"
#include "graphics.h"
#include <limits.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

/**************************************************************/

#if defined(HAVE_TERMIOS_H) && defined(HAVE_OPENPTY)
# define WITH_COMDRAW_DRIVER 1
#else
# define WITH_COMDRAW_DRIVER 0
#endif

#if WITH_COMDRAW_DRIVER

/**************************************************************/

#define DEBUG  0
#define MAXWIN 16
#define MAX_FNAME_LEN   150
#define MAX_OUTPUT_SIZE 255
#define MAX_INPUT_SIZE  255
#define NO_OF_DIGITS_INT  ((int)floor(log10(INT_MAX)+1))
#define NO_OF_DIGITS_UINT ((int)floor(log10(UINT_MAX)+1))
#define PI 3.14159

#define MAX(A,B) ((A)>(B) ? (A) : (B))
#define DEGTORAD(X) ((X)*PI/180)

/* The degree to compute the number of points to draw a 
   polygon so that the error between  a circle of R  
   and the polygon is minimized (1 pixel) */
#define OPTRAD(R) (sqrt(2.0/(double)MAX(4,(R))))  

/* for the do_line_clip function */
#define CODE_BOTTOM 0x0001
#define CODE_TOP    0x0010
#define CODE_LEFT   0x0100
#define CODE_RIGHT  0x1000

typedef struct Stack_node {
  int compview;
  struct Stack_node *link;
} stack_node, *stack_ptr;

static struct C_window {
  struct window lwin;
  char *filename;
  int  saveflag;
  FILE *finptr;
  FILE *foutptr;
  int width;  
  int height;
  stack_ptr group_stack; /*for grouping*/
  int compview_no; 
  int group_cnt;
  int command_type;
} wind[MAXWIN];



/* ======================  STACK FUNCTIONS ===================== */

static stack_ptr 
init_stack(void)
{
  stack_ptr node = malloc(sizeof(stack_node));
  node->link = NIL;
  return node;
}
 
static int 
push(stack_ptr stack, int cw) 
{
  stack_node *node = malloc(sizeof(stack_node));   
  if(node != NIL) {   
    node->compview = cw;     
    node->link = stack->link;  
    stack->link = node;
#if DEBUG
    printf("%d is pushed...\n", cw);
#endif
    return 0;     
  } else {
    fprintf(stdout,"Out of memory, compview stack is full\n"); 
    return -1;   
  }
}

static int 
pop(stack_ptr stack) 
{  
  stack_node *node;
  int cw;     
  node = stack->link;   
  stack->link = node->link;  
  cw = node->compview;
#if DEBUG
  printf("%d is poped...\n", cw);
#endif
  return cw;
}

static int 
read_top(stack_ptr stack) 
{  
  stack_node *node;
  int cw;            
  node = stack->link;
  cw = node->compview;                      
  return cw;
}

/* ==================  WINDOW CREATION ===================== */

static struct C_window *
comdraw_new_window(int x, int y, int w, int h, char* name)
{
  struct C_window *info;
  int i;
  char cmd[255];
  struct termios settings, default_settings;
  FILE *fin,*fout;
  
  if (w==0 && h==0)
    sprintf(cmd,"comdraw -geometry 512x512+%d+%d",x,y);
  else
    sprintf(cmd,"comdraw -geometry %dx%d+%d+%d",w, h,x,y);
  info = NULL;
  for(i=0; i<MAXWIN; i++)
    if (! wind[i].lwin.used ) 
    {
      info = &wind[i];
      break;
    }
  if (! info)
    error(NIL,"too many comdraw windows",NIL);
  info->filename = malloc(strlen(name)+1);
  strcpy(info->filename,name);
  info->saveflag = 0;
  info->finptr = NIL;
  info->foutptr = NIL;
  info->width = 512;
  info->height = 512;
  info->group_stack = init_stack(); 
  info->compview_no = 10000;
  info->group_cnt = 10000;
  info->command_type = 0;
  filteropenpty(cmd,&info->finptr,&info->foutptr);
  fin = info->finptr;
  fout = info->foutptr;
  if (fin && fout)
    { 
      tcgetattr(fileno(fout),&default_settings);
      settings = default_settings;
      settings.c_lflag &= (~ECHO);
      tcsetattr(fileno(fout),TCSANOW,&settings);
    }
  return info;  
}

/* ======================================================== */

static void 
delete_comdraw_win(struct C_window *info)
{
  info->lwin.backptr->Object = NIL;
  info->lwin.backptr->Class = &zombie_class;
  info->lwin.backptr->flags = C_EXTERN | X_ZOMBIE;
  free(info->filename);
  pclose(info->foutptr);
  fclose(info->finptr);
  free(info->group_stack);
  info->lwin.used = 0;
}

/* ============ PSEUDO_TERMINAL I/O functions ============= */

static int 
readnbytes(int fd, char* rbuffer, int size, long maxwait)
{
  struct timeval tv;
  struct timeval *ptv = &tv;
  int readreturn, selectreturn;
  char buffer[MAX_OUTPUT_SIZE+1];
  fd_set readset;
  
  buffer[0] = 0;
  FD_ZERO(&readset);
  FD_SET(fd,&readset);
  tv.tv_sec = 0;
  tv.tv_usec = maxwait;
  if (maxwait < 0)
    ptv = 0; 
  if ((selectreturn = select(fd+1, &readset, NIL, NIL, ptv))>0)
    if ((readreturn = read(fd, buffer, size))>=0)
      {
        buffer[readreturn] = 0;
        strcpy(rbuffer,buffer);      
        return readreturn;
      }
    else
      {
        return readreturn;
      }
  else if (selectreturn<0)
    perror("select(2)");
  return selectreturn;
}

static int 
writenbytes(int fd, char* command, int size, long maxwait)
{
  struct timeval tv;
  struct timeval *ptv = &tv;
  int selectreturn, writereturn;
  fd_set writeset;
  
  FD_ZERO(&writeset);
  FD_SET(fd,&writeset);
  tv.tv_sec = 0;
  tv.tv_usec = maxwait;
  if (maxwait < 0)
    ptv = 0;
  if ((selectreturn = select(fd+2, NIL, &writeset, NIL, ptv))>0)
    if((writereturn = write(fd,command,size)) >= 0)
      return writereturn;
    else
      {
        return writereturn;
      }
  else if(selectreturn == 0)
    fprintf(stdout,"Select: Write time out!\n" );
  else
    perror("select(2)");
  return selectreturn;
}

/* Run command */
static int 
runcommand(struct C_window* info, char* command, int noflines_toread)
{
  int fdout;
  int fdin;
  char buffer[MAX_OUTPUT_SIZE];
  int nofbytes = 0;
  int i,j;
  
  buffer[0] = 0;
  fdout = fileno(info->foutptr);
  fdin = fileno(info->finptr);

  /* read (for garbage) */
  nofbytes = readnbytes(fdout, buffer, MAX_OUTPUT_SIZE-1,0); 
  while (nofbytes>0) 
    nofbytes = readnbytes(fdout, buffer,MAX_OUTPUT_SIZE-1 ,0);      
#if DEBUG
  fprintf(stdout,"%d bytes are read...%s\n",nofbytes,buffer);
#endif
  if (nofbytes < 0)
    return -1;
  /* write */
  nofbytes = writenbytes(fdin,command,strlen(command),-1);
  fflush(info->finptr);
#if DEBUG
  fprintf(stdout,"%d bytes are written...%s\n",nofbytes,command);
#endif
  if (nofbytes < 0)
    return -1;
  if (nofbytes<strlen(command))
    fprintf(stdout,"Command is not written properly\n");
  nofbytes = 0; 
  i=0;
  /* read */
  while(i < noflines_toread)
    {
      buffer[0] = 0;
      nofbytes += readnbytes(fdout, buffer, MAX_OUTPUT_SIZE-1,-1);      
      for(j=0;j<nofbytes;j++)
        {
          if (buffer[j] == 0)
            j=nofbytes;
          else if(buffer[j]=='\n')
            i++;
        }  
    }   
#if DEBUG
  fprintf(stdout,"%d of %d  lines,%d bytes are read...\n",
          i,noflines_toread,nofbytes);
#endif
  return 0;
}

/* Run command and read its output */
static int 
runcommandReadout(struct C_window* info, char* command, 
                  char* rbuffer, int noflines_toread)
{
  int fdout;
  int fdin;
  char buffer[MAX_OUTPUT_SIZE];
  int nofbytes=0;
  int i,j,lastindex;

  fdout=fileno(info->foutptr);
  fdin=fileno(info->finptr);
  buffer[0] = 0;
  /* read */
  nofbytes = readnbytes(fdout, buffer,MAX_OUTPUT_SIZE-1 ,0);      
  while(nofbytes!=0)
    nofbytes = readnbytes(fdout, buffer, MAX_OUTPUT_SIZE-1,0);      
#if DEBUG
  fprintf(stdout,"%d bytes are read...%s\n",nofbytes,buffer);
#endif
  if (nofbytes < 0)
    return -1;
  /* write */
  nofbytes = writenbytes(fdin,command,strlen(command),-1);
  fflush(info->finptr);
#if DEBUG  
  fprintf(stdout,"%d bytes are written...%s\n",nofbytes,command);
#endif
  if (nofbytes < 0)
    return -1;
  if (nofbytes < strlen(command))
    fprintf(stdout,"Command is not written properly\n");
  buffer[0] = 0;
  nofbytes = 0; 
  i=lastindex=0;
  /* read */
  while(i < noflines_toread)
    {
      nofbytes += readnbytes(fdout, &buffer[strlen(buffer)], 
                             MAX_OUTPUT_SIZE-strlen(buffer)-1,-1);      
      for(j=lastindex;j<nofbytes;j++)
        {
          if (buffer[j] == 0 )
            j=nofbytes;
          else if(buffer[j] == '\n')
            i++;     
        }
      lastindex=nofbytes;
    }  
  strcpy(rbuffer,buffer);
#if DEBUG
  fprintf(stdout,"%d of %d  lines,%d bytes are read...%s\n",
          i,noflines_toread,nofbytes,rbuffer);
#endif
  return 0;
}


/* ======================================================== */

/* computes the code for each point for the do_line_clip function. */
static short 
compute_code (int x, int y, short clipx1, short clipy1, 
              short clipx2, short clipy2)
{
  short code = 0x0;
  if (y > (clipy2)) 
    code = CODE_BOTTOM;
  else if (y < clipy1)
    code = CODE_TOP;
  if (x > (clipx2))
    code |= CODE_RIGHT;
  else if (x < clipx1)
    code |= CODE_LEFT;
  return code;
}

/* ======================================================= */
/* Clips a line to the current clip rectangle. If the line
   lies completely outside of the clip boundary, 0 is returned.
   Uses  Cohen-Sutherland line clipping algorithm. */

static int 
do_line_clip(int *x1, int *y1, int *x2, int *y2, struct C_window *info)
{
  short code1;
  short code2;
  short code_out;
  int x=0, y=0;
  short clipx1 = info->lwin.clipx;
  short clipy1 = info->lwin.clipy;
  short clipw = info->lwin.clipw;
  short cliph = info->lwin.cliph;  
  short clipx2 = clipx1+clipw-1;
  short clipy2 = clipy1+cliph-1;

  code1 = compute_code(*x1, *y1, clipx1, clipy1, clipx2, clipy2);
  code2 = compute_code(*x2, *y2, clipx1, clipy1, clipx2, clipy2);
  if (*x1==*y1 && *y1==*x2 && *x2==*y2)
    return 0;
  if (clipw==0 && cliph==0)
    return 1;
  while (code1 || code2){
    /* line is out of the clip rectangle */
    if (code1 & code2)
      return 0;
    /* needs clipping */
    else {
      if (code1 > 0) 
	/* clip the first point */
	code_out = code1;
      else
	/* clip the second point */
	code_out = code2;
      if ((code_out & CODE_BOTTOM) == CODE_BOTTOM) {
	/* clip the line to the bottom of the clip rectangle */
	y = clipy2;
	x = *x1+(*x2-*x1)*(y-*y1)/(*y2-*y1);
      }
      else if ((code_out & CODE_TOP) == CODE_TOP) {
	/* clip the line to the top of the clip rectangle */
	y = clipy1;
	x = *x1+(*x2-*x1)*(y-*y1)/(*y2-*y1);
      }
      else if ((code_out & CODE_RIGHT) == CODE_RIGHT) {
	/* clip the line to the right of the clip rectangle */
	x = clipx2;
	y = *y1+(*y2-*y1)*(x-*x1)/(*x2-*x1);
      }
      else if ((code_out & CODE_LEFT) == CODE_LEFT) {
	/* clip the line to the left of the clip rectangle */
	x = clipx1;
	y = *y1+(*y2-*y1)*(x-*x1)/(*x2-*x1);
      }
      if (code_out == code1) {
	*x1 = x;
	*y1 = y;
	code1 = compute_code(*x1,*y1, clipx1, clipy1, clipx2, clipy2);
      }
      else {
	*x2 = x;
	*y2 = y;
	code2 = compute_code(*x2,*y2, clipx1, clipy1, clipx2, clipy2);
      }
    }
  }
  return 1;
}

/* ======================  BASIC FUNCTIONS ==================== */

static void 
comdraw_begin(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  char buffer[MAX_OUTPUT_SIZE];
  stack_ptr S = info->group_stack;

#if DEBUG
  fprintf(stdout,"begin...\n");      
#endif
  if (readnbytes(fileno(info->foutptr), buffer, MAX_OUTPUT_SIZE-1,0) < 0)
    {
      delete_comdraw_win(info);
      return;
    } 
  push(S,0);
}

/* ============================================================ */
static void 
begin(struct C_window *info)
{
  char command[MAX_INPUT_SIZE];
  command[0] = 0;
  if (!info->saveflag) {
    sprintf(command,"save(\"%s\")\n",info->filename);
    runcommand(info,command,1);
    info->saveflag = 1;
  }
  info->compview_no++;
  info->command_type = 1;
}

/* ============================================================ */
static void 
begin_no_cw(struct C_window *info)
{
  info->command_type = 2;
}
/****************************************************************/

static void 
comdraw_end(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  int group1 = 0;
  int group2 = 0;
  int image =  info->compview_no;
  char command[MAX_INPUT_SIZE];
  stack_ptr S = info->group_stack;
  int linecnt = 0;

  command[0] = 0;
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"end...\n");
#endif
  group1 = pop(S);
  if (!(S->link)) 
    return;
  if (group1 == 0){
    group2 = read_top(S);
    if (group2 == 0) 
      {
	if (info->command_type == 1){
	  pop(S);
	  info->group_cnt++;
	  push(S,info->group_cnt);
	  sprintf(command,"lushng%d=growgroup(c%d c%d)\n",
                  info->group_cnt, image, image);    
	  linecnt++;
	}
      }
    else 
      {
	sprintf(command,"lushng%d=growgroup(lushng%d c%d)\n",
                info->group_cnt, group2, image);    
	linecnt++;
      }
  }
  else 
    {
      group2 = read_top(S);
      sprintf(command,"lushng%d=growgroup(lushng%d lushng%d)\n",
              group2, group2, group1);    
      linecnt++;
    }
  if (info->command_type != 2){
    sprintf(&command[strlen(command)],"select(:clear)\n");
    runcommand(info,command,2);
  }
}      

/****************************************************************/
/* Close a window */

static void 
comdraw_close(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[5];

  ifn(info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"close...\n");
#endif
  command[0] = 0;
  sprintf(command,"save\n");
  runcommand(info,command,1);
  sprintf(command,"exit\n");
  writenbytes(fileno(info->finptr),command,sizeof command,-1);     
  free(info->filename);
  pclose(info->foutptr);
  fclose(info->finptr);
  free(info->group_stack);
  info->lwin.used = 0;
}

/****************************************************************/
/* Return the size of a window */

static int 
comdraw_xsize(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  char rbuffer[MAX_OUTPUT_SIZE];
  char* command = "ncols\n";
  int xsize = 0;
  char* bufptr;
  char number[NO_OF_DIGITS_INT];
  int i;

  begin_no_cw(info);
  rbuffer[0] = 0;
  if (! info->lwin.used)
    return 0 ;
#if DEBUG
  fprintf(stdout,"xsize...\n");
#endif
  runcommandReadout(info,command,rbuffer,1);
  bufptr = rbuffer;
  number[0] = 0;
  i = 0;
  while (*bufptr)
    {
      if (isdigit(*bufptr)) 
        {
          number[i] = (*bufptr);
          i++;
        }
      else if (i>0)
        xsize = atoi(number);    
      bufptr++;
    }
  info->width = xsize;
  return (xsize);
}

/****************************************************************/

static int 
comdraw_ysize(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  char rbuffer[MAX_OUTPUT_SIZE];
  char* command = "nrows\n";
  int ysize = 0;
  char* bufptr;
  char number[NO_OF_DIGITS_INT];
  int i;

 
  rbuffer[0] = 0;
  if (! info->lwin.used)
    return 0 ;
#if DEBUG
  fprintf(stdout,"ysize...\n");
#endif
  begin_no_cw(info);
  runcommandReadout(info,command,rbuffer,1);
  bufptr = rbuffer;
  number[0] = 0;
  i = 0;
  while (*bufptr)
    {
      if (isdigit(*bufptr))
        {
          number[i] = (*bufptr);
          i++;
        }
      else if (i>0)
        ysize = atoi(number);    
      bufptr++;
    }
  info->width = ysize;
  return (ysize);
}

/****************************************************************/
/* set the font in a window */

static char *
comdraw_setfont(struct window *linfo, char *f)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[MAX_INPUT_SIZE]; 
  char font[15];
  char def_font[] = "default";
  if (! info->lwin.used)
    return 0;
#if DEBUG
  fprintf(stdout,"setfont...\n");
#endif
  command[0] = 0;
  begin_no_cw(info);
  strcpy(font,f);
  ifn (strcmp(font,def_font))
    strcpy(font,"Helvetica-11"); 
  sprintf(command,"fontbyname(\"%s\")\n",font);
  runcommand(info, command,1);
  return f;
}

/****************************************************************/

static void 
comdraw_clear(struct window *linfo)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[MAX_INPUT_SIZE];
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"clear...\n");
#endif
  command[0] = 0;
  begin_no_cw(info);
  sprintf(command,
	  "lushtmp=select(:all)\n"
          "for(i=0 i<size(lushtmp) i++ delete(at(lushtmp i)))\n");
  runcommand(info,command,2);
  info->compview_no = 10000;
}

/* ======================  Drawing Functions =================== */

static void 
comdraw_draw_line(struct window *linfo, 
                  int x1, int y1,int x2, int y2)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[43+5*(NO_OF_DIGITS_INT)];
  
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"draw_line...\n");
#endif
  if (do_line_clip(&x1, &y1, &x2, &y2, info)) {
    begin(info);
    command[0] = 0;
    sprintf(command,"c%d=line(%d,nrows-(%d),%d,nrows-(%d))\n"
            "select(:clear)\n",
	    info->compview_no,x1, (y1+1), x2, (y2+1));
    runcommand(info, command,2);
  }
  else
    begin_no_cw(info);
}
/****************************************************************/
 
static void 
comdraw_draw_rect(struct window *linfo, 
                  int x1, int y1,unsigned int w, unsigned int h)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[47+5*(NO_OF_DIGITS_INT)];

  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"draw_rect...\n");
#endif
  begin(info);
  command[0] = 0;
  sprintf(command,"c%d=rect(%d,nrows-(%d),%d,nrows-(%d))\n"
          "select(:clear)\n",
	  info->compview_no,x1, (y1+1), x1+w-1, (y1+h));
  runcommand(info,command,2);
}
/****************************************************************/

static void 
comdraw_draw_circle(struct window *linfo, int x1,int y1,unsigned int r)
{
  struct C_window *info = (struct C_window*)linfo; 
  char command[38+5*(NO_OF_DIGITS_INT)];

  if (! info->lwin.used)
    return;    
#if DEBUG
  fprintf(stdout,"draw_circle...\n");
#endif
  begin(info);
  command[0] = 0;
  sprintf(command,"c%d=ellipse(%d,nrows-(%d),%d,%d)\nselect(:clear)\n",
	  info->compview_no,x1,(y1+1),r,r);
  runcommand(info,command,2);
}
/****************************************************************/

static void 
comdraw_fill_rect(struct window *linfo, 
                  int x1, int y1,unsigned int w, unsigned int h)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[65+5*(NO_OF_DIGITS_INT)];

  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"fill_rect...\n");
#endif
  begin(info); 
  command[0] = 0;
  sprintf(command,
	  "pattern(2)\n"
	  "c%d=rect(%d,nrows-(%d),%d,nrows-(%d))\n"
          "select(:clear)\npattern(1)\n",
	  info->compview_no,x1, (y1+1), x1+w-1, (y1+h));
  runcommand(info,command,4); 
}
/****************************************************************/

static void 
comdraw_fill_circle(struct window *linfo,int x1,int y1,unsigned int r)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[60+5*(NO_OF_DIGITS_INT)];
 
  if (! info->lwin.used)
    return; 
#if DEBUG
  fprintf(stdout,"fill_circle...\n");
#endif
  begin(info);
  command[0] = 0;
  sprintf(command,
	  "pattern(2)\n"
          "c%d=ellipse(%d,nrows-(%d),%d,%d)\n"
          "select(:clear)\npattern(1)\n",
          info->compview_no,x1, y1+1, r, r);
  runcommand(info,command,4);
}

/****************************************************************/

static void 
comdraw_draw_text(struct window *linfo, int x, int y, char *s)
{
  struct C_window *info = (struct C_window*)linfo;
  char command [MAX_INPUT_SIZE];
  char *t;
  
  if (! info->lwin.used)
    return;
  if (! s || !*s){
    begin_no_cw(info);
    return;
  }
#if DEBUG
  fprintf(stdout,"draw_text...\n");
#endif
 
  command[0] = 0;
  t = strdup(s);
  if (*t)
    begin(info);

  if (*t=='"' || *t=='\\')
    sprintf(command,"lushtmp=\"\\%c",*t);
  else
    sprintf(command,"lushtmp=\"%c",*t);
  t++;
  while (*t) {
    if (strlen(command) >= MAX_INPUT_SIZE-4) {
      sprintf(&command[strlen(command)],"\"\n");
      runcommand(info, command,1); 
      command[0] = 0;
      if (*t=='"' || *t=='\\')
	sprintf(command,"lushtmp=lushtmp+\"\\%c",*t);
      else
	sprintf(command,"lushtmp=lushtmp+\"%c",*t);
    }
    else
      if (*t=='"' || *t=='\\')
	sprintf(&command[strlen(command)],"\\%c",*t);
      else
	sprintf(&command[strlen(command)],"%c",*t);	
    ifn (*(t+1)){
      sprintf(&command[strlen(command)],"\"\n");
      runcommand(info, command,1); 
    }
    t++;
  }
  if (*s){
    command[0] = 0;
    sprintf(command,"c%d=text(%d,nrows-(%d) lushtmp)\nselect(:clear)\n",
	    info->compview_no,x,(y+1));
    runcommand(info, command,2);  
  }
}
/****************************************************************/

static char *hexmap = "0123456789ABCDEF";

static void 
comdraw_setcolor(struct window *linfo,int x)
{
  struct C_window *info = (struct C_window*)linfo;
  int rr,bb,gg;
  char hexcolor[7];
  char command[23];
  
  command[0] = 0;
  if (! info->lwin.used)
    return;  
#if DEBUG
  fprintf(stdout,"set_color...\n");
#endif
  begin_no_cw(info);
  switch(x)
    {
    case COLOR_FG:
      sprintf(hexcolor,"#000000");
      break;
    case COLOR_BG:
      sprintf(hexcolor,"#FFFFFF");
      break;
    case COLOR_GRAY:
      sprintf(hexcolor,"#808080");
      break;
    default:
      rr = (int)RED_256(x);
      gg = (int)GREEN_256(x);
      bb = (int)BLUE_256(x);
      sprintf(hexcolor,"#%c%c%c%c%c%c",
	      hexmap[ (rr&0xf0)>>4 ],
	      hexmap[ rr&0xf ],
	      hexmap[ (gg&0xf0)>>4 ],
	      hexmap[ gg&0xf ],
	      hexmap[ (bb&0xf0)>>4],
	      hexmap[ bb&0xf ]);
      break;
    }
  sprintf(command,"colorsrgb(\"%s\")\n",hexcolor);
  runcommand(info,command,1);
}
/****************************************************************/

static int 
comdraw_alloccolor(struct window *linfo,double r,double g,double b)
{
  struct C_window *info = (struct C_window*)linfo;

#if DEBUG
  fprintf(stdout,"alloccolor...\n");
#endif
  begin_no_cw(info);
  return COLOR_RGB(r,g,b);
}

/****************************************************************/

static void 
comdraw_fill_polygon(struct window *linfo,short (*points)[2], unsigned int n)
{
  struct C_window *info = (struct C_window*)linfo;
  int i;
  char command[MAX_INPUT_SIZE];
  
  command[0] = 0;
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"fill_polygon...\n"); 
#endif
  begin(info);

  sprintf(command,"lushtmp=%d,nrows-(%d)",
          points[0][0],(points[0][1]+1));
  for (i=1; i<n; i++){
    if (strlen(command)>=MAX_INPUT_SIZE-(11+2*NO_OF_DIGITS_INT)){
      sprintf(&command[strlen(command)],"\n");
      runcommand(info, command,1); 
      command[0] = 0;
      sprintf(command,"lushtmp=lushtmp,%d,nrows-(%d)",
              points[i][0],(points[i][1]+1));
    }
    else
      sprintf(&command[strlen(command)],",%d,nrows-(%d)",
	      points[i][0],(points[i][1]+1));
    
  }
  sprintf(&command[strlen(command)],"\n");
  runcommand(info,command,1); 
  
  command[0] = 0;
  sprintf(command,"c%d=polygon(lushtmp)\npattern(2)\n"
          "select(:clear)\npattern(1)\n",
	  info->compview_no);
  runcommand(info,command,2); 
} 

/****************************************************************/

static void comdraw_rect_text(struct window *linfo, 
                              int x, int y, char *s, 
                              int *xptr, int *yptr, int *wptr, int *hptr)
{
  struct C_window *info = (struct C_window*)linfo;
  char rbuffer[MAX_OUTPUT_SIZE];
  char command[MAX_INPUT_SIZE];
  char* bufptr;
  char number[NO_OF_DIGITS_INT+1];
  short int i,j;
  int coord[4];
  int ysize;
  char *t;
  
 *xptr = *yptr = *wptr = *hptr = 0;
 begin_no_cw(info);
  if (! s || !*s)
    {
      *xptr = x;
      *yptr = y;
      return;
    }
  
  rbuffer[0] = 0;
  command[0] = 0;
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"rect_text...\n");
#endif
  t = strdup(s);
  ysize = comdraw_ysize(linfo);

  if (*t=='"' || *t=='\\')
    sprintf(command,"lushtmp=\"\\%c",*t);
  else
    sprintf(command,"lushtmp=\"%c",*t);
  t++;
  while (*t) {
    if (strlen(command)>=MAX_INPUT_SIZE-4){
      sprintf(&command[strlen(command)],"\"\n");
      runcommand(info, command,1); 
      command[0] = 0;
      if (*t=='"' || *t=='\\')
	sprintf(command,"lushtmp=lushtmp+\"\\%c",*t);
      else
	sprintf(command,"lushtmp=lushtmp+\"%c",*t);
    }
    else
      if (*t=='"' || *t=='\\')
	sprintf(&command[strlen(command)],"\\%c",*t);
      else
	sprintf(&command[strlen(command)],"%c",*t);	
    t++;
  }
  sprintf(&command[strlen(command)],"\"\n");
  runcommand(info, command,1); 
  
  command[0] = 0;
  sprintf(command,"lushtmp2=text(%d,nrows-(%d) lushtmp)\n"
          "mbr(lushtmp2 :scrn)\ndelete(lushtmp2)\n",x,(y+1));
  runcommandReadout(info,command,rbuffer,3);
  bufptr = rbuffer;
  number[0] = 0;
  i = j = 0;
  while (*bufptr){
    if (isdigit(*bufptr)){
      number[i] = (*bufptr);
      i++;
    }   
    else if (i>0)
      {
	number[i] = 0;
	i = 0;
	coord[j] = atoi(number);
	j++;    
      }
    bufptr++;
  }
  (*xptr) = coord[0];
  (*yptr) = ysize-(coord[3]+1);
  (*wptr) = coord[2]-coord[0]+1;
  (*hptr) = coord[3]-coord[1]+1;
}

/****************************************************************/

static void 
comdraw_gspecial(struct window *linfo,char *s)
{
  struct C_window *info = (struct C_window*)linfo;
  char command[MAX_INPUT_SIZE];
  
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"gspecial...\n");
#endif
  begin_no_cw(info);
  command[0] = 0;
  sprintf(command,"%s\n",s);
  if (strlen(command)>MAX_INPUT_SIZE)
    fprintf(stdout,"String is too long!\n");
  else
    runcommand(info, command,1);
}

/****************************************************************/

static void 
comdraw_clip(struct window *linfo, int x, int y, 
             unsigned int w, unsigned int h)
{
  struct C_window *info = (struct C_window*)linfo;
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"clip...\n");
#endif
  begin_no_cw(info);
  if (!(w == 0 && h == 0)){
    info->lwin.clipx = x;
    info->lwin.clipy = y; 
    info->lwin.clipw = w;
    info->lwin.cliph = h;
  }
}

/****************************************************************/

static int 
comdraw_pixel_map(struct window *linfo,
                  unsigned int  *data, 
                  int x, int y,unsigned int w,unsigned int h,
                  unsigned int sx,unsigned int sy)
{
  struct C_window *info = (struct C_window*)linfo; 
  char command[MAX_INPUT_SIZE];
  register unsigned int *im, *jm, *data2;
  int sx_w = sx * w;
  int sy_h = sy * h;
  int i,j,k,l,hcnt,wcnt;
  int pointcnt;
  char line[MAX_INPUT_SIZE];
  int index = 0;
  int limit = MAX_INPUT_SIZE-22-5*NO_OF_DIGITS_INT;

  command[0] = 0;
  if (! info->lwin.used)
    return 0;
#if DEBUG
  fprintf(stdout,"pixel_map...\n");
#endif
  ifn (data)
    return TRUE;
  line[0] = 0;
  begin(info);
  
  if (sx == 1 && sy == 1) {
    sprintf(command,"c%d=raster(%d,nrows-(%d),%d,nrows-(%d))\n",
	    info->compview_no,x, (y+h),x+w-1 ,y+1);
    runcommand(info, command,1);
    for(hcnt=0; hcnt<h; hcnt++){
      pointcnt = 0;
      line[0] = 0;
      for (wcnt=0; wcnt<w-1; wcnt++){
	index = w*hcnt+wcnt;
	if(strlen(line)<limit){  
	  sprintf(&line[strlen(line)],"%d,",data[index]);
	}
	else{
	  sprintf(&line[strlen(line)],"%d",data[index]);
	  sprintf(command,"pokeline(c%d %d %d %d list(%s))\n", 
		  info->compview_no, pointcnt, (h-1-hcnt),
                  (wcnt-pointcnt+1), line);
	  runcommand(info, command,1);
	  pointcnt = wcnt+1;
	  line[0]= 0;
	}
      }  
      sprintf(&line[strlen(line)],"%d",data[index+1]);
      sprintf(command,"pokeline(c%d %d %d %d list(%s))\n", 
	      info->compview_no, pointcnt, (h-1-hcnt),
              (wcnt-pointcnt+1), line);
      runcommand(info, command,1);
    }
  }
  else{
    ifn (sx>0 && sy>0)
      return FALSE;
    ifn (data2 = malloc(sx_w * sy_h * sizeof(int)))
      return FALSE;
    /* create raster */
    for (i = 0; i < h; i++) {
      im = data2 + sx_w * sy * i;
      for (k = 0; k < w; k++, data++) {
	register int c;
	c = *data;
	jm = im;
	for (j = 0; j < sy; j++, jm += sx_w)
	  for (l = 0; l < sx; l++)
	    jm[l] = c;
	im += sx;
      }
    }
    sprintf(command,"c%d=raster(%d,nrows-(%d),%d,nrows-(%d))\n",
	    info->compview_no,x, (y+sy_h),x+sx_w-1 ,y+1);
    runcommand(info, command,1);
    for(hcnt=0; hcnt<sy_h; hcnt++){
      pointcnt=0;
      line[0] = 0;
      for (wcnt=0; wcnt<sx_w-1; wcnt++){
	index = sx_w*hcnt+wcnt;
	if(strlen(line)<limit){  
	  sprintf(&line[strlen(line)],"%d,",data2[index]);
	}
	else{
	  sprintf(&line[strlen(line)],"%d",data2[index]);
	  sprintf(command,"pokeline(c%d %d %d %d list(%s))\n", 
		  info->compview_no, pointcnt,
                  (h-1-hcnt), (wcnt-pointcnt+1), line);
	  runcommand(info, command,1);
	  pointcnt = wcnt+1;
	  line[0] = 0;
	}
      }
      sprintf(&line[strlen(line)],"%d",data2[index+1]);
      sprintf(command,"pokeline(c%d %d %d %d list(%s))\n", 
	      info->compview_no, pointcnt, (sy_h-1-hcnt),
              (wcnt-pointcnt+1), line);
      runcommand(info, command,1); 
    }
  }
  return 0;
}

/****************************************************************/
/* to>=from, to and from are  integers between -360 and 360 */

static void comdraw_draw_arc(struct window *linfo, 
			     int x1, int y1,unsigned int r, int from, int to)
{
  struct C_window *info = (struct C_window*)linfo; 
  char command[MAX_INPUT_SIZE]; 
  double dif = DEGTORAD((double)to - (double)from);
  double precision = dif/floor((dif+OPTRAD(r))/OPTRAD(r));
  int nofpoints = (int)floor(dif/precision);
  struct Point {
    int x;
    int y;
  } point[nofpoints+1];
  int cnt;
  
  command[0] = 0;
  if (! info->lwin.used)
    return; 
#if DEBUG
  fprintf(stdout,"draw_arc...\n");
#endif
  
  begin(info);
  for(cnt=0;cnt<nofpoints;cnt++){
    point[cnt].x = (int)floor(x1+r*cos(DEGTORAD(from)+cnt*precision)+0.5);
    point[cnt].y = (int)floor(y1-r*sin(DEGTORAD(from)+cnt*precision)+0.5);
  }
  point[nofpoints].x = (int)floor(x1+r*cos(DEGTORAD(to))+0.5);
  point[nofpoints].y = (int)floor(y1-r*sin(DEGTORAD(to))+0.5);
  sprintf(command,"lushtmp=%d,nrows-(%d)",point[0].x,(point[0].y+1));
  for (cnt=1; cnt<nofpoints+1; cnt++){
    if (strlen(command)>=MAX_INPUT_SIZE-(11+2*NO_OF_DIGITS_INT)){
      sprintf(&command[strlen(command)],"\n");
      runcommand(info, command,1); 
      command[0] = 0;
      sprintf(command,"lushtmp=lushtmp,%d,nrows-(%d)",
              point[cnt].x,(point[cnt].y+1));
    }
    else{
      sprintf(&command[strlen(command)],",%d,nrows-(%d)",
              point[cnt].x,(point[cnt].y+1));
    } 
  }  
  sprintf(&command[strlen(command)],"\n");
  runcommand(info, command,1);
  command[0] = 0;
  sprintf(command,"c%d=multiline(lushtmp)\nselect(:clear)\n",
          info->compview_no);
  runcommand(info, command,2);    

}
/****************************************************************/

static void 
comdraw_fill_arc(struct window *linfo, 
                 int x1, int y1,unsigned int r, int from, int to)
{
  struct C_window *info = (struct C_window*)linfo; 
  char command[MAX_INPUT_SIZE]; 
  double dif = DEGTORAD((double)to - (double) from);
  double precision = dif/floor((dif+OPTRAD(r))/OPTRAD(r));
  int nofpoints = (int)floor(dif/precision);
  struct Point {
    int x;
    int y;
  } point[nofpoints+1];
  int cnt;

  command[0] = 0;
  if (! info->lwin.used)
    return;
#if DEBUG
  fprintf(stdout,"fill_arc...\n");
#endif
  begin(info);
  for(cnt=0;cnt<nofpoints;cnt++){
    point[cnt].x = (int)floor(x1+r*cos(DEGTORAD(from)+cnt*precision+0.5));
    point[cnt].y = (int)floor(y1-r*sin(DEGTORAD(from)+cnt*precision+0.5));
  }
  point[nofpoints].x = (int)floor(x1+r*cos(DEGTORAD(to))+0.5);
  point[nofpoints].y = (int)floor(y1-r*sin(DEGTORAD(to))+0.5);
  sprintf(command,"lushtmp=%d,nrows-(%d)",x1,(y1+1));
  for (cnt=0; cnt<nofpoints+1; cnt++){
    if (strlen(command)>=MAX_INPUT_SIZE-(11+2*NO_OF_DIGITS_INT)){
      sprintf(&command[strlen(command)],"\n");
      runcommand(info, command,1); 
      command[0] = 0;
      sprintf(command,"lushtmp=lushtmp,%d,nrows-(%d)",
              point[cnt].x,(point[cnt].y+1));
    }
    else
      sprintf(&command[strlen(command)],",%d,nrows-(%d)",
	      point[cnt].x,(point[cnt].y+1));
  }
  sprintf(&command[strlen(command)],"\n");
  runcommand(info, command,1); 
  command[0] = 0;
  sprintf(command,"c%d=polygon(lushtmp)\npattern(2)\n"
          "select(:clear)\npattern(1)\n",
	  info->compview_no);
  runcommand(info, command,2);   
}

/****************************************************************/

int 
comdraw_get_mask(struct window *linfo, 
                 unsigned int *red_mask, 
                 unsigned int *green_mask, 
                 unsigned int *blue_mask)
     
{
  struct C_window *info = (struct C_window*)linfo;
#if DEBUG
  fprintf(stdout,"get_mask...\n");
#endif
  begin_no_cw(info);
  *red_mask = 0xff0000;
  *green_mask = 0xff00;
  *blue_mask = 0xff;
  return 1;
}

/* ======================  end of  functions =================== */

struct gdriver comdraw_driver = {
  "Comdraw",
  comdraw_begin,
  comdraw_end,
  comdraw_close,
  comdraw_xsize, 
  comdraw_ysize, 
  comdraw_setfont, 
  comdraw_clear,
  comdraw_draw_line,
  comdraw_draw_rect,
  comdraw_draw_circle,
  comdraw_fill_rect,
  comdraw_fill_circle,
  comdraw_draw_text,
  comdraw_setcolor,
  comdraw_alloccolor,
  comdraw_fill_polygon,
  comdraw_rect_text,
  comdraw_gspecial,
  comdraw_clip,          /* just line clipping */
  NIL, /* hilite */
  comdraw_pixel_map,
  NIL, /* hinton-map */
  comdraw_draw_arc,
  comdraw_fill_arc,
  NIL, /* get_image */
  comdraw_get_mask,
};
/****************************************************************/

static at *
comdraw_window(int x, int y, unsigned int w, unsigned int h, char*name)
{
  struct C_window *info;
  at *ans;
  info = comdraw_new_window(x,y,w,h,name);
  ans = new_extern( &window_class, info);
  info->lwin.used = 1;
  info->lwin.font = new_safe_string(FONT_STD);
  info->lwin.color = COLOR_FG;
  info->lwin.gdriver = &comdraw_driver;
  info->lwin.clipw = 0;
  info->lwin.cliph = 0;
  info->lwin.linestyle = 0;
  info->lwin.backptr = ans;
  return ans;
}

/****************************************************************/

DX(xcomdraw_window)
{
  char *name = "comdraw.output";
  at* ans;
  ALL_ARGS_EVAL;
  switch (arg_number)
    {
    case 0:
      ans = comdraw_window(0, 0, 0, 0, name);
      break;
    case 1:
      ans = comdraw_window(0, 0, 0, 0, ASTRING(1));
      break;
    case 2:
      ans = comdraw_window(0, 0, AINTEGER(1), AINTEGER(2),name);
      break;
    case 3:
      ans = comdraw_window(0, 0, AINTEGER(1), AINTEGER(2), ASTRING(3));
      break;   
    case 4:
      ans = comdraw_window(AINTEGER(1), AINTEGER(2), AINTEGER(3), 
                           AINTEGER(4),name);
      break;
    case 5:
      ans = comdraw_window(AINTEGER(1), AINTEGER(2), AINTEGER(3), 
                           AINTEGER(4), ASTRING(5));
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;;
    }
  return ans;; 
}


/****************************************************************/

#endif

/* =====================  INITIALISATION ====================== */


void 
init_comdraw_driver(void)
{
#if WITH_COMDRAW_DRIVER
  dx_define("comdraw-window",xcomdraw_window);
#endif
}
