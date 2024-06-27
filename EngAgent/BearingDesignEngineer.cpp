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


Float64 pgsBearingDesignEngineer::GetDistanceToPointOfFixity(const pgsPointOfInterest& poi_brg, SHEARDEFORMATIONDETAILS* pDetails) const
{

    GET_IFACE(IBridge, pBridge);
    GET_IFACE(IPointOfInterest, pPoi);

    Float64 L = 0;
    pgsPointOfInterest poi_fixity;

    SpanIndexType nSpans = pBridge->GetSpanCount();
    PierIndexType central_pierId = nSpans / 2;
    const CGirderKey& girderKey(poi_brg.GetSegmentKey());
        

    std::vector<CGirderKey> vGirderKeys;
    pBridge->GetGirderline(girderKey, &vGirderKeys);
    for (const auto& thisGirderKey : vGirderKeys)
    {
        SpanIndexType startSpanIdx, endSpanIdx;
        pBridge->GetGirderGroupSpans(ALL_GROUPS, &startSpanIdx, &endSpanIdx);
        for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
        {
            CSpanKey spanKey(spanIdx, thisGirderKey.girderIndex);

            PierIndexType pierIdx = PierIndexType(spanIdx);
            if (nSpans % 2 == 0)
            {
                
                if (pierIdx == central_pierId)
                {
                    poi_fixity = pPoi->GetPierPointOfInterest(girderKey, central_pierId);
                }
            }
            else
            {
                if (spanIdx == (int)ceil(nSpans / 2))
                {
                    PoiList vPoi;
                    pPoi->GetPointsOfInterest(spanKey, POI_SPAN | POI_5L, &vPoi);
                    poi_fixity = vPoi[0];
                }
            }

        }

    }

    pDetails->poi_fixity = poi_fixity;

    L = abs(pPoi->ConvertPoiToGirderlineCoordinate(poi_fixity) - pPoi->ConvertPoiToGirderlineCoordinate(poi_brg));

    return L;

}


Float64 pgsBearingDesignEngineer::GetTimeDependentComponentShearDeformation(const pgsPointOfInterest& poi, Float64 loss, SHEARDEFORMATIONDETAILS* pDetails) const
{


    GET_IFACE(ILosses, pLosses);
    GET_IFACE(IMaterials, pMaterials);
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);

    

    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
    auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    Float64 L = GetDistanceToPointOfFixity(poi, pDetails);


    Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE();

    pDetails->tendon_shortening = -(loss * L / Ep) / 3.0; // assumes 2/3 of creep and shrinkage occurs before girders are erected

    auto details = pLosses->GetLossDetails(poi, erectSegmentIntervalIdx);

    pDetails->flange_bottom_shortening = (1.0 + pDetails->ep * pDetails->yb / (pDetails->r * pDetails->r)) /
        (1 + pDetails->ep * pDetails->ep / (pDetails->r * pDetails->r)) * pDetails->tendon_shortening;
   
    return pDetails->flange_bottom_shortening;

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
            tdComponents->shrinkage = pRefined2005->ShrinkageLossBeforeDeckPlacement();
            tdComponents->creep = pRefined2005->CreepLossBeforeDeckPlacement();
            tdComponents->relaxation = pRefined2005->RelaxationLossBeforeDeckPlacement();
        }

            
        Float64 loss = tdComponents->creep + tdComponents->shrinkage + tdComponents->relaxation;

        return loss;
    }
}



Float64 pgsBearingDesignEngineer::GetTimeDependentShearDeformation( 
    const pgsPointOfInterest& poi, PierIndexType startPierIdx, SHEARDEFORMATIONDETAILS* pDetails) const
{

    
    GET_IFACE(IIntervals, pIntervals);
    GET_IFACE(IBridgeDescription, pIBridgeDesc);
    GET_IFACE(ILosses, pLosses);
    GET_IFACE(ISectionProperties, pSection);
    GET_IFACE(IStrandGeometry, pStrandGeom);


    // bearing time-dependent effects begin at the erect segment interval
    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
    IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
    IntervalIndexType castDeckIntervalIdx = pIntervals->GetLastCastDeckInterval();
    CSegmentKey seg_key = poi.GetSegmentKey();
    auto lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    auto details = pLosses->GetLossDetails(poi, erectSegmentIntervalIdx);

    pDetails->ep = pStrandGeom->GetEccentricity(erectSegmentIntervalIdx, poi, pgsTypes::Permanent).Y();

    pDetails->yb = pSection->GetNetYbg(erectSegmentIntervalIdx, poi);

    pDetails->Ixx = pSection->GetIxx(erectSegmentIntervalIdx, poi);

    pDetails->Ag = pSection->GetAg(erectSegmentIntervalIdx, poi);

    pDetails->r = sqrt(pDetails->Ixx / pDetails->Ag);

    const LOSSDETAILS* td_details_inf = pLosses->GetLossDetails(poi, lastIntervalIdx);
    TDCOMPONENTS components_inf;
    Float64 fpLossInfinity = GetBearingTimeDependentLosses(poi, pgsTypes::StrandType::Permanent, lastIntervalIdx, pgsTypes::IntervalTimeType::End, 
        nullptr, td_details_inf, &components_inf);

    // losses are sort of bogus bc thy don't actually go to ininity but just up to before deck casting............2/3 of that is before erection. There is slightly more after deck casting




    GET_IFACE(ILibrary, pLib);
    GET_IFACE(ISpecification, pSpec);
    pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
    const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
    PrestressLossCriteria::LossMethodType lossMethod = pSpecEntry->GetPrestressLossCriteria().LossMethod;
    bool bTimeStepMethod = lossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP;


    if (bTimeStepMethod)
    {

        std::ofstream file;
        file.open("td_output.csv");


        GET_IFACE(IBridge, pBridge);
        GET_IFACE(IProductForces, pProdForces);
        GET_IFACE(IPointOfInterest, pPoi);


        pDetails->creep = 0.0;
        pDetails->shrinkage = 0.0;
        pDetails->relaxation = 0.0;


        Float64 L = GetDistanceToPointOfFixity(poi, pDetails);
        pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
        GroupIndexType nGroups = pBridge->GetGirderGroupCount();
        GroupIndexType firstGroupIdx = (seg_key.groupIndex == ALL_GROUPS ? 0 : seg_key.groupIndex);
        GroupIndexType lastGroupIdx = (seg_key.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);

        

        for (IntervalIndexType intervalIdx = erectSegmentIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
        {

            Float64 prev_creep = pDetails->creep;
            Float64 prev_shrinkage = pDetails->shrinkage;
            Float64 prev_relax = pDetails->relaxation;

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
                std::stringstream coordinate_stream;


                for (IndexType idx = 1, nPoi = vPoi.size(); idx < nPoi; idx++)
                {
                    p0 = vPoi[idx - 1];
                    p1 = vPoi[idx];
                    d0 = pPoi->ConvertPoiToGirderlineCoordinate(p0);
                    d1 = pPoi->ConvertPoiToGirderlineCoordinate(p1);
                    if (d1 != d0)
                    {
                        const LOSSDETAILS* pDetails0 = pLosses->GetLossDetails(p0, intervalIdx);
                        const TIME_STEP_DETAILS& tsDetails0(pDetails0->TimeStepDetails[intervalIdx]);
                        Float64 strain_bot_girder_CR0 = 0.0;
                        Float64 strain_bot_girder_SH0 = 0.0;
                        Float64 strain_bot_girder_RE0 = 0.0;
                        strain_bot_girder_CR0 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftCreep][rtIncremental];
                        strain_bot_girder_SH0 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftShrinkage][rtIncremental];
                        strain_bot_girder_RE0 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftRelaxation][rtIncremental];

                        const LOSSDETAILS* pDetails1 = pLosses->GetLossDetails(p1, intervalIdx);
                        const TIME_STEP_DETAILS& tsDetails1(pDetails1->TimeStepDetails[intervalIdx]);
                        Float64 strain_bot_girder_CR1 = 0.0;
                        Float64 strain_bot_girder_SH1 = 0.0;
                        Float64 strain_bot_girder_RE1 = 0.0;
                        strain_bot_girder_CR1 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftCreep][rtIncremental];
                        strain_bot_girder_SH1 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftShrinkage][rtIncremental];
                        strain_bot_girder_RE1 = tsDetails0.Girder.strain_by_load_type[pgsTypes::BottomFace][pgsTypes::ProductForceType::pftRelaxation][rtIncremental];

                        Float64 avg_strain_BotCR = (strain_bot_girder_CR0 + strain_bot_girder_CR1) / 2.0;
                        Float64 avg_strain_BotSH = (strain_bot_girder_SH0 + strain_bot_girder_SH1) / 2.0;
                        Float64 avg_strain_BotRE = (strain_bot_girder_RE0 + strain_bot_girder_RE1) / 2.0;

                        pDetails->creep += avg_strain_BotCR * (d1 - d0);
                        pDetails->shrinkage += avg_strain_BotSH * (d1 - d0);
                        pDetails->relaxation += avg_strain_BotRE * (d1 - d0);

                        coordinate_stream << "," << strain_bot_girder_SH1;
                        
                        if (idx == nPoi-2)
                        {
                            if (intervalIdx == 6)
                            {
                                file << "event, end day,incremental shrinkage,cumulative shrinakge,incremental creep,cumulative creep,incremental relaxtion,cumulative relaxation, bottom shrinkage strain" << std::endl;
                            }

                            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                            std::string narrowStr = converter.to_bytes(pIntervals->GetDescription(intervalIdx));

                            narrowStr.erase(std::remove_if(narrowStr.begin(), narrowStr.end(), [](char c) { return c == ','; }), narrowStr.end());

                            file
                                << narrowStr << ","
                                << pIntervals->GetTime(intervalIdx, pgsTypes::IntervalTimeType::End) << ","
                                << pDetails->shrinkage - prev_shrinkage << "," << pDetails->shrinkage << ","
                                << pDetails->creep - prev_creep << "," << pDetails->creep << ","
                                << pDetails->relaxation - prev_relax << "," << pDetails->relaxation << coordinate_stream.str() << std::endl;
                                
                        }

                    }

                    
                    
                }

                
            }
        }

        file.close();


        Float64 total_time_dependent = pDetails->creep + pDetails->shrinkage + pDetails->relaxation;


        return total_time_dependent;
    }
    else
    {

        //calculate creep deformation
        Float64 creepLoss = components_inf.creep; // -components_erect.creep;
        pDetails->creep = GetTimeDependentComponentShearDeformation(poi, creepLoss, pDetails);
        pDetails->tendon_creep = pDetails->tendon_shortening;

        //calculate shrinkage deformation
        Float64 shrinkageLoss = components_inf.shrinkage; // -components_erect.shrinkage;
        pDetails->shrinkage = GetTimeDependentComponentShearDeformation(poi, shrinkageLoss, pDetails);
        pDetails->tendon_shrinkage = pDetails->tendon_shortening;

        //calculate relaxation deformation
        Float64 relaxationLoss = components_inf.relaxation; // -components_erect.relaxation;
        pDetails->relaxation = GetTimeDependentComponentShearDeformation(poi, relaxationLoss, pDetails);
        pDetails->tendon_relaxation = pDetails->tendon_shortening;

        Float64 sum_components = (pDetails->creep + pDetails->shrinkage + pDetails->relaxation);

        //calculate total time-dependent shear deformation
        Float64 tdLoss = fpLossInfinity; //- fpLossErect;
        Float64 total_time_dependent = GetTimeDependentComponentShearDeformation(poi, tdLoss, pDetails);

        pDetails->total_tendon_shortening = pDetails->tendon_shortening;

        //ASSERT(IsEqual(total_time_dependent, sum_components)); // use if differential shrinkage effects are considered

        return total_time_dependent;

    }

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


void pgsBearingDesignEngineer::GetThermalExpansionDetails(const pgsPointOfInterest& poi, SHEARDEFORMATIONDETAILS* pDetails) const
{

    Float64 L = 0;
    L = GetDistanceToPointOfFixity(poi, pDetails);
    pDetails->length_pf = L;

    Float64 inv_thermal_exp_coefficient = { WBFL::Units::ConvertToSysUnits(166666.6667, WBFL::Units::Measure::Fahrenheit) };
    Float64 thermal_expansion_coefficient = 1.0 / inv_thermal_exp_coefficient;
    pDetails->thermal_expansion_coefficient = thermal_expansion_coefficient;

    pDetails->max_design_temperature_cold = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->min_design_temperature_cold = WBFL::Units::ConvertToSysUnits(0.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->max_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(80.0, WBFL::Units::Measure::Fahrenheit);
    pDetails->min_design_temperature_moderate = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Fahrenheit);

    GET_IFACE(ILibrary, pLib);
    GET_IFACE(ISpecification, pSpec);
    pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
    const auto pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
    const auto& thermalFactor = pSpecEntry->GetThermalMovementCriteria();


    pDetails->percentExpansion = thermalFactor.ThermalMovementFactor;



    pDetails->thermal_expansion_cold = -pDetails->percentExpansion * pDetails->thermal_expansion_coefficient * L * (pDetails->max_design_temperature_cold - pDetails->min_design_temperature_cold);
    pDetails->thermal_expansion_moderate = -pDetails->percentExpansion * pDetails->thermal_expansion_coefficient * L * (pDetails->max_design_temperature_moderate - pDetails->min_design_temperature_moderate);

    pDetails->total_shear_deformation_cold = pDetails->thermal_expansion_cold + pDetails->time_dependent;
    pDetails->total_shear_deformation_moderate = pDetails->thermal_expansion_moderate + pDetails->time_dependent;

}

