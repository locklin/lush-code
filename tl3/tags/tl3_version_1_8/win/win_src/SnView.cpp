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
 
// SnView.cpp : implementation of the CSnView class
//

#include "stdafx.h"
#include "WinTL3.h"
#include "TextDoc.h"
#include "TextView.h"
#include "SnView.h"
#include "Console.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSNView

IMPLEMENT_DYNCREATE(CSNView, CTextView)

BEGIN_MESSAGE_MAP(CSNView, CTextView)
	//{{AFX_MSG_MAP(CSNView)
	ON_COMMAND(ID_EDIT_BALANCE, OnEditBalance)
	ON_COMMAND(ID_EDIT_EVAL, OnEditEval)
	ON_COMMAND(ID_EDIT_INDENT, OnEditIndent)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_UPDATE_COMMAND_UI(ID_EDIT_EVAL, OnUpdateEditEval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CSNView construction/destruction

CSNView::CSNView()
: CTextView()
{
}

CSNView::~CSNView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CSNView diagnostics

#ifdef _DEBUG
void 
CSNView::AssertValid() const
{
	CTextView::AssertValid();
}

void 
CSNView::Dump(CDumpContext& dc) const
{
	CTextView::Dump(dc);
}
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// Word Segmentation



// CSNView::IsSeparator
// -- return TRUE is character is a separator for the TL3 reader

inline int 
CSNView::IsSeparator(char c)
{
  int cl = CharacterClass(c);
  return (cl != CLASS_WORD) && (cl != CLASS_SPECIAL);
}



// CSNView::IsWordBoundary
// -- return information about word boundaries on location <loc>.

int 
CSNView::IsWordBoundary(CTextLoc loc, SParseState st)
{
  const CString& str = GetTextBuffer()->GetLine(loc.nLine);

  if (st==IN_STRING || st==IN_STRING_ESCAPE || st==IN_COMMENT) 
  {
    if ((loc.nChar>=2) && (str[loc.nChar-2]=='\\'))
      return WB_BEGIN|WB_END;
    else
      return CTextView::IsWordBoundary(loc);
  }
  else
  {
    int ans = 0;
    int nBefore = CLASS_BLANK;
    int nAfter = CLASS_BLANK;
    if (loc.nChar > 0) 
      nBefore = CharacterClass(str[loc.nChar - 1]);
    if (loc.nChar < str.GetLength())
      nAfter = CharacterClass(str[loc.nChar]);
    if (loc.nChar > 1)
      if (str[loc.nChar-2]=='#' || str[loc.nChar-2]=='^')
	return WB_BEGIN|WB_END;
    if (loc.nChar > 0)
      if (str[loc.nChar-1]=='#' || str[loc.nChar-1]=='^')
	return WB_INSIDE;
    if (nBefore == CLASS_SEPARATOR)
      ans |= WB_END;
    if (nAfter == CLASS_SEPARATOR)
      ans |= WB_BEGIN;
    if (nBefore == CLASS_SPECIAL)
      nBefore = CLASS_WORD;
    if (nAfter == CLASS_SPECIAL)
      nAfter = CLASS_WORD;
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
}



// CSNView::IsWordBoundary
// -- return information about word boundaries on location <loc>

int 
CSNView::IsWordBoundary(CTextLoc loc)
{
  return IsWordBoundary(loc, SyncState(loc));
}



/////////////////////////////////////////////////////////////////////////////
// Parsing



// CSNView::ForwardParse
// -- increment location <loc> while maintaining parser state in <st>
//    return character between old and new location

inline char 
CSNView::ForwardParse(CTextLoc& loc, SParseState &st)
{
  char c = GetTextBuffer()->GetPostInc(loc);
  if (c==0)
    return c;  
  switch(st) 
  {
    case IN_SEP:
    separator:
      switch (c)
      {
      case '\"':
	st = IN_STRING;
        return c;
      case '|':
	st = IN_WORD_ESCAPE;
        return c;
      case ';':
	st = IN_COMMENT;
        return c;
      case '#':
	if (GetTextBuffer()->GetAt(loc) == '?') 
          st = IN_HELP;
	else
	  st = IN_MCHAR;
        return c;
      case '^':
	st = IN_MCHAR;
        return c;
      }
      if (IsSeparator(c))
        st = IN_SEP;
      else 
        st = IN_WORD;
      return c;

    case IN_WORD:
      if (IsSeparator(c))
	goto separator;
      return c;

    case IN_WORD_ESCAPE:
      if (c=='|' || c=='\n')
	st = IN_SEP;
      return c;

    case IN_HELP:
    case IN_COMMENT:
      if (c=='\n')
	st = IN_SEP;
      return c;

    case IN_STRING:
      if (c=='\\')
	st = IN_STRING_ESCAPE;
      else if (c=='\"' || c=='\n')
	st = IN_SEP;
      return c;

    case IN_STRING_ESCAPE:
      st = IN_STRING;
      return c;

    default:
    case IN_MCHAR:
      st = IN_SEP;
      return c;
  }
}


// CSNView::BackwardParse
// -- decrement location <loc> while maintaining parser state in <st>
//    return character between new and old location

inline char 
CSNView::BackwardParse(CTextLoc& loc, SParseState &st)
{
  char c = GetTextBuffer()->GetPreDec(loc);
  if (loc.nLine==0 && loc.nChar==0)
    return 0;
  char newc = '\n';
  if (loc.nChar>0)
    newc = GetTextBuffer()->GetLine(loc.nLine)[loc.nChar-1];
  if (c=='\n' && newc!='\n')
  {
  synchronize:
    st = SyncState(loc);
    return newc;
  }
  switch(st) 
  {
    case IN_SEP:
    case IN_MCHAR:
      if (c=='\"')
	st = IN_STRING;
      else if (c=='|')
	st = IN_WORD_ESCAPE;
      else if (!IsSeparator(newc))
	st = IN_WORD;
      else
	st = IN_SEP;
      return newc;

    case IN_COMMENT:
      if (c==';' && newc!=';')
	goto synchronize;
      return newc;
 
    case IN_HELP:
      if (c=='#')
	goto synchronize;
      return newc;
 
    case IN_STRING:
    case IN_STRING_ESCAPE:
      if (c=='\"')
	goto synchronize;
      st = IN_STRING;
      return newc;

    case IN_WORD:  
      // not perfect but good enough
      if (IsSeparator(newc))
	st = IN_SEP;
      return newc;

    case IN_WORD_ESCAPE:
      if (c=='|')
	goto synchronize;
      return newc;

    default:
      goto synchronize;
  }
}



// CSNView::SyncState
// -- compute parser state at location <locNow> by
//    running the ForwardParse on the <nContext> lines
//    above location.

SParseState 
CSNView::SyncState(const CTextLoc& locNow, int nContext)
{
  CTextLoc loc;
  SParseState st = IN_SEP;
  loc.nLine = max(0, locNow.nLine - nContext);
  loc.nChar = 0;
  while (loc<locNow && ForwardParse(loc, st))
  {
    if (st==IN_COMMENT || st==IN_HELP)
      loc.nChar = GetTextBuffer()->GetLine(loc.nLine).GetLength();
  }
  return st;
}



// CSNView::FindNextRParen
// -- move <loc> forward and stops after the next 
//    non-matched right parenthesis
//    scan at most <nContext> lines

BOOL 
CSNView::FindNextRParen(CTextLoc& loc, int nContext)
{
  char c;
  int level=0;
  SParseState st = SyncState(loc);

  while ((c = ForwardParse(loc, st)))
  {
    if (st==IN_SEP)
    {
      if (c=='(')
	level += 1;
      if (c==')') 
      {
	level -= 1;
	if (level < 0)
	  return TRUE;
      }
    }
    if (c=='\n')
      if (--nContext == 0)
	return FALSE;
  }
  return FALSE;
}



// CSNView::FindPrevLParen
// -- if cursor is located after a right parenthesis, 
//    position <loc> before matching left parenthesis.
//    if cursor is located elsewhere, position <loc> before 
//    first previous non matching left parenthesis.
//    Scan at most <nContext> parenthesis.

BOOL 
CSNView::FindPrevLParen(CTextLoc& loc, int nContext)
{
  char c;
  int level = 0;
  SParseState st = SyncState(loc);

  while ((c = BackwardParse(loc, st)))
  {
    if (st==IN_SEP)
    {
      if (c==')')
        level += 1;
      if (c=='(') 
      {
        level -= 1;
	if (level < 0)
	{
	  GetTextBuffer()->GetPreDec(loc);
	  return TRUE;
	}
      }
    }
    if (c=='\n')
      if (--nContext == 0)
	return FALSE;
  }
  return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// Eating lisp components



///////// keyword table

#define KW_NONE       0
#define KW_WORD	      1
#define KW_KEYWORD    2
#define KW_DEFINE     3

struct SKeyword {
  int   code;    // function definition keyword or regular keyword
  int   indent;  // number of lisp expressions with special indentation
  char *text;
};

static SKeyword keywords[] = 
{
  // maintain alphabetical order
  KW_KEYWORD, 3,  "==>",
  KW_KEYWORD, 2,  "all",
  KW_KEYWORD, 1,  "cond",
  KW_DEFINE,  3,  "de",
  KW_DEFINE,  3,  "defclass",
  KW_DEFINE,  4,  "defmethod",
  KW_DEFINE,  3,  "df",
  KW_DEFINE,  3,  "dm",
  KW_DEFINE,  2,  "dmc",
  KW_DEFINE,  3,  "dmd",
  KW_KEYWORD, 2,  "do-while",
  KW_KEYWORD, 2,  "each",
  KW_KEYWORD, 2,  "flambda",
  KW_KEYWORD, 2,  "for",
  KW_KEYWORD, 3,  "if",
  KW_KEYWORD, 2,  "lambda",
  KW_KEYWORD, 2,  "let",
  KW_KEYWORD, 2,  "let*",
  KW_KEYWORD, 2,  "letslot",
  KW_KEYWORD, 2,  "mapfor",
  KW_KEYWORD, 2,  "mlambda",
  KW_KEYWORD, 2,  "new",
  KW_KEYWORD, 2,  "prog1",
  KW_KEYWORD, 1,  "progn",
  KW_KEYWORD, 2,  "reading",
  KW_KEYWORD, 2,  "repeat",
  KW_KEYWORD, 2,  "selectq",
  KW_KEYWORD, 0,  "setq",
  KW_KEYWORD, 2,  "when",
  KW_KEYWORD, 2,  "while",
  KW_KEYWORD, 2,  "writing",
};



// CSNView::EatSpace
// -- increment <loc> until all spaces have been scanned.
//    skip line separator is <bSameLine> is FALSE.

void 
CSNView::EatSpace(CTextLoc& loc, BOOL bSameLine)
{
  for (;;)
  {
    char c = GetTextBuffer()->GetAt(loc);
    if ((c==0) || (bSameLine && c=='\n'))
      return;
    if (CharacterClass(c) != CLASS_BLANK)
      return;
    GetTextBuffer()->GetPostInc(loc);
  }
}



// CSNView::EatWord
// -- position <loc> after current word. 
//    return keyword information and indenting information

int 
CSNView::EatWord(CTextLoc& loc, int *pIndent)
{
  const CString &str = GetTextBuffer()->GetLine(loc.nLine);

  // Check for beginning of word
  if (IsWordBoundary(loc, IN_SEP) & WB_BEGIN)
  {
    // Find end of word;
    CTextLoc locFrom = loc;
    ASSERT(loc.nChar < str.GetLength());
    while (++loc.nChar < str.GetLength())
      if (IsWordBoundary(loc, IN_WORD) & WB_END)
	break;

    // Compare with keyword list
    SKeyword *sk = keywords;
    int nsk = sizeof(keywords) / sizeof(SKeyword);
    ASSERT(loc.nLine == locFrom.nLine);
    LPCSTR start = (LPCSTR)str + locFrom.nChar;
    int length = loc.nChar - locFrom.nChar;
    for (; nsk-->0; sk++)
      if (_strnicmp(start, sk->text, length)==0)
	if (sk->text[length]==0)
        {
          if (pIndent) *pIndent = sk->indent;
  	  return sk->code;
	}
    if (pIndent) *pIndent = 0;
    return KW_WORD;
  }
  if (pIndent) *pIndent = 0;
  return KW_NONE;
}


/////////////////////////////////////////////////////////////////////////////
// Flashing parenthesis



// CSNView::OnChar
// -- flash matching left parenthesis when useer types a right parenthesis
//    override CTextView::OnChar, linked to message WM_CHAR

void 
CSNView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // Perform regular insertion
  CTextView::OnChar(nChar, nRepCnt, nFlags);
  // Flash matching parenthesis
  if (nChar==')' && nRepCnt==1)
  {
    CTextLoc locFrom, loc;
    GetSelection(locFrom, loc);
    if (locFrom==loc)
      if (SyncState(loc) == IN_SEP)
	if (FindPrevLParen(loc, 60))
        {
          int cx, cy;
          UpdateWindow();
    	  CClientDC dc(this);
          dc.SetMapMode(MM_ANISOTROPIC);
          dc.SetWindowExt(1440,1440);
          dc.SetViewportExt(m_nLogPixSX, m_nLogPixSY);
          if (LocToLP(&dc, loc.nLine, loc.nChar, cx, cy) == SLGotcha)
	  {
  	    CRect rect(cx, cy, cx+m_nCWNormal, cy+m_nCHNormal);
	    if (!GetQueueStatus(QS_INPUT))
	    {
	      dc.InvertRect(rect);
              GdiFlush();
	      Sleep(60);
	      dc.InvertRect(rect);
              GdiFlush();
	    }
	  }
	}
  }
}




/////////////////////////////////////////////////////////////////////////////
// Balancing



// CSNView::OnEditBalance
// -- expand selection to encompass surrounding lisp expression
//    linked to command ID_EDIT_BALANCE

void 
CSNView::OnEditBalance() 
{
  CTextLoc locTo, locFrom;
  GetSelection(locTo, locFrom);
  // if nothing is selected, consider parenthesis right before the cursor
  const CString& str = GetTextBuffer()->GetLine(locFrom.nLine);
  if (locTo==locFrom && locTo.nChar>0 && str[locTo.nChar-1]==')')
    locTo.nChar -= 1;
  // find next parenthesis
  CWaitCursor wait;
  if (FindNextRParen(locTo))
  {
    locFrom = locTo;
    // find previous parenthesis
    if (! FindPrevLParen(locFrom)) 
    {
      locFrom = locTo;
      GetTextBuffer()->GetPreDec(locFrom);
      SetSelection(locFrom, locTo);
      ScrollTo(locTo);
    }
    else
    {
      SetSelection(locFrom, locTo);
      // ScrollTo(locTo);
      ScrollTo(locFrom);
      return;
    }
  } 
  MessageBeep(MB_ICONINFORMATION);
}



/////////////////////////////////////////////////////////////////////////////
// Indenting



// CSNView::GetLineIndent
// -- returns current indentation for line <nLine>

int 
CSNView::GetLineIndent(int nLine)
{
  int nPos = 0;
  int nChar = 0;
  const CString& str = GetTextBuffer()->GetLine(nLine);
  while (nChar<str.GetLength() && CharacterClass(str[nChar])==CLASS_BLANK)
  {
    if (str[nChar] == '\t')
      nPos |= 7;
    nPos += 1;
    nChar += 1;
  }
  return nPos;  
}



// CSNView::IndentLine
// -- indent line <nLine> by specified amount

void 
CSNView::IndentLine(int nLine, int nIndent)
{
  int nPos = 0;
  int nChar = 0;
  const CString& str = GetTextBuffer()->GetLine(nLine);
  // compute current indentation
  while (nChar<str.GetLength() && CharacterClass(str[nChar])==CLASS_BLANK)
  {
    if (str[nChar] == '\t')
      nPos |= 7;
    nPos += 1;
    nChar += 1;
  }
  // exit if already ok
  if (nPos == nIndent)
    return;
  // perform replacement
  CTextLoc locFrom, locTo;
  locFrom.nLine = locTo.nLine = nLine;
  locFrom.nChar = 0;
  locTo.nChar = nChar;
  CString tabs('\t', nIndent/8);
  CString blanks(' ', nIndent%8);
  Replace(locFrom, locTo, tabs+blanks);
}



// CSNView::ComputeLineIndent
// -- compute indentation for line <nLine>

int 
CSNView::ComputeLineIndent(int nLine)
{
  CTextLoc loc;
  loc.nLine = nLine;
  loc.nChar = 0;
  { 
    // test if line starts with three semicolons
    int nChar;
    const CString& str = GetTextBuffer()->GetLine(nLine);
    for (nChar = 0; nChar<str.GetLength(); nChar++)
      if (CharacterClass(str[nChar]) != CLASS_BLANK)
	break;
    if (nChar<str.GetLength() && str[nChar]==';')
      if (nChar+2<str.GetLength() && str[nChar+1]==';' && str[nChar+2]==';')
	return GetLineIndent(nLine);
  }
  { 
    // test if line is a multiline string
    SParseState st = SyncState(loc);
    if (st==IN_STRING || st==IN_STRING_ESCAPE)
      return GetLineIndent(nLine);
  }
  {
    // search previous left parenthesis
    int level = 0;
    SParseState st = SyncState(loc);
    while (level>=0)
    {
      char c = BackwardParse(loc, st);
      if (c==0)
	return 0;
      if (st==IN_SEP)
      {
	if (c==')')
	  level += 1;
	if (c=='(') 
	  level -= 1;
        if (c=='\n')
        {
  	  const CString& str = GetTextBuffer()->GetLine(loc.nLine);
	  if (GetLineIndent(loc.nLine)==0)
	    // no indentation, not a comment and not empty
	    if (str.GetLength()>0 && str[0]!=';')
	      return 0;
	}
      }
    }
  }
  int nExprCount = 0;
  int nIndent = GetTextBuffer()->CharToPos(loc.nLine, loc.nChar) - 1;
  int nIndentReset = nIndent;
  EatSpace(loc);
  if (IsSeparator(GetTextBuffer()->GetAt(loc)))
  {
    // parenthesis not followed by a symbol
    nIndent = GetTextBuffer()->CharToPos(loc.nLine, loc.nChar);
    nIndentReset = nIndent;
  }
  else
  {
    EatWord(loc, &nExprCount);
    if (nExprCount > 0)
    {
      // symbol is a recognized keyword
      nIndentReset = nIndent + 2;
      nIndent = nIndent + 4;
    }
    else
    {
      // symbol is an unrecognized word
      EatSpace(loc);
      nIndent = nIndent + 2;
      if (loc.nChar < GetTextBuffer()->GetLine(loc.nLine).GetLength())
        nIndent = GetTextBuffer()->CharToPos(loc.nLine, loc.nChar);
      nIndentReset = nIndent;
    }
  }
  nExprCount -= 1;
  // follow expressions
  char c;
  int level = 0;
  SParseState st = IN_SEP;
  while ((c = ForwardParse(loc, st)))
  {
    if (nExprCount==0)
      nIndent = nIndentReset;
    if (loc.nLine >= nLine)
      break;
    switch(st)
    {
    case IN_SEP:
      if (loc.nChar==0 && level==0)
      {
  	EatSpace(loc);
  	// comment lines do not reset indentation
	if (GetTextBuffer()->GetAt(loc) != ';') 
	  nIndent = GetLineIndent(loc.nLine);
      }
      if (c=='(')
	level += 1;
      if (c==')')
      {
	level -= 1;
	ASSERT(level >= 0);
	if (level==0)
	  nExprCount -= 1;
      }
      break;
    case IN_STRING:
    case IN_STRING_ESCAPE:
      while ((c = ForwardParse(loc, st)))
        if (st!=IN_STRING && st!=IN_STRING_ESCAPE)
          break;
      if (level==0)
	nExprCount -= 1;
      break;
    case IN_WORD:
      GetTextBuffer()->GetPreDec(loc);
      EatWord(loc);
      if (level==0)
	nExprCount -= 1;
      break;
    case IN_WORD_ESCAPE:
      while (ForwardParse(loc, st)) 
	if (st!=IN_WORD_ESCAPE)
	  break;
      if (level==0)
	nExprCount -= 1;
      break;
    case IN_COMMENT:
    case IN_HELP:
      loc.nChar = GetTextBuffer()->GetLine(loc.nLine).GetLength();
      break;
    }
  }
  return nIndent;
}



// CSNView::OnEditIndent
// -- indent lines spanned by selection
//    linked to command ON_EDIT_INDENT

void 
CSNView::OnEditIndent() 
{
  const int nWaitCursorTrigger = 10;
  CTextLoc locFrom, locTo;
  GetSelection(locFrom, locTo);
  int nNumLines = locTo.nLine - locFrom.nLine;
  if (nNumLines > nWaitCursorTrigger)
  {
    CWaitCursor cursor;
    HideSelection();
    theApp.EnableWindows(FALSE);
    for (int nLine=locFrom.nLine; nLine<=locTo.nLine; nLine++) 
    {
      CString str;
      str.Format("Indenting: %d%%", MulDiv(nLine-locFrom.nLine, 100, nNumLines));
      IndentLine(nLine, ComputeLineIndent(nLine));
      theApp.SetStatusMessage(str);
      // Check for events
      if (theApp.CheckMessage())
      {
	locFrom.nLine = nLine;
	locFrom.nChar = 0;
	SetSelection(locFrom, locFrom);
        MessageBeep(MB_ICONINFORMATION);
	break;
      }
    }
    theApp.SetStatusMessage();
    theApp.EnableWindows(TRUE);
  }
  else
  {
    HideSelection();
    for (int nLine=locFrom.nLine; nLine<=locTo.nLine; nLine++)
      IndentLine(nLine, ComputeLineIndent(nLine));
  }
  // Set selection on start of last line
  GetSelection(locFrom, locTo);
  const CString &str = GetTextBuffer()->GetLine(locTo.nLine);
  int nChar = 0;
  while (nChar<str.GetLength() 
         && CharacterClass(str[nChar])==CLASS_BLANK)
    nChar += 1;
  if (locTo.nChar <nChar)
    locTo.nChar = nChar;
  SetSelection(locTo, locTo);
  ScrollTo(locTo);
  ShowSelection();
}




// CSNView::OnKeyDown
// -- perform automatic indentation when user presses TAB or RETURN
//    override CTextView::OnKeyDown linked to WM_KEYDOWN

void 
CSNView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  switch(nChar)
  {
  case VK_RETURN:
      HideSelection();
      CTextView::OnKeyDown(nChar, nRepCnt, nFlags);
      if (theApp.m_bAutoIndent)
	OnEditIndent();
      ShowSelection();
      break;
  case VK_TAB:
      if (theApp.m_bAutoIndent || GetKeyState(VK_SHIFT)<0)
      {
	OnEditIndent();
      }
      else
      {
        CTextView::OnKeyDown(nChar, nRepCnt, nFlags);
      }
      break;
  default:
      CTextView::OnKeyDown(nChar, nRepCnt, nFlags);
      break;
  }
}





/////////////////////////////////////////////////////////////////////////////
// Evaluating



// CSNView::OnUpdateEditEval
// -- transfer selection at end of input buffer

void 
CSNView::OnUpdateEditEval(CCmdUI* pCmdUI) 
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


// CSNView::OnEditEval
// -- if selection is empty, selects a candidate lisp expression
//    if selection is not empty, send it to the console
//    linked to command ID_EDIT_EVAL

void 
CSNView::OnEditEval() 
{
  BOOL bOk = FALSE;
  CTextLoc locFrom, locTo;
  CTextLoc locFrom1, locTo1;
  GetSelection(locFrom, locTo);
 
  if (locFrom==locTo)
  {
    // Suggest selection
    CWaitCursor wait;
    const CString& str = GetTextBuffer()->GetLine(locFrom.nLine);
    if (locFrom==locTo && locFrom.nChar>0 && str[locFrom.nChar-1]==')')
      locFrom.nChar -= 1;
    locTo1 = locFrom1 = locTo = locFrom;

    while (FindNextRParen(locTo1))
    {
      locFrom1 = locTo1;
      if (! FindPrevLParen(locFrom1))
        break;
      if (locFrom <= locFrom1)
        break;
      if (locTo1 <= locTo)
        break;
      bOk = TRUE;
      locFrom = locFrom1;
      locTo = locTo1;
    }
    if (bOk)
    {
      SetSelection(locFrom, locTo);
      ScrollTo(locFrom);
      return;
    }
    MessageBeep(MB_ICONINFORMATION);
    return;
  }

  // Check for full selection and not modified
  CString str;
  if ((! GetDocument()->IsModified()) &&
      (! GetDocument()->GetPathName().IsEmpty()) &&
      locFrom.nLine==0 && locFrom.nChar==0 &&
      locTo.nLine==GetTextBuffer()->GetDocLines() - 1 &&
      locTo.nChar==GetTextBuffer()->GetLine(locTo.nLine).GetLength() )
  {
    // Build load command (using macro to save \\)
    str.Format("^L %s", GetDocument()->GetPathName());
    SetSelection(locFrom, locFrom);
  }
  else
  {
    // Get buffer contents
    GetTextBuffer()->GetRange(str, locFrom, locTo, TRUE);
  }
  ASSERT(theApp.m_pConsole);
  theApp.m_pConsole->Append(str);
  theApp.ViewConsole();
}





/////////////////////////////////////////////////////////////////////////////
// Coloring



// CSNView::OnUpdate
// -- change update hints to handle coloring in multiline
//    strings before calling inherited function CTextView::OnUpdate

void 
CSNView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{  
  if (pHint && pHint->IsKindOf(RUNTIME_CLASS(CTextUpdateHint)))
  {
    CTextUpdateHint *ph = (CTextUpdateHint*)pHint;
    if (ph->m_bLineHint && ph->m_nToLine>=0)
    {
      // handle color change on multi-line strings
      while (ph->m_nToLine + 1 < GetTextBuffer()->GetDocLines())
      {
        CTextLoc loc;
	loc.nLine = ph->m_nToLine;
	const CString& str = GetTextBuffer()->GetLine(ph->m_nToLine);
	loc.nChar = str.GetLength();
	SParseState st = SyncState(loc);
	if (st!=IN_STRING && st!=IN_STRING_ESCAPE)
	  if (loc.nChar>0 && str[loc.nChar-1]!='\\')
	    break;
        ph->m_nToLine += 1;
      }
    }
  }
  // inherited
  CTextView::OnUpdate(pSender, lHint, pHint);
}



// CSNView::DrawRange
// -- draws text between locations <locFrom> and <locTo>
//    with specified style

CSNView::SLoc 
CSNView::DrawRange(CDC *pDC, CTextLoc locFrom, CTextLoc locTo, 
 		   COLORREF color, BOOL bBold)
{
  int nTabPos = 8 * m_nCWNormal;
  int nTabOrg = m_nLeftMargin - m_nFirstVisiblePos * m_nCWNormal;
  for (int nLine=locFrom.nLine; nLine<=locTo.nLine; nLine++)
  {
    const CString& str = GetTextBuffer()->GetLine(nLine);
    int nFromChar = (nLine==locFrom.nLine ? locFrom.nChar : 0);
    int nFirstChar = GetTextBuffer()->PosToChar(nLine, m_nFirstVisiblePos);
    int nToChar = (nLine==locTo.nLine ? locTo.nChar : str.GetLength());
    int cx, cy;
    if (nFromChar < nFirstChar)
      nFromChar = nFirstChar;
    if (nFromChar < nToChar)
    {
      SLoc sl = LocToLP(pDC, nLine, nFromChar, cx, cy);
      if (sl!=SLGotcha)
	return sl;
      if (cx < m_nCWNormal*256) // not too far right
      {
	LPCSTR pchar = (LPCSTR)str + nFromChar;
	int len = min(256, nToChar - nFromChar);
	pDC->SetTextColor(color);
	pDC->TabbedTextOut(cx, cy, pchar, len, 1, &nTabPos, nTabOrg);
	if (bBold)
	{
	  int dx = 1440 / m_nLogPixSX;
	  pDC->TabbedTextOut(cx+dx, cy, pchar, len, 1, &nTabPos, nTabOrg+dx);
	}
      }
    }
  }
  return SLGotcha;
}


				  
// CSNView::DrawText
// -- draw text in console window using syntax coloring
//    virtual function called from CTextView::OnDraw

void 
CSNView::DrawText(CDC *pDC, BOOL bClear)
{
  // Test if Colorization is disabled
  if (! theApp.m_bColorize)
  {
    CTextView::DrawText(pDC, bClear);
    return;
  }

  // Find first line to draw
  SLoc sl;
  CTextLoc loc;
  loc.nChar = 0;
  loc.nLine = m_nFirstVisibleLine;
  for( ;loc.nLine<GetTextBuffer()->GetDocLines();loc.nLine++)
  {
    int cx, cy;
    sl = LocToLP(pDC, loc.nLine, -1, cx, cy);
    if (sl != SLAbove)
      break;
  }
  // Initialize drawing
  SParseState st;
  if (sl == SLGotcha)
    st = SyncState(loc);
  if (bClear)
  {
    CRect rectClip;
    pDC->GetClipBox(rectClip);
    pDC->FillSolidRect(rectClip, PALETTERGB(255,255,255));
    bClear = FALSE;
  }

  // State information
  CTextLoc locOld = loc;
  COLORREF colorOld = theApp.m_colorNormal;
  BOOL bBoldOld = FALSE;
  CTextLoc locNew;
  COLORREF colorNew;
  BOOL bBoldNew;
  int indent = 0;
  int level = 0;

  // Go drawing
  while (sl == SLGotcha)
  {
    // Get string
    const CString& str = GetTextBuffer()->GetLine(loc.nLine);
    // Parse component
    locNew = loc;
    bBoldNew = FALSE;
    colorNew = theApp.m_colorNormal;
    if (indent>0)
    {
      bBoldNew = TRUE;
      colorNew = theApp.m_colorDefine;
    }

    char c = ForwardParse(loc, st);
    // Compute color
    if (c)
      switch(st)
      {
      case IN_STRING:
      case IN_STRING_ESCAPE:
	locNew = loc;
	colorNew = theApp.m_colorString;
        bBoldNew = FALSE;
	while (ForwardParse(loc, st))
	  if (st!=IN_STRING && st!=IN_STRING_ESCAPE)
	    break;
        if (locNew < loc) 
          BackwardParse(loc, st);
	if (indent>0 && level==0)
	  indent -= 1;
	break;
      case IN_HELP:
	loc.nChar = str.GetLength();
	colorNew = theApp.m_colorHelp;
	bBoldNew = TRUE;
	break;
      case IN_COMMENT:
	loc.nChar = str.GetLength();
	colorNew = theApp.m_colorComment;
        bBoldNew = FALSE;
	break;
      case IN_SEP:
	if (indent>0)
	{
	  if (c=='(')
	    level += 1;
	  if (c==')')
	  {
	    level -= 1;
	    if (level==0)
	      indent -= 1;
	    if (level<0)
	      indent = 0;
	  }
	  // definition do not extend past current line
	  if (c=='\n')
	    indent = 0;
	}
	else if (c=='(')
	{
	  int indTmp;
	  CTextLoc locTmp = loc;
	  EatSpace(locTmp);
	  if (EatWord(locTmp, &indTmp)==KW_DEFINE) 
	    if (indTmp > 0)
	    {
	      bBoldNew = TRUE;
	      colorNew = theApp.m_colorDefine;
	      indent = indTmp;
	    }
	}
	break;

      case IN_WORD:
	loc = locNew;
	if (EatWord(loc) == KW_KEYWORD)
	{
          bBoldNew = FALSE;
	  colorNew = theApp.m_colorKeyword;
	}
	if (indent>0 && level==0)
	  indent -= 1;
	break;

      case IN_WORD_ESCAPE:
	while (ForwardParse(loc, st))
	  if (st!=IN_WORD_ESCAPE)
	    break;
	if (indent>0 && level==0)
	  indent -= 1;
	break;
      }
    // Display
    if (colorNew!=colorOld || bBoldNew!=bBoldOld || locNew.nChar==str.GetLength())
    {
      if (locOld < locNew)
	sl = DrawRange(pDC, locOld, locNew, colorOld, bBoldOld);
      locOld = locNew;
      colorOld = colorNew;
      bBoldOld = bBoldNew;
    }
    // Check end of file
    if (c==0)
      break;
  }
}


