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
 
// TextView.cpp : implementation of the CTextView class
//

#include "stdafx.h"
#include "WinTL3.h"
#include "TextDoc.h"
#include "TextView.h"
#include "TL3Com.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextView

static const UINT nMsgFindReplace = ::RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNCREATE(CTextView, CView)

BEGIN_MESSAGE_MAP(CTextView, CView)
	//{{AFX_MSG_MAP(CTextView)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateNeedSel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateNeedClip)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateNeedUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateNeedText)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPEAT, OnUpdateNeedFind)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
	ON_COMMAND(ID_EDIT_REPEAT, OnEditRepeat)
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateNeedSel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateNeedText)
	//}}AFX_MSG_MAP
	// Indicators
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateOvrIndicator)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LC, OnUpdateLCIndicator)
	// For find+replace dialog
	ON_REGISTERED_MESSAGE(nMsgFindReplace, OnFindReplaceCmd)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()




// CTextView::s_hIBeamCursor
// -- the cursor for these windows

HCURSOR CTextView::s_hIBeamCursor = NULL;




/////////////////////////////////////////////////////////////////////////////
// CTextView construction/destruction

CTextView::CTextView()
{
  m_nFirstVisibleLine = 0;
  m_nFirstVisiblePos = 0;
  m_nLeftMargin = 1440/8;
  m_pTextBuffer = 0;
  m_bDragging = FALSE;
  m_bActive = FALSE;
  m_nSelHideCount = 1;
  m_bOverlayMode = FALSE;
  m_nUpDownPos = -1;
  m_iTimer = 0;
  m_nLogPixSX = 0;
  m_nLogPixSY = 0;
  // create statics
  if (! s_hIBeamCursor)
    s_hIBeamCursor = LoadCursor(NULL, IDC_IBEAM);
}

CTextView::~CTextView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CTextView diagnostics

#ifdef _DEBUG
void CTextView::AssertValid() const
{
	CView::AssertValid();
}

void CTextView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG






/////////////////////////////////////////////////////////////////////////////
// CTextView helpers





// CTextView::LocToLP
// -- compute logical pixel coordinates of specified character
//    compare this coordinates with clipping box

CTextView::SLoc 
CTextView::LocToLP(CDC *pDC, int nLine, int nChar, int& cx, int& cy)
{
  CRect rectClip;
  pDC->GetClipBox(rectClip);
  cx = m_nLeftMargin;
  cy = (nLine - m_nFirstVisibleLine) * m_nCHNormal;
  if (nChar>=0)
  {
    int nPos = GetTextBuffer()->CharToPos(nLine, nChar);
    cx += (nPos - m_nFirstVisiblePos) * m_nCWNormal;
  }
  if (cy > rectClip.bottom)
    return SLBelow;
  if (cy + m_nCHNormal < rectClip.top)
    return SLAbove;
  return SLGotcha;
}



// CTextView::DPToLoc
// --  compute text buffer location corresponding to point (device coordinates). 
//     return TRUE is point falls into left margin.

BOOL 
CTextView::DPToLoc(CPoint point, CTextLoc &loc)
{
  CRect rectClient;
  CTextBuffer* pBuf = GetTextBuffer();
  BOOL ans = FALSE;

  // Compute limits and make twips
  GetWindowRect(rectClient);
  int nVisibleLines = MulDiv(rectClient.Height(), 1440, m_nLogPixSY) / m_nCHNormal;
  point.x = MulDiv(point.x, 1440, m_nLogPixSX);
  point.y = MulDiv(point.y, 1440, m_nLogPixSY);

  // Compute line
  int nLine = (point.y /* + m_nCHNormal/2 */) / m_nCHNormal;
  if (nLine >= nVisibleLines)
    nLine = nVisibleLines - 1;
  if (nLine < 0) 
    nLine = 0;
  nLine += m_nFirstVisibleLine;

  // Compute horizontal position
  if (nLine >= pBuf->GetDocLines())
  {
    loc.nLine = pBuf->GetDocLines() - 1;
    loc.nChar = pBuf->GetLine(loc.nLine).GetLength();
  }
  else
  {
    int nPos = m_nFirstVisiblePos;
    ans = (point.x < m_nLeftMargin);
    if (ans == FALSE)
      nPos += (point.x - m_nLeftMargin + m_nCWNormal/2) / m_nCWNormal;
    loc.nChar = pBuf->PosToChar(nLine, nPos);
    loc.nLine = nLine;
  }
  return ans;
}



// CTextView::InvalidateLines
// -- invalidate lines <nFrom> to <nTo> 
//    these lines will then be redrawn during the next WM_PAINT message

void 
CTextView::InvalidateLines(int nFrom, int nTo)
{
  CRect rect;
  GetClientRect(rect);
  int nVisibleLines = MulDiv(rect.Height(), 1440, m_nLogPixSY) / m_nCHNormal + 1;
  nFrom = max(0, nFrom - m_nFirstVisibleLine);
  if (nTo < 0)
    nTo = nVisibleLines;
  else
    nTo = min(nVisibleLines, nTo - m_nFirstVisibleLine) + 1;
  if (nTo > nFrom)
  {
    rect.top = MulDiv(nFrom * m_nCHNormal, m_nLogPixSY, 1440);
    rect.bottom = MulDiv(nTo * m_nCHNormal, m_nLogPixSY, 1440);
    InvalidateRect(&rect, FALSE);
  }
}



// CTextView::SetScrollBars
// -- set horizontal and vertical scroll bar position, range, and sizes

void 
CTextView::SetScrollBars()
{
  if (m_nLogPixSX && m_nLogPixSY)
  {
    CTextBuffer* pBuf = GetTextBuffer();
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE|SIF_POS|SIF_RANGE|SIF_DISABLENOSCROLL;
    si.nMin = 0;
    CRect rectClient;
    GetClientRect(rectClient);
    // Vertical scrollbar
    si.nMax = pBuf->GetDocLines();
    si.nPos = m_nFirstVisibleLine;
    si.nPage = MulDiv(rectClient.Height(), 1440, m_nLogPixSY);
    si.nPage /= m_nCHNormal;
    SetScrollInfo(SB_VERT, &si);
    // Horizontal scrollbar
    si.nMax = pBuf->GetDocChars();
    si.nPos = m_nFirstVisiblePos;
    si.nPage = MulDiv(rectClient.Width(), 1440, m_nLogPixSX) - m_nLeftMargin;
    si.nPage /= m_nCWNormal;
    SetScrollInfo(SB_HORZ, &si);
    // Adjust window position
    if ((GetScrollPos(SB_VERT)==0 && m_nFirstVisibleLine>0) 
	|| (GetScrollPos(SB_HORZ)==0 && m_nFirstVisiblePos>0) )
      Scroll(0,0);
  }
}



// CTextView::Scroll
// -- scroll editor window by <nl> lines and <nc> columns

void 
CTextView::Scroll(int nl, int nc)
{
  SCROLLINFO si;
  // Set scrollbar
  si.cbSize = sizeof(si);
  si.fMask = SIF_POS|SIF_DISABLENOSCROLL;
  si.nPos = m_nFirstVisibleLine + nl;
  SetScrollInfo(SB_VERT, &si);
  si.nPos = m_nFirstVisiblePos + nc;
  SetScrollInfo(SB_HORZ, &si);
  // Get scroll information
  GetScrollInfo(SB_VERT, &si, SIF_POS);
  nl = si.nPos - m_nFirstVisibleLine;
  GetScrollInfo(SB_HORZ, &si, SIF_POS);
  nc = si.nPos - m_nFirstVisiblePos;
  // Perform scroll if necessary
  if (nl || nc)
  {
    UpdateWindow();
    if (m_tlSelFrom == m_tlSelTo)
      HideSelection();
    m_nFirstVisibleLine += nl;
    m_nFirstVisiblePos += nc;
    CRect rect;
    GetClientRect(rect);
    nc = MulDiv(nc * m_nCWNormal, m_nLogPixSX, 1440);
    nl = MulDiv(nl * m_nCHNormal, m_nLogPixSY, 1440);
    rect.left = MulDiv(m_nLeftMargin, m_nLogPixSX, 1440);
    ScrollWindow(-nc, -nl, &rect, &rect);
    if (m_tlSelFrom == m_tlSelTo)
      ShowSelection();
  }
}



// CTextView::ScrollTo
// -- scrolls editor window until location <loc> is visible

void 
CTextView::ScrollTo(CTextLoc loc)
{
  // Get page information
  CRect rectClient;
  GetClientRect(rectClient);
  int nLastLine = MulDiv(rectClient.Height(),1440,m_nLogPixSY);
  int nLastPos = MulDiv(rectClient.Width(),1440,m_nLogPixSX) - m_nLeftMargin;
  nLastLine = nLastLine/m_nCHNormal + m_nFirstVisibleLine - 1;
  nLastPos = nLastPos/m_nCWNormal + m_nFirstVisiblePos - 1;
  int nPos = GetTextBuffer()->CharToPos(loc.nLine, loc.nChar);
  // Compute vertical displacement
  int nl = 0;
  if (loc.nLine < m_nFirstVisibleLine)
    nl = loc.nLine - m_nFirstVisibleLine;
  else if (loc.nLine >= nLastLine)
    nl = loc.nLine - nLastLine;
  // Compute horizontal displacement
  int nc = 0;
  if (nPos < m_nFirstVisiblePos)
    nc = nPos - m_nFirstVisiblePos;
  else if (nPos >= nLastPos)
    nc = nPos - nLastPos;
  // Scroll
  if (nl || nc)
    Scroll(nl, nc);
}


/////////////////////////////////////////////////////////////////////////////
// CTextView selection



// CTextView::SelectionRgn
// -- computes region inverted by current selection

void 
CTextView::SelectionRgn(CDC *pDC, CRgn &rgnResult)
{
  CTextLoc tlFrom = m_tlSelFrom;
  CTextLoc tlTo = m_tlSelTo;
  rgnResult.CreateRectRgn(0,0,0,0);

  if (tlTo < tlFrom) 
  {
    tlFrom = m_tlSelTo;
    tlTo = m_tlSelFrom;
  }
  
  if (tlFrom == tlTo)
  {
    const int w = 15;
    int cx, cy;
    if (LocToLP(pDC, tlFrom.nLine, tlFrom.nChar, cx, cy) == SLGotcha)
      rgnResult.SetRectRgn(cx - w, cy, cx + w, cy + m_nCHNormal);
  }
  else
  {
    CRgn rgnRect;
    rgnRect.CreateRectRgn(0,0,0,0);
    CTextBuffer* pBuf = GetTextBuffer();
    for (int nLine = max(m_nFirstVisibleLine, tlFrom.nLine); 
	 nLine <= tlTo.nLine; 
	 nLine++)
    {
      int cx1, cx2, cy;
      int nCharFrom = 0;
      if (nLine == tlFrom.nLine)
	nCharFrom = tlFrom.nChar;
      SLoc sl = LocToLP(pDC,nLine,nCharFrom,cx1,cy);
      if (sl == SLAbove)
	continue;
      if (sl == SLBelow)
	break;
      int nCharTo = tlTo.nChar;
      if (nLine != tlTo.nLine)
	nCharTo = pBuf->GetLine(nLine).GetLength() + 1;
      LocToLP(pDC,nLine,nCharTo,cx2,cy);
      cx1 = max(cx1, m_nLeftMargin);
      cx2 = max(cx2, cx1);
      rgnRect.SetRectRgn(cx1,cy,cx2,cy + m_nCHNormal);
      rgnResult.CombineRgn(&rgnResult, &rgnRect, RGN_OR);
    }
  }
}



// CTextView::GetSelection
// -- return current selection in <tlFrom> and <tlTo>
//    reorder locations to make <tlFrom> smaller than <tlTo>

void 
CTextView::GetSelection(CTextLoc &tlFrom, CTextLoc &tlTo)
{
  if (m_tlSelFrom <= m_tlSelTo)
  {
    tlFrom = m_tlSelFrom;
    tlTo = m_tlSelTo;
  }
  else
  {
    tlTo = m_tlSelFrom;
    tlFrom = m_tlSelTo;
  }
}


// CTextView::SetSelection
// -- changes current selection to range <tlFrom> to <tlTo>
//    updates screen if selection is shown

void 
CTextView::SetSelection(CTextLoc &tlFrom, CTextLoc &tlTo)
{
  // check against end of file (why?)
  CTextLoc locEnd;
  locEnd.nLine = GetTextBuffer()->GetDocLines() - 1;
  locEnd.nChar = GetTextBuffer()->GetLine(locEnd.nLine).GetLength();
  if (locEnd < tlTo)
    tlTo = locEnd;
  if (locEnd < tlFrom)
    tlFrom= locEnd;
  // perform selection
  if (tlFrom==m_tlSelFrom && tlTo==m_tlSelTo)
    return;
  if (m_nSelHideCount > 0) 
  {
    m_tlSelFrom = tlFrom;
    m_tlSelTo = tlTo;
  }
  else // must redraw
  {
    CClientDC cdc(this);
    cdc.SetMapMode(MM_ANISOTROPIC);
    cdc.SetWindowExt(1440,1440);
    cdc.SetViewportExt(m_nLogPixSX,m_nLogPixSY);
    CRgn regBefore;
    SelectionRgn(&cdc, regBefore);
    m_tlSelFrom = tlFrom;
    m_tlSelTo = tlTo;
    CRgn regAfter;
    SelectionRgn(&cdc, regAfter);
    regAfter.CombineRgn(&regBefore, &regAfter, RGN_XOR);
    cdc.InvertRgn(&regAfter);
  }
}



// CTextView::HideSelection
// -- increment counter preventing selection display
//    selection is show when this counter is smaller or equal to zero

void 
CTextView::HideSelection()
{
  if (m_nSelHideCount==0 && m_pTextBuffer)
  {
    UpdateWindow();
    CClientDC cdc(this);
    cdc.SetMapMode(MM_ANISOTROPIC);
    cdc.SetWindowExt(1440,1440);
    cdc.SetViewportExt(m_nLogPixSX,m_nLogPixSY);
    CRgn rgnSel;
    SelectionRgn(&cdc, rgnSel);
    cdc.InvertRgn(&rgnSel);
  }
  m_nSelHideCount += 1;
}



// CTextView::ShowSelection
// -- decrement counter preventing selection display
//    selection is show when this counter is smaller or equal to zero

void 
CTextView::ShowSelection()
{
  if (m_nSelHideCount==1  && m_pTextBuffer)
  {
    UpdateWindow();
    CClientDC cdc(this);
    cdc.SetMapMode(MM_ANISOTROPIC);
    cdc.SetWindowExt(1440,1440);
    cdc.SetViewportExt(m_nLogPixSX,m_nLogPixSY);
    CRgn rgnSel;
    SelectionRgn(&cdc, rgnSel);
    cdc.InvertRgn(&rgnSel);
  }
  m_nSelHideCount -= 1;
}



// CTextView::Replace
// -- replace text between locations <tlFrom> and <tlTo> by string <str>
//    this virtual function is called by user actions
//    selection must be hidden

void 
CTextView::Replace(CTextLoc tlFrom, CTextLoc tlTo, const CString &str)
{
  GetTextBuffer()->Replace(tlFrom, tlTo, str);
}




/////////////////////////////////////////////////////////////////////////////
// CTextView drawing



// CTextView::OnEraseBkgnd
// -- do not erase background so early in order to avoid flicker
//    linked tomessage WM_ERASEBKGND

BOOL 
CTextView::OnEraseBkgnd(CDC* pDC) 
{
  return FALSE;
}



// CTextView::OnDraw
// -- redraw editor window
//    called my MFS when message WM_PAINT is received

void 
CTextView::OnDraw(CDC* pDC)
{
  CTextBuffer* pBuf = GetTextBuffer();
  // Prepare CDC
  pDC->SetMapMode(MM_ANISOTROPIC);
  pDC->SetWindowExt(1440,1440);
  pDC->SetViewportExt(m_nLogPixSX, m_nLogPixSY);
  pDC->SetTextAlign(TA_TOP|TA_LEFT|TA_NOUPDATECP);
  pDC->SetBkMode(TRANSPARENT);
  pDC->SetTextColor(theApp.m_colorNormal);
  // Create and select font
  CFont font;
  font.CreateFontIndirect(&m_lfNormal);
  CFont *oldfont = pDC->SelectObject(&font);
  // Check if area need to be cleared
  BOOL bClear;
  if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
    if (((CPaintDC*)pDC)->m_ps.fErase)
      bClear = TRUE;
  // Erase background and perform update
  DrawText(pDC, bClear);
  // Release font
  pDC->SelectObject(oldfont);
  // Display selection
  if (m_nSelHideCount <= 0) 
  {
    CRgn rgnSel;
    SelectionRgn(pDC, rgnSel);
    pDC->InvertRgn(&rgnSel);
  }
}



// CTextView::DrawText
// -- draw text in editor window
//    virtual function called by CTextView::OnDraw
//    overidden by CSNView and CConsole to implement syntax coloring

void 
CTextView::DrawText(CDC *pDC, BOOL bClear)
{
  // Initialize useful variables
  CTextBuffer* pBuf = GetTextBuffer();
  int nTabOrigin = m_nLeftMargin - m_nFirstVisiblePos * m_nCWNormal;
  int nTabPosition = 8 * m_nCWNormal;
  // Go drawing
  for(int nLine = m_nFirstVisibleLine; ;nLine++)
  {
    // Find location,
    int cx, cy;
    SLoc sl = LocToLP(pDC, nLine, -1, cx, cy);
    if (sl == SLAbove)
      continue;
    // Clear area if needed (as lately as possible)
    if (bClear)
    {
      CRect rectClip;
      pDC->GetClipBox(rectClip);
      pDC->FillSolidRect(rectClip, PALETTERGB(255,255,255));
      bClear = FALSE;
    }
    // Exit if below clip rectangle or past end of data
    if (sl == SLBelow) 
      break;
    if (nLine >= pBuf->GetDocLines())
      break;
    // Display current line
    int nChar = pBuf->PosToChar(nLine, m_nFirstVisiblePos);
    const CString& dsp = pBuf->GetLine(nLine).Mid(nChar,256);
    pDC->TabbedTextOut(m_nLeftMargin, cy, dsp, 1, &nTabPosition, nTabOrigin);
  }
}


/////////////////////////////////////////////////////////////////////////////
// CTextView word operations



// CTextView::CharacterClass
// -- return character class of <c>
//    this is used for computing word boundaries

int 
CTextView::CharacterClass(char c)
{
  int i;
  static int tb[256];
  static BOOL tbok;
  if (!tbok)
  {
    // ISO-LATIN1
    for (i=0; i<=' '; i++)   tb[i] = tb[i+128] = CLASS_CONTROL;
    tb[' '] = tb['\t'] = tb['\n'] = tb['\r'] = CLASS_BLANK;
    for (i='0'; i<='9'; i++) tb[i] = CLASS_WORD;
    for (i='A'; i<='Z'; i++) tb[i] = CLASS_WORD;
    for (i='a'; i<='z'; i++) tb[i] = CLASS_WORD;
    for (i=292; i<=255; i++) tb[i] = CLASS_WORD;
    tb[0xD7] = tb[0xF7] = CLASS_SPECIAL;
    // Special lisp characters
    tb['('] = tb[')'] = tb['\"'] = tb[';'] = CLASS_SEPARATOR;
    tb['|'] = tb['#'] = tb['^'] = tb['?'] = CLASS_SEPARATOR;
    // Common macro-characters
    tb['['] = tb[']'] = tb['{'] = tb['}'] = CLASS_SEPARATOR;
    tb['\''] = tb['~'] = tb['`'] = tb[','] = tb['!'] = CLASS_SEPARATOR;
    // Ready
    tb[0] = -1;
    tbok = TRUE;
  }
  i = c;
  i &= 0xFF;
  ASSERT(i>0 && i<256);
  return tb[i & 0xFF];
}



// CTextView::IsWordBoundary
// -- describe word boundaries at location <loc>

int 
CTextView::IsWordBoundary(CTextLoc loc)
{
  int ans = 0;
  int nBefore = CLASS_BLANK;
  int nAfter = CLASS_BLANK;
  if (loc.nLine < GetTextBuffer()->GetDocLines())
  {
    const CString& str = GetTextBuffer()->GetLine(loc.nLine);
    if (loc.nChar > 0) 
      nBefore = CharacterClass(str[loc.nChar - 1]);
    if (loc.nChar < str.GetLength())
      nAfter = CharacterClass(str[loc.nChar]);
  }
  if (nBefore == CLASS_SEPARATOR)
    ans |= WB_END;
  if (nAfter == CLASS_SEPARATOR)
    ans |= WB_BEGIN;
  if (nBefore != nAfter)
  {
    if (nBefore != CLASS_BLANK)
      ans |= WB_END;
    if (nAfter != CLASS_BLANK)
      ans |= WB_BEGIN;
  }
  if (ans==0)
    if (nBefore==nAfter && nBefore!=CLASS_BLANK)
      ans |= WB_INSIDE;
  return ans;
}


// CTextView::FindWordBoundary
// -- find next or previous word boundary
//    used by CTRL+LEFT and CTRL+RIGHT

void 
CTextView::FindWordBoundary(BOOL bForward, CTextLoc& loc)
{
  CTextBuffer *pBuf = GetTextBuffer();
  if (bForward)
  {
    while (pBuf->GetPostInc(loc))
      if (IsWordBoundary(loc) & WB_BEGIN) 
        return;
  }
  else
  {
    while (pBuf->GetPreDec(loc))
      if (IsWordBoundary(loc) & WB_END) 
        return;
  }
}



// CTextView::ExtendToWord
// -- find next or previous word or seperator boundary 
//    used for double click selection

void 
CTextView::ExtendToWord(BOOL bForward, CTextLoc& loc)
{
  CTextBuffer *pBuf = GetTextBuffer();
  if (bForward)
  {
    while (pBuf->GetPostInc(loc))
      if (IsWordBoundary(loc) & (WB_BEGIN|WB_END))
	return;
  }
  else
  {
    if (! (IsWordBoundary(loc) & (WB_BEGIN|WB_END)))
      while (pBuf->GetPreDec(loc))
        if (IsWordBoundary(loc) & (WB_BEGIN|WB_END))
	  return;
  }
}




/////////////////////////////////////////////////////////////////////////////
// CTextView miscellaneous message handlers



// CTextView::OnDestroy
// -- kill timers before destroying window
//    linked to message WM_DESTROY

void 
CTextView::OnDestroy() 
{
  CView::OnDestroy();
  if (m_iTimer)
  {
    KillTimer(m_iTimer);
    m_iTimer = 0;
  }
}


// CTextView::OnSize
// -- adjust scrollbars when window size change
//    linked to message WM_SIZE

void 
CTextView::OnSize(UINT nType, int cx, int cy) 
{
  CView::OnSize(nType, cx, cy);
  SetScrollBars();
  Scroll(0,0);
}



// CTextView::OnSetCursor
// -- set IBeam cursor

BOOL 
CTextView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
  if (s_hIBeamCursor && nHitTest==HTCLIENT)
  {
    SetCursor(s_hIBeamCursor);
    return TRUE;
  }
  else
  {
    return CView::OnSetCursor(pWnd, nHitTest, message);
  }
}



/////////////////////////////////////////////////////////////////////////////
// CTextView MFC update



// CTextView::OnActivateView
// -- show selection in active view only
//    called by MFC when active view changes

void 
CTextView::OnActivateView(BOOL bActivate, CView* pAView, CView* pDView) 
{
  CView::OnActivateView(bActivate, pAView, pDView);
  if (bActivate && !m_bActive) 
  {
    ShowSelection();
    m_bActive = TRUE;
  } 
  else if (!bActivate && m_bActive)
  {
    HideSelection();
    m_bActive = FALSE;
  }
}



// CTextView::OnInitialUpdate
// -- cache pointer to text buffer before first update
//    called by MFC before first update

void 
CTextView::OnInitialUpdate() 
{
  ASSERT(!m_pTextBuffer);
  ASSERT_KINDOF(CTextDoc, GetDocument());
  m_pTextBuffer = & ((CTextDoc*)GetDocument())->m_textBuffer;
  DoInitialUpdate();
}



// CTextView::DoInitialUpdate
// -- prepare variables used by text view (fonts, sizes, ...) 
//    before first update. Called by CTextView::OnInitialUpdate

void 
CTextView::DoInitialUpdate() 
{
  // Check text buffer
  ASSERT(m_pTextBuffer);
  // Inherited
  CView::OnInitialUpdate();
  // Fill LOGFONT structure
  memset(&m_lfNormal, 0, sizeof(LOGFONT));
  m_lfNormal.lfHeight = -20 * theApp.m_nFontSize;
  m_lfNormal.lfWeight = FW_NORMAL;
  m_lfNormal.lfCharSet = ANSI_CHARSET;
  m_lfNormal.lfPitchAndFamily = FIXED_PITCH;
  strcpy(m_lfNormal.lfFaceName, theApp.m_sFontName);

  // Set LOGTWIPS mapping mode
  CDisplayIC dc;
  dc.SetMapMode(MM_ANISOTROPIC);
  dc.SetWindowExt(1440,1440);
  m_nLogPixSX = dc.GetDeviceCaps(LOGPIXELSX);
  m_nLogPixSY = dc.GetDeviceCaps(LOGPIXELSY);
  dc.SetViewportExt(m_nLogPixSX,m_nLogPixSY);

  // Create font and get metrics
  CFont font;
  TEXTMETRIC tm;
  font.CreateFontIndirect(&m_lfNormal);
  CFont *oldfont = dc.SelectObject(&font);
  dc.GetCharWidth('0', '0', &m_nCWNormal);
  dc.GetTextMetrics(&tm);
  m_nCHNormal = tm.tmHeight /* + tm.tmExternalLeading */;
  dc.SelectObject(oldfont);
  // Update scrollbars
  SetScrollBars();
}



// CTextView::OnUpdate
// -- monitor redraws when underlying document is modified
//    called by MFC when UpdateAllViews is called.

void 
CTextView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
  SetScrollBars();
  if (pHint && pHint->IsKindOf(RUNTIME_CLASS(CTextUpdateHint)))
  {
    CTextUpdateHint *ph = (CTextUpdateHint*)pHint;
    if (ph->m_pBuffer == GetTextBuffer())
    {
      if (ph->m_bUpdateSelection)
      {
        ASSERT(m_nSelHideCount>0);
        GetTextBuffer()->AdjustLocation(m_tlSelFrom, ph);
	GetTextBuffer()->AdjustLocation(m_tlSelTo, ph);
      }
      if (ph->m_bLineHint)
        InvalidateLines(ph->m_nFromLine, ph->m_nToLine);
      else
	Invalidate();
    }
    return;
  }
  Invalidate();
}


/////////////////////////////////////////////////////////////////////////////
// CTextView mouse and keyboard 



// CTextView::OnVScroll
// -- vertical scrollbar action
//    linked to message WM_VSCROLL

void 
CTextView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  int pagelen;
  CRect rectClient;
  CTextBuffer* pBuf = GetTextBuffer();
  switch(nSBCode)
  {
  case SB_TOP:
    Scroll(-m_nFirstVisibleLine);
    break;
  case SB_BOTTOM:
    Scroll(pBuf->GetDocLines());
    break;
  case SB_LINEUP:
    Scroll(-1);
    break;
  case SB_LINEDOWN:
    Scroll( 1);
    break;
  case SB_PAGEUP:
  case SB_PAGEDOWN:
    GetClientRect(rectClient);
    pagelen = MulDiv(rectClient.Height(), 1440, m_nLogPixSY) / m_nCHNormal;
    if (nSBCode==SB_PAGEUP)
      Scroll(-pagelen);
    else
      Scroll( pagelen);
    break;
  case SB_THUMBTRACK:
  case SB_THUMBPOSITION:
    Scroll(nPos-m_nFirstVisibleLine);
    break;
  }
}



// CTextView::OnHScroll
// -- horizontal scrollbar action
//    linked to message WM_HSCROLL

void 
CTextView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  CTextBuffer* pBuf = GetTextBuffer();
  switch(nSBCode)
  {
  case SB_TOP:
    Scroll(0, -m_nFirstVisiblePos);
    break;
  case SB_BOTTOM:
    Scroll(0, pBuf->GetDocChars());
    break;
  case SB_LINEUP:
    Scroll(0, -1);
    break;
  case SB_LINEDOWN:
    Scroll(0,  1);
    break;
  case SB_PAGEUP:
    Scroll(0, -8);
    break;
  case SB_PAGEDOWN:
    Scroll(0,  8);
    break;
  case SB_THUMBTRACK:
  case SB_THUMBPOSITION:
    Scroll(0, nPos - m_nFirstVisiblePos);
    break;
  }
}



// CTextView::OnLButtonDown
// -- starts selecting characters
//    linked to message WM_LBUTTONDOWN

void 
CTextView::OnLButtonDown(UINT nFlags, CPoint point) 
{
  CRect rect;
  GetClientRect(rect);
  m_pntMouse = point;
  if ( rect.PtInRect(point) ) 
  {
    if (::GetKeyState(VK_SHIFT) < 0)  // shift click
    {
      GetSelection(m_tlMDFrom, m_tlMDTo);
    }
    else if (DPToLoc(point, m_tlMDTo))  // click in margin
    {
      m_tlMDFrom.nLine = m_tlMDTo.nLine;
      m_tlMDFrom.nChar = 0;
      if (m_tlMDTo.nLine < GetTextBuffer()->GetDocLines())
	m_tlMDTo.nLine += 1;
      else
	m_tlMDTo.nChar = GetTextBuffer()->GetLine(m_tlMDTo.nLine).GetLength();
      ScrollTo(m_tlMDFrom);
    }
    else // regular click
    {
      m_tlMDFrom = m_tlMDTo;
    }
    m_nUpDownPos = -1;
    m_bDragging = TRUE;
    m_bDblClick = FALSE;
    SetCapture();
    OnMouseMove(nFlags, point);
  }
}


// CTextView::OnLButtonDblClk
// -- starts selecting words
//    linked to message WM_LBUTTONDBLCLK

void 
CTextView::OnLButtonDblClk (UINT nFlags, CPoint point) 
{
  CRect rect;
  GetClientRect(rect);
  m_pntMouse = point;
  if ( rect.PtInRect(point) ) 
  {
    if (::GetKeyState(VK_SHIFT) < 0)  // shift dbl click
    {
      GetSelection(m_tlMDFrom, m_tlMDTo);
    }
    else // regular dbl click
    {
      DPToLoc(point, m_tlMDFrom);
      m_tlMDTo = m_tlMDFrom;
    }
    int ans = IsWordBoundary(m_tlMDFrom);
    if (ans & (WB_BEGIN|WB_INSIDE))
      ExtendToWord(TRUE, m_tlMDTo);
    if (ans & WB_INSIDE)
      ExtendToWord(FALSE, m_tlMDFrom);
    m_nUpDownPos = -1;
    m_bDragging = TRUE;
    m_bDblClick = TRUE;
    SetCapture();
    OnMouseMove(nFlags, point);
  }
}


// CTextView::OnMouseMove
// -- selects character or words
//    linked to message WM_MOUSEMOVE

void 
CTextView::OnMouseMove(UINT nFlags, CPoint point) 
{
  m_pntMouse = point;
  if (m_bDragging)
  {
    CRect rect;
    CTextLoc loc;
    CTextLoc locFrom = m_tlMDFrom;
    GetClientRect(rect);

    if (rect.PtInRect(point))
    {
      if (m_iTimer) 
      {
	KillTimer(m_iTimer);
	m_iTimer = 0;
      }
      DPToLoc(point, loc);
      if (m_bDblClick)
	ExtendToWord(locFrom<=loc, loc);
      if (loc <= m_tlMDFrom)
	locFrom = m_tlMDTo;
      m_nUpDownPos = -1;
      SetSelection(locFrom, loc);
    }
    else
    {
      if (!m_iTimer)
      {
	SetTimer(1,100,NULL);
	m_iTimer = 1;
      }
    }
  }
}



// CTextView::OnTimer
// -- scroll window when user moves mouse outside window
//    linked to message WM_TIMER

void 
CTextView::OnTimer(UINT nIDEvent) 
{
  if (nIDEvent == m_iTimer)
  {
    int nc, nl;
    CRect rect;
    CTextLoc loc;
    CTextLoc locFrom = m_tlMDFrom;
    GetClientRect(rect);
    nc = nl = 0;
    if (m_pntMouse.y < rect.top)
      nl = -2;
    else if (m_pntMouse.y >= rect.bottom)
      nl = +2;
    if (m_pntMouse.x < rect.left)
      nc = -1;
    else if (m_pntMouse.x >= rect.right)
      nc = +1;
    Scroll(nl, nc);
    DPToLoc(m_pntMouse, loc);
    if (m_bDblClick)
      ExtendToWord(locFrom<=loc, loc);
    if (loc <= m_tlMDFrom)
      locFrom = m_tlMDTo;
    SetSelection(locFrom, loc);
  }
  // Inherited
  CView::OnTimer(nIDEvent);
}



// CTextView::OnLButtonUp
// -- accept selection
//    linked to message WM_LBUTTONUP

void 
CTextView::OnLButtonUp(UINT nFlags, CPoint point) 
{
  m_pntMouse = point;
  if (m_iTimer) 
  {
    KillTimer(m_iTimer);
    m_iTimer = 0;
  }
  if (m_bDragging)
  {
    m_bDragging = FALSE;
    m_nUpDownPos = -1;
    ReleaseCapture();
  }
}



// CTextView::OnKeyDown
// -- handle special keys
//    linked to message WM_KEYDOWN

void 
CTextView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  CTextLoc loc = m_tlSelTo;
  CTextLoc locFrom = m_tlSelFrom;
  CTextBuffer *pBuf = GetTextBuffer();
  BOOL bControl = ::GetKeyState(VK_CONTROL) < 0;
  BOOL bShift = ::GetKeyState(VK_SHIFT) < 0;

  // Cancel memory of vertical position
  if (nChar!=VK_UP && nChar!=VK_DOWN)
    m_nUpDownPos = -1;

  // Handle keys
  switch(nChar)
  {
  case VK_PRIOR:
    {
      CRect rectClient;
      GetClientRect(rectClient);
      int plen = MulDiv(rectClient.Height(), 1440, m_nLogPixSY) / m_nCHNormal;
      if (m_nUpDownPos < 0)
	m_nUpDownPos = pBuf->CharToPos(loc.nLine, loc.nChar);
      loc.nLine = max(0, loc.nLine-plen);
      loc.nChar = pBuf->PosToChar(loc.nLine, m_nUpDownPos);
      Scroll(-plen);
      break;
    }
  case VK_NEXT:
    {
      CRect rectClient;
      GetClientRect(rectClient);
      int plen = MulDiv(rectClient.Height(), 1440, m_nLogPixSY) / m_nCHNormal;
      if (m_nUpDownPos < 0)
	m_nUpDownPos = pBuf->CharToPos(loc.nLine, loc.nChar);
      loc.nLine = min(pBuf->GetDocLines()-1, loc.nLine+plen);
      loc.nChar = pBuf->PosToChar(loc.nLine, m_nUpDownPos);
      Scroll(+plen);
      break;
    }
  case VK_HOME:
    if (m_tlSelFrom < loc) {
      loc = m_tlSelFrom;
      locFrom = m_tlSelTo;
    }
    if (bControl)
      loc.nLine = 0;
    loc.nChar = 0;
    break;
    
  case VK_END:
    if (loc < m_tlSelFrom) {
      loc = m_tlSelFrom;
      locFrom = m_tlSelTo;
    }
    if (bControl)
      loc.nLine = pBuf->GetDocLines() -1;
    loc.nChar = pBuf->GetLine(loc.nLine).GetLength();
    break;
    
  case VK_LEFT:
    if (!bShift && m_tlSelFrom<m_tlSelTo)
      loc = m_tlSelFrom;
    if (bControl)
      FindWordBoundary(FALSE, loc);
    else if (bShift || m_tlSelFrom==m_tlSelTo)
      pBuf->GetPreDec(loc);
    break;
    
  case VK_RIGHT:
    if (!bShift && m_tlSelTo<m_tlSelFrom)
      loc = m_tlSelFrom;
    if (bControl)
      FindWordBoundary(TRUE, loc);
    else if (bShift || m_tlSelFrom==m_tlSelTo)
      pBuf->GetPostInc(loc);
    break;
    
  case VK_UP:
    if (bControl) {
      Scroll(-1);
      return;
    }
    if (!bShift && m_tlSelFrom<m_tlSelTo)
      loc = m_tlSelFrom;
    if (m_nUpDownPos < 0)
      m_nUpDownPos = pBuf->CharToPos(loc.nLine, loc.nChar);
    if (loc.nLine > 0)
      loc.nLine -= 1;
    loc.nChar = pBuf->PosToChar(loc.nLine, m_nUpDownPos);
    break;
    
  case VK_DOWN:
    if (bControl) {
      Scroll(+1);
      return;
    }
    if (!bShift && m_tlSelTo<m_tlSelFrom)
      loc = m_tlSelFrom;
    if (m_nUpDownPos < 0)
      m_nUpDownPos = pBuf->CharToPos(loc.nLine, loc.nChar);
    if (loc.nLine < pBuf->GetDocLines() - 1)
      loc.nLine += 1;
    loc.nChar = pBuf->PosToChar(loc.nLine, m_nUpDownPos);
    break;

  case VK_INSERT:
    m_bOverlayMode = !m_bOverlayMode;
    return;

  case VK_DELETE:
    {
      CString str;
      HideSelection();
      if (m_tlSelFrom == m_tlSelTo)
        if (bControl)
          FindWordBoundary(TRUE, m_tlSelTo);
        else
          GetTextBuffer()->GetPostInc(m_tlSelTo);
      Replace(m_tlSelFrom, m_tlSelTo, str);
      ShowSelection();
    }
    return;
  
  case VK_BACK:
    {
      CString str;
      HideSelection();
      if (m_tlSelFrom == m_tlSelTo)
        if (bControl)
          FindWordBoundary(FALSE, m_tlSelFrom);
        else
          GetTextBuffer()->GetPreDec(m_tlSelFrom);
      Replace(m_tlSelFrom, m_tlSelTo, str);
      ScrollTo(m_tlSelTo);
      ShowSelection();
    }
    return;
  
  case VK_RETURN:
    {
      CString str("\n");
      HideSelection();
      Replace(m_tlSelFrom, m_tlSelTo, str);
      ScrollTo(m_tlSelTo);
      ShowSelection();
    }
    return;

  case VK_TAB:
    {
      CString str("\t");
      HideSelection();
      Replace(m_tlSelFrom, m_tlSelTo, str);
      ScrollTo(m_tlSelTo);
      ShowSelection();
    }
    return;

  default:
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
    return;
  }

  // We arrive here when we have a cursor motion key
  if (!bShift)
    locFrom = loc;
  SetSelection(locFrom,loc);
  ScrollTo(loc);
}



// CTextView::InsertChart
// -- insert a character without checking

void 
CTextView::InsertChar(const CString &str)
{
  // Cancel memory of vertical position
  m_nUpDownPos = -1;
  // Insert character
  if (m_bOverlayMode)
    if (m_tlSelFrom == m_tlSelTo)
      GetTextBuffer()->GetPostInc(m_tlSelTo);
  Replace(m_tlSelFrom, m_tlSelTo, str);
  ScrollTo(m_tlSelTo);
}


// CTextView::OnChar
// -- insert ascii character
//    linked to message WM_CHAR

void 
CTextView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // Handle ANSI printable characters
  if (isprint(nChar) && !strchr("\t\n\r\b\a", nChar))
  {
    CString str(nChar);
    HideSelection();
    while (nRepCnt-- > 0)
      InsertChar(str);
    ShowSelection();
  }
}



/////////////////////////////////////////////////////////////////////////////
// CTextView UI 



// CTextView::OnUpdateLCIndicator
// -- updates line/column indicator on main frame's status bar
//    linked to pseudo-command ID_INDICATOR_LC

void 
CTextView::OnUpdateLCIndicator(CCmdUI *pCmdUI)
{
  CString str;
  int nLine = m_tlSelTo.nLine;
  int nPos = GetTextBuffer()->CharToPos(nLine, m_tlSelTo.nChar);
  str.Format("L%05d, C%03d", nLine+1, nPos+1);
  pCmdUI->SetText(str);
  pCmdUI->Enable(TRUE);
}



// CTextView::OnUpdateOvrIndicator
// -- updates inset/overlay indicator on main frame's status bar
//    linked to pseudo-command ID_INDICATOR_OVR

void 
CTextView::OnUpdateOvrIndicator(CCmdUI *pCmdUI)
{
  if (m_bOverlayMode)
    pCmdUI->SetText("OVRL");
  else
    pCmdUI->SetText("INSR");
  pCmdUI->Enable(TRUE);
}


// CTextView::OnUpdateNeedSel
// -- enable command when selection is non empty
//    linked to ID_EDIT_CUT, ID_EDIT_COPY

void 
CTextView::OnUpdateNeedSel(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(m_tlSelFrom!=m_tlSelTo);
}



// CTextView::OnUpdateNeedText
// -- enable command when buffer is non empty
//    linked to ID_EDIT_FIND, ID_EDIT_SELECT_ALL

void 
CTextView::OnUpdateNeedText(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	BOOL ans = FALSE;
	if (GetTextBuffer()->GetDocLines() > 0)
	  ans = TRUE;
	if (GetTextBuffer()->GetLine(0).GetLength() > 0)
	  ans = TRUE;
	pCmdUI->Enable(ans);
}


// CTextView::OnUpdateNeedClip
// -- enable command when clipboard is non empty
//    linked to ID_EDIT_PASTE

void 
CTextView::OnUpdateNeedClip(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(CountClipboardFormats() && IsClipboardFormatAvailable(CF_TEXT));
}


// CTextView::OnUpdateNeedUndo
// -- enable command when undo information is present
//    linked to ID_EDIT_UNDO

void 
CTextView::OnUpdateNeedUndo(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(GetTextBuffer()->CanUndo());
}



/////////////////////////////////////////////////////////////////////////////
// CTextView commands 


// CTextView::OnEditSelectAll
// -- select all text
//    linked to ID_EDIT_SELECT_ALL

void 
CTextView::OnEditSelectAll() 
{
  CTextLoc tlFrom, tlTo;
  tlTo.nLine = GetTextBuffer()->GetDocLines() - 1;
  tlTo.nChar = GetTextBuffer()->GetLine(tlTo.nLine).GetLength();
  SetSelection(tlFrom, tlTo);
}



// CTextView::OnEditCopy
// -- copy selection into clipboard
//    linked to ID_EDIT_COPY

void 
CTextView::OnEditCopy() 
{
  CString str;
  GetTextBuffer()->GetRange(str, m_tlSelFrom, m_tlSelTo, TRUE);
  if (OpenClipboard())
  {
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, str.GetLength()+1);
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      strcpy(pmem, str);
      GlobalUnlock(hmem);
      EmptyClipboard();
      SetClipboardData(CF_TEXT, hmem);
    }
    CloseClipboard();
  }
}



// CTextView::OnEditCut
// -- cuts selection into clipboard
//    linked to ID_EDIT_CUT

void 
CTextView::OnEditCut() 
{
  CString strEmpty;
  HideSelection();
  GetTextBuffer()->ClearUndoBuffer();
  GetTextBuffer()->Replace(m_tlSelFrom, m_tlSelTo, strEmpty);
  ShowSelection();
  const CString& str = GetTextBuffer()->GetUndoBuffer();
  if (OpenClipboard())
  {
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, str.GetLength()+1);
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      strcpy(pmem, str);
      GlobalUnlock(hmem);
      EmptyClipboard();
      SetClipboardData(CF_TEXT, hmem);
    }
    CloseClipboard();
  }
}



// CTextView::OnEditPaste
// -- insert clipboard contents at current location
//    linked to ID_EDIT_PASTE

void 
CTextView::OnEditPaste() 
{
  // Retrieve clipboard data
  CString str;
  if (OpenClipboard())
  {
    HGLOBAL hmem = GetClipboardData(CF_TEXT);
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      str = CString(pmem); // copy constructor
      GlobalUnlock(hmem);
    }
    CloseClipboard();
  }
  // Paste string
  if (str.GetLength() > 0)
  {
    HideSelection();
    Replace(m_tlSelFrom, m_tlSelTo, str);
    ScrollTo(m_tlSelTo);
    ShowSelection();
  }
}



// CTextView::OnEditUndo
// -- Undo last changes
//    linked to ID_EDIT_UNDO

void 
CTextView::OnEditUndo() 
{
  if (GetTextBuffer()->CanUndo())
  {
    HideSelection(); 
    CTextLoc tlFrom, tlTo;
    GetTextBuffer()->PerformUndo(tlFrom, tlTo);
    SetSelection(tlFrom,tlTo);
    ScrollTo(tlTo);
    ShowSelection();
  }
}




/////////////////////////////////////////////////////////////////////////////
// CTextView context menu



// CTextView::OnRButtonDown
// -- Start selecting if clicked outside selection

void 
CTextView::OnRButtonDown(UINT nFlags, CPoint point) 
{
  CRect rect;
  CTextLoc loc, locFrom, locTo;;
  GetClientRect(rect);
  // Start dragging if clicked ouside selection
  if (rect.PtInRect(point))
  {
    DPToLoc(point, loc);
    GetSelection(locFrom, locTo);
    if (loc<locFrom || locTo<loc)
      OnLButtonDown(nFlags, point);
  }
  // Default action	
  CView::OnRButtonDown(nFlags, point);
}



// CTextView::OnRButtonDown
// -- End selecting and perform default action (ocntext menu)

void 
CTextView::OnRButtonUp(UINT nFlags, CPoint point) 
{
  // Stop dragging
  OnLButtonUp(nFlags, point);
  // Default action
  CView::OnRButtonUp(nFlags, point);
}



// CTextView::OnContextMenu
// -- Handle context menu for console

void 
CTextView::OnContextMenu(CWnd*, CPoint point) 
{
  GetParentFrame()->ActivateFrame();
  UpdateWindow();
  CMenu menu;
  if (menu.LoadMenu(ID_POPUP_TEXTVIEW))
  {
    CMenu *pMenu = menu.GetSubMenu(0);
    ASSERT(pMenu);
    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
  	  		  point.x, point.y, AfxGetMainWnd() );
  }
}
  


/////////////////////////////////////////////////////////////////////////////
// CTextView find and replace



// CFindState records find and replace dialog information

class CFindState
{
public:
	CFindReplaceDialog* pFindReplaceDlg; // find or replace dialog
	BOOL bFindOnly;			     // Is pFindReplace the find or replace?
	CString strFind;		     // last find string
	CString strReplace;		     // last replace string
	BOOL bCase;			     // TRUE==case sensitive
	BOOL bWord;			     // TRUE==match whole word
};

static CFindState cfsEditState;
static CFindState *pEditState = &cfsEditState;



// CTextView::OnUpdateNeedFind
// -- update command item when repeating last find is posssible
//    linked to command ID_EDIT_REPEAT (F3) 

void 
CTextView::OnUpdateNeedFind(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	BOOL ans = FALSE;
	if (GetTextBuffer()->GetDocLines() > 0)
	  ans = TRUE;
	if (GetTextBuffer()->GetLine(0).GetLength() > 0)
	  ans = TRUE;
	if (pEditState->strFind.IsEmpty())
	  ans = FALSE;
	pCmdUI->Enable(ans);
}


// CTextView::AdjustDialogPosition
// -- makes sure that find/replace dialog does not hide selection
//    called by find/replace code

void 
CTextView::AdjustDialogPosition(CDialog* pDlg)
{
	ASSERT(pDlg != NULL);
        CDisplayIC dc;
        dc.SetMapMode(MM_ANISOTROPIC);
	dc.SetWindowExt(1440,1440);
        dc.SetViewportExt(m_nLogPixSX,m_nLogPixSY);
	int cx, cy;
	LocToLP(&dc, m_tlSelTo.nLine, m_tlSelTo.nChar, cx, cy);
	CPoint point(MulDiv(cx, m_nLogPixSX, 1440), MulDiv(cy, m_nLogPixSY, 1440));
	ClientToScreen(&point);
	CRect rectDlg;
	pDlg->GetWindowRect(&rectDlg);
	if (rectDlg.PtInRect(point))
	{
		if (point.y > rectDlg.Height())
			rectDlg.OffsetRect(0, point.y - rectDlg.bottom - 20);
		else
		{
			int nVertExt = GetSystemMetrics(SM_CYSCREEN);
			if (point.y + rectDlg.Height() < nVertExt)
				rectDlg.OffsetRect(0, 40 + point.y - rectDlg.top);
		}
		pDlg->MoveWindow(&rectDlg);
	}
}



// CTextView::OnEditFindReplace
// -- common code for commands Find and Replace

void 
CTextView::OnEditFindReplace(BOOL bFindOnly)
{
	ASSERT_VALID(this);
	m_bFirstSearch = TRUE;
	if (pEditState->pFindReplaceDlg != NULL)
	{
		if (pEditState->bFindOnly == bFindOnly)
		{
			pEditState->pFindReplaceDlg->SetActiveWindow();
			pEditState->pFindReplaceDlg->ShowWindow(SW_SHOW);
			return;
		}
		else
		{
			ASSERT(pEditState->bFindOnly != bFindOnly);
			pEditState->pFindReplaceDlg->SendMessage(WM_CLOSE);
			ASSERT(pEditState->pFindReplaceDlg == NULL);
			ASSERT_VALID(this);
		}
	}
	CString strFind;
	GetTextBuffer()->GetRange(strFind, m_tlSelFrom, m_tlSelTo);
	// if selection is empty or spans multiple lines use old find text
	if (strFind.IsEmpty() || (strFind.FindOneOf(_T("\n\r")) != -1))
		strFind = pEditState->strFind;
	CString strReplace = pEditState->strReplace;
	pEditState->pFindReplaceDlg = new CFindReplaceDialog;
	ASSERT(pEditState->pFindReplaceDlg != NULL);
	DWORD dwFlags = FR_HIDEUPDOWN | FR_DOWN;
	if (pEditState->bCase)
		dwFlags |= FR_MATCHCASE;
	if (pEditState->bWord)
		dwFlags |= FR_WHOLEWORD;
	if (!pEditState->pFindReplaceDlg->Create(bFindOnly, strFind,
						 strReplace, dwFlags, this))
	{
		pEditState->pFindReplaceDlg = NULL;
		ASSERT_VALID(this);
		return;
	}
	ASSERT(pEditState->pFindReplaceDlg != NULL);
	pEditState->bFindOnly = bFindOnly;
	pEditState->pFindReplaceDlg->SetActiveWindow();
	pEditState->pFindReplaceDlg->ShowWindow(SW_SHOW);
	ASSERT_VALID(this);
}



// CTextView::OnFindNext
// -- find next occurence of search string
//    called when button "Find Next" of the find/replace dialog box is depressed

void 
CTextView::OnFindNext(LPCTSTR lpszFind, BOOL bCase, BOOL bWord)
{
	ASSERT_VALID(this);
	pEditState->strFind = lpszFind;
	pEditState->bCase = bCase;
	pEditState->bWord = bWord;
	if (!FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord))
		TextNotFound(pEditState->strFind);
	else
		AdjustDialogPosition(pEditState->pFindReplaceDlg);
	ASSERT_VALID(this);
}



// CTextView::OnReplaceSel
// -- replace selected occurence of search string
//    called when button "Replace" of the find/replace dialog box is depressed

void 
CTextView::OnReplaceSel(LPCTSTR lpszFind, BOOL bCase, BOOL bWord, LPCTSTR lpszReplace)
{
	ASSERT_VALID(this);
	pEditState->strFind = lpszFind;
	pEditState->strReplace = lpszReplace;
	pEditState->bCase = bCase;
	pEditState->bWord = bWord;
	if (!SameAsSelected(pEditState->strFind, pEditState->bCase))
	{
		if (!FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord))
			TextNotFound(pEditState->strFind);
		else
			AdjustDialogPosition(pEditState->pFindReplaceDlg);
		return;
	}
	HideSelection();
	Replace(m_tlSelFrom, m_tlSelTo, pEditState->strReplace);
	ShowSelection();
	if (!FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord))
		TextNotFound(pEditState->strFind);
	else
		AdjustDialogPosition(pEditState->pFindReplaceDlg);
	ASSERT_VALID(this);
}



// CTextView::OnReplaceAll
// -- replace all occurencec of search string
//    called when button "Replace All" of the find/replace dialog box is depressed

void 
CTextView::OnReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL bCase, BOOL bWord)
{
	ASSERT_VALID(this);
	pEditState->strFind = lpszFind;
	pEditState->strReplace = lpszReplace;
	pEditState->bCase = bCase;
	pEditState->bWord = bWord;
	CWaitCursor wait;
	// no selection or different than what looking for
	if (!SameAsSelected(pEditState->strFind, pEditState->bCase))
	{
	  if (!FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord))
	  {
	    TextNotFound(pEditState->strFind);
	    return;
	  }
	}
	HideSelection();
	do 
	{
	  Replace(m_tlSelFrom, m_tlSelTo, pEditState->strReplace);
	} 
	while (FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord));
	TextNotFound(pEditState->strFind);
	ShowSelection();
	ASSERT_VALID(this);
}


// CTextView::OnFindReplaceCmd
// -- process actions on buttons of the find/replace dialog box
//    called via a custom message registered by the common dialog box

LRESULT 
CTextView::OnFindReplaceCmd(WPARAM, LPARAM lParam)
{
	ASSERT_VALID(this);
	CFindReplaceDialog* pDialog = CFindReplaceDialog::GetNotifier(lParam);
	ASSERT(pDialog != NULL);
	ASSERT(pDialog == pEditState->pFindReplaceDlg);
	if (pDialog->IsTerminating())
		pEditState->pFindReplaceDlg = NULL;
	else if (pDialog->FindNext())
	{
		OnFindNext(pDialog->GetFindString(), pDialog->MatchCase(), 
			   pDialog->MatchWholeWord());
	}
	else if (pDialog->ReplaceCurrent())
	{
		ASSERT(!pEditState->bFindOnly);
		OnReplaceSel(pDialog->GetFindString(), pDialog->MatchCase(), 
			     pDialog->MatchWholeWord(), pDialog->GetReplaceString() );
	}
	else if (pDialog->ReplaceAll())
	{
		ASSERT(!pEditState->bFindOnly);
		OnReplaceAll(pDialog->GetFindString(), pDialog->GetReplaceString(),
			     pDialog->MatchCase(), pDialog->MatchWholeWord());
	}
	ASSERT_VALID(this);
	return 0;
}



// CTextView::SameAsSelected
// -- checks that selected text is equal to given string
//    called before performing a replacement!

BOOL 
CTextView::SameAsSelected(LPCTSTR lpszCompare, BOOL bCase)
{
	CString strSelect;
	GetTextBuffer()->GetRange(strSelect, m_tlSelFrom, m_tlSelTo);
	return (bCase && lstrcmp(lpszCompare, strSelect) == 0) ||
		(!bCase && lstrcmpi(lpszCompare, strSelect) == 0);
}



// CTextView::FindText
// -- finds text starting at current selection in the editor window

BOOL 
CTextView::FindText(LPCTSTR lpszFind, BOOL bCase, BOOL bWord)
{
	ASSERT(lpszFind != NULL);
	CTextLoc locFrom, locTo;
	GetSelection(locFrom, locTo);
	if (m_bFirstSearch)
	{
		m_tlSearchPos = locFrom;
		m_bFirstSearch = FALSE;
		m_bWrapSearch = FALSE;
	}
	if (locFrom < locTo)
	  GetTextBuffer()->GetPostInc(locFrom);
again:
	locTo.nLine = GetTextBuffer()->GetDocLines() - 1;
	locTo.nChar = GetTextBuffer()->GetLine(locTo.nLine).GetLength();
	if (m_bWrapSearch)
	  locTo = m_tlSearchPos;
	// if we find the text return TRUE
	if (Find(lpszFind, bCase, bWord, locFrom, locTo))
	{
	  SetSelection(locFrom, locTo);
          // ScrollTo(locTo);
          ScrollTo(locFrom);
	  return TRUE;
	}
	// attempt to wrap around
	if (!m_bWrapSearch)
	{
	  locFrom.nLine = locFrom.nChar = 0;
	  m_bWrapSearch = TRUE;
	  goto again;
	}
	return FALSE;
}


// CTextView::TextNotFound
// -- beeps when text is not found
//    called from CTextView::FindText

void 
CTextView::TextNotFound(LPCTSTR lpszFind)
{
	ASSERT_VALID(this);
	m_bFirstSearch = TRUE;
	MessageBeep(MB_ICONHAND);
}


// CTextView::Find
// -- finds texts <lpszFind> in specified range <locFrom> to <locTo>
//    called by CTextView::FindText

BOOL 
CTextView::Find(LPCSTR lpszFind, BOOL bCase, BOOL bWord, 
   	        CTextLoc& locFrom, CTextLoc& locTo)
{
  CTextBuffer *pBuf = GetTextBuffer();
  int nFindLen = strlen(lpszFind);
  ASSERT(locTo.nLine < pBuf->GetDocLines());
  
  while (locFrom < locTo)
  {
    const CString &str = pBuf->GetLine(locFrom.nLine);
    const char *pos = (LPCSTR)str + locFrom.nChar;
    for (; *pos; pos++, locFrom.nChar++)
    {
      if ((!bWord) || (IsWordBoundary(locFrom) & WB_BEGIN))
      {
	  if ((*pos == *lpszFind) 
	      || (!bCase && toupper(*pos)==toupper(*lpszFind)) )
	  {
	    if ((bCase && !strncmp(pos,lpszFind, nFindLen)
	        || (!bCase && !_strnicmp(pos,lpszFind, nFindLen))) )
	    {
	      CTextLoc tl;
      	      tl = locFrom;
	      tl.nChar += nFindLen;
	      if ((!bWord) || (IsWordBoundary(tl) & WB_END))
	      {
		locTo = tl;
		return TRUE;
	      }
	    }
	  }
      }
    }
    locFrom.nLine += 1;
    locFrom.nChar = 0;
  }
  return FALSE;
}



// CTextView::OnEditFind
// -- pop up find dialog box
//    linked to command ON_EDIT_FIND

void 
CTextView::OnEditFind() 
{
	OnEditFindReplace(TRUE);
}


// CTextView::OnEditReplace
// -- pop up replace dialog box
//    linked to command ON_EDIT_REPLACE

void 
CTextView::OnEditReplace() 
{
	OnEditFindReplace(FALSE);
}



// CTextView::OnEditRepeat
// -- finds next occurence of search string
//    linked to command ON_EDIT_REPEAT (F3)

void CTextView::OnEditRepeat() 
{
	if (!FindText(pEditState->strFind, pEditState->bCase, pEditState->bWord))
		TextNotFound(pEditState->strFind);
}



/////////////////////////////////////////////////////////////////////////////
// CTextView printing



// CTextView::OnFilePrint
// -- call printing routine
//    restore directory that may be destroyed by print file dialog

void CTextView::OnFilePrint() 
{
  // Save directory
  char buffer[256];
  DWORD dwResult;
  theCom.SuspendTLisp();
  dwResult = GetCurrentDirectory(sizeof(buffer), buffer);
  // Call inherited
  CView::OnFilePrint();
  // Restore directory
  if (dwResult>0 && dwResult<sizeof(buffer))
    SetCurrentDirectory(buffer);
  theCom.ResumeTLisp();
}



// CTextView::OnPreparePrinting
// -- perform default print initialization
//    restore directory that may be destroyed by print file dialog

BOOL 
CTextView::OnPreparePrinting(CPrintInfo* pInfo)
{
  return DoPreparePrinting(pInfo);
}



// CTextView::OnPrepareDC
// -- sets logical TWIPS mode when preparing DC for printing

void 
CTextView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
  if (pInfo)
  {
    pDC->SetMapMode(MM_ANISOTROPIC);
    pDC->SetWindowExt(1440,1440);
    pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX), 
			pDC->GetDeviceCaps(LOGPIXELSY));
  }
  CView::OnPrepareDC(pDC, pInfo);
}



// CTextView::OnBeginPrinting
// -- obtain suitable font and metric for printing
//    compute number of pages

void 
CTextView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
    // save dc
    CDC& dc = *pDC;
    dc.SaveDC();
    OnPrepareDC(&dc, pInfo);
    // Compute font metric
    LOGFONT lf = m_lfNormal;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    CFont font;
    font.CreateFontIndirect(&lf);
    TEXTMETRIC tm;
    CFont *oldfont = dc.SelectObject(&font);
    dc.GetCharWidth('0', '0', &m_nCWPrint);
    dc.GetTextMetrics(&tm);
    dc.SelectObject(oldfont);
    m_nCHPrint = tm.tmHeight;
    // Compute printing rectangle
    CRect& rectDraw = pInfo->m_rectDraw;
    rectDraw.SetRect(0,0,dc.GetDeviceCaps(HORZRES),dc.GetDeviceCaps(VERTRES));
    dc.DPtoLP(rectDraw);
    rectDraw.DeflateRect(960, 960);
    // Compute number of pages
    m_nLinesPerPage = pInfo->m_rectDraw.Height() / m_nCHPrint - 3;
    int nDocLines = GetTextBuffer()->GetDocLines();
    if (m_nLinesPerPage < 1)
      pInfo->SetMaxPage(0);
    else
      pInfo->SetMaxPage((nDocLines+m_nLinesPerPage-1) / m_nLinesPerPage);
    // restore dc
    dc.RestoreDC(-1);
    // inherited
    CView::OnBeginPrinting(pDC, pInfo);
}



// CTextView::OnPrint
// -- print a page (header and text)

void 
CTextView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
  if (m_nLinesPerPage < 1)
    return;
  CRect& rectDraw = pInfo->m_rectDraw;
  rectDraw.DeflateRect(960, 960);
  pDC->SetTextAlign(TA_TOP|TA_LEFT|TA_NOUPDATECP);
  {
    // Print header
    LOGFONT lf = m_lfNormal;
    lf.lfHeight = (lf.lfHeight * 5) / 4;
    lf.lfWeight = FW_BOLD;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    CFont font;
    font.CreateFontIndirect(&lf);
    CFont *oldfont = pDC->SelectObject(&font);
    CString str = ((CTextDoc*)GetDocument())->GetDecentPathName(48);
    pDC->DrawText(str, rectDraw, DT_LEFT);
    str.Format("Page %d", pInfo->m_nCurPage);
    pDC->DrawText(str, rectDraw, DT_RIGHT);
    pDC->SelectObject(oldfont);
  }
  {
    // Print text
    int nLine = (pInfo->m_nCurPage - 1) * m_nLinesPerPage;
    int nToLine = min(nLine + m_nLinesPerPage, GetTextBuffer()->GetDocLines());
    int nPosY = rectDraw.top + 3 * m_nCHPrint;
    int nTabPos = 8 * m_nCWPrint;
    LOGFONT lf = m_lfNormal;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    CFont font;
    font.CreateFontIndirect(&lf);
    CFont *oldfont = pDC->SelectObject(&font);
    while (nLine < nToLine)
    {
      const CString& str = GetTextBuffer()->GetLine(nLine);
      pDC->TabbedTextOut(rectDraw.left, nPosY, str, 1, &nTabPos, rectDraw.left);
      nLine += 1;
      nPosY += m_nCHPrint;
    }
    pDC->SelectObject(oldfont);
  }
}








