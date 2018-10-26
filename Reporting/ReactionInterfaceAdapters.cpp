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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(girderKey.groupIndex);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(girderKey.groupIndex);

   if (pierIdx < startPierIdx || endPierIdx < pierIdx)
   {
      return false;
   }
   else
   {
      bool bBack,bAhead;
      pointer->AreBearingReactionsAvailable(intervalIdx,girderKey, &bBack, &bAhead);
      if (pierIdx == startPierIdx)
      {
         return bBack;
      }
      else if ( pierIdx == endPierIdx )
      {
         return bAhead;
      }
      else
      {
         // this is a pier within a group (splice girder) and girder bearing reactions aren't available here
#pragma Reminder("REVIEW: are girder bearing reactions available before continuity?")
         return false;
      }
   }
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

ReactionLocationContainer GetBearingReactionLocations(IntervalIndexType intervalIdx,const CGirderKey& girderKey, 
                                                      IBridge* pBridge, IBearingDesign* pBearing)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (girderKey.girderIndex < nGirders ? girderKey.girderIndex : nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(grpIdx,&startPierIdx,&endPierIdx);

      bool bleft, bright;
      pBearing->AreBearingReactionsAvailable(intervalIdx, thisGirderKey, &bleft, &bright);
      if( bleft )
      {
         ReactionLocation location = MakeReactionLocation(startPierIdx, nPiers, rftAhead, thisGirderKey);
         container.push_back( location );
      }

      if( bright)
      {
         ReactionLocation location = MakeReactionLocation(endPierIdx, nPiers, rftBack, thisGirderKey);
         container.push_back( location );
      }
   }

   return container;
}

ReactionLocationContainer GetPierReactionLocations(const CGirderKey& girderKey, IBridge* pBridge)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();
   if (girderKey.groupIndex == ALL_GROUPS)
   {
      for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
      {
         ReactionLocation location = MakeReactionLocation(pierIdx, nPiers, rftMid, girderKey);
         container.push_back( location );
      }
   }
   else
   {
      // For one group we have two piers
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);

      ReactionLocation location = MakeReactionLocation(startPierIdx, nPiers, rftMid, girderKey);
      container.push_back( location );

      location = MakeReactionLocation(endPierIdx, nPiers, rftMid, girderKey);
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

ProductForcesReactionAdapter::ProductForcesReactionAdapter(IProductForces* pForces, const CGirderKey& girderKey):
   m_Pointer(pForces), m_GirderKey(girderKey)
{
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

Float64 ProductForcesReactionAdapter::GetReaction(IntervalIndexType intervalIdx,const ReactionLocation& rLocation,ProductForceType type,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_Pointer->GetReaction(intervalIdx, type, rLocation.PierIdx, rLocation.GirderKey, bat, ctIncremental);
}

void ProductForcesReactionAdapter::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx, const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   ATLASSERT(rLocation.Face == rftMid);

   m_Pointer->GetLiveLoadReaction(llType, intervalIdx, rLocation.PierIdx, rLocation.GirderKey, bat, bIncludeImpact, bIncludeLLDF, pRmin, pRmax, pMinConfig, pMaxConfig);
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

ReactionLocationIter BearingDesignProductReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = GetBearingReactionLocations(m_IntervalIdx, m_GirderKey, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool BearingDesignProductReactionAdapter::DoReportAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   return DoDoReportAtPier(m_IntervalIdx, pierIdx, girderKey, m_pBearingDesign);
}

Float64 BearingDesignProductReactionAdapter::GetReaction(IntervalIndexType intervalIdx,const ReactionLocation& rLocation,ProductForceType type,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

   Float64 Rleft, Rright;
   m_pBearingDesign->GetBearingProductReaction(intervalIdx, type, rLocation.GirderKey, ctIncremental, bat, &Rleft, &Rright);

   return rLocation.Face==rftAhead ? Rleft : Rright;
}

void BearingDesignProductReactionAdapter::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx, const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

 // convert from span to pier locations
  Float64 LeftRmin, LeftRmax, LeftTmin, LeftTmax;
  Float64 RightRmin, RightRmax, RightTmin, RightTmax;
  VehicleIndexType LeftMinConfig , LeftMaxConfig;
  VehicleIndexType RightMinConfig, RightMaxConfig;

  m_pBearingDesign->GetBearingLiveLoadReaction(llType, intervalIdx, rLocation.GirderKey, bat, bIncludeImpact, bIncludeLLDF, 
                                        &LeftRmin, &LeftRmax, &LeftTmin, &LeftTmax, &RightRmin, &RightRmax, &RightTmin, &RightTmax,
                                        &LeftMinConfig, &LeftMaxConfig, &RightMinConfig, &RightMaxConfig);

  if(rLocation.Face==rftAhead)
  {
      *pRmin = LeftRmin;
      *pRmax = LeftRmax;
      if (pMinConfig!=NULL)
      {
         *pMinConfig = LeftMinConfig;
         *pMaxConfig = LeftMaxConfig;
      }
  }
  else
  {
      *pRmin = RightRmin;
      *pRmax = RightRmax;
      if (pMinConfig!=NULL)
      {
         *pMinConfig = RightMinConfig;
         *pMaxConfig = RightMaxConfig;
      }
  }
}

/////////////////////////////////////////////
// class CmbLsBearingDesignReactionAdapter
/////////////////////////////////////////////

CombinedLsForcesReactionAdapter::CombinedLsForcesReactionAdapter(ICombinedForces* pCmbForces, ILimitStateForces* pForces, const CGirderKey& girderKey):
   m_CmbPointer(pCmbForces), m_LsPointer(pForces), m_GirderKey(girderKey)
{;}

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

Float64 CombinedLsForcesReactionAdapter::GetReaction(LoadingCombination combo,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_CmbPointer->GetReaction(combo, intervalIdx, rLocation.PierIdx, rLocation.GirderKey, type, bat);
}

void CombinedLsForcesReactionAdapter::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_CmbPointer->GetCombinedLiveLoadReaction(llType, intervalIdx, rLocation.PierIdx, rLocation.GirderKey, bat, bIncludeImpact, pRmin, pRmax);
}

// From ILimitStateForces
void CombinedLsForcesReactionAdapter::GetReaction(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_LsPointer->GetReaction(ls, intervalIdx, rLocation.PierIdx, rLocation.GirderKey, bat, bIncludeImpact, pMin, pMax);
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

ReactionLocationIter CmbLsBearingDesignReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = GetBearingReactionLocations(m_IntervalIdx, m_GirderKey, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool CmbLsBearingDesignReactionAdapter::DoReportAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   return DoDoReportAtPier(m_IntervalIdx, pierIdx, girderKey, m_pBearingDesign);
}

Float64 CmbLsBearingDesignReactionAdapter::GetReaction(LoadingCombination combo,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,CombinationType type,pgsTypes::BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

   Float64 Rleft, Rright;
   m_pBearingDesign->GetBearingCombinedReaction(combo, intervalIdx, rLocation.GirderKey, type, bat, &Rleft, &Rright);

   return rLocation.Face==rftAhead ? Rleft : Rright;
}

void CmbLsBearingDesignReactionAdapter::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

  Float64 LeftRmin, LeftRmax;
  Float64 RightRmin, RightRmax;
  m_pBearingDesign->GetBearingCombinedLiveLoadReaction(llType, intervalIdx, rLocation.GirderKey, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);

  if(rLocation.Face==rftAhead)
  {
     *pRmin = LeftRmin;
     *pRmax = LeftRmax;
  }
  else
  {
     *pRmin = RightRmin;
     *pRmax = RightRmax;
  }
}

void CmbLsBearingDesignReactionAdapter::GetReaction(pgsTypes::LimitState ls,IntervalIndexType intervalIdx,const ReactionLocation& rLocation,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

   Float64 LeftRmin, LeftRmax;
   Float64 RightRmin, RightRmax;
   m_pBearingDesign->GetBearingLimitStateReaction(ls, intervalIdx, rLocation.GirderKey, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);

   if(rLocation.Face==rftAhead)
   {
      *pRmin = LeftRmin;
      *pRmax = LeftRmax;
   }
   else
   {
      *pRmin = RightRmin;
      *pRmax = RightRmax;
   }
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

      bool isSimple(true);
      if(location.Face == rftBack)
      {
         isSimple = !(bIntegralOnLeft || bContinuousOnLeft);
      }
      else if(location.Face == rftAhead)
      {
         isSimple = !(bIntegralOnRight || bContinuousOnRight);
      }
      else
      {
         ATLASSERT(0); // should not happen for bearing location
      }

      if (isSimple)
      {
         m_bAlwaysReport = true;
      }
      else
      {
         m_bAlwaysReport = false; // when we report is based on stage when BC becomes continuous

         EventIndexType back_continunity_event, ahead_continuity_event;
         pBridge->GetContinuityEventIndex(location.PierIdx,&back_continunity_event,&ahead_continuity_event);

         IntervalIndexType back_continuity_interval  = pIntervals->GetInterval(girderKey,back_continunity_event);
         IntervalIndexType ahead_continuity_interval = pIntervals->GetInterval(girderKey,ahead_continuity_event);

         m_ThresholdInterval = (location.Face == rftBack ? back_continuity_interval : ahead_continuity_interval);
      }
   }
}

// If true, report results
bool ReactionDecider::DoReport(IntervalIndexType intervalIdx)
{
   if (m_bAlwaysReport)
   {
      return true;
   }
   else
   {
      return intervalIdx < m_ThresholdInterval;
   }
}
