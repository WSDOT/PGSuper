// SpecHaulingAnalysisPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecHaulingAnalysisPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <Stability/StabilityTypes.h>

// CSpecHaulingAnalysisPropertyPage

IMPLEMENT_DYNAMIC(CSpecHaulingAnalysisPropertyPage, CMFCPropertyPage)

CSpecHaulingAnalysisPropertyPage::CSpecHaulingAnalysisPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_HAULING_WSDOT_ANALYSIS)
{
}

CSpecHaulingAnalysisPropertyPage::~CSpecHaulingAnalysisPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecHaulingAnalysisPropertyPage, CMFCPropertyPage)
   ON_CBN_SELCHANGE(IDC_WIND_TYPE, OnCbnSelchangeWindType)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecHaulingAnalysisPropertyPage message handlers

void CSpecHaulingAnalysisPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeHaulingAnalysisData(pDX);
}


BOOL CSpecHaulingAnalysisPropertyPage::OnInitDialog()
{
   CComboBox* pcbWind = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   pcbWind->SetItemData(pcbWind->AddString(_T("Pressure")), (DWORD_PTR)WBFL::Stability::WindLoadType::Pressure);
   pcbWind->SetItemData(pcbWind->AddString(_T("Speed")), (DWORD_PTR)WBFL::Stability::WindLoadType::Speed);

   CComboBox* pcbCF = (CComboBox*)GetDlgItem(IDC_CF_TYPE);
   pcbCF->SetItemData(pcbCF->AddString(_T("Adverse")), (DWORD_PTR)WBFL::Stability::CFType::Adverse);
   pcbCF->SetItemData(pcbCF->AddString(_T("Favorable")), (DWORD_PTR)WBFL::Stability::CFType::Favorable);

   CComboBox* pcbImpactUsage = (CComboBox*)GetDlgItem(IDC_IMPACT_USAGE);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Normal Crown Slope and Max. Superelevation Cases")), (DWORD_PTR)WBFL::Stability::HaulingImpact::Both);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Normal Crown Slope Case Only")), (DWORD_PTR)WBFL::Stability::HaulingImpact::NormalCrown);
   pcbImpactUsage->SetItemData(pcbImpactUsage->AddString(_T("Max. Superelevation Case Only")), (DWORD_PTR)WBFL::Stability::HaulingImpact::MaxSuper);

   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingAnalysisPropertyPage::OnCbnSelchangeWindType()
{
   // TODO: Add your control notification handler code here
   CComboBox* pcbWindType = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   int curSel = pcbWindType->GetCurSel();
   auto windType = (WBFL::Stability::WindLoadType)pcbWindType->GetItemData(curSel);
   CDataExchange dx(this, false);
   CEAFApp* pApp = EAFGetApp();
   const auto* pDispUnits = pApp->GetDisplayUnits();
   if (windType == WBFL::Stability::WindLoadType::Speed)
   {
      DDX_Tag(&dx, IDC_WIND_LOAD_UNIT, pDispUnits->Velocity);
   }
   else
   {
      DDX_Tag(&dx, IDC_WIND_LOAD_UNIT, pDispUnits->WindPressure);
   }
}

void CSpecHaulingAnalysisPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
