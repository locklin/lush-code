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
 * $Id: main.c,v 1.1 2002-04-18 20:17:13 leonb Exp $
 **********************************************************************/

#include "header.h"

LUSHAPI int
main(int argc, char **argv)
{
  /* First argument '@@...' implies script mode. */
  int quiet = (argc>1 && argv[1][0]=='@' && argv[1][1]=='@');
  if (! quiet) 
    {
      FMODE_TEXT(stderr);
      fprintf(stderr,"LUSH Lisp Universal Shell"
#ifdef __DATE__
              " (compiled on " __DATE__ ")"
#endif
              "\n");
      fprintf(stderr,
              "   Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.\n"
              " Includes parts of TL3\n"
              "   Copyright (C) 1987-1999 Leon Bottou and Neuristique.\n"
              " Includes selected parts of SN3.2:\n"
              "   Copyright (C) 1991-2001 AT&T Corp.\n"
              "This program is free software; it is distributed under the terms\n"
              "of the GNU Public Licence (GPL) with ABSOLUTELY NO WARRANTY.\n"
              "Type `(helptool)' for details.\n\n");
    } 
  FMODE_BINARY(stderr);
  init_lush(argv[0]);
  start_lisp(argc,argv);
  return 0;
}
