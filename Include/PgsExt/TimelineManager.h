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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\Keys.h>

#define CREATE_TIMELINE_EVENT INVALID_ID-1  // special event ID used in the UI to indicate that we want to create a timeline event on the fly

#define TLM_OVERLAPS_PREVIOUS_EVENT                      0x00000001 // the new event occurs before the previous event ends
#define TLM_OVERRUNS_NEXT_EVENT                          0x00000002 // the new event ends after the next event begins
#define TLM_EVENT_NOT_FOUND                              0x00000004 // the event was not found

#define TLM_CAST_DECK_ACTIVITY_REQUIRED                  0x00000008 // deck casting is required if bridge has a deck
#define TLM_OVERLAY_ACTIVITY_REQUIRED                    0x00000010 // could not remove event because it has the overlay load activity and the bridge has an overlay
#define TLM_INTERMEDIATE_DIAPHRAGM_ACTIVITY_REQUIRED     0x00000020 // intermediate diaphragm loading activity is required
#define TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED             0x00000040 // railing system loading activity is required
#define TLM_LIVELOAD_ACTIVITY_REQUIRED                   0x00000080 // live load loading activity is required
#define TLM_USER_LOAD_ACTIVITY_REQUIRED                  0x00000100 // user defined loads must be in an event
#define TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED         0x00000200 // segment construction activity is required
#define TLM_ERECT_PIERS_ACTIVITY_REQUIRED                0x00000400 // an activity for pier erection is required
#define TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED             0x00000800 // an activity for segment erection is required
#define TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED  0x00001000 // an activity for temporary supports is required
#define TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED         0x00002000 // an activity for casting closure joints is required
#define TLM_STRESS_TENDONS_ACTIVITY_REQUIRED             0x00004000 // an activity for stressing tendons is required
#define TLM_CAST_LONGITUDINAL_JOINT_ACTIVITY_REQUIRED    0x00008000 // an activity for casting the longitudinal joints is required

#define TLM_STRONGBACK_ERECTION_ERROR                    0x00010000 // strongback erected before segment
#define TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR              0x00020000 // temporary support was removed before it was erected or before the segments were lifted off by PT
#define TLM_SEGMENT_ERECTION_ERROR                       0x00040000 // segment erected before its supports are erected
#define TLM_CLOSURE_JOINT_ERROR                          0x00080000 // closure joint is cast before the segment is erected or PT is applied before it is cased and cured
#define TLM_RAILING_SYSTEM_ERROR                         0x00100000 // railing system is installed before deck is cast
#define TLM_STRESS_TENDON_ERROR                          0x00200000 // tendon stressed before closure joints are cast or segments are erected
#define TLM_LOAD_RATING_ERROR                            0x00400000 // load rating occurs before bridge is open to traffic
#define TLM_INTERMEDIATE_DIAPHRAGM_LOADING_ERROR         0x00800000 // intermediate diaphragm are cast after the deck is cast (must occur before) or before segment erection (must occur after)
#define TLM_USER_LOAD_ERROR                              0x01000000 // user defined loads are applied before a segment is erected (must occur after)
#define TLM_GEOM_EVENT_TIME_ERROR                        0x02000000 // a geometry control event is placed before deck casting
#define TLM_GEOM_EVENT_MISSING_ERROR                     0x04000000 // must have one primary geometry control event
#define TLM_GEOM_EVENT_DUPL_ERROR                        0x08000000 // cannot have more than one primary geometry control event

#define TLM_SUCCESS                                      0x00000000 // event was successfully added

class CBridgeDescription2;
class CLoadManager;
class CClosureJointData;
class CSplicedGirderData;

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

   void Clear();

   CTimelineManager& operator= (const CTimelineManager& rOther);
   bool operator==(const CTimelineManager& rOther) const;
   bool operator!=(const CTimelineManager& rOther) const;

   void SetBridgeDescription(const CBridgeDescription2* pBridge);
   const CBridgeDescription2* GetBridgeDescription() const;

   void SetLoadManager(const CLoadManager* pLoadMgr);
   const CLoadManager* GetLoadManager() const;

   // Appends an event to the end of the timeline. The day of occurrence will be changed to
   // match the last event in the timeline. A unique ID will be assigned to the event if
   // its ID is INVALID_ID. The index of the event will be return through the pEventIdx parameter. 
   void AppendTimelineEvent(CTimelineEvent* pTimelineEvent,EventIndexType* pEventIdx);

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

   void RemoveEventByIndex(EventIndexType eventIdx);
   void RemoveEventByID(EventIDType id);

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

   bool IsLongitudinalJointCast() const;
   bool IsDeckCast() const;
   bool IsOverlayInstalled() const;
   bool IsIntermediateDiaphragmInstalled() const;
   bool IsRailingSystemInstalled() const;
   bool IsUserDefinedLoadApplied(LoadIDType loadID) const;
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
   void SetPierErectionEventByID(PierIDType pierID,EventIDType ID);
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

   void GetSegmentEvents(SegmentIDType segmentID,EventIndexType* pConstructEventIdx,EventIndexType* pErectEventIdx) const;
   void SetSegmentEvents(SegmentIDType segmentID,EventIndexType constructEventIdx,EventIndexType erectEventIdx);

   EventIndexType GetFirstSegmentErectionEventIndex() const;
   EventIDType GetFirstSegmentErectionEventID() const;

   EventIndexType GetLastSegmentErectionEventIndex() const;
   EventIDType GetLastSegmentErectionEventID() const;

   EventIndexType GetCastClosureJointEventIndex(const CClosureJointData* pClosure) const;
   EventIDType GetCastClosureJointEventID(const CClosureJointData* pClosure) const;
   EventIndexType GetCastClosureJointEventIndex(ClosureIDType closureID) const;
   EventIDType GetCastClosureJointEventID(ClosureIDType closureID) const;
   void SetCastClosureJointEventByIndex(ClosureIDType closureID,EventIndexType eventIdx);
   void SetCastClosureJointEventByIndex(const CClosureJointData* pClosure,EventIndexType eventIdx);
   void SetCastClosureJointEventByID(ClosureIDType closureID,EventIDType ID);
   void SetCastClosureJointEventByID(const CClosureJointData* pClosure,EventIDType ID);

   // Events when tendons are stressed.
   EventIndexType GetStressTendonEventIndex(GirderIDType girderID,DuctIndexType ductIdx) const;
   EventIDType GetStressTendonEventID(GirderIDType girderID,DuctIndexType ductIdx) const;
   void SetStressTendonEventByIndex(GirderIDType girderID,DuctIndexType ductIdx,EventIndexType eventIdx);
   void SetStressTendonEventByID(GirderIDType girderID,DuctIndexType ductIdx,EventIDType ID);

   EventIndexType GetCastLongitudinalJointEventIndex() const;
   EventIDType GetCastLongitudinalJointEventID() const;
   int SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline);
   int SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline);

   EventIndexType GetCastDeckEventIndex() const;
   EventIDType GetCastDeckEventID() const;
   int SetCastDeckEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline);
   int SetCastDeckEventByID(EventIDType eventID, bool bAdjustTimeline);

   EventIndexType GetIntermediateDiaphragmsLoadEventIndex() const;
   EventIDType GetIntermediateDiaphragmsLoadEventID() const;
   void SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx);
   void SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID);

   EventIndexType GetRailingSystemLoadEventIndex() const;
   EventIDType GetRailingSystemLoadEventID() const;
   void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx);
   void SetRailingSystemLoadEventByID(EventIDType eventID);

   EventIndexType GetOverlayLoadEventIndex() const;
   EventIDType GetOverlayLoadEventID() const;
   void SetOverlayLoadEventByIndex(EventIndexType eventIdx);
   void SetOverlayLoadEventByID(EventIDType eventID);
   void RemoveOverlayLoadEvent();

   EventIndexType GetLiveLoadEventIndex() const;
   EventIDType GetLiveLoadEventID() const;
   void SetLiveLoadEventByIndex(EventIndexType eventIdx);
   void SetLiveLoadEventByID(EventIDType eventID);

   EventIndexType GetLoadRatingEventIndex() const;
   EventIDType GetLoadRatingEventID() const;
   void SetLoadRatingEventByIndex(EventIndexType eventIdx);
   void SetLoadRatingEventByID(EventIDType eventID);

   void SetUserLoadEventByIndex(LoadIDType loadID,EventIndexType eventIdx);
   void SetUserLoadEventByID(LoadIDType loadID,EventIDType eventID);
   EventIndexType FindUserLoadEventIndex(LoadIDType loadID) const;
   EventIDType FindUserLoadEventID(LoadIDType loadID) const;

   EventIndexType GetPrimaryGeometryControlEventIndex() const; // main activity where geometry is based
   std::vector<EventIndexType> GetGeometryControlEventIndices(pgsTypes::GeometryControlActivityType type) const; // all other activities
   std::vector <EventIDType> GetGeometryControlEventIDs(pgsTypes::GeometryControlActivityType type) const;
   void SetGeometryControlEventByIndex(EventIndexType eventIdx,pgsTypes::GeometryControlActivityType type);
   void SetGeometryControlEventByID(EventIDType eventID,pgsTypes::GeometryControlActivityType type);


   Uint32 Validate() const;
   Uint32 ValidateEvent(const CTimelineEvent* pTimelineEvent) const;
   std::_tstring GetErrorMessage(Uint32 errorCode) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CTimelineManager& rOther);
   void MakeAssignment(const CTimelineManager& rOther);
   void Sort();

   void ClearCaches();
   void ClearValidationCaches() const;

   Uint32 ValidateDuct(const CSplicedGirderData* pGirder, DuctIndexType ductIdx) const;


   std::vector<CTimelineEvent*> m_TimelineEvents; // owns the timeline events... will be deleted in the destructor
   const CBridgeDescription2* m_pBridgeDesc;
   const CLoadManager* m_pLoadManager;

   // caches for error information from the last time Validate was called
   mutable std::vector<SupportIndexType> m_RemoveTemporarySupportsActivityError; // TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED
   mutable std::vector<std::pair<PierIndexType,SupportIndexType>> m_ErectPiersActivityError; // TLM_ERECT_PIERS_ACTIVITY_REQUIRED
   mutable std::vector<SupportIndexType> m_StrongBackErectionError; // TLM_STRONGBACK_ERECTION_ERROR
   mutable std::vector<CSegmentKey> m_ConstructSegmentActivityError; // TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED
   mutable std::vector<CSegmentKey> m_ErectSegmentActivityError; // TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED
   mutable std::vector<CSegmentKey> m_SegmentErectionError; // TLM_SEGMENT_ERECTION_ERROR
   mutable std::vector<SupportIndexType> m_TemporarySupportRemovalError; // TLM_TEMPORARY_SUPPORT_REMOVAL_ERROR
   mutable std::vector<CClosureKey> m_CastClosureJointActivityError; // TLM_CAST_CLOSURE_JOINT_ACTIVITY_REQUIRED
   mutable std::vector<CClosureKey> m_ClosureJointError; // TLM_CLOSURE_JOINT_ERROR
   mutable std::vector<CGirderTendonKey> m_StressTendonsActivityError; // TLM_STRESS_TENDONS_ACTIVITY_REQUIRED
   mutable std::vector<CGirderTendonKey> m_StressTendonError; // TLM_STRESS_TENDON_ERROR
   mutable std::vector<EventIndexType> m_GeomEventTimeError; //  TLM_GEOM_EVENT_TIME_ERROR
   mutable std::vector<std::pair<EventIndexType,EventIndexType>> m_GeomEventDuplicateError; //  TLM_GEOM_EVENT_DUPL_ERROR

   static EventIDType ms_ID;

   friend CTimelineEvent;
   friend CBridgeDescription2;
   friend CSegmentActivityBase;

#if defined _DEBUG
   void AssertValid() const;
#endif
};
