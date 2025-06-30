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
#include "PGSuperPluginMgr.h"
#include <EAF\EAFApp.h>

CPGSuperPluginMgrBase::CPGSuperPluginMgrBase()
{
}

bool CPGSuperPluginMgrBase::LoadPlugins()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   CWaitCursor cursor;

   // Load Importers
   UINT cmdImporter = FIRST_DATA_IMPORTER_PLUGIN;
   auto components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(GetImporterCATID());
   for(const auto& component : components)
   {
      if (LAST_DATA_IMPORTER_PLUGIN <= cmdImporter)
      {
         AfxMessageBox(_T("The maximum number of data importers has been exceeded."));
         break; // get out of while loop
      }

      LPOLESTR pszCLSID;
      ::StringFromCLSID(component.clsid, &pszCLSID);
      CString strState = pApp->GetProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Enabled"));

      if (strState.CompareNoCase(_T("Enabled")) == 0)
      {
         auto importer = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<PGS::IDataImporter>(component);
         if (importer)
         {
            WBFL::System::Logger::Debug(std::_tstring(_T("Importer Loaded: ")) + component.name + std::_tstring(_T(" from ") + component.dll));
            ImporterRecord record;
            record.commandID = cmdImporter++;
            record.Plugin = importer;

            importer->Init(record.commandID);

            HBITMAP hBmp = importer->GetBitmapHandle();
            record.Bitmap.Attach(hBmp);
            m_ImporterPlugins.push_back(record);
         }
         else
         {
            CString strMsg;
            strMsg.Format(_T("Failed to load %s PGSuper Data Importer plug-in (%s).\n\nWould you like to disable this plug-in?"), component.name.c_str(),OLE2T(pszCLSID));
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
               pApp->WriteProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Disabled"));
            }
         }
      }

      ::CoTaskMemFree((void*)pszCLSID);
   }

   // Load Exporters
   UINT cmdExporter = FIRST_DATA_EXPORTER_PLUGIN;
   components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(GetExporterCATID());
   for(const auto& component : components)
   {
      if (LAST_DATA_EXPORTER_PLUGIN <= cmdExporter)
      {
         AfxMessageBox(_T("The maximum number of data exporters has been exceeded."));
         break; // get out of the while loop
      }

      LPOLESTR pszCLSID;
      ::StringFromCLSID(component.clsid, &pszCLSID);
      CString strState = pApp->GetProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Enabled"));

      if (strState.CompareNoCase(_T("Enabled")) == 0)
      {
         auto exporter = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<PGS::IDataExporter>(component);
         if (exporter)
         {
            WBFL::System::Logger::Debug(std::_tstring(_T("Exporter Loaded: ")) + component.name + std::_tstring(_T(" from ") + component.dll));
            ExporterRecord record;
            record.commandID = cmdExporter++;
            record.Plugin = exporter;

            exporter->Init(record.commandID);

            HBITMAP hBmp = exporter->GetBitmapHandle();
            record.Bitmap.Attach(hBmp);
            m_ExporterPlugins.push_back(record);
         }
         else
         {
            CString strMsg;
            strMsg.Format(_T("Failed to load %s PGSuper Data Export plug-in (%s).\n\nWould you like to disable this plug-in?"), component.name.c_str(),OLE2T(pszCLSID));
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
               pApp->WriteProfileString(_T("Plugins"), OLE2T(pszCLSID), _T("Disabled"));
            }
         }
      }

      ::CoTaskMemFree((void*)pszCLSID);
   }

   return true;
}

void CPGSuperPluginMgrBase::UnloadPlugins()
{
   m_ImporterPlugins.clear();
   m_ExporterPlugins.clear();
}

IndexType CPGSuperPluginMgrBase::GetImporterCount()
{
   return m_ImporterPlugins.size();
}

IndexType CPGSuperPluginMgrBase::GetExporterCount()
{
   return m_ExporterPlugins.size();
}

std::shared_ptr<PGS::IDataImporter> CPGSuperPluginMgrBase::GetImporter(IndexType key, bool bByIndex) const
{
   if (bByIndex)
   {
      return m_ImporterPlugins[key].Plugin;
   }
   else
   {
      for( const auto& record : m_ImporterPlugins)
      {
         if (key == record.commandID)
         {
            return record.Plugin;
         }
      }
   }

   return nullptr;
}

std::shared_ptr<PGS::IDataExporter> CPGSuperPluginMgrBase::GetExporter(IndexType key, bool bByIndex) const
{
   if (bByIndex)
   {
      return m_ExporterPlugins[key].Plugin;
   }
   else
   {
      for(const auto& record : m_ExporterPlugins)
      {
         if (key == record.commandID)
         {
            return record.Plugin;
         }
      }
   }

   return nullptr;
}

UINT CPGSuperPluginMgrBase::GetImporterCommand(IndexType idx)
{
   return m_ImporterPlugins[idx].commandID;
}

UINT CPGSuperPluginMgrBase::GetExporterCommand(IndexType idx)
{
   return m_ExporterPlugins[idx].commandID;
}

const CBitmap* CPGSuperPluginMgrBase::GetImporterBitmap(IndexType idx)
{
   return &m_ImporterPlugins[idx].Bitmap;
}

const CBitmap* CPGSuperPluginMgrBase::GetExporterBitmap(IndexType idx)
{
   return &m_ExporterPlugins[idx].Bitmap;
}

void CPGSuperPluginMgrBase::LoadDocumentationMaps()
{
   for (const auto& record : m_ImporterPlugins)
   {
      auto pDocumentation = std::dynamic_pointer_cast<PGS::IPluginDocumentation>(record.Plugin);
      if (pDocumentation)
      {
         pDocumentation->LoadDocumentationMap();
      }
   }

   for (const auto& record : m_ExporterPlugins)
   {
      auto pDocumentation = std::dynamic_pointer_cast<PGS::IPluginDocumentation>(record.Plugin);
      if (pDocumentation)
      {
         pDocumentation->LoadDocumentationMap();
      }
   }
}

std::pair<WBFL::EAF::HelpResult,CString> CPGSuperPluginMgrBase::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID)
{
   CString strTargetDocSetName(lpszDocSetName);

   for (const auto& record : m_ImporterPlugins)
   {
      auto pDocumentation = std::dynamic_pointer_cast<PGS::IPluginDocumentation>(record.Plugin);
      if (pDocumentation)
      {
         auto strDocSetName = pDocumentation->GetDocumentationSetName();

         if (strDocSetName == strTargetDocSetName)
         {
            return pDocumentation->GetDocumentLocation(nHID);
         }
      }
   }

   for (const auto& record : m_ExporterPlugins)
   {
      auto pDocumentation = std::dynamic_pointer_cast<PGS::IPluginDocumentation>(record.Plugin);
      if (pDocumentation)
      {
         auto strDocSetName = pDocumentation->GetDocumentationSetName();

         if (strDocSetName == strTargetDocSetName)
         {
            return pDocumentation->GetDocumentLocation(nHID);
         }
      }
   }

   return { WBFL::EAF::HelpResult::DocSetNotFound,CString() };
}
