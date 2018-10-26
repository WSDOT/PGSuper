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
      return false;

   if (m_Piers != rOther.m_Piers )
      return false;

   if ( m_TempSupports != rOther.m_TempSupports )
      return false;

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
}

void CSupportActivityBase::AddPier(PierIDType pierID)
{
   m_Piers.insert(pierID);
}

void CSupportActivityBase::AddPiers(const std::vector<PierIDType>& piers)
{
   m_Piers.insert(piers.begin(),piers.end());
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
}

IndexType CSupportActivityBase::GetPierCount() const
{
   return m_Piers.size();
}

void CSupportActivityBase::AddTempSupport(SupportIDType tsID)
{
   m_TempSupports.insert(tsID);
}

void CSupportActivityBase::AddTempSupports(const std::vector<SupportIDType>& tempSupports)
{
   m_TempSupports.insert(tempSupports.begin(),tempSupports.end());
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
   catch(CHRException& exception)
   {
      ATLASSERT(false);
      return exception;
   };

   return S_OK;
}

HRESULT CSupportActivityBase::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(GetUnitName(),1.0);
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
// CCastClosurePourActivity
////////////////////////////////////////////////////////////////
CCastClosurePourActivity::CCastClosurePourActivity()
{
   m_Age = 7.0; // days
}

CCastClosurePourActivity::CCastClosurePourActivity(const CCastClosurePourActivity& rOther) :
CSupportActivityBase(rOther)
{
   MakeCopy(rOther);
}

CCastClosurePourActivity& CCastClosurePourActivity::operator= (const CCastClosurePourActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CCastClosurePourActivity::operator==(const CCastClosurePourActivity& rOther) const
{
   if ( CSupportActivityBase::operator !=(rOther) )
      return false;

   if ( !IsEqual(m_Age,rOther.m_Age) )
      return false;

   return true;
}

bool CCastClosurePourActivity::operator!=(const CCastClosurePourActivity& rOther) const
{
   return !operator==(rOther);
}

Float64 CCastClosurePourActivity::GetConcreteAgeAtContinuity() const
{
   return m_Age;
}

void CCastClosurePourActivity::SetConcreteAgeAtContinuity(Float64 age)
{
   m_Age = age;
}

void CCastClosurePourActivity::MakeCopy(const CCastClosurePourActivity& rOther)
{
   m_Age = rOther.m_Age;
}

void CCastClosurePourActivity::MakeAssignment(const CCastClosurePourActivity& rOther)
{
   CSupportActivityBase::MakeAssignment(rOther);
   MakeCopy(rOther);
}

HRESULT CCastClosurePourActivity::LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   if ( m_bEnabled )
   {
      CComVariant var;
      var.vt = VT_R8;
      HRESULT hr = pStrLoad->get_Property(_T("AgeAtContinuity"),&var);
      if ( FAILED(hr) )
         return hr;

      m_Age = var.dblVal;
   }
   return S_OK;
}

HRESULT CCastClosurePourActivity::SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress)
{
   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("AgeAtContinuity"),CComVariant(m_Age));
   }

   return S_OK;
}

