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

// TxDOTAgent.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>
#include <WBFLGeometry.h>

#include <initguid.h>
#include "CLSID.h"

// interfaces used in this DLL.... resolves symbols for the linker
#include <WBFLDManip_i.c>
#include <WBFLDManipTools_i.c>
#include <DManip\DManip_clsid.cpp>
#include <DManipTools\DManipTools_clsid.cpp>
#include <WBFLCore_i.c>
#include <WBFLTools_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>
#include "dllmain.h"

#include <EAF\EAFAppPlugin.h>
#include <EAF\EAFComponentInfo.h>

#include "BridgeLinkCATID.h"
#include "PGSuperCatCom.h"
#include "TogaCatCom.h"
#include <System\ComCatMgr.h>

#include <EAF\EAFUIIntegration.h>

#include "PGSComponentInfo.h"
#include <Plugins\PGSuperIEPlugin.h>
#include <WBFLReportManagerAgent_i.c>
#include "TxDOTOptionalDesignDocProxyAgent.h"

#include <IReportManager.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Selection.h>
#include <IFace\TestFileExport.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\DistributionFactors.h>
#include <IFace\MomentCapacity.h>
#include <IFace\PrestressForce.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\Test1250.h>
#include <IFace\GirderHandling.h>
#include <IFace/Limits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\Constructability.h>
#include <IFace\RatingSpecification.h>

#include "TxDOTOptionalDesignData.h"
#include "TogaSupportDrawStrategy.h"
#include "TogaSectionCutDrawStrategy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

HRESULT Register(bool bRegister)
{
   HRESULT hr = S_OK;

   // The TxDOTAppPlugin plugs into the BridgeLink application executable and brings the
   // TxDOT Optional Design Document functionality
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTAppPluginComponentInfo,CATID_BridgeLinkComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   // The TxDOT Agent extends the functionality of PGSuper by adding custom reporting and
   // other features
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTAgent,CATID_PGSuperExtensionAgent,bRegister);
   if ( FAILED(hr) )
      return hr;

   // The TxDOT Agent extends the functionality of Toga by adding custom reporting and
   // other features
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTAgent,CATID_TogaExtensionAgent,bRegister);
   if ( FAILED(hr) )
      return hr;

   // The TxDOT Cad Exporter provides custom export functionatlity
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTCadExporter,CATID_PGSuperDataExporter,bRegister);
   if ( FAILED(hr) )
      return hr;

   // The TxDOT component info objects provides information about this entire plug-in component
   // This information is used in the "About" dialog
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_TxDOTComponentInfo,CATID_PGSuperComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
   // registers object, typelib and all interfaces in typelib
   HRESULT hr = _AtlModule.DllRegisterServer(FALSE);
   if ( FAILED(hr) )
      return hr;

   hr = Register(true);
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

   hr = Register(false);
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


