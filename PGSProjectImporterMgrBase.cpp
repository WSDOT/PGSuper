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

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSProjectImporterMgrBase.h"
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSProjectImporterMgrBase::CPGSProjectImporterMgrBase()
{
}

CPGSProjectImporterMgrBase::~CPGSProjectImporterMgrBase()
{
}

bool CPGSProjectImporterMgrBase::LoadImporters()
{
   USES_CONVERSION;

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
   ID[0] = GetProjectImporterCATID();

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   // load all importers
   CEAFApp* pApp = EAFGetApp();
   CString strAppName = GetAppName();

   const int nCLSID = 5;
   CLSID clsid[nCLSID]; 
   ULONG nFetched = 0;
   while ( SUCCEEDED(pIEnumCLSID->Next(nCLSID,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         CString strState = pApp->GetProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Enabled"));

         if ( strState.CompareNoCase(_T("Enabled")) == 0 )
         {
            CComPtr<IPGSProjectImporter> importer;
            importer.CoCreateInstance(clsid[i]);

            if ( !importer )
            {
               LPOLESTR pszUserType;
               OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
               CString strMsg;
               strMsg.Format(_T("Failed to load %s %s Project Importer plug in.\n\nWould you like to disable this plug-in?"),strAppName,OLE2T(pszUserType));
               if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
               {
                  pApp->WriteProfileString(_T("Plugins"),OLE2T(pszCLSID),_T("Disabled"));
               }
            }
            else
            {
               Record record;
               record.clsid    = clsid[i];
               record.Importer = importer;

               m_ImporterRecords.insert( record );
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

CollectionIndexType CPGSProjectImporterMgrBase::GetImporterCount() const
{
   return m_ImporterRecords.size();
}

void CPGSProjectImporterMgrBase::GetImporter(CollectionIndexType idx,IPGSProjectImporter** ppImporter)
{
   std::set<Record>::iterator iter;
   CollectionIndexType count = 0;
   for ( iter = m_ImporterRecords.begin(); iter != m_ImporterRecords.end(); iter++, count++ )
   {
      if ( count == idx )
      {
         (*ppImporter) = iter->Importer;
         (*ppImporter)->AddRef();
         break;
      }
   }
}

void CPGSProjectImporterMgrBase::GetImporter(const CLSID& clsid,IPGSProjectImporter** ppImporter)
{
   Record record;
   record.clsid = clsid;
   std::set<Record>::iterator found = m_ImporterRecords.find(record);
   if ( found != m_ImporterRecords.end() )
   {
      (*ppImporter) = found->Importer;
      (*ppImporter)->AddRef();
   }
}

void CPGSProjectImporterMgrBase::AddImporter(const CLSID& clsid,IPGSProjectImporter* pImporter)
{
   Record record;
   record.clsid    = clsid;
   record.Importer = pImporter;
   m_ImporterRecords.insert(record);
}

void CPGSProjectImporterMgrBase::RemoveImporter(const CLSID& clsid)
{
   Record record;
   record.clsid = clsid;
   std::set<Record>::iterator found = m_ImporterRecords.find(record);
   if( found != m_ImporterRecords.end() )
   {
      m_ImporterRecords.erase(found);
   }
}
