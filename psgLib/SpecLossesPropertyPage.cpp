///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// SpecLossesPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLossesPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/PrestressLossCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/HaulingCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLossesPropertyPage property page

IMPLEMENT_DYNCREATE(CSpecLossesPropertyPage, CMFCPropertyPage)

CSpecLossesPropertyPage::CSpecLossesPropertyPage() : 
   CMFCPropertyPage(IDD_SPEC_LOSSES2)
{
	//{{AFX_DATA_INIT(CSpecLossesPropertyPage)
	//}}AFX_DATA_INIT
}

CSpecLossesPropertyPage::~CSpecLossesPropertyPage()
{
}

void CSpecLossesPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CMFCPropertyPage::DoDataExchange(pDX);
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   pDad->ExchangeLossesData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecLossesPropertyPage, CMFCPropertyPage)
	//{{AFX_MSG_MAP(CSpecLossesPropertyPage)
	ON_CBN_SELCHANGE(IDC_LOSS_METHOD, OnLossMethodChanged)
	ON_CBN_SELCHANGE(IDC_SHIPPING_LOSS_METHOD, OnShippingLossMethodChanged)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLossesPropertyPage message handlers

void CSpecLossesPropertyPage::OnLossMethodChanged()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   int method = pBox->GetCurSel();

   if ( 0 <= method && method < 6 )
   {
      EnableShippingLosses(m_SpecVersion <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 || method == 3 ? TRUE : FALSE);
      EnableRefinedShippingTime(WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 < m_SpecVersion && (method == 0 || method == 1 || method == 2) ? TRUE : FALSE);
      EnableApproximateShippingTime(WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 < m_SpecVersion && (method == 4 || method == 6) ? TRUE : FALSE);
      EnableTxDOT2013(method==3);
      EnableTimeDependentModel(FALSE);

      CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
      if (pDad->m_Entry.GetSectionPropertiesCriteria().SectionPropertyMode == pgsTypes::spmGross )
      {
         BOOL bEnable = WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 < m_SpecVersion && (method == 0 || method == 1 || method == 4) ? TRUE : FALSE;
         BOOL bEnableShrk = bEnable && method != 4 ? TRUE : FALSE;
         EnableElasticGains(bEnable, bEnableShrk);
      }
      else
      {
         // no elastic gains for transformed section analysis (they are implicit)
         EnableElasticGains(FALSE,FALSE);
      }

      BOOL bEnable = (0 <= method && method < 3) ? TRUE : FALSE;
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
      EnableElasticGains(FALSE, FALSE);
      EnableRelaxation(FALSE);
   }
}

#define ENABLE_WINDOW(x) pWnd = GetDlgItem(x); pWnd->EnableWindow(bEnable); pWnd->ShowWindow(bEnable ? SW_SHOW : SW_HIDE)
#define ENABLE_WINDOW_EX(x,y) pWnd = GetDlgItem(x); pWnd->EnableWindow(y); pWnd->ShowWindow(y ? SW_SHOW : SW_HIDE)
void CSpecLossesPropertyPage::EnableShippingLosses(BOOL bEnable)
{
   CWnd* pWnd;

   bEnable = bEnable && m_IsShippingEnabled ? TRUE : FALSE;

   ENABLE_WINDOW(IDC_SHIPPING_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING);
   ENABLE_WINDOW(IDC_SHIPPING_TAG);

   ENABLE_WINDOW(IDC_SHIPPING_LOSS_METHOD);
}

void CSpecLossesPropertyPage::EnableTimeDependentModel(BOOL bEnable)
{
   CWnd* pWnd;

   ENABLE_WINDOW(IDC_TIME_DEPENDENT_MODEL);
}

void CSpecLossesPropertyPage::EnableRefinedShippingTime(BOOL bEnable)
{
   CWnd* pWnd;

   bEnable = bEnable && m_IsShippingEnabled ? TRUE : FALSE;

   ENABLE_WINDOW(IDC_SHIPPING_TIME_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING_TIME);
   ENABLE_WINDOW(IDC_SHIPPING_TIME_TAG);
}

void CSpecLossesPropertyPage::EnableApproximateShippingTime(BOOL bEnable)
{
   CWnd* pWnd;

   bEnable = bEnable && m_IsShippingEnabled ? TRUE : FALSE;

   ENABLE_WINDOW(IDC_APPROXIMATE_SHIPPING_TIME_NOTE);
}

void CSpecLossesPropertyPage::EnableTxDOT2013(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_FCPG_STATIC);
   ENABLE_WINDOW(IDC_FCPG_COMBO);
}

void CSpecLossesPropertyPage::EnableRelaxation(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_RELAXATION_LOSS_METHOD_LABEL);
   ENABLE_WINDOW(IDC_RELAXATION_LOSS_METHOD);
}

void CSpecLossesPropertyPage::EnableElasticGains(BOOL bEnable, BOOL bEnableDeckShrinkage)
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

   ENABLE_WINDOW_EX(IDC_EG_SHRINKAGE, bEnableDeckShrinkage);
   ENABLE_WINDOW_EX(IDC_EG_SHRINKAGE_UNIT, bEnableDeckShrinkage);
   ENABLE_WINDOW_EX(IDC_EG_SHRINKAGE_LABEL, bEnableDeckShrinkage);

   ENABLE_WINDOW(IDC_EG_LIVELOAD);
   ENABLE_WINDOW(IDC_EG_LIVELOAD_UNIT);
   ENABLE_WINDOW(IDC_EG_LIVELOAD_LABEL);
}

BOOL CSpecLossesPropertyPage::OnInitDialog()
{
   // Fill up combo box for loss methods
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   pBox->AddString(CString(_T("Refined Estimate per LRFD ")) + CString(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4"))));
   pBox->AddString(_T("Refined Estimate per WSDOT Bridge Design Manual"));
   pBox->AddString(_T("Refined Estimate per TxDOT Bridge Design Manual"));
   pBox->AddString(_T("Refined Estimate per TxDOT Research Report 0-6374-2"));
   pBox->AddString(CString(_T("Approximate Lump Sum per LRFD ")) + CString(WBFL::LRFD::LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3"))));
   pBox->AddString(_T("Approximate Lump Sum per WSDOT Bridge Design Manual"));
   pBox->AddString(_T("Time-Step Method"));
   pBox->SetCurSel(0);

   pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   pBox->AddString(_T("Use a lump sum loss for shipping"));
   pBox->AddString(_T("Use a percentage of the final losses for shipping"));
   pBox->SetCurSel(0);

   pBox = (CComboBox*)GetDlgItem(IDC_RELAXATION_LOSS_METHOD);
   pBox->AddString(CString(_T("LRFD Equation ")) + CString(WBFL::LRFD::LrfdCw8th(_T("5.9.5.4.2c-1"), _T("5.9.3.4.2c-1"))));  // simplified
   pBox->AddString(CString(_T("LRFD Equation ")) + CString(WBFL::LRFD::LrfdCw8th(_T("C5.9.5.4.2c-1"),_T("C5.9.3.4.2c-1")))); // refined
   pBox->AddString(CString(_T("1.2 KSI (LRFD ")) + CString(WBFL::LRFD::LrfdCw8th(_T("C5.9.5.4.2c"),  _T("C5.9.3.4.2c"))));   // lump sum

   pBox = (CComboBox*)GetDlgItem(IDC_FCPG_COMBO);
   pBox->AddString(_T("Assumption that strand stress at release is 0.7 fpu"));
   pBox->AddString(_T("Iterative method described in LRFD ") + CString(WBFL::LRFD::LrfdCw8th(_T("C5.9.5.2.3a"),_T("C5.9.3.2.3a"))));
   pBox->AddString(_T("Assumption of 0.7 fpu unless special conditions"));

   pBox = (CComboBox*)GetDlgItem(IDC_TIME_DEPENDENT_MODEL);
   pBox->SetItemData(pBox->AddString(_T("AASHTO LRFD")), +PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO);
   pBox->SetItemData(pBox->AddString(_T("ACI 209R-92")), +PrestressLossCriteria::TimeDependentConcreteModelType::ACI209);
   pBox->SetItemData(pBox->AddString(_T("CEB-FIP 1990")), +PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP);

   CMFCPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLossesPropertyPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LOSSES );
}

BOOL CSpecLossesPropertyPage::IsFractionalShippingLoss()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   int curSel = pBox->GetCurSel();

   return curSel == 1 ? TRUE : FALSE;
}

void CSpecLossesPropertyPage::OnShippingLossMethodChanged()
{
   CEAFApp* pApp = EAFGetApp();
   const auto* pDisplayUnits = pApp->GetDisplayUnits();

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

BOOL CSpecLossesPropertyPage::OnSetActive()
{
   // if this is third edition or earlier, enable the shipping loss controls
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   m_SpecVersion = pDad->m_Entry.GetSpecificationCriteria().GetEdition();

   m_IsShippingEnabled = pDad->m_Entry.GetHaulingCriteria().bCheck;

   OnLossMethodChanged();

   return CMFCPropertyPage::OnSetActive();
}
