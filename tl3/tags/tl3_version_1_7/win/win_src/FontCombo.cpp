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
 
// FontCombo.cpp : implementation file
// See FontCombo.h for details on how to use this class
//

#include "stdafx.h"
#include "resource.h"
#include "FontCombo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFontNameComboBox

CFontNameComboBox::CFontNameComboBox()
{
  // Load TT bitmap
  m_bmpTrueType.LoadBitmap(IDB_TTIMAGE);
  m_bmpMask.LoadBitmap(IDB_TTMASK);
  // Get TT bitmap dimension
  BITMAP bm;
  m_bmpTrueType.GetBitmap(&bm);
  m_nBMWidth = bm.bmWidth;
  m_nBMHeight = bm.bmHeight;
}

CFontNameComboBox::~CFontNameComboBox()
{
}


BEGIN_MESSAGE_MAP(CFontNameComboBox, CComboBox)
	//{{AFX_MSG_MAP(CFontNameComboBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFontNameComboBox message handlers

#define DSx     0x00660046L
#define DSna    0x00220326L

// CFontNameComboBox::DrawItem
// -- This function draws the font name and true type stuff 
//    in the client space of this combo box item.

void 
CFontNameComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
  ASSERT( lpDIS->CtlType == ODT_COMBOBOX );
  if (lpDIS->itemID == -1)
    return;
  CDC* pDC = CDC::FromHandle(lpDIS->hDC);
  COLORREF crBk, crText;
  TEXTMETRIC tm;
  int x, y;
  
  // Calculate the colors to use
  crBk = GetSysColor(COLOR_WINDOW);
  crText = GetSysColor(COLOR_WINDOWTEXT);
  if (lpDIS->itemState & ODS_SELECTED)
  {
    crBk = GetSysColor(COLOR_HIGHLIGHT);
    crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
  }
  
  // Get information
  CString strItem;
  GetLBText(lpDIS->itemID,strItem);
  DWORD dwInfo = 0;
  m_map.Lookup(strItem, dwInfo);

  // Draw text
  pDC->GetTextMetrics( &tm );
  x = lpDIS->rcItem.left + m_nBMWidth + 4;
  y = (lpDIS->rcItem.bottom + lpDIS->rcItem.top - tm.tmHeight) / 2;
  crText = pDC->SetTextColor(crText);
  crBk = pDC->SetBkColor(crBk);
  pDC->ExtTextOut(x, y, ETO_CLIPPED|ETO_OPAQUE, &lpDIS->rcItem, strItem, NULL);
  
  // Draw the TT bitmap
  if (dwInfo & 0x80000000)
  {
    CDC dcCompat;
    dcCompat.CreateCompatibleDC(pDC);
    CBitmap* pbmpSave = dcCompat.SelectObject(&m_bmpTrueType);
    x = lpDIS->rcItem.left + 2;
    y = (lpDIS->rcItem.bottom + lpDIS->rcItem.top - m_nBMHeight) / 2;
    pDC->SetBkColor(RGB(255, 255, 255));
    pDC->SetTextColor(RGB(0, 0, 0));
    pDC->BitBlt(x, y, m_nBMWidth, m_nBMHeight, &dcCompat, 0, 0, DSx);
    dcCompat.SelectObject(&m_bmpMask);
    pDC->BitBlt(x, y, m_nBMWidth, m_nBMHeight, &dcCompat, 0, 0, DSna);
    dcCompat.SelectObject(&m_bmpTrueType);
    pDC->BitBlt(x, y, m_nBMWidth, m_nBMHeight, &dcCompat, 0, 0, DSx);
    dcCompat.SelectObject(pbmpSave);
  }

  // Draw focus rect
  pDC->SetTextColor( crText );
  pDC->SetBkColor( crBk );
  if (lpDIS->itemAction & ODA_FOCUS)
    pDC->DrawFocusRect(&lpDIS->rcItem);
}



// EnumFontCallback2
// -- Callback function for enumerating fonts sizes

static int CALLBACK 
EnumFontCallback2(ENUMLOGFONT *pelf, NEWTEXTMETRIC *ptm, int type, LPARAM lparam)
{
  DWORD *pdwInfo = (DWORD*)lparam;
  ASSERT(type != TRUETYPE_FONTTYPE);
  if (((pelf->elfLogFont.lfPitchAndFamily & 0x3)==FIXED_PITCH) &&
      (pelf->elfLogFont.lfCharSet==ANSI_CHARSET) )
  {
    int nSize = ptm->tmHeight;
    if (nSize>4 && nSize<31)
      *pdwInfo |= (1 << nSize);
  }
  return TRUE;
}



// EnumFontCallback1
// -- Callback function for enumerating fonts families

static int CALLBACK 
EnumFontCallback1(ENUMLOGFONT *pelf, NEWTEXTMETRIC *ptm, int type, LPARAM lparam)
{
  if (((pelf->elfLogFont.lfPitchAndFamily & 0x3)==FIXED_PITCH) &&
      (pelf->elfLogFont.lfCharSet==ANSI_CHARSET) )
  {
    DWORD dwInfo = 0xffffffff;
    if (type != TRUETYPE_FONTTYPE)
    {
      dwInfo = 0;
      CDisplayIC cdc;
      cdc.SetMapMode(MM_ANISOTROPIC);
      cdc.SetWindowExt(72,72);
      cdc.SetViewportExt(cdc.GetDeviceCaps(LOGPIXELSX), 
			 cdc.GetDeviceCaps(LOGPIXELSY));
      EnumFontFamilies(cdc,(LPCSTR)pelf->elfLogFont.lfFaceName, 
              (FONTENUMPROC)EnumFontCallback2, (LPARAM)&dwInfo);
      dwInfo &= 0x7fffffff;
    }
    CFontNameComboBox *pFNCB = (CFontNameComboBox*)lparam;
    pFNCB->AddFont((LPCSTR)pelf->elfLogFont.lfFaceName, dwInfo);
  }
  return TRUE;
}



// CFontNameComboBox::LoadList
// -- Load the listbox components

void 
CFontNameComboBox::LoadList()
{
  CDisplayIC cdc;
  EnumFontFamilies(cdc, NULL, (FONTENUMPROC)EnumFontCallback1, (LPARAM)this);
}




// CFontNameComboBox::AddFont
// -- Add a font if it is not already present
//    TT symbol will be displayed if high bit of dwInfo is set.

void 
CFontNameComboBox::AddFont(LPCSTR szName, DWORD dwInfo)
{
  DWORD dwJunk;
  if (!m_map.Lookup(szName, dwJunk))
    AddString(szName);
  m_map.SetAt(szName, dwInfo);
}



// CFontNameComboBox::GetLBInfo
// -- Return the info dword associated to the 
//    specified font name.

DWORD
CFontNameComboBox::GetLBInfo(int nindex)
{
  DWORD dwInfo;
  CString szName;
  GetLBText(nindex,szName);
  if (!m_map.Lookup(szName, dwInfo))
    return 0xffffffff;
  else
    return dwInfo;
}






