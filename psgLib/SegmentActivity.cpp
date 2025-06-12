///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <PsgLib\SegmentActivity.h>
#include <PsgLib\TimelineEvent.h>
#include <PsgLib\TimelineManager.h>


CSegmentActivityBase::CSegmentActivityBase(CTimelineEvent* pTimelineEvent)
{
   m_pTimelineEvent = pTimelineEvent;
   m_bEnabled = false;
}

CSegmentActivityBase::CSegmentActivityBase(const CSegmentActivityBase& rOther)
{
   MakeCopy(rOther);
}

CSegmentActivityBase::~CSegmentActivityBase()
{
}

CSegmentActivityBase& CSegmentActivityBase::operator= (const CSegmentActivityBase& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CSegmentActivityBase::operator==(const CSegmentActivityBase& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
   {
      return false;
   }

   if (m_Segments != rOther.m_Segments )
   {
      return false;
   }

   return true;
}

bool CSegmentActivityBase::operator!=(const CSegmentActivityBase& rOther) const
{
   return !operator==(rOther);
}

void CSegmentActivityBase::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CSegmentActivityBase::IsEnabled() const
{
   return m_bEnabled;
}

void CSegmentActivityBase::Clear()
{
   m_Segments.clear();
   m_bEnabled = false;
}

void CSegmentActivityBase::AddSegment(SegmentIDType segmentID)
{
   m_Segments.insert(segmentID);
   m_bEnabled = true;
   Update();
}

void CSegmentActivityBase::AddSegments(const std::set<SegmentIDType>& segments)
{
   m_Segments.insert(segments.begin(),segments.end());
   m_bEnabled = true;
   Update();
}

const std::set<SegmentIDType>& CSegmentActivityBase::GetSegments() const
{
   return m_Segments;
}

bool CSegmentActivityBase::HasSegment(SegmentIDType segmentID) const
{
   std::set<SegmentIDType>::const_iterator found = m_Segments.find(segmentID);
   return ( found == m_Segments.end() ? false : true );
}

void CSegmentActivityBase::RemoveSegment(SegmentIDType segmentID)
{
   std::set<SegmentIDType>::iterator found = m_Segments.find(segmentID);
   if ( found != m_Segments.end() )
   {
      m_Segments.erase(found);
   }

   if ( m_Segments.size() == 0 )
   {
      m_bEnabled = false;
   }
   Update();
}

IndexType CSegmentActivityBase::GetSegmentCount() const
{
   return m_Segments.size();
}


HRESULT CSegmentActivityBase::Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(GetUnitName());

      CComVariant var;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Enabled"),&var);
      m_bEnabled = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bEnabled )
      {
         hr = pStrLoad->BeginUnit(_T("Segments"));

         var.vt = VT_INDEX;
         IndexType nSegments;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         nSegments = VARIANT2INDEX(var);

         for ( IndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            pStrLoad->BeginUnit(_T("Segment"));

            var.vt = VT_ID;
            pStrLoad->get_Property(_T("ID"),&var);
            SegmentIDType segID = VARIANT2ID(var);
            m_Segments.insert(segID);

            pStrLoad->EndUnit(); // Segment
         }

         hr = pStrLoad->EndUnit(); // Segments
      } 

      hr = LoadSubclassData(pStrLoad,pProgress);

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CSegmentActivityBase::Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   pStrSave->BeginUnit(GetUnitName(),1.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->BeginUnit(_T("Segments"),1.0);
      
      pStrSave->put_Property(_T("Count"),CComVariant(m_Segments.size()));

      std::set<SegmentIDType>::iterator iter(m_Segments.begin());
      std::set<SegmentIDType>::iterator end(m_Segments.end());
      for ( ; iter != end; iter++ )
      {
         pStrSave->BeginUnit(_T("Segment"),1.0);
         pStrSave->put_Property(_T("ID"),CComVariant(*iter));
         pStrSave->EndUnit(); // Segment
      }
      pStrSave->EndUnit(); // Segments
   }

   SaveSubclassData(pStrSave,pProgress);

   pStrSave->EndUnit(); // UnitName

   return S_OK;
}

void CSegmentActivityBase::MakeCopy(const CSegmentActivityBase& rOther)
{
   m_bEnabled     = rOther.m_bEnabled;
   m_Segments     = rOther.m_Segments;
}

void CSegmentActivityBase::MakeAssignment(const CSegmentActivityBase& rOther)
{
   MakeCopy(rOther);
}

HRESULT CSegmentActivityBase::LoadSubclassData(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
{
   // does nothing by default
   return S_OK;
}

HRESULT CSegmentActivityBase::SaveSubclassData(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   // does nothing by default
   return S_OK;
}

void CSegmentActivityBase::Update()
{
   // There are caches of segment keys in the timeline manager. When the number of segments change,
   // the caches become invalid. This call tells the timeline manager to hit every timeline event
   // and clear its caches
   auto* pTimelineMgr = m_pTimelineEvent->GetTimelineManager();
   if (pTimelineMgr)
   {
      pTimelineMgr->ClearCaches();
   }
}

//////////////////////////////////////////////////////////////////////////////////
// CConstructSegmentActivity
CConstructSegmentActivity::CConstructSegmentActivity(CTimelineEvent* pTimelineEvent) : CSegmentActivityBase(pTimelineEvent)
{
   m_RelaxationTime = 1.0; // day
   m_TotalCuringDuration = 1.0; // day
}

CConstructSegmentActivity::CConstructSegmentActivity(const CConstructSegmentActivity& rOther) :
CSegmentActivityBase(rOther)
{
   MakeCopy(rOther);
}

CConstructSegmentActivity& CConstructSegmentActivity::operator= (const CConstructSegmentActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CConstructSegmentActivity::operator==(const CConstructSegmentActivity& rOther) const
{
   if ( !CSegmentActivityBase::operator ==(rOther) )
   {
      return false;
   }

   if ( !IsEqual(m_RelaxationTime,rOther.m_RelaxationTime) )
   {
      return false;
   }

   if ( !IsEqual(m_TotalCuringDuration,rOther.m_TotalCuringDuration) )
   {
      return false;
   }

   return true;
}

bool CConstructSegmentActivity::operator!=(const CConstructSegmentActivity& rOther) const
{
   return !operator==(rOther);
}

void CConstructSegmentActivity::SetRelaxationTime(Float64 r)
{
   m_RelaxationTime = r;
}

Float64 CConstructSegmentActivity::GetRelaxationTime() const
{
   return m_RelaxationTime;
}

void CConstructSegmentActivity::SetTotalCuringDuration(Float64 duration)
{
   m_TotalCuringDuration = duration;
}

Float64 CConstructSegmentActivity::GetTotalCuringDuration() const
{
   return m_TotalCuringDuration;
}

void CConstructSegmentActivity::MakeCopy(const CConstructSegmentActivity& rOther)
{
   m_RelaxationTime = rOther.m_RelaxationTime;
   m_TotalCuringDuration = rOther.m_TotalCuringDuration;
}

void CConstructSegmentActivity::MakeAssignment(const CConstructSegmentActivity& rOther)
{
   CSegmentActivityBase::MakeAssignment(rOther);
   MakeCopy(rOther);
}

HRESULT CConstructSegmentActivity::LoadSubclassData(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
{
   CHRException hr;

   try
   {
      if ( m_bEnabled )
      {
         CComVariant var;

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("RelaxationTime"),&var);
         m_RelaxationTime = var.dblVal;

         pStrLoad->get_Property(_T("AgeAtRelease"),&var);
         m_TotalCuringDuration = var.dblVal;
      } 
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CConstructSegmentActivity::SaveSubclassData(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("RelaxationTime"),CComVariant(m_RelaxationTime));
      pStrSave->put_Property(_T("AgeAtRelease"),CComVariant(m_TotalCuringDuration));
   }

   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////
// CErectSegmentActivity
CErectSegmentActivity::CErectSegmentActivity(CTimelineEvent* pTimelineEvent) : CSegmentActivityBase(pTimelineEvent)
{
}
