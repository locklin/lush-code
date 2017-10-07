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
 
/************************************************************/
/************************************************************/
/**                                                        **/
/**   TL/OPEN EXAMPLE: CAPITALIZE-NTH                      **/
/**      Please refer to the TL/Open manual for            **/
/**      additional information                            **/
/**                                                        **/
/************************************************************/
/************************************************************/




/* --- HEADER SECTION --- 
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




/* capitalize_nth -- 
 * Return a string (allocated in a static buffer)
 * equal to string <s> with the <n>th character
 * turned to uppercase.
 */

static char *
capitalize_nth(char *s, int n)
{
  static char buffer[256];
  char c;
  int i;

  /* Checks */
  i = strlen(s);
  if (i>255)
    error(NIL,"string is too long",NIL);
  if (n<0 || n>=i)
    error(NIL,"no nth character", NEW_NUMBER(n));

  /* Action */
  strcpy( buffer, s);
  c = buffer[n];
  if (c>='a' && c<='z')
    buffer[n] = c+'A'-'a';

  /* Return result in a static area */
  return buffer;
}


/* xcapitalize_nth --
 * TL3 interface for the above.
 */


DX(xcapitalize_nth)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  return new_string(
       capitalize_nth(ASTRING(1),AINTEGER(2)) );
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
  /* Declare primitives */
  dx_define("capitalize_nth", xcapitalize_nth);
  return 0;
}
