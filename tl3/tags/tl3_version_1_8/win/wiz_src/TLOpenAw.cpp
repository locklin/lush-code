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
 
// TLOpenaw.cpp : implementation file
//

#include <afxwin.h>   // MFC core and standard components
#include <afxext.h>   // MFC extensions
#include <afxcmn.h>   // MFC support for Windows 95 Common Controls
#include <customaw.h> // Custom AppWizard interface
#include "TLOpen.h"
#include "TLOpenAw.h"


// Microsoft VC42 compatibility 

#if (!defined(_MSC_VER) || _MSC_VER<1000)
#error "Microsoft Visual C++ 4.x compiler is required"
#elif (_MSC_VER<1020)
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#pragma comment(lib, "./open/msvc/msvcrt42.lib")
#endif


// Critical registry entries

#define DEFTL3 "c:\\Program Files\\WinTL3\\TL3"
#define KEYTL3 "Software\\Neuristique\\WinTL3\\TLOpen"


// This is called immediately after the custom AppWizard is loaded.  Initialize
// the state of the custom AppWizard here.
void CTLOpenAppWiz::InitCustomAppWiz()
{
  // There are no steps in this custom AppWizard.
  SetNumberOfSteps(0);
  // Inform AppWizard that we're making a DLL.
  m_Dictionary[_T("PROJTYPE_DLL")] = _T("1");
  // Locate WINTL3 directory (init with default value).
  CString strWinTL3 = DEFTL3;
  // Search in registry
  HKEY key;
  LONG status = RegOpenKeyEx(HKEY_CURRENT_USER, KEYTL3,
			     0, KEY_QUERY_VALUE, &key );
  if (status == ERROR_SUCCESS)
  {
    TCHAR buffer[256];
    DWORD bufsize = sizeof(buffer);
    DWORD type = REG_SZ;
    status = RegQueryValueEx(key, _T("Path"), NULL, 
                             &type, (LPBYTE)buffer, &bufsize);
    RegCloseKey(key);
    if (status==ERROR_SUCCESS  && type==REG_SZ)
    {
      strWinTL3 = buffer;
      // Set WINTL3DIR variable
      m_Dictionary[_T("WINTL3DIR")] = strWinTL3;
    }
  }
  DWORD len = GetShortPathName((LPCTSTR)strWinTL3, 0, 0);
  CString strWinTL3Short;
  GetShortPathName((LPCTSTR)strWinTL3, strWinTL3Short.GetBuffer(len), len);
  strWinTL3Short.ReleaseBuffer();
  // Slashification
  int i;
  for (i=0; i<strWinTL3.GetLength(); i++)
    if (strWinTL3Short[i]==_T('\\'))
      strWinTL3Short.SetAt(i,_T('/'));
  if (strWinTL3Short[i-1] != _T('/'))
    strWinTL3Short += _T('/');
  // Set WINTL3 variables
  m_Dictionary[_T("TLOPENINCLUDE")] = strWinTL3Short + _T("include");
  m_Dictionary[_T("TLOPENLIB")] = strWinTL3Short + _T("open/msvc");
}

// This is called just before the custom AppWizard is unloaded.
void CTLOpenAppWiz::ExitCustomAppWiz()
{
}

// This is called when the user clicks "Create..." on the New Project dialog
CAppWizStepDlg* CTLOpenAppWiz::Next(CAppWizStepDlg* pDlg)
{
  ASSERT(pDlg == NULL);	
  // Return NULL to indicate there are no more steps.
  return NULL;
}

// Here we define one instance of the CTLOpenAppWiz class.  You can access
//  m_Dictionary and any other public members of this class through the
//  global TLOpenaw.
CTLOpenAppWiz TLOpenaw;

