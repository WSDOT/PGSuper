///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#pragma once

#include <PgsExt\TimelineManager.h>
#include <PgsExt\PointLoadData.h> // for UserLoads

#include <IFace\BeamFactory.h>


////////////////////////////////////////////////
// This class manages the definitions for intervals of time for 
// a time-step loss analysis
class CIntervalManager : public IStages
{
public:
   // creates the time step intervals from the event model
   // if bTimeStepMethod is false, "time steps" that represent
   // the passage of time between intervals are not created
   // (this basically maps the PGSuper simple event model into a simple
   // interval model)
   void BuildIntervals(const CTimelineManager* pTimelineMgr,bool bTimeStepMethod);

   StageIndexType GetStageCount() const;
   StageIndexType GetStage(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const; // this is the IStages interface
   IntervalIndexType GetIntervalFromStage(const CGirderKey& girderKey,StageIndexType stageIdx) const;
   const std::map<IntervalIndexType,StageIndexType>& GetStageMap(const CGirderKey& girderKey) const;
   const std::map<CGirderKey,IntervalIndexType>& GetIntervalMap(StageIndexType stageIdx) const;


   IntervalIndexType GetIntervalCount(const CGirderKey& girderKey) const;
   EventIndexType GetStartEvent(const CGirderKey& girderKey,IntervalIndexType idx) const;
   EventIndexType GetEndEvent(const CGirderKey& girderKey,IntervalIndexType idx) const;
   Float64 GetTime(const CGirderKey& girderKey,IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const;
   Float64 GetDuration(const CGirderKey& girderKey,IntervalIndexType idx) const;
   LPCTSTR GetDescription(const CGirderKey& girderKey,IntervalIndexType idx) const;

   // returns the index of the first interval that starts with eventIdx
   IntervalIndexType GetInterval(const CGirderKey& girderKey,EventIndexType eventIdx) const;

   // returns the index of the interval when the prestressing strands are stressed for
   // the first segment constructed for this girder
   IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are stressed for
   // the last segment constructed for this girder
   IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are stressed
   IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the prestressing strands are released for
   // the first segment constructed for this girder
   IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are released for
   // the last segment constructed for this girder
   IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing is release
   // to the girder (girder has reached release strength)
   IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the segment is lifted from the
   // casting bed and placed into storage
   IntervalIndexType GetLiftingInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the segment is place in storage
   IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the segment is transported to the bridge site
   IntervalIndexType GetHaulingInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval for the first segment
   // to be erected
   IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval for the last segment
   // to be erected
   IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the specified segment is erected
   IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) const;

   // returns true if a segment is erected in the specified interval
   bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const;

   // returns the index of the interval when temporary strands are removed
   IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the deck and diaphragms are cast
   IntervalIndexType GetCastDeckInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the deck has finished curing
   // curing take place over the duration of an interval and cannot take load.
   // this method returns the index of the first interval when the deck can take load
   IntervalIndexType GetCompositeDeckInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when live load is first
   // applied to the structure. it is assumed that live
   // load can be applied to the structure at this interval and all
   // intervals thereafter
   IntervalIndexType GetLiveLoadInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the overlay is added to the bridge
   IntervalIndexType GetOverlayInterval(const CGirderKey& girderKey) const;

   // returns index of interval when the railing system is installed
   IntervalIndexType GetInstallRailingSystemInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when a user defined load is applied
   IntervalIndexType GetUserLoadInterval(const CGirderKey& girderKey,UserLoads::LoadCase loadCase,EventIndexType eventIdx) const;

   // returns the interval index when a temporary support is removed
   IntervalIndexType GetTemporarySupportRemovalInterval(const CGirderKey& girderKey,SupportIDType tsID) const;

   // returns the interval index when a tendon is stressed
   IntervalIndexType GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const;

   // returns the interval when the first tendon stressing occurs for the specified girder
   IntervalIndexType GetFirstTendonStressingInterval(const CGirderKey& girderKey) const;

   // returns the interval when the last tendon stressing occurs for the specified girder
   IntervalIndexType GetLastTendonStressingInterval(const CGirderKey& girderKey) const;

protected:
   bool m_bTimeStepMethod; // keeps track of the parameter used to create the intervals

   struct CInterval
   {
      CInterval():StartEventIdx(INVALID_INDEX),EndEventIdx(INVALID_INDEX){}
      EventIndexType StartEventIdx; // Event related to the start of this interval
      EventIndexType EndEventIdx;   // Event related to the end of this interval
      Float64        Start;         // Start of interval
      Float64        Middle;        // Middle of interval
      Float64        End;           // End of interval
      Float64        Duration;      // Interval duration
      std::_tstring  Description;   // Description of activity occuring during this interval
   };

   // each girder has its own sequence of intervals. this is because segments can
   // be constructed and erected at different times and there can be a different
   // number of tendons in each girder and they can be tensioned in different
   // sequences. after the deck is cast, all of the interval sequences are the same
   // it is just easier to do one complete sequence per girder
   std::map<CGirderKey,std::vector<CInterval>> m_IntervalSequences;

   // maps the interval to stage index for a girder
   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>> m_StageMap;
   
   // maps generic bridge modeling stages to girder/interval values
   // this is the reverse of m_StageMap. However, using the pair instead of CStageKey
   // because this map gets returned for external users
   std::map<StageIndexType,std::map<CGirderKey,IntervalIndexType>> m_IntervalMap;


   std::map<CTendonKey,IntervalIndexType> m_StressTendonIntervals;

   // map of when temporary supports are removed. this makes it easier
   // to provide this information
   class CTempSupportKey
   {
   public:
      CTempSupportKey(const CGirderKey& girderKey,SupportIDType tsID);
      CTempSupportKey(const CTempSupportKey& other);
      bool operator<(const CTempSupportKey& other) const;

      CGirderKey m_GirderKey;
      SupportIDType m_tsID;
   };
   std::map<CTempSupportKey,IntervalIndexType> m_TempSupportRemovalIntervals;

   // map of when the strands are stressed for the first and last segment constructed for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_StrandStressingSequenceIntervalLimits;

   // map of when the strands are release for the first and last segment constructed for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_ReleaseSequenceIntervalLimits;

   // map of when segments are erected. this makes it easier to provide this information
   std::map<CSegmentKey,IntervalIndexType> m_SegmentErectionIntervals;

   // map of when the first and last segment is erected for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_SegmentErectionSequenceIntervalLimits;

   std::map<CSegmentKey,IntervalIndexType> m_StressStrandIntervals;
   std::map<CSegmentKey,IntervalIndexType> m_ReleaseIntervals;

   std::map<CSegmentKey,IntervalIndexType> m_RemoveTemporaryStrandsIntervals;

   std::map<CGirderKey,IntervalIndexType> m_CastDeckInterval;
   std::map<CGirderKey,IntervalIndexType> m_CompositeDeckInterval; // interval when deck is composite
   std::map<CGirderKey,IntervalIndexType> m_LiveLoadInterval; // interval when live load is applied to the structure
   std::map<CGirderKey,IntervalIndexType> m_OverlayInterval;
   std::map<CGirderKey,IntervalIndexType> m_RailingSystemInterval;

   class CUserLoadKey
   {
   public:
      CUserLoadKey(const CGirderKey& girderKey,EventIndexType eventIdx);
      CUserLoadKey(const CUserLoadKey& other);
      bool operator<(const CUserLoadKey& other) const;

      CGirderKey m_GirderKey;
      EventIndexType m_EventIdx;
   };
   std::map<CUserLoadKey,IntervalIndexType> m_UserLoadInterval[2]; // interval when user DC/DW loads are applied
                                                                   // user LLIM are applied in the live load interval

   void AddToStageMap(const CGirderKey& girderKey,IntervalIndexType intervalIdx,StageIndexType stageIdx);

#if defined _DEBUG
   void AssertValid() const;
#endif
};