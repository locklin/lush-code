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
 
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"   // portable AFX headers

#include "WinTL3.h"
#include "MainFrm.h"
#include "TextDoc.h"
#include "TextView.h"
#include "Console.h"
#include "ConsFrm.h"
#include "GraphWin.h"
#include "Splash.h"
#include "TL3Com.h"
#include "PropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LC, OnUpdateUIDisable)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateUIDisable)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_PREF_CONSOLE, OnPropSheet)
	ON_COMMAND(ID_PREF_EDITOR, OnPropSheet)
	ON_COMMAND(ID_HELP, OnHelpWrapper)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpWrapper)
	ON_COMMAND(IDH_CONTENT, OnHelpWrapper)
	ON_COMMAND(IDH_COPYING, OnHelpWrapper)
        ON_COMMAND(IDH_WINTL3, OnHelpWrapper)
	ON_COMMAND(IDH_TLOPEN, OnHelpWrapper)
	ON_COMMAND(IDH_NSA, OnHelpWrapper)
	ON_UPDATE_COMMAND_UI(ID_HELP, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(IDH_CONTENT, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(IDH_COPYING, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(IDH_WINTL3, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(IDH_TLOPEN, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(IDH_NSA, OnUpdateUIHelpWrapper)
	//}}AFX_MSG_MAP
#ifndef NOFANCYHELP
	ON_UPDATE_COMMAND_UI(ID_HELP_INDEX, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(ID_HELP_FINDER, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(ID_HELP_USING, OnUpdateUIHelpWrapper)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_HELP, OnUpdateUIHelpWrapper)
	ON_COMMAND(ID_HELP_INDEX, OnHelpWrapper)
	ON_COMMAND(ID_HELP_FINDER, OnHelpWrapper)
	ON_COMMAND(ID_HELP_USING, OnHelpWrapper)
        ON_COMMAND(ID_CONTEXT_HELP, OnHelpWrapper)
#endif
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LC,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_OVR,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
  m_bUserConfirmExit = FALSE;
  m_bSavedScrollLock = FALSE;
  m_bPrivateScrollLock = FALSE;
  m_bActive = FALSE;
}

CMainFrame::~CMainFrame()
{
}


// CMainFrame::PreCreateWindow
// -- create main frame on last saved location

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
  CRect rect = theApp.m_rectInitialFrame;
  if (rect.Width() > 0 && rect.Height() > 0)
  {
    // make sure window will be visible
    CDisplayIC dc;
    CRect rectDisplay(0, 0, 
      dc.GetDeviceCaps(HORZRES), 
      dc.GetDeviceCaps(VERTRES));
    if (rectDisplay.PtInRect(rect.TopLeft()) && 
  	rectDisplay.PtInRect(rect.BottomRight()))
    {
      cs.x = rect.left;
      cs.y = rect.top;
      cs.cx = rect.Width();
      cs.cy = rect.Height();
    }
  }
  return CMDIFrameWnd::PreCreateWindow(cs);
}



// CMainFrame::OnCreate
// -- create toolbar and status bar
//    linked tomessage WM_CREATE

int 
CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
	  return -1;	
  if (!m_wndToolBar.Create(this) ||
	  !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
  {
	  TRACE0("Failed to create toolbar\n");
	  return -1;      // fail to create
  }
  if (!m_wndStatusBar.Create(this) ||
      !m_wndStatusBar.SetIndicators(indicators,sizeof(indicators)/sizeof(UINT)))
  {
	  TRACE0("Failed to create status bar\n");
	  return -1;      // fail to create
  }
  m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
	  CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
  EnableDocking(CBRS_ALIGN_ANY);
  DockControlBar(&m_wndToolBar);
  CSplashWnd::ShowSplashScreen(this);
  return 0;
}



/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void 
CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void 
CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers




// CMainFrame::OnUpdateUIDisable
// -- inconditionally disable item
//    linked to OVERLAY and LINE/COLUMN indicator

void 
CMainFrame::OnUpdateUIDisable(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(FALSE);
}



// CMainFrame::OnSize
// -- store new frame location (saved into registry on app exit)
//    linked to message WM_SIZE

void 
CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
  CMDIFrameWnd::OnSize(nType, cx, cy);
  switch (nType)
  {
  case SIZE_MAXIMIZED:
    theApp.m_bMaximized = TRUE;
    MakeAllVisible();
    break;
  case SIZE_RESTORED:
    theApp.m_bMaximized = FALSE;
    GetWindowRect(theApp.m_rectInitialFrame);
    MakeAllVisible();
    break;
  }
}
  


// CMainFrame::MakeAllVisible
// -- make sure all windows are visible

void 
CMainFrame::MakeAllVisible()
{
  CRect rect, rectFrame, rectMDI;
  // Get actual mdi frame
  CWnd *pMDI = CWnd::FromHandle(m_hWndMDIClient);
  pMDI->GetClientRect(rectMDI);
  // Arrange icons
  MDIIconArrange();
  // Iterate over all frames
  CWnd *pFrame =  pMDI->GetTopWindow(); 
  for ( ; pFrame; pFrame  = pFrame->GetNextWindow() )
  {
    if (pFrame->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
    {
      pFrame->GetWindowRect(rectFrame);
      ScreenToClient(rectFrame);
      if (! rect.IntersectRect(rectMDI, rectFrame))
      {
	int w = rectFrame.Width();
	int h = rectFrame.Height();
	if (rectFrame.right <= 0)
	{
	  rectFrame.right = 50;
	  rectFrame.left = rectFrame.right - w;
	}
	else if (rectFrame.left >= rectMDI.right)
	{
	  rectFrame.left = rectMDI.right - 50;
	  rectFrame.right = rectFrame.left + w;
	}
	if (rectFrame.bottom <= 0)
	{
	  rectFrame.bottom = 50;
	  rectFrame.top = rectFrame.bottom - h;
	}
	else if (rectFrame.top >= rectMDI.bottom)
	{
	  rectFrame.top = rectMDI.bottom - 50;
	  rectFrame.bottom = rectFrame.top + h;
	}
	pFrame->MoveWindow(rectFrame);
      }
    }
  }
}




// CMainFrame::GetMessageString
// -- use definition provided by application

void 
CMainFrame::GetMessageString( UINT nID, CString& rMessage ) const
{
  theApp.GetMessageString(nID, rMessage);
}



// CMainFrame::OnClose
// -- ask user confirmation, saves preferences, close main frame.
//    linked to WM_CLOSE

void 
CMainFrame::OnClose() 
{
  // Check that confirmation dialog has been displayed
  if (m_bUserConfirmExit ||
      theApp.m_bHidden   ||
      AfxMessageBox(IDP_CONSOLE_CLOSE_MSG, 
		    MB_OKCANCEL|MB_ICONQUESTION)==IDOK )
  {
    m_bUserConfirmExit = TRUE;
    // User has confirmed
    if (theCom.m_bThreadStarted)
    {
      // Thread is running: kill it
      theCom.KillTLisp();
    }
    else
    {
      // Exit now
      theApp.SaveOptions();
      CMDIFrameWnd::OnClose();
    }
  }
}



// CMainFrame::OnQueryEndSession
// -- called to allow or prevent windows to exit

BOOL
CMainFrame::OnQueryEndSession() 
{
  if (m_bUserConfirmExit ||  
      theApp.m_bHidden   ||
      AfxMessageBox(IDP_CONSOLE_CLOSE_MSG, 
		    MB_OKCANCEL|MB_ICONQUESTION)==IDOK )
  {
    if (CMDIFrameWnd::OnQueryEndSession())
    {
      // session will end
      m_bUserConfirmExit = TRUE;
      // mark all documents as not modified
      // (because SaveAllModified will be called again
      //  when the TLisp thread calls RPC TL3_EXIT which calls OnClose!)
      POSITION templpos = theApp.GetFirstDocTemplatePosition();
      while (templpos) 
      {
	CDocTemplate *tmpl = theApp.GetNextDocTemplate(templpos);
	POSITION docpos = tmpl->GetFirstDocPosition();
	while (docpos) {
	  CDocument *doc = tmpl->GetNextDoc(docpos);
	  doc->SetModifiedFlag(FALSE);
	}
      }
      return TRUE;
    }
  }
  return FALSE;
}



// CMainFrame::OnEndSession
// -- called when window exits (calls OnClose)

void 
CMainFrame::OnEndSession(BOOL bEnding) 
{
  if (bEnding)
  {
    // Kill TLisp
    if (theCom.m_bThreadStarted)
      theCom.KillTLisp();
    // Save all options
    theApp.SaveOptions();
  }
  CMDIFrameWnd::OnEndSession(bEnding);
}



// CMainFrame::TestHelpFile
// -- tests presence of help file

BOOL CMainFrame::TestHelpFile() 
{
  static BOOL bChecked = FALSE;
  static BOOL bPresent = FALSE;
  if (!bChecked)
  {
    bChecked = TRUE;
    char buffer[256];
    int index = GetModuleFileName(GetModuleHandle(NULL), buffer, 256);
    if (index>0 && index<255 && !_stricmp(buffer+index-4, ".exe"))
    {
      strcpy(buffer+index-4, ".hlp");
      DWORD attr = GetFileAttributes((LPTSTR)buffer);
      if (attr != 0xFFFFFFFF)
	bPresent = TRUE;
    }
  }
  return bPresent;
}


// CMainFrame::OnUpdateUIHelpWrapper
// -- all help message go there

void CMainFrame::OnUpdateUIHelpWrapper(CCmdUI *pCmdUI) 
{
  if (TestHelpFile())
    pCmdUI->Enable(TRUE);
  else
    pCmdUI->Enable(FALSE);
}


// CMainFrame::OnHelpWrapper
// -- all help message go there

void CMainFrame::OnHelpWrapper() 
{
  if (!TestHelpFile())
    return;

  DWORD wmcode = GetCurrentMessage()->wParam;
  switch(wmcode)
  {
  case ID_HELP_USING:
    CMDIFrameWnd::OnHelpUsing();
    break;
  case ID_HELP_INDEX:
    CMDIFrameWnd::OnHelpIndex();
    break;
  case ID_HELP_FINDER:
    CMDIFrameWnd::OnHelpFinder();
    break;
  case ID_CONTEXT_HELP:
    CMDIFrameWnd::OnContextHelp();
    break;
  // WinTL3 actually uses these
  case ID_HELP:
    CMDIFrameWnd::OnHelp();
    break;
  case ID_DEFAULT_HELP:
    theApp.WinHelp(IDH_CONTENT);
    break;
  // WinTL3 specific entries IDH_xxx
  case IDH_CONTENT:
  case IDH_WINTL3:
  case IDH_TLOPEN:
  case IDH_NSA:
  case IDH_COPYING:
    theApp.WinHelp(wmcode);
    break;
  }
}




// CMainFrame::OnQueryNewPalette
// -- install palette when useful

BOOL 
CMainFrame::OnQueryNewPalette() 
{
  CMDIChildWnd *pFrame = MDIGetActive();
  if (pFrame)
    return pFrame->SendMessage(WM_QUERYNEWPALETTE);
  else
    return FALSE;
}



// CMainFrame::OnPaletteChanged
// -- perform degraded palette assignment on window

void CMainFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
  CWnd *pMDI = CWnd::FromHandle(m_hWndMDIClient);
  CWnd *pFrame =  pMDI->GetTopWindow(); 
  for ( ; pFrame; pFrame  = pFrame->GetNextWindow() )
    if (pFrame->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
      pFrame->SendMessage(WM_PALETTECHANGED,(WPARAM)(HWND)*pFocusWnd);
}


// CMainFrame::SetScrollLock
// -- sets scroll-lock light if application is active

void 
CMainFrame::SetScrollLock(BOOL bLock)
{
  m_bPrivateScrollLock = bLock;
  if (m_bActive)
  {
    BYTE nbKeyState[256];
    GetKeyboardState(nbKeyState);
    nbKeyState[VK_SCROLL] &= ~1;
    if (bLock)
      nbKeyState[VK_SCROLL] |= 1;
    SetKeyboardState(nbKeyState);
  }
}



// CMainFrame::OnActivate
// -- track main window activation
//    adjust scroll lock light

void 
CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
  // inherited first
  CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);
  // save scroll lock state
  BYTE nbKeyState[256];
  GetKeyboardState(nbKeyState);
  nbKeyState[VK_SCROLL] &= ~1;
  if (nState == WA_INACTIVE)
  {
    // becoming inactive
    m_bActive = FALSE;
    if (m_bSavedScrollLock)
      nbKeyState[VK_SCROLL] |= 1;
  }
  else
  {
    // becoming active
    m_bActive = TRUE;
    m_bSavedScrollLock = ((GetKeyState(VK_SCROLL) & 1) ? TRUE : FALSE);
    if (m_bPrivateScrollLock)
      nbKeyState[VK_SCROLL] |= 1;
  } 
  SetKeyboardState(nbKeyState);
}




// CMainFrame::OnPropSheet
// -- pop up the editor setting property sheet

void 
CMainFrame::OnPropSheet()
{
  CPropSheet propSheet;

  // Set help flags
  if (! TestHelpFile())
  {
    propSheet.m_PageEdit.m_psp.dwFlags &= ~PSP_HASHELP;
    propSheet.m_PageCons.m_psp.dwFlags &= ~PSP_HASHELP;
  }

  // Set active page
  switch(GetCurrentMessage()->wParam)
  {
    case ID_PREF_EDITOR:
      propSheet.SetActivePage(&propSheet.m_PageEdit);
      break;
    case ID_PREF_CONSOLE:
      propSheet.SetActivePage(&propSheet.m_PageCons);
      break;
  }
  // Popup property page
  if (propSheet.DoModal() == IDOK)
    propSheet.Apply();
}
