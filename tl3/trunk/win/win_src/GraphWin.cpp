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
 
// GraphWin.cpp : implementation file
//

#include "stdafx.h"
#include "wintl3.h"
#include "TL3Com.h"
#include "GraphFrm.h"
#include "GraphWin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Borrowed from graphics.h
#define HILITE_NONE       (0)
#define HILITE_INVERT     (1)
#define HILITE_VECTOR     (2)
#define HILITE_RECT       (3)



/////////////////////////////////////////////////////////////////////////////
// CGraphDoc static members



// CGraphDoc::s_nPaletteCount
// -- number of windows sharing the palette.

int CGraphDoc::s_nPaletteCount = 0;


// CGraphDoc::s_hPalette
// -- shared palette used for 256 color displays.

HPALETTE CGraphDoc::s_hPalette = NULL;


// CGraphDoc::s_logPalette
// -- logpalette structure for managing the palette entries.
//    warning: Version field contains the maximal number of entries.

LOGPALETTE *CGraphDoc::s_logPalette = NULL;


// CGraphDoc::s_nSysEntries
// -- number of system entries in current logical palette

int CGraphDoc::s_nSysEntries = 0;



// CGraphDoc::s_csOffMap
// -- critical section object for synchronizing access
//    to graphical stuff.

CCriticalSection CGraphDoc::s_csOffMap;


/////////////////////////////////////////////////////////////////////////////
// CGraphDoc

IMPLEMENT_DYNCREATE(CGraphDoc, CDocument)


// CGraphDoc::szTitle
// -- name of the future window
//    this is a hidden argument for the GraphDoc ctor

const char *CGraphDoc::s_szTitle = "TL Graphics";


// CGraphDoc::CGraphDoc
// -- Constructor initialize all fields

CGraphDoc::CGraphDoc()
{
  // Initialize 
  m_pGraphView = NULL;
  m_bReady = FALSE;
  m_hbmOld = NULL;
  m_hbmOffMap = NULL;
  m_hPalette = NULL;
  m_szTitle = s_szTitle;
}



// CGraphDoc::~CGraphDoc
// -- Destructor for graph document window

CGraphDoc::~CGraphDoc()
{
  // Select bitmap out of memory dc
  if (m_bReady)
  {
    if (m_hPalette && m_hbmOffMap)
      SelectPalette(m_dcOffMap, (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE);
    if (m_hPalette)
      UnlockPalette(m_hPalette);
    m_hPalette = NULL;
    if (m_hbmOld)
      SelectObject(m_dcOffMap, m_hbmOld);
    m_hbmOld = NULL;
    if (m_hbmOffMap)
      DeleteObject(m_hbmOffMap);
    m_hbmOffMap = NULL;
  }
}



BEGIN_MESSAGE_MAP(CGraphDoc, CDocument)
	//{{AFX_MSG_MAP(CGraphDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSaveAs)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CGraphDoc diagnostics

#ifdef _DEBUG
void 
CGraphDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void 
CGraphDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// CGraphDoc serialization and clipboard



// CGraphDoc::OnFileSaveAs
// -- save session transcript
//    linked to commands ID_FILE_SAVE and ID_FILE_SAVE_AS

void 
CGraphDoc::OnFileSaveAs() 
{
  CString strFilter;
  VERIFY(strFilter.LoadString(IDS_SAVE_GRAPH_FILTER));
  CString strTitle;
  VERIFY(strTitle.LoadString(IDS_SAVE_GRAPH_TITLE));
  DWORD dwFlags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  CFileDialog dlgFile(FALSE, "bmp", "graphics.bmp", dwFlags, strFilter);
  dlgFile.m_ofn.lpstrTitle = strTitle;
  // perform modal interaction
  theCom.SuspendTLisp();
  int nResult = dlgFile.DoModal(); 
  theCom.ResumeTLisp();
  // save document
  if (nResult == IDOK)
  {
    CString strName = dlgFile.GetPathName();
    if (!OnSaveDocument(strName))
    {
      try 
      {
	CFile::Remove(strName);
      } 
      catch (CException *e) 
      {
	TRACE0("Warning: failed to delete file after failed SaveAs.\n");
	e->Delete();
      }
    }
  }
}




// CreateDIB
// -- create DIB components from a bitmap and dc handle

static PBITMAPINFO 
CreateDIBFromBitmap(HBITMAP hbmp, HDC hdc, PBITMAPFILEHEADER phdr, int nBitCount=0) 
{ 
    BITMAP bmp; 
    BITMAPINFOHEADER bmih;
    PBITMAPINFO pbmi; 
    LPBYTE lpBits;
    DWORD dwSize;
    // Retrieve the bitmap's color format, width, and height.
    if (!GetObject(hbmp, sizeof(BITMAP), (LPSTR)&bmp)) 
      return NULL; 
    if (bmp.bmPlanes != 1)
      return NULL;

    // Choose DIB format    
    memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    if (nBitCount==1 || nBitCount==4 || nBitCount==8 ||
        nBitCount==16 || nBitCount==24 || nBitCount==32 )
    {
      // User specified DIB format
      bmih.biWidth = bmp.bmWidth;
      bmih.biHeight = bmp.bmHeight;
      bmih.biPlanes = 1;
      bmih.biBitCount = nBitCount;
      bmih.biCompression = BI_RGB;
    }
    else
    {
      // Default DIB format
      if (!GetDIBits(hdc, hbmp, 0, bmp.bmHeight,
		     NULL, (PBITMAPINFO)&bmih, DIB_RGB_COLORS)) 
	return NULL;
      nBitCount = bmih.biBitCount;
    }
    // Fill biSizeImage field unless already done.
    if (bmih.biSizeImage == 0)
        bmih.biSizeImage = (((bmih.biWidth*nBitCount+31)>>5)*4) * bmih.biHeight;
    // Compute colormap size (ugh!)
    if (bmih.biBitCount < 16) 
      bmih.biClrUsed = (1<<bmih.biBitCount);
    else if (bmih.biCompression == BI_BITFIELDS)
      bmih.biClrUsed = 3;
    else
      bmih.biClrUsed = 0;
    // Allocate memory for the BITMAPINFO structure and the DIB BITS.
    dwSize = bmih.biSize + sizeof(RGBQUAD)*bmih.biClrUsed + bmih.biSizeImage;
    pbmi = (PBITMAPINFO) malloc(dwSize); 
    if (!pbmi)
      return NULL;
    // Initialize the fields in the BITMAPINFO structure.
    pbmi->bmiHeader = bmih;
    // Retrieve the bits and the colortable
    lpBits = (LPBYTE)pbmi + dwSize - bmih.biSizeImage;
    if (!GetDIBits(hdc, hbmp, 0, bmp.bmHeight, 
		   lpBits, pbmi, DIB_RGB_COLORS)) 
    {
      free(pbmi);
      return NULL;
    } 
    // Fill the bitmap file header if any
    if (phdr)
    {
      phdr->bfType = 0x4d42;  /* 0x42 = "B" 0x4d = "M" */ 
      phdr->bfReserved1 = 0; 
      phdr->bfReserved2 = 0; 
      phdr->bfSize = sizeof(BITMAPFILEHEADER) + dwSize; 
      phdr->bfOffBits = sizeof(BITMAPFILEHEADER) + dwSize - bmih.biSizeImage;
    }

    // Return BITMAPINFO + BITS
    return pbmi;
} 
 
  



// CGraphDoc::Serialize
// -- create a BMP file for window

void 
CGraphDoc::Serialize(CArchive& ar)
{
  if (m_bReady)
  {
    ASSERT(ar.IsStoring());
    ASSERT(m_hbmOld);
    
    BITMAPFILEHEADER hdr;
    PBITMAPINFO pbmi = NULL;
    HBITMAP bmSaved = NULL;
    CFile *pFile = ar.GetFile();

    try
    {
      CSingleLock lock(&s_csOffMap);
      lock.Lock();
      // Deselect bitmap
      bmSaved = (HBITMAP)SelectObject(m_dcOffMap, m_hbmOld);
      ASSERT(bmSaved == m_hbmOffMap);
      // Create memory image of the file
      pbmi = CreateDIBFromBitmap(m_hbmOffMap, m_dcOffMap, &hdr);
      if (!pbmi)
      {
	AfxMessageBox(IDP_CANNOT_CREATE_DIB, MB_OK|MB_ICONSTOP);
	AfxThrowUserException();
      }
      // Save memory image of the file
      pFile->Write(&hdr, sizeof(BITMAPFILEHEADER));
      pFile->Write(pbmi, hdr.bfSize - sizeof(BITMAPFILEHEADER));
      // Free memory and reselect bitmap
      free(pbmi);
      pbmi = NULL;
      if (bmSaved)
	SelectObject(m_dcOffMap, bmSaved);
      bmSaved = NULL;
    }
    catch(CException *exception)
    {
      if (bmSaved)
	SelectObject(m_dcOffMap, bmSaved);
      if (pbmi)
	free(pbmi);
      throw exception;
    }
  }
}




// CGraphDoc::OnEditCopy
// -- copy the image into the clipboard
//    as a graphics file.

void 
CGraphDoc::OnEditCopy() 
{
  BITMAPFILEHEADER hdr;
  PBITMAPINFO pbmi = NULL;
  DWORD dwSize;
  // CREATE DIB SEGMENT
  if (m_bReady)
  {
    CSingleLock lock(&s_csOffMap);
    lock.Lock();
    // Deselect bitmap
    HBITMAP bmSaved = (HBITMAP)SelectObject(m_dcOffMap, m_hbmOld);
    ASSERT(bmSaved == m_hbmOffMap);
    // Create memory image of the file
    pbmi = CreateDIBFromBitmap(m_hbmOffMap, m_dcOffMap, &hdr);
    dwSize = hdr.bfSize - sizeof(BITMAPFILEHEADER);
    // Reselect bitmap
    if (bmSaved)
      SelectObject(m_dcOffMap, bmSaved);
  }
  if (!pbmi)
  {
    AfxMessageBox(IDP_CANNOT_CREATE_DIB, MB_OK|MB_ICONSTOP);
    AfxThrowUserException();
  }
  // TRANSFER DIB INTO CLIPBOARD
  if (m_pGraphView->OpenClipboard())
  {
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwSize);
    if (hmem)
    {
      LPSTR pmem = (LPSTR)GlobalLock(hmem);
      memcpy(pmem, pbmi, dwSize);
      GlobalUnlock(hmem);
      EmptyClipboard();
      SetClipboardData(CF_DIB, hmem);
    }
    CloseClipboard();
  }
  // FREE DIB
  free(pbmi);
}




/////////////////////////////////////////////////////////////////////////////
// Palette support



// CGraphDoc::LockPalette
// -- returns shared palette if useful

HPALETTE 
CGraphDoc::LockPalette(HDC hdc)
{
  // Check display capabilities
  if (! (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    return NULL;
  if (GetDeviceCaps(hdc, BITSPIXEL) != 8)
    return NULL;
  if (GetDeviceCaps(hdc, PLANES) != 1)
    return NULL;
  // Get HPALETTE
  if (! s_hPalette)
  {
    // Create LOGPALETTE 
    if (! s_logPalette)
    {
      // Compute optimal palette size
      int nMaxEntries = GetDeviceCaps(hdc, SIZEPALETTE);
      int nSysEntries = GetDeviceCaps(hdc, NUMCOLORS);
      if (nMaxEntries != 256)
	return NULL;
      int nSizePalette = sizeof(LOGPALETTE) + nMaxEntries * sizeof(PALETTEENTRY);
      s_logPalette = (LOGPALETTE*)malloc(nSizePalette);
      if (! s_logPalette)
	return NULL;
      memset(s_logPalette, 0, nSizePalette);
      HPALETTE hpalDefault = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
      GetPaletteEntries(hpalDefault, 0, nSysEntries, s_logPalette->palPalEntry);
      for (int i=0; i<nSysEntries; i++)
	s_logPalette->palPalEntry[i].peFlags = 0;
      s_logPalette->palNumEntries = s_nSysEntries = nSysEntries;
      s_logPalette->palVersion = nMaxEntries;
    }
    // Create palette
    int nSavMaxEntries = s_logPalette->palVersion;
    s_logPalette->palVersion = 0x300;
    s_hPalette = CreatePalette(s_logPalette);
    s_logPalette->palVersion = nSavMaxEntries;
  }
  if (!s_hPalette)
    return NULL;
  s_nPaletteCount += 1;
  return s_hPalette;
}


// CGraphDoc::UnlockPalette
// -- count palette allocation and deallocate if necessary

void 
CGraphDoc::UnlockPalette(HPALETTE hpal)
{
  if (hpal && hpal==s_hPalette)
  {
    s_nPaletteCount -= 1;
    if (!s_nPaletteCount)
    {
      // Forget newly allocated colors besides system colors
      s_logPalette->palNumEntries = s_nSysEntries;
      // Delete Windows palette
      DeleteObject(s_hPalette);
      s_hPalette = NULL;
    }
  }
}



// CGraphDoc::MakeDIBSection
// -- create DIB section with 8 bit colors

HBITMAP 
CGraphDoc::MakeDIBSection(HDC hdc, int nWidth, int nHeight)
{
  int nSize = sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD);
  LPBITMAPINFO pbmi = (LPBITMAPINFO)malloc(nSize);
  HBITMAP hbm = NULL;
  if (pbmi)
  {
    ASSERT(s_logPalette);
    int nSysEntries = s_logPalette->palNumEntries;
    PALETTEENTRY *pe = s_logPalette->palPalEntry;
    RGBQUAD *lp = pbmi->bmiColors;
    memset(pbmi, 0, nSize);
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = nWidth;
    pbmi->bmiHeader.biHeight = nHeight;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biClrUsed = nSysEntries;
    for (int i=0; i<s_logPalette->palNumEntries; i++, lp++, pe++)
    {
      lp->rgbRed = pe->peRed;
      lp->rgbGreen = pe->peGreen;
      lp->rgbBlue = pe->peBlue;
    }
    void *lpBits;
    hbm = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, &lpBits, NULL, 0);
    free(pbmi);
  }
  return hbm;
}





/////////////////////////////////////////////////////////////////////////////
// CGraphDoc overrides and commands



// CGraphDoc::SetTitle
// -- do not allow MFC to change the title of this window.

void 
CGraphDoc::SetTitle(LPCTSTR lpszTitle) 
{
  CDocument::SetTitle(m_szTitle ? m_szTitle : lpszTitle);
}



// CGraphDoc::SetModifiedFlag
// -- override the standard modified flag
//    graph window never count as modified documents

void 
CGraphDoc::SetModifiedFlag(BOOL bModified)
{
    CDocument::SetModifiedFlag(FALSE);
}



// CGraphDoc::CreateOffMap
// -- create offscreen bitmap for a given size
//    flag bReady is TRUE on success

void
CGraphDoc::CreateOffMap(int nWidth, int nHeight)
{
  ASSERT(!m_bReady);
  m_nWidth = nWidth;
  m_nHeight = nHeight;
  CGraphView *pGView = m_pGraphView;
  ASSERT(pGView);
  CWindowDC dcView(pGView);
  if (m_dcOffMap.CreateCompatibleDC(&dcView))
  {
    nWidth = (nWidth | 0x7) + 1;
    nHeight = (nHeight | 0x7) + 1;
    // 256 color case
    m_hPalette = LockPalette(dcView);
    if (m_hPalette)
    {
      m_hbmOffMap = MakeDIBSection(dcView, nWidth, nHeight);
      if (m_hbmOffMap)
      {
        TRACE2("Creating DIB8 bitmap %d x %d\n", nWidth, nHeight);
        m_hbmOld = (HBITMAP)SelectObject(m_dcOffMap, m_hbmOffMap);
      }
      else
      {
	UnlockPalette(m_hPalette);
	m_hPalette = NULL;
      }
    }
    // Fallback
    if (!m_hbmOffMap)
    {
      m_hbmOffMap = CreateCompatibleBitmap(dcView, nWidth, nHeight);
      TRACE2("Creating compatible bitmap %d x %d\n", nWidth, nHeight);
      if (m_hbmOffMap)
      {
        m_hbmOld = (HBITMAP)SelectObject(m_dcOffMap, m_hbmOffMap);
      }
    }
    // Initialize
    if (m_hbmOffMap)
    {
      m_dcOffMap.FillSolidRect(0,0,nWidth,nHeight,RGB(255,255,255));
      m_bReady = TRUE;
    }
  }
}



// CGraphDoc::ResizeOffMap
// -- Resize offscreen bitmap for a given size
//    returns TRUE on success

BOOL
CGraphDoc::ResizeOffMap(int nWidth, int nHeight)
{
  if (m_bReady)
  {
    ASSERT(m_hbmOffMap);
    // Test if we need to create new bitmap
    if ((m_nWidth | 0x7) == (nWidth | 0x7))
      if ((m_nHeight | 0x7) == (nHeight | 0x7))
      {
        m_nWidth = nWidth;
	m_nHeight = nHeight;
	return TRUE;
      }
    // Update size
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    // Create new bitmap
    nWidth = (nWidth | 0x7) + 1;
    nHeight = (nHeight | 0x7) + 1;
    HBITMAP hbmNew = CreateCompatibleBitmap((HDC)m_dcOffMap, nWidth, nHeight);
    if (hbmNew)
    {
      CDC dcNew;
      CSingleLock lock(&s_csOffMap);
      lock.Lock();
      // Copy old bitmap bits into new bitmap
      if (dcNew.CreateCompatibleDC(&m_dcOffMap))
      {
	HBITMAP hbmTmp = (HBITMAP)SelectObject(dcNew, hbmNew);
        dcNew.FillSolidRect(0,0,nWidth,nHeight,RGB(255,255,255));
        dcNew.BitBlt(0,0,nWidth,nHeight,&m_dcOffMap,0,0,SRCCOPY);
        dcNew.SelectObject(hbmTmp);
	// Install new bitmap and delete old bitmap
        SelectObject(m_dcOffMap, hbmNew);
        DeleteObject(m_hbmOffMap);
        m_hbmOffMap = hbmNew;
	TRACE2("Bitmap resized to %d x %d\n", nWidth, nHeight);
        return TRUE;
      }
    }
  }
  return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// CGraphView


IMPLEMENT_DYNCREATE(CGraphView, CView)

CGraphView::CGraphView()
{
  m_bSizeChanged = FALSE;
  m_bButtonDown = FALSE;
  m_nModeHilite = HILITE_NONE;
  m_nMouseX = m_nMouseY = 0;
  // static cursors
  if (! s_hWaitCursor)
    s_hWaitCursor = LoadCursor(NULL, IDC_WAIT);
  if (! s_hArrowCursor)
    s_hArrowCursor = LoadCursor(NULL, IDC_ARROW);
}

CGraphView::~CGraphView()
{
}

BEGIN_MESSAGE_MAP(CGraphView, CView)
	//{{AFX_MSG_MAP(CGraphView)
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEW, OnUpdateUIDisable)
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	//}}AFX_MSG_MAP
	// Indicators
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateUIDisable)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LC, OnUpdateUIDisable)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CGraphView statics

HCURSOR CGraphView::s_hWaitCursor = NULL;
HCURSOR CGraphView::s_hArrowCursor = NULL;



/////////////////////////////////////////////////////////////////////////////
// CGraphView diagnostics

#ifdef _DEBUG
void CGraphView::AssertValid() const
{
	CView::AssertValid();
}

void CGraphView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG





/////////////////////////////////////////////////////////////////////////////
// CGraphView drawing


// CGraphView::OnInitialUpdate
// -- register view for this document
//    create offmap in document

void CGraphView::OnInitialUpdate() 
{
  // register view
  ASSERT(m_pDocument);
  ASSERT(GetGraphDoc()->m_pGraphView == 0);
  GetGraphDoc()->m_pGraphView = this;
  // create offscreen bitmap
  CRect rectClient;
  GetClientRect(rectClient);
  GetGraphDoc()->CreateOffMap(rectClient.Width(), rectClient.Height());
  // inherited
  CView::OnInitialUpdate();
}



// CGraphView::Hilite
// -- draw the hilited area

void 
CGraphView::Hilite(UINT mode, int x1, int y1, int x2, int y2)
{
  // Clear previous hilited area
  if (m_nModeHilite != HILITE_NONE)
  {
    m_rectHilite.InflateRect(2,2);
    Update(m_rectHilite);
    m_nModeHilite = HILITE_NONE;
  }
  // Draw new hilited area
  if (m_bButtonDown && mode!=HILITE_NONE)
  {
    m_nModeHilite = mode;
    m_rectHilite.top = min(y1, y2);
    m_rectHilite.bottom = max(y1, y2);
    m_rectHilite.left = min(x1, x2);
    m_rectHilite.right = max(x1, x2);
    CClientDC dc(this);
    CPen pen(PS_DOT,0,RGB(0,0,0));
    CPen *pPenOld = dc.SelectObject(&pen);
    CBrush *pBrushOld = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
    switch(mode)
    {
      case HILITE_INVERT:
        dc.InvertRect(m_rectHilite);
        break;
      case HILITE_VECTOR:
	dc.MoveTo(x1, y1);
	dc.LineTo(x2, y2);
	break;
      case HILITE_RECT:
	m_rectHilite.right += 1;
	m_rectHilite.bottom += 1;
	dc.Rectangle(m_rectHilite);
	break;
    }
    dc.SelectObject(pPenOld);
    dc.SelectObject(pBrushOld);
  }
}




// CGraphView::OnDraw
// -- copies the offscreen bitmap onto the screen

void 
CGraphView::OnDraw(CDC* pDC)
{
  CGraphDoc *pDoc = GetGraphDoc();
  CSingleLock lock(&pDoc->s_csOffMap);
  lock.Lock();
  // select palette
  HPALETTE hPalOld = NULL;
  if (pDoc->m_hPalette)
  {
    hPalOld = SelectPalette(pDC->m_hDC, pDoc->m_hPalette, FALSE);
    RealizePalette(pDC->m_hDC);
  }
  // Bitblt the offscreen bitmap
  CRect rectClip;
  pDC->GetClipBox(&rectClip);
  pDC->BitBlt(rectClip.left, rectClip.top, rectClip.Width(), rectClip.Height(),
	      &pDoc->m_dcOffMap, rectClip.left, rectClip.top, SRCCOPY);
  // Select palette out
  if (hPalOld)
    SelectPalette(pDC->m_hDC, hPalOld, FALSE);
}



// CGraphView::Update
// -- update window

void CGraphView::Update(CRect &rect) 
{
  CGraphDoc *pDoc = GetGraphDoc();
  CSingleLock lock(&pDoc->s_csOffMap);
  lock.Lock();
  // Select palette
  CClientDC dcWin(this);
  HPALETTE hPalOld = NULL;
  if (pDoc->m_hPalette)
  {
    hPalOld = SelectPalette(dcWin, pDoc->m_hPalette, FALSE);
    RealizePalette(dcWin);
  }
#if DEBUG_UPDATE
  // Debug code for displqying updated area
  dcWin.FillSolidRect(&rect, RGB(0,255,0));
  Sleep(200);
#endif
  // Bitblt the offscreen bitmap
  dcWin.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(),
	       &pDoc->m_dcOffMap, rect.left, rect.top, SRCCOPY );
  // Select palette out
  if (hPalOld)
    hPalOld = SelectPalette(dcWin, hPalOld, FALSE);
}




/////////////////////////////////////////////////////////////////////////////
// CGraphView operations



// CGraphView::EventAdd()
// -- Post an event to win_driver.c
	
void 
CGraphView::EventAdd(int code, int arg, int x, int y)
{
  theCom.EventAdd(GetGraphDoc(), code, arg, x, y);
}



// CGraphView::SetStatusCursor
// -- this function changes the cursor 
//    you should call it when this view is active

void 
CGraphView::SetStatusCursor()
{
  if (s_hWaitCursor && s_hArrowCursor) 
  {
    if (m_bButtonDown || theCom.m_bThreadWaiting)
      SetCursor(s_hArrowCursor);
    else
      SetCursor(s_hWaitCursor);
  }
}




/////////////////////////////////////////////////////////////////////////////
// CGraphView printing




// CGraphView::OnFilePrint
// -- call printing routine
//    restore directory that may be destroyed by print file dialog

void CGraphView::OnFilePrint() 
{
  // Save directory
  char buffer[256];
  DWORD dwResult;
  theCom.SuspendTLisp();
  dwResult = GetCurrentDirectory(sizeof(buffer), buffer);
  // Call inherited
  CView::OnFilePrint();
  // Restore directory
  if (dwResult>0 && dwResult<sizeof(buffer))
    SetCurrentDirectory(buffer);
  theCom.ResumeTLisp();
}



// CGraphView::OnPreparePrinting
// -- enable printing

BOOL 
CGraphView::OnPreparePrinting(CPrintInfo* pInfo) 
{
  pInfo->SetMaxPage(1);
  return DoPreparePrinting(pInfo);
}




// CGraphView::OnPrepareDC
// -- sets DC mode for regular display or for printing

void 
CGraphView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
  if (pInfo)
  {
    CGraphDoc *pDoc = GetGraphDoc();
    // test printer capabilities
    if (!(pDC->GetDeviceCaps(RASTERCAPS) & RC_STRETCHDIB)
	|| !(pDC->GetDeviceCaps(RASTERCAPS) & RC_DIBTODEV) )
    {
      AfxMessageBox(IDP_CANNOT_PRINT, MB_OK|MB_ICONSTOP);
      AfxThrowUserException();
    }
    // compute target rectangle
    int nHRes = pDC->GetDeviceCaps(LOGPIXELSX);
    int nVRes = pDC->GetDeviceCaps(LOGPIXELSY);
    int nHSize = pDC->GetDeviceCaps(HORZRES);
    int nVSize = pDC->GetDeviceCaps(VERTRES);
    int nHMargin = min(nHSize/4,nHRes/2);
    int nVMargin = min(nVSize/4,nVRes/2);
    nHSize -= 2*nHMargin;
    nVSize -= 2*nVMargin;
    CDisplayIC cdic;
    int nHScreenRes = cdic.GetDeviceCaps(LOGPIXELSX);
    int nVScreenRes = cdic.GetDeviceCaps(LOGPIXELSY);
    int nHImgSize = MulDiv(pDoc->m_nWidth, nHRes, nHScreenRes);
    int nVImgSize = MulDiv(pDoc->m_nHeight, nVRes, nVScreenRes);
    if (nHImgSize > nHSize)
    {
      nVImgSize = MulDiv(nVImgSize, nHSize, nHImgSize);
      nHImgSize = MulDiv(nHImgSize, nHSize, nHImgSize);
    }
    if (nVImgSize > nVSize)
    {
      nHImgSize = MulDiv(nHImgSize, nVSize, nVImgSize);
      nVImgSize = MulDiv(nVImgSize, nVSize, nVImgSize);
    }
    // prepare mapping mode
    pDC->SetMapMode(MM_ANISOTROPIC);
    pDC->SetWindowExt(pDoc->m_nWidth, pDoc->m_nHeight);
    pDC->SetViewportOrg(nHMargin, nVMargin);
    pDC->SetViewportExt(nHImgSize, nVImgSize);
  }
  else
  {
    // regular display
    pDC->SetMapMode(MM_TEXT);
  }
  // inherited
  CView::OnPrepareDC(pDC, pInfo);
}



// CGraphView::OnPrint
// -- actual printing code

void 
CGraphView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
  CGraphDoc *pDoc = GetGraphDoc();

  // CREATE DIB
  BITMAPFILEHEADER hdr;
  PBITMAPINFO pbmi = NULL;
  // Use printer compatible DIB resolution (helps a lot)
  int nBitCount = 0;
  if (pDC->GetDeviceCaps(PLANES)==1)
    nBitCount = pDC->GetDeviceCaps(BITSPIXEL);
  if (pDoc->m_bReady)
  {
    // Lock offmap while deselecting bitmap
    CSingleLock lock(&pDoc->s_csOffMap);
    lock.Lock();
    HBITMAP bmSaved = (HBITMAP)SelectObject(pDoc->m_dcOffMap, pDoc->m_hbmOld);
    ASSERT(bmSaved == pDoc->m_hbmOffMap);
    pbmi = CreateDIBFromBitmap(pDoc->m_hbmOffMap, pDoc->m_dcOffMap, &hdr, nBitCount);
    if (bmSaved)
      SelectObject(pDoc->m_dcOffMap, bmSaved);
  }
  if (!pbmi)
  {
    AfxMessageBox(IDP_CANNOT_CREATE_DIB, MB_OK|MB_ICONSTOP);
    AfxThrowUserException();
  }
  // SEND DIB TO DEVICE
  int cx = pDoc->m_nWidth;
  int cy = pDoc->m_nHeight;
  DWORD dwSize = hdr.bfSize - sizeof(BITMAPFILEHEADER);
  LPBYTE pBits = (LPBYTE)pbmi + dwSize - pbmi->bmiHeader.biSizeImage;
  SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
  StretchDIBits(pDC->m_hDC, 0, 0, cx, cy,
		0, pbmi->bmiHeader.biHeight - cy, cx, cy,
		pBits, pbmi, DIB_RGB_COLORS, SRCCOPY );
  // DISPLAY FRAME
  HBRUSH hbr = (HBRUSH)SelectObject(pDC->m_hDC, GetStockObject(NULL_BRUSH));
  Rectangle(pDC->m_hDC, 0, 0, cx, cy);
  SelectObject(pDC->m_hDC, hbr);
  // FREE DIB MEMORY
  free(pbmi);
}







/////////////////////////////////////////////////////////////////////////////
// CGraphView message handlers


// CGraphView::OnUpdateUIDisable
//  -- disable certain operations

void 
CGraphView::OnUpdateUIDisable(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(FALSE);
}



// CGraphView::OnSize
// -- signal that needs to be restored

void 
CGraphView::OnSize(UINT nType, int cx, int cy) 
{
  CGraphDoc *pGDoc = GetGraphDoc();
  if (nType==SIZE_MAXIMIZED || nType==SIZE_RESTORED)
    if (cx != pGDoc->m_nWidth || cy != pGDoc->m_nHeight)
      m_bSizeChanged = TRUE;
  // Inherited
  CView::OnSize(nType, cx, cy);
}




// CGraphView::AdjustSize
// -- adjust bitmap if size is changed

void
CGraphView::AdjustSize()
{
  if (m_bSizeChanged)
  {
    CRect rect;
    GetClientRect(rect);
    GetGraphDoc()->ResizeOffMap(rect.Width(), rect.Height());
    m_bSizeChanged = FALSE;
    // Post event to win_driver
    EventAdd(WM_SIZE, 0, rect.Width(), rect.Height());
  }
}


// CGraphView::OnPaint
// -- checks if size is changed and then perform regular painting.

void 
CGraphView::OnPaint() 
{
  AdjustSize();
  CView::OnPaint();
}



// CGraphView::OnSetCursor
// -- sets wait cursor when useful

BOOL 
CGraphView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
  if (s_hWaitCursor && s_hArrowCursor && nHitTest==HTCLIENT) 
  {
    SetStatusCursor();
    return TRUE;
  }
  return CView::OnSetCursor(pWnd, nHitTest, message);
}



// CGraphView::OnQueryNewPalette
// -- install palette when useful

BOOL 
CGraphView::OnQueryNewPalette() 
{
  if (CGraphDoc::s_hPalette)
  {
    CGraphDoc *pGDoc = GetGraphDoc();
    if (pGDoc->m_hPalette)
    {
      UnrealizeObject(pGDoc->m_hPalette);
      Invalidate(FALSE);
      UpdateWindow();
      return TRUE;
    }
  }
  return FALSE;
}



// CGraphView::OnPaletteChanged
// -- perform degraded palette assignment on window

void 
CGraphView::OnPaletteChanged(CWnd* pFocusWnd) 
{
  if (CGraphDoc::s_hPalette)
  {
    // Loop avoidance
    if (pFocusWnd == this)
      return;
    // Optimization (single palette for all windows)
    if (pFocusWnd->IsKindOf(RUNTIME_CLASS(CGraphView)))
      return;
    // Redraw (and re-realize palette)
    CGraphDoc *pGDoc = GetGraphDoc();
    if (pGDoc->m_hPalette)
    {
      CClientDC dc(this);
      HPALETTE oldPal = SelectPalette(dc, pGDoc->m_hPalette, FALSE);
      dc.RealizePalette();
      dc.UpdateColors();
      SelectPalette(dc, oldPal, FALSE);
    }
  }
}






/////////////////////////////////////////////////////////////////////////////
// CGraphView message forwarded to TLisp



// CGraphView::OnButtonDown
// -- called when user presses the left button

void 
CGraphView::OnButtonDown(UINT nFlags, CPoint point) 
{
  int arg = 0;
  if (::GetKeyState(VK_CONTROL) < 0)
    arg |= 0x20000000;
  if (::GetKeyState(VK_SHIFT) < 0)
    arg |= 0x10000000;
  // Record mouse position
  m_nMouseX = point.x;
  m_nMouseY = point.y;
  // Set feedback
  if (!theCom.m_bThreadWaiting)
    if (s_hWaitCursor && s_hArrowCursor)
      SetCursor(s_hArrowCursor);
  // Button is now down
  m_bButtonDown = TRUE;
  SetCapture();
  // Post event to win_driver
  EventAdd(GetCurrentMessage()->message, arg, point.x, point.y);
}

void 
CGraphView::OnLButtonDown(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

void 
CGraphView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

void 
CGraphView::OnMButtonDown(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

void 
CGraphView::OnMButtonDblClk(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

void 
CGraphView::OnRButtonDown(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

void 
CGraphView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
  OnButtonDown(nFlags, point);
}

// CGraphView::OnButtonUp
// -- called when user releases the left button

void 
CGraphView::OnButtonUp(UINT nFlags, CPoint point) 
{
  // Record mouse position
  m_nMouseX = point.x;
  m_nMouseY = point.y;
  // Remove feedback
  if (!theCom.m_bThreadWaiting)
    if (s_hWaitCursor && s_hArrowCursor)
      SetCursor(s_hWaitCursor);
  if (m_nModeHilite)
    Hilite(HILITE_NONE);
  // Button is no longer down
  m_bButtonDown = FALSE;
  ReleaseCapture();
  // Post event to win_driver
  EventAdd(GetCurrentMessage()->message, 0, point.x, point.y);
}

void 
CGraphView::OnLButtonUp(UINT nFlags, CPoint point) 
{
  OnButtonUp(nFlags, point);
}

void 
CGraphView::OnMButtonUp(UINT nFlags, CPoint point) 
{
  OnButtonUp(nFlags, point);
}

void 
CGraphView::OnRButtonUp(UINT nFlags, CPoint point) 
{
  OnButtonUp(nFlags, point);
}


// CGraphView::OnMouseMove
// -- called when user moves the mouse

void 
CGraphView::OnMouseMove(UINT nFlags, CPoint point) 
{
  // Record mouse position
  m_nMouseX = point.x;
  m_nMouseY = point.y;
  // Post event to win_driver
  if (m_bButtonDown)
    EventAdd(WM_MOUSEMOVE, 0, point.x, point.y);
}




// CGraphView::OnKeyDown
// -- called when user presses a key

void 
CGraphView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  static UINT vk_a = VkKeyScan('a');
  static UINT vk_z = VkKeyScan('z');
  BOOL bAlt = ::GetKeyState(VK_MENU) < 0;
  BOOL bControl = ::GetKeyState(VK_CONTROL) < 0;
  BOOL bShift = ::GetKeyState(VK_SHIFT) < 0;
  if (nChar>=vk_a && nChar<=vk_z && bControl && bShift && !bAlt)
  {
    // Handle control+shift+stuff
    EventAdd(WM_CHAR, nChar - vk_a + 1, 0, 0);
  }
  else if (!bAlt)
  {
    if (bControl)
      nChar |= 0x20000000;
    if (bShift)
      nChar |= 0x10000000;
    // Post event to win_driver
    while (nRepCnt-- > 0)
      EventAdd(WM_KEYDOWN, nChar, m_nMouseX, m_nMouseY);
  }
  else
  {
    // Inherited
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
  }
}



// CGraphView::OnChar
// -- called when user presses an ASCII key

void 
CGraphView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // Post event to win_driver
  while (nRepCnt-- > 0)
    EventAdd(WM_CHAR, nChar, m_nMouseX, m_nMouseY);
}


