///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <PgsExt\PgsExtLib.h>
#include "ConfigurePGSuperDlg.h"
#include "CatalogServerDlg.h"
#include <EAF\EAFDocument.h>
#include <BridgeLinkConfiguration.h>
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg dialog


CConfigurePGSuperDlg::CConfigurePGSuperDlg(LPCTSTR lpszAppName,LPCTSTR lpszTemplateExt,BOOL bFirstRun,CWnd* pParent /*=nullptr*/)
	: CPropertyPage(CConfigurePGSuperDlg::IDD/*, pParent*/), m_AppName(lpszAppName), m_TemplateFileExt(lpszTemplateExt)
{
	//{{AFX_DATA_INIT(CConfigurePGSuperDlg)
	m_Publisher = _T("");
	//}}AFX_DATA_INIT

   m_bNetworkError = false;

   m_PageTitle.Format(_T("Configure %s"),m_AppName);

   m_psp.dwFlags |= PSP_USETITLE | PSP_DEFAULT | PSP_HASHELP | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
   m_psp.pszTitle = m_PageTitle;
   m_psp.pszHeaderTitle = m_AppName;
   m_psp.pszHeaderSubTitle = _T("Select a Configuration to estabilish project templates and default settings");
}

CConfigurePGSuperDlg::~CConfigurePGSuperDlg()
{
}

void CConfigurePGSuperDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigurePGSuperDlg)
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
         const CCatalogServer* psrvr = m_Servers.GetServer(m_CurrentServer);
         if (psrvr==nullptr)
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

      //// check if we changed publishers (don't care if Default)
      //if ( !m_bUpdateCache && m_SharedResourceType != srtDefault)
      //{
      //   bool bchanged = m_bFirstRun!=FALSE;
      //   bchanged |= m_OriginalSharedResourceType != m_SharedResourceType;

      //   if ( m_SharedResourceType != srtLocal)
      //   {
      //      bchanged |= m_OriginalPublisher != m_Publisher;
      //   }

      //   bchanged |= m_OriginalServer != m_CurrentServer;

      //   if (m_OriginalServer == m_CurrentServer)
      //   {
      //      // use pointer comparison 
      //      const CCatalogServer* psvr = m_Servers.GetServer(m_CurrentServer); 
      //      bchanged |= psvr != m_OriginalServerPtr;
      //   }

      //   if (bchanged)
      //   {
      //      // Prompt user to update server
      //      int st = ::AfxMessageBox(_T("A change has been made to the server configuration. You must run an update before the configuration can be saved. Do you want to run update now?"),MB_YESNO);
      //      if (st==IDYES)
      //      {
      //         m_bUpdateCache = true;
      //      }
      //      else
      //      {
      //         pDX->Fail();
      //      }
      //   }
      //}
   }
}


BEGIN_MESSAGE_MAP(CConfigurePGSuperDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CConfigurePGSuperDlg)
   ON_BN_CLICKED(IDC_ADD,OnAddCatalogServer)
	ON_CBN_SELCHANGE(IDC_SERVERS, OnServerChanged)
	ON_BN_CLICKED(IDC_GENERIC, OnGeneric)
	ON_BN_CLICKED(IDC_DOWNLOAD, OnDownload)
   ON_COMMAND(ID_HELP,OnHelp)
	//}}AFX_MSG_MAP
   ON_LBN_SELCHANGE(IDC_PUBLISHERS, &CConfigurePGSuperDlg::OnLbnSelchangePublishers)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg message handlers

void CConfigurePGSuperDlg::OnHelp() 
{
   EAFHelp( m_AppName, IDH_CONFIGURE_PGSUPER );
}

LRESULT CConfigurePGSuperDlg::OnWizardNext()
{
   if ( !UpdateData() )
      return -1;

   AFX_MANAGE_STATE(AfxGetAppModuleState());
   IBridgeLinkConfigurationParent* pParent = dynamic_cast<IBridgeLinkConfigurationParent*>(GetParent());
   pParent->GetNextPage();
   return CPropertyPage::OnWizardNext();
}

LRESULT CConfigurePGSuperDlg::OnWizardBack()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   IBridgeLinkConfigurationParent* pParent = dynamic_cast<IBridgeLinkConfigurationParent*>(GetParent());
   pParent->GetBackPage();
   return CPropertyPage::OnWizardBack();
}

BOOL CConfigurePGSuperDlg::OnInitDialog() 
{
   // no cache update until we're told
   //m_bUpdateCache = false;

   m_Method = m_SharedResourceType==srtDefault ? 0 : 1;

   UpdateFrequencyList();

   // must be called before ServerList();
   CPropertyPage::OnInitDialog();

   ServerList();

   CWnd* pWnd = GetDlgItem(IDC_EDIT);
   CString strMsg;
   strMsg.Format(_T("%s doesn't have any default girders, design criteria, or other settings. All of the \"default\" information is stored in configurations. Select a configuration option."),m_AppName);
   pWnd->SetWindowText(strMsg);

   OnMethod();
   OnServerChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigurePGSuperDlg::UpdateFrequencyList()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_UPDATE_FREQUENCY);

   int idx = pCB->AddString(_T("Never"));
   pCB->SetItemData(idx,Never);

   idx = pCB->AddString(_T("Always"));
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
   {
      pCB->GetLBText(idx,strCurrAddress);
   }

   pCB->ResetContent();


   CollectionIndexType nServers = m_Servers.GetServerCount();
   for ( CollectionIndexType i = 0; i < nServers; i++ )
   {
      CString strName, strAddress;
      const CCatalogServer* pserver = m_Servers.GetServer(i);

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
   CCatalogServerDlg dlg(m_TemplateFileExt,m_AppName);

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SERVERS);
   int idx = pCB->GetCurSel();
   if ( idx != CB_ERR )
   {
      CString strCurrentServer;
      pCB->GetLBText(idx,strCurrentServer);
      dlg.m_Server = strCurrentServer;
   }

   dlg.m_Servers = m_Servers;
   if ( dlg.DoModal() == IDOK )
   {
      m_Servers = dlg.m_Servers;
      ServerList();

      idx = pCB->FindStringExact(0,dlg.m_Server);
      if ( idx != CB_ERR )
      {
         pCB->SetCurSel(idx);
      }
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

      const CCatalogServer* pserver = m_Servers.GetServer(strName);
      ATLASSERT(pserver);
      m_CurrentServer = strName;

      PublisherList();

      ConfigureWebLink();
   }
}

void CConfigurePGSuperDlg::PublisherList() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
      wndProgress->Show(CComBSTR("Fetching configuration names from server..."),GetSafeHwnd());

      // fill up the list box
      const CCatalogServer* pserver = m_Servers.GetServer(m_CurrentServer);
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
               {
                  pLB->SetCurSel(0);
               }
            }

            pLB->EnableWindow(TRUE);
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
		    pLB->AddString(_T(""));
		    pLB->AddString(_T("The most recently downloaded settings"));
		    pLB->AddString(_T("will be used"));
		    pLB->EnableWindow(FALSE);
      }
   }
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
   if (BST_CHECKED != pGen->GetCheck())
   {
      const CCatalogServer* pServer = m_Servers.GetServer(m_CurrentServer);
      if(pServer != nullptr)
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
               url = pServer->GetWebLink(strName);
            }
            catch(CCatalogServerException exp)
            {
               ATLASSERT(false); // not sure why this would happen this late in the game.
               CString msg = exp.GetErrorMessage();
            }

            m_PublisherHyperLink.SetWindowText(_T("More about this configuration..."));
            m_PublisherHyperLink.SetURL(url);
            pWeb->EnableWindow(TRUE);
            if (url.IsEmpty())
            {
               pWeb->EnableWindow(FALSE);
               m_PublisherHyperLink.SetWindowText(CString(_T("No web link defined for this publisher.")));
            }
         }
      }
   }
   else
   {
      pWeb->EnableWindow(FALSE);
      m_PublisherHyperLink.SetWindowText(CString(_T("No web link defined for this publisher.")));
   }

   m_PublisherHyperLink.SizeToContent();
}
