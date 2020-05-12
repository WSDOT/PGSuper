// SpecHaulingKDOTPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecHaulingKDOTPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecHaulingKDOTPropertyPage

IMPLEMENT_DYNAMIC(CSpecHaulingKDOTPropertyPage, CMFCPropertyPage)

CSpecHaulingKDOTPropertyPage::CSpecHaulingKDOTPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_HAULING_KDOT)
{
}

CSpecHaulingKDOTPropertyPage::~CSpecHaulingKDOTPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecHaulingKDOTPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX, OnBnClickedCheckHaulingTensMax)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecHaulingKDOTPropertyPage message handlers

void CSpecHaulingKDOTPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeHaulingKDOTData(pDX);
}


BOOL CSpecHaulingKDOTPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnBnClickedCheckHaulingTensMax();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingKDOTPropertyPage::OnBnClickedCheckHaulingTensMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_UNIT);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CSpecHaulingKDOTPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
