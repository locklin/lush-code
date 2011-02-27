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
 
// PropSheet.h : header file
//
// This class defines custom modal property sheet 
// CPropSheet.
 
#ifndef __PROPSHEET_H__
#define __PROPSHEET_H__


#include "ColorCombo.h"
#include "FontCombo.h"



/////////////////////////////////////////////////////////////////////////////
// CPageEdit dialog

class CPageEdit : public CPropertyPage
{
	DECLARE_DYNCREATE(CPageEdit)

// Construction
public:
	CPageEdit();
	~CPageEdit();
	BOOL SetPreferences();
        BOOL SetFontSizes();

// Dialog Data
	//{{AFX_DATA(CPageEdit)
	enum { IDD = IDD_PAGEEDIT };
	//}}AFX_DATA
	CFontNameComboBox       m_CBFace;
	CString                 m_strFace;
	CComboBox               m_CBSize;
	int                     m_nSize;
	int                     m_nColorize;
	CListBox                m_LBCategory;
	CColorComboBox          m_CBColor;
	COLORREF		m_colors[8];
	int	                m_nAutoIndent;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageEdit)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageEdit)
	afx_msg void OnColorSel();
	afx_msg void OnCategorySel();
	afx_msg void OnDefColor();
	afx_msg void OnDefFont();
	afx_msg void OnMiscAction();
	afx_msg void OnFontAction();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



/////////////////////////////////////////////////////////////////////////////
// CPageCons dialog

class CPageCons : public CPropertyPage
{
	DECLARE_DYNCREATE(CPageCons)

// Construction
public:
	CPageCons();
	~CPageCons();
	BOOL SetPreferences();

// Dialog Data
	//{{AFX_DATA(CPageCons)
	enum { IDD = IDD_PAGECONS };
	//}}AFX_DATA
	int			m_nLineWidth;
	int			m_nWordWrap;
	int			m_nColorize;
        int                     m_nAutoEOL;
	CListBox                m_LBCategory;
	CColorComboBox          m_CBColor;
	COLORREF		m_colors[8];


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageCons)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageCons)
	virtual BOOL OnInitDialog();
	afx_msg void OnColorSel();
	afx_msg void OnCategorySel();
	afx_msg void OnDefColor();
	afx_msg void OnMiscAction();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPropSheet


class CPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPropSheet)

// Construction
public:
	BOOL Apply();
	CPropSheet(CWnd* pParentWnd = NULL);

// Attributes
public:
	CPageEdit m_PageEdit;
	CPageCons m_PageCons;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CPropSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
#endif	// __PROPSHEET_H__


