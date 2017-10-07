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
 
// SnView.h : interface of the CSNView class
//
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////
// Context aware editor data structures


// Identify current location
enum SParseState {
  IN_SEP,
  IN_WORD,
  IN_WORD_ESCAPE,
  IN_STRING,
  IN_STRING_ESCAPE,
  IN_MCHAR,
  IN_COMMENT,
  IN_HELP,
};



//////////////////////////////////////////////////
// Class CSNView

class CSNView : public CTextView
{
protected: // create from serialization only
	CSNView();
	DECLARE_DYNCREATE(CSNView)

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSNView)
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL
protected:
	virtual void DrawText(CDC *pDC, BOOL bClear);
	virtual int IsWordBoundary(CTextLoc loc);

	
// Implementation
public:
	virtual ~CSNView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	int IsSeparator(char c);
	int IsWordBoundary(CTextLoc loc, SParseState st);
	char ForwardParse(CTextLoc& loc, SParseState &st);
	char BackwardParse(CTextLoc& loc, SParseState &st);
	SParseState SyncState(const CTextLoc& loc, int nContext=24);
	BOOL FindNextRParen( CTextLoc& loc, int nContext=1024 );
	BOOL FindPrevLParen( CTextLoc& loc, int nContext=1024 );
	void EatSpace(CTextLoc& loc, BOOL bSameLine=TRUE);
	int  EatWord(CTextLoc& loc, int *pIndent=0);
	int  GetLineIndent(int nLine);
	int  ComputeLineIndent(int nLine);
	void IndentLine(int nLine, int nIndent);
	SLoc DrawRange(CDC *pDC, CTextLoc, CTextLoc, COLORREF, BOOL bBold);

// Generated message map functions
protected:
	//{{AFX_MSG(CSNView)
	afx_msg void OnEditBalance();
	afx_msg void OnEditEval();
	afx_msg void OnEditIndent();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUpdateEditEval(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
