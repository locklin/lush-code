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
 
// ConsFrame.cpp : implementation file
//

#include "stdafx.h"
#include "WinTL3.h"
#include "ConsFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConsFrame

IMPLEMENT_DYNCREATE(CConsFrame, CMDIChildWnd)

CConsFrame::CConsFrame()
{
}

CConsFrame::~CConsFrame()
{
}


BEGIN_MESSAGE_MAP(CConsFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CConsFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConsFrame message handlers


// CConsFrame::GetMessageString
// -- use definition provided by application

void 
CConsFrame::GetMessageString( UINT nID, CString& rMessage ) const
{
  theApp.GetMessageString(nID, rMessage);
}

// CConsFrame::PreCreateWindow
// -- create new console location on last saved location

BOOL 
CConsFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
  CRect &rectFrame = theApp.m_rectConsFrame;
  if (cs.hwndParent)
  {
    // make sure window will be visible and large enough
    CRect rectMDI, rect;
    CWnd *pMDI = CWnd::FromHandle(cs.hwndParent);
    pMDI->GetClientRect(rectMDI);
    if (rect.IntersectRect(rectMDI, rectFrame))
    {
      if (rect.Width()>150 && rect.Height()>100)
      {
        cs.x = rect.left;
        cs.y = rect.top;
        cs.cx = rect.Width();
        cs.cy = rect.Height();
      }
    }
  }
  return CMDIChildWnd::PreCreateWindow(cs);
}



// CConsFrame::OnCreate
// -- setup toolbar after frame window is created
//    linked to message WM_CREATE

int 
CConsFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
	  return -1;
  if (!m_wndToolBar.Create(this) ||
      !m_wndToolBar.LoadToolBar(IDR_CONSFRAME))
  {
          TRACE0("Failed to create toolbar\n");
          return -1;      // fail to create
  }
  m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
          CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_ALIGN_TOP   );
  return 0;
}



// CConsFrame::OnGetMinMaxInfo
// -- prevent creation of too small console windows
//    linked to message WM_GETMINMAXINFO

void 
CConsFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
  lpMMI->ptMinTrackSize.x = max(lpMMI->ptMinTrackSize.x, 150);
  lpMMI->ptMinTrackSize.y = max(lpMMI->ptMinTrackSize.y, 100);
  CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
}



// CConsFrame::OnSize
// -- store new console location (saved into registry on app exit)
//    linked to message WM_SIZE

void 
CConsFrame::OnSize(UINT nType, int cx, int cy) 
{
  CMDIChildWnd::OnSize(nType, cx, cy);
  if (nType == SIZE_RESTORED) 
  { 
    CWnd* pWndParent = GetParent();
    ASSERT(pWndParent);
    GetWindowRect(theApp.m_rectConsFrame);
    pWndParent->ScreenToClient(theApp.m_rectConsFrame);
  }
}



// CConsFrame::OnMove
// -- store new console location (saved into registry on app exit)
//    linked to message WM_MOVE

void 
CConsFrame::OnMove(int x, int y) 
{
    CMDIChildWnd::OnMove(x, y);
    CWnd* pWndParent = GetParent();
    ASSERT(pWndParent);
    GetWindowRect(theApp.m_rectConsFrame);
    pWndParent->ScreenToClient(theApp.m_rectConsFrame);
}



// CConsFrame::OnCmdMsg
// -- commands ID_VIEW_TOOLBAR, ID_VIEW_STATUS_BAR 
//    are not processed by this frame (they do not affect the component
//    of the console frame).

BOOL 
CConsFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHInfo) 
{
  switch (nID)
  {
  case ID_VIEW_TOOLBAR:
  case ID_VIEW_STATUS_BAR:
    return FALSE;
  default:
    return CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHInfo);
  }
}


// CConsFrame::OnClose
// -- Closing the console is just like closing the application

void 
CConsFrame::OnClose() 
{
    CWnd *pMain = AfxGetMainWnd();
    ASSERT(pMain);
    pMain->SendMessage(WM_CLOSE);
}


