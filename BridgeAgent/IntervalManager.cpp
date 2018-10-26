///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

void CIntervalManager::BuildIntervals(const CTimelineManager* pTimelineMgr,bool bTimeStepMethod)
{
   m_StressStrandInterval  = INVALID_INDEX;
   m_ReleaseInterval       = INVALID_INDEX;
   m_FirstErectedInterval  = INVALID_INDEX;
   m_CastDeckInterval      = INVALID_INDEX;
   m_CompositeDeckInterval = INVALID_INDEX;
   m_LiveLoadInterval      = INVALID_INDEX;
   m_OverlayInterval       = INVALID_INDEX;
   m_RailingSystemInterval = INVALID_INDEX;

#pragma Reminder("UPDATE: need zero duration interval for installation of temporary strands if they are post-tensioned")
   // also need generic TemporaryStrandInstallationInterval method

   m_Intervals.clear();
   m_TempSupportRemovalIntervals.clear();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      if ( pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
      {
         // Model this with 3 intervals... stress strands, release prestress, place in storage
         Float64 start = pTimelineEvent->GetDay();
         Float64 duration = pTimelineEvent->GetConstructSegmentsActivity().GetRelaxationTime();
         CInterval i;

         // stress strands during segment construction. strands relax during this interval
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start = start;
         i.Duration = duration;
         i.End = start + duration;
         i.Middle = 0.5*(i.Start + i.End);
         i.Description = _T("Tension strand and cast girder segment");
         m_Intervals.push_back(i);
         m_StressStrandInterval = m_Intervals.size()-1;

         // release prestress is a sudden loading.... zero length interval
         i.Start = i.End;
         i.Duration = 0;
         i.Middle = i.Start;
         i.Description = _T("Release prestress");
         m_Intervals.push_back(i);
         m_ReleaseInterval = m_Intervals.size()-1;

         // lift segment
         i.Start = i.End;
         i.Duration = 0;
         i.Middle = i.Start;
         i.Description = _T("Lift segments");
         m_Intervals.push_back(i);

         // placing into storage changes boundary conditions... treat as sudden change in loading
         i.Start = i.End;
         i.Duration = 0;
         i.Middle = i.Start;
         i.Description = _T("Place segments into storage");
         m_Intervals.push_back(i);
      }

      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
      {
         CInterval i;

         // Haul segment to bridge site
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;
         i.Description   = _T("Haul segments");
         m_Intervals.push_back(i);

         // this is an abrupt change in boundary condition, treat as sudden change in loading
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;
         i.Description   = _T("Erect segments");
         m_Intervals.push_back(i);
         if ( m_FirstErectedInterval == INVALID_INDEX )
         {
            m_FirstErectedInterval = m_Intervals.size() - 1;
         }

         // remove temporary strands
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;
         i.Description   = _T("Remove temporary strands");
         m_Intervals.push_back(i);
      }

      if ( pTimelineEvent->GetCastClosurePourActivity().IsEnabled() && pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         CInterval i;

         // this is an abrupt change in boundary condition, treat as sudden change in loading
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;
         i.Description   = _T("Cast closure pours and deck");
         m_Intervals.push_back(i);
         m_CastDeckInterval = m_Intervals.size()-1;

         if ( bTimeStepMethod)
         {
            // deck is cured (can take load) 2 intervals after the deck is cast
            // casting interval idx = m_Intervals.size()-1
            // curing interval = 1
            m_CompositeDeckInterval = m_Intervals.size()+1;

            i.Start = i.End;
            i.Duration = max(pTimelineEvent->GetCastClosurePourActivity().GetConcreteAgeAtContinuity(),pTimelineEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity());
            i.End = i.Start + i.Duration;
            i.Middle = 0.5*(i.Start + i.End);
            i.Description = _T("Time Step - Deck Curing");
            if ( 0 < i.Duration )
            {
               m_Intervals.push_back(i);
            }
         }
         else
         {
            // for a non-time step method, the deck is composite in
            // the interval after it is cast
            m_CompositeDeckInterval = m_CastDeckInterval+1;
         }
      }
      else
      {
         if ( pTimelineEvent->GetCastClosurePourActivity().IsEnabled() )
         {
            CInterval i;

            // this is an abrupt change in boundary condition, treat as sudden change in loading
            i.StartEventIdx = eventIdx;
            i.EndEventIdx   = eventIdx;
            i.Start         = pTimelineEvent->GetDay();
            i.End           = i.Start;
            i.Middle        = i.Start;
            i.Duration      = 0;
            i.Description   = _T("Cast closure pour");
            m_Intervals.push_back(i);

            // time step to time when concrete reaches continuity age
            if ( bTimeStepMethod )
            {
               i.Start = i.End;
               i.Duration = pTimelineEvent->GetCastClosurePourActivity().GetConcreteAgeAtContinuity();
               i.End = i.Start + i.Duration;
               i.Middle = 0.5*(i.Start + i.End);
               i.Description = _T("Time Step");
               if ( 0 < i.Duration )
               {
                  m_Intervals.push_back(i);
               }
            }
         }

         if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
         {
            CInterval i;

            // this is an abrupt change in boundary condition, treat as sudden change in loading
            i.StartEventIdx = eventIdx;
            i.EndEventIdx   = eventIdx;
            i.Start         = pTimelineEvent->GetDay();
            i.End           = i.Start;
            i.Middle        = i.Start;
            i.Duration      = 0;
            i.Description   = _T("Cast Deck");
            m_Intervals.push_back(i);
            m_CastDeckInterval = m_Intervals.size()-1;

            if ( bTimeStepMethod )
            {
               // deck is cured (can take load) 2 intervals after the deck is cast
               // casting interval idx = m_Intervals.size()-1
               // curing interval = 1
               // interval after curing = 1
               // interval in m_Intervals.size()-1 + 1 + 1 = m_Intervals.size()+1
               m_CompositeDeckInterval = m_Intervals.size()+1;

               // time step to time when concrete reaches continuity age
               i.Start = i.End;
               i.Duration = pTimelineEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity();
               i.End = i.Start + i.Duration;
               i.Middle = 0.5*(i.Start + i.End);
               i.Description = _T("Time Step - Deck Curing");
               if ( 0 < i.Duration )
               {
                  m_Intervals.push_back(i);
               }
            }
            else
            {
               // for a non-time step method, the deck is composite in
               // the interval after it is cast
               m_CompositeDeckInterval = m_CastDeckInterval+1;
            }
         }
      }

      if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() && pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
      {
         CInterval i;

         // this is an abrupt change in boundary condition, treat as sudden change in loading
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;
         i.Description   = _T("Stress tendon and remove temporary supports");
         m_Intervals.push_back(i);

         const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
         std::vector<SupportIDType> tsIDs(removeTS.GetTempSupports());
         IntervalIndexType intervalIdx = m_Intervals.size()-1;
         std::vector<SupportIDType>::iterator iter(tsIDs.begin());
         std::vector<SupportIDType>::iterator end(tsIDs.end());
         for ( ; iter != end; iter++ )
         {
            SupportIDType tsID(*iter);
            m_TempSupportRemovalIntervals.insert(std::make_pair(tsID,intervalIdx));
         }
      }
      else
      {
         if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() )
         {
            CInterval i;

            // this is an abrupt change in boundary condition, treat as sudden change in loading
            i.StartEventIdx = eventIdx;
            i.EndEventIdx   = eventIdx;
            i.Start         = pTimelineEvent->GetDay();
            i.End           = i.Start;
            i.Middle        = i.Start;
            i.Duration      = 0;
            i.Description   = _T("Stress tendon");
            m_Intervals.push_back(i);
         }

         if ( pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
         {
            CInterval i;

            // this is an abrupt change in boundary condition, treat as sudden change in loading
            i.StartEventIdx = eventIdx;
            i.EndEventIdx   = eventIdx;
            i.Start         = pTimelineEvent->GetDay();
            i.End           = i.Start;
            i.Middle        = i.Start;
            i.Duration      = 0;
            i.Description   = _T("Remove temporary supports");
            m_Intervals.push_back(i);

            const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
            std::vector<SupportIDType> tsIDs(removeTS.GetTempSupports());
            IntervalIndexType intervalIdx = m_Intervals.size()-1;
            std::vector<SupportIDType>::iterator iter(tsIDs.begin());
            std::vector<SupportIDType>::iterator end(tsIDs.end());
            for ( ; iter != end; iter++ )
            {
               SupportIDType tsID(*iter);
               m_TempSupportRemovalIntervals.insert(std::make_pair(tsID,intervalIdx));
            }
         }
      }

      if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() )
      {
         CInterval i;

         // this is an abrupt change in loading
         i.StartEventIdx = eventIdx;
         i.EndEventIdx   = eventIdx;
         i.Start         = pTimelineEvent->GetDay();
         i.End           = i.Start;
         i.Middle        = i.Start;
         i.Duration      = 0;

         bool bRailing  = pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied();
         bool bOverlay  = pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied();
         bool bLiveLoad = pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied();
         bool bUserLoad = pTimelineEvent->GetApplyLoadActivity().GetUserLoadCount() != 0;

         if ( bUserLoad && !bRailing && !bOverlay && !bLiveLoad )
         {
            i.Description = _T("Apply User Defined Load");
            m_Intervals.push_back(i);
         }

         if ( bRailing )
         {
            if ( bOverlay )
            {
               // overlay and railing system applied at the same time (not a future overlay)
               i.Description = _T("Install Railing System and Overlay");
               m_Intervals.push_back(i);
               m_OverlayInterval = m_Intervals.size()-1;
               m_RailingSystemInterval = m_OverlayInterval;
               bOverlay = false; // don't want to apply overlay again below
            }
            else
            {
               // railing system applied by itself
               i.Description   = _T("Install Railing System");
               m_Intervals.push_back(i);
               m_RailingSystemInterval = m_Intervals.size()-1;
            }
         }

         if ( bOverlay )
         {
            if ( bLiveLoad )
            {
               // not a future overlay
               i.Description = _T("Open to Traffic");
               m_Intervals.push_back(i);
               m_OverlayInterval  = m_Intervals.size()-1;
               m_LiveLoadInterval = m_OverlayInterval;
               bLiveLoad = false; // don't want to apply live load again below
            }
            else
            {
               // overlay is applied in its own interval
               i.Description   = _T("Install Overlay");
               m_Intervals.push_back(i);
               m_OverlayInterval = m_Intervals.size()-1;
            }
         }

         if ( bLiveLoad )
         {
            // live load is applied in its own interval
            i.Description   = _T("Open to Traffic");
            m_Intervals.push_back(i);
            m_LiveLoadInterval = m_Intervals.size() - 1;
         }

#pragma Reminder("UPATE: Need to add loading interval for user defined external loads")
      }

      // At the end of every event, create a general time step
      // that goes from the end of last iterval for the current event
      // to the start of the next event
      if ( bTimeStepMethod )
      {
         CInterval time_step;
         time_step.StartEventIdx = eventIdx;
         time_step.Start = m_Intervals.back().End; // starts at the end of the previous interval

         time_step.EndEventIdx = eventIdx;
         if ( eventIdx < nEvents-1 )
         {
            time_step.EndEventIdx++; // this time step ends at the start of the next event unless this is the last event
         }

         time_step.End   = pTimelineMgr->GetStart(time_step.EndEventIdx); // ends at the start of the next event

         time_step.Duration = time_step.End - time_step.Start;
         time_step.Middle = 0.5*(time_step.Start + time_step.End);
         if ( eventIdx < nEvents-1 )
         {
            time_step.Description = _T("Time Step");
            if ( 0.0 < time_step.Duration )
            {
               m_Intervals.push_back(time_step);
            }
         }
         else
         {
            time_step.Description = _T("Final Time Step");
            m_Intervals.push_back(time_step);
         }
      }
      else
      {
         // not doing time step, so end this event with the start of the next event
         // this provides continuity in the time intervals
         if ( eventIdx < nEvents-1 )
            m_Intervals.back().End = pTimelineMgr->GetStart(eventIdx+1);
         else
            m_Intervals.back().End = m_Intervals.back().Start;
      }
   }

   if ( m_OverlayInterval == INVALID_INDEX )
      m_OverlayInterval = m_RailingSystemInterval;

#if defined _DEBUG
   AssertValid();
#endif
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

Float64 CIntervalManager::GetStart(IntervalIndexType idx) const
{
   return GetTime(idx,pgsTypes::Start);
}

Float64 CIntervalManager::GetMiddle(IntervalIndexType idx) const
{
   return GetTime(idx,pgsTypes::Middle);
}

Float64 CIntervalManager::GetEnd(IntervalIndexType idx) const
{
   return GetTime(idx,pgsTypes::End);
}

Float64 CIntervalManager::GetDuration(IntervalIndexType idx) const
{
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
         return i;
   }

   // if this is the last event, the return the last interval
   if ( m_Intervals.back().EndEventIdx == eventIdx )
   {
      return m_Intervals.size()-1;
   }

   ATLASSERT(false); // event not found... can't determine interval
   return INVALID_INDEX;
}

IntervalIndexType CIntervalManager::GetCastDeckInterval() const
{
   ATLASSERT(m_CastDeckInterval != INVALID_INDEX);
   return m_CastDeckInterval;
}

IntervalIndexType CIntervalManager::GetCompositeDeckInterval() const
{
   ATLASSERT(m_CompositeDeckInterval != INVALID_INDEX);
   return m_CompositeDeckInterval;
}

IntervalIndexType CIntervalManager::GetStressStrandInterval() const
{
   ATLASSERT(m_StressStrandInterval != INVALID_INDEX);
   return m_StressStrandInterval;
}

IntervalIndexType CIntervalManager::GetPrestressReleaseInterval() const
{
   ATLASSERT(m_ReleaseInterval != INVALID_INDEX);
   return m_ReleaseInterval;
}

IntervalIndexType CIntervalManager::GetLiftingInterval() const
{
   return GetPrestressReleaseInterval() + 1;
}

IntervalIndexType CIntervalManager::GetStorageInterval() const
{
   return GetPrestressReleaseInterval() + 2;
}

IntervalIndexType CIntervalManager::GetHaulingInterval() const
{
   return GetFirstErectedSegmentInterval() - 1;
}

IntervalIndexType CIntervalManager::GetTemporaryStrandRemovalInterval() const
{
   return GetFirstErectedSegmentInterval() + 1;
}

IntervalIndexType CIntervalManager::GetFirstErectedSegmentInterval() const
{
   ATLASSERT(m_FirstErectedInterval != INVALID_INDEX);
   return m_FirstErectedInterval;
}

IntervalIndexType CIntervalManager::GetLiveLoadInterval() const
{
   ATLASSERT(m_LiveLoadInterval != INVALID_INDEX);
   return m_LiveLoadInterval;
}

IntervalIndexType CIntervalManager::GetOverlayInterval() const
{
   ATLASSERT(m_OverlayInterval != INVALID_INDEX);
   return m_OverlayInterval; 
}

IntervalIndexType CIntervalManager::GetInstallRailingSystemInterval() const
{
   ATLASSERT(m_RailingSystemInterval != INVALID_INDEX);
   return m_RailingSystemInterval;
}

IntervalIndexType CIntervalManager::GetTemporarySupportRemovalInterval(SupportIDType tsID) const
{
   std::map<SupportIDType,IntervalIndexType>::const_iterator found(m_TempSupportRemovalIntervals.find(tsID) );
   if ( found == m_TempSupportRemovalIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

#if defined _DEBUG
void CIntervalManager::AssertValid() const
{
   IntervalIndexType nIntervals = m_Intervals.size();
   for ( IntervalIndexType i = 1; i < nIntervals; i++ )
   {
      // This interval must start when the previous one ends
      ATLASSERT(m_Intervals[i].Start == m_Intervals[i-1].End);
   }

   ATLASSERT(m_FirstErectedInterval < m_CastDeckInterval);
   ATLASSERT(m_CastDeckInterval < m_CompositeDeckInterval);
   ATLASSERT(m_CompositeDeckInterval <= m_OverlayInterval);
   ATLASSERT(m_CompositeDeckInterval <= m_RailingSystemInterval);
   ATLASSERT(m_CompositeDeckInterval <= m_LiveLoadInterval);
   ATLASSERT(m_RailingSystemInterval <= m_LiveLoadInterval);
}
#endif
