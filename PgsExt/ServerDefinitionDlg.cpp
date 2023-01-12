///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// ServerDefinitionDlg.cpp : implementation file
//
#include <PgsExt\PgsExtLib.h>
#include "ServerDefinitionDlg.h"
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg dialog
// constructor when adding a new server
CServerDefinitionDlg::CServerDefinitionDlg(const CCatalogServers& servers,const CString& strExt,const CString& appName,CWnd* pParent /*=nullptr*/)
	: CDialog(CServerDefinitionDlg::IDD, pParent), m_Servers(servers), m_TemplateFileExt(strExt),
   m_AppName(appName)
{
   // default to ftp
   m_ServerType = srtInternetFtp;

	//{{AFX_DATA_INIT(CServerDefinitionDlg)
	m_ServerName = _T("");
	m_ServerAddress = _T("");
	//}}AFX_DATA_INIT
}

// constructor when editing an existing server
CServerDefinitionDlg::CServerDefinitionDlg(const CCatalogServers& servers,const CCatalogServer* pCurrentServer,const CString& strExt,const CString& appName,
                                           CWnd* pParent /*=nullptr*/)
	: CDialog(CServerDefinitionDlg::IDD, pParent), m_Servers(servers), m_TemplateFileExt(strExt),
   m_AppName(appName)
{
	m_ServerName = pCurrentServer->GetServerName();
   m_ServerType = pCurrentServer->GetServerType();

   // Type casting here a bit ugly, but we are bonded to our servers
   if (m_ServerType==srtInternetFtp)
   {
      const CFtpCatalogServer* psvr = dynamic_cast<const CFtpCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_ServerAddress = psvr->GetAddress();
   }
   else if (m_ServerType==srtInternetHttp)
   {
      const CHttpCatalogServer* psvr = dynamic_cast<const CHttpCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_ServerAddress = psvr->GetAddress();
   }
   else if (m_ServerType==srtLocal)
   {
      const CFileSystemCatalogServer* psvr = dynamic_cast<const CFileSystemCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_LocalMasterLibraryFile = psvr->GetLibraryFileName();
      m_LocalWorkgroupTemplateFolder = psvr->GetTemplateFolderPath();
   }
   else if (m_ServerType==srtLocalIni)
   {
      const CFileSystemIniCatalogServer* psvr = dynamic_cast<const CFileSystemIniCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_LocalWorkgroupTemplateFolder = psvr->GetAddress();
   }
   else
   {
      ATLASSERT(false);
   }

}

void CServerDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   DDX_CBItemData(pDX, IDC_SERVER_TYPE, m_ServerType);

   //{{AFX_DATA_MAP(CServerDefinitionDlg)
   DDX_Text(pDX, IDC_NAME, m_ServerName);
   DDX_Text(pDX, IDC_URL, m_ServerAddress);
   //}}AFX_DATA_MAP

   // These are for the local network option
   DDX_FilenameControl(pDX, IDC_LIBRARY_FILE_LOCATION, IDC_LIBRARY_FILE_BROWSE,m_ctrlLibraryFile, 0/*GF_FILEMUSTEXIST*/, CString("Please specify library file"), CString("Library Files (*.lbr)|*.lbr||"));
   DDX_FilenameValue(pDX, m_ctrlLibraryFile, m_LocalMasterLibraryFile);

   if ( pDX->m_bSaveAndValidate && m_ServerType==srtLocal )
   {
      DDV_FilenameControl(pDX, m_ctrlLibraryFile);
   }

   DDX_FolderControl(pDX, IDC_WORKGROUP_TEMPLATE_LOCATION, IDC_WORKGROUP_TEMPLATE_BROWSE, m_ctrlWorkgroupFolder, 0, CString("Please specify a folder"));
   DDX_FolderValue(pDX, m_ctrlWorkgroupFolder, m_LocalWorkgroupTemplateFolder);

   if ( pDX->m_bSaveAndValidate && m_ServerType==srtLocal &&  m_ServerType==srtLocalIni)
   {
      DDV_FolderControl(pDX, m_ctrlWorkgroupFolder, GFLDR_FOLDER_MUST_EXIST);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      pDX->PrepareEditCtrl(IDC_NAME);
      if ( m_ServerName.GetLength()==0 )
      {
         AfxMessageBox(_T("Please enter a server name"),MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }

      if ( m_OriginalServerName != m_ServerName && m_Servers.IsServerDefined(m_ServerName) )
      {
         CString strMsg;
         strMsg.Format(_T("A server with the name %s is already defined. Please use a different name"),m_ServerName);
         AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }

      pDX->PrepareEditCtrl(IDC_URL);
      if ( m_ServerAddress.GetLength()==0 && (m_ServerType==srtInternetFtp || m_ServerType==srtInternetHttp) )
      {
         AfxMessageBox(_T("Please enter a server address"),MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CServerDefinitionDlg, CDialog)
	//{{AFX_MSG_MAP(CServerDefinitionDlg)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_TEST_SERVER, &CServerDefinitionDlg::OnBnClickedTestServer)
   ON_CBN_SELCHANGE(IDC_SERVER_TYPE, &CServerDefinitionDlg::OnCbnSelchangeServerType)
   ON_BN_CLICKED(ID_HELP, &CServerDefinitionDlg::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg message handlers

BOOL CServerDefinitionDlg::OnInitDialog() 
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_SERVER_TYPE);
   ASSERT(ptype_ctrl!=0);
   int idx = ptype_ctrl->AddString(_T("Internet FTP Server"));
   ptype_ctrl->SetItemData(idx,srtInternetFtp);
   idx = ptype_ctrl->AddString(_T("Internet HTTP Server"));
   ptype_ctrl->SetItemData(idx,srtInternetHttp);
   idx = ptype_ctrl->AddString(_T("Local or Network File System"));
   ptype_ctrl->SetItemData(idx,srtLocal);
   idx = ptype_ctrl->AddString(_T("Local or Network File System Using .ini File"));
   ptype_ctrl->SetItemData(idx,srtLocalIni);

	CDialog::OnInitDialog();
	
   m_OriginalServerName = m_ServerName;

   ConfigureControls(m_ServerType);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CServerDefinitionDlg::ConfigureControls(SharedResourceType type)
{
   int show_local_libr = SW_SHOW;
   int show_local_templ = SW_SHOW;
   int show_internet = SW_HIDE;

   if (type==srtInternetFtp || type==srtInternetHttp)
   {
      GetDlgItem(IDC_URL_STATIC)->SetWindowText(_T("Server Address (URL)"));
      show_local_libr = SW_HIDE;
      show_local_templ = SW_HIDE;
      show_internet = SW_SHOW;
   }
   else if (type==srtLocalIni)
   {
      GetDlgItem(IDC_URL_STATIC)->SetWindowText(_T(""));
      GetDlgItem(IDC_TEMPLATE_STATIC)->SetWindowText(_T("Path to where .ini file is located"));
      show_local_libr = SW_HIDE;
      show_local_templ = SW_SHOW;
      show_internet = SW_HIDE;
   }
   else
   {
      ATLASSERT(type==srtLocal);
      GetDlgItem(IDC_URL_STATIC)->SetWindowText(_T("Master Library File Name (full path)"));
      GetDlgItem(IDC_TEMPLATE_STATIC)->SetWindowText(_T("Path to Template Files"));
   }

   GetDlgItem(IDC_URL)->ShowWindow(show_internet);
   GetDlgItem(IDC_LIBRARY_FILE_LOCATION)->ShowWindow(show_local_libr);
   GetDlgItem(IDC_LIBRARY_FILE_BROWSE)->ShowWindow(show_local_libr);
   GetDlgItem(IDC_TEMPLATE_STATIC)->ShowWindow(show_local_templ);
   GetDlgItem(IDC_WORKGROUP_TEMPLATE_LOCATION)->ShowWindow(show_local_templ);
   GetDlgItem(IDC_WORKGROUP_TEMPLATE_BROWSE)->ShowWindow(show_local_templ);
}

void CServerDefinitionDlg::OnBnClickedTestServer()
{
   if (UpdateData(true))
   {
      CCatalogServer* pServer = CreateServer();
      if (pServer == nullptr)
      {
         AfxMessageBox(_T("Failed to create server. Check your input data"));
      }
      else
      {
         // create a progress window
         CComPtr<IProgressMonitorWindow> wndProgress;
         wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
         wndProgress->put_HasGauge(VARIANT_FALSE);
         wndProgress->put_HasCancel(VARIANT_FALSE);
         wndProgress->Show(CComBSTR("Attempting to connect to and test server...."),GetSafeHwnd());

         CString msg;
         if ( pServer->TestServer(msg) )
         {
            AfxMessageBox(_T("Connected to server successfully"));
         }
         else
         {
            ::AfxMessageBox(msg,MB_OK|MB_ICONEXCLAMATION);
         }
      }

      delete pServer;
   }
}

CCatalogServer* CServerDefinitionDlg::CreateServer()
{
   // Factory up our server
   if (m_ServerType==srtInternetFtp)
   {
      CFtpCatalogServer* psvr = new CFtpCatalogServer(m_Servers.GetAppName(),m_ServerName, m_ServerAddress, m_TemplateFileExt);
      ATLASSERT(psvr);
      return psvr;
   }
   else if (m_ServerType==srtInternetHttp)
   {
      CHttpCatalogServer* psvr = new CHttpCatalogServer(m_Servers.GetAppName(),m_ServerName, m_ServerAddress, m_TemplateFileExt);
      ATLASSERT(psvr);
      return psvr;
   }
   else if (m_ServerType==srtLocal)
   {
      CFileSystemCatalogServer* psvr = new CFileSystemCatalogServer(m_Servers.GetAppName(),m_ServerName, m_LocalMasterLibraryFile, m_LocalWorkgroupTemplateFolder, m_TemplateFileExt);
      ATLASSERT(psvr);
      return psvr;
   }
   else if (m_ServerType==srtLocalIni)
   {
      CFileSystemIniCatalogServer* psvr = new CFileSystemIniCatalogServer(m_Servers.GetAppName(),m_ServerName, m_LocalWorkgroupTemplateFolder, m_TemplateFileExt);
      ATLASSERT(psvr);
      return psvr;
   }
   else
   {
      ATLASSERT(false);
   }

   return nullptr;
}

void CServerDefinitionDlg::OnCbnSelchangeServerType()
{
   CComboBox* ptype_ctrl = (CComboBox*)GetDlgItem(IDC_SERVER_TYPE);
   ASSERT(ptype_ctrl!=0);
   int idx = ptype_ctrl->GetCurSel();
   ASSERT(idx!=CB_ERR);
   SharedResourceType type = (SharedResourceType)(ptype_ctrl->GetItemData(idx));

   ConfigureControls(type);
}

void CServerDefinitionDlg::OnHelp()
{
   EAFHelp( m_AppName, IDH_CONFIGURATION_SERVER_DEFINITION );
}
