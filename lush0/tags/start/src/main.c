/*   Lush
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, Neuristique S.A., AT&T Corp.
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
  T-Lisp3: (C) LYB YLC 1988
  main.c
  
  main routine & initialization
  
  $Id: main.c,v 0.1.1.1 2001-10-31 17:30:55 profshadoko Exp $
  ***********************************************************************/

#include "header.h"

LUSHAPI int
main(int argc, char **argv)
{
  /* First argument '@@...' implies script mode. */
  int quiet = (argc>1 && argv[1][0]=='@' && argv[1][1]=='@');
  if (! quiet) 
    {
      FMODE_TEXT(stderr);
      fprintf(stderr,"Lush" 
#ifdef __DATE__
              " (compiled on " __DATE__ ")"
#endif
              "\n");
      fprintf(stderr,
              "Copyright (C) 1987-2001 Leon Bottou, Yann Le Cun\n"
              "                        Neuristique S.A., AT&T Corp.\n"
              "This program is free software; it is distributed under the terms\n"
              "of the GNU Public Licence (GPL) with ABSOLUTELY NO WARRANTY.\n"
              "Type `(helptool)' for details.\n\n");
    } 
  FMODE_BINARY(stderr);
  init_lush(argv[0]);
  start_lisp(argc,argv);
  return 0;
}
