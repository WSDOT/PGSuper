///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "PGSpliceReporterImp.h"
#include <Reporting\PGSpliceTitlePageBuilder.h>

#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>

#include <Reporting\SegmentTendonGeometryChapterBuilder.h>
#include <Reporting\GirderTendonGeometryChapterBuilder.h>
#include <Reporting\ShrinkageStrainChapterBuilder.h>

#include <Reporting\EquilibriumCheckReportSpecificationBuilder.h>
#include <Reporting\EquilibriumCheckChapterBuilder.h>
#include <Reporting\PrincipalTensionStressDetailsChapterBuilder.h>

//#include <Reporting\InitialStrainAnalysisReportSpecificationBuilder.h>
//#include <Reporting\InitialStrainAnalysisChapterBuilder.h>

#include <Reporting\TemporarySupportReactionChapterBuilder.h>

#include <Reporting\TemporarySupportElevationsChapterBuilder.h>
#include <Reporting\TemporarySupportElevationDetailsChapterBuilder.h>

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
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(_T("Details Report"));
   VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSegmentTendonGeometryChapterBuilder>()), TEXT("Section Properties")));
   VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CGirderTendonGeometryChapterBuilder>()),TEXT("Segment Tendon Geometry")));
   VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CShrinkageStrainChapterBuilder>()),TEXT("Creep Coefficient Details")));
   VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTemporarySupportElevationsChapterBuilder>()), TEXT("Bearing Seat Elevations")));
   VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTemporarySupportElevationDetailsChapterBuilder>()), TEXT("Bearing Seat Elevation Details")));

   pRptBuilder = pRptMgr->GetReportBuilder(_T("Bridge Geometry Report"));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTemporarySupportElevationsChapterBuilder>()));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTemporarySupportElevationDetailsChapterBuilder>()));


#if defined _DEBUG || defined _BETA_VERSION
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pEquilibriumCheckSpecBuilder(std::make_shared<CEquilibriumCheckReportSpecificationBuilder>(m_pBroker));
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pMyRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("(DEBUG) Equilibrium Check")));
   pMyRptBuilder->IncludeTimingChapter();
   pMyRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pMyRptBuilder->GetName())) );
   pMyRptBuilder->SetReportSpecificationBuilder( pEquilibriumCheckSpecBuilder );
   pMyRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CEquilibriumCheckChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pMyRptBuilder );
#endif

   //std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pInitialStrainAnalysisSpecBuilder(new CInitialStrainAnalysisReportSpecificationBuilder(m_pBroker));
   //pMyRptBuilder = new WBFL::Reporting::ReportBuilder(_T("Initial Strain Analysis"));
   //pMyRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pMyRptBuilder->GetName())) );
   //pMyRptBuilder->SetReportSpecificationBuilder( pInitialStrainAnalysisSpecBuilder );
   //pMyRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(new CInitialStrainAnalysisChapterBuilder) );
   //pRptMgr->AddReportBuilder( pMyRptBuilder );

   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pTSRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Temporary Support Reactions Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pTSRptBuilder->IncludeTimingChapter();
#endif
   pTSRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pTSRptBuilder->GetName())) );
   pTSRptBuilder->SetReportSpecificationBuilder( pRptSpecBuilder );
   pTSRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTemporarySupportReactionChapterBuilder>(true)) );
   pRptMgr->AddReportBuilder( pTSRptBuilder );

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

   HRESULT hr = InitReportBuilders();
   ATLASSERT(SUCCEEDED(hr));
   if ( FAILED(hr) )
   {
      return hr;
   }

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CPGSpliceReporterImp::Init2()
{
   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the specification
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

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
   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
//
HRESULT CPGSpliceReporterImp::OnSpecificationChanged()
{
   HRESULT hr = CReporterBase::OnSpecificationChanged();
   if ( FAILED(hr) )
      return hr;

   // Available reports and chapters for principal web stresses are dependent on settings
   GET_IFACE(IReportManager,pRptMgr);
   GET_IFACE(ISpecification, pSpec);

   CSegmentKey key(0, 0, 0); // any key should work for splice
   ISpecification::PrincipalWebStressCheckType  checkType = pSpec->GetPrincipalWebStressCheckType(key);

   bool bIsTimeStepPrincStress = ISpecification::pwcNCHRPTimeStepMethod == checkType;


   std::_tstring prinRepName(_T("Principal Web Stress Details Report"));
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pPsRptBuilder = pRptMgr->GetReportBuilder(_T("Principal Web Stress Details Report"));
   ATLASSERT(pPsRptBuilder);
   if (pPsRptBuilder != nullptr)
   {
      pPsRptBuilder->Hidden(!bIsTimeStepPrincStress);
   }

   return S_OK;
}

HRESULT CPGSpliceReporterImp::OnAnalysisTypeChanged()
{
   return S_OK;
}

WBFL::Reporting::TitlePageBuilder* CPGSpliceReporterImp::CreateTitlePageBuilder(LPCTSTR strReportName,bool bFullVersion)
{
   return new CPGSpliceTitlePageBuilder(m_pBroker,strReportName,bFullVersion);
}
