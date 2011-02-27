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
 
// Console.cpp : implementation file
//

#include "stdafx.h"
#include "WinTL3.h"
#include "TextDoc.h"
#include "TextView.h"
#include "ConsFrm.h"
#include "MainFrm.h"
#include "Console.h"
#include "TL3Com.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LFLAG_EOF   0x80
#define LMASK_INP   0x7F


/////////////////////////////////////////////////////////////////////////////
// CConsDoc

IMPLEMENT_DYNCREATE(CConsDoc, CTextDoc)

CConsDoc::CConsDoc()
{
  // register console into the application
  ASSERT(theApp.m_pConsole==0);
  theApp.m_pConsole = this;
  // initialize
  m_consView = 0;
  m_bHeld = FALSE;
  m_tlOutput.nLine = m_tlOutput.nChar = 0;
  m_bInStr = FALSE;
  m_bInEsc = FALSE;
  m_bMayBeCRLF = FALSE;

}

CConsDoc::~CConsDoc()
{
}

BEGIN_MESSAGE_MAP(CConsDoc, CTextDoc)
	//{{AFX_MSG_MAP(CConsDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSaveAs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CConsDoc diagnostics

#ifdef _DEBUG
void CConsDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void 
CConsDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CConsDoc operations



// CConsDoc::Serialize
// -- read or save console text

void 
CConsDoc::Serialize(CArchive& ar)
{
  // console cannot read text
  ASSERT(ar.IsStoring());
  CTextDoc::Serialize(ar);
}



// CConsDoc::Write
// -- insert program output into console window
//    called when there is data on the TL3 pipe

void 
CConsDoc::Write(const CString& ostr)
{
  CString str;

  // PREFORMAT BUFFER
  // -- insert newlines, handle special characters

  // get preformat information
  int nMaxPos = theApp.m_nConsRightMargin;
  int bWordWrap = theApp.m_bConsWordWrap;
  // allocate large enough buffer
  int nTmp = ostr.GetLength();
  nTmp = nTmp + nTmp*8/nMaxPos + 2;
  // copy string into buffer
  int nCurPos = m_textBuffer.CharToPos(m_tlOutput.nLine, m_tlOutput.nChar);
  int nBreakPos = 0;
  LPSTR d = str.GetBuffer(nTmp);
  LPSTR lpstrBreak = NULL;
  for (LPCSTR s = ostr; *s; s++)
  {
    int c = (unsigned char)*s;
    // handle special characters
    switch (c)
    {
    case '\a':
      MessageBeep(MB_ICONINFORMATION);
      break;

    case '\n':
      if (m_bMayBeCRLF)
	break;
      // continue with CR
    case '\r':
      nCurPos = 0;
      *d++ = c;
      m_bInStr = FALSE;
      lpstrBreak = NULL;
      nBreakPos = 0;
      break;

    case '\t':
      nCurPos |= 7;
      // continue with space
    case ' ':
      nCurPos += 1;
      *d++ = c;
      if (!m_bInStr)
      {
	lpstrBreak = d;
	nBreakPos = nCurPos;
      }
      break;

    case '\"':
      if (!m_bInStr || !m_bInEsc)
	m_bInStr = !m_bInStr;
      if (!m_bInStr)
      {
	lpstrBreak = d;
	nBreakPos = nCurPos;
      }
      // continue with default:
    default:
      if (isprint(c))
      {
        nCurPos += 1;
      	*d++ = c;
      }
      break;
    }
    m_bInEsc = (m_bInStr && !m_bInEsc && c=='\\');
    m_bMayBeCRLF = (c=='\r');
    // check right margin
    if (nCurPos >= nMaxPos)
    {
      LPSTR b = d;
      if (bWordWrap && lpstrBreak && nBreakPos > nMaxPos / 2)
	b = lpstrBreak;
      char cInsert = '\n';
      nCurPos = 0;
      while (b <= d)
      {
	char cTmp = *b;
	*b++ = cInsert;
	nCurPos += 1;
	cInsert = cTmp;
      }
      d++;
    }
  }
  // release string buffer
  *d = 0;
  str.ReleaseBuffer();

  // PERFORM INSERTION IN CONSOLE
  // -- insert, adjust output point, update screen, scroll
  
  // hide selection
  if (m_consView)
    m_consView->HideSelection();
  // update 
  CTextUpdateHint hint(&m_textBuffer);
  m_textBuffer.ReplaceHint(m_tlOutput, m_tlOutput, str, FALSE, hint);
  m_textBuffer.AdjustLocation(m_tlOutput, &hint);
  UpdateAllViews(NULL, 0, &hint);
  // reset selection and scroll
  if (m_consView) 
  {
    CTextLoc locFrom, locTo;
    m_consView->GetSelection(locFrom, locTo);
    // perform optimized vertical scroll
    if (m_tlOutput <= locFrom)
	if (hint.m_bUpdateSelection)
	    m_consView->Scroll(hint.m_nLinesAdded);	
    // if not held scroll to bottommost of output and selection
    if (!m_bHeld)
    {
      CTextLoc loc = locFrom;
      if (locFrom.nLine <= m_tlOutput.nLine)
      {
	// avoid lateral scroll (until getline is called)
        loc.nLine = m_tlOutput.nLine;
	loc.nChar = 0;
      }
      m_consView->ScrollTo(loc);
    }
    m_consView->ShowSelection();
  }
}



// CConsDoc::Append
// -- insert new user input into the console window
//    called when other window send text to evaluate

void 
CConsDoc::Append(const CString& str)
{
  CTextLoc loc;
  loc.nLine = m_textBuffer.GetDocLines() - 1;
  loc.nChar = m_textBuffer.GetLine(loc.nLine).GetLength();
  m_textBuffer.Replace(loc,loc,"\n");
  loc.nLine = m_textBuffer.GetDocLines() - 1;
  loc.nChar = m_textBuffer.GetLine(loc.nLine).GetLength();
  m_textBuffer.Replace(loc,loc,str);
  loc.nLine = m_textBuffer.GetDocLines() - 1;
  loc.nChar = m_textBuffer.GetLine(loc.nLine).GetLength();
  if (loc.nChar > 0)
    m_textBuffer.Replace(loc,loc,"\n");
  loc.nLine = m_textBuffer.GetDocLines() - 1;
  loc.nChar = m_textBuffer.GetLine(loc.nLine).GetLength();
  if (m_consView)
      m_consView->SetSelection(loc,loc);
}



// CConsDoc::ReadLine
// -- reads a line of user input into str.
//    returns FALSE is no user input is ready for this
//    called for sending input to TL3 thread

BOOL 
CConsDoc::ReadLine(CString& str, BOOL &bEof)
{
  if (m_tlOutput.nLine >= m_textBuffer.GetDocLines() - 1)
    return FALSE;
  if (m_consView) {
    CTextLoc tlFrom, tlTo;
    m_consView->GetSelection(tlFrom, tlTo);
    if (tlFrom.nLine<=m_tlOutput.nLine && m_tlOutput.nLine<=tlTo.nLine)
      return FALSE;
  }
  bEof = FALSE;
  if (m_textBuffer.GetFlags(m_tlOutput.nLine) & LFLAG_EOF)
    bEof = TRUE;
  CTextLoc loc = m_tlOutput;
  loc.nLine += 1;
  loc.nChar = 0;
  m_textBuffer.GetRange(str, m_tlOutput, loc, TRUE);
  CTextUpdateHint hint(&m_textBuffer);
  m_textBuffer.ReplaceHint(m_tlOutput, loc, str, FALSE, hint);
  BYTE &flags = m_textBuffer.GetFlags(m_tlOutput.nLine);
  if (m_tlOutput.nChar+1 < LMASK_INP)
    flags |= (m_tlOutput.nChar + 1);
  if (bEof)
    flags |= LFLAG_EOF;
  hint.m_bUpdateSelection = FALSE;
  UpdateAllViews(NULL, 0, &hint);
  m_tlOutput = loc;
  return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// CConsDoc commands



// CConsDoc::SetTitle
// -- override the standard title routine 
//    title would be "Console1" instead of "Console"

void 
CConsDoc::SetTitle(LPCTSTR lpszTitle) 
{
  CString str;
  if (! GetDocTemplate()->GetDocString(str, CDocTemplate::docName))
    str = "Console";
  CDocument::SetTitle(str);
}



// CConsDoc::SetModifiedFlag
// -- override the standard modified flag
//    console never counts as a modified document

void 
CConsDoc::SetModifiedFlag(BOOL bModified)
{
    CDocument::SetModifiedFlag(FALSE);
}


// CConsDoc::OnFileSaveAs
// -- save session transcript
//    linked to commands ID_FILE_SAVE and ID_FILE_SAVE_AS

void 
CConsDoc::OnFileSaveAs() 
{
  CString strFilter;
  VERIFY(strFilter.LoadString(IDS_SAVE_CONSOLE_FILTER));
  CString strTitle;
  VERIFY(strTitle.LoadString(IDS_SAVE_CONSOLE_TITLE));
  DWORD dwFlags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  CFileDialog dlgFile(FALSE, "txt", "script.txt", dwFlags, strFilter);
  dlgFile.m_ofn.lpstrTitle = strTitle;
  // perform modal interaction
  theCom.SuspendTLisp();
  int nResult = dlgFile.DoModal(); 
  theCom.ResumeTLisp();
  // save document
  if (nResult == IDOK)
  {
    CString strName = dlgFile.GetPathName();
    if (!OnSaveDocument(strName))
    {
      try 
      {
	CFile::Remove(strName);
      } 
      catch (CException *e) 
      {
	TRACE0("Warning: failed to delete file after failed SaveAs.\n");
	e->Delete();
      }
    }
  }
}





/////////////////////////////////////////////////////////////////////////////
// CConsView

IMPLEMENT_DYNCREATE(CConsView, CTextView)

CConsView::CConsView()
{
}

CConsView::~CConsView()
{
}


BEGIN_MESSAGE_MAP(CConsView, CTextView)
	//{{AFX_MSG_MAP(CConsView)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateUIPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateUICut)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEW, OnUpdateDisable)
	ON_COMMAND(ID_CONSOLE_JUMP, OnConsoleJump)
	ON_COMMAND(ID_CONSOLE_BREAK, OnConsoleBreak)
	ON_COMMAND(ID_CONSOLE_EOF, OnConsoleEof)
	ON_UPDATE_COMMAND_UI(ID_CONSOLE_EOF, OnUpdateConsoleEof)
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_EVAL, OnEditEval)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, OnUpdateDisable)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EVAL, OnUpdateEditEval)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateOvrIndicator)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConsView creation

BOOL 
CConsView::PreCreateWindow(CREATESTRUCT& cs) 
{
  // add scrollbars
  cs.style |= (WS_HSCROLL|WS_VSCROLL);	
  return CView::PreCreateWindow(cs);
}

void 
CConsView::OnInitialUpdate() 
{
  // register unique console view into console document
  ASSERT(m_pDocument);
  ASSERT(GetConsoleDoc()->m_consView == 0);
  GetConsoleDoc()->m_consView = this;
  // inherited
  CTextView::OnInitialUpdate();
}


/////////////////////////////////////////////////////////////////////////////
// CConsView diagnostics

#ifdef _DEBUG
void 
CConsView::AssertValid() const
{
	CView::AssertValid();
}

void 
CConsView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CConsView operations



// CConsView::DrawText
// -- draw text in console window using a three color scheme
//    virtual function called from CTextView::OnDraw

void 
CConsView::DrawText(CDC *pDC, BOOL bClear)
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
    SLoc sl = LocToLP(pDC,nLine, -1, cx, cy);
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
    int nStrLen = dsp.GetLength();
    LPCSTR pchar = dsp; 
    // display the body of the line
    if (!theApp.m_bConsColorize)
    {
      pDC->TabbedTextOut(m_nLeftMargin, cy, pchar, nStrLen,
			 1, &nTabPosition, nTabOrigin);
    }
    else
    {
      // split line in two
      int nPosChange = nStrLen;
      COLORREF colorChange = theApp.m_colorConsEdit;
      CTextLoc tlOutput = GetConsoleDoc()->m_tlOutput;
      if (nLine > tlOutput.nLine)
	nPosChange = 0;
      else if (nLine == tlOutput.nLine)
	nPosChange = tlOutput.nChar - nChar;
      else 
      {
	int nTmp = GetTextBuffer()->GetFlags(nLine) & LMASK_INP;
        colorChange = theApp.m_colorConsInput;
	if (nTmp > 0)
	  nPosChange = nTmp - nChar - 1;
      }
      if (nPosChange < 0)
	nPosChange = 0;
      // display first part in black
      pDC->SetTextColor(theApp.m_colorConsNormal);
      if (nPosChange > 0)
	pDC->TabbedTextOut(m_nLeftMargin, cy, pchar, nPosChange,
			   1, &nTabPosition, nTabOrigin);
      // display second part in green (editable) or blue (old input)
      pDC->SetTextColor(colorChange);
      if (nPosChange < nStrLen)
      {
  	LocToLP(pDC, nLine, nPosChange+nChar, cx, cy);
	pDC->TabbedTextOut(cx, cy, pchar+nPosChange, nStrLen-nPosChange, 
			   1, &nTabPosition, nTabOrigin);
      }
    }
    // display eof of file marker when necessary
    if (pBuf->GetFlags(nLine) & LFLAG_EOF)
    {
	LOGFONT lfEof = m_lfNormal;
	lfEof.lfItalic = TRUE;
        CFont font;
	font.CreateFontIndirect(&lfEof);
	LocToLP(pDC, nLine, nStrLen, cx, cy);
	CFont *oldfont = pDC->SelectObject(&font);
	pDC->TextOut(cx, cy, " [EOF]", 6);
	pDC->SelectObject(oldfont);
    }
  }
}



// CConsView::Replace
// -- override CTextView::Replace to respect read-only sections of the console
//    called by interactive operations

void 
CConsView::Replace(CTextLoc tlFrom, CTextLoc tlTo, const CString &str)
{
  CTextLoc tlOutput = GetConsoleDoc()->m_tlOutput;
  if (tlFrom<tlOutput || tlTo<tlOutput)
  {
    MessageBeep(MB_ICONHAND);
    CTextLoc loc;
    CTextBuffer *pBuf = GetTextBuffer();
    loc.nLine = pBuf->GetDocLines() -1;
    loc.nChar = pBuf->GetLine(loc.nLine).GetLength();
    SetSelection(loc,loc);
    ScrollTo(loc);
  }
  else
  {
    CTextView::Replace(tlFrom, tlTo, str);
  }
}



/////////////////////////////////////////////////////////////////////////////
// CConsView message handlers



// CConsView::OnUpdateDisable
// -- inconditionally disables command item
//    linked to command ID_EDIT_REPLACE and ID_WINDOW_NEW

void 
CConsView::OnUpdateDisable(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(FALSE);
}


// CConsView::OnUpdateUICut
// -- enables command item when there is a modifiable selection
//    linked to command ID_EDIT_CUT

void 
CConsView::OnUpdateUICut(CCmdUI* pCmdUI) 
{
  CTextLoc tlFrom, tlTo;
  CTextLoc tlOutput = GetConsoleDoc()->m_tlOutput;
  GetSelection(tlFrom, tlTo);
  if (tlFrom<tlOutput || tlTo<=tlFrom)
    pCmdUI->Enable(FALSE);
  else
    pCmdUI->Enable(TRUE);
}


// CConsView::OnUpdateUIPaste
// -- enables command item when there selection is modifiable and clipboard full
//    linked to command ID_EDIT_PASTE

void 
CConsView::OnUpdateUIPaste(CCmdUI* pCmdUI) 
{
  CTextLoc tlFrom, tlTo;
  CTextLoc tlOutput = GetConsoleDoc()->m_tlOutput;
  GetSelection(tlFrom, tlTo);
  if (tlFrom<tlOutput || tlTo<tlFrom)
    pCmdUI->Enable(FALSE);
  else
    CTextView::OnUpdateNeedClip(pCmdUI);
}



// CConsView::OnUpdateOvrIndicator
// -- update INSERT/OVERLAY/READONLY indicatior on main frame's status bar
//    linked to pseudo-command ID_INDICATOR_OVR

void 
CConsView::OnUpdateOvrIndicator(CCmdUI *pCmdUI)
{
  CTextLoc tlFrom, tlTo;
  GetSelection(tlFrom, tlTo);
  if (tlFrom < GetConsoleDoc()->m_tlOutput)
  {
    pCmdUI->SetText("READ");
    pCmdUI->Enable(TRUE);
    return;
  }
  CTextView::OnUpdateOvrIndicator(pCmdUI);
}


// CConsView::OnUpdateConsoleEof
// -- Disable InsertEOF item when selected area is not modifiable 

void 
CConsView::OnUpdateConsoleEof(CCmdUI* pCmdUI) 
{
  CTextLoc tlFrom, tlTo;
  GetSelection(tlFrom, tlTo);
  if (tlFrom < GetConsoleDoc()->m_tlOutput)
    pCmdUI->Enable(FALSE);
  else
    pCmdUI->Enable(TRUE);
}


// CConsView::OnConsoleEof
// -- Insert End of File character into console
//    Linked to command ID_CONSOLE_EOF

void 
CConsView::OnConsoleEof() 
{
  CTextLoc tlFrom, tlTo;
  GetSelection(tlFrom,tlTo);
  CString eof("\n");
  HideSelection();
  InsertChar(eof);
  GetTextBuffer()->GetFlags(tlFrom.nLine) |= LFLAG_EOF;
  ShowSelection();
}



// CConsView::OnConsoleJump
// -- Scroll console view to make output point visible
//    linked to command ID_CONSOLE_JUMP

void 
CConsView::OnConsoleJump() 
{
    ScrollTo(GetConsoleDoc()->m_tlOutput);
}



// CConsView::OnConsoleBreak
// -- interrupts TLisp 
//    linked to command ID_CONSOLE_BREAK

void 
CConsView::OnConsoleBreak() 
{
  theCom.BreakTLisp();
  CTextLoc &tlOutput = GetConsoleDoc()->m_tlOutput;
  SetSelection(tlOutput, tlOutput);
}



// CConsView::OnKeyDown
// -- handle CTRL+SHIFT+Letter to produce 
//    characters ^+Letter. This is welcome
//    on European keyboards

void 
CConsView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  static UINT vk_a = VkKeyScan('a');
  static UINT vk_z = VkKeyScan('z');

  BOOL bControl = (GetKeyState(VK_CONTROL)<0 ? TRUE : FALSE);
  BOOL bShift = (GetKeyState(VK_SHIFT)<0 ? TRUE : FALSE);
  BOOL bAlt = (GetKeyState(VK_MENU)<0 ? TRUE : FALSE);

  if (nChar>=vk_a && nChar<=vk_z && bControl && bShift && !bAlt)
  {
    HideSelection();
    InsertChar( CString('^') + CString(nChar - vk_a + 'A') );
    ShowSelection();
  }
  else
  {
    // Return has implicit EOL
    if (nChar==VK_RETURN && theApp.m_bConsAutoEOL
        && !bShift && !bControl && !theApp.m_pConsole->m_bHeld )
      CTextView::OnKeyDown(VK_END, 1, 0);
    // Inherited
    CTextView::OnKeyDown(nChar, nRepCnt, nFlags);
  }
}




// CConsView::OnUpdateEditEval
// -- transfer selection at end of input buffer

void 
CConsView::OnUpdateEditEval(CCmdUI* pCmdUI) 
{
  // change toolbar state
  if (pCmdUI->m_pOther && 
      pCmdUI->m_pOther->IsKindOf(RUNTIME_CLASS(CToolBar)))
  {
    CTextLoc locFrom, locTo;
    GetSelection(locFrom, locTo);
    if (locFrom < locTo)
      pCmdUI->SetCheck(TRUE);
    else
      pCmdUI->SetCheck(FALSE);
  }
}


// CConsView::OnEditEval
// -- transfer selection at end of input buffer
//    suggest selection when none is available

void 
CConsView::OnEditEval() 
{
    int nLine, nChar;
    CTextLoc locFrom, locTo;
    GetSelection(locFrom, locTo);
    if (locFrom == locTo)
    {
      // find beginning of suggested selection
      for (nLine = locFrom.nLine; nLine>0; nLine--)
      {
        int nTmp = GetTextBuffer()->GetFlags(nLine) & LMASK_INP;
	if (nTmp==0)
	  continue;
        locFrom.nLine = nLine;
        locFrom.nChar = nTmp = nTmp - 1;
	if (locTo < locFrom)
	  continue;
	const CString& line = GetTextBuffer()->GetLine(nLine);
	if (nTmp == line.GetLength())
	  continue;
	if (GetTextBuffer()->GetFlags(nLine-1) & LMASK_INP == 0)
	  break;
	for (nChar=0; nChar<nTmp; nChar++)
	  if (line[nChar] != ' ')
	    break;
	if (nChar < nTmp)
	  break;
      }
      // find end of suggested selection
      if (nLine>0)
      {
	for (nLine=nLine+1; nLine<GetTextBuffer()->GetDocLines(); nLine++)
	{
	  int nTmp = GetTextBuffer()->GetFlags(nLine) & LMASK_INP;
	  if (nTmp==0)
	    break;
	  nTmp -= 1;
	  const CString& line = GetTextBuffer()->GetLine(nLine);
	  for (nChar=0; nChar<nTmp; nChar++)
	    if (line[nChar]!=' ')
	      break;
	  if (nChar<nTmp)
	    break;
	}
	if (nLine < GetTextBuffer()->GetDocLines()) 
	{
	  locTo.nLine = nLine;
	  locTo.nChar = 0;
	  SetSelection(locFrom, locTo);
	  ScrollTo(locFrom);
	  return;
	}
      }
      MessageBeep(MB_ICONINFORMATION);
    }
    else
    {
      // copy selection at end of input buffer
      CTextLoc loc;
      CString str;
      GetTextBuffer()->GetRange(str, locFrom, locTo, TRUE);
      loc.nLine = GetTextBuffer()->GetDocLines() - 1;
      loc.nChar = GetTextBuffer()->GetLine(loc.nLine).GetLength();
      HideSelection();
      Replace(loc,loc,str);
      loc.nLine = GetTextBuffer()->GetDocLines() - 1;
      loc.nChar = GetTextBuffer()->GetLine(loc.nLine).GetLength();
      SetSelection(loc,loc);
      ScrollTo(loc);
      ShowSelection();
    }
}


// CConsView::OnContextMenu
// -- Handle context menu for console

void 
CConsView::OnContextMenu(CWnd*, CPoint point) 
{
  GetParentFrame()->ActivateFrame();
  UpdateWindow();
  CMenu menu;
  if (menu.LoadMenu(ID_POPUP_CONSVIEW))
  {
    CMenu *pMenu = menu.GetSubMenu(0);
    ASSERT(pMenu);
    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
  	  		  point.x, point.y, AfxGetMainWnd() );
  }
}
  
