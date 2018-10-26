///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <PgsExt\ApplyLoadActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CApplyLoadActivity::CApplyLoadActivity()
{
   m_bEnabled = false;
   m_bApplyRailingSystemLoad = false;
   m_bApplyOverlayLoad = false;
   m_bApplyLiveLoad = false;
   m_bApplyRatingLiveLoad = false;
}

CApplyLoadActivity::CApplyLoadActivity(const CApplyLoadActivity& rOther)
{
   MakeCopy(rOther);
}

CApplyLoadActivity::~CApplyLoadActivity()
{
}

CApplyLoadActivity& CApplyLoadActivity::operator= (const CApplyLoadActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CApplyLoadActivity::operator==(const CApplyLoadActivity& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
   {
      return false;
   }

   if ( m_bApplyRailingSystemLoad != rOther.m_bApplyRailingSystemLoad )
   {
      return false;
   }

   if ( m_bApplyOverlayLoad != rOther.m_bApplyOverlayLoad )
   {
      return false;
   }

   if ( m_bApplyLiveLoad != rOther.m_bApplyLiveLoad )
   {
      return false;
   }

   if ( m_bApplyRatingLiveLoad != rOther.m_bApplyRatingLiveLoad )
   {
      return false;
   }

   if ( m_UserLoads != rOther.m_UserLoads )
   {
      return false;
   }

   return true;
}

bool CApplyLoadActivity::operator!=(const CApplyLoadActivity& rOther) const
{
   return !operator==(rOther);
}

void CApplyLoadActivity::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CApplyLoadActivity::IsEnabled() const
{
   return m_bEnabled;
}

void CApplyLoadActivity::Clear()
{
   m_bApplyRailingSystemLoad = false;
   m_bApplyOverlayLoad       = false;
   m_bApplyLiveLoad          = false;
   m_bApplyRatingLiveLoad    = false;
   m_UserLoads.clear();
   m_bEnabled                = false;
}

void CApplyLoadActivity::ApplyRailingSystemLoad(bool bApplyLoad)
{
   m_bApplyRailingSystemLoad = bApplyLoad;
   Update();
}

bool CApplyLoadActivity::IsRailingSystemLoadApplied() const
{
   return m_bApplyRailingSystemLoad;
}

void CApplyLoadActivity::ApplyOverlayLoad(bool bApplyLoad)
{
   m_bApplyOverlayLoad = bApplyLoad;
   Update();
}

bool CApplyLoadActivity::IsOverlayLoadApplied() const
{
   return m_bApplyOverlayLoad;
}

void CApplyLoadActivity::ApplyLiveLoad(bool bApplyLoad)
{
   m_bApplyLiveLoad = bApplyLoad;
   Update();
}

bool CApplyLoadActivity::IsLiveLoadApplied() const
{
   return m_bApplyLiveLoad;
}

void CApplyLoadActivity::ApplyRatingLiveLoad(bool bApplyLoad)
{
   m_bApplyRatingLiveLoad = bApplyLoad;
   Update();
}

bool CApplyLoadActivity::IsRatingLiveLoadApplied() const
{
   return m_bApplyRatingLiveLoad;
}

void CApplyLoadActivity::AddUserLoad(LoadIDType loadID)
{
   m_UserLoads.insert(loadID);
   Update();
}

void CApplyLoadActivity::RemoveUserLoad(LoadIDType loadID)
{
   std::set<LoadIDType>::iterator found = m_UserLoads.find(loadID);
   if ( found != m_UserLoads.end() )
   {
      m_UserLoads.erase(found);
   }
   Update();
}

bool CApplyLoadActivity::HasUserLoad(LoadIDType loadID) const
{
   std::set<LoadIDType>::const_iterator found = m_UserLoads.find(loadID);
   return found == m_UserLoads.end() ? false : true;
}

IndexType CApplyLoadActivity::GetUserLoadCount() const
{
   return m_UserLoads.size();
}

LoadIDType CApplyLoadActivity::GetUserLoadID(IndexType idx) const
{
   std::set<LoadIDType>::const_iterator iter(m_UserLoads.begin());
   for ( IndexType i = 0; i < idx; i++ )
      iter++;

   LoadIDType id = *iter;
   return id;
}

bool CApplyLoadActivity::IsUserLoadApplied() const
{
   return (0 < GetUserLoadCount() ? true : false);
}

HRESULT CApplyLoadActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("ApplyLoad"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Enabled"),&var);
      m_bEnabled = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bEnabled )
      {
         hr = pStrLoad->get_Property(_T("ApplyRailingSystemLoad"),&var);
         m_bApplyRailingSystemLoad = (var.boolVal == VARIANT_TRUE ? true : false);

         hr = pStrLoad->get_Property(_T("ApplyOverlayLoad"),&var);
         m_bApplyOverlayLoad = (var.boolVal == VARIANT_TRUE ? true : false);

         hr = pStrLoad->get_Property(_T("ApplyLiveLoad"),&var);
         m_bApplyLiveLoad = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( 1 < version )
         {
            hr = pStrLoad->get_Property(_T("ApplyRatingLiveLoad"),&var);
            m_bApplyRatingLiveLoad = (var.boolVal == VARIANT_TRUE ? true : false);
         }

         pStrLoad->BeginUnit(_T("UserLoads"));
         var.vt = VT_INDEX;
         pStrLoad->get_Property(_T("Count"),&var);
         IndexType nLoads = VARIANT2INDEX(var);
         var.vt = VT_ID;
         for ( IndexType idx = 0; idx < nLoads; idx++ )
         {
            pStrLoad->get_Property(_T("ID"),&var);
            m_UserLoads.insert( VARIANT2ID(var) );
         }
         pStrLoad->EndUnit();
      }

      pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CApplyLoadActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("ApplyLoad"),2.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("ApplyRailingSystemLoad"),CComVariant(m_bApplyRailingSystemLoad));
      pStrSave->put_Property(_T("ApplyOverlayLoad"),CComVariant(m_bApplyOverlayLoad));
      pStrSave->put_Property(_T("ApplyLiveLoad"),CComVariant(m_bApplyLiveLoad));
      pStrSave->put_Property(_T("ApplyRatingLiveLoad"),CComVariant(m_bApplyRatingLiveLoad));

      pStrSave->BeginUnit(_T("UserLoads"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_UserLoads.size()));
      std::set<IDType>::iterator iter(m_UserLoads.begin());
      std::set<IDType>::iterator end(m_UserLoads.end());
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("ID"),CComVariant(*iter));
      }
      pStrSave->EndUnit();
   }

   pStrSave->EndUnit();

   return S_OK;
}

void CApplyLoadActivity::MakeCopy(const CApplyLoadActivity& rOther)
{
   m_bEnabled                = rOther.m_bEnabled;
   m_bApplyRailingSystemLoad = rOther.m_bApplyRailingSystemLoad;
   m_bApplyOverlayLoad       = rOther.m_bApplyOverlayLoad;
   m_bApplyLiveLoad          = rOther.m_bApplyLiveLoad;
   m_bApplyRatingLiveLoad    = rOther.m_bApplyRatingLiveLoad;
   m_UserLoads               = rOther.m_UserLoads;
}

void CApplyLoadActivity::MakeAssignment(const CApplyLoadActivity& rOther)
{
   MakeCopy(rOther);
}

void CApplyLoadActivity::Update()
{
   // if none of the loads are applied... disable this activity
   if ( !m_bApplyRailingSystemLoad && !m_bApplyOverlayLoad && !m_bApplyLiveLoad && !m_bApplyRatingLiveLoad && m_UserLoads.size() == 0 )
   {
      m_bEnabled = false;
   }
   else
   {
      m_bEnabled = true;
   }
}
