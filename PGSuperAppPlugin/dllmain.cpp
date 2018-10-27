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

// dllmain.cpp : Implementation of DllMain.

#include "PGSuperAppPlugin\stdafx.h"
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

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>


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
   sysComCatMgr::CreateCategory(L"PGSuper Agent",                   CATID_PGSuperAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Extension Agent",         CATID_PGSuperExtensionAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Beam Family",             CATID_PGSuperBeamFamily);
   sysComCatMgr::CreateCategory(L"PGSuper Project Importer Plugin", CATID_PGSuperProjectImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Importer Plugin",    CATID_PGSuperDataImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Exporter Plugin",    CATID_PGSuperDataExporter);
   sysComCatMgr::CreateCategory(L"PGSuper Component Information",   CATID_PGSuperComponentInfo);

   sysComCatMgr::CreateCategory(L"PGSplice Agent",                   CATID_PGSpliceAgent);
   sysComCatMgr::CreateCategory(L"PGSplice Extension Agent",         CATID_PGSpliceExtensionAgent);
   sysComCatMgr::CreateCategory(L"PGSplice Beam Family",             CATID_PGSpliceBeamFamily);
   sysComCatMgr::CreateCategory(L"PGSplice Project Importer Plugin", CATID_PGSpliceProjectImporter);
   sysComCatMgr::CreateCategory(L"PGSplice Data Importer Plugin",    CATID_PGSpliceDataImporter);
   sysComCatMgr::CreateCategory(L"PGSplice Data Exporter Plugin",    CATID_PGSpliceDataExporter);
   sysComCatMgr::CreateCategory(L"PGSplice Component Information",   CATID_PGSpliceComponentInfo);

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
   
   CString strVersion = verInfo.GetProductVersionAsString();

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif

   if (!bIncludeBuildNumber)
   {
      // remove the build number
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

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
   HRESULT hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperComponentInfo,CATID_BridgeLinkComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_PGSpliceComponentInfo,CATID_BridgeLinkComponentInfo,bRegister);
   if ( FAILED(hr) )
      return hr;

   // Need to register the library application plugin with the BridgeLinkAppPlugin category

   // PGSuper
   hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSuper Project Importers
   hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperProjectImporterAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSplice
   hr = sysComCatMgr::RegWithCategory(CLSID_PGSpliceAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   // PGSplice Project Importers
   hr = sysComCatMgr::RegWithCategory(CLSID_PGSpliceProjectImporterAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
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
