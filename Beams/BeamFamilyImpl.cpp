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


// BeamFamilyImpl.cpp : Implementation of IBeamFamilyImpl
#include "stdafx.h"
#include "Beams.h"
#include <Beams\Helper.h>
#include <IFace\BeamFactory.h>
#include <EAF/ComponentManager.h>

using namespace PGS::Beams;

void BeamFamilyImpl::Init()
{
   m_Names.clear();
   auto components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(GetCATID());
   for (auto& component : components)
   {
      m_Factories.insert(std::make_pair(CString(component.name.c_str()), component.clsid));
      m_Names.push_back(CString(component.name.c_str()));
   }
}

CString BeamFamilyImpl::GetName() const
{
   const CLSID& clsid = GetCLSID();
   auto component = WBFL::EAF::ComponentManager::GetInstance().GetComponent(clsid);
   return CString(component.name.c_str());
}

void BeamFamilyImpl::RefreshFactoryList()
{
   m_Factories.clear();
   Init();
}

const std::vector<CString>& BeamFamilyImpl::GetFactoryNames() const
{
   return m_Names;
}

CLSID BeamFamilyImpl::GetFactoryCLSID(LPCTSTR strName) const
{
   auto found = m_Factories.find(CString(strName));
   if ( found == m_Factories.end() )
      return CLSID_NULL;

   return found->second;
}

std::shared_ptr<BeamFactory> BeamFamilyImpl::CreateFactory(LPCTSTR strName) const
{
   auto found = m_Factories.find(CString(strName));
   if ( found == m_Factories.end() )
      return nullptr;

   CLSID clsid = found->second;
   auto factory = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<BeamFactory>(clsid);
   return factory;
}
