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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadManager.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\GirderLabel.h>

#include <System\Flags.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// initialize the ID counter
EventIDType CTimelineManager::ms_ID = 0;

CTimelineManager::CTimelineManager()
{
   m_pBridgeDesc = nullptr;
   m_pLoadManager = nullptr;
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
   PGS_ASSERT_VALID;

   if ( m_TimelineEvents.size() != rOther.m_TimelineEvents.size() )
   {
      return false;
   }

   auto& myIter(    m_TimelineEvents.cbegin()        );
   const auto& myEnd(     m_TimelineEvents.cend()          );
   auto& otherIter( rOther.m_TimelineEvents.cbegin() );
   for ( ; myIter != myEnd; myIter++, otherIter++ )
   {
      const auto* pMyEvent = *myIter;
      const auto* pOtherEvent = *otherIter;

      if (*pMyEvent != *pOtherEvent)
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

void CTimelineManager::SetLoadManager(const CLoadManager* pLoadMgr)
{
   m_pLoadManager = pLoadMgr;
}

const CLoadManager* CTimelineManager::GetLoadManager() const
{
   return m_pLoadManager;
}

void CTimelineManager::AppendTimelineEvent(CTimelineEvent* pTimelineEvent,EventIndexType* pEventIdx)
{
   if ( 0 < m_TimelineEvents.size() )
   {
      pTimelineEvent->SetDay(m_TimelineEvents.back()->GetDay());
   }

   if ( pTimelineEvent->GetID() == INVALID_ID )
   {
      pTimelineEvent->SetID(ms_ID++);
   }

   pTimelineEvent->SetTimelineManager(this);

   *pEventIdx = m_TimelineEvents.size();
   m_TimelineEvents.emplace_back(pTimelineEvent);
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
      if (pTimelineEvent->GetApplyLoadActivity().IsIntermediateDiaphragmLoadApplied())
      {
         CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetIntermediateDiaphragmsLoadEventIndex());
         if (pOtherTimelineEvent)
         {
            pOtherTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad(false);
         }
      }

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
      auto pierIDs = pTimelineEvent->GetCastClosureJointActivity().GetPiers();
      for ( const auto& pierID : pierIDs)
      {
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

      auto tsIDs = pTimelineEvent->GetCastClosureJointActivity().GetTempSupports();
      for ( const auto& tsID : tsIDs)
      {
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

   if (pTimelineEvent->GetCastLongitudinalJointActivity().IsEnabled())
   {
      CTimelineEvent* pOtherTimelineEvent = GetEventByIndex(GetCastLongitudinalJointEventIndex());
      if (pOtherTimelineEvent)
      {
         pOtherTimelineEvent->GetCastLongitudinalJointActivity().Enable(false);
      }
   }


   if ( pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
   {
      auto segmentIDs = pTimelineEvent->GetConstructSegmentsActivity().GetSegments();
      for ( const auto& segmentID : segmentIDs)
      {
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
      for ( const auto& pierID : pierIDs)
      {
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

      auto tsIDs = pTimelineEvent->GetErectPiersActivity().GetTempSupports();
      for ( const auto& tsID : tsIDs)
      {
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
      auto segmentIDs = pTimelineEvent->GetErectSegmentsActivity().GetSegments();
      for ( const auto& segmentID : segmentIDs)
      {
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
      auto tsIDs = pTimelineEvent->GetRemoveTempSupportsActivity().GetTempSupports();
      for(const auto& tsID : tsIDs)
      {
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
      auto vTendons = pTimelineEvent->GetStressTendonActivity().GetTendons();
      for ( const auto& tendonKey : vTendons)
      {
         GirderIDType gdrID = tendonKey.girderID;
         DuctIndexType ductIdx = tendonKey.ductIdx;
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
   m_TimelineEvents.emplace_back(pTimelineEvent);
   Sort(); // sort

   auto found = std::find(m_TimelineEvents.cbegin(),m_TimelineEvents.cend(),pTimelineEvent);
   ATLASSERT(found != m_TimelineEvents.cend());

   EventIndexType eventIdx = std::distance(m_TimelineEvents.cbegin(),found);

   PGS_ASSERT_VALID;

   *pEventIdx = eventIdx;

   return result;
}

int CTimelineManager::AddTimelineEvent(const CTimelineEvent& timelineEvent,bool bAdjustTimeline,EventIndexType* pEventIdx)
{
   std::unique_ptr<CTimelineEvent> pTimelineEvent(std::make_unique<CTimelineEvent>(timelineEvent));
   return AddTimelineEvent(pTimelineEvent.release(),bAdjustTimeline,pEventIdx);
}

void CTimelineManager::RemoveEventByIndex(EventIndexType eventIdx)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );

   CTimelineEvent* pTimelineEvent = m_TimelineEvents[eventIdx];
   delete pTimelineEvent;
   m_TimelineEvents.erase(m_TimelineEvents.cbegin() + eventIdx);

   PGS_ASSERT_VALID;
}

void CTimelineManager::RemoveEventByID(EventIDType id)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const CTimelineEvent* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return RemoveEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
      }
   }
}

int CTimelineManager::SetEventByIndex(EventIndexType eventIdx,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline)
{
   ATLASSERT(0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() );
   if ( !bAdjustTimeline )
   {
      // not automatically adjusting the timeline so validate the new event data before modifying it

      // remove the event that is going to be updated so the new event data wont conflict with it when it is evaluated
      CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];
      m_TimelineEvents.erase(m_TimelineEvents.cbegin()+eventIdx);
      
      // validate the new event against the timeline
      int result = ValidateEvent(pTimelineEvent);

      // return the old event data to the collection (put things back they way they were)
      m_TimelineEvents.insert(m_TimelineEvents.cbegin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         // the new event wont fit in the timeline without adjustment
         // leave now and let the caller know
         return result;
      }
   }

   // get the event we are replacing
   CTimelineEvent* pOldEvent = m_TimelineEvents[eventIdx];

   // retain it's event ID in the new event
   pTimelineEvent->SetID(pOldEvent->GetID());

   // replace the old event in the timeline with the new event
   m_TimelineEvents[eventIdx] = pTimelineEvent;

   // done with the old event... delete it
   delete pOldEvent;
   pOldEvent = nullptr;

   // sort the timeline... this will automatically adjust the timeline so all the events fit
   Sort();

   PGS_ASSERT_VALID;

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
      m_TimelineEvents.erase(m_TimelineEvents.cbegin()+eventIdx);

      int result = ValidateEvent(&timelineEvent);

      // return the old event data to the collection
      m_TimelineEvents.insert(m_TimelineEvents.cbegin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         return result;
      }
   }

   *m_TimelineEvents[eventIdx] = timelineEvent;
   Sort();

   PGS_ASSERT_VALID;

   return TLM_SUCCESS;
}

int CTimelineManager::SetEventByID(EventIDType id,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return SetEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter),pTimelineEvent,bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

int CTimelineManager::SetEventByID(EventIDType id,const CTimelineEvent& timelineEvent,bool bAdjustTimeline)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return SetEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter), timelineEvent,bAdjustTimeline);
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
      m_TimelineEvents.erase(m_TimelineEvents.cbegin()+eventIdx);

      Float64 oldDay = pOldEvent->GetDay();
      pOldEvent->SetDay(day);

      int result = ValidateEvent(pOldEvent);

      // return the old event data to the collection
      pOldEvent->SetDay(oldDay);
      m_TimelineEvents.insert(m_TimelineEvents.cbegin()+eventIdx,pOldEvent);

      if ( result != TLM_SUCCESS )
      {
         return result;
      }
   }

   m_TimelineEvents[eventIdx]->SetDay(day);
   Sort();

   PGS_ASSERT_VALID;

   return TLM_SUCCESS;
}

int CTimelineManager::AdjustDayByID(EventIDType id,Float64 day,bool bAdjustTimeline)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return AdjustDayByID(std::distance(m_TimelineEvents.cbegin(), iter),day,bAdjustTimeline);
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

   auto& iter(m_TimelineEvents.cbegin()+eventIdx+1);
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      auto* pTimelineEvent = *iter;
      // NOTE: don't call SetDay(). SetDay() causes the timeline events to be sorted.
      // The purpose of this method is to just adjust the elasped time between events
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
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( wcscmp(pTimelineEvent->GetDescription(),description) == 0 )
      {
         *pIndex = std::distance(m_TimelineEvents.cbegin(), iter);
         *ppTimelineEvent = pTimelineEvent;
         return true;
      }
   }

   *pIndex = INVALID_INDEX;
   *ppTimelineEvent = nullptr;
   return false;
}

EventIndexType CTimelineManager::GetEventIndex(EventIDType ID) const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         return std::distance(m_TimelineEvents.cbegin(), iter);
      }
   }

   ATLASSERT(false); // ID not found (this may be ok)
   return INVALID_INDEX;
}

bool CTimelineManager::HasEvent(Float64 day) const
{
   PGS_ASSERT_VALID;

   auto found(std::find_if(std::cbegin(m_TimelineEvents), std::cend(m_TimelineEvents), [day](const auto& event) {return IsEqual(day, event->GetDay()); }));
   return ( found != m_TimelineEvents.cend() ? true : false);
}

Float64 CTimelineManager::GetStart(EventIndexType eventIdx) const
{
   PGS_ASSERT_VALID;

   return m_TimelineEvents[eventIdx]->GetDay();
}

Float64 CTimelineManager::GetEnd(EventIndexType eventIdx) const
{
   PGS_ASSERT_VALID;

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
   PGS_ASSERT_VALID;

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
   PGS_ASSERT_VALID;

   if (0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size())
   {
      return m_TimelineEvents[eventIdx];
   }
   else
   {
      return nullptr;
   }
}

const CTimelineEvent* CTimelineManager::GetEventByID(EventIDType id) const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return GetEventByIndex(std::distance(m_TimelineEvents.cbegin(), iter));
      }
   }

   return nullptr;
}

CTimelineEvent* CTimelineManager::GetEventByIndex(EventIndexType eventIdx)
{
   PGS_ASSERT_VALID;

   if (0 <= eventIdx && eventIdx < (EventIndexType)m_TimelineEvents.size() )
   {
      return m_TimelineEvents[eventIdx];
   }
   else
   {
      return nullptr;
   }
}

CTimelineEvent* CTimelineManager::GetEventByID(EventIDType id)
{
   ATLASSERT(id != INVALID_ID);
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == id )
      {
         return GetEventByIndex(std::distance(m_TimelineEvents.cbegin(), iter));
      }
   }

   return nullptr;
}

bool CTimelineManager::IsDeckCast() const
{
   return GetCastDeckEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsLongitudinalJointCast() const
{
   return GetCastLongitudinalJointEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsOverlayInstalled() const
{
   return GetOverlayLoadEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsIntermediateDiaphragmInstalled() const
{
   return GetIntermediateDiaphragmsLoadEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsRailingSystemInstalled() const
{
   return GetRailingSystemLoadEventIndex() != INVALID_INDEX;
}

bool CTimelineManager::IsUserDefinedLoadApplied(LoadIDType loadID) const
{
   return FindUserLoadEventIndex(loadID) != INVALID_INDEX;
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
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
      {
         return std::distance(m_TimelineEvents.cbegin(), iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetPierErectionEventID(PierIDType pierID) const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetErectPiersActivity().HasPier(pierID) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetPierErectionEventByIndex(PierIDType pierID,EventIndexType eventIdx)
{
   PGS_ASSERT_VALID;

   for (auto* pTimelineEvent : m_TimelineEvents)
   {
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
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetPierErectionEventByIndex(pierID, std::distance(m_TimelineEvents.cbegin(), iter));
         break;
      }
   }
}

bool CTimelineManager::IsTemporarySupportErected(SupportIDType tsID) const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsTemporarySupportRemoved(SupportIDType tsID) const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsTendonStressedByID(EventIDType ID) const
{
   PGS_ASSERT_VALID;

   const CTimelineEvent* pTimelineEvent = GetEventByID(ID);
   if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed() )
   {
      return true;
   }

   return false;
}

bool CTimelineManager::IsTendonStressedByIndex(EventIndexType eventIdx) const
{
   PGS_ASSERT_VALID;

   const CTimelineEvent* pTimelineEvent = GetEventByIndex(eventIdx);
   if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed() )
   {
      return true;
   }

   return false;
}

bool CTimelineManager::IsSegmentErected(SegmentIDType segmentID) const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents )
   {
      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() && pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsSegmentErected(SegmentIDType segmentID,EventIndexType eventIdx) const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cbegin() + eventIdx + 1);
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() && pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointAtPier(PierIDType pierID) const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents )
   {
      if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pierID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointAtTempSupport(SupportIDType tsID) const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents )
   {
      if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(tsID) )
      {
         return true;
      }
   }

   return false;
}

bool CTimelineManager::IsClosureJointCast(EventIndexType eventIdx,ClosureIDType closureID) const
{
   PGS_ASSERT_VALID;

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
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() && pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return true;
      }
   }

   return false;
}

void CTimelineManager::SetTempSupportEvents(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx)
{
   PGS_ASSERT_VALID;

   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetErectPiersActivity().HasTempSupport(tsID) )
      {
         // event has this temp support for erection, but it is not the event we want
         // remove temp support from this activity
         pTimelineEvent->GetErectPiersActivity().RemoveTempSupport(tsID);
      }

      if (pTimelineEvent->GetRemoveTempSupportsActivity().HasTempSupport(tsID) )
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
   PGS_ASSERT_VALID;

   *pErectIdx  = INVALID_INDEX;
   *pRemoveIdx = INVALID_INDEX;

   EventIndexType idx = 0;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
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
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetSegmentConstructionEventID(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
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
   Float64 totalCuringDuration = 1.0;
   Float64 relaxationTime = 1.0;

   // If this construction of this segment is currently being modeled, get the construction
   // timing information.
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         if ( std::distance(m_TimelineEvents.cbegin(),iter) == eventIdx )
         {
            // the construction event isn't really changing... do nothing and return
            return;
         }

         // construction of this segment is being modeled and it is moving to a different timeline event
         bUpdateConstructionTiming = true;
         totalCuringDuration = pTimelineEvent->GetConstructSegmentsActivity().GetTotalCuringDuration();
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
         m_TimelineEvents[eventIdx]->GetConstructSegmentsActivity().SetTotalCuringDuration(totalCuringDuration);
         m_TimelineEvents[eventIdx]->GetConstructSegmentsActivity().SetRelaxationTime(relaxationTime);
      }
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetSegmentConstructionEventByID(SegmentIDType segmentID,EventIDType ID)
{
   ATLASSERT(segmentID != INVALID_ID);
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetSegmentConstructionEventByIndex(segmentID,std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetSegmentErectionEventIndex(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetSegmentErectionEventID(SegmentIDType segmentID) const
{
   ATLASSERT(segmentID != INVALID_ID);
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
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
   for ( auto& pTimelineEvent : m_TimelineEvents)
   {
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

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetSegmentErectionEventByID(SegmentIDType segmentID,EventIDType ID)
{
   ATLASSERT(segmentID != INVALID_ID);
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetSegmentErectionEventByIndex(segmentID,std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::GetSegmentEvents(SegmentIDType segmentID,EventIndexType* pConstructEventIdx,EventIndexType* pErectEventIdx) const
{
   ATLASSERT(segmentID != INVALID_ID);
   PGS_ASSERT_VALID;

   *pConstructEventIdx = INVALID_INDEX;
   *pErectEventIdx    = INVALID_INDEX;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetConstructSegmentsActivity().HasSegment(segmentID) )
      {
         ATLASSERT(*pConstructEventIdx == INVALID_INDEX); // if this fires, we found the event twice... that's a bug
         *pConstructEventIdx = std::distance(m_TimelineEvents.cbegin(),iter);
      }

      if ( pTimelineEvent->GetErectSegmentsActivity().HasSegment(segmentID) )
      {
         ATLASSERT(*pErectEventIdx == INVALID_INDEX); // if this fires, we found the event twice... that's a bug
         *pErectEventIdx = std::distance(m_TimelineEvents.cbegin(),iter);
      }

      if ( *pConstructEventIdx != INVALID_INDEX && *pErectEventIdx != INVALID_INDEX )
      {
         // found them both
         return;
      }
   }
}

void CTimelineManager::SetSegmentEvents(SegmentIDType segmentID,EventIndexType constructEventIdx,EventIndexType erectEventIdx)
{
   SetSegmentConstructionEventByIndex(segmentID,constructEventIdx);
   SetSegmentErectionEventByIndex(segmentID,erectEventIdx);
}

EventIndexType CTimelineManager::GetFirstSegmentErectionEventIndex() const
{
   PGS_ASSERT_VALID;

   EventIndexType eventIdx = 0;
   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( 0 < pTimelineEvent->GetErectSegmentsActivity().GetSegments().size() )
      {
         return eventIdx;
      }
      eventIdx++;
   }

   ATLASSERT(false); // there aren't any segments erected
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetFirstSegmentErectionEventID() const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( 0 < pTimelineEvent->GetErectSegmentsActivity().GetSegments().size() )
      {
         return pTimelineEvent->GetID();
      }
   }

   ATLASSERT(false); // there aren't any segments erected
   return INVALID_ID;
}

EventIndexType CTimelineManager::GetLastSegmentErectionEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.crbegin());
   const auto& end(m_TimelineEvents.crend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent(*iter);
      if (0 < pTimelineEvent->GetErectSegmentsActivity().GetSegments().size())
      {
         return std::distance(iter,m_TimelineEvents.crend());
      }
   }
   ATLASSERT(false); // there aren't any segments erected
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetLastSegmentErectionEventID() const
{
   PGS_ASSERT_VALID;

   EventIDType eventID = INVALID_ID;
   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if (pTimelineEvent->GetErectSegmentsActivity().GetSegments().size() != 0)
      {
         eventID = pTimelineEvent->GetID();
      }
   }

   return eventID;
}

EventIndexType CTimelineManager::GetCastClosureJointEventIndex(ClosureIDType closureID) const
{
   // when the BridgeDescription object is being copied, during an intermediate state, we might
   // not be able to find the closure joint. this is ok... we check pClosure here just in case it was not found
   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);
   if ( pClosure )
   {
      return GetCastClosureJointEventIndex(pClosure);
   }
   else
   {
      return INVALID_INDEX;
   }
}

EventIndexType CTimelineManager::GetCastClosureJointEventIndex(const CClosureJointData* pClosure) const
{
   PGS_ASSERT_VALID;

   if ( pClosure == nullptr )
   {
      ATLASSERT(false); // there should be a closure
      return INVALID_INDEX;
   }

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pClosure->GetPier() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pClosure->GetPier()->GetID()))
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }

      if ( pClosure->GetTemporarySupport() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pClosure->GetTemporarySupport()->GetID()))
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetCastClosureJointEventID(ClosureIDType closureID) const
{
   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);
   return GetCastClosureJointEventID(pClosure);
}

EventIDType CTimelineManager::GetCastClosureJointEventID(const CClosureJointData* pClosure) const
{
   PGS_ASSERT_VALID;

   if ( pClosure == nullptr )
   {
      ATLASSERT(false); // there should be a closure
      return INVALID_ID;
   }

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
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
   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);
   SetCastClosureJointEventByIndex(pClosure,eventIdx);
}

void CTimelineManager::SetCastClosureJointEventByIndex(const CClosureJointData* pClosure,EventIndexType eventIdx)
{
   PGS_ASSERT_VALID;

   if ( pClosure == nullptr )
   {
      ATLASSERT(false); // there should be a closure
      return;
   }

   Float64 totalCuringDuration = 7.0;
   bool bUpdateAge = false;

   for (auto& pTimelineEvent : m_TimelineEvents)
   {
      if ( pClosure->GetPier() && pTimelineEvent->GetCastClosureJointActivity().HasPier(pClosure->GetPier()->GetID()) )
      {
         bUpdateAge = true;
         totalCuringDuration = pTimelineEvent->GetCastClosureJointActivity().GetTotalCuringDuration();
         pTimelineEvent->GetCastClosureJointActivity().RemovePier(pClosure->GetPier()->GetID());
         break;
      }

      if ( pClosure->GetTemporarySupport() && pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pClosure->GetTemporarySupport()->GetID()) )
      {
         bUpdateAge = true;
         totalCuringDuration = pTimelineEvent->GetCastClosureJointActivity().GetTotalCuringDuration();
         pTimelineEvent->GetCastClosureJointActivity().RemoveTempSupport(pClosure->GetTemporarySupport()->GetID());
         break;
      }
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().Enable(true);

      if ( bUpdateAge )
      {
         m_TimelineEvents[eventIdx]->GetCastClosureJointActivity().SetTotalCuringDuration(totalCuringDuration);
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
   const CClosureJointData* pClosure = m_pBridgeDesc->FindClosureJoint(closureID);
   SetCastClosureJointEventByID(pClosure,ID);
}

void CTimelineManager::SetCastClosureJointEventByID(const CClosureJointData* pClosure,EventIDType ID)
{
   if ( pClosure == nullptr )
   {
      ATLASSERT(false); // there should be a closure
      return;
   }

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         EventIndexType eventIdx = std::distance(m_TimelineEvents.cbegin(),iter);
         SetCastClosureJointEventByIndex(pClosure,eventIdx);
         break;
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetStressTendonEventIndex(GirderIDType girderID,DuctIndexType ductIdx) const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetStressTendonEventID(GirderIDType girderID,DuctIndexType ductIdx) const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetStressTendonActivity().IsTendonStressed(girderID,ductIdx) )
      {
         return pTimelineEvent->GetID();
      }
   }

   return INVALID_ID;
}

void CTimelineManager::SetStressTendonEventByIndex(GirderIDType girderID,DuctIndexType ductIdx,EventIndexType eventIdx)
{
   for (auto& pTimelineEvent : m_TimelineEvents)
   {
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

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetStressTendonEventByID(GirderIDType girderID,DuctIndexType ductIdx,EventIDType ID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         EventIndexType eventIdx = std::distance(m_TimelineEvents.cbegin(),iter);
         SetStressTendonEventByIndex(girderID,ductIdx,eventIdx);
         break;
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetCastDeckEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetCastDeckEventID() const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

int CTimelineManager::SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline)
{
   EventIndexType currentCastDeckEventIdx = GetCastDeckEventIndex();

   CTimelineEvent* pOldCastDeckEvent = m_TimelineEvents[currentCastDeckEventIdx];
   pOldCastDeckEvent->GetCastDeckActivity().Enable(false);

   if ( eventIdx != INVALID_INDEX )
   {
      CTimelineEvent new_cast_deck_event = *m_TimelineEvents[eventIdx];
      new_cast_deck_event.SetCastDeckActivity(pOldCastDeckEvent->GetCastDeckActivity()); // copy the cast deck activity details from the old event to the new event
      new_cast_deck_event.GetCastDeckActivity().Enable(true);

      int result = SetEventByIndex(eventIdx, new_cast_deck_event, bAdjustTimeline);
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
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         return SetCastDeckEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter),bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

EventIndexType CTimelineManager::GetCastLongitudinalJointEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetCastLongitudinalJointActivity().IsEnabled())
      {
         return std::distance(m_TimelineEvents.begin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetCastLongitudinalJointEventID() const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
      if (pTimelineEvent->GetCastLongitudinalJointActivity().IsEnabled())
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

int CTimelineManager::SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline)
{
   bool bUpdateAge = false;
   Float64 totalCuringDuration = 7.0;
   CTimelineEvent* pOldCastLongitudinalJointEvent = nullptr;

   // search for the event where the longitudinal joint is cast
   for( auto* pTimelineEvent : m_TimelineEvents)
   {
      if (pTimelineEvent->GetCastLongitudinalJointActivity().IsEnabled())
      {
         bUpdateAge = true;
         totalCuringDuration = pTimelineEvent->GetCastLongitudinalJointActivity().GetTotalCuringDuration();
         pTimelineEvent->GetCastLongitudinalJointActivity().Enable(false);
         pOldCastLongitudinalJointEvent = pTimelineEvent; // hang onto the event in case the edit needs to be rolled back
         break;
      }
   }

   if (eventIdx != INVALID_INDEX)
   {
      CTimelineEvent longitudinal_joint_event = *m_TimelineEvents[eventIdx];
      longitudinal_joint_event.GetCastLongitudinalJointActivity().Enable(true);

      if (bUpdateAge)
      {
         longitudinal_joint_event.GetCastLongitudinalJointActivity().SetTotalCuringDuration(totalCuringDuration);
      }

      int result = SetEventByIndex(eventIdx, longitudinal_joint_event, bAdjustTimeline);
      if (result != TLM_SUCCESS)
      {
         // there was a problem... roll back the edit
         pOldCastLongitudinalJointEvent->GetCastLongitudinalJointActivity().Enable(true);
         return result;
      }
   }

   return TLM_SUCCESS;
}

int CTimelineManager::SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetID() == eventID)
      {
         return SetCastLongitudinalJointEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter), bAdjustTimeline);
      }
   }

   return TLM_EVENT_NOT_FOUND;
}

EventIndexType CTimelineManager::GetIntermediateDiaphragmsLoadEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetApplyLoadActivity().IsIntermediateDiaphragmLoadApplied())
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetIntermediateDiaphragmsLoadEventID() const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
      if (pTimelineEvent->GetApplyLoadActivity().IsIntermediateDiaphragmLoadApplied())
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx)
{
   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad(false);
   }

   if (eventIdx != INVALID_INDEX)
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad(true);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetID() == eventID)
      {
         SetIntermediateDiaphragmsLoadEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetRailingSystemLoadEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetRailingSystemLoadEventID() const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetRailingSystemLoadEventByIndex(EventIndexType eventIdx)
{
   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyRailingSystemLoad(true);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetRailingSystemLoadEventByID(EventIDType ID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetRailingSystemLoadEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetOverlayLoadEventIndex() const
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied() )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetOverlayLoadEventID() const
{
   PGS_ASSERT_VALID;

   for ( const auto* pTimelineEvent : m_TimelineEvents)
   {
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
   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad(false);
   }

   // activate the overlay load in the specified event
   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyOverlayLoad(true);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetOverlayLoadEventByID(EventIDType ID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetOverlayLoadEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::RemoveOverlayLoadEvent()
{
   SetOverlayLoadEventByIndex(INVALID_INDEX);

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetLiveLoadEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetLiveLoadEventID() const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetLiveLoadEventByIndex(EventIndexType eventIdx)
{
   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyLiveLoad(true);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetLiveLoadEventByID(EventIDType ID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetLiveLoadEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::GetLoadRatingEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetApplyLoadActivity().IsRatingLiveLoadApplied() )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

EventIDType CTimelineManager::GetLoadRatingEventID() const
{
   PGS_ASSERT_VALID;

   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if ( pTimelineEvent->GetApplyLoadActivity().IsRatingLiveLoadApplied() )
      {
         return pTimelineEvent->GetID();
      }
   }
   return INVALID_ID;
}

void CTimelineManager::SetLoadRatingEventByIndex(EventIndexType eventIdx)
{
   for (auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->GetApplyLoadActivity().ApplyRatingLiveLoad(false);
   }

   if ( eventIdx != INVALID_INDEX )
   {
      m_TimelineEvents[eventIdx]->GetApplyLoadActivity().ApplyRatingLiveLoad(true);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetLoadRatingEventByID(EventIDType ID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == ID )
      {
         SetLoadRatingEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetUserLoadEventByIndex(LoadIDType loadID,EventIndexType newEventIdx)
{
   EventIndexType currentEventIdx = FindUserLoadEventIndex(loadID);
   if (currentEventIdx == newEventIdx)
   {
      return;
   }

   if ( currentEventIdx != INVALID_INDEX )
   {
      CTimelineEvent* pTimelineEvent = m_TimelineEvents[currentEventIdx];
      pTimelineEvent->GetApplyLoadActivity().RemoveUserLoad(loadID);
   }

   if ( newEventIdx != INVALID_INDEX )
   {
      CTimelineEvent* pTimelineEvent = m_TimelineEvents[newEventIdx];
      pTimelineEvent->GetApplyLoadActivity().AddUserLoad(loadID);
   }

   PGS_ASSERT_VALID;
}

void CTimelineManager::SetUserLoadEventByID(LoadIDType loadID,EventIDType eventID)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      if ( pTimelineEvent->GetID() == eventID )
      {
         SetUserLoadEventByIndex(loadID,std::distance(m_TimelineEvents.cbegin(),iter));
         break;
      }
   }

   PGS_ASSERT_VALID;
}

EventIndexType CTimelineManager::FindUserLoadEventIndex(LoadIDType loadID) const
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for ( ; iter != end; iter++ )
   {
      const auto* pTimelineEvent = *iter;
      const CApplyLoadActivity& activity = pTimelineEvent->GetApplyLoadActivity();
      if ( activity.HasUserLoad(loadID) )
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

EventIDType CTimelineManager::FindUserLoadEventID(LoadIDType loadID) const
{
   EventIndexType eventIdx = FindUserLoadEventIndex(loadID);
   if ( eventIdx == INVALID_INDEX )
   {
      return INVALID_ID;
   }

   CTimelineEvent* pEvent = m_TimelineEvents[eventIdx];
   return pEvent->GetID();
}


EventIndexType CTimelineManager::GetPrimaryGeometryControlEventIndex() const
{
   PGS_ASSERT_VALID;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetGeometryControlActivity().IsGeometryControlEvent())
      {
         return std::distance(m_TimelineEvents.cbegin(),iter);
      }
   }
   return INVALID_INDEX;
}

std::vector<EventIndexType> CTimelineManager::GetGeometryControlEventIndices(pgsTypes::GeometryControlActivityType type) const
{
   PGS_ASSERT_VALID;
   std::vector<EventIndexType> events;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetGeometryControlActivity().GetGeometryControlEventType() == type)
      {
         events.push_back( std::distance(m_TimelineEvents.cbegin(),iter) );
      }
   }

   if (events.empty())
   {
      events.push_back(INVALID_INDEX);
   }

   return events;
}

void CTimelineManager::SetGeometryControlEventByIndex(EventIndexType eventIdx,pgsTypes::GeometryControlActivityType type)
{
   if(eventIdx != INVALID_INDEX)
   {
      if (type == pgsTypes::gcaGeometryControlEvent)
      {
         // can have only one main event
         EventIndexType currentGeometryControlEventIdx = GetPrimaryGeometryControlEventIndex();
         if (currentGeometryControlEventIdx != INVALID_INDEX)
         {
            CTimelineEvent* pOldGeometryControlEvent = m_TimelineEvents[currentGeometryControlEventIdx];
            pOldGeometryControlEvent->GetGeometryControlActivity().SetGeometryControlEventType(pgsTypes::gcaDisabled);
         }
         else
         {
            ATLASSERT(0); // should always have a prime event set
         }
      }

      CTimelineEvent new_geom_event = *m_TimelineEvents[eventIdx];
      new_geom_event.GetGeometryControlActivity().SetGeometryControlEventType(pgsTypes::gcaGeometryControlEvent);
      int result = SetEventByIndex(eventIdx,new_geom_event,false);
      ATLASSERT(result != TLM_SUCCESS);
   }
   else
   {
      ATLASSERT(0);
   }
}

std::vector <EventIDType> CTimelineManager::GetGeometryControlEventIDs(pgsTypes::GeometryControlActivityType type) const
{
   PGS_ASSERT_VALID;

   std::vector <EventIDType> events;
   for (const auto* pTimelineEvent : m_TimelineEvents)
   {
      if (pTimelineEvent->GetGeometryControlActivity().GetGeometryControlEventType() == type)
      {
         events.push_back( pTimelineEvent->GetID() );
      }
   }

   if (events.empty())
   {
      events.push_back(INVALID_ID);
   }

   return events;
}

void CTimelineManager::SetGeometryControlEventByID(EventIDType ID,pgsTypes::GeometryControlActivityType type)
{
   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
   for (; iter != end; iter++)
   {
      const auto* pTimelineEvent = *iter;
      if (pTimelineEvent->GetID() == ID)
      {

         SetGeometryControlEventByIndex(std::distance(m_TimelineEvents.cbegin(),iter), type);
         return;
      }
   }

   PGS_ASSERT_VALID;
}

Uint32 CTimelineManager::Validate() const
{
   ClearValidationCaches();

   if ( m_pBridgeDesc == nullptr )
   {
      return TLM_SUCCESS;
   }

   Uint32 error = TLM_SUCCESS;

   // Make sure the deck is cast
   if ( m_pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone && !IsDeckCast() )
   {
      error |= TLM_CAST_DECK_ACTIVITY_REQUIRED;
   }

   if ( m_pBridgeDesc->GetDeckDescription()->WearingSurface == pgsTypes::wstOverlay && !IsOverlayInstalled() )
   {
      error |= TLM_OVERLAY_ACTIVITY_REQUIRED;
   }

   // Make sure the intermediate diaphragms are installed
   if (!IsIntermediateDiaphragmInstalled())
   {
      error |= TLM_INTERMEDIATE_DIAPHRAGM_ACTIVITY_REQUIRED;
   }

   // Make sure the railing system is installed
   if ( !IsRailingSystemInstalled() )
   {
      error |= TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED;
   }
   
   // Check user defined loads
   if (m_pLoadManager)
   {
      for (const auto& load : m_pLoadManager->m_PointLoads)
      {
         if (!IsUserDefinedLoadApplied(load.m_ID))
         {
            error |= TLM_USER_LOAD_ACTIVITY_REQUIRED;
         }
      }

      for (const auto& load : m_pLoadManager->m_DistributedLoads)
      {
         if (!IsUserDefinedLoadApplied(load.m_ID))
         {
            error |= TLM_USER_LOAD_ACTIVITY_REQUIRED;
         }
      }

      for (const auto& load : m_pLoadManager->m_MomentLoads)
      {
         if (!IsUserDefinedLoadApplied(load.m_ID))
         {
            error |= TLM_USER_LOAD_ACTIVITY_REQUIRED;
         }
      }
   }

   // Geometry Control Events (GCE)
   EventIndexType castDeckEventIdx = GetCastDeckEventIndex();
   bool isDeck = INVALID_INDEX != castDeckEventIdx; // indirect way to determine
   EventIndexType firstPossibleGCE; // event just before the earliest event when a GCE can occur
   if (isDeck)
   {
      firstPossibleGCE = castDeckEventIdx;
   }
   else
   {
      firstPossibleGCE = GetLastSegmentErectionEventIndex();
   }

   EventIndexType currEventIdx = 0;
   EventIndexType gceIndex = INVALID_INDEX;
   for (const auto pTimelineEvent : m_TimelineEvents)
   {
      pgsTypes::GeometryControlActivityType gcType = pTimelineEvent->GetGeometryControlActivity().GetGeometryControlEventType();
      if (pgsTypes::gcaDisabled != gcType)
      {
         if (currEventIdx <= firstPossibleGCE)
         {
            // geometry control cannot take place before firstPossibleGCE
            error |= TLM_GEOM_EVENT_TIME_ERROR;
            m_GeomEventTimeError.push_back(currEventIdx);
         }
         else if (pgsTypes::gcaGeometryControlEvent == gcType)
         {
            // look for duplicate event
            if (gceIndex != INVALID_INDEX)
            {
               error |= TLM_GEOM_EVENT_DUPL_ERROR;
               m_GeomEventDuplicateError.push_back(std::make_pair(currEventIdx,gceIndex));
            }

            gceIndex = currEventIdx;
         }
      }

      currEventIdx++;
   }

   if (gceIndex == INVALID_INDEX)
   {
      // Must have one GCE
      error |= TLM_GEOM_EVENT_MISSING_ERROR;
   }

   // Make sure railing system is installed after the deck, or if there is no
   // deck, after the last segment is erected
   EventIndexType diaphragmEventIdx = GetIntermediateDiaphragmsLoadEventIndex();
   if (m_pBridgeDesc->GetDeckDescription()->GetDeckType() == pgsTypes::sdtNone)
   {
      EventIndexType lastSegmentErectionEventIdx = GetLastSegmentErectionEventIndex();
      if (GetRailingSystemLoadEventIndex() < lastSegmentErectionEventIdx)
      {
         error |= TLM_RAILING_SYSTEM_ERROR;
      }
   }
   else
   {
      if (GetRailingSystemLoadEventIndex() <= castDeckEventIdx)
      {
         error |= TLM_RAILING_SYSTEM_ERROR;
      }

      // Make sure intermediate diaphragms are installed before the deck is cast
      if (castDeckEventIdx < diaphragmEventIdx)
      {
         error |= TLM_INTERMEDIATE_DIAPHRAGM_LOADING_ERROR;
      }
   }

  

   EventIndexType liveLoadEventIdx = GetLiveLoadEventIndex();
   if (liveLoadEventIdx == INVALID_INDEX)
   {
      error |= TLM_LIVELOAD_ACTIVITY_REQUIRED;
   }

   // Make sure load rating doesn't occur before bridge is open to traffic
   EventIndexType loadRatingEventIdx = GetLoadRatingEventIndex();
   if ( loadRatingEventIdx < liveLoadEventIdx )
   {
      error |= TLM_LOAD_RATING_ERROR;
   }

   // Make sure all piers are erected
   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      if ( !IsPierErected(pierID) )
      {
         m_ErectPiersActivityError.emplace_back(std::make_pair(pierIdx,INVALID_INDEX));
         error |= TLM_ERECT_PIERS_ACTIVITY_REQUIRED;
      }
   }

   // Make sure all temporary supports are erected and removed and that they
   // are removed after they are erected
   std::vector<const CTemporarySupportData*> vStrongBacks;
   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();
      if ( !IsTemporarySupportErected(tsID) )
      {
         m_ErectPiersActivityError.emplace_back(std::make_pair(INVALID_INDEX, tsIdx));
         error |= TLM_ERECT_PIERS_ACTIVITY_REQUIRED;
      }

      if ( !IsTemporarySupportRemoved(tsID) )
      {
         m_RemoveTemporarySupportsActivityError.emplace_back(tsIdx);
         error |= TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED;
      }
   
      EventIndexType erectIdx, removeIdx;
      GetTempSupportEvents(tsID,&erectIdx,&removeIdx);
      if ( removeIdx < erectIdx )
      {
         m_TemporarySupportRemovalError.emplace_back(tsIdx);
         error |= TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR;
      }

      // save the strongbacks, there is additional validation to be done later
      if (pTS->GetSupportType() == pgsTypes::StrongBack)
      {
         vStrongBacks.push_back(pTS);
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
         const CPTData* pPTData = pGirder->GetPostTensioning();
         DuctIndexType nDucts = pPTData->GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            // tendon must be stressed
            if ( !IsTendonStressed(girderID,ductIdx) )
            {
               m_StressTendonsActivityError.emplace_back(pGirder->GetGirderKey(), ductIdx);
               error |= TLM_STRESS_TENDONS_ACTIVITY_REQUIRED;
            }

			   int result = ValidateDuct(pGirder,ductIdx);
			   if (result != TLM_SUCCESS)
			   {
               m_StressTendonError.emplace_back(pGirder->GetGirderKey(), ductIdx);
               error |= result;
			   }
         }


         if (gdrIdx == 0)
         {
            for (const auto& pTS : vStrongBacks)
            {
               // make sure strong backs are erected
               // 1) at the same time or after one of the segments it supports
               // 2) before both segments it supports are erected
               ATLASSERT(pTS->GetSupportType() == pgsTypes::StrongBack);
               const CClosureJointData* pCJ = pTS->GetClosureJoint(gdrIdx);
               const CPrecastSegmentData* pLeftSegment = pCJ->GetLeftSegment();
               const CPrecastSegmentData* pRightSegment = pCJ->GetRightSegment();

               SegmentIDType leftSegmentID = pLeftSegment->GetID();
               SegmentIDType rightSegmentID = pRightSegment->GetID();
               EventIndexType erectLeftSegmentEventIdx = GetSegmentErectionEventIndex(leftSegmentID);
               EventIndexType erectRightSegmentEventIdx = GetSegmentErectionEventIndex(rightSegmentID);

               EventIndexType erectTempSupportEventIdx, removeTempSupportEventIdx;
               GetTempSupportEvents(pTS->GetID(), &erectTempSupportEventIdx, &removeTempSupportEventIdx);

               if ((erectLeftSegmentEventIdx <= erectTempSupportEventIdx) && (erectTempSupportEventIdx <= erectRightSegmentEventIdx)
                  ||
                  (erectRightSegmentEventIdx <= erectTempSupportEventIdx) && (erectTempSupportEventIdx <= erectLeftSegmentEventIdx)
                  )
               {
                  // 1) temporary support is erected at or after the left segment is erected and at or before the right segment is erected
                  // OR
                  // 2) temporary support is erected at or after the right segment is erected and at or before the left segment is erected 
                  //
                  // These are the valid cases
               }
               else
               {
                  // everything else is invalid
                  m_StrongBackErectionError.emplace_back(pTS->GetIndex());
                  error |= TLM_STRONGBACK_ERECTION_ERROR;
               }
            }
         }

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segID = pSegment->GetID();

            // Segment must be constructed...
            if ( !IsSegmentConstructed(segID) )
            {
               m_ConstructSegmentActivityError.emplace_back(pSegment->GetSegmentKey());
               error |= TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED;
            }

            // and erected...
            if ( !IsSegmentErected(segID) )
            {
               m_ErectSegmentActivityError.emplace_back(pSegment->GetSegmentKey());
               error |= TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED;
            }

            // and its supports must be erected prior to the segment being erected
            EventIndexType erectSegmentEventIdx = GetSegmentErectionEventIndex(segID);

            std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
            for (const auto& pPier : vPiers)
            {
               EventIndexType erectPierEventIdx = GetPierErectionEventIndex(pPier->GetID());
               if (erectSegmentEventIdx < erectPierEventIdx)
               {
                  // segment cannot be erected before the pier
                  m_SegmentErectionError.emplace_back(pSegment->GetSegmentKey());
                  error |= TLM_SEGMENT_ERECTION_ERROR;
               }
            }

            // Make sure intermediate diaphragms are installed after the segments are erected
            if (diaphragmEventIdx < erectSegmentEventIdx)
            {
               error |= TLM_INTERMEDIATE_DIAPHRAGM_LOADING_ERROR;
               m_SegmentErectionError.emplace_back(pSegment->GetSegmentKey());
            }

            if (m_pLoadManager)
            {
               for (const auto& load : m_pLoadManager->m_PointLoads)
               {
                  EventIndexType eventIdx = FindUserLoadEventIndex(load.m_ID);
                  if (eventIdx < erectSegmentEventIdx)
                  {
                     error |= TLM_USER_LOAD_ERROR;
                     m_SegmentErectionError.emplace_back(pSegment->GetSegmentKey());
                  }
               }

               for (const auto& load : m_pLoadManager->m_DistributedLoads)
               {
                  EventIndexType eventIdx = FindUserLoadEventIndex(load.m_ID);
                  if (eventIdx < erectSegmentEventIdx)
                  {
                     error |= TLM_USER_LOAD_ERROR;
                     m_SegmentErectionError.emplace_back(pSegment->GetSegmentKey());
                  }
               }

               for (const auto& load : m_pLoadManager->m_MomentLoads)
               {
                  EventIndexType eventIdx = FindUserLoadEventIndex(load.m_ID);
                  if (eventIdx < erectSegmentEventIdx)
                  {
                     error |= TLM_USER_LOAD_ERROR;
                     m_SegmentErectionError.emplace_back(pSegment->GetSegmentKey());
                  }
               }
            }


            // Returns a vector of all the temporary supports that support this segment
            if (gdrIdx == 0)
            {
               // temporary supports are the same for all girder lines... just check this for the first girder
               std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();
               for (const auto& pTS : vTS)
               {
                  EventIndexType erectTempSupportEventIdx, removeTempSupportEventIdx;
                  GetTempSupportEvents(pTS->GetID(), &erectTempSupportEventIdx, &removeTempSupportEventIdx);

                  // can't remove temporary support before segment is erected
                  if (removeTempSupportEventIdx <= erectSegmentEventIdx)
                  {
                     m_TemporarySupportRemovalError.emplace_back(pTS->GetIndex());
                     error |= TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR;
                  }
               }

               const CClosureJointData* pClosureJoint = pSegment->GetClosureJoint(pgsTypes::metEnd);
               if (pClosureJoint)
               {
                  // if there is a closure joint....

                  // it must be cast
                  if (!IsClosureJointCast(pClosureJoint->GetID()))
                  {
                     m_CastClosureJointActivityError.emplace_back(pClosureJoint->GetClosureKey());
                     error |= TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED;
                  }

                  // and cast after the adjacent segments are erected
                  EventIndexType erectRightSegmentEventIdx = GetSegmentErectionEventIndex(pClosureJoint->GetRightSegment()->GetID());
                  EventIndexType castClosureEventIdx = GetCastClosureJointEventIndex(pClosureJoint);
                  if (castClosureEventIdx < erectSegmentEventIdx ||   // must be cast after left segment is erected
                     castClosureEventIdx < erectRightSegmentEventIdx // must be cast after right segment is erected
                     )
                  {
                     m_ClosureJointError.emplace_back(pClosureJoint->GetClosureKey());
                     error |= TLM_CLOSURE_JOINT_ERROR;
                  }
               } // if pClosureJoint
            } // if gdrIdx == 0
         } // next segment
      } // next girder
   } // next group

   // temporary supports can be at the ends of two different segments, that can lead to duplicate IDs in this container. remove duplicates
   m_TemporarySupportRemovalError.erase(std::unique(std::begin(m_TemporarySupportRemovalError), std::end(m_TemporarySupportRemovalError)), std::end(m_TemporarySupportRemovalError));

   return error;
}

Uint32 CTimelineManager::ValidateDuct(const CSplicedGirderData* pGirder, DuctIndexType ductIdx) const
{
	EventIndexType ptEventIdx = GetStressTendonEventIndex(pGirder->GetID(), ductIdx);
	const CPTData* pPTData = pGirder->GetPostTensioning();
	const CDuctData* pDuctData = pPTData->GetDuct(ductIdx);
	SegmentIndexType startSegIdx, endSegIdx;
	switch (pDuctData->DuctGeometryType)
	{
	case CDuctGeometry::Linear:
		startSegIdx = 0;
		endSegIdx = pGirder->GetSegmentCount() - 1;
		break;
	case CDuctGeometry::Parabolic:
	{
		PierIndexType startPierIdx, endPierIdx;
		pDuctData->ParabolicDuctGeometry.GetRange(&startPierIdx, &endPierIdx);
		SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
		SpanIndexType endSpanIdx = (SpanIndexType)(endPierIdx - 1);
		auto vStartSegments = pGirder->GetSegmentsForSpan(startSpanIdx);
		auto vEndSegments = pGirder->GetSegmentsForSpan(endSpanIdx);
		startSegIdx = vStartSegments.front()->GetSegmentKey().segmentIndex;
		endSegIdx = vEndSegments.back()->GetSegmentKey().segmentIndex;
      break;
	}
	case CDuctGeometry::Offset:
		ATLASSERT(false); // not fully implemented yet
		break;
	default:
		ATLASSERT(false); // is there a new type
	}

	for (SegmentIndexType segIdx = startSegIdx; segIdx <= endSegIdx; segIdx++)
	{
		SegmentIDType segID = pGirder->GetSegment(segIdx)->GetID();
		EventIndexType erectSegmentEventIdx = GetSegmentErectionEventIndex(segID);
		// segment must be erected before its field installed tendon is stressed
		if (ptEventIdx <= erectSegmentEventIdx)
		{
			return TLM_STRESS_TENDON_ERROR;
		}

		const auto* pClosureJoint = pGirder->GetSegment(segIdx)->GetClosureJoint(pgsTypes::metEnd);
		if (pClosureJoint && segIdx != endSegIdx)
		{
			// if there is a closure at the end of the segment, and this is not the last segment, 
			// the closure joint must be cast before the tendon crossing it is stressed.
			// the tendon ends in the last segment, so the tendon doesn't cross the closure joint
			// at end of the last segment
			EventIndexType castClosureEventIdx = GetCastClosureJointEventIndex(pClosureJoint);
			if (ptEventIdx <= castClosureEventIdx)
			{
				return TLM_STRESS_TENDON_ERROR;
			}
		}
	}

	return TLM_SUCCESS;
}

HRESULT CTimelineManager::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      Clear();

      hr = pStrLoad->BeginUnit(_T("TimelineEvents"));

      Float64 version;
      hr = pStrLoad->get_Version(&version);

      std::vector<CGirderTendonKey> vStressedTendons; // keeps track of the tendons that are stressed (see bug issue below)

      CComVariant var;
      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("Count"), &var);
      EventIndexType nEvents = VARIANT2INDEX(var);
      for (EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++)
      {
         CTimelineEvent* pTimelineEvent = new CTimelineEvent;
         pTimelineEvent->SetTimelineManager(this);

         pTimelineEvent->Load(pStrLoad, pProgress);
         if (pTimelineEvent->GetID() == INVALID_ID)
         {
            pTimelineEvent->SetID(ms_ID++);
         }
         else
         {
            ms_ID = Max(ms_ID, pTimelineEvent->GetID() + 1);
         }

         // because of bugs in early versions, we need to make sure that the same tendon
         // isn't stressed in more than one timeline events. if it is, remove it from this event
         CStressTendonActivity& stressTendonActivity = pTimelineEvent->GetStressTendonActivity();
         if (stressTendonActivity.IsEnabled())
         {
            std::vector<CGirderTendonKey> vTendonKeys = stressTendonActivity.GetTendons();
            for (const auto& tendonKey : vTendonKeys)
            {
               if (std::find(std::cbegin(vStressedTendons), std::cend(vStressedTendons), tendonKey) == std::cend(vStressedTendons))
               {
                  // the tendon has not been previously stressed
                  vStressedTendons.push_back(tendonKey);
                  std::sort(std::begin(vStressedTendons), std::end(vStressedTendons));
               }
               else
               {
                  stressTendonActivity.RemoveTendon(tendonKey.girderID, tendonKey.ductIdx, false);
               }
            }
         }

         m_TimelineEvents.push_back(pTimelineEvent);
      }

      pStrLoad->EndUnit();

      if (version < 2.0)
      {
         // there could be buggy data
         CTimelineEvent* pLastEvent = m_TimelineEvents.back();
         if (pLastEvent->GetDescription() == std::_tstring(_T("Final without Live Load (Bridge Site 2)")))
         {
            IndexType nEvents = m_TimelineEvents.size();
            CTimelineEvent* pPrevEvent = m_TimelineEvents[nEvents - 2];
            if (pPrevEvent->GetDescription() == std::_tstring(_T("Final with Live Load (Bridge Site 3)")))
            {
               // these events need to be swapped and the user loads need to stay where they are
               auto prevEventUserLoads = pPrevEvent->GetApplyLoadActivity().GetUserLoads();
               auto lastEventUserLoads = pLastEvent->GetApplyLoadActivity().GetUserLoads();

               pLastEvent->GetApplyLoadActivity().SetUserLoads(prevEventUserLoads);
               pPrevEvent->GetApplyLoadActivity().SetUserLoads(lastEventUserLoads);

               std::swap(pPrevEvent->m_Day, pLastEvent->m_Day);

               m_TimelineEvents[nEvents - 2] = pLastEvent;
               m_TimelineEvents[nEvents - 1] = pPrevEvent;
            }
         }
      }
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   PGS_ASSERT_VALID;

   return S_OK;
}

HRESULT CTimelineManager::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   PGS_ASSERT_VALID;

   // there was a bug in the ProjectAgent for building PGSuper timeline events
   // the bug happened because we assumed the railing system was installed 28 days
   // after the deck was composite. However, if the total curing duration of the deck
   // wasn't 28 days or greater after the railing system installation, the order of
   // the "With Liveload" and "Without Liveload" events got reversed.
   // This happed with data block unit 1.0. We change the data block unit to 2.0
   // to identify when the bad data my be in the storage stream
   pStrSave->BeginUnit(_T("TimelineEvents"),2.0);
   pStrSave->put_Property(_T("Count"),CComVariant(m_TimelineEvents.size()));

   for( auto* pTimelineEvent : m_TimelineEvents)
   {
      pTimelineEvent->Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit();

   return S_OK;
}

void CTimelineManager::MakeCopy(const CTimelineManager& rOther)
{
   Clear();

   for(const auto* pTimelineEvent : rOther.m_TimelineEvents)
   {
      CTimelineEvent* pNewTimelineEvent = new CTimelineEvent(*pTimelineEvent);
      pNewTimelineEvent->SetID(pTimelineEvent->GetID());
      pNewTimelineEvent->SetTimelineManager(this);
      m_TimelineEvents.push_back(pNewTimelineEvent);
   }

   m_pBridgeDesc = rOther.GetBridgeDescription();
   m_pLoadManager = rOther.GetLoadManager();

   PGS_ASSERT_VALID;
}

void CTimelineManager::MakeAssignment(const CTimelineManager& rOther)
{
   MakeCopy(rOther);
}

void CTimelineManager::Clear()
{
   for ( auto* pTimelineEvent : m_TimelineEvents)
   {
      delete pTimelineEvent;
   }
   m_TimelineEvents.clear();
}

void CTimelineManager::Sort()
{
   // sort events using the event objects, not the pointers in the collection
   std::sort(std::begin(m_TimelineEvents), std::end(m_TimelineEvents), [](const auto& event1, const auto& event2) {return *event1 < *event2; });

   // make sure events don't overlap and that there is only one
   // of each type of single Occurrence activities (such as cast deck or open to traffic)
   EventIndexType nLiveLoadEvents = 0;
   EventIndexType nRailingSystemEvents = 0;
   EventIndexType nOverlayEvents = 0;
   EventIndexType nIntermediateDiaphragmEvents = 0;

   EventIndexType nEvents = m_TimelineEvents.size();
   ATLASSERT(0 < nEvents);
   Float64 end = m_TimelineEvents[0]->GetDay() + m_TimelineEvents[0]->GetMinElapsedTime();
   Float64 running_time_offset = 0; // sum of all previous time offsets that must be applied to an event
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
      ATLASSERT(0 <= running_time_offset);
      running_time_offset = Max(0.0,running_time_offset); // make sure running_time_offset is never less than zero

      pTimelineEvent->m_Day += running_time_offset;

      // update end time for next interval
      Float64 min_elapsed_time = pTimelineEvent->GetMinElapsedTime();
      ATLASSERT(0 <= min_elapsed_time);
      min_elapsed_time = Max(0.0,min_elapsed_time); // make sure elapsed time is never less than zero
      end = start + min_elapsed_time;
      ATLASSERT(start <= end);

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

      // keep track of the number of intermediate diaphragm events... keep the first one
      // and then remove any subsequent intermediate diaphragm events
      if (nIntermediateDiaphragmEvents < 1 && pTimelineEvent->GetApplyLoadActivity().IsEnabled() && pTimelineEvent->GetApplyLoadActivity().IsIntermediateDiaphragmLoadApplied())
      {
         nIntermediateDiaphragmEvents++;
      }
      else
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad(false);
      }
   }
}

void CTimelineManager::ClearCaches()
{
   std::for_each(std::begin(m_TimelineEvents), std::end(m_TimelineEvents), [](auto* pTimelineEvent) {pTimelineEvent->ClearCaches(); });
}

void CTimelineManager::ClearValidationCaches() const
{
   m_RemoveTemporarySupportsActivityError.clear();
   m_ErectPiersActivityError.clear();
   m_StrongBackErectionError.clear();
   m_ConstructSegmentActivityError.clear();
   m_ErectSegmentActivityError.clear();
   m_SegmentErectionError.clear();
   m_TemporarySupportRemovalError.clear();
   m_CastClosureJointActivityError.clear();
   m_ClosureJointError.clear();
   m_StressTendonsActivityError.clear();
   m_StressTendonError.clear();
}

Uint32 CTimelineManager::ValidateEvent(const CTimelineEvent* pTimelineEvent) const
{
   if ( m_TimelineEvents.size() == 0 ||
        pTimelineEvent == m_TimelineEvents.back()
      )
   {
      // if the event is at the end of the timeline, we can just add it
      return TLM_SUCCESS;
   }

   const CTimelineEvent* pNextEvent = nullptr;
   const CTimelineEvent* pPrevEvent = nullptr;

   // find the first event that comes after the new event
   auto found(std::find_if(std::cbegin(m_TimelineEvents), std::cend(m_TimelineEvents), [day = pTimelineEvent->GetDay()](const auto& event) {return day <= event->GetDay(); }));
   if ( found == m_TimelineEvents.cend() )
   {
      // the "next" event wasn't found... that means there is no event that would come after the event that is being validated
      pPrevEvent = m_TimelineEvents.back();
   }
   else if ( found == m_TimelineEvents.cbegin() )
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
      pNextEvent = *found;
      if ( *found == pTimelineEvent && std::distance(m_TimelineEvents.cbegin(),found) < (std::vector<CTimelineEvent*>::difference_type)m_TimelineEvents.size() )
      {
         pNextEvent = *(found+1);
      }

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

std::_tstring CTimelineManager::GetErrorMessage(Uint32 errorCode) const
{
   std::_tostringstream os;
   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_OVERLAPS_PREVIOUS_EVENT))
   {
      os << _T("This event begins before the activities in the previous event have completed.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_OVERRUNS_NEXT_EVENT))
   {
      os << _T("The activities in this event end after the next event begins.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_EVENT_NOT_FOUND))
   {
      os << _T("Event not found") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_CAST_DECK_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include an activity for casting the deck.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_OVERLAY_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include an activity for installing the overlay.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_INTERMEDIATE_DIAPHRAGM_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include an activity for installing intermediate diaphragms.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include an activity for installing the traffic barrier/railing system.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_LIVELOAD_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include an activity for opening the bridge to traffic.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_USER_LOAD_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activities for one or more user defined loads.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activites for constructing the following segments:") << std::endl;
      std::for_each(std::cbegin(m_ConstructSegmentActivityError), std::cend(m_ConstructSegmentActivityError), [&os](const auto& segmentKey) {os << SEGMENT_LABEL(segmentKey) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activites for erecting the following segments:") << std::endl;
      std::for_each(std::cbegin(m_ErectSegmentActivityError), std::cend(m_ErectSegmentActivityError), [&os](const auto& segmentKey) {os << SEGMENT_LABEL(segmentKey) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_SEGMENT_ERECTION_ERROR))
   {
      os << _T("The following segments are erected before their supporting elements (Pier or Temporary Support) have been constructed:") << std::endl;
      std::for_each(std::cbegin(m_SegmentErectionError), std::cend(m_SegmentErectionError), [&os](const auto& segmentKey) {os << SEGMENT_LABEL(segmentKey) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR))
   {
      os << _T("The following temporary supports has been removed while they are still supporting segments:") << std::endl;
      std::for_each(std::cbegin(m_TemporarySupportRemovalError), std::cend(m_TemporarySupportRemovalError), [&os](const auto& tsIdx) {os << _T("Temporary Support ") << LABEL_INDEX(tsIdx) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activities for casting the following closure joints:") << std::endl;
      std::for_each(std::cbegin(m_CastClosureJointActivityError), std::cend(m_CastClosureJointActivityError), [&os](const auto& cjKey) {os << _T("Group ") << LABEL_GROUP(cjKey.groupIndex) << _T(", Closure ") << LABEL_SEGMENT(cjKey.segmentIndex) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_CLOSURE_JOINT_ERROR))
   {
      os << _T("The following closure joints are cast before the adjacent segments have been erected:") << std::endl;
      std::for_each(std::cbegin(m_ClosureJointError), std::cend(m_ClosureJointError), [&os](const auto& cjKey) {os << _T("Group ") << LABEL_GROUP(cjKey.groupIndex) << _T(", Closure ") << LABEL_SEGMENT(cjKey.segmentIndex) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_ERECT_PIERS_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activities for constructing the following piers or temporary supports:") << std::endl;
      std::for_each(std::cbegin(m_ErectPiersActivityError), std::cend(m_ErectPiersActivityError), [&os](const auto& pair) {if(pair.first != INVALID_INDEX) os << _T("Pier ") << LABEL_PIER(pair.first) << std::endl; else os << _T("Temporary Support ") << LABEL_TEMPORARY_SUPPORT(pair.second) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activities for removing the following temporary supports:") << std::endl;
      std::for_each(std::cbegin(m_RemoveTemporarySupportsActivityError), std::cend(m_RemoveTemporarySupportsActivityError), [&os](const auto& tsIdx) {os << _T("Temporary Support ") << LABEL_TEMPORARY_SUPPORT(tsIdx) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_STRESS_TENDONS_ACTIVITY_REQUIRED))
   {
      os << _T("The timeline does not include activites for stressing the following tendons:") << std::endl;
      std::for_each(std::cbegin(m_StressTendonsActivityError), std::cend(m_StressTendonsActivityError), [&os](const auto& key) {os << GIRDER_LABEL(key.girderKey) << _T(", Duct ") << LABEL_DUCT(key.ductIdx) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_STRONGBACK_ERECTION_ERROR))
   {
      os << _T("The following strongbacks are erected before the segments that support them:") << std::endl;
      std::for_each(std::cbegin(m_StrongBackErectionError), std::cend(m_StrongBackErectionError), [&os](const auto& tsIdx) {os << _T("Temporary Support ") << LABEL_TEMPORARY_SUPPORT(tsIdx) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_RAILING_SYSTEM_ERROR))
   {
      os << _T("The traffic barrier/railing system has been installed before the deck was cast.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_STRESS_TENDON_ERROR))
   {
      os << _T("The following tendons are stressed before the segments and closure joints have been assembled:") << std::endl;
      std::for_each(std::cbegin(m_StressTendonError), std::cend(m_StressTendonError), [&os](const auto& key) {os << GIRDER_LABEL(key.girderKey) << _T(", Duct ") << LABEL_DUCT(key.ductIdx) << std::endl; });
      os << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_LOAD_RATING_ERROR))
   {
      os << _T("Bridge must be open to traffic before load rating.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_INTERMEDIATE_DIAPHRAGM_LOADING_ERROR))
   {
      os << _T("Intermediate diaphragm loads are applied before segments are erected or after the deck is cast. They must be applied after all segments are erected up until deck casting. To fix the problem add an Apply Load activity with intermediate diaphragms loads at or before the event containing the Cast Deck activity to the timeline.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode, TLM_USER_LOAD_ERROR))
   {
      os << _T("User defined loads are applied before segments are erected. They must be applied after all segments are erected. To fix the problem add an Apply Load activity with user defined loads at or after the event containing the last Erect Segment activity to the timeline.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode,TLM_GEOM_EVENT_TIME_ERROR))
   {
      bool isDeck = INVALID_INDEX != GetCastDeckEventIndex(); // indirect way to determine

      for (auto event : m_GeomEventTimeError)
      {
         if (isDeck)
         {
            os << _T("Roadway Geometry Control Events (GCE) must occur after deck placement. Please remove the GCE from event ") << LABEL_EVENT(event) << std::endl << std::endl;
         }
         else
         {
            os << _T("Roadway Geometry Control Events (GCE) must occur after last segment has been erected. Please remove the GCE from event ") << LABEL_EVENT(event) << std::endl << std::endl;
         }
      }
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode,TLM_GEOM_EVENT_MISSING_ERROR))
   {
      os << _T("There must be a Roadway Geometry Control Event (GCE) defined. Please add a GCE after placement of deck.") << std::endl << std::endl;
   }

   if (WBFL::System::Flags<Uint32>::IsSet(errorCode,TLM_GEOM_EVENT_DUPL_ERROR))
   {
      for (auto dup : m_GeomEventDuplicateError)
      {
         os << _T("Only one Roadway Geometry Control Event (GCE) can be defined. There are duplicates in events ") << LABEL_EVENT(dup.first) << _T(" and ") << LABEL_EVENT(dup.second) << std::endl << std::endl;
      }
   }

   return os.str();
}

#if defined _DEBUG
void CTimelineManager::AssertValid() const
{
   EventIndexType nLiveLoadEvents = 0;
   EventIndexType nOverlayEvents = 0;

   // these sets are used to ensure that bridge elements don't get added to the
   // timeline more than once. For example, you can't erect the same pier or stress
   // the same tendon more than once.
   // We'll put the IDs into the set and if the insert fails, it is because the ID
   // is already there meaning the item was already modeled in the timeline
   std::set<SupportIDType> erectedPiers; // and temporary supports
   std::set<SegmentIDType> constructedSegments;
   std::set<SegmentIDType> erectedSegments;
   std::set<SupportIDType> removedTempSupports;
   std::set<SupportIDType> castClosureJoints;
   std::set<CGirderTendonKey> stressTendons;

   auto& iter(m_TimelineEvents.cbegin());
   const auto& end(m_TimelineEvents.cend());
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


      if (pTimelineEvent->GetErectPiersActivity().IsEnabled())
      {
         const auto& piers = pTimelineEvent->GetErectPiersActivity().GetPiers();
         for (const auto& pierID : piers)
         {
            auto result = erectedPiers.insert(pierID);
            ATLASSERT(result.second == true); // if this fires, the pier has been previously assigned to an erection activity
         }
      }

      if (pTimelineEvent->GetConstructSegmentsActivity().IsEnabled())
      {
         const auto& segments = pTimelineEvent->GetConstructSegmentsActivity().GetSegments();
         for (const auto& segmentID : segments)
         {
            auto result = constructedSegments.insert(segmentID);
            ATLASSERT(result.second == true); // if this fires, the segment has been previously assigned to a construct segments activity
         }
      }

      if (pTimelineEvent->GetErectSegmentsActivity().IsEnabled())
      {
         const auto& segments = pTimelineEvent->GetErectSegmentsActivity().GetSegments();
         for (const auto& segmentID : segments)
         {
            auto result = erectedSegments.insert(segmentID);
            ATLASSERT(result.second == true); // if this fires, the segment has been previously assigned to an erection activity
         }
      }

      if (pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled())
      {
         const auto& supports = pTimelineEvent->GetRemoveTempSupportsActivity().GetTempSupports();
         for (const auto& supportID : supports)
         {
            auto result = removedTempSupports.insert(supportID);
            ATLASSERT(result.second == true); // if this fires, the temporary support has been previously assigned to a removal activity
         }
      }

      if (pTimelineEvent->GetCastClosureJointActivity().IsEnabled())
      {
         std::set<SupportIDType> supports = pTimelineEvent->GetCastClosureJointActivity().GetPiers();
         for (SupportIDType supportID : supports)
         {
            auto result = castClosureJoints.insert(supportID);
            ATLASSERT(result.second == true); // if this fires, the closure joint has been previously assigned to a casting activity
         }
      }

      if (pTimelineEvent->GetStressTendonActivity().IsEnabled())
      {
         const auto& tendonKeys = pTimelineEvent->GetStressTendonActivity().GetTendons();
         for (const auto& tendonKey : tendonKeys)
         {
            auto result = stressTendons.insert(tendonKey);
            ATLASSERT(result.second == true); // if this fires, the tendon has been previously assigned to a stressing activity
         }
      }

      if ( iter != m_TimelineEvents.cbegin() )
      {
         const CTimelineEvent* pPrevEvent = *(iter-1);
         ATLASSERT(pPrevEvent->GetDay() <= pTimelineEvent->GetDay());
      }
   }

   ATLASSERT(nLiveLoadEvents <= 1); // 0 means not set yet, 1 is ok, 2 or more... no good
   ATLASSERT(nOverlayEvents <= 1); // 0 means not set yet, 1 is ok, 2 or more... no good
}
#endif
