// SpecGeneralSectionPropertiesPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGeneralSectionPropertiesPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecGeneralSectionPropertiesPropertyPage

IMPLEMENT_DYNAMIC(CSpecGeneralSectionPropertiesPropertyPage, CMFCPropertyPage)

CSpecGeneralSectionPropertiesPropertyPage::CSpecGeneralSectionPropertiesPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_GENERAL_SECTION_PROPERTIES)
{
}

CSpecGeneralSectionPropertiesPropertyPage::~CSpecGeneralSectionPropertiesPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGeneralSectionPropertiesPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGeneralSectionPropertiesPropertyPage message handlers

void CSpecGeneralSectionPropertiesPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGeneralSectionPropertiesData(pDX);
}


BOOL CSpecGeneralSectionPropertiesPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecGeneralSectionPropertiesPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}


BOOL CSpecGeneralSectionPropertiesPropertyPage::OnSetActive()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EFF_FLANGE_WIDTH);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   pCB->AddString(_T("in accordance with LRFD 4.6.2.6"));

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   WBFL::LRFD::BDSManager::Edition version = pParent->GetSpecVersion();
   if (version < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims)
   {
      pCB->AddString(_T("using the tributary width"));
   }

   if (pCB->SetCurSel(curSel) == CB_ERR)
   {
      pCB->SetCurSel(0);
   }

   return CMFCPropertyPage::OnSetActive();
}
