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

/***********************************************************************
  REGEX.C (C) LYB 1991
  A full regular expression package.
************************************************************************/

/*
 * This is the regular expression grammar...
 * 
 * regex.single		:   LIT
 * 			|   RANGE
 * 			|   '.'
 * 	  		|   '(' regex.alternate ')'
 * 			;
 * 
 * regex.several	:   regex.single
 * 			|   regex.single '+'
 * 			|   regex.single '*'
 * 			|   regex.single '?'
 * 			;
 * 
 * regex.catenate	:   regex.several
 * 			|   regex.catenate regex.several
 * 			;
 * 
 * regex.alternate	:   regex.catenate
 * 			|   regex.catenate '|' regex.alternate
 * 			;
 * 
 * 
 * regex		:   regex.alternate
 * 			;
 */

/* ----------- declarations ------------ */


static sigjmp_buf rejmpbuf;
static const char *pat;
static const char *dat, *datstart;
static unsigned short *buf, *bufstart;

#define RE_LIT      0x0000
#define RE_RNG      0x1000
#define RE_ANY      0x2000
#define RE_JMP      0x3800
#define RE_FAIL     0x4800
#define RE_START    0x5000
#define RE_END      0x6000
#define RE_CARET    0x7000
#define RE_DOLLAR   0x8000

#define charset_zero(cset) \
   { int i; for(i=0;i<16;i++) (cset)[i]=0; }
#define charset_toggle(cset) \
   { int i; for(i=0;i<16;i++) (cset)[i]^=0xffff; }
#define charset_set(cset,i) \
   { (cset)[((unsigned char)(i))/16] |= (1<<(((unsigned char)(i))%16)); }
#define charset_tst(cset,i) \
   ( (cset)[((unsigned char)(i))/16] & (1<<(((unsigned char)(i))%16)) )

typedef struct bounds {
   unsigned short *beg, *end; } bounds;

static char *err[] = {
   "Regex is too complex",
   "Unbalanced () in regex",
   "Unbalanced [] in regex",
   "Bad badslash escape in regex",
   "Regex syntax error",
};

#define serror(n)           { dat=err[n];siglongjmp(rejmpbuf,-1); }
#define concatc(b1,c,in)    { if (b1.end>in.end-4) serror(0);\
                              *b1.end++ = c; }
#define concatb(b1,b2,in)   { unsigned short *s=b2.beg;\
                              if (b1.end+(b2.end-b2.beg)>in.end-4) serror(0);\
			      if (s==b1.end) b1.end=b2.end; else \
                              while(s<b2.end) *b1.end++ = *s++; }


/* ----------- private routines ------------ */


static void regex_alternate(bounds*, bounds, int*);

static void regex_single(bounds *ans, bounds buf, int *rnum)
{
   bounds tmp;
   unsigned short *set;
   int toggle=0,last=0;
   unsigned char c;
   
   ans->beg = ans->end = buf.beg;
   
   switch (c = *pat++) {

   case 0:
      pat--;
   case '|':
      serror(4);
   case ']':
      serror(2);
   case ')':
      serror(1);
      
   case '\\':
      if (!*pat) serror(3);
      concatc((*ans),*pat,buf);
      pat++;
      break;
    
   case '.':
      concatc((*ans),RE_ANY,buf);
      break;

   case '(':
      last = *rnum;
      *rnum = *rnum + 1;
      concatc((*ans),RE_START+last,buf);
      buf.beg += 1;
      regex_alternate(&tmp, buf,rnum);
      concatb((*ans),tmp,buf);
      if (*pat!=')') serror(1);
      concatc((*ans),RE_END+last,buf);
      pat++;
      break;
    
   case '[':
      if (ans->end>buf.beg+18) serror(0);
      set = ans->end+1;
      charset_zero(set);
      toggle = last = 0;
      if (*pat=='^')  { toggle=1; pat++; }
      if (*pat==']')  { charset_set(set,']'); pat++; }
      while(*pat!=']') {
         if (!*pat) serror(2);
         if (*pat=='-' && last && pat[1] && pat[1]!=']') {
            pat++;
            while(last<=*pat) {
               charset_set(set,last);
               last++;
            }
         } else {
            charset_set(set,*pat);
            last = *pat;
         }
      pat++;
      }
      pat++;
      if (toggle)
         charset_toggle(set);
      last = 15;
      while (set[last]==0 && last>0) 
         last--;
      concatc((*ans),RE_RNG+last+1,buf);
      ans->end += last+1;
      break;
      
   case '^':
      concatc((*ans),RE_CARET,buf);
      break;
      
   case '$':
      concatc((*ans),RE_DOLLAR,buf);
      break;
      
   default:
      concatc((*ans),c,buf);
      break;
      
   }
}


static void regex_several(bounds *ans, bounds buf, int *rnum)
{
   bounds b;
   bounds rem;
   
   ans->beg = buf.beg;
   ans->end = buf.beg;
   rem.beg = ans->beg+2;
   rem.end = buf.end;
   
   regex_single(&b, rem,rnum);

   switch (*pat) {
      
   case '?':
      concatc((*ans), RE_FAIL+(b.end-b.beg), buf);
      concatb((*ans), b, buf);
      pat++;
      break;
      
   case '+':
      concatb((*ans),b,buf);
      concatc((*ans), RE_FAIL+1, buf);
      concatc((*ans), RE_JMP+(b.beg-b.end)-2, buf);
      pat++;
      break;

   case '*':
      concatc((*ans), RE_FAIL+(b.end-b.beg)+1, buf);
      concatb((*ans),b,buf);
      concatc((*ans), RE_JMP+(b.beg-b.end)-2, buf);
      pat++;
      break;

   default:
     concatb((*ans),b,buf);
     break;
   }
}

static void regex_catenate(bounds *ans, bounds buf, int *rnum)
{
   bounds b;
   bounds rem;

   ans->beg = buf.beg;
   ans->end = buf.beg;
   rem.beg = buf.beg;
   rem.end = buf.end;
   
   do {
      rem.beg = ans->end;
      regex_several(&b,rem,rnum);
      concatb((*ans),b,buf);
   } while (*pat && *pat!='|' && *pat!=')');
}

static void regex_alternate(bounds *ans, bounds buf, int *rnum)
{ 
   bounds b1,b2;
   bounds rem;
   int newrnum;
   
   ans->beg = buf.beg;
   ans->end = buf.beg;
   rem.beg = buf.beg+1;
   rem.end = buf.end;
   
   newrnum = *rnum;
   regex_catenate(&b1,rem,rnum);
   if (*pat == '|') {
      pat++;
      rem.beg = b1.end+1;
      regex_alternate(&b2,rem,&newrnum);
      if (newrnum>*rnum)
         *rnum = newrnum;
      concatc((*ans), RE_FAIL+(b1.end-b1.beg+1), buf);
      concatb((*ans), b1, buf);
      concatc((*ans), RE_JMP +(b2.end-b2.beg), buf);
      concatb((*ans), b2, buf);
   } else {
      concatb((*ans), b1, buf);
   }
}

#define MIN_EXECBUF 2000

static int regex_execute(const char **regsptr, int *regslen, int nregs)
{
   unsigned short *bfail_buf[MIN_EXECBUF], **bfail = bfail_buf;
   const char     *dfail_buf[MIN_EXECBUF], **dfail = dfail_buf;

   int sp = 0;
   int spmax = MIN_EXECBUF;
   unsigned short  c;
   while ((c = *buf++)) {
#ifdef DEBUG_REGEX
      printf("%04x (%02x)\n",(int)(c)&0xffff,(dat?*dat:0)); 
#endif
      switch (c&0xf000) {

      fail:
         if (--sp < 0)
            return 0;
         buf = bfail[sp];
         dat = dfail[sp];
         break;
         
      case RE_CARET:
         if (dat != datstart)
            goto fail;
         break;

      case RE_DOLLAR:
         if (*dat)
            goto fail;
         break;

      case RE_LIT:
         if (!*dat)
            goto fail;
         if (*dat++ != (char)(c&0x00ff))
            goto fail;
         break;
         
      case RE_RNG:
         if (!*dat)
            goto fail;
         c -= RE_RNG;
         if (*dat>=c*16)
            goto fail;
         if (!charset_tst( buf, *dat))
            goto fail;
         buf += c;
         dat++;
         break;

      case RE_ANY:
         if (!*dat)
            goto fail;
         dat++;
         break;

      case RE_JMP&0xf000:
         if ((*dat==0) && (c<RE_JMP)) /* never jump backwards if end of data */
            break;
         buf = buf + c - RE_JMP;
         break;
      
      case RE_FAIL&0xf000:
         if (sp >= spmax) { /* enlarge stack */
            spmax += spmax;
            unsigned short **bfail2 = mm_blob(sizeof(short *)*spmax);
            char **dfail2 = mm_blob(sizeof(char *)*spmax);
            if (!bfail2 || !dfail2)
               error(NIL,"out of memory",NIL);
            memcpy(bfail2, bfail, sizeof(short *)*spmax/2);
            memcpy(dfail2, dfail, sizeof(short *)*spmax/2);
            bfail = bfail2;
            dfail = (const char **)dfail2;
         }
         dfail[sp] = dat;
         bfail[sp] = buf + c - RE_FAIL;
         sp += 1;
         break;
 
      case RE_START:
         c &= 0x00ff;
         if (c < nregs)
            regsptr[nregs+c] = dat;
         break;

      case RE_END:
         c &= 0x00ff;
         if (c >= nregs)
            break;
         regsptr[c] = regsptr[nregs+c];
         regslen[c] = dat-regsptr[c];
         break;
      }
   }
   buf--;
   return 1;
}



/* ----------- public routines ------------ */


const char *regex_compile(const char *pattern, 
                          unsigned short *bufstart, unsigned short *bufend, 
                          int strict, int *rnum)
{
   int regnum = 0;
   if (!rnum) 
      rnum = &regnum;
   *rnum = 0;
   dat = 0L;
   pat = pattern;
   if (sigsetjmp(rejmpbuf, 1)) {
      return dat;

   } else  {
      bounds buf, tmp;
      buf.beg = bufstart + 1;
      buf.end = bufend - 1;
      if (strict) {
         *buf.beg++ = RE_CARET;
         buf.end--;
      }
      regex_alternate(&tmp,buf,rnum); 
      buf = tmp;
      if (strict) 
         *buf.end++ = RE_DOLLAR;
      *buf.end = 0;
      *bufstart = buf.end - bufstart + 1;
      return 0L;
   }
}


int regex_exec(unsigned short *buffer, const char *string, 
               const char **regptr, int *reglen, int nregs)
{
   for(int c=0;c<2*nregs;c++)
      regptr[c] = 0;
   for(int c=0;c<nregs;c++)
      reglen[c] = 0;
   dat = datstart = string;
   buf = bufstart = (unsigned short*)(buffer+1);
   return regex_execute(regptr,reglen,nregs);
}


int regex_seek(unsigned short *buffer, const char *string, const char *seekstart, 
               const char **regptr, int *reglen, int nregs, const char **start, const char **end)
{
   datstart = string;
   while (*seekstart) {
      for(int c=0;c<2*nregs;c++)
         regptr[c] = 0;
      for (int c=0;c<nregs;c++)
         reglen[c] = 0;
      dat = seekstart;
      buf = bufstart = (unsigned short*)(buffer+1);
      if (regex_execute(regptr, reglen, nregs)) {
         *start = seekstart;
         *end = dat;
         return 1;
      }
      seekstart++;
   }
   return 0;
}


/* -------- undefines ------------ */

#undef RE_LIT
#undef RE_RNG
#undef RE_ANY
#undef RE_JMP
#undef RE_FAIL
#undef RE_START
#undef RE_END
#undef charset_zero
#undef charset_toggle
#undef charset_set
#undef charset_tst
#undef serror
#undef concatc
#undef concatb

/* ---------- lisp interface ------- */


DX(xregex_match)
{
   unsigned short buffer[1024];

   ARG_NUMBER(2);
   const char *pat = ASTRING(1);
   const char *dat = ASTRING(2);

   pat = regex_compile(pat,buffer,buffer+1024,1,NULL);
   if (pat)
      error(NIL,pat,APOINTER(1));

   if (regex_exec(buffer,dat,NULL,NULL,0))
      return t();  
   else
      return NIL;
}


DX(xregex_extract)
{
   unsigned short buffer[1024];
   at *ans=NIL;
   at **where = &ans;
   int regnum = 0;
   int i;

   ARG_NUMBER(2);
   const char *pat = ASTRING(1);
   const char *dat = ASTRING(2);
   pat = regex_compile(pat,buffer,buffer+1024,1,&regnum);
   if (pat)
      error(NIL,pat,APOINTER(1));

   const char **regptr = mm_blob(2*regnum*sizeof(char*));
   int *reglen = mm_blob(regnum*sizeof(int));
   if (regnum && (!regptr || !reglen))
      error(NIL, "out of memory", NIL);
   
   if (regex_exec(buffer,dat,regptr,reglen,regnum)) {
      for (i=0; i<regnum; i++) {
         char *s = mm_blob(reglen[i]+1);
         strncpy(s, regptr[i], reglen[i]);
         s[reglen[i]] = '\0';
         *where = new_cons(NEW_STRING(s), NIL);
         where = &Cdr(*where);
      }
      if (!ans)
         *where = new_cons(APOINTER(2),NIL);
   }
   return ans;
}


DX(xregex_seek)
{
   int n;
   unsigned short buffer[1024];

   if (arg_number==3)
      n = AINTEGER(3);
   else {
      n = 0;
      ARG_NUMBER(2);
   }
   const char *datstart;
   const char *pat = ASTRING(1);
   const char *dat = datstart = ASTRING(2);
   while (--n>=0 && *dat)
      dat++;

   pat = regex_compile(pat,buffer,buffer+1024,0,NULL);
   if (pat)
      error(NIL,pat,APOINTER(1));
  
   const char *start,*end;
   if (regex_seek(buffer,datstart,dat,NULL,NULL,0,&start,&end)) {
      dat = ASTRING(2);
      return new_cons(NEW_NUMBER(start-dat),
                      new_cons(NEW_NUMBER(end-start),
                               NIL ) );
   }
   return NIL;
}



DX(xregex_subst)
{
   ARG_NUMBER(3);
   const char *datstart;
   const char *pat = ASTRING(1);
   const char *str = ASTRING(2);
   const char *dat = datstart = ASTRING(3);
 
   int regnum = 0;
   unsigned short buffer[1024];
   pat = regex_compile(pat,buffer,buffer+1024,0,&regnum);
   if (pat)
      error(NIL,pat,APOINTER(1));
  
   struct large_string ls;
   large_string_init(&ls);
   do {
      int reglen[10];
      const char *regptr[20];
      const char *start, *end, *s1;
      if (!regex_seek(buffer,datstart,dat,regptr,reglen,10,&start,&end))
         start = end = dat + strlen(dat);
      if (end <= dat)
         start = end = dat + 1;
      if (dat < start) {
         s1 = dat;
         large_string_add(&ls, s1, start-s1);
         s1 = start;
      }
      if (start < end) {
         s1 = str;
         while (*s1) {
            if (*s1 == '%') {
               s1++;
               if (*s1 == '%') {
                  large_string_add(&ls, s1++, 1);

               } else {
                  int reg = *s1++ - '0';
                  if (reg<0 || reg>9 || reg>=regnum)
                     error(NIL,"bad register number",APOINTER(3));
                  if (reglen[reg]>0)
                     large_string_add(&ls, regptr[reg], reglen[reg]);
               }

            } else {
               const char *s2 = s1;
               while (*s1 && *s1!='%')
                  s1 += 1;
               large_string_add(&ls, s2, s1-s2);
            }
         }
      }
      while (*dat && dat<end)
         dat++;
   } while (*dat);
   
   return large_string_collect(&ls);
}


void init_lushregex(void)
{
   dx_define("regex-match", xregex_match);
   dx_define("regex-extract", xregex_extract);
   dx_define("regex-seek", xregex_seek);
   dx_define("regex-subst", xregex_subst);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
