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
 
/* --- HEADER SECTION --- 
 *
 * This section loads the appropriate header files
 * and make sure that the correct libraries will be loaded.
 * (WIN32 details are specific to Visual C++ 4.x.)
 */

#include "tlopen.h"



/* --- PRIMITIVE SECTION --- 
 *
 * This section contains your new TL3 primitives.
 * You may also choose to implement them in auxilliary files.
 */


/* EXAMPLE: (hypot <x> <y>)
 * -- this sample function computesthe hypothenuse
 *    of a triangle whose sides are <x> and <y>.
 */

DX(xhypot)
{
  real x, y;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;

  x = AREAL(1);
  y = AREAL(2);
  return NEW_NUMBER(sqrt(x*x + y*y));
}



/* --- INITIALISATION SECTION --- 
 * 
 * Function 'init_user_dll' is called when you load 
 * the TLOpen extension. You may :
 * -- perform your initializations here
 * -- declare your primitives here
 */


int init_user_dll(int major, int minor)
{
  /* Check version */
  if (major!=TLOPEN_MAJOR || minor<TLOPEN_MINOR)
    return -2;
  /* Perform initializations */
  /* Declare primitives */
  dx_define("hypot", xhypot);
  return 0;
}
