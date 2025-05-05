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

// PGSpliceReporterImp.cpp : Implementation of CPGSpliceReporterImp
#include "stdafx.h"
#include "ReporterBase.h"

#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>
#include <Reporting\TimelineManagerReportSpecificationBuilder.h>
#include <Reporting\CopyGirderPropertiesReportSpecificationBuilder.h>
#include <Reporting\CopyPierPropertiesReportSpecificationBuilder.h>
#include <Reporting\CopyTempSupportPropertiesReportSpecificationBuilder.h>

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
#include <Reporting\SplittingCheckDetailsChapterBuilder.h>
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
#include <Reporting\CopyGirderPropertiesChapterBuilder.h>
#include <Reporting\CopyPierPropertiesChapterBuilder.h>
#include <Reporting\CopyTempSupportPropertiesChapterBuilder.h>

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

#include <Reporting\GirderTendonGeometryChapterBuilder.h>
#include <Reporting\SegmentTendonGeometryChapterBuilder.h>
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\BearingDesignSummaryChapterBuilder.h>
#include <Reporting\BearingDesignDetailsChapterBuilder.h>
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>

#include <Reporting\TimeStepDetailsChapterBuilder.h>
#include <Reporting\TimeStepDetailsReportSpecificationBuilder.h>

#include <Reporting\BearingTimeStepDetailsChapterBuilder.h>
#include <Reporting\BearingTimeStepDetailsReportSpecificationBuilder.h>

#include <Reporting\PrincipalWebStressDetailsChapterBuilder.h>
#include <Reporting\PrincipalWebStressDetailsReportSpecificationBuilder.h>

#include <Reporting\PointOfInterestChapterBuilder.h>

#include <Reporting\DistributionFactorSummaryChapterBuilder.h>

#include <Reporting\InternalForceChapterBuilder.h>

#include <Reporting\MultiGirderHaunchGeometryChapterBuilder.h>

#include <Reporting\PierReactionChapterBuilder.h>

#include <Reporting\PrincipalTensionStressDetailsChapterBuilder.h>

#include <Reporting\MomentCapacityReportSpecificationBuilder.h>
#include <Reporting\MomentCapacityChapterBuilder.h>

#include <Reporting\CrackedSectionReportSpecificationBuilder.h>
#include <Reporting\CrackedSectionChapterBuilder.h>

#include <IFace\Tools.h>
#include <IFace\Project.h>


HRESULT CReporterBase::InitCommonReportBuilders(std::shared_ptr<WBFL::EAF::Broker> broker)
{
   GET_IFACE2_NOCHECK(broker, IEAFReportManager, pRptMgr);

   CreateBridgeGeometryReport(pRptMgr);
   CreateDetailsReport(pRptMgr);
   CreateLoadRatingReport(pRptMgr);
   CreateLoadRatingSummaryReport(pRptMgr);
   CreateBearingDesignReport(pRptMgr);
   CreateBearingTimeStepDetailsReport(pRptMgr);
   CreateBridgeAnalysisReport(pRptMgr);
   CreateHaulingReport(pRptMgr);
   CreateLiftingReport(pRptMgr);
   CreateSpecChecReport(pRptMgr);
   CreateDistributionFactorSummaryReport(pRptMgr);
   CreateTimeStepDetailsReport(pRptMgr);
   CreatePrincipalWebStressDetailsReport(pRptMgr);
   CreatePierReactionsReport(pRptMgr);
   CreateTimelineReport(pRptMgr);
   CreateCopyGirderPropertiesReport(pRptMgr);
   CreateCopyPierPropertiesReport(pRptMgr);
   CreateCopyTempSupportPropertiesReport(pRptMgr);

#if defined _DEBUG || defined _BETA_VERSION
   // these are just some testing/debugging reports
   CreateDistributionFactorsReport(pRptMgr);
   CreateStageByStageDetailsReport(pRptMgr);
   CreatePointOfInterestReport(pRptMgr);
#endif

   CreateMomentCapacityDetailsReport(pRptMgr);
   CreateCrackedSectionDetailsReport(pRptMgr);

   return S_OK;
}

void CReporterBase::CreateBridgeGeometryReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pBrokerRptSpecBuilder(  std::make_shared<CBrokerReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Bridge Geometry Report")));
   pRptBuilder->EnableHeadingNumbers(true);

#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->SetReportSpecificationBuilder(pBrokerRptSpecBuilder);
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CAlignmentChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDeckElevationChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPierGeometryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CGirderGeometryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingSeatElevationsChapterBuilder2>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingSeatElevationsDetailsChapterBuilder2>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder(std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Details Report")));
   pRptBuilder->EnableHeadingNumbers(true);
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CAlignmentChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDeckElevationChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingSeatElevationsChapterBuilder2>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeDescChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTimelineChapterBuilder>()));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CIntervalChapterBuilder>()));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpanDataChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CUserDefinedLoadsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMVRChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CStressChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPrestressForceChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckSummaryChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeDescDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CProjectCriteriaChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiveLoadDetailsChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDevLengthDetailsChapterBuilder>()) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLossesChapterBuilder>()) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCastingYardRebarRequirementChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMomentCapacityDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CShearCapacityDetailsChapterBuilder>(true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CStirrupDetailingCheckChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCritSectionChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLongReinfShearCheckChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CHorizontalInterfaceShearCapacityDetailsChapterBuilder>(true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPrincipalTensionStressDetailsChapterBuilder>()));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSplittingCheckDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CEffFlangeWidthDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDistributionFactorDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCreepCoefficientChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCamberChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CADimChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingSeatElevationsDetailsChapterBuilder2>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDesignDetailsChapterBuilder>()));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDesignParametersChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiftingCheckDetailsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CHaulingCheckDetailsChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateLoadRatingReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pLoadRatingRptSpecBuilder(std::make_shared<CLoadRatingReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Load Rating Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingDetailsChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLongitudinalReinforcementForShearLoadRatingChapterBuilder>(true)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingReactionsChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CAlignmentChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeDescChapterBuilder>(true)) );
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpanDataChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CUserDefinedLoadsChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMVRChapterBuilder>(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CStressChapterBuilder>(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPrestressForceChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeDescDetailsChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CProjectCriteriaChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiveLoadDetailsChapterBuilder>(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDevLengthDetailsChapterBuilder>(false)) ); 
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLossesChapterBuilder>(false)) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CFinalLossesChapterBuilder>(false)) ); 
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMomentCapacityDetailsChapterBuilder>(false,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CShearCapacityDetailsChapterBuilder>(false, true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCritSectionChapterBuilder>(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLongReinfShearCheckChapterBuilder>(false,true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CHorizontalInterfaceShearCapacityDetailsChapterBuilder>(false, true, false)));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CEffFlangeWidthDetailsChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDistributionFactorDetailsChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCrackedSectionDetailsChapterBuilder>(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateLoadRatingSummaryReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pLoadRatingRptSpecBuilder(std::make_shared<CLoadRatingSummaryReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Load Rating Summary Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingDetailsChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadRatingReactionsChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CAlignmentChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeDescChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(false,true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiveLoadDetailsChapterBuilder>(false,true,true)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateBearingDesignReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Bearing Design Parameters Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDesignSummaryChapterBuilder>()));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDesignDetailsChapterBuilder>()));
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDesignParametersChapterBuilder>()));
   pRptMgr->AddReportBuilder( pRptBuilder );
}


void CReporterBase::CreateSpecChecReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Spec Check Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckSummaryChapterBuilder>(false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateMultiGirderSpecCheckReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Multi-Girder Spec Check Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckSummaryChapterBuilder>(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateLiftingReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pSegmentRptSpecBuilder(std::make_shared<CSegmentReportSpecificationBuilder>(m_pBroker, CSegmentKey(0, 0, 0)));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Lifting Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder(pSegmentRptSpecBuilder);
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiftingCheckChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiftingCheckDetailsChapterBuilder>()) );
   pRptMgr->AddReportBuilder(pRptBuilder );
}

void CReporterBase::CreateHaulingReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pSegmentRptSpecBuilder(std::make_shared<CSegmentReportSpecificationBuilder>(m_pBroker, CSegmentKey(0, 0, 0)));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Hauling Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder(pSegmentRptSpecBuilder);
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CHaulingCheckChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CHaulingCheckDetailsChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateBridgeAnalysisReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pBridgeAnalysisRptSpecBuilder( std::make_shared<CBridgeAnalysisReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Bridge Analysis Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBridgeAnalysisRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeAnalysisChapterBuilder>(_T("Simple Span"),pgsTypes::Simple)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeAnalysisChapterBuilder>(_T("Continuous Span"),pgsTypes::Continuous)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBridgeAnalysisChapterBuilder>(_T("Envelope of Simple/Continuous Spans"),pgsTypes::Envelope)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateDistributionFactorSummaryReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pBrokerRptSpecBuilder( std::make_shared<CBrokerReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Live Load Distribution Factors Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDistributionFactorSummaryChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

#if defined _DEBUG || defined _BETA_VERSION
void CReporterBase::CreateDistributionFactorsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder(std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("(DEBUG) Live Load Distribution Factors Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CDistributionFactorDetailsChapterBuilder>()) );
   //pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}
#endif

void CReporterBase::CreateStageByStageDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder(  std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("(DEBUG) Interval by Interval Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CIntervalChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSegmentTendonGeometryChapterBuilder>() ));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CGirderTendonGeometryChapterBuilder>() ));
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTimeStepParametersChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateTimeStepDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pTimeStepRptSpecBuilder(  std::make_shared<CTimeStepDetailsReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Time Step Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pTimeStepRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTimeStepDetailsChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateBearingTimeStepDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
    std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pBearingTimeStepRptSpecBuilder(std::make_shared<CBearingTimeStepDetailsReportSpecificationBuilder>(m_pBroker));

    std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Bearing Shear Deformation Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
    pRptBuilder->IncludeTimingChapter();
#endif
    pRptBuilder->AddTitlePageBuilder(std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())));
    pRptBuilder->SetReportSpecificationBuilder(pBearingTimeStepRptSpecBuilder);
    pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingTimeStepDetailsChapterBuilder>()));
    pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreatePrincipalWebStressDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pPoiRptSpecBuilder(  std::make_shared<CPrincipalWebStressDetailsReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Principal Web Stress Details Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pPoiRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPrincipalWebStressDetailsChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreatePointOfInterestReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder(  std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder( std::make_shared<WBFL::Reporting::ReportBuilder>(_T("(DEBUG) Points of Interest Report")) );
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPointOfInterestChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateMultiHaunchGeometryReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Multi-Girder Haunch Geometry Summary")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMultiGirderHaunchGeometryChapterBuilder>(true)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreatePierReactionsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CGirderLineReportSpecificationBuilder>(m_pBroker) );

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Pier Reactions Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CPierReactionChapterBuilder>(true)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateTimelineReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CTimelineManagerReportSpecificationBuilder>(m_pBroker));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Timeline Manager Report"), true)); // hidden report
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder(pRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTimelineChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreateCopyGirderPropertiesReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CCopyGirderPropertiesReportSpecificationBuilder>(m_pBroker));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Copy Girder Properties Report"), true)); // hidden report
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder(pRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCopyGirderPropertiesChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreateCopyPierPropertiesReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CCopyPierPropertiesReportSpecificationBuilder>(m_pBroker));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Copy Pier Properties Report"), true)); // hidden report
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder(pRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCopyPierPropertiesChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreateCopyTempSupportPropertiesReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder(std::make_shared<CCopyTempSupportPropertiesReportSpecificationBuilder>(m_pBroker));

   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Copy Temporary Support Properties Report"), true)); // hidden report
   //pRptBuilder->AddTitlePageBuilder(nullptr); // no title page for this report
   pRptBuilder->SetReportSpecificationBuilder(pRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCopyTempSupportPropertiesChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreateMomentCapacityDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Moment Capacity Details Report"));
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMomentCapacityRptSpecBuilder(std::make_shared<CMomentCapacityReportSpecificationBuilder>(m_pBroker));
   pRptBuilder->AddTitlePageBuilder(std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())));
   pRptBuilder->SetReportSpecificationBuilder(pMomentCapacityRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CMomentCapacityChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

void CReporterBase::CreateCrackedSectionDetailsReport(std::shared_ptr<IEAFReportManager> pRptMgr)
{
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("Cracked Section Analysis Details Report"));
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pCrackedSectionRptSpecBuilder(std::make_shared<CCrackedSectionReportSpecificationBuilder>(m_pBroker));
   pRptBuilder->AddTitlePageBuilder(std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())));
   pRptBuilder->SetReportSpecificationBuilder(pCrackedSectionRptSpecBuilder);
   pRptBuilder->AddChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CCrackedSectionChapterBuilder>()));
   pRptMgr->AddReportBuilder(pRptBuilder);
}

HRESULT CReporterBase::OnSpecificationChanged(std::shared_ptr<WBFL::EAF::Broker> broker)
{
   GET_IFACE2(broker,IEAFReportManager,pRptMgr);
   std::shared_ptr<WBFL::Reporting::ReportBuilder> detailsRptBuilder  = pRptMgr->GetReportBuilder(_T("Details Report"));
   std::shared_ptr<WBFL::Reporting::ReportBuilder> loadRatingRptBuilder = pRptMgr->GetReportBuilder(_T("Load Rating Report"));

   GET_IFACE2(broker, ILossParameters, pLossParams);
   bool is_timestep = pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP;
   if ( is_timestep )
   {
      detailsRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CInternalForceChapterBuilder>()), _T("Moments, Shears, and Reactions"));
      loadRatingRptBuilder->InsertChapterBuilder(std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CInternalForceChapterBuilder>(false)), _T("Moments, Shears, and Reactions"));
   }
   else
   {
      detailsRptBuilder->RemoveChapterBuilder(_T("Internal Time-Dependent Forces"));
      loadRatingRptBuilder->RemoveChapterBuilder(_T("Internal Time-Dependent Forces"));
   }

   // Disable bearing elevations chapters in geometry report for time step. They take too long 
   std::shared_ptr<WBFL::Reporting::ReportBuilder> brgGeomRptBuilder = pRptMgr->GetReportBuilder(_T("Bridge Geometry Report"));
   auto pbsChap = brgGeomRptBuilder->GetChapterBuilder(TEXT("Bearing Seat Elevations"));
   auto pbsPgChap = std::static_pointer_cast<CPGSuperChapterBuilder>(pbsChap);
   pbsPgChap->SetSelect(!is_timestep);

   auto pbsdetChap = brgGeomRptBuilder->GetChapterBuilder(TEXT("Bearing Seat Elevation Details"));
   auto pbsdetPgChap = std::static_pointer_cast<CPGSuperChapterBuilder>(pbsdetChap);
   pbsdetPgChap->SetSelect(!is_timestep);

   return S_OK;
}
