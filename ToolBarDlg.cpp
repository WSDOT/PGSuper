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

// ToolBarDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "ToolBarDlg.h"
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolBarDlg dialog


CToolBarDlg::CToolBarDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CToolBarDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CToolBarDlg)
	m_bShowToolTips = FALSE;
	//}}AFX_DATA_INIT
}


void CToolBarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CToolBarDlg)
	DDX_Control(pDX, IDC_LIST, m_List);
	DDX_Check(pDX, IDC_TOOLTIPS, m_bShowToolTips);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CToolBarDlg, CDialog)
	//{{AFX_MSG_MAP(CToolBarDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarDlg message handlers

BOOL CToolBarDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

   // Load up the list box with toolbar names and select the check boxes
   // for the visible toolbars.
   CHECK( m_ToolBarResId.size() == m_ToolBarStates.size() );
   std::vector<UINT>::iterator tb_iter    = m_ToolBarResId.begin();
   std::vector<BOOL>::iterator state_iter = m_ToolBarStates.begin();

   for ( ; tb_iter < m_ToolBarResId.end(); tb_iter++, state_iter++ )
   {
      UINT nID   = *tb_iter;
      BOOL bShow = *state_iter;

      CString strName( (LPCSTR)nID );
      UINT idx = m_List.AddString( strName );
      m_List.SetCheck( idx, bShow ? 1 : 0 );
   }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CToolBarDlg::OnOK() 
{
	// TODO: Add extra validation here

   // Capture the check box states
   m_ToolBarStates.clear();
   UINT cList = m_List.GetCount();
   for ( UINT i = 0; i < cList; i++ )
   {
      BOOL bShow = (m_List.GetCheck(i) == 0 ? FALSE : TRUE);
      m_ToolBarStates.push_back(bShow);
   }

   CHECK( m_ToolBarResId.size() == m_ToolBarStates.size() );
	
	CDialog::OnOK();
}

void CToolBarDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath , HH_HELP_CONTEXT, IDH_DIALOG_TOOLBARS );
}
