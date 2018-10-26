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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\TemporarySupportData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// predicate function for sorting timeline events by the less than operator
bool CompareEvents(CTimelineEvent* pTimelineEvent1,CTimelineEvent* pTimelineEvent2)
{
   return *pTimelineEvent1 < *pTimelineEvent2;
}

class FindTimelineEventByDay
{
public:
   FindTimelineEventByDay(Float64 day) { m_Day = day; }
   bool operator()(const CTimelineEvent* pTimelineEvent) { return IsEqual(m_Day,pTimelineEvent->GetDay()); }
private:
   Float64 m_Day;
};

// initialize the ID counter
EventIDType CTimelineManager::ms_ID = 0;

CTimelineManager::CTimelineManager()
{
}

CTimelineManager::CTimelineManager(const CTimelineManager& rOther)
{
   MakeCopy(rOther);
}

CTimelineManager::~CTimelineManager()
{
   Clear();
}

CTimelineManager& CTimelineManager::operator= (const CTimelineManager& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CTimelineManager::operator==(const CTimelineManager& rOther) const
{
   ASSERT_VALID;

   if ( m_TimelineEvents.size() != rOther.m_TimelineEvents.size() )
   {
      return false;
   }

   std::vector<CTimelineEvent*>::const_iterator myIter(    m_TimelineEvents.begin()        );
   std::vector<CTimelineEvent*>::const_iterator myEnd(     m_TimelineEvents.end()          );
   std::vector<CTimelineEvent*>::const_iterator otherIter( rOther.m_TimelineEvents.begin() );
   std::vector<CTimelineEvent*>::const_iterator otherEnd(  rOther.m_TimelineEvents.end()   );
   for ( ; myIter != myEnd; myIter++, otherIter++ )
   {
      const CTimelineEvent* pMyEvent = *myIter;
      const CTimelineEvent* pOtherEvent = *otherIter;

      if ( *pMyEvent != *pOtherEvent )
      {
         return false;
      }
   }

   return true;
}

bool CTimelineManager::operator!=(const CTimelineManager& rOther) const
{
   return !operator==(rOther);
}

void CTimelineManager::SetBridgeDescription(const CBridgeDescription2* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription2* CTimelineManager::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

int CTimelineManager::AddTimelineEvent(CTimelineEvent* pTimelineEvent,bool bAdjustTimeline,EventIndexType* pEventIdx)
{
   int result = TLM_SUCCESS;
   if ( !bAdjustTimeline )
   {
      result = ValidateEvent(pTimelineEvent);
   }

   if ( result != TLM_SUCCESS )
   {
      *pEventIdx = INVALID_INDEX;
      return result;
   }

   if ( pTimelineEvent->GetID() == INVALID_ID )
   {
      pTimelineEvent->SetID(ms_ID++);
   }

   pTimelineEvent->SetTimelineManager(this);

   // Move activities if necessary
   if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() )
   {
      if ( pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetLiveLoadEventIndex());
         if ( pOtherTimelineEvent )
         {
            pOtherTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad(false);
         }
      }

      if ( pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetOverlayLoadEventIndex());
         if ( pOtherTimelineEvent )
         {
            pOtherTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad(false);
         }
      }

      if ( pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetRailingSystemLoadEventIndex());
         if ( pOtherTimelineEvent )
         {
            pOtherTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad(false);
         }
      }

      IndexType nUserLoads = pTimelineEvent->GetApplyLoadActivity().GetUserLoadCount();
      for ( IndexType idx = 0; idx < nUserLoads; idx++ )
      {
         LoadIDType loadID = pTimelineEvent->GetApplyLoadActivity().GetUserLoadID(idx);
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetApplyLoadActivity().HasUserLoad(loadID) )
            {
               pOtherTimelineEvent->GetApplyLoadActivity().RemoveUserLoad(loadID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() )
   {
      std::set<PierIDType> pierIDs = pTimelineEvent->GetCastClosureJointActivity().GetPiers();
      std::set<PierIDType>::iterator pierIDIter(pierIDs.begin());
      std::set<PierIDType>::iterator pierIDIterEnd(pierIDs.end());
      for ( ; pierIDIter != pierIDIterEnd; pierIDIter++ )
      {
         PierIDType pierID = *pierIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetCastClosureJointActivity().HasPier(pierID) )
            {
               pOtherTimelineEvent->GetCastClosureJointActivity().RemovePier(pierID);
            }
         }
      }

      std::set<SupportIDType> tsIDs = pTimelineEvent->GetCastClosureJointActivity().GetTempSupports();
      std::set<SupportIDType>::iterator tsIDIter(tsIDs.begin());
      std::set<SupportIDType>::iterator tsIDIterEnd(tsIDs.end());
      for ( ; tsIDIter != tsIDIterEnd; tsIDIter++ )
      {
         SupportIDType tsID = *tsIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetCastClosureJointActivity().HasTempSupport(tsID) )
            {
               pOtherTimelineEvent->GetCastClosureJointActivity().RemoveTempSupport(tsID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
   {
      CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetCastDeckEventIndex());
      if ( pOtherTimelineEvent )
      {
         pOtherTimelineEvent->GetCastDeckActivity().Enable(false);
      }
   }


   if ( pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
   {
      std::set<SegmentIDType> segmentIDs = pTimelineEvent->GetConstructSegmentsActivity().GetSegments();
      std::set<SegmentIDType>::iterator segmentIDIter(segmentIDs.begin());
      std::set<SegmentIDType>::iterator segmentIDIterEnd(segmentIDs.end());
      for ( ; segmentIDIter != segmentIDIterEnd; segmentIDIter++ )
      {
         SegmentIDType segmentID = *segmentIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
            {
               pOtherTimelineEvent->GetConstructSegmentsActivity().RemoveSegment(segmentID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetErectPiersActivity().IsEnabled() )
   {
      std::set<PierIDType> pierIDs = pTimelineEvent->GetErectPiersActivity().GetPiers();
      std::set<PierIDType>::iterator pierIDIter(pierIDs.begin());
      std::set<PierIDType>::iterator pierIDIterEnd(pierIDs.end());
      for ( ; pierIDIter != pierIDIterEnd; pierIDIter++ )
      {
         PierIDType pierID = *pierIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
            {
               pOtherTimelineEvent->GetErectPiersActivity().RemovePier(pierID);
            }
         }
      }

      std::set<SupportIDType> tsIDs = pTimelineEvent->GetErectPiersActivity().GetTempSupports();
      std::set<SupportIDType>::iterator tsIDIter(tsIDs.begin());
      std::set<SupportIDType>::iterator tsIDIterEnd(tsIDs.end());
      for ( ; tsIDIter != tsIDIterEnd; tsIDIter++ )
      {
         SupportIDType tsID = *tsIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
            {
               pOtherTimelineEvent->GetErectPiersActivity().RemoveTempSupport(tsID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
   {
      std::set<SegmentIDType> segmentIDs = pTimelineEvent->GetErectSegmentsActivity().GetSegments();
      std::set<SegmentIDType>::iterator segmentIDIter(segmentIDs.begin());
      std::set<SegmentIDType>::iterator segmentIDIterEnd(segmentIDs.end());
      for ( ; segmentIDIter != segmentIDIterEnd; segmentIDIter++ )
      {
         SegmentIDType segmentID = *segmentIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
            {
               pOtherTimelineEvent->GetErectSegmentsActivity().RemoveSegment(segmentID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
   {
      std::vector<SupportIDType> tsIDs = pTimelineEvent->GetRemoveTempSupportsActivity().GetTempSupports();
      std::vector<SupportIDType>::iterator tsIDIter(tsIDs.begin());
      std::vector<SupportIDType>::iterator tsIDIterEnd(tsIDs.end());
      for ( ; tsIDIter != tsIDIterEnd; tsIDIter++ )
      {
         SupportIDType tsID = *tsIDIter;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
            {
               pOtherTimelineEvent->GetRemoveTempSupportsActivity().RemoveTempSupport(tsID);
            }
         }
      }
   }

   if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() )
   {
      std::set<CTendonKey> vTendons = pTimelineEvent->GetStressTendonActivity().GetTendons();
      std::set<CTendonKey>::iterator iter(vTendons.begin());
      std::set<CTendonKey>::iterator end(vTendons.end());
      for ( ; iter != end; iter++ )
      {
         CTendonKey& key = *iter;
         GirderIDType gdrID = key.girderID;
         DuctIndexType ductIdx = key.ductIdx;
         EventIndexType nEvents = GetEventCount();
         for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(eventIdx);
            if ( pOtherTimelineEvent && pOtherTimelineEvent->GetStressTendonActivity().IsTendonStressed(gdrID,ductIdx) )
            {
               pOtherTimelineEvent->GetStressTendonActivity().RemoveTendon(gdrID,ductIdx,false);
            }
         }
      }
   }

   // Put the timeline event in the collection
   m_TimelineEvents.push_back(pTimelineEvent);
   Sort(); // sort

   std::vector<CTimelineEvent*>::iterator found = std::find(m_TimelineEvents.begin(),m_TimelineEvents.end(),pTimelineEvent);
   ATLASSERT(found != m_TimelineEvents.end());

   EventIndexType eventIdx = found - m_TimelineEvents.begin();

   ASSERT_VALID;

   *pEventIdx = eventIdx;

   return result;
}

int CTimelineManager::AddTimelineEvent(const CTimelineEvent& timelineEvent,bool bAdjustTimeline,EventIndexType* pEventIdx)
{
   CTimelineEvent* pTimelineEvent = new CTimelineEvent(timelineEvent);
   return AddTimelineEvent(pTimelineEvent,bAdjustTimeline,pEventIdx);
}

int CTimelineManager::RemoveEventByIndex(EventIndexType eventIdx)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );

   CTimelineEvent* pTimelineEvent = m_TimelineEvents[eventIdx];
   int result = CanRemoveEvent(pTimelineEvent);
   if ( result == TLM_SUCCESS )
   {
      delete pTimelineEvent;
      m_TimelineEvents.erase(m_TimelineEvents.begin() + eventIdx);
   }

   ASSERT_VALID;

   return result;
}

int CTimelineManager::RemoveEventByID(EventIDType id)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return RemoveEventByIndex(iter - m_TimelineEvents.begin());
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

int CTimelineManager::SetEventByIndex(EventIndexType eventIdx,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );
   if ( !bAdjustTimeline )
   {
      // not automatically adjusting the timeline so validate the new event data

      // remove the event that is going to be replaced so the new event data wont conflict with it when it is evaluated
      CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];
      m_TimelineEvents.erase(m_TimelineEvents.begin()+eventIdx);
      int result = ValidateEvent(pTimelineEvent);

      // return the old event data to the collection
      m_TimelineEvents.insert(m_TimelineEvents.begin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         return result;
      }
   }

   CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];
   pTimelineEvent->SetID(pOldEvent->GetID());
   m_TimelineEvents[eventIdx] = pTimelineEvent;
   Sort();
   delete pOldEvent;
   pOldEvent = NULL;

   ASSERT_VALID;

   return TLM_SUCCESS;
}

int CTimelineManager::SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& timelineEvent,bool bAdjustTimeline)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );

   if ( !bAdjustTimeline )
   {
      // not automatically adjusting the timeline so validate the new event data

      // remove the event that is going to be replaced so the new event data wont conflict with it when it is evaluated
      CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];
      m_TimelineEvents.erase(m_TimelineEvents.begin()+eventIdx);

      int result = ValidateEvent(&timelineEvent);

      // return the old event data to the collection
      m_TimelineEvents.insert(m_TimelineEvents.begin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         return result;
      }
   }

   *m_TimelineEvents[eventIdx] = timelineEvent;
   Sort();

   ASSERT_VALID;

   return TLM_SUCCESS;
}

int CTimelineManager::SetEventByID(EventIDType id,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return SetEventByIndex(iter-m_TimelineEvents.begin(),pTimelineEvent,bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

int CTimelineManager::SetEventByID(EventIDType id,const CTimelineEvent& timelineEvent,bool bAdjustTimeline)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return SetEventByIndex(iter-m_TimelineEvents.begin(),timelineEvent,bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

int CTimelineManager::AdjustDayByIndex(EventIndexType eventIdx,Float64 day,bool bAdjustTimeline)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );

   if ( !bAdjustTimeline )
   {
      // not automatically adjusting the timeline so validate the new event data

      // remove the event that is going to be replaced so the new event data wont conflict with it when it is evaluated
      CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];
      m_TimelineEvents.erase(m_TimelineEvents.begin()+eventIdx);

      Float64 oldDay = pOldEvent->GetDay();
      pOldEvent->SetDay(day);

      int result = ValidateEvent(pOldEvent);

      // return the old event data to the collection
      pOldEvent->SetDay(oldDay);
      m_TimelineEvents.insert(m_TimelineEvents.begin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         return result;
      }
   }

   m_TimelineEvents[eventIdx]->SetDay(day);
   Sort();

   ASSERT_VALID;

   return TLM_SUCCESS;
}

int CTimelineManager::AdjustDayByID(EventIDType id,Float64 day,bool bAdjustTimeline)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return AdjustDayByID(iter-m_TimelineEvents.begin(),day,bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

void CTimelineManager::SetElapsedTime(EventIndexType eventIdx,Float64 elapsedTime)
{
   if ( eventIdx == m_TimelineEvents.size()-1 )
   {
      return; // eventIdx is the last event so the elapsed time to the next event doesn't do anything
   }

   CTimelineEvent* pThisTimelineEvent = GetEventByIndex(eventIdx);
   CTimelineEvent* pNextTimelineEvent = GetEventByIndex(eventIdx+1);

   Float64 elapsed_time = pNextTimelineEvent->GetDay() - pThisTimelineEvent->GetDay();
   Float64 delta_elapsed_time = elapsedTime - elapsed_time;

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin()+eventIdx+1);
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      // NOTE: don't call SetDay(). SetDay() causes the timeline events to be sorted
      // the purpose of this method is to just adjust the elasped time between events
      // keeping the same event order
      pTimelineEvent->m_Day = pTimelineEvent->m_Day + delta_elapsed_time;
   }
}

EventIndexType CTimelineManager::GetEventCount() const
{
   return m_TimelineEvents.size();
}

bool CTimelineManager::FindEvent(LPCTSTR description,EventIndexType* pIndex,const CTimelineEvent** ppTimelineEvent) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( wcscmp(pTimelineEvent->GetDescription(),description) == 0 )
      {
         *pIndex = (iter - m_TimelineEvents.begin());
         *ppTimelineEvent = pTimelineEvent;
         return true;
      }
   }

   *pIndex = INVALID_INDEX;
   *ppTimelineEvent = NULL;
   return false;
}

EventIndexType CTimelineManager::GetEventIndex(EventIDType ID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         return (iter - m_TimelineEvents.begin());
      }
   }

   ATLASSERT(false); // ID not found (this may be ok)
   return INVALID_INDEX;
}

bool CTimelineManager::HasEvent(Float64 day) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator found( std::find_if(m_TimelineEvents.begin(),m_TimelineEvents.end(),FindTimelineEventByDay(day)) );
   return ( found != m_TimelineEvents.end() ? true : false);
}

Float64 CTimelineManager::GetStart(EventIndexType eventIdx) const
{
   ASSERT_VALID;

   return m_TimelineEvents[eventIdx]->GetDay();
}

Float64 CTimelineManager::GetEnd(EventIndexType eventIdx) const
{
   ASSERT_VALID;

   if ( eventIdx == m_TimelineEvents.size()-1 )
   {
      return m_TimelineEvents[eventIdx]->GetDay(); // end of the last event is the same as the start day (last event has a duration of 0)
   }
   else
   {
      return m_TimelineEvents[eventIdx+1]->GetDay();
   }
}

Float64 CTimelineManager::GetDuration(EventIndexType eventIdx) const
{
   ASSERT_VALID;

   return GetEnd(eventIdx) - GetStart(eventIdx);
}

bool CTimelineManager::AreAllPiersErected() const
{
   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      PierIDType pierID = m_pBridgeDesc->GetPier(pierIdx)->GetID();
      if ( !IsPierErected(pierID) )
      {
         return false;
      }
   }

   return true;
}

bool CTimelineManager::AreAllTemporarySupportsErected() const
{
   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      SupportIDType tsID = m_pBridgeDesc->GetTemporarySupport(tsIdx)->GetID();
      if ( !IsTemporarySupportErected(tsID) )
      {
         return false;
      }
   }

   return true;
}

bool CTimelineManager::AreAllTemporarySupportsRemoved() const
{
   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      SupportIDType tsID = m_pBridgeDesc->GetTemporarySupport(tsIdx)->GetID();
      if ( !IsTemporarySupportRemoved(tsID) )
      {
         return false;
      }
   }

   return true;
}

bool CTimelineManager::AreAllSegmentsErected() const
{
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segID = pSegment->GetID();
            if ( !IsSegmentErected(segID) )
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool CTimelineManager::AreAllSegmentsErected(GirderIDType girderID,EventIndexType eventIdx) const
{
   ATLASSERT(girderID != INVALID_ID);

   const CSplicedGirderData* pGirder = m_pBridgeDesc->FindGirder(girderID);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      SegmentIDType segID = pSegment->GetID();
      if ( !IsSegmentErected(segID,eventIdx) )
      {
         return false;
      }
   }
   return true;
}

bool CTimelineManager::AreAllClosureJointsCast() const
{
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         IndexType nClosures = pGirder->GetClosureJointCount();
         for ( IndexType cjIdx = 0; cjIdx < nClosures; cjIdx++ )
         {
            const CClosureJointData* pClosure = pGirder->GetClosureJoint(cjIdx);
            IDType cjID = pClosure->GetID();
            if ( !IsClosureJointCast(cjID) )
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool CTimelineManager::AreAllTendonsStressed() const
{
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         const CPTData* pPTData = pGirder->GetPostTensioning();
         DuctIndexType nDucts = pPTData->GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            if ( !IsTendonStressed(pGirder->GetID(),ductIdx) )
            {
               return false;
            }
         }
      }
   }
   return true;
}

const CTimelineEvent* CTimelineManager::GetEventByIndex(EventIndexType eventIdx) const
{
   ASSERT_VALID;

   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );
   return m_TimelineEvents[eventIdx];
}

const CTimelineEvent* CTimelineManager::GetEventByID(IDType id) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return GetEventByIndex(iter - m_TimelineEvents.begin());
      }
   }

   return NULL;
}

CTimelineEvent* CTimelineManager::GetEventByIndex(EventIndexType eventIdx)
{
   ASSERT_VALID;

   if (0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() )
   {
      return m_TimelineEvents[eventIdx];
   }
   else
   {
      return NULL;
   }
}

CTimelineEvent* CTimelineManager::GetEventByID(IDType id)
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return GetEventByIndex(iter - m_TimelineEvents.begin());
      }
   }

   return NULL;
}

bool CTimelineManager::IsDeckCast() const
{
   return GetCastDeckEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsRailingSystemInstalled() const
{
   return GetRailingSystemLoadEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsSegmentConstructed(SegmentIDType segmentID) const
{
   return GetSegmentConstructionEventIndex(segmentID) != INVALID_INDEX;
}

bool CTimelineManager::IsClosureJointCast(ClosureIDType closureID) const
{
   return GetCastClosureJointEventIndex(closureID) != INVALID_INDEX;
}

bool CTimelineManager::IsPierErected(PierIDType pierID) const
{
   return GetPierErectionEventIndex(pierID) != INVALID_INDEX;
}

EventIndexType CTimelineManager::GetPierErectionEventIndex(PierIDType pierID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
      {
         return iter - m_TimelineEvents.begin();
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetPierErectionEventID(PierIDType pierID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetPierErectionEventByIndex(PierIDType pierID,EventIndexType eventIdx)
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
      {
         pTimelineEvent->GetErectPiersActivity().RemovePier(pierID);
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetErectPiersActivity().AddPier(pierID);
   }
}

void CTimelineManager::SetPierErectionEventByID(PierIDType pierID,EventIDType ID)
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetPierErectionEventByIndex(pierID,iter-m_TimelineEvents.begin());
         break;
      }
   }
}

bool CTimelineManager::IsTemporarySupportErected(SupportIDType tsID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsTemporarySupportRemoved(SupportIDType tsID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsTendonStressedByID(EventIDType ID) const
{
   ASSERT_VALID;

   const CTimelineEvent* pTimelineEvent = GetEventByID(ID);
   if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed() )
   {
      return true;
   }

   return false;
}

bool CTimelineManager::IsTendonStressedByIndex(EventIndexType eventIdx) const
{
   ASSERT_VALID;

   const CTimelineEvent* pTimelineEvent = GetEventByIndex(eventIdx);
   if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed() )
   {
      return true;
   }

   return false;
}

bool CTimelineManager::IsSegmentErected(SegmentIDType segmentID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() && pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsSegmentErected(SegmentIDType segmentID,EventIndexType eventIdx) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.begin() + eventIdx + 1);
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() && pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointAtPier(PierIDType pierID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pierID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointAtTempSupport(SupportIDType tsID) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointCast(EventIndexType eventIdx,ClosureIDType closureID) const
{
   ASSERT_VALID;

   const CClosureJointData* pClosure  = m_pBridgeDesc->FindClosureJoint(closureID);
   const CPierData2*            pPier = pClosure->GetPier();
   const CTemporarySupportData* pTS   = pClosure->GetTemporarySupport();

   PierIDType    pierID = (pPier ? pPier->GetID() : INVALID_ID);
   SupportIDType tsID   = (pTS   ? pTS->GetID()   : INVALID_ID);

   const CTimelineEvent* pTimelineEvent = GetEventByIndex(eventIdx);
   const CCastClosureJointActivity& activity = pTimelineEvent->GetCastClosureJointActivity();
   if ( activity.IsEnabled() && (activity.HasPier(pierID) || activity.HasTempSupport(tsID) ) )
   {
      return true;
   }

   return false;
}

bool CTimelineManager::IsTendonStressed(GirderIDType girderID,DuctIndexType ductIdx) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() && pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return true;
      }
   }

   return false;
}

void CTimelineManager::SetTempSupportEvents(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx)
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
      {
         // event has this temp support for erection, but it is not the event we want
         // remove temp support from this activity
         pTimelineEvent->GetErectPiersActivity().RemoveTempSupport(tsID);
      }

      if ( pTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
      {
         // event has this temp support for removal, but it is not the event we want
         // remove temp support from this activity
         pTimelineEvent->GetRemoveTempSupportsActivity().RemoveTempSupport(tsID);
      }
   }

   // At this point, the temp support is not assigned to any event for erection or removal
   // Assign it now.
   if ( erectIdx != INVALID_INDEX )
   {
      m_TimelineEvents[erectIdx]->GetErectPiersActivity().AddTempSupport(tsID);
   }

   if ( removeIdx != INVALID_INDEX )
   {
      m_TimelineEvents[removeIdx]->GetRemoveTempSupportsActivity().AddTempSupport(tsID);
   }
}

void CTimelineManager::GetTempSupportEvents(SupportIDType tsID,EventIndexType* pErectIdx,EventIndexType* pRemoveIdx) const
{
   ASSERT_VALID;

   *pErectIdx  = INVALID_INDEX;
   *pRemoveIdx = INVALID_INDEX;

   EventIndexType idx = 0;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
      {
         *pErectIdx = idx;
      }

      if ( pTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
      {
         *pRemoveIdx = idx;
      }

      if ( *pErectIdx != INVALID_INDEX && *pRemoveIdx != INVALID_INDEX )
      {
         break;
      }

      idx++;
   }
}

EventIndexType CTimelineManager::GetSegmentConstructionEventIndex(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   ASSERT_VALID;

   EventIndexType idx = 0;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         return idx;
      }

      idx++;
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetSegmentConstructionEventID(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetSegmentConstructionEventByIndex(SegmentIDType segmentID,EventIndexType eventIdx)
{
   ATLASSERT(segmentID != INVALID_ID);

   // if eventIdx == INVALID_INDEX, we are just removing the construction from the timeline

   bool bUpdateConstructionTiming = false;
   Float64 ageAtRelease = 1.0;
   Float64 relaxationTime = 1.0;

   // If this construction of this segment is currently being modeled, get the construction
   // timing information.
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         if ( iter - m_TimelineEvents.begin() == eventIdx )
         {
            // the construction event isn't really changing... do nothing and return
            return;
         }

         // construction of this segment is being modeled and it is moving to a different timeline event
         bUpdateConstructionTiming = true;
         ageAtRelease   = pTimelineEvent->GetConstructSegmentsActivity().GetAgeAtRelease();
         relaxationTime = pTimelineEvent->GetConstructSegmentsActivity().GetRelaxationTime();
         pTimelineEvent->GetConstructSegmentsActivity().RemoveSegment(segmentID); // remove segment from timeline
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetConstructSegmentsActivity().AddSegment(segmentID);
      if ( bUpdateConstructionTiming )
      {
         m_TimelineEvents[eventIdx]->GetConstructSegmentsActivity().SetAgeAtRelease(ageAtRelease);
         m_TimelineEvents[eventIdx]->GetConstructSegmentsActivity().SetRelaxationTime(relaxationTime);
      }
   }

   ASSERT_VALID;
}

void CTimelineManager::SetSegmentConstructionEventByID(SegmentIDType segmentID,EventIDType ID)
{
   ATLASSERT(segmentID != INVALID_ID);
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetSegmentConstructionEventByIndex(segmentID,iter - m_TimelineEvents.begin());
         break;
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetSegmentErectionEventIndex(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return iter - m_TimelineEvents.begin();
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetSegmentErectionEventID(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetSegmentErectionEventByIndex(SegmentIDType segmentID,EventIndexType eventIdx)
{
   // if eventIdx == INVALID_INDEX we are just moving the erection of this segment from the timeline

   ATLASSERT(segmentID != INVALID_ID);
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         pTimelineEvent->GetErectSegmentsActivity().RemoveSegment(segmentID);
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetErectSegmentsActivity().AddSegment(segmentID);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetSegmentErectionEventByID(SegmentIDType segmentID,EventIDType ID)
{
   ATLASSERT(segmentID != INVALID_ID);
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetSegmentErectionEventByIndex(segmentID,iter - m_TimelineEvents.begin());
         break;
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetFirstSegmentErectionEventIndex() const
{
   ASSERT_VALID;

   EventIndexType eventIdx = 0;
   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++, eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().GetSegments().size() != 0 )
      {
         return eventIdx;
      }
   }

   ATLASSERT(false); // there aren't any segments erected
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetFirstSegmentErectionEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().GetSegments().size() != 0 )
      {
         return pTimelineEvent->GetID();
      }
   }

   ATLASSERT(false); // there aren't any segments erected
   return INVALID_ID;
}

EventIndexType CTimelineManager::GetCastClosureJointEventIndex(ClosureIDType closureID) const
{
   ASSERT_VALID;

   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);

   if ( pClosure == NULL )
   {
      ATLASSERT(false); // there should be a closure
      return INVALID_INDEX;
   }

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pClosure->GetPier() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pClosure->GetPier()->GetID()))
      {
         return iter - m_TimelineEvents.begin();
      }

      if ( pClosure->GetTemporarySupport() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pClosure->GetTemporarySupport()->GetID()))
      {
         return iter - m_TimelineEvents.begin();
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetCastClosureJointEventID(ClosureIDType closureID) const
{
   ASSERT_VALID;

   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);

   if ( pClosure == NULL )
   {
      ATLASSERT(false); // there should be a closure
      return INVALID_ID;
   }

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pClosure->GetPier() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pClosure->GetPier()->GetID()))
      {
         return pTimelineEvent->GetID();
      }

      if ( pClosure->GetTemporarySupport() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pClosure->GetTemporarySupport()->GetID()))
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetCastClosureJointEventByIndex(ClosureIDType closureID,EventIndexType eventIdx)
{
   ASSERT_VALID;

   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);

   if ( pClosure == NULL )
   {
      ATLASSERT(false); // there should be a closure
      return;
   }

   Float64 ageAtContinuity = 7.0;
   bool bUpdateAge = false;

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pClosure->GetPier() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pClosure->GetPier()->GetID()) )
      {
         bUpdateAge = true;
         ageAtContinuity = pTimelineEvent->GetCastClosureJointActivity().GetConcreteAgeAtContinuity();
         pTimelineEvent->GetCastClosureJointActivity().RemovePier(pClosure->GetPier()->GetID());
         break;
      }

      if ( pClosure->GetTemporarySupport() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pClosure->GetTemporarySupport()->GetID()) )
      {
         bUpdateAge = true;
         ageAtContinuity = pTimelineEvent->GetCastClosureJointActivity().GetConcreteAgeAtContinuity();
         pTimelineEvent->GetCastClosureJointActivity().RemoveTempSupport(pClosure->GetTemporarySupport()->GetID());
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().Enable(true);

      if ( bUpdateAge )
      {
         m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().SetConcreteAgeAtContinuity(ageAtContinuity);
      }

      if ( pClosure->GetPier() )
      {
         m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().AddPier(pClosure->GetPier()->GetID());
      }
      else
      {
         m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().AddTempSupport(pClosure->GetTemporarySupport()->GetID());
      }
   }
}

void CTimelineManager::SetCastClosureJointEventByID(ClosureIDType closureID,EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         EventIndexType eventIdx = iter-m_TimelineEvents.begin();
         SetCastClosureJointEventByIndex(closureID,eventIdx);
         break;
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetStressTendonEventIndex(GirderIDType girderID,DuctIndexType ductIdx) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return iter - m_TimelineEvents.begin();
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetStressTendonEventID(GirderIDType girderID,DuctIndexType ductIdx) const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetStressTendonEventByIndex(GirderIDType girderID,DuctIndexType ductIdx,EventIndexType eventIdx)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         pTimelineEvent->GetStressTendonActivity().RemoveTendon(girderID,ductIdx,false);
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetStressTendonActivity().AddTendon(girderID,ductIdx);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetStressTendonEventByID(GirderIDType girderID,DuctIndexType ductIdx,EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         EventIndexType eventIdx = iter-m_TimelineEvents.begin();
         SetStressTendonEventByIndex(girderID,ductIdx,eventIdx);
         break;
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetCastDeckEventIndex() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         return iter - m_TimelineEvents.begin();
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetCastDeckEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

int CTimelineManager::SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline)
{
   bool bUpdateAge = false;
   Float64 age_at_continuity = 7.0;
   CTimelineEvent* pOldCastDeckEvent = NULL;

   // search for the event where the deck is cast
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         bUpdateAge = true;
         age_at_continuity = pTimelineEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity();
         pTimelineEvent->GetCastDeckActivity().Enable(false);
         pOldCastDeckEvent = pTimelineEvent; // hang onto the event in case the edit needs to be rolled back
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      CTimelineEvent cast_deck_event = *m_TimelineEvents[eventIdx];
      cast_deck_event.GetCastDeckActivity().Enable(true);

      if ( bUpdateAge )
      {
         cast_deck_event.GetCastDeckActivity().SetConcreteAgeAtContinuity(age_at_continuity);
      }

      int result = SetEventByIndex(eventIdx,cast_deck_event,bAdjustTimeline);
      if ( result != TLM_SUCCESS )
      {
         // there was a problem... roll back the edit
         pOldCastDeckEvent->GetCastDeckActivity().Enable(true);
         return result;
      }
   }

   return TLM_SUCCESS;
}

int CTimelineManager::SetCastDeckEventByID(EventIDType ID,bool bAdjustTimeline)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         return SetCastDeckEventByIndex(iter-m_TimelineEvents.begin(),bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

EventIndexType CTimelineManager::GetRailingSystemLoadEventIndex() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         return iter - m_TimelineEvents.begin();
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetRailingSystemLoadEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetRailingSystemLoadEventByIndex(EventIndexType eventIdx)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyRailingSystemLoad(true);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetRailingSystemLoadEventByID(EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetRailingSystemLoadEventByIndex(iter - m_TimelineEvents.begin());
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetOverlayLoadEventIndex() const
{
   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         return iter - m_TimelineEvents.begin();
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetOverlayLoadEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetOverlayLoadEventByIndex(EventIndexType eventIdx)
{
   // remove the overlay load from all events
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad(false);
   }

   // activate the overlay load in the specified event
   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyOverlayLoad(true);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetOverlayLoadEventByID(EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetOverlayLoadEventByIndex(iter - m_TimelineEvents.begin());
         break;
      }
   }

   ASSERT_VALID;
}

int CTimelineManager::RemoveOverlayLoadEvent()
{
   EventIndexType eventIdx = GetOverlayLoadEventIndex();
   if ( eventIdx == INVALID_INDEX )
   {
      return TLM_SUCCESS; // there isn't an overlay load event
   }

   CTimelineEvent* pOverlayEvent = GetEventByIndex(eventIdx);
   int result = CanRemoveEvent(pOverlayEvent);
   if ( result == TLM_SUCCESS )
   {
      SetOverlayLoadEventByIndex(INVALID_INDEX);
   }

   ASSERT_VALID;

   return result;
}

EventIndexType CTimelineManager::GetLiveLoadEventIndex() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         return iter - m_TimelineEvents.begin();
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetLiveLoadEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetLiveLoadEventByIndex(EventIndexType eventIdx)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyLiveLoad(true);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetLiveLoadEventByID(EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetLiveLoadEventByIndex(iter - m_TimelineEvents.begin());
         break;
      }
   }

   ASSERT_VALID;
}

EventIndexType CTimelineManager::GetLoadRatingEventIndex() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRatingLiveLoadApplied() )
      {
         return iter - m_TimelineEvents.begin();
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetLoadRatingEventID() const
{
   ASSERT_VALID;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRatingLiveLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetLoadRatingEventByIndex(EventIndexType eventIdx)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->GetApplyLoadActivity().ApplyRatingLiveLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyRatingLiveLoad(true);
   }

   ASSERT_VALID;
}

void CTimelineManager::SetLoadRatingEventByID(EventIDType ID)
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetLoadRatingEventByIndex(iter - m_TimelineEvents.begin());
         break;
      }
   }

   ASSERT_VALID;
}

int CTimelineManager::Validate() const
{
   if ( m_pBridgeDesc == NULL )
   {
      return TLM_SUCCESS;
   }

   // Make sure the deck is cast
   if ( m_pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone && !IsDeckCast() )
   {
      return TLM_CAST_DECK_ACTIVITY_REQUIRED;
   }

   // Make sure the railing system is installed
   if ( !IsRailingSystemInstalled() )
   {
      return TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED;
   }

   // Make sure railing system is installed after the deck
   if ( GetRailingSystemLoadEventIndex() <= GetCastDeckEventIndex() )
   {
      return TLM_RAILING_SYSTEM_ERROR;
   }

   // Make sure load rating doesn't occur before bridge is open to traffic
   if ( GetLoadRatingEventIndex() < GetLiveLoadEventIndex() )
   {
      return TLM_LOAD_RATING_ERROR;
   }

   // Make sure all piers are erected
   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      if ( !IsPierErected(pierID) )
      {
         return TLM_ERECT_PIERS_ACTIVITY_REQUIRED;
      }
   }

   // Make sure all temporary supports are erected and removed and that they
   // are removed after they are erected
   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();
      if ( !IsTemporarySupportErected(tsID) )
      {
         return TLM_ERECT_PIERS_ACTIVITY_REQUIRED;
      }

      if ( !IsTemporarySupportRemoved(tsID) )
      {
         return TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED;
      }
   
      EventIndexType erectIdx, removeIdx;
      GetTempSupportEvents(tsID,&erectIdx,&removeIdx);
      if ( removeIdx < erectIdx )
      {
         return TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR;
      }
   }

   // validate segments, girders, and groups
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         GirderIDType girderID = pGirder->GetID();

         // find out when the first tendon is stressed in this girder
         EventIndexType firstPTEventIdx = GetEventCount();
         const CPTData* pPTData = pGirder->GetPostTensioning();
         DuctIndexType nDucts = pPTData->GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            // tendon must be stressed
            if ( !IsTendonStressed(girderID,ductIdx) )
            {
               return TLM_STRESS_TENDONS_ACTIVITY_REQUIRED;
            }

            firstPTEventIdx = Min(firstPTEventIdx,GetStressTendonEventIndex(girderID,ductIdx));
         }

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segID = pSegment->GetID();

            // Segment must be constructed...
            if ( !IsSegmentConstructed(segID) )
            {
               return TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED;
            }

            // and erected...
            if ( !IsSegmentErected(segID) )
            {
               return TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED;
            }

            // and its supports must be erected prior to the segment being erected...
            EventIndexType erectSegmentEventIdx = GetSegmentErectionEventIndex(segID);

            // and it must be erected before the first tendon is stressed
            if ( firstPTEventIdx <= erectSegmentEventIdx )
            {
               return TLM_STRESS_TENDON_ERROR;
            }

            const CPierData2* pPier;
            const CTemporarySupportData* pTS;
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(i);
               pSegment->GetSupport(endType,&pPier,&pTS);

               if ( pPier )
               {
                  EventIndexType erectPierEventIdx = GetPierErectionEventIndex(pPier->GetID());
                  if ( erectSegmentEventIdx < erectPierEventIdx )
                  {
                     return TLM_SEGMENT_ERECTION_ERROR;
                  }
               }
               else
               {
                  EventIndexType erectTempSupportEventIdx, removeTempSupportEventIdx;
                  GetTempSupportEvents(pTS->GetID(),&erectTempSupportEventIdx,&removeTempSupportEventIdx);

                  // can't erect segment before temporary support
                  if ( erectSegmentEventIdx < erectTempSupportEventIdx )
                  {
                     return TLM_SEGMENT_ERECTION_ERROR;
                  }

                  // can't remove temporary support before segment is erected
                  if ( removeTempSupportEventIdx <= erectSegmentEventIdx )
                  {
                     return TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR;
                  }
               }

               const CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
               if ( pClosureJoint )
               {
                  // if there is a closure joint....

                  // it must be cast
                  if ( !IsClosureJointCast(pClosureJoint->GetID()) )
                  {
                     return TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED;
                  }

                  // and cast after the adjent segments are erected
                  EventIndexType erectRightSegmentEventIdx = GetSegmentErectionEventIndex(pClosureJoint->GetRightSegment()->GetID());
                  EventIndexType castClosureEventIdx = GetCastClosureJointEventIndex(pClosureJoint->GetID());
                  if ( castClosureEventIdx < erectSegmentEventIdx ||   // must be cast after left segment is erected
                       castClosureEventIdx < erectRightSegmentEventIdx // must be cast after right segment is erected
                     )
                  {
                     return TLM_CLOSURE_JOINT_ERROR;
                  }

                  // must be cast before the first tendon is stressed
                  if ( firstPTEventIdx <= castClosureEventIdx )
                  {
                     return TLM_STRESS_TENDON_ERROR;
                  }
               }
            } // next i
         } // next segment
      } // next girder
   } // next group

   return TLM_SUCCESS;
}

HRESULT CTimelineManager::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      Clear();

      hr = pStrLoad->BeginUnit(_T("TimelineEvents"));

      CComVariant var;
      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("Count"),&var);
      EventIndexType nEvents = VARIANT2INDEX(var);
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {
         CTimelineEvent* pTimelineEvent = new CTimelineEvent;
         pTimelineEvent->SetTimelineManager(this);

         pTimelineEvent->Load(pStrLoad,pProgress);
         if ( pTimelineEvent->GetID() == INVALID_ID )
         {
            pTimelineEvent->SetID(ms_ID++);
         }
         else
         {
            ms_ID = Max(ms_ID,pTimelineEvent->GetID()+1);
         }

         m_TimelineEvents.push_back(pTimelineEvent);
      }

      pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   ASSERT_VALID;

   return S_OK;
}

HRESULT CTimelineManager::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   ASSERT_VALID;

   pStrSave->BeginUnit(_T("TimelineEvents"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(m_TimelineEvents.size()));

   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit();

   return S_OK;
}

void CTimelineManager::MakeCopy(const CTimelineManager& rOther)
{
   Clear();

   std::vector<CTimelineEvent*>::const_iterator iter(rOther.m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(rOther.m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      CTimelineEvent* pNewTimelineEvent = new CTimelineEvent(*pTimelineEvent);
      pNewTimelineEvent->SetID(pTimelineEvent->GetID());
      pNewTimelineEvent->SetTimelineManager(this);
      m_TimelineEvents.push_back(pNewTimelineEvent);
   }

   m_pBridgeDesc = rOther.GetBridgeDescription();

   ASSERT_VALID;
}

void CTimelineManager::MakeAssignment(const CTimelineManager& rOther)
{
   MakeCopy(rOther);
}

void CTimelineManager::Clear()
{
   std::vector<CTimelineEvent*>::iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      CTimelineEvent* pTimelineEvent = *iter;
      delete pTimelineEvent;
   }
   m_TimelineEvents.clear();
}

void CTimelineManager::Sort()
{
   std::sort(m_TimelineEvents.begin(),m_TimelineEvents.end(),CompareEvents);

   // make sure events don't overlap and that there is only one
   // of each type of single occurance activities (such as cast deck or open to traffic)
   EventIndexType nLiveLoadEvents = 0;
   EventIndexType nRailingSystemEvents = 0;
   EventIndexType nOverlayEvents = 0;

   EventIndexType nEvents = m_TimelineEvents.size();
   Float64 end = m_TimelineEvents[0]->GetDay() + m_TimelineEvents[0]->GetMinElapsedTime();
   Float64 running_time_offset = 0; // sum of all previous time offets that must be applied to an event
   for ( EventIndexType eventIdx = 1; eventIdx < nEvents; eventIdx++ )
   {
      CTimelineEvent* pTimelineEvent = m_TimelineEvents[eventIdx];
      Float64 start = pTimelineEvent->GetDay();

      Float64 time_offset = start - end; // amount of time between the end of the previous event and the start of the current event
      if ( time_offset < 0 )
      {
         // the current interval starts before the previous interval ends... adjust
         // the interval days
         running_time_offset -= time_offset;
      }

      // adjust the day when the event occurs by the time offset for all previous events
      // do this using the friend attribute so we don't re-enter this method
      pTimelineEvent->m_Day += running_time_offset;

      // update end time for next interval
      end = start + pTimelineEvent->GetMinElapsedTime();

      // keep track of the number of live load events... keep the first one
      // and then remove any subsequent live load events
      if ( nLiveLoadEvents < 1 && pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         nLiveLoadEvents++;
      }
      else
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad(false);
      }

      // keep track of the number of railing system events... keep the first one
      // and then remove any subsequent railing events
      if ( nRailingSystemEvents < 1 && pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         nRailingSystemEvents++;
      }
      else
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad(false);
      }

      // keep track of the number of overlay events... keep the first one
      // and then remove any subsequent overlay events
      if ( nOverlayEvents < 1 && pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         nOverlayEvents++;
      }
      else
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad(false);
      }
   }
}

Float64 g_Day;
bool SearchMe(CTimelineEvent* pEvent)
{ 
   // returns true when the trial event occurs on or after g_Day
   // searching for the first event after the event that is being validated
   return g_Day <= pEvent->GetDay(); 
}

int CTimelineManager::ValidateEvent(const CTimelineEvent* pTimelineEvent) const
{
   const CTimelineEvent* pNextEvent = NULL;
   const CTimelineEvent* pPrevEvent = NULL;

   // find the first event that comes after the new event
   g_Day = pTimelineEvent->GetDay();
   std::vector<CTimelineEvent*>::const_iterator found( std::find_if(m_TimelineEvents.begin(),m_TimelineEvents.end(),SearchMe) );
   if ( found == m_TimelineEvents.end() )
   {
      // the "next" event wasn't found... that means there is no event that would come after the event that is being validated
      pPrevEvent = m_TimelineEvents.back();
   }
   else if ( found == m_TimelineEvents.begin() )
   {
      // all the previously defined events come after this event
      pNextEvent = *found;
      if ( pNextEvent == pTimelineEvent )
      {
         found++;
         pNextEvent = *found;
      }
   }
   else
   {
      found++; // advance to next event
      pNextEvent = *found;
      found--; // back up to this event
      found--; // back up to previous event
      pPrevEvent = *found;
   }

   // check the duration of the prev event. this event has to start after it
   if ( pPrevEvent && (pTimelineEvent->GetDay() < (pPrevEvent->GetDay() + pPrevEvent->GetMinElapsedTime())) )
   {
      // the new event occurs before the previous event ends
      return TLM_OVERLAPS_PREVIOUS_EVENT;
   }

   // check the duration of this event. this event has to end before it starts
   if ( pNextEvent && (pNextEvent->GetDay() < (pTimelineEvent->GetDay() + pTimelineEvent->GetMinElapsedTime())) )
   {
      // the new event overruns the next event
      return TLM_OVERRUNS_NEXT_EVENT;
   }

   return TLM_SUCCESS;
}

int CTimelineManager::CanRemoveEvent(CTimelineEvent* pTimelineEvent)
{
   // we can do anything if there isn't an associated bridge
   if ( m_pBridgeDesc == NULL )
   {
      return TLM_SUCCESS;
   }

   if ( pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
   {
      // can't remove this event because it defines when the segments are constructed
      return TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetErectPiersActivity().IsEnabled() )
   {
      return TLM_ERECT_PIERS_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
   {
      return TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
   {
      return TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() )
   {
      return TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() )
   {
      return TLM_STRESS_TENDONS_ACTIVITY_REQUIRED;
   }

   if ( m_pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone  &&
       pTimelineEvent->GetCastDeckActivity().IsEnabled() )
   {
      // there is a deck so you can't remove the deck casting event
      return TLM_CAST_DECK_ACTIVITY_REQUIRED;
   }

   if ( m_pBridgeDesc->GetDeckDescription()->WearingSurface != pgsTypes::wstSacrificialDepth &&
        pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
   {
      // there is an overlay in the bridge, can't remove the overlay loading event
      return TLM_OVERLAY_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
   {
      // railing system loading must be applied
      return TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
   {
      // live load loading must be applied
      return TLM_LIVELOAD_ACTIVITY_REQUIRED;
   }

   if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsUserLoadApplied() )
   {
      // there are use loads in this activity so it can't be removed
      return TLM_USER_LOAD_ACTIVITY_REQUIRED;
   }

   return TLM_SUCCESS;
}

CString CTimelineManager::GetErrorMessage(int errorCode) const
{
   CString strMsg;
   switch(errorCode)
   {
   case TLM_CAST_DECK_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include an activity for casting the deck.");
      break;

   case TLM_OVERLAY_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include an activity for installing the overlay.");
      break;

   case TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include an activity for installing the traffic barrier/railing system.");
      break;

   case TLM_LIVELOAD_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include an activity for opening the bridge to traffic.");
      break;

   case TLM_USER_LOAD_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activities for one or more user defined loads.");
      break;

   case TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activites for constructing one or more segments.");
      break;

   case TLM_ERECT_PIERS_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activities for constructing one or more piers or temporary supports.");
      break;

   case TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activites for erecting one or more segments.");
      break;

   case TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activities for removing one or more of the temporary supports.");
      break;

   case TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activities for casting one or more of the closure joints.");
      break;

   case TLM_STRESS_TENDONS_ACTIVITY_REQUIRED:
      strMsg = _T("The timeline does not include activites for stressing one or more tendons.");
      break;

   case TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR:
      strMsg = _T("A temporary support has been removed while it is still supporting a segment.");
      break;

   case TLM_SEGMENT_ERECTION_ERROR:
      strMsg = _T("A segment has been erected before its supporting elements (Pier or Temporary Support) have been erected.");
      break;

   case TLM_CLOSURE_JOINT_ERROR:
      strMsg = _T("A closure joint has been cast before its adjacent segments have been erected.");
      break;

   case TLM_RAILING_SYSTEM_ERROR:
      strMsg = _T("The traffic barrier/railing system has been installed before the deck was cast.");
      break;

   case TLM_STRESS_TENDON_ERROR:
      strMsg = _T("A tendon has been stressed before the segments and closure joints have been assembled.");
      break;

   default:
      ATLASSERT(false);
   }
   return strMsg;
}

#if defined _DEBUG
void CTimelineManager::AssertValid() const
{
   EventIndexType nLiveLoadEvents = 0;
   EventIndexType nOverlayEvents = 0;

   std::vector<CTimelineEvent*>::const_iterator iter(m_TimelineEvents.begin());
   std::vector<CTimelineEvent*>::const_iterator end(m_TimelineEvents.end());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      pTimelineEvent->AssertValid();

      if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         nLiveLoadEvents++;
      }

      if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         nOverlayEvents++;
      }

      if ( iter != m_TimelineEvents.begin() )
      {
         const CTimelineEvent* pPrevEvent = *(iter-1);
         ATLASSERT(pPrevEvent->GetDay() <= pTimelineEvent->GetDay());
      }
   }

   ATLASSERT(nLiveLoadEvents <= 1); // 0 means not set yet, 1 is ok, 2 or more... no good
   ATLASSERT(nOverlayEvents <= 1); // 0 means not set yet, 1 is ok, 2 or more... no good
}
#endif
