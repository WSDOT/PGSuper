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

#include <algorithm>
#include "StdAfx.h"
#include "BearingDesignEngineer.h"
#include <Units\Convert.h>
#include <PGSuperException.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\ShearCapacity.h>
#include <IFace\PrestressForce.h> 
#include <IFace\MomentCapacity.h>
#include <IFace\StatusCenter.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\DistributionFactors.h>
#include <IFace\RatingSpecification.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace/Limits.h>
#include <LRFD\Rebar.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/LimitStateConcreteStrengthCriteria.h>

#include <PgsExt\statusitem.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\GirderLabel.h>



#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsBearingDesignEngineer
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsBearingDesignEngineer::pgsBearingDesignEngineer(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

//======================== OPERATIONS =======================================
void pgsBearingDesignEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}


void pgsBearingDesignEngineer::GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const
{
}


Float64 pgsBearingDesignEngineer::BearingSkewFactor(const ReactionLocation& reactionLocation, bool isFlexural) const
{
    GET_IFACE(IBridge, pBridge);

    //skew factor
    CComPtr<IAngle> pSkew;
    pBridge->GetPierSkew(reactionLocation.PierIdx, &pSkew);
    Float64 skew;
    pSkew->get_Value(&skew);

    Float64 skewFactor;

    if (isFlexural)
    {
        skewFactor = 1.0;
    }
    else
    {
        skewFactor = tan(skew);
    }

    return skewFactor;
}


void pgsBearingDesignEngineer::GetBearingTableParameters(const CGirderKey& girderKey, TABLEPARAMETERS* pDetails) const
{
    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IUserDefinedLoadData, pUserLoads);

    pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

    pDetails->bDeck = false;
    if (deckType != pgsTypes::sdtNone)
    {
        pDetails->bDeck = true;
    }

    pDetails->bHasOverlay = pBridge->HasOverlay();
    pDetails->bFutureOverlay = pBridge->IsFutureOverlay();

    pDetails->bDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);

    pDetails->startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
    pDetails->endGroup = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : pDetails->startGroup);

    CGirderKey key(pDetails->startGroup, girderKey.girderIndex);
    GET_IFACE(IProductLoads, pLoads);
    pDetails->bSegments = (1 < pBridge->GetSegmentCount(key) ? true : false);
    pDetails->bPedLoading = pLoads->HasPedestrianLoad(key);
    pDetails->bSidewalk = pLoads->HasSidewalkLoad(key);
    pDetails->bShearKey = pLoads->HasShearKeyLoad(key);
    pDetails->bLongitudinalJoint = pLoads->HasLongitudinalJointLoad();
    pDetails->bConstruction = !IsZero(pUserLoads->GetConstructionLoad());

    GET_IFACE(IGirderTendonGeometry, pTendonGeom);
    pDetails->nDucts = pTendonGeom->GetDuctCount(girderKey);

    GET_IFACE(ILossParameters, pLossParams);
    pDetails->bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);


    // determine continuity stage
    GET_IFACE(IIntervals, pIntervals);
    IntervalIndexType continuityIntervalIdx = MAX_INDEX;
    PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(pDetails->startGroup);
    PierIndexType lastPierIdx = pBridge->GetGirderGroupEndPier(pDetails->endGroup);
    for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++)
    {
        if (pBridge->IsBoundaryPier(pierIdx))
        {
            IntervalIndexType left_interval_index, right_interval_index;
            pIntervals->GetContinuityInterval(pierIdx, &left_interval_index, &right_interval_index);
            continuityIntervalIdx = Min(continuityIntervalIdx, left_interval_index);
            continuityIntervalIdx = Min(continuityIntervalIdx, right_interval_index);
        }
    }

    IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
    if (pDetails->bDeck)
    {
        pDetails->bContinuousBeforeDeckCasting = (continuityIntervalIdx <= firstCastDeckIntervalIdx) ? true : false;
    }
    else
    {
        pDetails->bContinuousBeforeDeckCasting = false;
    }

}


//Float64 pgsBearingDesignEngineer::GetStaticBearingRotation(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi) const
//{


//
//
//    Float64 stInfMin, stInfMax;
//    limitForces->GetRotation(
//        lastIntervalIdx,
//        pgsTypes::ServiceI,
//        poi,
//        maxBAT,
//        true,
//        false,
//        true,
//        true,
//        true,
//        &stInfMin, &stInfMax);
//
//
//    Float64 maxInfDiff = std::abs(stInfMax - stErectMax);
//    Float64 minInfDiff = std::abs(stInfMin - stErectMax);
//    Float64 maxErectDiff = std::abs(stErectMax - stInfMin);
//    Float64 minErectDiff = std::abs(stErectMin - stInfMin);
//
//    Float64 maxDiff = max(maxInfDiff, max(minInfDiff, max(maxErectDiff, minErectDiff)));
//
//    if (maxDiff == maxInfDiff) {
//        return (stInfMax - stErectMax);
//    }
//    else if (maxDiff == minInfDiff) {
//        return (stInfMin - stErectMax);
//    }
//    else if (maxDiff == maxErectDiff) {
//        return (stInfMax - stErectMin);
//    }
//    else {
//        return (stInfMin - stErectMin);
//    }
//
//}



Float64 pgsBearingDesignEngineer::GetSpanContributoryLength(CGirderKey girderKey) const
{
    GET_IFACE(IBridge, pBridge);

    Float64 L = 0;

    SpanIndexType nSpans = pBridge->GetSpanCount();

    Float64 halfOfSpans = std::ceil(nSpans / 2.0);

    for (SpanIndexType spanIdx = 0; spanIdx < halfOfSpans; spanIdx++)
    {
        if (spanIdx == halfOfSpans && nSpans % 2 != 0)
        {
            L += pBridge->GetSpanLength(spanIdx) / 2.0;
        }
        else
        {
            L += pBridge->GetSpanLength(spanIdx);
        }
    }

    return L;

}


Float64 pgsBearingDesignEngineer::GetTimeDependentComponentShearDeformation(CGirderKey girderKey, const pgsPointOfInterest& poi, Float64 loss) const
{

    GET_IFACE(ISectionProperties, pSection);
    GET_IFACE(ILosses, pLosses);
    GET_IFACE(IMaterials, pMaterials);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);

    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

    auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    Float64 L = GetSpanContributoryLength(girderKey);

    Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE();

    Float64 tendonShortening = loss * L / Ep;

    auto details = pLosses->GetLossDetails(poi, erectSegmentIntervalIdx);

    Float64 ep = details->pLosses->GetEccpgRelease().Y();

    Float64 yb = pSection->GetNetYbg(erectSegmentIntervalIdx, poi);

    Float64 r = sqrt(pSection->GetIxx(erectSegmentIntervalIdx, poi) / pSection->GetAg(erectSegmentIntervalIdx, poi));

    Float64 FlangeBottomShortening = (1.0 + ep * yb / (r * r)) / (1 + ep * ep / (r * r)) * tendonShortening;

    return FlangeBottomShortening;

}


Float64 pgsBearingDesignEngineer::GetBearingTimeDependentLosses(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig, const LOSSDETAILS* pDetails, TDCOMPONENTS* tdComponents) const
{
    GET_IFACE(IPointOfInterest, pPoi);
    if (pPoi->IsOffSegment(poi))
    {
        return 0;
    }

    // NOTE: Losses are just time-dependent change in prestress force.
    // Losses do not include elastic effects include elastic shortening or elastic gains due to external loads
    //
    // fpe = fpj - loss + gains

    const CSegmentKey& segmentKey = poi.GetSegmentKey();

    // if losses were computed with the time-step method
    // look up change in stress in strands due to creep, shrinkage, and relaxation
    if (pDetails->LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP)
    {
        if (intervalIdx == 0 && intervalTime == pgsTypes::Start)
        {
            return 0; // wanting losses at the start of the first interval.. nothing has happened yet
        }

        // time step losses are computed for the end of an interval
        IntervalIndexType theIntervalIdx = intervalIdx;
        switch (intervalTime)
        {
        case pgsTypes::Start:
            theIntervalIdx--; // losses at start of this interval are equal to losses at end of previous interval
            break;

        case pgsTypes::Middle:
            // drop through so we just use the end
        case pgsTypes::End:
            break; // do nothing... theIntervalIdx is correct
        }

#if !defined LUMP_STRANDS
        GET_IFACE(IStrandGeometry, pStrandGeom);
#endif

        if (strandType == pgsTypes::Permanent)
        {
#if defined LUMP_STRANDS
            GET_IFACE(IStrandGeometry, pStrandGeom);
            std::array<Float64, 2> Aps{ pStrandGeom->GetStrandArea(poi, theIntervalIdx, pgsTypes::Straight, pConfig), pStrandGeom->GetStrandArea(poi, theIntervalIdx, pgsTypes::Harped, pConfig) };
            Float64 A = std::accumulate(Aps.begin(), Aps.end(), 0.0);
            if (IsZero(A))
            {
                return 0;
            }

            Float64 dfpe_creep_straight = 0;
            Float64 dfpe_shrinkage_straight = 0;
            Float64 dfpe_relaxation_straight = 0;
            Float64 dfpe_creep_harped = 0;
            Float64 dfpe_shrinkage_harped = 0;
            Float64 dfpe_relaxation_harped = 0;



            for (IntervalIndexType i = 0; i <= theIntervalIdx; i++)
            {
                dfpe_creep_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftCreep];
                dfpe_creep_harped += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped].dfpei[pgsTypes::pftCreep];

                dfpe_shrinkage_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftShrinkage];
                dfpe_shrinkage_harped += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped].dfpei[pgsTypes::pftShrinkage];

                dfpe_relaxation_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftRelaxation];
                dfpe_relaxation_harped += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped].dfpei[pgsTypes::pftRelaxation];
            }

            tdComponents->creep = -(Aps[pgsTypes::Straight] * dfpe_creep_straight + Aps[pgsTypes::Harped] * dfpe_creep_harped) / A;
            tdComponents->shrinkage = -(Aps[pgsTypes::Straight] * dfpe_shrinkage_straight + Aps[pgsTypes::Harped] * dfpe_shrinkage_harped) / A;
            tdComponents->relaxation = -(Aps[pgsTypes::Straight] * dfpe_relaxation_straight + Aps[pgsTypes::Harped] * dfpe_relaxation_harped) / A;


            return -(Aps[pgsTypes::Straight] * (dfpe_creep_straight + dfpe_shrinkage_straight + dfpe_relaxation_straight) + Aps[pgsTypes::Harped] * (dfpe_creep_harped + dfpe_shrinkage_harped + dfpe_relaxation_harped)) / (A);
#else
#pragma Reminder("IMPLEMENT")
#endif
        }
        else
        {
#if defined LUMP_STRANDS
            Float64 dfpe_creep = 0;
            Float64 dfpe_shrinkage = 0;
            Float64 dfpe_relaxation = 0;
            for (IntervalIndexType i = 0; i <= theIntervalIdx; i++)
            {
                dfpe_creep += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftCreep];

                tdComponents->creep = dfpe_creep;

                dfpe_shrinkage += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftShrinkage];

                tdComponents->shrinkage = dfpe_shrinkage;

                dfpe_relaxation += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftRelaxation];

                tdComponents->relaxation = dfpe_relaxation;
            }

            return -(dfpe_creep + dfpe_shrinkage + dfpe_relaxation);
#else
#pragma Reminder("IMPLEMENT")
#endif
        }
    }
    else
    {
        // some method other than Time Step
        GET_IFACE(IIntervals, pIntervals);
        IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

        Float64 loss = 0;


        if (intervalIdx == erectSegmentIntervalIdx)
        {
            if (strandType == pgsTypes::Temporary)
            {
                loss = pDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
            }
            else
            {
                loss = pDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();

                if (pDetails->LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_REFINED_2005)
                {
                    auto pRefined2005 = std::dynamic_pointer_cast<const WBFL::LRFD::RefinedLosses2005>(pDetails->pLosses);
                    tdComponents->shrinkage = pRefined2005->PermanentStrand_ShrinkageLossAtShipping();
                    tdComponents->creep = pRefined2005->PermanentStrand_CreepLossAtShipping();
                    tdComponents->relaxation = pRefined2005->PermanentStrand_RelaxationLossAtShipping();
                }
            }
        }
        else
        {
            if (strandType == pgsTypes::Temporary)
            {
                loss = pDetails->pLosses->TemporaryStrand_Final();
            }
            else
            {
                loss = pDetails->pLosses->PermanentStrand_Final();
            }
        }

        return loss;
    }
}



Float64 pgsBearingDesignEngineer::GetTimeDependentShearDeformation(CGirderKey girderKey, 
    const pgsPointOfInterest& poi, PierIndexType startPierIdx, SHEARDEFORMATIONDETAILS* pDetails) const
{

    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);
    GET_IFACE(ILosses, pLosses);

    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

    CSegmentKey seg_key = pBridge->GetSegmentAtPier(startPierIdx, girderKey);
    auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    const LOSSDETAILS* td_details_erect = pLosses->GetLossDetails(poi, erectSegmentIntervalIdx);
    TDCOMPONENTS components_erect;
    Float64 fpLossErect = GetBearingTimeDependentLosses(poi, pgsTypes::StrandType::Straight, erectSegmentIntervalIdx, pgsTypes::IntervalTimeType::End, 
        nullptr, td_details_erect, &components_erect);

    const LOSSDETAILS* td_details_inf = pLosses->GetLossDetails(poi, lastIntervalIdx);
    TDCOMPONENTS components_inf;
    Float64 fpLossInfinity = GetBearingTimeDependentLosses(poi, pgsTypes::StrandType::Straight, lastIntervalIdx, pgsTypes::IntervalTimeType::End, 
        nullptr, td_details_inf, &components_inf);

    Float64 creepLoss = components_inf.creep - components_erect.creep;

    pDetails->creep = GetTimeDependentComponentShearDeformation(girderKey, poi, creepLoss);

    Float64 shrinkageLoss = components_inf.shrinkage - components_erect.shrinkage;

    pDetails->shrinkage = GetTimeDependentComponentShearDeformation(girderKey, poi, shrinkageLoss);

    Float64 relaxationLoss = components_inf.relaxation - components_erect.relaxation;

    pDetails->relaxation = GetTimeDependentComponentShearDeformation(girderKey, poi, relaxationLoss);

    Float64 tdLoss = fpLossInfinity - fpLossErect;

    Float64 time_dependent = GetTimeDependentComponentShearDeformation(girderKey, poi, tdLoss);

    return time_dependent;

}





void pgsBearingDesignEngineer::GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
    const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const
{


    GET_IFACE(IProductForces, pProductForces);
    GET_IFACE(IProductLoads, pProductLoads);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(ILoadFactors, pLF);
    GET_IFACE_NOCHECK(ICamber, pCamber);
    GET_IFACE(ILossParameters, pLossParams);

    Float64 min, max, DcreepErect, RcreepErect, DcreepFinal, RcreepFinal;
    VehicleIndexType minConfig, maxConfig;
    pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
    auto skewFactor = this->BearingSkewFactor(reactionLocation, isFlexural);
    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

    // Static rotations

    /////

    pDetails->erectedSegmentRotation = skewFactor * (pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false) - 
        pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false));

    pDetails->maxShearKeyRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false);
    pDetails->maxGirderRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftGirder, poi, minBAT, rtCumulative, false);
    pDetails->diaphragmRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftDiaphragm, poi, maxBAT, rtCumulative, false);
    pDetails->maxSlabRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false);
    pDetails->maxHaunchRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false);
    pDetails->minHaunchRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, minBAT, rtCumulative, false);
    pDetails->maxRailingSystemRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false);
    pDetails->minRailingSystemRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, minBAT, rtCumulative, false);
    pDetails->maxFutureOverlayRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false);
    pDetails->minFutureOverlayRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, minBAT, rtCumulative, false);
    pDetails->maxLongitudinalJointRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false);
    pDetails->maxLongitudinalJointRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, minBAT, rtCumulative, false);
    pDetails->maxConstructionRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, maxBAT, rtCumulative, false);
    pDetails->minConstructionRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, minBAT, rtCumulative, false);
    pDetails->maxSlabPanelRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabPanelRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, minBAT, rtCumulative, false);
    pDetails->maxSidewalkRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false);
    pDetails->minSidewalkRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, minBAT, rtCumulative, false);

    pDetails->maxUserDCRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, maxBAT, rtCumulative, false);
    pDetails->minUserDCRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, minBAT, rtCumulative, false);
    pDetails->maxUserDWRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, maxBAT, rtCumulative, false);
    pDetails->minUserDWRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, minBAT, rtCumulative, false);

    pDetails->preTensionRotation = skewFactor * (
        pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false) -
        pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false));

    pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDiaphragm, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &DcreepErect, &RcreepErect);

    pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &DcreepFinal, &RcreepFinal);

    bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

    if (bTimeStep)
    {
        Float64 cFinal = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false);
        Float64 cErect = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false);

        pDetails->creepRotation = skewFactor * (cFinal - cErect);
        pDetails->shrinkageRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false);
        pDetails->relaxationRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false);
        pDetails->postTensionRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false);
    }
    else
    {
        pDetails->creepRotation = skewFactor * (RcreepFinal - RcreepErect);
        pDetails->shrinkageRotation = 0.0;
        pDetails->relaxationRotation = 0.0;
        pDetails->postTensionRotation = 0.0;
    }

    Float64 minDCrotation{ 0.0 };
    Float64 maxDCrotation{ 0.0 };
    auto dcLoads = pProductLoads->GetProductForcesForCombo(LoadingCombinationType::lcDC);
    for (auto loadType : dcLoads)
    {
        minDCrotation += pProductForces->GetRotation(lastIntervalIdx, loadType, poi, minBAT, rtCumulative, false) -
            pProductForces->GetRotation(erectSegmentIntervalIdx, loadType, poi, minBAT, rtCumulative, false);
        maxDCrotation += pProductForces->GetRotation(lastIntervalIdx, loadType, poi, maxBAT, rtCumulative, false) -
            pProductForces->GetRotation(erectSegmentIntervalIdx, loadType, poi, maxBAT, rtCumulative, false);
    }
    Float64 minDWrotation{ 0.0 };
    Float64 maxDWrotation{ 0.0 };
    auto dwLoads = pProductLoads->GetProductForcesForCombo(LoadingCombinationType::lcDW);
    for (auto loadType : dwLoads)
    {
        minDWrotation += pProductForces->GetRotation(lastIntervalIdx, loadType, poi, minBAT, rtCumulative, false) -
            pProductForces->GetRotation(erectSegmentIntervalIdx, loadType, poi, minBAT, rtCumulative, false);
        maxDWrotation += pProductForces->GetRotation(lastIntervalIdx, loadType, poi, maxBAT, rtCumulative, false) -
            pProductForces->GetRotation(erectSegmentIntervalIdx, loadType, poi, maxBAT, rtCumulative, false);
    }

    const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
    auto dcDF = pLoadFactors->GetDCMax(pgsTypes::ServiceI);
    auto dwDF = pLoadFactors->GetDWMax(pgsTypes::ServiceI);
    auto llDF = pLoadFactors->GetLLIMMax(pgsTypes::ServiceI);


    if (reactionLocation.Face == PierReactionFaceType::rftAhead)
    {
        pDetails->staticRotation = skewFactor * (dcDF * minDCrotation + dwDF * minDWrotation + pDetails->preTensionRotation +
            pDetails->creepRotation + pDetails->shrinkageRotation + pDetails->relaxationRotation + pDetails->postTensionRotation - 0.005);
    }
    else if (reactionLocation.Face == PierReactionFaceType::rftBack)
    {
        pDetails->staticRotation = skewFactor * (dcDF * minDCrotation + dwDF * minDWrotation + pDetails->preTensionRotation +
            pDetails->creepRotation + pDetails->shrinkageRotation + pDetails->relaxationRotation + pDetails->postTensionRotation + 0.005);
    }


    // Cyclic Rotations

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
    pDetails->maxDesignLLrotation = skewFactor * max;
    pDetails->maxConfigRotation = maxConfig;
    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
    pDetails->minDesignLLrotation = skewFactor * min;
    pDetails->minConfigRotation = minConfig;

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, maxBAT, bIncludeImpact, true, &min, &max);
    pDetails->maxPedRotation = skewFactor * max;

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, minBAT, bIncludeImpact, true, &min, &max);
    pDetails->minPedRotation = skewFactor * min;

    pDetails->maxUserLLrotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false);
    pDetails->minUserLLrotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, minBAT, rtCumulative, false);

    if (reactionLocation.Face == PierReactionFaceType::rftAhead)
    {
        pDetails->cyclicRotation = skewFactor * llDF * (pDetails->minDesignLLrotation + pDetails->minUserLLrotation + pDetails->minPedRotation);
    }
    else if (reactionLocation.Face == PierReactionFaceType::rftBack)
    {
        pDetails->cyclicRotation = skewFactor * llDF * (pDetails->maxDesignLLrotation + pDetails->maxUserLLrotation + pDetails->maxPedRotation);
    }
        
    
    pDetails->totalRotation = pDetails->staticRotation +  pDetails->cyclicRotation;
    

}



void pgsBearingDesignEngineer::GetBearingReactionDetails(const ReactionLocation& reactionLocation,
    CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const


{


    GET_IFACE(IProductForces, pProdForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);

    pgsTypes::BridgeAnalysisType batSS = pgsTypes::SimpleSpan;
    pgsTypes::BridgeAnalysisType batCS = pgsTypes::ContinuousSpan;


    GET_IFACE(IIntervals, pIntervals);
    IntervalIndexType diaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
    IntervalIndexType lastCastDeckIntervalIdx = pIntervals->GetLastCastDeckInterval(); // deck cast be cast in multiple stages, use interval after entire deck is cast
    IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
    IntervalIndexType ljIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
    IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
    IntervalIndexType constructionIntervalIdx = pIntervals->GetConstructionLoadInterval();
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
    IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();


    
    GET_IFACE(IBridge, pBridge);

    PierIndexType nPiers = pBridge->GetPierCount();

    // TRICKY: use adapter class to get correct reaction interfaces
    std::unique_ptr<IProductReactionAdapter> pForces;

    GET_IFACE(IReactions, pReactions);
    pForces = std::make_unique<ProductForcesReactionAdapter>(pReactions, girderKey);

    const CGirderKey& thisGirderKey(reactionLocation.GirderKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);


    


    pDetails->erectedSegmentReaction = pForces->GetReaction(erectSegmentIntervalIdx, reactionLocation, pgsTypes::pftGirder, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
    pDetails->maxGirderReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftGirder, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
    pDetails->diaphragmReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftDiaphragm, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

    if (pDetails->bShearKey)
    {
        pDetails->maxShearKeyReaction = pForces->GetReaction(shearKeyIntervalIdx, reactionLocation, pgsTypes::pftShearKey, maxBAT);
        pDetails->minShearKeyReaction = pForces->GetReaction(shearKeyIntervalIdx, reactionLocation, pgsTypes::pftShearKey, minBAT);
    }

    if (pDetails->bLongitudinalJoint)
    {
        pDetails->maxLongitudinalJointReaction = pForces->GetReaction(ljIntervalIdx, reactionLocation, pgsTypes::pftLongitudinalJoint, maxBAT);
        pDetails->minLongitudinalJointReaction = pForces->GetReaction(ljIntervalIdx, reactionLocation, pgsTypes::pftLongitudinalJoint, minBAT);
    }

    pDetails->maxConstructionReaction = pForces->GetReaction(constructionIntervalIdx, reactionLocation, pgsTypes::pftConstruction, maxBAT);
    pDetails->minConstructionReaction = pForces->GetReaction(constructionIntervalIdx, reactionLocation, pgsTypes::pftConstruction, minBAT);

    pDetails->maxSlabReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlab, maxBAT);
    pDetails->minSlabReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlab, minBAT);
    pDetails->maxHaunchReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, maxBAT);
    pDetails->minHaunchReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, minBAT);

    if (pDetails->bDeckPanels)
    {
        pDetails->maxSlabPanelReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, maxBAT);
        pDetails->minSlabPanelReaction = pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, minBAT);
    }

    pDetails->maxSidewalkReaction = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftSidewalk, batSS);
    pDetails->minSidewalkReaction = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftSidewalk, batCS);

    Float64 R1 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftTrafficBarrier, batSS);
    Float64 R2 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftTrafficBarrier, batCS);
    pDetails->maxRailingSystemReaction = Max(R1, R2);
    pDetails->minRailingSystemReaction = Min(R1, R2);

    if (pDetails->bHasOverlay)
    {
        R1 = pForces->GetReaction(overlayIntervalIdx, reactionLocation, pgsTypes::pftOverlay, batSS);
        R2 = pForces->GetReaction(overlayIntervalIdx, reactionLocation, pgsTypes::pftOverlay, batCS);
        pDetails->maxFutureOverlayReaction = Max(R1, R2);
        pDetails->minFutureOverlayReaction = Min(R1, R2);
    }






    // TRICKY:
    // Use the adapter class to get the reaction response functions we need and to iterate piers
    std::unique_ptr<ICmbLsReactionAdapter> pComboForces;

    GET_IFACE(IBearingDesign, pBearingDesign);
    pComboForces = std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, lastIntervalIdx, girderKey);



    Float64 R1min, R1max, R2min, R2max;

    if (pDetails->bPedLoading)
    {
        pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, batSS, bIncludeImpact, true, &R1min, &R2max);
        pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, batCS, bIncludeImpact, true, &R2min, &R2max);
        pDetails->maxPedReaction = Max(R1max, R2max);
        pDetails->minPedReaction = Min(R1min, R2min);
    }
    else
    {
        pDetails->maxPedReaction = 0.0;
        pDetails->minPedReaction = 0.0;
    }

    pDetails->creepReaction = 0.0;
    pDetails->relaxationReaction = 0.0;
    pDetails->preTensionReaction = 0.0;
    pDetails->postTensionReaction = 0.0;
    pDetails->shrinkageReaction = 0.0;



    VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;
    pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, false, true, &R1min, &R1max, &minConfig1, &maxConfig1);
    pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, false, true, &R2min, &R2max, &minConfig2, &maxConfig2);

    GET_IFACE(ILiveLoadDistributionFactors, pLLDF);
    SpanIndexType spanIdx = pBridge->GetGirderGroupEndSpan(girderKey.groupIndex);
    CSpanKey spanKey(spanIdx, girderKey.girderIndex);
    Float64 lldf = pLLDF->GetDeflectionDistFactor(spanKey);

    pDetails->maxDesignLLReaction = Max(R1max, R2max)*lldf;
    VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;
    pDetails->maxConfigReaction = maxConfig;

    pDetails->minDesignLLReaction = Min(R1min, R2min)*lldf;
    VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;
    pDetails->minConfigReaction = minConfig;

    pDetails->maxUserDCReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserDC, maxBAT);
    pDetails->maxUserDWReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserDW, maxBAT);
    pDetails->maxUserLLReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserLLIM, maxBAT);
    pDetails->minUserDCReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserDC, minBAT);
    pDetails->minUserDWReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserDW, minBAT);
    pDetails->minUserLLReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftUserLLIM, minBAT);

    auto dcReaction = pComboForces->GetReaction(lastIntervalIdx, lcDC, reactionLocation, maxBAT, rtCumulative);
    auto dwReaction = pComboForces->GetReaction(lastIntervalIdx, lcDW, reactionLocation, maxBAT, rtCumulative);

    // LRFD Limit States
    GET_IFACE(ILoadFactors, pLF);
    const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
    auto dcLF = pLoadFactors->GetDCMax(pgsTypes::ServiceI);
    auto dwLF = pLoadFactors->GetDWMax(pgsTypes::ServiceI);
    auto llLF = pLoadFactors->GetLLIMMax(pgsTypes::ServiceI);

    pDetails->totalDLreaction = dcLF * dcReaction + dwLF * dwReaction + pDetails->preTensionReaction + pDetails->creepReaction + pDetails->relaxationReaction + pDetails->shrinkageReaction;

    pComboForces->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, bIncludeImpact, &R1min, &R1max);
    pComboForces->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, bIncludeImpact, &R2min, &R2max);

    pDetails->maxComboDesignLLReaction = llLF * (Max(R1max, R2max) + pDetails->maxPedReaction);

    pDetails->totalReaction = pDetails->totalDLreaction + pDetails->maxComboDesignLLReaction;

}


void pgsBearingDesignEngineer::GetThermalExpansionDetails(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const
{

    GET_IFACE(IBridge, pBridge);

    Float64 L = 0;
    SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
    for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
    {

        CSegmentKey segmentKey(girderKey, segIdx);

        L += pBridge->GetSegmentLayoutLength(segmentKey);

    }

    Float64 inv_thermal_exp_coefficient = { WBFL::Units::ConvertToSysUnits(166666.6667, WBFL::Units::Measure::Fahrenheit) };
    Float64 thermal_expansion_coefficient = 1.0 / inv_thermal_exp_coefficient;
    pDetails->thermal_expansion_coefficient = thermal_expansion_coefficient;

    pDetails->max_design_temperature_cold = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->min_design_temperature_cold = WBFL::Units::ConvertToSysUnits(0.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->max_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->min_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Fahrenheit);

    GET_IFACE(ILibrary, pLibrary);
    WBFL::System::Time time;
    bool bPrintDate = WBFL::System::Time::PrintDate(true);

    std::_tstring strServer;
    std::_tstring strConfiguration;
    std::_tstring strMasterLibFile;
    pLibrary->GetMasterLibraryInfo(strServer, strConfiguration, strMasterLibFile, time);

    if (strConfiguration == _T("WSDOT"))
    {
        pDetails->percentExpansion = 0.75;
    }
    else
    {
        pDetails->percentExpansion = 0.65;
    }

    pDetails->thermal_expansion_cold = -pDetails->percentExpansion * pDetails->thermal_expansion_coefficient * L * (pDetails->max_design_temperature_cold - pDetails->min_design_temperature_cold);
    pDetails->thermal_expansion_moderate = -pDetails->percentExpansion * pDetails->thermal_expansion_coefficient * L * (pDetails->max_design_temperature_moderate - pDetails->min_design_temperature_moderate);

    pDetails->total_shear_deformation_cold = pDetails->thermal_expansion_cold + pDetails->time_dependent;
    pDetails->total_shear_deformation_moderate = pDetails->thermal_expansion_moderate + pDetails->time_dependent;

}


void pgsBearingDesignEngineer::GetBearingShearDeformationDetails(pgsTypes::AnalysisType analysisType, PierIndexType startPierIdx, const pgsPointOfInterest& poi,
    const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, SHEARDEFORMATIONDETAILS* pDetails) const
{
    

    GET_IFACE(IBearingDesignParameters, pBearing);

    pDetails->time_dependent = pBearing->GetTimeDependentShearDeformation(girderKey, poi, startPierIdx, pDetails);

    pBearing->GetThermalExpansionDetails(girderKey, pDetails);

    


}

