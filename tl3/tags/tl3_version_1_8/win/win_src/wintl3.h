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
 
// WinTL3.h : main header file for the TL3E application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Forward

class CConsDoc;

/////////////////////////////////////////////////////////////////////////////
// CTL3EApp:
//

class CTL3App : public CWinApp
{
// Attributes
public:
	// console
	CConsDoc *m_pConsole;
        BOOL      m_bHidden;
	// templates
	CMultiDocTemplate* m_pSnDocTemplate;
	CMultiDocTemplate* m_pTextDocTemplate;
	CMultiDocTemplate* m_pConsoleDocTemplate;
	CMultiDocTemplate* m_pGraphDocTemplate;
	CMultiDocTemplate* m_pGraphDocTopTemplate;

public:
	// location preferences
  	CRect m_rectInitialFrame;     // Location of main window
	BOOL  m_bMaximized;           // TLisp runs in maximized window
	// editor preferences
	CString m_sFontName;          // font face (must be monospaced)
	int  m_nFontSize;             // font size
	BOOL m_bColorize;             // enable colorization
	BOOL m_bAutoIndent;           // RET and TAB indent line
	COLORREF m_colorNormal;       // color for regular text
	COLORREF m_colorComment;      // color for comments
	COLORREF m_colorKeyword;      // color for TLisp keywords
	COLORREF m_colorString;       // color for strings
	COLORREF m_colorHelp;         // color for help entries
	COLORREF m_colorDefine;       // color for function definitions
	// console preferences
	CRect m_rectConsFrame;        // position
	BOOL m_bConsColorize;         // enable colorization
	COLORREF m_colorConsNormal;   // color for char output by Tlisp
	COLORREF m_colorConsEdit;     // color for char typed by user
	COLORREF m_colorConsInput;    // color for editable part of window
	int m_nConsRightMargin;       // right margin for TLisp output
	int m_bConsWordWrap;          // word wrap to right margin 
        BOOL m_bConsAutoEOL;          // automatic eol when typing return

// Operations
public:
	CTL3App();
	void SaveOptions();
	void LoadOptions();
	void EnableWindows(BOOL bEnable=TRUE);
	BOOL CheckMessage();
	void SetStatusMessage(LPCSTR lpsz = NULL);
	void GetMessageString( UINT nID, CString& rMessage ) const;
	void ShowMainWindow(BOOL bShow = TRUE);
	void ViewConsole();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTL3App)
	public:
	virtual BOOL InitInstance();
	virtual BOOL PumpMessage();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL
	virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
				      DWORD lFlags, BOOL bOpenFileDialog, 
				      CDocTemplate* pTemplate);

// Implementation
private:
	void RegisterPath();
// Generated message map functions
	protected:
	//{{AFX_MSG(CTL3App)
	afx_msg void OnFileSaveAll();
	afx_msg void OnUpdateFileSaveAll(CCmdUI* pCmdUI);
	afx_msg void OnFileNewSn();
	afx_msg void OnFileNewTxt();
	afx_msg void OnViewConsole();
	afx_msg void OnUpdateViewConsole(CCmdUI* pCmdUI);
	afx_msg void OnRunHelptool();
	afx_msg void OnUpdateRunHelptool(CCmdUI* pCmdUI);
	afx_msg void OnConsoleHold();
	afx_msg void OnUpdateConsoleHold(CCmdUI* pCmdUI);
	afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

extern CTL3App theApp;
