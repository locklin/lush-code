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
 
// CG: This file was added by the Splash Screen component.
// Splash.cpp : implementation file
//

#include "stdafx.h"  // e. g. stdafx.h
#include "resource.h"  // e.g. resource.h
#include "Splash.h"  // e.g. splash.h

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif





/////////////////////////////////////////////////////////////////////////////
//   Code for handling bitmap resources as DIB instead of DDB
//   (from Knowledge Base Q124947



static HPALETTE 
CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
  LPBITMAPINFOHEADER  lpbi;
  LPLOGPALETTE     lpPal;
  HANDLE           hLogPal;
  HPALETTE         hPal = NULL;
  int              i;
  lpbi = (LPBITMAPINFOHEADER)lpbmi;
  if (lpbi->biBitCount <= 8)
    *lpiNumColors = (1 << lpbi->biBitCount);
  else
    *lpiNumColors = 0;  // No palette needed for 24 BPP DIB
  if (*lpiNumColors)
  {
    hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                           sizeof (PALETTEENTRY) * (*lpiNumColors));
    lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
    lpPal->palVersion    = 0x300;
    lpPal->palNumEntries = *lpiNumColors;

    for (i = 0;  i < *lpiNumColors;  i++)
    {
      lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
      lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
      lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
      lpPal->palPalEntry[i].peFlags = 0;
    }
    hPal = CreatePalette (lpPal);
    GlobalUnlock (hLogPal);
    GlobalFree   (hLogPal);
  }
  return hPal;
}


static HBITMAP 
LoadResourceBitmap(HINSTANCE hInstance, LPSTR lpString,
                   HPALETTE FAR* lphPalette)
{
  HRSRC  hRsrc;
  HGLOBAL hGlobal;
  HBITMAP hBitmapFinal = NULL;
  LPBITMAPINFOHEADER  lpbi;
  HDC hdc;
  int iNumColors;

  if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
  {
    hGlobal = LoadResource(hInstance, hRsrc);
    lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);
    hdc = GetDC(NULL);
    *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
    if (*lphPalette)
    {
      SelectPalette(hdc,*lphPalette,FALSE);
      RealizePalette(hdc);
    }
    hBitmapFinal = CreateDIBitmap(hdc,
       (LPBITMAPINFOHEADER)lpbi, (LONG)CBM_INIT,
       (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),
       (LPBITMAPINFO)lpbi, DIB_RGB_COLORS );
    ReleaseDC(NULL,hdc);
    UnlockResource(hGlobal);
    FreeResource(hGlobal);
  }
  return (hBitmapFinal);
}
 


/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class  
//   (derived from microsoft's component gallery)

BOOL CSplashWnd::c_bShowSplashWnd;
CSplashWnd* CSplashWnd::c_pSplashWnd;

CSplashWnd::CSplashWnd()
{
    m_hBitmap = NULL;
    m_hPalette = NULL;
}

CSplashWnd::~CSplashWnd()
{
    // Delete bitmap and palette
    if (m_hBitmap)
      DeleteObject(m_hBitmap);
    if (m_hPalette)
      DeleteObject(m_hPalette);
    m_hBitmap = NULL;
    m_hPalette = NULL;
    // Clear the static window pointer.
    ASSERT(c_pSplashWnd == this);
    c_pSplashWnd = NULL;
}

BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
	//{{AFX_MSG_MAP(CSplashWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void 
CSplashWnd::EnableSplashScreen(BOOL bEnable /*= TRUE*/)
{
    c_bShowSplashWnd = bEnable;
}

void 
CSplashWnd::ShowSplashScreen(CWnd* pParentWnd /*= NULL*/)
{
    if (!c_bShowSplashWnd || c_pSplashWnd != NULL)
      return;
    // Allocate a new splash screen and create the window.
    c_pSplashWnd = new CSplashWnd;
    if (!c_pSplashWnd->Create(pParentWnd))
      delete c_pSplashWnd;
    else
      c_pSplashWnd->UpdateWindow();
}

BOOL 
CSplashWnd::Create(CWnd* pParentWnd /*= NULL*/)
{
    // Load bitmap
    CDisplayIC dc;
    if (dc.GetDeviceCaps(BITSPIXEL) < 8)
      m_hBitmap = LoadResourceBitmap(AfxGetInstanceHandle(),
		     MAKEINTRESOURCE(IDB_SPLASH16), &m_hPalette);
    if (!m_hBitmap)
      m_hBitmap = LoadResourceBitmap(AfxGetInstanceHandle(),
		     MAKEINTRESOURCE(IDB_SPLASH), &m_hPalette);
    if (!m_hBitmap)
      return FALSE;
    // Create window
    BITMAP bm;
    GetObject(m_hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    return CreateEx(0,
		AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW)),
		NULL, WS_POPUP | WS_VISIBLE, 0, 0, 
		bm.bmWidth, bm.bmHeight, pParentWnd->GetSafeHwnd(), NULL);
}

void 
CSplashWnd::HideSplashScreen()
{
    // Destroy the window, and update the mainframe.
    DestroyWindow();
    AfxGetMainWnd()->UpdateWindow();
}

void 
CSplashWnd::PostNcDestroy()
{
    // Free the C++ class.
    delete this;
}

int 
CSplashWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;
    // Center the window.
    CenterWindow(AfxGetMainWnd());
    // Set a timer to destroy the splash screen.
    SetTimer(1, 2000, NULL);
    return 0;
}

void 
CSplashWnd::OnPaint()
{
  CPaintDC dc(this);
  CDC dcImage;
  if (!dcImage.CreateCompatibleDC(&dc))
    return;
  BITMAP bm;
  GetObject(m_hBitmap, sizeof(BITMAP), (LPSTR)&bm);
  // Select palettes
  HPALETTE hOldPalDC = NULL;
  HPALETTE hOldPalDCImage = NULL;
  if (dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
  {
    hOldPalDC = SelectPalette(dc,m_hPalette,FALSE);
    RealizePalette(dc);
    hOldPalDCImage = SelectPalette(dcImage,m_hPalette,FALSE);
    RealizePalette(dcImage);
  }
  // Paint the image.
  HBITMAP hOldBitmap = (HBITMAP)SelectObject(dcImage, m_hBitmap);
  dc.BitBlt(0, 0, bm.bmWidth, bm.bmHeight, &dcImage, 0, 0, SRCCOPY);
  SelectObject(dcImage, hOldBitmap);
  // Paint version number
  LOGFONT lf;
  memset(&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = -14;
  lf.lfWeight = FW_BOLD;
  lf.lfCharSet = ANSI_CHARSET;
  strcpy(lf.lfFaceName, "Arial");
  CFont fontVer;
  fontVer.CreateFontIndirect(&lf);
  CFont *fontOld = dc.SelectObject(&fontVer);
  dc.SetTextColor(RGB(0,0,0));
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextAlign(TA_BOTTOM|TA_RIGHT);
  dc.TextOut(bm.bmWidth-25, bm.bmHeight-28,"Version 1.0");
  dc.SelectObject(&fontOld);
  // Restore initial palette
  if (hOldPalDC)
    SelectPalette(dc, hOldPalDC, FALSE);
  if (hOldPalDCImage)
    SelectPalette(dcImage, hOldPalDCImage, FALSE);
}

void 
CSplashWnd::OnTimer(UINT nIDEvent)
{
  // Destroy the splash screen window.
  HideSplashScreen();
}
