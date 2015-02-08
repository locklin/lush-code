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
  main.c
  
  main routine & initialization
  
  $Id: main.c,v 1.2 2015-02-08 02:00:16 leonb Exp $
  ***********************************************************************/

#include "header.h"

#if HAVE_LOCALE_H
# include <locale.h>
#endif

TLAPI int
main(int argc, char **argv)
{
  /* Setup locale */
#if HAVE_SETLOCALE 
# ifdef LC_ALL
  setlocale(LC_ALL,"");
# endif
# ifdef LC_NUMERIC
  setlocale(LC_NUMERIC,"C");
# endif
#endif
  FMODE_TEXT(stderr);
  fprintf(stderr,"TL3 version 1.7" 
#ifdef __DATE__
	  " (compiled on " __DATE__ ")"
#endif
	  "\n");
  fprintf(stderr,"copyright (c) Leon Bottou and Neuristique, 1991-2000\n");
  FMODE_BINARY(stderr);
  init_tl3(argv[0]);
  start_lisp(argc,argv);
  return 0;
}
