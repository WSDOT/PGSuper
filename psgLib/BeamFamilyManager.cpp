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
#include <PGSuperCatCom.h>

#include <initguid.h>
#include <PsgLib\BeamFamilyManager.h>

CBeamFamilyManager::FamilyContainer CBeamFamilyManager::m_Families;

HRESULT CBeamFamilyManager::Init()
{
   CComPtr<ICatRegister> pICatReg = 0;
   HRESULT hr;
   hr = ::CoCreateInstance( CLSID_StdComponentCategoriesMgr,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ICatRegister,
                            (void**)&pICatReg );
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format("Failed to create Component Category Manager. hr = %d\nIs the correct version of Internet Explorer installed", hr);
      AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return hr;
   }

   CComPtr<ICatInformation> pICatInfo;
   pICatReg->QueryInterface(IID_ICatInformation,(void**)&pICatInfo);
   IEnumCLSID* pIEnumCLSID = 0;

   const int nID = 1;
   CATID ID[nID];
   ID[0] = CATID_BeamFamily;

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   CLSID clsid[1];
   while ( pIEnumCLSID->Next(1,clsid,NULL) != S_FALSE )
   {
      LPOLESTR pszUserType;
      OleRegGetUserType(clsid[0],USERCLASSTYPE_SHORT,&pszUserType);
      CString str(pszUserType);

      m_Families.insert( std::make_pair(str,clsid[0]) );
   }

   return S_OK;
}

std::vector<CString> CBeamFamilyManager::GetBeamFamilyNames()
{
   std::vector<CString> names;
   FamilyContainer::iterator iter;
   for ( iter = m_Families.begin(); iter != m_Families.end(); iter++ )
   {
      names.push_back( iter->first );
   }

   return names;
}

HRESULT CBeamFamilyManager::GetBeamFamily(const char* strName,IBeamFamily** ppFamily)
{
   FamilyContainer::iterator found = m_Families.find(CString(strName));
   if ( found == m_Families.end() )
      return E_FAIL;

   CLSID clsid = found->second;
   IBeamFamily* pFamily;
   HRESULT hr = ::CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IBeamFamily,(void**)&pFamily);
   if ( FAILED(hr) )
      return hr;

   (*ppFamily) = pFamily;

   return S_OK;
}

CLSID CBeamFamilyManager::GetBeamFamilyCLSID(const char* strName)
{
   FamilyContainer::iterator found = m_Families.find(CString(strName));
   if ( found == m_Families.end() )
      return CLSID_NULL;

   return found->second;
}
