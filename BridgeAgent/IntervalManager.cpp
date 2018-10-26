///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\ClosureJointData.h>

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

CIntervalManager::CIntervalManager()
{
   m_bIsPGSuper = false;
}

void CIntervalManager::BuildIntervals(const CTimelineManager* pTimelineMgr)
{
   // this method builds the analysis interval sequence as well as defines the stage model
   // for the generic bridge model.

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IDocumentType,pDocType);
   m_bIsPGSuper = pDocType->IsPGSuperDocument();

   // reset everything
   m_CastDeckIntervalIdx      = INVALID_INDEX;
   m_CompositeDeckIntervalIdx = INVALID_INDEX;
   m_LiveLoadIntervalIdx      = INVALID_INDEX;
   m_OverlayIntervalIdx       = INVALID_INDEX;
   m_RailingSystemIntervalIdx = INVALID_INDEX;
   m_UserLoadInterval[UserLoads::DC].clear();
   m_UserLoadInterval[UserLoads::DW].clear();

   m_StressStrandIntervals.clear();
   m_ReleaseIntervals.clear();

   m_Intervals.clear();
   m_SegmentHaulingIntervals.clear();
   m_SegmentErectionIntervals.clear();
   m_CastClosureIntervals.clear();
   m_StrandStressingSequenceIntervalLimits.clear();
   m_ReleaseSequenceIntervalLimits.clear();
   m_SegmentErectionSequenceIntervalLimits.clear();
   m_RemoveTemporaryStrandsIntervals.clear();

   m_ErectPierIntervals.clear();
   m_ErectTemporarySupportIntervals.clear();
   m_RemoveTemporarySupportIntervals.clear();

   m_StressTendonIntervals.clear();

   const CBridgeDescription2* pBridgeDesc = pTimelineMgr->GetBridgeDescription();

   // work through all the events in the timeline and build analysis intervals and modeling stages
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   ATLASSERT(0 < nEvents);
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      ProcessStep1(eventIdx,pTimelineEvent);
      ProcessStep2(eventIdx,pTimelineEvent);
      ProcessStep3(eventIdx,pTimelineEvent);
      ProcessStep4(eventIdx,pTimelineEvent);
      ProcessStep5(eventIdx,pTimelineEvent);
   } // next event
   
   // If we aren't doing time step analysis, this is a PGSuper project
   // Append the old Bridge Site names to the interval descriptions for
   // continuity with previous versions
   ATLASSERT(0 < m_Intervals.size());
   if ( m_bIsPGSuper )
   {
      m_Intervals[m_ReleaseIntervals.begin()->second].Description += _T(" (Casting Yard)");
      m_Intervals[m_CastDeckIntervalIdx].Description              += _T(" (Bridge Site 1)");
      if ( m_OverlayIntervalIdx == INVALID_INDEX )
      {
         // no overlay case
         m_Intervals[m_RailingSystemIntervalIdx].Description      += _T(" (Bridge Site 2)");
      }
      else
      {
         m_Intervals[m_OverlayIntervalIdx].Description            += _T(" (Bridge Site 2)");
      }
      m_Intervals[m_LiveLoadIntervalIdx].Description              += _T(" (Bridge Site 3)");
   }


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
   std::map<PierIndexType,IntervalIndexType>::const_iterator found = m_ErectPierIntervals.find(pierIdx);
   if ( found == m_ErectPierIntervals.end() )
   {
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetCastDeckInterval() const
{
   return m_CastDeckIntervalIdx;
}

IntervalIndexType CIntervalManager::GetCompositeDeckInterval() const
{
   return m_CompositeDeckIntervalIdx;
}

IntervalIndexType CIntervalManager::GetFirstStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_StrandStressingSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_StrandStressingSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_StrandStressingSequenceIntervalLimits.end());
      return found->second.first;
   }
}

IntervalIndexType CIntervalManager::GetLastStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = INVALID_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_StrandStressingSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_StrandStressingSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            if ( intervalIdx == MAX_INDEX )
            {
               intervalIdx = 0;
            }
            intervalIdx = Max(intervalIdx,iter->second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_StrandStressingSequenceIntervalLimits.end());
      return found->second.second;
   }
}

IntervalIndexType CIntervalManager::GetStressStrandInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_StressStrandIntervals.find(segmentKey));
   ATLASSERT( found != m_StressStrandIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_ReleaseSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_ReleaseSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_ReleaseSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_ReleaseSequenceIntervalLimits.end());
      return found->second.first;
   }
}

IntervalIndexType CIntervalManager::GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_ReleaseSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_ReleaseSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            if ( intervalIdx == MAX_INDEX )
            {
               intervalIdx = 0;
            }
            intervalIdx = Max(intervalIdx,iter->second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_ReleaseSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_ReleaseSequenceIntervalLimits.end());
      return found->second.second;
   }
}

IntervalIndexType CIntervalManager::GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_ReleaseIntervals.find(segmentKey));
   ATLASSERT( found != m_ReleaseIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetLiftingInterval(const CSegmentKey& segmentKey) const
{
   return GetPrestressReleaseInterval(segmentKey) + 1;
}

IntervalIndexType CIntervalManager::GetFirstLiftingInterval(const CGirderKey& girderKey) const
{
   return GetFirstPrestressReleaseInterval(girderKey)+1;
}

IntervalIndexType CIntervalManager::GetLastLiftingInterval(const CGirderKey& girderKey) const
{
   return GetLastPrestressReleaseInterval(girderKey)+1;
}

IntervalIndexType CIntervalManager::GetStorageInterval(const CSegmentKey& segmentKey) const
{
   return GetPrestressReleaseInterval(segmentKey) + 2;
}

IntervalIndexType CIntervalManager::GetFirstStorageInterval(const CGirderKey& girderKey) const
{
   return GetFirstPrestressReleaseInterval(girderKey)+2;
}

IntervalIndexType CIntervalManager::GetLastStorageInterval(const CGirderKey& girderKey) const
{
   return GetLastPrestressReleaseInterval(girderKey)+2;
}

IntervalIndexType CIntervalManager::GetHaulingInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_SegmentHaulingIntervals.find(segmentKey));
   ATLASSERT( found != m_SegmentHaulingIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetErectSegmentInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_SegmentErectionIntervals.find(segmentKey));
   ATLASSERT( found != m_SegmentErectionIntervals.end());
   return found->second;
}

bool CIntervalManager::IsSegmentErectionInterval(IntervalIndexType intervalIdx) const
{
   ATLASSERT(intervalIdx != INVALID_INDEX);
   std::map<CSegmentKey,IntervalIndexType>::const_iterator iter(m_SegmentErectionIntervals.begin());
   std::map<CSegmentKey,IntervalIndexType>::const_iterator iterEnd(m_SegmentErectionIntervals.end());
   for ( ; iter != iterEnd; iter++ )
   {
      if ( iter->second == intervalIdx )
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

   std::map<CSegmentKey,IntervalIndexType>::const_iterator iter(m_SegmentErectionIntervals.begin());
   std::map<CSegmentKey,IntervalIndexType>::const_iterator iterEnd(m_SegmentErectionIntervals.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CSegmentKey& segmentKey = iter->first;
      if ( girderKey.IsEqual(segmentKey) )
      {
         IntervalIndexType erectionIntervalIdx = iter->second;
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
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_RemoveTemporaryStrandsIntervals.find(segmentKey));
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
   std::map<CClosureKey,IntervalIndexType>::const_iterator found(m_CastClosureIntervals.find(clousreKey));
   ATLASSERT( found != m_CastClosureIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstCastClosureJointInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   IntervalIndexType intervalIdx = INVALID_INDEX;
   std::map<CClosureKey,IntervalIndexType>::const_iterator iter(m_CastClosureIntervals.begin());
   std::map<CClosureKey,IntervalIndexType>::const_iterator end(m_CastClosureIntervals.end());
   for ( ; iter != end; iter++ )
   {
      const CClosureKey& closureKey = iter->first;
      if ( closureKey.groupIndex == girderKey.groupIndex && closureKey.girderIndex == girderKey.girderIndex )
      {
         intervalIdx = Min(intervalIdx,iter->second);
      }
   }

   return intervalIdx;
}

IntervalIndexType CIntervalManager::GetLastCastClosureJointInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   IntervalIndexType intervalIdx = 0;
   std::map<CClosureKey,IntervalIndexType>::const_iterator iter(m_CastClosureIntervals.begin());
   std::map<CClosureKey,IntervalIndexType>::const_iterator end(m_CastClosureIntervals.end());
   for ( ; iter != end; iter++ )
   {
      const CClosureKey& closureKey = iter->first;
      if ( closureKey.groupIndex == girderKey.groupIndex && closureKey.girderIndex == girderKey.girderIndex )
      {
         intervalIdx = Max(intervalIdx,iter->second);
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
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_SegmentErectionSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_SegmentErectionSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_SegmentErectionSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_SegmentErectionSequenceIntervalLimits.end());
      return found->second.first;
   }

}

IntervalIndexType CIntervalManager::GetLastSegmentErectionInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_SegmentErectionSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_SegmentErectionSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            if ( intervalIdx == MAX_INDEX )
            {
               intervalIdx = 0;
            }
            intervalIdx = Max(intervalIdx,iter->second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_SegmentErectionSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_SegmentErectionSequenceIntervalLimits.end());
      return found->second.second;
   }
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
   std::map<CUserLoadKey,IntervalIndexType>::const_iterator found(m_UserLoadInterval[loadCase].find(key));
   if ( found == m_UserLoadInterval[loadCase].end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CTendonKey,IntervalIndexType>::const_iterator found(m_StressTendonIntervals.find(CTendonKey(girderKey,ductIdx)));
   if ( found == m_StressTendonIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   std::map<CTendonKey,IntervalIndexType>::const_iterator iter(m_StressTendonIntervals.begin());
   std::map<CTendonKey,IntervalIndexType>::const_iterator end(m_StressTendonIntervals.end());
   for ( ; iter != end; iter++ )
   {
      if ( girderKey == iter->first.girderKey )
      {
         intervals.insert(iter->second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.begin();
}

IntervalIndexType CIntervalManager::GetLastTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   std::map<CTendonKey,IntervalIndexType>::const_iterator iter(m_StressTendonIntervals.begin());
   std::map<CTendonKey,IntervalIndexType>::const_iterator end(m_StressTendonIntervals.end());
   for ( ; iter != end; iter++ )
   {
      if ( girderKey == iter->first.girderKey )
      {
         intervals.insert(iter->second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.rbegin();
}

IntervalIndexType CIntervalManager::GetTemporarySupportErectionInterval(SupportIndexType tsIdx) const
{
   std::map<SupportIndexType,IntervalIndexType>::const_iterator found(m_ErectTemporarySupportIntervals.find(tsIdx) );
   if ( found == m_ErectTemporarySupportIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) const
{
   std::map<SupportIndexType,IntervalIndexType>::const_iterator found(m_RemoveTemporarySupportIntervals.find(tsIdx) );
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
   BOOST_FOREACH(PierIDType pierID,pierIDs)
   {
      const CPierData2* pPier = pBridgeDesc->FindPier(pierID);
      PierIndexType pierIdx = pPier->GetIndex();
      m_ErectPierIntervals.insert(std::make_pair(pierIdx,intervalIdx));
   }

   const std::set<SupportIDType>& tsIDs(activity.GetTempSupports());
   BOOST_FOREACH(SupportIDType tsID,tsIDs)
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
   // d) Cast Deck
   // e) Cast Closure

   // For each activity in this event, get phrase to use in the description of the interval
   // as well as record the interval index if needed. Since the interval object hasn't
   // been added to the m_Intervals collection yet, the interval is the size of the collection
   IntervalIndexType intervalIdx = m_Intervals.size();
   
   std::vector<CString> strDescriptions;

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   bool bRemoveTemporaryStrands = false; // need to know if there are temporary strands that are removed
   // if so, we will add another interval after this interval
   const CErectSegmentActivity& erectSegmentsActivity = pTimelineEvent->GetErectSegmentsActivity();
   if ( erectSegmentsActivity.IsEnabled() )
   {
      // Segments must be hauled to the bridge site before they are erected
      CInterval haulSegmentInterval;
      haulSegmentInterval.StartEventIdx = eventIdx;
      haulSegmentInterval.EndEventIdx   = eventIdx;
      haulSegmentInterval.Start         = pTimelineEvent->GetDay();
      haulSegmentInterval.End           = haulSegmentInterval.Start;
      haulSegmentInterval.Middle        = haulSegmentInterval.Start;
      haulSegmentInterval.Duration      = 0;
      haulSegmentInterval.Description   = (m_bIsPGSuper ? _T("Haul Girders") : _T("Haul Segments"));
      IntervalIndexType haulIntervalIdx = StoreInterval(haulSegmentInterval);

      std::set<SegmentIDType> erectedSegments(erectSegmentsActivity.GetSegments());
      BOOST_FOREACH(SegmentIDType segmentID, erectedSegments)
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

      BOOST_FOREACH(SegmentIDType segmentID, erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         m_SegmentErectionIntervals.insert(std::make_pair(segmentKey,erectSegmentIntervalIdx));

         if ( 0 < pSegment->Strands.GetStrandCount(pgsTypes::Temporary) )
         {
            bRemoveTemporaryStrands = true;
         }

         // this is for keeping track of when the first and last segments in a girder are erected
         std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator found(m_SegmentErectionSequenceIntervalLimits.find(segmentKey));
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

   const CRemoveTemporarySupportsActivity& removeTemporarySupportActivity = pTimelineEvent->GetRemoveTempSupportsActivity();
   if ( removeTemporarySupportActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Remove Temporary Support")));
      IntervalIndexType removeTempSupportIntervalIdx = intervalIdx;

      const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
      const std::vector<SupportIDType>& tsIDs(removeTS.GetTempSupports());
      BOOST_FOREACH(SupportIDType tsID,tsIDs)
      {
         const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
         SupportIndexType tsIdx = pTS->GetIndex();
         m_RemoveTemporarySupportIntervals.insert(std::make_pair(tsIdx,removeTempSupportIntervalIdx));
      }
   }

   const CStressTendonActivity& stressTendonActivity = pTimelineEvent->GetStressTendonActivity();
   if ( stressTendonActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Stress Tendons")));
      IntervalIndexType stressTendonIntervalIdx = intervalIdx;

      const std::set<CTendonKey>& tendons( stressTendonActivity.GetTendons() );
      BOOST_FOREACH(CTendonKey tendonKey,tendons)
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
         const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         CComPtr<IGirderSection> gdrSection;
         factory->CreateGirderSection(NULL,INVALID_ID,pGdrEntry->GetDimensions(),-1,-1,&gdrSection);

         WebIndexType nWebs;
         gdrSection->get_WebCount(&nWebs);

         for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
         {
            DuctIndexType thisDuctIdx = nWebs*tendonKey.ductIdx + webIdx;
            CTendonKey thisTendonKey(tendonKey.girderKey,thisDuctIdx);
            m_StressTendonIntervals.insert(std::make_pair(thisTendonKey,stressTendonIntervalIdx));
         }
      }
   }

   const CCastDeckActivity& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
   if ( castDeckActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Cast Deck")));
      m_CastDeckIntervalIdx = intervalIdx;
   } // end if cast deck activity

   const CCastClosureJointActivity& closureJointActivity = pTimelineEvent->GetCastClosureJointActivity();
   if ( closureJointActivity.IsEnabled() )
   {
      strDescriptions.push_back(CString(_T("Cast Closure Joint")));
      IntervalIndexType castClosureIntervalIdx = intervalIdx;

      const std::set<PierIDType>& vPierIDs(closureJointActivity.GetPiers());
      BOOST_FOREACH(PierIDType pierID,vPierIDs)
      {
         const CPierData2* pPier = pBridgeDesc->FindPier(pierID);
         const CGirderGroupData* pGroup = pPier->GetGirderGroup(pgsTypes::Ahead); // shouldn't matter which side
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            const CClosureJointData* pClosureJoint = pPier->GetClosureJoint(gdrIdx);
            CClosureKey closureKey(pClosureJoint->GetClosureKey());
            m_CastClosureIntervals.insert(std::make_pair(closureKey,castClosureIntervalIdx));
         }
      }
      const std::set<SupportIDType>& vTempSupportIDs(closureJointActivity.GetTempSupports());
      BOOST_FOREACH(SupportIDType tsID,vTempSupportIDs)
      {
         const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
         GirderIndexType nGirders = pTS->GetSpan()->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(gdrIdx);
            CClosureKey closureKey(pClosureJoint->GetClosureKey());
            m_CastClosureIntervals.insert(std::make_pair(closureKey,castClosureIntervalIdx));
         }
      }
   } // end if cast deck activity

   if ( strDescriptions.size() == 0 )
   {
      return; // none of the activities are enabled for this event
   }

   // Build up the composite description for this interval
   CString strDescription;
   BOOST_FOREACH(CString& str,strDescriptions)
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
      BOOST_FOREACH(SegmentIDType segmentID, erectedSegments)
      {
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
         CSegmentKey segmentKey(pSegment->GetSegmentKey());
         m_RemoveTemporaryStrandsIntervals.insert(std::make_pair(segmentKey,removeTempStrandsIntervalIdx));
      } 
   }
}

#define SEGMENT 1
#define DECK 2
#define CLOSURE 3
void CIntervalManager::ProcessStep3(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // Step 3: Create curing duration intervals
   const CConstructSegmentActivity& constructSegmentActivity = pTimelineEvent->GetConstructSegmentsActivity();
   const CCastDeckActivity& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
   const CCastClosureJointActivity& castClosureJointActivity = pTimelineEvent->GetCastClosureJointActivity();

   std::map<Float64,int> curingDurations;
   if ( constructSegmentActivity.IsEnabled() )
   {
      curingDurations.insert( std::make_pair(constructSegmentActivity.GetRelaxationTime(),SEGMENT) );
   }

   if ( castDeckActivity.IsEnabled() )
   {
      curingDurations.insert( std::make_pair(castDeckActivity.GetConcreteAgeAtContinuity(),DECK) );
   }

   if ( castClosureJointActivity.IsEnabled() )
   {
      curingDurations.insert( std::make_pair(castClosureJointActivity.GetConcreteAgeAtContinuity(),CLOSURE) );
   }

   if ( curingDurations.size() == 0 )
   {
      return; // none of the activities are enabled during this event
   }

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   // work in order of shortest duration first
   Float64 previous_curing_duration = 0;
   std::map<Float64,int>::iterator iter(curingDurations.begin());
   std::map<Float64,int>::iterator end(curingDurations.end());
   for ( ; iter != end; iter++ )
   {
      Float64 duration = iter->first;
      int activityType = iter->second;

      // the concrete for all concrete casting activities during this event
      // are cast at the same time. the curing intervals are based on the
      // one with the shortest curing time, then the next longest, and so on.
      // the duration of the curing time is the curing time for the
      // given activity less the duration of the other curing times that
      // have already occuring during this activity
      //
      // e.g. Assume deck concrete and closure joint concrete are cast at the same
      // time. It takes the closure joint 3 days to cure and the deck 10 days.
      // The curing duration for the closure joint and deck together is modeled as 3 days. the
      // curing duration duration for the deck then proceeds for an additional 7 days
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

         // lift segment
         CInterval liftSegmentInterval(releaseInterval);
         liftSegmentInterval.Start = releaseInterval.End;
         liftSegmentInterval.Duration = 0;
         liftSegmentInterval.Middle = liftSegmentInterval.Start;
         liftSegmentInterval.Description = m_bIsPGSuper ? _T("Lift girders") : _T("Lift segments");
         IntervalIndexType liftIntervalIdx = StoreInterval(liftSegmentInterval);

         // placing into storage changes boundary conditions... treat as sudden change in loading
         CInterval storageInterval(liftSegmentInterval);
         storageInterval.Start = liftSegmentInterval.End;
         storageInterval.Duration = 0;
         storageInterval.Middle = storageInterval.Start;
         storageInterval.Description = m_bIsPGSuper ? _T("Place girders into storage") : _T("Place segments into storage");
         IntervalIndexType storateIntervalIdx = StoreInterval(storageInterval);


         // record the segments that are constructed during this activity
         const std::set<SegmentIDType>& segments = constructSegmentActivity.GetSegments();
         BOOST_FOREACH(SegmentIDType segmentID,segments)
         {
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            ASSERT_SEGMENT_KEY(segmentKey);

            m_StressStrandIntervals.insert(std::make_pair(segmentKey,stressStrandIntervalIdx));
            m_ReleaseIntervals.insert(std::make_pair(segmentKey,releaseIntervalIdx));

            // this is for keeping track of when the strands are stressed for the first and last segments constructed for this girder
            std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator strandStressingFound(m_StrandStressingSequenceIntervalLimits.find(segmentKey));
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
            std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator releaseFound(m_ReleaseSequenceIntervalLimits.find(segmentKey));
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
      else if ( activityType == DECK )
      {
         CInterval cureDeckInterval;
         ATLASSERT(IsEqual(duration,castDeckActivity.GetConcreteAgeAtContinuity()));
         if ( 0 < duration )
         {
            // only model deck curing if we are doing a time-step analysis
            ATLASSERT(m_CastDeckIntervalIdx != INVALID_INDEX); // deck must have been previously cast
            cureDeckInterval.StartEventIdx = eventIdx;
            cureDeckInterval.EndEventIdx   = eventIdx;
            cureDeckInterval.Start         = m_Intervals.back().End; // curing starts when the previous interval ends
            cureDeckInterval.Duration      = remaining_duration;
            cureDeckInterval.End           = cureDeckInterval.Start + cureDeckInterval.Duration;
            cureDeckInterval.Middle        = 0.5*(cureDeckInterval.Start + cureDeckInterval.End);
            cureDeckInterval.Description = _T("Deck curing");
            StoreInterval(cureDeckInterval);

            CInterval compositeDeckInterval;
            compositeDeckInterval.StartEventIdx = eventIdx;
            compositeDeckInterval.EndEventIdx   = eventIdx;
            compositeDeckInterval.Start = cureDeckInterval.End;
            compositeDeckInterval.Duration = 0;
            compositeDeckInterval.End = compositeDeckInterval.Start + compositeDeckInterval.Duration;
            compositeDeckInterval.Middle = 0.5*(compositeDeckInterval.Start + compositeDeckInterval.End);
            compositeDeckInterval.Description = _T("Composite Deck");
            m_CompositeDeckIntervalIdx = StoreInterval(compositeDeckInterval);
         }
         else
         {
            // for non-timestep analysis (PGSuper) deck is composite the interval after it is cast
            m_CompositeDeckIntervalIdx = m_CastDeckIntervalIdx+1;
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

void CIntervalManager::ProcessStep4(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent)
{
   // Step 4: Create a single interval for loadings

   // For each activity in this event, get phrase to use in the description of the interval
   // as well as record the interval index if needed. Since the interval object hasn't
   // been added to the m_Intervals collection yet, the interval index is the size of the collection
   IntervalIndexType intervalIdx;
   bool bNeedNewInterval = true;
   if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
   {
      // loads are being applied with the deck, use the deck casting interval
      intervalIdx = m_CastDeckIntervalIdx;
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
      if ( m_CompositeDeckIntervalIdx+1 == intervalIdx )
      {
         // loads are being applied in the same interval the deck becomes composite.
         // the deck becoming composite is a zero duration interval and so is this loading
         // interval. put the loading in the composite deck interval
         intervalIdx = m_CompositeDeckIntervalIdx;
         bNeedNewInterval = false;
      }
   }
   
   std::vector<CString> strDescriptions;

   const CBridgeDescription2* pBridgeDesc = pTimelineEvent->GetTimelineManager()->GetBridgeDescription();

   const CApplyLoadActivity& applyLoadActivity = pTimelineEvent->GetApplyLoadActivity();
   if ( applyLoadActivity.IsEnabled() )
   {
      IntervalIndexType loadingIntervalIdx = intervalIdx;

      bool bRailing    = applyLoadActivity.IsRailingSystemLoadApplied();
      bool bOverlay    = applyLoadActivity.IsOverlayLoadApplied();
      bool bLiveLoad   = applyLoadActivity.IsLiveLoadApplied();
      bool bLoadRating = applyLoadActivity.IsRatingLiveLoadApplied();
      bool bUserLoad   = applyLoadActivity.IsUserLoadApplied();

      bool bHasRailingBeenApplied    = false;
      bool bHasOverlayBeenApplied    = false;
      bool bHasLiveLoadBeenApplied   = false;
      bool bHasLoadRatingBeenApplied = false;

      if ( bUserLoad && !bRailing && !bOverlay && !bLiveLoad && !bLoadRating )
      {
         strDescriptions.push_back(CString(_T("User Defined Loading Applied")));
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
            // overaly and live load has been applied at the same tine
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
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

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
                  if ( pSpan == NULL )
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
   BOOST_FOREACH(CString& str,strDescriptions)
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
      m_Intervals.back().Description += _T(", ") + strDescription;
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
   BOOST_FOREACH(PierIDType pierID,vPierIDs)
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
   BOOST_FOREACH(SupportIDType tsID,vTempSupportIDs)
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

#if defined _DEBUG
void CIntervalManager::AssertValid() const
{
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
