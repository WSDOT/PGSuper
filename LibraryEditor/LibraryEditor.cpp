///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// LibraryEditor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LibraryEditor.h"

#include "MainFrm.h"
#include "MyChildFrame.h"
#include "LibraryEditorDoc.h"
#include "LibraryEditorView.h"

#include <Units\SysUnitsMgr.h>
#include <psgLib\psgLib.h>
#include <psgLib\BeamFamilyManager.h>
#include <PGSuperUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorApp

BEGIN_MESSAGE_MAP(CLibraryEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CLibraryEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_APP_LEGAL, OnAppLegal)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorApp construction

CLibraryEditorApp::CLibraryEditorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
   m_bShowLegalNotice = VARIANT_TRUE;

   // Setup system units, Must be same as the PGSuper Library Editor
   InitUnitSystem();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLibraryEditorApp object

CLibraryEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorApp initialization

BOOL CLibraryEditorApp::InitInstance()
{
   // This call will initialize the grid library
	GXInit( );

//	GXSetLocaleAnd3dControl(); // see gxver.h; calls _tsetlocale and Enable3dControls
	_tsetlocale(LC_ALL, NULL);

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_AFXOLEINIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

   CBeamFamilyManager::Init();

   // Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.


	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(IDS_COMPANY);

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

   CString strLegalNotice = GetProfileString(_T("Settings"),_T("LegalNotice"),_T("On"));
   if ( strLegalNotice.CompareNoCase(_T("On")) == 0 )
      m_bShowLegalNotice = VARIANT_TRUE;
   else
      m_bShowLegalNotice = VARIANT_FALSE;

   if ( ShowLegalNoticeAtStartup() == atReject )
      return FALSE; // License was not accepted

   // Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_LIBRARTYPE,
		RUNTIME_CLASS(CLibraryEditorDoc),
		RUNTIME_CLASS(CMyChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CLibraryEditorView));
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(FALSE); 

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

   // Don't display a new MDI child window during startup
   // Q141725
   if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
      cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

   // Change help file name
   CString strHelpFile(m_pszHelpFilePath);
   strHelpFile.MakeLower();
   strHelpFile.Replace("libraryeditor.hlp","pgsuper.chm");
#if defined _DEBUG
   strHelpFile.Replace("libraryeditor\\debug\\","");
#else
   strHelpFile.Replace("libraryeditor\\release\\","");
#endif
   free((void*)m_pszHelpFilePath);
   m_pszHelpFilePath = _tcsdup(_T(strHelpFile));

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
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
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
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CLibraryEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorApp commands

int CLibraryEditorApp::ExitInstance() 
{
// This call performs cleanup etc
	GXTerminate( );

   VERIFY(WriteProfileString( _T("Settings"),_T("LegalNotice"),m_bShowLegalNotice == VARIANT_TRUE ? _T("On") : _T("Off") ));
	
	return CWinApp::ExitInstance();
}


AcceptanceType CLibraryEditorApp::ShowLegalNoticeAtStartup(void)
{
   if ( m_bShowLegalNotice )
   {
      return ShowLegalNotice(VARIANT_TRUE);
   }

   return atAccept;
}

AcceptanceType CLibraryEditorApp::ShowLegalNotice(VARIANT_BOOL bGiveChoice)
{
   CComPtr<IARPNotice> pNotice;
   if ( FAILED(pNotice.CoCreateInstance( CLSID_ARPNotice ) ) )
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

void CLibraryEditorApp::OnAppLegal() 
{
	// TODO: Add your command handler code here
	ShowLegalNotice();
}
