///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2022  Washington State Department of Transportation
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

// ExtensionAgentExample.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include "ExtensionAgentExample_i.h"
#include "dllmain.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>

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

void RegsiterAsExtensionAgent(bool bRegister)
{
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_ExampleExtensionAgent, CATID_PGSuperExtensionAgent, bRegister);
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_ExampleExtensionAgent, CATID_PGSpliceExtensionAgent, bRegister);
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();

   // Register extension agent
   RegsiterAsExtensionAgent(true);

   return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();

   // Register extension agent
   RegsiterAsExtensionAgent(false);

   return hr;
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


