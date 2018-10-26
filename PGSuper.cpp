///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "PGSuperTemplateManager.h"

#include "PGSuperAppPlugin.h"
#include "PGSuperProjectImporterAppPlugin.h"
#include <psgLib\PGSuperLibrary_i.h>

#include "PGSuperDocTemplate.h" // for update template file information after cache updates

#include "MainFrm.h"
#include "PGSuperDoc.h"
#include "PGSuperDocManager.h"
#include "ConfigurePGSuperDlg.h"

#include "PGSuperCatCom.h"

#include <PGSuperUnits.h>
#include <PGSuperColors.h>

#include <System\StructuredLoadXml.h>
#include <System\ComCatMgr.h>
#include <fstream>

#include <MfcTools\MfcTools.h>

#include "PGSuperException.h"
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Test1250.h>

#include "ScreenSizeDlg.h"
#include <EAF\EAFAboutDlg.h>

#include "process.h"

#include <shlobj.h>

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

BEGIN_MESSAGE_MAP(CPGSuperApp, CEAFApp)
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

   m_CacheUpdateFrequency = Daily;
   m_SharedResourceType = srtInternetFtp;

//   m_Catalog.FakeNetworkError(true);
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
CEAFSplashScreenInfo CPGSuperApp::GetSplashScreenInfo()
{
   CBitmap bmp;
   CEAFSplashScreenInfo info;

   info.m_bShow = m_CommandLineInfo.m_bShowSplash;

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
   return "Washington State Department of Transportation";
}

BOOL CPGSuperApp::CreateApplicationPlugins()
{
   // NOTE: Once we go to application plugins that are registered with a com component category
   // this function will no longer be needed. When the change is made, we will use the
   // CEAFPluginApp as the base class and it implements CreateApplicationPlugins and there
   // isn't any reason for us to override it

   // Application plugin for regular PGSuper template based documents
   CComObject<CPGSuperAppPlugin>* pPGSuperAppPlugin;
   CComObject<CPGSuperAppPlugin>::CreateInstance(&pPGSuperAppPlugin);
   GetAppPluginManager()->AddPlugin(CLSID_PGSuperAppPlugin,pPGSuperAppPlugin);

   // Application plugin for PGSuper documents that are created from a PGSuper Project Importer plug-in
   CComObject<CPGSuperProjectImporterAppPlugin>* pPGSuperProjectImporterDocPlugin;
   CComObject<CPGSuperProjectImporterAppPlugin>::CreateInstance(&pPGSuperProjectImporterDocPlugin);
   GetAppPluginManager()->AddPlugin(CLSID_PGSuperProjectImporterDocumentPlugin,pPGSuperProjectImporterDocPlugin);

   //CComObject<CPGSpliceAppPlugin>* pPGSpliceAppPlugin;
   //CComObject<CPGSpliceAppPlugin>::CreateInstance(&pPGSpliceAppPlugin);
   //GetPluginManager()->AddPlugin(pPGSpliceAppPlugin);

   // Application plugin for Master Library Documents
   CComPtr<IEAFAppPlugin> pLibraryAppPlugin;
   HRESULT hr = pLibraryAppPlugin.CoCreateInstance(CLSID_LibraryAppPlugin);
   ATLASSERT(SUCCEEDED(hr));
   GetAppPluginManager()->AddPlugin(CLSID_LibraryAppPlugin,pLibraryAppPlugin);

   return TRUE;
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

CEAFCommandLineInfo& CPGSuperApp::GetCommandLineInfo()
{
   return m_CommandLineInfo;
}

CATID CPGSuperApp::GetComponentInfoCategoryID()
{
   return CATID_PGSuperComponents;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp initialization

BOOL CPGSuperApp::InitInstance()
{
   // Initialize OLE libraries
	if (!SUCCEEDED(OleInitialize(NULL)))
	{
		AfxMessageBox("OLE initialization failed. Make sure that the OLE libraries are the correct version.");
		return FALSE;
	}

   CREATE_LOGFILE("PGSuperApp"); 

   // This call will initialize the grid library
	GXInit( );   

   // Tip of the Day
   CString strTipFile = GetAppLocation() + CString("PGSuper.tip");
#if defined _DEBUG
   strTipFile.Replace("RegFreeCOM\\Debug\\","");
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
   strTipFile.Replace("RegFreeCOM\\Release\\","");
#endif
   EnableTipOfTheDay(strTipFile); // must be enabled before InitInstance

   // Do this before InitInstace...
   // Set the first command ID for plugins that add commands to the interface
   GetPluginCommandManager()->SetBaseCommandID(BRIDGELINK_PLUGIN_COMMAND_BASE);


   if ( !CEAFApp::InitInstance() )
      return FALSE;


   // Update any files that need to be cached from a web server
   // This needs to be generalized and moved into the base class
   // Don't update if we are testing
   if (!m_CommandLineInfo.m_bDo1250Test && !m_CommandLineInfo.m_DoTxCadReport)
      UpdateCache();

   // user can double click on a file to open
   EnableShellOpen();

   // Help file defaults to the location of the application
   // In our development environment, it is in the \ARP\PGSuper folder
   //
   // Change help file name
   CString strHelpFile(m_pszHelpFilePath);
#if defined _DEBUG
   strHelpFile.Replace("RegFreeCOM\\Debug\\","");
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
   strHelpFile.Replace("RegFreeCOM\\Release\\","");
#endif
   free((void*)m_pszHelpFilePath);
   m_pszHelpFilePath = _tcsdup(_T(strHelpFile));

   // command line
   // app.exe <AppPluginName> /Test <plugin defined test parameters>
   // /Test tells CEAFApp that we want to run unit tests
   // <AppPluginName> tells CEAFApp which app plugin to call
   // Need to add a generic Test(CEAFCommandLineInfo& cmdInfo); call to the IAppPlugin interface

   // The PGSuperAppPlugin object will look at the command line info and determine if we
   // have 1250 test or txDot tests and call the correct function. The 2 process methods below
   // will be moved into the PGSuperAppPlugin object

   // all command lines will be as follows
   // app.exe <paramters> - directed to the main exe, if parameter is a file name, find the correct plugin and open the file
   // app.exe AppPluginName <paramters> - directed to the plugin object

   //////////////////////// PGSUPER Doc Specific initialization ////////////////////
   //
   //
   // Do some special processing for 12-50 test cases
   if (m_CommandLineInfo.m_bDo1250Test)
   {
      Process1250Testing(m_CommandLineInfo);
   }
   else if (m_CommandLineInfo.m_DoTxCadReport)
   {
      ProcessTxDotCad(m_CommandLineInfo);
   }
   //
   //
   //////////////////////// PGSUPER Doc Specific initialization ////////////////////


   // We are doing command line processing, and it should have already happened...
   // At this point, the application is done running, but we don't want to return FALSE
   // because that says application initialization failed which isn't the case.
   // Close the documents and post a WM_QUITE message and return TRUE. This will cause
   // the application to close normally and to do all of its necessary cleanup
   if ( m_CommandLineInfo.m_CommandLineMode )
   {
      CloseAllDocuments(TRUE);
      AfxPostQuitMessage(0);
   }
   
	return TRUE;
}

int CPGSuperApp::ExitInstance() 
{
   // This call performs cleanup etc for the grid classes
	GXTerminate( );

   CLOSE_LOGFILE;

   ::OleUninitialize();

   return CEAFApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp commands

CString CPGSuperApp::GetVersion(bool bIncludeBuildNumber) const
{
   CString strExe( m_pszExeName );
   strExe += ".exe";

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

CString CPGSuperApp::GetVersionString(bool bIncludeBuildNumber) const
{
   CString str("Version ");
   str += GetVersion(bIncludeBuildNumber);
#if defined _BETA_VERSION
   str += CString(" BETA");
#endif

   str += CString(" - Built on ");
   str += CString(__DATE__);
   return str;
}








sysDate CPGSuperApp::GetInstallDate()
{
   HKEY hKey = GetUninstallRegistryKey();

   // The GUID used here is the Product Code from InstallShield
   // This code uniquely identifies PGSuper so don't change it
   CString strDate = GetLocalMachineString(hKey,_T("{AF0C80B1-F24B-4F7F-AD58-E8DFD309BEFC}"),_T("InstallDate"),_T("191001015"));
   int year  = atoi(strDate.Left(4));
   int month = atoi(strDate.Mid(4,2));
   int day   = atoi(strDate.Right(2));

   sysDate date(day,month,year);
   return date;
}

sysDate CPGSuperApp::GetLastRunDate()
{
   return m_LastRunDate;
}

void CPGSuperApp::RegistryInit()
{
   CEAFApp::RegistryInit();

   // The default values are read from HKEY_LOCAL_MACHINE\Software\Washington State Deparment of Transportation\PGSuper
   // If the default values are missing, the hard coded defaults found herein are used.
   // Install writers can create MSI transforms to alter the "defaults" by changing the registry values

   int iDefaultSharedResourceType     = GetLocalMachineInt(   _T("Settings"),_T("SharedResourceType"),    1);
   int iDefaultCacheUpdateFrequency   = GetLocalMachineInt(   _T("Settings"),_T("CacheUpdateFrequency"),  2);

   CString strDefaultUserTemplateFolder     = GetLocalMachineString(_T("Options"),_T("UserTemplateLocation"), _T("C:\\"));
   CString strDefaultCatalogServer          = GetLocalMachineString(_T("Options"),_T("CatalogServer"),_T("WSDOT"));
   CString strDefaultPublisher              = GetLocalMachineString(_T("Options"),_T("Publisher"),_T("WSDOT"));
   CString strDefaultLocalMasterLibraryFile = GetLocalMachineString(_T("Options"),_T("MasterLibraryLocal"),     GetDefaultMasterLibraryFile());
   CString strLocalWorkgroupTemplateFolder  = GetLocalMachineString(_T("Options"),_T("WorkgroupTemplatesLocal"),GetDefaultWorkgroupTemplateFolder());

   CString strDefaultCompany                = GetLocalMachineString(_T("Options"),_T("CompanyName"), _T("Your Company"));
   CString strDefaultEngineer               = GetLocalMachineString(_T("Options"),_T("EngineerName"),_T("Your Name"));

   // NOTE: Settings is an MFC created Registry section

   // Do any necessary conversions from previous versions of PGSuper
   RegistryConvert();

   /////////////////////////////////
   // User Information Settings
   /////////////////////////////////

   // company name
   m_CompanyName = GetProfileString(_T("Options"),_T("CompanyName"), strDefaultCompany);

   // engineer name
   m_EngineerName = GetProfileString(_T("Options"),_T("EngineerName"), strDefaultEngineer);

   /////////////////////////////////
   // Shared Resource Settings
   /////////////////////////////////

   // defaults
   CString strVersion = GetVersion(true);
   CString strFTPServer("ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper");
   CString strDefaultMasterLibraryURL;
   strDefaultMasterLibraryURL.Format("%s/Version_%s/WSDOT.lbr",strFTPServer,strVersion);
   CString strDefaultWorkgroupTemplateFolderURL;
   strDefaultWorkgroupTemplateFolderURL.Format("%s/Version_%s/WSDOT_Templates/",strFTPServer,strVersion);

   m_SharedResourceType   = (SharedResourceType)  GetProfileInt(_T("Settings"),_T("SharedResourceType"),  iDefaultSharedResourceType);
   m_CacheUpdateFrequency = (CacheUpdateFrequency)GetProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),iDefaultCacheUpdateFrequency);

   // Internet resources
   m_CatalogServers.LoadFromRegistry(this);
   m_CurrentCatalogServer = GetProfileString(_T("Options"),_T("CatalogServer"),strDefaultCatalogServer);

   m_Publisher = GetProfileString(_T("Options"),_T("Publisher"),strDefaultPublisher);

   m_MasterLibraryFileURL = GetProfileString(_T("Options"),_T("MasterLibraryURL"),strDefaultMasterLibraryURL);

   // Cache file/folder for Internet or Local Network resources
   m_MasterLibraryFileCache       = GetProfileString(_T("Options"),_T("MasterLibraryCache"),     GetCacheFolder()+GetMasterLibraryFileName());
   m_WorkgroupTemplateFolderCache = GetProfileString(_T("Options"),_T("WorkgroupTemplatesCache"),GetCacheFolder()+GetTemplateSubFolderName()+"\\");

   // the "default" time of last run will be the install time less one day so that if the
   // "LastRun" setting is not in the registry, this will look like there was a new install
   // prior to this run
   sysDate install_date = GetInstallDate();
   --install_date;
   sysTime default_time(install_date);
   
   ClockTy last_run = GetProfileInt(_T("Settings"),_T("LastRun"),default_time.Seconds());
   m_LastRunDate = sysTime(last_run);
}

void CPGSuperApp::RegistryConvert()
{
   // Prior to version 2.2.3 we allowed only a single local file system catalog server and the
   // app class did much of the server heavy lifting. The code below removes those registry keys
   // and converts the data to the new format

   // This code was written in Jan, 2010. If you are reading this in, say 2012, you 
   // are probably safe to blast it as nobody should be using 2 year old versions of PGSuper

   SharedResourceType type   = (SharedResourceType)  GetProfileInt(_T("Settings"),_T("SharedResourceType"),  -1);
   CString masterlib  = GetProfileString(_T("Options"),_T("MasterLibraryLocal"), _T("Bogus"));
   CString tempfolder = GetProfileString(_T("Options"),_T("WorkgroupTemplatesLocal"), _T("Bogus"));

   // If we were set to a local library stored in old format, create a new catalog server 
   // and add its settings to the registry
   if (masterlib!="Bogus" && type==srtLocal)
   {
      // our publisher and server is "Local Files"
      WriteProfileString(_T("Options"),_T("CatalogServer"),"Local Files");
      WriteProfileString(_T("Options"),_T("Publisher"),    "Local Files");

      // create that server
      CFileSystemPGSuperCatalogServer server("Local Files",masterlib,tempfolder);
      CString create_string = GetCreationString(&server);

      int count = GetProfileInt(_T("Servers"),_T("Count"),-1);
      count==-1 ? count=1 : count++;
      WriteProfileInt(_T("Servers"),_T("Count"),count);

      CString key(char(count-1+'A'));
      WriteProfileString(_T("Servers"), key, create_string);
   }

   // Delete profile strings if they are not empty
   if (masterlib!="Bogus")
   {
      WriteProfileString(_T("Options"),_T("MasterLibraryLocal"),NULL);
   }

   if (tempfolder!="Bogus")
   {
      WriteProfileString(_T("Options"),_T("WorkgroupTemplatesLocal"), NULL);
   }

   //  m_WorkgroupTemplateFolderURL is no longer used
   WriteProfileString(_T("Options"),_T("WorkgroupTemplatesURL"),NULL);

   // user template location no longer used
   WriteProfileString(_T("Options"), _T("UserTemplateLocation"), NULL);

}

void CPGSuperApp::RegistryExit()
{
   CEAFApp::RegistryExit();


   // Options settings
   VERIFY(WriteProfileString(_T("Options"), _T("CompanyName"), m_CompanyName));

   // engineer name
   VERIFY(WriteProfileString(_T("Options"), _T("EngineerName"), m_EngineerName));

   WriteProfileInt(_T("Settings"),_T("SharedResourceType"),m_SharedResourceType);
   WriteProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),m_CacheUpdateFrequency);

   // Internet resources
   WriteProfileString(_T("Options"),_T("CatalogServer"),m_CurrentCatalogServer);
   WriteProfileString(_T("Options"),_T("Publisher"),m_Publisher);
   WriteProfileString(_T("Options"),_T("MasterLibraryURL"),m_MasterLibraryFileURL);

   // Cache file/folder for Internet or Local Network resources
   WriteProfileString(_T("Options"),_T("MasterLibraryCache"),     m_MasterLibraryFileCache);
   WriteProfileString(_T("Options"),_T("WorkgroupTemplatesCache"),m_WorkgroupTemplateFolderCache);
   
   sysTime time;
   WriteProfileInt(_T("Settings"),_T("LastRun"),time.Seconds());

   m_CatalogServers.SaveToRegistry(this);
}

///////////////////// back door functions to make managing PGSuper easier //////////////////
//                                                                                        //

void CPGSuperApp::OnScreenSize()
{
   CWnd* pWnd = AfxGetMainWnd();
   CScreenSizeDlg dlg;
   CRect rClient;
   pWnd->GetWindowRect(&rClient);
   dlg.m_Height = rClient.Height();
   dlg.m_Width  = rClient.Width();

   if ( dlg.DoModal() == IDOK )
   {
      int cx = dlg.m_Width;
      int cy = dlg.m_Height;
      pWnd->SetWindowPos(NULL,0,0,cx,cy,SWP_NOMOVE | SWP_NOZORDER);
   }
}
//                                                                                        //
///////////////////// back door functions to make managing PGSuper easier //////////////////

void CPGSuperApp::OnProgramSettings(BOOL bFirstRun) 
{
   if (!bFirstRun && IsDocLoaded())
   {
      AfxMessageBox("Program settings cannot be changed while a project is open. Close this project and try again.",MB_OK|MB_ICONINFORMATION);
   }
   else
   {
      CConfigurePGSuperDlg dlg(bFirstRun,AfxGetMainWnd());

      dlg.m_Company  = m_CompanyName;
      dlg.m_Engineer = m_EngineerName;

      dlg.m_SharedResourceType               = m_SharedResourceType;
      dlg.m_CacheUpdateFrequency = m_CacheUpdateFrequency;
      dlg.m_CurrentServer  = m_CurrentCatalogServer;
      dlg.m_Publisher      = m_Publisher;
      dlg.m_Servers = m_CatalogServers;

      // Save a copy of all server information in case our update fails
      SharedResourceType        original_type = m_SharedResourceType;
      CacheUpdateFrequency      original_freq = m_CacheUpdateFrequency;
      CString                 original_server = m_CurrentCatalogServer;
      CString              original_publisher = m_Publisher;
      CPGSuperCatalogServers original_servers = m_CatalogServers;

      int result = dlg.DoModal();

      if ( result == IDOK )
      {
         m_EngineerName         = dlg.m_Engineer;
         m_CompanyName          = dlg.m_Company;

         m_SharedResourceType   = dlg.m_SharedResourceType;
         m_CacheUpdateFrequency = dlg.m_CacheUpdateFrequency;

         m_CatalogServers       = dlg.m_Servers;
         m_CurrentCatalogServer = dlg.m_CurrentServer;
         m_Publisher            = dlg.m_Publisher;

         if ( m_SharedResourceType == srtDefault )
         {
            // Using hard coded defaults
            RestoreLibraryAndTemplatesToDefault();
         }
      }

      if ( dlg.m_bUpdateCache )
      {
         if (!DoCacheUpdate())
         {  
            // DoCacheUpdate will restore the cache, we need also to restore local data
            m_SharedResourceType   = original_type;
            m_CacheUpdateFrequency = original_freq;
            m_CurrentCatalogServer = original_server;
            m_Publisher            = original_publisher;
            m_CatalogServers       = original_servers;
         }
      }

      if ( result == IDOK )
      {
         RegistryExit(); // Saves all the current settings to the registery
                         // There is no sense waiting until PGSuper closes to do this
       }
   }
}

CString CPGSuperApp::GetEngineerName()
{
   return m_EngineerName;
}

CString CPGSuperApp::GetEngineerCompany()
{
   return m_CompanyName;
}

LRESULT CPGSuperApp::ProcessWndProcException(CException* e, const MSG* pMsg) 
{
   LRESULT lResult = 0L;

   if ( e->IsKindOf(RUNTIME_CLASS(CInternetException) ) )
   {
      lResult = 1L;
      AfxMessageBox("There was a critical error accessing the Master Library and Workgroup Templates on the Internet.\n\nPGSuper will use the generic libraries and templates that were installed with the application.\n\nUse File | Program Settings to reset the Master Library and Workgroup Templates to shared Internet resources at a later date.",MB_OK | MB_ICONEXCLAMATION);
      m_SharedResourceType = srtDefault;
      m_MasterLibraryFileCache       = GetDefaultMasterLibraryFile();
      m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();
   }
   else
   {
      lResult = CEAFApp::ProcessWndProcException(e, pMsg);
   }

	return lResult;
}

CString CPGSuperApp::GetWsdotUrl()
{
//   CString url = GetProfileString(_T("Settings"), _T("WsdotUrl"), _T("http://www.wsdot.wa.gov"));

   CString strDefault("http://www.wsdot.wa.gov");

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings",0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   unsigned char url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("WsdotUrl"),0,&type,&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

CString CPGSuperApp::GetWsdotBridgeUrl()
{
//   CString url = GetProfileString(_T("Settings"), _T("WsdotBridgeUrl"), _T("http://www.wsdot.wa.gov/eesc/bridge"));

   CString strDefault("http://www.wsdot.wa.gov/eesc/bridge");

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings",0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   unsigned char url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("WsdotBridgeUrl"),0,&type,&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

CString CPGSuperApp::GetPGSuperUrl()
{
   // NOTE: If URL isn't found in the registry, just go to the main software page.
//   CString url = GetProfileString(_T("Settings"), _T("PGSuperUrl"), _T("http://www.wsdot.wa.gov/eesc/bridge"));
   CString strDefault("http://www.wsdot.wa.gov/eesc/bridge/software/index.cfm?fuseaction=software_detail&software_id=47");

   HKEY key;
   LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Washington State Department of Transportation\\PGSuper\\Settings",0,KEY_QUERY_VALUE,&key);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   unsigned char url[MAX_PATH];
   DWORD size = MAX_PATH;
   DWORD type;
   result = ::RegQueryValueEx(key,_T("PGSuperUrl"),0,&type,&url[0],&size);
   if ( result != ERROR_SUCCESS )
      return strDefault;

   ::RegCloseKey(key);

   return url;
}

void CPGSuperApp::OnHelpInetWsdot() 
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        "open",
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
                                        "open",
										"http://www.pgsuper.com/drupal/content/arplist",
                                         0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

void CPGSuperApp::OnHelpInetARP()
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        "open",
                                        "http://wsdot.wa.gov/eesc/bridge/alternateroute",
                                         0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}

void CPGSuperApp::OnHelpInetPgsuper() 
{
   HINSTANCE hInstance = ::ShellExecute(m_pMainWnd->GetSafeHwnd(),
                                        "open",
                                        GetPGSuperUrl(),
                                        0,0,SW_SHOWDEFAULT);

   if ( (INT)hInstance < 32 )
   {
      AfxMessageBox(IDS_E_ONLINERESOURCES);
   }
}






CString CPGSuperApp::GetCachedMasterLibraryFile()
{
   return m_MasterLibraryFileCache;
}

CString CPGSuperApp::GetMasterLibraryFile()
{
   CString strMasterLibFile;
   switch( m_SharedResourceType )
   {
   case srtDefault:
      strMasterLibFile = GetDefaultMasterLibraryFile();
      break;

   case srtInternetFtp:
   case srtInternetHttp:
   case srtLocal:
      {
         // use our cached value
         strMasterLibFile = m_MasterLibraryFileURL;
      }
      break;

   default:
      ATLASSERT(false);
      strMasterLibFile = GetDefaultMasterLibraryFile();
      break;
   }

   return strMasterLibFile;
}

void CPGSuperApp::GetTemplateFolders(CString& strWorkgroupFolder)
{
   strWorkgroupFolder = m_WorkgroupTemplateFolderCache;
}


void CPGSuperApp::SetCacheUpdateFrequency(CacheUpdateFrequency frequency)
{
   m_CacheUpdateFrequency = frequency;
}

CacheUpdateFrequency CPGSuperApp::GetCacheUpdateFrequency()
{
   return m_CacheUpdateFrequency;
}

void CPGSuperApp::SetSharedResourceType(SharedResourceType resType)
{
   m_SharedResourceType = resType;
}

SharedResourceType CPGSuperApp::GetSharedResourceType()
{
   return m_SharedResourceType;
}

CString CPGSuperApp::GetMasterLibraryPublisher() const
{
   CString strPublisher;
   switch( m_SharedResourceType )
   {
   case srtDefault:
      strPublisher = "Default libraries installed with PGSuper";
      break;

   case srtInternetFtp:
   case srtInternetHttp:
      strPublisher = m_Publisher;
      break;

   case srtLocal:
      strPublisher = "Published on Local Network";
      break;

   default:
      ATLASSERT(false);
      break;
   }

   return strPublisher;
}

void CPGSuperApp::UpdateCache()
{
   bool was_error=false;
   CString error_msg;

   try
   {
      BOOL bFirstRun = IsFirstRun();
      if (bFirstRun)
      {
         LOG("Update Cache -> First Run");

         // if this is the first time PGSuper is run after installation
         // go right to the program settings. OnProgramSettings will
         // force an update
         OnProgramSettings(TRUE); 
      }
      else if ( IsTimeToUpdateCache() && AreUpdatesPending() )
      {
         LOG("Time to update cache and Updates are pending");
         // this is not the first time, it is time to check for updates,
         // and sure enough there are updates pending.... do the update
         int result = ::MessageBox(AfxGetMainWnd()->GetSafeHwnd(),"There are updates to Master Library and Workgroup Templates pending.\n\nWould you like to update PGSuper now?","Pending Updates",MB_YESNO | MB_ICONINFORMATION);

         if ( result == IDYES )
            was_error = !DoCacheUpdate();
      }
   }
   catch(CCatalogServerException exp)
   {
      // catch and report error message
      error_msg = exp.GetErrorMessage();
      was_error = true;
   }
   catch(...)
   {
      error_msg = "Error cause was unknown";
      was_error = true;
   }

   if (was_error)
   {
      // Things are totally screwed if we end up here. Reset to default library and templates from install
      RestoreLibraryAndTemplatesToDefault();
      CString msg;
      msg.Format("The following error occurred while loading Library and Template server information:\n %s \nThese settings have been restored to factory defaults.",error_msg);
      ::AfxMessageBox(msg,MB_ICONINFORMATION);
   }
}

void CPGSuperApp::RestoreLibraryAndTemplatesToDefault()
{
   m_SharedResourceType = srtDefault;

   m_MasterLibraryFileCache       = GetDefaultMasterLibraryFile();
   m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();

   m_MasterLibraryFileURL = m_MasterLibraryFileCache;
}

void CPGSuperApp::DeleteCache(LPCSTR pstrCache)
{
   RecursiveDelete(pstrCache);
   ::RemoveDirectory(pstrCache);
}

void CPGSuperApp::RecursiveDelete(LPCSTR pstr)
{
   CFileFind finder;

   CString strWildcard(pstr);
   strWildcard += "\\*.*";

   BOOL bWorking = finder.FindFile(strWildcard);
   while ( bWorking )
   {
      bWorking = finder.FindNextFile();

      if ( finder.IsDots() )
         continue;

      CString str = finder.GetFilePath();
      if ( finder.IsDirectory() )
      {
         RecursiveDelete(str);

         ::RemoveDirectory(str);
      }
      else
      {
         ::DeleteFile(str);
      }
   }

   finder.Close();
}

bool CPGSuperApp::IsTimeToUpdateCache()
{
   LOG("IsTimeToUpdateCache()");
   if ( m_SharedResourceType == srtDefault )
   {
      LOG("Using default resource type, no updating");
      return false;
   }

   // resources are local network or Internet
   bool bTimeToUpdate = false;
   if ( m_CacheUpdateFrequency == Never )
   {
      LOG("Never update");
      bTimeToUpdate = false;
   }
   else if ( m_CacheUpdateFrequency == Always )
   {
      LOG("Always update");
      bTimeToUpdate = true;
   }
   else if ( m_CacheUpdateFrequency == Daily )
   {
      LOG("Update daily");
      CTime now = CTime::GetCurrentTime();
      CTime last = GetLastCacheUpdateTime();
      CTimeSpan duration = now - last;
      LOG("Duration = " << Int32(duration.GetDays()));
      bTimeToUpdate = (1 <= duration.GetDays());
   }
   else if ( m_CacheUpdateFrequency == Weekly )
   {
      LOG("Update weekly");
      CTime now = CTime::GetCurrentTime();
      CTime last = GetLastCacheUpdateTime();
      CTimeSpan duration = now - last;
      LOG("Duration = " << Uint32(duration.GetDays()));
      bTimeToUpdate = (7 <= duration.GetDays());
   }
   else if ( m_CacheUpdateFrequency == Monthly )
   {
      LOG("Update monthly");
      CTime now = CTime::GetCurrentTime();
      CTime last = GetLastCacheUpdateTime();
      LOG("now = " << now.GetMonth() << " last = " << last.GetMonth());
      bTimeToUpdate = (now.GetMonth() != last.GetMonth() ||  // not the same month
                (now.GetMonth() == last.GetMonth() && now.GetYear() != last.GetYear())); // same month, but different years
   }

   LOG("Time to update = " << (bTimeToUpdate ? "Yes" : "No"));
   return bTimeToUpdate;
}

bool CPGSuperApp::AreUpdatesPending()
{
   // get the MD5 files from the Internet or local network, compute the MD5 of the cache
   // if different, then there is an update pending

   LOG("AreUpdatesPending()");

   bool bUpdatesPending = false;
   if ( m_SharedResourceType == srtDefault )
   {
      bUpdatesPending = false;
   }
   else if ( m_SharedResourceType == srtInternetFtp ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal)
   {

      // create a progress window
      CComPtr<IProgressMonitorWindow> wndProgress;
      wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
      wndProgress->put_HasGauge(VARIANT_FALSE);
      wndProgress->put_HasCancel(VARIANT_FALSE);
      wndProgress->Show(CComBSTR("Checking the Internet for Library updates"),NULL);

      try
      {
         // Catalog server does the work here
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         bUpdatesPending = pserver->CheckForUpdates(m_Publisher, NULL, GetCacheFolder());
      }
      catch(...)
      {
         throw;
      }
   }
   else
   {
      ATLASSERT(0);
   }

   LOG("Pending updates = " << (bUpdatesPending ? "Yes" : "No"));
   return bUpdatesPending;
}


bool CPGSuperApp::DoCacheUpdate()
{
   LOG("DoUpdateCache()");

   // create a progress window
   CComPtr<IProgressMonitorWindow> wndProgress;
   wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
   wndProgress->put_HasGauge(VARIANT_FALSE);
   wndProgress->put_HasCancel(VARIANT_FALSE);

   CComQIPtr<IProgressMonitor> progress(wndProgress);
   wndProgress->Show(CComBSTR("Update Libraries and Templates"),NULL);

   // setup cache folders
   CString strAppPath = GetAppLocation();

   CString strSaveCache = GetSaveCacheFolder();
   CString strCache     = GetCacheFolder();

   SetCurrentDirectory(strAppPath);

   // Save the current cache in case of failure
   DeleteCache(strSaveCache); // be safe and delete the previous "SaveCache" if one exists
   int retval = rename(strCache,strSaveCache); // rename the existing cache to "SaveCache" in case there is an update error
   if ( retval != 0 )
   {
      // there was an error renaming the cache
      // just delete its contents
      DeleteCache(strCache);
   }

   ::CreateDirectory(strCache,0); // create a new cache folder to fill up
   ::SetFileAttributes(strCache,FILE_ATTRIBUTE_HIDDEN); // make the cache a hidden folder

   // fill up the cache
   bool bSuccessful(false);
   if ( m_SharedResourceType == srtDefault )
   {
      m_MasterLibraryFileCache = GetDefaultMasterLibraryFile();
      m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();
      return true;
   }
   else if ( m_SharedResourceType == srtInternetFtp ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal)
   {
      // set cache folder
      m_MasterLibraryFileCache = GetCacheFolder() + GetMasterLibraryFileName();
      m_WorkgroupTemplateFolderCache = GetCacheFolder() + GetTemplateSubFolderName() + CString("\\");

      // Catalog server takes care of business
      try
      {
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         bSuccessful = pserver->PopulateCatalog(m_Publisher,progress,GetCacheFolder());

         m_MasterLibraryFileURL = pserver->GetMasterLibraryURL(m_Publisher);
      }
      catch(CCatalogServerException exp)
      {
         // catch and report error message
         CString msg = exp.GetErrorMessage();
         AfxMessageBox(msg,MB_ICONEXCLAMATION);
         bSuccessful = false;
      }
      catch(...)
      {
         ATLASSERT(0);
         bSuccessful = false;
      }
   }
   else
   {
      ATLASSERT(0);
   }

   // if the cache was successfully updated, delete the saved copy and update the time stamp
   if ( bSuccessful )
   {
      DeleteCache(strSaveCache);
      CTime now = CTime::GetCurrentTime();
      SetLastCacheUpdateTime(now);
   }
   else
   {
      // otherwise, delete the messed up cache and put it back the way it was
      DeleteCache(strCache);
      rename(strSaveCache,strCache);
   }

   if (bSuccessful)
      progress->put_Message(0,CComBSTR("The Master Library and Templates have been updated"));
   else
      progress->put_Message(0,CComBSTR("Update failed. Previous settings restored."));

   if ( bSuccessful )
   {
      // Need to update the main document template so that the File | New dialog is updated
      // Search for the CPGSuperDocTemplate object
      POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
      do
      {
         CDocTemplate* pDocTemplate = m_pDocManager->GetNextDocTemplate(pos);
         if ( pDocTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperDocTemplate)) )
         {
            CPGSuperDocTemplate* pPGSuperDocTemplate = dynamic_cast<CPGSuperDocTemplate*>(pDocTemplate);
            pPGSuperDocTemplate->LoadTemplateInformation();
            break;
         }
      } while ( pos != NULL );
   }

   return bSuccessful;
}

CTime CPGSuperApp::GetLastCacheUpdateTime()
{
   time_t last_update = GetProfileInt(_T("Settings"),_T("LastCacheUpdate"),0);
   return CTime(last_update);
}

void CPGSuperApp::SetLastCacheUpdateTime(const CTime& time)
{
   WriteProfileInt(_T("Settings"),_T("LastCacheUpdate"),int(time.GetTime()));
}

BOOL CPGSuperApp::IsFirstRun()
{
   return ( GetLastRunDate() < GetInstallDate() ) ? TRUE : FALSE;
}

CString CPGSuperApp::GetDefaultMasterLibraryFile()
{
   CString strAppPath = GetAppLocation();
   return strAppPath + CString("WSDOT.lbr");
}

CString CPGSuperApp::GetDefaultWorkgroupTemplateFolder()
{
   CString strAppPath = GetAppLocation();
   return strAppPath + CString("Templates");
}

CString CPGSuperApp::GetCacheFolder()
{
   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return GetAppLocation() + CString("Cache\\");
   else
      return CString(buffer) + CString("\\PGSuper\\");
}

CString CPGSuperApp::GetSaveCacheFolder()
{
   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return GetAppLocation() + CString("SaveCache\\");
   else
      return CString(buffer) + CString("\\PGSuper_Save\\");
}



bool create_txdot_file_names(const CString& output, CString* pErrFileName)
{
   CString tmp(output);
   tmp.MakeLower();
   int loc = tmp.Find(".",0);
   if (loc>0)
   {
      CString basename = output.Left(loc);
      *pErrFileName = basename + ".err";
      return true;
   }
   else
   {
      *pErrFileName = output + ".err";

      return false;
   }
}

void CPGSuperApp::Process1250Testing(const CPGSuperCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_bDo1250Test);

   // The document is opened when CEAFApp::InitInstance calls ProcessShellCommand
   // Get the document
   CEAFMainFrame* pMainFrame = (CEAFMainFrame*)AfxGetMainWnd();
   CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pMainFrame->GetDocument();

   CComPtr<IBroker> pBroker;
   pPgsDoc->GetBroker(&pBroker);
   GET_IFACE2( pBroker, ITest1250, ptst );

   CString resultsfile, poifile, errfile;
   if (create_test_file_names(rCmdInfo.m_strFileName,&resultsfile,&poifile,&errfile))
   {
      try
      {
         if (!ptst->RunTest(rCmdInfo.m_SubdomainId, std::string(resultsfile), std::string(poifile)))
         {
            CString msg = CString("Error - Running test on file")+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }

         if ( pPgsDoc->IsModified() )
            pPgsDoc->DoFileSave();
      }
      catch(const sysXBase& e)
      {
         std::string msg;
         e.GetErrorMessage(&msg);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running test for input file: "<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running test for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running test for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::string strMsg(pex->what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running test for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
         std::string strMsg(ex.what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running test for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::ofstream os;
         os.open(errfile);
         os <<"Unknown Error running test for input file: "<<rCmdInfo.m_strFileName;
      }
   }
}

void CPGSuperApp::ProcessTxDotCad(const CPGSuperCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_DoTxCadReport);


   if (rCmdInfo.m_TxGirder != TXALLGIRDERS && 
       rCmdInfo.m_TxGirder != TXEIGIRDERS && 
       (rCmdInfo.m_TxGirder < 0 || 27 < rCmdInfo.m_TxGirder))
   {
      ::AfxMessageBox("Invalid girder specified on command line for TxDOT CAD report");
      return;
   }

   if (rCmdInfo.m_TxSpan != ALL_SPANS && rCmdInfo.m_TxSpan < 0)
   {
      ::AfxMessageBox("Invalid span specified on command line for TxDOT CAD report");
      return;
   }

   // The document is opened when CEAFApp::InitInstance calls ProcessShellCommand
   // Get the document
   CEAFMainFrame* pMainFrame = (CEAFMainFrame*)AfxGetMainWnd();
   CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pMainFrame->GetDocument();

   CComPtr<IBroker> pBroker;
   pPgsDoc->GetBroker(&pBroker);
   GET_IFACE2( pBroker, ITest1250, ptst );

   CString errfile;
   if (create_txdot_file_names(rCmdInfo.m_TxOutputFile, &errfile))
   {
      try
      {
         if ( !pPgsDoc->DoTxDotCadReport(rCmdInfo.m_TxOutputFile, errfile, rCmdInfo) )
         {
            CString msg = CString("Error - Running test on file")+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }
      }
      catch(const sysXBase& e)
      {
         std::string msg;
         e.GetErrorMessage(&msg);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::string strMsg(pex->what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
          std::string strMsg(ex.what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::ofstream os;
         os.open(errfile);
         os <<"Unknown Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName;
      }
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

   CHyperLink m_WSDOT;
   CHyperLink m_TxDOT;
   CHyperLink m_BridgeSight;
};

CAboutDlg::CAboutDlg() : CEAFAboutDlg(AfxGetApp()->LoadIconA(IDR_MAINFRAME),IDD_ABOUTBOX)
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
   DDX_Control(pDX,IDC_BRIDGESIGHT,m_BridgeSight);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CEAFAboutDlg)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CEAFAboutDlg::OnInitDialog();
	

   m_WSDOT.SetURL("http://www.wsdot.wa.gov/");
   m_TxDOT.SetURL("http://www.dot.state.tx.us/");
   m_BridgeSight.SetURL("http://www.bridgesight.com/");

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// App command to run the dialog
void CPGSuperApp::OnAppAbout()
{
   CAboutDlg dlg;
   dlg.DoModal();
}
