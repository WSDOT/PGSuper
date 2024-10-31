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

#include <Reporting\ReactionInterfaceAdapters.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <psgLib/ThermalMovementCriteria.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/LimitStateConcreteStrengthCriteria.h>

#include <PgsExt\statusitem.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\GirderLabel.h>


#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>


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


void pgsBearingDesignEngineer::GetLongitudinalPointOfFixity(const CGirderKey& girderKey, TABLEPARAMETERS* pDetails) const
{
    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IBridgeDescription, pBridgeDesc);
    GET_IFACE(IPointOfInterest, pPoi);

    PierIndexType centermostPier;
    SpanIndexType nSpans = pBridge->GetSpanCount();
    bool bHasXConstraint = false;

    centermostPier = nSpans / 2;

    pgsPointOfInterest poi_fixity{ pPoi->GetPierPointOfInterest(girderKey, 0) };

    PierIndexType fixityPier{ 0 };

    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey, &vGirderKeys);
    for (const auto& thisGirderKey : vGirderKeys)
    {
        SpanIndexType startSpanIdx, endSpanIdx;
        pBridge->GetGirderGroupSpans(ALL_GROUPS, &startSpanIdx, &endSpanIdx);
        for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
        {
            // pier indicies related to this span
            PierIndexType PierIdx = PierIndexType(spanIdx);
            const CPierData2* pPier = pBridgeDesc->GetPier(PierIdx);

            if (!bHasXConstraint)
            {
                if (pPier->IsBoundaryPier())
                {
                    pgsTypes::BoundaryConditionType bct = pPier->GetBoundaryConditionType();
                    if (bct != pgsTypes::bctRoller && bct != pgsTypes::bctContinuousAfterDeck && bct != pgsTypes::bctContinuousBeforeDeck)
                    {
                        poi_fixity = pPoi->GetPierPointOfInterest(girderKey, PierIdx);
                        fixityPier = PierIdx;
                        bHasXConstraint = true;
                    }
                }
                else if (pPier->IsInteriorPier())
                {
                    pgsTypes::PierSegmentConnectionType sct = pPier->GetSegmentConnectionType();
                    if (sct == pgsTypes::psctIntegralClosureJoint || sct == pgsTypes::psctIntegralSegment)
                    {
                        poi_fixity = pPoi->GetPierPointOfInterest(girderKey, PierIdx);
                        fixityPier = PierIdx;
                        bHasXConstraint = true;
                    }
                }
            }
        }
    }

    if (!bHasXConstraint)
    {
        for (const auto& thisGirderKey : vGirderKeys)
        {
            SpanIndexType startSpanIdx, endSpanIdx;
            pBridge->GetGirderGroupSpans(ALL_GROUPS, &startSpanIdx, &endSpanIdx);
            for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
            {
                // pier indicies related to this span
                PierIndexType prevPierIdx = PierIndexType(spanIdx);
                PierIndexType nextPierIdx = prevPierIdx + 1;
                auto nPiers = pBridge->GetPierCount();

                if (pBridgeDesc->GetPier(nextPierIdx)->GetPierModelType() != pgsTypes::pmtPhysical && nextPierIdx == centermostPier && nextPierIdx != nPiers - 1)
                {
                    poi_fixity = pPoi->GetPierPointOfInterest(girderKey, nextPierIdx);
                    fixityPier = nextPierIdx;
                    bHasXConstraint = true;
                }

            }
        }
    }

    if (!bHasXConstraint)
    {
        poi_fixity = pPoi->GetPierPointOfInterest(girderKey, 0);
        fixityPier = 0;
        bHasXConstraint = true;
    }

    pDetails->poi_fixity = poi_fixity;
    pDetails->fixityPier = fixityPier;
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

    GetLongitudinalPointOfFixity(girderKey, pDetails);

}



Float64 pgsBearingDesignEngineer::GetDistanceToPointOfFixity(const pgsPointOfInterest& poi_brg,
    SHEARDEFORMATIONDETAILS* pDetails) const
{

    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IPointOfInterest, pPoi);

    Float64 L = 0;

    const CGirderKey& girderKey(poi_brg.GetSegmentKey());

    CSegmentKey segmentKey = pDetails->poi_fixity.GetSegmentKey();
    Float64 gpcBrgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);

    Float64 gpcBrg = pPoi->ConvertPoiToGirderPathCoordinate(poi_brg);
    Float64 gpcFixity = pPoi->ConvertPoiToGirderPathCoordinate(pDetails->poi_fixity);



    if (gpcBrg > gpcFixity && !IsEqual(gpcBrg, gpcFixity, 0.001) && pBridge->IsAbutment(pDetails->fixityPier))
    {
        gpcFixity += gpcBrgOffset;
    }
    if (gpcBrg < gpcFixity && !IsEqual(gpcBrg, gpcFixity, 0.001) && pBridge->IsAbutment(pDetails->fixityPier))
    {
        gpcFixity -= gpcBrgOffset;
    }


    L = abs(gpcFixity - gpcBrg);

    return L;

}


std::array<Float64, 2> pgsBearingDesignEngineer::GetTimeDependentComponentShearDeformation(Float64 loss, BEARINGSHEARDEFORMATIONDETAILS* bearing) const
{


    GET_IFACE(IMaterials, pMaterials);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);

    

    const CSegmentKey& segmentKey(bearing->rPoi.GetSegmentKey());
    const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(bearing->rPoi.GetSegmentKey());
    auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;


    Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE();

    Float64 tendon_shortening = -(loss * bearing->length_pf / Ep) / 3.0; // assumes 2/3 of creep and shrinkage occurs before girders are erected

    Float64 flange_bottom_shortening = (1.0 + bearing->ep * bearing->yb / (bearing->r * bearing->r)) /
        (1 + bearing->ep * bearing->ep / (bearing->r * bearing->r)) * tendon_shortening;

    std::array<Float64, 2> shortening = {tendon_shortening, flange_bottom_shortening};
   
    return shortening;

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


        if (pDetails->LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_REFINED_2005 || pDetails->LossMethod == PrestressLossCriteria::LossMethodType::AASHTO_REFINED_2005)
        {
            auto pRefined2005 = std::dynamic_pointer_cast<const WBFL::LRFD::RefinedLosses2005>(pDetails->pLosses);
            tdComponents->creep = pRefined2005->CreepLossBeforeDeckPlacement();
            tdComponents->shrinkage = pRefined2005->ShrinkageLossBeforeDeckPlacement();
            tdComponents->relaxation = pRefined2005->RelaxationLossBeforeDeckPlacement();
        }

            
        Float64 loss = tdComponents->creep + tdComponents->shrinkage + tdComponents->relaxation;

        return loss;
    }
}



void pgsBearingDesignEngineer::GetTimeDependentShearDeformation(CGirderKey girderKey, SHEARDEFORMATIONDETAILS* pDetails) const
{

    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);
    GET_IFACE(ILosses, pLosses);
    GET_IFACE(ISectionProperties, pSection);
    GET_IFACE(IStrandGeometry, pStrandGeom);
    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IPointOfInterest, pPOI);
    GET_IFACE(IBearingDesign, pBearingDesign);


    IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
    std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));


    // get poi where pier Reactions occur
    PoiList vPoi;
    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey.girderIndex, pDetails->startGroup, pDetails->endGroup, &vGirderKeys);
    for (const auto& thisGirderKey : vGirderKeys)
    {
        pPOI->GetPointsOfInterest(CSpanKey(ALL_SPANS, thisGirderKey.girderIndex), POI_ABUTMENT | POI_BOUNDARY_PIER | POI_INTERMEDIATE_PIER, &vPoi);
    }

    ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

    PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

    // Use iterator to walk locations
    for (iter.First(); !iter.IsDone(); iter.Next())
    {
        GET_IFACE(IBearingDesignParameters, pBearing);
        BEARINGSHEARDEFORMATIONDETAILS brg_details;

        const ReactionLocation& reactionLocation(iter.CurrentItem());

        brg_details.reactionLocation = reactionLocation;

        const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

        const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx - startPierIdx];

        brg_details.rPoi = poi;

        GET_IFACE(ILibrary, pLib);
        GET_IFACE(ISpecification, pSpec);
        pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
        const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
        PrestressLossCriteria::LossMethodType lossMethod = pSpecEntry->GetPrestressLossCriteria().LossMethod;
        bool bTimeStepMethod = lossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP;

        brg_details.length_pf = pBearing->GetDistanceToPointOfFixity(poi, pDetails);

        const CSegmentKey& segmentKey(poi.GetSegmentKey());
        const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
        IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
        IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
        IntervalIndexType castDeckIntervalIdx = pIntervals->GetLastCastDeckInterval();
        auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

        auto details = pLosses->GetLossDetails(poi, erectSegmentIntervalIdx);

        brg_details.ep = pStrandGeom->GetEccentricity(erectSegmentIntervalIdx, poi, pgsTypes::Permanent).Y();

        brg_details.yb = pSection->GetNetYbg(erectSegmentIntervalIdx, poi);

        brg_details.Ixx = pSection->GetIxx(erectSegmentIntervalIdx, poi);

        brg_details.Ag = pSection->GetAg(erectSegmentIntervalIdx, poi);

        brg_details.r = sqrt(brg_details.Ixx / brg_details.Ag);

        const LOSSDETAILS* td_details_inf = pLosses->GetLossDetails(poi, lastIntervalIdx);
        TDCOMPONENTS components_inf;
        Float64 fpLossInfinity = GetBearingTimeDependentLosses(poi, pgsTypes::StrandType::Permanent, castDeckIntervalIdx, pgsTypes::IntervalTimeType::End,
            nullptr, td_details_inf, &components_inf);


        if (bTimeStepMethod)
        {

            GET_IFACE(IBridge, pBridge);



            Float64 L = GetDistanceToPointOfFixity(poi, pDetails);
            GroupIndexType nGroups = pBridge->GetGirderGroupCount();
            GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
            GroupIndexType lastGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);

            for (IntervalIndexType intervalIdx = erectSegmentIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
            {

                TSSHEARDEFORMATIONDETAILS timestep_details;

                for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
                {

                    PoiList vPoi;
                    GET_IFACE(IPointOfInterest, pIPoi);

                    if (pIPoi->ConvertPoiToGirderlineCoordinate(poi) < pIPoi->ConvertPoiToGirderlineCoordinate(pDetails->poi_fixity))
                    {
                        pIPoi->GetPointsOfInterestInRange(0, poi, L, &vPoi);
                    }
                    else
                    {
                        pIPoi->GetPointsOfInterestInRange(L, poi, 0, &vPoi);
                    }

                    vPoi.erase(std::remove_if(vPoi.begin(), vPoi.end(), [](const pgsPointOfInterest& i) {
                        return i.HasAttribute(POI_BOUNDARY_PIER);
                        }), vPoi.end());

                    pgsPointOfInterest p0, p1;
                    Float64 d0, d1;

                    timestep_details.interval_creep = 0.0;
                    timestep_details.interval_shrinkage = 0.0;
                    timestep_details.interval_relaxation = 0.0;

                    timestep_details.interval = intervalIdx;

                    for (IndexType idx = 1, nPoi = vPoi.size(); idx < nPoi; idx++)
                    {
                        if (pIPoi->ConvertPoiToGirderlineCoordinate(poi) < pIPoi->ConvertPoiToGirderlineCoordinate(pDetails->poi_fixity))
                        {
                            p0 = vPoi[idx - 1];
                            p1 = vPoi[idx];
                            d0 = pIPoi->ConvertPoiToGirderlineCoordinate(p0);
                            d1 = pIPoi->ConvertPoiToGirderlineCoordinate(p1);
                        }
                        else 
                        {
                            p0 = vPoi[idx];
                            p1 = vPoi[idx - 1];
                            d0 = pIPoi->ConvertPoiToGirderlineCoordinate(p1);
                            d1 = pIPoi->ConvertPoiToGirderlineCoordinate(p0);
                        }

                        std::vector<pgsTypes::ProductForceType> td_types{
                            pgsTypes::ProductForceType::pftCreep,
                                pgsTypes::ProductForceType::pftShrinkage,
                                pgsTypes::ProductForceType::pftRelaxation};

                        TSSHEARDEFORMATION_DIFF_ELEMS td_diff_elems;

                        if (d1 != d0)
                        {
                            td_diff_elems.poi = p1;
                            td_diff_elems.delta_d = d1 - d0;

                            for (IndexType ty = 0, nTypes = td_types.size(); ty < nTypes; ty++)
                            {

                                const LOSSDETAILS* pDetails0erect = pLosses->GetLossDetails(p0, erectSegmentIntervalIdx);
                                const TIME_STEP_DETAILS& tsDetails0erect(pDetails0erect->TimeStepDetails[erectSegmentIntervalIdx]);
                                const LOSSDETAILS* pDetails0 = pLosses->GetLossDetails(p0, intervalIdx);
                                const TIME_STEP_DETAILS& tsDetails0(pDetails0->TimeStepDetails[intervalIdx]);
                                Float64 strain_bot_girder_0_erect = tsDetails0erect.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtCumulative];
                                Float64 inc_strain_bot_girder_0 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtIncremental];
                                Float64 cum_strain_bot_girder_0 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtCumulative] - strain_bot_girder_0_erect;

                                const LOSSDETAILS* pDetails1erect = pLosses->GetLossDetails(p1, erectSegmentIntervalIdx);
                                const TIME_STEP_DETAILS& tsDetails1erect(pDetails1erect->TimeStepDetails[erectSegmentIntervalIdx]);
                                const LOSSDETAILS* pDetails1 = pLosses->GetLossDetails(p1, intervalIdx);
                                const TIME_STEP_DETAILS& tsDetails1(pDetails1->TimeStepDetails[intervalIdx]);
                                Float64 strain_bot_girder_1_erect = tsDetails1erect.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtCumulative];
                                Float64 inc_strain_bot_girder_1 = tsDetails1.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtIncremental];
                                Float64 cum_strain_bot_girder_1 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][td_types[ty]][rtCumulative] - strain_bot_girder_1_erect;

                                if (td_types[ty] == pgsTypes::pftCreep)
                                {
                                    td_diff_elems.creep = {
                                        inc_strain_bot_girder_1,
                                        cum_strain_bot_girder_1,
                                        (cum_strain_bot_girder_0 + cum_strain_bot_girder_1) / 2.0, // average strain using mid-point rule
                                        -(cum_strain_bot_girder_0 + cum_strain_bot_girder_1) * (d1 - d0) / 2.0
                                    };
                                    timestep_details.interval_creep += td_diff_elems.creep[3];
                                }
                                if (td_types[ty] == pgsTypes::pftShrinkage)
                                {
                                    td_diff_elems.shrinkage = {
                                        inc_strain_bot_girder_1,
                                        cum_strain_bot_girder_1,
                                        (cum_strain_bot_girder_0 + cum_strain_bot_girder_1) / 2.0, // average strain using mid-point rule
                                        -(cum_strain_bot_girder_0 + cum_strain_bot_girder_1) * (d1 - d0) / 2.0
                                    };
                                    timestep_details.interval_shrinkage += td_diff_elems.shrinkage[3];
                                }
                                if (td_types[ty] == pgsTypes::pftRelaxation)
                                {
                                    td_diff_elems.relaxation = {
                                        inc_strain_bot_girder_1,
                                        cum_strain_bot_girder_1,
                                        (cum_strain_bot_girder_0 + cum_strain_bot_girder_1) / 2.0, // average strain using mid-point rule
                                        -(cum_strain_bot_girder_0 + cum_strain_bot_girder_1) * (d1 - d0) / 2.0
                                    };
                                    timestep_details.interval_relaxation += td_diff_elems.relaxation[3];
                                }

                            };
                            

                            timestep_details.ts_diff_elems.emplace_back(td_diff_elems);

                        }

                        if (intervalIdx == lastIntervalIdx)
                        {
                            brg_details.creep = timestep_details.interval_creep;
                            brg_details.shrinkage = timestep_details.interval_shrinkage;
                            brg_details.relaxation = timestep_details.interval_relaxation;
                        }

                    }

                }

                brg_details.timestep_details.emplace_back(timestep_details);

            }

            Float64 total_time_dependent = brg_details.creep + brg_details.shrinkage + brg_details.relaxation;

            brg_details.time_dependent = total_time_dependent;



        }
        else
        {

            //calculate creep deformation
            Float64 creepLoss = components_inf.creep; // -components_erect.creep;
            auto creep_shortening = GetTimeDependentComponentShearDeformation(creepLoss, &brg_details);
            brg_details.tendon_creep = creep_shortening[0];
            brg_details.creep = -creep_shortening[1];

            //calculate shrinkage deformation
            Float64 shrinkageLoss = components_inf.shrinkage; // -components_erect.shrinkage;
            auto shrinkage_shortening = GetTimeDependentComponentShearDeformation(shrinkageLoss, &brg_details);
            brg_details.tendon_shrinkage = shrinkage_shortening[0];
            brg_details.shrinkage = -shrinkage_shortening[1];

            //calculate relaxation deformation
            Float64 relaxationLoss = components_inf.relaxation; // -components_erect.relaxation;
            auto relaxation_shortening = GetTimeDependentComponentShearDeformation(relaxationLoss, &brg_details);
            brg_details.tendon_relaxation = relaxation_shortening[0];
            brg_details.relaxation = -relaxation_shortening[1];

            Float64 sum_tendon_components = (brg_details.tendon_creep + brg_details.tendon_shrinkage + brg_details.tendon_relaxation);
            brg_details.tendon_shortening = sum_tendon_components;

            Float64 sum_components = (brg_details.creep + brg_details.shrinkage + brg_details.relaxation);
            brg_details.time_dependent = sum_components;

        }

        pDetails->brg_details.emplace_back(brg_details);

        if (bTimeStepMethod)
        {
            
            for (const auto& brg : pDetails->brg_details)
            {

                Float64 sum_last_interval = 0.0;

                for (const auto& interval : brg.timestep_details)
                {

                    Float64 sum_td_elems = 0.0;

                    for (const auto& elem : interval.ts_diff_elems)
                    {

                        sum_td_elems += elem.creep[3] + elem.shrinkage[3] + elem.relaxation[3];

                    }

                    Float64 sum_interval = interval.interval_creep + interval.interval_shrinkage + interval.interval_relaxation;

                    ATLASSERT(IsEqual(sum_td_elems , sum_interval));

                    sum_last_interval = sum_interval;

                }

                ATLASSERT(IsEqual(brg.time_dependent, brg.creep + brg.shrinkage + brg.relaxation));
                ATLASSERT(IsEqual(sum_last_interval, brg.time_dependent));

            }

        }

    }

}




void pgsBearingDesignEngineer::GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
    const ReactionLocation& reactionLocation, CGirderKey girderKey, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const
{

    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IProductForces, pProductForces);
    GET_IFACE(IProductLoads, pProductLoads);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(ILoadFactors, pLF);
    GET_IFACE_NOCHECK(ICamber, pCamber);
    GET_IFACE(ILossParameters, pLossParams);
    GET_IFACE(IPointOfInterest, pPoi);

    Float64 min, max, DcreepErect, RcreepErect, DcreepFinal, RcreepFinal;
    VehicleIndexType minConfig, maxConfig;
    pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
    auto skewFactor = this->BearingSkewFactor(reactionLocation, isFlexural);
    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

    Float64 gpcFixity = pPoi->ConvertPoiToGirderPathCoordinate(pDetails->poi_fixity);
    Float64 pXgp;
    pBridge->GetPierLocation(girderKey, reactionLocation.PierIdx, &pXgp);

    Float64 sign_correction = 1;
    if (IsGT(gpcFixity, pXgp, 0.01))
    {
        sign_correction = -1;
    }


    // Static rotations

    pDetails->erectedSegmentRotation = sign_correction * skewFactor * pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false);

    pDetails->maxShearKeyRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false);
    Float64 girder_final_rotation = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false);
    Float64 girder_erect_rotation = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false);
    pDetails->maxGirderRotation = sign_correction * skewFactor * (girder_final_rotation - girder_erect_rotation);
    pDetails->diaphragmRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftDiaphragm, poi, maxBAT, rtCumulative, false);
    pDetails->maxSlabRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false);
    pDetails->maxHaunchRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false);
    pDetails->minHaunchRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, minBAT, rtCumulative, false);
    pDetails->maxRailingSystemRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false);
    pDetails->minRailingSystemRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, minBAT, rtCumulative, false);
    pDetails->maxFutureOverlayRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false);
    pDetails->minFutureOverlayRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, minBAT, rtCumulative, false);
    pDetails->maxLongitudinalJointRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false);
    pDetails->maxLongitudinalJointRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, minBAT, rtCumulative, false);
    pDetails->maxConstructionRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, maxBAT, rtCumulative, false);
    pDetails->minConstructionRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, minBAT, rtCumulative, false);
    pDetails->maxSlabPanelRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabPanelRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, minBAT, rtCumulative, false);
    pDetails->maxSidewalkRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false);
    pDetails->minSidewalkRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, minBAT, rtCumulative, false);

    pDetails->maxUserDCRotation = sign_correction * skewFactor * (
        pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, maxBAT, rtCumulative, false) -
        pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftUserDC, poi, maxBAT, rtCumulative, false));
    pDetails->minUserDCRotation = sign_correction * skewFactor * (
        pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, minBAT, rtCumulative, false) -
        pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftUserDC, poi, minBAT, rtCumulative, false));
    pDetails->maxUserDWRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, maxBAT, rtCumulative, false);
    pDetails->minUserDWRotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, minBAT, rtCumulative, false);

    pDetails->preTensionRotation = sign_correction * skewFactor * (
        pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false) -
        pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false));

    pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDiaphragm, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &DcreepErect, &RcreepErect);
    pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &DcreepFinal, &RcreepFinal);

    bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

    if (bTimeStep)
    {
        Float64 cFinal = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false);
        Float64 cErect = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false);

        Float64 sFinal = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false);
        Float64 sErect = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false);

        Float64 rFinal = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false);
        Float64 rErect = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false);

        Float64 pFinal = pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false);
        Float64 pErect = pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false);

        pDetails->creepRotation = sign_correction * skewFactor * (cFinal - cErect);
        pDetails->shrinkageRotation = sign_correction * skewFactor * (sFinal - sErect);
        pDetails->relaxationRotation = sign_correction * skewFactor * (rFinal - rErect);
        pDetails->postTensionRotation = sign_correction * skewFactor * (pFinal - pErect);
    }
    else
    {
        pDetails->creepRotation = sign_correction * skewFactor * (RcreepFinal - RcreepErect);
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




    // Cyclic Rotations

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
    pDetails->maxDesignLLrotation = sign_correction * skewFactor * max;
    pDetails->maxConfigRotation = maxConfig;
    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
    pDetails->minDesignLLrotation = sign_correction * skewFactor * min;
    pDetails->minConfigRotation = minConfig;

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, maxBAT, bIncludeImpact, true, &min, &max);
    pDetails->maxPedRotation = sign_correction * skewFactor * max;

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, minBAT, bIncludeImpact, true, &min, &max);
    pDetails->minPedRotation = sign_correction * skewFactor * min;

    pDetails->maxUserLLrotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false);
    pDetails->minUserLLrotation = sign_correction * skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, minBAT, rtCumulative, false);


    Float64 minStaticRotation = sign_correction * skewFactor * (dcDF * minDCrotation + dwDF * minDWrotation) + pDetails->preTensionRotation +
        pDetails->creepRotation + pDetails->shrinkageRotation + pDetails->relaxationRotation + pDetails->postTensionRotation;

    Float64 maxStaticRotation = sign_correction * skewFactor * (dcDF * maxDCrotation + dwDF * maxDWrotation) + pDetails->preTensionRotation +
        pDetails->creepRotation + pDetails->shrinkageRotation + pDetails->relaxationRotation + pDetails->postTensionRotation;

    if (abs(maxStaticRotation) >= abs(minStaticRotation))
    {
        pDetails->staticRotation = maxStaticRotation;
    }
    else
    {
        pDetails->staticRotation = minStaticRotation;
    }

    Float64 minCyclicRotation = llDF * (pDetails->minDesignLLrotation + pDetails->minUserLLrotation + pDetails->minPedRotation);

    Float64 maxCyclicRotation = llDF * (pDetails->maxDesignLLrotation + pDetails->maxUserLLrotation + pDetails->maxPedRotation);

    if (abs(maxCyclicRotation) >= abs(minCyclicRotation))
    {
        pDetails->cyclicRotation = maxCyclicRotation;
    }
    else
    {
        pDetails->cyclicRotation = minCyclicRotation;
    }

    if (pDetails->staticRotation + pDetails->cyclicRotation >= 0)
    {
        pDetails->staticRotation += 0.005;
    }
    else
    {
        pDetails->staticRotation -= 0.005;
    }

    pDetails->totalRotation = pDetails->staticRotation + pDetails->cyclicRotation;
    
}



void pgsBearingDesignEngineer::GetBearingReactionDetails(const ReactionLocation& reactionLocation,
    CGirderKey girderKey, pgsTypes::AnalysisType analysisType, bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const


{


    GET_IFACE(IProductForces, pProdForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);

    pgsTypes::BridgeAnalysisType batSS = pgsTypes::SimpleSpan;
    pgsTypes::BridgeAnalysisType batCS = pgsTypes::ContinuousSpan;

    GET_IFACE(IBridge, pBridge);

    


    GET_IFACE(IIntervals, pIntervals);
    IntervalIndexType diaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
    IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
    IntervalIndexType ljIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
    IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
    IntervalIndexType constructionIntervalIdx = pIntervals->GetConstructionLoadInterval();
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
    IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();


    
    

    PierIndexType nPiers = pBridge->GetPierCount();


    

    GET_IFACE(IReactions, pReactions);
    std::unique_ptr<IProductReactionAdapter> pForces = std::make_unique<ProductForcesReactionAdapter>(pReactions, girderKey);

    const CGirderKey& thisGirderKey(reactionLocation.GirderKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);


    


    pDetails->erectedSegmentReaction = pForces->GetReaction(erectSegmentIntervalIdx, reactionLocation, pgsTypes::pftGirder, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
    pDetails->maxGirderReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftGirder, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

    if (pDetails->bDeck)
    {
        pDetails->diaphragmReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftDiaphragm, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
        pDetails->maxSlabReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlab, maxBAT);
        pDetails->minSlabReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlab, minBAT);
        pDetails->maxHaunchReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, maxBAT);
        pDetails->minHaunchReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, minBAT);
    }


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



    if (pDetails->bDeckPanels)
    {
        pDetails->maxSlabPanelReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, maxBAT);
        pDetails->minSlabPanelReaction = pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, minBAT);
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



    Float64 R1min{ 0 }, R1max{ 0 }, R2min(0), R2max{ 0 };

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


void pgsBearingDesignEngineer::GetThermalExpansionDetails(CGirderKey girderKey, BEARINGSHEARDEFORMATIONDETAILS* bearing) const
{

    GET_IFACE(IMaterials, pMaterials);
    const CSegmentKey& segmentKey(bearing->rPoi.GetSegmentKey());
    auto concreteType = pMaterials->GetSegmentConcreteType(segmentKey);

    Float64 inv_thermal_exp_coefficient;

    if (concreteType == pgsTypes::Normal)
    {
        inv_thermal_exp_coefficient = { WBFL::Units::ConvertToSysUnits(166666.6667, WBFL::Units::Measure::Fahrenheit) };
    }
    if (concreteType == pgsTypes::AllLightweight || concreteType == pgsTypes::SandLightweight)
    {
        inv_thermal_exp_coefficient = { WBFL::Units::ConvertToSysUnits(20000.0, WBFL::Units::Measure::Fahrenheit) };
    }

    Float64 thermal_expansion_coefficient = 1.0 / inv_thermal_exp_coefficient;
    bearing->thermal_expansion_coefficient = thermal_expansion_coefficient;
    bearing->max_design_temperature_cold = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    bearing->min_design_temperature_cold = WBFL::Units::ConvertToSysUnits(0.0, WBFL::Units::Measure::Fahrenheit);
    bearing->max_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    bearing->min_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Fahrenheit);

    GET_IFACE(ILibrary, pLib);
    GET_IFACE(ISpecification, pSpec);
    pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
    const auto pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
    const auto& thermalFactor = pSpecEntry->GetThermalMovementCriteria();

    bearing->percentExpansion = thermalFactor.ThermalMovementFactor;
    bearing->thermal_expansion_cold = bearing->percentExpansion * bearing->thermal_expansion_coefficient * bearing->length_pf * 
        (bearing->max_design_temperature_cold - bearing->min_design_temperature_cold);
    bearing->thermal_expansion_moderate = bearing->percentExpansion * bearing->thermal_expansion_coefficient * bearing->length_pf * 
        (bearing->max_design_temperature_moderate - bearing->min_design_temperature_moderate);
    bearing->total_shear_deformation_cold = bearing->thermal_expansion_cold + bearing->time_dependent;
    bearing->total_shear_deformation_moderate = bearing->thermal_expansion_moderate + bearing->time_dependent;

}

