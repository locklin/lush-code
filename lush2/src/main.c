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

#if HAVE_LOCALE_H
# include <locale.h>
#endif

int lush_argc;
char **lush_argv;

extern void api_init_tables(void);  /* defined in misc.c */

// deferred work can be done here as long as it comes
// in small enough chunks
static bool lush_idle(void)
{
   return oostruct_idle() || mm_idle();
}

static int mm_spoll(void)
{
   if (lush_idle())
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
      fprintf(stderr,
              " LUSH Lisp Universal Shell " PACKAGE_VERSION
#ifdef __DATE__
              " (built " __DATE__ ")"
#endif
              "\n"
              "   Copyright (C) 2010 Leon Bottou, Yann LeCun, Ralf Juengling.\n"
              "   Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.\n"
              " Includes parts of TL3:\n"
              "   Copyright (C) 1987-1999 Leon Bottou and Neuristique.\n"
              " Includes selected parts of SN3.2:\n"
              "   Copyright (C) 1991-2001 AT&T Corp.\n"
              "\n"
              "This program is free software distributed under the terms of\n"
              "the Lesser GNU Public Licence (LGPL) with ABSOLUTELY NO WARRANTY.\n"
              "\n"
              "Type '(helptool)' to get started.\n");
   } 

   /* Start */
   FMODE_BINARY(stderr);
   
   //FILE *mmlog = fopen("lushmm.log", "w");
   FILE *mmlog = NULL;
   MM_INIT((1<<12) * sizeof(struct at), (notify_func_t *)run_notifiers, mmlog);
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
