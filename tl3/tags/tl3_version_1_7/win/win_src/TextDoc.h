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
 
// TextDoc.h : interface of the CTextDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CTextLoc;
class CTextUpdateHint;
class CTextBuffer;
class CTextDoc;


////////// CTextLoc

class CTextLoc 
{
public:
  int nLine;
  int nChar;
  CTextLoc() : nLine(0), nChar(0) { };
  CTextLoc(CTextLoc &loc) : nLine(loc.nLine), nChar(loc.nChar) { };
};

////////// CTextLoc inlines


static inline BOOL 
operator == ( const CTextLoc& loc1, const CTextLoc& loc2) {
  return (loc1.nLine==loc2.nLine) && (loc1.nChar==loc2.nChar);
}

static inline BOOL 
operator != ( const CTextLoc& loc1, const CTextLoc& loc2) {
  return (loc1.nLine!=loc2.nLine) || (loc1.nChar!=loc2.nChar);
}

static inline BOOL 
operator < ( const CTextLoc& loc1, const CTextLoc& loc2) {
  if (loc1.nLine == loc2.nLine)
    return loc1.nChar < loc2.nChar;
  else
    return loc1.nLine < loc2.nLine;
}

static inline BOOL 
operator <= ( const CTextLoc& loc1, const CTextLoc& loc2) {
  if (loc1.nLine == loc2.nLine)
    return loc1.nChar <= loc2.nChar;
  else
    return loc1.nLine < loc2.nLine;
}

////////// CTextUpdateHint

class CTextUpdateHint : public CObject 
{
protected:
  DECLARE_DYNAMIC(CTextUpdateHint)
public:
  CTextUpdateHint(CTextBuffer *pBuffer);
  CTextBuffer* m_pBuffer;   // which buffer has been modified
  BOOL m_bLineHint;	    // which line has been modified
  int m_nFromLine;
  int m_nToLine;
  BOOL m_bUpdateSelection;  // should selection should be moved
  CTextLoc m_tlRef;
  int m_nLinesAdded;
  int m_nCharsAdded;
  int m_nLinesRemoved;
  int m_nCharsRemoved;
};


////////// CTextBuffer

class CTextBuffer 
{
// Attributes
protected:
	CDocument* m_pDoc;
	int m_nMaxLineLength;
	CArray<CString,CString&> m_Lines;
	CArray<BYTE,BYTE> m_Flags;
	BOOL m_bUndoOk;
	CTextLoc m_tlUndoLocFrom;
	CTextLoc m_tlUndoLocTo;
	CString  m_sUndoString;

// Operations
public:
	// information
	int GetDocLines() { return m_Lines.GetSize(); };
	int GetDocChars() { return m_nMaxLineLength; };
	int CharToPos(int nLine, int nChar);
	int PosToChar(int nLine, int nPos);
	void AdjustLocation(CTextLoc &loc, const CTextUpdateHint* hint);
	// access functions
	char GetAt(CTextLoc& loc);
	char GetPostInc(CTextLoc& loc);
	char GetPreDec(CTextLoc& loc);
	const CString& GetLine(int nLine);
	BYTE& GetFlags(int nLine);
	void GetRange(CString& str, CTextLoc tlFrom, CTextLoc tlTo, BOOL bCrLf=FALSE);
	// edition
	BOOL CanUndo();
	void PerformUndo(CTextLoc& tlFrom, CTextLoc& tlTo);
	void ReplaceHint(CTextLoc, CTextLoc, const CString&, BOOL, CTextUpdateHint&);
	void Replace(CTextLoc, CTextLoc, const CString&);
	void ClearUndoBuffer();
	const CString& GetUndoBuffer();
	void Serialize(CArchive& ar);
	void DeleteContents();

// Implementation
public:
	CTextBuffer();
	void SetDocument(CDocument *pDoc);
	virtual ~CTextBuffer();
protected:
	void DoInsert(CTextLoc &loc, const CString& str, CTextUpdateHint& hint);
	void DoRemove(CTextLoc tlFrom, CTextLoc tlTo, CTextUpdateHint& hint);
	int ComputeMaxLineLength();
};


//////// CTextBuffer inlines

inline const CString&
CTextBuffer::GetLine(int nLine) {
  ASSERT(nLine>=0 && nLine<GetDocLines());
  return m_Lines[nLine];
}

inline BYTE&
CTextBuffer::GetFlags(int nLine) {
  ASSERT(nLine>=0 && nLine<GetDocLines());
  return m_Flags[nLine];
}


inline char 
CTextBuffer::GetAt(CTextLoc &loc) {
  if (loc.nChar>=0 && loc.nLine>=0 && loc.nLine<m_Lines.GetSize()) {
    CString& str = m_Lines[loc.nLine];
    if (loc.nChar < str.GetLength())
      return str[loc.nChar];
    if (loc.nLine < m_Lines.GetUpperBound())
      if (loc.nChar == str.GetLength())
	return '\n';
  }
  return 0;
}

inline char 
CTextBuffer::GetPostInc(CTextLoc &loc) {
  ASSERT(loc.nChar >= 0);
  ASSERT(loc.nLine >= 0);
  if (loc.nLine >= m_Lines.GetSize()) 
    return 0;
  CString& str = m_Lines[loc.nLine];
  if (loc.nChar < str.GetLength())
    return str[loc.nChar++];
  if (loc.nLine==m_Lines.GetUpperBound())
    return 0;
  loc.nLine += 1;
  loc.nChar = 0;
  return '\n';
}

inline char 
CTextBuffer::GetPreDec(CTextLoc &loc) {
  if (loc.nChar > 0) {
    loc.nChar -= 1;
    CString& str = m_Lines[loc.nLine];
    ASSERT (loc.nChar < str.GetLength());
    return str[loc.nChar];
  } else if (loc.nLine > 0) {
    loc.nLine -= 1;
    ASSERT(loc.nLine < m_Lines.GetSize());
    loc.nChar = m_Lines[loc.nLine].GetLength();
    return '\n';
  }
  return 0;
}
	  



////////// CTextDoc

class CTextDoc : public CDocument
{
protected:
	DECLARE_DYNCREATE(CTextDoc)
public:
	CTextDoc();
        CString GetDecentPathName(int maxlen=32) const;

// Attributes
protected:
	friend class CTextView;
	CTextBuffer m_textBuffer;

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void SetTitle(LPCTSTR lpszTitle);
	//}}AFX_VIRTUAL
	void SetModifiedFlag(BOOL bModified = TRUE);

// Implementation
public:
	virtual ~CTextDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CTextDoc)
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};





/////////////////////////////////////////////////////////////////////////////
