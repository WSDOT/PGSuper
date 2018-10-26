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
#include <BridgeLinkCatCom.h>
#include <PGSuperCatCom.h>
#include <Plugins\PGSuperIEPlugin.h>

#include <MFCTools\VersionInfo.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

CPGSuperAppPluginModule _AtlModule;
CPGSuperAppPluginApp theApp;

BEGIN_MESSAGE_MAP(CPGSuperAppPluginApp, CWinApp)
END_MESSAGE_MAP()


BOOL CPGSuperAppPluginApp::InitInstance()
{
   ::GXInit();

   // Register the component categories PGSuper needs
   // This should be done in the installer, but do it again here just in case
   sysComCatMgr::CreateCategory(L"PGSuper Agent",                   CATID_PGSuperAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Extension Agent",         CATID_PGSuperExtensionAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Beam Family",             CATID_BeamFamily);
   sysComCatMgr::CreateCategory(L"PGSuper Project Importer Plugin", CATID_PGSuperProjectImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Importer Plugin",    CATID_PGSuperDataImporter);
   sysComCatMgr::CreateCategory(L"PGSuper Data Exporter Plugin",    CATID_PGSuperDataExporter);


   // Use the same help file as the main application
   if ( EAFGetApp() )
   {
      free((void*)m_pszHelpFilePath);
      m_pszHelpFilePath = _tcsdup(EAFGetApp()->m_pszHelpFilePath);
   }

	return CWinApp::InitInstance();
}

int CPGSuperAppPluginApp::ExitInstance()
{
   ::GXTerminate();
	return CWinApp::ExitInstance();
}

CString CPGSuperAppPluginApp::GetVersion(bool bIncludeBuildNumber) const
{
   CString strExe( m_pszExeName );
   strExe += ".dll";

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   
   CString strVersion = verInfo.GetProductVersionAsString();

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif

   if (!bIncludeBuildNumber)
   {
      // remove the build number
      int pos = strVersion.ReverseFind('.'); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

   return strVersion;
}

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
   HRESULT hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperComponentInfo,CATID_BridgeLinkComponents,bRegister);
   if ( FAILED(hr) )
      return hr;

   // Need to register the library application plugin with the BridgeLinkAppPlugin category
   hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
   if ( FAILED(hr) )
      return hr;

   hr = sysComCatMgr::RegWithCategory(CLSID_PGSuperProjectImporterAppPlugin,CATID_BridgeLinkAppPlugin,bRegister);
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
//
//
