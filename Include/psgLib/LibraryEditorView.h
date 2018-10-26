///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2011  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// LibraryView.h : interface of the CLibraryEditorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIBRARYEDITORVIEW_H__486E8C30_15EE_11D2_8EA8_006097DF3C68__INCLUDED_)
#define AFX_LIBRARYEDITORVIEW_H__486E8C30_15EE_11D2_8EA8_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "psgLibLib.h"

#include <afxcview.h>
#include "LibEditorListView.h"

class PSGLIBCLASS CLibraryEditorView : public CTreeView
{
protected: // create from serialization only
	CLibraryEditorView();
	DECLARE_DYNCREATE(CLibraryEditorView)

// Attributes
public:
   // sequence for state icons in state imagelist
   enum IconSequence{InUse=1,ReadOnly,InUseAndReadOnly};
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryEditorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLibraryEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CLibraryEditorView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Extended functionality
public:
   void SetListView(CLibEditorListView* pListView)
   {ASSERT(pListView); m_pListView=pListView;}

protected:
	CImageList          m_LibraryImages;  // images for libraries
	CImageList          m_StateImages;    // images for overlaying state information in entries
   CLibEditorListView* m_pListView;
private:
void CLibraryEditorView::InsertLibraryManager(Uint32 ilib_man, Uint32 ilib_man_sel, 
                                              int man_num, libLibraryManager* pMan, CTreeCtrl& tree, HTREEITEM hParent, int* lastIcon);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBRARYEDITORVIEW_H__486E8C30_15EE_11D2_8EA8_006097DF3C68__INCLUDED_)
