// SpecHaulingLimitsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecHaulingLimitsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecHaulingLimitsPropertyPage

IMPLEMENT_DYNAMIC(CSpecHaulingLimitsPropertyPage, CMFCPropertyPage)

CSpecHaulingLimitsPropertyPage::CSpecHaulingLimitsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_HAULING_WSDOT_LIMITS)
{
}

CSpecHaulingLimitsPropertyPage::~CSpecHaulingLimitsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecHaulingLimitsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX_CROWN, OnCheckHaulingTensMaxCrown)
   ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX_SUPER, OnCheckHaulingTensMaxSuper)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecHaulingLimitsPropertyPage message handlers

void CSpecHaulingLimitsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeHaulingLimitsData(pDX);
}


BOOL CSpecHaulingLimitsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingLimitsPropertyPage::OnCheckHaulingTensMaxCrown()
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

void CSpecHaulingLimitsPropertyPage::OnCheckHaulingTensMaxSuper()
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

void CSpecHaulingLimitsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
