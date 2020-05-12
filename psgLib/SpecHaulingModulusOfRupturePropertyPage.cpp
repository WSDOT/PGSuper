// SpecHaulingModulusOfRupturePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecHaulingModulusOfRupturePropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>

// CSpecHaulingModulusOfRupturePropertyPage

IMPLEMENT_DYNAMIC(CSpecHaulingModulusOfRupturePropertyPage, CMFCPropertyPage)

CSpecHaulingModulusOfRupturePropertyPage::CSpecHaulingModulusOfRupturePropertyPage() :
   CMFCPropertyPage(IDD_SPEC_HAULING_WSDOT_MOR)
{
}

CSpecHaulingModulusOfRupturePropertyPage::~CSpecHaulingModulusOfRupturePropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecHaulingModulusOfRupturePropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecHaulingModulusOfRupturePropertyPage message handlers

void CSpecHaulingModulusOfRupturePropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeHaulingModulusOfRuptureData(pDX);
}


BOOL CSpecHaulingModulusOfRupturePropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecHaulingModulusOfRupturePropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
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

   return __super::OnSetActive();
}

void CSpecHaulingModulusOfRupturePropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
