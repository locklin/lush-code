/***********************************************************************
  SN3: Heir of the SN family
  (LYB) 91
  ******************************************************************/

#include "header.h"
#include "dh.h"


extern void exit();

#define NOLISP

/*
 * INCLUDE SOME FILES FROM SN SOURCE
 */

#include "../src/check_func.c"
#include "../src/fltlib.c"
#include "../src/nr.c"

/*
 * ERROR CODES
 */

void
run_time_error(s)
     char* s;
{
  print_dh_trace_stack();
  fprintf(stderr,"ERROR: %s\n", s);
  exit(10);
}

void
error(char *prefix, char *s, void *atptr)
{
  print_dh_trace_stack();
  fprintf(stderr,"ERROR %s: %s\n", (prefix ? prefix : ""), s);
  exit(10);
}


/* 
 * THIS ARRAY STOLEN FROM /home/leonb/sn3.1/src/storage.c
 */

/*
 * These arrays contain the necessary information
 * about the numerical types of all our storage classes.
 *
 * 'storage_type_size' describes the size of each element
 */

int 
storage_type_size[ST_LAST] = {
  0,
  sizeof(char),
  sizeof(flt),
  sizeof(real),
  sizeof(long),
  sizeof(short),
  sizeof(char),
  sizeof(unsigned char),
  sizeof(gptr),
};

/*
 * THESE ROUTINES STOLEN FROM /home/leonb/sn3.1/src/allocate.c
 */

/* Malloc replacement */

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef cfree
#include <malloc.h>

static FILE *malloc_file;

void 
set_malloc_file(s)
char *s;
{
  if (malloc_file) 
    fclose(malloc_file);
  if (s)
    malloc_file = fopen(s,"w");
  else
    malloc_file = NULL;
}


void *
sn3_malloc(x,file,line)
int x;
char *file;
int line;
{
  void *z = malloc(x);
  if (malloc_file)
    fprintf(malloc_file,"%x\tmalloc\t%d\t%s:%d\n",z,x,file,line);
  return z;
}


void *
sn3_calloc(x,y,file,line)
     int x,y;
     char *file;
     int line;
{
  void *z = calloc(x,y);
  if (malloc_file)
    fprintf(malloc_file,"%x\tcalloc\t%d\t%s:%d\n",z,x*y,file,line);
  return z;
}

void *
sn3_realloc(x,y,file,line)
     void *x;
     int y;
     char *file;
     int line;
{
  void *z = (void*) realloc(x,y);
  if (malloc_file) {
    fprintf(malloc_file,"%x\trefree\t%d\t%s:%d\n",x,y,file,line);
    fprintf(malloc_file,"%x\trealloc\t%d\t%s:%d\n",z,y,file,line);
  }
  return z;
}


void 
sn3_free(x,file,line)
     void *x;
     char *file;
     int line;
{
  free(x);
  if (malloc_file)
    fprintf(malloc_file,"%x\tfree\t%d\t%s:%d\n",x,0,file,line);
}

void 
sn3_cfree(x,file,line)
     void *x;
     char *file;
     int line;
{
  cfree(x);
  if (malloc_file)
    fprintf(malloc_file,"%x\tcfree\t%d\t%s:%d\n",x,0,file,line);
}

