///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// StatusMessageDialog.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "StatusMessageDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatusMessageDialog dialog
CStatusMessageDialog::CStatusMessageDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CStatusMessageDialog::IDD, pParent),
   m_HelpID(0), m_IsSevere(false)
{
	//{{AFX_DATA_INIT(CStatusMessageDialog)
	m_Message = _T("");
	//}}AFX_DATA_INIT
}


void CStatusMessageDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatusMessageDialog)
	DDX_Control(pDX, IDCANCEL, m_CancelBtn);
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatusMessageDialog, CDialog)
	//{{AFX_MSG_MAP(CStatusMessageDialog)
	ON_BN_CLICKED(IDHELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusMessageDialog message handlers
#include "HtmlHelp\HelpTopics.hh"

void CStatusMessageDialog::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, m_HelpID );
}

BOOL CStatusMessageDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   this->m_CancelBtn.ShowWindow(m_IsSevere ? SW_HIDE:SW_SHOW);
	GetDlgItem(IDHELP)->ShowWindow(m_HelpID==0 ? SW_HIDE:SW_SHOW);

   // string for group box
   CString groupstr = m_IsSevere ? "Error Message":"Warning Message";
	GetDlgItem(IDC_GROUP)->SetWindowText(groupstr);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
