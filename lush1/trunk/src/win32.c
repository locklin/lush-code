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
 * $Id: win32.c,v 1.1 2002-04-18 20:17:13 leonb Exp $
 **********************************************************************/
 
/************************************************************************
   win32.cpp:  OS interface file for WIN32
*********************************************************************** */

/* Include windows file */
#define VC_EXTRALEAN
#include <windows.h>

/* Include other system header files */
#include <process.h>
#include <direct.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <float.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <errno.h>

/* Include TL header files */
#include "win32.h"
#include "header.h"

/* Undefined CRT replacement functions */
#undef exit
#undef isatty
#undef popen
#undef pclose



/* ------------------------------------
 * FILE MODE STUFF
 * ------------------------------------ */

/* alter file mode on OUTPUT files */

void
win32_fmode_text(FILE *f)
{
  fflush(f);
  _setmode(_fileno(f), _O_TEXT);
}

void 
win32_fmode_binary(FILE *f)
{
  fflush(f);
  _setmode(_fileno(f), _O_BINARY);
}



/* ------------------------------------
 * SENDING MESSAGES
 * ------------------------------------ */




/* tl3port -- rendez-vous point for passing messages between TL3 and WINTL3 */

TL3PORT *tl3port = NULL;


/* set_winevent_notification -- signal event dispatch */

#define MAXWAITPROC 8

static BOOL (*waitproc[MAXWAITPROC])(MSG*);

int 
win32_waitproc(void *wproc)
{
  int i;
  for (i=0; i<MAXWAITPROC && waitproc[i]; i++) /*nop*/;
  if (i>=MAXWAITPROC)
    return FALSE;
  waitproc[i] = (BOOL(*)(MSG*)) wproc;
  return TRUE;
}


/* tl3send -- send a message to TL3 environment */

void 
tl3send(TL3MSG msgcode, TL3ARG *msgarg)
{
    DWORD status;
    if (!tl3port)
      abort("Internal error: TL3 environment not found");
    tl3port->msgcode = msgcode;
    tl3port->msgarg = msgarg;
    SetEvent(tl3port->hevSendMessage);
  again:
    {
      int i;
      MSG msg;
      while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
      {
	for (i=0; waitproc[i] && i<MAXWAITPROC; i++)
	  if ((*waitproc[i])(&msg))
	    goto again;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
      }
      status = WaitForSingleObject(tl3port->hevReplyMessage, 0);
      if (status==WAIT_OBJECT_0)
	return;
      if (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
	goto again;
      for (i=0; waitproc[i] && i<MAXWAITPROC; i++)
	if ((*waitproc[i])(NULL))
	  goto again;
      status = MsgWaitForMultipleObjects(1, 
		  &tl3port->hevReplyMessage, 
		  FALSE, INFINITE, QS_ALLINPUT );
      if (status==WAIT_OBJECT_0)
	return;
      if (status==WAIT_OBJECT_0 + 1)
	goto again;
      abort("Internal error: waiting wintl3 answer");
    }
}




/* ------------------------------------
 * EXITING
 * ------------------------------------ */

void 
win32_exit(int retcode)
{
    if (tl3port)
    {
	/* signal termination to main process */
	struct msg_simple arg_simple;
	arg_simple.status = retcode;
	tl3send(TL3_EXIT, (TL3ARG*)&arg_simple);
	_endthreadex(retcode);
    }
    else
    {
	/* normal exit sequence */
	_cexit();
	_exit(retcode);
    }
}




/* ------------------------------------
 * MAP OS ERRORS ON UNIX CODES
 * ------------------------------------ */

static struct errentry {
        unsigned long oscode;
        int errnocode;
} errtable[] = {
        {  ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
        {  ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
        {  ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
        {  ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
        {  ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
        {  ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
        {  ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
        {  ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
        {  ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
        {  ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
        {  ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
        {  ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
        {  ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
        {  ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
        {  ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
        {  ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
        {  ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
        {  ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
        {  ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
        {  ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
        {  ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
        {  ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
        {  ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
        {  ERROR_FAIL_I24,               EACCES    },  /* 83 */
        {  ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
        {  ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
        {  ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
        {  ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
        {  ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
        {  ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
        {  ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
        {  ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
        {  ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
        {  ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
        {  ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
        {  ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
        {  ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
        {  ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
        {  ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
        {  ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
        {  ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
        {  ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
        {  ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
        {  ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
	{  ERROR_DLL_INIT_FAILED,        EBADF     },  /* 1114 */
	{  ERROR_IO_DEVICE,              EIO       },  /* 1117 */
	{  ERROR_DLL_NOT_FOUND,          ENOENT    },  /* 1157 */
        {  ERROR_INVALID_DLL,            EBADF     },  /* 1154 */
        {  ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }   /* 1816 */
};

#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED

static void 
dosmaperr (unsigned long oserrno)
{
  int i;
  _doserrno = oserrno;        /* set _doserrno */
  for (i = 0; i < (sizeof(errtable)/sizeof(errtable[0])); ++i) 
    if (oserrno == errtable[i].oscode) 
    {
      errno = errtable[i].errnocode;
      return;
    }
  if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
    errno = EACCES;
  else if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
    errno = ENOEXEC;
  else
    errno = EINVAL;
}



/* ------------------------------------
 * SPAWNING PROCESSES
 * ------------------------------------ */

static int
spawn_process (char *lpCommand, 
	       HANDLE hIn, HANDLE hOut, HANDLE hErr,
	       BOOL bCommand, BOOL bDetach, BOOL bWait)
{
  PROCESS_INFORMATION sProcessInformation;
  HANDLE savIn, savOut, savErr;
  STARTUPINFO sStartupInfo;
  BOOL bOk;
  int nRet = -1;
  /* recompose command line if required */
  if (bCommand)
  {
    char *lpShell = getenv("COMSPEC");
    if (!lpShell)
      lpShell = ( (_osver & 0x8000) ? "command.com" : "cmd.exe" );
    if (strlen(lpCommand) > 1024) 
    {
      errno = EINVAL;
      return -1;
    }
    sprintf(string_buffer, "%s /c %s", lpShell, lpCommand);
    lpCommand = string_buffer;
  }
  /* create startup information */
  memset(&sStartupInfo, 0, sizeof(sStartupInfo));
  sStartupInfo.cb = sizeof(sStartupInfo);
  sStartupInfo.lpTitle = (LPSTR)lpCommand;
  /* save standard handles */
  savIn = GetStdHandle(STD_INPUT_HANDLE);
  savOut = GetStdHandle(STD_OUTPUT_HANDLE);
  savErr = GetStdHandle(STD_ERROR_HANDLE);
  /* set custom handles */
  if (hIn)
    SetStdHandle(STD_INPUT_HANDLE, hIn);
  if (hOut)
    SetStdHandle(STD_OUTPUT_HANDLE, hOut);
  if (hErr)
    SetStdHandle(STD_ERROR_HANDLE, hErr);
  /* create process */
  bOk = CreateProcess(NULL, lpCommand, NULL, NULL, TRUE,
		      (bDetach ? DETACHED_PROCESS : 0),
		      NULL, NULL,
		      &sStartupInfo,
		      &sProcessInformation );		     
  /* restore saved handles */
  SetStdHandle(STD_INPUT_HANDLE, savIn);
  SetStdHandle(STD_OUTPUT_HANDLE, savOut);
  SetStdHandle(STD_ERROR_HANDLE, savErr);
  /* check return value */
  if (!bOk)
  {
    dosmaperr(GetLastError());
    return -1;
  }
  /* wait for process if required */
  if (bWait)
  {
    DWORD dwExitCode;
    while (WaitForSingleObject(sProcessInformation.hProcess, 1000) == WAIT_TIMEOUT)
      if (break_attempt)
      {
	TerminateProcess(sProcessInformation.hProcess, 99);
	errno = EINTR;
	goto finish;
      }
    if (!GetExitCodeProcess(sProcessInformation.hProcess, &dwExitCode))
    {
      dosmaperr(GetLastError());
      goto finish;
    }
    /* return exit code */
    nRet = dwExitCode;
  }
  else
  {
    /* return zero on success */
    nRet = 0;
  }
  /* cleanup code */
finish:
  CloseHandle(sProcessInformation.hProcess);
  CloseHandle(sProcessInformation.hThread);
  return nRet;
}

FILE* 
win32_popen(char *fname, char *mode)
{
  if (!tl3port)
  {
    return _popen(fname, mode);
  }
  else
  {
    BOOL bReadP;
    FILE *pIob = NULL;
    HANDLE hRead, hWrite, hInherit;
    HANDLE hCurProc, hConsole;
    int nTmp;
    /* Find pipe direction */
    if (mode && mode[0]=='r')
      bReadP = TRUE;
    else if (mode && mode[0]=='w')
      bReadP = FALSE;
    else 
    {
      errno = EINVAL;
      return NULL;
    }
    /* Check for windows 95 */
#ifndef WIN95_BUG_FIXED
    if (_osver & 0x8000)
      error(NIL,"Windows95 bug(?) forbids using pipes from WinTL3",NIL);
#endif
    /* Create WIN32 pipe */
    hRead = hWrite = hInherit = NULL;
    if (! CreatePipe(&hRead, &hWrite, NULL, 0))
      goto err;
    /* Create inheritable handle */
    hCurProc = GetCurrentProcess();
    if (! DuplicateHandle(hCurProc, (bReadP ? hWrite : hRead), 
			  hCurProc, &hInherit, 0L, TRUE, 
			  DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE ) )
      goto err;
    /* Create client end */
    if (bReadP)
      nTmp = _open_osfhandle((long)hRead, _O_RDONLY|_O_NOINHERIT);
    else
      nTmp = _open_osfhandle((long)hWrite, _O_WRONLY|_O_NOINHERIT);
    if (nTmp < 0)
      goto err;
    pIob = fdopen(nTmp, mode);
    if (! pIob)
      goto err;
    /* Create process */
    hConsole = (tl3port ? tl3port->hfStdoutInheritable : NULL);
    if (bReadP)
      nTmp = spawn_process(fname, NULL, hInherit, hConsole, TRUE, TRUE, FALSE );
    else
      nTmp = spawn_process(fname, hInherit, hConsole, hConsole, TRUE, TRUE, FALSE );
    if (nTmp < 0)
      goto err;
    /* Cleanup */
    CloseHandle(hInherit);
    return pIob;
    /* If an error has occurred */
    err:
      dosmaperr(GetLastError());
      if (hRead)
	CloseHandle(hRead);
      if (hWrite)
	CloseHandle(hWrite);
      if (hInherit)
	CloseHandle(hInherit);
      if (pIob)
	fclose(pIob);
      return NULL;
  }
}

int 
win32_pclose(FILE *f)
{
  if (!tl3port)
    return _pclose(f);
  else
    return fclose(f);
}



/* ------------------------------------
 * OS DEPENDENT FUNCTION
 * ------------------------------------ */


/*
 * (getpid)
 * returns my process ID
 */

DX(xgetpid)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(_getpid());
}

/*
 * (getuid) (getusername)
 * returns user ID / username 
 */

DX(xgetuid)
{
  ARG_NUMBER(0);
  return NEW_NUMBER(100);
}

DX(xgetusername)
{
  unsigned long size;
  char user[80];
  ARG_NUMBER(0);
  size = sizeof(user);
  if (GetUserName(user,&size))
    return new_string(user);
  else
    return NIL;
}


/* 
 * (isatty <fdesc>)
 * tells if a file is a tty
 */

int win32_isatty(int fh)
{
  if (tl3port  &&  fh<=2)
      return TRUE;
  return _isatty(fh);
}

DX(xisatty)
{
  FILE *f;
  int fd;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  f = open_read(ASTRING(1),NIL);
  fd = win32_isatty(fileno(f));
  file_close(f);
  if (fd)
    return TLtrue();
  else
    return NIL;
}



/*
 * (sys <command>)
 * executes a shell command
 */

int
sys(char *s)
{
  int status;
  if (tl3port)
  {
    /* spawn process without waiting */
    HANDLE hInv = INVALID_HANDLE_VALUE;
    status = spawn_process(s, hInv, hInv, hInv, TRUE, FALSE, FALSE);
  }
  else
  {
    /* use standard system command */
    status = system(s);
  }
  if (status < 0)
    test_file_error(NIL);
  return status;
}

DX(xsys)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return NEW_NUMBER(sys(ASTRING(1)));
}



/*
 * (time (....listes...) (....listes...) )
 * time the execution of a program
 */

DY(ytime)
{
  register at *q;

  ifn(ARG_LIST) {
    time_t newtime;
    time(&newtime);
    return NEW_NUMBER(newtime);
  } else {
    clock_t oldtime, newtime;
    oldtime = clock();
    q = progn(ARG_LIST);
    newtime = clock();
    UNLOCK(q);
    return NEW_NUMBER((double)(newtime - oldtime)/CLOCKS_PER_SEC);
  }
}


/*
 * (ctime) 
 * returns a string for the current time
 */

DX(xctime)
{
  char *t;
  time_t tl;
  if (arg_number==0) {
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


/*
 * (localtime) 
 * returns split up time information
 */

DX(xlocaltime)
{
#define ADD_TIME_PROP(p,v) ans=cons(named(p),cons(NEW_NUMBER(t->v),ans))

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


/*
 * (sleep <seconds>)
 * Do nothing during the specified number of seconds
 */

DX(xsleep)
{
  real delay;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  delay = AREAL(1);
  while (delay >= 1.0) {
    Sleep(1000);
    delay -= 1.0;
    CHECK_MACHINE("on");
  }
  if (delay >= 0) {
    Sleep((int)(1000*delay));
    CHECK_MACHINE("on");
  }
  LOCK(APOINTER(1));
  return APOINTER(1);
}


/*
 * (beep)
 */

DX(xbeep)
{
  MessageBeep(MB_ICONEXCLAMATION);
  return new_safe_string("beep");
}


/*
 * (getenv <var>)
 * return value of environment variable
 */

DX(xgetenv)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(getenv(ASTRING(1)));
}



/* ------------------------------------
 * WIN32 SPECIFIC FUNCTIONS
 * ------------------------------------ */


/* 
 * (wintl3p)
 * Returns TRUE if running WINTL3 instead of TL3CMD
 */

DX(xwintl3p)
{
  ARG_NUMBER(0);
  if (tl3port)
    return TLtrue();
  else
    return NIL;
}


/*
 * (winedit <filename> [<untitledbuffer>])
 * Open an editor window on a given file
 */

DX(xwinedit)
{
  char *s;
  int mode = MSG_EDIT_AUTO;
  struct msg_string arg_edit;

  ALL_ARGS_EVAL;
  switch (arg_number)
  {
  case 2:
    if (APOINTER(2))
      mode = MSG_EDIT_UNNAMED;
    else 
      mode = MSG_EDIT_NAMED;
  case 1:
    s = ASTRING(1);
    break;
  default:
    ARG_NUMBER(-1);
  }
  if (!tl3port)
    error(NIL,"Not running in WinTL3",NIL);
  arg_edit.string = s;
  arg_edit.status = mode;
  tl3send(TL3_WINEDIT, (TL3ARG*)&arg_edit);
  return NEW_NUMBER(arg_edit.status);
}


/*
 * (winsys <stdin> <stdout> <stderr> <commandp> <detachp> <waitp> <cmdline>)
 * Creates process with fine options:
 * - file handles: Invalid:(), Standard:t, Pipe:symbol
 * - mode: use command interpreter, run as detached process, wait until exit
 */


DX(xwinsys)
{
  BOOL bCommand, bDetach, bWait;
  HANDLE hIn, hOut, hErr;
  HANDLE hKIn, hKOut, hKErr;
  HANDLE rPipe, wPipe;
  HANDLE curProc;
  at *arg;
  at *atf;
  at *at_true;
  at *outpipesym;
  char *cmdline;
  int fd;
  int status;
  FILE *ff;

  ALL_ARGS_EVAL;
  ARG_NUMBER(7);
  curProc = GetCurrentProcess();
  hKIn = hKOut = hKErr = NULL;
  at_true = TLtrue();
  UNLOCK(at_true);

  /* Process simple arguments */
  bCommand = (APOINTER(4) ? TRUE : FALSE);
  bDetach = (APOINTER(5) ? TRUE : FALSE);
  bWait = (APOINTER(6) ? TRUE : FALSE);
  cmdline = ASTRING(7);

  /* Process argument STDIN */
  arg = APOINTER(1);
  if (!arg) 
  {
    hIn = INVALID_HANDLE_VALUE;
  } 
  else if (arg==at_true) 
  {
    hIn = NULL;
    if (tl3port)
      error(NIL,"WinTL3 has no valid standard input handle",NIL);
  } 
  else if (EXTERNP(arg, &symbol_class))
  {
    wPipe = rPipe = NULL;
    if (!CreatePipe(&rPipe, &wPipe, NULL, 0))
      goto win32_error;
    if (!DuplicateHandle(curProc, rPipe, curProc, &hKIn, 0, TRUE, 
			 DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE) )
    {
      CloseHandle(wPipe);
      goto win32_error;
    }
    fd = _open_osfhandle((long)wPipe, _O_WRONLY|_O_NOINHERIT);
    ff = fdopen(fd, "w");
    atf = new_extern(&file_W_class, ff);
    var_set(arg, atf);
    UNLOCK(atf);
    hIn = hKIn;
  }
  else
    error(NIL,"Illegal argument for defining stdin",arg);

  /* Process argument STDOUT */
  arg = APOINTER(2);
  if (!arg) 
  {
    hOut = INVALID_HANDLE_VALUE;
  } 
  else if (arg==at_true) 
  {
    hOut = NULL;
    if (tl3port)
      hOut = tl3port->hfStdoutInheritable;
  } 
  else if (EXTERNP(arg, &symbol_class))
  {
    outpipesym = arg;
    wPipe = rPipe = NULL;
    if (!CreatePipe(&rPipe, &wPipe, NULL, 0))
      goto win32_error;
    if (!DuplicateHandle(curProc, wPipe, curProc, &hKOut, 0, TRUE,  
			 DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE) )
    {
      CloseHandle(rPipe);
      goto win32_error;
    }
    fd = _open_osfhandle((long)rPipe, _O_RDONLY|_O_NOINHERIT);
    ff = fdopen(fd, "r");
    atf = new_extern(&file_R_class, ff);
    var_set(arg, atf);
    UNLOCK(atf);
    hOut = hKOut;
  }
  else
    error(NIL,"Illegal argument for defining stdout",arg);

  /* Process argument STDERR */
  arg = APOINTER(3);
  if (!arg) 
  {
    hErr = INVALID_HANDLE_VALUE;
  } 
  else if (arg==at_true) 
  {
    hErr = NULL;
    if (tl3port)
      hErr = tl3port->hfStdoutInheritable;
  } 
  else if (arg == outpipesym)
  {
    hErr = hOut;  /* same pipe for stdin and stdout */
  }
  else if (EXTERNP(arg, &symbol_class))
  {
    wPipe = rPipe = NULL;
    if (!CreatePipe(&rPipe, &wPipe, NULL, 0))
      goto win32_error;
    if (!DuplicateHandle(curProc, wPipe, curProc, &hKErr, 0, TRUE,  
			 DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE) )
    {
      CloseHandle(rPipe);
      goto win32_error;
    }
    fd = _open_osfhandle((long)rPipe, _O_RDONLY|_O_NOINHERIT);
    ff = fdopen(fd, "r");
    atf = new_extern(&file_R_class, ff);
    var_set(arg, atf);
    UNLOCK(atf);
    hErr = hKErr;
  }
  else
    error(NIL,"Illegal argument for defining stdout",arg);

  /* spawn process */
  status = spawn_process(cmdline, hIn, hOut, hErr, bCommand, bDetach, bWait);

  /* close inherited pipe handles */
  if (hKIn) CloseHandle(hKIn);
  if (hKOut) CloseHandle(hKOut);
  if (hKErr) CloseHandle(hKErr);

  /* check error and return */
  CHECK_MACHINE("on");
  if (status < 0)
    test_file_error(NULL);
  return NEW_NUMBER(status);

  /* error handling */
win32_error:
  dosmaperr(GetLastError());
  test_file_error(NULL);
  error(NIL,"Unknown WIN32 error", NIL);
  return NIL;
}



/* (win-show-workbench <flag>)
 * Control display of main wintl3 window
 */

DX(xwin_show_workbench)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (tl3port)
  {
    struct msg_simple arg;
    arg.status = ( APOINTER(1) ? TRUE : FALSE );
    tl3send(TL3_SHOWWORKBENCH, (TL3ARG*)&arg);
    return TLtrue();
  }
  return NIL;
}


/* ------------------------------------
 * WIN32 DLL FUNCTIONS
 * ------------------------------------ */








static struct dlrecord {
  struct dlrecord *next;
  HINSTANCE hndl;
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
  char *fname, *cname;
  char  modname[256];
  char *path;
  HINSTANCE hndl;
  int   (*init)(int,int);
  struct dlrecord *rec;
  at *ans;
  
  ARG_NUMBER(1);
  ARG_EVAL(1);
  /* Get filename */
  fname = ASTRING(1);
  cname = concat_fname(NULL,fname);
  /* Add dll directory to path used for searching DLLs */
  path = getenv("PATH");
  if (path)
  {
    char *s, *buffer;
    buffer = (char*)tl_malloc(strlen(cname)+strlen(path)+8);
    strcpy(buffer,"PATH=");
    strcat(buffer,cname);
    s = strrchr(buffer,'\\');
    if (s)
    {
      s[0] = 0;
      strcat(buffer,";");
      strcat(buffer,path);
      _putenv(buffer);
      strcpy(buffer,"PATH=");
      strcat(buffer,path);
    }
    path = buffer;
  }
  /* Load library */
  hndl = LoadLibrary(cname);
  if (!hndl)
    hndl = LoadLibrary(fname);
  /* Restore path */
  if (path)
  {
    _putenv(path);
    free(path);
  }
  /* Check for error */
  if (!hndl)
    error(NIL,"Dynamic linker cannot load this DLL",APOINTER(1));
  /* Get Instance name */
  if (GetModuleFileName(hndl, modname, sizeof(modname)) == 0)
  {
    dosmaperr(GetLastError());
    test_file_error(NULL);
  }

  /* Check that DLL has not been loaded yet */
  for (rec=dlfirst; rec; rec=rec->next)
    if (hndl==rec->hndl || !strcmp(modname, rec->name))
      break;
  if (rec)
  {
    FreeLibrary(hndl);
    error(NIL,"Module is already loaded", new_string(modname));
  }

  /* Find init symbol */
  init = (int(*)(int,int))GetProcAddress(hndl, "init_user_dll");
  if (!init)
  {
    FreeLibrary(hndl);
    error(NIL,"Cannot locate function 'init_user_dll'",APOINTER(1));
  }
  /* Calling init function */
  switch ( (*init)(TLOPEN_MAJOR,TLOPEN_MINOR) )
  {
  case -1:
    FreeLibrary(hndl);
    error(NIL,"Module initialization has failed",APOINTER(1));
    break;
  case -2:
    FreeLibrary(hndl);
    error(NIL,"Module is not compiled for this version of TL3",APOINTER(1));
    break;
  default:
    break;
  }
  /* Prepare record and return */
  rec = (struct dlrecord*)
	  tl_malloc(sizeof(struct dlrecord) + strlen(modname));
  if (!rec)
  {
    FreeLibrary(hndl);
    error(NIL,"Out of memory",NIL);
  }
  rec->next = dlfirst;
  rec->hndl = hndl;
  strcpy(rec->name, modname);
  dlfirst = rec;
  ans = new_string(modname);
  return ans;
}



/* ------------------------------------
 * TRAPPING ERRORS
 * ------------------------------------ */

/* flag for control-C interrupt */
int break_attempt;
int kill_attempt;


/* called when testing break_attempt */

void win32_user_break(char *s)
{
  if (kill_attempt)
  {
    /* acknowldge */
    kill_attempt = 0;
    /* exit */
    FMODE_TEXT(stderr);
    fprintf(stderr,"\nTlisp exiting...\n");
    clean_up();
    win32_exit(0);
  }
  user_break(s);
}



/* Called (really?) when everything goes badly */
void 
lastchance(char *s)
{
  at *q;
  static char reason[128];
  
  /* Test for recursive call */
  if (!s || !*s)
    s = "Unknown reason";
  if (strcmp(s,reason) == 0)
    {
      abort(reason);
      _exit(100);
    }
  if (reason[0]==0)
    strcpy(reason,s);
  
  /* Signal problem */
  error_doc.ready_to_an_error = FALSE;
  FMODE_TEXT(stderr);
  fprintf(stderr, "\n\007**** GASP: %s\n", reason);
  FMODE_BINARY(stderr);
  q = eval(named("toplevel"));
  if (isatty(0) && EXTERNP(q,&de_class))
    {
      FMODE_TEXT(stderr);
      fprintf(stderr,"**** GASP: Critical data structures are damaged\n");
      fprintf(stderr,"**** GASP: Save your work immediatly !\n\n");
      FMODE_BINARY(stderr);
      /* Sanitize IO */
      break_attempt = 0;      
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
      FMODE_TEXT(stderr);
      fprintf(stderr,"**** GASP: %s", ctime(&clock));
      FMODE_BINARY(stderr);
    }
  abort(reason);
}

/* user manipulates console */
static BOOL WINAPI 
close_capture (DWORD CtrlType)
{
    switch (CtrlType)
    {
    case CTRL_CLOSE_EVENT:
      clean_up();
      exit(0);
      return TRUE;
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      break_attempt = 1;
      return TRUE;
    default:
      return FALSE;
    }
}



/* Exception filter called by WinTL3 */
int 
tl3exception(LPEXCEPTION_POINTERS ExceptionInfo)
{
  DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  if (error_doc.ready_to_an_error)
  {
#ifdef _M_IX86_DISABLED
    if (code == EXCEPTION_STACK_OVERFLOW)
    {
      fprintf(stderr,"\n*** Stack overflow\n\a\n");
      ExceptionInfo->ContextRecord->Eax =
        ExceptionInfo->ContextRecord->Esp;
      ExceptionInfo->ContextRecord->Esp =
        ExceptionInfo->ContextRecord->Esp + 0x2000;
      ExceptionInfo->ContextRecord->Eip = 
        (DWORD)stack_irq;
      return EXCEPTION_CONTINUE_EXECUTION;
    }
#endif
  }
  return EXCEPTION_CONTINUE_SEARCH;
}


/* unhandled exception */
static long CALLBACK
gasp_irq(LPEXCEPTION_POINTERS ExceptionInfo)
{
  char buffer[128];
  DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  /* Enter lastchance mode */
  if (code == EXCEPTION_STACK_OVERFLOW)
    sprintf(buffer,"Stack overflow exception %lx occurred",code);
  else
    sprintf(buffer,"Unhandled exception %lx occurred",code);
  error_doc.ready_to_an_error = FALSE;
  lastchance(buffer);
  return EXCEPTION_EXECUTE_HANDLER;
}



/* ------------------------------------
 * READING CONSOLE AND HANDLING EVENTS
 * ------------------------------------ */


/* tl3event_add, tl3event_test -- normally implemented in win_driver.c */

#ifdef NOGRAPHICS

BOOL tl3event_test(void) 
{ return FALSE; }

BOOL tl3event_add(void *wid, int code, int arg, int x, int y)
{ return TRUE; }

#endif


/* trigger_handle -- function called when trigger is run */

static void (*trigger_handler)(void);


/* trigger_handle -- function called after trigger is run by GETLINE */

static void (*trigger_process)(void);


/* set_trigger -- set event callback functions */

void set_trigger(void (*trigger)(void), void (*process)(void))
{
  if (tl3port)
  {
    trigger_handler = trigger;
    trigger_process = process;
  }
}



/* call_sync_trigger -- read events and enqueue them */

void 
call_sync_trigger(void)
{
  if (tl3port)
  {
    int i;
    MSG msg;
  again:
    /* Process Win32 messages */
    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
      for (i=0; waitproc[i] && i<MAXWAITPROC; i++)
	if ((*waitproc[i])(&msg))
	  goto again;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    /* Call trigger */
    if (trigger_handler)
      (*trigger_handler)();
  }
}


/* wait_trigger -- wait until an event occurs */

void 
wait_trigger(void)
{
  if (tl3port)
  {
    struct msg_simple arg_simple;
    arg_simple.status = 0;
    tl3send(TL3_WAITEVENT, (TL3ARG*)&arg_simple);
  }
  else
  {
    /* do not hog computer */ 
    Sleep(1000);
  }
}


/* console_getline -- reads a line on the console */

void
console_getline(char *prompt, char *buf, int size)
{
  if (tl3port)
  {
      struct msg_string arg_getline;
      fputs(prompt,stdout);
      if (trigger_process)
        (*trigger_process)();
      for (;;)
      {
	tl3send(TL3_GETLINE, (TL3ARG*)&arg_getline);
	if (arg_getline.status == MSG_GETLINE_EOF)
	{
  	  strncpy(buf, arg_getline.string, size-2);
	  stdin->_flag |= _IOEOF;
	  return;
	}
	if (arg_getline.status  == MSG_GETLINE_NORMAL)
	{
  	  strncpy(buf, arg_getline.string, size-2);
	  stdin->_flag &= ~_IOEOF;
	  return;
	}
	if (trigger_process)
	  (*trigger_process)();
      }
  }
  else
  {
      fputs(prompt,stdout);
      if (break_attempt)
	return;
      if (feof(stdin))
	return;
      fgets(buf,size-2,stdin);
  }
}



/* ------------------------------------
 * INITIALISATION FUNCTION
 * ------------------------------------ */

#ifdef __cplusplus
extern "C" 
#endif
void
init_win32(void)
{
  /* Set file mode */
  _fmode = _O_BINARY;
  _setmode(_fileno(stdin),_O_BINARY);
  _setmode(_fileno(stdout),_O_BINARY);
  _setmode(_fileno(stderr),_O_BINARY);

  /* Set console mode (if using a console) */
  if (!tl3port && _isatty(0)) 
  {
    SetConsoleCP(GetACP()); /* noop on win95 */
    SetConsoleOutputCP(GetACP()); /* noop on win95 */
    SetConsoleCtrlHandler(close_capture, TRUE);
  }

  /* Set exception handlers */
  SetUnhandledExceptionFilter(gasp_irq);

  /* Initialization */
  Fseed(time(NULL));
  dx_define("getpid", xgetpid);
  dx_define("getuid", xgetuid);
  dx_define("getusername", xgetusername);
  dx_define("isatty", xisatty);
  dx_define("sys", xsys);
  dy_define("time", ytime);
  dx_define("ctime", xctime);
  dx_define("localtime", xlocaltime);
  dx_define("sleep", xsleep);
  dx_define("beep", xbeep);
  dx_define("getenv", xgetenv);
  dx_define("wintl3p", xwintl3p);
  dx_define("winedit", xwinedit);
  dx_define("winsys", xwinsys);
  dx_define("win_show_workbench", xwin_show_workbench);
  dx_define("mod_load", xmod_load);
  dx_define("mod_list", xmod_list);
}
