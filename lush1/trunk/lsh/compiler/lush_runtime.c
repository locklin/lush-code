#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include "idxmac.h"

void run_time_error(char *s) { printf("%s\n",s); exit(1); }

#undef popen
#undef pclose
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef cfree

int unix_pclose(FILE *f) { return pclose(f); }

FILE* unix_popen(const char *cmd, const char *mode) { return popen(cmd, mode); }


int storage_type_size[ST_LAST] = {
  sizeof(at*),
  sizeof(char),
  sizeof(flt),
  sizeof(real),
  sizeof(int),
  sizeof(short),
  sizeof(char),
  sizeof(unsigned char),
  sizeof(gptr),
};

static FILE *malloc_file = 0;
void set_malloc_file(char *s)
{
    if (malloc_file) 
	fclose(malloc_file);
    if (s)
	malloc_file = fopen(s,"w");
    else
	malloc_file = NULL;
}

void *lush_malloc(int x, char *file, int line)
{
    void *z = malloc(x);
    if (malloc_file)
	fprintf(malloc_file,"%p\tmalloc\t%d\t%s:%d\n",z,x,file,line);
    if (!z)
      run_time_error ("Memory exhausted");
    return z;
}

void *lush_calloc(int x,int y,char *file,int line)
{
    void *z = calloc(x,y);
    if (malloc_file)
	fprintf(malloc_file,"%p\tcalloc\t%d\t%s:%d\n",z,x*y,file,line);
    if (!z)
      run_time_error ("Memory exhausted");
    return z;
}

void *lush_realloc(void *x,int y,char *file,int line)
{
    void *z = (void*)realloc(x,y);
    if (malloc_file) {
	fprintf(malloc_file,"%p\trefree\t%d\t%s:%d\n",x,y,file,line);
	fprintf(malloc_file,"%p\trealloc\t%d\t%s:%d\n",z,y,file,line);
    }
    if (!z)
      run_time_error ("Memory exhausted");
    return z;
}

void lush_free(void *x,char *file,int line)
{
    free(x);
    if (malloc_file)
	fprintf(malloc_file,"%p\tfree\t%d\t%s:%d\n",x,0,file,line);
}

void lush_cfree(void *x,char *file,int line)
{
#if HAVE_CFREE
    cfree(x);
#else
    free(x);
#endif
    if (malloc_file)
	fprintf(malloc_file,"%p\tcfree\t%d\t%s:%d\n",x,0,file,line);
}

struct srg* new_lush_string( char* s) {
  Msrg_declare(srgp);
  Msrg_init(srgp, ST_U8);
  Msrg_resize(srgp, strlen(s)+1);
  strcpy((char*)((srgp)->data),s);
  return srgp;
}

void delete_lush_string(struct srg* srgp) {
  Msrg_free(srgp);
}


