///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2015  Washington State Department of Transportation
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

// ChildFrm.cpp : implementation of the CLibraryEditorChildFrame class
//

#include "stdafx.h"
#include "resource.h"
#include <psglib\ISupportLibraryManager.h>

#include <psglib\LibraryEditorChildFrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorChildFrame

IMPLEMENT_DYNCREATE(CLibraryEditorChildFrame, CEAFChildFrame)

BEGIN_MESSAGE_MAP(CLibraryEditorChildFrame, CEAFChildFrame)
	//{{AFX_MSG_MAP(CLibraryEditorChildFrame)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorChildFrame construction/destruction

CLibraryEditorChildFrame::CLibraryEditorChildFrame()
{
}

CLibraryEditorChildFrame::~CLibraryEditorChildFrame()
{
}

BOOL CLibraryEditorChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
#if defined _EAF_USING_MFC_FEATURE_PACK
   // If MFC Feature pack is used, we are using tabbed MDI windows so we don't want
   // the system menu or the minimize and maximize boxes
   cs.style &= ~(WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
#endif

	return CEAFChildFrame::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorChildFrame diagnostics

#ifdef _DEBUG
void CLibraryEditorChildFrame::AssertValid() const
{
	CEAFChildFrame::AssertValid();
}

void CLibraryEditorChildFrame::Dump(CDumpContext& dc) const
{
	CEAFChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorChildFrame message handlers
BOOL CLibraryEditorChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class

   // Create a splitter window with 1 rows and 2 column
   if ( !m_SplitterWnd.CreateStatic( this, 1, 2 ) )
   {
      TRACE0("Failed to create static splitter");
      return FALSE;
   }

   // Add the first pane
   if ( !m_SplitterWnd.CreateView(0,0,pContext->m_pNewViewClass, CSize(lpcs->cx/3,lpcs->cy), pContext) )
   {
      TRACE0("Failed to create first pane");
      return FALSE;
   }

   // Add the second pane
   if ( !m_SplitterWnd.CreateView(0,1,RUNTIME_CLASS(CLibEditorListView), CSize(0,0), pContext ) )
   {
      TRACE0("Failed to create second pane");
      return FALSE;
   }

   // Activate the first pane
   SetActiveView( (CView*)m_SplitterWnd.GetPane(0,0));

   // introduce the tree view to the list view
   CLibraryEditorView* pView= (CLibraryEditorView*)m_SplitterWnd.GetPane(0,0);
   CLibEditorListView* pList= (CLibEditorListView*)m_SplitterWnd.GetPane(0,1);
   pView->SetListView(pList);
	

   // I don't know why you don't call the parent but it makes the difference between
   // the splitter working and not.  See the ViewEx example. They don't call the
   // parent method either.
	//return CEAFChildFrame::OnCreateClient(lpcs, pContext);
   return TRUE;
 }

CLibEditorListView* CLibraryEditorChildFrame::GetListView()
{
   CLibEditorListView* pList= (CLibEditorListView*)m_SplitterWnd.GetPane(0,1);
   return pList;
}

CLibraryEditorView* CLibraryEditorChildFrame::GetTreeView()
{
   CLibraryEditorView* pView= (CLibraryEditorView*)m_SplitterWnd.GetPane(0,0);
   return pView;
}

void CLibraryEditorChildFrame::DoUpdateAddEntry(CCmdUI* pCmdUI)
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pCmdUI->Enable(pList->IsLibrarySelected());
}

bool CLibraryEditorChildFrame::DoAddEntry()
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   return pList->AddNewEntry();
}

void CLibraryEditorChildFrame::DoUpdateEditEntry(CCmdUI* pCmdUI) 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pCmdUI->Enable(pList->IsItemSelected());
}

void CLibraryEditorChildFrame::DoEditEntry() 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pList->EditSelectedEntry();
}

void CLibraryEditorChildFrame::DoDuplicateEntry() 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pList->DuplicateSelectedEntry();
}

void CLibraryEditorChildFrame::DoUpdateDuplicateEntry(CCmdUI* pCmdUI) 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pCmdUI->Enable(pList->IsItemSelected());
}

void CLibraryEditorChildFrame::DoUpdateDeleteEntry(CCmdUI* pCmdUI) 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pCmdUI->Enable(pList->IsEditableItemSelected());
}

void CLibraryEditorChildFrame::DoDeleteEntry() 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pList->DeleteSelectedEntry();
}

void CLibraryEditorChildFrame::DoListView() 
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	if ((dwStyle & LVS_TYPEMASK) != LVS_LIST)
		SetWindowLong(pList->m_hWnd, GWL_STYLE,
			(dwStyle & ~LVS_TYPEMASK) | LVS_LIST);
}

void CLibraryEditorChildFrame::DoReportView() 
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	if ((dwStyle & LVS_TYPEMASK) != LVS_REPORT)
		SetWindowLong(pList->m_hWnd, GWL_STYLE,
			(dwStyle & ~LVS_TYPEMASK) | LVS_REPORT);
}

void CLibraryEditorChildFrame::DoLargeIcons() 
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	if ((dwStyle & LVS_TYPEMASK) != LVS_ICON)
		SetWindowLong(pList->m_hWnd, GWL_STYLE,
			(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);
}

void CLibraryEditorChildFrame::DoSmallIcons()
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	if ((dwStyle & LVS_TYPEMASK) != LVS_SMALLICON)
		SetWindowLong(pList->m_hWnd, GWL_STYLE,
			(dwStyle & ~LVS_TYPEMASK) | LVS_SMALLICON);
}

bool CLibraryEditorChildFrame::DoUpdateListView()
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	return ((dwStyle & LVS_TYPEMASK) == LVS_LIST);

}

bool CLibraryEditorChildFrame::DoUpdateReportView()
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	return ((dwStyle & LVS_TYPEMASK) == LVS_REPORT);
}

bool CLibraryEditorChildFrame::DoUpdateLargeIcons()
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
					
	return ((dwStyle & LVS_TYPEMASK) == LVS_ICON);
}

bool CLibraryEditorChildFrame::DoUpdateSmallIcons()
{
	DWORD dwStyle;
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);

	dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE);
	return ((dwStyle & LVS_TYPEMASK) == LVS_SMALLICON);
}

void CLibraryEditorChildFrame::DoArrangeIcons()
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   CListCtrl& rctrl = pList->GetListCtrl();
   ASSERT(rctrl);

   rctrl.Arrange(LVA_DEFAULT);
}

void CLibraryEditorChildFrame::DoUpdateRenameEntry(CCmdUI* pCmdUI) 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pCmdUI->Enable(pList->IsEditableItemSelected());
}

void CLibraryEditorChildFrame::DoRenameEntry() 
{
   CLibEditorListView* pList= this->GetListView();
   ASSERT(pList);
   pList->RenameSelectedEntry();
}
