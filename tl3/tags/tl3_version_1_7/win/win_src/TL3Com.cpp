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
 
// TL3Com.cpp
// Implements communication between Tlisp thread and WinTL3


#include "stdafx.h"
#include <process.h>
#include <fcntl.h>
#include <io.h>

#include "win32.h"

#include "TL3Com.h"
#include "TextDoc.h"
#include "TextView.h"
#include "Console.h"
#include "GraphFrm.h"
#include "GraphWin.h"
#include "WinTL3.h"


#define PIPESIZE 1024


//////////////////////////////////////////////////////////////////////
//// Class CTL3Com handle all communications



// CTL3Com::CTL3Com
// -- create all communication objects

CTL3Com::CTL3Com()
{
  m_bThreadStarted = FALSE;
  m_bThreadWaiting = FALSE;
  m_bPipeThreadMustDie = FALSE;
  m_hthrTLisp = NULL;
  m_hthrPipe = NULL;

  // Create RPC events
  m_hevSendMessage = CreateEvent(NULL,FALSE,FALSE,NULL);
  m_hevReplyMessage = CreateEvent(NULL,FALSE,FALSE,NULL);
  ASSERT(m_hevSendMessage);
  ASSERT(m_hevReplyMessage);

  // Create PIPE events
  m_hevPipeReady = CreateEvent(NULL,FALSE,FALSE,NULL);
  m_hevPipeAgain = CreateEvent(NULL,FALSE,FALSE,NULL);
  ASSERT(m_hevPipeReady);
  ASSERT(m_hevPipeAgain);
  m_bufPipe = new char[PIPESIZE];
 
  // Make STDOUT handle
  HANDLE wPipe;
  CreatePipe(&m_hfStdoutPipe, &wPipe, NULL, PIPESIZE);
  DuplicateHandle(GetCurrentProcess(), wPipe,
		  GetCurrentProcess(), &m_hfStdoutInheritable,
		  0L, TRUE, DUPLICATE_SAME_ACCESS );

  // Install new handle as CRT standard output 
  // This releted to the internal mechanisms of MSVC C RunTime (CRT)
  int nStdout = _open_osfhandle((long)wPipe, _O_WRONLY|_O_BINARY);
  dup2(nStdout, 1);
  dup2(nStdout, 2);
  close(nStdout);
  stdout->_file = 1;
  stderr->_file = 2;
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  // Setup TL3PORT object
  static TL3PORT thePort;
  tl3port = &thePort;
  tl3port->hfStdoutInheritable = m_hfStdoutInheritable;
  tl3port->hevSendMessage = m_hevSendMessage;
  tl3port->hevReplyMessage = m_hevReplyMessage;
}



// CTL3Com::~CTL3Com
// -- destroys all communication objects

CTL3Com::~CTL3Com()
{
  ASSERT(!m_bThreadStarted);
  // Terminate TLisp thread
  if (m_hthrTLisp)
  {
    KillTLisp();
  }
  // Terminate pipe thread
  if (m_hthrPipe)
  {
    m_bPipeThreadMustDie = TRUE;
    fputs("\n<EOF>\n",stdout);
    SetEvent(m_hevPipeAgain);
    if (WaitForSingleObject(m_hthrPipe, 1000) != WAIT_OBJECT_0)
      TerminateThread(m_hthrPipe, 0);
    CloseHandle(m_hthrPipe);
    m_hthrPipe = NULL;
  }
  // Close everything
  close(0);
  close(1);
  close(2);
  CloseHandle(m_hevPipeReady);
  CloseHandle(m_hevPipeAgain);
  CloseHandle(m_hfStdoutPipe);
  CloseHandle(m_hfStdoutInheritable);
  CloseHandle(m_hevSendMessage);
  CloseHandle(m_hevReplyMessage);
  if (m_bufPipe)
    delete m_bufPipe;
}




////////////////////////////////////////////////////////////////////
///// TL3Com definition

// theCom 
// -- global variable managing all communications

CTL3Com theCom;




////////////////////////////////////////////////////////////////////
///// TL3Com operations


// pipe_start
// -- this is the entry point for the Pipe thread

static unsigned __stdcall
pipe_start(void *)
{
  for(;;)
  {
    theCom.m_nBytesFromPipe = 0;
    if (! ReadFile(theCom.m_hfStdoutPipe, theCom.m_bufPipe,
                    PIPESIZE, &theCom.m_nBytesFromPipe, NULL) )
      break;
    if (theCom.m_bPipeThreadMustDie || ! theCom.m_nBytesFromPipe)
      continue;
    SetEvent(theCom.m_hevPipeReady);
    DWORD dwResult = WaitForSingleObject(theCom.m_hevPipeAgain,INFINITE);
    if (theCom.m_bPipeThreadMustDie || dwResult != WAIT_OBJECT_0)
      break;
  }
  return 0;
}



// tlisp_start
// -- this is the entry point for the TLisp thread

static unsigned __stdcall
tlisp_start(void*)
{
  __try 
  {
    return tl3main(__argc, __argv);
  } 
  __except (tl3exception(GetExceptionInformation()))
  {
    return GetExceptionCode();
  }
}


// CTL3Com::RunTLisp
// -- starts the TLisp thread

void 
CTL3Com::RunTLisp()
{
  ASSERT(!m_bThreadStarted);

  // Disable memory leak tracking 
  // (TLisp exits with memory allocated)
  AfxEnableMemoryTracking(FALSE);

  // Test for Win32s and buggy WinNT
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (GetVersionEx(&versionInfo)) 
  {
      if (versionInfo.dwPlatformId==VER_PLATFORM_WIN32s)
      {
        AfxMessageBox(IDP_CANNOT_RUN_ON_WIN32S, MB_OK|MB_ICONSTOP);
        AfxThrowUserException();
      }
  }
  // Create Pipe thread
  unsigned int thrID;
  if (!m_hthrPipe)
  {
    m_hthrPipe = (HANDLE)_beginthreadex(NULL, 0, pipe_start, NULL, 0, &thrID);
    if (! m_hthrPipe)
    {
      AfxMessageBox(IDP_CANNOT_CREATE_THREAD, MB_OK|MB_ICONSTOP);
      AfxThrowUserException();
    }
  }
  // Create TLisp thread
  if (! m_hthrTLisp)
  {
    m_hthrTLisp = (HANDLE)_beginthreadex(NULL, 0, tlisp_start, NULL, 0, &thrID);
    if (! m_hthrTLisp)
    {
      AfxMessageBox(IDP_CANNOT_CREATE_THREAD, MB_OK|MB_ICONSTOP);
      AfxThrowUserException();
    }
  }
  // Let's go
  m_bThreadStarted = TRUE;
  theApp.SetStatusMessage();
}



// CTL3Com::SuspendTLisp()
// -- suspend the TLisp thread

void 
CTL3Com::SuspendTLisp()
{
  if (m_bThreadStarted)
  {
    ASSERT(m_hthrTLisp);
    SuspendThread(m_hthrTLisp);
  }
}


// CTL3Com::ResumeTLisp()
// -- resume the TLisp thread

void 
CTL3Com::ResumeTLisp()
{
  if (m_bThreadStarted)
  {
    ASSERT(m_hthrTLisp);
    ResumeThread(m_hthrTLisp);
  }
}


// CTL3Com::BreakTLisp()
// -- sends a break to TLisp 

void 
CTL3Com::BreakTLisp()
{
  break_attempt = 1;
  // flush if necessary
  if (m_bThreadWaiting)
  {
      SetEvent(m_hevReplyMessage);
      m_bThreadWaiting = FALSE;
  }
}


// CTL3Com::KillTLisp()
// -- sends a break to *kill* TLisp 

void 
CTL3Com::KillTLisp()
{
  if (m_bThreadStarted)
  {
    // SEND SUICIDE REQUEST
    kill_attempt = 1;
    break_attempt = 1;
    // Waits until nice exit or timeout
    CWaitCursor wait;
    HANDLE events[2];
    events[0] = m_hevSendMessage;
    events[1] = m_hevPipeReady;
    DWORD killdate = GetTickCount();
    while (m_bThreadStarted)
    {
      // Cancel pending RPC
      if (m_bThreadWaiting)
      {
        break_attempt = 1;
        kill_attempt = 1;
        SetEvent(m_hevReplyMessage);
        m_bThreadWaiting = FALSE;
      }
      // Check if wait exceeded time limit
      if (GetTickCount() - killdate > 10000)
        break;
      // Wait for RPC or Output
      switch (WaitForMultipleObjects(2, events, FALSE, 500))
      {
      case WAIT_OBJECT_0:
        DispatchRPC();
        break;
      case WAIT_OBJECT_0 + 1:
        DisplayOutput();
        break;
      }
    }
    // USE STRONG METHOD IF TLISP RESISTS
    if (m_bThreadStarted)
    {
      if (AfxMessageBox(IDP_CANNOT_CLOSE_NICELY, 
                        MB_OKCANCEL | MB_ICONSTOP) != IDOK) 
      {
        // Cancel kill
        kill_attempt = 0;
        break_attempt = 0;
        return;
      }
      m_bThreadStarted = FALSE;
      CloseHandle(m_hthrTLisp);
      m_hthrTLisp = NULL;
      AfxGetMainWnd()->SendMessage(WM_CLOSE);
    }
  }
}




// CTL3Com::EventAdd
// -- post an event to win_driver.c

void 
CTL3Com::EventAdd(CGraphDoc *wid, int code, int arg, int x, int y)
{
  tl3event_add((void*)wid, code, arg, x, y);
}



////////////////////////////////////////////////////////////////////
///// RPC, Output and Event dispatcher


inline void 
CTL3Com::DisplayOutput()
{
  if (m_nBytesFromPipe > 0)
  {
    CString str(m_bufPipe, m_nBytesFromPipe);
    theApp.m_pConsole->Write(str);
  }
  SetEvent(m_hevPipeAgain);
  Sleep(0);
}





// CTL3Com::DispatchRPC
// -- RPC messages are received here.
//    This function test the message code and calls
//    the proper RPC routine

void 
CTL3Com::DispatchRPC()
{
  ASSERT(!m_bThreadWaiting);
  switch (tl3port->msgcode)
  {
  case TL3_EXIT:
    RPCExit(tl3port->msgarg);
    break;
  case TL3_WINEDIT:
    RPCWinEdit(tl3port->msgarg);
    break;
  case TL3_GETLINE:
    RPCBeginGetline(tl3port->msgarg);
    break;
  case TL3_WAITEVENT:
    RPCBeginWaitEvent(tl3port->msgarg);
    break;
  case TL3_CREATEWIN:
    RPCCreateWindow(tl3port->msgarg);
    break;
  case TL3_CREATETOPWIN:
    RPCCreateTopWindow(tl3port->msgarg);
    break;
  case TL3_CLOSEWIN:
    RPCCloseWindow(tl3port->msgarg);
    break;
  case TL3_UPDATEWIN:
    RPCUpdateWindow(tl3port->msgarg);
    break;
  case TL3_QUERYWIN:
    RPCQueryWindow(tl3port->msgarg);
    break;
  case TL3_CONFIGWIN:
    RPCConfigWindow(tl3port->msgarg);
    break;
  case TL3_HILITEWIN:
    RPCHiliteWindow(tl3port->msgarg);
    break;
  case TL3_SHOWWORKBENCH:
    RPCShowWorkbench(tl3port->msgarg);
    break;
  default:
    TRACE1("Unknown RPC #%d (task held)\n", tl3port->msgcode);
    break;
  }
}



// CTL3Com::OnIdle
// -- code for dispatching finishing code of RPC 
//    useful for RPC that cause Tlisp to wait

void 
CTL3Com::OnIdle()
{
  if (m_bThreadStarted && m_bThreadWaiting)
  {
    /* Call finishing routines */
    switch (m_nThreadWaitingMsgCode)
    {
    case TL3_GETLINE:
      RPCFinishGetline();
      break;
    case TL3_WAITEVENT:
      RPCFinishWaitEvent();
      break;
    default:
      TRACE1("Unknown RPC finishing code %d\n", 
              m_nThreadWaitingMsgCode);
      break;
    }
    /* Stop waiting immediatly if break is pending */
    if (break_attempt && m_bThreadWaiting)
    {
      SetEvent(m_hevReplyMessage);
      m_bThreadWaiting = FALSE;
    }
  }
}



// CTL3Com::OnPumpMessage
// -- This is the function for scheduling processing
//    of (a) RPC messages, (b) Tlisp output, (c) Windows messages

void 
CTL3Com::OnPumpMessage()
{
  MSG msg;
  HANDLE events[2];

  events[0] = m_hevSendMessage;
  events[1] = m_hevPipeReady;

  while (m_bThreadStarted && !m_bThreadWaiting)
  {
    DWORD dwResult;
    // quickly checks for events
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
      return;
    // quickly checks for rpc
    dwResult = MsgWaitForMultipleObjects(2, events, 
                       FALSE, 0, QS_ALLEVENTS );
    // go waiting
    if (dwResult == WAIT_TIMEOUT)
    {
      OnIdle();
      dwResult = MsgWaitForMultipleObjects(2, events, 
                       FALSE, INFINITE, QS_ALLEVENTS );
    }
    switch(dwResult)
    {
    case WAIT_OBJECT_0:
      DispatchRPC();
      break;
    case WAIT_OBJECT_0 + 1:
      DisplayOutput();
      break;
    }
  }
  // Check for pending output
  if (m_bThreadStarted && m_bThreadWaiting)
  {
    while (WaitForSingleObject(m_hevPipeReady,0)==WAIT_OBJECT_0)
      DisplayOutput();
  }
  // Always perform cleanup task
  OnIdle();
}





////////////////////////////////////////////////////////////////////
///// RPC handlers



// CTL3Com::RPCExit
// -- Handle EXIT RPC

void 
CTL3Com::RPCExit(LPARG)
{
  SetEvent(m_hevReplyMessage);
  // Wait until thread terminates
  ASSERT(m_hthrTLisp);
  WaitForSingleObject(m_hthrTLisp, INFINITE);
  // Display output
  while (WaitForSingleObject(m_hevPipeReady,0)==WAIT_OBJECT_0)
    DisplayOutput();
  // Free resources
  CloseHandle(m_hthrTLisp);
  m_hthrTLisp = NULL;
  m_bThreadStarted = FALSE;
  theApp.SetStatusMessage();
  CWnd *pMain = AfxGetMainWnd();
  ASSERT(pMain);
  pMain->SendMessage(WM_CLOSE);
}





// CTL3Com::RPCWinEdit
// -- Open an editor window on a file

void 
CTL3Com::RPCWinEdit(LPARG pArg)
{
  CString csDocName(pArg->arg_string.string);
  int mode = pArg->arg_string.status;
  // select convenient operation mode
  if (mode!=MSG_EDIT_NAMED && mode!=MSG_EDIT_UNNAMED)
  {
    char tempdir[256];
    int templen = GetTempPath(sizeof(tempdir),tempdir);
    if (templen>0 && templen<sizeof(tempdir)
        && !_strnicmp(csDocName, tempdir, templen) )
      mode = MSG_EDIT_UNNAMED;
  }
  // lets go
  try
  {
    if (mode == MSG_EDIT_UNNAMED)
    {
      // CREATE AN UNTITLED DOCUMENT
      //
      CDocTemplate *pDocTmpl;
      // searches convenient document template
      if (! _stricmp(csDocName.Right(3),".sn"))
	pDocTmpl = theApp.m_pSnDocTemplate;
      else
	pDocTmpl = theApp.m_pTextDocTemplate;
      // create file and archive
      CFile loadFile(csDocName, CFile::modeRead|CFile::shareDenyWrite);
      CArchive loadArchive(&loadFile, CArchive::load);
      // create new document
      CDocument *pDoc = pDocTmpl->OpenDocumentFile(NULL);
      if (!pDoc)
	AfxThrowUserException();
      pDoc->DeleteContents(); 
      pDoc->SetModifiedFlag();
      loadArchive.m_pDocument = pDoc;
      loadArchive.m_bForceFlat = FALSE;
      CWaitCursor wait;
      if (loadFile.GetLength() != 0)
	pDoc->Serialize(loadArchive);
      loadArchive.Close();
      loadFile.Close();
      pDoc->UpdateAllViews(NULL);
    }
    else
    {
      // CREATE A DOCUMENT ATTACHED TO A FILE
      //
      CDocument *pDoc = theApp.OpenDocumentFile(csDocName);
      if (!pDoc)
	AfxThrowUserException();
    }
    // success
    pArg->arg_string.status = 0;
    SetEvent(m_hevReplyMessage);
  }
  catch( CException* e )
  {
    // failure
    pArg->arg_string.status = -1;
    SetEvent(m_hevReplyMessage);
    e->Delete();
  }
}



// CTL3Com::RPCBeginGetLine
// -- Handle GETLINE RPC

void 
CTL3Com::RPCBeginGetline(LPARG pArg)
{
  // Show main window
  if (theApp.m_bHidden)
    theApp.ShowMainWindow();
  // Start waiting
  m_bThreadWaiting = TRUE;
  m_nThreadWaitingMsgCode = TL3_GETLINE;
  ASSERT(pArg == tl3port->msgarg);
  // Set an acceptable default
  pArg->arg_string.string = "\n";
  pArg->arg_string.status = MSG_GETLINE_NORMAL;
  // Display output
  while (WaitForSingleObject(m_hevPipeReady,0)==WAIT_OBJECT_0)
    DisplayOutput();
  // Check if data is readily available
  RPCFinishGetline();
  // If still waiting
  if (m_bThreadWaiting)
  {
    // Update status message
    theApp.SetStatusMessage();
    // Scroll console to make entry point visible
    if (!theApp.m_pConsole->m_bHeld)
    {
      CTextLoc locFrom, locTo;
      CConsView *consView = theApp.m_pConsole->m_consView;
      consView->GetSelection(locFrom, locTo);
      locTo = theApp.m_pConsole->m_tlOutput;
      if (locTo < locFrom)
        locTo = locFrom;
      consView->ScrollTo(locTo);
    }
  }
}



// CTL3Com::RPCFinishGetLine
// -- Called by OnIdle to terminate the GETLINE RPC

void 
CTL3Com::RPCFinishGetline()
{
  if (m_bThreadWaiting && m_nThreadWaitingMsgCode == TL3_GETLINE)
  {
    BOOL bEof;
    static CString str;
    LPARG pArg = tl3port->msgarg;
    // TEST AVAILABILITY OF EVENTS
    if (tl3event_test())
    {
      // events detected
      pArg->arg_string.string = "\n";
      pArg->arg_string.status = MSG_GETLINE_EVENT;
      m_bThreadWaiting = FALSE;
      SetEvent(m_hevReplyMessage);
      theApp.SetStatusMessage();      
      return;
    }
    // TEST AVAILABILITY OF USER INPUT
    if (!theApp.m_pConsole->m_bHeld 
	&& theApp.m_pConsole->ReadLine(str, bEof))
    {
      pArg->arg_string.string = (LPCSTR)str;
      if (bEof)
	pArg->arg_string.status = MSG_GETLINE_EOF;
      else
	pArg->arg_string.status = MSG_GETLINE_NORMAL;
      m_bThreadWaiting = FALSE;
      SetEvent(m_hevReplyMessage);
      theApp.SetStatusMessage();
      return;
    }
  }
}



// CTL3Com::RPCBeginWaitEvent
// -- waits until events are available

void 
CTL3Com::RPCBeginWaitEvent(LPARG pArg)
{
  if (tl3event_test())
  {
    pArg->arg_simple.status = MSG_OK;    
    SetEvent(m_hevReplyMessage);
  }
  else
  {
    m_bThreadWaiting = TRUE;
    m_nThreadWaitingMsgCode = TL3_WAITEVENT;
    ASSERT(pArg == tl3port->msgarg);
    pArg->arg_simple.status = MSG_ERROR;
    theApp.SetStatusMessage();
  }
}



// CTL3Com::RPCFinishWaitEvent
// -- end wait when events become available

void 
CTL3Com::RPCFinishWaitEvent()
{
  if (m_bThreadWaiting && m_nThreadWaitingMsgCode == TL3_WAITEVENT)
  {
    // Events are now available
    if (tl3event_test())
    {
      ASSERT(tl3port->msgarg);
      tl3port->msgarg->arg_simple.status = MSG_OK;
      m_bThreadWaiting = FALSE;
      SetEvent(m_hevReplyMessage);
      theApp.SetStatusMessage();
    }
  }
}


// CTL3Com::RPCCreateWindow
// -- creates a new WBM window

void
CTL3Com::RPCCreateWindow(LPARG pArg)
{
  // Create a toplevel window if option /hide
  if (theApp.m_bHidden)
  {
    RPCCreateTopWindow(pArg);
    return;
  }

  // Get opening information
  int x = pArg->arg_createwin.x;
  int y = pArg->arg_createwin.y;
  int cx = pArg->arg_createwin.w;
  int cy = pArg->arg_createwin.h;

  // Setup opening information
  CGraphFrame::x = x;
  CGraphFrame::y = y;
  CGraphFrame::cx = cx;
  CGraphFrame::cy = cy;
  CGraphDoc::s_szTitle = pArg->arg_createwin.title;

  // Open document
  CDocument *pDoc = theApp.m_pGraphDocTemplate->OpenDocumentFile(NULL);
  pArg->arg_createwin.status = MSG_ERROR;
  if (pDoc)
  {
    ASSERT_KINDOF(CGraphDoc, pDoc);
    CGraphDoc *pGDoc = (CGraphDoc*)pDoc;
    pArg->arg_createwin.status = MSG_OK;
    pArg->arg_createwin.wid = (void*)pGDoc;
    pArg->arg_createwin.lock = (CRITICAL_SECTION*)pGDoc->s_csOffMap;
    pArg->arg_createwin.hdc = pGDoc->m_dcOffMap.m_hDC;
    pArg->arg_createwin.hpal = pGDoc->m_hPalette;
    pArg->arg_createwin.logpal = pGDoc->s_logPalette;
  }

  // Reply
  SetEvent(m_hevReplyMessage);
}



// CTL3Com::RPCCreateTopWindow
// -- creates a new WBM window

void
CTL3Com::RPCCreateTopWindow(LPARG pArg)
{
  // Get opening information
  int x = pArg->arg_createwin.x;
  int y = pArg->arg_createwin.y;
  int cx = pArg->arg_createwin.w;
  int cy = pArg->arg_createwin.h;
  // Setup opening information
  CGraphTopFrame::x = x;
  CGraphTopFrame::y = y;
  CGraphTopFrame::cx = cx;
  CGraphTopFrame::cy = cy;
  CGraphDoc::s_szTitle = pArg->arg_createwin.title;
  // Open document
  CDocument *pDoc = theApp.m_pGraphDocTopTemplate->OpenDocumentFile(NULL);
  pArg->arg_createwin.status = MSG_ERROR;
  if (pDoc)
  {
    ASSERT_KINDOF(CGraphDoc, pDoc);
    CGraphDoc *pGDoc = (CGraphDoc*)pDoc;
    pArg->arg_createwin.status = MSG_OK;
    pArg->arg_createwin.wid = (void*)pGDoc;
    pArg->arg_createwin.lock = (CRITICAL_SECTION*)pGDoc->s_csOffMap;
    pArg->arg_createwin.hdc = pGDoc->m_dcOffMap.m_hDC;
    pArg->arg_createwin.hpal = pGDoc->m_hPalette;
    pArg->arg_createwin.logpal = pGDoc->s_logPalette;
  }
  // Reply
  SetEvent(m_hevReplyMessage);
}



// CTL3Com::RPCCloseWindow
// -- close WBM window

void
CTL3Com::RPCCloseWindow(LPARG pArg)
{
  CGraphDoc *pGDoc = (CGraphDoc*)pArg->arg_simplewin.wid;
  ASSERT_KINDOF(CGraphDoc, pGDoc);
  pGDoc->OnCloseDocument();
  pArg->arg_createwin.status = MSG_OK;
  SetEvent(m_hevReplyMessage);
}



// CTL3Com::RPCUpdateWindow
// -- update display for a WBM window

void
CTL3Com::RPCUpdateWindow(LPARG pArg)
{
  CGraphDoc *pGDoc = (CGraphDoc*)pArg->arg_configwin.wid;
  ASSERT_KINDOF(CGraphDoc, pGDoc);
  ASSERT(pGDoc->m_pGraphView);
  CRect rect(pArg->arg_configwin.x, pArg->arg_configwin.y,
	     pArg->arg_configwin.w, pArg->arg_configwin.h);
  pGDoc->m_pGraphView->Update(rect);
  pArg->arg_createwin.status = MSG_OK;
  SetEvent(m_hevReplyMessage);
}



// CTL3Com::RPCQueryWindow
// -- return size of WBM window

void
CTL3Com::RPCQueryWindow(LPARG pArg)
{
  CGraphDoc *pGDoc = (CGraphDoc*)pArg->arg_configwin.wid;
  ASSERT_KINDOF(CGraphDoc, pGDoc);
  ASSERT(pGDoc->m_pGraphView);
  // Obtain window dimensions
  pArg->arg_configwin.w = pGDoc->m_nWidth;
  pArg->arg_configwin.h = pGDoc->m_nHeight;
  pArg->arg_configwin.hwnd = pGDoc->m_pGraphView->m_hWnd;
  pArg->arg_configwin.status = MSG_CONFIGWIN_W | MSG_CONFIGWIN_H;
  // Test if window is iconic
  CFrameWnd *pFrame = pGDoc->m_pGraphView->GetParentFrame();
  if (pFrame && pFrame->IsIconic())
    pArg->arg_configwin.status |= MSG_CONFIGWIN_ICON;
  // Reply
  SetEvent(m_hevReplyMessage);
}




// CTL3Com::RPCConfigWindow
// -- change position and size of WBM window

#define MSG_CONFIGWIN_MOVE (MSG_CONFIGWIN_X|MSG_CONFIGWIN_Y)
#define MSG_CONFIGWIN_SIZE (MSG_CONFIGWIN_W|MSG_CONFIGWIN_H)

void
CTL3Com::RPCConfigWindow(LPARG pArg)
{
  // Find graph doc and view
  CGraphDoc *pGDoc = (CGraphDoc*)pArg->arg_configwin.wid;
  ASSERT_KINDOF(CGraphDoc, pGDoc);
  CGraphView *pGView = pGDoc->m_pGraphView;
  ASSERT(pGView);
  // Find frame window
  CFrameWnd *pFrame = pGView->GetParentFrame();
  ASSERT(pFrame);
  CGraphFrame *pGFrame = NULL;
  if (pFrame->IsKindOf(RUNTIME_CLASS(CGraphFrame)))
    pGFrame = (CGraphFrame*)pFrame;
  // Activate window
  if (pArg->arg_configwin.status & MSG_CONFIGWIN_RAISE)
  {
    if (pGFrame)
    {
      if (pGFrame->IsIconic())
        pGFrame->MDIRestore();
      pGFrame->MDIActivate();
    }
    else
    {
      if (pFrame->IsIconic())
        pFrame->ActivateFrame(SW_SHOWNORMAL);
      else
        pFrame->ActivateFrame(SW_SHOW);
    }
  }
  // Move window
  if (pArg->arg_configwin.status & (MSG_CONFIGWIN_MOVE|MSG_CONFIGWIN_SIZE))
  {
    // Restore window if zoomed and active */
    if (pGFrame)
    {
      if (pFrame->IsZoomed() && 
          pGFrame->GetMDIFrame()->MDIGetActive()==pGFrame )
        pGFrame->MDIRestore();
    }
    else
    {
      if (pFrame->IsZoomed())
        pFrame->ActivateFrame(SW_SHOWNORMAL);
    }
    // Compute frame width
    CRect rect;
    pGView->GetClientRect(&rect);
    if (pArg->arg_configwin.status & MSG_CONFIGWIN_W)
      rect.right = pArg->arg_configwin.w;
    if (pArg->arg_configwin.status & MSG_CONFIGWIN_H)
      rect.bottom = pArg->arg_configwin.h;
    AdjustWindowRectEx(&rect, pGView->GetStyle(), FALSE, pGView->GetExStyle());
    AdjustWindowRectEx(&rect, pFrame->GetStyle(), FALSE, pFrame->GetExStyle());
    // Compute frame location
    CRect rectWin;
    pFrame->GetWindowRect(&rectWin);
    if (pFrame->GetParent())
      pFrame->GetParent()->ScreenToClient(&rectWin);
    if (pArg->arg_configwin.status & MSG_CONFIGWIN_X)
      rectWin.left = pArg->arg_configwin.x;
    if (pArg->arg_configwin.status & MSG_CONFIGWIN_Y)
      rectWin.top = pArg->arg_configwin.y;
    // Compute new frame rect and move window
    rect.OffsetRect(rectWin.left-rect.left, rectWin.top-rect.top);
    pFrame->MoveWindow(&rect, TRUE);
    pGView->AdjustSize();
  }

  // Return the new window size and visibility
  RPCQueryWindow(pArg);
}




// CTL3Com::RPCHiliteWindow
// -- implement function 'hilite'

void
CTL3Com::RPCHiliteWindow(LPARG pArg)
{
  CGraphDoc *pGDoc = (CGraphDoc*)pArg->arg_configwin.wid;
  ASSERT_KINDOF(CGraphDoc, pGDoc);
  ASSERT(pGDoc->m_pGraphView);
  CGraphView *pView = pGDoc->m_pGraphView;
  pView->Hilite(pArg->arg_configwin.status, 
		pArg->arg_configwin.x, pArg->arg_configwin.y,
		pArg->arg_configwin.w, pArg->arg_configwin.h );
  pArg->arg_createwin.status = MSG_OK;
  SetEvent(m_hevReplyMessage);
}




// CTL3Com::RPCHiliteWindow
// -- implement function 'hilite'

void
CTL3Com::RPCShowWorkbench(LPARG pArg)
{
  if (pArg->arg_simple.status)
    theApp.ShowMainWindow(TRUE);
  else
    theApp.ShowMainWindow(FALSE);
  pArg->arg_simple.status = MSG_OK;
  SetEvent(m_hevReplyMessage);
}