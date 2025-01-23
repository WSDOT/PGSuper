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

//
// WsdotHaulingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WsdotHaulingDlg.h"
#include "SpecHaulingErectionPage.h"
#include "SpecMainSheet.h"
#include <Stability/StabilityTypes.h>
#include <EAF\EAFApp.h>
#include <psgLib/SpecificationCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CWsdotHaulingDlg dialog

IMPLEMENT_DYNAMIC(CWsdotHaulingDlg, CDialog)

CWsdotHaulingDlg::CWsdotHaulingDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CWsdotHaulingDlg::IDD, pParent)
{

}

CWsdotHaulingDlg::~CWsdotHaulingDlg()
{
}

BOOL CWsdotHaulingDlg::OnInitDialog()
{
   CComboBox* pcbWind = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   pcbWind->SetItemData(pcbWind->AddString(_T("Pressure")),(DWORD_PTR)WBFL::Stability::WindLoadType::Pressure);
   pcbWind->SetItemData(pcbWind->AddString(_T("Speed")),   (DWORD_PTR)WBFL::Stability::WindLoadType::Speed);

   CComboBox* pcbCF = (CComboBox*)GetDlgItem(IDC_CF_TYPE);
   pcbCF->SetItemData(pcbCF->AddString(_T("Adverse")),  (DWORD_PTR)WBFL::Stability::CFType::Adverse);
   pcbCF->SetItemData(pcbCF->AddString(_T("Favorable")),(DWORD_PTR)WBFL::Stability::CFType::Favorable);

   CComboBox* pcbImpactUsage = (CComboBox*)GetDlgItem(IDC_IMPACT_USAGE);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Normal Crown Slope and Max. Superelevation Cases")),(DWORD_PTR)WBFL::Stability::HaulingImpact::Both);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Normal Crown Slope Case Only")),(DWORD_PTR)WBFL::Stability::HaulingImpact::NormalCrown);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Max. Superelevation Case Only")),(DWORD_PTR)WBFL::Stability::HaulingImpact::MaxSuper);

   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CWsdotHaulingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWsdotHaulingDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX_CROWN, OnCheckHaulingTensMaxCrown)
	ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX_SUPER, OnCheckHaulingTensMaxSuper)
   ON_CBN_SELCHANGE(IDC_WIND_TYPE, &CWsdotHaulingDlg::OnCbnSelchangeWindType)
END_MESSAGE_MAP()

// CWsdotHaulingDlg message handlers

void CWsdotHaulingDlg::OnCheckHaulingTensMaxCrown()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENSION_MAX_CROWN);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_CROWN);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_UNIT_CROWN);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CWsdotHaulingDlg::OnCheckHaulingTensMaxSuper()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENSION_MAX_SUPER);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_SUPER);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_UNIT_SUPER);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CWsdotHaulingDlg::OnSetActive()
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)(GetParent()->GetParent());
   if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= pDad->m_Entry.GetSpecificationCriteria().GetEdition())
   {
      GetDlgItem(IDC_SLWC_FR_TXT)->SetWindowText(_T("Lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_TXT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_HIDE);
   }
   else
   {
      GetDlgItem(IDC_SLWC_FR_TXT)->SetWindowText(_T("Sand lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_TXT)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_SHOW);
   }

   OnCheckHaulingTensMaxCrown();
   OnCheckHaulingTensMaxSuper();
}

void CWsdotHaulingDlg::OnCbnSelchangeWindType()
{
   // TODO: Add your control notification handler code here
   CComboBox* pcbWindType = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   int curSel = pcbWindType->GetCurSel();
   WBFL::Stability::WindLoadType windType = (WBFL::Stability::WindLoadType)pcbWindType->GetItemData(curSel);
   CDataExchange dx(this,false);
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();
   if ( windType == WBFL::Stability::WindLoadType::Speed )
   {
      DDX_Tag(&dx,IDC_WIND_LOAD_UNIT,pDispUnits->Velocity);
   }
   else
   {
      DDX_Tag(&dx,IDC_WIND_LOAD_UNIT,pDispUnits->WindPressure);
   }
}
