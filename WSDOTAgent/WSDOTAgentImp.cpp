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

// WSDOTAgentImp.cpp : Implementation of CWSDOTAgentImp

#include "stdafx.h"
#include "WSDOTAgentImp.h"

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>

#include <IFace\StatusCenter.h>
#include <IReportManager.h>

#include "GirderScheduleChapterBuilder.h"
#include "InputSummaryChapter.h"
#include "OutputSummaryChapter.h"

DECLARE_LOGFILE;

// CWSDOTAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgentEx
//
STDMETHODIMP CWSDOTAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
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

   AGENT_INIT;

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

   boost::shared_ptr<CReportSpecificationBuilder> pSpanGirderRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(m_pBroker) );

   // WSDOT Girder Schedule
   CReportBuilder* pRptBuilder = new CReportBuilder("WSDOT Girder Schedule");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CGirderScheduleChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // WSDOT Summary Report
   pRptBuilder = new CReportBuilder("WSDOT Summary Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(true)) ); // may have to move this chapter to a common DLL
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CInputSummaryChapter) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new COutputSummaryChapter) );
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
   AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}
