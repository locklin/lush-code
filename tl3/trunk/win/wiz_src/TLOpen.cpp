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
 
// TLOpen.cpp : Defines the initialization routines for the DLL.
//

#include <afxwin.h>   // MFC core and standard components
#include <afxext.h>   // MFC extensions
#include <afxcmn.h>   // MFC support for Windows 95 Common Controls
#include <customaw.h> // Custom AppWizard interface
#include <afxdllx.h>
#include "TLOpen.h"
#include "TLOpenAw.h"

#ifdef _PSEUDO_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE TLOpenDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("TLOPEN.AWX Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(TLOpenDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(TLOpenDLL);

		// Register this custom AppWizard with MFCAPWZ.DLL
		SetCustomAppWizClass(&TLOpenaw);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("TLOPEN.AWX Terminating!\n");
	}
	return 1;   // ok
}
