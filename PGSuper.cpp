///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// PGSuper.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PGSuper.h"

// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define BRIDGELINK_PLUGIN_COMMAND_BASE 0xC000 // 49152 (this gives us about 8100 plug commands)
#if BRIDGELINK_PLUGIN_COMMAND_BASE < _APS_NEXT_COMMAND_VALUE
#error "BridgeLink Application Plugins: Command IDs interfere with plug-in commands, change the plugin command base ID"
#endif

#include "MainFrm.h"
#include "PGSuperDocManager.h"

#include "ScreenSizeDlg.h"
#include <EAF\EAFAboutDlg.h>

#include <MFCTools\MFCTools.h>

#include "process.h"

#include <shlobj.h>

#include <initguid.h>
#include "BridgeLinkCatCom.h"
#include <WBFLCore.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


class CAboutDlg; // defined at bottom of file

// block multithreaded passes through exception handler
static bool sis_handling_neg_span = false;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp

BEGIN_MESSAGE_MAP(CPGSuperApp, CEAFPluginApp)
	//{{AFX_MSG_MAP(CPGSuperApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_JOINARPLIST, OnHelpJoinArpList)
	ON_COMMAND(ID_HELP_INET_WSDOT, OnHelpInetWsdot)
	ON_COMMAND(ID_HELP_INET_PGSUPER, OnHelpInetPgsuper)
   ON_COMMAND(ID_HELP_INET_ARP, OnHelpInetARP)
   ON_COMMAND(ID_SCREEN_SIZE,OnScreenSize)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp construction

CPGSuperApp::CPGSuperApp()
{
   SetHelpMode(afxHTMLHelp);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPGSuperApp object

CPGSuperApp theApp;
CComModule _Module;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {59D503E4-265C-11D2-8EB0-006097DF3C68}
static const CLSID clsid =
{ 0x59d503e4, 0x265c, 0x11d2, { 0x8e, 0xb0, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68 } };

// Required CEAFApp methods

CString CPGSuperApp::GetProductCode()
{
   // Return the Product Code from InstallShield
   // This code uniquely identifies PGSuper so don't change it
   return CString("{AF0C80B1-F24B-4F7F-AD58-E8DFD309BEFC}");
}

CEAFSplashScreenInfo CPGSuperApp::GetSplashScreenInfo()
{
   CBitmap bmp;
   CEAFSplashScreenInfo info;

   info.m_bShow = GetCommandLineInfo().m_bShowSplash;

   bmp.LoadBitmap(IDB_SPLASH);
   info.m_hBitmap   = bmp;
   info.m_TextColor = DARKGREEN;
   info.m_BgColor   = WHITE;
   info.m_Rect      = CRect(28,300,491,315);

   bmp.Detach();

   return info;
}

CDocManager* CPGSuperApp::CreateDocumentManager()
{
   return new CPGSuperDocManager;
}

LPCTSTR CPGSuperApp::GetRegistryKey()
{
   return _T("Washington State Department of Transportation");
}

OLECHAR* CPGSuperApp::GetAppPluginCategoryName()
{
   return _T("BridgeLink Application Plugin");
}

CATID CPGSuperApp::GetAppPluginCategoryID()
{
   return CATID_BridgeLinkAppPlugin;
}

CMDIFrameWnd* CPGSuperApp::CreateMainFrame()
{
   // don't call base class, the functionality is being replaced
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
   {
      delete pMainFrame;
		return NULL;
   }

   // files can be opened with drag and drop
   pMainFrame->DragAcceptFiles(TRUE);

   return pMainFrame;
}

CATID CPGSuperApp::GetComponentInfoCategoryID()
{
   return CATID_BridgeLinkComponents;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp initialization

BOOL CPGSuperApp::InitInstance()
{
   // Initialize OLE libraries
	if (!SUCCEEDED(OleInitialize(NULL)))
	{
		AfxMessageBox(_T("OLE initialization failed. Make sure that the OLE libraries are the correct version."));
		return FALSE;
	}

   sysComCatMgr::CreateCategory(_T("PGSuper Components"),CATID_BridgeLinkComponents);
   sysComCatMgr::CreateCategory(_T("PGSuper Application Plugin"),CATID_BridgeLinkAppPlugin);

//   CREATE_LOGFILE("PGSuperApp"); 

   // Tip of the Day
   CString strTipFile = GetAppLocation() + CString("PGSuper.tip");
#if defined _DEBUG
   strTipFile.Replace(_T("RegFreeCOM\\Debug\\"),_T(""));
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
   strTipFile.Replace(_T("RegFreeCOM\\Release\\"),_T(""));
#endif
   EnableTipOfTheDay(strTipFile); // must be enabled before InitInstance

   // Do this before InitInstace...
   // Set the first command ID for plugins that add commands to the interface
   GetPluginCommandManager()->SetBaseCommandID(BRIDGELINK_PLUGIN_COMMAND_BASE);

   // user can double click on a file to open
   EnableShellOpen();

   // Help file defaults to the location of the application
   // In our development environment, it is in the \ARP\PGSuper folder
   //
   // Change help file name
   CString strHelpFile(m_pszHelpFilePath);
#if defined _DEBUG
#if defined _WIN64
   strHelpFile.Replace(_T("RegFreeCOM\\x64\\Debug\\"),_T(""));
#else
   strHelpFile.Replace(_T("RegFreeCOM\\Win32\\Debug\\"),_T(""));
#endif
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
#if defined _WIN64
   strHelpFile.Replace(_T("RegFreeCOM\\x64\\Release\\"),_T(""));
#else
   strHelpFile.Replace(_T("RegFreeCOM\\Win32\\Release\\"),_T(""));
#endif
#endif
   free((void*)m_pszHelpFilePath);
   m_pszHelpFilePath = _tcsdup(strHelpFile);


  if ( !CEAFApp::InitInstance() )
      return FALSE;

	return TRUE;
}

int CPGSuperApp::ExitInstance() 
{
//   CLOSE_LOGFILE;

   ::OleUninitialize();

   return CEAFApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp commands

CString CPGSuperApp::GetVersion(bool bIncludeBuildNumber) const
{
   CString strExe( m_pszExeName );
   strExe += _T(".exe");

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

CString CPGSuperApp::GetVersionString(bool bIncludeBuildNumber) const
{
   CString str(_T("Version "));
   str += GetVersion(bIncludeBuildNumber);
#if defined _BETA_VERSION
   str += CString(_T(" BETA"));
#endif

   str += CString(_T(" - Built on "));
   str += CString(__DATE__);
   return str;
}

void CPGSuperApp::RegistryInit()
{
   CEAFApp::RegistryInit();

}

void CPGSuperApp::RegistryExit()
{
   CEAFApp::RegistryExit();
}

void CPGSuperApp::OnScreenSize()
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CScreenSizeDlg dlg;
   CRect rClient;
   pFrame->GetWindowRect(&rClient);
   dlg.m_Height = rClient.Height();
   dlg.m_Width  = rClient.Width();

   if ( dlg.DoModal() == IDOK )
   {
      int cx = dlg.m_Width;
      int cy = dlg.m_Height;
      pFrame->SetWindowPos(NULL,0,0,cx,cy,SWP_NOMOVE | SWP_NOZORDER);
   }
}

CString CPGSuperApp::GetWsdotUrl()
{
//   CString url = GetProfileString(_T("Settings"), _T("WsdotUrl"), _T("http://www.wsdot.wa.gov"));

   CString strDefault(_T("http://www.wsdot.wa.gov"));

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings"),0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   TCHAR url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("WsdotUrl"),0,&type,(LPBYTE)&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

CString CPGSuperApp::GetWsdotBridgeUrl()
{
//   CString url = GetProfileString(_T("Settings"), _T("WsdotBridgeUrl"), _T("http://www.wsdot.wa.gov/eesc/bridge"));

   CString strDefault(_T("http://www.wsdot.wa.gov/eesc/bridge"));

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings"),0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   TCHAR url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("WsdotBridgeUrl"),0,&type,(LPBYTE)&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

CString CPGSuperApp::GetPGSuperUrl()
{
   // NOTE: If URL isn't found in the registry, just go to the main software page.
//   CString url = GetProfileString(_T("Settings"), _T("PGSuperUrl"), _T("http://www.wsdot.wa.gov/eesc/bridge"));
   CString strDefault(_T("http://www.wsdot.wa.gov/eesc/bridge/software/index.cfm?fuseaction=software_detail&software_id=47"));

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings"),0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   TCHAR url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("PGSuperUrl"),0,&type,(LPBYTE)&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

void CPGSuperApp::OnHelpInetWsdot() 
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        _T("open"),
                                        GetWsdotUrl(),
                                         0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

void CPGSuperApp::OnHelpJoinArpList()
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        _T("open"),
										_T("http://www.pgsuper.com/drupal/forum"),
                                         0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

void CPGSuperApp::OnHelpInetARP()
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        _T("open"),
                                        _T("http://wsdot.wa.gov/eesc/bridge/alternateroute"),
                                         0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

void CPGSuperApp::OnHelpInetPgsuper() 
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        _T("open"),
                                        GetPGSuperUrl(),
                                        0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CEAFAboutDlg
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CMFCLinkCtrl m_WSDOT;
   CMFCLinkCtrl m_TxDOT;
   CMFCLinkCtrl m_KDOT;
   CMFCLinkCtrl m_BridgeSight;
};

CAboutDlg::CAboutDlg() : CEAFAboutDlg(AfxGetApp()->LoadIcon(IDR_MAINFRAME),IDD_ABOUTBOX)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CEAFAboutDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP

   DDX_Control(pDX,IDC_WSDOT,m_WSDOT);
   DDX_Control(pDX,IDC_TXDOT,m_TxDOT);
   DDX_Control(pDX,IDC_KDOT, m_KDOT);
   DDX_Control(pDX,IDC_BRIDGESIGHT,m_BridgeSight);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CEAFAboutDlg)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CEAFAboutDlg::OnInitDialog();
	

   m_WSDOT.SetURL(_T("http://www.wsdot.wa.gov"));
   m_WSDOT.SetTooltip(_T("http://www.wsdot.wa.gov"));
   m_WSDOT.SizeToContent();

   m_TxDOT.SetURL(_T("http://www.dot.state.tx.us"));
   m_TxDOT.SetTooltip(_T("http://www.dot.state.tx.us"));
   m_TxDOT.SizeToContent();

   m_KDOT.SetURL(_T("http://www.ksdot.org"));
   m_KDOT.SetTooltip(_T("http://www.ksdot.org"));
   m_KDOT.SizeToContent();

   m_BridgeSight.SetURL(_T("http://www.bridgesight.com"));
   m_BridgeSight.SetTooltip(_T("http://www.bridgesight.com"));
   m_BridgeSight.SizeToContent();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// App command to run the dialog
void CPGSuperApp::OnAppAbout()
{
   CAboutDlg dlg;
   dlg.DoModal();
}
