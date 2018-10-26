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

#pragma once

#include <PgsExt\TimelineManager.h>

////////////////////////////////////////////////
// This class manages the definitions for intervals of time for 
// a time-step loss analysis
class CIntervalManager
{
public:
   // creates the time step intervals from the event model
   // if bTimeStepMethod is false, "time steps" that represent
   // the passage of time between intervals are not created
   // (this basically maps the PGSuper simple event model into a simple
   // interval model)
   void BuildIntervals(const CTimelineManager* pTimelineMgr,bool bTimeStepMethod);
   IntervalIndexType GetIntervalCount() const;
   EventIndexType GetStartEvent(IntervalIndexType idx) const;
   EventIndexType GetEndEvent(IntervalIndexType idx) const;
   Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const;
   Float64 GetStart(IntervalIndexType idx) const;
   Float64 GetMiddle(IntervalIndexType idx) const;
   Float64 GetEnd(IntervalIndexType idx) const;
   Float64 GetDuration(IntervalIndexType idx) const;
   LPCTSTR GetDescription(IntervalIndexType idx) const;

   // returns the index of the first interval that starts with eventIdx
   IntervalIndexType GetInterval(EventIndexType eventIdx) const;

   // returns the index of the interval when the prestressing strands are stressed
   IntervalIndexType GetStressStrandInterval() const;

   // returns the index of the interval when the prestressing is release
   // to the girder (girder has reached release strength)
   IntervalIndexType GetPrestressReleaseInterval() const;

   // returns the index of the interval when the segment is lifted from the
   // casting bed and placed into storage
   IntervalIndexType GetLiftingInterval() const;

   // returns the index of the interval when the segment is place in storage
   IntervalIndexType GetStorageInterval() const;

   // returns the index of the interval when the segment is transported to the bridge site
   IntervalIndexType GetHaulingInterval() const;

   // returns the index of the interval for the first segment
   // to be erected
   IntervalIndexType GetFirstErectedSegmentInterval() const;

   // returns the index of the interval when temporary strands are removed
   IntervalIndexType GetTemporaryStrandRemovalInterval() const;

   // returns the index of the interval when the deck and diaphragms are cast
   IntervalIndexType GetCastDeckInterval() const;

   // returns the index of the interval when the deck has finished curing
   // curing take place over the duration of an interval and cannot take load.
   // this method returns the index of the next interval when the deck can take load
   IntervalIndexType GetCompositeDeckInterval() const;

   // returns the index of the interval when live load is first
   // applied to the structure. it is assumed that live
   // load can be applied to the structure at this interval and all
   // intervals thereafter
   IntervalIndexType GetLiveLoadInterval() const;

   // returns the index of the interval when the overlay is added to the bridge
   IntervalIndexType GetOverlayInterval() const;

   // returns index of interval when the railing system is installed
   IntervalIndexType GetInstallRailingSystemInterval() const;

   // returns the interval index when a temporary support is removed
   IntervalIndexType GetTemporarySupportRemovalInterval(SupportIDType tsID) const;

protected:
   struct CInterval
   {
      EventIndexType      StartEventIdx; // Event related to the start of this interval
      EventIndexType      EndEventIdx;   // Event related to the end of this interval
      Float64        Start;         // Start of interval
      Float64        Middle;        // Middle of interval
      Float64        End;           // End of interval
      Float64        Duration;      // Interval duration
      std::_tstring  Description;   // Description of activity occuring during this interval
   };

   std::vector<CInterval> m_Intervals;

   std::map<SupportIDType,IntervalIndexType> m_TempSupportRemovalIntervals;

   IntervalIndexType m_StressStrandInterval;
   IntervalIndexType m_ReleaseInterval; // interval when prestress is released (segments have reached release strength)
   IntervalIndexType m_FirstErectedInterval;
   IntervalIndexType m_CastDeckInterval;
   IntervalIndexType m_CompositeDeckInterval; // interval when deck is composite
   IntervalIndexType m_LiveLoadInterval; // interval when live load is applied to the structure
   IntervalIndexType m_OverlayInterval;
   IntervalIndexType m_RailingSystemInterval;

#if defined _DEBUG
   void AssertValid() const;
#endif
};