///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\ErectPiersActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSupportActivityBase::CSupportActivityBase()
{
   m_bEnabled = false;
}

CSupportActivityBase::CSupportActivityBase(const CSupportActivityBase& rOther)
{
   MakeCopy(rOther);
}

CSupportActivityBase::~CSupportActivityBase()
{
}

CSupportActivityBase& CSupportActivityBase::operator= (const CSupportActivityBase& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CSupportActivityBase::operator==(const CSupportActivityBase& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
   {
      return false;
   }

   if (m_Piers != rOther.m_Piers )
   {
      return false;
   }

   if ( m_TempSupports != rOther.m_TempSupports )
   {
      return false;
   }

   return true;
}

bool CSupportActivityBase::operator!=(const CSupportActivityBase& rOther) const
{
   return !operator==(rOther);
}

void CSupportActivityBase::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CSupportActivityBase::IsEnabled() const
{
   return m_bEnabled;
}

void CSupportActivityBase::Clear()
{
   m_Piers.clear();
   m_TempSupports.clear();
   m_bEnabled = false;
}

void CSupportActivityBase::AddPier(PierIDType pierID)
{
   m_Piers.insert(pierID);
   m_bEnabled = true;
}

void CSupportActivityBase::AddPiers(const std::vector<PierIDType>& piers)
{
   m_Piers.insert(piers.begin(),piers.end());
   m_bEnabled = true;
}

const std::set<PierIDType>& CSupportActivityBase::GetPiers() const
{
   return m_Piers;
}

bool CSupportActivityBase::HasPier(PierIDType pierID) const
{
   std::set<PierIDType>::const_iterator found( m_Piers.find(pierID) );
   return ( found != m_Piers.end() );
}

void CSupportActivityBase::RemovePier(PierIDType pierID)
{
   std::set<PierIDType>::iterator found( m_Piers.find(pierID) );
   if ( found != m_Piers.end() )
   {
      m_Piers.erase(found);
   }

   if ( m_TempSupports.size() == 0 && m_Piers.size() == 0 )
   {
      m_bEnabled = false;
   }
}

IndexType CSupportActivityBase::GetPierCount() const
{
   return m_Piers.size();
}

void CSupportActivityBase::AddTempSupport(SupportIDType tsID)
{
   m_TempSupports.insert(tsID);
   m_bEnabled = true;
}

void CSupportActivityBase::AddTempSupports(const std::vector<SupportIDType>& tempSupports)
{
   m_TempSupports.insert(tempSupports.begin(),tempSupports.end());
   m_bEnabled = true;
}

const std::set<SupportIDType>& CSupportActivityBase::GetTempSupports() const
{
   return m_TempSupports;
}

bool CSupportActivityBase::HasTempSupport(SupportIDType tsID) const
{
   std::set<SupportIDType>::const_iterator found( m_TempSupports.find(tsID) );
   return ( found != m_TempSupports.end() );
}

void CSupportActivityBase::RemoveTempSupport(SupportIDType tsID)
{
   std::set<SupportIDType>::const_iterator found( m_TempSupports.find(tsID) );
   if ( found != m_TempSupports.end() )
   {
      m_TempSupports.erase(found);
   }

   if ( m_TempSupports.size() == 0 && m_Piers.size() == 0 )
   {
      m_bEnabled = false;
   }
}

IndexType CSupportActivityBase::GetTemporarySupportCount() const
{
   return m_TempSupports.size();
}

HRESULT CSupportActivityBase::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
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
         hr = pStrLoad->BeginUnit(_T("Piers"));

         var.vt = VT_INDEX;
         CollectionIndexType N;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         N = VARIANT2INDEX(var);
         var.vt = VT_ID;
         for ( CollectionIndexType i = 0; i < N; i++ )
         {
            hr = pStrLoad->get_Property(_T("PierID"),&var);
            m_Piers.insert( VARIANT2ID(var) );
         }

         hr = pStrLoad->EndUnit();

         hr = pStrLoad->BeginUnit(_T("TempSupports"));
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         N = VARIANT2INDEX(var);
         var.vt = VT_ID;
         for ( CollectionIndexType i = 0; i < N; i++ )
         {
            hr = pStrLoad->get_Property(_T("TempSupportID"),&var);
            m_TempSupports.insert(VARIANT2ID(var));
         }

         hr = pStrLoad->EndUnit();
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

HRESULT CSupportActivityBase::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(GetUnitName(),GetUnitVersion());
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->BeginUnit(_T("Piers"),1.0);
      
      pStrSave->put_Property(_T("Count"),CComVariant(m_Piers.size()));

      std::set<PierIDType>::iterator pierIter(m_Piers.begin());
      std::set<PierIDType>::iterator pierIterEnd(m_Piers.end());
      for ( ; pierIter != pierIterEnd; pierIter++ )
      {
         pStrSave->put_Property(_T("PierID"),CComVariant(*pierIter));
      }
      pStrSave->EndUnit();

      pStrSave->BeginUnit(_T("TempSupports"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_TempSupports.size()));

      std::set<SupportIDType>::iterator tsIter(m_TempSupports.begin());
      std::set<SupportIDType>::iterator tsIterEnd(m_TempSupports.end());
      for ( ; tsIter != tsIterEnd; tsIter++ )
      {
         pStrSave->put_Property(_T("TempSupportID"),CComVariant(*tsIter));
      }
      pStrSave->EndUnit();
   }

   SaveSubclassData(pStrSave,pProgress);

   pStrSave->EndUnit();

   return S_OK;
}

HRESULT CSupportActivityBase::LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   // does nothing by default
   return S_OK;
}

HRESULT CSupportActivityBase::SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress)
{
   // does nothing by default
   return S_OK;
}

void CSupportActivityBase::MakeCopy(const CSupportActivityBase& rOther)
{
   m_bEnabled     = rOther.m_bEnabled;
   m_Piers        = rOther.m_Piers;
   m_TempSupports = rOther.m_TempSupports;
}

void CSupportActivityBase::MakeAssignment(const CSupportActivityBase& rOther)
{
   MakeCopy(rOther);
}


////////////////////////////////////////////////////////////////
// CCastClosureJointActivity
////////////////////////////////////////////////////////////////
CCastClosureJointActivity::CCastClosureJointActivity()
{
   m_Age = 7.0; // days
   m_CuringDuration = 3.0; // days (WSDOT Std Specs 6-02.3(11)
}

CCastClosureJointActivity::CCastClosureJointActivity(const CCastClosureJointActivity& rOther) :
CSupportActivityBase(rOther)
{
   MakeCopy(rOther);
}

CCastClosureJointActivity& CCastClosureJointActivity::operator= (const CCastClosureJointActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CCastClosureJointActivity::operator==(const CCastClosureJointActivity& rOther) const
{
   if ( CSupportActivityBase::operator !=(rOther) )
   {
      return false;
   }

   if ( !IsEqual(m_Age,rOther.m_Age) )
   {
      return false;
   }

   if ( !IsEqual(m_CuringDuration,rOther.m_CuringDuration) )
   {
      return false;
   }

   return true;
}

bool CCastClosureJointActivity::operator!=(const CCastClosureJointActivity& rOther) const
{
   return !operator==(rOther);
}

Float64 CCastClosureJointActivity::GetConcreteAgeAtContinuity() const
{
   return m_Age;
}

void CCastClosureJointActivity::SetConcreteAgeAtContinuity(Float64 age)
{
   m_Age = age;
}

void CCastClosureJointActivity::SetCuringDuration(Float64 duration)
{
   m_CuringDuration = duration;
}

Float64 CCastClosureJointActivity::GetCuringDuration() const
{
   return m_CuringDuration;
}

void CCastClosureJointActivity::MakeCopy(const CCastClosureJointActivity& rOther)
{
   m_Age = rOther.m_Age;
   m_CuringDuration = rOther.m_CuringDuration;
}

void CCastClosureJointActivity::MakeAssignment(const CCastClosureJointActivity& rOther)
{
   CSupportActivityBase::MakeAssignment(rOther);
   MakeCopy(rOther);
}

HRESULT CCastClosureJointActivity::LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   if ( m_bEnabled )
   {
      CComVariant var;
      var.vt = VT_R8;
      HRESULT hr = pStrLoad->get_Property(_T("AgeAtContinuity"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      m_Age = var.dblVal;

      Float64 version;
      pStrLoad->get_Version(&version);
      if ( 1 < version )
      {
         // added in version
         var.vt = VT_R8;
         HRESULT hr = pStrLoad->get_Property(_T("CuringDuration"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         m_CuringDuration = var.dblVal;
      }
      else
      {
         m_CuringDuration = m_Age;
      }
   }
   return S_OK;
}

HRESULT CCastClosureJointActivity::SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress)
{
   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("AgeAtContinuity"),CComVariant(m_Age));
      pStrSave->put_Property(_T("CuringDuration"),CComVariant(m_CuringDuration));
   }

   return S_OK;
}

