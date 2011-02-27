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
 
// GraphFrm.cpp : implementation file
//

#include "stdafx.h"
#include "WinTL3.h"
#include "TL3Com.h"
#include "GraphWin.h"
#include "GraphFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CGraphFrame

IMPLEMENT_DYNCREATE(CGraphFrame, CMDIChildWnd)

CGraphFrame::CGraphFrame()
{
}

CGraphFrame::~CGraphFrame()
{
}


BEGIN_MESSAGE_MAP(CGraphFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CGraphFrame)
	ON_WM_CLOSE()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CGraphFrame statics


// CGraphFrame::x,y,cx,cy
// -- size and location of the future window
//    this information is used by PreCreateWindow

int CGraphFrame::x = 0;
int CGraphFrame::y = 0;
int CGraphFrame::cx = 0;
int CGraphFrame::cy = 0;



/////////////////////////////////////////////////////////////////////////////
// CGraphFrame message handlers


// CGraphFrame::GetMessageString
// -- use definition provided by application

void 
CGraphFrame::GetMessageString( UINT nID, CString& rMessage ) const
{
  theApp.GetMessageString(nID, rMessage);
}



// CGraphFrame::PreCreateWindow
// -- setup size and title of new window

BOOL 
CGraphFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
  // Call inherited routine
  if (! CMDIChildWnd::PreCreateWindow(cs))
    return FALSE;
  // Setup default dimension
  if (cx<=0  || cx>=4096)
    cx = 400;
  if (cy<=0  || cy>=4096)
    cy = 400;
  // Compute size of target window
  CRect rectFrame(0, 0, cx, cy);
  AdjustWindowRectEx(&rectFrame, cs.style, FALSE, cs.dwExStyle);
  cs.cx = rectFrame.Width();
  cs.cy = rectFrame.Height();
  // Compute position
  cs.x = x;
  cs.y = y;
  if (x==0 && y==0)
    cs.x = cs.y = CW_USEDEFAULT;
  CRect rectMDIClient;
  if (x<0 || y<0)
  {
    CWnd *pMain = AfxGetMainWnd();
    ASSERT(pMain);
    ASSERT_KINDOF(CMDIFrameWnd, pMain);
    CMDIFrameWnd *pMDIFrame = (CMDIFrameWnd*)pMain;
    ASSERT(::IsWindow(pMDIFrame->m_hWndMDIClient));
    VERIFY(::GetClientRect(pMDIFrame->m_hWndMDIClient, &rectMDIClient));
  }
  if (x < 0)
    cs.x = rectMDIClient.Width() + x - cs.cx;
  if (y < 0)
    cs.y = rectMDIClient.Height() + y - cs.cy;
  // Creation is allowed
  return TRUE;
}




// CGraphFrame::OnClose
// -- called when user presses the close button

void 
CGraphFrame::OnClose() 
{
  CDocument *pDoc = GetActiveDocument();
  if (pDoc)
  {
    ASSERT_KINDOF(CGraphDoc, pDoc);
    CGraphDoc *pGDoc = (CGraphDoc*)pDoc;
    // post event to win_driver
    ASSERT(pGDoc->m_pGraphView);
    pGDoc->m_pGraphView->EventAdd(WM_CLOSE);
  }
  else
  {
    // I do not know if this can happen!
    CMDIChildWnd::OnClose();
  }
}


// CGraphFrame::OnQueryNewPalette
// -- install palette when useful

BOOL 
CGraphFrame::OnQueryNewPalette() 
{
  if (CFrameWnd::OnQueryNewPalette())
    return TRUE;
  CView *pView = GetActiveView();
  ASSERT_KINDOF(CGraphView, pView);
  CGraphView *pGView = (CGraphView*)pView;
  return pGView->SendMessage(WM_QUERYNEWPALETTE);
}


// CGraphFrame::OnPaletteChanged
// -- perform degraded palette assignment on window

void 
CGraphFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
  CFrameWnd::OnPaletteChanged(pFocusWnd);
  CView *pView = GetActiveView();
  ASSERT_KINDOF(CGraphView, pView);
  CGraphView *pGView = (CGraphView*)pView;
  pGView->SendMessage(WM_PALETTECHANGED, (WPARAM)(HWND)*pFocusWnd);
}





/////////////////////////////////////////////////////////////////////////////
// CGraphTopFrame

IMPLEMENT_DYNCREATE(CGraphTopFrame, CFrameWnd)

CGraphTopFrame::CGraphTopFrame()
{
}

CGraphTopFrame::~CGraphTopFrame()
{
}


BEGIN_MESSAGE_MAP(CGraphTopFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CGraphTopFrame)
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CGraphTopFrame statics


// CGraphTopFrame::x,y,cx,cy
// -- size and location of the future window
//    this information is used by PreCreateWindow

int CGraphTopFrame::x = 0;
int CGraphTopFrame::y = 0;
int CGraphTopFrame::cx = 0;
int CGraphTopFrame::cy = 0;


/////////////////////////////////////////////////////////////////////////////
// CGraphTopFrame message handlers



// CGraphTopFrame::PreCreateWindow
// -- setup size and title of new window

BOOL 
CGraphTopFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
  // Call inherited routine
  if (! CFrameWnd::PreCreateWindow(cs))
    return FALSE;
  // Setup default dimension
  if (cx<=0  || cx>=4096)
    cx = 400;
  if (cy<=0  || cy>=4096)
    cy = 400;
  // Disable menu
  cs.hMenu = NULL;
  // Set title
  cs.style &= ~FWS_ADDTOTITLE;
  cs.lpszName = CGraphDoc::s_szTitle;
  // Compute size of target window
  CRect rectFrame(0, 0, cx, cy);
  AdjustWindowRectEx(&rectFrame, cs.style, FALSE, cs.dwExStyle);
  cs.cx = rectFrame.Width();
  cs.cy = rectFrame.Height();
  // Compute position
  cs.x = x;
  cs.y = y;
  if (x==0 && y==0)
    cs.x = cs.y = CW_USEDEFAULT;
  CDisplayIC dc;
  if (x < 0)
    cs.x = dc.GetDeviceCaps(HORZRES) + x - cs.cx;
  if (y < 0)
    cs.y = dc.GetDeviceCaps(VERTRES) + y - cs.cy;
  // Creation is allowed
  return TRUE;
}




// CGraphTopFrame::OnClose
// -- called when user presses the close button

void 
CGraphTopFrame::OnClose() 
{
  CDocument *pDoc = GetActiveDocument();
  if (pDoc)
  {
    ASSERT_KINDOF(CGraphDoc, pDoc);
    CGraphDoc *pGDoc = (CGraphDoc*)pDoc;
    // post event to win_driver
    ASSERT(pGDoc->m_pGraphView);
    pGDoc->m_pGraphView->EventAdd(WM_CLOSE);
  }
  else
  {
    // I do not know if this can happen!
    CFrameWnd::OnClose();
  }
}



// CGraphTopFrame::OnCreate
// -- called when frame is created

int 
CGraphTopFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
	  return -1;
  // Insert disabled popup menu in system menu
  if (menuPopup.LoadMenu(ID_POPUP_GRAPHVIEW))
  {
    CMenu *pmPopup = menuPopup.GetSubMenu(0);
    ASSERT(pmPopup);
    CString strPopup;
    menuPopup.GetMenuString(0, strPopup, MF_BYPOSITION);
    CMenu *pmSys = GetSystemMenu(FALSE);
    pmSys->AppendMenu(MF_SEPARATOR);
    pmSys->AppendMenu(MF_POPUP, (UINT)(HMENU)(*pmPopup), strPopup);
    pmSys->Detach();
  }	
  return 0;
}


// CGraphTopFrame::PreTranslateMessage
// -- convert sysmenu messages into regular messages

BOOL 
CGraphTopFrame::PreTranslateMessage(MSG* pMsg) 
{
  if (pMsg->message == WM_SYSCOMMAND)
  {
    if (pMsg->wParam<0xF000)
    {
      pMsg->message = WM_COMMAND;
      pMsg->lParam = 0;
    }
  }
  return CFrameWnd::PreTranslateMessage(pMsg);
}


// CGraphTopFrame::OnQueryNewPalette
// -- install palette when useful

BOOL 
CGraphTopFrame::OnQueryNewPalette() 
{
  if (CFrameWnd::OnQueryNewPalette())
    return TRUE;
  CView *pView = GetActiveView();
  ASSERT_KINDOF(CGraphView, pView);
  CGraphView *pGView = (CGraphView*)pView;
  return pGView->SendMessage(WM_QUERYNEWPALETTE);
}


// CGraphTopFrame::OnPaletteChanged
// -- perform degraded palette assignment on window

void 
CGraphTopFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
  CFrameWnd::OnPaletteChanged(pFocusWnd);
  CView *pView = GetActiveView();
  ASSERT_KINDOF(CGraphView, pView);
  CGraphView *pGView = (CGraphView*)pView;
  pGView->SendMessage(WM_PALETTECHANGED, (WPARAM)(HWND)*pFocusWnd);
}
