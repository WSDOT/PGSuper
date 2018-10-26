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

// ConfigurePGSuperDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
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


CConfigurePGSuperDlg::CConfigurePGSuperDlg(BOOL bFirstRun,CWnd* pParent /*=NULL*/)
	: CDialog(CConfigurePGSuperDlg::IDD, pParent)
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
	DDX_Text(pDX, IDC_COMPANY_NAME, m_Company);
	DDX_Text(pDX, IDC_ENGINEER_NAME, m_Engineer);
	DDX_Radio(pDX, IDC_GENERIC, m_Method);
	DDX_LBString(pDX, IDC_PUBLISHERS, m_Publisher);
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_PUBLISHER_HYPERLINK,  m_PublisherHyperLink);

   DDX_CBItemData(pDX,IDC_UPDATE_FREQUENCY,m_CacheUpdateFrequency);

   // We need to force an update if our catalog server or publisher changed
   if (!pDX->m_bSaveAndValidate)
   {
      // save original data on the way in
      m_OriginalSharedResourceType = m_SharedResourceType;
      if ( m_SharedResourceType != srtDefault )
      {
	      m_OriginalPublisher = m_Publisher;
         m_OriginalServer = m_CurrentServer;

         // Taking a shortcut here - we know if a server is edited it will
         // be reallocated, and given a new address. A better approach would 
         // involve some way of diffing polymorphic types
         m_OriginalServerPtr = m_Servers.GetServer(m_CurrentServer);
      }
   }
   else
   {
      // Error checking can be very tricky if network burps
      // Make sure the selected publisher is in the catalog
      if(m_Method != 0) // srtDefault
      {
         const CPGSuperCatalogServer* psrvr = m_Servers.GetServer(m_CurrentServer);
         if (psrvr==NULL)
         {
            ::AfxMessageBox(_T("The selected server is invalid - please select another or the generic library package"),MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         if(!psrvr->DoesPublisherExist(m_Publisher))
         {
            ::AfxMessageBox(_T("The selected server is invalid - please select another or the generic library package"),MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         m_SharedResourceType = psrvr->GetServerType();

      }
      else
      {
         m_SharedResourceType = srtDefault;
      }

      // check if we changed publishers (don't care if Default)
      if ( !m_bUpdateCache && m_SharedResourceType != srtDefault)
      {
         bool bchanged = m_bFirstRun!=FALSE;
         bchanged |= m_OriginalSharedResourceType != m_SharedResourceType;

         if ( m_SharedResourceType != srtLocal)
         {
            bchanged |= m_OriginalPublisher != m_Publisher;
         }

         bchanged |= m_OriginalServer != m_CurrentServer;

         if (m_OriginalServer == m_CurrentServer)
         {
            // use pointer comparison 
            const CPGSuperCatalogServer* psvr = m_Servers.GetServer(m_CurrentServer); 
            bchanged |= psvr != m_OriginalServerPtr;
         }

         if (bchanged)
         {
            // Prompt user to update server
            int st = ::AfxMessageBox(_T("A change has been made to the server configuration. You must run an update before the configuration can be saved. Do you want to run update now?"),MB_YESNO);
            if (st==IDYES)
            {
               m_bUpdateCache = true;
            }
            else
            {
               pDX->Fail();
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CConfigurePGSuperDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigurePGSuperDlg)
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_ADD,OnAddCatalogServer)
	ON_CBN_SELCHANGE(IDC_SERVERS, OnServerChanged)
	ON_BN_CLICKED(IDC_UPDATENOW, OnUpdatenow)
	ON_BN_CLICKED(IDC_GENERIC, OnGeneric)
	ON_BN_CLICKED(IDC_DOWNLOAD, OnDownload)
	//}}AFX_MSG_MAP
   ON_LBN_SELCHANGE(IDC_PUBLISHERS, &CConfigurePGSuperDlg::OnLbnSelchangePublishers)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg message handlers

void CConfigurePGSuperDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONFIGURE_PGSUPER );
}

BOOL CConfigurePGSuperDlg::OnInitDialog() 
{
   // no cache update until we're told
   m_bUpdateCache = false;

   m_Method = m_SharedResourceType==srtDefault ? 0 : 1;

   UpdateFrequencyList();

   // must be called before ServerList();
   CDialog::OnInitDialog();

   ServerList();

   CWnd* pWnd = GetDlgItem(IDC_EDIT);
   pWnd->SetWindowText(_T("PGSuper doesn't have any default girders, design criteria, or other settings. All of the \"default\" information is stored in the Master Library and User and Workgroup Templates.\r\n\r\nPGSuper must be configured to use a specific Master Library and Templates. Use the Help button to get more information."));

   pWnd = GetDlgItem(IDC_FIRST_RUN);
   if ( m_bFirstRun )
      pWnd->SetWindowText(_T("This is the first time you've run PGSuper since it was installed. Before you can use PGSuper, it must be configured."));
   else
      pWnd->SetWindowText(_T("Set the User, Library, and Template Configuration information."));

   if ( m_bFirstRun )
      HideOkAndCancelButtons();

   OnMethod();
   OnServerChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigurePGSuperDlg::HideOkAndCancelButtons()
{
   CWnd* pCancel = GetDlgItem(IDCANCEL);
   pCancel->ShowWindow(SW_HIDE);


//   CWnd* pOK = GetDlgItem(IDOK);
//   pOK->SetDlgCtrlID(IDC_UPDATENOW);
//   pOK->ShowWindow(SW_HIDE);
}

void CConfigurePGSuperDlg::UpdateFrequencyList()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_UPDATE_FREQUENCY);

   int idx = pCB->AddString(_T("Never"));
   pCB->SetItemData(idx,Never);

   idx = pCB->AddString(_T("Every time PGSuper starts"));
   pCB->SetItemData(idx,Always);

   idx = pCB->AddString(_T("Once a day"));
   pCB->SetItemData(idx,Daily);

   idx = pCB->AddString(_T("Once a week"));
   pCB->SetItemData(idx,Weekly);

   idx = pCB->AddString(_T("Once a month"));
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
      const CPGSuperCatalogServer* pserver = m_Servers.GetServer(i);

      pCB->AddString( pserver->GetServerName() );
   }

   if ( idx == CB_ERR )
   {
      strCurrAddress = m_CurrentServer;
   }
   idx = pCB->SelectString(-1,strCurrAddress);
   
   if ( idx == CB_ERR )
   {
      pCB->SetCurSel(0);
   }
}

void CConfigurePGSuperDlg::OnAddCatalogServer()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CCatalogServerDlg dlg;
   dlg.m_Servers = m_Servers;
   if ( dlg.DoModal() == IDOK )
   {
      m_Servers = dlg.m_Servers;
      ServerList();
      OnServerChanged();
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

      const CPGSuperCatalogServer* pserver = m_Servers.GetServer(strName);
      ATLASSERT(pserver);
      m_CurrentServer = strName;

      PublisherList();

      ConfigureWebLink();
   }
}



void CConfigurePGSuperDlg::PublisherList() 
{
   CButton* pGen = (CButton*)GetDlgItem(IDC_GENERIC);
   int check = pGen->GetCheck();

   if (BST_CHECKED==check)
   {
      // default publisher
      // no need to hit the internet if we don't have to
      CListBox* pLB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);
      pLB->ResetContent();
   }
   else
   {
      // this can take a while, so create a progress window,
      CComPtr<IProgressMonitorWindow> wndProgress;
      wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
      wndProgress->put_HasGauge(VARIANT_FALSE);
      wndProgress->put_HasCancel(VARIANT_FALSE);
      wndProgress->Show(CComBSTR(_T("Fetching package names from server...")),GetSafeHwnd());

      // fill up the list box
      const CPGSuperCatalogServer* pserver = m_Servers.GetServer(m_CurrentServer);
      m_bNetworkError = pserver->IsNetworkError();

      bool is_error = m_bNetworkError;

      if ( !m_bNetworkError )
      {
         try
         {
            std::vector<CString> items = pserver->GetPublishers();

            CListBox* pLB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);

            int idx = pLB->GetCurSel();
            pLB->ResetContent();

            for ( std::vector<CString>::iterator iter = items.begin(); iter != items.end(); iter++ )
            {
               CString strItem = *iter;
               pLB->AddString(strItem);
            }

            if ( idx != LB_ERR && idx< (int)items.size() )
            {
               pLB->SetCurSel(idx);
            }
            else
            {
               // try to set current publisher
               int sst = pLB->SelectString(-1,m_Publisher);
               if (sst==LB_ERR)
                  pLB->SetCurSel(0);
            }

            pLB->EnableWindow(TRUE);
            GetDlgItem(IDC_UPDATENOW)->EnableWindow(TRUE);
         }
         catch (CCatalogServerException excp)
         {
            // lots of things could have gone wrong, none of them good.
            // let the exception do the talking
            CString msg = excp.GetErrorMessage();
            ::AfxMessageBox(msg,MB_ICONEXCLAMATION);
            is_error = true;
         }
      }

      if (is_error)
      {
         // internet option isn't avaliable
         CListBox* pLB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);
         pLB->ResetContent();

		 pLB->AddString(_T("An error occured while accessing the list"));
		 pLB->AddString(_T("of publishers."));
         if (!m_bFirstRun )
		 {
			 pLB->AddString(_T(""));
			 pLB->AddString(_T("The most recently downloaded settings"));
			 pLB->AddString(_T("will be used"));
			 pLB->EnableWindow(FALSE);
		 }

         if ( m_Method == 1 )
            GetDlgItem(IDC_UPDATENOW)->EnableWindow(FALSE);
      }
   }
}

void CConfigurePGSuperDlg::OnUpdatenow() 
{
   m_bUpdateCache = true;

	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	EndDialog(IDOK);
}

void CConfigurePGSuperDlg::OnGeneric() 
{
   OnMethod();
   this->PublisherList();
   ConfigureWebLink();
}

void CConfigurePGSuperDlg::OnDownload() 
{
   OnMethod();
   this->PublisherList();
   ConfigureWebLink();
}

void CConfigurePGSuperDlg::OnMethod()
{
	BOOL bEnable = IsDlgButtonChecked(IDC_DOWNLOAD);
   GetDlgItem(IDC_SERVERS)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADD)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVERS_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVERS_STATIC2)->EnableWindow(bEnable);
   GetDlgItem(IDC_PUBLISHERS)->EnableWindow(bEnable);
   GetDlgItem(IDC_UPDATENOW)->EnableWindow( bEnable);
   GetDlgItem(IDC_UPDATE_FREQUENCY)->EnableWindow( bEnable);
   GetDlgItem(IDC_UPDATES_STATIC)->EnableWindow( bEnable);
   GetDlgItem(IDC_STATIC_EARTH)->EnableWindow( bEnable);
   m_PublisherHyperLink.EnableWindow(bEnable);
}

void CConfigurePGSuperDlg::OnLbnSelchangePublishers()
{
   ConfigureWebLink();
}

void CConfigurePGSuperDlg::ConfigureWebLink()
{
   CWnd* pWeb = (CWnd*)GetDlgItem(IDC_PUBLISHER_HYPERLINK);

   CButton* pGen = (CButton*)GetDlgItem(IDC_GENERIC);
   if (BST_CHECKED!=pGen->GetCheck())
   {
      const CPGSuperCatalogServer* pserver = m_Servers.GetServer(m_CurrentServer);
      if(pserver!=NULL)
      {
         CListBox* pCB = (CListBox*)GetDlgItem(IDC_PUBLISHERS);
         int idx = pCB->GetCurSel();
         if ( idx != CB_ERR )
         {
            CString strName;
            pCB->GetText(idx,strName);

            CString url;
            
            try
            {
               url = pserver->GetWebLink(strName);
            }
            catch(CCatalogServerException exp)
            {
               ATLASSERT(0); // not sure why this would happen this late in the game.
               CString msg = exp.GetErrorMessage();
            }

            if (!url.IsEmpty())
            {
               pWeb->EnableWindow(TRUE);
               m_PublisherHyperLink.SetURL(url);
            }
            else
            {
               pWeb->EnableWindow(FALSE);
               m_PublisherHyperLink.SetURL(CString(_T("No web link defined for this publisher.")));
            }
         }
      }
   }
   else
   {
      pWeb->EnableWindow(FALSE);
      m_PublisherHyperLink.SetURL(CString(_T("No web link defined for this publisher.")));
   }
}
