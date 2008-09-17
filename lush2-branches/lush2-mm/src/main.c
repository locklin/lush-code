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
 * $Id: main.c,v 1.10 2007/04/02 16:02:57 leonb Exp $
 **********************************************************************/

#include "header.h"
#include "mm.h"

#if HAVE_LOCALE_H
# include <locale.h>
#endif

int lush_argc;
char **lush_argv;

extern void api_init_tables(void);  /* defined in misc.c */

static int mm_spoll(void)
{
   if (mm_idle())
      return 50;  /* msec */
   else
      return 500; /* msec */
}

LUSHAPI int main(int argc, char **argv)
{
   /* Call inits of support code */
   api_init_tables();
   
   /* Define quiet mode. */
   bool quiet = false;
   int i = 1;
   if (i<argc && argv[i][0]=='@')
      i++;
   while (i<argc && argv[i][0]=='-')
      i++;
   if (i < argc)
      quiet = true;
   lush_argc = argc;
   lush_argv = argv;
   
   /* Setup locale */
#if HAVE_SETLOCALE && defined(LC_ALL)
   setlocale(LC_ALL,"");
   setlocale(LC_NUMERIC,"C");
#endif

   /* Message */
   if (! quiet) {
      FMODE_TEXT(stderr);
      fprintf(stderr, "\n");
      fprintf(stderr, PACKAGE_STRING
#ifdef __DATE__
              " (compiled " __DATE__ ")"
#endif
              "\n");
      fprintf(stderr, "Have fun!\n\n");
      /*
      fprintf(stderr,
              "   Copyright (C) 2005 Ralf Juengling.\n"
              " Derived from LUSH Lisp Universal Shell\n"
              "   Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T, NECI.\n"
              " Includes parts of TL3:\n"
              "   Copyright (C) 1987-1999 Leon Bottou and Neuristique.\n"
              " Includes selected parts of SN3.2:\n"
              "   Copyright (C) 1991-2001 AT&T Corp.\n"
              "This program is free software distributed under the terms\n"
              "of the GNU Public Licence (GPL) with ABSOLUTELY NO WARRANTY.\n"
              "Type `(helptool)' for details.\n");
      */
   } 

   /* Start */
   FMODE_BINARY(stderr);
   
   FILE *mmlog = fopen("lushmm.log", "w");
   //FILE *mmlog = NULL;
   mm_init(10000, (notify_func_t *)run_notifiers, mmlog);
   MM_ENTER;

   init_lush(argv[0]);
   register_poll_functions(mm_spoll, 0, 0, 0, 0);
   start_lisp(argc, argv, quiet);

   MM_EXIT;
   if (mmlog) fclose(mmlog);
   return 0;
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
