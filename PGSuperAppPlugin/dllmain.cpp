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

// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin_i.h"
#include "dllmain.h"
#include "PGSuperApp.h"

#include <initguid.h>
#include <EAF\EAFComponentInfo.h>
#include <EAF\EAFUIIntegration.h>
#include <System\ComCatMgr.h>
#include <BridgeLinkCATID.h>
#include <PGSuperCatCom.h>
#include <PGSpliceCatCom.h>
#include <Plugins\PGSuperIEPlugin.h>
#include <IFace\BeamFactory.h>

#include <BridgeModelViewController.h>
#include <GirderModelViewController.h>
#include <LoadsViewController.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#include <DManip/DManip.h>
#include <DManipTools/DManipTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CPGSuperAppPluginModule _AtlModule;
CPGSuperAppPluginApp theApp;

BEGIN_MESSAGE_MAP(CPGSuperAppPluginApp, CWinApp)
   ON_COMMAND(ID_HELP,OnHelp)
END_MESSAGE_MAP()


BOOL CPGSuperAppPluginApp::InitInstance()
{
   GXInit();
	GXSetNewGridLineMode(TRUE);	// use smarter grid lines (and dotted) 

   CGXTabWnd::RegisterClass(m_hInstance);

   SetRegistryKey( _T("Washington State Department of Transportation") );

   // Register the component categories PGSuper needs
   // This should be done in the installer, but do it again here just in case
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Agent",                   CATID_PGSuperAgent);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Extension Agent",         CATID_PGSuperExtensionAgent);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Beam Family",             CATID_PGSuperBeamFamily);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Project Importer Plugin", CATID_PGSuperProjectImporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Data Importer Plugin",    CATID_PGSuperDataImporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Data Exporter Plugin",    CATID_PGSuperDataExporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSuper Component Information",   CATID_PGSuperComponentInfo);

   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Agent",                   CATID_PGSpliceAgent);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Extension Agent",         CATID_PGSpliceExtensionAgent);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Beam Family",             CATID_PGSpliceBeamFamily);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Project Importer Plugin", CATID_PGSpliceProjectImporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Data Importer Plugin",    CATID_PGSpliceDataImporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Data Exporter Plugin",    CATID_PGSpliceDataExporter);
   WBFL::System::ComCatMgr::CreateCategory(L"PGSplice Component Information",   CATID_PGSpliceComponentInfo);

	return CWinApp::InitInstance();
}

int CPGSuperAppPluginApp::ExitInstance()
{
   GXForceTerminate();
	return CWinApp::ExitInstance();
}

void CPGSuperAppPluginApp::OnHelp()
{
   // just need a default handler so the CDialog doesn't hide our help buttons
}

CString CPGSuperAppPluginApp::GetVersion(bool bIncludeBuildNumber) const
{
   CString strExe( m_pszExeName );
   strExe += ".dll";

   CVersionInfo verInfo;
   if ( !verInfo.Load(strExe) )
   {
      ATLASSERT(false);
      return CString(_T("Unknown"));
   }
   
#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif
   CString strVersion = verInfo.GetProductVersionAsString(bIncludeBuildNumber);

   return strVersion;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


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
   HRESULT hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSuperComponentInfo,CATID_BridgeLinkComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSpliceComponentInfo,CATID_BridgeLinkComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   // Need to register the library application plugin with the BridgeLinkAppPlugin category

   // PGSuper
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSuperAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSuper Project Importers
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSuperProjectImporterAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSplice
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSpliceAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSplice Project Importers
   hr = WBFL::System::ComCatMgr::RegWithCategory(CLSID_PGSpliceProjectImporterAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
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

	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer(FALSE);
    if ( FAILED(hr) )
       return hr;

    hr = Register(false);

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
//
//
