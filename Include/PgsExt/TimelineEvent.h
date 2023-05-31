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
#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\RemoveTemporarySupportsActivity.h>
#include <PgsExt\CastDeckActivity.h>
#include <PgsExt\ApplyLoadActivity.h>
#include <PgsExt\SegmentActivity.h>
#include <PgsExt\StressTendonActivity.h>
#include <PgsExt\CastLongitudinalJointActivity.h>
#include <PgsExt\GeometryControlActivity.h>

class CTimelineManager;

/*****************************************************************************
CLASS 
   CTimelineEvent

DESCRIPTION
   Encapsulates the data for the spliced girder bridge construction events
*****************************************************************************/

// NOTE: All time data for events and construction activities are in DAYS.
// They are not in system units. All the mathematically models required the time
// to be in days so there is no sense in constantly converting the values from
// system units to days.

class PGSEXTCLASS CTimelineEvent
{
public:
   CTimelineEvent();
   CTimelineEvent(const CTimelineEvent& rOther);
   ~CTimelineEvent();

   CTimelineEvent& operator= (const CTimelineEvent& rOther);
   bool operator<(const CTimelineEvent& rOther) const;
   bool operator==(const CTimelineEvent& rOther) const;
   bool operator!=(const CTimelineEvent& rOther) const;

   // Returns the timeline manager that manages this timeline event
   CTimelineManager* GetTimelineManager();
   const CTimelineManager* GetTimelineManager() const;

   // Set/Get the ID
   void SetID(EventIDType id);
   EventIDType GetID() const;

   // Set/Get the description
   void SetDescription(LPCTSTR description);
   LPCTSTR GetDescription() const;

   // Set/Get the day this timeline event occurs. If this timeline
   // event is associated with a timeline manager the timeline manager
   // will sort all of the timeline events and reposition this event
   // into the correct position in the timeline.
   void SetDay(Float64 day);
   Float64 GetDay() const;

   // returns the duration of this event as the longest duration of the
   // activities in this event
   Float64 GetDuration() const;

   // Set/Get the various activities the can occur during a timeline event

   void SetConstructSegmentsActivity(const CConstructSegmentActivity& activity);
   const CConstructSegmentActivity& GetConstructSegmentsActivity() const;
   CConstructSegmentActivity& GetConstructSegmentsActivity();

   void SetErectPiersActivity(const CErectPiersActivity& activity);
   const CErectPiersActivity& GetErectPiersActivity() const;
   CErectPiersActivity& GetErectPiersActivity();

   void SetErectSegmentsActivity(const CErectSegmentActivity& activity);
   const CErectSegmentActivity& GetErectSegmentsActivity() const;
   CErectSegmentActivity& GetErectSegmentsActivity();

   void SetRemoveTempSupportsActivity(const CRemoveTemporarySupportsActivity& activity);
   const CRemoveTemporarySupportsActivity& GetRemoveTempSupportsActivity() const;
   CRemoveTemporarySupportsActivity& GetRemoveTempSupportsActivity();

   void SetCastClosureJointActivity(const CCastClosureJointActivity& activity);
   const CCastClosureJointActivity& GetCastClosureJointActivity() const;
   CCastClosureJointActivity& GetCastClosureJointActivity();

   void SetCastDeckActivity(const CCastDeckActivity& activity);
   const CCastDeckActivity& GetCastDeckActivity() const;
   CCastDeckActivity& GetCastDeckActivity();

   void SetCastLongitudinalJointActivity(const CCastLongitudinalJointActivity& activity);
   const CCastLongitudinalJointActivity& GetCastLongitudinalJointActivity() const;
   CCastLongitudinalJointActivity& GetCastLongitudinalJointActivity();

   void SetStressTendonActivity(const CStressTendonActivity& activity);
   const CStressTendonActivity& GetStressTendonActivity() const;
   CStressTendonActivity& GetStressTendonActivity();

   void SetApplyLoadActivity(const CApplyLoadActivity& activity);
   const CApplyLoadActivity& GetApplyLoadActivity() const;
   CApplyLoadActivity& GetApplyLoadActivity();

   void SetGeometryControlActivity(const CGeometryControlActivity& activity);
   const CGeometryControlActivity& GetGeometryControlActivity() const;
   CGeometryControlActivity& GetGeometryControlActivity();

   // Returns the minimum elapsed time to the next event. The minimum elapsed time
   // is the sum of the time parameters associated with the activities
   // that are active during this event. This method is used to help
   // validate input parameters (the next event cannot occur
   // before all of the activities in this event have been completed)
   Float64 GetMinElapsedTime() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CTimelineEvent& rOther);
   void MakeAssignment(const CTimelineEvent& rOther);

   CTimelineManager* m_pTimelineMgr; // weak reference
   void SetTimelineManager(CTimelineManager* pTimelineMgr);

   void ClearCaches();

   EventIDType m_ID; // unique identifier for this event (it is your job to make sure it is unique)

   Float64 m_Day;
   std::_tstring m_Description;

   CApplyLoadActivity m_ApplyLoads;
   CErectPiersActivity m_ErectPiers;
   CConstructSegmentActivity m_ConstructSegments;
   CErectSegmentActivity m_ErectSegments;
   CCastClosureJointActivity m_CastClosureJoints;
   CCastDeckActivity m_CastDeck;
   CStressTendonActivity m_StressTendons;
   CRemoveTemporarySupportsActivity m_RemoveTempSupports;
   CCastLongitudinalJointActivity m_CastLongitudinalJoints;
   CGeometryControlActivity m_GeometryControl;

   Uint16 GetActivityScore() const;

   friend CTimelineManager;

#if defined _DEBUG
   void AssertValid() const;
#endif
};
