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
 
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//



#define VC_EXTRALEAN	    // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>	    // MFC support for Windows 95 Common Controls
#include <afxtempl.h>	    // MFC container template
#include <afxmt.h>	    // MFC synchronization objects


#if defined(BETA) && !defined(_DEBUG)
void WinTL3Check();
void WinTL3Abort(LPCSTR);
BOOL WinTL3Assert(LPCSTR, UINT);
#undef ASSERT
#undef VERIFY
#define ASSERT(f)  ((f) || WinTL3Assert(__FILE__, __LINE__))
#define VERIFY(f)  ASSERT(f)
#endif

/////////////////////////////////////////////////////////////
// DisplayDC

class CDisplayIC : public CDC
{
public:
    CDisplayIC() { CreateIC(_T("DISPLAY"), NULL, NULL, NULL); }
};



