///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// SpecLimitsPage.cpp : implementation file
//

#include "stdafx.h"
#include "SpecLimitsPage.h"
#include "psgLib\psglib.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/PrestressLossCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLimitsPage property page

IMPLEMENT_DYNCREATE(CSpecLimitsPage, CPropertyPage)

CSpecLimitsPage::CSpecLimitsPage() : CPropertyPage(CSpecLimitsPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecLimitsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecLimitsPage::~CSpecLimitsPage()
{
}

void CSpecLimitsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecLimitsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLimitsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecLimitsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLimitsPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_CHECK_GIRDER_SAG, &CSpecLimitsPage::OnBnClickedCheckGirderSag)
END_MESSAGE_MAP()

BOOL CSpecLimitsPage::OnInitDialog()
{
   CComboBox* pcbSagOptions = (CComboBox*)GetDlgItem(IDC_SAG_OPTIONS);
   int idx = pcbSagOptions->AddString(_T("Upper bound camber"));
   pcbSagOptions->SetItemData(idx,(DWORD_PTR)pgsTypes::SagCamber::UpperBoundCamber);

   idx = pcbSagOptions->AddString(_T("Average camber"));
   pcbSagOptions->SetItemData(idx,(DWORD_PTR)pgsTypes::SagCamber::AverageCamber);

   idx = pcbSagOptions->AddString(_T("Lower bound camber"));
   pcbSagOptions->SetItemData(idx,(DWORD_PTR)pgsTypes::SagCamber::LowerBoundCamber);

   CPropertyPage::OnInitDialog();

   OnBnClickedCheckGirderSag();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CSpecLimitsPage message handlers
void CSpecLimitsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LIMITS );
}

void CSpecLimitsPage::OnBnClickedCheckGirderSag()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = (IsDlgButtonChecked(IDC_CHECK_GIRDER_SAG) == BST_CHECKED) ? TRUE : FALSE;
   GetDlgItem(IDC_SAG_OPTIONS)->EnableWindow(bEnable);
}

BOOL CSpecLimitsPage::OnSetActive() 
{
   // if this is third edition or earlier, enable the shipping loss controls
   CSpecMainSheet* pParent = (CSpecMainSheet*)GetParent();
   if ( pParent->m_Entry.GetPrestressLossCriteria().LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      GetDlgItem(IDC_SAG_OPTIONS_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SAG_OPTIONS)->ShowWindow(SW_HIDE);
   }

   return CPropertyPage::OnSetActive();
}
