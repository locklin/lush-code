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
 
// TL3E.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"   // portable AFX headers

#include "WinTL3.h"
#include "MainFrm.h"
#include "SplitFrm.h"
#include "ConsFrm.h"
#include "TextDoc.h"
#include "TextView.h"
#include "SNView.h"
#include "Console.h"
#include "GraphWin.h"
#include "GraphFrm.h"
#include "Splash.h"
#include "TL3Com.h"

#include <dos.h>
#include <direct.h>
#include <locale.h>
#include <time.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Assertion stuff for BETA versions

#ifdef BETA

void 
WinTL3Abort(LPCSTR sz)
{
  if (AfxMessageBox(sz, MB_ABORTRETRYIGNORE|MB_ICONEXCLAMATION)!=IDIGNORE)
  {
    AfxWinTerm();
    exit(100);
  }
}

void
WinTL3Check()
{
  time_t clk = time(NULL);
  struct tm *tm = localtime(&clk);
  int now = tm->tm_year*100 + tm->tm_mon + 1;
  if (now >= BETA)
  {
    char buffer[256];
    sprintf(buffer, "WINTL3 BETA MESSAGE:\n\n"
  		    "  This beta version expires on %d/%d.\n"
		    "  Please obtain a newer version of WinTL3.",
		    (UINT)BETA%100, (UINT)BETA/100 );
    AfxMessageBox(buffer, MB_OK|MB_ICONEXCLAMATION);
  }
#ifndef _DEBUG
  if (now > BETA)
  {
    AfxWinTerm();
    exit(100);
  }
#endif
}

BOOL 
WinTL3Assert(LPCSTR file, UINT line)
{
  char buffer[256];
  sprintf(buffer, "WINTL3 BETA MESSAGE:\n\n"
		  "  Assertion failed on %s:%d\n"
		  "  Please indicate this message to whoever\n"
		  "  gave you this beta version of WinTL3.",
		  file, line);
  WinTL3Abort(buffer);
  return TRUE;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Strings for the registry

static const char *szBarState =	 "Toolbars\\Info";
static const char *szMainFrame = "MainFrame";
static const char *szMaximized  =     "bMax";
static const char *szFrameRect =      "rectFrame";
static const char *szEditor =	 "Editor";
static const char *szColorize =       "bColorize";
static const char *szAutoIndent =     "bAutoIndent";
static const char *szFontName =	      "szFontName";
static const char *szFontSize =	      "nFontSize";
static const char *szColNormal =      "colorNormal";
static const char *szColComment =     "colorComment";
static const char *szColKeyword =     "colorKeyword";
static const char *szColString =      "colorString";
static const char *szColHelp =        "colorHelp";
static const char *szColDefine =      "colorDefine";
static const char *szConsole =	 "Console";
static const char *szConsFrame =      "rectFrame";
static const char *szConsColorize =   "bColorize";
static const char *szConsColNormal =  "colorNormal";
static const char *szConsColInput =   "colorInput";
static const char *szConsColEdit =    "colorEditable";
static const char *szConsRightMarg =  "nRightMargin";
static const char *szConsWordWrap =   "bWordWrap";
static const char *szConsAutoEOL =    "bAutoEOL";
static const char *szSettings =  "Settings";
static const char *szBuildDate =      "szBuildDate";

/////////////////////////////////////////////////////////////////////////////
// CTL3App

BEGIN_MESSAGE_MAP(CTL3App, CWinApp)
	//{{AFX_MSG_MAP(CTL3App)
	ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdateFileSaveAll)
	ON_COMMAND(ID_FILE_NEW_SN, OnFileNewSn)
	ON_COMMAND(ID_FILE_NEW_TXT, OnFileNewTxt)
	ON_COMMAND(ID_VIEW_CONSOLE, OnViewConsole)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONSOLE, OnUpdateViewConsole)
	ON_COMMAND(ID_RUN_HELPTOOL, OnRunHelptool)
	ON_UPDATE_COMMAND_UI(ID_RUN_HELPTOOL, OnUpdateRunHelptool)
	ON_COMMAND(ID_CONSOLE_HOLD, OnConsoleHold)
	ON_UPDATE_COMMAND_UI(ID_CONSOLE_HOLD, OnUpdateConsoleHold)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTL3App construction

CTL3App::CTL3App()
{
  m_pConsole = 0;
  m_bHidden = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTL3App object

CTL3App theApp;

/////////////////////////////////////////////////////////////////////////////
// The subclassed CMultiDocTemplate for tweaking matchdoctype

class CStrongDocTemplate : public CMultiDocTemplate {
  DECLARE_DYNAMIC(CStrongDocTemplate);
  CStrongDocTemplate(UINT nID, CRuntimeClass* pD, CRuntimeClass* pF, CRuntimeClass* pV)
		    : CMultiDocTemplate(nID, pD, pF, pV) {};
  virtual Confidence MatchDocType( LPCTSTR lpszPathName, CDocument*& rpDocMatch );
};

IMPLEMENT_DYNAMIC(CStrongDocTemplate, CMultiDocTemplate);

// CStrongDocTemplate::MatchDocType 
// -- provide a stronger match for non registered files

CDocTemplate::Confidence 
CStrongDocTemplate::MatchDocType( LPCTSTR lpszPathName, CDocument*& rpDocMatch)
{
  Confidence result = CMultiDocTemplate::MatchDocType(lpszPathName, rpDocMatch);
  if (result < yesAttemptNative)
    return noAttempt;
  return result;
}


/////////////////////////////////////////////////////////////////////////////
// CTL3App initialization


// CTL3App::InitInstance
// -- The place where MFC initializes the application

BOOL 
CTL3App::InitInstance()
{
	// Load options
	Enable3dControls();
	SetRegistryKey("Neuristique");
	LoadStdProfileSettings(6);  
	LoadOptions();

	// Set ANSI locale
	setlocale(LC_ALL,"C");
	setlocale(LC_CTYPE,".ACP");

        // Examine arguments /DDE and /HIDE
        if (__argc>1)
        {
          if (_stricmp(__argv[1],"/dde")==0 ||
              _stricmp(__argv[1],"-dde")==0 )
          {
            __argc=1;
          }
          else if (_stricmp(__argv[1],"/hide")==0 ||
                   _stricmp(__argv[1],"-hide")==0 )
          {
            m_bHidden = TRUE;
            m_nCmdShow = SW_HIDE;
            __argc -= 1;
            __argv += 1;
          }
        }

	// Handle nCmdShow
	CSplashWnd::EnableSplashScreen(TRUE);
	switch (m_nCmdShow)
	{
	  case SW_HIDE:
	  case SW_SHOWMINIMIZED:
	  case SW_MINIMIZE:
	  case SW_SHOWMINNOACTIVE:
	    CSplashWnd::EnableSplashScreen(FALSE);
	    break;
  	  case SW_RESTORE:
	  case SW_SHOW:
	  case SW_SHOWDEFAULT:
	  case SW_SHOWNA:
	  case SW_SHOWNOACTIVATE:
	  case SW_SHOWNORMAL:
	  case SW_SHOWMAXIMIZED:
	  if (m_bMaximized)
	    m_nCmdShow = SW_SHOWMAXIMIZED;
	  break;
	}

	// Create document templates.
	m_pSnDocTemplate = new CStrongDocTemplate(
		IDR_SNTYPE,
		RUNTIME_CLASS(CTextDoc),
		RUNTIME_CLASS(CSplitFrame),
		RUNTIME_CLASS(CSNView) );
	m_pTextDocTemplate = new CMultiDocTemplate(
		IDR_TEXTTYPE,
		RUNTIME_CLASS(CTextDoc),
		RUNTIME_CLASS(CSplitFrame), 
		RUNTIME_CLASS(CTextView) );
	m_pConsoleDocTemplate = new CMultiDocTemplate(
		IDR_CONSOLETYPE,
		RUNTIME_CLASS(CConsDoc),
		RUNTIME_CLASS(CConsFrame), 
		RUNTIME_CLASS(CConsView) );
	m_pGraphDocTemplate = new CMultiDocTemplate(
		IDR_GRAPHTYPE,
		RUNTIME_CLASS(CGraphDoc),
		RUNTIME_CLASS(CGraphFrame), 
		RUNTIME_CLASS(CGraphView) );
	m_pGraphDocTopTemplate = new CMultiDocTemplate(
		IDR_GRAPHTYPE,
		RUNTIME_CLASS(CGraphDoc),
		RUNTIME_CLASS(CGraphTopFrame), 
		RUNTIME_CLASS(CGraphView) );

        // Register document templates.
	AddDocTemplate(m_pSnDocTemplate);
        RegisterShellFileTypes(TRUE);
	AddDocTemplate(m_pTextDocTemplate);
	AddDocTemplate(m_pConsoleDocTemplate);
	AddDocTemplate(m_pGraphDocTemplate);
	AddDocTemplate(m_pGraphDocTopTemplate);

	// Set TLOpen path
	RegisterPath();

	// Create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

        // Enable DDE Execute open
	EnableShellOpen();

	// Show and update main window.
	pMainFrame->LoadBarState(szBarState);
        pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// Create Console
	m_pConsoleDocTemplate->OpenDocumentFile(NULL);
	ASSERT(m_pConsole);
#ifdef BETA
	// Beta version check
	WinTL3Check();
#endif
	// Start Tlisp
	theCom.RunTLisp();

        // Show License
        CString sBuildDate;
        sBuildDate = GetProfileString(szSettings,szBuildDate,"");
        if (sBuildDate != __TIMESTAMP__)
        {
          WriteProfileString(szSettings,szBuildDate, __TIMESTAMP__);
          if (pMainFrame->TestHelpFile())
            WinHelp(IDH_COPYING);
        }
        
        // Return
	return TRUE;
}




// CTL3App::ShowMainWindow
// -- Show main WinTL3 window when initially hidden

void 
CTL3App::ShowMainWindow(BOOL bShow)
{
  if (bShow && m_bHidden)
  {
    m_nCmdShow = SW_SHOW;
    if (m_bMaximized)
      m_nCmdShow = SW_SHOWMAXIMIZED;
    m_pMainWnd->ShowWindow(m_nCmdShow);
    m_bHidden = FALSE;
  }
  else if (!bShow && !m_bHidden)
  {
    m_pMainWnd->ShowWindow(SW_HIDE);
    m_bHidden = TRUE;
  }
}




/////////////////////////////////////////////////////////////////////////////
// CTL3App idle processing



// CTL3App::PumpMessage
// -- Should call waiting routine in com object

BOOL 
CTL3App::PumpMessage()
{
  // wait until message is present
  theCom.OnPumpMessage();
  // dispatch message
  return CWinApp::PumpMessage();
}



// CTL3App::OnIdle
// -- checks if input is present and process it

BOOL 
CTL3App::OnIdle(LONG lCount) 
{
  if (CWinApp::OnIdle(lCount))
    return TRUE;
  theCom.OnIdle();
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CTL3App options



// CTL3App::LoadOptions
// -- loads preferences from the registry (called from InitInstance)

void 
CTL3App::LoadOptions()
{
  BYTE *pb;
  UINT nLen = 0;

  // Frame window state
  m_bMaximized = GetProfileInt(szMainFrame, szMaximized, (int)FALSE);
  m_rectInitialFrame.SetRect(0,0,0,0);
  if (GetProfileBinary(szMainFrame, szFrameRect, &pb, &nLen)) {
    if (nLen == sizeof(CRect)) 
      memcpy(&m_rectInitialFrame, pb, sizeof(CRect));
    delete pb;
  }

  // Editor preferences
  m_bColorize = GetProfileInt(szEditor, szColorize, 1);
  m_bAutoIndent = GetProfileInt(szEditor, szAutoIndent, 1);
  m_sFontName = GetProfileString(szEditor, szFontName, "Courier");
  m_nFontSize = GetProfileInt(szEditor, szFontSize, 10);
  m_colorNormal = GetProfileInt(szEditor, szColNormal, PALETTERGB(0,0,0));
  m_colorComment = GetProfileInt(szEditor, szColComment, PALETTERGB(128,0,0));
  m_colorKeyword = GetProfileInt(szEditor, szColKeyword, PALETTERGB(0,0,128));
  m_colorString = GetProfileInt(szEditor, szColString, PALETTERGB(128,128,128));
  m_colorHelp = GetProfileInt(szEditor, szColHelp, PALETTERGB(128,0,0));
  m_colorDefine = GetProfileInt(szEditor, szColDefine, PALETTERGB(0,128,128));

  // Console preferences
  m_rectConsFrame.SetRect(0,0,0,0);
  if (GetProfileBinary(szConsole, szConsFrame, &pb, &nLen)) {
    if (nLen == sizeof(CRect)) 
      memcpy(&m_rectConsFrame, pb, sizeof(CRect));
    delete pb;
  }
  m_bConsColorize = GetProfileInt(szConsole, szColorize, 1);
  m_colorConsNormal = GetProfileInt(szConsole, szConsColNormal, PALETTERGB(0,0,0));
  m_colorConsInput = GetProfileInt(szConsole, szConsColInput, PALETTERGB(0,0,128));
  m_colorConsEdit = GetProfileInt(szConsole, szConsColEdit, PALETTERGB(0,128,0));
  m_nConsRightMargin = GetProfileInt(szConsole, szConsRightMarg, 250);
  m_bConsWordWrap = GetProfileInt(szConsole, szConsWordWrap, 1);
  m_bConsAutoEOL = GetProfileInt(szConsole, szConsAutoEOL, 1);
}



// CTL3App::SaveOptions
// -- save preferences into the registry 
//    called from CMainFrame::OnClose

void 
CTL3App::SaveOptions()
{
  // Main frame state
  WriteProfileInt(szMainFrame, szMaximized, m_bMaximized);
  WriteProfileBinary(szMainFrame, szFrameRect, (BYTE*)&m_rectInitialFrame, sizeof(CRect));

  // Editor preferences
  WriteProfileInt(szEditor, szColorize, m_bColorize);
  WriteProfileInt(szEditor, szAutoIndent, m_bAutoIndent);
  WriteProfileString(szEditor, szFontName, m_sFontName);
  WriteProfileInt(szEditor, szFontSize, m_nFontSize);
  WriteProfileInt(szEditor, szColNormal, m_colorNormal);
  WriteProfileInt(szEditor, szColComment, m_colorComment);
  WriteProfileInt(szEditor, szColKeyword, m_colorKeyword);
  WriteProfileInt(szEditor, szColString, m_colorString);
  WriteProfileInt(szEditor, szColHelp, m_colorHelp);
  WriteProfileInt(szEditor, szColDefine, m_colorDefine);

  // Console preferences
  WriteProfileBinary(szConsole, szConsFrame, (BYTE*)&m_rectConsFrame, sizeof(CRect));
  WriteProfileInt(szConsole, szColorize, m_bConsColorize);
  WriteProfileInt(szConsole, szConsColNormal, m_colorConsNormal);
  WriteProfileInt(szConsole, szConsColEdit, m_colorConsEdit);
  WriteProfileInt(szConsole, szConsColInput, m_colorConsInput);
  WriteProfileInt(szConsole, szConsRightMarg, m_nConsRightMargin);
  WriteProfileInt(szConsole, szConsWordWrap, m_bConsWordWrap);
  WriteProfileInt(szConsole, szConsAutoEOL, m_bConsAutoEOL);

  // Bar state
  CWnd *pMain = AfxGetMainWnd();
  if (pMain) 
  {
    ASSERT_KINDOF(CMainFrame,pMain);
    CMainFrame* pMainFrame = (CMainFrame*)(pMain);
    pMainFrame->SaveBarState(szBarState);
  }
}


// CTL3App::RegisterPath
// -- Saves the tlopen path into the registry

void
CTL3App::RegisterPath()
{
  char buffer[1024+24];
  int len = GetModuleFileName(GetModuleHandle(NULL), buffer, 1024);
  while (len > 0)
  {
    while (len>0 && buffer[--len]!='\\')
      buffer[len] = 0;
    strcpy(buffer+len+1, "include\\header.h");
    if (_access(buffer, 0) >= 0) 
    {
      buffer[len] = 0;
      WriteProfileString("TLOpen","Path", buffer);
      return;
    }
  }
}



/////////////////////////////////////////////////////////////////////////////
// CTL3App Operations



// CTL3App::ViewConsole
// -- activate console and bring it to front

void 
CTL3App::ViewConsole() 
{
  CWnd *pMain = AfxGetMainWnd();
  ASSERT_KINDOF(CMainFrame, pMain);
  CMainFrame *pMainFrame = (CMainFrame*)pMain;
  ASSERT(m_pConsole && m_pConsole->m_consView);
  CWnd* pConsoleFrame = m_pConsole->m_consView->GetParentFrame();
  ASSERT(pConsoleFrame);
  // restore iconic console
  if (pConsoleFrame->IsIconic())
    pMainFrame->MDIRestore(pConsoleFrame);
  // activate console
  pMainFrame->MDIActivate(pConsoleFrame);
  // make sure window is visible
  pMainFrame->MakeAllVisible();
}



// CTL3App::SetStatusMessage
// -- set message string in the status bar

void 
CTL3App::SetStatusMessage(LPCSTR lpsz)
{
  CWnd *pMain = AfxGetMainWnd();
  ASSERT(pMain);
  ASSERT_KINDOF(CMainFrame, pMain);
  CMainFrame *mf = (CMainFrame*)pMain;
  if (lpsz)
  {
    mf->SetMessageText(lpsz);
  }
  else
  {
#ifdef AFX_IDS_IDLEMESSAGE
    mf->SetMessageText(AFX_IDS_IDLEMESSAGE);
#else
    mf->SetMessageText(AFX_IDS_APP_TITLE);
#endif
    // Change cursor 
    POINT point;
    if (GetCursorPos(&point))
    {
      CWnd *pWnd = CWnd::WindowFromPoint(point);
      if (pWnd->IsKindOf(RUNTIME_CLASS(CGraphView)))
	((CGraphView*)pWnd)->SetStatusCursor();
    }
  }
  mf->UpdateWindow();
}



// CTL3App::GetMessageString
// -- returns message string for command ID.
//    display a convenient idle message

void 
CTL3App::GetMessageString( UINT nID, CString& rMessage ) const
{
  CWnd *pMain = AfxGetMainWnd();
  ASSERT(pMain);
  ASSERT_KINDOF(CMainFrame, pMain);
  CMainFrame *mf = (CMainFrame*)pMain;
  // redefine idle message
  if (nID==AFX_IDS_IDLEMESSAGE)
  {
    if (!theCom.m_bThreadStarted)
      nID = IDS_TERM_MSG;
    else if (!theCom.m_bThreadWaiting)
      nID = IDS_RUNNING_MSG;
    else if (theApp.m_pConsole->m_bHeld)
      nID = IDS_HELD_MSG;
    else
      nID = IDS_READY_MSG;
  }
  // use definition provided by MFC
  mf->CMDIFrameWnd::GetMessageString(nID, rMessage);
}


// CTL3App::EnableWindows
// -- enable or disable main window (all the application in fact)

void 
CTL3App::EnableWindows(BOOL bEnable)
{
  CWnd *pMain = AfxGetMainWnd();
  if (pMain)
    pMain->EnableWindow(bEnable);
}



// CTL3App::CheckMessage
// -- process pending messages and return TRUE is escape key is depressed.

BOOL 
CTL3App::CheckMessage()
{
  MSG msg;
  BOOL bFound = FALSE;
  while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) 
  {
    bFound = TRUE;
    // Process message
    if (! PumpMessage()) 
    {
      PostQuitMessage(msg.wParam); 
      return TRUE;
    }
  }
  // Process idle loop
  if (bFound)
  {
    long lIdle = 0;
    while (OnIdle(lIdle))
      lIdle++;
  }
  // ESCAPE key has been depressed
  if (GetKeyState(VK_ESCAPE) < 0)
    return TRUE;
  else
    return FALSE;
}




/////////////////////////////////////////////////////////////////////////////
// CTL3App overrides



// CTL3App::DoPromptFileName
// -- Make sure that flag OFN_NOCHANGEDIR is set and TLisp thread halted
//    The FileOpen and FileSave dialog box indeed change CWD! 

BOOL 
CTL3App::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags,
			  BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
  lFlags |= OFN_NOCHANGEDIR;
  theCom.SuspendTLisp();
  BOOL bStatus = CWinApp::DoPromptFileName(fileName, nIDSTitle, lFlags,
					  bOpenFileDialog, pTemplate);
  theCom.ResumeTLisp();
  return bStatus;
}




/////////////////////////////////////////////////////////////////////////////
// CTL3App commands



// CTL3App::OnFileOpen
// -- make sure that we call DoPromptFileName here
//    linked to command ID_FILE_OPEN
 
void 
CTL3App::OnFileOpen() 
{
  CString newName;
  if (!DoPromptFileName(newName, AFX_IDS_OPENFILE,
      OFN_HIDEREADONLY|OFN_FILEMUSTEXIST, TRUE, NULL))
    return; // open cancelled
  AfxGetApp()->OpenDocumentFile(newName);
}



// CTL3App::OnFileSaveAll
// -- save all modified files
//    linked to command ID_FILE_SAVE_ALL

void 
CTL3App::OnFileSaveAll() 
{
   POSITION templpos = GetFirstDocTemplatePosition();
   while (templpos) {
     CDocTemplate *tmpl = GetNextDocTemplate(templpos);
     POSITION docpos = tmpl->GetFirstDocPosition();
     while (docpos) {
       CDocument *doc = tmpl->GetNextDoc(docpos);
       if (doc->IsModified())
	 doc->DoFileSave();
     }
   }
}



// CTL3App::OnUpdateFileSaveAll
// -- test if there are modified files and update command ID_FILE_SAVE_ALL

void 
CTL3App::OnUpdateFileSaveAll(CCmdUI* pCmdUI) 
{
   BOOL modified = FALSE;
   POSITION templpos = GetFirstDocTemplatePosition();
   while (templpos && !modified) {
     CDocTemplate *tmpl = GetNextDocTemplate(templpos);
     POSITION docpos = tmpl->GetFirstDocPosition();
     while (docpos && !modified) {
       CDocument *doc = tmpl->GetNextDoc(docpos);
       if (doc->IsModified())
	 modified = TRUE;
     }
   }
   pCmdUI->Enable(modified);
}



// CTL3App::OnFileNewSn
// -- creates a new SN document
//    linked to command ID_FILE_NEW_SN 

void 
CTL3App::OnFileNewSn() 
{
  m_pSnDocTemplate->OpenDocumentFile(NULL);
}



// CTL3App::OnFileNewTxt
// -- creates a new text document
//    linked to command ID_FILE_NEW_TXT

void 
CTL3App::OnFileNewTxt() 
{
  m_pTextDocTemplate->OpenDocumentFile(NULL);
}


// CTL3App::OnUpdateViewConsole
// -- checks if console is the active window and update command ID_VIEW_CONSOLE

void 
CTL3App::OnUpdateViewConsole(CCmdUI* pCmdUI) 
{
  CWnd *pMain = AfxGetMainWnd();
  if (pMain && m_pConsole && m_pConsole->m_consView)
  {
    ASSERT_KINDOF(CMDIFrameWnd, pMain);
    CMDIFrameWnd *pMDIFrame = (CMDIFrameWnd*)pMain;
    CWnd* pConsoleFrame = m_pConsole->m_consView->GetParentFrame();
    pCmdUI->SetCheck(pMDIFrame->MDIGetActive() == pConsoleFrame);
  }
}



// CTL3App::OnViewConsole
// -- toggle console and frontmost window
//    linked to command ID_VIEW_CONSOLE

void 
CTL3App::OnViewConsole() 
{
  CWnd *pMain = AfxGetMainWnd();
  ASSERT_KINDOF(CMDIFrameWnd, pMain);
  CMDIFrameWnd *pMDIFrame = (CMDIFrameWnd*) pMain;
  ASSERT(m_pConsole && m_pConsole->m_consView);
  CWnd* pConsoleFrame = m_pConsole->m_consView->GetParentFrame();
  // Toggle console and last active frame
  if (pMDIFrame->MDIGetActive() == pConsoleFrame)
  {
    pMDIFrame->MDINext();
    CWnd *pNewActiveFrame = pMDIFrame->MDIGetActive();
    // Shift F9 skips avoids selecting graphic windows
    if (::GetKeyState(VK_SHIFT) < 0) {
      while (pNewActiveFrame &&
             pNewActiveFrame->IsKindOf(RUNTIME_CLASS(CGraphFrame))) {
        pMDIFrame->MDINext();
        pNewActiveFrame = pMDIFrame->MDIGetActive();
      }
    }
    // Make sure window is not iconic
    if (pNewActiveFrame && pNewActiveFrame->IsIconic())
      pMDIFrame->MDIRestore(pNewActiveFrame);
  }
  else
  {
    ViewConsole();
  }
}



// CTL3App::OnUpdateConsoleHold
// -- sets hold indicator on console's toolbar
//    linked to command ID_CONSOLE_HOLD

void 
CTL3App::OnUpdateConsoleHold(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(m_pConsole ? TRUE : FALSE);
  if (m_pConsole)
    pCmdUI->SetCheck(m_pConsole->m_bHeld);
}



// CTL3App::OnConsoleHold
// -- start or resume automatic feeding of input text to the TL3 thread
//    linked to command ID_CONSOLE_HOLD

void 
CTL3App::OnConsoleHold() 
{
  if (m_pConsole)
  {
    BOOL bHeld = !m_pConsole->m_bHeld;
    m_pConsole->m_bHeld = bHeld;
    SetStatusMessage();
    CWnd *pMain = AfxGetMainWnd();
    ASSERT(pMain);
    ASSERT_KINDOF(CMainFrame, pMain);
    ((CMainFrame*)pMain)->SetScrollLock(bHeld);
  }
}


// CTL3App::OnUpdateRunHelptool
// -- cannot send command Helptool unless 
//    console is present and TLisp is running

void 
CTL3App::OnUpdateRunHelptool(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(m_pConsole && theCom.m_bThreadStarted);
}


// CTL3App::OnRunHelptool
// -- send command Helptool to console

void 
CTL3App::OnRunHelptool() 
{
  if (m_pConsole && m_pConsole->m_consView)
  {
    m_pConsole->m_consView->HideSelection();
    m_pConsole->Append("(helptool)");
    m_pConsole->m_consView->ShowSelection();
    ViewConsole();
  }
}


