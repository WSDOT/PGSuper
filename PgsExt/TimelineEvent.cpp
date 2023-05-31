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
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\TimelineManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define AS_APPLY_LOADS                  0x0000
#define AS_ERECT_PIERS                  0x0001
#define AS_CONSTRUCT_SEGMENTS           0x0002
#define AS_ERECT_SEGMENTS               0x0004
#define AS_CAST_CLOSURE_JOINTS          0x0005
#define AS_CAST_LONGIUTIDINAL_JOINTS    0x0010
#define AS_CAST_DIAPHRAGMS              0x0020
#define AS_CAST_DECK                    0x0040
#define AS_STRESS_TENDONS_AND_REMOVE_TEMP_SUPPORTS 0x0080
#define AS_STRESS_TENDONS               0x0100
#define AS_REMOVE_TEMP_SUPPORTS         0x0200

CTimelineEvent::CTimelineEvent() :
m_ConstructSegments(this),
m_ErectSegments(this)
{
   m_ID = INVALID_ID;
   m_Day = 0;
   m_Description = _T("Unknown");
   m_pTimelineMgr = nullptr;
}

CTimelineEvent::CTimelineEvent(const CTimelineEvent& rOther) :
   m_ConstructSegments(this),
   m_ErectSegments(this)
{
   m_pTimelineMgr = nullptr;
   m_ID = INVALID_ID;
   MakeCopy(rOther);
}

CTimelineEvent::~CTimelineEvent()
{
}

CTimelineEvent& CTimelineEvent::operator= (const CTimelineEvent& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CTimelineEvent::operator<(const CTimelineEvent& rOther) const
{
   if ( m_Day < rOther.m_Day )
   {
      return true;
   }

   if ( m_Day == rOther.m_Day )
   {
      // occur on the same day... check the activities. some activities logically
      // occur before others. 
      if ( GetActivityScore() < rOther.GetActivityScore() )
      {
         return true;
      }
      else if ( rOther.GetActivityScore() < GetActivityScore() )
      {
         return false;
      }

      ATLASSERT(GetActivityScore() == rOther.GetActivityScore());

      // tendons are being stressed... lower index tendons come first
      if (GetActivityScore() == AS_STRESS_TENDONS)
      {
         DuctIndexType myDuctIdx = INVALID_INDEX;
         const auto& myTendons = GetStressTendonActivity().GetTendons();
         for (const auto& tendon : myTendons)
         {
            myDuctIdx = Min(myDuctIdx, tendon.ductIdx);
         }

         const auto& yourTendons = rOther.GetStressTendonActivity().GetTendons();
         DuctIndexType yourDuctIdx = INVALID_INDEX;
         for (const auto& tendon : yourTendons)
         {
            yourDuctIdx = Min(yourDuctIdx, tendon.ductIdx);
         }

         if (myDuctIdx < yourDuctIdx)
         {
            return true;
         }
         else if (yourDuctIdx < myDuctIdx)
         {
            return false;
         }
      }

      // occur on the same day... the one with the shorter duration comes first
      if ( GetMinElapsedTime() < rOther.GetMinElapsedTime() )
      {
         return true;
      }

      // if same day and same duration... use the ID as a last-ditch tie breaker
      if ( IsEqual(GetMinElapsedTime(),rOther.GetMinElapsedTime()) )
      {
         if ( m_ID < rOther.m_ID )
         {
            return true;
         }
      }
   }

   return false;
}

bool CTimelineEvent::operator==(const CTimelineEvent& rOther) const
{
   if ( m_Day != rOther.m_Day )
   {
      return false;
   }

   if ( m_Description != rOther.m_Description )
   {
      return false;
   }

   if ( m_ConstructSegments != rOther.m_ConstructSegments )
   {
      return false;
   }

   if ( m_ErectPiers != rOther.m_ErectPiers )
   {
      return false;
   }

   if ( m_ErectSegments != rOther.m_ErectSegments ) 
   {
      return false;
   }

   if ( m_RemoveTempSupports != rOther.m_RemoveTempSupports )
   {
      return false;
   }

   if ( m_CastClosureJoints != rOther.m_CastClosureJoints )
   {
      return false;
   }

   if ( m_CastDeck != rOther.m_CastDeck )
   {
      return false;
   }

   if (m_CastLongitudinalJoints != rOther.m_CastLongitudinalJoints)
   {
      return false;
   }

   if ( m_ApplyLoads != rOther.m_ApplyLoads )
   {
      return false;
   }

   if (m_GeometryControl != rOther.m_GeometryControl)
   {
      return false;
   }

   if ( m_StressTendons != rOther.m_StressTendons )
   {
      return false;
   }

   return true;
}

bool CTimelineEvent::operator!=(const CTimelineEvent& rOther) const
{
   return !operator==(rOther);
}

void CTimelineEvent::ClearCaches()
{
   // only the uncommented activities have caches... we could have do nothing
   // ClearCaches methods on all activities, but it would be unnecessary overhead
   // to call them to do nothing
   //m_ApplyLoads.ClearCaches();
   m_ErectPiers.ClearCaches();
   //m_ConstructSegments.ClearCaches();
   //m_ErectSegments.ClearCaches();
   m_CastClosureJoints.ClearCaches();
   //m_CastDeck.ClearCaches();
   //m_StressTendons.ClearCaches();
   //m_RemoveTempSupports.ClearCaches();
   //m_CastLongitudinalJoints.ClearCaches();
}

void CTimelineEvent::SetTimelineManager(CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = pTimelineMgr;
}

CTimelineManager* CTimelineEvent::GetTimelineManager()
{
   return m_pTimelineMgr;
}

const CTimelineManager* CTimelineEvent::GetTimelineManager() const
{
   return m_pTimelineMgr;
}

void CTimelineEvent::SetID(EventIDType id)
{
   m_ID = id;
}

EventIDType CTimelineEvent::GetID() const
{
   return m_ID;
}

void CTimelineEvent::SetDescription(LPCTSTR description)
{
   m_Description = description;
}

LPCTSTR CTimelineEvent::GetDescription() const
{
   return m_Description.c_str();
}

void CTimelineEvent::SetDay(Float64 day)
{
   m_Day = day;
   if ( m_pTimelineMgr )
   {
      m_pTimelineMgr->Sort();
   }
}

Float64 CTimelineEvent::GetDay() const
{
   return m_Day;
}

Float64 CTimelineEvent::GetDuration() const
{
   Float64 duration = 0;

   // Construct Segments
   if ( m_ConstructSegments.IsEnabled() )
   {
      duration = Max(duration,m_ConstructSegments.GetRelaxationTime());
   }

   // Erect Piers (zero duration)
   // Erect Segments (zero duration)
   // Remove Temp Supports (zero duration)

   if ( m_CastClosureJoints.IsEnabled() )
   {
      duration = Max(duration,m_CastClosureJoints.GetTotalCuringDuration());
   }

   if ( m_CastDeck.IsEnabled() )
   {
      duration = Max(duration,m_CastDeck.GetTotalCuringDuration());
   }

   if (m_CastLongitudinalJoints.IsEnabled())
   {
      duration = Max(duration, m_CastLongitudinalJoints.GetTotalCuringDuration());
   }

   // Apply Loads ( zero duration )
   // Stress Tendonds ( zero duration )

   return duration;
}

void CTimelineEvent::SetConstructSegmentsActivity(const CConstructSegmentActivity& activity)
{
   m_ConstructSegments = activity;
}

const CConstructSegmentActivity& CTimelineEvent::GetConstructSegmentsActivity() const
{
   return m_ConstructSegments;
}

CConstructSegmentActivity& CTimelineEvent::GetConstructSegmentsActivity()
{
   return m_ConstructSegments;
}

void CTimelineEvent::SetErectPiersActivity(const CErectPiersActivity& activity)
{
   m_ErectPiers = activity;
}

const CErectPiersActivity& CTimelineEvent::GetErectPiersActivity() const
{
   return m_ErectPiers;
}

CErectPiersActivity& CTimelineEvent::GetErectPiersActivity()
{
   return m_ErectPiers;
}

void CTimelineEvent::SetErectSegmentsActivity(const CErectSegmentActivity& activity)
{
   m_ErectSegments = activity;
}

const CErectSegmentActivity& CTimelineEvent::GetErectSegmentsActivity() const
{
   return m_ErectSegments;
}

CErectSegmentActivity& CTimelineEvent::GetErectSegmentsActivity()
{
   return m_ErectSegments;
}

void CTimelineEvent::SetRemoveTempSupportsActivity(const CRemoveTemporarySupportsActivity& activity)
{
   m_RemoveTempSupports = activity;
}

const CRemoveTemporarySupportsActivity& CTimelineEvent::GetRemoveTempSupportsActivity() const
{
   return m_RemoveTempSupports;
}

CRemoveTemporarySupportsActivity& CTimelineEvent::GetRemoveTempSupportsActivity()
{
   return m_RemoveTempSupports;
}

void CTimelineEvent::SetCastClosureJointActivity(const CCastClosureJointActivity& activity)
{
   m_CastClosureJoints = activity;
}

const CCastClosureJointActivity& CTimelineEvent::GetCastClosureJointActivity() const
{
   return m_CastClosureJoints;
}

CCastClosureJointActivity& CTimelineEvent::GetCastClosureJointActivity()
{
   return m_CastClosureJoints;
}

void CTimelineEvent::SetCastDeckActivity(const CCastDeckActivity& activity)
{
   m_CastDeck = activity;
}

const CCastDeckActivity& CTimelineEvent::GetCastDeckActivity() const
{
   return m_CastDeck;
}

CCastDeckActivity& CTimelineEvent::GetCastDeckActivity()
{
   return m_CastDeck;
}

void CTimelineEvent::SetCastLongitudinalJointActivity(const CCastLongitudinalJointActivity& activity)
{
   m_CastLongitudinalJoints = activity;
}

const CCastLongitudinalJointActivity& CTimelineEvent::GetCastLongitudinalJointActivity() const
{
   return m_CastLongitudinalJoints;
}

CCastLongitudinalJointActivity& CTimelineEvent::GetCastLongitudinalJointActivity()
{
   return m_CastLongitudinalJoints;
}

void CTimelineEvent::SetStressTendonActivity(const CStressTendonActivity& activity)
{
   m_StressTendons = activity;
}

const CStressTendonActivity& CTimelineEvent::GetStressTendonActivity() const
{
   return m_StressTendons;
}

CStressTendonActivity& CTimelineEvent::GetStressTendonActivity()
{
   return m_StressTendons;
}

void CTimelineEvent::SetApplyLoadActivity(const CApplyLoadActivity& activity)
{
   m_ApplyLoads = activity;
}

const CApplyLoadActivity& CTimelineEvent::GetApplyLoadActivity() const
{
   return m_ApplyLoads;
}

CApplyLoadActivity& CTimelineEvent::GetApplyLoadActivity()
{
   return m_ApplyLoads;
}

void CTimelineEvent::SetGeometryControlActivity(const CGeometryControlActivity& activity)
{
   m_GeometryControl = activity;
}

const CGeometryControlActivity& CTimelineEvent::GetGeometryControlActivity() const
{
   return m_GeometryControl;
}

CGeometryControlActivity& CTimelineEvent::GetGeometryControlActivity()
{
   return m_GeometryControl;
}

Float64 CTimelineEvent::GetMinElapsedTime() const
{
   // The minimum elapsed time for this event is
   // the greatest of the elapsed time of all the activities in this event
   Float64 elapsedTime = 0;

   if (m_ConstructSegments.IsEnabled())
   {
      // the duration of this activity is the time from strand stressing to release
      elapsedTime = Max(elapsedTime, m_ConstructSegments.GetRelaxationTime());
   }

   // Commented out code below are the zero duration activities... no need to explicitly check them

   //if ( m_ErectPiers.IsEnabled() )
   //{
   //   // zero duration activity
   //   elapsedTime += 0;
   //}

   //if ( m_ErectSegments.IsEnabled() )
   //{
   //   // zero duration activity
   //   elapsedTime += 0;
   //}

   //if ( m_RemoveTempSupports.IsEnabled() )
   //{
   //   // zero duration activity
   //   elapsedTime += 0;
   //}

   if (m_CastClosureJoints.IsEnabled() && !m_CastDeck.IsEnabled())
   {
      elapsedTime = Max(elapsedTime, m_CastClosureJoints.GetTotalCuringDuration());
   }
   else if (m_CastDeck.IsEnabled() && !m_CastClosureJoints.IsEnabled())
   {
      elapsedTime = Max(elapsedTime, m_CastDeck.GetDuration());
   }
   else if (m_CastClosureJoints.IsEnabled() && m_CastDeck.IsEnabled())
   {
      Float64 deck_duration = m_CastDeck.GetDuration();
      Float64 cj_duration = m_CastDeck.GetTimeOfClosureJointCasting() + m_CastClosureJoints.GetTotalCuringDuration();
      elapsedTime = Max(elapsedTime, deck_duration, cj_duration);
   }

   if (m_CastLongitudinalJoints.IsEnabled())
   {
      elapsedTime = Max(elapsedTime, m_CastLongitudinalJoints.GetTotalCuringDuration());
   }

   //if ( m_ApplyLoads.IsEnabled() )
   //{
   //   // zero duration activity
   //   elapsedTime += 0;
   //}

   //if ( m_StressTendons.IsEnabled() )
   //{
   //   // zero duration activity
   //   elapsedTime += 0;
   //}

   ATLASSERT(0 <= elapsedTime);

   return elapsedTime;
}

HRESULT CTimelineEvent::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("TimelineEvent"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_ID = VARIANT2ID(var);

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Description"),&var);
      m_Description = OLE2T(var.bstrVal);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Day"),&var);
      m_Day = var.dblVal;

      hr = m_ConstructSegments.Load(pStrLoad,pProgress);
      hr = m_ErectPiers.Load(pStrLoad,pProgress);
      hr = m_ErectSegments.Load(pStrLoad,pProgress);
      hr = m_CastClosureJoints.Load(pStrLoad,pProgress);
      hr = m_StressTendons.Load(pStrLoad,pProgress);
      hr = m_RemoveTempSupports.Load(pStrLoad,pProgress);
      hr = m_CastDeck.Load(pStrLoad,pProgress);
      hr = m_ApplyLoads.Load(pStrLoad,pProgress);

      if (2 < version)
      {
         hr = m_GeometryControl.Load(pStrLoad,pProgress); // added in version 3.0
      }
      else if (m_ApplyLoads.IsLiveLoadApplied())
      {
         // By default set the live load application event to when roadway geometry is controlled.
         m_GeometryControl.SetGeometryControlEventType(pgsTypes::gcaGeometryControlEvent);
      }

      // DeckCast activity, datablock version 4 added the closure joint casting region. However, the region cannot be resolved
      // in the cast deck activity object because it doesn't know if a closure joint is added at the same time. The casting region
      // is INVALID_INDEX if it was never set. INVALID_INDEX also means there is not a closure joint cast with the deck.
      if (m_CastClosureJoints.IsEnabled() && m_CastDeck.IsEnabled() && m_CastDeck.GetClosureJointCastingRegion() == INVALID_INDEX)
      {
         // A closure joint is cast with the deck and the casting region was not specified. The default behavior is the closure joint
         // is cast with the first deck region casting. Set the closure joint casting region here.
         m_CastDeck.SetClosureJointCastingRegion(0);
      }
      
      if (1 < version)
      {
         // added in version 2
         hr = m_CastLongitudinalJoints.Load(pStrLoad, pProgress);
      }

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CTimelineEvent::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("TimelineEvent"),3.0);
   pStrSave->put_Property(_T("ID"),CComVariant(m_ID));
   pStrSave->put_Property(_T("Description"),CComVariant(m_Description.c_str()));
   pStrSave->put_Property(_T("Day"),CComVariant(m_Day));
   
   m_ConstructSegments.Save(pStrSave,pProgress);
   m_ErectPiers.Save(pStrSave,pProgress);
   m_ErectSegments.Save(pStrSave,pProgress);
   m_CastClosureJoints.Save(pStrSave,pProgress);
   m_StressTendons.Save(pStrSave,pProgress);
   m_RemoveTempSupports.Save(pStrSave,pProgress);
   m_CastDeck.Save(pStrSave,pProgress);
   m_ApplyLoads.Save(pStrSave,pProgress);
   m_GeometryControl.Save(pStrSave,pProgress);

   m_CastLongitudinalJoints.Save(pStrSave, pProgress); // added in version 2

   pStrSave->EndUnit();

   return S_OK;
}

void CTimelineEvent::MakeCopy(const CTimelineEvent& rOther)
{
   m_Description    = rOther.m_Description;
   m_Day            = rOther.m_Day;

   m_ConstructSegments  = rOther.m_ConstructSegments;
   m_ErectPiers         = rOther.m_ErectPiers;
   m_ErectSegments      = rOther.m_ErectSegments;
   m_CastClosureJoints   = rOther.m_CastClosureJoints;
   m_RemoveTempSupports = rOther.m_RemoveTempSupports;
   m_CastDeck           = rOther.m_CastDeck;
   m_ApplyLoads         = rOther.m_ApplyLoads;
   m_StressTendons      = rOther.m_StressTendons;
   m_CastLongitudinalJoints = rOther.m_CastLongitudinalJoints;
   m_GeometryControl    = rOther.m_GeometryControl;

   if ( m_pTimelineMgr )
   {
      m_pTimelineMgr->Sort();
   }
}

void CTimelineEvent::MakeAssignment(const CTimelineEvent& rOther)
{
   MakeCopy(rOther);
}

// Scores for activities to define sort order when two timeline events
// occur on the same day. Certain activities will take place before others.
// For example, if two events occur on the same day and one event has
// cast closure joints and the other event has cast closure joints and cast deck
// the event with cast closure joints only will come first in the timeline
// because deck casting must come after closure joint casting.
Uint16 CTimelineEvent::GetActivityScore() const
{
   Uint16 activityScore = 0;

   if ( m_ApplyLoads.IsEnabled() )
   {
      if (m_ApplyLoads.IsIntermediateDiaphragmLoadApplied())
      {
         activityScore |= AS_CAST_DIAPHRAGMS;
      }
      else
      {
         activityScore |= AS_APPLY_LOADS;
      }
   }

   if ( m_ErectPiers.IsEnabled() )
   {
      activityScore |= AS_ERECT_PIERS;
   }

   if ( m_ConstructSegments.IsEnabled() )
   {
      activityScore |= AS_CONSTRUCT_SEGMENTS;
   }

   if ( m_ErectSegments.IsEnabled() )
   {
      activityScore |= AS_ERECT_SEGMENTS;
   }

   if ( m_CastClosureJoints.IsEnabled() )
   {
      activityScore |= AS_CAST_CLOSURE_JOINTS;
   }

   if (m_CastLongitudinalJoints.IsEnabled())
   {
      activityScore |= AS_CAST_LONGIUTIDINAL_JOINTS;
   }

   if ( m_CastDeck.IsEnabled() )
   {
      activityScore |= AS_CAST_DECK;
   }

   if (m_StressTendons.IsEnabled() && m_RemoveTempSupports.IsEnabled())
   {
      activityScore |= AS_STRESS_TENDONS_AND_REMOVE_TEMP_SUPPORTS;
   }
   else
   {
      if (m_StressTendons.IsEnabled())
      {
         activityScore |= AS_STRESS_TENDONS;
      }

      if (m_RemoveTempSupports.IsEnabled())
      {
         activityScore |= AS_REMOVE_TEMP_SUPPORTS;
      }
   }

   return activityScore;
}

#if defined _DEBUG
void CTimelineEvent::AssertValid() const
{
   //m_ConstructSegments.AssertValid();
   //m_ErectPiers.AssertValid();
   //m_ErectSegments.AssertValid();
   //m_CastClosureJoints.AssertValid();
   //m_RemoveTempSupports.AssertValid();
   //m_CastDeck.AssertValid();
   //m_ApplyLoads.AssertValid();
   //m_StressTendons.AssertValid();
}
#endif
