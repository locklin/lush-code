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
 
/***********************************************************************
	TL3: (C) LYB YLC 1988
	fileio.c
        WARNING: OLD (PRE-ARNO) SLIGHTLY EDITED
        $Id: fileio.c,v 1.2 2005-11-14 19:00:24 leonb Exp $
********************************************************************** */

#include <errno.h>

#ifdef WIN32
# include <windows.h>
# include <direct.h>
# include <io.h>
# include <time.h>
# include <process.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# define access _access
# define R_OK 04
# define W_OK 02
#endif

#include "header.h"

#ifdef UNIX
# include <sys/types.h>
# include <sys/stat.h>
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
# include <fcntl.h>
# include <stdio.h>
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
# ifdef HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
# else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#   include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#   include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#   include <ndir.h>
#  endif
# endif
#endif


/* --------- VARIABLES --------- */

static char path_buffer[PATHLEN];
char *path[PATHNUMBER];
char file_name[FILELEN];


/* --------- FILE OPERATIONS --------- */



/** cwd **/

char *
cwd(char *s)
{
#ifdef UNIX
  if (s)
    {
      if (chdir(s)==-1)
	test_file_error(NULL);
    }
#ifdef HAVE_GETCWD
  return getcwd(string_buffer,STRING_BUFFER);
#else
  return getwd(string_buffer);
#endif
#endif
#ifdef WIN32
  char drv[2];
  if (s)
    if (_chdir(s)==-1)
      test_file_error(NULL);
  drv[0]='.'; drv[1]=0;
  GetFullPathName(drv, STRING_BUFFER, string_buffer, &s);
  return string_buffer;
#endif
}

DX(xchdir)
{
  if (arg_number!=0) {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    return new_string(cwd(ASTRING(1)));
  } else
    return new_string(cwd(NULL));
}



/** files **/

at *
files(char *s)
{
  at *ans = NIL;
  at **where = &ans;
  /* UNIX code */
#ifdef UNIX
  DIR *dirp;
  struct dirent *d;
  dirp = opendir(s);
  if (dirp) {
    while ((d = readdir(dirp))) {
      int n = NAMLEN(d);
      at *ats = new_string_bylen(n);
      char *s = SADD(ats->Object);
      strncpy(s, d->d_name, n); s[n] = 0;
      *where = cons(ats,NIL);
      where = &(*where)->Cdr;
    }
    closedir(dirp);
  }
#endif
  /* WIN32 code */
#ifdef WIN32
  struct _finddata_t info;
  char *last;
  long hfind;
  if ((s[0]=='/' || s[0]=='\\') && 
      (s[1]=='/' || s[1]=='\\') && !s[2]) 
  {
    hfind = GetLogicalDrives();
    strcpy(info.name,"A:\\");
    for (info.name[0]='A'; info.name[0]<='Z'; info.name[0]++)
      if (hfind & (1<<(info.name[0]-'A'))) {
       *where = cons(new_string(info.name),NIL);
       where = &(*where)->Cdr;
      }
  } else if (dirp(s)) {
    *where = cons(new_string(".."),NIL);
    where = &(*where)->Cdr;
  }
  strcpy(string_buffer,s);
  last = string_buffer + strlen(string_buffer);
  if (last>string_buffer && last[-1]!='/' && last[-1]!='\\')
    strcpy(last,"\\*.*");
  else 
    strcpy(last,"*.*");
  hfind = _findfirst(string_buffer, &info);
  if (hfind != -1) {
    do {
      if (strcmp(".",info.name) && strcmp("..",info.name)) {
       *where = cons(new_string(info.name),NIL);
       where = &(*where)->Cdr;
      }
    } while ( _findnext(hfind, &info) != -1 );
    _findclose(hfind);
  }
#endif
  /* Return */
  return ans;
}

DX(xfiles)
{
  if (arg_number==0)
    return files(".");
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return files(ASTRING(1));
}


static int 
makedir(char *s)
{
#ifdef UNIX
  return mkdir(s,0777);
#endif
#ifdef WIN32
  return _mkdir(s);
#endif
}

DX(xmkdir)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (makedir(ASTRING(1))!=0) 
    test_file_error(NULL);
  return NIL;
}


static int 
deletefile(char *s)
{
#ifdef WIN32
   if (dirp(s))
    return _rmdir(s);
   else
    return _unlink(s);
#else
   if (dirp(s))
    return rmdir(s);
   else
    return unlink(s);
#endif
}

DX(xunlink)
{
  char *s;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  s = ASTRING(1);
  if (deletefile(s))
    test_file_error(NULL);
  return NIL;
}


DX(xrename)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  if (rename(ASTRING(1),ASTRING(2))<0)
    test_file_error(NULL);
  return NIL;
}


DX(xcopyfile)
{
  FILE *fin, *fout;
  at *atfin, *atfout;
  char buffer[4096];
  int bread, nread;;
  /* parse arguments */
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  atfin = APOINTER(1);
  if (EXTERNP(atfin, &file_R_class)) {
    LOCK(atfin);
  } else if (EXTERNP(atfin, &string_class)) {
    atfin = OPEN_READ(SADD(atfin->Object),NULL);
  } else
    error(NIL,"string or file descriptor expected", atfin);
  atfout = APOINTER(2);
  if (EXTERNP(atfout, &file_W_class)) {
    LOCK(atfout);
  } else if (EXTERNP(atfout, &string_class)) {
    atfout = OPEN_WRITE(SADD(atfout->Object),NULL);
  } else
    error(NIL,"string or file descriptor expected", atfout);
  /* perform copy */
  fin = atfin->Object;
  fout = atfout->Object;
  nread = 0;
  for (;;)
  {
    bread = fread(buffer, 1, sizeof(buffer), fin);
    if (bread==0)
      break;
    nread += bread;
    while (bread > 0)
      bread -= fwrite(buffer, 1, bread, fout);
  }
  /* return size */
  UNLOCK(atfin);
  UNLOCK(atfout);
  return NEW_NUMBER(nread);
}


/* create a locking file 
 * using O_EXCL */
   
static int 
lockfile(char *filename)
{
  int fd;
  time_t tl;

#ifdef WIN32
  fd = _open(filename, _O_RDWR|_O_CREAT|_O_EXCL, 0644);
#else
  fd = open(filename, O_RDWR|O_CREAT|O_EXCL, 0644);
#endif
  if (fd<0) {
    if (errno==EEXIST)
      return 0;
    else
      test_file_error(NULL);
  }
  time(&tl);
#ifdef UNIX
  {
    char hname[80];
    char *user;
    gethostname(hname,79);
    ifn (user=getenv("USER"))
      ifn (user=getenv("LOGNAME"))
        user="<unknown>";  
    sprintf(string_buffer,"created by %s@%s (pid=%d)\non %s", 
	    user, hname, (int)getpid(), ctime(&tl));
  }
#endif
#ifdef WIN32
  {
    int size;
    char user[80];
    char computer[80];
    size = sizeof(user);
    ifn (GetUserName(user,&size))
      strcpy(user,"<unknown>");
    size = sizeof(computer);
    ifn (GetComputerName(computer,&size))
      strcpy(computer,"<unknown>");
    sprintf(string_buffer,"created by %s@%s on %s", 
	    user, computer, time(&tl));
  }
#endif
  write(fd, string_buffer, strlen(string_buffer));
  close(fd);
  return 1;
}

DX(xlockfile)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  if (lockfile(ASTRING(1)))
    return true();
  else
    return NIL;
}




/** dirp **/

int 
dirp(char *s)
{
  /* UNIX implementation */
#ifdef UNIX
  struct stat buf;
  if (stat(s,&buf)==0)
    if (buf.st_mode & S_IFDIR)
      return TRUE;
#endif
  /* WIN32 implementation (bug around) */
#ifdef WIN32
  char *last;
  char buffer[FILELEN];
  struct _stat buf;
  if ((s[0]=='/' || s[0]=='\\') && 
      (s[1]=='/' || s[1]=='\\') && !s[2]) 
    return TRUE;
  if (strlen(s) > sizeof(buffer) - 4)
    error(NIL,"Filename too long",NIL);
  strcpy(buffer,s);
  last = buffer + strlen(buffer) - 1;
  if (*last=='/' || *last=='\\' || *last==':')
    strcat(last,".");
  if (_stat(buffer,&buf)==0)
    if (buf.st_mode & S_IFDIR)
      return TRUE;
#endif
  return FALSE;
}

DX(xdirp)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return dirp(ASTRING(1)) ? true() : NIL;
}



/** filep **/

int 
filep(char *s)
{
#ifdef UNIX
  struct stat buf;
  if (stat(s,&buf)==-1)
    return FALSE;
  if (buf.st_mode & S_IFDIR) 
    return FALSE;
#endif
#ifdef WIN32
  struct _stat buf;
  if (_stat(s,&buf)==-1)
    return FALSE;
  if (buf.st_mode & S_IFDIR) 
    return FALSE;
#endif
  return TRUE;
}

DX(xfilep)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return filep(ASTRING(1)) ? true() : NIL;
}



/* --------- FILENAME MANIPULATION --------- */


/** dirname **/

char *
dirname(char *fname)
{
  /* UNIX implementation */  
#ifdef UNIX
  char *s = fname;
  char *p = 0;
  char *q = string_buffer;
  while (*s) {
    if (s[0]=='/' && s[1])
      p = s;
    s++;
  }
  if (!p) {
    if (fname[0]=='/')
      return fname;
    else
      return ".";
  }
  s = fname;
  if (p-s > STRING_BUFFER-1)
    error(NIL,"filename is too long",NIL);
  do {
    *q++ = *s++;
  } while (s<p);
  *q = 0;
  return string_buffer;
#endif

  /* WIN32 implementation */
#ifdef WIN32
  char *s, *p;
  char *q = string_buffer;
  /* Handle leading drive specifier */
  if (fname[0] && fname[1]==':') {
    *q++ = *fname++;
    *q++ = *fname++;
  }
  /* Search last non terminal / or \ */
  p = 0;
  s = fname;
  while (*s) {
    if (s[0]=='\\' || s[0]=='/')
      if (s[1] && s[1]!='/' && s[1]!='\\')
        p = s;
    s++;
  }
  /* Cannot find non terminal / or \ */
  if (p == 0) {
    if (q>string_buffer) {
      if (fname[0]==0 || fname[0]=='/' || fname[0]=='\\')
	return "\\\\";
      *q = 0;
      return string_buffer;
    } else {
      if (fname[0]=='/' || fname[0]=='\\')
	return "\\\\";
      else
	return ".";
    }
  }
  /* Single leading slash */
  if (p == fname) {
    strcpy(q,"\\");
    return string_buffer;
  }
  /* Backtrack all slashes */
  while (p>fname && (p[-1]=='/' || p[-1]=='\\'))
    p--;
  /* Multiple leading slashes */
  if (p == fname)
    return "\\\\";
  /* Regular case */
  s = fname;
  if (p-s > STRING_BUFFER-4)
    error(NIL,"filename is too long",NIL);
  do {
    *q++ = *s++;
  } while (s<p);
  *q = 0;
  return string_buffer;
#endif
}


DX(xdirname)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  return new_string(dirname(ASTRING(1)));
}



/** basename **/

char *
basename(char *fname, char *suffix)
{
  /* UNIX implementation */
#ifdef UNIX
  int sl;
  char *s;
  if (strlen(fname) > STRING_BUFFER-4)
    error(NIL,"filename is too long",NIL);
  s = strrchr(fname,'/');
  if (s)
    fname = s+1;
  /* Process suffix */
  if (suffix==0 || suffix[0]==0)
    return fname;
  if (suffix[0]=='.')
    suffix += 1;
  if (suffix[0]==0)
    return fname;
  strcpy(string_buffer,fname);
  sl = strlen(suffix);
  s = string_buffer + strlen(string_buffer);
  if (s > string_buffer + sl) {
    s =  s - (sl + 1);
    if (s[0]=='.' && strcmp(s+1,suffix)==0)
      *s = 0;
  }
  return string_buffer;
#endif
  
  /* WIN32 implementation */
#ifdef WIN32
  int sl;
  char *p = fname;
  char *s = fname;
  /* Special cases */
  if (fname[0] && fname[1]==':') {
    strcpy(string_buffer,fname);
    if (fname[2]==0)
      return string_buffer;
    string_buffer[2] = '\\'; 
    if (fname[3]==0 && (fname[2]=='/' || fname[2]=='\\'))
      return string_buffer;
  }
  /* Position p after last slash */
  while (*s) {
    if (s[0]=='\\' || s[0]=='/')
        p = s + 1;
    s++;
  }
  /* Copy into buffer */
  if (strlen(p) > STRING_BUFFER-10)
    error(NIL,"Filename too long",NIL);
  s = string_buffer;
  while (*p && *p!='/' && *p!='\\')
    *s++ = *p++;
  *s = 0;
  /* Process suffix */
  if (suffix==0 || suffix[0]==0)
    return string_buffer;
  if (suffix[0]=='.')
    suffix += 1;
  if (suffix[0]==0)
    return string_buffer;    
  sl = strlen(suffix);
  if (s > string_buffer + sl) {
    s = s - (sl + 1);
    if (s[0]=='.' && stricmp(s+1,suffix)==0)
      *s = 0;
  }
  return string_buffer;
#endif
}

DX(xbasename)
{
  ALL_ARGS_EVAL;
  if (arg_number!=1) {
    ARG_NUMBER(2)
    return new_string(basename(ASTRING(1),ASTRING(2)));
  } else {
    ARG_NUMBER(1);
    return new_string(basename(ASTRING(1),NULL));
  }
}

/** concat_fname **/

char *
concat_fname(char *from, char *fname)
{
  /* UNIX implementation */
#ifdef UNIX
  char *s;
  if (fname && fname[0]=='/') 
    strcpy(string_buffer,"/");
  else if (from)
    strcpy(string_buffer,concat_fname(NULL,from));
  else
    strcpy(string_buffer,cwd(NULL));
  s = string_buffer + strlen(string_buffer);
  for (;;) {
    while (fname && fname[0]=='/')
      fname++;
    if (!fname || !fname[0]) {
      while (s>string_buffer+1 && s[-1]=='/')
	s--;
      *s = 0;
      return string_buffer;
    }
    if (fname[0]=='.') {
      if (fname[1]=='/' || fname[1]==0) {
	fname +=1;
	continue;
      }
      if (fname[1]=='.')
	if (fname[2]=='/' || fname[2]==0) {
	  fname +=2;
	  while (s>string_buffer+1 && s[-1]=='/')
	    s--;
	  while (s>string_buffer+1 && s[-1]!='/')
	    s--;
	  continue;
	}
    }
    if (s==string_buffer || s[-1]!='/')
      *s++ = '/';
    while (*fname!=0 && *fname!='/') {
      *s++ = *fname++;
      if (s-string_buffer > STRING_BUFFER-10)
	error(NIL,"filename is too long",NIL);
    }
    *s = 0;
  }
#endif


  /* WIN32 implementation */
#ifdef WIN32
  char *s;
  char  drv[4];
  /* Handle base */
  if (from)
    strcpy(string_buffer, concat_fname(NULL,from));
  else
    strcpy(string_buffer, cwd(NULL));
  s = string_buffer;
  if (fname==0)
    return s;
  /* Handle absolute part of fname */
  if (fname[0]=='/' || fname[0]=='\\') {
    if (fname[1]=='/' || fname[1]=='\\') {	    /* Case "//abcd" */
      s[0]=s[1]='\\'; s[2]=0;
    } else {					    /* Case "/abcd" */
      if (s[0]==0 || s[1]!=':')
	s[0] = _getdrive() + 'A' - 1;
      s[1]=':'; s[2]= 0;
    }
  } else if (fname[0] && fname[1]==':') {
    if (fname[2]!='/' && fname[2]!='\\') {	    /* Case "x:abcd"   */
      if ( toupper((unsigned char)s[0])!=toupper((unsigned char)fname[0]) || s[1]!=':') {
	drv[0]=fname[0]; drv[1]=':'; drv[2]='.'; drv[3]=0;
	GetFullPathName(drv, STRING_BUFFER, string_buffer, &s);
        s = string_buffer;
      }
      fname += 2;
    } else if (fname[3]!='/' && fname[3]!='\\') {   /* Case "x:/abcd"  */
	s[0]=toupper((unsigned char)fname[0]); s[1]=':'; s[2]='\\'; s[3]=0;
	fname += 2;
    } else {					    /* Case "x://abcd" */
	s[0]=s[1]='\\'; s[2]=0;
	fname += 2;
    }
  }
  /* Process path components */
  for (;;)
  {
    while (*fname=='/' || *fname=='\\')
      fname ++;
    if (*fname == 0)
      break;
    if (fname[0]=='.') {
      if (fname[1]=='/' || fname[1]=='\\' || fname[1]==0) {
	fname += 1;
	continue;
      }
      if (fname[1]=='.')
        if (fname[2]=='/' || fname[2]=='\\' || fname[2]==0) {
	  fname += 2;
	  strcpy(string_buffer, dirname(string_buffer));
	  s = string_buffer;
	  continue;
      }
    }
    while (*s) 
      s++;
    if (s[-1]!='/' && s[-1]!='\\')
      *s++ = '\\';
    while (*fname && *fname!='/' && *fname!='\\')
      if (s-string_buffer > STRING_BUFFER-10)
	error(NIL,"Filename is too long",NIL);
      else
        *s++ = *fname++;
    *s = 0;
  }
  return string_buffer;
#endif
}
    
DX(xconcat_fname)
{
  ALL_ARGS_EVAL;
  if (arg_number==1)
    return new_string(concat_fname(NULL,ASTRING(1)));
  ARG_NUMBER(2);
  return new_string(concat_fname(ASTRING(1),ASTRING(2)));
}


/* -------------- TMP FILES ------------- */


static struct tmpname {
  struct tmpname *next;
  char file[1];
} *tmpnames = 0;


void 
clean_tmp_files(void)
{
  struct tmpname *tm;
  tm = tmpnames;
  while (tm) {
    if (filep(tm->file))
      unlink(tm->file);
    tm = tm->next;
  }
}

char *
tmpname(char *dir, char *suffix)
{
  char buffer[64];
  char *tmp;
  char *dot;
  struct tmpname *tm;
  static int uniq = 0;
  int fd;

  /* check temp directory */
  if (! dirp(dir))
    error(NIL,"Invalid directory", new_string(dir));
  if (! suffix)
    dot = suffix = "";
  else if (strlen(suffix)>32)
    error(NIL,"Suffix is too long", new_string(suffix));
  else
    dot = ".";

  /* searches free filename */
  do {
#ifdef WIN32
    sprintf(buffer,"sn%d%s%s", ++uniq, dot, suffix);
    tmp = concat_fname(dir, buffer);
    fd = _open(tmp, _O_RDWR|_O_CREAT|_O_EXCL, 0644);
#else
    sprintf(buffer,"sn.%d.%d%s%s", (int)getpid(), ++uniq, dot, suffix);
    tmp = concat_fname(dir, buffer);
    fd = open(tmp, O_RDWR|O_CREAT|O_EXCL, 0644);
#endif
  }
  while (fd<0 && errno==EEXIST);
  /* test for error and close file */
  if (fd<0)
    test_file_error(NULL);
  close(fd);
  /* record temp file name */
  if (! (tm = tl_malloc(sizeof(struct tmpname)+strlen(tmp))) )
    error(NIL,"memory exhausted",NIL);
  tm->next = tmpnames;
  strcpy(tm->file,tmp);
  tmpnames = tm;
  return tm->file;
}


DX(xtmpname)
{
  char tempdir[256];
#ifdef WIN32
  GetTempPath(sizeof(tempdir),tempdir);
#else
  strcpy(tempdir,"/tmp");
#endif
  ALL_ARGS_EVAL;
  switch (arg_number)
  {
  case 0:
    return new_string(tmpname(tempdir, NULL));
  case 1:
    return new_string(tmpname(ASTRING(1), NULL));
  case 2:
    return new_string(tmpname( (APOINTER(1) ? ASTRING(1) : tempdir),
			       (APOINTER(2) ? ASTRING(2) : NULL) ));
  default:
    ARG_NUMBER(-1);
    return NIL;
  }
}


/* --------- AUTOMATIC DIRECTORY--------- */


static char *
search_sn3dir(char *progname)
{
#ifdef UNIX
  /* Finds executable under UNIX */
  if (!progname)
    abort("internal error (fileio): progname is null");
  if (progname[0]=='/') 
    {
      /* explicit absolute name */
      strcpy(file_name,progname);
    } 
  else if (strchr(progname,'/'))  
    {
      /* explicit relative name */
      strcpy(file_name,concat_fname(NULL,progname));
    } 
  else 
    {
      /* search along path */
      char *s1, *s2;
      s1 = getenv("PATH");
      for (;;) {
	ifn (s1 && *s1)
	  return 0;
	s2 = file_name;
	while (*s1 && *s1!=':')
	  *s2++ = *s1++;
	if (*s1==':')
	  s1++;
	*s2 = 0;
	strcpy(file_name, concat_fname(file_name,progname));
#ifdef DEBUG_DIRSEARCH
	printf("P %s\n",file_name);
#endif
	if (filep(file_name) && access(file_name,X_OK)==0)
	  break;
      }
    }
#endif

#ifdef WIN32
  /* Finds executable under WIN32 (program_name is unecessary) */
  if (GetModuleFileName(GetModuleHandle(NULL), file_name, FILELEN-1)==0)
    return 0;
#ifdef DEBUG_DIRSEARCH
  printf("P %s\n",file_name);
#endif
#endif

  /* Tests for symlink */
#ifdef S_IFLNK
  { 
    int len;
    char buffer[FILELEN];
    while ((len=readlink(file_name,buffer,FILELEN)) > 0) {
      buffer[len]=0;
      strcpy(file_name, dirname(file_name));
      strcpy(file_name, concat_fname(file_name,buffer));
#ifdef DEBUG_DIRSEARCH
      printf("L %s\n",file_name);
#endif
    }
  }
#endif

  /* Searches auxilliary files */
  {
    static char *trials[] = {
      "sysenv.dump",
      "lib/sysenv.dump",
      "lib/sysenv.snc",
      "lib/sysenv.sn",
      "../lib/sysenv.dump",
      "../lib/sysenv.sn",
      "../../lib/sysenv.dump",
      "../../lib/sysenv.sn",
      "../../../lib/sysenv.dump",
      "../../../lib/sysenv.sn",
      0L,
    };
    char **st = trials;
    strcpy(file_name,dirname(file_name));
    while (*st) 
      {
	char *s = concat_fname(file_name,*st++);
#ifdef DEBUG_DIRSEARCH
	printf("D %s\n",s);
#endif
	if (filep(s))
	  if (access(s,R_OK)!=-1)
	    {
	      s = dirname(s);
	      s = dirname(s);
	      return s;
	    }
      }
  }
  /* Failure */
  return NULL;
}

  



/* --------- SEARCH PATH ROUTINES --------- */


/*
 * A lisp interface to set and get the search path (path)
 * returns the list of the path directories, (path "d1" "d2" .. )
 * sets the search path.
 */
DX(xpath)
{
  ALL_ARGS_EVAL;

  if (arg_number > PATHNUMBER)
    error("path", "too many directories in path", NIL);

  if (arg_number) {
    register char *s, *d;
    register int i;
    s = path_buffer;
    for (i = 0; i < PATHNUMBER; i++)
      path[i] = NIL;
    for (i = 1; i <= arg_number; i++) {
      d = concat_fname(NULL,ASTRING(i));
      path[i - 1] = s;
      while (*d) {
	if (s < path_buffer + PATHLEN - 2)
	  *s++ = *d++;
	else {
	  path[i - 1] = NIL;
	  error("path", "too long path", NIL);
	}
      }
      *s++ = '\0';
    }
  }
  
  {
    register at **where;
    register int i;
    at *answer;
    
    answer = NIL;
    where = &answer;
    for (i = 0; i < PATHNUMBER && path[i]; i++) {
      *where = cons(new_string(path[i]), NIL);
      where = &((*where)->Cdr);
    }
    return answer;
  }
}



/*
 * add_suffix
 * - adds a suffix to variable unless
 *   variable already contains a suffix.
 *   
 */

static char *
add_suffix(char *q, char *suffixes)
{
  char *s;
  /* Trivial suffixes */
  if (!suffixes)
    return q;
  if (suffixes[0]==0 || suffixes[0]=='|')
    return q;

  /* Test if there is already a suffix */
  if ((s = strrchr(q,'.')))
    if (!strchr(s,'/'))
#ifdef WIN32
      if (!strchr(s,'\\'))
#endif
	return q;

  /* Test if this is a old style suffix */
  if (suffixes[0]!='.')
  {
    s = suffixes + strlen(suffixes);
    /* handle old macintosh syntax */
    if (strlen(suffixes)>5 && suffixes[4]=='@')
      suffixes += 5;
  }
  else
  {
    s = suffixes = suffixes + 1;
    while (*s && *s!='|')
      s++;
  }

  /* add suffix to file_name */
  if (strlen(q) + (s - suffixes) > FILELEN - 4)
    error(NIL,"Filename is too long",NIL);
  strcpy(file_name, q);
  q = file_name + strlen(file_name);
  *q++ = '.';
  strncpy(q, suffixes, s - suffixes);
  q[s-suffixes] = 0;
  return file_name;
}




/* test_suffixes
 * Tests whether the name obtained by adding suffixes
 * to file_name is an existing file. Variable file_name
 * contains the found file name on return
 */

static int
test_suffixes(char *suffixes)
{
  if (!suffixes)
  {
    /* No suffix */
    return filep(file_name);
  }
  else if (suffixes[0]!='.' && suffixes[0]!='|')
  {
    /* Old style suffix */
    char *q = file_name + strlen(file_name);
    /* -- process obsolete macintosh syntax */
    if (strlen(suffixes)>5 && suffixes[4]=='@')
      suffixes += 5;
    /* -- attempt to add suffix */
    if (strlen(file_name)+strlen(suffixes) > FILELEN - 2)
      error(NIL,"File name is too long",NIL);
    strcat(q, ".");
    strcat(q, suffixes);
    if (filep(file_name))
      return 1;
    /* -- test without suffix */
    q[0] = 0;
    return filep(file_name);
  }
  else
  {
    /* New style suffix */
    char *s = suffixes;
    char *q = file_name + strlen(file_name);
    char *r;
    /* -- loop over suffix string */
    while (s)
    {
      r = s;
      if (*r && *r!='|' && *r!='.')
        error(NIL,"Illegal suffix specification",new_string(suffixes));
      while (*r && *r!='|')
        r++;
      if (q + (r - s) > file_name + FILELEN - 1)
        error(NIL,"File name is too long",NIL);
      strncpy(q, s, (r - s));
      *(q + (r-s)) = 0;
      s = (*r ? r+1 : NULL);
      if (filep(file_name))
        return 1;
    }
    /* -- not found */
    return 0;  
  }
}


/*
 * Search a file
 * - first in the current directory, 
 * - then along the path. 
 * - with the specified suffixes.
 * Returns the full filename in a static area.
 */

char *
search_file(char *ss, char *suffixes)
{
  int i;
  char *c;
  static char s[FILELEN];

  /* -- copy ss into static buffer */
  if (strlen(ss) > FILELEN - 1)
    error(NIL,"File name is too long",NIL);
  strcpy(s,ss);
  
  /* -- search from cwd */
  c = concat_fname(NULL,s);
  if (strlen(c) > FILELEN - 1)
    error(NIL,"File name is too long",NIL);
  strcpy(file_name,c);
  if (test_suffixes(suffixes))
    return file_name;

  /* -- prune when absolute pathname */
#ifdef UNIX
  if (*s == '/')
    return NIL;
#endif

  /* -- examine path components */
  for(i=0; i<PATHNUMBER && path[i]; i++) 
  {
    c = concat_fname(path[i],s);
    if (strlen(c) > FILELEN - 1)
      error(NIL,"File name is too long",NIL);
    strcpy(file_name,c);
    if (test_suffixes(suffixes))
      return file_name;
  }
  return NIL;
}


DX(xfilepath)
{
  char *suf = ".snc|.sn|.tlc|.tl|";
  char *ans;
  ALL_ARGS_EVAL;
  if (arg_number!=1)
  {
    ARG_NUMBER(2);
    suf = (APOINTER(2) ? ASTRING(2) : NULL);
  }
  ans = search_file(ASTRING(1),suf);
  if (ans)
    return new_string(ans);
  else
    return NIL;
}



/* --------------- ERROR CHECK ---------------- */


/*
 * Print the error according to variable ERRNO.
 * If a file is specified, return if there is no error on this file.
 */

int stdin_errors = 0;
int stdout_errors = 0;

#ifndef HAVE_STRERROR
char *
strerror(int errno)
{

  extern int sys_nerr;
  extern char *sys_errlist[];

  if (errno>0 && errno<sys_nerr) 
    return sys_errlist[errno];
  return "Unknown error";
}
#endif


void 
test_file_error(FILE *f)
{
  char *s = NIL;
  char buffer[80];

  if (f && !ferror(f)) {
    if (f == stdin)
      stdin_errors = 0;
    if (f==stdout || f==stderr)
      stdout_errors = 0;
    return;
  }
  if (f && f == error_doc.script_file) {
    file_close(f);
    error_doc.script_file = NIL;
    set_script(NIL);
    s = "SCRIPT";
  }
  if (f==stdin) {
    if (stdin_errors > 8)
      abort("ABORT -- STDIN failure");
    else {
      clearerr(stdin);
      errno = 0;
      stdin_errors++;
      return;
    }
  } else if (f==stdout || f==stderr) {
    if (stdout_errors > 8)
      abort("ABORT -- STDOUT failure");
    else {
      clearerr(stdout);      
      clearerr(stderr);
      errno = 0;
      stdout_errors++;
      return;
    }
  }
  sprintf(buffer,"%s (errno=%d)",strerror(errno),errno);
  error(s,buffer,NIL);
}


/* --------- LOW LEVEL FILE FUNCTIONS --------- */






/*
 * open_read( filename, regular_suffix )
 * opens a file for reading
 */

FILE *
attempt_open_read(char *s, char *suffixes)
{
  FILE *f;
  char *name;

  /*** spaces in name ***/
  while (isspace((int)(unsigned char)*s))
    s += 1;
  strcpy(file_name, s);
  
  /*** stdin ***/
  ifn(strcmp(s, "$stdin"))
    return stdin;

  /*** pipes ***/
  if (*s == '|') {
    errno = 0;
    f = popen(s + 1, "r");
    if (f)
      return f;
    else
      return NIL;
  }

  /*** search and open ***/
  name = search_file(s,suffixes);
  if (name && ((f = fopen(name, "r"))))
    return f;
  else
    return NIL;
}



FILE *
open_read(char *s, char *suffixes)
{
  FILE *f;
  
  f = attempt_open_read(s,suffixes);
  ifn (f) {
    test_file_error(NIL);
    error(NIL,"Cannot open file",new_string(s));
  }
  return f;
}




/*
 * open_write(filename,regular_suffix) 
 * opens a file for writing
 */

FILE *
attempt_open_write(char *s, char *suffixes)
{
  FILE *f;

  /*** spaces in name ***/

  while (isspace((int)(unsigned char)*s))
    s += 1;
  strcpy(file_name, s);

  /*** stdout ***/
  ifn(strcmp(s, "$stdout"))
    return stdout;

  /*** stderr ***/
  ifn(strcmp(s, "$stderr"))
    return stderr;

  /*** demo check ***/

#ifdef DEMO
  error(NIL,"Beyond the capabilities of the demo version",NIL);
#endif


  /*** pipes ***/
  if (*s == '|') {
    errno = 0;
    if ((f = popen(s + 1, "w")))
      return f;
    else
      return NIL;
  }

  /*** suffix ***/
  if (access(s, W_OK) == -1) {
    s = add_suffix(s, suffixes);
    strcpy(file_name, s);
  }

  /*** open ***/
  if ((f = fopen(s, "w")))
    return f;
  else
    return NIL;
}


FILE *
open_write(char *s, char *suffixes)
{
  FILE *f;
  
  f = attempt_open_write(s,suffixes);
  ifn (f) {
    test_file_error(NIL);
    error(NIL,"Cannot open file",new_string(s));
  }
  return f;
}


/*
 * open_append( s,suffix ) 
 * opens a file for appending 
 * this file must exist before
 */

FILE *
attempt_open_append(char *s, char *suffixes)
{
  FILE *f;

  /*** spaces in name ***/
  while (isspace((int)(unsigned char)*s))
    s += 1;
  strcpy(file_name, s);

  /*** stdout ***/
  ifn(strcmp(s, "$stdout"))
    return stdout;
  ifn(strcmp(s, "$stderr"))
    return stderr;
  
  /*** demo check ***/
#ifdef DEMO
  error(NIL,"Beyond the capabilities of the demo version",NIL);
#endif

  /*** pipes ***/
  if (*s == '|') {
    errno = 0;
    if ((f = popen(s + 1, "w")))
      return f;
    else
      return NIL;
  }

  /*** suffix ***/
  if (access(s, W_OK) == -1) {
    s = add_suffix(s, suffixes);
    strcpy(file_name, s);
  }
  
  /*** open ***/
  if ((f = fopen(s, "a")))
    return f;
  else
    return NIL;
}


FILE *
open_append(char *s, char *suffixes)
{
  FILE *f;
  
  f = attempt_open_append(s,suffixes);
  ifn (f) {
    test_file_error(NIL);
    error(NIL,"Cannot open file",new_string(s));
  }
  return f;
}


void 
file_close(FILE *f)
{
  if (f!=stdin && f!=stdout && f!=stderr && f) 
    {
      if (pclose(f)<0 && fclose(f)<0)
	test_file_error(f);
    }
}



/* read4(f), write4(f,x) 
 * Low level function to read or write 4-bytes integers, 
 * with highest order byte at lowest address..... 
 *
 * These functions are useful for accessing 
 * file headers into machine independant files.
 */


int 
read4(FILE *f)
{
  int i;
  int status;
  status = fread(&i, sizeof(int), 1, f);
  if (status != 1)
    test_file_error(f);
  return i;
}

int 
write4(FILE *f, unsigned int l)
{
  int status;
  status = fwrite(&l, sizeof(int), 1, f);
  if (status != 1)
    test_file_error(f);
  return l;
}


/* file_size returns the remaining length of the file
 * It causes an error when the file is not exhaustable
 */
long
file_size(FILE *f)
{
  long e, x;
  x=ftell(f);
  if(fseek(f,0L,SEEK_END))
    error(NIL,"Non exhaustable file (pipe or terminal ?)",NIL);
  e=ftell(f);
  if(fseek(f,x,SEEK_SET))
    error(NIL,"Non rewindable file (pipe or terminal ?)",NIL);
  return e - x;
}


/* --------- FILE CLASS FUNCTIONS --------- */


static void 
file_dispose(at *p)
{
  FILE *f;

  f = p->Object;
  if (f != stdin && f != stdout && f) {
    if (pclose(f) < 0)
      fclose(f);
  }
  p->Object = NIL;
}

class file_R_class =
{
  file_dispose,
  generic_action,
  generic_name,
  generic_eval,
  generic_listeval,
};

class file_W_class =
{
  file_dispose,
  generic_action,
  generic_name,
  generic_eval,
  generic_listeval,
};


/* --------- LISP LEVEL FILE MANIPULATION FUNCTIONS --------- */

/*
 * script( filename)
 * - sets the script file as 'filename'.
 */

void 
set_script(char *s)
{
  if (error_doc.script_file) {
    fputs("\n\n *** End of script ***\n", error_doc.script_file);
    file_close(error_doc.script_file);
  }
  error_doc.script_file = NIL;
  error_doc.script_mode = SCRIPT_OFF;
  if (s) {
    error_doc.script_file = open_write(s, "script");
    fputs("\n\n *** Start of script ***\n", error_doc.script_file);
  }
}

DX(xscript)
{
  ifn(arg_number) {
    set_script(NIL);
    return NIL;
  }
  else
  {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    set_script(ASTRING(1));
    LOCK(APOINTER(1));
    return APOINTER(1);
  }
}


/*
 * (open_read	STRING)	   returns a file descriptor for reading 
 * (open_write  STRING)				     for writing 
 * (open_append STRING)				     for appending
 */

DX(xopen_read)
{
  FILE *f;

  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 1:
      f = attempt_open_read(ASTRING(1), NIL);
      break;
    case 2:
      f = attempt_open_read(ASTRING(1), ASTRING(2));
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
  if (f)
    return new_extern(&file_R_class, f);
  else
    return NIL;
}

DX(xopen_write)
{
  FILE *f;

  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 1:
      f = attempt_open_write(ASTRING(1), NIL);
      break;
    case 2:
      f = attempt_open_write(ASTRING(1), ASTRING(2));
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
  if (f)
    return new_extern(&file_W_class, f);
  else
    return NIL;
}

DX(xopen_append)
{
  FILE *f;

  ALL_ARGS_EVAL;
  switch (arg_number) {
    case 1:
      f = attempt_open_append(ASTRING(1), NIL);
      break;
    case 2:
      f = attempt_open_append(ASTRING(1), ASTRING(2));
      break;
    default:
      ARG_NUMBER(-1);
      return NIL;
  }
  if (f)
    return new_extern(&file_W_class, f);
  else
    return NIL;
}




/*
 * writing - reading - appending LISP I/O FUNCTIONS
 * (reading "filename" | filedesc ( ..l1.. ) .......ln.. ) )	
 * (writing "filename" | filedesc ( ..l1.. ) ........... ) )
 */

DY(yreading)
{
  struct context mycontext;
  at *answer, *fdesc;
  FILE *f;

  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "syntax error", NIL);

  fdesc = eval(ARG_LIST->Car);

  if (fdesc && fdesc->flags & X_STRING)
    f = open_read(SADD(fdesc->Object), NIL);
  else if (fdesc && (fdesc->flags & C_EXTERN) &&
	   (fdesc->Class == &file_R_class))
    f = fdesc->Object;
  else
    error("reading", "file name or read descriptor expected", fdesc);

  context_push(&mycontext);
  context->input_tab = 0;
  context->input_file = f;

  if (sigsetjmp(context->error_jump, 1)) {
    f = context->input_file;
    if (fdesc->flags & X_STRING) {
      if (f != stdin && f != stdout && f)
        if (pclose(f) == -1)
           fclose(f);
    }
    context_pop();
    UNLOCK(fdesc);
    siglongjmp(context->error_jump, -1L);
  }
  answer = progn(ARG_LIST->Cdr);

  if (fdesc->flags & X_STRING)
    file_close(context->input_file);

  context_pop();
  UNLOCK(fdesc);
  return answer;
}


DY(ywriting)
{
  struct context mycontext;
  at *answer, *fdesc;
  FILE *f;

  ifn(CONSP(ARG_LIST) && CONSP(ARG_LIST->Cdr))
    error(NIL, "syntax error", NIL);

  fdesc = eval(ARG_LIST->Car);

  if (fdesc && fdesc->flags & X_STRING)
    f = open_write(SADD(fdesc->Object), NIL);
  else if (fdesc && (fdesc->flags & C_EXTERN) &&
	   (fdesc->Class == &file_W_class))
    f = fdesc->Object;
  else
    error("writing", "file name or write descriptor expected", fdesc);

  context_push(&mycontext);
  context->output_tab = 0;
  context->output_file = f;

  if (sigsetjmp(context->error_jump, 1)) {
    f = context->output_file;
    if (fdesc->flags & X_STRING) {
      if (f != stdin && f != stdout && f)
        if (pclose(f) == -1)
           fclose(f);
    } else
      fflush(f);
    context_pop();
    UNLOCK(fdesc);
    siglongjmp(context->error_jump, -1L);
  }
  answer = progn(ARG_LIST->Cdr);

  if (fdesc->flags & X_STRING)
    file_close(context->output_file);
  else
    fflush(context->output_file);
  context_pop();
  UNLOCK(fdesc);
  return answer;
}




DX(xread8)
{
  at *fdesc;
  FILE *f;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  fdesc = APOINTER(1);
  ifn (fdesc && (fdesc->flags&C_EXTERN) && (fdesc->Class==&file_R_class)) 
    error(NIL, "read file descriptor expected", fdesc);
  f = fdesc->Object;
  return (NEW_NUMBER(fgetc(f)));
}





DX(xwrite8)
{
  at *fdesc;
  FILE *f;
  int x;

  ARG_NUMBER(2);
  ARG_EVAL(1);
  ARG_EVAL(2);
  fdesc = APOINTER(1);
  x = AINTEGER(2);

  ifn (fdesc && (fdesc->flags&C_EXTERN) && (fdesc->Class==&file_W_class)) 
    error(NIL, "write file descriptor expected", fdesc);
  f = fdesc->Object;
  return (NEW_NUMBER(fputc(x,f)));
}



DX(xfsize)
{
  at *p, *ans;

  ARG_NUMBER(1);
  ARG_EVAL(1);

  if (ISSTRING(1)) {
    p = OPEN_READ(ASTRING(1), NULL);
  } else {
    p = APOINTER(1);
    ifn (p && (p->flags & C_EXTERN) && (p->Class == &file_R_class)) 
      error(NIL, "not a string or read descriptor", p);
    LOCK(p);
  }
  
  ans = NEW_NUMBER(file_size(p->Object));
  
  UNLOCK(p);
  return ans;
}




/* --------- INITIALISATION CODE --------- */



void 
init_fileio(char *program_name)
{
  at *q;
  char *s;
  at *tldir;

  /** FIND THE INITIAL PATH */
  if (!(s=search_sn3dir(program_name)))
    abort("Cannot locate library files");
  tldir = var_define("tl3dir");
  var_set(tldir, q = new_string(s));
  UNLOCK(q);
  s = SADD(q->Object);
  path[0] = 0;
  if (s[1] || (s[0]!='.'))
    {
      path[0] = path_buffer;
      strcpy(path[0], concat_fname(s,"lib/"));
      path[1] = 0;
    }
  
  /* DECLARE THE FUNCTIONS */

  class_define("FILE_RO",&file_R_class );
  class_define("FILE_WO",&file_W_class );

  dx_define("chdir", xchdir);
  dx_define("dirp", xdirp);
  dx_define("filep", xfilep);
  dx_define("files", xfiles);
  dx_define("mkdir", xmkdir);
  dx_define("unlink", xunlink);
  dx_define("rename", xrename);
  dx_define("copyfile", xcopyfile);
  dx_define("lockfile", xlockfile);
  dx_define("basename",xbasename);
  dx_define("dirname", xdirname);
  dx_define("concat_fname", xconcat_fname);
  dx_define("tmpname", xtmpname);
  dx_define("path", xpath);
  dx_define("filepath", xfilepath);
  dx_define("script", xscript);
  dx_define("open_read", xopen_read);
  dx_define("open_write", xopen_write);
  dx_define("open_append", xopen_append);
  dy_define("reading", yreading);
  dy_define("writing", ywriting);
  dx_define("read8", xread8);
  dx_define("write8", xwrite8);
  dx_define("fsize", xfsize);
}
