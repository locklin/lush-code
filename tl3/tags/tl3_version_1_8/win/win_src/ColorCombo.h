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
 
// ColorCombo.h : header file
//

#ifndef __COLORCOMBO_H__
#define __COLORCOMBO_H__

// This class handles an owner-draw combo box.  The combo box in question
// handles color bars instead of text.  It shows all of the required
// features for an owner-draw combo box.  
//
// You can actually handle just about anything you want in an owner-draw 
// combo box as long as you override functions CompareItem, DrawItem, 
// and MeasureItem.  Strings are easiest and don't require all the 
// overrides.  Other examples: bitmaps, text in varying fonts, text in a 
// non-standard font, etc. 
//
// TODO:
//	1.	Create an owner-draw combo box in your resources to use with
//		this class.  You will need to have the following features set
//		on the Styles page for the combo box:
//		- Type: Drop List
//		- Owner-Draw: Variable
//		- Has Strings: Unchecked
//		- Sort: Checked (to show off owner-draw sorting)
//
//	2.	Subclass your owner-draw combo box to associate it with this
//		class.  The following statement placed in your dialog 
//		initialization function (typically OnInitDialog() for MFC
//		applications) for the dialog you used this combo box in should 
//		work fine:
//			m_myDraw.SubclassDlgItem(IDC_MYDRAW, this);
//		where:
//			m_myDraw is an instance of the owner-draw combo box class
//			IDC_MYDRAW is the ID associated with your combo box resource
//
//	3.	Call m_myDraw.LoadList() to initialize the contents of the owner-draw
//		combo box.  Do this immediately after you subclass the combo box
//		control.

/////////////////////////////////////////////////////////////////////////////
// CColorComboBox window

class CColorComboBox : public CComboBox
{
// Construction
public:
	CColorComboBox();

// Attributes
public:

// Operations
public:
	void AddListItem(COLORREF color);
	int SelectItem(COLORREF color);
	void LoadList();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorComboBox)
	public:
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorComboBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif __COLORCOMBO_H__		// include class CColorComboBox