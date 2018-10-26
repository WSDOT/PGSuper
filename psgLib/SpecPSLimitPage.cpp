///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// SpecPSLimitPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecPSLimitPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecPSLimitPage property page

IMPLEMENT_DYNCREATE(CSpecPSLimitPage, CPropertyPage)

CSpecPSLimitPage::CSpecPSLimitPage() : CPropertyPage(CSpecPSLimitPage::IDD,IDS_SPEC_STRAND)
{
	//{{AFX_DATA_INIT(CSpecPSLimitPage)
	//}}AFX_DATA_INIT
}

CSpecPSLimitPage::~CSpecPSLimitPage()
{
}

void CSpecPSLimitPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecPSLimitPage)
	//}}AFX_DATA_MAP
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangePSLimitData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecPSLimitPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecPSLimitPage)
	ON_BN_CLICKED(IDC_CHECK_PS_AT_JACKING, OnChecked)
	ON_BN_CLICKED(IDC_CHECK_PS_BEFORE_TRANSFER, OnChecked)
	ON_BN_CLICKED(IDC_CHECK_PS_AFTER_TRANSFER, OnCheckPsAfterTransfer)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecPSLimitPage message handlers

BOOL CSpecPSLimitPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   OnChecked();
   OnCheckPsAfterTransfer();


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecPSLimitPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_STRAND );
   return TRUE;
}

void CSpecPSLimitPage::OnChecked() 
{
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AT_JACKING),IDC_PS_AT_JACKING_SR,IDC_PS_AT_JACKING_LR);
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_BEFORE_TRANSFER),IDC_PS_BEFORE_TRANSFER_SR,IDC_PS_BEFORE_TRANSFER_LR);
}

void CSpecPSLimitPage::EnableControls(BOOL bEnable,UINT nSR,UINT nLR)
{
   GetDlgItem(nSR)->EnableWindow(bEnable);
   GetDlgItem(nLR)->EnableWindow(bEnable);
}

void CSpecPSLimitPage::OnCheckPsAfterTransfer() 
{
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AFTER_TRANSFER),IDC_PS_AFTER_TRANSFER_SR,IDC_PS_AFTER_TRANSFER_LR);
}
