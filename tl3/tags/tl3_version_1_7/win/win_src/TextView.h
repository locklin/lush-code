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
 
// TextView.h : interface of the CTextView class
//
/////////////////////////////////////////////////////////////////////////////


//////////// CTextView

class CTextView : public CView
{
protected: // create from serialization only
	CTextView();
	DECLARE_DYNCREATE(CTextView)

// Attributes
protected:
	// Screen
	int m_nFirstVisibleLine;
	int m_nFirstVisiblePos;
	LOGFONT m_lfNormal;
	int m_nCWNormal;
	int m_nCHNormal;
	int m_nLogPixSX;
	int m_nLogPixSY;
	int m_nLeftMargin;
	// Printing
	int m_nCWPrint;
	int m_nCHPrint;
	int m_nLinesPerPage;
	// Document
	CTextBuffer *m_pTextBuffer;
private:
	// Selection
	int m_nSelHideCount;
	BOOL m_bOverlayMode;
	CTextLoc m_tlSelFrom;
	CTextLoc m_tlSelTo;
	BOOL m_bActive;
	// Mouse tracking
	int  m_nUpDownPos;
	BOOL m_bDragging;
	BOOL m_bDblClick;
	CTextLoc m_tlMDFrom;
	CTextLoc m_tlMDTo;
	CPoint m_pntMouse;
	UINT m_iTimer;
	// Search and Replace
	BOOL m_bFirstSearch;
	BOOL m_bWrapSearch;
	CTextLoc m_tlSearchPos;
	// Cursor
	static HCURSOR s_hIBeamCursor;

// Operations
public:
	CTextBuffer* GetTextBuffer() { return m_pTextBuffer; };
	void GetSelection(CTextLoc& tlFrom, CTextLoc& tlTo);
	void SetSelection(CTextLoc& tlFrom, CTextLoc& tlTo);
	void HideSelection();
	void ShowSelection();
	void ScrollTo(CTextLoc loc);
	void Scroll(int nl, int nc=0);
	void InsertChar(const CString &str);
	void DoInitialUpdate();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnInitialUpdate();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView*, CView*);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTextView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	// Display
	enum SLoc { SLAbove, SLGotcha, SLBelow };
	SLoc LocToLP(CDC *pDC, int nLine, int nChar, int& cx, int& cy);
	BOOL DPToLoc(CPoint point, CTextLoc &loc);
	void SetScrollBars();
	// Selection
	void SelectionRgn(CDC *pDC, CRgn& rgn);
	int  CharacterClass(char c);
	void ExtendToWord(BOOL bForward, CTextLoc& loc);
	void FindWordBoundary(BOOL bForward, CTextLoc& loc);
	// Find and Replace
	void AdjustDialogPosition(CDialog* pDlg);
	void OnEditFindReplace(BOOL bFindOnly);
	void OnFindNext(LPCTSTR lpszFind, BOOL bCase, BOOL bWord);
	void OnReplaceSel(LPCTSTR szFind, BOOL bCase, BOOL bWord, LPCTSTR szRep);
	void OnReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL, BOOL);
	LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);
	BOOL SameAsSelected(LPCTSTR lpszCompare, BOOL bCase);
	BOOL FindText(LPCTSTR lpszFind, BOOL bCase, BOOL bWord);
	void TextNotFound(LPCTSTR lpszFind);
	BOOL Find(LPCSTR, BOOL bCase, BOOL bWord, CTextLoc&, CTextLoc& );
	// Overridables
	virtual void InvalidateLines(int nFrom, int nTo);
	virtual void DrawText(CDC *pDC, BOOL bClear);
	virtual int  IsWordBoundary(CTextLoc loc);  // 1: begin, 2: end, 4: inword
	virtual void Replace(CTextLoc, CTextLoc, const CString &);	
	// Generated message map functions
protected:
	//{{AFX_MSG(CTextView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateNeedSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedClip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedText(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedFind(CCmdUI* pCmdUI);
	afx_msg void OnEditFind();
	afx_msg void OnEditReplace();
	afx_msg void OnEditRepeat();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFilePrint();
	//}}AFX_MSG
	afx_msg void OnUpdateOvrIndicator(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLCIndicator(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
};


//////// Character Classes

#define CLASS_SPECIAL	  0
#define CLASS_CONTROL     1
#define CLASS_SEPARATOR   2
#define CLASS_BLANK       3
#define CLASS_WORD        4


//////// WORD BOUNDARY CONSTANT

#define WB_BEGIN   1
#define WB_END     2
#define WB_INSIDE  4

   


/////////////////////////////////////////////////////////////////////////////
