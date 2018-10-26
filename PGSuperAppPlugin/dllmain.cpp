///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <BridgeLinkCatCom.h>
#include <PGSuperCatCom.h>
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
   ::GXInit();
	GXSetNewGridLineMode(TRUE);	// use smarter grid lines (and dotted) 

   CGXTabWnd::RegisterClass(m_hInstance);

   SetRegistryKey( _T("Washington State Department of Transportation") );

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

   EnableHtmlHelp();

	return CWinApp::InitInstance();
}

int CPGSuperAppPluginApp::ExitInstance()
{
   ::GXTerminate();
	return CWinApp::ExitInstance();
}

void CPGSuperAppPluginApp::OnHelp()
{
   ::HtmlHelp( *EAFGetMainFrame(), EAFGetApp()->m_pszHelpFilePath, HH_DISPLAY_TOPIC, 0 );
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
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

   return strVersion;
}

// returns key for HKEY_LOCAL_MACHINE\Software\Washington State Department of Transportation\PGSuper"
// responsibility of the caller to call RegCloseKey() on the returned HKEY
// key is not created if missing (
HKEY CPGSuperAppPluginApp::GetAppLocalMachineRegistryKey()
{
	ASSERT(m_pszRegistryKey != NULL);
	ASSERT(m_pszProfileName != NULL);

	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;


   // open the "software" key
   LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("software"), 0, KEY_WRITE|KEY_READ, &hSoftKey);
	if ( result == ERROR_SUCCESS)
	{
      // open the "Washington State Department of Transportation" key
      result = RegOpenKeyEx(hSoftKey, m_pszRegistryKey, 0, KEY_WRITE|KEY_READ, &hCompanyKey);
		if (result == ERROR_SUCCESS)
		{
         // Open the "PGSuper" key
			result = RegOpenKeyEx(hCompanyKey, m_pszProfileName, 0, KEY_WRITE|KEY_READ, &hAppKey);
		}
	}

	if (hSoftKey != NULL)
		RegCloseKey(hSoftKey);
	
   if (hCompanyKey != NULL)
		RegCloseKey(hCompanyKey);

	return hAppKey;
}

// returns key for:
//      HKEY_LOCAL_MACHINE\"Software"\Washington State Deparment of Transportation\PGSuper\lpszSection
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CPGSuperAppPluginApp::GetLocalMachineSectionKey(LPCTSTR lpszSection)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return NULL;

   return GetLocalMachineSectionKey(hAppKey,lpszSection);
}

HKEY CPGSuperAppPluginApp::GetLocalMachineSectionKey(HKEY hAppKey,LPCTSTR lpszSection)
{
	ASSERT(lpszSection != NULL);

	HKEY hSectionKey = NULL;

	LONG result = RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_WRITE|KEY_READ, &hSectionKey);
	RegCloseKey(hAppKey);
	return hSectionKey;
}

UINT CPGSuperAppPluginApp::GetLocalMachineInt(LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return nDefault;

   return GetLocalMachineInt(hAppKey,lpszSection,lpszEntry,nDefault);
}

UINT CPGSuperAppPluginApp::GetLocalMachineInt(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault)
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	ASSERT(m_pszRegistryKey != NULL);

	HKEY hSecKey = GetLocalMachineSectionKey(hAppKey,lpszSection);
	if (hSecKey == NULL)
		return nDefault;
	DWORD dwValue;
	DWORD dwType;
	DWORD dwCount = sizeof(DWORD);
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_DWORD);
		ASSERT(dwCount == sizeof(dwValue));
		return (UINT)dwValue;
	}
	return nDefault;
}

CString CPGSuperAppPluginApp::GetLocalMachineString(LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return lpszDefault;

   return GetLocalMachineString(hAppKey,lpszSection,lpszEntry,lpszDefault);
}

CString CPGSuperAppPluginApp::GetLocalMachineString(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault)
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	ASSERT(m_pszRegistryKey != NULL);
	HKEY hSecKey = GetLocalMachineSectionKey(hAppKey,lpszSection);
	if (hSecKey == NULL)
		return lpszDefault;
	CString strValue;
	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
		NULL, &dwCount);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
			(LPBYTE)strValue.GetBuffer(dwCount/sizeof(TCHAR)), &dwCount);
		strValue.ReleaseBuffer();
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		return strValue;
	}
	return lpszDefault;
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
