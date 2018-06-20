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

/*************************g**********************************************
 * $Id: unix.c,v 1.68 2015-03-14 16:12:05 leonb Exp $
 **********************************************************************/

/************************************************************************
   machine dependant code

   INIT_MACHINE         general initializations
   CHECK_MACHINE        called by EVAL
   TOPLEVEL_MACHINE     called by TOPLEVEL
   SYS                  shell command execution
   CHDIR                current directory change
   BGROUND              executes a progn in background
   GETENV               get environment variable
   XDMC                 Apollo: execute DM command

*********************************************************************** */


/* Config */
#ifdef HAVE_CONFIG_H
# include "lushconf.h"
#endif

/* UNIX header files */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
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
#ifdef HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(s) ((unsigned)(s)>>8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(s) (((s)&255)==0)
#endif
#ifdef HAVE_STROPTS_H
# include <stropts.h>
#endif
#ifdef HAVE_SYS_STROPTS_H
# include <sys/stropts.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_SYS_TTOLD_H
# include <sys/ttold.h>
#endif
#ifdef HAVE_PTY_H
# include <pty.h>
#endif
#ifdef HAVE_UTIL_H
# include <util.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
# ifdef HAVE_CONFIG_H
#  define HAVE_LUSHCONF_H HAVE_CONFIG_H
#  undef HAVE_CONFIG_H
# endif
# include <readline/readline.h>
# ifdef HAVE_READLINE_HISTORY_H
#  include <readline/history.h>
# endif
# ifdef HAVE_LUSHCONF_H
#  define HAVE_CONFIG_H HAVE_LUSHCONF_H 
# endif
# ifndef RL_READLINE_VERSION
#  define RL_READLINE_VERSION 0  /* non-gnu */
# elif ! defined(HAVE_RL_GETC)
#  undef RL_READLINE_VERSION  
#  define RL_READLINE_VERSION 0  /* non-gnu */
# endif
#endif
#ifdef HAVE_MPI
# ifdef HAVE_MPI_H
#  include <mpi.h>
# endif
#endif

typedef RETSIGTYPE (*SIGHANDLERTYPE)();

/* Lush header files */
#include "header.h"
#include "lushmake.h"



/* ---------------------------------------- */
/* CYGWIN STUFF (untested)                  */
/* ---------------------------------------- */

#ifdef __CYGWIN32__
#include <fcntl.h>
#include <io.h>

void 
cygwin_fmode_text(FILE *f) 
{
  setmode(fileno(f), O_TEXT); 
}
void 
cygwin_fmode_binary(FILE *f) 
{
  setmode(fileno(f), O_BINARY); 
}

#endif



/* ---------------------------------------- */
/* INTERRUPTIONS AND SIGNALS                */
/* ---------------------------------------- */

/* #define POSIXSIGNAL */
/* #define BSDSIGNAL   */
/* #define SYSVSIGNAL  */

#ifndef POSIXSIGNAL
#ifndef BSDSIGNAL
#ifndef SYSVSIGNAL
#ifdef HAVE_SIGACTION
#define POSIXSIGNAL
#else
#ifdef HAVE_SIGVEC
#define BSDSIGNAL
#else
#define SYSVSIGNAL
#endif /* HAVE_SIGVEC */
#endif /* HAVE_SIGACTION */
#endif /* SYSVSIGNAL */
#endif /* BSDSIGNAL */
#endif /* POSIXSIGNAL */



/* break_attempt: set when ctrl-c has been pressed */

static int break_attempt;

/* async_attempt: set when async trigger is pending */

static int async_attempt;




/* goodsignal -- sets signal using POSIX or BSD when available */

void 
goodsignal(int sig, SIGHANDLERTYPE vec)
{
#ifdef POSIXSIGNAL
  struct sigaction act;
  act.sa_handler = vec;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;
#endif
  sigaction(sig, &act, NULL);
#endif /* POSIXSIGNAL */
#ifdef BSDSIGNAL
  struct sigvec act;
  act.sv_handler = vec;
  sv.sv_mask = 0L;
  sv.sv_flags = 0L;
#ifdef SV_BSDSIG
  sv.sv_mask = SV_BSDSIG;
#endif
  sigvec(sig, &act, NULL);
#endif /* BSDSIGNAL */
#ifdef SYSVSIGNAL
  signal(sig, vec);
#endif /* SYSVSIGNAL */
}


/* quit_irq -- signal handler for QUIT signal */

static RETSIGTYPE
quit_irq(void)
{
#ifdef SYSVSIGNAL
  goodsignal(SIGQUIT, quit_irq);
#endif
  error(NIL, "user quit", NIL);
}


/* break_irq -- signal handler for Control-C */

static RETSIGTYPE
break_irq(void)
{
  break_attempt = 1;
#ifdef SYSVSIGNAL
  goodsignal(SIGINT, break_irq);
#endif
}


/* lastchance -- safety code for hopeless situations */

void 
lastchance(const char *s)
{
  static int already = 0;
  at *q;
  /* Test for recursive call */
  if (!already) 
    {
      already = 1;
      /* Signal problem */
      argeval_ptr = eval_ptr = eval_std;
      error_doc.ready_to_an_error = FALSE;
      fprintf(stderr, "\n\007**** GASP: Severe error : %s\n", s);
      q = eval(named("toplevel"));
      if (isatty(0) && EXTERNP(q,&de_class))
	{
	  fprintf(stderr,"**** GASP: Trying to recover\n");
	  fprintf(stderr,"**** GASP: You should save your work immediatly\n\n");
	  /* Sanitize IO */
	  break_attempt = 0; 
          async_attempt = 0;
	  block_async_poll();
	  context->input_file = stdin;
	  context->output_file = stdout;
	  context->input_tab = 0;
	  context->output_tab = 0;
	  /* Go toplevel */
	  error_doc.ready_to_an_error = FALSE;
	  apply(q,NIL);
	}
      else
	{
	  time_t clock;
	  time(&clock);
	  fprintf(stderr,"**** GASP: %s", ctime(&clock));
	}
      abort("gasp handler");
    }
  else
    {
      fprintf(stderr,"**** GASP: recursive error: %s\n", s);
    }
  _exit(100);
}


/* gasp_irq -- signal handler for hopeless situations */

static RETSIGTYPE
gasp_irq(int sig)
{
  char buffer[80];
  sprintf(buffer,"Signal %d has occurred",sig);
  error_doc.ready_to_an_error = FALSE;
  argeval_ptr = eval_ptr = eval_std;
  lastchance(buffer);
}


/* set_irq -- set signal handlers */

static void
set_irq(void)
{
#ifdef SIGHUP
  /* default */
#endif
#ifdef SIGINT
  goodsignal(SIGINT, break_irq);
#endif
#ifdef SIGQUIT
  goodsignal(SIGQUIT, quit_irq);
#endif
#ifdef SIGILL
  goodsignal(SIGILL,  gasp_irq);
#endif
#ifdef SIGTRAP
  goodsignal(SIGTRAP, gasp_irq);
#endif
#ifdef SIGABRT
  /* default */
#endif
#ifdef SIGEMT
  goodsignal(SIGEMT,  gasp_irq);
#endif
#ifdef SIGBUS
  goodsignal(SIGBUS,  gasp_irq);
#endif
#ifdef SIGFPE
  /* set below */
#endif
#ifdef SIGKILL
  /* cannot change */
#endif
#ifdef SIGSEGV
  goodsignal(SIGSEGV, gasp_irq);
#endif
#ifdef SIGSYS
  goodsignal(SIGSYS,  gasp_irq);
#endif
#ifdef SIGPIPE
  goodsignal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGALRM
  goodsignal(SIGALRM,  gasp_irq);
#endif
#ifdef SIGTERM
  /* default */
#endif
#ifdef SIGXCPU
  goodsignal(SIGXCPU, SIG_IGN);
#endif
#ifdef SIGXFSZ
  goodsignal(SIGXFSZ, SIG_IGN);
#endif
#ifdef SIGVTALRM
  /* default */
#endif
#ifdef SIGPROF
  /* default */
#endif
#ifdef SIGLOST
  goodsignal(SIGLOST, gasp_irq);
#endif
#ifdef SIGUSR1
  goodsignal(SIGUSR1,  gasp_irq);
#endif
#ifdef SIGUSR2
  goodsignal(SIGUSR2,  gasp_irq);
#endif
}



/* ------------------------------------- */
/* TRIGGERS                              */
/* ------------------------------------- */     

/* This is ported very directly from TL3 because it works 
   well and was close to implement the new stuff */

#ifdef BROKEN_EVERYTHING
#define BROKEN_FASYNC
#define BROKEN_FIOASYNC
#define BROKEN_SETSIG
#define BROKEN_TIMER
#define BROKEN_ALARM
#endif

#ifdef sun
#ifdef svr4
/* FASYNC exists but it broken on Solaris 2.4 */
#define BROKEN_FASYNC
#endif
#endif


/* trigger_mode -- operating system mode used for managing events */
static enum { 
  MODE_UNKNOWN, 
  MODE_FASYNC, MODE_FIOASYNC, MODE_SETSIG, 
  MODE_ITIMER, MODE_ALARM 
} trigger_mode = MODE_UNKNOWN;

/* trigger_handler -- async event handler */
static void (*trigger_handler)();

/* trigger_signal -- signal used by the trigger system */
static int trigger_signal = -1;

/* block_count -- asynchronous trigger blocking count */
static int block_count = 0;


/* check_unix(s): function to test async events and ctrl-c */
void 
check_unix(char *s)
{
  if (async_attempt && !block_count)
    {
      async_attempt = 0;
      if (trigger_handler)
        trigger_handler();
    }
  if (break_attempt)
    {
      user_break(s);
    }
}


/* trigger_fds -- file descriptor for trigger messages */
#define MAX_TRIGGER_NFDS 32
static int trigger_nfds = 0;
static int trigger_fds[MAX_TRIGGER_NFDS];

/* trigger_irq -- signal handler for trigger */
static RETSIGTYPE
trigger_irq(void)
{
  async_attempt = 1;
  if (trigger_signal < 0)
    return;
  /* reset trigger signal */
#ifdef SYSVSIGNAL
  signal(trigger_signal, (SIGHANDLERTYPE) trigger_irq);
#endif
#ifndef BROKEN_ALARM
  if (trigger_mode == MODE_ALARM)
    alarm(1);
#endif
#ifndef BROKEN_TIMER
  if (trigger_mode == MODE_ITIMER) {
    static struct itimerval delay = {{0,0},{0,500000}};
    setitimer(ITIMER_REAL,&delay,0);
  }
#endif
}

/* unblock_async_trigger -- stops blocking asynchronous trigger */
static void
unblock_async_trigger(void)
{
  if (++block_count == 0)
    {
      if (trigger_signal >= 0 && trigger_nfds >= 0)
	{
#ifdef POSIXSIGNAL
	  sigset_t sset;
#endif
	  trigger_irq();
#ifdef POSIXSIGNAL
	  sigemptyset(&sset);
	  sigaddset(&sset, trigger_signal);
	  sigprocmask(SIG_UNBLOCK,&sset,NULL);
#endif
#ifdef BSDSIGNAL
	  sigsetmask(sigblock(0)&~(sigmask(trigger_signal)));
#endif
	}
    }
}

/* block_async_trigger -- blocks asynchronous trigger */
static void
block_async_trigger(void)
{
  if (--block_count == -1)
    {
      if (trigger_signal >= 0 && trigger_nfds >= 0)
      {
#ifdef SYSVSIGNAL
	signal(trigger_signal,SIG_IGN);
#ifndef BROKEN_ALARM
	if (trigger_mode == MODE_ALARM)
	  alarm(0);
#endif
#ifndef BROKEN_TIMER
	if (trigger_mode == MODE_ITIMER) {
	  static struct itimerval delay = {{0,0},{0,0}};
	  setitimer(ITIMER_REAL,&delay,0);
	}
#endif
#endif /* SYSVSIGNAL */
#ifdef POSIXSIGNAL
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset,trigger_signal);
	sigprocmask(SIG_BLOCK,&sset,NULL);
#endif /* POSIXSIGNAL */
#ifdef BSDSIGNAL
	sigblock(sigmask(trigger_signal));
#endif /* BSDSIGNAL */
      }
    }
}

/* setup_signal_once -- called once to set the trigger signal */
static void
setup_signal_once(void)
{
#ifdef POSIXSIGNAL
  sigset_t sset;
  struct sigaction sact;
  sact.sa_flags = 0L;
  sact.sa_handler = (SIGHANDLERTYPE)trigger_irq;
#ifdef SA_BSDSIG
  sact.sa_flags = SA_BSDSIG;
#endif
#ifdef SA_RESTART
  sact.sa_flags = SA_RESTART;
#endif
  sigemptyset(&sact.sa_mask);
  if (block_count<0)
    { 
      sigemptyset(&sset);
      sigaddset(&sset, trigger_signal);
      sigprocmask(SIG_BLOCK, &sset, NULL);
    }
  if (sigaction(trigger_signal, &sact, NULL) < 0)
    error(NIL,"internal error: sigaction failed",NIL);
#endif /* POSIXSIGNAL */
#ifdef BSDSIGNAL
  struct sigvec sv;
  sv.sv_handler = (SIGHANDLERTYPE)trigger_irq;
  sv.sv_mask = 0L;
  sv.sv_flags = 0L;
#ifdef SV_BSDSIG
  sv.sv_mask = SV_BSDSIG;
#endif
  if (block_count<0)
    sigblock(sigmask(trigger_signal));
  if (sigvec(trigger_signal, &sv, NULL) < 0)
    error(NIL,"internal error: sigvec failed",NIL);
#endif /* BSDSIGNAL */

#ifdef DEBUG
  printf("trigger_signal mode = %d\n", trigger_mode);
#endif
}

/* unset_trigger_signal */
static void
unset_trigger_signal(void)
{
  int i;
  for (i=0; i<trigger_nfds; i++)
    {
      int fd = trigger_fds[i];
      int flag = 0;
      switch(trigger_mode)
        {
#ifdef FASYNC
        case MODE_FASYNC:
          flag = fcntl(fd, F_GETFL, 0);
          fcntl(fd, F_SETFL, flag & ~FASYNC);
          break;
#endif
#ifdef FIOASYNC
        case MODE_FIOASYNC:
          flag = 0;
          ioctl(fd, FIOASYNC, &flag);
          break;
#endif
#ifdef I_SETSIG
        case MODE_SETSIG:
          ioctl(fd, I_GETSIG, &flag);
          ioctl(fd,I_SETSIG, flag & ~S_INPUT);
          break;
#endif
        default:
          break;
        }
    }
}

/* set_trigger_signal -- detects appropriate mode and sets signalling */
static void
setup_trigger_signal()
{
  int i;
  for (i=0; i<trigger_nfds; i++)
    {
      int fd = trigger_fds[i];
      int pid = 0;
      int flag = 0;

      switch (trigger_mode)
        {
        case MODE_UNKNOWN:
#ifndef BROKEN_FASYNC
#ifdef FASYNC
#ifdef F_SETOWN
          pid = getpid();
          if (fcntl(fd, F_SETOWN, pid) != -1)
            if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | FASYNC) != -1)
              {
                trigger_mode = MODE_FASYNC;
                trigger_signal = SIGIO;
                setup_signal_once();
                break;
              }
#endif
#endif
#endif /* !BROKEN_FASYNC */
#ifndef BROKEN_FIOASYNC
#ifdef FIOASYNC
#ifdef FIOSETOWN
          flag = 1;
          pid = getpid();
          if (ioctl(fd, FIOSETOWN, &pid) != -1)
            if (ioctl(fd, FIOASYNC, &flag) != -1)
              {
                trigger_mode = MODE_FIOASYNC;
                trigger_signal = SIGIO;
                setup_signal_once();
                break;
              }
#endif
#endif
#endif /* !BROKEN_FIOASYNC */
#ifndef BROKEN_SETSIG
#ifdef I_SETSIG
#ifdef I_GETSIG
          flag = 0;
          ioctl(fd, I_GETSIG, &flag);
          if (ioctl(fd,I_SETSIG, flag|S_INPUT) != -1)
            {
              trigger_mode = MODE_SETSIG;
              trigger_signal = SIGPOLL;
              setup_signal_once();
              break;
            }
#endif
#endif
#endif /* !BROKEN_SETSIG */
#ifndef BROKEN_TIMER
#ifdef ITIMER_REAL
          trigger_mode = MODE_ITIMER;
          trigger_signal = SIGALRM;
          setup_signal_once();
          break;
#endif
#endif /* !BROKEN_TIMER */
#ifndef BROKEN_ALARM
          trigger_mode = MODE_ALARM;
          trigger_signal = SIGALRM;
          setup_signal_once();
#endif /* !BROKEN_ALARM */
          break;
          
#ifndef BROKEN_FASYNC
#ifdef FASYNC
#ifdef F_SETOWN
        case MODE_FASYNC:
          pid = getpid();
          fcntl(fd, F_SETOWN, pid);
          fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | FASYNC);
          break;
#endif
#endif
#endif /* !BROKEN_FASYNC */
          
#ifndef BROKEN_FIOASYNC
#ifdef FIOASYNC
#ifdef FIOSETOWN
        case MODE_FIOASYNC:
          flag = 1;
          pid = getpid();
          ioctl(fd, FIOSETOWN, &pid);
          ioctl(fd, FIOASYNC, &flag);
          break;
#endif
#endif
#endif /* !BROKEN_FIOASYNC */
          
#ifndef BROKEN_SETSIG
#ifdef I_GETSIG
#ifdef I_SETSIG
        case MODE_SETSIG:
          ioctl(fd, I_GETSIG, &flag);
          ioctl(fd,I_SETSIG, flag|S_INPUT);
          break;
#endif
#endif
#endif /* !BROKEN_SETSIG */
        default:
          break;
        }
    }
}



/* ------------------------------------- */
/* EVENT SUPPORT ROUTINES                */
/* ------------------------------------- */     

void 
os_block_async_poll(void)
{
  block_async_trigger();
}

void 
os_unblock_async_poll(void)
{
  unblock_async_trigger();
}

void 
os_setup_async_poll(int* fds, int nfds, void(*apoll)(void))
{
  block_async_trigger();
  trigger_handler = 0;
  unset_trigger_signal();
  if (apoll && nfds>0)
    {
      int i;
      trigger_handler = apoll;
      for (i=0; i<nfds && i<MAX_TRIGGER_NFDS; i++)
        trigger_fds[i] = fds[i];
      trigger_nfds = i;
      setup_trigger_signal();
    }
  unblock_async_trigger();
}

void 
os_curtime(int *sec, int *msec)
{
#if defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *sec = tv.tv_sec;
  *msec = tv.tv_usec / 1000;
#elif defined(HAVE_FTIME)
  struct timeb tb;
  ftime(&tb);
  *sec = (int)tb.time;
  *msec = (int)tb.millitm;
#else
  time_t tm;
  time(&tm);
  *sec = (int)tm;
  *msec = 0;
#endif
}

int  
os_wait(int nfds, int* fds, int console, unsigned long ms)
{
  int i;
  int maxfd = 0;
  fd_set set;
  struct timeval tv;
  FD_ZERO(&set);
  for (i=0; i<nfds; i++) {
    int fd = fds[i];
    FD_SET(fd, &set);
    if (fd > maxfd)
      maxfd = fd;
  }
  if (console)
    FD_SET(0, &set);
  tv.tv_sec = ms/1000;
  tv.tv_usec = (ms%1000)*1000;
  block_async_poll();
  select(maxfd+1, &set, 0, 0, &tv);
  unblock_async_poll();
  return FD_ISSET(0,&set);
}


/* console_getline -- 
   gets a line on the console (and process events) */

#ifdef RL_READLINE_VERSION

static int console_in_eventproc = 0;

static void
console_wait_for_char(int prep)
{
  fflush(stdout);
  console_in_eventproc = 1;
  process_pending_events();
  if (prep) rl_prep_terminal(1);
  console_in_eventproc = 0;
  for(;;)
    {
      at *handler = event_wait(TRUE);
      console_in_eventproc = 1;
      process_pending_events();
      if (prep)
	rl_prep_terminal(1);
      console_in_eventproc = 0;
      if (! handler)
	break;
      UNLOCK(handler);
    }
}


static int
console_getc(FILE *f)
{
#if HAVE_RL_GETC  
  if (f != stdin)
    return rl_getc(f);
  console_wait_for_char(TRUE);
  if (break_attempt)
    return EOF;
  return rl_getc(f);
#else
  char buf;
  if (f && f != stdin)
    return getc(f);
  console_wait_for_char(TRUE);
  if (!break_attempt)
    if (read(0, &buf, 1) == 1)
      return (int)buf;
  return EOF;
#endif
}

static char *
symbol_generator(const char *text, int state)
{
  int i;
  static int hni;
  static struct hash_name *hn;
  if (!text || !text[0])
    return NULL;
  if (! state) 
    {
      hni = 0;
      hn = 0;
    }
  while (hni < HASHTABLESIZE) 
    {
      struct hash_name *h;
      const unsigned char *a;
      unsigned char *b;
      /* move to next */
      if (! hn) 
        {
          hn = names[hni];
          hni++;
          continue;
        }
      h = hn;
      hn = hn->next;
      /* compare symbol names */
      if (text[0]=='|' || tolower(text[0])==h->name[0])
        {
          a = (const unsigned char*) text;
          b = (unsigned char*) pname(h->named);
          i = 0;
          while (a[i])
            {
              unsigned char ac = a[i];
              unsigned char bc = b[i];
              if (text[0] != '|') 
                ac = tolower(ac);
              if (text[0] != '|' && ac=='_')
                ac = '-';
              if (ac != bc) 
                break;
              i++;
            }
          if (! a[i])
            return strdup((const char*) b);
        }
    }
  return 0;
}


#if RL_READLINE_VERSION > 0x400

static char **
console_complete(const char *text, int start, int end)
{
  int i;
  int state = 0;
  int lasti = -1;
  /* Where are we? */
  for (i=0; i<start; i++)
    {
      char c = rl_line_buffer[i];
      if (! strchr(rl_basic_word_break_characters, c))
        lasti = i;
      switch(state) {
      case 0:    
        if (c==';' || c=='|' || c=='"')
          state = c; 
        break;
      case '\\': 
        state = '"'; 
        break;
      case '"':  
        if (c=='\\') 
          state = '\\';
      case '|':  
        if (c==state) 
          state = 0;
      case ';':  
        if ( c=='\n' || c=='\r') 
          state = 0; 
        break;
      }
    }
  /* Filename completion */
  if (state == '"' && start>0 && rl_line_buffer[start-1]=='"')
    return NULL; /* first word of a string */
  if (state == 0 && lasti>=0 && rl_line_buffer[lasti]==(0x1f&'L'))
    return NULL; /* after ctrl-L */
  if (state == 0 && lasti>=1 && 
      rl_line_buffer[lasti-1]=='^' && rl_line_buffer[lasti]=='L')
    return NULL; /* after ^L */
  /* Symbol completion */
  if ((state==0 || state=='|') && end>start)
    return rl_completion_matches(text, symbol_generator);
  /* No completion */
  rl_attempted_completion_over = 1;
  return 0;
}

#endif

static void
console_init(void)
{
  /* quotes etc. */
  static const char *special_prefixes = "|";
  static const char *quote_characters = "\"";
  static const char *word_break_characters = 
    " ()#^\"|;"   /* syntactic separators */
    "~'`,@[]{}:"  /* common macro characters */
    "\001\002\003\004\005\006\007"
    "\010\011\012\013\014\015\016\017"
    "\020\021\022\023\024\025\026\027"
    "\030\031\032\033\034\035\036\037";
  /* callbacks */
  rl_getc_function = console_getc;
  /* completion */
#if RL_READLINE_VERSION > 0x400
  rl_special_prefixes = special_prefixes;
  rl_basic_quote_characters = quote_characters;
  rl_basic_word_break_characters = word_break_characters;
  rl_attempted_completion_function = console_complete;
#else
  rl_special_prefixes = (char*)special_prefixes;
  rl_completer_quote_characters = (char*)quote_characters;
  rl_completer_word_break_characters = (char*)word_break_characters;
  rl_completion_entry_function = (void*)symbol_generator;
#endif
  /* matching parenthesis */
#if RL_READLINE_VERSION > 0x400
  rl_set_paren_blink_timeout(250000);
  rl_variable_bind("blink-matching-paren","on");
  rl_variable_bind("comment-begin",";;; ");
#endif  
}

void
console_getline(char *prompt, char *buf, int size)
{
  char *s, *line;
  buf[0] = 0;
  /* Problem: Recursive calls to readline happen when an 
     event handler attempts to read something on the tty.
     Solution: Revert to non-readline non-event operation. */
  if (console_in_eventproc)
    {
      rl_deprep_terminal();
      fputs(prompt,stdout);
      fflush(stdout);
      if (!break_attempt)
        if (!feof(stdin))
          if (!fgets(buf,size-2,stdin))
            buf[0] = 0;
      return;
    }
  /* Problem: Readline erases previous output on the same line.
     Solution: Revert to non-readline operation. */
  if (context->output_tab > 0)
    {
      rl_deprep_terminal();
      fputs(prompt, stdout);
      fflush(stdout);
      console_wait_for_char(FALSE);
      if (context->output_tab > 0)
        {
          if (!break_attempt)
            if (!feof(stdin))
              if (!fgets(buf,size-2,stdin))
                buf[0] = 0;
          return;
        }
    }
  /* Call readline */
  line = readline(prompt);
  if (line)
    {
      /* Hack to allow very long lines */
      if (buf == line_buffer)
        {
          int l = strlen(line) + 4;
          if (l < LINE_BUFFER)
            l = LINE_BUFFER;
          if ((s = malloc(l)))
            {
              free(line_buffer);
              line_pos = line_buffer = buf = s;
              size = l-2;
            }
        }
      /* End of hack */
      strncpy(buf, line, size);
      buf[size-1] = 0;
      buf[size-2] = 0;
      strcat(buf,"\n");
      s = line;
      while (*s==' ' || *s=='\t')
        s += 1;
      if (*s)
        add_history(line);
      free(line);
    }
  else
    {
      /* Got a ctrl+d */
      extern void set_toplevel_exit_flag(void);
      set_toplevel_exit_flag();
      print_string("\n");
      buf[0] = (char)EOF;
      buf[1] = 0;
    }
}

#else /* !RL_READLINE_VERSION */

static void
console_init(void)
{
}


void
console_getline(char *prompt, char *buf, int size)
{
  fputs(prompt,stdout);
  fflush(stdout);
  process_pending_events();
  for(;;)
    {
      at *handler = event_wait(TRUE);
      process_pending_events();
      if (! handler) break;
      UNLOCK(handler);
    }
  if (break_attempt)
    return;
  if (feof(stdin))
    return;
  fgets(buf,size-2,stdin);
}

#endif


void
toplevel_unix(void)
{
  break_attempt = 0;
  async_attempt = 0;
#ifdef RL_READLINE_VERSION
  console_in_eventproc = 0;
  rl_deprep_terminal();
#endif
}


/* ---------------------------------------- */
/* SYSTEM DEPENDENT TL PRIMITIVES           */
/* ---------------------------------------- */


/* getpid -- returns my process ID */

DX(xgetpid)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(getpid());
}


/* getuid -- returns my user ID */

DX(xgetuid)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(getuid());
}


/* getusername -- returns my user name */

DX(xgetusername)
{
  struct passwd *pwd;
  ARG_NUMBER(0);
  pwd = getpwuid(getuid());
  if (pwd)
    return new_string(pwd->pw_name);
  return NIL;
}


/* isatty -- tells if a file is a terminal */

DX(xisatty)
{
  FILE *f;
  int fd;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  f = open_read(ASTRING(1),NIL);
  fd = isatty(fileno(f));
  file_close(f);
  if (fd)
    return true();
  else
    return NIL;
}



/* sys -- executes a shell command */

DX(xsys)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(system(ASTRING(1)));
}


/* real-time, cpu-time -- return the number of 
   seconds (real or cpu) executing expressions. */

DY(yrealtime)
{
  at *q;
  int s1, ms1, s2, ms2;
  os_curtime(&s1, &ms1);
  q = progn(ARG_LIST);
  os_curtime(&s2, &ms2);
  UNLOCK(q);
  return NEW_NUMBER(s2 - s1 + (double) (ms2 - ms1) * 0.001);
}

DY(ycputime)
{
  long ticks;
  struct tms buffer;
  time_t oldtime, newtime;
  at *q;
  
  times(&buffer);
  oldtime = buffer.tms_utime + buffer.tms_stime;
  q = progn(ARG_LIST);
  times(&buffer);
  newtime = buffer.tms_utime + buffer.tms_stime;
  UNLOCK(q);
  
  ticks = -1;
#ifdef HAVE_SYSCONF
#ifdef _SC_CLK_TCK
  ticks = sysconf(_SC_CLK_TCK);
#endif
#endif
#ifdef CLK_TCK
  if (ticks <= 0)
    ticks = CLK_TCK;
#else
  if (ticks <= 0)
    ticks = 60;
#endif
  return NEW_NUMBER((newtime - oldtime) / (double) ticks);
}

/* time -- return the number of seconds
   spent since a constant, system dependent, date */

DY(ytime)
{
  if (! ARG_LIST) 
    {
      int s, ms;
      os_curtime(&s, &ms);
      return NEW_NUMBER( s + (double) ms * 0.001 );
    } 
  else 
    {
      fprintf(stderr,"+++ Warning: deprecated function\n");
      fprintf(stderr,"+++ Use <cputime> or <realtime> instead.\n");
      return ycputime(ARG_LIST);
    }
}


/* ctime -- returns string with current time */

DX(xctime)
{
  char *t;
  time_t tl;

  ifn(arg_number) {
    time(&tl);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    tl = AINTEGER(1);
  }
  t = (char *) ctime(&tl);
  t[strlen(t) - 1] = 0;
  return new_string(t);
}


/* localtime -- returns plist with current time */

DX(xlocaltime)
{
  struct tm *t;
  time_t tl;
  at *ans = NIL;

  ifn(arg_number) {
    time(&tl);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    tl = AINTEGER(1);
  }
  t = (struct tm *) localtime(&tl);
#define ADD_TIME_PROP(p,v) ans=cons(named(p),cons(NEW_NUMBER(t->v),ans))
  ADD_TIME_PROP("tm-sec", tm_sec);
  ADD_TIME_PROP("tm-min", tm_min);
  ADD_TIME_PROP("tm-hour", tm_hour);
  ADD_TIME_PROP("tm-mday", tm_mday);
  ADD_TIME_PROP("tm-mon", tm_mon);
  ADD_TIME_PROP("tm-year", tm_year);
  ADD_TIME_PROP("tm-wday", tm_wday);
  ADD_TIME_PROP("tm-yday", tm_yday);
  ADD_TIME_PROP("tm-isdst", tm_isdst);
#undef ADD_TIME_PROP
  return ans;
}


/* beep -- emits alert sound */

DX(xbeep)
{
  putchar(7);
  fflush(stdout);
  return new_safe_string("beep");
}


/* bground -- forks process in background */

DY(ybground)
{
  int pid;
  int f;
  at *fname;
  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "syntax error", NIL);
  fname = eval(ARG_LIST->Car);
  ifn(fname && (fname->flags & X_STRING))
    error("bground", "string expected as first argument", NIL);
  if (isatty(0) && ask("Launch a background job") == 0)
    error("ask", "Background launch aborted", NIL);
  f = open(SADD(fname->Object), O_WRONLY|O_TRUNC|O_CREAT, 0666);
  if (f<0)
    test_file_error(NULL);
  fflush(NULL);
  if ((pid = fork())) {		/* FATHER */
    close(f);
    return NEW_NUMBER(pid);
  } else {			/* SON */
    time_t clock;
    goodsignal(SIGHUP, SIG_IGN);
    goodsignal(SIGINT, SIG_IGN);
    goodsignal(SIGQUIT, SIG_IGN);
    fflush(stdin);
    if (error_doc.script_file)
      set_script(NIL);
#ifdef BUFSIZ
    setbuf(stdin,NULL);
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
#endif
    dup2(f,0);
    dup2(f,1);
    dup2(f,2);
    setsid();
    time(&clock);
    fprintf(stderr,"\n\n**** BGROUND: Starting job on %s\n", ctime(&clock));
    error_doc.debug_toplevel = TRUE;
    if (sigsetjmp(context->error_jump, 1)) {
      time(&clock);
      fprintf(stderr,"\n\n**** BGROUND: Error occured on %s\n", ctime(&clock));
      _exit(10);
    }
    fname = progn(ARG_LIST->Cdr);
    time(&clock);
    fprintf(stderr,"\n\n****   BGROUND: Job terminated on %s\n", ctime(&clock));
    _exit(0);
  }
  return NIL; /* Never reached; silences compiler */
}


/* savevm -- execute expression without side effects */

DY(ysavevm)
{
  int pid;
  int pipefd[2];
  FILE *piperd;
  FILE *pipewr;
  at *code = NIL;
  at *result = NIL;

  ifn(CONSP(ARG_LIST))
    error(NIL, "syntax error", NIL);
  fflush(NULL);
  if (pipe(pipefd) < 0)
    error(NIL,"pipe() failed", NULL);
  piperd = fdopen(pipefd[0],"rb");
  pipewr = fdopen(pipefd[1],"wb");
  pid = fork();
  if (pid < 0)
    {
      fclose(piperd);
      fclose(pipewr);
      error(NIL,"fork() failed", NULL);
    }
  if (pid == 0)
    {
      /* SON PROCESS */
      fclose(piperd);
      error_doc.debug_toplevel = FALSE;
      if (sigsetjmp(context->error_jump, 1))
        {
          extern char *error_text(); /* toplevel.c */
          result = new_string(error_text());
        }
      else
        {
          extern at *at_break;
          extern at *at_debug;
          at *nullb = cons(true(), NIL);
          at *nullf = new_de(NIL,nullb);
          UNLOCK(nullb);
          symbol_push(at_break, nullf);
          symbol_push(at_debug, nullf);
          UNLOCK(nullf);
          result = progn(ARG_LIST);
          symbol_pop(at_break);
          symbol_pop(at_debug);
          code = true();
        }
      bwrite(code, pipewr, FALSE);
      bwrite(result, pipewr, FALSE);
      fclose(pipewr);
      UNLOCK(result);
      UNLOCK(code);
      fflush(NULL);
      _exit(0);
    }
  else
    {
      /* PARENT PROCESS */
      fclose(pipewr);
      goodsignal(SIGINT, SIG_IGN);
      code = bread(piperd, FALSE);
      result = bread(piperd, FALSE);
      goodsignal(SIGINT, break_irq);      
      fclose(piperd);
      if (code)
        {
          UNLOCK(code);
          return result;
        }
      else
        {
          const char *errtext = "Unknown error";
          if (EXTERNP(result, &string_class))
            errtext = SADD(result->Object);
          error(NIL, errtext, NIL);
        }
    }
  /* not reached */
  return NIL;
}


/* ---------------------------------------- */
/* ENVIRONMENT */
/* ---------------------------------------- */


int
unix_setenv(const char *name, const char *value)
{
#ifdef HAVE_SETENV
  return setenv(name, value, TRUE);
#else 
  char *s;
  if (! (s = malloc(strlen(name)+strlen(value)+2)))
    return ENOMEM;
  strcpy(s,name);
  strcat(s,"=");
  strcat(s,value);
#ifdef HAVE_PUTENV
  return putenv(s);
#else
  {
    static char **lastenv;
    extern char **environ;
    int envsize = 0;
    char **newenv = 0;
    while (environ[envsize])
      envsize += 1;
    if (! (newenv = malloc( (envsize+2)*sizeof(char*))))
      return ENOMEM;
    envsize = -1;
    while(environ[++envsize])
      newenv[envsize] = environ[envsize];
    newenv[envsize] = s;
    newenv[envsize+1] = 0;
    if (environ == lastenv)
      free(environ);
    environ = lastenv = newenv;
    return 0;
  } 
#endif
#endif
}


/* getenv -- return environment string */

DX(xgetenv)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(getenv(ASTRING(1)));
}


/* getconf -- return autoconf string */

DX(xgetconf)
{
  int i;
  const char *k;
  static struct { char *k,*v; } confdata[] = {
    { "LUSH_MAJOR", enclose_in_string(LUSH_MAJOR) },
    { "LUSH_MINOR", enclose_in_string(LUSH_MINOR) },
#ifdef __DATE__
    { "LUSH_DATE", __DATE__ },
#endif
#ifdef __TIME__
    { "LUSH_TIME", __TIME__ },
#endif
    LUSH_MAKE_MACROS,
    { 0, 0 } 
  };
  ARG_NUMBER(1);
  ARG_EVAL(1);
  k = ASTRING(1);
  for (i=0; confdata[i].k; i++)
    if (!strcmp(k, confdata[i].k))
      return new_string(confdata[i].v);
  return NIL;
}



/* ---------------------------------------- */
/* PIPES, FILTERS, SOCKETS, ...                    */
/* ---------------------------------------- */


/*
 * popen/pclose
 * reimplemented because too many unices are broken 
 */

#ifdef HAVE_WAITPID
#define NEED_POPEN
#endif
#undef popen
#undef pclose

#ifdef NEED_POPEN

static pid_t *kidpid = 0;
static int  kidpidsize = 0;

static pid_t *
kidpidalloc(int fd)
{
  pid_t *kp = kidpid;
  int sz = kidpidsize;
  if (fd >= sz)
    {
      while (fd >= sz)
        sz += 256;
      if (! kp)
        kp = malloc(sizeof(int)*sz);
      else
        kp = realloc(kp, sizeof(int)*sz);
      if (! kp)
        return NULL;
      kidpid = kp;
      while (kidpidsize < sz)
        kp[kidpidsize++] = 0;
    }
  return kp;
}
#endif


FILE* 
unix_popen(const char *cmd, const char *mode)
{
#ifndef NEED_POPEN
  return popen(cmd, mode);
#else
# define tst(a,b) (*mode == 'r'? (b) : (a))
  int i;
  int p[2];
  int rd = (mode[0]=='r');
  int parent_fd, child_fd, std_fd;
  pid_t pid;

  /* Create pipe */
  if (pipe(p) < 0) 
    test_file_error(NULL);
  parent_fd = (rd ? p[0] : p[1]);
  child_fd =  (rd ? p[1] : p[0]);
  std_fd = (rd ? fileno(stdout) : fileno(stdin));
  /* Fork */
#ifdef HAVE_VFORK
  pid = vfork();
#else
  pid = fork();
#endif
  if (pid < 0)
    {
      close(parent_fd);
      close(child_fd);
      return NULL;
    }
  else if (pid == 0) 
    {
      /* Child process */
      close(parent_fd);
      if (child_fd != std_fd) 
        {
          close(std_fd);
          if (dup2(child_fd, std_fd) < 0)
            _exit(127);
          close(child_fd);
        }
#ifdef HAVE_SETPGRP
# ifdef SETPGRP_VOID
      setpgrp();
# else
      setpgrp(0,0);
# endif
#endif
      for (i=0; i<kidpidsize; i++)
        if (kidpid[i])
          close(i);
      execl("/bin/sh", "sh", "-c", cmd, NULL);
      _exit(127);
    }
  /* Parent process */
  if (kidpidalloc(parent_fd))
    kidpid[parent_fd] = pid;
  close(child_fd);
  return fdopen(parent_fd, mode);
#endif
}


int 
unix_pclose(FILE *f)
{
#ifndef NEED_POPEN
  return pclose(f);
#else
  pid_t pid;
  int status;
  int fd;
  int i;
  /* Check */
  fd = fileno(f);
  if (fd < 0 || fd >= kidpidsize)
    return -1;
  if (kidpid[fd] == 0)
    return -1;
  /* Remove from pidlist */
  pid = kidpid[fd];
  for (i=0; i<kidpidsize; i++)
    if (kidpid[i] == pid)
      kidpid[i] = 0;
  /* Close */
  i = FALSE;
  if (fflush(f) < 0)
    i = TRUE;
  if (close(fd) < 0)
    i = TRUE;
  if (waitpid(pid, &status, 0) < 0)
    i = TRUE;
  if (i)
    return -1;
  /* Perform real fclose */
  i = errno;
  fclose(f);
  errno = i;
  return (unsigned)(status);
#endif
}


void
filteropen(const char *cmd, FILE **pfw, FILE **pfr)
{
  extern char *string_buffer;
  int i, fd_up[2], fd_dn[2];
  pid_t pid;
  
  sprintf(string_buffer,"exec %s",cmd);
  if (pipe(fd_up) < 0) 
    test_file_error(NULL);
  if (pipe(fd_dn) < 0) {
    close(fd_up[0]);
    close(fd_up[1]);
    test_file_error(NULL);
  }
#ifdef HAVE_VFORK
  pid = vfork();
#else
  pid = fork();
#endif
  if (pid < 0)
    {
      close(fd_up[0]);
      close(fd_up[1]);
      close(fd_dn[0]);
      close(fd_dn[1]);
      test_file_error(NULL);
    }
  else if (pid == 0) 
    {
      /* Child process */
      close(fd_up[1]);
      close(fd_dn[0]);
      if (fd_up[0] != fileno(stdin)) {
        if (dup2(fd_up[0], fileno(stdin)) < 0)
          _exit(127);
        close(fd_up[0]);
      }
      if (fd_dn[1] != fileno(stdout)) {
        if (dup2(fd_dn[1], fileno(stdout)) < 0)
          _exit(127);
        close(fd_dn[1]);
      }
#ifdef HAVE_SETPGRP
# ifdef SETPGRP_VOID
      setpgrp();
# else
      setpgrp(0,0);
# endif
#endif
#ifdef NEED_POPEN
      for (i=0; i<kidpidsize; i++)
        if (kidpid[i])
          close(i);
#endif
      execl("/bin/sh", "sh", "-c", string_buffer, NULL);
      _exit(127);
    }
  /* Parent process */ 
  close(fd_up[0]);
  close(fd_dn[1]);
#ifdef NEED_POPEN
  if (kidpidalloc(fd_up[1]))
    kidpid[fd_up[1]] = pid;
  if (kidpidalloc(fd_dn[0]))
    kidpid[fd_dn[0]] = pid;
#endif
  if (! (*pfw = fdopen(fd_up[1], "w")))
    test_file_error(NULL);
  if (! (*pfr = fdopen(fd_dn[0], "r")))
    test_file_error(NULL);
}


DX(xfilteropen) 
{
  char* cmd;
  FILE* str_up;
  FILE* str_dn;
  at *p1,*p2, *f1, *f2;
  
  ALL_ARGS_EVAL;
  if (arg_number==3) {
    p1 = APOINTER(2);
    ifn (EXTERNP(p1, &symbol_class))
      error(NIL,"not a symbol",p1);
    p2 = APOINTER(3);
    ifn (EXTERNP(p2, &symbol_class))
      error(NIL,"not a symbol",p2);
  } else {
    p1 = p2 = NIL;
    ARG_NUMBER(1);
  }
  cmd = ASTRING(1);
  filteropen(cmd, &str_up, &str_dn);
  f1 = new_extern(&file_R_class, str_dn);
  f2 = new_extern(&file_W_class, str_up);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
}


void
filteropenpty(const char *cmd, FILE **pfw, FILE **pfr)
{
#if HAVE_OPENPTY
  extern char *string_buffer;
  int i, master, slave;
  pid_t pid;
  
  sprintf(string_buffer,"exec %s",cmd);
  if (openpty(&master, &slave, 0, 0, 0) < 0)
    test_file_error(NULL);
# ifdef HAVE_VFORK
  pid = vfork();
# else
  pid = fork();
# endif
  if (pid < 0)
    {
      close(master);
      close(slave);
      test_file_error(NULL);
    }
  else if (pid == 0) 
    {
      /* Child process */
      close(master);
      if (dup2(slave, fileno(stdin)) < 0)
        _exit(127);
      if (dup2(slave, fileno(stdout)) < 0)
        _exit(127);
      if (slave != fileno(stdin) && slave != fileno(stdout))
        close(slave);
# ifdef HAVE_SETPGRP
#  ifdef SETPGRP_VOID
      setpgrp();
#  else
      setpgrp(0,0);
#  endif
# endif
# ifdef NEED_POPEN
      for (i=0; i<kidpidsize; i++)
        if (kidpid[i])
          close(i);
# endif
      execl("/bin/sh", "sh", "-c", string_buffer, NULL);
      _exit(127);
    }
  /* Parent process */ 
  close(slave);
  slave = dup(master);
# ifdef NEED_POPEN
  if (kidpidalloc(master))
    kidpid[master] = pid;
  if (kidpidalloc(slave))
    kidpid[slave] = pid;
# endif
  if (! (*pfw = fdopen(master, "w")))
    test_file_error(NULL);
  if (! (*pfr = fdopen(slave, "r")))
    test_file_error(NULL);
#else
  error(NIL,"filteropenpty is not supported on this system",NIL);
#endif
}


DX(xfilteropenpty) 
{
  char* cmd;
  FILE* str_up;
  FILE* str_dn;
  at *p1,*p2, *f1, *f2;
  
  ALL_ARGS_EVAL;
  if (arg_number==3) {
    p1 = APOINTER(2);
    ifn (EXTERNP(p1, &symbol_class))
      error(NIL,"not a symbol",p1);
    p2 = APOINTER(3);
    ifn (EXTERNP(p2, &symbol_class))
      error(NIL,"not a symbol",p2);
  } else {
    p1 = p2 = NIL;
    ARG_NUMBER(1);
  }
  cmd = ASTRING(1);
  filteropenpty(cmd, &str_up, &str_dn);
  f1 = new_extern(&file_R_class, str_dn);
  f2 = new_extern(&file_W_class, str_up);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
}


DX(xsocketopen)
{
#ifdef HAVE_GETHOSTBYNAME
  at *p1, *p2 , *f1, *f2;
  int sock1, sock2;
  char *hostname;
  int portnumber;
  int noerror = 0;
  struct sockaddr_in server;
  struct hostent *hp;
  FILE *ff1, *ff2;
  
  p1 = NIL;
  p2 = NIL;
  ALL_ARGS_EVAL;
  if (arg_number!=2) 
    {
      ARG_NUMBER(4);
      ASYMBOL(3);
      ASYMBOL(4);
      p1 = APOINTER(3);
      p2 = APOINTER(4);
    }
  hostname = ASTRING(1);
  portnumber = AINTEGER(2);
  noerror = (portnumber < 0);
  portnumber = abs(portnumber);
  hp = gethostbyname(hostname);
  if (hp==0) 
    error(NIL,"unknown host",APOINTER(1));
  sock1 = socket( AF_INET, SOCK_STREAM, 0);
  if (sock1<0)
    test_file_error(NULL);
  server.sin_family = AF_INET;
  memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
  server.sin_port = htons(portnumber);
  if (connect(sock1, (struct sockaddr*)&server, sizeof(server) ) < 0)
    {
      close(sock1);
      if (noerror)
        return NIL;
      test_file_error(NULL);
    }
  sock2 = dup(sock1);
  ff1 = fdopen(sock1,"r");
  ff2 = fdopen(sock2,"w");
  f1=new_extern(&file_R_class, ff1);
  f2=new_extern(&file_W_class, ff2);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
#else
  error(NIL,"Sockets are not supported on this machine",NIL);
#endif
}



DX(xsocketaccept)
{
#ifdef HAVE_GETHOSTBYNAME
  at *f1, *f2;
  int sock1, sock2;
  int portnumber;
  struct sockaddr_in server;
  FILE *ff1, *ff2;
  at *p;
  
  ALL_ARGS_EVAL;
  if (arg_number != 1) 
    {
      ARG_NUMBER(3);
      ASYMBOL(2);
      ASYMBOL(3);
    }
  
  p = APOINTER(1);
  if (NUMBERP(p))
    {
      portnumber = AINTEGER(1);
      sock1 = socket( AF_INET, SOCK_STREAM, 0);
      if (sock1 < 0)
        test_file_error(NULL);
      server.sin_family = AF_INET;
      memset(&server.sin_addr, 0, sizeof(server.sin_addr));
      server.sin_port = htons(portnumber);
      if ((bind(sock1, (struct sockaddr*)&server, sizeof(server) ) < 0) ||
          (listen(sock1, 1) < 0) )
        {
          close(sock1);
          if (arg_number == 1)
            return NIL;
          test_file_error(NULL);
        }
      if (arg_number == 1)
        return new_extern(&file_R_class, fdopen(sock1, "rb"));
    }
  else
    {
      ARG_NUMBER(3);
      if (! EXTERNP(p, &file_R_class))
        error(NIL,"Port number or socket descriptor expected", p);
      sock1 = fileno(p->Object);
      portnumber = -1;
    }
  sock2 = accept(sock1, NULL, NULL);
  if (sock2 < 0)
    test_file_error(NULL);
  if (portnumber >= 0)
    close(sock1);
  sock1 = dup(sock2);
  ff1 = fdopen(sock1,"rb");
  ff2 = fdopen(sock2,"wb");
  f1 = new_extern(&file_R_class, ff1);
  f2 = new_extern(&file_W_class, ff2);
  var_set(APOINTER(2),f1);
  var_set(APOINTER(3),f2);
  return cons(f2,f1);
#else
  error(NIL,"Sockets are not supported on this machine",NIL);
#endif
}




DX(xsocketselect)
{
  int i;
  int n = 0;
  int status;
  fd_set rset;
  fd_set wset;
  struct timeval tv;
  struct timeval *ptv = 0;
  at *ans = NIL;

  ALL_ARGS_EVAL;
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  for (i=1; i<=arg_number; i++)
    {
      at *p = APOINTER(i);
      if (NUMBERP(p))
        {
          int ms = p->Number;
          if (ptv)
            error(NIL,"Timeout already provided",NIL);
          if (ms < 0)
            ms = 0;
          tv.tv_sec = ms / 1000;
          tv.tv_usec = (ms - 1000 * tv.tv_sec) * 1000;
          ptv = &tv;
        }
      else if (EXTERNP(p,&file_R_class))
        {
          FILE *f = p->Object;
          int fd = fileno(f);
          FD_SET(fd, &rset);
          if (fd >= n)
            n = fd + 1;
        }
      else if (EXTERNP(p,&file_W_class))
        {
          FILE *f = p->Object;
          int fd = fileno(f);
          FD_SET(fd, &wset);
          if (fd >= n)
            n = fd + 1;
        }
      else
        error(NIL,"Unexpected argument",p);
    }
  status = select(n, &rset, &wset, NULL, ptv);
  if (status > 0)
    {
      for (i=arg_number; i>0; i--)
        {
          at *p = APOINTER(i);
          if (p && (p->flags & C_EXTERN))
            {
              FILE *f = p->Object;
              int fd = fileno(f);
              if (FD_ISSET(fd,&rset) || FD_ISSET(fd,&wset))
                {
                  LOCK(p);
                  ans = cons(p,ans);
                }
            }
        }
    }
  return ans;
}



/* ---------------------------------------- */
/* MPI                                      */
/* ---------------------------------------- */

#if HAVE_MPI
static int mpi_initialized = 0;

DX(xmpi_init)
{
  ARG_NUMBER(0);
  if (mpi_initialized)
    return NIL;
  MPI_Init(&lush_argc, &lush_argv);
  mpi_initialized = 1;
  return true();
}

DX(xmpi_finalize)
{
  ARG_NUMBER(0);
  if (!mpi_initialized)
    return NIL;
  mpi_initialized = 0;
  MPI_Finalize();
  return true();
}
#endif


/* ---------------------------------------- */
/* INITIALIZATION CODE                      */
/* ---------------------------------------- */

void
init_unix(void)
{
  set_irq();
  console_init();
  Fseed(time(NULL));
  dx_define("getpid", xgetpid);
  dx_define("getuid", xgetuid);
  dx_define("getusername", xgetusername);
  dx_define("isatty", xisatty);
  dx_define("sys", xsys);
  dy_define("bground", ybground);
  dy_define("savevm", ysavevm);
  dy_define("time", ytime);
  dy_define("realtime", yrealtime);
  dy_define("cputime", ycputime);
  dx_define("ctime", xctime);
  dx_define("localtime", xlocaltime);
  dx_define("beep", xbeep);
  dx_define("getenv", xgetenv);
  dx_define("getconf", xgetconf);
  dx_define("filteropen", xfilteropen);
  dx_define("filteropenpty", xfilteropenpty);
  dx_define("socketopen", xsocketopen);
  dx_define("socketaccept", xsocketaccept);
  dx_define("socketselect", xsocketselect);
#ifdef HAVE_MPI
  dx_define("mpi-init", xmpi_init);
  dx_define("mpi-finalize", xmpi_finalize);
#endif
}


void
fini_unix(void)
{
#ifdef HAVE_MPI
  if (mpi_initialized)
    MPI_Finalize();
#endif  
}
