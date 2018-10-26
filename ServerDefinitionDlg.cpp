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

// ServerDefinitionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "ServerDefinitionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg dialog


CServerDefinitionDlg::CServerDefinitionDlg(const CPGSuperCatalogServers& servers,CWnd* pParent /*=NULL*/)
	: CDialog(CServerDefinitionDlg::IDD, pParent), m_Servers(servers)
{
	//{{AFX_DATA_INIT(CServerDefinitionDlg)
	m_ServerName = _T("");
	m_ServerAddress = _T("");
	//}}AFX_DATA_INIT
}


void CServerDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerDefinitionDlg)
	DDX_Text(pDX, IDC_NAME, m_ServerName);
	DDX_Text(pDX, IDC_URL, m_ServerAddress);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      pDX->PrepareEditCtrl(IDC_NAME);
      if ( m_OriginalName != m_ServerName && m_Servers.IsServerDefined(m_ServerName) )
      {
         CString strMsg;
         strMsg.Format("A server with the name %s is already defined. Please use a different name",m_ServerName);
         AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }

      pDX->PrepareEditCtrl(IDC_URL);
      if ( m_ServerAddress.GetLength() == 0 )
      {
         AfxMessageBox("Please enter a server name",MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CServerDefinitionDlg, CDialog)
	//{{AFX_MSG_MAP(CServerDefinitionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg message handlers

BOOL CServerDefinitionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   m_OriginalName = m_ServerName;
   
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
