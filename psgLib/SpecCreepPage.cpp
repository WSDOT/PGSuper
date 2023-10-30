///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// SpecCreepPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecCreepPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecCreepPage property page

IMPLEMENT_DYNCREATE(CSpecCreepPage, CPropertyPage)

CSpecCreepPage::CSpecCreepPage() : CPropertyPage(CSpecCreepPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecCreepPage)
	//}}AFX_DATA_INIT
}

CSpecCreepPage::~CSpecCreepPage()
{
}

void CSpecCreepPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecCreepPage)
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeCreepData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecCreepPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecCreepPage)
	ON_BN_CLICKED(IDC_CREEP_METHOD, OnWsdotCreepMethod)
	ON_BN_CLICKED(IDC_CREEP_METHOD2, OnWsdotCreepMethod)
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_CBN_SELCHANGE(IDC_HAUNCH_COMP_CB, &CSpecCreepPage::OnCbnSelchangeHaunchCompCb)
   ON_CBN_SELCHANGE(IDC_HAUNCH_COMP_PROPS_CB, &CSpecCreepPage::OnCbnSelchangeHaunchCompPropCb)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecCreepPage message handlers

void CSpecCreepPage::OnWsdotCreepMethod() 
{
	// TODO: Add your control notification handler code here
   CButton* pButton = (CButton*)GetDlgItem(IDC_CREEP_METHOD2);
   ASSERT(pButton);

   BOOL bEnable;
   if ( pButton->GetCheck() == BST_CHECKED )
      bEnable = TRUE;
   else
      bEnable = FALSE;

   CWnd* pWnd;
   pWnd = GetDlgItem(IDC_CREEP_FACTOR_TITLE);
   pWnd->EnableWindow(bEnable);

   pWnd = GetDlgItem(IDC_CREEP_FACTOR);
   pWnd->EnableWindow(bEnable);
}

BOOL CSpecCreepPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
//   OnWsdotCreepMethod();	

   OnChangeHaunch();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecCreepPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_CREEP );
}

void CSpecCreepPage::OnCbnSelchangeHaunchCompCb()
{
   OnChangeHaunch();
}

void CSpecCreepPage::OnCbnSelchangeHaunchCompPropCb()
{
   OnChangeHaunch();
}

void CSpecCreepPage::OnChangeHaunch()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_HAUNCH_COMP_CB);
   int idxl = pBox->GetCurSel();
   BOOL enablel = idxl==(int)pgsTypes::hlcDetailedAnalysis;

   GetDlgItem(IDC_HAUNCH_FACTOR)->EnableWindow(enablel);
   GetDlgItem(IDC_HAUNCH_FACTOR1)->EnableWindow(enablel);
   GetDlgItem(IDC_HAUNCH_FACTOR2)->EnableWindow(enablel);

   pBox =(CComboBox*)GetDlgItem(IDC_HAUNCH_COMP_PROPS_CB);
   int idxp = pBox->GetCurSel();
   BOOL enablep = enablel || idxp==(int)pgsTypes::hspDetailedDescription;

   GetDlgItem(IDC_HAUNCH_TOLER_STATIC)->EnableWindow(enablep);
   GetDlgItem(IDC_HAUNCH_TOLER_UNIT)->EnableWindow(enablep);
   GetDlgItem(IDC_HAUNCH_TOLER)->EnableWindow(enablep);
}
