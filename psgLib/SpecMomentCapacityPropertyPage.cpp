// SpecMomentCapacityPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecMomentCapacityPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>

// CSpecMomentCapacityPropertyPage

IMPLEMENT_DYNAMIC(CSpecMomentCapacityPropertyPage, CMFCPropertyPage)

CSpecMomentCapacityPropertyPage::CSpecMomentCapacityPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_MOMENT_CAPACITY)
{
}

CSpecMomentCapacityPropertyPage::~CSpecMomentCapacityPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecMomentCapacityPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecMomentCapacityPropertyPage message handlers

void CSpecMomentCapacityPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeMomentCapacityData(pDX);
}


BOOL CSpecMomentCapacityPropertyPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_NEG_MOMENT);
   pCB->SetItemData(pCB->AddString(_T("Include noncomposite moments in Mu")), (DWORD_PTR)true);
   pCB->SetItemData(pCB->AddString(_T("Exclude noncomposite moments from Mu")), (DWORD_PTR)false);
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecMomentCapacityPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   CWnd* wndMoment = GetDlgItem(IDC_MOMENT);
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims < pDad->m_Entry.GetSpecificationCriteria().GetEdition())
   {
      wndMoment->ShowWindow(SW_HIDE);
   }
   else
   {
      wndMoment->ShowWindow(SW_SHOW);
   }

   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= pDad->m_Entry.GetSpecificationCriteria().GetEdition())
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

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_FR_HEADING)->SetWindowText(CString(_T("Modulus of rupture for cracking moment (LRFD 5.4.2.6, ")) + pDad->LrfdCw8th(_T("5.7.3.3.2"), _T("5.6.3.3")) + _T(")"));

   return __super::OnSetActive();
}

void CSpecMomentCapacityPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
