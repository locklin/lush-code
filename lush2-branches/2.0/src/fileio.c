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

#ifdef HAVE_CONFIG_H
# include "lushconf.h"
#endif

#ifdef WIN32
# include <errno.h>
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

#ifdef UNIX
# include <errno.h>
# include <sys/stat.h>
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

at *at_path;
at *at_lushdir;

char file_name[FILELEN];
char lushdir[FILELEN];


/* --------- FILE OPERATIONS --------- */

const char *cwd(const char *s)
{
#ifdef UNIX
   if (s) {
      errno = 0;
      if (chdir(s)==-1)
         test_file_error(NULL, errno);
   }
#  ifdef HAVE_GETCWD
   assert(getcwd(string_buffer,STRING_BUFFER));
   return mm_strdup(string_buffer);
#  else
   assert(getwd(string_buffer));
   return mm_strdup(string_buffer);
#  endif
#endif
#ifdef WIN32
   char drv[2];
   if (s)
      errno = 0;
      if (_chdir(s)==-1)
         test_file_error(NULL, errno);
   drv[0]='.'; drv[1]=0;
   GetFullPathName(drv, STRING_BUFFER, string_buffer, &s);
   return mm_strdup(string_buffer);
#endif
}

DX(xchdir)
{
   if (arg_number!=0) {
      ARG_NUMBER(1);
      return NEW_STRING(cwd(ASTRING(1)));
   } else
      return NEW_STRING(cwd(NULL));
}



/** files **/

at *files(const char *s)
{
   at *ans = NIL;
   at **where = &ans;

#ifdef UNIX
   DIR *dirp = opendir(s);
   if (dirp) {
      struct dirent *d;
      while ((d = readdir(dirp))) {
         int n = NAMLEN(d);
         at *ats = make_string_of_length(n);
         char *s = (char *)String(ats);
         strncpy(s, d->d_name, n); s[n] = 0;
         *where = new_cons(ats,NIL);
         where = &Cdr(*where);
      }
      closedir(dirp);
   }
#endif

#ifdef WIN32

   struct _finddata_t info;

   if ((s[0]=='/' || s[0]=='\\') && 
       (s[1]=='/' || s[1]=='\\') && !s[2]) {
      long hfind = GetLogicalDrives();
      strcpy(info.name,"A:\\");
      for (info.name[0]='A'; info.name[0]<='Z'; info.name[0]++)
         if (hfind & (1<<(info.name[0]-'A'))) {
            *where = new_cons(new_string(info.name),NIL);
            where = &Cdr(*where);
         }
   } else if (dirp(s)) {
      *where = new_cons(new_string(".."),NIL);
      where = &Cdr(*where);
   }
   strcpy(string_buffer,s);
   char *last = string_buffer + strlen(string_buffer);
   if (last>string_buffer && last[-1]!='/' && last[-1]!='\\')
      strcpy(last,"\\*.*");
   else 
      strcpy(last,"*.*");
   long hfind = _findfirst(string_buffer, &info);
   if (hfind != -1) {
      do {
         if (strcmp(".",info.name) && strcmp("..",info.name)) {
            *where = new_cons(new_string(info.name),NIL);
            where = &Cdr(*where);
         }
      } while ( _findnext(hfind, &info) != -1 );
      _findclose(hfind);
   }
#endif
   return ans;
}

DX(xfiles)
{
   if (arg_number==0)
      return files(".");
   ARG_NUMBER(1);
   return files(ASTRING(1));
}


static int makedir(const char *s)
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
   errno = 0;
   if (makedir(ASTRING(1))!=0) 
      test_file_error(NULL, errno);
   return NIL;
}


static int deletefile(const char *s)
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
   ARG_NUMBER(1);
   errno = 0;
   if (deletefile(ASTRING(1)))
      test_file_error(NULL, errno);
   return NIL;
}


DX(xrename)
{
   ARG_NUMBER(2);
   errno = 0;
   if (rename(ASTRING(1),ASTRING(2))<0)
      test_file_error(NULL, errno);
   return NIL;
}


DX(xcopyfile)
{
   char buffer[4096];

   /* parse arguments */
   ARG_NUMBER(2);
   at *atfin = APOINTER(1);
   if (RFILEP(atfin)) {
      // ok
   } else if (STRINGP(atfin)) {
      atfin = OPEN_READ(String(atfin), NULL);
   } else
      RAISEFX("not a string or file descriptor", atfin);

   at *atfout = APOINTER(2);
   if (WFILEP(atfout)) {
      // ok
   } else if (STRINGP(atfout)) {
      atfout = OPEN_WRITE(String(atfout), NULL);
   } else
      RAISEFX("not a string or file descriptor", atfout);

   /* copy */
   FILE *fin = Gptr(atfin);
   FILE *fout = Gptr(atfout);
   int nread = 0;
   for (;;) {
      int bread = fread(buffer, 1, sizeof(buffer), fin);
      if (bread==0)
         break;
      nread += bread;
      while (bread > 0)
         bread -= fwrite(buffer, 1, bread, fout);
   }
   fflush(fout);
   /* return size */
   return NEW_NUMBER(nread);
}


/* create a locking file using O_EXCL */
   
static int lockfile(const char *filename)
{
   errno = 0;
#ifdef WIN32
   int fd = _open(filename, _O_RDWR|_O_CREAT|_O_EXCL, 0644);
#else
   int fd = open(filename, O_RDWR|O_CREAT|O_EXCL, 0644);
#endif
   if (fd<0) {
      if (errno==EEXIST)
         return 0;
      else
         test_file_error(NULL, errno);
   }
   time_t tl;
   time(&tl);
#ifdef UNIX
   {
      char hname[80];
      char *user;
      gethostname(hname,79);
      if (! (user=getenv("USER")))
      if (! (user=getenv("LOGNAME")))
         user="<unknown>";  
      sprintf(string_buffer,"created by %s@%s (pid=%d)\non %s", 
              user, hname, (int)getpid(), ctime(&tl));
  }
#endif
#ifdef WIN32
   {
      char user[80];
      char computer[80];
      int size = sizeof(user);
      if (! (GetUserName(user,&size)))
         strcpy(user,"<unknown>");
      size = sizeof(computer);
      if (! (GetComputerName(computer,&size)))
         strcpy(computer,"<unknown>");
      sprintf(string_buffer,"created by %s@%s on %s", 
              user, computer, time(&tl));
  }
#endif
   char *s = string_buffer;
   size_t n = strlen(s);
   while (n > 0) {
      ssize_t l = write(fd, s, n);
      if (l <= 0) {
	 close(fd);
	 test_file_error(NULL, errno);
      }
      s += l;
      n -= l;
   }
   close(fd);
   return 1;
}

DX(xlockfile)
{
   ARG_NUMBER(1);
   if (lockfile(ASTRING(1)))
      return t();
   else
      return NIL;
}


bool dirp(const char *s)
{
#ifdef UNIX
   struct stat buf;
   if (stat(s,&buf)==0)
      if (buf.st_mode & S_IFDIR)
         return true;
#endif
   
#ifdef WIN32

   char buffer[FILELEN];
   struct _stat buf;
   if ((s[0]=='/' || s[0]=='\\') && 
       (s[1]=='/' || s[1]=='\\') && !s[2]) 
      return false;
   if (strlen(s) > sizeof(buffer) - 4)
      RAISEF(NIL, "filename too long",NIL);
   strcpy(buffer,s);
   char *last = buffer + strlen(buffer) - 1;
   if (*last=='/' || *last=='\\' || *last==':')
      strcat(last,".");
   if (_stat(buffer,&buf)==0)
      if (buf.st_mode & S_IFDIR)
         return false;
#endif
   return false;
}

DX(xdirp)
{
   ARG_NUMBER(1);
   return dirp(ASTRING(1)) ? t() : NIL;
}


bool filep(const char *s)
{
#ifdef UNIX
   errno = 0;
   struct stat buf;
   if (stat(s,&buf)==-1) {
      if (errno!=ENOENT) {
         char *errmsg = strerror(errno);
         fprintf(stderr, "*** Warning: error in '(filep \"%s\")'\n%s\n", s, errmsg);
      }
      return false;

   } else if (buf.st_mode & S_IFDIR) 
      return false;
#endif
#ifdef WIN32
   struct _stat buf;
   if (_stat(s,&buf)==-1)
      return false;
   if (buf.st_mode & S_IFDIR) 
      return false;
#endif
   errno = 0; /* filep is used in tmpname finalizer */
   return true;
}

DX(xfilep)
{
   ARG_NUMBER(1);
   return filep(ASTRING(1)) ? t() : NIL;
}


DX(xfileinfo)
{

#ifdef UNIX
  struct stat buf;
  ARG_NUMBER(1);
  if (stat(ASTRING(1),&buf)==-1)
     return NIL;
#endif
#ifdef WIN32
  struct _stat buf;
  ARG_NUMBER(1);
  if (_stat(ASTRING(1),&buf)==-1)
     return NIL;
#endif

  at *ans = NIL;
  ans = new_cons(new_cons(named("ctime"), 
                          new_date_from_time(&buf.st_ctime, DATE_YEAR, DATE_SECOND)), 
                 ans);
  ans = new_cons(new_cons(named("mtime"), 
                          new_date_from_time(&buf.st_mtime, DATE_YEAR, DATE_SECOND)), 
                 ans);
  ans = new_cons(new_cons(named("atime"), 
                          new_date_from_time(&buf.st_atime, DATE_YEAR, DATE_SECOND)), 
                 ans);
#ifdef UNIX
  ans = new_cons(new_cons(named("gid"),   NEW_NUMBER(buf.st_gid)), ans);
  ans = new_cons(new_cons(named("uid"),   NEW_NUMBER(buf.st_uid)), ans);
  ans = new_cons(new_cons(named("nlink"), NEW_NUMBER(buf.st_nlink)), ans);
  ans = new_cons(new_cons(named("ino"),   NEW_NUMBER(buf.st_ino)), ans);
  ans = new_cons(new_cons(named("dev"),   NEW_NUMBER(buf.st_dev)), ans);
#endif  
  ans = new_cons(new_cons(named("mode"),  NEW_NUMBER(buf.st_mode)), ans);
  ans = new_cons(new_cons(named("size"),  NEW_NUMBER(buf.st_size)), ans);

  at *type = NIL;
#ifdef S_ISREG
  if (!type && S_ISREG(buf.st_mode))
     type = named("reg");
#endif
#ifdef S_ISDIR
  if (! type && S_ISDIR(buf.st_mode))
     type = named("dir");
#endif
#ifdef S_ISCHR
  if (! type && S_ISCHR(buf.st_mode))
     type = named("chr");
#endif
#ifdef S_ISBLK
  if (! type && S_ISBLK(buf.st_mode))
     type = named("blk");
#endif
#ifdef S_ISFIFO
  if (! type && S_ISFIFO(buf.st_mode))
     type = named("fifo");
#endif
#ifdef S_ISLNK
  if (! type && S_ISLNK(buf.st_mode))
    type = named("lnk");
#endif
#ifdef S_ISSOCK
  if (! type && S_ISSOCK(buf.st_mode))
     type = named("sock");
#endif
  if (type)
     ans = new_cons(new_cons(named("type"), type), ans);
  return ans;
}


/* --------- FILENAME MANIPULATION --------- */


static char *strcpyif(char *d, const char *s)
{
   if (d != s)
      return strcpy(d,s);
   return d;
}

/* expects a managed string & returns a managed string */
const char *lush_dirname(const char *fname)
{
#ifdef UNIX
   const char *s = fname;
   const char *p = 0;
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
         return mm_strdup(".");
   }
   s = fname;
   if (p-s > STRING_BUFFER-1)
      RAISEF("filename is too long",NIL);
   do {
      *q++ = *s++;
   } while (s<p);
   *q = 0;
   return mm_strdup(string_buffer);
#endif

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
            return mm_strdup("\\\\");
         *q = 0;
         return mm_strdup(string_buffer);
      } else {
         if (fname[0]=='/' || fname[0]=='\\')
            return mm_strdup("\\\\");
         else
            return mm_strdup(".");
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
   return mm_strdup(string_buffer);
#endif
}


DX(xdirname)
{
   ARG_NUMBER(1);
   return NEW_STRING(lush_dirname(ASTRING(1)));
}


/* lush_basename returns a new, managed string */
const char *lush_basename(const char *fname, const char *suffix)
{
#ifdef UNIX
   if (strlen(fname) > STRING_BUFFER-4)
      RAISEF("filename is too long",NIL);
   char *s = strrchr(fname,'/');
   if (s)
      fname = s+1;
   /* Process suffix */
   if (suffix==0 || suffix[0]==0)
      return mm_strdup(fname);
   if (suffix[0]=='.')
      suffix += 1;
   if (suffix[0]==0)
      return mm_strdup(fname);
   strcpyif(string_buffer,fname);
   int sl = strlen(suffix);
   s = string_buffer + strlen(string_buffer);
   if (s > string_buffer + sl) {
      s =  s - (sl + 1);
      if (s[0]=='.' && strcmp(s+1,suffix)==0)
         *s = 0;
   }
   return mm_strdup(string_buffer);
#endif
  
#ifdef WIN32
   const char *p = fname;
   const char *s = fname;
   /* Special cases */
   if (fname[0] && fname[1]==':') {
      strcpyif(string_buffer,fname);
      if (fname[2]==0)
         return mm_strdup(string_buffer);
      string_buffer[2] = '\\'; 
      if (fname[3]==0 && (fname[2]=='/' || fname[2]=='\\'))
         return mm_strdup(string_buffer);
   }
   /* Position p after last slash */
   while (*s) {
      if (s[0]=='\\' || s[0]=='/')
         p = s + 1;
      s++;
   }
   /* Copy into buffer */
   if (strlen(p) > STRING_BUFFER-10)
      RAISEF("filename too long",NIL);
   s = string_buffer;
   while (*p && *p!='/' && *p!='\\')
      *s++ = *p++;
   *s = 0;
   /* Process suffix */
   if (suffix==0 || suffix[0]==0)
      return mm_strdup(string_buffer);
   if (suffix[0]=='.')
      suffix += 1;
   if (suffix[0]==0)
      return mm_strdup(string_buffer);    
   int sl = strlen(suffix);
   if (s > string_buffer + sl) {
      s = s - (sl + 1);
      if (s[0]=='.' && stricmp(s+1,suffix)==0)
         *s = 0;
   }
   return mm_strdup(string_buffer);
#endif
}

DX(xbasename)
{
   if (arg_number!=1) {
      ARG_NUMBER(2)
         return NEW_STRING(lush_basename(ASTRING(1),ASTRING(2)));
   } else {
      ARG_NUMBER(1);
      return NEW_STRING(lush_basename(ASTRING(1),NULL));
   }
}

/* concat_fname returns a new, managed string */
const char *concat_fname(const char *from, const char *fname)
{
#ifdef UNIX
   if (fname && fname[0]=='/') 
      strcpyif(string_buffer,"/");
   else if (from)
      strcpyif(string_buffer,concat_fname(NULL,from));
   else
      strcpyif(string_buffer,cwd(NULL));
   char *s = string_buffer + strlen(string_buffer);
   for (;;) {
      while (fname && fname[0]=='/')
         fname++;
      if (!fname || !fname[0]) {
         while (s>string_buffer+1 && s[-1]=='/')
            s--;
         *s = 0;
         return mm_strdup(string_buffer);
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
            RAISEF("filename is too long",NIL);
      }
      *s = 0;
   }
#endif
   
#ifdef WIN32
   char  drv[4];
   /* Handle base */
   if (from)
      strcpyif(string_buffer, concat_fname(NULL,from));
   else
      strcpyif(string_buffer, cwd(NULL));
   char *s = string_buffer;
   if (fname==0)
      return mm_strdup(s);
   /* Handle absolute part of fname */
   if (fname[0]=='/' || fname[0]=='\\') {
      if (fname[1]=='/' || fname[1]=='\\') {	    /* Case "//abcd" */
         s[0]=s[1]='\\'; s[2]=0;
      } else {					    /* Case "/abcd" */
         if (s[0]==0 || s[1]!=':')
            s[0] = _getdrive() + 'A' - 1;
         s[1]=':'; s[2]='\\'; s[3]=0;
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
   for (;;) {
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
               strcpyif(string_buffer, lush_dirname(string_buffer));
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
            RAISEF("filename is too long",NIL);
         else
            *s++ = *fname++;
      *s = 0;
   }
   return mm_strdup(string_buffer);
#endif
}
    
DX(xconcat_fname)
{
   if (arg_number==1)
      return NEW_STRING(concat_fname(NULL,ASTRING(1)));
   ARG_NUMBER(2);
   return NEW_STRING(concat_fname(ASTRING(1),ASTRING(2)));
}


/* relative_fname returns a new, managed string */
const char *relative_fname(const char *from, const char *fname)
{
   from = concat_fname(NULL,from);
   int fromlen = strlen(from);
   if (fromlen > FILELEN-1)
      return NULL;
   strcpyif(file_name, from);
   from = file_name;
   fname = concat_fname(NULL,fname);
#ifdef UNIX
   if (fromlen>0 && !strncmp(from, fname, fromlen)) {
      if ( fname[fromlen]==0 )
         return mm_strdup(".");
      if (fname[fromlen]=='/')
         return mm_strdup(fname + fromlen + 1);
      if (fname[fromlen-1]=='/')
         return mm_strdup(fname + fromlen);
   }
#endif
#ifdef WIN32
   if (fromlen>3 && !strncmp(from, fname, fromlen)) {
      if ( fname[fromlen]==0 )
         return mm_strdup(".");
      if (fname[fromlen]=='/' || fname[fromlen]=='\\')
         return mm_strdup(fname + fromlen + 1);
      if (fname[fromlen-1]=='/' || fname[fromlen-1]=='\\')
         return mm_strdup(fname + fromlen);
   }
#endif
   return 0;
}

DX(xrelative_fname)
{
   ARG_NUMBER(2);
   const char *s = relative_fname(ASTRING(1), ASTRING(2));
   return s ? NEW_STRING(s) : NIL;
}




/* -------------- TMP FILES ------------- */


static struct tmpname {
   struct tmpname *next;
   char *file;
} *tmpnames = NULL;

bool finalize_tmpname(struct tmpname *tn, void *_)
{
   if (filep(tn->file)) {
      if (unlink(tn->file) == 0)
         tn->file = NULL;
      else
         return false;
   }
   return true;
}

void unlink_tmp_files(void)
{
   struct tmpname *tn = tmpnames;
   while (tn) {
      struct tmpname *tnext = tn->next;
      if (filep(tn->file)) {
         if (unlink(tn->file) == -1) {
            if (errno!=ENOENT) {
               const char *errmsg = strerror(errno);
               fprintf(stderr, "*** Warning: could not remove temp file '%s'\n%s\n",
                       tn->file, errmsg);
            }
         }
      }
      free(tn->file);
      free(tn);
      tn = tnext;
   }
}

const char *tmpname(const char *dir, const char *suffix)
{
   static int uniq = 0;
   
   /* check temp directory */
   const char *dot;
   if (! dirp(dir)) {
      RAISEF("invalid directory", NEW_STRING(dir));
   }
   if (! suffix)
      dot = suffix = "";
   else if (strlen(suffix)>32) {
      RAISEF("suffix is too long", make_string(suffix));
   } else
      dot = ".";
   
   /* searches free filename */
   char buffer[256];
   const char *tmp;
   int fd;
   errno = 0;
   do {
#ifdef WIN32
      sprintf(buffer,"lush%d%s%s", ++uniq, dot, suffix);
      tmp = concat_fname(dir, buffer);
      fd = _open(tmp, _O_RDWR|_O_CREAT|_O_EXCL, 0644);
#else
      sprintf(buffer,"lush.%d.%d%s%s", (int)getpid(), ++uniq, dot, suffix);
      tmp = concat_fname(dir, buffer);
      fd = open(tmp, O_RDWR|O_CREAT|O_EXCL, 0644);
#endif
   } while (fd<0 && errno==EEXIST);

   /* test for error and close file */
   if (fd<0)
      test_file_error(NULL, errno);
   close(fd);

   /* record temp file name */
   struct tmpname *tn = malloc(sizeof(struct tmpname));
   tn->next = tmpnames;
   tn->file = strdup(tmp);
   tmpnames = tn;
   return mm_strdup(tn->file);
}


DX(xtmpname)
{
   char tempdir[256];
#ifdef WIN32
   GetTempPath(sizeof(tempdir),tempdir);
#else
   strcpy(tempdir,"/tmp");
#endif
   switch (arg_number) {
   case 0:
      return NEW_STRING(tmpname(tempdir, NULL));
   case 1:
      return NEW_STRING(tmpname(ASTRING(1), NULL));
   case 2:
      return NEW_STRING(tmpname((APOINTER(1) ? ASTRING(1) : tempdir),
                                (APOINTER(2) ? ASTRING(2) : NULL) ));
   default:
      ARG_NUMBER(-1);
      return NIL;
   }
}


/* --------- AUTOMATIC DIRECTORY--------- */

/* return true on success */
bool init_lushdir(const char *progname)
{
#ifdef UNIX
   assert(progname);
   if (progname[0]=='/') {
      strcpy(file_name,progname);

   } else if (strchr(progname,'/')) {
      strcpy(file_name,concat_fname(NULL,progname));

   } else { /* search along path */
      char *s1 = getenv("PATH");
      for (;;) {
         if (! (s1 && *s1))
            return false;
         char *s2 = file_name;
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
   if (GetModuleFileName(GetModuleHandle(NULL), file_name, FILELEN-1)==0)
      return false;
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
         strcpy(file_name, lush_dirname(file_name));
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
         "stdenv.dump",
         "sys/stdenv.dump",
         "sys/stdenv.lshc",
         "sys/stdenv.lsh",
         "../sys/stdenv.dump",
         "../sys/stdenv.lshc",
         "../sys/stdenv.lsh",
         "../share/" PACKAGE_TARNAME "/sys/stdenv.dump",
         "../share/" PACKAGE_TARNAME "/sys/stdenv.lshc",
         "../share/" PACKAGE_TARNAME "/sys/stdenv.lsh",
         "../../sys/stdenv.dump",
         "../../sys/stdenv.lshc",
         "../../sys/stdenv.lsh",
         "../../share/" PACKAGE_TARNAME "/sys/stdenv.dump",
         "../../share/" PACKAGE_TARNAME "/sys/stdenv.lshc",
         "../../share/" PACKAGE_TARNAME "/sys/stdenv.lsh",
#ifdef DIR_DATADIR
         DIR_DATADIR "/" PACKAGE_TARNAME "/sys/stdenv.dump",
         DIR_DATADIR "/" PACKAGE_TARNAME "/sys/stdenv.lshc",
         DIR_DATADIR "/" PACKAGE_TARNAME "/sys/stdenv.lsh",
#endif
         0L,
      };
      char **st = trials;
      strcpy(file_name, lush_dirname(file_name));
      while (*st) 
      {
         const char *s = concat_fname(file_name,*st++);
#ifdef DEBUG_DIRSEARCH
         printf("D %s\n",s);
#endif
         if (filep(s))
            if (access(s,R_OK)!=-1) {
               s = lush_dirname(s);
               s = lush_dirname(s);
               strcpy(lushdir, s);
               return true;
	    }
      }
   }
   /* Failure */
   return false;
}

/* --------- SEARCH PATH ROUTINES --------- */

/* add a file-suffix to string unless it already has one */
/* return the suffixed string as a managed string        */
static const char *add_suffix(const char *q, const char *suffixes)
{
   /* Trivial suffixes */
   if (!suffixes)
      return q;
   if (suffixes[0]==0 || suffixes[0]=='|')
      return q;
   
   /* Test if there is already a suffix */
   const char *s = strrchr(q,'.');
   if (s)
      if (!strchr(s,'/'))
#ifdef WIN32
         if (!strchr(s,'\\'))
#endif
            return q;

   /* Test if this is an old style suffix */
   if (suffixes[0]!='.') {
      s = suffixes + strlen(suffixes);
      /* handle old macintosh syntax */
      if (strlen(suffixes)>5 && suffixes[4]=='@')
         suffixes += 5;
   } else {
      s = suffixes = suffixes + 1;
      while (*s && *s!='|')
         s++;
   }

   /* add suffix to file_name */
   if (strlen(q) + (s - suffixes) > FILELEN - 4)
      error(NIL,"Filename is too long",NIL);

   strcpy(file_name, q);
   char *ss = file_name + strlen(file_name);
   *ss++ = '.';
   strncpy(ss, suffixes, s - suffixes);
   ss[s - suffixes] = 0;
   return mm_strdup(file_name);
}




/* suffixed_file
 * Tests whether the name obtained by adding suffices
 * to file_name is an existing file. The first completed
 * filename of an existing file is returned and the
 * suffix completion is returned as a managed string. If
 * no file is found, suffixed_file returns NULL.
 */

static const char *suffixed_file(const char *suffices)
{
   if (!suffices) {
      /* No suffix */
      return (filep(file_name) ? mm_strdup(file_name) : NULL);

   } else {
      const char *s = suffices;
      char *q = file_name + strlen(file_name);
      /* -- loop over suffix string */
      while (s) {
         const char *r = s;
         if (*r && *r!='|' && *r!='.')
            error(NIL,"Illegal suffix specification", make_string(suffices));
         while (*r && *r!='|')
            r++;
         if (q + (r - s) + 1 > file_name + FILELEN)
            error(NIL,"File name is too long",NIL);
         strncpy(q, s, (r - s));
         *(q + (r-s)) = 0;
         s = (*r ? r+1 : NULL);
         if (filep(file_name))
            return mm_strdup(file_name);
      }
      /* -- not found */
      return NULL;  
   }
}


/*
 * Search a file
 * - first in the current directory, 
 * - then along the path. 
 * - with the specified suffixes.
 * Returns the full filename as a managed string.
 */

extern char *pname_buffer;
const char *search_file(const char *ss, const char *suffices)
{
   char s[FILELEN];
  
   if (strlen(ss)+1 > FILELEN)
      error(NIL,"File name is too long",NIL);
   strcpy(s,ss);

   /* -- search along path */
   const char *c = 0;
#ifdef UNIX
   if (*s != '/')
#endif
      if (SYMBOLP(at_path)) {
         at *q = var_get(at_path);
         
         while (CONSP(q)) {
            if (STRINGP(Car(q))) {
               c = concat_fname(String(Car(q)), s);
               if (strlen(c)+1 > FILELEN)
                  error(NIL,"File name is too long",NIL);
               strcpy(file_name, c);
               const char *sf = suffixed_file(suffices);
               if (sf) return sf;
            }
            q = Cdr(q);
         }
      }
   /* -- absolute filename or broken path */
   if (!c) {
      c = concat_fname(NULL, s);
      if (strlen(c)+1 > FILELEN)
         error(NIL,"File name is too long",NIL);
      strcpy(file_name,c);
      const char *sf = suffixed_file(suffices);
      if (sf) return sf;
   }
   /* -- fail */
   return NULL;
}


DX(xfilepath)
{
   const char *suf = "|.lshc|.snc|.tlc|.lsh|.sn|.tl";
   if (arg_number!=1){
     ARG_NUMBER(2);
     suf = (APOINTER(2) ? ASTRING(2) : NULL);
  }
   const char *ans = search_file(ASTRING(1),suf);
   return ans ? NEW_STRING(ans) : NIL;
}


/* --------------- ERROR CHECK ---------------- */

/*
 * Print the error according to variable ERRNO.
 * If a file is specified, return if there is no error on this file.
 */

int stdin_errors = 0;
int stdout_errors = 0;

void test_file_error(FILE *f, int _errno)
{
   char *s = NIL;
   char buffer[200];

   if (f) {
      if (!ferror(f)) {
         if (f == stdin)
            stdin_errors = 0;
         if (f==stdout || f==stderr)
            stdout_errors = 0;
         return;
      }
      if (f==error_doc.script_file) {
         file_close(f);
         error_doc.script_file = NIL;
         set_script(NIL);
         s = "SCRIPT";
      }
      if (f==stdin) {
         if (stdin_errors > 8)
            lush_abort("ABORT -- STDIN failure");
         else {
            clearerr(stdin);
            _errno = 0;
            stdin_errors++;
            return;
         }
      } else if (f==stdout || f==stderr) {
         if (stdout_errors > 8)
            lush_abort("ABORT -- STDOUT failure");
         else {
            clearerr(stdout);      
            clearerr(stderr);
            _errno = 0;
            stdout_errors++;
            return;
         }
      }
   } 
   if (_errno) {
      sprintf(buffer,"%s (errno=%d)",strerror(_errno),_errno);
      error(s,buffer,NIL);
   }
}


/* --------- LOW LEVEL FILE FUNCTIONS --------- */

/*
 * open_read( filename, regular_suffix )
 * opens a file for reading
 */

FILE *attempt_open_read(const char *s, const char *suffixes)
{
  /*** spaces in name ***/
  while (isspace((int)(unsigned char)*s))
     s += 1;
  strcpyif(file_name, s);
  
  if (! strcmp(s, "$stdin"))
     return stdin;
  
  /*** pipes ***/
  if (*s == '|') {
     errno = 0;
     FILE *f = popen(s + 1, "r");
     if (!f && errno==EMFILE) {
        mm_collect_now();
        errno = 0;
        f = popen(s + 1, "r");
     }
     if (f) {
        FMODE_BINARY(f);
        return f;
     } else
        return NULL;
  }
  
  /*** search and open ***/
  const char *name = search_file(s, suffixes);
  if (name) {
     errno = 0;
     FILE *f = fopen(name, "rb");
     if (!f && errno==EMFILE) {
        mm_collect_now();
        errno = 0;
        f = fopen(name, "rb");
     }
     if (f) {
        FMODE_BINARY(f);
        return f;
     }
  }
  return NULL;
}


FILE *open_read(const char *s, const char *suffixes)
{
   FILE *f = attempt_open_read(s, suffixes);
   ifn (f) {
      test_file_error(NULL, errno);
      RAISEF("cannot open file", NEW_STRING(s));
   }
   return f;
}


/*
 * open_write(filename,regular_suffix) 
 * opens a file for writing
 */

FILE *attempt_open_write(const char *s, const char *suffixes)
{
   /*** spaces in name ***/
   while (isspace((int)(unsigned char)*s))
      s += 1;
   strcpyif(file_name, s);
   
   if (! strcmp(s, "$stdout"))
      return stdout;
   if (! strcmp(s, "$stderr"))
      return stderr;

   /*** pipes ***/
   if (*s == '|') {
      errno = 0;
      FILE *f = popen(s + 1, "w");
      if (!f && errno==EMFILE) {
         mm_collect_now();
         errno = 0;
         f = popen(s + 1, "w");
      }
      if (f) {
         FMODE_BINARY(f);
         return f;
      } else
         return NULL;
   }
   
   /*** suffix ***/
   if (access(s, W_OK) == -1) {
      s = add_suffix(s, suffixes);
      strcpy(file_name, s); // why?
   }

   /*** open ***/
   errno = 0;
   FILE *f = fopen(s, "w"); 
   if (!f && errno==EMFILE) {
      mm_collect_now();
      errno = 0;
      f = fopen(s, "w");
   }
   if (f) {
      FMODE_BINARY(f);
      return f;
   }
   return NULL;
}


FILE *open_write(const char *s, const char *suffixes)
{
   FILE *f = attempt_open_write(s, suffixes);
   ifn (f) {
      test_file_error(NULL, errno);
      RAISEF("cannot open file", NEW_STRING(s));
   }
   return f;
}


/*
 * open_append( s,suffix ) 
 * opens a file for appending 
 * this file must exist before
 */

FILE *attempt_open_append(const char *s, const char *suffixes)
{
   /*** spaces in name ***/
   while (isspace((int)(unsigned char)*s))
      s += 1;
   strcpy(file_name, s);

   if (!strcmp(s, "$stdout"))
      return stdout;
   if (!strcmp(s, "$stderr"))
      return stderr;
  
   /*** pipes ***/
   if (*s == '|') {
      errno = 0;
      FILE *f = popen(s + 1, "w");
     if (!f && errno==EMFILE) {
        mm_collect_now();
        errno = 0;
        f = popen(s + 1, "w");
     }
      if (f) {
         FMODE_BINARY(f);
         return f;
      } else
         return NULL;
   }

   /*** suffix ***/
   if (access(s, W_OK) == -1) {
      s = add_suffix(s, suffixes);
      strcpy(file_name, s); // why ?
   }
  
   /*** open ***/
   errno = 0;
   FILE *f = fopen(s, "a");
   if (!f && errno==EMFILE) {
      mm_collect_now();
      errno = 0;
      f = fopen(s, "a");
   }
   if (f) {
      FMODE_BINARY(f);
      return f;
   }
   return NULL;
}


FILE *open_append(const char *s, const char *suffixes)
{
   FILE *f = attempt_open_append(s,suffixes);
   ifn (f) {
      test_file_error(NULL, errno);
      RAISEF("cannot open file", NEW_STRING(s));
   }
   return f;
}


void file_close(FILE *f)
{
   if (f!=stdin && f!=stdout && f!=stderr && f) {
      errno = 0;
      if (pclose(f)<0 && fclose(f)<0)
         test_file_error(f, errno);
   }
}


/* read4(f), write4(f,x) 
 * Low level function to read or write 4-bytes integers, 
 * Assuming sizeof(int)==4
 */
int read4(FILE *f)
{
   int i;
   errno = 0;
   int status = fread(&i, sizeof(int), 1, f);
   if (status != 1)
      test_file_error(f, errno);
   return i;
}

int write4(FILE *f, unsigned int l)
{
   errno = 0;
   int status = fwrite(&l, sizeof(int), 1, f);
   if (status != 1)
      test_file_error(f, errno);
   return l;
}

/* file_size returns the remaining length of the file
 * It causes an error when the file is not exhaustable
 */
off_t file_size(FILE *f)
{
#if HAVE_FSEEKO
   off_t x = ftello(f);
   if (fseeko(f,(off_t)0,SEEK_END))
      RAISEF("non exhaustable file (pipe or terminal ?)",NIL);
   off_t e = ftello(f);
   if (fseeko(f,(off_t)x,SEEK_SET))
      RAISEF("non rewindable file (pipe or terminal ?)",NIL);
   return e - x;
#else
   off_t x = (off_t)ftell(f);
   if(fseek(f,(long)0,SEEK_END))
      RAISEF("non exhaustable file (pipe or terminal ?)",NIL);
   off_t e = (off_t)ftell(f);
   if(fseek(f, (long)x,SEEK_SET))
      RAISEF("non rewindable file (pipe or terminal ?)",NIL);
   return e - x;
#endif
}


/* --------- FILE CLASS FUNCTIONS --------- */


static FILE *file_dispose(FILE *f)
{
   if (f != stdin && f != stdout && f) {
      if (pclose(f) < 0)
         fclose(f);
   }
   return NULL;
}

/* 
 * When an AT referencing a file is reclaimed,
 * call the file_dispose.
 */

void at_file_notify(at *p, void *_)
{
   FILE *f = (FILE *)Gptr(p);
   if (f) {
      Gptr(p) = file_dispose(f);
   }
} 

at *new_rfile(const FILE *f)
{
   at *p = new_at(rfile_class, (void *)f);
   add_notifier(p, (wr_notify_func_t *)at_file_notify, NULL);
   return p;
}

at *new_wfile(const FILE *f)
{
   at *p = new_at(wfile_class, (void *)f);
   add_notifier(p, (wr_notify_func_t *)at_file_notify, NULL);
   return p;
}

/* --------- LISP LEVEL FILE MANIPULATION FUNCTIONS --------- */

/*
 * script( filename)
 * - sets the script file as 'filename'.
 */

void set_script(const char *s)
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
   if (!arg_number) {
      set_script(NIL);
      return NIL;

   } else {
      ARG_NUMBER(1);
      set_script(ASTRING(1));
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
      return new_rfile(f);
   else
      return NIL;
}

DX(xopen_write)
{
   FILE *f;
   
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
      return new_wfile(f);
   else
      return NIL;
}

DX(xopen_append)
{
   FILE *f;
   
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
      return new_wfile(f);
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
     ifn (CONSP(ARG_LIST) && CONSP(Cdr(ARG_LIST)))
      RAISEFX("syntax error", NIL);
   
   at *fdesc = eval(Car(ARG_LIST));

   FILE *f;
   if (STRINGP(fdesc))
      f = open_read(String(fdesc), NIL);
   else if (RFILEP(fdesc))
      f = Gptr(fdesc);
   else
      RAISEFX("file name or read descriptor expected", fdesc);

   struct lush_context mycontext;
   context_push(&mycontext);
   context->input_tab = 0;
   context->input_case_sensitive = 0;
   context->input_string = 0;
   context->input_file = f;

   if (sigsetjmp(context->error_jump, 1)) {
      f = context->input_file;
      if (STRINGP(fdesc)) {
         if (f != stdin && f != stdout && f)
            if (pclose(f) < 0)
               fclose(f);
      }
      context->input_tab = -1;
      context->input_string = NULL;
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(Cdr(ARG_LIST));
   if (STRINGP(fdesc))
      file_close(context->input_file);
   
   context->input_tab = -1;
   context->input_string = NULL;
   context_pop();
   return answer;
}


DY(ywriting)
{
   ifn (CONSP(ARG_LIST) && CONSP(Cdr(ARG_LIST)))
      RAISEFX("syntax error", NIL);
   
   at *fdesc = eval(Car(ARG_LIST));

   FILE *f;
   if (STRINGP(fdesc))
      f = open_write(String(fdesc), NIL);
   else if (WFILEP(fdesc))
      f = Gptr(fdesc);
   else
      RAISEFX("file name or write descriptor expected", fdesc);

   struct lush_context mycontext;
   context_push(&mycontext);
   context->output_tab = 0;
   context->output_file = f;
   
   if (sigsetjmp(context->error_jump, 1)) {
      f = context->output_file;
      if (STRINGP(fdesc)) {
         if (f != stdin && f != stdout && f)
            if (pclose(f) < 0)
               fclose(f);
      } else
         fflush(f);
      context->output_tab = -1;
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(Cdr(ARG_LIST));
   if (STRINGP(fdesc))
      file_close(context->output_file);
   else
      fflush(context->output_file);
   context->output_tab = -1;
   context_pop();
   return answer;
}

/*
 * writing - reading - appending LISP I/O FUNCTIONS
 * (reading "filename" | filedesc ( ..l1.. ) .......ln.. ) )	
 * (writing "filename" | filedesc ( ..l1.. ) ........... ) )
 */

DY(yreading_string)
{
   ifn (CONSP(ARG_LIST) && CONSP(Cdr(ARG_LIST)))
      RAISEFX("syntax error", NIL);

   at *p = eval(Car(ARG_LIST));
   
   ifn (STRINGP(p))
      RAISEFX("string expected", p);

   struct lush_context mycontext;
   context_push(&mycontext);
   context->input_tab = 0;
   context->input_case_sensitive = 0;
   context->input_string = String(p);
   if (sigsetjmp(context->error_jump, 1)) {
      context->input_tab = -1;
      context->input_string = NULL;
      context_pop();
      siglongjmp(context->error_jump, -1L);
   }

   at *answer = progn(Cdr(ARG_LIST));
   context->input_tab = -1;
   context->input_string = NULL;
   context_pop();
   return answer;
}



DX(xread8)
{
   ARG_NUMBER(1);
   at *fdesc = APOINTER(1);
   ifn (RFILEP(fdesc))
      RAISEFX("read file descriptor expected", fdesc);
   FILE *f = Gptr(fdesc);
   return NEW_NUMBER(fgetc(f));
}





DX(xwrite8)
{
   ARG_NUMBER(2);
   at *fdesc = APOINTER(1);
   int x = AINTEGER(2);

   ifn (WFILEP(fdesc))
     RAISEFX("write file descriptor expected", fdesc);
   FILE *f = Gptr(fdesc);
   return NEW_NUMBER(fputc(x,f));
}



DX(xfsize)
{
   ARG_NUMBER(1);
   at *p;
   if (ISSTRING(1)) {
      p = OPEN_READ(ASTRING(1), NULL);
   } else {
      p = APOINTER(1);
      ifn (RFILEP(p))
         RAISEFX("not a string or read descriptor", p);
   }
   return NEW_NUMBER(file_size(Gptr(p)));
}




/* --------- INITIALISATION CODE --------- */

class_t *rfile_class, *wfile_class;

void init_fileio(char *program_name)
{
   /** SETUP PATH */
   at_path = var_define("*PATH");
   at_lushdir = var_define("lushdir");

   ifn (init_lushdir(program_name) || init_lushdir("lush2"))
      lush_abort("cannot locate library files");
#ifdef UNIX
   unix_setenv("LUSHDIR",lushdir);
#endif
   var_set(at_lushdir, make_string(lushdir));
   var_lock(at_lushdir);
   const char *s = concat_fname(lushdir, "sys");
   var_set(at_path, new_cons(NEW_STRING(s),NIL));
   
   /* setting up classes */
   rfile_class = new_builtin_class(NIL);
   rfile_class->dispose = (dispose_func_t *)file_dispose;
   rfile_class->managed = false;
   class_define("RFile", rfile_class);
   
   wfile_class = new_builtin_class(NIL);
   wfile_class->dispose = (dispose_func_t *)file_dispose;
   wfile_class->managed = false;
   class_define("WFile", wfile_class);
   
   /* DECLARE THE FUNCTIONS */
   dx_define("chdir", xchdir);
   dx_define("dirp", xdirp);
   dx_define("filep", xfilep);
   dx_define("fileinfo", xfileinfo);
   dx_define("files", xfiles);
   dx_define("mkdir", xmkdir);
   dx_define("unlink", xunlink);
   dx_define("rename", xrename);
   dx_define("copyfile", xcopyfile);
   dx_define("lockfile", xlockfile);
   dx_define("basename",xbasename);
   dx_define("dirname", xdirname);
   dx_define("concat-fname", xconcat_fname);
   dx_define("relative-fname", xrelative_fname);
   dx_define("tmpname", xtmpname);
   dx_define("filepath", xfilepath);
   dx_define("script", xscript);
   dx_define("open-read", xopen_read);
   dx_define("open-write", xopen_write);
   dx_define("open-append", xopen_append);
   dy_define("reading", yreading);
   dy_define("writing", ywriting);
   dy_define("reading-string", yreading_string);
   dx_define("read8", xread8);
   dx_define("write8", xwrite8);
   dx_define("fsize", xfsize);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
