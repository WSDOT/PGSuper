///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
   {
      return false;
   }

   if ( m_Tendons != rOther.m_Tendons )
   {
      return false;
   }

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
   m_bEnabled = false;
}

void CStressTendonActivity::AddTendon(GirderIDType gdrID,DuctIndexType ductIdx)
{
   m_Tendons.insert(CTendonKey(gdrID,ductIdx));
   m_bEnabled = true;
}

void CStressTendonActivity::AddTendon(const CTendonKey& tendonKey)
{
   ATLASSERT(tendonKey.girderID != INVALID_ID); // must be using girder ID
   m_Tendons.insert(tendonKey);
   m_bEnabled = true;
}

void CStressTendonActivity::AddTendons(const std::set<CTendonKey>& tendons)
{
#if defined _DEBUG
   std::set<CTendonKey>::const_iterator iter(tendons.begin());
   std::set<CTendonKey>::const_iterator end(tendons.end());
   for ( ; iter != end; iter++ )
   {
      const CTendonKey& key = *iter;
      ATLASSERT(key.girderID != INVALID_ID); // must be using girder ID
   }
#endif
   m_Tendons.insert(tendons.begin(),tendons.end());
   m_bEnabled = true;
}

void CStressTendonActivity::RemoveTendon(GirderIDType gdrID,DuctIndexType ductIdx,bool bRemovedFromBridge)
{
   CTendonKey key(gdrID,ductIdx);
   std::set<CTendonKey>::iterator found(m_Tendons.find(key));
   if ( found != m_Tendons.end() )
   {
      m_Tendons.erase(found);

      if ( bRemovedFromBridge )
      {
         // adjust the remaining keys for this girder.
         // if we remove ductIdx 0, ductIdx 1 becomes 0, 2 becomes 1, etc
         std::set<CTendonKey>::iterator iter(m_Tendons.begin());
         std::set<CTendonKey>::iterator end(m_Tendons.end());
         for ( ; iter != end; iter++ )
         {
            CTendonKey& thisKey = *iter;
            if ( thisKey.girderID == gdrID && ductIdx < thisKey.ductIdx )
            {
               thisKey.ductIdx--;
            }
         }
      }
   }
   else
   {
      ATLASSERT(false); // not found???
   }

   if ( m_Tendons.size() == 0 )
   {
      m_bEnabled = false;
   }
}

class MatchGirderID
{
public:
   MatchGirderID(GirderIDType gdrID) : m_GirderID(gdrID) {}
   bool operator()(const CTendonKey& tendonKey) const
   {
      return( tendonKey.girderID == m_GirderID ? true : false);
   }

private:
   GirderIDType m_GirderID;
};

void CStressTendonActivity::RemoveTendons(GirderIDType gdrID)
{
   m_Tendons.erase(std::remove_if(m_Tendons.begin(),m_Tendons.end(),MatchGirderID(gdrID)),m_Tendons.end());

   if ( m_Tendons.size() == 0 )
   {
      m_bEnabled = false;
   }
}

bool CStressTendonActivity::IsTendonStressed(GirderIDType gdrID,DuctIndexType ductIdx) const
{
   CTendonKey key(gdrID,ductIdx);
   std::set<CTendonKey>::const_iterator found(m_Tendons.find(key));
   if ( found == m_Tendons.end() )
   {
      return false;
   }
   else
   {
      return true;
   }
}

bool CStressTendonActivity::IsTendonStressed() const
{
   return (m_Tendons.size() != 0 ? true : false);
}

const std::set<CTendonKey>& CStressTendonActivity::GetTendons() const
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

         for ( CollectionIndexType i = 0; i < nTendons; i++ )
         {
            pStrLoad->BeginUnit(_T("Tendon"));
            var.vt = VT_ID;
            hr = pStrLoad->get_Property(_T("GirderID"),&var);
            GirderIDType gdrID = VARIANT2ID(var);

            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("DuctIndex"),&var);
            DuctIndexType ductIdx = VARIANT2INDEX(var);

            m_Tendons.insert(CTendonKey(gdrID,ductIdx));

            pStrLoad->EndUnit(); // Tendon
         }
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

HRESULT CStressTendonActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("StressTendons"),1.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));

   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("Count"),CComVariant(m_Tendons.size()));

      std::set<CTendonKey>::iterator iter(m_Tendons.begin());
      std::set<CTendonKey>::iterator iterEnd(m_Tendons.end());
      for ( ; iter != iterEnd; iter++ )
      {
         CTendonKey& key = *iter;
         pStrSave->BeginUnit(_T("Tendon"),1.0);
         pStrSave->put_Property(_T("GirderID"),CComVariant(key.girderID));
         pStrSave->put_Property(_T("DuctIndex"),CComVariant(key.ductIdx));
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
