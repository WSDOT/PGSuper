///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2019  Washington State Department of Transportation
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

// LibraryEditorChildFrm.h : interface of the CLibraryEditorChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__340EC2FC_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
#define AFX_CHILDFRM_H__340EC2FC_20E1_11D2_9D35_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "psgLibLib.h"

#include "LibEditorListView.h"
#include "LibraryEditorView.h"

class PSGLIBCLASS CLibraryEditorChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CLibraryEditorChildFrame)
public:
	CLibraryEditorChildFrame();

// Attributes
public:
   CSplitterWnd m_SplitterWnd;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryEditorChildFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLibraryEditorChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CLibraryEditorChildFrame)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Class Extensions
// Here is where you wire in events from your derived ChildFrame class
protected:

   // Add an entry the the selected library. Return false if failed
   void DoUpdateAddEntry(CCmdUI* pCmdUI);
   bool DoAddEntry();
   void DoUpdateEditEntry(CCmdUI* pCmdUI);
   void DoEditEntry();
   void DoUpdateDuplicateEntry(CCmdUI* pCmdUI);
   void DoDuplicateEntry();
   void DoUpdateDeleteEntry(CCmdUI* pCmdUI);
   void DoDeleteEntry();
   void DoRenameEntry();
   void DoUpdateRenameEntry(CCmdUI* pCmdUI);
   void DoListView();
   bool DoUpdateListView();
   void DoReportView();
   bool DoUpdateReportView();
   void DoLargeIcons();
   bool DoUpdateLargeIcons();
   void DoSmallIcons();
   bool DoUpdateSmallIcons();
   void DoArrangeIcons();

private:
   CLibEditorListView* GetListView();
   CLibraryEditorView* GetTreeView();

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__340EC2FC_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
