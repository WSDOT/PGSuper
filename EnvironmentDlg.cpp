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

// EnvironmentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EnvironmentDlg.h"
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnvironmentDlg dialog


CEnvironmentDlg::CEnvironmentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnvironmentDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnvironmentDlg)
	m_Exposure = -1;
	m_RelHumidity = 0.0;
	//}}AFX_DATA_INIT
}


void CEnvironmentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnvironmentDlg)
	DDX_Radio(pDX, IDC_EXPOSURE, m_Exposure);
	DDX_Text(pDX, IDC_RELHUMIDITY, m_RelHumidity);
	DDV_MinMaxDouble(pDX, m_RelHumidity, 0., 100.);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEnvironmentDlg, CDialog)
	//{{AFX_MSG_MAP(CEnvironmentDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnvironmentDlg message handlers

void CEnvironmentDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_ENVIRONMENT );
}
