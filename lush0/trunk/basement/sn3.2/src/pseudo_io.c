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


/* pseudo_io.c
	define a type File that can support both 
        regular file i/o operations
        as well as the equivalent operations 
        on a memory buffer.
*/



#include <stdio.h>
#include <varargs.h>
#include "header.h"

/* replace the C macros for feof, ferror and clearerr by equivalent functions */
int f_feof(p) FILE *p; { return feof(p); }
int f_ferror(p) FILE *p; { return ferror(p); }
void f_clearerr(p) FILE *p; { clearerr(p);  } 
int f_fileno(p) FILE *p; { return fileno(p); }
void f_rewind(p) FILE *p; { rewind(p); }
extern int f_close();
extern int myfread();
extern int myfwrite();

static io_methods file_methods = {
fwrite,
fread,
fseek,
f_rewind,
ftell,
f_close,
fflush,
fputs,
fgetc,
fputc,
f_ferror,
test_file_error,
f_feof,
fscanf,
ungetc,
f_clearerr,
f_fileno,
},
storage_methods = {
st_write, 
st_read,
st_seek,
st_rewind,
st_tell,
st_close,
st_flush,
st_fputs,
st_fgetc,
st_fputc,
st_ferror,
st_test_file_error,
st_feof,
st_scanf1,
st_ungetc,
st_clearerr,
st_fileno,
};


/* new File functions */

File *new_real_File(f, mode)
FILE *f;
char *mode;
{
	File *pf = (File *)malloc(sizeof(File));
        memset(pf,0,sizeof(File));
	pf->methods= &file_methods;
	pf->type = REAL_FILE;
	pf->stream.fp=f; return(pf);
}

File *real_fopen(fname, mode)
char *fname, *mode;
{
        FILE *popen();
	File *pf = (File *)malloc(sizeof(File));
        memset(pf,0,sizeof(File));

	pf->methods= &file_methods;
	pf->type = REAL_FILE;
        
	ifn(strcmp(fname, "$stdout"))
            pf->stream.fp=stdout;
	else ifn(strcmp(fname, "$stdin"))
            pf->stream.fp=stdin;
	else ifn(strcmp(fname, "$sterr"))
            pf->stream.fp=stderr;
#ifdef HASPIPES
	else if (*fname == '|')
            pf->stream.fp = popen(fname + 1, mode);
#endif
	else
            pf->stream.fp=fopen(fname, mode);
	if (pf->stream.fp)
            return pf;
	/* else: open must have failed! */
	free(pf);
	return NIL;
}

File *pseudo_fopen(sto, mod) /* sto is an at containing a U8 storage */
at *sto;
char *mod; 
{
	File *pf = (File *)malloc(sizeof(File));
        memset(pf,0,sizeof(File));
	pf->stream.pfp=(pfiob *)malloc(sizeof(pfiob));
	pf->methods= &storage_methods;
	pf->type = PSEUDO_FILE;
	{
		int size = ((struct storage *)(sto->Object))->srg.size;
		pfiob *iob = pf->stream.pfp;
		iob->st_at = sto;
		LOCK(sto);
		iob->mode = *mod;
		iob->pos = (*mod == 'a')? size : 0;
		iob->eof = FALSE;
	}
	return pf;
}

/* Emulate regular file operations */

int st_write(source, n, size, pf)
unsigned char *source;
int n, size;
pfiob *pf;
{
	int obj_size = n*size;
	struct storage *sto = pf->st_at->Object;
	int old_size = sto->srg.size;
	int new_posn = pf->pos + obj_size;

	if (pf->mode == 'r') 
            error(NIL,
                  "ram pseudo file: attempting to write on a storage opened for reading",NIL);
        
	if (new_posn > old_size)	/* then reallocate storage */
  	{	if (sto->srg.flags & STF_UNSIZED)
                    storage_malloc(pf->st_at,new_posn,0);
		else
                    storage_realloc(pf->st_at,new_posn,0);
	}

	/* copy source data to storage */
	{	register unsigned char *p_dest= (unsigned char *)sto->srg.data + pf->pos;
		register unsigned char *p_src= source;
		register int i;
		for (i=0;i<obj_size;i++)
			*p_dest++ = *p_src++;
	}
	pf->pos=new_posn;
	return n;
}

int st_read(dest, size, n, pf)
unsigned char *dest;
int n, size;
pfiob *pf;
{
	int obj_size = n*size;
	struct storage *sto = pf->st_at->Object;
	int sto_size = sto->srg.size;
	int new_posn = pf->pos + obj_size;

	if (pf->mode != 'r') 
		error(NIL,
			"ram pseudo file: attempting to read on a storage opened for writing",NIL);

	if (new_posn > sto_size)	
	/* one possibility is to give an error: */
	/* error(NIL,"ram pseudo file: attempting to read past the end of the storage",NIL);
	/* another possibility is just to read what's available and return the appropriate
		number of read items. For compatibility with fread, that's what we do here: */
	{
		/* new value of */ n = (sto_size - pf->pos)/size;
		/* new value of */ obj_size = n * size;
		/* new value of */ new_posn = pf->pos + obj_size;
		/* flag */ pf->eof=TRUE;
	}

	/* copy from storage to source buffer */
	{	register unsigned char *p_dest= dest;
		register unsigned char *p_src= (unsigned char *)sto->srg.data + pf->pos;
		register int i;
		for (i=0;i<obj_size;i++)
			*p_dest++ = *p_src++;
	}
	pf->pos=new_posn;
	return n;
}

int st_seek(pf, offset, ptrname)
pfiob *pf;
long offset;
int ptrname;
{
    int st_size = ((struct storage *)(pf->st_at->Object))->srg.size;
    int new_pos;
    switch (ptrname) {
    case 0:
        new_pos = offset; 
        break;
    case 1:
        new_pos = pf->pos + offset; 
        break;
    case 2:
        new_pos = st_size + offset; 
        break;
    default:
        error(NIL,"ram pseudo file: 3rd argument of st_seek should be 0,1 or 2",NIL);
        break;
    }
    if (new_pos > st_size) {
        pf->eof = TRUE;
        return -1;
    }
    pf->pos = new_pos;
    return 0;
}

void st_rewind(pf)
pfiob *pf;
{
    st_seek(pf, 0L, 0);
}


long st_tell(pf)
pfiob *pf;
{
    return pf->pos;
}

int st_close(pf)
File *pf;
{
	/* should we free the whole pseudo-file? or just unlock it and leave it to SN? */
	UNLOCK(pf->stream.pfp->st_at);
	free(pf->stream.pfp);
	free(pf);
}

int f_close(pf)
File *pf;
{
  FILE *f = pf->stream.fp;
  if (f!=stdin && f!=stdout && f!=stderr && f)
    {
#ifndef NO_IN_FLUSH
      fflush(f);
      test_file_error(f);
#endif
#ifdef HASPIPES
      if (pclose(f) < 0)
#endif
      fclose(f);
    }
  free(pf);
}

int st_flush(pf)
pfiob *pf;
{
	/* do nothing */
}

int st_fputs(s, pf)
char *s;
pfiob *pf;
{
	return st_write(s, strlen(s), 1, pf);
}

int st_fgetc(pf)
pfiob *pf;
{
	char c;
	if ((st_read(&c, 1, 1, pf)==0) && (pf->eof)) c=EOF;
	return((int)c);
}

int st_fputc(c, pf)
char c;
pfiob *pf;
{
	return st_write(&c, 1, 1, pf);
}

int st_ferror(pf)
pfiob *pf;
{
	return 0;		/* for now, since we don't have any error codes... */
}

						/* that could be improved too... */
void st_test_file_error(pf) pfiob *pf; { }

int st_feof(pf)
pfiob *pf;
{
	return pf->eof;
}

static char FPRINTF_buffer[10000];
		/* arbitrarily chosen: hopefully large enough */

int FPRINTF(va_alist)
va_dcl
{
   va_list args;
   char *format;
	int res;
   File *f;

   va_start(args);
   f = va_arg(args, File *);
   format = va_arg(args, char *);
   res=vsprintf(FPRINTF_buffer, format, args); 
   /* NOTE: this is inefficient:
      It would be better if we could estimate
      the required size for s and directly
      put the result in the storage (for pseudo-files)
      or directly call vfprintf (for real files) */
   FPUTS(FPRINTF_buffer, f);
   va_end(args);
   return res;
}

#define WHITE_SPACES " \t\n\v\r\f"

/* read a token delimited by white space or EOF */
char *read_token(f)
File *f;
{
	char *s;
	int end_of_file=FALSE,n=0;
	/* read all leading white space */
	int c=FGETC(f);
	while(!(end_of_file=FEOF(f)) && strchr(WHITE_SPACES,c))
		c=FGETC(f);
	if (end_of_file)
		return "";
	/* read all characters until white space (or EOF) */
	FPRINTF_buffer[n++]=c;
	while(!FEOF(f) && !strchr(WHITE_SPACES,c))
		FPRINTF_buffer[n++]=FGETC(f);
	FPRINTF_buffer[n++]='\0';
	s=(char *)malloc(n);
	strncpy(FPRINTF_buffer,s,n);
	return s;
}

char *st_read_token(pf)
pfiob *pf;
{
	char *s;
	int end_of_file=FALSE,n=0;
	/* read all leading white space */
	int c=st_fgetc(pf);
	while(!(end_of_file=st_feof(pf)) && strchr(WHITE_SPACES,c))
		c=st_fgetc(pf);
	if (end_of_file)
		return "";
	/* read all characters until white space (or EOF) */
	FPRINTF_buffer[n++]=c;
	c=st_fgetc(pf);
	while(!st_feof(pf) && !strchr(WHITE_SPACES,c))
	{	FPRINTF_buffer[n++]=c;
		c=st_fgetc(pf);
	}
	FPRINTF_buffer[n++]='\0';
	s=(char *)malloc(n);
	strncpy(FPRINTF_buffer,s,n);
	return s;
}

int st_scanf1(pf, format, addr)
pfiob *pf;
char *format;
ptr addr;
{
	char *s = st_read_token(pf);

	if (*s == '\0')
		return 0;

	return sscanf(s, format, addr);
}

int st_ungetc(c, pf)
char c;
pfiob *pf;
{
	if (c != EOF && pf->pos > 0)
	{
		char old_mode = pf->mode;
		pf->mode='w';
                pf->pos--;
		st_fputc(c, pf);
		pf->mode = old_mode;
                pf->pos--;
	}
	return c;
}

void st_clearerr(pf)
pfiob *pf;
{
	pf->eof=FALSE;
}


int st_fileno(pf)
pfiob *pf;
{
	error(NIL,"fileno() is not defined on ram pseudo-files!",NIL);
}

