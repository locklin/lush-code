/*   Lush Lisp interpreter and development tools
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
 

#ifndef TLOPEN_H
#define TLOPEN_H

/* ---------------------------------
 * This file simply loads the Lush 
 * header file under Unix. The proper
 * directories are specified by the
 * compiler options.
 */
#ifndef WIN32
#include "header.h"
#else /* WIN32 */

/* ---------------------------------
 * Specify Lush header file (windows)
 **** You may need to edit thje winlush directory name.
 */
#include "$$TLOPENINCLUDE$$/header.h"


/* ---------------------------------
 * Specify TL/Open library file (windows)
 **** You may need to edit the winlush directory name.
 */
#pragma comment(lib,"$$TLOPENLIB$$/lushdll.lib")


/* ---------------------------------
 * Check that TL/Open extension is
 * compiled with the correct options.
 **** You may need to edit thje winlush directory name.
 */

#if (!defined(_MSC_VER) || _MSC_VER<1000)
#error "Microsoft Visual C++ 4.x compiler is required"
#elif (_MSC_VER<1020)
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#pragma comment(lib, "$$TLOPENLIB$$/msvcrt42.lib")
#elif (defined(_DEBUG))
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(lib, "msvcrt.lib")
#endif
#if (!defined(_MT) && !defined(_DLL))
#error "TLOpen project requires compiler option /MD"
#endif


/* --------------------------------- */
#endif /* WIN32    */
#endif /* TLOPEN_H */
