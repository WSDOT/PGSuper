///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperProjectImporterMgr.h"
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperProjectImporterMgr::CPGSuperProjectImporterMgr()
{
}

bool CPGSuperProjectImporterMgr::LoadImporters()
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
   ID[0] = CATID_PGSuperProjectImporter;

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   // load all importers
   CEAFApp* pApp = (CEAFApp*)AfxGetApp();

   const int nCLSID = 5;
   CLSID clsid[nCLSID]; 
   ULONG nFetched = 0;
   while ( SUCCEEDED(pIEnumCLSID->Next(nCLSID,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         CString strState = pApp->GetProfileString(_T("Plugins"),OLE2A(pszCLSID),_T("Enabled"));

         if ( strState.CompareNoCase("Enabled") == 0 )
         {
            CComPtr<IPGSuperProjectImporter> importer;
            importer.CoCreateInstance(clsid[i]);

            if ( !importer )
            {
               LPOLESTR pszUserType;
               OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
               CString strMsg;
               strMsg.Format("Failed to load %s PGSuper Project Importer plug in.\n\nWould you like to disable this plug-in?",OLE2A(pszUserType));
               if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
               {
                  pApp->WriteProfileString(_T("Plugins"),OLE2A(pszCLSID),_T("Disabled"));
               }
            }
            else
            {
               Record record;
               record.commandID = 0; // project importers don't use command IDs
               record.Importer  = importer;

               m_ImporterRecords.push_back( record );
            }
         }
         ::CoTaskMemFree((void*)pszCLSID);
      }
   }

   return true;
}

void CPGSuperProjectImporterMgr::UnloadImporters()
{
   m_ImporterRecords.clear();
}

Uint32 CPGSuperProjectImporterMgr::GetImporterCount() const
{
   return m_ImporterRecords.size();
}

void CPGSuperProjectImporterMgr::GetImporter(Uint32 key,bool bByIndex,IPGSuperProjectImporter** ppImporter)
{
   if ( bByIndex )
   {
      (*ppImporter) = m_ImporterRecords[key].Importer;
      (*ppImporter)->AddRef();
   }
   else
   {
      std::vector<Record>::iterator iter;
      for ( iter = m_ImporterRecords.begin(); iter != m_ImporterRecords.end(); iter++ )
      {
         if ( key == (*iter).commandID )
         {
            (*ppImporter) = (*iter).Importer;
            (*ppImporter)->AddRef();
            return;
         }
      }
   }
}

UINT CPGSuperProjectImporterMgr::GetImporterCommand(Uint32 idx) const
{
   return m_ImporterRecords[idx].commandID;
}
