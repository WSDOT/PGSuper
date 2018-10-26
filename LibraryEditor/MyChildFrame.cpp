///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include "libraryeditor.h"
#include "MyChildFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyChildFrame

IMPLEMENT_DYNCREATE(CMyChildFrame, CLibraryEditorChildFrame)

CMyChildFrame::CMyChildFrame()
{
}

CMyChildFrame::~CMyChildFrame()
{
}


BEGIN_MESSAGE_MAP(CMyChildFrame, CLibraryEditorChildFrame)
	//{{AFX_MSG_MAP(CMyChildFrame)
	ON_COMMAND(IDM_ADD_ENTRY, OnAddEntry)
	ON_COMMAND(IDM_DELETE_ENTRY, OnDeleteEntry)
	ON_UPDATE_COMMAND_UI(IDM_DELETE_ENTRY, OnUpdateDeleteEntry)
	ON_COMMAND(IDM_EDIT_ENTRY, OnEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_ENTRY, OnUpdateEditEntry)
	ON_COMMAND(IDM_DUPLICATE_ENTRY, OnDuplicateEntry)
	ON_UPDATE_COMMAND_UI(IDM_DUPLICATE_ENTRY, OnUpdateDuplicateEntry)
	ON_COMMAND(IDM_SI, OnSi)
	ON_COMMAND(IDM_US, OnUs)
	ON_UPDATE_COMMAND_UI(IDM_SI, OnUpdateSi)
	ON_UPDATE_COMMAND_UI(IDM_US, OnUpdateUs)
	ON_COMMAND(IDM_SMALL_ICONS, OnSmallIcons)
	ON_COMMAND(IDM_LARGE_ICONS, OnLargeIcons)
	ON_COMMAND(IDM_LIST_ICONS, OnListIcons)
	ON_COMMAND(IDM_ARRANGE_ICONS, OnArrangeIcons)
	ON_UPDATE_COMMAND_UI(IDM_LARGE_ICONS, OnUpdateLargeIcons)
	ON_UPDATE_COMMAND_UI(IDM_LIST_ICONS, OnUpdateListIcons)
	ON_UPDATE_COMMAND_UI(IDM_SMALL_ICONS, OnUpdateSmallIcons)
	ON_UPDATE_COMMAND_UI(IDM_ADD_ENTRY, OnUpdateAddEntry)
	ON_UPDATE_COMMAND_UI(IDM_RENAME_ENTRY, OnUpdateRenameEntry)
	ON_COMMAND(IDM_RENAME_ENTRY, OnRenameEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyChildFrame message handlers

void CMyChildFrame::OnAddEntry() 
{
   CLibraryEditorChildFrame::DoAddEntry();	
}

void CMyChildFrame::OnDeleteEntry() 
{
   // call parent
   DoDeleteEntry();
}

void CMyChildFrame::OnUpdateDeleteEntry(CCmdUI* pCmdUI) 
{
   DoUpdateDeleteEntry(pCmdUI);
}

void CMyChildFrame::OnEditEntry() 
{
   DoEditEntry();
}

void CMyChildFrame::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
   DoUpdateEditEntry(pCmdUI);
}

void CMyChildFrame::OnDuplicateEntry() 
{
   DoDuplicateEntry();
}

void CMyChildFrame::OnUpdateDuplicateEntry(CCmdUI* pCmdUI) 
{
   DoUpdateDuplicateEntry(pCmdUI);
}

void CMyChildFrame::OnSi() 
{
   DoSetUnitsMode(libUnitsMode::UNITS_SI);	
}

void CMyChildFrame::OnUs()
{
   DoSetUnitsMode(libUnitsMode::UNITS_US);	
}

void CMyChildFrame::OnUpdateSi(CCmdUI* pCmdUI) 
{
   libUnitsMode::Mode mode = DoGetUnitsMode();
   pCmdUI->SetRadio(mode==libUnitsMode::UNITS_SI);
}

void CMyChildFrame::OnUpdateUs(CCmdUI* pCmdUI) 
{
   libUnitsMode::Mode mode = DoGetUnitsMode();
   pCmdUI->SetRadio(mode==libUnitsMode::UNITS_US);
}

void CMyChildFrame::OnSmallIcons() 
{
	DoSmallIcons();
}

void CMyChildFrame::OnLargeIcons() 
{
	DoLargeIcons();
}

void CMyChildFrame::OnListIcons() 
{
	DoListView();
}

void CMyChildFrame::OnArrangeIcons() 
{
	DoArrangeIcons();
}

void CMyChildFrame::OnUpdateLargeIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateLargeIcons());
}

void CMyChildFrame::OnUpdateListIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateListView());
}

void CMyChildFrame::OnUpdateSmallIcons(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(DoUpdateSmallIcons());
}

void CMyChildFrame::OnUpdateAddEntry(CCmdUI* pCmdUI) 
{
	DoUpdateAddEntry(pCmdUI);
}

void CMyChildFrame::OnUpdateRenameEntry(CCmdUI* pCmdUI) 
{
   DoUpdateRenameEntry(pCmdUI);
}

void CMyChildFrame::OnRenameEntry() 
{
	DoRenameEntry();
}
