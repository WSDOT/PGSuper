///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperPluginMgr::CPGSuperPluginMgr()
{
}

bool CPGSuperPluginMgr::LoadPlugins()
{
   USES_CONVERSION;

   CComPtr<ICatRegister> pICatReg;
   HRESULT hr = pICatReg.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
   if ( FAILED(hr) )
   {
      AfxMessageBox("Failed to create the component category manager");
      return false;
   }

   CComQIPtr<ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];
   ID[0] = CATID_PGSuperIEPlugin;

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   // load all importers
   UINT cmdImporter = FIRST_IMPORT_PLUGIN;
   UINT cmdExporter = FIRST_EXPORT_PLUGIN;

   CLSID clsid[5]; 
   ULONG nFetched = 0;
   while ( SUCCEEDED(pIEnumCLSID->Next(5,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         CComPtr<IUnknown> unknown;
         unknown.CoCreateInstance(clsid[i]);

         if ( !unknown )
         {
            LPOLESTR pszUserType;
            OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
            CString strMsg;
            strMsg.Format("Failed to load %s Import/Export plug in",OLE2A(pszUserType));
            AfxMessageBox(strMsg);
         }
         else
         {
            CComQIPtr<IPGSuperImporter> importer(unknown);
            CComQIPtr<IPGSuperExporter> exporter(unknown);

            if ( importer )
            {
               m_ImporterPlugins.push_back( std::make_pair(cmdImporter++,importer) );
            }

            if ( exporter )
            {
               m_ExporterPlugins.push_back( std::make_pair(cmdExporter++,exporter) );
            }
         }
      }
   }

   return true;
}

void CPGSuperPluginMgr::UnloadPlugins()
{
   m_ImporterPlugins.clear();
   m_ExporterPlugins.clear();
}

Uint32 CPGSuperPluginMgr::GetImporterCount()
{
   return m_ImporterPlugins.size();
}

Uint32 CPGSuperPluginMgr::GetExporterCount()
{
   return m_ExporterPlugins.size();
}

void CPGSuperPluginMgr::GetPGSuperImporter(Uint32 key,bool bByIndex,IPGSuperImporter** ppImporter)
{
   if ( bByIndex )
   {
      (*ppImporter) = m_ImporterPlugins[key].second;
      (*ppImporter)->AddRef();
   }
   else
   {
      std::vector<ImporterRecord>::iterator iter;
      for ( iter = m_ImporterPlugins.begin(); iter != m_ImporterPlugins.end(); iter++ )
      {
         if ( key == (*iter).first )
         {
            (*ppImporter) = (*iter).second;
            (*ppImporter)->AddRef();
            return;
         }
      }
   }
}

void CPGSuperPluginMgr::GetPGSuperExporter(Uint32 key,bool bByIndex,IPGSuperExporter** ppExporter)
{
   if ( bByIndex )
   {
      (*ppExporter) = m_ExporterPlugins[key].second;
      (*ppExporter)->AddRef();
   }
   else
   {
      std::vector<ExporterRecord>::iterator iter;
      for ( iter = m_ExporterPlugins.begin(); iter != m_ExporterPlugins.end(); iter++ )
      {
         if ( key == (*iter).first )
         {
            (*ppExporter) = (*iter).second;
            (*ppExporter)->AddRef();
            return;
         }
      }
   }
}

UINT CPGSuperPluginMgr::GetPGSuperImporterCommand(Uint32 idx)
{
   return m_ImporterPlugins[idx].first;
}

UINT CPGSuperPluginMgr::GetPGSuperExporterCommand(Uint32 idx)
{
   return m_ExporterPlugins[idx].first;
}
