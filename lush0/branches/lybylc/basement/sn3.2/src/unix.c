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


#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include "header.h"
#include "errno.h"
#ifdef LINUX
#include "check_func.h"
#endif
#ifndef CLK_TCK
#include "limits.h"
#endif

#ifdef SOCK
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>
#endif

#ifdef __GNUC__
/* gnuc is too clever for vfork */
#undef FORK
#define FORK fork
#endif

/*extern time_t time();*/
#ifndef __STDC__
extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;
#endif

int break_attempt;

#ifdef PARANOIAC_IEEE
#ifndef SGI_OR_ALPHA
#ifndef LINUX
#include <floatingpoint.h>
#endif
#endif
#endif

#ifdef SGI
#include <siginfo.h>
#include <ucontext.h>
#endif



/* 
 * REPLACEMENT FUNCTIONS FOR SILLY UNIXES
 *
 */

/* popen_sgi pclose
 * from jsd's filteropen.c
 */

#define RDR 0
#define WTR 1
#ifdef MYPOPEN
static int* kidpid;	
static int* exstat;	
static int  tabsiz = 0;	
#endif


extern struct dh_trace_stack *dh_trace_root;

#ifdef MYPOPEN
FILE* mypopen(cmd,mode)
char *cmd, *mode;
{
#define	tst(a,b) (*mode == 'r'? (b) : (a))
  int p[2];
  register int parent_fd, child_fd, pid;
  if(pipe(p) < 0) 
    return NULL;
  parent_fd = tst(p[WTR], p[RDR]);
  child_fd =  tst(p[RDR], p[WTR]);
  if ((pid = FORK()) == 0) {
    close(parent_fd);
    if (child_fd !=  tst(fileno(stdin), fileno(stdout))) {
      dup2(child_fd, tst(fileno(stdin), fileno(stdout)));
      close(child_fd);
    }
#ifndef SUNOS
    setpgrp();
#else
    setpgrp(0,0);
#endif
    execl("/bin/sh", "sh", "-c", cmd, 0);
    _exit(1);
  }
  if (pid == -1) {
    close(parent_fd);
    close(child_fd);
    return NULL;
  }
  kidpid[parent_fd] = pid;
  exstat[parent_fd] = -1;	
  close(child_fd);
  return(fdopen(parent_fd, mode));
}
#endif


/* getwd
 * sometimes is not there!
 */


#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif
#ifdef NOGETWD
char *getwd(s)
char *s;
{
  FILE *f;
  strcpy(s,".");
  f = popen("pwd","r");
  fscanf(f," %255s ",s);
  pclose(f);
  return s;
}
#endif

#undef fread
#undef fwrite




#ifdef MYPOPEN

int
pclose(pt)
     FILE *pt;
{
  register int f, pid, sts,ii;
  ptr hstat;
  ptr istat;
  ptr qstat;
  union wait www;	
  
  sts = fflush(pt);
  f = fileno(pt);
#ifdef SOCK
  shutdown(f,2);
#endif
  fclose(pt);
  if (kidpid[f] < 0) 
    return sts;
  /* ignore close sts on downside */
  
  if ((sts = exstat[f]) != -1) {
    kidpid[f] = -1;
    return sts;
  }
  
  qstat = signal(SIGQUIT, SIG_IGN);
  istat = signal(SIGINT, SIG_IGN);
  hstat = signal(SIGHUP, SIG_IGN);
#ifdef DEBUG
  printf ("wait on %d\n",kidpid[f]);
#endif
  for (;;) {
    pid = wait(&www);
    if (pid == -1) {
      www.w_status = -1;
      break;
    }
#ifdef DEBUG
    printf ("found %d\n",pid);
#endif
    if (pid == kidpid[f]) 
      break;
    for (ii = 0; ii < tabsiz; ii++){	
      if (pid == kidpid[ii]) {
	exstat[ii] = www.w_status;
	break;
      }
    }
    /* wait again */
  }
  
  signal(SIGINT, istat);
  signal(SIGQUIT, qstat);
  signal(SIGHUP, hstat);
  
  kidpid[f] = -1;
  return(www.w_status);
}
#endif




#undef fread
#undef fwrite

/* Automatic restart of FREAD, FWRITE... */

int myfread(pt,size,nitems,f)
  char *pt;
  int size,nitems;
  FILE *f;
{
  int now;
  int ok;
  
  now = 0;
  errno = 0;
  while (nitems>0 && !feof(f)) {
    ok = fread(pt,size,nitems,f);
    if (ok==0 && errno && errno!=EINTR) {
      test_file_error(f);
      break;
    }
    now += ok;
    nitems -= ok;
    pt += size*ok;
  }
  return now;
}

int myfwrite(pt,size,nitems,f)
  char *pt;
  int size,nitems;
  FILE *f;
{
  int now;
  int ok;
  
  now = 0;
  errno = 0;
  while (nitems>0) {
    ok = fwrite(pt,size,nitems,f);
    if (ok==0 && errno && errno!=EINTR) {
      test_file_error(f);
      break;
    }
    now += ok;
    nitems -= ok;
    pt += size*ok;
  }
  return now;
}

/*
 * FILENAME FUNCTIONS...
 *   concat_filename(s,s1,buffer,len)
 *   addslash_filename(buffer,len)
 *   ok_dir_filename(s)
 *   ok_read_filename(s)
 *   ok_write_filename(s)
 * 
 * UTILITIES
 *   system_error()
 */


extern char *getenv();


static void
catfname(dst,src,len)
  char *dst,*src;
  int len;
{
  char *s;
  
  if (src) {
    s = dst + strlen(dst);
    while (*src) {
      if (s>dst+len-2)
	error(NIL,"filename is too long",NIL);
      *s++ = *src++;
    }
    s[0]=0;
    s[1]=0;
  }
}

void 
concat_filename(s0,s1,buffer,len)
  char *s0,*s1;
  char *buffer; 
  int len;
{
  char pathname[MAXPATHLEN];
  char *idx, *s, *r, *scratch;
  
  buffer[0] = 0;
  scratch = (char*)alloca(len);
  
  /* if $ of | */
  
  if (s0 && (s0[0]=='$' || s0[0]=='|')) {
    catfname(buffer,s0,len);
    return;
  }
  if (s1 && (s1[0]=='$' || s1[0]=='|')) {
    catfname(buffer,s1,len);
    return;
  }
  
  /* Copy everything in scratch */
  scratch[0]=0;
  
  if (!s1 || (s1[0]!='/' && s1[0]!='~'))
    if (s0 && *s0) {
      catfname(scratch,s0,len);
      catfname(scratch,"/",len);
    }
  if (s1 && *s1)
    catfname(scratch,s1,len);
  
  s = scratch;
  
  /* handle WD and TILDE */
  if (s[0]=='~' && (!s[1] || s[1]=='/')) {
    catfname(buffer,getenv("HOME"),len);
    s+=2;
  } else if (s[0]=='/') {
    s+=1;
  } else {
    catfname(buffer,getwd(pathname),len);
  }
  
  while (*s) {
    idx = strchr(s,'/');
    if (!idx) 
      idx = s+strlen(s);
    *idx = 0;
    if (strcmp(s,"..")==0) {
      r = strrchr(buffer,'/');
      r[0] = 0;
    } else if (strcmp(s,".")==0) {
      /* do nothing */
    } else {
      catfname(buffer,"/",len);
      catfname(buffer,s,len);
    }
    s = idx+1;  
    /* *s=zero if at end! */
    while (*s=='/')
      s++;
  } while (*s);
  
#ifdef DEBUG
  printf("concat: '%s', '%s' => '%s'\n", 
	 (s0?s0:"(nil)"), (s1?s1:"(nil)"), buffer);
#endif
}


void
addslash_filename(buffer,len)
  char *buffer;
  int len;
{
  catfname(buffer,"/",len);
}

void 
addsuffix_filename(suffix,buffer,len)
  char *suffix;
  char *buffer;
  int len;
{
  char *idx;
  ifn (idx = strrchr(buffer,'/'))
    idx = buffer;
  
  if (idx=strrchr(idx,'.'))
  {
    /* final dot: No suffix */
    if (!idx[1])
      idx[0]=0;
  } 
  else if (suffix)
  {
    /* vanilla suffix */
    catfname(buffer,".",len);
    catfname(buffer,suffix,len);
  }
}



/*
 *   ok_dir_filename(s)
 *   ok_read_filename(s)
 *   ok_write_filename(s)
 */

int ok_dir_filename(s)
  char *s;
{
  struct stat buf;
  if (stat(s,&buf)==-1)
    return FALSE;
#ifdef S_ISDIR
  if (S_ISDIR(buf.st_mode))
    return TRUE;
#else
  if (buf.st_mode & S_IFDIR)
    return TRUE;
#endif
  return FALSE;
}

int ok_read_filename(s)
  char *s;
{
  struct stat buf;
  if (stat(s,&buf)==-1)
    return FALSE;
#ifdef S_IFDIR
  if (buf.st_mode & S_IFDIR)
    return FALSE;
#endif
#ifdef S_IROTH
#ifdef S_IRGRP
#ifdef S_IRUSR
  if (buf.st_mode & S_IROTH)
    return TRUE;
  if ((buf.st_gid==getgid()) && (buf.st_mode & S_IRGRP))
    return TRUE;
  if ((buf.st_uid==getuid()) && (buf.st_mode & S_IRUSR))
    return TRUE;
  return FALSE;
#endif
#endif
#endif
  return TRUE;
}

int ok_write_filename(s)
  char *s;
{
  struct stat buf;
  
  if (access(s,F_OK)==-1)
    return FALSE;
  if (access(s,W_OK)==-1)
    return FALSE;
  if (ok_dir_filename(s))
    return FALSE;
  return TRUE;
}


char 
*temp_filename()
{
  static int i=0;
  static char buffer[30];
  do {
    sprintf(buffer,"/tmp/sn%05d.%d",getpid(),++i);
  } while (ok_read_filename(buffer));
  return buffer;
}


char
*system_error()
{
  static char buffer[40];

  if (errno<=0 || errno>=sys_nerr) {
    sprintf(buffer,"Unknown system error #%d",errno);
    return buffer;
  }
  return (char*)sys_errlist[errno];
}


/* ============================================== *
 * UNIX RELATED AND SYSTEM DEPENDENT DX FUNCTIONS *
 * ============================================== */


/*
 * chdir(string) changes current directory to STRING.
 */

DX(xchdir)
{
  char pathname[MAXPATHLEN];
  
  if (arg_number) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    if (chdir(ASTRING(1))==-1)
      error(NIL,system_error(),NIL);
  }
  return new_string(getwd(pathname));
}

/*
 * (dir-existsp <dir>) return argument or () 
 * depending on wether the directory <dir> exists.
 */

DX(xdir_existsp)
{
  register at *p;
  FILE *f;
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  s = ASTRING(1);
  if ((f=fopen(s,"r"))&& ok_dir_filename(s)) {
     fclose(f);
     LOCK(p);
     return p;
  }
  return NIL;
}

/*
 * (mkdir <dir> [<mode>]) tries to create directory STRING,
 * but returns () if it fails. If argument <mode> is provided
 * then it is used to set the permission mode of the directory,
 * otherwise the default is drwxrwxr-x
 */

DX(xmkdir)
{
  register at *p;
  FILE *f;
  int mode;
  ALL_ARGS_EVAL;
  if (arg_number!=1 && arg_number!=2) error(NIL,"Expected 1 or 2 arguments",NIL);
  p = APOINTER(1);
  if (arg_number==2) 
     mode=AINTEGER(2);
  else 
     mode=S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP 
          | S_IXGRP | S_IROTH | S_IXOTH;
  if (mkdir(ASTRING(1),mode)== -1)
     return NIL;
  LOCK(p);
  return p;
}


/*
 * tmpnam() returns a unique temporary filename
 */

DX(xtmpnam)
{
  ARG_NUMBER(0);
  return new_string(temp_filename());
}

/*
 * (getpid) returns this sn pid
 */ 
DX(xgetpid)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(getpid());
}


/*
 * filteropen
 * a VERY useful function...
 */

DX(xfilteropen) 
{
  char* cmd;
  extern char *string_buffer;
  FILE* str_up;
  FILE* str_dn;
  int fd_up[2], fd_dn[2];
  register int pid;
  at *p1,*p2, *f1, *f2;
  int i;
  
  ALL_ARGS_EVAL;
  
  if (arg_number==3) {
    p1 = APOINTER(2);
    ifn (p1 && p1->ctype==XT_SYMBOL)
      error(NIL,"not a symbol",p1);
    p2 = APOINTER(3);
    ifn (p2 && p2->ctype==XT_SYMBOL)
      error(NIL,"not a symbol",p2);
  } else {
    p1 = p2 = NIL;
    ARG_NUMBER(1);
  }
  
  cmd = ASTRING(1);
  sprintf(string_buffer,"exec %s",cmd);
  
  if (pipe(fd_up) < 0) 
    error(NIL,"pipe failed",NIL);
  if (pipe(fd_dn) < 0) {
    close(fd_up[RDR]);
    close(fd_up[WTR]);
    error(NIL,"pipe failed",NIL);
  }
  
  if((pid = FORK()) == 0) {
    close(fd_up[WTR]);
    close(fd_dn[RDR]);
    if (fd_up[RDR] != fileno(stdin)) {
      dup2(fd_up[RDR], fileno(stdin));
      close(fd_up[RDR]);
    }
    if (fd_dn[WTR] != fileno(stdout)) {
      dup2(fd_dn[WTR], fileno(stdout));
      close(fd_dn[WTR]);
    }
#ifndef SUNOS
    setpgrp();
#else
    setpgrp(0,0);
#endif
    for (i=3; i<256; i++) close(i);
    execl("/bin/sh", "sh", "-c", string_buffer, 0);
    _exit(1);
  }
  if(pid == -1) {
    close(fd_up[RDR]);
    close(fd_up[WTR]);
    close(fd_dn[RDR]);
    close(fd_dn[WTR]);
    error(NIL,"fork failed",NIL);
  }
#ifdef MYPOPEN
  kidpid[fd_up[WTR]] = -1;
  exstat[fd_up[WTR]] = -1;
  kidpid[fd_dn[RDR]] = pid;
  exstat[fd_dn[RDR]] = -1;
#endif
  close(fd_up[RDR]);
  close(fd_dn[WTR]);
  str_up = fdopen(fd_up[WTR], "w");
  str_dn = fdopen(fd_dn[RDR], "r");
  f1=new_extern(&file_R_class, new_real_File(str_dn,"r"), XT_FILE_RO);
  f2=new_extern(&file_W_class, new_real_File(str_up,"w"), XT_FILE_WO);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
  
}



DX(xpipe)
{
  at *p1,*p2, *f1, *f2;
  FILE* str_up;
  FILE* str_dn;
  int fd[2];
  
  ALL_ARGS_EVAL;
  if (arg_number==2) {
    p1 = APOINTER(1);
    ifn (p1 && p1->ctype==XT_SYMBOL)
      error(NIL,"not a symbol",p1);
    p2 = APOINTER(2);
    ifn (p2 && p2->ctype==XT_SYMBOL)
      error(NIL,"not a symbol",p2);
  } else {
    p1 = p2 = NIL;
    ARG_NUMBER(0);
  }
  if (pipe(fd) < 0) 
    error(NIL,"pipe failed",NIL);
  str_up = fdopen(fd[WTR], "w");
  str_dn = fdopen(fd[RDR], "r");
  f1=new_extern(&file_R_class, new_real_File(str_dn,"r"), XT_FILE_RO);
  f2=new_extern(&file_W_class, new_real_File(str_up,"w"), XT_FILE_WO);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
}








/*
 * sys_io execute with redirected stdin stdout
 */


int 
sysio(s, fin, fout, waitp)
char *s;
FILE *fin;
FILE *fout;
int waitp;
{
  int pid, status;
  int fdin, fdout;

  /* Prepare */
  fdin = fdout = -1;
  if (fin) {
    fdin = fileno(fin);
    fcntl(fdin, F_SETFD, 0);
  }
  if (fout) {
    fflush(fout);
    fdout = fileno(fout);
    fcntl(fdin, F_SETFD, 0);
  } 
  /* Fork */
  if (! (pid = FORK())) {
    if (fdin > 0) {
      close(0);
      dup(fdin);
      close(fdin);
    }
    if (fdout > 1) {
      close(1);
      dup(fdout);
      close(fdout);
    }
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    execlp("/bin/sh","/bin/sh", "-c", s, 0L);
    _exit(0);
  }
  /* Wait */
  if (!waitp)
    return 0;
  for(;;) {
    if (waitpid(pid,&status,0) == pid)
      break;
    CHECK_MACHINE("on");
    if (errno != EINTR)
      test_file_error(NULL);
  }
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  else 
    return 1000 + WTERMSIG(status);
}



DX(xsysio)
{
  at *atf;
  File *pf;
  FILE *fin = NULL;
  FILE *fout = NULL;
  int waitp = 1;

  ALL_ARGS_EVAL;
  if (arg_number==4) {
    if (!APOINTER(4))
      waitp = 0;
  } else {
    ARG_NUMBER(3);
  }
  atf = APOINTER(1);
  if (atf) {
    if (! (atf->ctype == XT_FILE_RO))
      error(NIL,"Argument #1 must be a read-only file descriptor",atf);
    pf = atf->Object;
    if (pf->type != REAL_FILE)
      error(NIL,"Argument #1 must be a real file descriptor",atf);
    fin = pf->stream.fp;
  }
  atf = APOINTER(2);
  if (atf) {
    if (! (atf->ctype == XT_FILE_WO))
      error(NIL,"Argument #2 must be a write-only file descriptor",atf);
    pf = atf->Object;
    if (pf->type != REAL_FILE)
      error(NIL,"Argument #2 must be a real file descriptor",atf);
    fout = pf->stream.fp;
  }
  return NEW_NUMBER(sysio(ASTRING(3),fin,fout, waitp));
}




/*
 * sys command execute a shell command
 */

int
sys(s)
char *s;
{
#ifndef MYPOPEN
  return system(s);
#else
  ptr hstat;
  ptr istat;
  ptr qstat;
#ifndef SUNOS
  int www;
  pid_t pid, wait_pid;
#else
  union wait www;	
  int pid, waitpid;
#endif
  int ii;
  char *shell = "/bin/sh";
  
#ifdef CUSTOMSHELL
  ifn (shell = getenv("ESHELL"))
    ifn (shell = getenv("SHELL"))
      shell = "/bin/sh";
#endif

  ifn(pid = FORK()) {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
#ifndef SUNOS
    setpgrp();
#else
    setpgrp(0,0);
#endif
    execlp(shell,shell, "-c", s, 0L);
    _exit(0);
  }
  qstat = signal(SIGQUIT, SIG_IGN);
  istat = signal(SIGINT, SIG_IGN);
  hstat = signal(SIGHUP, SIG_IGN);
  for (;;) {
#ifndef SUNOS
    wait_pid = wait(&www);
    if (wait_pid == -1) {
      www = -1;
      break;
    }
    if (pid == wait_pid) 
      break;
    for (ii = 0; ii < tabsiz; ii++){	
      if (wait_pid == kidpid[ii]) {
	exstat[ii] = www;
	break;
      }
    }
    /* wait again */
  }
  signal(SIGINT, istat);
  signal(SIGQUIT, qstat);
  signal(SIGHUP, hstat);
  return www;
#else
    waitpid = wait(&www);
    if (waitpid == -1) {
      www.w_status = -1;
      break;
    }
    if (pid == waitpid) 
      break;
    for (ii = 0; ii < tabsiz; ii++){	
      if (waitpid == kidpid[ii]) {
	exstat[ii] = www.w_status;
	break;
      }
    }
    /* wait again */
  }
  signal(SIGINT, istat);
  signal(SIGQUIT, qstat);
  signal(SIGHUP, hstat);
  return www.w_status;
#endif
#endif
}


DX(xsys)
{
  extern at *at_true;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(sys(ASTRING(1)));
}



#ifdef SOCK

DX(xsocketopen)
{
  at *p1, *p2 , *f1, *f2;
  int sock1, sock2;
  char *hostname;
  int portnumber;
  struct sockaddr_in server;
  struct hostent *hp, *gethostbyname();
  struct passwd *pwd;
  FILE *ff1, *ff2;
  
  p1 = NIL;
  p2 = NIL;
  ALL_ARGS_EVAL;
  
  if (arg_number!=2) {
    ARG_NUMBER(4);
    ASYMBOL(3);
    ASYMBOL(4);
    p1 = APOINTER(3);
    p2 = APOINTER(4);
  }
  hostname = ASTRING(1);
  portnumber = AINTEGER(2);
  
  sock1 = socket( AF_INET, SOCK_STREAM, 0);
  if (sock1<0)
    test_file_error(NIL);
  server.sin_family = AF_INET;
  hp = gethostbyname(hostname);
  if (hp==0) 
    error(NIL,"unknown host",APOINTER(1));
  bcopy((char*)hp->h_addr,(char*)&server.sin_addr,hp->h_length);
  server.sin_port = htons(portnumber);
  
  if (connect(sock1, (struct sockaddr*)&server, sizeof(server) ) < 0)
    test_file_error(NIL);
  
  sock2 = dup(sock1);
  ff1 = fdopen(sock1,"r");
  ff2 = fdopen(sock2,"w");
  f1=new_extern(&file_R_class, new_real_File(ff1,"r"), XT_FILE_RO);
  f2=new_extern(&file_W_class, new_real_File(ff2,"w"), XT_FILE_WO);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
}



DX(xremoteopen)
{
  at *p1, *p2 , *f1, *f2;
  int sock1, sock2;
  int soc2;
  char *hostname;
  char *command;
  struct servent *sp;
  struct passwd *pwd;
  FILE *ff1, *ff2;
  int pid;

  p1 = NIL;
  p2 = NIL;
  ALL_ARGS_EVAL;
  if (arg_number!=2) {
    ARG_NUMBER(4);
    ASYMBOL(3);
    ASYMBOL(4);
    p1 = APOINTER(3);
    p2 = APOINTER(4);
  }
  hostname = ASTRING(1);
  command = ASTRING(2);
  
  pwd = getpwuid(getuid());
  ifn (pwd)
    error(NIL,"who are you?",NIL);
  
  sp = getservbyname("exec","tcp");
  ifn (sp)
    error(NIL,"unknown service",new_safe_string("shell"));
  
  sock1 = rexec(&hostname, sp->s_port, 
		pwd->pw_name, 0L, 
		command, &soc2);
  if (sock1<0)
    error(NIL,"can't connect",APOINTER(1));
  
  if ((pid=FORK()) == 0) {
    int i;
    dup2(soc2,0);
    dup2(2,1);
    for (i=3; i<256; i++) close(i);
#ifndef SUNOS
    setpgrp();
#else
    setpgrp(0,0);
#endif
    execl("/bin/cat","cat",(char*)0L);
  }
  close(soc2);
  
  sock2 = dup(sock1);
  ff1 = fdopen(sock1,"r");
  ff2 = fdopen(sock2,"w");
  f1=new_extern(&file_R_class, new_real_File(ff1,"r"), XT_FILE_RO);
  f2=new_extern(&file_W_class, new_real_File(ff2,"w"), XT_FILE_WO);
  if (p1)
    var_set(p1,f1);
  if (p2)
    var_set(p2,f2);
  return cons(f2,f1);
}



#endif



/*
 * (time (....listes...) (....listes...) )
 */

DY(ytime)
{
  struct tms buffer;
  long oldtime, newtime;
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
    return NEW_NUMBER((newtime - oldtime) / (float)(CLK_TCK));
  }
}



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




  
DX(xlocaltime)
{
#define ADD_TIME_PROP(p,v) { \
  ans=cons(NEW_NUMBER(t->v),ans); \
  ans=cons(named(p),ans); }

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

  ADD_TIME_PROP("tm_sec", tm_sec);
  ADD_TIME_PROP("tm_min", tm_min);
  ADD_TIME_PROP("tm_hour", tm_hour);
  ADD_TIME_PROP("tm_mday", tm_mday);
  ADD_TIME_PROP("tm_mon", tm_mon);
  ADD_TIME_PROP("tm_year", tm_year);
  ADD_TIME_PROP("tm_wday", tm_wday);
  ADD_TIME_PROP("tm_yday", tm_yday);
  ADD_TIME_PROP("tm_isdst", tm_isdst);
  return ans;

#undef ADD_TIME_PROP
}




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


DY(ybeep)
{
  putchar(7);
  fflush(stdout);
  return new_safe_string("beep");
}



/*
 * (bground "filename" (... listes ...) ......... (... listes ...) )
 * 
 * 1- FORK 
 * 2- Father process answers 
 * 3- Son process execute 'listes' writing
 *    filename, then quit.
 */

DY(ybground)
{
  int pid;
  File *f;
  at *fname;
  struct context *ctx;
  
  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "syntax error", NIL);
  
  fname = eval(ARG_LIST->Car);
  ifn(fname && (fname->ctype==XT_STRING))
    error("bground", "string expected as first argument", NIL);
  
  if (ask("Ok for background launch") != 1)
    error("ask", "background launch aborted", NIL);
  
  f = open_write(SADD(fname->Object), "job");
  for (ctx = context; ctx; ctx = ctx->next) {
    if (ctx->input_file)
      FFLUSH(ctx->input_file);
    if (ctx->output_file)
      FFLUSH(ctx->output_file);
  }
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  if (error_doc.script_file->stream.fp)
    FFLUSH(error_doc.script_file);
  
  
  if (pid = fork()) {		/* FATHER */
    
    if (error_doc.script_file->stream.fp) {
      set_script(NIL);
      print_string("Background task has stolen the script file\n");
    }
    file_close(f);
    return NEW_NUMBER(pid);
  } else {			/* SON */
    
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
#ifndef SUNOS
    setpgrp();
#else
    setpgrp(0,0);
#endif
    
    for (ctx = context; ctx; ctx = ctx->next) {
      ctx->input_file = NIL;
      ctx->output_file = f;
    }
    
    print_string("\n\n *** background launch ***\n");
    
    /* MAY CLOSE ALL BUT F */
    
    if (setjmp(context->error_jump)) {
      FPRINTF(f, "DEATH: %s : %s\n",
	      error_doc.error_prefix, error_doc.error_text);
      file_close(f);
      exit(10);
    }
    fname = progn(ARG_LIST->Cdr);
    
    print_string("\n\n *** end of background task ***\n");
    file_close(f);
    exit(0);
  }
}



DX(xgetenv)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(getenv(ASTRING(1)));
}



#ifdef BLORT
static char *undumpmsg = NIL;

DX(xblort)
{
  int status;
  static char blortmsg[] = "\
\nFor creating a new SN, with built-in context, type :\
\n\t\t% undump <newsn3> <sn3> core\
\nwith\
\n  <newsn3>: new name for the new executable file,\
\n     <sn3>: current filename for sn3.\n\n";
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (ask("Are you sure you want to 'blort'") != 1)
    error("ask", "blort aborted", NIL);
  if (ask("Are you sure you want to quit sn3") != 1)
    error("ask", "blort aborted", NIL);	
  
  print_string(blortmsg);
  
  undumpmsg = ASTRING(1);
  signal(SIGQUIT, SIG_DFL);
  kill(getpid(), SIGQUIT);
  sleep(1);
  error(NIL, "missed !!", NIL);
}

#endif

#ifdef APOLLO

#include "/sys/ins/base.ins.c"
#include "/sys/ins/pad.ins.c"

void 
xdmc(s)
  char *s;
{
  status_$t status;
  
  pad_$dm_cmd(stream_$stdout, *s, (short) (strlen(s)), status);
  if (status.all != status_$ok)
    error(NIL, "problem in DM command", new_safe_string(s));
}

DX(xxdmc)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  xdmc(ASTRING(1));
  return NIL;
}

#endif



/* 
 * MACHINE DEPENDANT INITIALISATION,
 * INTERRUPTIONS HANDLING.
 */


/*
 * init_unix It's the first initialisation routine called.
 */

static void 
quit_irq()
{
  signal(SIGQUIT, quit_irq);
  error(NIL, "user quit", NIL);
}

static void 
fpe_irq(sig,code,scp)
  unsigned int sig,code;
  struct sigcontext *scp;
{
  char buffer[80];
#ifndef SUNOS
  sprintf(buffer,"Floating exception SIGFPE:%02x@",code);
#else
  sprintf(buffer,"Floating exception SIGFPE:%02x@%x",code,scp->sc_pc);
#endif
  signal(SIGFPE, fpe_irq);
  if (dh_trace_root) {
      printf("** In compiled code:\n");
      print_dh_trace_stack();
      dh_trace_root = 0;
  }
  error(NIL, buffer, NIL);
}

static void 
break_irq()
{
  signal(SIGINT, break_irq);
  break_attempt = 1;
}

static void
severe_irq(sig)
  int sig;
{
  char buffer[80];
#ifdef SIGILL
  signal(SIGILL, severe_irq);
#endif
#ifdef SIGBUS
  signal(SIGBUS, severe_irq);
#endif
#ifdef SIGSEGV
  signal(SIGSEGV, severe_irq);
#endif
#ifdef SIGSYS
  signal(SIGSYS, severe_irq);
#endif
  set_malloc_file(NULL);
  sprintf(buffer,"*** Signal %d has occured",sig);
  lastchance(buffer);
}


INIT_MACHINE
{
  /* initialize the filter table */
#ifdef MYPOPEN
  {
    int ii;
#ifdef SOLARIS
    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    tabsiz = rlp.rlim_cur;
#else
    tabsiz = getdtablesize();
#endif
    kidpid = (int*) malloc(tabsiz*sizeof(int));
    exstat = (int*) malloc(tabsiz*sizeof(int));
    for (ii = 0; ii < tabsiz; ii++)  {
      kidpid[ii] = -1;
      exstat[ii] = -1;
    }
  }
#endif
  /* catch fatal signals */
  {
#ifdef SIGINT
    signal(SIGINT, break_irq);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, quit_irq);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGILL
    signal(SIGILL, severe_irq);
#endif
#ifdef SIGBUS
    signal(SIGBUS, severe_irq);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, severe_irq);
#endif
#ifdef SIGSYS
    signal(SIGSYS, severe_irq);
#endif
#ifdef PARANOIAC_IEEE
#ifndef SUNOS
    signal(SIGFPE, fpe_irq);
#else
    ieee_handler("set","common",fpe_irq);
#endif
#endif
  }
  
  /* sets the random number seed */
  Fseed(time(NULL));
  
#ifdef BLORT
  /* is that a undumped SN? */
  {
    extern char *undumpmsg;
    extern struct context first_context;
    
    if (undumpmsg) {
      error_doc.this_call = NIL;
      context = &first_context;
      context->next = NIL;
      context->input_file = new_real_File(stdin,"r");
      context->output_file = new_real_File(stdout,"w");
      context->input_tab = context->output_tab = 0;
      error(NIL, undumpmsg, NIL);
    }
  }
#endif
  
  
  dx_define("chdir", xchdir);
  dx_define("dir-existsp", xdir_existsp);
  dx_define("mkdir", xmkdir);
  dx_define("tmpnam", xtmpnam);
  dx_define("getpid", xgetpid);
  dx_define("sys", xsys);
  dx_define("sysio", xsysio);
  dy_define("bground", ybground);
  dy_define("time", ytime);
  dx_define("ctime", xctime);
  dx_define("localtime", xlocaltime);
  dx_define("sleep", xsleep);
  dy_define("beep", ybeep);
  dx_define("getenv", xgetenv);
  dx_define("filteropen", xfilteropen);
  dx_define("pipe", xpipe);
#ifdef SOCK
  dx_define("socketopen", xsocketopen);
  dx_define("remoteopen", xremoteopen);
#endif
#ifdef BLORT
  dx_define("blort", xblort);
#endif
#ifdef APOLLO
  dx_define("xdmc", xxdmc);
#endif
}



BYEBYE_MACHINE
{
  int ii;
  char buffer[80];
  
  /* KILL ALL TEMPORARY FILES */
  
  sprintf(buffer,"/bin/rm -f /tmp/sn%05d.*",getpid());
  sys(buffer);
  
  /* KILL ALL DEPENDENT PROCESSES */
#ifdef MYPOPEN
  /* SOFT */
  for(ii=0;ii<tabsiz;ii++)
    if (kidpid[ii]>0) {
      kill(kidpid[ii],15);
    }
  /* Then HARD */
  sleep(1);
  for(ii=0;ii<tabsiz;ii++) {
    if (kidpid[ii]>0) {
      kill(kidpid[ii],2);
    }
#ifdef SOCK
    shutdown(ii,2);
#endif
  }
#endif
}



