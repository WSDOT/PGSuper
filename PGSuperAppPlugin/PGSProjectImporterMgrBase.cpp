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
#include "resource.h"
#include "PGSProjectImporterMgrBase.h"
#include <EAF\EAFApp.h>


CPGSProjectImporterMgrBase::CPGSProjectImporterMgrBase()
{
}

CPGSProjectImporterMgrBase::~CPGSProjectImporterMgrBase()
{
}

bool CPGSProjectImporterMgrBase::LoadImporters()
{
   // Loads all of the registered project importers
   CEAFApp* pApp = EAFGetApp();
   CString strAppName = GetAppName();

   auto components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(GetProjectImporterCATID());
   for (const auto& component : components)
   {
      LPOLESTR pszCLSID;
      ::StringFromCLSID(component.clsid, &pszCLSID);
      CString strState = pApp->GetProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Enabled"));

      if (strState.CompareNoCase(_T("Enabled")) == 0)
      {
         auto importer = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<PGS::IProjectImporter>(component);
         if (importer)
         {
            Record record;
            record.clsid = component.clsid;
            record.importer = importer;

            m_ImporterRecords.insert(record);
         }
         else
         {
            CString strMsg;
            strMsg.Format(_T("Failed to load %s %s Project Importer plug in.\n\nWould you like to disable this plug-in?"), strAppName, component.name.c_str());
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
               pApp->WriteProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Disabled"));
            }
         }
         ::CoTaskMemFree((void*)pszCLSID);
      }
   }

   return true;
}

void CPGSProjectImporterMgrBase::UnloadImporters()
{
   m_ImporterRecords.clear();
}

IndexType CPGSProjectImporterMgrBase::GetImporterCount() const
{
   return m_ImporterRecords.size();
}

std::shared_ptr<PGS::IProjectImporter> CPGSProjectImporterMgrBase::GetImporter(IndexType idx) const
{
   IndexType count = 0;
   for(const auto& record : m_ImporterRecords)
   {
      if ( count == idx )
      {
         return record.importer;
      }
      count++;
   }
   return nullptr;
}

std::shared_ptr<PGS::IProjectImporter> CPGSProjectImporterMgrBase::GetImporter(const CLSID& clsid) const
{
   Record record;
   record.clsid = clsid;
   auto found = m_ImporterRecords.find(record);
   if ( found != m_ImporterRecords.end() )
   {
      return found->importer;
   }
   return nullptr;
}

void CPGSProjectImporterMgrBase::AddImporter(const CLSID& clsid,std::shared_ptr<PGS::IProjectImporter>& pImporter)
{
   Record record;
   record.clsid    = clsid;
   record.importer = pImporter;
   m_ImporterRecords.insert(record);
}

void CPGSProjectImporterMgrBase::RemoveImporter(const CLSID& clsid)
{
   Record record;
   record.clsid = clsid;
   auto found = m_ImporterRecords.find(record);
   if( found != m_ImporterRecords.end() )
   {
      m_ImporterRecords.erase(found);
   }
}
