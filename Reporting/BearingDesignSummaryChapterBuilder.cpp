///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\BearingDesignSummaryChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\BearingRotationTable.h>
#include <Reporting\PrestressRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\BearingDesignParameters.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DistributionFactors.h>

#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBearingDesignParametersChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingDesignSummaryChapterBuilder::CBearingDesignSummaryChapterBuilder(bool bSelect) :
    CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingDesignSummaryChapterBuilder::GetName() const
{
    return TEXT("Bearing Design Summary");
}

rptChapter* CBearingDesignSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const
{
    auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
    CComPtr<IBroker> pBroker;
    pGirderRptSpec->GetBroker(&pBroker);
    const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

    // we want the final configuration... that would be in the last interval
    GET_IFACE2(pBroker, IIntervals, pIntervals);
    IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

    rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    GET_IFACE2(pBroker, IUserDefinedLoads, pUDL);
    bool are_user_loads = pUDL->DoUserLoadsExist(girderKey);

    GET_IFACE2(pBroker, IBearingDesign, pBearingDesign); ///////////////////////////////////////////////

    bool bIncludeImpact = pBearingDesign->BearingLiveLoadReactionsIncludeImpact();

    // Don't create much of report if no simple span ends
    std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(intervalIdx, girderKey);
    bool doFinalLoads = (0 < vPiers.size() ? true : false);

    GET_IFACE2(pBroker, IProductLoads, pProductLoads);
    bool bPedestrian = pProductLoads->HasPedestrianLoad();

    GET_IFACE2(pBroker, ISpecification, pSpec);

    rptParagraph* p = new rptParagraph;
    *pChapter << p;

    GET_IFACE2(pBroker, IBridge, pBridge);
    SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact, 
        true, true, true, pDisplayUnits, false, true) << rptNewLine;

    *p << CBearingRotationTable().BuildBearingRotationTable(pBroker, girderKey, pSpec->GetAnalysisType(), bIncludeImpact,
        true, true, true, pDisplayUnits, false, false) << rptNewLine;

    
    ///////////////////////////////////////
    //GET_IFACE2(pBroker, IMaterials, pMaterial);
    //pgsPointOfInterest poi;
    //PoiList vPoi;
    //poi = vPoi.front();
    //IndexType rgn = pPoi->GetDeckCastingRegion(poi);
    //CSegmentKey seg_key = pBridge->GetSegmentAtPier(startPierIdx, girderKey);
    //pPoi->GetPointsOfInterest(seg_key, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
    //IntervalIndexType cd_event = pIntervals->GetCastDeckInterval(rgn);
    //IntervalIndexType eg_event = pIntervals->GetErectSegmentInterval(seg_key);
    //IntervalIndexType release = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
    
    // CREEP //
    //Float64 cstrain1 = pMaterial->GetSegmentCreepCoefficient(
    //    seg_key,
    //    release,
    //    pgsTypes::IntervalTimeType::Start,
    //    eg_event, 
    //    pgsTypes::IntervalTimeType::Start
    //    );
    //
    //Float64 cstrain2 = pMaterial->GetSegmentCreepCoefficient(
    //    seg_key,
    //    release,
    //    pgsTypes::IntervalTimeType::Start,
    //    cd_event,
    //    pgsTypes::IntervalTimeType::Start
    //);

    // SHRINKAGE //
    //Float64 sstrain1 = pMaterial->GetTotalSegmentFreeShrinkageStrain(seg_key, eg_event, pgsTypes::IntervalTimeType::Start);
    //Float64 sstrain2 = pMaterial->GetTotalSegmentFreeShrinkageStrain(seg_key, cd_event, pgsTypes::IntervalTimeType::Start);


    return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder>CBearingDesignSummaryChapterBuilder::Clone() const
{
    return std::make_unique<CBearingDesignSummaryChapterBuilder>();
}
