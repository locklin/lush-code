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
 
// TextDoc.cpp : implementation of the CTextDoc class
//


#include "stdafx.h"
#include "WinTL3.h"
#include "TextDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CTextUpdateHint

IMPLEMENT_DYNAMIC(CTextUpdateHint, CObject)


// CTextUpdateHint::CTextUpdateHint
// -- initializes update hint

CTextUpdateHint::CTextUpdateHint(CTextBuffer *pBuffer)
{
  m_pBuffer = pBuffer;
  m_bLineHint = FALSE;
  m_bUpdateSelection = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CTextBuffer


 
// MAXLINELENGTH
// -- length of longest visible line
//    we will refuse to scroll horizontally after that many characters

#define MAXLINELENGTH 1024



// CTextBuffer::CTextBuffer
// -- initializes text buffer 

CTextBuffer::CTextBuffer()
{ 
  m_pDoc = 0;
  m_Lines.SetSize(1,4096);
  m_Flags.SetSize(1,4096);
  m_nMaxLineLength = 0;
  m_bUndoOk = FALSE;
}

CTextBuffer::~CTextBuffer()
{
}



// CTextBuffer::SetDocument
// -- sets document used for calling functions
//    UpdateAllViews and SetModifiedFlag when buffer is modified


void CTextBuffer::SetDocument(CDocument *pDoc)
{
  m_pDoc = pDoc;
}



// CTextBuffer::Serialize
// -- save and load text from/into text buffer
//    handle incomplete newline (CR or LF instead of CRLF)

void CTextBuffer::Serialize(CArchive& ar)
{
  CFile& cf = *(ar.GetFile());
  if (ar.IsStoring())
  {
    for (int i=0; i<GetDocLines(); i++)
    {
      CString &cstr = m_Lines[i];
      cf.Write((const char*)cstr, cstr.GetLength());
      if (i < m_Lines.GetUpperBound())
        cf.Write("\r\n", 2);
    }
    cf.Flush();
  }
  else
  {
    long nFileLength = cf.GetLength();
    char cBuffer[4096];
    int nBufSize;
    CString cstr;
    BOOL bNewLine = TRUE;
    BOOL bMaybeCRLF = FALSE;
    int i,j;

    m_Lines.RemoveAll();
    ClearUndoBuffer();
    while (nFileLength > 0)
    {
      nBufSize = cf.Read(cBuffer, sizeof(cBuffer));
      if (nBufSize==0)
        AfxThrowFileException(CFileException::endOfFile);
      nFileLength -= nBufSize;
      for (i=j=0; i<nBufSize; i++)
      {
        if (bNewLine) 
        {
  	  j = i;
  	  bNewLine = FALSE;
	  if (bMaybeCRLF && cBuffer[i]=='\n')
	    bNewLine = TRUE;
	  bMaybeCRLF = FALSE;
        }
        if (!bNewLine)
        {
	  if (cBuffer[i]=='\r')
	    bMaybeCRLF = bNewLine = TRUE;
	  if (cBuffer[i]=='\n')
	    bNewLine = TRUE;  
	  if (!cBuffer[i])
	  {
	    MessageBeep(MB_ICONEXCLAMATION);
	    AfxMessageBox(IDP_CANNOT_EDIT_BINARY, MB_OK|MB_ICONEXCLAMATION);
	    AfxThrowUserException();
	  }
	  if (bNewLine)
	  {
	    cstr += CString(cBuffer+j, i-j);
	    m_Lines.SetAtGrow(m_Lines.GetSize(), cstr);
	    m_Flags.SetAtGrow(m_Lines.GetSize(), 0);
	    cstr.Empty();
	  }
	}
      }
      if (!bNewLine)
        if (i > j)
	  cstr += CString(cBuffer+j, i-j);
    }
    m_Lines.SetAtGrow(m_Lines.GetSize(), cstr);
    m_Flags.SetAtGrow(m_Lines.GetSize(), 0);
    TRACE1("read %d lines\n",m_Lines.GetSize());
  }
  ComputeMaxLineLength();
}



// CTextBuffer::DeleteContents
// -- delete all text managed by text buffer

void CTextBuffer::DeleteContents() 
{
  m_Lines.SetSize(0);
  m_Lines.SetSize(1);
  m_Flags.SetSize(0);
  m_Flags.SetSize(1);
  m_nMaxLineLength = 0;
  ClearUndoBuffer();
}



// CTextBuffer::CharToPos
// -- return position of character <nChar> of line <nLine>
//    non trivial because of tabulations

int CTextBuffer::CharToPos(int nLine, int nChar)
{
  const CString & str = GetLine(nLine);
  int nMaxChar = str.GetLength();
  int nPos = 0;
  for (int i=0; i<nChar; i++)
  {
    if (i<nMaxChar && str[i]=='\t')
      nPos |= 7;
    nPos += 1;
  }
  return nPos;
}



// CTextBuffer::PosToChar
// -- retrives character index for position <nPos> in line <nLine>
//    non trivial because of tabulations

int CTextBuffer::PosToChar(int nLine, int nPos)
{
  const CString & str = GetLine(nLine);
  int nMaxChar = str.GetLength();
  int nCurPos = 0;
  for (int i=0; i<nMaxChar && nCurPos<nPos; i++)
  {
    if (str[i]=='\t')
      nCurPos |= 7;
    nCurPos += 1;
  }
  if (nCurPos>nPos && i>0 && str[i-1]=='\t')
    i -= 1;
  return i;
}



// CTextBuffer::AdjustLocation
// -- adjusts location <loc> for taking into account the 
//    lines and characters inserted or deleted as reported by <hint>.
//    useful for adjusting selection after modifying buffer.

void CTextBuffer::AdjustLocation(CTextLoc &loc, const CTextUpdateHint* hint)
{
  if (hint->m_tlRef <= loc)
  {
    int diffLine = loc.nLine - hint->m_tlRef.nLine;
    // removed characters
    if (diffLine > hint->m_nLinesRemoved)
      loc.nLine -= hint->m_nLinesRemoved;
    else if (diffLine < hint->m_nLinesRemoved)
      loc = hint->m_tlRef;
    else
    {
      loc.nLine = hint->m_tlRef.nLine;
      if (hint->m_nLinesRemoved > 0)
	loc.nChar += hint->m_tlRef.nChar;
      if (loc.nChar < hint->m_tlRef.nChar + hint->m_nCharsRemoved)
	loc.nChar = hint->m_tlRef.nChar;
      else
	loc.nChar -= hint->m_nCharsRemoved;
    } 
    
    // added characters
    if (loc.nLine > hint->m_tlRef.nLine)
      loc.nLine += hint->m_nLinesAdded;
    else if (hint->m_nLinesAdded == 0)
      loc.nChar += hint->m_nCharsAdded;
    else
    {
      loc.nLine += hint->m_nLinesAdded;
      loc.nChar = loc.nChar - hint->m_tlRef.nChar + hint->m_nCharsAdded;      
    }
  }
}



// CTextBuffer::ComputeMaxLineLength
// -- returns length of longest line
//    useful for setting horizontal scroll bar

int CTextBuffer::ComputeMaxLineLength()
{
  m_nMaxLineLength = 0;
  for (int i=0; i<m_Lines.GetSize(); i++)
    m_nMaxLineLength = max(m_nMaxLineLength, CharToPos(i, m_Lines[i].GetLength()));
  m_nMaxLineLength = min(MAXLINELENGTH, m_nMaxLineLength); // security
  return m_nMaxLineLength;
}   



// CTextBuffer::GetRange
// -- stores into string <str> the text between locations <tlFrom> and <tlTo>
//    new lines are marked as CRLF or LF depending on flag <bCrLf>

void CTextBuffer::GetRange(CString& str, CTextLoc tlFrom, CTextLoc tlTo, BOOL bCrLf)
{
  // Swap range is necessary
  if (tlTo < tlFrom)
  {
    CTextLoc tmp = tlFrom;
    tlFrom = tlTo;
    tlTo = tmp;
  }
  // Compute string size
  int size = 0;
  int nLine;
  for (nLine=tlFrom.nLine; nLine<=tlTo.nLine; nLine++)
  {
    const CString& strLine = GetLine(nLine);
    int nCharFrom = (nLine==tlFrom.nLine ? tlFrom.nChar : 0);
    int nCharTo = (nLine==tlTo.nLine ? tlTo.nChar : strLine.GetLength());
    size += nCharTo - nCharFrom;
    if (nLine<tlTo.nLine)
      size += (bCrLf ? 2 : 1);
  }
  // Empty string
  str.Empty();
  LPSTR buffer = str.GetBuffer(size+1);
  LPSTR buf = buffer;
  // Loop on lines
  for (nLine=tlFrom.nLine; nLine<=tlTo.nLine; nLine++)
  {
    const CString& strLine = GetLine(nLine);
    int nCharFrom = (nLine==tlFrom.nLine ? tlFrom.nChar : 0);
    int nCharTo = (nLine==tlTo.nLine ? tlTo.nChar : strLine.GetLength());
    memcpy(buf, (LPCSTR)strLine + nCharFrom, nCharTo - nCharFrom);
    buf += nCharTo - nCharFrom;
    if (nLine<tlTo.nLine) 
    {
      if (bCrLf)
	*buf++ = '\r';
      *buf++ = '\n';
    }
  }
  *buf = 0;
  ASSERT(buf-buffer == size);
  str.ReleaseBuffer(size);
}


// CTextBuffer::DoRemove
// -- removes text between locations <tlFrom> and <tlTo>
//    store number of lines/chars deleted into <hint>

void CTextBuffer::DoRemove(CTextLoc tlFrom, CTextLoc tlTo, CTextUpdateHint& hint)
{
  BOOL bCheat = FALSE;;
  hint.m_nLinesRemoved = 0;
  hint.m_nCharsRemoved = 0;

  if (tlTo.nLine == tlFrom.nLine)
  {
    // Remove a part of a line
    if (tlTo.nChar > tlFrom.nChar)
    {
      ASSERT(tlTo.nLine < m_Lines.GetSize());
      CString& str = m_Lines[tlTo.nLine];
      ASSERT(tlTo.nChar <= str.GetLength());
      char *buffer = str.GetBuffer(0);
      char *delfrom = buffer + tlFrom.nChar;
      char *delto = buffer + tlTo.nChar;
      while (*delto) *delfrom++ = *delto++;
      *delfrom = 0;
      str.ReleaseBuffer(delfrom - buffer);
      hint.m_nCharsRemoved = tlTo.nChar - tlFrom.nChar;
    }
    return;
  }
  else
  {
    // Remove multiline selection
    CString str = GetLine(tlFrom.nLine).Left(tlFrom.nChar)
		+ GetLine(tlTo.nLine).Mid(tlTo.nChar);
    hint.m_nCharsRemoved = tlTo.nChar;
    hint.m_nLinesRemoved = tlTo.nLine - tlFrom.nLine;
    m_Lines.RemoveAt(tlFrom.nLine, hint.m_nLinesRemoved);
    m_Flags.RemoveAt(tlFrom.nLine, hint.m_nLinesRemoved);
    ASSERT(tlFrom.nLine < GetDocLines());
    m_Lines[tlFrom.nLine] = str;
    m_nMaxLineLength = max(m_nMaxLineLength, str.GetLength()+1);
    m_nMaxLineLength = min(MAXLINELENGTH, m_nMaxLineLength); // security
  }
}



// CTextBuffer::DoInsert
// -- insert string <str> at location <loc> and updates location
//    store number of lines/chars added into <hint>

void CTextBuffer::DoInsert(CTextLoc &loc, const CString& str, CTextUpdateHint& hint)
{
  hint.m_nCharsAdded = 0;
  hint.m_nLinesAdded = 0;
  // Count lines;
  char c;
  LPCSTR buf = str;
  while ((c=*buf++)) 
  {
    if (c=='\n' || c=='\r') 
      hint.m_nLinesAdded += 1;
    if (c=='\r' && *buf=='\n')
      buf++;
  }
  buf = str;
  // Perform insertion
  if (hint.m_nLinesAdded==0)
  {
    ASSERT(loc.nLine < GetDocLines());
    CString& strLine = m_Lines[loc.nLine];
    int lenLine = strLine.GetLength();
    int lenInsert = str.GetLength();
    LPSTR target = strLine.GetBuffer(lenLine + lenInsert + 1);
    if (lenLine > loc.nChar)
      memmove(target+loc.nChar+lenInsert, target+loc.nChar, lenLine-loc.nChar);
    if (lenInsert > 0)
      memcpy(target+loc.nChar, buf, lenInsert);
    strLine.ReleaseBuffer(lenLine+lenInsert);
    m_nMaxLineLength = max(m_nMaxLineLength, CharToPos(loc.nLine, strLine.GetLength()));
    m_nMaxLineLength = min(MAXLINELENGTH, m_nMaxLineLength); // security
    hint.m_nCharsAdded = lenInsert;
    loc.nChar += lenInsert;
  }
  else
  {
    CString strOld = GetLine(loc.nLine);
    m_Lines.InsertAt(loc.nLine, strOld, hint.m_nLinesAdded);
    m_Flags.InsertAt(loc.nLine, 0, hint.m_nLinesAdded);
    LPCSTR line = buf;
    int lcount = 0;
    for(;;)
    {
      // - find end of current line
      buf = line;
      while (*buf!='\n' && *buf!='\r' && *buf)
	buf++;
      // - handle special insertion cases
      if (lcount==0)
      {
	CString strInsert(line,buf-line);
	m_Lines[loc.nLine] = strOld.Left(loc.nChar) + strInsert;
	strOld = strOld.Mid(loc.nChar);
      }
      else if (lcount<hint.m_nLinesAdded)
      {
	CString strInsert(line,buf-line);
	m_Lines[loc.nLine] = strInsert;
      }
      else
      {
	ASSERT(*buf==0);
	CString strInsert(line,buf-line);
	ASSERT(loc.nLine < GetDocLines());
	m_Lines[loc.nLine] = strInsert + strOld;
	loc.nChar = hint.m_nCharsAdded = buf - line;
      }
      CString &strLine = m_Lines[loc.nLine];
      m_nMaxLineLength = max(m_nMaxLineLength,CharToPos(loc.nLine,strLine.GetLength()));
      m_nMaxLineLength = min(MAXLINELENGTH, m_nMaxLineLength); // security
      // - next line
      line = buf;
      if (*line==0)
	break;
      if (*line=='\r') 
	line++;
      if (*line=='\n') 
	line++;
      loc.nLine += 1;
      lcount += 1;
    }
    ASSERT(lcount == hint.m_nLinesAdded);
  }
}



// CTextBuffer::Replace
// -- main function for modifying text buffer
//    replace text between location <locFrom> and <locTo> by string <str>
//    update all views (selection must be hidden)

void CTextBuffer::Replace(CTextLoc locFrom, CTextLoc locTo, const CString &str)
{
  CTextUpdateHint hint(this);
  ReplaceHint(locFrom, locTo, str, TRUE, hint);
  if (m_pDoc)
    m_pDoc->UpdateAllViews(NULL, 0, &hint);
}


// CTextBuffer::ReplaceHint
// -- detailled function for modifying text buffer
//    replace text between location <locFrom> and <locTo> by string <str>
//    stores undo information if flag <bUndo> is set
//    stores update hints into variable <hint>.
//    does not update the views.

void CTextBuffer::ReplaceHint(CTextLoc locFrom, CTextLoc locTo, const CString &str, 
			      BOOL bUndo, CTextUpdateHint& hint)
{
  // Swap range is necessary
  if (locTo < locFrom) 
  {
    CTextLoc tmp = locFrom;
    locFrom = locTo;
    locTo = tmp;
  }
  // collect undo information
  if (bUndo)
  {
    // Check actions
    BOOL bUndoLSet = FALSE;
    BOOL bUndoSSet = FALSE;
    BOOL bDelete = (locFrom < locTo);
    BOOL bInsert = (! str.IsEmpty());
    if (!bInsert && !bDelete)
      return;
 
    // Collapse undo information
    CString strSav;
    GetRange(strSav, locFrom, locTo, TRUE);
    if (m_bUndoOk)
    {
      if (locFrom == m_tlUndoLocTo) {
	m_sUndoString = m_sUndoString + strSav;
        bUndoSSet = TRUE;
        bUndoLSet = TRUE;
      } else if (locTo == m_tlUndoLocFrom) {
	m_sUndoString = strSav + m_sUndoString;
        bUndoSSet = TRUE;
      }
    }
    if (!bUndoSSet)
      m_sUndoString = strSav;
    if (!bUndoLSet)
      m_tlUndoLocFrom = locFrom;
  }
  else if (m_bUndoOk && m_tlUndoLocFrom < locTo && locFrom < m_tlUndoLocTo )
  {
    // Cancel undo information if replace hits the undo area
    m_bUndoOk = FALSE;
  }

  // Perform replacement
  if (m_pDoc)
    m_pDoc->SetModifiedFlag();
  hint.m_bLineHint = TRUE;
  hint.m_nFromLine = locFrom.nLine;
  hint.m_nToLine = locTo.nLine;
  hint.m_bUpdateSelection = TRUE;
  hint.m_tlRef = locFrom;
  DoRemove(locFrom, locTo, hint);
  DoInsert(locFrom, str, hint);
  // Complete undo information
  if (bUndo) 
  {
    m_tlUndoLocTo = locFrom;
    m_bUndoOk = TRUE;
  } 
  else if (m_bUndoOk && hint.m_bUpdateSelection) 
  {
    AdjustLocation(m_tlUndoLocFrom, &hint);
    AdjustLocation(m_tlUndoLocTo, &hint);
  }
  // Adjust line hint
  if (!hint.m_bUpdateSelection)
    hint.m_bLineHint = FALSE;
  else if (hint.m_nLinesAdded != hint.m_nLinesRemoved)
    hint.m_nToLine = -1;
}




// CTextBuffer::ClearUndoBuffer
// -- clear undo information

void CTextBuffer::ClearUndoBuffer()
{
  m_bUndoOk = FALSE;
  m_sUndoString.Empty();
}



// CTextBuffer::GetUndoBuffer
// -- return last text removed and stored in undo buffer
 
const CString& CTextBuffer::GetUndoBuffer()
{
  if (!m_bUndoOk)
    m_sUndoString.Empty();
  return m_sUndoString;
}



// CTextBuffer::CanUndo
// -- return TRUE if there is valid undo information

BOOL CTextBuffer::CanUndo()
{
  return m_bUndoOk;
}



// CTextBuffer::PerformUndo
// -- apply undo buffer and return affected region

void CTextBuffer::PerformUndo(CTextLoc& tlFrom, CTextLoc& tlTo)
{
  if (m_bUndoOk)
  {
    CString strUndo = m_sUndoString;
    tlFrom = m_tlUndoLocFrom;
    tlTo = m_tlUndoLocTo;
    ClearUndoBuffer();
    Replace(tlFrom, tlTo, strUndo);
    tlFrom = m_tlUndoLocFrom;
    tlTo = m_tlUndoLocTo;
  }
}





/////////////////////////////////////////////////////////////////////////////
// CTextDoc

IMPLEMENT_DYNCREATE(CTextDoc, CDocument)

BEGIN_MESSAGE_MAP(CTextDoc, CDocument)
	//{{AFX_MSG_MAP(CTextDoc)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)
END_MESSAGE_MAP()


// CTextDoc::CTextDoc
// -- create text document and initializes text buffer

CTextDoc::CTextDoc()
{
  m_textBuffer.SetDocument(this);
}

CTextDoc::~CTextDoc()
{
}



// CTextDoc diagnostics
#ifdef _DEBUG
void CTextDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTextDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG



// CTextDoc::Serialize
// -- save or load text buffer
//    called by MFC save and load operations

void CTextDoc::Serialize(CArchive& ar)
{
	m_textBuffer.Serialize(ar);
}


// CTextDoc::DeleteContents
// -- cleanup text buffer and document variables
//    called by MFC

void CTextDoc::DeleteContents() 
{
  m_textBuffer.DeleteContents();
  CDocument::DeleteContents();
}



// CTextDoc::OnSaveDocument
// -- clears undo buffer when document is savec

BOOL CTextDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
  m_textBuffer.ClearUndoBuffer();
  return CDocument::OnSaveDocument(lpszPathName);
}



// CTextDoc::SetModifiedFlag
// -- adjust title when text buffer is modified
//    override CDocument::SetModifiedFlag

void CTextDoc::SetModifiedFlag(BOOL bModified)
{
  BOOL bOldModified = IsModified();
  CDocument::SetModifiedFlag(bModified);
  if (bModified != bOldModified)
    SetTitle(GetTitle());
}


// CTextDoc::GetDecentPathName()
// -- returns a path name that contains "\...\"
//    when the full path name is too long.

CString CTextDoc::GetDecentPathName(int maxlen) const
{
  CString str = GetPathName();
  int len = str.GetLength();
  if (len > maxlen)
  {
    int i,j;  
    LPSTR s = str.GetBuffer(0);
    for (i=0; i<len; i++)
      if (s[i]=='\\')
        break;
    for (j=i+4; j<len; j++)
      if (s[j]=='\\')
        if (len-(j-i-1)+3 <= maxlen)
        {
          strcpy(s+i,"\\...\\");
          strcpy(s+i+4,s+j);
          break;
        }
    str.ReleaseBuffer();
  }
  return str;
}




// CTextDoc::SetTitle
// -- computes title for editor window
//    adds a star when text buffer is modified
//    override CDocument::SetTitle

void 
CTextDoc::SetTitle(LPCTSTR lpszTitle) 
{
    CString cstr = GetDecentPathName();
    if (cstr.IsEmpty()) 
    {
      CDocument::SetTitle(lpszTitle);
    }
    else
    {
      if (m_bModified)
        CDocument::SetTitle(cstr + " *");
      else
        CDocument::SetTitle(cstr);
    }
}



// CTextDoc::OnFileSave
// -- message handler for saving document
//    needs to call our DoPromptFileName
//    in order to stop TLisp while
//    Windows is changing CWD  @##@@!!!

void CTextDoc::OnFileSave() 
{
  const CString &str = GetPathName();
  if (str.IsEmpty())
  {
    OnFileSaveAs();
  }
  else 
  {
    DWORD dwAttrib = GetFileAttributes(m_strPathName);
    if (dwAttrib & FILE_ATTRIBUTE_READONLY)
    {
      OnFileSaveAs();
    }
    else
    {
      DoSave(str);
    }
  }
}


// CTextDoc::OnFileSaveAs
// -- message handler for saving document
//    needs to call our DoPromptFileName
//    in order to stop TLisp while
//    Windows is changing CWD  @##@@!!!

void CTextDoc::OnFileSaveAs() 
{
  CDocTemplate* pTemplate = GetDocTemplate();
  ASSERT(pTemplate != NULL);
  CString newName = GetPathName();
  if (theApp.DoPromptFileName(newName,
                  AFX_IDS_SAVEFILE,
		  OFN_HIDEREADONLY|OFN_PATHMUSTEXIST, 
                  FALSE, pTemplate ) )
  {
    DoSave(newName);
  }
}
