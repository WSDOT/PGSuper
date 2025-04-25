///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// RefinedAnalysisOptionsDlg.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "RefinedAnalysisOptionsDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CRefinedAnalysisOptionsDlg dialog


CRefinedAnalysisOptionsDlg::CRefinedAnalysisOptionsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CRefinedAnalysisOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRefinedAnalysisOptionsDlg)
	m_Choice = lldfIgnore;
	//}}AFX_DATA_INIT
}


void CRefinedAnalysisOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRefinedAnalysisOptionsDlg)
	DDX_Radio(pDX, IDC_RADIO_INPUT, (int&)m_Choice);
	//}}AFX_DATA_MAP
   DDX_Text(pDX,IDC_DESCRIPTION,m_strDescription);
}


BEGIN_MESSAGE_MAP(CRefinedAnalysisOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(CRefinedAnalysisOptionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRefinedAnalysisOptionsDlg message handlers

BOOL CRefinedAnalysisOptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
