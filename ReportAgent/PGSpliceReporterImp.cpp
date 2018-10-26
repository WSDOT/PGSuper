///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// PGSpliceReporterImp.cpp : Implementation of CPGSpliceReporterImp
#include "stdafx.h"
#include "ReportAgent_i.h"
#include "PGSpliceReporterImp.h"
#include <Reporting\PGSpliceTitlePageBuilder.h>

#include <Reporting\IntervalChapterBuilder.h>
#include <Reporting\TendonGeometryChapterBuilder.h>

// Interfaces
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\StatusCenter.h>

#include <IReportManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//CPGSpliceReporterImp::InitReportBuilders
//
//Initialize report builders at project load time
//
//Initialize report builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CPGSpliceReporterImp::InitReportBuilders()
{
   HRESULT hr = CReporterBase::InitCommonReportBuilders();
   if ( FAILED(hr) )
   {
      return hr;
   }

   GET_IFACE(IReportManager,pRptMgr);

   // Update details report to contain a couple of extra chapters
   boost::shared_ptr<CReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(_T("Details Report"));
   VERIFY(pRptBuilder->InsertChapterBuilder(boost::shared_ptr<CChapterBuilder>(new CIntervalChapterBuilder),TEXT("Bridge Description")));
   VERIFY(pRptBuilder->InsertChapterBuilder(boost::shared_ptr<CChapterBuilder>(new CTendonGeometryChapterBuilder),TEXT("Section Properties")));

   return S_OK;
}

STDMETHODIMP CPGSpliceReporterImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   CReporterBase::SetBroker(pBroker);
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceReporterImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   // this agent doesn't implement any interfaces... it just provides reports

   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceReporterImp::Init()
{
   /* Gets done at project load time */
   EAF_AGENT_INIT;

   return InitReportBuilders();
}

STDMETHODIMP CPGSpliceReporterImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CPGSpliceReporterImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_PGSpliceReportAgent;
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceReporterImp::Reset()
{
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSpliceReporterImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}

CTitlePageBuilder* CPGSpliceReporterImp::CreateTitlePageBuilder(LPCTSTR strReportName,bool bFullVersion)
{
   return new CPGSpliceTitlePageBuilder(m_pBroker,strReportName,bFullVersion);
}
