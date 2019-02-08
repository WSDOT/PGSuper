///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// MyChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include <psgLib\LibChildFrm.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLibChildFrame

IMPLEMENT_DYNCREATE(CLibChildFrame, CLibraryEditorChildFrame)

CLibChildFrame::CLibChildFrame()
{
}

CLibChildFrame::~CLibChildFrame()
{
}

BOOL CLibChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CLibraryEditorChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_LIBRARYTYPE);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}


BEGIN_MESSAGE_MAP(CLibChildFrame, CLibraryEditorChildFrame)
	//{{AFX_MSG_MAP(CLibChildFrame)
	ON_COMMAND(IDM_ADD_ENTRY, OnAddEntry)
	ON_UPDATE_COMMAND_UI(IDM_ADD_ENTRY, OnUpdateAddEntry)
	ON_COMMAND(IDM_DELETE_ENTRY, OnDeleteEntry)
	ON_UPDATE_COMMAND_UI(IDM_DELETE_ENTRY, OnUpdateDeleteEntry)
	ON_COMMAND(IDM_EDIT_ENTRY, OnEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_ENTRY, OnUpdateEditEntry)
	ON_COMMAND(IDM_DUPLICATE_ENTRY, OnDuplicateEntry)
	ON_UPDATE_COMMAND_UI(IDM_DUPLICATE_ENTRY, OnUpdateDuplicateEntry)
	ON_COMMAND(IDM_SMALL_ICONS, OnSmallIcons)
	ON_COMMAND(IDM_LARGE_ICONS, OnLargeIcons)
	ON_COMMAND(IDM_LIST_ICONS, OnListIcons)
	ON_COMMAND(IDM_ARRANGE_ICONS, OnArrangeIcons)
	ON_UPDATE_COMMAND_UI(IDM_LARGE_ICONS, OnUpdateLargeIcons)
	ON_UPDATE_COMMAND_UI(IDM_LIST_ICONS, OnUpdateListIcons)
	ON_UPDATE_COMMAND_UI(IDM_SMALL_ICONS, OnUpdateSmallIcons)
	ON_UPDATE_COMMAND_UI(IDM_ADD_ENTRY, OnUpdateAddEntry)
	ON_COMMAND(IDM_RENAME_ENTRY, OnRenameEntry)
	ON_UPDATE_COMMAND_UI(IDM_RENAME_ENTRY, OnUpdateRenameEntry)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibChildFrame message handlers

void CLibChildFrame::OnAddEntry() 
{
   CLibraryEditorChildFrame::DoAddEntry();	
}

void CLibChildFrame::OnUpdateAddEntry(CCmdUI* pCmdUI) 
{
   DoUpdateAddEntry(pCmdUI);
}

void CLibChildFrame::OnDeleteEntry() 
{
   // call parent
   DoDeleteEntry();
}

void CLibChildFrame::OnUpdateDeleteEntry(CCmdUI* pCmdUI) 
{
   DoUpdateDeleteEntry(pCmdUI);
}

void CLibChildFrame::OnEditEntry() 
{
   DoEditEntry();
}

void CLibChildFrame::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
   DoUpdateEditEntry(pCmdUI);
}

void CLibChildFrame::OnDuplicateEntry() 
{
   DoDuplicateEntry();
}

void CLibChildFrame::OnUpdateDuplicateEntry(CCmdUI* pCmdUI) 
{
   DoUpdateDuplicateEntry(pCmdUI);
}

void CLibChildFrame::OnSmallIcons() 
{
	DoSmallIcons();
}

void CLibChildFrame::OnLargeIcons() 
{
	DoLargeIcons();
}

void CLibChildFrame::OnListIcons() 
{
	DoListView();
}

void CLibChildFrame::OnArrangeIcons() 
{
	DoArrangeIcons();
}

void CLibChildFrame::OnUpdateLargeIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateLargeIcons());
}

void CLibChildFrame::OnUpdateListIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateListView());
}

void CLibChildFrame::OnUpdateSmallIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateSmallIcons());
}

void CLibChildFrame::OnRenameEntry() 
{
	DoRenameEntry();
}

void CLibChildFrame::OnUpdateRenameEntry(CCmdUI* pCmdUI) 
{
	DoUpdateRenameEntry(pCmdUI);
}

void CLibChildFrame::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_LIBRARY_VIEW);
}

void CLibChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg("PGSLibrary Editor");

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}
