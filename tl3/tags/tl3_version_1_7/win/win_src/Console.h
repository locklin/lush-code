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
 
// Console.h : header file
//


class CConsDoc;
class CConsView;

/////////////////////////////////////////////////////////////////////////////
// CConsDoc document

class CConsDoc : public CTextDoc
{
protected:
	CConsDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CConsDoc)

// Attributes
public:
	CConsView* m_consView;
	CTextLoc m_tlOutput;
	BOOL m_bHeld;

private:
	BOOL m_bInStr;
	BOOL m_bInEsc;
	BOOL m_bMayBeCRLF;

// Operations
public:
	void Write(const CString& str);
	void Append(const CString& str);
	BOOL ReadLine(CString& str, BOOL &bEof);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual void SetTitle(LPCTSTR lpszTitle);
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConsDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CConsDoc)
	afx_msg void OnFileSaveAs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CConsView view

class CConsView : public CTextView
{
protected:
	CConsView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CConsView)

// Operations
public:
	CConsDoc* GetConsoleDoc();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
	virtual void Replace(CTextLoc, CTextLoc, const CString &);
	virtual void DrawText(CDC *pDC, BOOL bClear);

// Implementation
protected:
	virtual ~CConsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CConsView)
	afx_msg void OnUpdateUIPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUICut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDisable(CCmdUI* pCmdUI);
	afx_msg void OnConsoleJump();
	afx_msg void OnConsoleBreak();
	afx_msg void OnConsoleEof();
	afx_msg void OnUpdateConsoleEof(CCmdUI* pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditEval();
	afx_msg void OnUpdateEditEval(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
        void OnUpdateOvrIndicator(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

inline CConsDoc* CConsView::GetConsoleDoc()
{
  CDocument *pDoc = GetDocument();
  ASSERT_KINDOF(CConsDoc, pDoc);
  return (CConsDoc*)pDoc;
}
