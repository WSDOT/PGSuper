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

// WSDOTAgent.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

#include "WSDOTAgent_i.h"
#include "dllmain.h"

#include "BridgeLinkCATID.h"
#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>
#include "PGSComponentInfo.h"

// interfaces used in this DLL.... resolves symbols for the linker
#include <WBFLCore_i.c>
#include <WBFLUnitServer_i.c>
#include <IReportManager.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\Selection.h>
#include <IFace\Allowables.h>

#include <Plugins\BeamFamilyCLSID.h>

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _AtlModule.GetLockCount()==0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

HRESULT RegisterAgent(bool bRegister)
{
   HRESULT hr = S_OK;

   hr = sysComCatMgr::RegWithCategory(CLSID_WSDOTAgent,CATID_PGSuperExtensionAgent,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = sysComCatMgr::RegWithCategory(CLSID_WSDOTAgent,CATID_PGSpliceExtensionAgent,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperComponentInfo,CATID_PGSuperComponentInfo,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = sysComCatMgr::RegWithCategory(CLSID_PGSpliceComponentInfo,CATID_PGSpliceComponentInfo,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
    if ( FAILED(hr) )
    {
       return hr;
    }

    return RegisterAgent(true);
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
   HRESULT hr = RegisterAgent(false);
   if ( FAILED(hr) )
   {
      return hr;
   }

	return _AtlModule.DllUnregisterServer();
}

//// DllInstall - Adds/Removes entries to the system registry per user
////              per machine.	
//STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
//{
//    HRESULT hr = E_FAIL;
//    static const wchar_t szUserSwitch[] = _T("user");
//
//    if (pszCmdLine != nullptr)
//    {
//    	if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
//    	{
//    		AtlSetPerUserRegistration(true);
//    	}
//    }
//
//    if (bInstall)
//    {	
//    	hr = DllRegisterServer();
//    	if (FAILED(hr))
//    	{	
//    		DllUnregisterServer();
//    	}
//    }
//    else
//    {
//    	hr = DllUnregisterServer();
//    }
//
//    return hr;
//}
//
//
