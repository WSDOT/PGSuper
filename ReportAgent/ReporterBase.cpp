///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include "ReporterBase.h"

#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>

#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\DeckElevationChapterBuilder.h>
#include <Reporting\PierGeometryChapterBuilder.h>
#include <Reporting\GirderGeometryChapterBuilder.h>
#include "Reporting\BearingSeatElevationsChapterBuilder2.h"

#include <Reporting\BridgeDescChapterBuilder.h>
#include <Reporting\BridgeDescDetailsChapterBuilder.h>
#include <Reporting\TimelineChapterBuilder.h>
#include <Reporting\IntervalChapterBuilder.h>
#include <Reporting\SectPropChapterBuilder.h>
#include <Reporting\SpanDataChapterBuilder.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\StressChapterBuilder.h>
#include <Reporting\PrestressForceChapterBuilder.h>
#include <Reporting\DevLengthDetailsChapterBuilder.h>
#include <Reporting\LossesChapterBuilder.h>
#include <Reporting\MomentCapacityDetailsChapterBuilder.h>
#include <Reporting\ShearCapacityDetailsChapterBuilder.h>
#include <Reporting\HorizontalInterfaceShearCapacityDetailsChapterBuilder.h>
#include <Reporting\CritSectionChapterBuilder.h>
#include <Reporting\StirrupDetailingCheckChapterBuilder.h>
#include <Reporting\ADimChapterBuilder.h>
#include <Reporting\BurstingZoneDetailsChapterBuilder.h>
#include <Reporting\CreepCoefficientChapterBuilder.h>
#include <Reporting\CamberChapterBuilder.h>
#include <Reporting\LongReinfShearCheckChapterBuilder.h>
#include <Reporting\ProjectCriteriaChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\EffFlangeWidthDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>
#include <Reporting\CastingYardRebarRequirementChapterBuilder.h>
#include <Reporting\BearingSeatElevationsDetailsChapterBuilder2.h>

#include <Reporting\LoadRatingChapterBuilder.h>
#include <Reporting\LoadRatingDetailsChapterBuilder.h>
#include <Reporting\LoadRatingReactionsChapterBuilder.h>
#include <Reporting\CrackedSectionDetailsChapterBuilder.h>
#include <Reporting\FinalLossesChapterBuilder.h>
#include <Reporting\LongitudinalReinforcementForShearLoadRatingChapterBuilder.h>
#include <Reporting\SpecCheckChapterBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>
#include <Reporting\LiftingCheckChapterBuilder.h>
#include <Reporting\LiftingCheckDetailsChapterBuilder.h>
#include <Reporting\HaulingCheckDetailsChapterBuilder.h>
#include <Reporting\HaulingCheckChapterBuilder.h>
#include <Reporting\BridgeAnalysisChapterBuilder.h>

#include <Reporting\IntervalChapterBuilder.h>
#include <Reporting\GirderTendonGeometryChapterBuilder.h>
#include <Reporting\SegmentTendonGeometryChapterBuilder.h>
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>

#include <Reporting\TimeStepDetailsChapterBuilder.h>
#include <Reporting\TimeStepDetailsReportSpecificationBuilder.h>

#include <Reporting\PointOfInterestChapterBuilder.h>

#include <Reporting\DistributionFactorSummaryChapterBuilder.h>

#include <Reporting\InternalForceChapterBuilder.h>

#include <Reporting\MultiGirderHaunchGeometryChapterBuilder.h>

#include <Reporting\PierReactionChapterBuilder.h>

#include <Reporting\PrincipalTensionStressDetailsChapterBuilder.h>

#include <IReportManager.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CReporterBase::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

HRESULT CReporterBase::InitCommonReportBuilders()
{
   CreateBridgeGeometryReport();
   CreateDetailsReport();
   CreateLoadRatingReport();
   CreateLoadRatingSummaryReport();
   CreateBearingDesignReport();
   CreateBridgeAnalysisReport();
   CreateHaulingReport();
   CreateLiftingReport();
   CreateSpecChecReport();
   CreateDistributionFactorSummaryReport();
   CreateTimeStepDetailsReport();
   CreatePierReactionsReport();
   CreateTimelineReport();

#if defined _DEBUG || defined _BETA_VERSION
   // these are just some testing/debugging reports
   CreateDistributionFactorsReport();
   CreateStageByStageDetailsReport();
   CreatePointOfInterestReport();
#endif

   return S_OK;
}

void CReporterBase::CreateBridgeGeometryReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   
   std::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder(  std::make_shared<CBrokerReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Bridge Geometry Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPierGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CGirderGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingSeatElevationsChapterBuilder2) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingSeatElevationsDetailsChapterBuilder2) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateDetailsReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)));

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingSeatElevationsChapterBuilder2) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTimelineChapterBuilder));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CIntervalChapterBuilder));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpanDataChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMVRChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CStressChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPrestressForceChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeDescDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CProjectCriteriaChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDevLengthDetailsChapterBuilder) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLossesChapterBuilder) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCastingYardRebarRequirementChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMomentCapacityDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CShearCapacityDetailsChapterBuilder(true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CHorizontalInterfaceShearCapacityDetailsChapterBuilder(true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CStirrupDetailingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCritSectionChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLongReinfShearCheckChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPrincipalTensionStressDetailsChapterBuilder));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSplittingZoneDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CEffFlangeWidthDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCreepCoefficientChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCamberChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CADimChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingSeatElevationsDetailsChapterBuilder2) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingDesignParametersChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateLoadRatingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pLoadRatingRptSpecBuilder(std::make_shared<CLoadRatingReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Load Rating Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingDetailsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingReactionsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLongitudinalReinforcementForShearLoadRatingChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder(true)) );
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpanDataChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMVRChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CStressChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPrestressForceChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeDescDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CProjectCriteriaChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDevLengthDetailsChapterBuilder(false)) ); 
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLossesChapterBuilder(false)) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CFinalLossesChapterBuilder(false)) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMomentCapacityDetailsChapterBuilder(false,false)) );
   pRptBuilder->AddChapterBuilder(std::shared_ptr<CChapterBuilder>(new CShearCapacityDetailsChapterBuilder(false, true, false)));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<CChapterBuilder>(new CHorizontalInterfaceShearCapacityDetailsChapterBuilder(false, true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCritSectionChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLongReinfShearCheckChapterBuilder(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CEffFlangeWidthDetailsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CCrackedSectionDetailsChapterBuilder(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateLoadRatingSummaryReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pLoadRatingRptSpecBuilder(std::make_shared<CLoadRatingSummaryReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Load Rating Summary Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingDetailsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadRatingReactionsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(false,true,true)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateBearingDesignReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Bearing Design Parameters Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingDesignParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateSpecChecReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Spec Check Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateMultiGirderSpecCheckReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Multi-Girder Spec Check Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateLiftingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Lifting Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiftingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder(pRptBuilder.release() );
}

void CReporterBase::CreateHaulingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Hauling Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CHaulingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateBridgeAnalysisReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pBridgeAnalysisRptSpecBuilder( std::make_shared<CBridgeAnalysisReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Bridge Analysis Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBridgeAnalysisRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Simple Span"),pgsTypes::Simple)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Continuous Span"),pgsTypes::Continuous)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Envelope of Simple/Continuous Spans"),pgsTypes::Envelope)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateDistributionFactorSummaryReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder( std::make_shared<CBrokerReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Live Load Distribution Factors Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDistributionFactorSummaryChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

#if defined _DEBUG || defined _BETA_VERSION
void CReporterBase::CreateDistributionFactorsReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("(DEBUG) Live Load Distribution Factors Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder) );
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}
#endif

void CReporterBase::CreateStageByStageDetailsReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("(DEBUG) Interval by Interval Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CIntervalChapterBuilder) );
   pRptBuilder->AddChapterBuilder(std::shared_ptr<CChapterBuilder>(new CSegmentTendonGeometryChapterBuilder));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<CChapterBuilder>(new CGirderTendonGeometryChapterBuilder));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTimeStepParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateTimeStepDetailsReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> pPoiRptSpecBuilder(  std::make_shared<CTimeStepDetailsReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Time Step Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pPoiRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTimeStepDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreatePointOfInterestReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder( std::make_unique<CReportBuilder>(_T("(DEBUG) Points of Interest Report")) );
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPointOfInterestChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateMultiHaunchGeometryReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Multi-Girder Haunch Geometry Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CMultiGirderHaunchGeometryChapterBuilder(true)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreatePierReactionsReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Pier Reactions Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CPierReactionChapterBuilder(true)) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );
}

void CReporterBase::CreateTimelineReport()
{
   GET_IFACE(IReportManager, pRptMgr);

   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CBrokerReportSpecificationBuilder>(m_pBroker));

   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("Timeline Manager Report"), true)); // hidden report
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder(pRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<CChapterBuilder>(new CTimelineChapterBuilder));
   pRptMgr->AddReportBuilder(pRptBuilder.release());
}

HRESULT CReporterBase::OnSpecificationChanged()
{
   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportBuilder> detailsRptBuilder  = pRptMgr->GetReportBuilder(_T("Details Report"));
   std::shared_ptr<CReportBuilder> loadRatingRptBuilder = pRptMgr->GetReportBuilder(_T("Load Rating Report"));

   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      detailsRptBuilder->InsertChapterBuilder(std::shared_ptr<CChapterBuilder>(new CInternalForceChapterBuilder()), _T("Moments, Shears, and Reactions"));
      loadRatingRptBuilder->InsertChapterBuilder(std::shared_ptr<CChapterBuilder>(new CInternalForceChapterBuilder(false)), _T("Moments, Shears, and Reactions"));
   }
   else
   {
      detailsRptBuilder->RemoveChapterBuilder(_T("Internal Time-Dependent Forces"));
      loadRatingRptBuilder->RemoveChapterBuilder(_T("Internal Time-Dependent Forces"));
   }

   return S_OK;
}
