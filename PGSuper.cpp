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
#include "PGSuperDocManager.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "LibChildFrm.h"
#include "OutputChildFrame.h"
#include "BridgeModelViewChildFrame.h"
#include "FactorOfSafetyView.h"
#include "FactorOfSafetyChildFrame.h"
#include "EditLoadsView.h"
#include "EditLoadsChildFrm.h"
#include "AnalysisResultsView.h"
#include "AnalysisResultsChildFrame.h"
#include "GirderModelChildFrame.h"
#include "PGSuperDoc.h"
#include "Splash.h"
#include "PgsuperCommandLineInfo.h"
#include "ConfigurePGSuperDlg.h"

#include "MyChildFrame.h"
#include "LibraryEditorDoc.h"

#include <PGSuperUnits.h>

#include <System\StructuredLoadXml.h>
#include <System\ComCatMgr.h>
#include <fstream>

#include <MfcTools\MfcTools.h>

#include <PsgLib\BeamFamilyManager.h>

#include "PGSuperException.h"
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\DisplayUnits.h>
#include <IFace\Test1250.h>

#include <Reporting\ReportStyleHolder.h>

#include "PGSuperCatCom.h"
#include "Plugins\PGSuperIEPlugin.h"

#include "ScreenSizeDlg.h"

#include "process.h"

#include <shlobj.h>

#include <occimpl.h>
#include "custsite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void save_doc(CDocument* pDoc,void* pStuff)
{
   if ( pDoc->IsModified() )
         pDoc->DoFileSave();
}

void edit_bridge(CDocument* pDoc,void* pStuff)
{
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pDoc;
      pPgsDoc->EditBridgeDescription(*((int*)pStuff));
   }
}

// Global function for getting the broker from the current document
HRESULT AfxGetBroker(IBroker** ppBroker)
{
   // let's try it the easy way first
   CMainFrame* pWnd = (CMainFrame*)AfxGetMainWnd();
   CPGSuperDoc* pDoc = pWnd->GetPGSuperDocument();
   if ( !pDoc )
   {
      // looks like we have to do it the hard way
      CWinApp* pApp = AfxGetApp();
      CDocument* pDocument = NULL;
      bool bDone = false;
      POSITION doc_template_pos = pApp->GetFirstDocTemplatePosition();

      while ( doc_template_pos != NULL || !bDone )
      {
         CDocTemplate* pTemplate = pApp->GetNextDocTemplate(doc_template_pos);

         POSITION doc_pos = pTemplate->GetFirstDocPosition();
         while ( doc_pos != NULL )
         {
            pDocument = pTemplate->GetNextDoc(doc_pos);
            if ( pDocument )
            {
               bDone = true;
               break;
            }
         }
      }

      ASSERT_KINDOF(CPGSuperDoc,pDocument);
      pDoc = (CPGSuperDoc*)pDocument;
   }

   return pDoc->GetBroker(ppBroker);
}

// block multithreaded passes through exception handler
static bool sis_handling_neg_span = false;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp

BEGIN_MESSAGE_MAP(CPGSuperApp, CWinApp)
	ON_COMMAND(CG_IDS_TIPOFTHEDAY, ShowTipOfTheDay)
	//{{AFX_MSG_MAP(CPGSuperApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
   ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_VIEW_OPTIONS, OnProgramSettings)
	ON_COMMAND(ID_HELP_JOINARPLIST, OnHelpJoinArpList)
	ON_COMMAND(ID_HELP_INET_WSDOT, OnHelpInetWsdot)
	ON_COMMAND(ID_HELP_INET_PGSUPER, OnHelpInetPgsuper)
   ON_COMMAND(ID_HELP_INET_ARP, OnHelpInetARP)
	ON_COMMAND(ID_APP_LEGAL, OnAppLegal)
   ON_COMMAND(ID_UPDATE_TEMPLATE,OnUpdateTemplate)
   ON_COMMAND(ID_SCREEN_SIZE,OnScreenSize)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_UPDATE_COMMAND_UI(FIRST_IMPORT_PLUGIN, OnImportMenu)
   ON_COMMAND_RANGE(FIRST_IMPORT_PLUGIN,LAST_IMPORT_PLUGIN, OnImport)
	ON_UPDATE_COMMAND_UI(FIRST_EXPORT_PLUGIN, OnExportMenu)
   ON_COMMAND_RANGE(FIRST_EXPORT_PLUGIN,LAST_EXPORT_PLUGIN, OnExport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp construction

CPGSuperApp::CPGSuperApp() :
m_strWindowPlacementFormat("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d")
{
   m_bAutoCalcEnabled = true;
   m_bShowLegalNotice = VARIANT_TRUE;
   m_bShowProjectProperties = true;

   // Setup system units, Must be same as the PGSuper Library Editor
   ::InitUnitSystem();

   m_CacheUpdateFrequency = Daily;
   m_SharedResourceType = srtInternetFtp;

   m_bUpdatingTemplate = false;

//   m_Catalog.FakeNetworkError(true);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPGSuperApp object

CPGSuperApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {59D503E4-265C-11D2-8EB0-006097DF3C68}
static const CLSID clsid =
{ 0x59d503e4, 0x265c, 0x11d2, { 0x8e, 0xb0, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68 } };

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp initialization

BOOL CPGSuperApp::InitInstance()
{
   // Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

   // This call will initialize the grid library
	GXInit( );

   CBeamFamilyManager::Init();

	// CG: The following block was added by the Splash Screen component.
	CPGSuperCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

   // command line could not be parsed.
   if (cmdInfo.m_bAbort)
   {
      return FALSE;
   }
  // we don't show splash when in command line mode
  CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
  CSplashWnd::ShowSplashScreen();

   if ( !CreateAppUnitSystem(&m_AppUnitSystem) )
      return FALSE;

   CCustomOccManager *pMgr = new CCustomOccManager;

	// Set our control containment up but using our control container 
	// management class instead of MFC's default
	AfxEnableControlContainer(pMgr);


	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

   // Initialize the registry key for this App and
   // read out any important values.
   RegistryInit();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
   RegDocTemplates();

	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, m_pBridgeModelEditorTemplate, FALSE);

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleTemplateServer::RegisterAll();
   sysComCatMgr::CreateCategory(L"PGSuper Agent",CATID_PGSuperAgent);
   sysComCatMgr::CreateCategory(L"PGSuper Beam Family",CATID_BeamFamily);
   sysComCatMgr::CreateCategory(L"PGSuper IE Plugin",CATID_PGSuperIEPlugin);

   // Legal Notice - start up
   if (!cmdInfo.m_CommandLineMode )
   {
      if ( ShowLegalNoticeAtStartup() == atReject )
         return FALSE; // License was not accepted
   }

   // Note: MDI applications register all server objects without regard
		//  to the /Embedding or /Automation on the command line.
   
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
// The installer is taking care of this now (9, Oct, 2006)
//	RegisterShellFileTypes(FALSE); // Use FALSE and Print and PrintTo wont be added to
//                                  // the context menu in the Windows Explorer

	// Parse command line for standard shell commands, DDE, file open
   // RAB: Not needed... Splash screen code, above, does this
//	CCommandLineInfo cmdInfo;
//	ParseCommandLine(cmdInfo);


	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

// NOTE: This is obsolete code... This kind of registration now happens with the installer
// and is self-repairing. It also has the potential for causing problems when users don't
// have sufficient access to the registry.
// RAB 8.24.2007
//
//	// When a server application is launched stand-alone, it is a good idea
//	//  to update the system registry in case it has been damaged.
//	m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
//	COleObjectFactory::UpdateRegistryAll();

   // Don't display a new MDI child window during startup
   // Q141725
   if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
      cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

   // Do some special processing for 12-50 test cases and bail from showing windows.
   if (cmdInfo.m_bDo1250Test)
   {
      Process1250Testing(cmdInfo);
      return FALSE;
   }
   else if (cmdInfo.m_DoTxCadReport)
   {
      ProcessTxDotCad(cmdInfo);
      return FALSE;
   }

   CREATE_LOGFILE("PGSuperApp");

   // Update any files that need to be cached from a web server
   UpdateCache();

   // Change help file name
   CString strHelpFile(m_pszHelpFilePath);
   strHelpFile.MakeLower();
   strHelpFile.Replace(".hlp",".chm");
#if defined _DEBUG
   strHelpFile.Replace("debug\\","");
#else
   strHelpFile.Replace("release\\","");
#endif
   free((void*)m_pszHelpFilePath);
   m_pszHelpFilePath = _tcsdup(_T(strHelpFile));
   

   pMainFrame->AutoCalcEnabled( m_bAutoCalcEnabled );

   // The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
   	return FALSE;

   m_PluginMgr.LoadPlugins();

   // Close splash screen as it causes problems
   CSplashWnd::CloseSplashScreen();

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
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

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP

   DDX_Control(pDX,IDC_WSDOT,m_WSDOT);
   DDX_Control(pDX,IDC_TXDOT,m_TxDOT);
   DDX_Control(pDX,IDC_BRIDGESIGHT,m_BridgeSight);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CPGSuperApp::OnAppAbout()
{
   CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp commands

CDocument* CPGSuperApp::OpenDocumentFile(LPCTSTR lpszFileName) 
{
   CDocument* pdoc = 0;
   // Check to make sure that file is a .pgs or .pgt 
   CString file_ext;
   CString file_name(lpszFileName);
	int pos = file_name.ReverseFind('.');
	if (0 <= pos)
		file_ext = file_name.Right(file_name.GetLength() - pos - 1);

   if ( file_ext.CompareNoCase("pgs") == 0 || 
        file_ext.CompareNoCase("pgt") == 0 ||
        file_ext.CompareNoCase("lbr") == 0 )
   {
      // The current project must be closed before opening the new one
      if (m_pDocManager && m_pDocManager->SaveAllModified())
      {
         m_pDocManager->CloseAllDocuments(FALSE);
         pdoc = CWinApp::OpenDocumentFile(lpszFileName);
      }
   }
   else
   {
      ::AfxMessageBox("Error Invalid File Type - Valid PGSuper files must have either a .PGS, .PGT, or .LBR extension.",MB_ICONEXCLAMATION|MB_OK);
   }

   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );
   pMainFrame->UpdateStatusBar();

   // for template files, we need to rename the file to Untitled
   if (pdoc!=0 && file_ext.CompareNoCase("pgt")==0 )
   {
      pdoc->SetPathName("Untitled",FALSE);
      pdoc->SetModifiedFlag(TRUE);
      pMainFrame->UpdateFrameTitle("Untitled");
   }

   return pdoc;
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

bool CPGSuperApp::ShowProjectPropertiesOnNewProject()
{
   return m_bShowProjectProperties;
}

void CPGSuperApp::ShowProjectPropertiesOnNewProject(bool bShow)
{
   m_bShowProjectProperties = bShow;
}

void CPGSuperApp::SetTipFilePath(const CString& strTipPath)
{
   m_TipFilePath = strTipPath;
}

CString CPGSuperApp::GetTipFilePath() const
{
   return m_TipFilePath;
}

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

bool CPGSuperApp::UpdatingTemplate()
{
   return m_bUpdatingTemplate;
}

void CPGSuperApp::OnUpdateTemplate()
{
   int result = AfxMessageBox("All of the template library entries will be updated to match the Master Library.\n\nDo you want to proceed?",MB_YESNO);
   if ( result == IDYES )
   {
      m_bUpdatingTemplate = true;
      OnFileNew();
      m_bUpdatingTemplate = false;

      AfxMessageBox("Update complete",MB_OK);
   }
}

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

void CPGSuperApp::OnFileNew()
{
   // Before creating a new document, save the open document
   // if it is modifed and then close it.
   if ( SaveAllModified() )
   {
      CloseAllDocuments( FALSE );
      CWinApp::OnFileNew();
   }
}

void CPGSuperApp::OnFileOpen()
{
   // Before creating a new document, save the open document
   // if it is modifed and then close it.
   if ( SaveAllModified() )
   {
      CloseAllDocuments( FALSE );
      CWinApp::OnFileOpen();
   }
}

void CPGSuperApp::FinalDocumentInit(CPGSuperDoc* pDoc)
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   pgsTypes::UnitMode unitMode = pDisplayUnits->GetUnitDisplayMode();


   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );

   pMainFrame->UpdateStatusBar();
}

int CPGSuperApp::ExitInstance() 
{
   // This call performs cleanup etc for the grid classes
	GXTerminate( );
	
   // release the shared menu
   ::DestroyMenu( m_hMenuShared );

   // Write settings to the registry
   RegistryExit();

   m_AppUnitSystem.Release();
   m_PluginMgr.UnloadPlugins();

   CLOSE_LOGFILE;

   return CWinApp::ExitInstance();
}

CCountedMultiDocTemplate* CPGSuperApp::GetBridgeModelEditorTemplate()
{
   return m_pBridgeModelEditorTemplate;
}

CCountedMultiDocTemplate* CPGSuperApp::GetGirderModelEditorTemplate()
{
   return m_pGirderModelEditorTemplate.get();
}

CCountedMultiDocTemplate* CPGSuperApp::GetLibraryEditorTemplate()
{
   return m_pLibraryEditorTemplate.get();
}

CCountedMultiDocTemplate* CPGSuperApp::GetAnalysisResultsViewTemplate()
{
   return m_pAnalysisResultsViewTemplate.get();
}

CReportViewDocTemplate* CPGSuperApp::GetReportViewTemplate(CollectionIndexType rptIdx,bool bPromptForSpec)
{
   m_pReportViewTemplate->SetReportIndex(rptIdx);
   m_pReportViewTemplate->PromptForReportSpecification(bPromptForSpec);
   return m_pReportViewTemplate.get();
}

CReportViewDocTemplate* CPGSuperApp::GetReportViewTemplate()
{
   return m_pReportViewTemplate.get();
}

CCountedMultiDocTemplate* CPGSuperApp::GetFactorOfSafetyViewTemplate()
{
   return m_pFactorOfSafetyViewTemplate.get();
}

CCountedMultiDocTemplate* CPGSuperApp::GetEditLoadsViewTemplate()
{
   return m_pEditLoadsViewTemplate.get();
}


void CPGSuperApp::RegDocTemplates()
{
   if ( m_pDocManager )
      delete m_pDocManager;

   m_pDocManager = new CPGSuperDocManager; // use a custom doc manager (provides "Import" functionality)

   // Using a shared menu
   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   m_hMenuShared = ::LoadMenu( m_hInstance, MAKEINTRESOURCE(IDR_PGSUPERTYPE) );

   // MFC doc template for new documents based on PGSuper template files
   m_pBridgeModelEditorTemplate = new CPGSuperCountedMultiDocTemplate(
		IDR_BRIDGEMODELEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CBridgePlanView),1);
   m_pBridgeModelEditorTemplate->m_hMenuShared = m_hMenuShared;
	AddDocTemplate(m_pBridgeModelEditorTemplate);

   // MFC doc template for new documents based on import plugin
   CPGSuperImportPluginDocTemplate* pImportTemplate = new CPGSuperImportPluginDocTemplate(
		IDR_PGSUPERTYPE,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CBridgePlanView),1);
   pImportTemplate->m_hMenuShared = m_hMenuShared;
	AddDocTemplate(pImportTemplate);

   m_pGirderModelEditorTemplate = DocTemplatePtr(new CPGSuperCountedMultiDocTemplate(
      IDR_GIRDERMODELEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CGirderModelChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CGirderModelElevationView),1));
   m_pGirderModelEditorTemplate->m_hMenuShared = m_hMenuShared;

   m_pLibraryEditorTemplate = DocTemplatePtr(new CPGSuperCountedMultiDocTemplate(
      IDR_LIBRARYEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CLibChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CLibraryEditorView),1));
   m_pLibraryEditorTemplate->m_hMenuShared = m_hMenuShared;

   m_pAnalysisResultsViewTemplate = DocTemplatePtr(new CPGSuperCountedMultiDocTemplate(
      IDR_ANALYSISRESULTS,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CAnalysisResultsChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CAnalysisResultsView)));
   m_pAnalysisResultsViewTemplate->m_hMenuShared = m_hMenuShared;

   m_pReportViewTemplate = std::auto_ptr<CReportViewDocTemplate>(new CReportViewDocTemplate(
      IDR_REPORT,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(COutputChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CReportView)));
   m_pReportViewTemplate->m_hMenuShared = m_hMenuShared;

   m_pFactorOfSafetyViewTemplate = DocTemplatePtr(new CPGSuperCountedMultiDocTemplate(
      IDR_FACTOROFSAFETY,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CFactorOfSafetyChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CFactorOfSafetyView)));
   m_pFactorOfSafetyViewTemplate->m_hMenuShared = m_hMenuShared;

   m_pEditLoadsViewTemplate = DocTemplatePtr(new CPGSuperCountedMultiDocTemplate(
      IDR_EDITLOADS,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CEditLoadsChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CEditLoadsView),1));
   m_pEditLoadsViewTemplate->m_hMenuShared = m_hMenuShared;


   // for when we want to include library documents
	//CCountedMultiDocTemplate* pLibMgrDocTemplate;
	//pLibMgrDocTemplate = new CCountedMultiDocTemplate(
	//	IDR_LIBRARTYPE,
	//	RUNTIME_CLASS(CLibraryEditorDoc),
	//	RUNTIME_CLASS(CMyChildFrame), // custom MDI child frame
	//	RUNTIME_CLASS(CLibraryEditorView),1);
	//AddDocTemplate(pLibMgrDocTemplate);
} 

// returns key for HKEY_LOCAL_MACHINE\Software\Washington State Department of Transportation\PGSuper"
// responsibility of the caller to call RegCloseKey() on the returned HKEY
// key is not created if missing (
HKEY CPGSuperApp::GetAppLocalMachineRegistryKey()
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


HKEY CPGSuperApp::GetUninstallRegistryKey()
{
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;
   HKEY hWinKey = NULL;
   HKEY hCVKey = NULL;
   HKEY hUninstallKey = NULL;


   // open the "software" key
   LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("software"), 0, KEY_READ, &hSoftKey);
	if ( result == ERROR_SUCCESS)
	{
      // open the "Microsoft" key
      result = RegOpenKeyEx(hSoftKey, _T("Microsoft"), 0,  KEY_READ, &hCompanyKey);
		if (result == ERROR_SUCCESS)
		{
         // Open the "Windows" key
			result = RegOpenKeyEx(hCompanyKey, _T("Windows"), 0, KEY_READ, &hWinKey);

         if (result == ERROR_SUCCESS)
		   {
            // Open the "CurrentVersion" key
			   result = RegOpenKeyEx(hWinKey, _T("CurrentVersion"), 0, KEY_READ, &hCVKey);
            if (result == ERROR_SUCCESS)
		      {
               // Open the "Uninstall" key
			      result = RegOpenKeyEx(hCVKey, _T("Uninstall"), 0, KEY_READ, &hUninstallKey);
		      }
		   }
		}
	}

	if (hSoftKey != NULL)
		RegCloseKey(hSoftKey);
	
   if (hCompanyKey != NULL)
		RegCloseKey(hCompanyKey);
	
   if (hWinKey != NULL)
		RegCloseKey(hWinKey);
	
   if (hCVKey != NULL)
		RegCloseKey(hCVKey);

	return hUninstallKey;
}

// returns key for:
//      HKEY_LOCAL_MACHINE\"Software"\Washington State Deparment of Transportation\PGSuper\lpszSection
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CPGSuperApp::GetLocalMachineSectionKey(LPCTSTR lpszSection)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return NULL;

   return GetLocalMachineSectionKey(hAppKey,lpszSection);
}

HKEY CPGSuperApp::GetLocalMachineSectionKey(HKEY hAppKey,LPCTSTR lpszSection)
{
	ASSERT(lpszSection != NULL);

	HKEY hSectionKey = NULL;

	LONG result = RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_WRITE|KEY_READ, &hSectionKey);
	RegCloseKey(hAppKey);
	return hSectionKey;
}

UINT CPGSuperApp::GetLocalMachineInt(LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return nDefault;

   return GetLocalMachineInt(hAppKey,lpszSection,lpszEntry,nDefault);
}

UINT CPGSuperApp::GetLocalMachineInt(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault)
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

CString CPGSuperApp::GetLocalMachineString(LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault)
{
	HKEY hAppKey = GetAppLocalMachineRegistryKey();
	if (hAppKey == NULL)
		return lpszDefault;

   return GetLocalMachineString(hAppKey,lpszSection,lpszEntry,lpszDefault);
}

CString CPGSuperApp::GetLocalMachineString(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault)
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

//"WorkGroupTemplateLocation"
void CPGSuperApp::RegistryInit()
{
   // Sets the registry to 
   // HKEY_CURRENT_USER\Software\Washington State Department of Transportation\PGSuper
   // 
   // After this method is called, you can make calls to GetProfileInt, GetProfileString,
   // WriteProfileInt, and WriteProfileString.  The data will read/write from the registry
   // instead of INI files.  This makes life a whole bunch easier.
	SetRegistryKey(IDS_COMPANY);

   
   // The default values are read from HKEY_LOCAL_MACHINE\Software\Washington State Deparment of Transportation\PGSuper
   // If the default values are missing, the hard coded defaults found herein are used.
   // Install writers can create MSI transforms to alter the "defaults" by changing the registry values

   CString strAutoCalcDefault          = GetLocalMachineString(_T("Settings"),_T("AutoCalc"),              _T("On"));
   CString strShowProjectProperties    = GetLocalMachineString(_T("Settings"),_T("ShowProjectProperties"), _T("On"));
   CString strDefaultDesignFlexure     = GetLocalMachineString(_T("Settings"),_T("DesignFlexure"),         _T("On"));
   CString strDefaultDesignShear       = GetLocalMachineString(_T("Settings"),_T("DesignShear"),           _T("Off"));
   CString strDefaultLegalNotice       = GetLocalMachineString(_T("Settings"),_T("LegalNotice"),           _T("On"));
   CString strDefaultReportCoverImage  = GetLocalMachineString(_T("Settings"),_T("ReportCoverImage"),      _T(""));
   CString strDefaultGirderLabelFormat = GetLocalMachineString(_T("Settings"),_T("GirderLabelFormat"),     _T("Alpha"));

   int iDefaultSharedResourceType     = GetLocalMachineInt(   _T("Settings"),_T("SharedResourceType"),    1);
   int iDefaultCacheUpdateFrequency   = GetLocalMachineInt(   _T("Settings"),_T("CacheUpdateFrequency"),  2);

   CString strDefaultUserTemplateFolder     = GetLocalMachineString(_T("Options"),_T("UserTemplateLocation"), _T("C:\\"));
   CString strDefaultCompany                = GetLocalMachineString(_T("Options"),_T("CompanyName"), _T("Your Company"));
   CString strDefaultEngineer               = GetLocalMachineString(_T("Options"),_T("EngineerName"),_T("Your Name"));
   CString strDefaultCatalogServer          = GetLocalMachineString(_T("Options"),_T("CatalogServer"),_T("WSDOT"));
   CString strDefaultPublisher              = GetLocalMachineString(_T("Options"),_T("Publisher"),_T("WSDOT"));
   CString strDefaultLocalMasterLibraryFile = GetLocalMachineString(_T("Options"),_T("MasterLibraryLocal"),     GetDefaultMasterLibraryFile());
   CString strLocalWorkgroupTemplateFolder  = GetLocalMachineString(_T("Options"),_T("WorkgroupTemplatesLocal"),GetDefaultWorkgroupTemplateFolder());

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

   // NOTE: Settings is an MFC created Registry section

   // Do any necessary conversions from previous versions of PGSuper
   RegistryConvert();

   // Get AutoCalc setting.
   // Default is On.
   // If the string is not Off, then assume autocalc is on.
   CString strAutoCalc = GetProfileString(_T("Settings"),_T("AutoCalc"),strAutoCalcDefault);
   if ( strAutoCalc.CompareNoCase(_T("Off")) == 0 )
      m_bAutoCalcEnabled = false;
   else
      m_bAutoCalcEnabled = true;

   CString strProjectProperties = GetProfileString(_T("Settings"),_T("ShowProjectProperties"),strShowProjectProperties);
   if ( strProjectProperties.CompareNoCase(_T("Off")) == 0 )
      m_bShowProjectProperties = false;
   else
      m_bShowProjectProperties = true;

   CString strGirderLabelFormat = GetProfileString(_T("Settings"),_T("GirderLabelFormat"),strDefaultGirderLabelFormat);
   if ( strGirderLabelFormat.CompareNoCase(_T("Alpha")) == 0 )
      pgsGirderLabel::UseAlphaLabel(true);
   else
      pgsGirderLabel::UseAlphaLabel(false);

   // bridge model editor settings
   // turn on all settings for default
   UINT def_bm = IDB_PV_LABEL_PIERS     |
                 IDB_PV_LABEL_ALIGNMENT |
                 IDB_PV_LABEL_GIRDERS   |
                 IDB_PV_LABEL_BEARINGS  |
                 IDB_PV_LABEL_TICKMARKS |
                 IDB_PV_SHOW_TICKMARKS  |
                 IDB_PV_DRAW_ISOTROPIC  |
                 IDB_CS_LABEL_GIRDERS   |
                 IDB_CS_SHOW_DIMENSIONS |
                 IDB_CS_DRAW_ISOTROPIC;

   m_BridgeModelEditorSettings = GetProfileInt(_T("Settings"),_T("BridgeEditor"),def_bm);

   m_GirderModelEditorSettings = GetProfileInt(_T("Settings"),_T("GirderEditor"),def_bm);

   m_UIHintSettings = GetProfileInt(_T("Settings"),_T("UIHints"),0); // default, all hints enabled

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

   // Tip of the Day file location
   m_TipFilePath = GetProfileString(_T("Tip"),_T("TipFile"), GetAppLocation() + CString("PGSuper.tip"));

   // Document Template New Dialog view mode
   m_DocTemplateViewMode = GetProfileInt(_T("Settings"),_T("TemplateDlg"),0);

   // Flexure and stirrup design defaults for design dialog.
   // Default is to design flexure and not shear.
   // If the string is not Off, then assume it is on.
   CString strDesignFlexure = GetProfileString(_T("Settings"),_T("DesignFlexure"),strDefaultDesignFlexure);
   if ( strDesignFlexure.CompareNoCase(_T("Off")) == 0 )
      m_bDesignFlexureEnabled = false;
   else
      m_bDesignFlexureEnabled = true;

   CString strDesignShear = GetProfileString(_T("Settings"),_T("DesignShear"),strDefaultDesignShear);
   if ( strDesignShear.CompareNoCase(_T("Off")) == 0 )
      m_bDesignShearEnabled = false;
   else
      m_bDesignShearEnabled = true;

   CString strLegalNotice = GetProfileString(_T("Settings"),_T("LegalNotice"),strDefaultLegalNotice);
   if ( strLegalNotice.CompareNoCase(_T("On")) == 0 )
      m_bShowLegalNotice = VARIANT_TRUE;
   else
      m_bShowLegalNotice = VARIANT_FALSE;

   pgsReportStyleHolder::SetReportCoverImage(GetProfileString(_T("Settings"),_T("ReportCoverImage"),strDefaultReportCoverImage));

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

   // If we were set to a local library, create a new catalog server 
   // and add its settings to the registry
   if (type==srtLocal)
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
   // Save the AutoCalc mode setting
   VERIFY(WriteProfileString( _T("Settings"),_T("AutoCalc"),m_bAutoCalcEnabled ? _T("On") : _T("Off") ));
   VERIFY(WriteProfileString( _T("Settings"),_T("ShowProjectProperties"),m_bShowProjectProperties ? _T("On") : _T("Off") ));
   VERIFY(WriteProfileString( _T("Settings"),_T("LegalNotice"),m_bShowLegalNotice == VARIANT_TRUE ? _T("On") : _T("Off") ));
   VERIFY(WriteProfileString( _T("Settings"),_T("GirderLabelFormat"),pgsGirderLabel::UseAlphaLabel() ? _T("Alpha") : _T("Numeric") ));

   // bridge editor view
   VERIFY(WriteProfileInt(_T("Settings"),_T("BridgeEditor"),m_BridgeModelEditorSettings));

   // girder editor view
   VERIFY(WriteProfileInt(_T("Settings"),_T("GirderEditor"),m_GirderModelEditorSettings));

   VERIFY(WriteProfileInt(_T("Settings"),_T("UIHints"),m_UIHintSettings));

   // Options settings
   VERIFY(WriteProfileString(_T("Options"), _T("CompanyName"), m_CompanyName));

   // engineer name
   VERIFY(WriteProfileString(_T("Options"), _T("EngineerName"), m_EngineerName));

   WriteProfileInt(_T("Settings"),_T("SharedResourceType"),m_SharedResourceType);
   WriteProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),m_CacheUpdateFrequency);

   // Internet resources
   WriteProfileString(_T("Options"),_T("CatalogServer"),m_CurrentCatalogServer);
   WriteProfileString(_T("Options"),_T("Publisher"),m_Publisher);

   // Cache file/folder for Internet or Local Network resources
   WriteProfileString(_T("Options"),_T("MasterLibraryCache"),     m_MasterLibraryFileCache);
   WriteProfileString(_T("Options"),_T("WorkgroupTemplatesCache"),m_WorkgroupTemplateFolderCache);

   // Tip of the Day file location
   VERIFY(WriteProfileString(_T("Tip"),_T("TipFile"), m_TipFilePath));

   // Document Template New Dialog view mode
   WriteProfileInt(_T("Settings"),_T("TemplateDlg"),m_DocTemplateViewMode);

   // Save the design mode settings
   VERIFY(WriteProfileString( _T("Settings"),_T("DesignFlexure"),m_bDesignFlexureEnabled ? _T("On") : _T("Off") ));
   VERIFY(WriteProfileString( _T("Settings"),_T("DesignShear"),  m_bDesignShearEnabled   ? _T("On") : _T("Off") ));

   
   sysTime time;
   WriteProfileInt(_T("Settings"),_T("LastRun"),time.Seconds());

   m_CatalogServers.SaveToRegistry(this);
}

BOOL CPGSuperApp::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}

void CPGSuperApp::ShowTipAtStartup(void)
{
	// CG: This function added by 'Tip of the Day' component.

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_bShowSplash)
	{
		CTipDlg dlg;
		if (dlg.m_bStartup)
			dlg.DoModal();
	}
}

void CPGSuperApp::ShowTipOfTheDay(void)
{
	// CG: This function added by 'Tip of the Day' component.

	CTipDlg dlg;
	dlg.DoModal();

}

AcceptanceType CPGSuperApp::ShowLegalNoticeAtStartup(void)
{
   if ( m_bShowLegalNotice == VARIANT_TRUE )
   {
      return ShowLegalNotice(VARIANT_TRUE);
   }

   return atAccept;
}

AcceptanceType CPGSuperApp::ShowLegalNotice(VARIANT_BOOL bGiveChoice)
{
   CComPtr<IARPNotice> pNotice;
   if ( FAILED(pNotice.CoCreateInstance(CLSID_ARPNotice) ) )
   {
      // There was an error creating the legal notice
      m_bShowLegalNotice = VARIANT_TRUE;
      return atAccept;
   }

   pNotice->put_ShowAgain( m_bShowLegalNotice );
   AcceptanceType accept;
   pNotice->Show(bGiveChoice,ltAROSL,&accept);
   pNotice->get_ShowAgain(&m_bShowLegalNotice);

   return accept;
}

void CPGSuperApp::OnProgramSettings() 
{
   OnProgramSettings(FALSE);
}

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

      dlg.m_Method               = (int)m_SharedResourceType;
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

         m_SharedResourceType   = (SharedResourceType)(dlg.m_Method);
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

bool CPGSuperApp::IsAutoCalcEnabled() const
{
   return m_bAutoCalcEnabled;
}

void CPGSuperApp::EnableAutoCalc( bool bEnable )
{
   if ( m_bAutoCalcEnabled != bEnable )
   {
      m_bAutoCalcEnabled = bEnable;

      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      pMainFrame->AutoCalcEnabled( m_bAutoCalcEnabled );
   }
}

UINT CPGSuperApp::GetBridgeEditorSettings() const
{
   return m_BridgeModelEditorSettings;
}

void CPGSuperApp::SetBridgeEditorSettings(UINT settings)
{
   m_BridgeModelEditorSettings = settings;
}

UINT CPGSuperApp::GetGirderEditorSettings() const
{
   return m_GirderModelEditorSettings;
}

void CPGSuperApp::SetGirderEditorSettings(UINT settings)
{
   m_GirderModelEditorSettings = settings;
}

UINT CPGSuperApp::GetUIHintSettings() const
{
   return m_UIHintSettings;
}

void CPGSuperApp::SetUIHintSettings(UINT settings)
{
   m_UIHintSettings = settings;
}

UINT CPGSuperApp::GetDocTemplateViewMode() const
{
   return m_DocTemplateViewMode;
}

void CPGSuperApp::SetDocTemplateViewMode(UINT mode)
{
   m_DocTemplateViewMode = mode;
}

CString CPGSuperApp::GetEngineerName()
{
   return m_EngineerName;
}

CString CPGSuperApp::GetEngineerCompany()
{
   return m_CompanyName;
}

bool CPGSuperApp::IsDesignFlexureEnabled() const
{
   return m_bDesignFlexureEnabled;
}

void CPGSuperApp::EnableDesignFlexure( bool bEnable )
{
   m_bDesignFlexureEnabled = bEnable;
}

bool CPGSuperApp::IsDesignShearEnabled() const
{
   return m_bDesignShearEnabled;
}

void CPGSuperApp::EnableDesignShear( bool bEnable )
{
   m_bDesignShearEnabled = bEnable;
}

void notify_error(CDocument* pDoc,void* pStuff)
{
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pDoc;
      pPgsDoc->OnUpdateError(*((CString*)pStuff));
   }
}

void log_error(CDocument* pDoc,void* pStuff)
{
   CXShutDown* pXShutDown = (CXShutDown*)pStuff;
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pDoc;

      CComPtr<IBroker> pBroker;
      pPgsDoc->GetBroker(&pBroker);

      GET_IFACE2( pBroker, IProjectLog, pLog );

      std::string error_message;
      pXShutDown->GetErrorMessage( &error_message );

      CString msg;
      msg.Format("%s\nFile : %s\nLine : %d\n", error_message.c_str(), pXShutDown->GetFile().c_str(), pXShutDown->GetLine() );

      pLog->LogMessage( msg );
   }
}

LRESULT CPGSuperApp::ProcessWndProcException(CException* e, const MSG* pMsg) 
{
   LRESULT lResult = 0L;

	if ( e->IsKindOf(RUNTIME_CLASS(CXShutDown) ) )
   {
      CXShutDown* pXShutDown = (CXShutDown*)e;
      std::string error_msg;
      pXShutDown->GetErrorMessage( &error_msg );

      CString msg1;
      AfxFormatString1( msg1, IDS_E_PROBPERSISTS, "log" ); 
      CString msg2;
      AfxFormatString2( msg2, pXShutDown->AttemptSave() ? IDS_FATAL_MSG_SAVE : IDS_FATAL_MSG_NOSAVE, error_msg.c_str(), msg1 );
      int retval = AfxMessageBox( msg2, (pXShutDown->AttemptSave() ? MB_YESNO : MB_OK) | MB_ICONEXCLAMATION );
      ForEachDoc(log_error,(void*)pXShutDown);
      if ( retval == IDYES )
      {
         ForEachDoc( save_doc, NULL );
      }

      lResult = 1L;
      AfxPostQuitMessage( 0 );
   }
   else if ( e->IsKindOf(RUNTIME_CLASS(CXUnwind) ) )
   {
      CXUnwind* pXUnwind = (CXUnwind*)e;
      std::string error_msg;
      pXUnwind->GetErrorMessage( &error_msg );
      m_LastError = error_msg.c_str();
      AfxMessageBox( error_msg.c_str(),  MB_OK | MB_ICONWARNING );


      ForEachDoc( notify_error, (void*)&m_LastError );
      lResult = 1L;
   }
   else if ( e->IsKindOf(RUNTIME_CLASS(CInternetException) ) )
   {
      lResult = 1L;
      AfxMessageBox("There was a critical error accessing the Master Library and Workgroup Templates on the Internet.\n\nPGSuper will use the generic libraries and templates that were installed with the application.\n\nUse File | Program Settings to reset the Master Library and Workgroup Templates to shared Internet resources at a later date.",MB_OK | MB_ICONEXCLAMATION);
      m_SharedResourceType = srtDefault;
      m_MasterLibraryFileCache       = GetDefaultMasterLibraryFile();
      m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();
   }
   else
   {
      lResult = CWinApp::ProcessWndProcException(e, pMsg);
   }

	return lResult;
}

void CPGSuperApp::ForEachDoc(DocCallback pfn,void* pStuff)
{
   POSITION tplpos = theApp.GetFirstDocTemplatePosition();
   while ( tplpos != NULL )
   {
      CDocTemplate* pTpl = theApp.GetNextDocTemplate( tplpos );

      POSITION docpos = pTpl->GetFirstDocPosition();
      while ( docpos != NULL )
      {
         CDocument* pDoc = pTpl->GetNextDoc( docpos );
         (*pfn)(pDoc,pStuff);
      }
   }
}

bool CPGSuperApp::IsDocLoaded()
{
   POSITION tplpos = theApp.GetFirstDocTemplatePosition();
   if ( tplpos != NULL )
   {
      CDocTemplate* pTpl = theApp.GetNextDocTemplate( tplpos );
      POSITION docpos = pTpl->GetFirstDocPosition();
      return ( docpos != NULL );
   }
   return false;
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

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   CWnd* pWnd = GetDlgItem(IDC_VERSION);
   pWnd->SetWindowText( theApp.GetVersionString(true) );


   pWnd = GetDlgItem(IDC_COPYRIGHT);
   CString strText;
   CTime now = CTime::GetCurrentTime();
   strText.Format("Copyright © 1999-%d, WSDOT, All Rights Reserved",now.GetYear());
   pWnd->SetWindowText(strText);


   m_WSDOT.SetURL("http://www.wsdot.wa.gov/");
   m_TxDOT.SetURL("http://www.dot.state.tx.us/");
   m_BridgeSight.SetURL("http://www.bridgesight.com/");

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CPGSuperApp::Run() 
{
	int retval = -2;

   try
   {
      retval = CWinApp::Run();
   }
   catch(sysXBase& e)
   {
      std::string msg;
      e.GetErrorMessage(&msg);
      AfxMessageBox(msg.c_str(),MB_OK);
      ExitInstance();
   }

   return retval;
}

void CPGSuperApp::OnAppLegal() 
{
   ShowLegalNotice();	
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
   CString inputFile(rCmdInfo.m_strFileName);
   if (!inputFile.IsEmpty())
   {
      CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)OpenDocumentFile(rCmdInfo.m_strFileName);
		if (pPgsDoc!=0)
      {
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
      else
      {
         CString msg = CString("Error - Opening test input file")+rCmdInfo.m_strFileName;
         ::AfxMessageBox(msg);
      }
   }
   else
   {
      ::AfxMessageBox("Error - Input file name must be specified on command line when Test option is used");
   }

}

void CPGSuperApp::ProcessTxDotCad(const CPGSuperCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_DoTxCadReport);


   if (rCmdInfo.m_TxGirder!=TXALLGIRDERS && rCmdInfo.m_TxGirder!=TXEIGIRDERS && (rCmdInfo.m_TxGirder<0 || rCmdInfo.m_TxGirder>27))
   {
      ::AfxMessageBox("Invalid girder specified on command line for TxDOT CAD report");
      return;
   }

   if (rCmdInfo.m_TxSpan!=ALL_SPANS && rCmdInfo.m_TxSpan<0)
   {
      ::AfxMessageBox("Invalid span specified on command line for TxDOT CAD report");
      return;
   }

   CString inputFile(rCmdInfo.m_strFileName);
   if (!inputFile.IsEmpty())
   {
      CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)OpenDocumentFile(rCmdInfo.m_strFileName);
		if (pPgsDoc!=0)
      {
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
      else
      {
         CString msg = CString("Error - Opening test input file")+rCmdInfo.m_strFileName;
         ::AfxMessageBox(msg);
      }
   }
   else
   {
      ::AfxMessageBox("Error - Input file name must be specified on command line when TxDOT CAD report option is used");
   }
}

void CPGSuperApp::UpdateCache()
{
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
            DoCacheUpdate();
      }
   }
   catch(...)
   {
      // Things are totally screwed if we end up here. Reset to default library and templates from install
      RestoreLibraryAndTemplatesToDefault();
      ::AfxMessageBox("An error occurred while loading Library and Template server information. These settings have been restored to factory defaults.",MB_ICONINFORMATION);
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
      wndProgress->Show(CComBSTR("Checking the Internet for Library updates"));

      try
      {
         // Catalog server does the work here
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         bUpdatesPending = pserver->CheckForUpdates(m_Publisher, NULL, GetCacheFolder());

         wndProgress->Hide();
         wndProgress.Release();
      }
      catch(...)
      {
         wndProgress->Hide();
         wndProgress.Release();
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
   wndProgress->Show(CComBSTR("Update Libraries and Templates"));

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
      const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
      bSuccessful = pserver->PopulateCatalog(m_Publisher,progress,GetCacheFolder());

      m_MasterLibraryFileURL = pserver->GetMasterLibraryURL(m_Publisher);

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

   wndProgress->Close();

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

CString CPGSuperApp::GetAppLocation()
{
   TCHAR szBuff[_MAX_PATH];
   ::GetModuleFileName(AfxGetInstanceHandle(), szBuff, _MAX_PATH);
   CString filename(szBuff);
   filename.MakeUpper();

//#if defined _DEBUG
//   CString exe_file = "DEBUG\\PGSUPER.EXE";
//   int loc;
//#else
   //// if "Release" is in the path, remove it (this happens when testing release builds)
   //int loc = filename.Find("RELEASE\\");
   //if ( loc != -1 )
   //   filename.Replace("RELEASE\\","");

   CString exe_file = "PGSUPER.EXE";
//#endif
   int loc = filename.Find(exe_file);
   if ( loc == -1 )
   {
       // something is wrong... that find should have succeeded
      // hard code the default install location so that there is a remote chance of success
      filename = "C:\\PROGRAM FILES\\WSDOT\\PGSUPER\\";
   }
   else
   {
      filename.Replace(exe_file,"");
   }

   return filename;
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


void CPGSuperApp::GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem)
{
   (*ppAppUnitSystem) = m_AppUnitSystem;
   (*ppAppUnitSystem)->AddRef();
}

void CPGSuperApp::OnImportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
      return;

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);

   Uint32 nImporters = m_PluginMgr.GetImporterCount();
   if ( nImporters == 0 )
   {
      pCmdUI->SetText("Custom importers not installed");
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      Uint32 idx;
      // clean up the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+idx,MF_BYCOMMAND);
      }

      // figure out if there is a document or not
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );
      CMDIChildWnd* pChild = pMainFrame->MDIGetActive();
      CPGSuperDoc* pDoc = NULL;

      if ( pChild )
      {
         CView* pView = pChild->GetActiveView();
         pDoc = (CPGSuperDoc*)pView->GetDocument();
         ASSERT( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) );
      }

      // populate the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         CComPtr<IPGSuperImporter> importer;
         m_PluginMgr.GetPGSuperImporter(idx,true,&importer);

         UINT cmdID = m_PluginMgr.GetPGSuperImporterCommand(idx);

         CComBSTR bstrMenuText;
         importer->GetMenuText(&bstrMenuText);
         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2A(bstrMenuText));

      	pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();

         // disable command if it is a data importer and there isn't an open document
         ImporterType type;
         importer->GetType(&type);
         if ( type == impData )
            pCmdUI->Enable(pDoc ? TRUE : FALSE);
         else
            pCmdUI->Enable(TRUE);

         pCmdUI->m_nIndex++;
      }
   }

   pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperApp::OnExportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
      return;

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);

   CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
   CMDIChildWnd* pChild = pMainFrm->MDIGetActive();

   Uint32 nExporters = m_PluginMgr.GetExporterCount();
   if ( nExporters == 0 )
   {
      pCmdUI->SetText("Custom exporters not installed");
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      Uint32 idx;
      for ( idx = 0; idx < nExporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+idx,MF_BYCOMMAND);
      }

      for ( idx = 0; idx < nExporters; idx++ )
      {
         CComPtr<IPGSuperExporter> exporter;
         m_PluginMgr.GetPGSuperExporter(idx,true,&exporter);

         UINT cmdID = m_PluginMgr.GetPGSuperExporterCommand(idx);

         CComBSTR bstrMenuText;
         exporter->GetMenuText(&bstrMenuText);

         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2A(bstrMenuText));
      	pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();
         pCmdUI->Enable(pChild ? TRUE : FALSE);
         pCmdUI->m_nIndex++;
      }
   }

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperApp::OnImport(UINT nID)
{
   CComPtr<IPGSuperImporter> importer;
   m_PluginMgr.GetPGSuperImporter(nID,false,&importer);
   ImporterType type;
   importer->GetType(&type);

   if ( type == impProject )
   {
      if ( SaveAllModified() )
      {
         CloseAllDocuments( FALSE );
         ((CPGSuperDocManager*)m_pDocManager)->OnImport(nID);

         CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
         ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );
         pMainFrame->UpdateStatusBar();
      }
   }
   else
   {
      CComPtr<IBroker> broker;
      AfxGetBroker(&broker);
      importer->Import(broker);
   }
}

void CPGSuperApp::OnExport(UINT nID)
{
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );
   CMDIChildWnd* pChild = pMainFrame->MDIGetActive();
   CView* pView = pChild->GetActiveView();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();
   ASSERT( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) );

   pDoc->OnExportDocument(nID);
}

void CPGSuperApp::GetImporter(UINT nID,IPGSuperImporter** importer)
{
   m_PluginMgr.GetPGSuperImporter(nID,false,importer);
}

void CPGSuperApp::GetExporter(UINT nID,IPGSuperExporter** exporter)
{
   m_PluginMgr.GetPGSuperExporter(nID,false,exporter);
}


/////////////////////////////////////////////////////////////////////////////
// Helpers for saving/restoring window state
BOOL CPGSuperApp::ReadWindowPlacement(const CString& strKey,LPWINDOWPLACEMENT pwp)
{
   CString strBuffer = AfxGetApp()->GetProfileString(CString((LPCSTR)IDS_REG_SETTINGS), strKey);

   if (strBuffer.IsEmpty())
      return FALSE;

   WINDOWPLACEMENT wp;
   int nRead = sscanf_s(strBuffer, m_strWindowPlacementFormat,
      &wp.flags, &wp.showCmd,
      &wp.ptMinPosition.x, &wp.ptMinPosition.y,
      &wp.ptMaxPosition.x, &wp.ptMaxPosition.y,
      &wp.rcNormalPosition.left, &wp.rcNormalPosition.top,
      &wp.rcNormalPosition.right, &wp.rcNormalPosition.bottom);

   if (nRead != 10)
      return FALSE;

   wp.length = sizeof wp;
   *pwp = wp;

   return TRUE;
}

void CPGSuperApp::WriteWindowPlacement(const CString& strKey,LPWINDOWPLACEMENT pwp)
{
   TCHAR szBuffer[sizeof("-32767")*8 + sizeof("65535")*2];

   wsprintf(szBuffer, m_strWindowPlacementFormat,
      pwp->flags, pwp->showCmd,
      pwp->ptMinPosition.x, pwp->ptMinPosition.y,
      pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
      pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
      pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
   AfxGetApp()->WriteProfileString(CString((LPCSTR)IDS_REG_SETTINGS), strKey, szBuffer);
}
