///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// ProjectPropertiesDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ProjectPropertiesDlg.h"

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectPropertiesDlg dialog


CProjectPropertiesDlg::CProjectPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProjectPropertiesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProjectPropertiesDlg)
	//}}AFX_DATA_INIT
	m_bShowProjectProperties = false;
}


void CProjectPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProjectPropertiesDlg)
	//}}AFX_DATA_MAP
	DDX_String(pDX, IDC_BRIDGE, m_Bridge);
	DDX_String(pDX, IDC_BRIDGEID, m_BridgeID);
	DDX_String(pDX, IDC_COMMENTS, m_Comments);
	DDX_String(pDX, IDC_COMPANY, m_Company);
	DDX_String(pDX, IDC_ENGINEER, m_Engineer);
	DDX_String(pDX, IDC_JOBNUMBER, m_JobNumber);
	DDX_Check_Bool(pDX, IDC_SHOW_DIALOG, m_bShowProjectProperties);
}


BEGIN_MESSAGE_MAP(CProjectPropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CProjectPropertiesDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectPropertiesDlg message handlers

void CProjectPropertiesDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_PROPERTIES );
}
