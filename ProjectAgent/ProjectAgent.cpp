///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// ProjectAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ProjectAgentps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "ProjectAgent.h"

#include "ProjectAgent_i.h"
#include "ProjectAgent_i.c"

#include <WBFLCore_i.c>
#include <WBFLGenericBridge_i.c>

#include <WBFLCogo_i.c>

#include "ProjectAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>

#include <IFace\PrestressForce.h>
#include <IFace\StatusCenter.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Transactions.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFUIIntegration.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ProjectAgent, CProjectAgentImp)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
   LONG cLock = _Module.GetLockCount();
   bool bCanUnload = ( cLock == 0 );
	return ( bCanUnload ) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

HRESULT RegisterAgent(bool bRegister)
{
   HRESULT hr = S_OK;
   hr = sysComCatMgr::RegWithCategory(CLSID_ProjectAgent,CATID_PGSuperAgent,bRegister);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_ProjectAgent,CATID_PGSpliceAgent,bRegister);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _Module.RegisterServer(FALSE);
   if ( FAILED(hr) )
      return hr;

   return RegisterAgent(true);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   RegisterAgent(false);
	_Module.UnregisterServer();
	return S_OK;
}
