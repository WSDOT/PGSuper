// SpecLiftingAnalysisPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiftingAnalysisPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <Stability/StabilityTypes.h>

// CSpecLiftingAnalysisPropertyPage

IMPLEMENT_DYNAMIC(CSpecLiftingAnalysisPropertyPage, CMFCPropertyPage)

CSpecLiftingAnalysisPropertyPage::CSpecLiftingAnalysisPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIFTING_ANALYSIS)
{
}

CSpecLiftingAnalysisPropertyPage::~CSpecLiftingAnalysisPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiftingAnalysisPropertyPage, CMFCPropertyPage)
   ON_CBN_SELCHANGE(IDC_WIND_TYPE, OnCbnSelchangeWindType)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiftingAnalysisPropertyPage message handlers

void CSpecLiftingAnalysisPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiftingAnalysisData(pDX);
}


BOOL CSpecLiftingAnalysisPropertyPage::OnInitDialog()
{
   CComboBox* pcbWind = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   pcbWind->SetItemData(pcbWind->AddString(_T("Pressure")), (DWORD_PTR)WBFL::Stability::WindLoadType::Pressure);
   pcbWind->SetItemData(pcbWind->AddString(_T("Speed")), (DWORD_PTR)WBFL::Stability::WindLoadType::Speed);

   __super::OnInitDialog();

   OnCbnSelchangeWindType();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiftingAnalysisPropertyPage::OnCbnSelchangeWindType()
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

void CSpecLiftingAnalysisPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
