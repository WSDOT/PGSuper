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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\Keys.h>

#define CREATE_TIMELINE_EVENT INVALID_INDEX-1

#define TLM_OVERLAPS_PREVIOUS_EVENT                      0x00000001 // the new event occurs before the previous event ends
#define TLM_OVERRUNS_NEXT_EVENT                          0x00000002 // the new event ends after the next event begins
#define TLM_EVENT_NOT_FOUND                              0x00000004 // the event was not found

#define TLM_CAST_DECK_ACTIVITY_REQUIRED                  0x00000008 // deck casting is required if bridge has a deck
#define TLM_OVERLAY_ACTIVITY_REQUIRED                    0x00000010 // could not remove event because it has the overlay load activity and the bridge has an overlay
#define TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED             0x00000020 // railing system loading activity is required
#define TLM_LIVELOAD_ACTIVITY_REQUIRED                   0x00000040 // live load loading activity is required
#define TLM_USER_LOAD_ACTIVITY_REQUIRED                  0x00000080 // user defined loads must be in an event
#define TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED         0x00000100 // segment construction activity is required
#define TLM_ERECT_PIERS_ACTIVITY_REQUIRED                0x00000200
#define TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED             0x00000400
#define TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED  0x00000800
#define TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED         0x00001000
#define TLM_STRESS_TENDONS_ACTIVITY_REQUIRED             0x00002000

#define TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR              0x00004000 // temporary support was removed before it was erected or before the segments were lifted off by PT
#define TLM_SEGMENT_ERECTION_ERROR                       0x00008000 // segment erected before its supports are erected
#define TLM_CLOSURE_JOINT_ERROR                          0x00010000 // closure joint is cast before the segment is erected or PT is applied before it is cased and cured
#define TLM_RAILING_SYSTEM_ERROR                         0x00020000 // railing system is installed before deck is cast
#define TLM_STRESS_TENDON_ERROR                          0x00040000 // tendon stressed before closure joints are cast or segments are erected

#define TLM_SUCCESS                                      0xffffffff // event was successfully added

class CBridgeDescription2;

/*****************************************************************************
CLASS 
   CTimelineManager

DESCRIPTION
   Manages the construction timeline.
   
   Bridge components (segments, girders, piers, etc) are referenced by ID
   rather than index. When the bridge configuration changes (piers, girders, segments
   are added/deleted) the index of the components change. IDs are unique and
   do not change when the position of an item in an ordered collection changes.
*****************************************************************************/

class PGSEXTCLASS CTimelineManager
{
public:
   CTimelineManager();
   CTimelineManager(const CTimelineManager& rOther);
   ~CTimelineManager();

   CTimelineManager& operator= (const CTimelineManager& rOther);
   bool operator==(const CTimelineManager& rOther) const;
   bool operator!=(const CTimelineManager& rOther) const;

   void SetBridgeDescription(const CBridgeDescription2* pBridge);
   const CBridgeDescription2* GetBridgeDescription() const;

   // Adds a new event to the timeline. If bAdjustTimeline is true, the timeline from the insertion point to the end
   // is automatically adjusted so that time events do not overlap. A unique ID will be assigned to the event if
   // its ID is INVALID_ID. The index of the event will be return through the pEventIdx parameter. This method
   // returns one of the TLM_XXX return codes defined above
   int AddTimelineEvent(CTimelineEvent* pTimelineEvent,bool bAdjustTimeline,EventIndexType* pEventIdx);
   int AddTimelineEvent(const CTimelineEvent& timelineEvent,bool bAdjustTimeline,EventIndexType* pEventIdx);

   // Sets the data for a previously defined event. If bAdjustTimeline is true, the timeline
   // is automatically adjusted so that events do not overlap. One of the TLM_XXX codes defined above
   // is returned
   int SetEventByIndex(EventIndexType eventIdx,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline);
   int SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& timelineEvent,bool bAdjustTimeline);
   int SetEventByID(EventIDType id,CTimelineEvent* pTimelineEvent,bool bAdjustTimeline);
   int SetEventByID(EventIDType id,const CTimelineEvent& timelineEvent,bool bAdjustTimeline);

   // Adjusts the day when an event occurs. If bAdjustTimeline is true, the timeline is automatically
   // adjusted so that events do not overlap. One of the TLM_XXX codes defined above is returned
   int AdjustDayByIndex(EventIndexType eventIdx,Float64 day,bool bAdjustTimeline);
   int AdjustDayByID(EventIDType id,Float64 day,bool bAdjustTimeline);

   // Sets the elapsed time from the specified event to the next event.
   // The day of the specified event does not changed.
   void SetElapsedTime(EventIndexType eventIdx,Float64 elapsedTime);

   int RemoveEventByIndex(EventIndexType eventIdx);
   int RemoveEventByID(EventIDType id);

   const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx) const;
   CTimelineEvent* GetEventByIndex(EventIndexType eventIdx);
   const CTimelineEvent* GetEventByID(EventIDType id) const;
   CTimelineEvent* GetEventByID(EventIDType id);

   EventIndexType GetEventCount() const;
   bool FindEvent(LPCTSTR name,EventIndexType* pIndex,const CTimelineEvent** ppTimelineEvent) const;
   EventIndexType GetEventIndex(EventIDType ID) const;

   bool HasEvent(Float64 day) const;

   Float64 GetStart(EventIndexType eventIdx) const;
   Float64 GetEnd(EventIndexType eventIdx) const;
   Float64 GetDuration(EventIndexType eventIdx) const;

   bool AreAllPiersErected() const;
   bool AreAllTemporarySupportsErected() const;
   bool AreAllTemporarySupportsRemoved() const;
   bool AreAllSegmentsErected() const;
   bool AreAllSegmentsErected(GirderIDType girderID,EventIndexType eventIdx) const;
   bool AreAllClosureJointsCast() const;
   bool AreAllTendonsStressed() const;

   bool IsDeckCast() const;
   bool IsRailingSystemInstalled() const;
   bool IsSegmentConstructed(SegmentIDType segmentID) const;
   bool IsClosureJointCast(ClosureIDType closureID) const;
   bool IsPierErected(PierIDType pierID) const;
   bool IsTemporarySupportErected(SupportIDType tsID) const;
   bool IsTemporarySupportRemoved(SupportIDType tsID) const;
   bool IsTendonStressedByIndex(EventIndexType index) const;
   bool IsTendonStressedByID(EventIDType ID) const;

   // returns true if the segment is erected in some event
   bool IsSegmentErected(SegmentIDType segmentID) const;

   // returns true if the segment is erected in or has been erected prior to the specified event
   bool IsSegmentErected(SegmentIDType segmentID,EventIndexType eventIdx) const;

   // returns true if there is a closure joint at this pier
   bool IsClosureJointAtPier(PierIDType pierID) const;

   // returns true if there is a closure joint at this temporary support
   bool IsClosureJointAtTempSupport(SupportIDType tsID) const;

   // returns true if the specified closure joint is cast
   bool IsClosureJointCast(EventIndexType eventIdx,ClosureIDType closureID) const;

   bool IsTendonStressed(GirderIDType girderID,DuctIndexType ductIdx) const;

   void SetPierErectionEventByIndex(PierIDType pierID,EventIndexType eventIdx);
   void SetPierErectionEventByID(PierIDType pierID,IDType ID);
   EventIndexType GetPierErectionEventIndex(PierIDType pierID) const;
   EventIDType GetPierErectionEventID(PierIDType pierID) const;

   void SetTempSupportEvents(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx);
   void GetTempSupportEvents(SupportIDType tsID,EventIndexType* pErectIdx,EventIndexType* pRemoveIdx) const;

   EventIndexType GetSegmentConstructionEventIndex(SegmentIDType segmentID) const;
   EventIDType GetSegmentConstructionEventID(SegmentIDType segmentID) const;
   void SetSegmentConstructionEventByIndex(SegmentIDType segmentID,EventIndexType eventIdx);
   void SetSegmentConstructionEventByID(SegmentIDType segmentID,EventIDType ID);

   EventIndexType GetSegmentErectionEventIndex(SegmentIDType segmentID) const;
   EventIDType GetSegmentErectionEventID(SegmentIDType segmentID) const;
   void SetSegmentErectionEventByIndex(SegmentIDType segmentID,EventIndexType eventIdx);
   void SetSegmentErectionEventByID(SegmentIDType segmentID,EventIDType ID);

   EventIndexType GetFirstSegmentErectionEventIndex() const;
   EventIDType GetFirstSegmentErectionEventID() const;

   EventIndexType GetCastClosureJointEventIndex(ClosureIDType closureID) const;
   EventIDType GetCastClosureJointEventID(ClosureIDType closureID) const;
   void SetCastClosureJointEventByIndex(ClosureIDType closureID,EventIndexType eventIdx);
   void SetCastClosureJointEventByID(ClosureIDType closureID,EventIDType ID);

   // Events when tendons are stressed.
   EventIndexType GetStressTendonEventIndex(GirderIDType girderID,DuctIndexType ductIdx) const;
   EventIDType GetStressTendonEventID(GirderIDType girderID,DuctIndexType ductIdx) const;
   void SetStressTendonEventByIndex(GirderIDType girderID,DuctIndexType ductIdx,EventIndexType eventIdx);
   void SetStressTendonEventByID(GirderIDType girderID,DuctIndexType ductIdx,EventIDType ID);

   EventIndexType GetCastDeckEventIndex() const;
   EventIDType GetCastDeckEventID() const;
   int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline);
   int SetCastDeckEventByID(EventIDType ID,bool bAdjustTimeline);

   EventIndexType GetRailingSystemLoadEventIndex() const;
   EventIDType GetRailingSystemLoadEventID() const;
   void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx);
   void SetRailingSystemLoadEventByID(EventIDType ID);

   EventIndexType GetOverlayLoadEventIndex() const;
   EventIDType GetOverlayLoadEventID() const;
   void SetOverlayLoadEventByIndex(EventIndexType eventIdx);
   void SetOverlayLoadEventByID(EventIDType ID);
   int RemoveOverlayLoadEvent();

   EventIndexType GetLiveLoadEventIndex() const;
   EventIDType GetLiveLoadEventID() const;
   void SetLiveLoadEventByIndex(EventIndexType eventIdx);
   void SetLiveLoadEventByID(EventIDType ID);

   int Validate() const;
   int ValidateEvent(const CTimelineEvent* pTimelineEvent) const;
   CString GetErrorMessage(int errorCode) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CTimelineManager& rOther);
   virtual void MakeAssignment(const CTimelineManager& rOther);
   void Clear();
   void Sort();
   int CanRemoveEvent(CTimelineEvent* pTimelineEvent);

   std::vector<CTimelineEvent*> m_TimelineEvents; // owns the timeline events... will be deleted in the destructor
   const CBridgeDescription2* m_pBridgeDesc;

   static EventIDType ms_ID;

   friend CTimelineEvent;
   friend CBridgeDescription2;

#if defined _DEBUG
   void AssertValid() const;
#endif
};
