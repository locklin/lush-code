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
 
// PropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WinTL3.h"
#include "TextDoc.h"
#include "TextView.h"
#include "PropSheet.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif




/////////////////////////////////////////////////////////////////////////////
// CPropSheet

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

CPropSheet::CPropSheet(CWnd* pWndParent)
	 : CPropertySheet(IDS_PROPSHEET, pWndParent)
{
  AddPage(&m_PageEdit);
  AddPage(&m_PageCons);
}

CPropSheet::~CPropSheet()
{
}


BEGIN_MESSAGE_MAP(CPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPropSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropSheet message handlers


// CPropSheet::Apply()
// -- Call the SetPreferences message in all pages
//    and update the text editor windows

BOOL 
CPropSheet::Apply()
{
  // Setup preferences
  m_PageEdit.SetPreferences();
  m_PageCons.SetPreferences();
  // Update all text windows
  POSITION templpos = theApp.GetFirstDocTemplatePosition();
  while (templpos) 
  {
    CDocTemplate *tmpl = theApp.GetNextDocTemplate(templpos);
    POSITION docpos = tmpl->GetFirstDocPosition();
    while (docpos) 
    {
      CDocument *doc = tmpl->GetNextDoc(docpos);
      POSITION viewpos = doc->GetFirstViewPosition();
      while (viewpos)
      {
       CView *view = doc->GetNextView(viewpos);
       if (view && view->IsKindOf(RUNTIME_CLASS(CTextView)))
       {
	 CTextView *pTV = (CTextView*)view;
	 pTV->DoInitialUpdate();
	 pTV->Invalidate();
       }
      }
    }
  }
  // Return true
  return TRUE;
}





/////////////////////////////////////////////////////////////////////////////
// CPageEdit property page

IMPLEMENT_DYNCREATE(CPageEdit, CPropertyPage)

CPageEdit::CPageEdit() : CPropertyPage(CPageEdit::IDD)
{
  //{{AFX_DATA_INIT(CPageEdit)
  //}}AFX_DATA_INIT
  m_nSize = theApp.m_nFontSize;
  m_strFace = theApp.m_sFontName;
  m_nColorize = theApp.m_bColorize;
  m_nAutoIndent = theApp.m_bAutoIndent;
  m_colors[0] = theApp.m_colorNormal;
  m_colors[1] = theApp.m_colorComment;
  m_colors[2] = theApp.m_colorKeyword;
  m_colors[3] = theApp.m_colorString;
  m_colors[4] = theApp.m_colorDefine;
  m_colors[5] = theApp.m_colorHelp;
}

CPageEdit::~CPageEdit()
{
}

// CPageEdit::DoDataExchange
// -- transfer data from dialog controls to/from dialog variables.

void 
CPageEdit::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageEdit)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_FONTNAME, m_CBFace);
	DDX_CBString(pDX, IDC_FONTNAME, m_strFace);
	DDX_Control(pDX, IDC_FONTSIZE, m_CBSize);
	DDX_Text(pDX, IDC_FONTSIZE, m_nSize);
	DDV_MinMaxInt(pDX, m_nSize, 4, 48);
	DDX_Check(pDX, IDC_COLORIZE, m_nColorize);
	DDX_Control(pDX, IDC_CATEGORY, m_LBCategory);
	DDX_Control(pDX, IDC_COLOR, m_CBColor);
	DDX_Check(pDX, IDC_AUTOINDENT, m_nAutoIndent);
        // Adjust size 
        if (!pDX->m_bSaveAndValidate)
          SetFontSizes();
}

BEGIN_MESSAGE_MAP(CPageEdit, CPropertyPage)
	//{{AFX_MSG_MAP(CPageEdit)
	ON_CBN_SELCHANGE(IDC_COLOR, OnColorSel)
	ON_LBN_SELCHANGE(IDC_CATEGORY, OnCategorySel)
	ON_BN_CLICKED(IDC_DEFCOLOR, OnDefColor)
	ON_BN_CLICKED(IDC_DEFFONT, OnDefFont)
	ON_BN_CLICKED(IDC_AUTOINDENT, OnMiscAction)
	ON_BN_CLICKED(IDC_COLORIZE, OnMiscAction)
	ON_CBN_SELCHANGE(IDC_FONTNAME, OnFontAction)
	ON_CBN_SELCHANGE(IDC_FONTSIZE, OnMiscAction)
	ON_CBN_EDITCHANGE(IDC_FONTSIZE, OnMiscAction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CPageEdit message handlers


// CPageEdit::OnInitDialog
// -- Initialize the dialog data

BOOL 
CPageEdit::OnInitDialog() 
{
  CPropertyPage::OnInitDialog();
  // Fill the CBFace entries
  m_CBFace.LoadList();
  // Fill the CBSize entries
  m_CBSize.ResetContent();
  // Fill the LBCategory entries
  m_LBCategory.AddString("Code");
  m_LBCategory.AddString("Comments");
  m_LBCategory.AddString("Keywords");
  m_LBCategory.AddString("Strings");
  m_LBCategory.AddString("Function Headers (bold)");
  m_LBCategory.AddString("Help Headers (bold)");
  m_LBCategory.SetCurSel(0);
  // Fill the CBColor entries
  m_CBColor.LoadList();
  m_CBColor.SelectItem(m_colors[0]);
  // Update dialog
  UpdateData(FALSE);
  return TRUE;
}



// CPageEdit::OnColorSel
// -- called when user selects a color in the combo box

void 
CPageEdit::OnColorSel() 
{
  int nCategory = m_LBCategory.GetCurSel();
  if (nCategory>=0 && nCategory<sizeof(m_colors))
  {
    int nSelect = m_CBColor.GetCurSel();
    if (nSelect != CB_ERR)
      m_colors[nCategory] = m_CBColor.GetItemData(nSelect);
    SetModified();
  }
}

// CPageEdit::OnCategorySel
// -- called when user selects a text category

void 
CPageEdit::OnCategorySel() 
{
  int nCategory = m_LBCategory.GetCurSel();
  if (nCategory>=0 && nCategory<sizeof(m_colors))
    m_CBColor.SelectItem(m_colors[nCategory]);
}


// CPageEdit::OnDefColor
// -- called when user presses the default color button


void 
CPageEdit::OnDefColor() 
{
  m_colors[0] = PALETTERGB(0,0,0);
  m_colors[1] = PALETTERGB(128,0,0);
  m_colors[2] = PALETTERGB(0,0,128);
  m_colors[3] = PALETTERGB(128,128,128);
  m_colors[4] = PALETTERGB(0,128,128);
  m_colors[5] = PALETTERGB(128,0,0);
  OnCategorySel();
  SetModified();
}


// CPageEdit::OnDefFont
// -- called when user presses the default font button

void 
CPageEdit::OnDefFont() 
{
  m_CBFace.SelectString(-1,"Courier");
  SetFontSizes();
  m_CBSize.SelectString(-1,"10");
  SetModified();
}


// CPageEdit::OnFontAction
// -- called when user changes the font

void 
CPageEdit::OnFontAction() 
{
  SetFontSizes();
  SetModified();
}


// CPageEdit::OnMiscAction
// -- called when user changes something that do not
//    require complex action.

void 
CPageEdit::OnMiscAction() 
{
  SetModified();
}


// CPageEdit::SetFontSizes()
// -- set available font sizes

BOOL CPageEdit::SetFontSizes()
{
  int nIndex = m_CBFace.GetCurSel();
  DWORD dwInfo = 0xffffffff;
  if (nIndex != CB_ERR)
    dwInfo = m_CBFace.GetLBInfo(nIndex);
  if (dwInfo & 0x80000000)
    dwInfo = (1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12)|(1<<14)|(1<<18);
  // Set sizes
  CString szSel;
  m_CBSize.GetWindowText(szSel);
  m_CBSize.ResetContent();
  for (int i = 4; i<31; i++)
    if (dwInfo & (1<<i))
    {
      char buffer[4];
      sprintf(buffer,"%d",i);
      m_CBSize.AddString(buffer);
    }
  m_CBSize.SetWindowText(szSel);
  return TRUE;
}



// CPageEdit::SetPreferences()
// -- store preferences settings
//    return true

BOOL CPageEdit::SetPreferences()
{
  theApp.m_nFontSize = m_nSize;
  theApp.m_sFontName = m_strFace;
  theApp.m_bColorize = m_nColorize;
  theApp.m_bAutoIndent = m_nAutoIndent;
  theApp.m_colorNormal = m_colors[0];
  theApp.m_colorComment = m_colors[1];
  theApp.m_colorKeyword = m_colors[2];
  theApp.m_colorString = m_colors[3];
  theApp.m_colorDefine = m_colors[4];
  theApp.m_colorHelp = m_colors[5];
  return TRUE;
}



// CPageEdit::OnApply
// -- Called when user presses the <<Apply>> button.

BOOL CPageEdit::OnApply() 
{
  if (UpdateData(TRUE))
  {
    CWnd *pWnd = GetParent();
    if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CPropSheet)))
      ((CPropSheet*)pWnd)->Apply();
    return TRUE;
  }
  return FALSE;
}






/////////////////////////////////////////////////////////////////////////////
// CPageCons property page

IMPLEMENT_DYNCREATE(CPageCons, CPropertyPage)

CPageCons::CPageCons() : CPropertyPage(CPageCons::IDD)
{
  //{{AFX_DATA_INIT(CPageCons)
  //}}AFX_DATA_INIT
  m_nColorize = theApp.m_bConsColorize;
  m_nLineWidth = theApp.m_nConsRightMargin;
  m_nWordWrap = theApp.m_bConsWordWrap;
  m_nAutoEOL = theApp.m_bConsAutoEOL;
  m_colors[0] = theApp.m_colorConsNormal;
  m_colors[1] = theApp.m_colorConsInput;
  m_colors[2] = theApp.m_colorConsEdit;
}

CPageCons::~CPageCons()
{
}

void CPageCons::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CPageCons)
  //}}AFX_DATA_MAP
  DDX_Text(pDX, IDC_LINEWIDTH, m_nLineWidth);
  DDV_MinMaxInt(pDX, m_nLineWidth, 40, 1024);
  DDX_Check(pDX, IDC_WORDWRAP, m_nWordWrap);
  DDX_Check(pDX, IDC_COLORIZE, m_nColorize);
  DDX_Check(pDX, IDC_AUTOEOL, m_nAutoEOL);
  DDX_Control(pDX, IDC_CATEGORY, m_LBCategory);
  DDX_Control(pDX, IDC_COLOR, m_CBColor);
}


BEGIN_MESSAGE_MAP(CPageCons, CPropertyPage)
	//{{AFX_MSG_MAP(CPageCons)
	ON_CBN_SELCHANGE(IDC_COLOR, OnColorSel)
	ON_LBN_SELCHANGE(IDC_CATEGORY, OnCategorySel)
	ON_BN_CLICKED(IDC_DEFCOLOR, OnDefColor)
	ON_BN_CLICKED(IDC_COLORIZE, OnMiscAction)
	ON_BN_CLICKED(IDC_WORDWRAP, OnMiscAction)
	ON_BN_CLICKED(IDC_AUTOEOL,  OnMiscAction)
	ON_EN_CHANGE(IDC_LINEWIDTH, OnMiscAction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageCons message handlers


// CPageCons::OnInitDialog
// -- Initialize the dialog data

BOOL 
CPageCons::OnInitDialog() 
{
  CPropertyPage::OnInitDialog();
  // Fill the LBCategory entries
  m_LBCategory.AddString("Normal");
  m_LBCategory.AddString("Previous Input");
  m_LBCategory.AddString("Editable Input");
  m_LBCategory.SetCurSel(0);
  // Fill the CBColor entries
  m_CBColor.LoadList();
  m_CBColor.SelectItem(m_colors[0]);
  // Update dialog
  UpdateData(FALSE);
  return TRUE;
}



// CPageCons::OnColorSel
// -- called when user selects a color in the combo box

void 
CPageCons::OnColorSel() 
{
  int nCategory = m_LBCategory.GetCurSel();
  if (nCategory>=0 && nCategory<sizeof(m_colors))
  {
    int nSelect = m_CBColor.GetCurSel();
    if (nSelect != CB_ERR)
      m_colors[nCategory] = m_CBColor.GetItemData(nSelect);
    SetModified();
  }
}

// CPageCons::OnCategorySel
// -- called when user selects a text category

void 
CPageCons::OnCategorySel() 
{
  int nCategory = m_LBCategory.GetCurSel();
  if (nCategory>=0 && nCategory<sizeof(m_colors))
    m_CBColor.SelectItem(m_colors[nCategory]);
}


// CPageCons::OnDefColor
// -- called when user presses the default color button


void 
CPageCons::OnDefColor() 
{
  m_colors[0] = PALETTERGB(0,0,0);
  m_colors[1] = PALETTERGB(0,0,128);
  m_colors[2] = PALETTERGB(0,128,0);
  OnCategorySel();
  SetModified();
}


// CPageCons::OnMiscAction
// -- called when user changes something that do not
//    require complex action.

void 
CPageCons::OnMiscAction() 
{
  SetModified();
}


// CPageCons::SetPreferences()
// -- store preferences settings
//    return true

BOOL CPageCons::SetPreferences()
{
  theApp.m_nConsRightMargin = m_nLineWidth;
  theApp.m_bConsWordWrap = m_nWordWrap;
  theApp.m_bConsColorize = m_nColorize;
  theApp.m_bConsAutoEOL = m_nAutoEOL;
  theApp.m_colorConsNormal = m_colors[0];
  theApp.m_colorConsInput = m_colors[1];
  theApp.m_colorConsEdit = m_colors[2];
  return TRUE;
}


// CPageCons::OnApply
// -- Called when user presses the <<Apply>> button.

BOOL CPageCons::OnApply() 
{
  if (UpdateData(TRUE))
  {
    CWnd *pWnd = GetParent();
    if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CPropSheet)))
      ((CPropSheet*)pWnd)->Apply();
    return TRUE;
  }
  return FALSE;
}



