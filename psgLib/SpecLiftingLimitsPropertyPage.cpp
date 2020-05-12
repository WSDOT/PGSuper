// SpecLiftingLimitsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiftingLimitsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecLiftingLimitsPropertyPage

IMPLEMENT_DYNAMIC(CSpecLiftingLimitsPropertyPage, CMFCPropertyPage)

CSpecLiftingLimitsPropertyPage::CSpecLiftingLimitsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIFTING_LIMITS)
{
}

CSpecLiftingLimitsPropertyPage::~CSpecLiftingLimitsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiftingLimitsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_LIFTING_TENSION_MAX, OnCheckLiftingNormalMax)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiftingLimitsPropertyPage message handlers

void CSpecLiftingLimitsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiftingLimitsData(pDX);
}


BOOL CSpecLiftingLimitsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnCheckLiftingNormalMax();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiftingLimitsPropertyPage::OnCheckLiftingNormalMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_LIFTING_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_LIFTING_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_LIFTING_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecLiftingLimitsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
