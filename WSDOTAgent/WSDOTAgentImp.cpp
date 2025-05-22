///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// WSDOTAgentImp.cpp : Implementation of CWSDOTAgentImp

#include "stdafx.h"
#include "WSDOTAgentImp.h"
#include "CLSID.h"
#include <EAF\Broker.h>

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>

#include <EAF/EAFStatusCenter.h>
#include <IFace\DocumentType.h>
#include <EAF/EAFReportManager.h>

#include "GirderScheduleChapterBuilder.h"
#include "LoadRatingSummaryChapterBuilder.h"

// CWSDOTAgentImp

bool CWSDOTAgentImp::Init()
{
   EAF_AGENT_INIT;
   CREATE_LOGFILE("WSDOTAgent");

   // Maps old agent CLSID to new CLSID since version 3.0
   m_pBroker->AddMappedCLSID(_T("{338AD645-BAF2-41DC-964E-A9DFC8123253}"), _T("{B1A19633-8880-40BC-A3C9-DDF47F7F1844}"));


   // Register our reports
   GET_IFACE(IEAFReportManager,pRptMgr);

   //
   // Create report spec builders
   //
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker, CGirderKey(0, 0)));
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderLineRptSpecBuilder( std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder( std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

      // WSDOT Girder Schedule
      std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("WSDOT Girder Schedule")));
#if defined _DEBUG || defined _BETA_VERSION
      pRptBuilder->IncludeTimingChapter();
#endif
      pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker, pRptBuilder->GetName())));
      pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
      pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CGirderScheduleChapterBuilder>()) );
      pRptMgr->AddReportBuilder( pRptBuilder );
   }

   // WSDOT Load Rating Summary
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("WSDOT Load Rating Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->SetReportSpecificationBuilder( pGirderLineRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingSummaryChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   return true;
}

CLSID CWSDOTAgentImp::GetCLSID() const
{
   return CLSID_WSDOTAgent;
}

bool CWSDOTAgentImp::ShutDown()
{
   EAF_AGENT_SHUTDOWN;
   CLOSE_LOGFILE;
   return true;
}
