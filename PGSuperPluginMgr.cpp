///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperPluginMgr.h"
#include <EAF\EAFApp.h>

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

   CWaitCursor cursor;

   CComPtr<ICatRegister> pICatReg;
   HRESULT hr = pICatReg.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
   if ( FAILED(hr) )
   {
      AfxMessageBox(_T("Failed to create the component category manager"));
      return false;
   }

   CComQIPtr<ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];

   ID[0] = CATID_PGSuperDataImporter;
   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   CEAFApp* pApp = EAFGetApp();

   const int nPlugins = 5;
   CLSID clsid[nPlugins]; 
   ULONG nFetched = 0;

   // Load Importers
   UINT cmdImporter = FIRST_DATA_IMPORTER_PLUGIN;
   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         CString strState = pApp->GetProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Enabled"));

         if ( strState.CompareNoCase(_T("Enabled")) == 0 )
         {
            CComPtr<IPGSuperDataImporter> importer;
            importer.CoCreateInstance(clsid[i]);

            if ( !importer )
            {
               LPOLESTR pszUserType;
               OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
               CString strMsg;
               strMsg.Format(_T("Failed to load %s PGSuper Data Importer plug in.\n\nWould you like to disable this plug-in?"),OLE2T(pszUserType));
               if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
               {
                  pApp->WriteProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Disabled"));
               }
            }
            else
            {
               ImporterRecord record;
               record.commandID = cmdImporter++;
               record.Plugin    = importer;

               importer->Init(record.commandID);

               HBITMAP hBmp;
               importer->GetBitmapHandle(&hBmp);
               record.Bitmap.Attach(hBmp);
               m_ImporterPlugins.push_back( record );
            }
         }

         ::CoTaskMemFree((void*)pszCLSID);
      }
   }

   // Load Exporters
   ID[0] = CATID_PGSuperDataExporter;
   pIEnumCLSID.Release();
   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);
   UINT cmdExporter = FIRST_DATA_EXPORTER_PLUGIN;
   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         CString strState = pApp->GetProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Enabled"));

         if ( strState.CompareNoCase(_T("Enabled")) == 0 )
         {
            CComPtr<IPGSuperDataExporter> exporter;
            exporter.CoCreateInstance(clsid[i]);

            if ( !exporter )
            {
               LPOLESTR pszUserType;
               OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
               CString strMsg;
               strMsg.Format(_T("Failed to load %s PGSuper Data Export plug in.\n\nWould you like to disable this plug-in?"),OLE2T(pszUserType));
               if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
               {
                  pApp->WriteProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Disabled"));
               }
            }
            else
            {
               ExporterRecord record;
               record.commandID = cmdExporter++;
               record.Plugin    = exporter;

               exporter->Init(record.commandID);

               HBITMAP hBmp;
               exporter->GetBitmapHandle(&hBmp);
               record.Bitmap.Attach(hBmp);
               m_ExporterPlugins.push_back( record );
            }
         }

         ::CoTaskMemFree((void*)pszCLSID);
      }
   }

   return true;
}

void CPGSuperPluginMgr::UnloadPlugins()
{
   m_ImporterPlugins.clear();
   m_ExporterPlugins.clear();
}

CollectionIndexType CPGSuperPluginMgr::GetImporterCount()
{
   return m_ImporterPlugins.size();
}

CollectionIndexType CPGSuperPluginMgr::GetExporterCount()
{
   return m_ExporterPlugins.size();
}

void CPGSuperPluginMgr::GetPGSuperImporter(CollectionIndexType key,bool bByIndex,IPGSuperDataImporter** ppImporter)
{
   if ( bByIndex )
   {
      (*ppImporter) = m_ImporterPlugins[key].Plugin;
      (*ppImporter)->AddRef();
   }
   else
   {
      std::vector<ImporterRecord>::iterator iter;
      for ( iter = m_ImporterPlugins.begin(); iter != m_ImporterPlugins.end(); iter++ )
      {
         if ( key == (*iter).commandID )
         {
            (*ppImporter) = (*iter).Plugin;
            (*ppImporter)->AddRef();
            return;
         }
      }
   }
}

void CPGSuperPluginMgr::GetPGSuperExporter(CollectionIndexType key,bool bByIndex,IPGSuperDataExporter** ppExporter)
{
   if ( bByIndex )
   {
      (*ppExporter) = m_ExporterPlugins[key].Plugin;
      (*ppExporter)->AddRef();
   }
   else
   {
      std::vector<ExporterRecord>::iterator iter;
      for ( iter = m_ExporterPlugins.begin(); iter != m_ExporterPlugins.end(); iter++ )
      {
         if ( key == (*iter).commandID )
         {
            (*ppExporter) = (*iter).Plugin;
            (*ppExporter)->AddRef();
            return;
         }
      }
   }
}

UINT CPGSuperPluginMgr::GetPGSuperImporterCommand(CollectionIndexType idx)
{
   return m_ImporterPlugins[idx].commandID;
}

UINT CPGSuperPluginMgr::GetPGSuperExporterCommand(CollectionIndexType idx)
{
   return m_ExporterPlugins[idx].commandID;
}

const CBitmap* CPGSuperPluginMgr::GetPGSuperImporterBitmap(CollectionIndexType idx)
{
   return &m_ImporterPlugins[idx].Bitmap;
}

const CBitmap* CPGSuperPluginMgr::GetPGSuperExporterBitmap(CollectionIndexType idx)
{
   return &m_ExporterPlugins[idx].Bitmap;
}
