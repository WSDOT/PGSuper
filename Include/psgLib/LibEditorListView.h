///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2013  Washington State Department of Transportation
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

#if !defined(AFX_LIBEDITORLISTVIEW_H__340EC324_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
#define AFX_LIBEDITORLISTVIEW_H__340EC324_20E1_11D2_9D35_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LibEditorListView.h : header file
//
#include "psgLibLib.h"

#include <afxcview.h>
#include <LibraryFw\ILibrary.h>

class CLibraryEditorView;

/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView view

class PSGLIBCLASS CLibEditorListView : public CListView
{
   friend CLibraryEditorView;
protected:
	CLibEditorListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLibEditorListView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibEditorListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLibEditorListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CLibEditorListView)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

      // class extensions
public:
   // a new library was selected, need to redraw
   void OnLibrarySelected(int libnun, const CString& name);
   // redraw
   void RedrawAllEntries();
   // add a new entry to the library
   bool AddNewEntry();
   // is an item selected
   bool IsItemSelected()const;
   // is an item selected and available for edit,delete,duplicate
   bool IsEditableItemSelected()const;
   // delete selected entry - must be an item selected
   void DeleteSelectedEntry();
   // duplicate selected entry
   void DuplicateSelectedEntry();
   // edit selected entry
   void EditSelectedEntry();
   // rename selected entry
   void RenameSelectedEntry();
   // return true if a library or library entry is selected
   bool IsLibrarySelected()const;

private:
   CString             m_LibName;
   int           m_LibIndex; // currently select library, -1 if none
   mutable int   m_ItemSelected;// current item selected. -1 if none

	CImageList          m_NormalEntryImages;    // images for entries
	CImageList          m_SmallEntryImages; 


   // context menu is selected
   //void OnCmenuSelected(UINT id);
   bool EditEntry(libILibrary* plib, LPCTSTR entryName);
   void DeleteEntry(libILibrary* plib, LPCTSTR entryName, bool force=false);
   void DuplicateEntry(libILibrary* plib, LPCTSTR entryName);
   bool DoesEntryExist(const CString& entryName);
   bool GetSelectedEntry(CString* entryName, libILibrary** pplib)const;
   int InsertEntryToList(const libLibraryEntry* pentry, const libILibrary* plib, int i);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBEDITORLISTVIEW_H__340EC324_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
