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
 
// SplitFrm.cpp : implementation of the CSplitFrame class
// [Frame class for text and sn editors]

#include "stdafx.h"
#include "WinTL3.h"
#include "SplitFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitFrame

IMPLEMENT_DYNCREATE(CSplitFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CSplitFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSplitFrame)
	ON_WM_GETMINMAXINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitFrame construction/destruction

CSplitFrame::CSplitFrame()
{
}

CSplitFrame::~CSplitFrame()
{
}

BOOL CSplitFrame::OnCreateClient(LPCREATESTRUCT, CCreateContext* pContext)
{
  // create splitter window (this is a vanilla split window)
  return m_wndSplitter.Create(this, 2, 1, CSize( 10, 10 ), pContext);
}

void CSplitFrame::GetMessageString( UINT nID, CString& rMessage ) const
{
  // message string provided by application global function.
  theApp.GetMessageString(nID, rMessage);
}

/////////////////////////////////////////////////////////////////////////////
// CSplitFrame diagnostics

#ifdef _DEBUG
void CSplitFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CSplitFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitFrame message handlers



// CSplitFrame::OnGetMinMaxInfo
// -- prevent creation of too small console windows
//    linked to message WM_GETMINMAXINFO

void 
CSplitFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
  lpMMI->ptMinTrackSize.x = max(lpMMI->ptMinTrackSize.x, 150);
  lpMMI->ptMinTrackSize.y = max(lpMMI->ptMinTrackSize.y, 100);
  CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
}
