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

// ServerDefinitionDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ServerDefinitionDlg.h"

#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg dialog
// constructor when adding a new server
CServerDefinitionDlg::CServerDefinitionDlg(const CPGSuperCatalogServers& servers,CWnd* pParent /*=NULL*/)
	: CDialog(CServerDefinitionDlg::IDD, pParent), m_Servers(servers)
{
   // default to ftp
   m_ServerType = srtInternetFtp;

	//{{AFX_DATA_INIT(CServerDefinitionDlg)
	m_ServerName = _T("");
	m_ServerAddress = _T("");
	//}}AFX_DATA_INIT
}

// constructor when editing an existing server
CServerDefinitionDlg::CServerDefinitionDlg(const CPGSuperCatalogServers& servers,const CPGSuperCatalogServer* pCurrentServer,
                                           CWnd* pParent /*=NULL*/)
	: CDialog(CServerDefinitionDlg::IDD, pParent), m_Servers(servers)
{
	m_ServerName = pCurrentServer->GetServerName();
   m_ServerType = pCurrentServer->GetServerType();

   // Type casting here a bit ugly, but we are bonded to our servers
   if (m_ServerType==srtInternetFtp)
   {
      const CFtpPGSuperCatalogServer* psvr = dynamic_cast<const CFtpPGSuperCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_ServerAddress = psvr->GetAddress();
   }
   else if (m_ServerType==srtInternetHttp)
   {
      const CHttpPGSuperCatalogServer* psvr = dynamic_cast<const CHttpPGSuperCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_ServerAddress = psvr->GetAddress();
   }
   else if (m_ServerType==srtLocal)
   {
      const CFileSystemPGSuperCatalogServer* psvr = dynamic_cast<const CFileSystemPGSuperCatalogServer*>(pCurrentServer);
      ATLASSERT(psvr);
      m_LocalMasterLibraryFile = psvr->GetLibraryFileName();
      m_LocalWorkgroupTemplateFolder = psvr->GetTemplateFolderPath();
   }
   else
   {
      ATLASSERT(0);
   }

}

void CServerDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   //// map dialog data to server type
   //int server_type;
   //if ( !pDX->m_bSaveAndValidate)
   //{
   //   if (m_ServerType==InternetFtp)
   //      server_type=0;
   //   else if (m_ServerType==InternetHttp)
   //      server_type=1;
   //   else if (m_ServerType==Local)
   //      server_type=2;
   //   else
   //      ATLASSERT(0);
   //}
   //DDX_CBItemData
   DDX_CBItemData(pDX, IDC_SERVER_TYPE, m_ServerType);
   //if ( pDX->m_bSaveAndValidate)
   //{
   //   if (server_type==0)
   //      m_ServerType=InternetFtp; 
   //   else if (server_type==1)
   //      m_ServerType=InternetHttp;
   //   else if (server_type==2)
   //      m_ServerType=Local;
   //   else
   //      ATLASSERT(0);
   //}

   //{{AFX_DATA_MAP(CServerDefinitionDlg)
   DDX_Text(pDX, IDC_NAME, m_ServerName);
   DDX_Text(pDX, IDC_URL, m_ServerAddress);
   //}}AFX_DATA_MAP

   // These are for the local network option
   DDX_FilenameControl(pDX, IDC_LIBRARY_FILE_LOCATION, IDC_LIBRARY_FILE_BROWSE,m_ctrlLibraryFile, 0/*GF_FILEMUSTEXIST*/, "Please specify library file", "Library Files (*.lbr)|*.lbr||");
   DDX_FilenameValue(pDX, m_ctrlLibraryFile, m_LocalMasterLibraryFile);

   if ( pDX->m_bSaveAndValidate && m_ServerType==srtLocal )
      DDV_FilenameControl(pDX, m_ctrlLibraryFile);

   DDX_FolderControl(pDX, IDC_WORKGROUP_TEMPLATE_LOCATION, IDC_WORKGROUP_TEMPLATE_BROWSE, m_ctrlWorkgroupFolder, 0, "Please specify a folder");
   DDX_FolderValue(pDX, m_ctrlWorkgroupFolder, m_LocalWorkgroupTemplateFolder);

   if ( pDX->m_bSaveAndValidate && m_ServerType==srtLocal )
      DDV_FolderControl(pDX, m_ctrlWorkgroupFolder, GFLDR_FOLDER_MUST_EXIST);

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

	CDialog::OnInitDialog();
	
   m_OriginalServerName = m_ServerName;

   ConfigureControls(m_ServerType);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CServerDefinitionDlg::ConfigureControls(SharedResourceType type)
{
   int show_local = SW_SHOW;
   int show_internet = SW_HIDE;

   if (type==srtInternetFtp || type==srtInternetHttp)
   {
      GetDlgItem(IDC_URL_STATIC)->SetWindowText(_T("Server Address (URL)"));
      show_local = SW_HIDE;
      show_internet = SW_SHOW;
   }
   else
   {
      ATLASSERT(type==srtLocal);
      GetDlgItem(IDC_URL_STATIC)->SetWindowText(_T("Master Library File Name (full path)"));
   }

   GetDlgItem(IDC_URL)->ShowWindow(show_internet);
   GetDlgItem(IDC_LIBRARY_FILE_LOCATION)->ShowWindow(show_local);
   GetDlgItem(IDC_LIBRARY_FILE_BROWSE)->ShowWindow(show_local);
   GetDlgItem(IDC_TEMPLATE_STATIC)->ShowWindow(show_local);
   GetDlgItem(IDC_WORKGROUP_TEMPLATE_LOCATION)->ShowWindow(show_local);
   GetDlgItem(IDC_WORKGROUP_TEMPLATE_BROWSE)->ShowWindow(show_local);
}

void CServerDefinitionDlg::OnBnClickedTestServer()
{
   BOOL st = UpdateData(true);
   if (st!=0)
   {
      CPGSuperCatalogServer* psvr = CreateServer();
      if (psvr==NULL)
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
         bool st = psvr->TestServer(msg);

         if (!st)
         {
            ::AfxMessageBox(msg,MB_OK|MB_ICONEXCLAMATION);
         }
         else
         {
            AfxMessageBox(_T("Connected to server successfully"));
         }
      }
   }
}

CPGSuperCatalogServer* CServerDefinitionDlg::CreateServer()
{
   // Factory up our server
   if (m_ServerType==srtInternetFtp)
   {
      CFtpPGSuperCatalogServer* psvr = new CFtpPGSuperCatalogServer(m_ServerName, m_ServerAddress);
      ATLASSERT(psvr);
      return psvr;
   }
   else if (m_ServerType==srtInternetHttp)
   {
      CHttpPGSuperCatalogServer* psvr = new CHttpPGSuperCatalogServer(m_ServerName, m_ServerAddress);
      ATLASSERT(psvr);
      return psvr;
   }
   else if (m_ServerType==srtLocal)
   {
      CFileSystemPGSuperCatalogServer* psvr = new CFileSystemPGSuperCatalogServer(m_ServerName, m_LocalMasterLibraryFile, m_LocalWorkgroupTemplateFolder);
      ATLASSERT(psvr);
      return psvr;
   }
   else
   {
      ATLASSERT(0);
   }

   return NULL;
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
