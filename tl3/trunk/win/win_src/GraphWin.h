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
 
// GraphWin.h : header file
//


// Forward declaration of classes
class CGraphDoc;
class CGraphView;




/////////////////////////////////////////////////////////////////////////////
// CGraphDoc document

class CGraphDoc : public CDocument
{
protected:
	CGraphDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGraphDoc)

// Attributes
public:
	CGraphView	*m_pGraphView;
	const char      *m_szTitle;
	int		 m_nWidth;
	int		 m_nHeight;
	// Drawing surface
	BOOL		 m_bReady;
	CDC		 m_dcOffMap;
	HPALETTE	 m_hPalette;
	HBITMAP		 m_hbmOffMap;
	HBITMAP          m_hbmOld;
        // Creation
        static const char *s_szTitle;
	// Synchronization
	static CCriticalSection s_csOffMap;
	// Palette
	static HPALETTE	s_hPalette;
	static LOGPALETTE *s_logPalette;
	static int s_nPaletteCount;
	static int s_nSysEntries;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual void SetTitle(LPCTSTR lpszTitle);
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGraphDoc();
	void CreateOffMap(int nWidth, int nHeight);
	BOOL ResizeOffMap(int nWidth, int nHeight);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
private:
  	HPALETTE LockPalette(HDC hdc);
	void UnlockPalette(HPALETTE hpal);
	HBITMAP MakeDIBSection(HDC hdc, int nWidth, int nHeight);

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphDoc)
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditCopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};










/////////////////////////////////////////////////////////////////////////////
// CGraphView view

class CGraphView : public CView
{
protected:
	CGraphView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGraphView)

// Attributes
public:
	BOOL m_bSizeChanged;
private:
	BOOL  m_bButtonDown;
	UINT  m_nModeHilite;
	CRect m_rectHilite;
	int   m_nMouseX;
	int   m_nMouseY;
	static HCURSOR s_hWaitCursor;
	static HCURSOR s_hArrowCursor;

// Operations
public:
	CGraphDoc *GetGraphDoc();
	void AdjustSize();
	void Update(CRect &rect);
	void Hilite(UINT mode, int x1=0, int y1=0, int x2=0, int y2=0);
	void EventAdd(int code, int arg=0, int x=0, int y=0);
        void SetStatusCursor();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CGraphView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
private:
	void OnButtonDown(UINT nFlags, CPoint point);
	void OnButtonUp(UINT nFlags, CPoint point);


	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateUIDisable(CCmdUI* pCmdUI);
	afx_msg void OnPaint();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnFilePrint();
	afx_msg BOOL OnQueryNewPalette();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};





/////////////////////////////////////////////////////////////////////////////

inline CGraphDoc* CGraphView::GetGraphDoc()
{
  CDocument *pDoc = GetDocument();
  ASSERT_KINDOF(CGraphDoc, pDoc);
  return (CGraphDoc*)pDoc;
}
