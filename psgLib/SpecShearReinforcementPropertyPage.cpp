// SpecShearReinforcementPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecShearReinforcementPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecShearReinforcementPropertyPage

IMPLEMENT_DYNAMIC(CSpecShearReinforcementPropertyPage, CMFCPropertyPage)

CSpecShearReinforcementPropertyPage::CSpecShearReinforcementPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_SHEAR_REINFORCEMENT)
{
}

CSpecShearReinforcementPropertyPage::~CSpecShearReinforcementPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecShearReinforcementPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecShearReinforcementPropertyPage message handlers

void CSpecShearReinforcementPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeShearReinforcementData(pDX);
}


BOOL CSpecShearReinforcementPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecShearReinforcementPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_SPACING_GROUP)->SetWindowText(CString(_T("Maximum Spacing of Transverse Reinforcement (LRFD ")) + pDad->LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) + _T(")"));
   GetDlgItem(IDC_SLTSPACING)->SetWindowText(CString(_T("LRFD Eq ")) + pDad->LrfdCw8th(_T("5.8.2.7-1"), _T("5.7.2.6-1")));
   GetDlgItem(IDC_SGTSPACING)->SetWindowText(CString(_T("LRFD Eq ")) + pDad->LrfdCw8th(_T("5.8.2.7-2"), _T("5.7.2.6-2")));

   if (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= pDad->GetSpecVersion())
   {
      GetDlgItem(IDC_SPACING_LABEL_1)->SetWindowText(_T("If vu <  0.125f'c, then: Smax ="));
      GetDlgItem(IDC_SPACING_LABEL_2)->SetWindowText(_T("If vu >= 0.125f'c, then: Smax ="));
   }
   else
   {
      GetDlgItem(IDC_SPACING_LABEL_1)->SetWindowText(_T("Vu < 0.1*f'c*bv*dv Smax = "));
      GetDlgItem(IDC_SPACING_LABEL_2)->SetWindowText(_T("Vu >= 0.1*f'c*bv*dv Smax = "));
   }

   return __super::OnSetActive();
}

void CSpecShearReinforcementPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
