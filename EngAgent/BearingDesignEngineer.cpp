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
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace/Limits.h>
#include <LRFD\Rebar.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/LimitStateConcreteStrengthCriteria.h>

#include <PgsExt\statusitem.h>
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


Float64 pgsBearingDesignEngineer::GetBearingCyclicRotation(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
    const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural) const
{

    GET_IFACE(IProductForces, pProductForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);

    GET_IFACE(IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
    Float64 min, max;
    VehicleIndexType minConfig, maxConfig;

    auto skewFactor = this->BearingSkewFactor(reactionLocation, isFlexural);

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
    Float64 cyclic_rotation = skewFactor * (max + pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false));
    return cyclic_rotation;

}


void pgsBearingDesignEngineer::GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
    const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const
{

    Float64 pMin, pMax;
    Float64 min, max;
    VehicleIndexType minConfig, maxConfig;

    GET_IFACE(IProductForces, pProductForces);
    pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
    pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);

    GET_IFACE(IIntervals, pIntervals);
    IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
    IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

    GET_IFACE(ICombinedForces, comboForces);


    GET_IFACE(ILimitStateForces, limitForces);
    limitForces->GetRotation(lastIntervalIdx, pgsTypes::ServiceI, poi, maxBAT, 
        true, true, true, true, true, &pMin, &pMax);

    pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);

    const CSegmentKey& segmentKey(poi.GetSegmentKey());
    IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
    IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

    auto skewFactor = this->BearingSkewFactor(reactionLocation, isFlexural);

    pDetails->maxDCrotation = skewFactor * comboForces->GetRotation(lastIntervalIdx, lcDC, poi, maxBAT, rtCumulative);
    pDetails->maxDWrotation = skewFactor * comboForces->GetRotation(lastIntervalIdx, lcDW, poi, minBAT, rtCumulative);
    pDetails->cyclicRotation = skewFactor * this->GetBearingCyclicRotation(analysisType, poi, reactionLocation, bIncludeImpact, bIncludeLLDF, isFlexural);
    pDetails->maxUserDCRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, maxBAT, rtCumulative, false);
    pDetails->minUserDCRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDC, poi, minBAT, rtCumulative, false);
    pDetails->maxUserDWRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, maxBAT, rtCumulative, false);
    pDetails->minUserDWRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserDW, poi, minBAT, rtCumulative, false);
    pDetails->maxUserLLrotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, maxBAT, rtCumulative, false);
    pDetails->minUserLLrotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftUserLLIM, poi, minBAT, rtCumulative, false);
    pDetails->serviceIRotation = skewFactor * pMax;
    pDetails->preTensionRotation = skewFactor * pProductForces->GetRotation(releaseIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false);


    GET_IFACE_NOCHECK(ICamber, pCamber);
    Float64 Dcreep1, Rcreep1;
    pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &Dcreep1, &Rcreep1);

    GET_IFACE(ILossParameters, pLossParams);
    bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

    if (bTimeStep)
    {
        pDetails->creepRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false);
        pDetails->shrinkageRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false);
        pDetails->relaxationRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false);
        pDetails->postTensionRotation = skewFactor * pProductForces->GetRotation(releaseIntervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false);

    }
    else
    {
        pDetails->creepRotation = skewFactor * Rcreep1;
        pDetails->shrinkageRotation = 0.0;
        pDetails->relaxationRotation = 0.0;
        pDetails->postTensionRotation = 0.0;
    }
    pDetails->totalTimeDependentRotation = skewFactor*(pDetails->preTensionRotation + pDetails->creepRotation + 
        pDetails->shrinkageRotation + pDetails->relaxationRotation + pDetails->postTensionRotation);

    pDetails->maxDesignLLrotation = skewFactor * max;
    pDetails->minDesignLLrotation = skewFactor * min;
    pDetails->girderRotation = skewFactor * pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false);
    pDetails->diaphragmRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftDiaphragm, poi, maxBAT, rtCumulative, false);
    pDetails->maxSlabRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false);
    pDetails->maxHaunchRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false);
    pDetails->minHaunchRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false);
    pDetails->maxRailingSystemRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false);
    pDetails->minRailingSystemRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, minBAT, rtCumulative, false);
    pDetails->maxFutureOverlayRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false);
    pDetails->minFutureOverlayRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftOverlay, poi, minBAT, rtCumulative, false);
    pDetails->longitudinalJointRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false);
    pDetails->maxConstructionRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, maxBAT, rtCumulative, false);
    pDetails->minConstructionRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction, poi, minBAT, rtCumulative, false);
    pDetails->maxSlabPanelRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, maxBAT, rtCumulative, false);
    pDetails->minSlabPanelRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel, poi, minBAT, rtCumulative, false);
    pDetails->maxSidewalkRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false);
    pDetails->minSidewalkRotation = skewFactor * pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, minBAT, rtCumulative, false);


}



