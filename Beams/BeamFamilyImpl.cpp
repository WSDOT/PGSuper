///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "BeamFamilyImpl.h"
#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// IBeamFamilyImpl
HRESULT IBeamFamilyImpl::Init()
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
      msg.Format(_T("Failed to create Component Category Manager. hr = %d\nIs the correct version of Internet Explorer installed"), hr);
      AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
      return hr;
   }

   CComPtr<ICatInformation> pICatInfo;
   pICatReg->QueryInterface(IID_ICatInformation,(void**)&pICatInfo);
   IEnumCLSID* pIEnumCLSID = 0;

   const int nID = 1;
   CATID ID[nID];
   ID[0] = GetCATID();

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   CLSID clsid[1];
   while ( pIEnumCLSID->Next(1,clsid,NULL) != S_FALSE )
   {
      LPOLESTR pszUserType;
      OleRegGetUserType(clsid[0],USERCLASSTYPE_SHORT,&pszUserType);
      CString str(pszUserType);

      m_Factories.insert( std::make_pair(str,clsid[0]) );
      m_Names.push_back(str);
   }

   return S_OK;
}

CString IBeamFamilyImpl::GetName()
{
   const CLSID& clsid = GetCLSID();
   LPOLESTR pszUserType;
   HRESULT result = OleRegGetUserType(clsid,USERCLASSTYPE_SHORT,&pszUserType);
   ATLASSERT( SUCCEEDED(result) );

   CString strName(pszUserType);
   return strName;
}

const std::vector<CString>& IBeamFamilyImpl::GetFactoryNames()
{
   return m_Names;
}

CLSID IBeamFamilyImpl::GetFactoryCLSID(LPCTSTR strName)
{
   FactoryContainer::iterator found = m_Factories.find(CString(strName));
   if ( found == m_Factories.end() )
      return CLSID_NULL;

   return found->second;
}

HRESULT IBeamFamilyImpl::CreateFactory(LPCTSTR strName,IBeamFactory** ppFactory)
{
   FactoryContainer::iterator found = m_Factories.find(CString(strName));
   if ( found == m_Factories.end() )
      return E_FAIL;

   CLSID clsid = found->second;
   IBeamFactory* pFactory;
   HRESULT hr = ::CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IBeamFactory,(void**)&pFactory);
   if ( FAILED(hr) )
      return hr;

   (*ppFactory) = pFactory;

   return S_OK;
}
