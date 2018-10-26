// IEPluginExample.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f IEPluginExampleps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "KDOTExport_i.h"
#include "KDOTExport_i.c"
#include "PGSuperCatCom.h"
#include <WBFLCore_i.c>
#include <WBFLCogo.h>
#include <WBFLCogo_i.c>

#include "PGSuperDataExporter.h"

#include "PGSuperInterfaces.h"
#include "\BSDev\PGSuperExtensions\BridgeSightBearings.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_PGSuperDataExporter,    CPGSuperDataExporter)
END_OBJECT_MAP()

class CKDOTExportAppPlugin : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKDOTExportAppPlugin)
	public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CKDOTExportAppPlugin)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CKDOTExportAppPlugin, CWinApp)
	//{{AFX_MSG_MAP(CKDOTExportAppPlugin)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CKDOTExportAppPlugin theApp;

BOOL CKDOTExportAppPlugin::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance, &LIBID_KDOTExport);
    return CWinApp::InitInstance();
}

int CKDOTExportAppPlugin::ExitInstance()
{
    _Module.Term();
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

void RegisterPlugins(bool bRegister)
{
   // Importer/Exporter Plugins

   // PGSuper
   sysComCatMgr::RegWithCategory(CLSID_PGSuperDataExporter,    CATID_PGSuperDataExporter,    bRegister);
   sysComCatMgr::RegWithCategory(CLSID_PGSuperDataExporter,    CATID_PGSpliceDataExporter,   bRegister);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _Module.RegisterServer(FALSE);
   if ( FAILED(hr) )
      return hr;

   RegisterPlugins(true);

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   RegisterPlugins(false);
   
   _Module.UnregisterServer();
	return S_OK;
}


