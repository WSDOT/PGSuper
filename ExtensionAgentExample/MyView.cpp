///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

// MyView.cpp : implementation file
//

#include "stdafx.h"
#include "MyView.h"

#include "resource.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocTemplate.h>
#include <EAF\EAFMenu.h>


// CMyView

IMPLEMENT_DYNCREATE(CMyView, CView)

CMyView::CMyView()
{
   m_pAgentCallback = nullptr;
}

CMyView::~CMyView()
{
}

BEGIN_MESSAGE_MAP(CMyView, CView)
   ON_WM_CONTEXTMENU()
   ON_COMMAND(ID_COMMAND2,OnViewOnlyCommand)
END_MESSAGE_MAP()


// CMyView drawing

void CMyView::OnDraw(CDC* pDC)
{
//	CDocument* pDoc = GetDocument();

   pDC->TextOut(0,0,_T("This is a view created by an extension agent. Right click for a context menu."));
}

// CMyView diagnostics

#ifdef _DEBUG
void CMyView::AssertValid() const
{
	CView::AssertValid();
}

#ifndef _WIN32_WCE
void CMyView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif
#endif //_DEBUG


// CMyView message handlers

void CMyView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
   // Menu items are not guarenteed to be unique in the system. Using the regular MFC
   // method of creating context menus, we could run into command ID issues. Our commands
   // could be handled by a different plugin or the main application.
   //
   // To get around this, use the IEAFMainMenu interface to create a context menu object.
   // When the menu resource is loaded into the object, the commands are mapped into the
   // command callback
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFMainMenu,pUI);
   CEAFMenu* pMenu = pUI->CreateContextMenu();
   
   // Load the context menu resource. By using nullptr as the command callback, all commands
   // on the context menu (as defined by the menu resource) are routed dirctly back to this view
   // because this view is the first stop on the MFC command routing (and that is true because
   // we are using "this" as the window handle in the call to TrackPopupMenu below)
   pMenu->LoadMenu(IDR_CONTEXT,nullptr);

   // we can add a command to the context menu that gets routed to the extension agent to handle
   pMenu->AppendMenu(ID_COMMAND1,_T("Command"),m_pAgentCallback);

   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,point.x,point.y,this);
}

void CMyView::OnInitialUpdate()
{
   CView::OnInitialUpdate();

   // TODO: Add your specialized code here and/or call the base class
   CDocument* pDoc = GetDocument();
   CDocTemplate* pDocTemplate = pDoc->GetDocTemplate();
   ASSERT( pDocTemplate->IsKindOf(RUNTIME_CLASS(CEAFDocTemplate)) );

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)pDocTemplate;
   m_pAgentCallback = (IEAFCommandCallback*)pTemplate->GetViewCreationData();
}

void CMyView::OnViewOnlyCommand()
{
   AfxMessageBox(_T("This is a view only command"));
}