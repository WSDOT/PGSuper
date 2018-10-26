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

// TxDOTAgent.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"

#include <initguid.h>
#include "TxDOTAgent_i.h"
#include "dllmain.h"

#include "PGSuperCatCom.h"
#include <System\ComCatMgr.h>

#include <EAF\EAFComponentInfo.h>

// interfaces used in this DLL.... resolves symbols for the linker
#include <WBFLCore_i.c>
#include <PGSuperIEPlugin_i.c>
#include <IReportManager.h>
#include <IFace\StatusCenter.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Selection.h>
#include <IFace\TxDOTCadExport.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\DistributionFactors.h>
#include <IFace\MomentCapacity.h>
#include <IFace\PrestressForce.h>



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

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
   // registers object, typelib and all interfaces in typelib
   HRESULT hr = _AtlModule.DllRegisterServer();
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTAgent,CATID_PGSuperExtensionAgent,true);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTCadExporter,CATID_PGSuperDataExporter,true);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTComponentInfo,CATID_PGSuperComponents,true);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTAgent,CATID_PGSuperExtensionAgent,false);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTCadExporter,CATID_PGSuperDataExporter,false);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_TxDOTComponentInfo,CATID_PGSuperComponents,false);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

//// DllInstall - Adds/Removes entries to the system registry per user
////              per machine.	
//STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
//{
//    HRESULT hr = E_FAIL;
//    static const wchar_t szUserSwitch[] = _T("user");
//
//    if (pszCmdLine != NULL)
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


