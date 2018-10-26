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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\TimelineManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimelineEvent::CTimelineEvent()
{
   m_ID = INVALID_ID;
   m_Day = 0;
   m_Description = _T("Unknown");
   m_pTimelineMgr = NULL;
}

CTimelineEvent::CTimelineEvent(const CTimelineEvent& rOther)
{
   m_pTimelineMgr = NULL;
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
   return m_Day < rOther.m_Day;
}

bool CTimelineEvent::operator==(const CTimelineEvent& rOther) const
{
   if ( m_Day != rOther.m_Day )
      return false;

   if ( m_Description != rOther.m_Description )
      return false;

   if ( m_ConstructSegments != rOther.m_ConstructSegments )
      return false;

   if ( m_ErectPiers != rOther.m_ErectPiers )
      return false;

   if ( m_ErectSegments != rOther.m_ErectSegments ) 
      return false;

   if ( m_RemoveTempSupports != rOther.m_RemoveTempSupports )
      return false;

   if ( m_CastClosureJoints != rOther.m_CastClosureJoints )
      return false;

   if ( m_CastDeck != rOther.m_CastDeck )
      return false;

   if ( m_ApplyLoads != rOther.m_ApplyLoads )
      return false;

   if ( m_StressTendons != rOther.m_StressTendons )
      return false;

   return true;
}

bool CTimelineEvent::operator!=(const CTimelineEvent& rOther) const
{
   return !operator==(rOther);
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
      m_pTimelineMgr->Sort();
}

Float64 CTimelineEvent::GetDay() const
{
   return m_Day;
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

Float64 CTimelineEvent::GetMinElapsedTime() const
{
   Float64 elapsedTime = 0;

   if ( m_ConstructSegments.IsEnabled() )
   {
      // the duration of this activity is the time from strand stressing to release
      elapsedTime = Max(elapsedTime,m_ConstructSegments.GetRelaxationTime());
   }

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

   if ( m_CastClosureJoints.IsEnabled() )
   {
      elapsedTime = Max(elapsedTime,m_CastClosureJoints.GetConcreteAgeAtContinuity());
   }


   if ( m_CastDeck.IsEnabled() )
   {
      elapsedTime = Max(elapsedTime,m_CastDeck.GetConcreteAgeAtContinuity());
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

   return elapsedTime;
}

HRESULT CTimelineEvent::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("TimelineEvent"));

      CComVariant var;
      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_ID = VARIANT2ID(var);

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Description"),&var);
      m_Description = OLE2T(var.bstrVal);

      var.vt = VT_I2;
      hr = pStrLoad->get_Property(_T("Day"),&var);
      m_Day = var.iVal;

      hr = m_ConstructSegments.Load(pStrLoad,pProgress);
      hr = m_ErectPiers.Load(pStrLoad,pProgress);
      hr = m_ErectSegments.Load(pStrLoad,pProgress);
      hr = m_CastClosureJoints.Load(pStrLoad,pProgress);
      hr = m_StressTendons.Load(pStrLoad,pProgress);
      hr = m_RemoveTempSupports.Load(pStrLoad,pProgress);
      hr = m_CastDeck.Load(pStrLoad,pProgress);
      hr = m_ApplyLoads.Load(pStrLoad,pProgress);

      hr = pStrLoad->EndUnit();
   }
   catch(HRESULT hResult)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CTimelineEvent::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("TimelineEvent"),1.0);
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

   if ( m_pTimelineMgr )
      m_pTimelineMgr->Sort();
}

void CTimelineEvent::MakeAssignment(const CTimelineEvent& rOther)
{
   MakeCopy(rOther);
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
