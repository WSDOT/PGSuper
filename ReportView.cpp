///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecificationBuilder.h>
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
      m_ErrorMsg = _T("Errors exist that prevent analysis. Review the errors posted in the status center for more information");
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
      RefreshReport();
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
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Working..."));

   GET_IFACE2(pBroker,IProjectProperties,pProjectProperties);

   // Set Header and footer information for printed reports
   std::_tstring  foot1(_T("Bridge: "));
   foot1 += pProjectProperties->GetBridgeName();
   m_pReportSpec->SetLeftFooter(foot1.c_str());
   std::_tstring  foot2(_T("Job: "));
   foot2 += pProjectProperties->GetJobNumber();
   m_pReportSpec->SetCenterFooter(foot2.c_str());

   AFX_MANAGE_STATE(AfxGetAppModuleState()); /////////

   return CEAFAutoCalcReportView::UpdateReportBrowser(pHint);
}

void CPGSuperReportView::RefreshReport()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress progress(pProgress);
   pProgress->UpdateMessage(_T("Updating report..."));

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

bool CPGSuperReportView::CreateReport(CollectionIndexType rptIdx,bool bPromptForSpec)
{
   if ( !bPromptForSpec )
   {
      // Not prompting for spec... this is a quick report, just handle it normaly
      return CEAFAutoCalcReportView::CreateReport(rptIdx,bPromptForSpec);
   }

   // If the requested report is a span/girder report we want to support creating multiple individual reports
   // Learn if this is a multi-span/girder report
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   std::vector<std::_tstring> names = pRptMgr->GetReportNames();
   boost::shared_ptr<CReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(names[rptIdx]);
   CReportDescription rptDesc = pRptBuilder->GetReportDescription();

   boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pRptBuilder->GetReportSpecificationBuilder();

   // See if we have a CMultiViewSpanGirderReportSpecificationBuilder. 
   // If so, we will cycle through creating report windows - one for each girder
   //////////////////////////////////////////////////////////////////////////////////////////////////////
   CMultiViewSpanGirderReportSpecificationBuilder* pSGRptSpecBuilder(dynamic_cast<CMultiViewSpanGirderReportSpecificationBuilder*>(pRptSpecBuilder.get()));
   if ( pSGRptSpecBuilder )
   {
      // this is a Span/Girder spec builder
      // Create the report specification. This will define the girders to be reported on and the chapters to report
      boost::shared_ptr<CReportSpecification> nullSpec;
      boost::shared_ptr<CReportSpecification> rptSpec = pSGRptSpecBuilder->CreateReportSpec(rptDesc,nullSpec);

      if(rptSpec)
      {
         CMultiViewSpanGirderReportSpecification* pSGRptSpec( dynamic_cast<CMultiViewSpanGirderReportSpecification*>(rptSpec.get()) );

         AFX_MANAGE_STATE(AfxGetStaticModuleState()); /////////

         GET_IFACE2(pBroker,IProgress,pProgress);
         CEAFAutoProgress ap(pProgress,0,PW_ALL | PW_NOGAUGE); // progress window has a cancel button

         CEAFDocument* pEAFDoc = (CEAFDocument*)GetDocument();
         CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;

         std::_tstring reportName = pSGRptSpec->GetReportName();

         std::vector<SpanGirderHashType> girderList =  pSGRptSpec->GetGirderList();
         ATLASSERT(!girderList.empty()); // UI should not allow this

         bool first(true);
         for ( std::vector<SpanGirderHashType>::iterator it=girderList.begin(); it!=girderList.end(); it++)
         {
            SpanIndexType span;
            GirderIndexType girder;
            UnhashSpanGirder(*it, &span, &girder);

            // Progress button
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(span) << _T(" Girder ") << LABEL_GIRDER(girder) << std::ends;
            pProgress->UpdateMessage(os.str().c_str());

            if ( pProgress->Continue() != S_OK )
               break; // cancel button pressed... quit creating reports


            // Creata a CSpanGirderReportSpecification. A single report view news a specification for a 
            // single girder.
            // Set the span/girder to report on
            boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanGirderReportSpecification(*pSGRptSpec, span, girder) );
            CSpanGirderReportSpecification* pMyReportSpec = (CSpanGirderReportSpecification*)pRptSpec.get();

            // Also need a SpanGirder Report Spec Builder for when the Edit button is pressed
            boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(pBroker) );

            if ( first )
            {
               // first report goes in this view
               CEAFAutoCalcReportView::CreateReport(rptIdx,pRptSpec,pRptSpecBuilder);
               first = false;
            }
            else
            {
               CEAFReportViewCreationData data;
               data.m_RptIdx = rptIdx;
               data.m_pRptSpecification = pRptSpec;
               data.m_pRptSpecificationBuilder = pRptSpecBuilder;
               data.m_pRptMgr = pRptMgr;

               // create new views for all other reports
               CView* pView = pEAFDoc->CreateView(pDoc->GetReportViewKey(),LPVOID(&data));
               CEAFReportView* pRptView = (CEAFReportView*)pView;
               pRptView->CreateReport(rptIdx,pRptSpec,pRptSpecBuilder);
            }
         }

         return true; 
      }
      else
      {
         return false; // user probably cancelled dialog
      }
   }
   else
   {
      // Not a span/girder spec builder so just do normal processing
      return CEAFAutoCalcReportView::CreateReport(rptIdx,bPromptForSpec);
   }

   ATLASSERT(false); // should never get here... should have already return
   return false;
}