///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// ConfigurePGSuperDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "ConfigurePGSuperDlg.h"
#include "CatalogServerDlg.h"
#include "HtmlHelp\HelpTopics.hh"
#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg dialog


CConfigurePGSuperDlg::CConfigurePGSuperDlg(const CPGSuperCatalog& catalog,BOOL bFirstRun,CWnd* pParent /*=NULL*/)
	: CDialog(CConfigurePGSuperDlg::IDD, pParent), m_Catalog(catalog)
{
	//{{AFX_DATA_INIT(CConfigurePGSuperDlg)
	m_Company = _T("");
	m_Engineer = _T("");
	m_Publisher = _T("");
	//}}AFX_DATA_INIT

   m_bFirstRun = bFirstRun;
   m_bNetworkError = false;
}


void CConfigurePGSuperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigurePGSuperDlg)
	DDX_Control(pDX, IDC_PUBLISHERS, m_PublisherList);
	DDX_Text(pDX, IDC_COMPANY_NAME, m_Company);
	DDX_Text(pDX, IDC_ENGINEER_NAME, m_Engineer);
	DDX_Radio(pDX, IDC_GENERIC, m_Method);
	DDX_LBString(pDX, IDC_PUBLISHERS, m_Publisher);
	//}}AFX_DATA_MAP

   // These are for the local network option
   DDX_FilenameControl(pDX, IDC_LIBRARY_FILE_LOCATION, IDC_LIBRARY_FILE_BROWSE,m_ctrlLibraryFile, GF_FILEMUSTEXIST, "Please specify library file", "Library Files (*.lbr)|*.lbr||");
   DDX_FilenameValue(pDX, m_ctrlLibraryFile, m_LocalMasterLibraryFile);

   if ( pDX->m_bSaveAndValidate && m_Method == 2 )
      DDV_FilenameControl(pDX, m_ctrlLibraryFile);

   DDX_FolderControl(pDX, IDC_WORKGROUP_TEMPLATE_LOCATION, IDC_WORKGROUP_TEMPLATE_BROWSE, m_ctrlWorkgroupFolder, 0, "Please specify a folder");
   DDX_FolderValue(pDX, m_ctrlWorkgroupFolder, m_LocalWorkgroupTemplateFolder);

   if ( pDX->m_bSaveAndValidate && m_Method == 2 )
      DDV_FolderControl(pDX, m_ctrlWorkgroupFolder, GFLDR_FOLDER_MUST_EXIST);

   DDX_FolderControl(pDX, IDC_USER_TEMPLATE_LOCATION, IDC_USER_TEMPLATE_BROWSE, m_ctrlUserFolder, 0, "Please specify a folder");
   DDX_FolderValue(pDX, m_ctrlUserFolder, m_UserFolder);
   DDV_FolderControl(pDX, m_ctrlUserFolder, GFLDR_FOLDER_MUST_EXIST);

   DDX_CBItemData(pDX,IDC_UPDATE_FREQUENCY,m_CacheUpdateFrequency);
}


BEGIN_MESSAGE_MAP(CConfigurePGSuperDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigurePGSuperDlg)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_LBN_SETFOCUS(IDC_PUBLISHERS, OnSetfocusPublishers)
	ON_EN_SETFOCUS(IDC_LIBRARY_FILE_LOCATION, OnSetFocusNetwork)
   ON_BN_CLICKED(IDC_ADD,OnAddCatalogServer)
	ON_CBN_SELCHANGE(IDC_SERVERS, OnServerChanged)
	ON_BN_CLICKED(IDC_UPDATENOW, OnUpdatenow)
	ON_EN_SETFOCUS(IDC_WORKGROUP_TEMPLATE_LOCATION, OnSetFocusNetwork)
	ON_BN_CLICKED(IDC_LIBRARY_FILE_BROWSE, OnSetFocusNetwork)
	ON_BN_CLICKED(IDC_WORKGROUP_TEMPLATE_BROWSE, OnSetFocusNetwork)
	ON_BN_CLICKED(IDC_GENERIC, OnGeneric)
	ON_BN_CLICKED(IDC_LOCAL, OnLocal)
	ON_BN_CLICKED(IDC_DOWNLOAD, OnDownload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg message handlers

void CConfigurePGSuperDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONFIGURE_PGSUPER );
}

BOOL CConfigurePGSuperDlg::OnInitDialog() 
{
   UpdateFrequencyList();
   ServerList();

   CWnd* pWnd = GetDlgItem(IDC_EDIT);
   pWnd->SetWindowText("PGSuper doesn't have any default girders, design criteria, or other settings. All of the \"default\" information is stored in the Master Library and User and Workgroup Templates.\r\n\r\nPGSuper must be configured to use a specific Master Library and Templates. Use the Help button to get more information.");

   pWnd = GetDlgItem(IDC_FIRST_RUN);
   if ( m_bFirstRun )
      pWnd->SetWindowText("This is the first time you've run PGSuper since it was installed. Before you can use PGSuper, it must by configured.");
   else
      pWnd->SetWindowText("Set the User, Library, and Template Configuration information.");

   CDialog::OnInitDialog();

   if ( m_bFirstRun )
      HideOkAndCancelButtons();

   OnMethod();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigurePGSuperDlg::HideOkAndCancelButtons()
{
   CWnd* pCancel = GetDlgItem(IDCANCEL);
   pCancel->ShowWindow(SW_HIDE);


   CWnd* pOK = GetDlgItem(IDOK);
   pOK->ShowWindow(SW_HIDE);
}

void CConfigurePGSuperDlg::UpdateFrequencyList()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_UPDATE_FREQUENCY);

   int idx = pCB->AddString("Never");
   pCB->SetItemData(idx,Never);

   idx = pCB->AddString("Every time PGSuper starts");
   pCB->SetItemData(idx,Always);

   idx = pCB->AddString("Once a day");
   pCB->SetItemData(idx,Daily);

   idx = pCB->AddString("Once a week");
   pCB->SetItemData(idx,Weekly);

   idx = pCB->AddString("Once a month");
   pCB->SetItemData(idx,Monthly);
}

void CConfigurePGSuperDlg::ServerList()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SERVERS);
   int idx = pCB->GetCurSel();
   CString strCurrAddress;

   if ( idx != CB_ERR )
      pCB->GetLBText(idx,strCurrAddress);

   pCB->ResetContent();


   long nServers = m_Servers.GetServerCount();
   for ( long i = 0; i < nServers; i++ )
   {
      CString strName, strAddress;
      m_Servers.GetServer(i,strName,strAddress);

      pCB->AddString(strName);
   }

   if ( idx == CB_ERR )
   {
      strCurrAddress = m_Catalog.GetCatalogName();
   }
   idx = pCB->SelectString(-1,strCurrAddress);
   
   if ( idx == CB_ERR )
   {
      pCB->SetCurSel(0);
   }

   OnServerChanged();
}

void CConfigurePGSuperDlg::OnSetfocusPublishers() 
{
   // when the Publisher list gets the focus, select the radio button
   CheckRadioButton(IDC_GENERIC,IDC_LOCAL,IDC_DOWNLOAD);
}

void CConfigurePGSuperDlg::OnSetFocusNetwork() 
{
   CheckRadioButton(IDC_GENERIC,IDC_LOCAL,IDC_LOCAL);
}

void CConfigurePGSuperDlg::OnAddCatalogServer()
{
   CCatalogServerDlg dlg;
   dlg.m_Servers = m_Servers;
   if ( dlg.DoModal() == IDOK )
   {
      m_Servers = dlg.m_Servers;
      ServerList();
   }
}

void CConfigurePGSuperDlg::OnServerChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SERVERS);
   int idx = pCB->GetCurSel();

   if ( idx != CB_ERR )
   {
      CString strName;
      pCB->GetLBText(idx,strName);

      CString strAddress = m_Servers.GetServerAddress(strName);
      m_Catalog.SetCatalogServer(strName,strAddress);

      PublisherList();

      OnSetfocusPublishers();
   }
}


void CConfigurePGSuperDlg::PublisherList() 
{
   // fill up the list box
   m_bNetworkError = m_Catalog.IsNetworkError();

   if ( !m_bNetworkError )
   {
      std::vector<CString> items = m_Catalog.GetCatalogItems();
      CListBox* pLB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);

      int idx = pLB->GetCurSel();
      pLB->ResetContent();

      for ( std::vector<CString>::iterator iter = items.begin(); iter != items.end(); iter++ )
      {
         CString strItem = *iter;
         pLB->AddString(strItem);
      }

      if ( idx != LB_ERR )
         pLB->SetCurSel(idx);
      else
         pLB->SetCurSel(0);

      pLB->EnableWindow(TRUE);
      GetDlgItem(IDC_UPDATENOW)->EnableWindow(TRUE);
   }
   else
   {
      // internet option isn't avaliable
      CListBox* pLB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);
      pLB->ResetContent();
      pLB->AddString("An error occured while accessing the list");
      pLB->AddString("of publishers.");
      pLB->AddString("");
      pLB->AddString("The most recently downloaded settings");
      pLB->AddString("will be used");
      pLB->EnableWindow(FALSE);


      if ( m_bFirstRun )
      {
         m_Method = (m_Method == 1 ? 0 : m_Method);
         GetDlgItem(IDC_DOWNLOAD)->EnableWindow(FALSE);
      }
      else
      {
         if ( m_Method == 1 )
            GetDlgItem(IDC_UPDATENOW)->EnableWindow(FALSE);
      }
   }
}

void CConfigurePGSuperDlg::OnUpdatenow() 
{
	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	EndDialog(IDC_UPDATENOW);
}

void CConfigurePGSuperDlg::OnGeneric() 
{
   GetDlgItem(IDC_UPDATENOW)->EnableWindow(TRUE);
   OnMethod();
}

void CConfigurePGSuperDlg::OnLocal() 
{
   GetDlgItem(IDC_UPDATENOW)->EnableWindow(TRUE);
   OnMethod();
}

void CConfigurePGSuperDlg::OnDownload() 
{
   GetDlgItem(IDC_UPDATENOW)->EnableWindow( m_Catalog.IsNetworkError() ? FALSE : TRUE);
   OnMethod();
}

void CConfigurePGSuperDlg::OnMethod()
{
	BOOL bEnable = IsDlgButtonChecked(IDC_DOWNLOAD);
   GetDlgItem(IDC_SERVERS)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADD)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVERS_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVERS_STATIC2)->EnableWindow(bEnable);
   GetDlgItem(IDC_PUBLISHERS)->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_SERVER_STATIC3)->ShowWindow(!bEnable ? SW_SHOW : SW_HIDE);

	bEnable = IsDlgButtonChecked(IDC_LOCAL);
   GetDlgItem(IDC_LOCAL_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_LIBRARY_FILE_LOCATION)->EnableWindow(bEnable);
   GetDlgItem(IDC_LOCALS_STATIC2)->EnableWindow(bEnable);
   GetDlgItem(IDC_WORKGROUP_TEMPLATE_LOCATION)->EnableWindow(bEnable);
}
