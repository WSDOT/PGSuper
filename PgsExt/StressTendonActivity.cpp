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
#include <PgsExt\StressTendonActivity.h>

///////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStressTendonActivity::CStressTendonActivity()
{
   m_bEnabled = false;
}

CStressTendonActivity::CStressTendonActivity(const CStressTendonActivity& rOther)
{
   MakeCopy(rOther);
}

CStressTendonActivity::~CStressTendonActivity()
{
}

CStressTendonActivity& CStressTendonActivity::operator= (const CStressTendonActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CStressTendonActivity::operator==(const CStressTendonActivity& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
      return false;

   if ( m_Tendons != rOther.m_Tendons )
      return false;

   return true;
}

bool CStressTendonActivity::operator!=(const CStressTendonActivity& rOther) const
{
   return !operator==(rOther);
}

void CStressTendonActivity::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CStressTendonActivity::IsEnabled() const
{
   return m_bEnabled;
}

void CStressTendonActivity::Clear()
{
   m_Tendons.clear();
}

void CStressTendonActivity::AddTendon(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   m_Tendons.push_back(std::make_pair(girderKey,ductIdx));
   std::sort(m_Tendons.begin(),m_Tendons.end());
}

void CStressTendonActivity::AddTendons(const std::vector<std::pair<CGirderKey,DuctIndexType>>& tendons)
{
   m_Tendons.insert(m_Tendons.end(),tendons.begin(),tendons.end());
   std::sort(m_Tendons.begin(),m_Tendons.end());
}

void CStressTendonActivity::RemoveTendon(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iter(m_Tendons.begin());
   std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iterEnd(m_Tendons.end());
   for ( ; iter != iterEnd; iter++ )
   {
      std::pair<CGirderKey,DuctIndexType> p = *iter;
      if ( p.first == girderKey && p.second == ductIdx )
      {
         m_Tendons.erase(iter);
         break;
      }
   }
}

bool CStressTendonActivity::IsTendonStressed(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   std::vector<std::pair<CGirderKey,DuctIndexType>>::const_iterator iter(m_Tendons.begin());
   std::vector<std::pair<CGirderKey,DuctIndexType>>::const_iterator iterEnd(m_Tendons.end());
   for ( ; iter != iterEnd; iter++ )
   {
      std::pair<CGirderKey,DuctIndexType> p = *iter;
      if ( p.first == girderKey && p.second == ductIdx )
      {
         return true;
      }
   }

   return false;
}

bool CStressTendonActivity::IsTendonStressed() const
{
   return (m_Tendons.size() != 0 ? true : false);
}

const std::vector<std::pair<CGirderKey,DuctIndexType>>& CStressTendonActivity::GetTendons() const
{
   return m_Tendons;
}

IndexType CStressTendonActivity::GetTendonCount() const
{
   return m_Tendons.size();
}

HRESULT CStressTendonActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("StressTendons"));

      CComVariant var;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Enabled"),&var);
      m_bEnabled = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bEnabled )
      {
         CollectionIndexType nTendons;
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         nTendons = VARIANT2INDEX(var);
         m_Tendons.clear();

         m_Tendons.resize(nTendons);

         for ( CollectionIndexType i = 0; i < nTendons; i++ )
         {
            pStrLoad->BeginUnit(_T("Tendon"));
            CGirderKey girderKey;
            girderKey.Load(pStrLoad,pProgress);

            var.vt = VT_INDEX;
            pStrLoad->get_Property(_T("DuctIndex"),&var);
            DuctIndexType ductIdx = VARIANT2INDEX(var);

            m_Tendons[i] = std::make_pair(girderKey,ductIdx);

            pStrLoad->EndUnit(); // Tendon
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

HRESULT CStressTendonActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("StressTendons"),1.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("Count"),CComVariant(m_Tendons.size()));

      std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iter(m_Tendons.begin());
      std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iterEnd(m_Tendons.end());
      for ( ; iter != iterEnd; iter++ )
      {
         std::pair<CGirderKey,DuctIndexType> p = *iter;
         pStrSave->BeginUnit(_T("Tendon"),1.0);
         p.first.Save(pStrSave,pProgress);
         pStrSave->put_Property(_T("DuctIndex"),CComVariant(p.second));
         pStrSave->EndUnit(); // Tendon
      }
   }

   pStrSave->EndUnit(); // Stress Tendons

   return S_OK;
}

void CStressTendonActivity::MakeCopy(const CStressTendonActivity& rOther)
{
   m_bEnabled  = rOther.m_bEnabled;
   m_Tendons   = rOther.m_Tendons;
}

void CStressTendonActivity::MakeAssignment(const CStressTendonActivity& rOther)
{
   MakeCopy(rOther);
}
