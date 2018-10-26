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

// ReporterImp.cpp : Implementation of CReporterImp
#include "stdafx.h"
#include "ReportAgent_i.h"
#include "ReporterImp.h"
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>

#include <Reporting\SpecCheckChapterBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>
#include <Reporting\BridgeDescDetailsChapterBuilder.h>
#include <Reporting\SectPropChapterBuilder.h>
#include <Reporting\SpanDataChapterBuilder.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\StressChapterBuilder.h>
#include <Reporting\PrestressForceChapterBuilder.h>
#include <Reporting\DevLengthDetailsChapterBuilder.h>
#include <Reporting\LossesChapterBuilder.h>
#include <Reporting\MomentCapacityDetailsChapterBuilder.h>
#include <Reporting\ShearCapacityDetailsChapterBuilder.h>
#include <Reporting\CritSectionChapterBuilder.h>
#include <Reporting\StirrupDetailingCheckChapterBuilder.h>
#include <Reporting\ADimChapterBuilder.h>
#include <Reporting\BurstingZoneDetailsChapterBuilder.h>
#include <Reporting\CreepCoefficientChapterBuilder.h>
#include <Reporting\CamberChapterBuilder.h>
#include <Reporting\LiftingCheckChapterBuilder.h>
#include <Reporting\LiftingCheckDetailsChapterBuilder.h>
#include <Reporting\HaulingCheckDetailsChapterBuilder.h>
#include <Reporting\HaulingCheckChapterBuilder.h>
#include <Reporting\LongReinfShearCheckChapterBuilder.h>
#include <Reporting\GirderComparisonChapterBuilder.h>
#include <Reporting\ProjectCriteriaChapterBuilder.h>
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\EffFlangeWidthDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>
#include <Reporting\CastingYardRebarRequirementChapterBuilder.h>
#include <Reporting\BridgeAnalysisChapterBuilder.h>
#include <Reporting\OptimizedFabricationChapterBuilder.h>
#include <Reporting\BearingDesignParametersChapterBuilder.h>

#include <Reporting\DesignOutcomeChapterBuilder.h>

#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\DeckElevationChapterBuilder.h>
#include <Reporting\PierGeometryChapterBuilder.h>
#include <Reporting\GirderGeometryChapterBuilder.h>

#include <Reporting\LoadRatingChapterBuilder.h>
#include <Reporting\LoadRatingDetailsChapterBuilder.h>
#include <Reporting\CrackedSectionDetailsChapterBuilder.h>
#include <Reporting\FinalLossesChapterBuilder.h>
#include <Reporting\LongitudinalReinforcementForShearLoadRatingChapterBuilder.h>

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

//CReporterImp::InitReportBuilders
//
//Initialize report builders at project load time
//
//Initialize report builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CReporterImp::InitReportBuilders()
{
   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   // report spec builders are objects that present a dialog that is used to
   // define the report specification. The spec builder then creates a CReportSpecification objects
   // that is passed to all the chapter builders

   // this report spec builder prompts for span #, girder # and chapter list
   boost::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder(     new CBrokerReportSpecificationBuilder(m_pBroker) );
   boost::shared_ptr<CReportSpecificationBuilder> pSpanRptSpecBuilder(       new CSpanReportSpecificationBuilder(m_pBroker) );
   boost::shared_ptr<CReportSpecificationBuilder> pSpanGirderRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(m_pBroker) );
   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(     new CGirderReportSpecificationBuilder(m_pBroker) );
   boost::shared_ptr<CReportSpecificationBuilder> pBridgeAnalysisRptSpecBuilder ( new CBridgeAnalysisReportSpecificationBuilder(m_pBroker) );
   boost::shared_ptr<CReportSpecificationBuilder> pLoadRatingRptSpecBuilder( new CLoadRatingReportSpecificationBuilder(m_pBroker) );


   // Design Outcome
   CReportBuilder* pRptBuilder = new CReportBuilder("Design Outcome Report",true); // hidden report
   //pRptBuilder->AddTitlePageBuilder(NULL); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDesignOutcomeChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Geometry Report
   pRptBuilder = new CReportBuilder("Bridge Geometry Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPierGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CGirderGeometryChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Details Report
   pRptBuilder = new CReportBuilder("Details Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpanDataChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CMVRChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CStressChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPrestressForceChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeDescDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CProjectCriteriaChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,false,true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDevLengthDetailsChapterBuilder) ); 
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLossesChapterBuilder) ); 
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCastingYardRebarRequirementChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CMomentCapacityDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CShearCapacityDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CStirrupDetailingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCritSectionChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLongReinfShearCheckChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSplittingZoneDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CEffFlangeWidthDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCreepCoefficientChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCamberChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CADimChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Bearing Design Report
   pRptBuilder = new CReportBuilder("Bearing Design Parameters Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBearingDesignParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Spec Check Report
   pRptBuilder = new CReportBuilder("Spec Check Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Load Rating Report
   pRptBuilder = new CReportBuilder("Load Rating Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadRatingChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadRatingDetailsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder(true)) );
   //pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpanDataChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CMVRChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CStressChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPrestressForceChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeDescDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CProjectCriteriaChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(false,true,true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(false,true,true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDevLengthDetailsChapterBuilder(false)) ); 
   //pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLossesChapterBuilder(false)) ); 
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CFinalLossesChapterBuilder(false)) ); 
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CMomentCapacityDetailsChapterBuilder(false,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CShearCapacityDetailsChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCritSectionChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLongReinfShearCheckChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CEffFlangeWidthDetailsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CCrackedSectionDetailsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLongitudinalReinforcementForShearLoadRatingChapterBuilder(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Girder Comparison Report
   pRptBuilder = new CReportBuilder("Girder Comparison Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CGirderComparisonChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Lifting Report
   pRptBuilder = new CReportBuilder("Lifting Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Hauling Report
   pRptBuilder = new CReportBuilder("Hauling Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Bridge Analysis Report
   pRptBuilder = new CReportBuilder("Bridge Analysis Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBridgeAnalysisRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder("Simple Span",pgsTypes::Simple)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder("Continuous Span",pgsTypes::Continuous)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder("Envelope of Simple/Continuous Spans",pgsTypes::Envelope)) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Fabrication Options Report
   pRptBuilder = new CReportBuilder("Fabrication Options Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new COptimizedFabricationChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Distribution Factors
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder = new CReportBuilder("Distribution Factors Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
#endif
//

   return S_OK;
}

STDMETHODIMP CReporterImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CReporterImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   // this agent doesn't implement any interfaces... it just provides reports

   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CReporterImp::Init()
{
   /* Gets done at project load time */
   AGENT_INIT;

   return InitReportBuilders();
}

STDMETHODIMP CReporterImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CReporterImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_ReportAgent;
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CReporterImp::Reset()
{
   return S_OK;
}

/*--------------------------------------------------------------------*/
STDMETHODIMP CReporterImp::ShutDown()
{
   AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}
