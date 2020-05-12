// SpecLiveLoadDistributionFactorsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiveLoadDistributionFactorsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>

// CSpecLiveLoadDistributionFactorsPropertyPage

IMPLEMENT_DYNAMIC(CSpecLiveLoadDistributionFactorsPropertyPage, CMFCPropertyPage)

CSpecLiveLoadDistributionFactorsPropertyPage::CSpecLiveLoadDistributionFactorsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIVE_LOAD_DISTRIBUTION_FACTORS)
{
}

CSpecLiveLoadDistributionFactorsPropertyPage::~CSpecLiveLoadDistributionFactorsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiveLoadDistributionFactorsPropertyPage, CMFCPropertyPage)
   ON_CBN_SELCHANGE(IDC_LLDF, OnCbnSelchangeLldf)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiveLoadDistributionFactorsPropertyPage message handlers

void CSpecLiveLoadDistributionFactorsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiveLoadDistributionFactorsData(pDX);
}


BOOL CSpecLiveLoadDistributionFactorsPropertyPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   pCB->AddString(_T("Compute in accordance with LRFD 4.6.2.2"));
   pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual"));
   pCB->AddString(_T("Compute in accordance with TxDOT Bridge Design Manual"));

   __super::OnInitDialog();

   OnCbnSelchangeLldf();


   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecLiveLoadDistributionFactorsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   int nCmdShow = (WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= pDad->m_Entry.GetSpecificationCriteria().GetEdition()) ? SW_SHOW : SW_HIDE;
   GetDlgItem(IDC_RIGID_METHOD)->ShowWindow(nCmdShow);
   return __super::OnSetActive();
}

void CSpecLiveLoadDistributionFactorsPropertyPage::OnCbnSelchangeLldf()
{
   // TODO: Add your control notification handler code here
   CWnd* pOption = GetDlgItem(IDC_RIGID_METHOD);
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   auto curSel = pCB->GetCurSel();
   pOption->ShowWindow(curSel == 0 ? SW_SHOW : SW_HIDE);
   // this option is never used by WSDOT or TxDOT methods
}

void CSpecLiveLoadDistributionFactorsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
