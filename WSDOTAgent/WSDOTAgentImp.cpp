///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>

#include <IFace\StatusCenter.h>
#include <IFace\DocumentType.h>
#include <IReportManager.h>

#include "GirderScheduleChapterBuilder.h"
#include "LoadRatingSummaryChapterBuilder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CWSDOTAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgentEx
//
STDMETHODIMP CWSDOTAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);

   CComQIPtr<ICLSIDMap> clsidMap(pBroker);
   clsidMap->AddCLSID(CComBSTR("{338AD645-BAF2-41DC-964E-A9DFC8123253}"),CComBSTR("{B1A19633-8880-40BC-A3C9-DDF47F7F1844}"));

   return S_OK;
}

STDMETHODIMP CWSDOTAgentImp::RegInterfaces()
{
   //CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   // This object doesn't implement any new interfaces... it just provides reports
   return S_OK;
}

STDMETHODIMP CWSDOTAgentImp::Init()
{
   CREATE_LOGFILE("WSDOTAgent");

   EAF_AGENT_INIT;

   // We are going to add new reports to PGSuper. In order to do this, the agent that implements
   // IReportManager must be loaded. We have no way of knowing if that agent is loaded before
   // or after us. Request the broker call our Init2 function after all registered agents
   // are loaded
   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CWSDOTAgentImp::Init2()
{
   // Register our reports
   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );
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
      pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName())) );
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

   return S_OK;
}

STDMETHODIMP CWSDOTAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_WSDOTAgent;
   return S_OK;
}

STDMETHODIMP CWSDOTAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CWSDOTAgentImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}
