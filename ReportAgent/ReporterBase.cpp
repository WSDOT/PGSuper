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
#include "ReporterBase.h"

#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>

#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\DeckElevationChapterBuilder.h>
#include <Reporting\PierGeometryChapterBuilder.h>
#include <Reporting\GirderGeometryChapterBuilder.h>

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
#include <Reporting\LongReinfShearCheckChapterBuilder.h>
#include <Reporting\ProjectCriteriaChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\EffFlangeWidthDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>
#include <Reporting\CastingYardRebarRequirementChapterBuilder.h>

#include <Reporting\LoadRatingChapterBuilder.h>
#include <Reporting\LoadRatingDetailsChapterBuilder.h>
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
#include <Reporting\TendonGeometryChapterBuilder.h>
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>

#include <Reporting\PointOfInterestChapterBuilder.h>

#include <Reporting\DistributionFactorSummaryChapterBuilder.h>

#include <IReportManager.h>

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
   CreateBearingDesignReport();
   CreateBridgeAnalysisReport();
   CreateHaulingReport();
   CreateLiftingReport();
   CreateMultiGirderSpecCheckReport();
   CreateSpecChecReport();
   CreateDistributionFactorSummaryReport();

#if defined _DEBUG || defined _BETA_VERSION
   CreateDistributionFactorsReport();
#endif

   // these are just some testing/debugging reports
   CreateStageByStageDetailsReport();
   CreatePointOfInterestReport();
   return S_OK;
}

void CReporterBase::CreateBridgeGeometryReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   
   boost::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder(  new CBrokerReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Bridge Geometry Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPierGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CGirderGeometryChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateDetailsReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(new CGirderReportSpecificationBuilder(m_pBroker,CGirderKey(0,0)));

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Details Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDeckElevationChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeDescChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpanDataChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CMVRChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CStressChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPrestressForceChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
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
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBearingDesignParametersChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateLoadRatingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pLoadRatingRptSpecBuilder(new CLoadRatingReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Load Rating Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pLoadRatingRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadRatingChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadRatingDetailsChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CAlignmentChapterBuilder) );
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
}

void CReporterBase::CreateBearingDesignReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(new CMultiViewSpanGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Bearing Design Parameters Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBearingDesignParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateSpecChecReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(new CMultiViewSpanGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Spec Check Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateMultiGirderSpecCheckReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiGirderRptSpecBuilder( new CMultiGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Multi-Girder Spec Check Summary"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateLiftingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(new CMultiViewSpanGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Lifting Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiftingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateHaulingReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(new CMultiViewSpanGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Hauling Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CHaulingCheckDetailsChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateBridgeAnalysisReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pBridgeAnalysisRptSpecBuilder( new CBridgeAnalysisReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Bridge Analysis Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBridgeAnalysisRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Simple Span"),pgsTypes::Simple)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Continuous Span"),pgsTypes::Continuous)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CBridgeAnalysisChapterBuilder(_T("Envelope of Simple/Continuous Spans"),pgsTypes::Envelope)) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreateDistributionFactorSummaryReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pBrokerRptSpecBuilder( new CBrokerReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Live Load Distribution Factors Summary"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pBrokerRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDistributionFactorSummaryChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

#if defined _DEBUG || defined _BETA_VERSION
void CReporterBase::CreateDistributionFactorsReport()
{
   GET_IFACE(IReportManager,pRptMgr);

   boost::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder(new CMultiViewSpanGirderReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Distribution Factors Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CDistributionFactorDetailsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}
#endif

void CReporterBase::CreateStageByStageDetailsReport()
{
#pragma Reminder("UPDATE: this is a report for development purposes")
   // either flesh out this report or eliminate it

   GET_IFACE(IReportManager,pRptMgr);
   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  new CGirderReportSpecificationBuilder(m_pBroker,CGirderKey(0,0)) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Stage by Stage Details Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CIntervalChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTendonGeometryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTimeStepParametersChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}

void CReporterBase::CreatePointOfInterestReport()
{
   GET_IFACE(IReportManager,pRptMgr);
   boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder(  new CGirderLineReportSpecificationBuilder(m_pBroker) );

   CReportBuilder* pRptBuilder = new CReportBuilder(_T("Points of Interest Report"));
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(CreateTitlePageBuilder(pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CPointOfInterestChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );
}
