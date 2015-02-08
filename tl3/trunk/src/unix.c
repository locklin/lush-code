/*   TL3 Lisp interpreter and development tools
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
 
/************************************************************************
   TL3:   (C) YLC LYB 1988
   mdep_unix.c
   machine dependant code
   $Id: unix.c,v 1.2 2015-02-08 02:31:18 leonb Exp $

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
#include "config.h"

/* UNIX header files */

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

#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/param.h>
#include <time.h>
#include <pwd.h>

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_SYS_STROPTS_H
#include <sys/stropts.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TTOLD_H
#include <sys/ttold.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
# ifdef HAVE_CONFIG_H
#  define HAVE_TLCONF_H HAVE_CONFIG_H
#  undef HAVE_CONFIG_H
# endif
# include <readline/readline.h>
# ifdef HAVE_READLINE_HISTORY_H
#  include <readline/history.h>
# endif
# ifdef HAVE_TLCONF_H
#  define HAVE_CONFIG_H HAVE_LUSHCONF_H 
# endif
# ifndef RL_READLINE_VERSION
#  define RL_READLINE_VERSION 0  /* non-gnu */
# endif
#endif

/* TLISP header files */
#include "header.h"


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


/* break_attempt - flag for control-C interrupt */

int break_attempt;


/* goodsignal -- sets signal using POSIX or BSD when available */

void 
goodsignal(int sig, void *vec)
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
  signal(sig,vec);
#endif /* SYSVSIGNAL */
}


/* quit_irq -- signal handler for QUIT signal */

static void 
quit_irq(void)
{
#ifdef SYSVSIGNAL
  goodsignal(SIGQUIT, quit_irq);
#endif
  error(NIL, "user quit", NIL);
}


/* break_irq -- signal handler for Control-C */

static void 
break_irq(void)
{
  break_attempt = 1;
#ifdef SYSVSIGNAL
  goodsignal(SIGINT, break_irq);
#endif
}


/* lastchance -- safety code for hopeless situations */

void 
lastchance(char *s)
{
  at *q;
  static char reason[40];
  void block_async_trigger(void);
  /* Test for recursive call */
  if (!s || !*s)
    s = "Unknown reason";
  if (strncmp(s,reason,38) == 0)
    {
      abort(reason);
      _exit(100);
    }
  if (reason[0]==0)
    strncpy(reason,s,38);
  /* Signal problem */
  error_doc.ready_to_an_error = FALSE;
  fprintf(stderr, "\n\007**** GASP: Severe error : %s\n", s);
  q = eval(named("toplevel"));
  if (isatty(0) && EXTERNP(q,&de_class))
    {
      fprintf(stderr,"**** GASP: Trying to recover\n");
      fprintf(stderr,"**** GASP: You should save your work immediatly\n\n");
      /* Sanitize IO */
      break_attempt = 0;      
      block_async_trigger();
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
  abort(reason);
}


/* gasp_irq -- signal handler for hopeless situations */

static void
gasp_irq(int sig)
{
  char buffer[80];
  sprintf(buffer,"Signal %d has occurred",sig);
  error_doc.ready_to_an_error = FALSE;
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
  goodsignal(SIGVTALRM,  gasp_irq);
#endif
#ifdef SIGPROF
  goodsignal(SIGPROF,  gasp_irq);
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
/* RUNTIME CONFIGURATION OF EVENT SIGNAL */
/* ------------------------------------- */     

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


/* trigger_signal -- signal used by the trigger system */

static int trigger_signal = -1;

/* block_count -- asynchronous trigger blocking count */

static int block_count = 0;

/* trigger_fd -- file descriptor for trigger messages */

static int trigger_fd = -1;

/* trigger_handle -- function called when trigger is run */

static void (*trigger_handler)(int);

/* trigger_handle -- function called after trigger is run by GETLINE */

static void (*trigger_process)(void);



/* trigger_irq -- signal handler for trigger */
     
static void 
trigger_irq(void)
{
  if (trigger_handler)
    (*trigger_handler)(OP_ASYNC);
  if (trigger_signal < 0)
    return;
  /* reset trigger signal */
#ifdef SYSVSIGNAL
  signal(trigger_signal, trigger_irq);
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

void
unblock_async_trigger(void)
{
  if (++block_count == 0)
    {
      if (trigger_handler)
	(*trigger_handler)(OP_END_WAIT);
      if (trigger_signal >= 0)
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

void
block_async_trigger(void)
{
  if (--block_count == -1)
    if (trigger_signal >= 0)
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


/* setup_signal_once -- called once to set the trigger signal */

static void
setup_signal_once(int fd)
{
#ifdef POSIXSIGNAL
  sigset_t sset;
  struct sigaction sact;
  sact.sa_flags = 0L;
  sact.sa_handler = (void*)trigger_irq;
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
  sv.sv_handler = (void*)trigger_irq;
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


/* set_trigger_signal -- detects appropriate mode and sets signalling */

void
setup_trigger_signal(int fd)
{
  int pid, flag;
  pid = 0;
  flag = 0;
  switch (trigger_mode)
    {
    case MODE_UNKNOWN:
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
	    setup_signal_once(fd);
	    break;
	  }
#endif
#endif
#endif /* !BROKEN_FIOASYNC */
#ifndef BROKEN_FASYNC
#ifdef FASYNC
#ifdef F_SETOWN
      pid = getpid();
      if (fcntl(fd, F_SETOWN, pid) != -1)
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | FASYNC) != -1)
	  {
	    trigger_mode = MODE_FASYNC;
	    trigger_signal = SIGIO;
	    setup_signal_once(fd);
	    break;
	  }
#endif
#endif
#endif /* !BROKEN_FASYNC */
#ifndef BROKEN_SETSIG
#ifdef I_SETSIG
#ifdef I_GETSIG
      flag = 0;
      ioctl(fd, I_GETSIG, &flag);
      if (ioctl(fd,I_SETSIG, flag|S_INPUT) != -1)
      {
        trigger_mode = MODE_SETSIG;
        trigger_signal = SIGPOLL;
        setup_signal_once(fd);
        break;
      }
#endif
#endif
#endif /* !BROKEN_SETSIG */
#ifndef BROKEN_TIMER
#ifdef ITIMER_REAL
      trigger_mode = MODE_ITIMER;
      trigger_signal = SIGALRM;
      setup_signal_once(fd);
      break;
#endif
#endif /* !BROKEN_TIMER */
#ifndef BROKEN_ALARM
      trigger_mode = MODE_ALARM;
      trigger_signal = SIGALRM;
      setup_signal_once(fd);
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


/* call_sync_trigger -- calls trigger synchronously */

void
call_sync_trigger(void)
{
  block_async_trigger();
  if (trigger_handler)
    (*trigger_handler)(OP_SYNC);
  unblock_async_trigger();
}


/* wait_trigger -- waits for a reason call the trigger */

void 
wait_trigger(void)
{
  fd_set set;
  int max = 32;
  FD_ZERO(&set);
  if (trigger_fd >= 0)
    FD_SET(trigger_fd, &set);
  block_async_trigger();
  if (trigger_handler)
    (*trigger_handler)(OP_BEGIN_WAIT);
  select(max,&set,0,&set,0);
  unblock_async_trigger();
}


/* wait_trigger -- waits for the trigger or until input is present */

int
wait_trigger_or_input(void)
{
  fd_set set;
  int max = 32;
  FD_ZERO(&set);
  FD_SET(0, &set);  
  if (trigger_fd >= 0)
    FD_SET(trigger_fd, &set);
  block_async_trigger();
  if (trigger_handler && block_count<0)
    (*trigger_handler)(OP_BEGIN_WAIT);
  select(max,&set,0,0,0);
  unblock_async_trigger();
  return FD_ISSET(0,&set);
}


/* set_trigger -- setup trigger system */

void 
set_trigger(int fd, void (*trigger)(int), void (*process)(void))
{
  if (trigger_fd >= 0)
    error(NIL, "internal error: trigger already set", NIL);
  trigger_fd = fd;
  trigger_handler = trigger;
  trigger_process = process;
  block_async_trigger();
  setup_trigger_signal(fd);
  unblock_async_trigger();
}


/* console_getline -- gets a line on the console (and process events) */

static void *unused;

#ifdef RL_READLINE_VERSION

static int console_in_eventproc = 0;

static void
console_wait_for_char(int prep)
{
  int ok = 0;
  fflush(stdout);
  console_in_eventproc = 1;
  if (trigger_process)
    (*trigger_process)();
  while (!ok)
    {
      block_async_trigger();
      ok = wait_trigger_or_input();
      unblock_async_trigger();
      if (trigger_process)
	(*trigger_process)();
    }      
  if (prep) 
    rl_prep_terminal(1);
  console_in_eventproc = 0;
}

static int
console_getc(FILE *f)
{
  if (f != stdin)
    return rl_getc(f);
  console_wait_for_char(TRUE);
  if (break_attempt)
    return EOF;
  return rl_getc(f);
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
  rl_special_prefixes = special_prefixes;
  rl_completer_quote_characters = quote_characters;
  rl_completer_word_break_characters = word_break_characters;
  rl_completion_entry_function = symbol_generator;
#endif
  /* matching parenthesis */
#if RL_READLINE_VERSION > 0x402
  rl_set_paren_blink_timeout(250000);
#endif
#if RL_READLINE_VERSION > 0x401
  rl_bind_key (')', rl_insert_close);
  rl_bind_key (']', rl_insert_close);
  rl_bind_key ('}', rl_insert_close);
#endif 
  /* comments */
#if RL_READLINE_VERSION > 0x400
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
          unused = fgets(buf,size-2,stdin);
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
              unused = fgets(buf,size-2,stdin);
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
  int ok = 0;
  fputs(prompt,stdout);
  fflush(stdout);
  if (trigger_process)
    (*trigger_process)();
  while (!ok)
    {
      block_async_trigger();
      ok = wait_trigger_or_input();
      unblock_async_trigger();
      if (trigger_process)
	(*trigger_process)();
    }      
  if (break_attempt)
    return;
  if (feof(stdin))
    return;
  unused = fgets(buf,size-2,stdin);
}

#endif



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

int
sys(char *s)
{
#ifndef ALTERNATE_SYSTEM
  /* Uses the stdio routine SYSTEM(s) */
  return system(s);
#else
  /* This old code should not be used on modern systems */
  int status, pid, wai;
  ifn(pid = vfork()) {
    goodsignal(SIGINT, SIG_DFL);
    goodsignal(SIGQUIT, SIG_DFL);
    goodsignal(SIGPIPE, SIG_DFL);
    execl("/bin/sh", "sh", "-c", s, 0L);
    _exit(0);
  }
  wai = 0;
  while (wai != pid && (wai != -1 || errno == EINTR))
    wai = wait(&status);
  if (wai == -1)
    test_file_error(NIL);
  return status >> 8;
#endif
}

DX(xsys)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(sys(ASTRING(1)));
}


/* time -- count time for executing commands */

DY(ytime)
{
  struct tms buffer;
  time_t oldtime, newtime;
  register at *q;

  ifn(ARG_LIST) {
    time(&newtime);
    return NEW_NUMBER(newtime);
  } else {
    times(&buffer);
    oldtime = buffer.tms_utime;
    q = progn(ARG_LIST);
    times(&buffer);
    newtime = buffer.tms_utime;
    UNLOCK(q);
#ifdef CLK_TCK
    return NEW_NUMBER((newtime - oldtime) / (CLK_TCK));
#else
    return NEW_NUMBER((newtime - oldtime) / 60.0);
#endif
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
  ADD_TIME_PROP("tm_sec", tm_sec);
  ADD_TIME_PROP("tm_min", tm_min);
  ADD_TIME_PROP("tm_hour", tm_hour);
  ADD_TIME_PROP("tm_mday", tm_mday);
  ADD_TIME_PROP("tm_mon", tm_mon);
  ADD_TIME_PROP("tm_year", tm_year);
  ADD_TIME_PROP("tm_wday", tm_wday);
  ADD_TIME_PROP("tm_yday", tm_yday);
  ADD_TIME_PROP("tm_isdst", tm_isdst);
#undef ADD_TIME_PROP
  return ans;
}


/* sleep -- waits specified number of seconds */

DX(xsleep)
{
  int t, i;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  t = AINTEGER(1);
  for (i = 0; i < t; i++) {
    sleep(1);
    CHECK_MACHINE("on");
  }
  return NEW_NUMBER(t);
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
    test_file_error(NIL);
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  if ((pid = fork())) {		/* FATHER */
    close(f);
    return NEW_NUMBER(pid);
  } else {			/* SON */
    time_t clock;
    goodsignal(SIGHUP, SIG_IGN);
    goodsignal(SIGINT, SIG_IGN);
    goodsignal(SIGQUIT, SIG_IGN);
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


/* getenv -- return environment string */

DX(xgetenv)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(getenv(ASTRING(1)));
}


/* xdmc -- executes apollo's display manager command */

#ifdef apollo
#include "/sys/ins/base.ins.c"
#include "/sys/ins/pad.ins.c"
void 
xdmc(char *s)
{
  status_$t status;
  pad_$dm_cmd(stream_$stdout, *s, (short) (strlen(s)), status);
  if (status.all != status_$ok)
    error(NIL, "problem in DM command", new_string(s));
}
DX(xxdmc)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  xdmc(ASTRING(1));
  return NIL;
}
#endif



/* ---------------------------------------- */
/* DYNAMIC LOADING                          */
/* ---------------------------------------- */

#ifdef HAVE_DLFCN_H
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#define dlopen_handle_t void*
#endif /* HAVE_LIBDL */
#endif /* HAVE_DLFCN */

#ifdef HAVE_DL_H
#ifdef HAVE_LIBDLD
#include <dl.h>
#define HAVE_DLOPEN
#define dlopen_handle_t shl_t 
/* DLOPEN emulation under HP/UX */
static dlopen_handle_t 
dlopen(char *soname, int mode)
{ 
  return shl_load(soname,BIND_IMMEDIATE|BIND_NONFATAL|
		  BIND_NOSTART|BIND_VERBOSE, 0L ); 
}
static void
dlclose(dlopen_handle_t hndl)
{ 
  shl_unload(hndl); 
}
static void*
dlsym(dlopen_handle_t hndl, char *sym)
{ 
  void *addr = 0;
  if (shl_findsym(&hndl,sym,TYPE_PROCEDURE,&addr) >= 0)
    return addr;
  return 0;
}
static char*
dlerror(void)
{
  return "Function shl_load() has failed";
}
#endif /* HAVE_DL */
#endif /* HAVE_LIBDLD */

#ifdef HAVE_DLOPEN

static struct dlrecord {
  struct dlrecord *next;
  dlopen_handle_t handle;
  char name[1];
} *dlfirst;

DX(xmod_list)
{
  at *ans = NULL;
  at **where = &ans;
  struct dlrecord *rec = dlfirst;
  ARG_NUMBER(0);
  while (rec)
    {
      *where = cons(new_string(rec->name), NIL);
      where = &((*where)->Cdr);
      rec = rec->next;
    }
  return ans;
}

DX(xmod_load)
{
  char *soname;
  char *srname;
  char buffer[80];
  dlopen_handle_t handle;
  int   mode;
  int (*init)(int,int);
  struct dlrecord *rec;
  at *ans;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  /* Search DLL */
  soname = ASTRING(1);
  srname = search_file(soname, "|.so|.sl");
  if (srname)
    srname = concat_fname(NULL,srname);
  else
    srname = soname;
  /* Set DLOPEN mode */
#ifdef RTLD_NOW
  mode = RTLD_NOW;
#else
  mode = 1;
#endif
  /* Load DLL */
  handle = dlopen(srname, mode);
  if (!handle)
  {
    sprintf(buffer, "Dynamic linker error '%s'", dlerror());
    error(NIL, buffer, APOINTER(1));
  }
  /* Check that DLL has not been loaded yet */
  for (rec=dlfirst; rec; rec=rec->next)
    if (handle==rec->handle || !strcmp(srname, rec->name))
      break;
  if (rec)
  {
    dlclose(handle);
    error(NIL,"Module already loaded", APOINTER(1));
  }
  /* Find init symbol */
  init = dlsym(handle, "init_user_dll");
  soname = (char*)dlerror();
  if (init != 0)
  {
    /* Calling init function */
    switch ( (*init)(TLOPEN_MAJOR,TLOPEN_MINOR) )
      {
      case -1:
	dlclose(handle);
	error(NIL,"Module initialization failed", APOINTER(1));
	break;
      case -2:
	dlclose(handle);
	error(NIL,"Module is not compiled for this version of TL3",APOINTER(1));
	break;
      default:
	break;
      }
  }
  /* Prepare record */
  rec = tl_malloc(sizeof(struct dlrecord) + strlen(srname));
  if (!rec)
    {
      dlclose(handle);
      error(NIL,"Out of memory",NIL);
    }
  rec->next = dlfirst;
  rec->handle = handle;
  strcpy(rec->name, srname);
  dlfirst = rec;
  ans = new_string(srname);
  return ans;
}

#endif /* HAVE_DLOPEN */



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
  dy_define("time", ytime);
  dx_define("ctime", xctime);
  dx_define("localtime", xlocaltime);
  dx_define("sleep", xsleep);
  dx_define("beep", xbeep);
  dx_define("getenv", xgetenv);
#ifdef apollo
  dx_define("xdmc", xxdmc);
#endif
#ifdef HAVE_DLOPEN
  dx_define("mod_list", xmod_list);
  dx_define("mod_load", xmod_load);
#endif
}

