///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// PGSuperReporterImp.cpp : Implementation of CPGSuperReporterImp
#include "stdafx.h"
#include "PGSuperReporterImp.h"

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>

#include <Reporting\OptimizedFabricationChapterBuilder.h>
#include <Reporting\DesignOutcomeChapterBuilder.h>
#include <Reporting\PrincipalTensionStressDetailsChapterBuilder.h>
#if defined _DEBUG
#include <Reporting\IntervalChapterBuilder.h> // for testing
#endif

#include <Reporting\ShrinkageStrainChapterBuilder.h>


// Interfaces
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\StatusCenter.h>

#include <IReportManager.h>

#include <psgLib/PrincipalTensionStressCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//CPGSuperReporterImp::InitReportBuilders
//
//Initialize report builders at project load time
//
//Initialize report builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CPGSuperReporterImp::InitReportBuilders()
{
   HRESULT hr = CReporterBase::InitCommonReportBuilders();
   if ( FAILED(hr) )
   {
      return hr;
   }

   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   // report spec builders are objects that present a dialog that is used to
   // define the report specification. The spec builder then creates a WBFL::Reporting::ReportSpecification objects
   // that is passed to all the chapter builders

   // this report spec builder prompts for span #, girder # and chapter list
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pSpanRptSpecBuilder(             std::make_shared<CSpanReportSpecificationBuilder>(m_pBroker) );
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiGirderRptSpecBuilder(    std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder(      std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder(         std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );

   CreateMultiGirderSpecCheckReport();

   CreateMultiHaunchGeometryReport();

   // Design Outcome
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Design Outcome Report"),true)); // hidden report
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDesignOutcomeChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Fabrication Options Report
   pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Fabrication Options Report"));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<COptimizedFabricationChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   return S_OK;
}

STDMETHODIMP CPGSuperReporterImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   CReporterBase::SetBroker(pBroker);
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperReporterImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface(IID_IReportOptions,this);

   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperReporterImp::Init()
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

STDMETHODIMP CPGSuperReporterImp::Init2()
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

STDMETHODIMP CPGSuperReporterImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_PGSuperReportAgent;
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperReporterImp::Reset()
{
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CPGSuperReporterImp::ShutDown()
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
HRESULT CPGSuperReporterImp::OnSpecificationChanged()
{
   HRESULT hr = CReporterBase::OnSpecificationChanged();
   if ( FAILED(hr) )
      return hr;

   // Show/Hide time-step analysis reports based on the loss method
   std::vector<std::_tstring> strReportNames;
   strReportNames.push_back(_T("Time Step Details Report"));

#if defined _DEBUG || defined _BETA_VERSION
   strReportNames.push_back(_T("(DEBUG) Interval by Interval Details Report"));
#endif // _DEBUG || _BETA_VERSION

   GET_IFACE(IReportManager,pRptMgr);
   GET_IFACE( ILossParameters, pLossParams);

   bool bTimeStep = pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP;
   bool bHidden = true;
   if ( bTimeStep )
   {
      bHidden = false;
   }

   for (const auto& strReportName : strReportNames)
   {
      std::vector<std::shared_ptr<WBFL::Reporting::ReportBuilder>> vRptBuilders;
      vRptBuilders.push_back( pRptMgr->GetReportBuilder(strReportName.c_str()) );

      std::vector<std::shared_ptr<WBFL::Reporting::ReportBuilder>>::iterator iter(vRptBuilders.begin());
      std::vector<std::shared_ptr<WBFL::Reporting::ReportBuilder>>::iterator end(vRptBuilders.end());
      for ( ; iter != end; iter++ )
      {
         std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(*iter);
         pRptBuilder->Hidden(bHidden);
      }
   }

   // Principal web stress report has two conditions
   bool bIsTimeStepPrincStress = false;
   if (bTimeStep)
   {
      GET_IFACE(ILibrary, pLib);
      GET_IFACE(ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();
      if (principal_tension_stress_criteria.Method == pgsTypes::ptsmNCHRP)
      {
         bIsTimeStepPrincStress = true;
      }
   }

   std::_tstring prinRepName(_T("Principal Web Stress Details Report"));
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pPsRptBuilder = pRptMgr->GetReportBuilder(_T("Principal Web Stress Details Report"));
   ATLASSERT(pPsRptBuilder);
   if (pPsRptBuilder != nullptr)
   {
      pPsRptBuilder->Hidden(!bIsTimeStepPrincStress);
   }

   // Add time-step chapters the details report
   
   // Update details report to contain a couple of extra chapters
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(_T("Details Report"));
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      auto pChBuilder = pRptBuilder->GetChapterBuilder(TEXT("Shrinkage Strain Details"));
      if (pChBuilder == nullptr)
      {
         // chapter wasn't previously added
         VERIFY(pRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(new CShrinkageStrainChapterBuilder), TEXT("Creep Coefficient Details")/*this is the name of the chapter after which the shrinkage strain chapter will be added*/));
      }
   }
   else
   {
      pRptBuilder->RemoveChapterBuilder(_T("Shrinkage Strain Details"));
   }


   Fire_ReportsChanged();
   

   return S_OK;
}

HRESULT CPGSuperReporterImp::OnAnalysisTypeChanged()
{
   return S_OK;
}

bool CPGSuperReporterImp::IncludeSpanAndGirder4Pois(const CGirderKey& girderKey)
{
   return girderKey.groupIndex == ALL_GROUPS;
}

WBFL::Reporting::TitlePageBuilder* CPGSuperReporterImp::CreateTitlePageBuilder(LPCTSTR strReportName,bool bFullVersion)
{
   return new CPGSuperTitlePageBuilder(m_pBroker,strReportName,bFullVersion);
}
