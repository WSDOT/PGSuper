///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// ReportView.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"

#include "PGSuperDoc.h"

#include "ReportView.h"
#include "Hints.h"

#include "htmlhelp\HelpTopics.hh"

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFStatusCenter.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReportView


IMPLEMENT_DYNCREATE(CPGSuperReportView, CEAFAutoCalcReportView)

CPGSuperReportView::CPGSuperReportView()
{
}

CPGSuperReportView::~CPGSuperReportView()
{
}

BEGIN_MESSAGE_MAP(CPGSuperReportView, CEAFAutoCalcReportView)
	//{{AFX_MSG_MAP(CPGSuperReportView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReportView drawing

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReportView diagnostics

#ifdef _DEBUG
void CPGSuperReportView::AssertValid() const
{
	CEAFAutoCalcReportView::AssertValid();
}

void CPGSuperReportView::Dump(CDumpContext& dc) const
{
	CEAFAutoCalcReportView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPGSuperReportView message handlers

void CPGSuperReportView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if ( m_bIsNewReport )
      return; // this the OnUpdate that comes from OnInitialUpdate() ... nothing to do here

   if ( lHint == HINT_GIRDERLABELFORMATCHANGED )
   {
      UpdateViewTitle();
   }

   if ( 0 < lHint && lHint <= MAX_DISPLAY_HINT )
      return; // some display feature changed... not data... nothing to do here


   // The current implementation of this method is to re-generate a report
   // every time anything changes.  We might want to consider making the
   // report objects themselves be connection points for the things
   // they depend on.  That way,  a report only re-generates if it needs to.
   if ( lHint == HINT_UPDATEERROR )
   {
      CString* pmsg = (CString*)pHint;
      m_ErrorMsg = *pmsg;
      m_bUpdateError = true;

      Invalidate();
      return;
   }

   CEAFAutoCalcReportView::OnUpdate(pSender,lHint,pHint);
}

BOOL CPGSuperReportView::PreTranslateMessage(MSG* pMsg) 
{
   if (pMsg->message == WM_KEYDOWN) 
   {
      if (pMsg->wParam == VK_F1)
      {
         ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_REPORT_VIEW );
         return TRUE;
      }
   }

	return CEAFAutoCalcReportView::PreTranslateMessage(pMsg);
}

HRESULT CPGSuperReportView::UpdateReportBrowser()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0);

   pProgress->UpdateMessage("Working...");

   return CEAFAutoCalcReportView::UpdateReportBrowser();
}

int CPGSuperReportView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEAFAutoCalcReportView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStatusCenter,pStatusCenter);

   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
      return eafTypes::statusError;

   return 0;
}

void CPGSuperReportView::CreateEditButton()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStatusCenter,pStatusCenter);

   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
      return;

   CEAFAutoCalcReportView::CreateEditButton();
}