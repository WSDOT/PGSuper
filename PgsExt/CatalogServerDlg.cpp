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

// CatalogServerDlg.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include "CatalogServerDlg.h"
#include "ServerDefinitionDlg.h"
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCatalogServerDlg dialog

CCatalogServerDlg::CCatalogServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CCatalogServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCatalogServerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


CCatalogServerDlg::CCatalogServerDlg(const CString& strExt,const CString& appName,CWnd* pParent /*=nullptr*/)
	: CDialog(CCatalogServerDlg::IDD, pParent),m_TemplateFileExt(strExt),
   m_AppName(appName)
{
	//{{AFX_DATA_INIT(CCatalogServerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCatalogServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCatalogServerDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_LBString(pDX,IDC_SERVERS,m_Server);
}


BEGIN_MESSAGE_MAP(CCatalogServerDlg, CDialog)
	//{{AFX_MSG_MAP(CCatalogServerDlg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_LBN_DBLCLK(IDC_SERVERS, OnDblclkServers)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_HELP, &CCatalogServerDlg::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCatalogServerDlg message handlers

void CCatalogServerDlg::OnAdd() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CServerDefinitionDlg dlg(m_Servers,m_TemplateFileExt,m_AppName);
   if ( dlg.DoModal() == IDOK )
   {
      m_Servers.RemoveServer(dlg.m_ServerName);

      m_Servers.AddServer( dlg.CreateServer() );

      UpdateServerList();
   }

   UpdateButtonState();
}

void CCatalogServerDlg::OnDelete() 
{
   CListBox* pLB = (CListBox*)GetDlgItem(IDC_SERVERS);
   int idx = pLB->GetCurSel();
   if ( idx == LB_ERR )
   {
      return;
   }

   CString strName;
   pLB->GetText(idx,strName);
   m_Servers.RemoveServer(strName);
   UpdateServerList();

   UpdateButtonState();
}

void CCatalogServerDlg::OnEdit() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CListBox* pLB = (CListBox*)GetDlgItem(IDC_SERVERS);
   int idx = pLB->GetCurSel();
   if ( idx == LB_ERR )
   {
      idx = 0;
   }

   CString strName;
   pLB->GetText(idx,strName);
   const CCatalogServer* pserver = m_Servers.GetServer(strName);
	
   CServerDefinitionDlg dlg(m_Servers,pserver,m_TemplateFileExt,m_AppName);

   if ( dlg.DoModal() == IDOK )
   {
      m_Servers.RemoveServer(strName);
      CCatalogServer* psvr = dlg.CreateServer();
      m_Servers.AddServer(psvr);
      UpdateServerList();
   }
}

BOOL CCatalogServerDlg::OnInitDialog() 
{
   UpdateServerList();

   CDialog::OnInitDialog();

   UpdateButtonState();
   
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCatalogServerDlg::UpdateServerList()
{
   CListBox* pLB = (CListBox*)GetDlgItem(IDC_SERVERS);
   int curSel = pLB->GetCurSel();

   pLB->ResetContent();

   CollectionIndexType nServers = m_Servers.GetServerCount();
   for ( CollectionIndexType i = 0; i < nServers; i++ )
   {
      const CCatalogServer* server = m_Servers.GetServer(i);
      pLB->AddString( server->GetServerName() );
   }

   if ( curSel != LB_ERR )
   {
      pLB->SetCurSel(curSel);
   }
   else
   {
      pLB->SetCurSel(0);
   }
}

void CCatalogServerDlg::UpdateButtonState()
{
   CollectionIndexType nServers = m_Servers.GetServerCount();
   if ( nServers <= 1 )
   {
      GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
   }
}

void CCatalogServerDlg::OnDblclkServers() 
{
   OnEdit();	
}

void CCatalogServerDlg::OnHelp()
{
   EAFHelp( m_AppName, IDH_CONFIGURATION_SERVERS );
}
