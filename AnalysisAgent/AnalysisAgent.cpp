///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// AnalysisAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f AnalysisAgentps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "AnalysisAgent.h"
#include "CLSID.h"

#include <WBFLFem2d_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>

#include <WBFLLBAMAnalysisUtility_i.c>
#include <WBFLLBAMAnalysis_i.c>
#include <WBFLLBAMLiveLoader_i.c>
#include <WBFLLBAMLoadCombiner_i.c>
#include <WBFLLBAMUtility_i.c>
#include <WBFLLBAM_i.c>

#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <WBFLCore_i.c>

#include "AnalysisAgentImp.h"

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>

#include <IFace\StatusCenter.h>
#include <IFace\PointOfInterest.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\GirderHandling.h>
#include <IFace\Constructability.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_AnalysisAgent, CAnalysisAgentImp)
END_OBJECT_MAP()


class CAnalysisAgentApp : public CWinApp
{
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
};

CAnalysisAgentApp theApp;

BOOL CAnalysisAgentApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CAnalysisAgentApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   LONG cLock = _Module.GetLockCount();
   HRESULT hr = AfxDllCanUnloadNow();
   bool bCanUnload = ( hr == S_OK && cLock == 0 );
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
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_AnalysisAgent,CATID_PGSuperAgent,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_AnalysisAgent,CATID_PGSpliceAgent,bRegister);
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _Module.RegisterServer(FALSE);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = RegisterAgent(true);
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   HRESULT hr = RegisterAgent(false);
   if ( FAILED(hr) )
   {
      return hr;
   }

   _Module.UnregisterServer();
	return S_OK;
}


