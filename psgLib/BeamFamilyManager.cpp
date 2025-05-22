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

#include "stdafx.h"
#include <PGSuperCatCom.h>

#include <initguid.h>
#include <PsgLib\BeamFamilyManager.h>

#include <EAF\EAFUtilities.h>
#include <EAF\ComponentManager.h>

using namespace PGS::Library;

BeamFamilyManager::FamilyContainer BeamFamilyManager::m_Families;

HRESULT BeamFamilyManager::Init(CATID catid)
{
   auto found = m_Families.find(catid);
   if (found == m_Families.end())
   {
      BeamContainer beams;
      auto result = m_Families.insert(std::make_pair(catid, beams));
      found = result.first;
   }
   auto& beams = found->second;

   auto components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(catid);
   for (auto& component : components)
   {
      beams.insert(std::make_pair(component.name.c_str(), component.clsid));
   }

   return S_OK;
}

std::vector<CString> BeamFamilyManager::GetBeamFamilyNames()
{
   std::vector<CString> vNames;
   for( auto& [catid,beams] : m_Families)
   {
      for( auto& [name,clsid] : beams)
      {
         vNames.push_back(name);
      }
   }

   std::sort(vNames.begin(),vNames.end());
   return vNames;
}

std::vector<CString> BeamFamilyManager::GetBeamFamilyNames(CATID catid)
{
   std::vector<CString> vNames;
   auto found(m_Families.find(catid));
   if ( found == m_Families.end() )
      return vNames;

   BeamContainer& beams = found->second;
   for( auto& [name,clsid] : beams)
   {
      vNames.push_back(name);
   }

   std::sort(vNames.begin(),vNames.end());
   return vNames;
}

std::shared_ptr<PGS::Beams::BeamFamily> BeamFamilyManager::GetBeamFamily(LPCTSTR strName)
{
   for(auto& [catid,beams] : m_Families)
   {
      auto found = beams.find(CString(strName));
      if ( found != beams.end() )
      {
         CLSID clsid = found->second;
         auto family = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<PGS::Beams::BeamFamily>(clsid);
         return family;
      }
   }

   // if we got this far, the name wasn't found
   return nullptr;
}

CLSID BeamFamilyManager::GetBeamFamilyCLSID(LPCTSTR strName)
{
   for (auto& [catid, beams] : m_Families)
   {
      auto found = beams.find(CString(strName));
      if ( found != beams.end() )
      {
         return found->second;
      }
   }

   // if we got this far, the name wasn't found
   return CLSID_NULL;
}

void BeamFamilyManager::Reset()
{
   m_Families.clear();
}

void BeamFamilyManager::UpdateFactories()
{
   std::vector<CString> vNames = BeamFamilyManager::GetBeamFamilyNames();
   for(auto& strName : vNames)
   {
      auto family = GetBeamFamily(strName);
      family->RefreshFactoryList();
   }
}
