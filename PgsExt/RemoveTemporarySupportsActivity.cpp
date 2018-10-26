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
#include <PgsExt\RemoveTemporarySupportsActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTemporarySupportActivityBase::CTemporarySupportActivityBase()
{
   m_bEnabled = false;
}

CTemporarySupportActivityBase::CTemporarySupportActivityBase(const CTemporarySupportActivityBase& rOther)
{
   MakeCopy(rOther);
}

CTemporarySupportActivityBase::~CTemporarySupportActivityBase()
{
}

CTemporarySupportActivityBase& CTemporarySupportActivityBase::operator= (const CTemporarySupportActivityBase& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CTemporarySupportActivityBase::operator==(const CTemporarySupportActivityBase& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
      return false;

   if ( m_TempSupports != rOther.m_TempSupports )
      return false;

   return true;
}

bool CTemporarySupportActivityBase::operator!=(const CTemporarySupportActivityBase& rOther) const
{
   return !operator==(rOther);
}

void CTemporarySupportActivityBase::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CTemporarySupportActivityBase::IsEnabled() const
{
   return m_bEnabled;
}

void CTemporarySupportActivityBase::Clear()
{
   m_TempSupports.clear();
}

void CTemporarySupportActivityBase::AddTempSupport(SupportIDType tsID)
{
   m_TempSupports.push_back(tsID);
   std::sort(m_TempSupports.begin(),m_TempSupports.end());
}

void CTemporarySupportActivityBase::AddTempSupports(const std::vector<SupportIDType>& tempSupports)
{
   m_TempSupports.insert(m_TempSupports.end(),tempSupports.begin(),tempSupports.end());
   std::sort(m_TempSupports.begin(),m_TempSupports.end());
}

const std::vector<SupportIDType>& CTemporarySupportActivityBase::GetTempSupports() const
{
   return m_TempSupports;
}

bool CTemporarySupportActivityBase::HasTempSupport(SupportIDType tsID) const
{
   std::vector<SupportIDType>::const_iterator found( std::find(m_TempSupports.begin(),m_TempSupports.end(),tsID) );
   return ( found != m_TempSupports.end() );
}

void CTemporarySupportActivityBase::RemoveTempSupport(SupportIDType tsID)
{
   std::vector<SupportIDType>::iterator found( std::find(m_TempSupports.begin(),m_TempSupports.end(),tsID) );
   if ( found != m_TempSupports.end() )
   {
      m_TempSupports.erase(found);
   }
}

IndexType CTemporarySupportActivityBase::GetTempSupportCount() const
{
   return m_TempSupports.size();
}

HRESULT CTemporarySupportActivityBase::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
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
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         SupportIndexType N = VARIANT2INDEX(var);
         var.vt = VT_ID;
         for ( SupportIndexType i = 0; i < N; i++ )
         {
            hr = pStrLoad->get_Property(_T("TempSupportID"),&var);
            m_TempSupports.push_back(VARIANT2ID(var));
         }
      }

      pStrLoad->EndUnit();
   }
   catch(CHRException& exception)
   {
      ATLASSERT(false);
      return exception;
   };

   return S_OK;
}

HRESULT CTemporarySupportActivityBase::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(GetUnitName(),1.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("Count"),CComVariant(m_TempSupports.size()));

      std::vector<SupportIDType>::iterator tsIter(m_TempSupports.begin());
      std::vector<SupportIDType>::iterator tsIterEnd(m_TempSupports.end());
      for ( ; tsIter != tsIterEnd; tsIter++ )
      {
         pStrSave->put_Property(_T("TempSupportID"),CComVariant(*tsIter));
      }
   }

   pStrSave->EndUnit();

   return S_OK;
}

void CTemporarySupportActivityBase::MakeCopy(const CTemporarySupportActivityBase& rOther)
{
   m_bEnabled     = rOther.m_bEnabled;
   m_TempSupports = rOther.m_TempSupports;
}

void CTemporarySupportActivityBase::MakeAssignment(const CTemporarySupportActivityBase& rOther)
{
   MakeCopy(rOther);
}
