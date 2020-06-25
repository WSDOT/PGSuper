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

#include "stdafx.h"
#include "IntervalManager.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFStatusCenter.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\Helpers.h>

#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\StatusItem.h>

#include <MfcTools\Exceptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined ASSERT_VALID
#undef ASSERT_VALID
#endif

#if defined _DEBUG
#define ASSERT_VALID AssertValid()
#else
#define ASSERT_VALID
#endif

inline CGirderKey GetSafeGirderKey(IBroker* pBroker,const CGirderKey& oldKey)
{
   // called if there is an unequal number of girders per group, and this one has less. use the right-most girder
   GET_IFACE2(pBroker,IBridge,pBridge);

   GirderIndexType gdrIdx = pBridge->GetGirderCount(oldKey.groupIndex)-1;
   ATLASSERT(gdrIdx < oldKey.girderIndex); // our assumption is wrong
   return CGirderKey(oldKey.groupIndex, gdrIdx);
}

inline CSegmentKey GetSafeSegmentKey(IBroker* pBroker,const CSegmentKey& key)
{
   return CSegmentKey(GetSafeGirderKey(pBroker,key), key.segmentIndex);
}

CIntervalManager::CIntervalManager()
{
   m_bIsPGSuper = false;
   m_pBroker = nullptr;
   m_StatusGroupID = INVALID_ID;
}

void CIntervalManager::Init(IBroker* pBroker, StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter, pStatusCenter);
   m_scidTimelineError = pStatusCenter->RegisterCallback(new pgsTimelineStatusCallback(m_pBroker,eafTypes::statusError));
}

void CIntervalManager::BuildIntervals(const CTimelineManager* pTimelineMgr)
{
   // this method builds the analysis interval sequence as well as defines the stage model
   // for the generic bridge model.
   int result = pTimelineMgr->Validate();
   if (result != TLM_SUCCESS)
   {
      CString strError = pTimelineMgr->GetErrorMessage(result).c_str();

      strError += _T("\nUse the timeline manager to correct the error.");
      GET_IFACE(IEAFStatusCenter, pStatusCenter);

      pgsTimelineStatusItem* pStatusItem = new pgsTimelineStatusItem(m_StatusGroupID, m_scidTimelineError, strError);
      pStatusCenter->Add(pStatusItem);

      strError += _T("\n\nSee the Status Center for details");
   }

   GET_IFACE(IDocumentType,pDocType);
   m_bIsPGSuper = pDocType->IsPGSuperDocument();

   // reset everything
   m_CastIntermediateDiaphragmsIntervalIdx = INVALID_INDEX;
   m_CastLongitudinalJointsIntervalIdx = INVALID_INDEX;
   m_CompositeLongitudinalJointsIntervalIdx = INVALID_INDEX;
   
   EventIndexType castDeckEventIdx = pTimelineMgr->GetCastDeckEventIndex();
   IndexType nRegions = (castDeckEventIdx == INVALID_INDEX ? 0 : pTimelineMgr->GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity().GetCastingRegionCount());
   if (0 < nRegions)
   {
      m_vCastDeckIntervalIdx.assign(nRegions, INVALID_INDEX);
      m_vCompositeDeckIntervalIdx.assign(nRegions, INVALID_INDEX);
   }
   else
   {
      m_vCastDeckIntervalIdx.clear();
      m_vCompositeDeckIntervalIdx.clear();
   }

   m_LiveLoadIntervalIdx      = INVALID_INDEX;
   m_OverlayIntervalIdx       = INVALID_INDEX;
   m_RailingSystemIntervalIdx = INVALID_INDEX;
   m_UserLoadInterval[UserLoads::DC].clear();
   m_UserLoadInterval[UserLoads::DW].clear();

   m_StressStrandIntervals.clear();
   m_ReleaseIntervals.clear();

   m_Intervals.clear();
   m_SegmentLiftingIntervals.clear();
   m_SegmentStorageIntervals.clear();
   m_SegmentHaulingIntervals.clear();
   m_SegmentErectionIntervals.clear();
   m_CastClosureIntervals.clear();
   m_StrandStressingSequenceIntervalLimits.clear();
   m_ReleaseSequenceIntervalLimits.clear();
   m_LiftingSequenceIntervalLimits.clear();
   m_StorageSequenceIntervalLimits.clear();
   m_SegmentErectionSequenceIntervalLimits.clear();
   m_RemoveTemporaryStrandsIntervals.clear();

   m_ErectPierIntervals.clear();
   m_ErectTemporarySupportIntervals.clear();
   m_RemoveTemporarySupportIntervals.clear();

   m_SegmentTendonStressingIntervals.clear();

   m_GirderTendonStressingIntervals.clear();

   const CBridgeDescription2* pBridgeDesc = pTimelineMgr->GetBridgeDescription();

   // work through all the events in the timeline and the build analysis intervals
   // a single timeline event results in one or more analysis intervals
   // This is our SmartEvent technology!
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   ATLASSERT(0 < nEvents);
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      ProcessStep1(eventIdx,pTimelineEvent); // support erection
      ProcessStep2(eventIdx,pTimelineEvent); // zero duration construction events
      ProcessStep3(eventIdx,pTimelineEvent); // concrete curing intervals
      ProcessStep4(eventIdx,pTimelineEvent); // external loadings
      ProcessStep5(eventIdx,pTimelineEvent); // interval to start of next event
   } // next event
   

   ASSERT_VALID;
}


IntervalIndexType CIntervalManager::GetIntervalCount() const
{
   return m_Intervals.size();
}

EventIndexType CIntervalManager::GetStartEvent(IntervalIndexType idx) const
{
   return m_Intervals[idx].StartEventIdx;
}

EventIndexType CIntervalManager::GetEndEvent(IntervalIndexType idx) const
{
   return m_Intervals[idx].EndEventIdx;
}

Float64 CIntervalManager::GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const
{
   if (idx == INVALID_INDEX)
   {
      return 0;
   }

   Float64 time;
   switch(timeType)
   {
   case pgsTypes::Start:
      time = m_Intervals[idx].Start;
      break;

   case pgsTypes::Middle:
      time = m_Intervals[idx].Middle;
      break;

   case pgsTypes::End:
      time = m_Intervals[idx].End;
      break;

   default:
      ATLASSERT(false);
   }

   return time;
}

Float64 CIntervalManager::GetDuration(IntervalIndexType idx) const
{
#if defined _DEBUG
   ATLASSERT(m_Intervals[idx].Duration == GetTime(idx,pgsTypes::End) - GetTime(idx,pgsTypes::Start));
#endif
   return m_Intervals[idx].Duration;
}

LPCTSTR CIntervalManager::GetDescription(IntervalIndexType idx) const
{
   return m_Intervals[idx].Description.c_str();
}

IntervalIndexType CIntervalManager::GetInterval(EventIndexType eventIdx) const
{
   IntervalIndexType nIntervals = m_Intervals.size();
   for ( IntervalIndexType i = 0; i < nIntervals; i++ )
   {
      if ( m_Intervals[i].StartEventIdx == eventIdx )
      {
         return i;
      }
   }

   // if this is the last event, the return the last interval
   if ( m_Intervals.back().EndEventIdx == eventIdx )
   {
      return m_Intervals.size()-1;
   }

   ATLASSERT(false); // event not found... can't determine interval
   return INVALID_INDEX;
}

IntervalIndexType CIntervalManager::GetErectPierInterval(PierIndexType pierIdx) const
{
   const auto& found = m_ErectPierIntervals.find(pierIdx);
   if ( found == m_ErectPierIntervals.end() )
   {
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetCastIntermediateDiaphragmsInterval() const
{
   return m_CastIntermediateDiaphragmsIntervalIdx;
}

IntervalIndexType CIntervalManager::GetCompositeIntermediateDiaphragmsInterval() const
{
   // intermediate diaphragms become composite immediately... these are just loads, like railing system
   return m_CastIntermediateDiaphragmsIntervalIdx;
}

IntervalIndexType CIntervalManager::GetCastLongitudinalJointInterval() const
{
   return m_CastLongitudinalJointsIntervalIdx;
}

IntervalIndexType CIntervalManager::GetCompositeLongitudinalJointInterval() const
{
   return m_CompositeLongitudinalJointsIntervalIdx;
}

IntervalIndexType CIntervalManager::GetCastDeckInterval(IndexType castingRegionIdx) const
{
   if (castingRegionIdx == INVALID_INDEX)
   {
      // if there is no deck, sometimes the casting region will be passed in anyway
      // so just deal with it
      return INVALID_INDEX;
   }
   return m_vCastDeckIntervalIdx.size() == 0 ? INVALID_INDEX : m_vCastDeckIntervalIdx[castingRegionIdx];
}

IntervalIndexType CIntervalManager::GetFirstCastDeckInterval() const
{
   if (m_vCastDeckIntervalIdx.size() == 0)
   {
      return INVALID_INDEX;
   }
   else
   {
      return *std::min_element(std::begin(m_vCastDeckIntervalIdx), std::end(m_vCastDeckIntervalIdx));
   }
}

IntervalIndexType CIntervalManager::GetLastCastDeckInterval() const
{
   if (m_vCastDeckIntervalIdx.size() == 0)
   {
      return INVALID_INDEX;
   }
   else
   {
      return *std::max_element(std::begin(m_vCastDeckIntervalIdx), std::end(m_vCastDeckIntervalIdx));
   }
}

IntervalIndexType CIntervalManager::GetCompositeDeckInterval(IndexType castingRegionIdx) const
{
   if (castingRegionIdx == INVALID_INDEX)
   {
      // if there is no deck, sometimes the casting region will be passed in anyway
      // so just deal with it
      return INVALID_INDEX;
   }
   return m_vCompositeDeckIntervalIdx.size() == 0 ? INVALID_INDEX : m_vCompositeDeckIntervalIdx[castingRegionIdx];
}

IntervalIndexType CIntervalManager::GetFirstCompositeDeckInterval() const
{
   if (m_vCompositeDeckIntervalIdx.size() == 0)
   {
      return INVALID_INDEX;
   }
   else
   {
      return *std::min_element(std::begin(m_vCompositeDeckIntervalIdx), std::end(m_vCompositeDeckIntervalIdx));
   }
}

IntervalIndexType CIntervalManager::GetLastCompositeDeckInterval() const
{
   if (m_vCompositeDeckIntervalIdx.size() == 0)
   {
      return INVALID_INDEX;
   }
   else
   {
      return *std::max_element(std::begin(m_vCompositeDeckIntervalIdx), std::end(m_vCompositeDeckIntervalIdx));
   }
}

IntervalIndexType CIntervalManager::GetFirstStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      for (const auto& iter : m_StrandStressingSequenceIntervalLimits)
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter.first.girderIndex) ||
              (girderKey.groupIndex == iter.first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter.second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      if( found != m_StrandStressingSequenceIntervalLimits.end() )
      {
         return found->second.first;
      }
      else
      {
         // probably an unequal number of girders per group, and this one has less.
         CGirderKey newKey = GetSafeGirderKey(m_pBroker,girderKey);

         found = m_StrandStressingSequenceIntervalLimits.find(newKey);
         return found->second.second; // this will crash if not found, so no bother with assert
      }
   }
}

IntervalIndexType CIntervalManager::GetLastStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = INVALID_INDEX;
      for( const auto& iter : m_StrandStressingSequenceIntervalLimits )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter.first.girderIndex) ||
              (girderKey.groupIndex == iter.first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            if ( intervalIdx == MAX_INDEX )
            {
               intervalIdx = 0;
            }
            intervalIdx = Max(intervalIdx,iter.second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      auto found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      if(found != m_StrandStressingSequenceIntervalLimits.end())
      {
         return found->second.second;
      }
      else
      {
         // probably an unequal number of girders per group, and this one has less.
         CGirderKey newKey = GetSafeGirderKey(m_pBroker,girderKey);

         found = m_StrandStressingSequenceIntervalLimits.find(newKey);
         return found->second.second; // this will crash if not found, so no bother with assert
      }
   }
}

IntervalIndexType CIntervalManager::GetStressStrandInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_StressStrandIntervals.find(segmentKey));
   if( found != m_StressStrandIntervals.end() )
   {
      return found->second;
   }
   else
   {
      // there is an unequal number of girders per group, and this one has less. use the right-most girder
      CSegmentKey newkey(GetSafeSegmentKey(m_pBroker,segmentKey));

      found = m_StressStrandIntervals.find(newkey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

IntervalIndexType CIntervalManager::GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   return GetFirstInterval(girderKey, m_ReleaseSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   return GetLastInterval(girderKey, m_ReleaseSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_ReleaseIntervals.find(segmentKey));
   if( found != m_ReleaseIntervals.end())
   {
   return found->second;
}
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CSegmentKey newKey = GetSafeSegmentKey(m_pBroker,segmentKey);

      found = m_ReleaseIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }

}

IntervalIndexType CIntervalManager::GetLiftingInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_SegmentLiftingIntervals.find(segmentKey));
   if (found != m_SegmentLiftingIntervals.end())
   {
      return found->second;
   }
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CSegmentKey newKey = GetSafeSegmentKey(m_pBroker, segmentKey);

      found = m_SegmentLiftingIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

IntervalIndexType CIntervalManager::GetFirstLiftingInterval(const CGirderKey& girderKey) const
{
   return GetFirstInterval(girderKey, m_LiftingSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetLastLiftingInterval(const CGirderKey& girderKey) const
{
   return GetLastInterval(girderKey, m_LiftingSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetStorageInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_SegmentStorageIntervals.find(segmentKey));
   if (found != m_SegmentStorageIntervals.end())
   {
      return found->second;
   }
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CSegmentKey newKey = GetSafeSegmentKey(m_pBroker, segmentKey);

      found = m_SegmentStorageIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

IntervalIndexType CIntervalManager::GetFirstStorageInterval(const CGirderKey& girderKey) const
{
   return GetFirstInterval(girderKey, m_StorageSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetLastStorageInterval(const CGirderKey& girderKey) const
{
   return GetLastInterval(girderKey, m_StorageSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetHaulingInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_SegmentHaulingIntervals.find(segmentKey));
   if( found != m_SegmentHaulingIntervals.end())
   {
      return found->second;
   }
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CSegmentKey newKey = GetSafeSegmentKey(m_pBroker,segmentKey);

      found = m_SegmentHaulingIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

bool CIntervalManager::IsHaulingInterval(IntervalIndexType intervalIdx) const
{
   for (const auto& segHaulInterval : m_SegmentHaulingIntervals)
   {
      if (segHaulInterval.second == intervalIdx)
      {
         return true;
      }
   }
   return false;
}

IntervalIndexType CIntervalManager::GetErectSegmentInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   auto found(m_SegmentErectionIntervals.find(segmentKey));
   if(found != m_SegmentErectionIntervals.end())
   {
   return found->second;
}
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CSegmentKey newKey = GetSafeSegmentKey(m_pBroker,segmentKey);

      found = m_SegmentErectionIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

bool CIntervalManager::IsSegmentErectionInterval(IntervalIndexType intervalIdx) const
{
   ATLASSERT(intervalIdx != INVALID_INDEX);
   for ( const auto& iter : m_SegmentErectionIntervals)
   {
      if ( iter.second == intervalIdx )
      {
         return true;
      }
   }

   return false;
}

bool CIntervalManager::IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ATLASSERT(intervalIdx != INVALID_INDEX);

   for ( const auto& iter : m_SegmentErectionIntervals)
   {
      const CSegmentKey& segmentKey = iter.first;
      if ( girderKey.IsEqual(segmentKey) )
      {
         IntervalIndexType erectionIntervalIdx = iter.second;
         if ( erectionIntervalIdx == intervalIdx )
         {
            return true;
         }
      }
   }

   return false;
}

IntervalIndexType CIntervalManager::GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const
{
   auto found(m_RemoveTemporaryStrandsIntervals.find(segmentKey));
   if ( found != m_RemoveTemporaryStrandsIntervals.end() )
   {
      return found->second;
   }
   else
   {
      return GetErectSegmentInterval(segmentKey);
      //return INVALID_INDEX; // there aren't any temporary strands in this segment
   }
}

IntervalIndexType CIntervalManager::GetCastClosureInterval(const CClosureKey& clousreKey) const
{
   ASSERT_CLOSURE_KEY(clousreKey); // must be a specific segment key
   auto found(m_CastClosureIntervals.find(clousreKey));
   if(found != m_CastClosureIntervals.end())
   {
      return found->second;
   }
   else
   {
      // probably an unequal number of girders per group, and this one has less.
      CClosureKey newKey = GetSafeSegmentKey(m_pBroker,clousreKey);

      found = m_CastClosureIntervals.find(newKey);
      return found->second; // this will crash if not found, so no bother with assert
   }
}

IntervalIndexType CIntervalManager::GetFirstCastClosureJointInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   IntervalIndexType intervalIdx = INVALID_INDEX;
   for ( const auto& iter : m_CastClosureIntervals)
   {
      const auto& closureKey = iter.first;
      if ( closureKey.groupIndex == girderKey.groupIndex && closureKey.girderIndex == girderKey.girderIndex )
      {
         intervalIdx = Min(intervalIdx,iter.second);
      }
   }

   return intervalIdx;
}

IntervalIndexType CIntervalManager::GetLastCastClosureJointInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   IntervalIndexType intervalIdx = 0;
   for ( const auto& iter : m_CastClosureIntervals)
   {
      const auto& closureKey = iter.first;
      if ( closureKey.groupIndex == girderKey.groupIndex && closureKey.girderIndex == girderKey.girderIndex )
      {
         intervalIdx = Max(intervalIdx,iter.second);
      }
   }

   if ( intervalIdx == 0 )
   {
      intervalIdx = INVALID_INDEX;
   }

   return intervalIdx;
}

IntervalIndexType CIntervalManager::GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const
{
   return GetFirstInterval(girderKey, m_SegmentErectionSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetLastSegmentErectionInterval(const CGirderKey& girderKey) const
{
   return GetLastInterval(girderKey, m_SegmentErectionSequenceIntervalLimits);
}

IntervalIndexType CIntervalManager::GetLiveLoadInterval() const
{
   return m_LiveLoadIntervalIdx;
}

IntervalIndexType CIntervalManager::GetOverlayInterval() const
{
   return m_OverlayIntervalIdx;
}

IntervalIndexType CIntervalManager::GetInstallRailingSystemInterval() const
{
   return m_RailingSystemIntervalIdx;
}

IntervalIndexType CIntervalManager::GetUserLoadInterval(const CSpanKey& spanKey,UserLoads::LoadCase loadCase,LoadIDType userLoadID) const
{
   ASSERT_SPAN_KEY(spanKey);
   ASSERT(loadCase == UserLoads::DC || loadCase == UserLoads::DW);

   CUserLoadKey key(spanKey,userLoadID);
   auto found(m_UserLoadInterval[loadCase].find(key));
   if ( found == m_UserLoadInterval[loadCase].end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetStressSegmentTendonInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   auto found(m_SegmentTendonStressingIntervals.find(segmentKey));
   if (found == m_SegmentTendonStressingIntervals.end())
   {
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstSegmentTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   for (const auto& iter : m_SegmentTendonStressingIntervals)
   {
      if (girderKey.IsEqual(iter.first))
      {
         intervals.insert(iter.second);
      }
   }

   if (intervals.size() == 0)
   {
      return INVALID_INDEX;
   }

   return *intervals.begin();
}

IntervalIndexType CIntervalManager::GetLastSegmentTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   for (const auto& iter : m_SegmentTendonStressingIntervals)
   {
      if (girderKey.IsEqual(iter.first))
      {
         intervals.insert(iter.second);
      }
   }

   if (intervals.size() == 0)
   {
      return INVALID_INDEX;
   }

   return *intervals.rbegin();
}

IntervalIndexType CIntervalManager::GetStressGirderTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   auto found(m_GirderTendonStressingIntervals.find(CGirderTendonKey(girderKey,ductIdx)));
   if ( found == m_GirderTendonStressingIntervals.end() )
   {
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstGirderTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   for ( const auto& iter : m_GirderTendonStressingIntervals)
   {
      if ( girderKey.IsEqual(iter.first.girderKey) )
      {
         intervals.insert(iter.second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.begin();
}

IntervalIndexType CIntervalManager::GetLastGirderTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   for ( const auto& iter : m_GirderTendonStressingIntervals)
   {
      if (girderKey.IsEqual(iter.first.girderKey))
      {
         intervals.insert(iter.second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.rbegin();
}

std::vector<IntervalIndexType> CIntervalManager::GetGirderTendonStressingIntervals(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::vector<IntervalIndexType> intervals;
   for (const auto& iter : m_GirderTendonStressingIntervals)
   {
      if (girderKey == iter.first.girderKey)
      {
         intervals.push_back(iter.second);
      }
   }

   std::sort(std::begin(intervals), std::end(intervals));
   intervals.erase(std::unique(std::begin(intervals), std::end(intervals)), std::end(intervals));

   return intervals;
}

IntervalIndexType CIntervalManager::GetTemporarySupportErectionInterval(SupportIndexType tsIdx) const
{
   auto found(m_ErectTemporarySupportIntervals.find(tsIdx) );
   if ( found == m_ErectTemporarySupportIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) const
{
   auto found(m_RemoveTemporarySupportIntervals.find(tsIdx) );
   if ( found == m_RemoveTemporarySupportIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::StoreInterval(CInterval& interval)
{
#if defined _DEBUG
   interval.AssertValid();
   if ( 0 < m_Intervals.size() )
   {
      ATLASSERT(m_Intervals.back().End == interval.Start);
   }
#endif

   m_Intervals.push_back(interval);
   return m_Intervals.size()-1;
}

void CIntervalManager::ProcessStep1(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // Step 1. Record interval index when piers and temporary supports are erected
   // We don't need an actual interval for this since support erection does not
   // induce loading or time-duration into the system.
   const CErectPiersActivity& activity(pTimelineEvent->GetErectPiersActivity());
   if ( !activity.IsEnabled() )
   {
      return;
   }

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   IntervalIndexType intervalIdx = m_Intervals.size();
   const std::set<PierIDType>& pierIDs(activity.GetPiers());
   for( const auto& pierID : pierIDs)
   {
      const CPierData2* pPier = pBridgeDesc->FindPier(pierID);
      PierIndexType pierIdx = pPier->GetIndex();
      m_ErectPierIntervals.insert(std::make_pair(pierIdx,intervalIdx));
   }

   const std::set<SupportIDType>& tsIDs(activity.GetTempSupports());
   for(const auto& tsID : tsIDs)
   {
      const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
      SupportIndexType tsIdx = pTS->GetIndex();
      m_ErectTemporarySupportIntervals.insert(std::make_pair(tsIdx,intervalIdx));
   }
}

void CIntervalManager::ProcessStep2(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // Step 2: Create a single interval for all zero duration activities that can happen at the same time
   // These activities are:
   // a) Erect Segment
   // b) Remove Temporary Supports
   // c) Stress Tendons
   // d) Cast Closure

   // For each activity in this event, get phrase to use in the description of the interval
   // as well as record the interval index if needed. Since the interval object hasn't
   // been added to the m_Intervals collection yet, the interval index is the size of the collection
   IntervalIndexType intervalIdx = m_Intervals.size();
   
   std::vector<CString> strDescriptions;

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   bool bRemoveTemporaryStrands = false; // need to know if there are temporary strands that are removed
   // if so, we will add another interval after this interval
   const CErectSegmentActivity& erectSegmentsActivity = pTimelineEvent->GetErectSegmentsActivity();
   if ( erectSegmentsActivity.IsEnabled() )
   {
      std::set<SegmentIDType> erectedSegments(erectSegmentsActivity.GetSegments());

      // first check if plant installed segment PT occurs immedately before hauling, if so, we need to create an analysis interval
      bool bSegmentPT = false;
      for (const auto& segmentID : erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         if (0 < pSegment->Tendons.GetDuctCount() && pSegment->Tendons.InstallationEvent == pgsTypes::SegmentPTEventType::sptetHauling)
         {
            bSegmentPT = true;
            m_SegmentTendonStressingIntervals.insert(std::make_pair(segmentKey, intervalIdx));
         }
      } // next segment ID

      CInterval segmentPTInterval;
      if (bSegmentPT)
      {
         segmentPTInterval.StartEventIdx = eventIdx;
         segmentPTInterval.EndEventIdx = eventIdx;
         segmentPTInterval.Start = pTimelineEvent->GetDay();
         segmentPTInterval.End = segmentPTInterval.Start;
         segmentPTInterval.Middle = segmentPTInterval.Start;
         segmentPTInterval.Duration = 0;
         segmentPTInterval.Description = _T("Stress Segment Tendons");
         IntervalIndexType ptIntervalIdx = StoreInterval(segmentPTInterval);
         ATLASSERT(ptIntervalIdx == intervalIdx);
      }

      // Segments must be hauled to the bridge site before they are erected
      CInterval haulSegmentInterval;
      haulSegmentInterval.StartEventIdx = eventIdx;
      haulSegmentInterval.EndEventIdx   = eventIdx;
      haulSegmentInterval.Start         = (bSegmentPT ? segmentPTInterval.End : pTimelineEvent->GetDay());
      haulSegmentInterval.End           = haulSegmentInterval.Start;
      haulSegmentInterval.Middle        = haulSegmentInterval.Start;
      haulSegmentInterval.Duration      = 0;
      haulSegmentInterval.Description   = (m_bIsPGSuper ? _T("Haul Girders") : _T("Haul Segments"));
      IntervalIndexType haulIntervalIdx = StoreInterval(haulSegmentInterval);

      for( const auto& segmentID : erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         m_SegmentHaulingIntervals.insert(std::make_pair(segmentKey,haulIntervalIdx));
      } // next segment ID

      // update the interval index for all the other activities since it just incremented by one
      // for segment hauling
      intervalIdx = m_Intervals.size();

      
      strDescriptions.push_back(CString(m_bIsPGSuper ? _T("Erect Girders") : _T("Erect Segments")));
      IntervalIndexType erectSegmentIntervalIdx = intervalIdx;

      for(const auto& segmentID : erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         m_SegmentErectionIntervals.insert(std::make_pair(segmentKey,erectSegmentIntervalIdx));

         if ( 0 < pSegment->Strands.GetStrandCount(pgsTypes::Temporary) )
         {
            bRemoveTemporaryStrands = true;
         }

         // this is for keeping track of when the first and last segments in a girder are erected
         auto found(m_SegmentErectionSequenceIntervalLimits.find(segmentKey));
         if ( found == m_SegmentErectionSequenceIntervalLimits.end() )
         {
            // this is the first segment from the girder to be erected
            m_SegmentErectionSequenceIntervalLimits.insert(std::make_pair(segmentKey,std::make_pair(erectSegmentIntervalIdx,erectSegmentIntervalIdx)));
         }
         else
         {
            // a segment from this girder has already been erected.. update the record
            found->second.first  = Min(found->second.first, erectSegmentIntervalIdx);
            found->second.second = Max(found->second.second,erectSegmentIntervalIdx);
         }
      } // next segment ID
   } // erect segments activity

   const CStressTendonActivity& stressTendonActivity = pTimelineEvent->GetStressTendonActivity();
   if ( stressTendonActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Stress Tendons")));
      IntervalIndexType stressTendonIntervalIdx = intervalIdx;

      const std::vector<CGirderTendonKey>& tendons( stressTendonActivity.GetTendons() );
      for(auto tendonKey : tendons)
      {
         if ( tendonKey.girderKey.groupIndex == ALL_GROUPS )
         {
            // tendon key doesn't have a valid girder key, so it must have a valid girder ID
            // need to get the associated girder key
            ATLASSERT(tendonKey.girderID != INVALID_ID);
            const CSplicedGirderData* pGirder = pBridgeDesc->FindGirder(tendonKey.girderID);
            tendonKey.girderKey = pGirder->GetGirderKey();
         }

         // we need to know the number of webs in a girder, but since we are in the middle
         // of validating the overall bridge model, we can't make a request throught the
         // IGirder interface. Doing so would cause recursion and *crash*. 
         //
         // Here is an alternative method that works
         const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(tendonKey.girderKey.groupIndex)->GetGirder(tendonKey.girderKey.girderIndex);
         const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
         CComPtr<IBeamFactory> factory;
         pGirderEntry->GetBeamFactory(&factory);
         WebIndexType nWebs = factory->GetWebCount(pGirderEntry->GetDimensions());

         for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
         {
            DuctIndexType thisDuctIdx = nWebs*tendonKey.ductIdx + webIdx;
            CGirderTendonKey thisTendonKey(tendonKey.girderKey,thisDuctIdx);
            m_GirderTendonStressingIntervals.insert(std::make_pair(thisTendonKey,stressTendonIntervalIdx));
         }
      }
   }

   const CRemoveTemporarySupportsActivity& removeTemporarySupportActivity = pTimelineEvent->GetRemoveTempSupportsActivity();
   if (removeTemporarySupportActivity.IsEnabled())
   {
      strDescriptions.push_back(CString(_T("Remove Temporary Support")));
      IntervalIndexType removeTempSupportIntervalIdx = intervalIdx;

      const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
      const std::vector<SupportIDType>& tsIDs(removeTS.GetTempSupports());
      for (const auto& tsID : tsIDs)
      {
         const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
         SupportIndexType tsIdx = pTS->GetIndex();
         m_RemoveTemporarySupportIntervals.insert(std::make_pair(tsIdx, removeTempSupportIntervalIdx));
      }
   }

   const CCastLongitudinalJointActivity& castLJActivity = pTimelineEvent->GetCastLongitudinalJointActivity();
   if (castLJActivity.IsEnabled())
   {
      strDescriptions.push_back(CString(_T("Cast Longitudinal Joints")));
      m_CastLongitudinalJointsIntervalIdx = intervalIdx;
   }

   const CCastClosureJointActivity& closureJointActivity = pTimelineEvent->GetCastClosureJointActivity();
   if ( closureJointActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Cast Closure Joint")));
      IntervalIndexType castClosureIntervalIdx = intervalIdx;

      const auto& vClosureKeys(closureJointActivity.GetClosureKeys(pBridgeDesc));
      for (const auto& closureKey : vClosureKeys)
      {
         m_CastClosureIntervals.emplace(closureKey, castClosureIntervalIdx);
      }
   } // end if cast deck activity

   if ( strDescriptions.size() == 0 )
   {
      return; // none of the activities are enabled for this event
   }

   // Build up the composite description for this interval
   CString strDescription;
   for( const auto& str : strDescriptions)
   {
      if (strDescription.GetLength() != 0 )
      {
         strDescription += _T(", ");
      }

      strDescription += str;
   }

   CInterval interval;
   interval.StartEventIdx = eventIdx;
   interval.EndEventIdx   = eventIdx;
   interval.Start         = pTimelineEvent->GetDay();
   interval.End           = interval.Start;
   interval.Middle        = interval.Start;
   interval.Duration      = 0;
   interval.Description = strDescription;
   StoreInterval(interval);

   // If there are temporary strands to be removed... create an interval
   if ( bRemoveTemporaryStrands )
   {
      // at least one segment that is erected in this activity has temporary strands

      // remove temporary strands
      CInterval removeTempStrandInterval;
      removeTempStrandInterval.StartEventIdx = eventIdx;
      removeTempStrandInterval.EndEventIdx   = eventIdx;
      removeTempStrandInterval.Start         = pTimelineEvent->GetDay();
      removeTempStrandInterval.End           = removeTempStrandInterval.Start;
      removeTempStrandInterval.Middle        = removeTempStrandInterval.Start;
      removeTempStrandInterval.Duration      = 0;
      removeTempStrandInterval.Description   = _T("Remove temporary strands");
      IntervalIndexType removeTempStrandsIntervalIdx = StoreInterval(removeTempStrandInterval);

      std::set<SegmentIDType> erectedSegments(erectSegmentsActivity.GetSegments());
      for(const auto& segmentID : erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         m_RemoveTemporaryStrandsIntervals.insert(std::make_pair(segmentKey,removeTempStrandsIntervalIdx));
      } 
   }
}

#define SEGMENT 1
#define CLOSURE 2
#define LONGITUDINAL_JOINT 3
#define DECK 4

struct CuringInfo
{
   Float64 duration;
   int activity;
   CuringInfo(Float64 d, int a) : duration(d), activity(a) {}

   bool operator<(const CuringInfo& other) const
   {
      if (duration < other.duration)
         return true;

      if (other.duration < duration)
         return false;

      return activity < other.activity;
   }
};

void CIntervalManager::ProcessStep3(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // Step 3: Create curing duration intervals
   // Deck casting intervals are also handled here because of the staged nature of the deck model

   const CConstructSegmentActivity& constructSegmentActivity = pTimelineEvent->GetConstructSegmentsActivity();
   const CCastLongitudinalJointActivity& castLongitudinalJointActivity = pTimelineEvent->GetCastLongitudinalJointActivity();
   const CCastDeckActivity& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
   const CCastClosureJointActivity& castClosureJointActivity = pTimelineEvent->GetCastClosureJointActivity();

   std::set<CuringInfo> curingDurations;
   if ( constructSegmentActivity.IsEnabled() )
   {
      curingDurations.emplace(constructSegmentActivity.GetRelaxationTime(),SEGMENT);
   }

   if (castLongitudinalJointActivity.IsEnabled())
   {
      curingDurations.emplace(castLongitudinalJointActivity.GetConcreteAgeAtContinuity(), LONGITUDINAL_JOINT);
   }

   if ( castDeckActivity.IsEnabled() )
   {
      // we want the total duration of all deck casting regions to cure
      curingDurations.emplace(castDeckActivity.GetDuration(),DECK);
   }

   if ( castClosureJointActivity.IsEnabled() )
   {
      curingDurations.emplace(castClosureJointActivity.GetConcreteAgeAtContinuity(),CLOSURE);
   }

   if ( curingDurations.size() == 0 )
   {
      return; // none of the activities are enabled during this event
   }

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   // build the intervals for the curing durations
   // work in order of shortest to longest duration

   // the concrete for all concrete casting activities during this event
   // are cast at the same time. the curing intervals are based on the
   // one with the shortest curing time, then the next longest, and so on.
   // the duration of the curing time is the curing time for the
   // given activity less the duration of the other curing times that
   // have already occuring during this activity
   //
   // e.g. Deck concrete and closure joint concrete are cast at the same
   // time. It takes the closure joint 3 days to cure and the deck 10 days.
   // The curing duration for the closure joint and deck together is modeled as 3 days. the
   // curing duration for the deck then proceeds for an additional 7 days

   Float64 previous_curing_duration = 0;
   for ( const auto& curingInfo : curingDurations)
   {
      Float64 duration = curingInfo.duration;
      int activityType = curingInfo.activity;

      Float64 remaining_duration = duration - previous_curing_duration;

      if ( activityType == SEGMENT )
      {
         ATLASSERT(IsEqual(duration,constructSegmentActivity.GetRelaxationTime()));

         // stress strands during segment construction. strands relax during this interval
         CInterval stressStrandInterval;
         stressStrandInterval.StartEventIdx = eventIdx;
         stressStrandInterval.EndEventIdx   = eventIdx;
         stressStrandInterval.Start         = pTimelineEvent->GetDay();
         stressStrandInterval.Duration      = remaining_duration;
         stressStrandInterval.End           = stressStrandInterval.Start + stressStrandInterval.Duration;
         stressStrandInterval.Middle        = 0.5*(stressStrandInterval.Start + stressStrandInterval.End);
         stressStrandInterval.Description   = m_bIsPGSuper ? _T("Tension strand, cast girder, strand relaxation, and concrete curing") :  _T("Tension strand, cast segment, strand relaxation, and concrete curing");
         IntervalIndexType stressStrandIntervalIdx = StoreInterval(stressStrandInterval);

         // release prestress is a sudden loading.... zero length interval
         CInterval releaseInterval(stressStrandInterval);
         releaseInterval.Start = stressStrandInterval.End;
         releaseInterval.Duration = 0;
         releaseInterval.Middle = releaseInterval.Start;
         releaseInterval.Description = _T("Prestress Release");
         IntervalIndexType releaseIntervalIdx = StoreInterval(releaseInterval);

         IntervalIndexType intervalIdx = m_Intervals.size();

         bool bPTAfterRelease = false;
         bool bPTAfterStorage = false;
         const std::set<SegmentIDType>& segments = constructSegmentActivity.GetSegments();
         for (const auto& segmentID : segments)
         {
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            ASSERT_SEGMENT_KEY(segmentKey);

            if (0 < pSegment->Tendons.GetDuctCount())
            {
               if (pSegment->Tendons.InstallationEvent == pgsTypes::sptetRelease)
               {
                  bPTAfterRelease = true;
                  m_SegmentTendonStressingIntervals.insert(std::make_pair(segmentKey, intervalIdx));
               }
               else if (pSegment->Tendons.InstallationEvent == pgsTypes::sptetStorage)
               {
                  bPTAfterStorage = true;
                  m_SegmentTendonStressingIntervals.insert(std::make_pair(segmentKey, intervalIdx));
               }
            }
         }

         CInterval segmentPTInterval;
         if (bPTAfterRelease)
         {
            segmentPTInterval.StartEventIdx = eventIdx;
            segmentPTInterval.EndEventIdx = eventIdx;
            segmentPTInterval.Start = releaseInterval.End;
            segmentPTInterval.End = segmentPTInterval.Start;
            segmentPTInterval.Middle = segmentPTInterval.Start;
            segmentPTInterval.Duration = 0;
            segmentPTInterval.Description = _T("Stress Segment Tendons");
            IntervalIndexType ptIntervalIdx = StoreInterval(segmentPTInterval);
            ATLASSERT(ptIntervalIdx == intervalIdx);
         }

         // lift segment
         CInterval liftSegmentInterval(releaseInterval);
         liftSegmentInterval.Start = (bPTAfterRelease ? segmentPTInterval.End : releaseInterval.End);
         liftSegmentInterval.Duration = 0;
         liftSegmentInterval.Middle = liftSegmentInterval.Start;
         liftSegmentInterval.Description = m_bIsPGSuper ? _T("Lift girders") : _T("Lift segments");
         IntervalIndexType liftIntervalIdx = StoreInterval(liftSegmentInterval);
         for (const auto& segmentID : segments)
         {
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            ASSERT_SEGMENT_KEY(segmentKey);
            m_SegmentLiftingIntervals.insert(std::make_pair(segmentKey, liftIntervalIdx));

            // this is for keeping track of when the segments are lifted for the first and last segments constructed for this girder
            auto liftingFound(m_LiftingSequenceIntervalLimits.find(segmentKey));
            if (liftingFound == m_LiftingSequenceIntervalLimits.end())
            {
               // this is the first segment from the girder to have its strands released
               m_LiftingSequenceIntervalLimits.insert(std::make_pair(segmentKey, std::make_pair(liftIntervalIdx, liftIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands released.. update the record
               liftingFound->second.first = Min(liftingFound->second.first, liftIntervalIdx);
               liftingFound->second.second = Max(liftingFound->second.second, liftIntervalIdx);
            }
         }


         // placing into storage changes boundary conditions... treat as sudden change in loading
         CInterval storageInterval(liftSegmentInterval);
         storageInterval.Start = liftSegmentInterval.End;
         storageInterval.Duration = 0;
         storageInterval.Middle = storageInterval.Start;
         storageInterval.Description = m_bIsPGSuper ? _T("Place girders into storage") : _T("Place segments into storage");
         IntervalIndexType storageIntervalIdx = StoreInterval(storageInterval);
         for (const auto& segmentID : segments)
         {
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            ASSERT_SEGMENT_KEY(segmentKey);
            m_SegmentStorageIntervals.insert(std::make_pair(segmentKey, storageIntervalIdx));

            // this is for keeping track of when the segments are lifted for the first and last segments constructed for this girder
            auto storageFound(m_StorageSequenceIntervalLimits.find(segmentKey));
            if (storageFound == m_StorageSequenceIntervalLimits.end())
            {
               // this is the first segment from the girder to have its strands released
               m_StorageSequenceIntervalLimits.insert(std::make_pair(segmentKey, std::make_pair(storageIntervalIdx, storageIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands released.. update the record
               storageFound->second.first = Min(storageFound->second.first, storageIntervalIdx);
               storageFound->second.second = Max(storageFound->second.second, storageIntervalIdx);
            }
         }

         if (bPTAfterStorage)
         {
            IntervalIndexType intervalIdx = m_Intervals.size();
            CInterval segmentPTInterval;
            segmentPTInterval.StartEventIdx = eventIdx;
            segmentPTInterval.EndEventIdx = eventIdx;
            segmentPTInterval.Start = storageInterval.End;
            segmentPTInterval.End = segmentPTInterval.Start;
            segmentPTInterval.Middle = segmentPTInterval.Start;
            segmentPTInterval.Duration = 0;
            segmentPTInterval.Description = _T("Stress Segment Tendons");
            IntervalIndexType ptIntervalIdx = StoreInterval(segmentPTInterval);
            ATLASSERT(ptIntervalIdx == intervalIdx);
         }

         // record the segments that are constructed during this activity
         for(const auto& segmentID : segments)
         {
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            ASSERT_SEGMENT_KEY(segmentKey);

            m_StressStrandIntervals.insert(std::make_pair(segmentKey,stressStrandIntervalIdx));
            m_ReleaseIntervals.insert(std::make_pair(segmentKey,releaseIntervalIdx));

            // this is for keeping track of when the strands are stressed for the first and last segments constructed for this girder
            auto strandStressingFound(m_StrandStressingSequenceIntervalLimits.find(segmentKey));
            if ( strandStressingFound == m_StrandStressingSequenceIntervalLimits.end() )
            {
               // this is the first segment from the girder have strands stressed
               m_StrandStressingSequenceIntervalLimits.insert(std::make_pair(segmentKey,std::make_pair(stressStrandIntervalIdx,stressStrandIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands stressed.. update the record
               strandStressingFound->second.first  = Min(strandStressingFound->second.first, stressStrandIntervalIdx);
               strandStressingFound->second.second = Max(strandStressingFound->second.second,stressStrandIntervalIdx);
            }

            // this is for keeping track of when the strands are released for the first and last segments constructed for this girder
            auto releaseFound(m_ReleaseSequenceIntervalLimits.find(segmentKey));
            if ( releaseFound == m_ReleaseSequenceIntervalLimits.end() )
            {
               // this is the first segment from the girder to have its strands released
               m_ReleaseSequenceIntervalLimits.insert(std::make_pair(segmentKey,std::make_pair(releaseIntervalIdx,releaseIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands released.. update the record
               releaseFound->second.first  = Min(releaseFound->second.first, releaseIntervalIdx);
               releaseFound->second.second = Max(releaseFound->second.second,releaseIntervalIdx);
            }
         } // next segment
      }
      else if (activityType == LONGITUDINAL_JOINT)
      {
         CInterval cureLongitudinalJointInterval;
         ATLASSERT(IsEqual(duration, castLongitudinalJointActivity.GetConcreteAgeAtContinuity()));
         if (0 < duration)
         {
            // only model longitudinal joint curing if we are doing a time-step analysis
            ATLASSERT(m_CastLongitudinalJointsIntervalIdx != INVALID_INDEX); // longitudinal joints must have been previously cast
            cureLongitudinalJointInterval.StartEventIdx = eventIdx;
            cureLongitudinalJointInterval.EndEventIdx = eventIdx;
            cureLongitudinalJointInterval.Start = m_Intervals.back().End; // curing starts when the previous interval ends
            cureLongitudinalJointInterval.Duration = remaining_duration;
            cureLongitudinalJointInterval.End = cureLongitudinalJointInterval.Start + cureLongitudinalJointInterval.Duration;
            cureLongitudinalJointInterval.Middle = 0.5*(cureLongitudinalJointInterval.Start + cureLongitudinalJointInterval.End);
            cureLongitudinalJointInterval.Description = _T("Longitudinal joints curing");
            StoreInterval(cureLongitudinalJointInterval);

            CInterval compositeLongitudinalJointInterval;
            compositeLongitudinalJointInterval.StartEventIdx = eventIdx;
            compositeLongitudinalJointInterval.EndEventIdx = eventIdx;
            compositeLongitudinalJointInterval.Start = cureLongitudinalJointInterval.End;
            compositeLongitudinalJointInterval.Duration = 0;
            compositeLongitudinalJointInterval.End = compositeLongitudinalJointInterval.Start + compositeLongitudinalJointInterval.Duration;
            compositeLongitudinalJointInterval.Middle = 0.5*(compositeLongitudinalJointInterval.Start + compositeLongitudinalJointInterval.End);

            compositeLongitudinalJointInterval.Description = _T("Composite longitudinal joints");

            m_CompositeLongitudinalJointsIntervalIdx = StoreInterval(compositeLongitudinalJointInterval);
         }
         else
         {
            // for non-timestep analysis (PGSuper) longitudinal joints are composite the interval after they are cast
            m_CompositeLongitudinalJointsIntervalIdx = m_CastLongitudinalJointsIntervalIdx + 1;
         }
      }
      else if ( activityType == DECK )
      {
         ATLASSERT(IsEqual(duration,castDeckActivity.GetDuration()));
         ATLASSERT(pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone);
         if (0 <= duration)
         {
            // only model deck curing if we are doing a time-step analysis

            IndexType nCastings = castDeckActivity.GetCastingCount();
            IndexType lastCastingIdx = nCastings - 1;

            std::vector<std::pair<Float64,Float64>> vCuringTime; // keep track of when curing starts for a casting and how long curing takes
            // first item in the pair is time when curing starts, the second item is curing duration
            // the current curing duration is the time at the end of the cure deck region interval less the time when casting occured
            // if the current curing duration is equal to the actual curing duration, the concrete in the casting is cured and has become composite
            vCuringTime.resize(nCastings, std::make_pair(-99999,-99999)); // initialize with obvious bad value

            for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
            {
               // interval for casting a deck region
               CInterval castDeckRegionInterval;
               castDeckRegionInterval.StartEventIdx = eventIdx;
               castDeckRegionInterval.EndEventIdx = eventIdx;
               castDeckRegionInterval.Start = m_Intervals.back().End;
               castDeckRegionInterval.Duration = 0;
               castDeckRegionInterval.End = castDeckRegionInterval.Start + castDeckRegionInterval.Duration;
               castDeckRegionInterval.Middle = 0.5*(castDeckRegionInterval.Start + castDeckRegionInterval.End);

               std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx);
               if (1 < nCastings)
               {
                  CString strDesc = _T("Cast deck regions ");
                  auto begin = std::begin(vRegions);
                  auto iter = begin;
                  auto end = std::end(vRegions);
                  for (; iter != end; iter++)
                  {
                     if (iter != begin)
                     {
                        strDesc += _T(", ");
                     }
                     CString strRegion;
                     strRegion.Format(_T("%d"), LABEL_INDEX(*iter));
                     strDesc += strRegion;
                  }
                  castDeckRegionInterval.Description = strDesc;
               }
               else
               {
                  castDeckRegionInterval.Description = GetCastDeckEventName(pBridgeDesc->GetDeckDescription()->GetDeckType());
               }

               IntervalIndexType castDeckIntervalIdx;
               if (castingIdx == 0 && (castLongitudinalJointActivity.IsEnabled() || castClosureJointActivity.IsEnabled()))
               {
                  // if this is the first casting, and LJ or CJ are cast at the same time, use the previously defined (defined in ProcessStep2)
                  // interval as the deck casting interval for the first deck casting
                  if (castLongitudinalJointActivity.IsEnabled())
                  {
                     castDeckIntervalIdx = m_CastLongitudinalJointsIntervalIdx;
                     auto& interval = m_Intervals[castDeckIntervalIdx]; // get the interval
                     interval.Description += _T(", ") + castDeckRegionInterval.Description; // append the deck casting description

                     castDeckRegionInterval = interval;
                  }
                  else
                  {
                     ATLASSERT(castClosureJointActivity.IsEnabled());
                     // the interval for all the closure joints cast in this activity are the same, so just get the first one
                     const auto& vClosureKeys = castClosureJointActivity.GetClosureKeys(pBridgeDesc);
                     const auto& closureKey = vClosureKeys.front();
                     IntervalIndexType castClosureJointIntervalIdx = m_CastClosureIntervals[closureKey];
                     castDeckIntervalIdx = castClosureJointIntervalIdx;

                     auto& interval = m_Intervals[castDeckIntervalIdx]; // get the interval
                     interval.Description += _T(", ") + castDeckRegionInterval.Description; // append the deck casting description

                     castDeckRegionInterval = interval;
                  }
               }
               else
               {
                  // there weren't other castings happening at the same time as the deck casting so store the new castDeckRegionInterval in the interval manager
                  castDeckIntervalIdx = StoreInterval(castDeckRegionInterval);
               }

               for (auto regionIdx : vRegions)
               {
                  m_vCastDeckIntervalIdx[regionIdx] = castDeckIntervalIdx;
               }

               // interval for the deck region curing
               CInterval cureDeckRegionInterval;
               cureDeckRegionInterval.StartEventIdx = eventIdx;
               cureDeckRegionInterval.EndEventIdx = eventIdx;
               cureDeckRegionInterval.Start = castDeckRegionInterval.End; // curing starts when the previous interval ends

               // time when curing begins for this casting, and the curing duration
               vCuringTime[castingIdx] = std::make_pair(cureDeckRegionInterval.Start, castDeckActivity.GetCuringDuration());
                                                                           
               // the curing duration for this interval is the lesser of the time between castings and the curing duration for a casting.
               // if the time between casting is less then the curing duration, the next casting occurs before this casting becomes composite
               // the total curing of this casting will be modeled over multiple intervals
               Float64 curing_duration_this_interval;
               if (1 < nCastings)
               {
                  curing_duration_this_interval = Min(castDeckActivity.GetTimeBetweenCasting(), castDeckActivity.GetCuringDuration());
               }
               else
               {
                  curing_duration_this_interval = castDeckActivity.GetCuringDuration();
               }

               cureDeckRegionInterval.Duration = curing_duration_this_interval;
               cureDeckRegionInterval.End = cureDeckRegionInterval.Start + cureDeckRegionInterval.Duration;
               cureDeckRegionInterval.Middle = 0.5*(cureDeckRegionInterval.Start + cureDeckRegionInterval.End);
               if (pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeOverlay)
               {
                  cureDeckRegionInterval.Description = _T("Composite overlay curing");
               }
               else
               {
                  if (1 < nCastings)
                  {
                     CString strDesc = _T("Deck curing regions ");
                     auto begin = std::begin(vRegions);
                     auto iter = begin;
                     auto end = std::end(vRegions);
                     for (; iter != end; iter++)
                     {
                        if (iter != begin)

                        {
                           strDesc += _T(", ");
                        }
                        CString strRegion;
                        strRegion.Format(_T("%d"), LABEL_INDEX(*iter));
                        strDesc += strRegion;
                     }

                     cureDeckRegionInterval.Description = strDesc;
                  }
                  else
                  {
                     cureDeckRegionInterval.Description = _T("Deck curing");
                  }
               }

               IntervalIndexType curingIntervalIdx;
               if (castingIdx == 0 && (castLongitudinalJointActivity.IsEnabled() || castClosureJointActivity.IsEnabled()))
               {
                  // if this is the first casting, and LJ or CJ are cast at the same time, use the previously defined (defined in ProcessStep2)
                  // interval as the deck curing interval for the first deck casting
                  if (castLongitudinalJointActivity.IsEnabled())
                  {
                     curingIntervalIdx = m_CastLongitudinalJointsIntervalIdx + 1; // curing is in the interval after casting
                     auto& interval = m_Intervals[curingIntervalIdx]; // get the interval
                     interval.Description += _T(", ") + cureDeckRegionInterval.Description; // append the deck curing description

                     cureDeckRegionInterval = interval;
                  }
                  else
                  {
                     ATLASSERT(castClosureJointActivity.IsEnabled());
                     // the interval for all the closure joints cast in this activity are the same, so just get the first one
                     const auto& vClosureKeys = castClosureJointActivity.GetClosureKeys(pBridgeDesc);
                     const auto& closureKey = vClosureKeys.front();
                     IntervalIndexType cureClosureJointIntervalIdx = m_CastClosureIntervals[closureKey] + 1; // curing is in the interval after casting
                     curingIntervalIdx = cureClosureJointIntervalIdx;

                     if (m_Intervals.size() <= curingIntervalIdx)
                     {
                        // there is an overlap of the deck curing and closure curing intervals and the closure joint
                        // curing interval hasn't been created yet.
                        // add the new cure deck interval now
                        ATLASSERT(m_Intervals.size() == curingIntervalIdx);
                        cureDeckRegionInterval.Description += _T(", Closure Joints curing");
                        curingIntervalIdx = StoreInterval(cureDeckRegionInterval);
                     }
                     else
                     {
                        auto& interval = m_Intervals[curingIntervalIdx]; // get the interval
                        interval.Description += _T(", ") + cureDeckRegionInterval.Description; // append the deck curing description
                        cureDeckRegionInterval = interval;
                     }
                  }
               }
               else
               {
                  curingIntervalIdx = StoreInterval(cureDeckRegionInterval);
               }

               // determine if this or one of the preceding castings becomes composite at the end of this curing interval
               IntervalIndexType compositeCastingIdx = INVALID_INDEX; // index of the casting that has become composite
               for (IndexType ci = 0; ci <= castingIdx; ci++)
               {
                  Float64 elapsed_curing_time = cureDeckRegionInterval.End - vCuringTime[ci].first;
                  Float64 curing_time_for_casting = vCuringTime[ci].second;
                  if (IsEqual(elapsed_curing_time,curing_time_for_casting))
                  {
                     // the casting has become composite
                     compositeCastingIdx = ci;
                     break;
                  }
               }

               if (IsStructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType()) && compositeCastingIdx != INVALID_INDEX)
               {
                  CInterval compositeDeckRegionInterval;
                  compositeDeckRegionInterval.StartEventIdx = eventIdx;
                  compositeDeckRegionInterval.EndEventIdx = eventIdx;
                  compositeDeckRegionInterval.Start = cureDeckRegionInterval.End;
                  compositeDeckRegionInterval.Duration = 0;
                  compositeDeckRegionInterval.End = compositeDeckRegionInterval.Start + compositeDeckRegionInterval.Duration;
                  compositeDeckRegionInterval.Middle = 0.5*(compositeDeckRegionInterval.Start + compositeDeckRegionInterval.End);

                  if (pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtCompositeOverlay)
                  {
                     compositeDeckRegionInterval.Description = _T("Composite overlay");
                  }
                  else
                  {
                     if (1 < nCastings)
                     {
                        CString strDesc = _T("Composite deck regions ");
                        auto begin = std::begin(vRegions);
                        auto iter = begin;
                        auto end = std::end(vRegions);
                        for (; iter != end; iter++)
                        {
                           if (iter != begin)
                           {
                              strDesc += _T(", ");
                           }
                           CString strRegion;
                           strRegion.Format(_T("%d"), LABEL_INDEX(*iter));
                           strDesc += strRegion;
                        }
                        compositeDeckRegionInterval.Description = strDesc;
                     }
                     else
                     {
                        compositeDeckRegionInterval.Description = _T("Composite deck");
                     }
                  }

                  IntervalIndexType compositeDeckRegionIntervalIdx = StoreInterval(compositeDeckRegionInterval);

                  for (auto regionIdx : vRegions)
                  {
                     m_vCompositeDeckIntervalIdx[regionIdx] = compositeDeckRegionIntervalIdx;
                  }
               }

               if (1 < nCastings && castDeckActivity.GetCuringDuration() < castDeckActivity.GetTimeBetweenCasting() && castingIdx != lastCastingIdx)
               {
                  // each casting cures and becomes composite before the next casting occurs
                  // model the time-step between the time this casting becomes composite and the next
                  // casting occurs
                  CInterval timeStepInterval;
                  timeStepInterval.StartEventIdx = eventIdx;
                  timeStepInterval.EndEventIdx = eventIdx;
                  timeStepInterval.Start = m_Intervals.back().End;
                  timeStepInterval.Duration = castDeckActivity.GetTimeBetweenCasting() - castDeckActivity.GetCuringDuration();
                  timeStepInterval.End = timeStepInterval.Start + timeStepInterval.Duration;
                  timeStepInterval.Middle = 0.5*(timeStepInterval.Start + timeStepInterval.End);
                  timeStepInterval.Description = _T("Time-step between deck castings");
                  StoreInterval(timeStepInterval);
               }
            }
         }
         else
         {
            ATLASSERT(false); // this is just to see if we ever get here. this might be dead code
            // for non-timestep analysis (PGSuper) deck is composite the interval after it is cast
            if (IsStructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType()))
            {
               //m_CompositeDeckIntervalIdx = m_CastDeckIntervalIdx + 1;
               m_vCompositeDeckIntervalIdx = m_vCastDeckIntervalIdx;
               std::for_each(std::begin(m_vCompositeDeckIntervalIdx), std::end(m_vCompositeDeckIntervalIdx), [](auto& value) {return value + 1;});
            }
         }
      }
      else if ( activityType == CLOSURE )
      {
         ATLASSERT(IsEqual(duration,castClosureJointActivity.GetConcreteAgeAtContinuity()));

         CInterval cureClosureInterval;
         cureClosureInterval.StartEventIdx = eventIdx;
         cureClosureInterval.EndEventIdx   = eventIdx;
         cureClosureInterval.Start         = m_Intervals.back().End; // curing starts when the previous interval ends
         cureClosureInterval.Duration      = remaining_duration;
         cureClosureInterval.End           = cureClosureInterval.Start + cureClosureInterval.Duration;
         cureClosureInterval.Middle        = 0.5*(cureClosureInterval.Start + cureClosureInterval.End);
         cureClosureInterval.Description   = _T("Closure joints curing");
         StoreInterval(cureClosureInterval);
      }

      previous_curing_duration += duration;
   }
}

void CIntervalManager::ProcessStep4(EventIndexType eventIdx, const CTimelineEvent* pTimelineEvent)
{
   // Step 4: Create a single interval for loadings

   // For each activity in this event, get phrase to use in the description of the interval
   // as well as record the interval index if needed. Since the interval object hasn't
   // been added to the m_Intervals collection yet, the interval index is the size of the collection
   IntervalIndexType intervalIdx = INVALID_INDEX;
   bool bNeedNewInterval = true;
   if (pTimelineEvent->GetCastLongitudinalJointActivity().IsEnabled())
   {
      // loads are being applied with the longitudinal joints, use the longitudinal joint casting interval
      intervalIdx = m_CastLongitudinalJointsIntervalIdx;
      bNeedNewInterval = false;
   }
   else if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
   {
      // loads are being applied with the deck, use the deck casting interval for the first region
      // assumes loads are applied when the first region is cast
      intervalIdx = m_vCastDeckIntervalIdx.front();
      bNeedNewInterval = false;
   }
   else if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() )
   {
      // loads are being applied with closure joints, use the closure joint casting interval
      std::vector<CClosureKey> vClosureKeys = GetClosureJoints(pTimelineEvent);
      ATLASSERT(0 < vClosureKeys.size());
      intervalIdx = GetCastClosureInterval(vClosureKeys.front());
      bNeedNewInterval = false;
   }
   else
   {
      intervalIdx = m_Intervals.size();
      if (m_CompositeLongitudinalJointsIntervalIdx + 1 == intervalIdx)
      {
         // loads are being applied in the same interval the longitudinal joints become composite.
         // the longitudinal joints becoming composite is a zero duration interval and so is this loading
         // interval. put the loading in the composite longitudinal joint interval
         intervalIdx = m_CompositeLongitudinalJointsIntervalIdx;
         bNeedNewInterval = false;
      }
      else if (!m_vCompositeDeckIntervalIdx.empty() && m_vCompositeDeckIntervalIdx.front() + 1 == intervalIdx)
      {
         // loads are being applied in the same interval the deck becomes composite.
         // assume the loads are applied when the first cast region becomes composite
         // the deck becoming composite is a zero duration interval and so is this loading
         // interval. put the loading in the composite deck interval
         intervalIdx = m_vCompositeDeckIntervalIdx.front();
         bNeedNewInterval = false;
      }
   }
   
   std::vector<CString> strDescriptions;

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   const CApplyLoadActivity& applyLoadActivity = pTimelineEvent->GetApplyLoadActivity();
   if ( applyLoadActivity.IsEnabled() )
   {
      IntervalIndexType loadingIntervalIdx = intervalIdx;

      bool bDiaphragms = applyLoadActivity.IsIntermediateDiaphragmLoadApplied();
      bool bRailing    = applyLoadActivity.IsRailingSystemLoadApplied();
      bool bOverlay    = applyLoadActivity.IsOverlayLoadApplied();
      bool bLiveLoad   = applyLoadActivity.IsLiveLoadApplied();
      bool bLoadRating = applyLoadActivity.IsRatingLiveLoadApplied();
      bool bUserLoad   = applyLoadActivity.IsUserLoadApplied();

      bool bHasDiaphragmBeenApplied  = false;
      bool bHasRailingBeenApplied    = false;
      bool bHasOverlayBeenApplied    = false;
      bool bHasLiveLoadBeenApplied   = false;
      bool bHasLoadRatingBeenApplied = false;

      if ( bUserLoad )
      {
         strDescriptions.push_back(CString(_T("Apply User Defined Loads")));
      }

      if (bDiaphragms && !bHasDiaphragmBeenApplied)
      {
         strDescriptions.push_back(CString(_T("Install Intermediate Diaphragms")));
         bHasDiaphragmBeenApplied = true;
      }

      if ( bRailing && !bHasRailingBeenApplied )
      {
         // railing system load is applied in this activity and it has not yet
         // been accounted for

         if ( bOverlay )
         {
            // overlay and railing system applied at the same time (not a future overlay)
            strDescriptions.push_back(CString(_T("Install Railing System and Overlay")));
            bHasRailingBeenApplied = true;
            bHasOverlayBeenApplied = true; // overlay had already been accounted for..
         }
         else
         {
            // railing system applied by itself
            strDescriptions.push_back(CString(_T("Install Railing System")));
            bHasRailingBeenApplied = true;
         }
      }

      if ( bOverlay && !bHasOverlayBeenApplied )
      {
         // overlay load is applied in this activity and it has not yet 
         // been accounted for
         if ( bLiveLoad && !bHasLiveLoadBeenApplied )
         {
            // overlay and live load has been applied at the same tine
            // not a future overlay
            strDescriptions.push_back(CString(_T("Open to Traffic")));
            bHasOverlayBeenApplied = true;
            bHasLiveLoadBeenApplied = true;
         }
         else
         {
            // overlay is applied in its own interval
            strDescriptions.push_back(CString(_T("Install Overlay")));
            bHasOverlayBeenApplied = true;
         }
      }

      if ( bLiveLoad && !bHasLiveLoadBeenApplied )
      {
         // live load is applied in its own interval
         strDescriptions.push_back(CString(_T("Open to Traffic")));
         bHasLiveLoadBeenApplied = true;

         if ( bLoadRating )
         {
            bHasLoadRatingBeenApplied = true;
         }
      }

      if ( bLiveLoad && !bHasLiveLoadBeenApplied )
      {
         // live load is applied in its own interval
         strDescriptions.push_back(CString(_T("Open to Traffic")));
         bHasLiveLoadBeenApplied = true;

         if ( bLoadRating )
         {
            bHasLoadRatingBeenApplied = true;
         }
      }

      if ( bLoadRating && !bHasLoadRatingBeenApplied )
      {
         strDescriptions.push_back(CString(_T("Load Rating")));
         bHasLoadRatingBeenApplied = true;
      }

      if (bHasDiaphragmBeenApplied)
      {
         m_CastIntermediateDiaphragmsIntervalIdx = loadingIntervalIdx;
      }

      if ( bHasLiveLoadBeenApplied )
      {
         m_LiveLoadIntervalIdx = loadingIntervalIdx;
      }

      if ( bHasOverlayBeenApplied )
      {
         m_OverlayIntervalIdx = loadingIntervalIdx;
      }

      if ( bHasRailingBeenApplied )
      {
         m_RailingSystemIntervalIdx = loadingIntervalIdx;
      }

      if ( bUserLoad )
      {
         GET_IFACE(IUserDefinedLoadData,pUserDefinedLoadData);

         IndexType nUserLoads = applyLoadActivity.GetUserLoadCount();
         for ( IndexType userLoadIdx = 0; userLoadIdx < nUserLoads; userLoadIdx++ )
         {
            LoadIDType userLoadID = applyLoadActivity.GetUserLoadID(userLoadIdx);

            UserLoads::LoadCase loadCase;

            const CPointLoadData*       pPointLoad       = pUserDefinedLoadData->FindPointLoad(userLoadID);
            const CDistributedLoadData* pDistributedLoad = pUserDefinedLoadData->FindDistributedLoad(userLoadID);
            const CMomentLoadData*      pMomentLoad      = pUserDefinedLoadData->FindMomentLoad(userLoadID);

            CSpanKey spanKey;
            if ( pPointLoad )
            {
               loadCase = pPointLoad->m_LoadCase;
               spanKey = pPointLoad->m_SpanKey;
               ATLASSERT(pPointLoad->m_ID == userLoadID);
            }
            else if ( pDistributedLoad )
            {
               loadCase = pDistributedLoad->m_LoadCase;
               spanKey = pDistributedLoad->m_SpanKey;
               ATLASSERT(pDistributedLoad->m_ID == userLoadID);
            }
            else
            {
               ATLASSERT(pMomentLoad);
               loadCase = pMomentLoad->m_LoadCase;
               spanKey = pMomentLoad->m_SpanKey;
               ATLASSERT(pMomentLoad->m_ID == userLoadID);
            }

            if ( loadCase != UserLoads::LL_IM )
            {
               SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
               SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
               SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
               for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
               {
                  const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
                  if ( pSpan == nullptr )
                  {
                     // loading is out of range... ValidateLoad will deal with it. We'll just ignore it and continue
                     continue;
                  }
                  const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
                  GirderIndexType nGirders = pGroup->GetGirderCount();
                  GirderIndexType startGirderIdx = (spanKey.girderIndex == ALL_GIRDERS ? 0 : spanKey.girderIndex);
                  GirderIndexType endGirderIdx   = (spanKey.girderIndex == ALL_GIRDERS ? nGirders-1 : startGirderIdx);
                  for ( GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++ )
                  {
                     CSpanKey thisSpanKey(spanIdx,gdrIdx);
                     CUserLoadKey key(thisSpanKey,userLoadID);
                     m_UserLoadInterval[loadCase].insert(std::make_pair(key,loadingIntervalIdx));
                  }
               }
            }
         }
      }
   } // end if loading activity

   if ( strDescriptions.size() == 0 )
   {
      return; // none of the activities are enabled for this event
   }

   // Build up the composite description for this interval
   CString strDescription;
   for(const auto& str : strDescriptions)
   {
      if (strDescription.GetLength() != 0 )
      {
         strDescription += _T(", ");
      }

      strDescription += str;
   }

   if ( bNeedNewInterval )
   {
      CInterval interval;
      interval.StartEventIdx = eventIdx;
      interval.EndEventIdx   = eventIdx;
      interval.Start         = pTimelineEvent->GetDay() + pTimelineEvent->GetDuration(); // this interval starts at end end of the event and has zero duration
      interval.End           = interval.Start;
      interval.Middle        = interval.Start;
      interval.Duration      = 0;
      interval.Description = strDescription;
      StoreInterval(interval);
   }
   else
   {
      if (m_Intervals[intervalIdx].Description.length() != 0)
      {
         m_Intervals[intervalIdx].Description += _T(", ");
      }

      m_Intervals[intervalIdx].Description += strDescription;
   }
}

void CIntervalManager::ProcessStep5(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // At the end of every event, create a general time step
   // that goes from the end of last interval for the current event
   // to the start of the next event
   EventIndexType nEvents = pTimelineEvent->GetTimelineManager()->GetEventCount();
   if ( eventIdx == nEvents-1 && pTimelineEvent->GetDuration() == 0 )
   {
      // don't need to create a zero duration time step at the end of the timeline
      return;
   }

   CInterval timeStepInterval;
   timeStepInterval.StartEventIdx = eventIdx;
   timeStepInterval.EndEventIdx   = eventIdx;
   timeStepInterval.Start = (m_Intervals.size() == 0 ? pTimelineEvent->GetDay() : m_Intervals.back().End);
   if ( eventIdx < nEvents-1 )
   {
      timeStepInterval.EndEventIdx++; // this time step ends at the start of the next event unless this is the last event
   }
   timeStepInterval.End = pTimelineEvent->GetTimelineManager()->GetStart(timeStepInterval.EndEventIdx); // ends at the start of the next event

   timeStepInterval.Duration = timeStepInterval.End - timeStepInterval.Start;
   timeStepInterval.Middle = 0.5*(timeStepInterval.Start + timeStepInterval.End);
   timeStepInterval.Description = _T("Time Step");

   if ( 0 < timeStepInterval.Duration )
   {
      StoreInterval(timeStepInterval);
   }
}

std::vector<CClosureKey> CIntervalManager::GetClosureJoints(const CTimelineEvent* pTimelineEvent)
{
   const CCastClosureJointActivity& closureJointActivity = pTimelineEvent->GetCastClosureJointActivity();
   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   std::vector<CClosureKey> vClosureKeys;
   const std::set<PierIDType>& vPierIDs(closureJointActivity.GetPiers());
   for( const auto& pierID : vPierIDs)
   {
      const CPierData2* pPier = pBridgeDesc->FindPier(pierID);
      const CGirderGroupData* pGroup = pPier->GetGirderGroup(pgsTypes::Ahead); // shouldn't matter which side
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CClosureJointData* pClosureJoint = pPier->GetClosureJoint(gdrIdx);
         CClosureKey closureKey(pClosureJoint->GetClosureKey());
         vClosureKeys.push_back(closureKey);
      }
   }
   const std::set<SupportIDType>& vTempSupportIDs(closureJointActivity.GetTempSupports());
   for(const auto& tsID : vTempSupportIDs)
   {
      const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
      GirderIndexType nGirders = pTS->GetSpan()->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(gdrIdx);
         CClosureKey closureKey(pClosureJoint->GetClosureKey());
         vClosureKeys.push_back(closureKey);
      }
   }

   return vClosureKeys;
}

IntervalIndexType CIntervalManager::GetFirstInterval(const CGirderKey& girderKey,const std::map<CGirderKey, std::pair<IntervalIndexType, IntervalIndexType>>& intervalLimits) const
{
   if (girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS)
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      for (const auto& iter : intervalLimits)
      {
         if ((girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
            (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter.first.girderIndex) ||
            (girderKey.groupIndex == iter.first.groupIndex && girderKey.girderIndex == ALL_GIRDERS)
            )
         {
            intervalIdx = Min(intervalIdx, iter.second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      auto found(intervalLimits.find(girderKey));
      if (found != intervalLimits.end())
      {
         return found->second.first;
      }
      else
      {
         // probably an unequal number of girders per group, and this one has less.
         CGirderKey newKey = GetSafeGirderKey(m_pBroker, girderKey);

         found = intervalLimits.find(newKey);
         return found->second.second; // this will crash if not found, so no bother with assert
      }
   }
}

IntervalIndexType CIntervalManager::GetLastInterval(const CGirderKey& girderKey, const std::map<CGirderKey, std::pair<IntervalIndexType, IntervalIndexType>>& intervalLimits) const
{
   if (girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS)
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      for (const auto& iter : intervalLimits)
      {
         if ((girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
            (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter.first.girderIndex) ||
            (girderKey.groupIndex == iter.first.groupIndex && girderKey.girderIndex == ALL_GIRDERS)
            )
         {
            if (intervalIdx == MAX_INDEX)
            {
               intervalIdx = 0;
            }
            intervalIdx = Max(intervalIdx, iter.second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      auto found(intervalLimits.find(girderKey));
      if (found != intervalLimits.end())
      {
         return found->second.second;
      }
      else
      {
         // probably an unequal number of girders per group, and this one has less.
         CGirderKey newKey = GetSafeGirderKey(m_pBroker, girderKey);

         found = intervalLimits.find(newKey);
         return found->second.second; // this will crash if not found, so no bother with assert
      }
   }
}

#if defined _DEBUG
void CIntervalManager::AssertValid() const
{
   for (const auto& interval : m_Intervals)
   {
      interval.AssertValid();
   }
}

void CIntervalManager::CInterval::AssertValid() const
{
   ATLASSERT(StartEventIdx != INVALID_INDEX);
   ATLASSERT(EndEventIdx != INVALID_INDEX);
   ATLASSERT(StartEventIdx <= EndEventIdx);
   ATLASSERT(Start <= Middle);
   ATLASSERT(Start <= End);
   ATLASSERT(Middle <= End);
   ATLASSERT(Duration == End - Start);
   ATLASSERT(Middle == 0.5*(Start + End));
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// User Load Key
CIntervalManager::CUserLoadKey::CUserLoadKey(const CSpanKey& spanKey,LoadIDType loadID) :
m_SpanKey(spanKey),m_LoadID(loadID)
{
}

CIntervalManager::CUserLoadKey::CUserLoadKey(const CUserLoadKey& other) :
m_SpanKey(other.m_SpanKey),m_LoadID(other.m_LoadID)
{
}

bool CIntervalManager::CUserLoadKey::operator<(const CUserLoadKey& other) const
{
   if ( m_SpanKey < other.m_SpanKey )
   {
      return true;
   }

   if ( m_SpanKey.IsEqual(other.m_SpanKey) && m_LoadID < other.m_LoadID )
   {
      return true;
   }

   return false;
}

