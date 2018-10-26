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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\RemoveTemporarySupportsActivity.h>
#include <PgsExt\CastDeckActivity.h>
#include <PgsExt\ApplyLoadActivity.h>
#include <PgsExt\SegmentActivity.h>
#include <PgsExt\StressTendonActivity.h>

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

   CTimelineManager* GetTimelineManager();
   const CTimelineManager* GetTimelineManager() const;

   void SetID(EventIDType id);
   EventIDType GetID() const;

   void SetDescription(LPCTSTR description);
   LPCTSTR GetDescription() const;

   void SetDay(Float64 day);
   Float64 GetDay() const;

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

   void SetCastClosurePourActivity(const CCastClosurePourActivity& activity);
   const CCastClosurePourActivity& GetCastClosurePourActivity() const;
   CCastClosurePourActivity& GetCastClosurePourActivity();

   void SetCastDeckActivity(const CCastDeckActivity& activity);
   const CCastDeckActivity& GetCastDeckActivity() const;
   CCastDeckActivity& GetCastDeckActivity();

   void SetStressTendonActivity(const CStressTendonActivity& activity);
   const CStressTendonActivity& GetStressTendonActivity() const;
   CStressTendonActivity& GetStressTendonActivity();

   void SetApplyLoadActivity(const CApplyLoadActivity& activity);
   const CApplyLoadActivity& GetApplyLoadActivity() const;
   CApplyLoadActivity& GetApplyLoadActivity();

   // Returns the minimum duration for an event. The minimum duration is
   // the sum of the time parameters associated with the activities
   // that are active during this event. This method is used to help
   // validate input parameters (you can't have 10 days from stressing to
   // prestress release when the event after construction occurs 5 days before
   // the stressing event)
   Float64 GetMinimumDuration() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CTimelineEvent& rOther);
   virtual void MakeAssignment(const CTimelineEvent& rOther);

   CTimelineManager* m_pTimelineMgr; // weak reference
   void SetTimelineManager(CTimelineManager* pTimelineMgr);

   EventIDType m_ID; // unique identifier for this event (it is your job to make sure it is unique)

   Float64 m_Day;
   std::_tstring m_Description;

   CConstructSegmentActivity m_ConstructSegments;
   CErectPiersActivity m_ErectPiers;
   CErectSegmentActivity m_ErectSegments;
   CRemoveTemporarySupportsActivity m_RemoveTempSupports;
   CCastClosurePourActivity m_CastClosurePours;
   CCastDeckActivity m_CastDeck;
   CApplyLoadActivity m_ApplyLoads;
   CStressTendonActivity m_StressTendons;

   friend CTimelineManager;

#if defined _DEBUG
   void AssertValid() const;
#endif
};
