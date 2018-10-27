///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CBeamFamilyManager::FamilyContainer CBeamFamilyManager::m_Families;

HRESULT CBeamFamilyManager::Init(CATID catid)
{
   CComPtr<ICatRegister> pICatReg = 0;
   HRESULT hr;
   hr = ::CoCreateInstance( CLSID_StdComponentCategoriesMgr,
                            nullptr,
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
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];
   ID[0] = catid;

   pICatInfo->EnumClassesOfCategories(nID,ID,0,nullptr,&pIEnumCLSID);

   FamilyContainer::iterator found(m_Families.find(catid));
   if ( found == m_Families.end() )
   {
      BeamContainer beams;
      m_Families.insert(std::make_pair(catid,beams));
      found = m_Families.find(catid);
   }

   BeamContainer& beams = found->second;

   CLSID clsid[1];
   while ( pIEnumCLSID->Next(1,clsid,nullptr) != S_FALSE )
   {
      LPOLESTR pszUserType;
      OleRegGetUserType(clsid[0],USERCLASSTYPE_SHORT,&pszUserType);
      CString str(pszUserType);

      beams.insert( std::make_pair(str,clsid[0]) );
   }

   return S_OK;
}

std::vector<CString> CBeamFamilyManager::GetBeamFamilyNames()
{
   std::vector<CString> vNames;
   FamilyContainer::iterator familyIter(m_Families.begin());
   FamilyContainer::iterator familyIterEnd(m_Families.end());
   for ( ; familyIter != familyIterEnd; familyIter++ )
   {
      BeamContainer& beams = familyIter->second;
      BeamContainer::iterator beamIter(beams.begin());
      BeamContainer::iterator beamIterEnd(beams.end());
      for ( ; beamIter != beamIterEnd; beamIter++ )
      {
         const CString& strName = beamIter->first;
         vNames.push_back(strName);
      }
   }

   std::sort(vNames.begin(),vNames.end());
   return vNames;
}

std::vector<CString> CBeamFamilyManager::GetBeamFamilyNames(CATID catid)
{
   std::vector<CString> vNames;
   FamilyContainer::iterator found(m_Families.find(catid));
   if ( found == m_Families.end() )
      return vNames;

   BeamContainer& beams = found->second;
   BeamContainer::iterator beamIter(beams.begin());
   BeamContainer::iterator beamIterEnd(beams.end());
   for ( ; beamIter != beamIterEnd; beamIter++ )
   {
      const CString& strName = beamIter->first;
      vNames.push_back(strName);
   }

   std::sort(vNames.begin(),vNames.end());
   return vNames;
}

HRESULT CBeamFamilyManager::GetBeamFamily(LPCTSTR strName,IBeamFamily** ppFamily)
{
   FamilyContainer::iterator familyIter(m_Families.begin());
   FamilyContainer::iterator familyIterEnd(m_Families.end());
   for ( ; familyIter != familyIterEnd; familyIter++ )
   {
      BeamContainer& beams = familyIter->second;

      BeamContainer::iterator found = beams.find(CString(strName));
      if ( found != beams.end() )
      {
         CLSID clsid = found->second;
         IBeamFamily* pFamily;
         HRESULT hr = ::CoCreateInstance(clsid,nullptr,CLSCTX_ALL,IID_IBeamFamily,(void**)&pFamily);
         if ( FAILED(hr) )
            return hr;

         (*ppFamily) = pFamily;
         return S_OK;
      }
   }

   // if we got this far, the name wasn't found
   return E_FAIL;
}

CLSID CBeamFamilyManager::GetBeamFamilyCLSID(LPCTSTR strName)
{
   FamilyContainer::iterator familyIter(m_Families.begin());
   FamilyContainer::iterator familyIterEnd(m_Families.end());
   for ( ; familyIter != familyIterEnd; familyIter++ )
   {
      BeamContainer& beams = familyIter->second;

      BeamContainer::iterator found = beams.find(CString(strName));
      if ( found != beams.end() )
      {
         return found->second;
      }
   }

   // if we got this far, the name wasn't found
   return CLSID_NULL;
}

void CBeamFamilyManager::Reset()
{
   m_Families.clear();
}

void CBeamFamilyManager::UpdateFactories()
{
   std::vector<CString> vNames = CBeamFamilyManager::GetBeamFamilyNames();
   std::vector<CString>::iterator iter(vNames.begin());
   std::vector<CString>::iterator end(vNames.end());
   for ( ; iter != end; iter++ )
   {
      CString& strName(*iter);
      CComPtr<IBeamFamily> family;
      GetBeamFamily(strName,&family);
      family->RefreshFactoryList();
   }
}
