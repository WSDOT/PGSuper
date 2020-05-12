// SpecGirderTemporaryPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGirderTemporaryPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecGirderTemporaryPropertyPage

IMPLEMENT_DYNAMIC(CSpecGirderTemporaryPropertyPage, CMFCPropertyPage)

CSpecGirderTemporaryPropertyPage::CSpecGirderTemporaryPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_GIRDER_TEMPORARY)
{
}

CSpecGirderTemporaryPropertyPage::~CSpecGirderTemporaryPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGirderTemporaryPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_RELEASE_TENSION_MAX, OnCheckReleaseTensionMax)
   ON_BN_CLICKED(IDC_CHECK_TEMPORARY_STRESSES, OnCheckTemporaryStresses)
   ON_BN_CLICKED(IDC_CHECK_TS_REMOVAL_TENSION_MAX, OnCheckTSRemovalTensionMax)
   ON_BN_CLICKED(IDC_CHECK_AFTER_DECK_TENSION_MAX, OnCheckAfterDeckTensionMax)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGirderTemporaryPropertyPage message handlers

void CSpecGirderTemporaryPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGirderTemporaryData(pDX);
}


BOOL CSpecGirderTemporaryPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnCheckReleaseTensionMax();
   OnCheckTemporaryStresses();
   OnCheckTSRemovalTensionMax();
   OnCheckAfterDeckTensionMax();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecGirderTemporaryPropertyPage::OnSetActive()
{
   // 2017 crosswalk chapter 5 reorg
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   GetDlgItem(IDC_GTEMP)->SetWindowText(CString(_T("Stress Limits for Temporary Stresses before Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.1"), _T("5.9.2.3.1")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecGirderTemporaryPropertyPage::OnCheckReleaseTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_RELEASE_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_RELEASE_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_RELEASE_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderTemporaryPropertyPage::OnCheckTSRemovalTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_TS_REMOVAL_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderTemporaryPropertyPage::OnCheckAfterDeckTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_AFTER_DECK_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_AFTER_DECK_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_AFTER_DECK_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderTemporaryPropertyPage::OnCheckTemporaryStresses()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_TEMPORARY_STRESSES);
   BOOL bEnable = pchk->GetCheck() == BST_CHECKED;

   GetDlgItem(IDC_TEMP_STRAND_REMOVAL_GROUP)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESS_FC)->EnableWindow(bEnable);

   GetDlgItem(IDC_TS_REMOVAL_TENSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_TENSION_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_CHECK_TS_REMOVAL_TENSION_MAX)->EnableWindow(bEnable);
   BOOL bEnable2 = bEnable;
   if (IsDlgButtonChecked(IDC_CHECK_TS_REMOVAL_TENSION_MAX) != BST_CHECKED)
   {
      bEnable2 = FALSE;
   }
   GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX)->EnableWindow(bEnable2);
   GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX_UNIT)->EnableWindow(bEnable2);

   GetDlgItem(IDC_TS_TENSION_WITH_REBAR_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_TENSION_WITH_REBAR)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_TENSION_WITH_REBAR_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_AFTER_DECK_GROUP)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION_FC)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_CHECK_AFTER_DECK_TENSION_MAX)->EnableWindow(bEnable);
   BOOL bEnable3 = bEnable;
   if (IsDlgButtonChecked(IDC_CHECK_AFTER_DECK_TENSION_MAX) != BST_CHECKED)
   {
      bEnable3 = FALSE;
   }
   GetDlgItem(IDC_AFTER_DECK_TENSION_MAX)->EnableWindow(bEnable3);
   GetDlgItem(IDC_AFTER_DECK_TENSION_MAX_UNIT)->EnableWindow(bEnable3);
}

void CSpecGirderTemporaryPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

