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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSegmentActivityBase::CSegmentActivityBase()
{
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
      return false;

   if (m_Segments != rOther.m_Segments )
      return false;

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
}

void CSegmentActivityBase::AddSegment(SegmentIDType segmentID)
{
   m_Segments.insert(segmentID);
}

void CSegmentActivityBase::AddSegments(const std::set<SegmentIDType>& segments)
{
   m_Segments.insert(segments.begin(),segments.end());
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
}

IndexType CSegmentActivityBase::GetSegmentCount() const
{
   return m_Segments.size();
}


HRESULT CSegmentActivityBase::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
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
         CollectionIndexType nSegments;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         nSegments = VARIANT2INDEX(var);

         for ( CollectionIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
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
   catch(CHRException& exception)
   {
      ATLASSERT(false);
      return exception;
   };

   return S_OK;
}

HRESULT CSegmentActivityBase::Save(IStructuredSave* pStrSave,IProgress* pProgress)
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

HRESULT CSegmentActivityBase::LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   // does nothing by default
   return S_OK;
}

HRESULT CSegmentActivityBase::SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress)
{
   // does nothing by default
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////
// CConstructSegmentActivity
CConstructSegmentActivity::CConstructSegmentActivity()
{
   m_bEnabled = false;
   m_RelaxationTime = 1.0; // day
   m_AgeAtRelease   = 1.0; // day
}

CConstructSegmentActivity::CConstructSegmentActivity(const CConstructSegmentActivity& rOther)
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
   if ( m_bEnabled != rOther.m_bEnabled )
      return false;

   if ( !IsEqual(m_RelaxationTime,rOther.m_RelaxationTime) )
      return false;

   if ( !IsEqual(m_AgeAtRelease,rOther.m_AgeAtRelease) )
      return false;

   return true;
}

bool CConstructSegmentActivity::operator!=(const CConstructSegmentActivity& rOther) const
{
   return !operator==(rOther);
}

void CConstructSegmentActivity::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CConstructSegmentActivity::IsEnabled() const
{
   return m_bEnabled;
}

void CConstructSegmentActivity::SetRelaxationTime(Float64 r)
{
   m_RelaxationTime = r;
}

Float64 CConstructSegmentActivity::GetRelaxationTime() const
{
   return m_RelaxationTime;
}

void CConstructSegmentActivity::SetAgeAtRelease(Float64 age)
{
   m_AgeAtRelease = age;
}

Float64 CConstructSegmentActivity::GetAgeAtRelease() const
{
   return m_AgeAtRelease;
}

void CConstructSegmentActivity::MakeCopy(const CConstructSegmentActivity& rOther)
{
   m_bEnabled       = rOther.m_bEnabled;
   m_RelaxationTime = rOther.m_RelaxationTime;
   m_AgeAtRelease   = rOther.m_AgeAtRelease;
}

void CConstructSegmentActivity::MakeAssignment(const CConstructSegmentActivity& rOther)
{
   MakeCopy(rOther);
}

HRESULT CConstructSegmentActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
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
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("RelaxationTime"),&var);
         m_RelaxationTime = var.dblVal;

         pStrLoad->get_Property(_T("AgeAtRelease"),&var);
         m_AgeAtRelease = var.dblVal;
      } 

      hr = pStrLoad->EndUnit();
   }
   catch(CHRException& exception)
   {
      ATLASSERT(false);
      return exception;
   };

   return S_OK;
}

HRESULT CConstructSegmentActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(GetUnitName(),1.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));
   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("RelaxationTime"),CComVariant(m_RelaxationTime));
      pStrSave->put_Property(_T("AgeAtRelease"),CComVariant(m_AgeAtRelease));
   }
   pStrSave->EndUnit();

   return S_OK;
}
