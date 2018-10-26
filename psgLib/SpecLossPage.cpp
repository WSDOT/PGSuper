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

// SpecLossPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecLossPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLossPage property page

IMPLEMENT_DYNCREATE(CSpecLossPage, CPropertyPage)

CSpecLossPage::CSpecLossPage() : CPropertyPage(CSpecLossPage::IDD,IDS_SPEC_LOSS)
{
	//{{AFX_DATA_INIT(CSpecLossPage)
	//}}AFX_DATA_INIT
}

CSpecLossPage::~CSpecLossPage()
{
}

void CSpecLossPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecLossPage)
	//}}AFX_DATA_MAP
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLossData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecLossPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLossPage)
	ON_CBN_SELCHANGE(IDC_LOSS_METHOD, OnLossMethodChanged)
	ON_CBN_SELCHANGE(IDC_SHIPPING_LOSS_METHOD, OnShippingLossMethodChanged)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLossPage message handlers

void CSpecLossPage::OnLossMethodChanged() 
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   int method = pBox->GetCurSel();

   if ( 0 <= method && method < 6 )
   {
      EnableShippingLosses(m_SpecVersion <= lrfdVersionMgr::ThirdEdition2004 || method == 3 ? TRUE : FALSE);
      EnableRefinedShippingTime(lrfdVersionMgr::ThirdEdition2004 < m_SpecVersion && (method == 0 || method == 1 || method == 2) ? TRUE : FALSE);
      EnableApproximateShippingTime(lrfdVersionMgr::ThirdEdition2004 < m_SpecVersion && (method == 4 || method == 6) ? TRUE : FALSE);
      EnableTxDOT2013(method==3);
      EnableTimeDependentModel(FALSE);

      BOOL bEnable = lrfdVersionMgr::ThirdEdition2004 < m_SpecVersion && (method == 0 || method == 1 || method == 3 || method == 4) ? TRUE : FALSE;
      CSpecMainSheet* pParent = (CSpecMainSheet*)GetParent();
      bEnable = pParent->m_Entry.GetSectionPropertyMode() == pgsTypes::spmTransformed ? FALSE : bEnable;

      EnableElasticGains(bEnable);

      bEnable = (0 <= method && method < 4) ? TRUE : FALSE;
      EnableRelaxation(bEnable);
   }
   else
   {
      // time step
      EnableShippingLosses(FALSE);
      EnableRefinedShippingTime(FALSE);
      EnableApproximateShippingTime(FALSE);
      EnableTxDOT2013(FALSE);
      EnableTimeDependentModel(TRUE);
      EnableElasticGains(FALSE);
      EnableRelaxation(FALSE);
   }
}

#define ENABLE_WINDOW(x) pWnd = GetDlgItem(x); pWnd->EnableWindow(bEnable); pWnd->ShowWindow(bEnable ? SW_SHOW : SW_HIDE)
void CSpecLossPage::EnableShippingLosses(BOOL bEnable)
{
   CWnd* pWnd;

   ENABLE_WINDOW(IDC_SHIPPING_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING);
   ENABLE_WINDOW(IDC_SHIPPING_TAG);

   ENABLE_WINDOW(IDC_SHIPPING_LOSS_METHOD);
}

void CSpecLossPage::EnableTimeDependentModel(BOOL bEnable)
{
   CWnd* pWnd;

   ENABLE_WINDOW(IDC_TIME_DEPENDENT_MODEL);
}

void CSpecLossPage::EnableRefinedShippingTime(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_SHIPPING_TIME_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING_TIME);
   ENABLE_WINDOW(IDC_SHIPPING_TIME_TAG);
}

void CSpecLossPage::EnableApproximateShippingTime(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_APPROXIMATE_SHIPPING_TIME_NOTE);
}

void CSpecLossPage::EnableTxDOT2013(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_FCPG_STATIC);
   ENABLE_WINDOW(IDC_FCPG_COMBO);
}

void CSpecLossPage::EnableRelaxation(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_RELAXATION_LOSS_METHOD_LABEL);
   ENABLE_WINDOW(IDC_RELAXATION_LOSS_METHOD);
}

void CSpecLossPage::EnableElasticGains(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_ELASTIC_GAINS_GROUP);

   ENABLE_WINDOW(IDC_EG_LABEL);

   ENABLE_WINDOW(IDC_EG_SLAB);
   ENABLE_WINDOW(IDC_EG_SLAB_UNIT);
   ENABLE_WINDOW(IDC_EG_SLAB_LABEL);

   ENABLE_WINDOW(IDC_EG_SLABPAD);
   ENABLE_WINDOW(IDC_EG_SLABPAD_UNIT);
   ENABLE_WINDOW(IDC_EG_SLABPAD_LABEL);

   ENABLE_WINDOW(IDC_EG_DIAPHRAGM);
   ENABLE_WINDOW(IDC_EG_DIAPHRAGM_UNIT);
   ENABLE_WINDOW(IDC_EG_DIAPHRAGM_LABEL);

   ENABLE_WINDOW(IDC_EG_DC_BS2);
   ENABLE_WINDOW(IDC_EG_DC_BS2_UNIT);
   ENABLE_WINDOW(IDC_EG_DC_BS2_LABEL);

   ENABLE_WINDOW(IDC_EG_DW_BS2);
   ENABLE_WINDOW(IDC_EG_DW_BS2_UNIT);
   ENABLE_WINDOW(IDC_EG_DW_BS2_LABEL);

   ENABLE_WINDOW(IDC_EG_DC_BS3);
   ENABLE_WINDOW(IDC_EG_DC_BS3_UNIT);
   ENABLE_WINDOW(IDC_EG_DC_BS3_LABEL);

   ENABLE_WINDOW(IDC_EG_DW_BS3);
   ENABLE_WINDOW(IDC_EG_DW_BS3_UNIT);
   ENABLE_WINDOW(IDC_EG_DW_BS3_LABEL);

   ENABLE_WINDOW(IDC_EG_RAILING);
   ENABLE_WINDOW(IDC_EG_RAILING_UNIT);
   ENABLE_WINDOW(IDC_EG_RAILING_LABEL);

   ENABLE_WINDOW(IDC_EG_OVERLAY);
   ENABLE_WINDOW(IDC_EG_OVERLAY_UNIT);
   ENABLE_WINDOW(IDC_EG_OVERLAY_LABEL);

   ENABLE_WINDOW(IDC_EG_SHRINKAGE);
   ENABLE_WINDOW(IDC_EG_SHRINKAGE_UNIT);
   ENABLE_WINDOW(IDC_EG_SHRINKAGE_LABEL);

   ENABLE_WINDOW(IDC_EG_LIVELOAD);
   ENABLE_WINDOW(IDC_EG_LIVELOAD_UNIT);
   ENABLE_WINDOW(IDC_EG_LIVELOAD_LABEL);
}

BOOL CSpecLossPage::OnInitDialog() 
{
   // Fill up combo box for loss methods
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   pBox->AddString(_T("Refined Estimate per LRFD 5.9.5.4"));
   pBox->AddString(_T("Refined Estimate per WSDOT Bridge Design Manual"));
   pBox->AddString(_T("Refined Estimate per TxDOT Bridge Design Manual"));
   pBox->AddString(_T("Refined Estimate per TxDOT Research Report 0-6374-2"));
   pBox->AddString(_T("Approximate Lump Sum per LRFD 5.9.5.3"));
   pBox->AddString(_T("Approximate Lump Sum per WSDOT Bridge Design Manual"));
   pBox->AddString(_T("Time-Step Method"));
   pBox->SetCurSel(0);

   pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   pBox->AddString(_T("Use a lump sum loss for shipping"));
   pBox->AddString(_T("Use a percentage of the final losses for shipping"));
   pBox->SetCurSel(0);

   pBox = (CComboBox*)GetDlgItem(IDC_RELAXATION_LOSS_METHOD);
   pBox->AddString(_T("LRFD Equation 5.9.5.4.2c-1"));  // simplified
   pBox->AddString(_T("LRFD Equation C5.9.5.4.2c-1")); // refined
   pBox->AddString(_T("1.2 KSI (LRFD 5.9.5.4.2c)"));   // lump sum

   pBox = (CComboBox*)GetDlgItem(IDC_FCPG_COMBO);
   pBox->AddString(_T("Assumption that strand stress at release is 0.7 fpu"));
   pBox->AddString(_T("Iterative method described in LRFD C5.9.5.2.3a"));
   pBox->AddString(_T("Assumption of 0.7 fpu unless special conditions"));

   pBox = (CComboBox*)GetDlgItem(IDC_TIME_DEPENDENT_MODEL);
   pBox->SetItemData(pBox->AddString(_T("AASHTO LRFD")), TDM_AASHTO);
   pBox->SetItemData(pBox->AddString(_T("ACI 209R-92")), TDM_ACI209);
   pBox->SetItemData(pBox->AddString(_T("CEB-FIP 1990")),TDM_CEBFIP);

   CPropertyPage::OnInitDialog();


   OnLossMethodChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecLossPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_LOSSES );
   return TRUE;
}

BOOL CSpecLossPage::IsFractionalShippingLoss()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   int curSel = pBox->GetCurSel();

   return curSel == 1 ? TRUE : FALSE;
}

void CSpecLossPage::OnShippingLossMethodChanged() 
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CWnd* pTag = GetDlgItem(IDC_SHIPPING_TAG);

	if ( IsFractionalShippingLoss() )
   {
      pTag->SetWindowText(_T("%"));
   }
   else
   {
      pTag->SetWindowText(pDisplayUnits->Stress.UnitOfMeasure.UnitTag().c_str());
   }
}

BOOL CSpecLossPage::OnSetActive() 
{
   // if this is third edition or earlier, enable the shipping loss controls
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   m_SpecVersion = pDad->m_Entry.GetSpecificationType();

   OnLossMethodChanged();

   return CPropertyPage::OnSetActive();
}
