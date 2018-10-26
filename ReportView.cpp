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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"

#include "PGSuperDoc.h"

#include "ReportView.h"
#include "Hints.h"

#include "htmlhelp\HelpTopics.hh"

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFStatusCenter.h>

#include <Reporting\SpanGirderReportSpecification.h>

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
	ON_WM_CREATE()
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
void CPGSuperReportView::OnInitialUpdate()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFStatusCenter,pStatusCenter);

   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      m_bUpdateError = true;
      m_ErrorMsg = "Errors exist that prevent analysis. Review the errors posted in the status center for more information";
      Invalidate();
      UpdateWindow();
      return;
   }

   CEAFAutoCalcReportView::OnInitialUpdate();
}

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

HRESULT CPGSuperReportView::UpdateReportBrowser(CReportHint* pHint)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0);

   pProgress->UpdateMessage("Working...");

   return CEAFAutoCalcReportView::UpdateReportBrowser(pHint);
}

void CPGSuperReportView::RefreshReport()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress progress(pProgress);
   pProgress->UpdateMessage("Updating report...");

   CEAFAutoCalcReportView::RefreshReport();
}

CReportHint* CPGSuperReportView::TranslateHint(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_GIRDERCHANGED )
   {
      CGirderHint* pGdrHint = (CGirderHint*)pHint;
      CSpanGirderReportHint* pSGHint = new CSpanGirderReportHint(pGdrHint->spanIdx,pGdrHint->gdrIdx,pGdrHint->lHint);
      return pSGHint;
   }
   return NULL;
}

int CPGSuperReportView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEAFAutoCalcReportView::OnCreate(lpCreateStruct) == -1)
		return -1;

   return 0;
}
