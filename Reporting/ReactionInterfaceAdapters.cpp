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

#include "StdAfx.h"
#include <Reporting\ReactionInterfaceAdapters.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Bridge.h>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// MISCELLANEOUS
//
bool DoDoReportAtPier(IntervalIndexType intervalIdx,PierIndexType pierIdx,const CGirderKey& girderKey, IBearingDesign* pointer)
{
   std::vector<PierIndexType> vPiers = pointer->GetBearingReactionPiers(intervalIdx,girderKey);
   std::vector<PierIndexType>::iterator found = std::find(vPiers.begin(),vPiers.end(),pierIdx);
   return found != vPiers.end() ? true : false;
}

void LabelPier(std::_tostringstream& os, PierIndexType pierIdx, PierIndexType nPiers, PierReactionFaceType face)
{
   if (pierIdx == 0 || pierIdx == nPiers-1 )
   {
      os << _T("Abutment ") << LABEL_PIER(pierIdx);
   }
   else
   {
      os << _T("Pier ") << LABEL_PIER(pierIdx);
   }

   if (face == rftBack)
   {
      os << _T(" - Back");
   }
   else if (face == rftAhead)
   {
      os << _T(" - Ahead");
   }
}         

ReactionLocation MakeReactionLocation(PierIndexType pierIdx, PierIndexType nPiers, PierReactionFaceType face, const CGirderKey& girderKey)
{
   ReactionLocation location;
   location.PierIdx = pierIdx;
   location.Face = face;
   location.GirderKey = girderKey; // analysis agent should sort out if pier has different # girders framing into it

   std::_tostringstream os;
   LabelPier(os, pierIdx, nPiers, location.Face);

   location.PierLabel = os.str();

   return location;
}

ReactionLocationContainer CmbLsBearingDesignReactionAdapter::GetBearingReactionLocations(IntervalIndexType intervalIdx,const CGirderKey& girderKey, 
                                                      IBridge* pBridge, IBearingDesign* pBearing)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(thisGirderKey.groupIndex,&startPierIdx,&endPierIdx);

      std::vector<PierIndexType> vPiers = pBearing->GetBearingReactionPiers(intervalIdx,thisGirderKey);
      for (const auto& pierIdx : vPiers)
      {
         PierReactionFaceType face;
         if ( pierIdx == startPierIdx )
         {
            face = rftAhead;
         }
         else if ( pierIdx == endPierIdx )
         {
            face = rftBack;
         }
         else
         {
            face = rftMid;
         }

         ReactionLocation location = MakeReactionLocation(pierIdx, nPiers, face, thisGirderKey);
         container.push_back( location );
      }
   }

   return container;
}

ReactionLocationContainer GetPierReactionLocations(const CGirderKey& girderKey, IBridge* pBridge)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPierIdx, endPierIdx;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      startPierIdx = 0;
      endPierIdx = nPiers-1;
   }
   else
   {
      pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   }

   for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
   {
      GroupIndexType backGroupIdx,aheadGroupIdx;
      pBridge->GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);

      GroupIndexType grpIdx = (aheadGroupIdx == INVALID_INDEX ? backGroupIdx : aheadGroupIdx);
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);

      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(girderKey.girderIndex, grpIdx, grpIdx, &vGirderKeys);
      ATLASSERT(vGirderKeys.size() == 1);
      const auto& thisGirderKey = vGirderKeys.front();

      PierReactionFaceType face = rftMid; // pier reactions are always for the whole pier, not just a face
      ReactionLocation location = MakeReactionLocation(pierIdx, nPiers, face, thisGirderKey);
      container.push_back( location );
   }

   return container;
}

/****************************************************************************
CLASS
   ProductForcesReactionAdapter
   A simple iterator class for walking reaction locations
****************************************************************************/
ReactionLocationIter::ReactionLocationIter(const ReactionLocationContainer& container):
   m_rContainer(container)
{
   m_Iter = m_rContainer.begin();
}

void ReactionLocationIter::First()
{
   m_Iter = m_rContainer.begin();
}

void ReactionLocationIter::Next()
{
   m_Iter++;
}

bool ReactionLocationIter::IsDone()
{
   return m_Iter==m_rContainer.end();
}

const ReactionLocation& ReactionLocationIter::CurrentItem()
{
   ATLASSERT(!IsDone());
   return *m_Iter;
}

/****************************************************************************
CLASS
   ProductForcesReactionAdapter
****************************************************************************/

ProductForcesReactionAdapter::ProductForcesReactionAdapter(IReactions* pReactions,const CGirderKey& girderKey):
m_pReactions(pReactions), m_GirderKey(girderKey)
{
}

ProductForcesReactionAdapter::~ProductForcesReactionAdapter()
{
   m_Locations.clear();
}

ReactionLocationIter ProductForcesReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   if (m_Locations.empty())
   {
      m_Locations = GetPierReactionLocations(m_GirderKey, pBridge);
   }

   return ReactionLocationIter(m_Locations);
}

bool ProductForcesReactionAdapter::DoReportAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   return true; // always report pier reactions for all piers
}

Float64 ProductForcesReactionAdapter::GetReaction(IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat)
{
   return m_pReactions->GetReaction(rLocation.GirderKey,rLocation.PierIdx,pgsTypes::stPier,intervalIdx,pfType,bat,rtCumulative).Fy;
}

void ProductForcesReactionAdapter::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType, const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   REACTION Rmin, Rmax;
   m_pReactions->GetLiveLoadReaction(intervalIdx, llType, rLocation.PierIdx, rLocation.GirderKey, bat, bIncludeImpact, pgsTypes::fetFy, &Rmin, &Rmax, pMinConfig, pMaxConfig);
   *pRmin = Rmin.Fy;
   *pRmax = Rmax.Fy;
}

/****************************************************************************
CLASS
   BearingDesignProductReactionAdapter
****************************************************************************/
BearingDesignProductReactionAdapter::BearingDesignProductReactionAdapter(IBearingDesign* pForces, IntervalIndexType intervalIdx, const CGirderKey& girderKey):
   m_pBearingDesign(pForces),
   m_GirderKey(girderKey),
   m_IntervalIdx(intervalIdx)
{
}

BearingDesignProductReactionAdapter::~BearingDesignProductReactionAdapter()
{
   m_Locations.clear();
}

ReactionLocationIter BearingDesignProductReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = CmbLsBearingDesignReactionAdapter::GetBearingReactionLocations(m_IntervalIdx, m_GirderKey, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool BearingDesignProductReactionAdapter::DoReportAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   return DoDoReportAtPier(m_IntervalIdx, pierIdx, girderKey, m_pBearingDesign);
}

Float64 BearingDesignProductReactionAdapter::GetReaction(IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat)
{
   return m_pBearingDesign->GetBearingProductReaction(intervalIdx,rLocation,pfType,bat,rtCumulative);
}

void BearingDesignProductReactionAdapter::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType, const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   Float64 Tmin, Tmax;
   m_pBearingDesign->GetBearingLiveLoadReaction(intervalIdx, rLocation, llType, bat, bIncludeImpact, bIncludeLLDF, pRmin, pRmax, &Tmin, &Tmax, pMinConfig, pMaxConfig);
}

/////////////////////////////////////////////
// class CmbLsBearingDesignReactionAdapter
/////////////////////////////////////////////

CombinedLsForcesReactionAdapter::CombinedLsForcesReactionAdapter(IReactions* pReactions, ILimitStateForces* pForces, const CGirderKey& girderKey):
   m_pReactions(pReactions), m_LsPointer(pForces), m_GirderKey(girderKey)
{;}

CombinedLsForcesReactionAdapter::~CombinedLsForcesReactionAdapter()
{
   m_Locations.clear();
}

ReactionLocationIter CombinedLsForcesReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   if (m_Locations.empty())
   {
      m_Locations = GetPierReactionLocations(m_GirderKey, pBridge);
   }

   return ReactionLocationIter(m_Locations);
}

bool CombinedLsForcesReactionAdapter::DoReportAtPier(PierIndexType pier,const CGirderKey& girderKey)
{
   return true; // always report for pier reactions
}

Float64 CombinedLsForcesReactionAdapter::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   return m_pReactions->GetReaction(rLocation.GirderKey,rLocation.PierIdx,pgsTypes::stPier,intervalIdx,combo,bat,resultsType).Fy;
}

void CombinedLsForcesReactionAdapter::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   m_pReactions->GetCombinedLiveLoadReaction(intervalIdx, llType, rLocation.PierIdx, rLocation.GirderKey, bat, bIncludeImpact, pRmin, pRmax);
}

/////////////////////////////////////////////
// class CmbLsBearingDesignReactionAdapter
/////////////////////////////////////////////
CmbLsBearingDesignReactionAdapter::CmbLsBearingDesignReactionAdapter(IBearingDesign* pForces, IntervalIndexType intervalIdx, const CGirderKey& girderKey):
   m_pBearingDesign(pForces),
   m_IntervalIdx(intervalIdx),
   m_GirderKey(girderKey)
{
}

CmbLsBearingDesignReactionAdapter::~CmbLsBearingDesignReactionAdapter()
{
   m_Locations.clear();
}

ReactionLocationIter CmbLsBearingDesignReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = GetBearingReactionLocations(m_IntervalIdx, m_GirderKey, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool CmbLsBearingDesignReactionAdapter::DoReportAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   return DoDoReportAtPier(m_IntervalIdx, pierIdx, girderKey, m_pBearingDesign);
}

Float64 CmbLsBearingDesignReactionAdapter::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   return m_pBearingDesign->GetBearingCombinedReaction(intervalIdx, rLocation, combo, bat, resultsType);
}

void CmbLsBearingDesignReactionAdapter::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   m_pBearingDesign->GetBearingCombinedLiveLoadReaction(intervalIdx, rLocation, llType, bat, bIncludeImpact, pRmin, pRmax);
}

/////////////////////////////////////////////
// class ReactionDecider
/////////////////////////////////////////////
ReactionDecider::ReactionDecider(ReactionTableType tableType, const ReactionLocation& location,const CGirderKey& girderKey,IBridge* pBridge,IIntervals* pIntervals)
{
   // full pier reactions are always reported
   if (tableType == PierReactionsTable)
   {
      m_bAlwaysReport = true;
   }
   else
   {
      // Always report bearing data if simple supports always
      bool bIntegralOnLeft, bIntegralOnRight;
      pBridge->IsIntegralAtPier(location.PierIdx, &bIntegralOnLeft, &bIntegralOnRight);

      bool bContinuousOnLeft, bContinuousOnRight;
      pBridge->IsContinuousAtPier(location.PierIdx,&bContinuousOnLeft,&bContinuousOnRight);

      bool bIsSimple(true);
      if(location.Face == rftBack)
      {
         bIsSimple = !(bIntegralOnLeft || bContinuousOnLeft);
      }
      else if(location.Face == rftAhead)
      {
         bIsSimple = !(bIntegralOnRight || bContinuousOnRight);
      }
      else
      {
         // always report for mid-locations... set bIsSimple to true so that m_bAlwaysReport will be set to true below
         bIsSimple = true;
      }

      if (bIsSimple)
      {
         m_bAlwaysReport = true;
      }
      else
      {
         m_bAlwaysReport = false; // when we report is based on stage when BC becomes continuous

         IntervalIndexType back_continuity_interval, ahead_continuity_interval;
         pIntervals->GetContinuityInterval(location.PierIdx,&back_continuity_interval,&ahead_continuity_interval);

         m_ThresholdInterval = (location.Face == rftBack ? back_continuity_interval : ahead_continuity_interval);
      }
   }
}

// If true, report results
bool ReactionDecider::DoReport(IntervalIndexType intervalIdx)
{
   if ( intervalIdx == INVALID_INDEX )
   {
      // no overlay
      return false;
   }

   if (m_bAlwaysReport)
   {
      return true;
   }
   else
   {
      return intervalIdx < m_ThresholdInterval;
   }
}
