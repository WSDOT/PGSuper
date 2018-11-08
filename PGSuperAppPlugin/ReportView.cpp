///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include "PGSuperApp.h"

#include "PGSuperDocBase.h"

#include "ReportView.h"
#include "Hints.h"

#include <IFace\DocumentType.h>
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

   // Need to know if AutoCalc is turned on and if this is a Multi-View report
   CDocument* pDoc = GetDocument();
   CEAFAutoCalcDocMixin* pAutoCalcDoc = dynamic_cast<CEAFAutoCalcDocMixin*>(pDoc);
   ATLASSERT(pAutoCalcDoc); // your document must use the autocalc mix in

   CDocTemplate* pDocTemplate = pDoc->GetDocTemplate();
   ASSERT( pDocTemplate->IsKindOf(RUNTIME_CLASS(CEAFDocTemplate)) );

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)pDocTemplate;
   CEAFReportViewCreationData* pCreateData = (CEAFReportViewCreationData*)pTemplate->GetViewCreationData();
   ASSERT(pCreateData != nullptr);
   std::vector<std::_tstring> rptNames(pCreateData->m_pRptMgr->GetReportNames());
   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pCreateData->m_pRptMgr->GetReportSpecificationBuilder(rptNames[pCreateData->m_RptIdx]);
   CMultiViewSpanGirderReportSpecificationBuilder* pMultiViewRptSpecBuilder(dynamic_cast<CMultiViewSpanGirderReportSpecificationBuilder*>(pRptSpecBuilder.get()));

   // if autocalc is turned on, or this is not a multi-view report, just process this normally
   // by calling the base class OnInitialUpdate method
   if ( pAutoCalcDoc->IsAutoCalcEnabled() || pMultiViewRptSpecBuilder == nullptr )
   {
      CEAFAutoCalcReportView::OnInitialUpdate();
   }
   else
   {
      // AutoCalc is off .... by-pass the base class OnInitialUpdate method as it will cause
      // the reports to generate...

      // The base class calls this method, and it is necessary so do it here since
      // we are by-passing the base class
      CEAFAutoCalcViewMixin::Initialize();

      // Continue with initialization by skipping over the direct base of this class
      CEAFReportView::OnInitialUpdate();
   }
}

void CPGSuperReportView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if ( m_bIsNewReport )
   {
      return; // this the OnUpdate that comes from OnInitialUpdate() ... nothing to do here
   }

   if ( lHint == HINT_GIRDERLABELFORMATCHANGED )
   {
      RefreshReport();
   }

   if ( 0 < lHint && lHint <= MAX_DISPLAY_HINT )
   {
      return; // some display feature changed... not data... nothing to do here
   }


   CEAFAutoCalcReportView::OnUpdate(pSender,lHint,pHint);
}

BOOL CPGSuperReportView::PreTranslateMessage(MSG* pMsg) 
{
   if (pMsg->message == WM_KEYDOWN) 
   {
      if (pMsg->wParam == VK_F1)
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState());
         EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_REPORT_VIEW );
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
      CGirderHint* pGirderHint = (CGirderHint*)pHint;
      CGirderReportHint* pSGHint = new CGirderReportHint(pGirderHint->girderKey,pGirderHint->lHint);
      return pSGHint;
   }
   return nullptr;
}

int CPGSuperReportView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEAFAutoCalcReportView::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }

   return 0;
}

bool CPGSuperReportView::CreateReport(CollectionIndexType rptIdx,BOOL bPromptForSpec)
{
   // Everything in this version of CreateReport is done in support of multi-view report
   // creation because the underlying framework doesn't support it directly.

   CEAFDocument* pEAFDoc = (CEAFDocument*)GetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;
   CEAFAutoCalcDocMixin* pAutoCalcDoc = dynamic_cast<CEAFAutoCalcDocMixin*>(pDoc);
   ATLASSERT(pAutoCalcDoc); // your document must use the autocalc mix in

   if ( !bPromptForSpec )
   {
      // Not prompting for spec... this is a quick report, just handle it normally if AutoCalc is enabled
      if ( pAutoCalcDoc->IsAutoCalcEnabled() )
      {
         return CEAFAutoCalcReportView::CreateReport(rptIdx,bPromptForSpec);
      }
      else
      {
         // AutoCalc is off so we have to mimic CreateReport with the exception that we don't actually
         // create the report... Create the default report specification and then initialize the report view
         CreateReportSpecification(rptIdx,bPromptForSpec);
         return CEAFAutoCalcReportView::InitReport(m_pReportSpec,m_pRptSpecBuilder);
      }
   }

   // If the requested report is a span/girder report we want to support creating multiple individual reports
   // Learn if this is a multi-span/girder report
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   std::vector<std::_tstring> names = pRptMgr->GetReportNames();
   std::shared_ptr<CReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(names[rptIdx]);
   CReportDescription rptDesc = pRptBuilder->GetReportDescription();

   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pRptBuilder->GetReportSpecificationBuilder();

   // See if we have a CMultiViewSpanGirderReportSpecificationBuilder. 
   // If so, we will cycle through creating report windows - one for each girder
   //////////////////////////////////////////////////////////////////////////////////////////////////////
   CMultiViewSpanGirderReportSpecificationBuilder* pSGRptSpecBuilder(dynamic_cast<CMultiViewSpanGirderReportSpecificationBuilder*>(pRptSpecBuilder.get()));
   if ( pSGRptSpecBuilder )
   {
      // this is a Span/Girder spec builder
      // Create the report specification. This will define the girders to be reported on and the chapters to report
      std::shared_ptr<CReportSpecification> nullSpec;
      std::shared_ptr<CReportSpecification> rptSpec = pSGRptSpecBuilder->CreateReportSpec(rptDesc,nullSpec);

      if(rptSpec)
      {
         CMultiViewSpanGirderReportSpecification* pSGRptSpec( dynamic_cast<CMultiViewSpanGirderReportSpecification*>(rptSpec.get()) );

         AFX_MANAGE_STATE(AfxGetStaticModuleState()); /////////

         const std::vector<CGirderKey>& girderKeys( pSGRptSpec->GetGirderKeys() );
         ATLASSERT(!girderKeys.empty()); // UI should not allow this

         GET_IFACE2(pBroker,IProgress,pProgress);
         DWORD dwMask(girderKeys.size() == 1 ? PW_ALL | PW_NOGAUGE | PW_NOCANCEL : PW_ALL | PW_NOGAUGE);
         CEAFAutoProgress ap(pProgress,0,dwMask); // progress window has a cancel button

         std::_tstring reportName = pSGRptSpec->GetReportName();

         bool first(true);
         std::vector<CGirderKey>::const_iterator iter(girderKeys.begin());
         std::vector<CGirderKey>::const_iterator iterEnd(girderKeys.end());
         for ( ; iter != iterEnd; iter++)
         {
            const CGirderKey& girderKey(*iter);

            // Progress button
            pProgress->UpdateMessage(GIRDER_LABEL(girderKey));

            if ( pProgress->Continue() != S_OK )
            {
               break; // cancel button pressed... quit creating reports
            }


            // Creata a CSegmentReportSpecification. A single report view news a specification for a 
            // single segmentr.
            // Set the segment to report on
            std::shared_ptr<CReportSpecification> pRptSpec( std::make_shared<CGirderReportSpecification>(pSGRptSpec->GetReportTitle().c_str(),pBroker,girderKey) );
            CGirderReportSpecification* pMyReportSpec = (CGirderReportSpecification*)pRptSpec.get();
            pRptSpec->SetChapterInfo(pSGRptSpec->GetChapterInfo());

            // Also need a SpanGirder Report Spec Builder for when the Edit button is pressed
            std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(pBroker,girderKey) );

            if ( first )
            {
               // first report goes in this view
               if ( pAutoCalcDoc->IsAutoCalcEnabled() )
               {
                  CEAFAutoCalcReportView::CreateReport(rptIdx,pRptSpec,pRptSpecBuilder);
               }
               else
               {
                  InitReport(pRptSpec,pRptSpecBuilder);
               }
               first = false;
            }
            else
            {
               CEAFReportViewCreationData data;
               data.m_RptIdx = rptIdx;
               data.m_pRptSpecification = pRptSpec;
               data.m_pRptSpecificationBuilder = pRptSpecBuilder;
               data.m_pRptMgr = pRptMgr;

               if ( !pAutoCalcDoc->IsAutoCalcEnabled() )
               {
                  data.m_bInitializeOnly = true;
               }

               // create new views for all other reports
               CView* pView = pEAFDoc->CreateView(pDoc->GetReportViewKey(),LPVOID(&data));
            }
         }

         return true; 
      }
      else
      {
         CEAFMainFrame* pFrame = EAFGetMainFrame();
         pFrame->DisableFailCreateMessage();
         pFrame->CreateCanceled();
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