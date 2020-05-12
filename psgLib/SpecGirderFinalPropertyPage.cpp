// SpecGirderFinalPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGirderFinalPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecGirderFinalPropertyPage

IMPLEMENT_DYNAMIC(CSpecGirderFinalPropertyPage, CMFCPropertyPage)

CSpecGirderFinalPropertyPage::CSpecGirderFinalPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_GIRDER_FINAL)
{
}

CSpecGirderFinalPropertyPage::~CSpecGirderFinalPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGirderFinalPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_I_TENSION, OnCheckServiceITensileStress)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_I_TENSION_MAX, OnCheckServiceITensionMax)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_III_TENSION_MAX, OnCheckServiceIIITensionMax)
   ON_BN_CLICKED(IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX, OnCheckSevereServiceIIITensionMax)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGirderFinalPropertyPage message handlers

void CSpecGirderFinalPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGirderFinalData(pDX);
}


BOOL CSpecGirderFinalPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnCheckServiceITensileStress();
   OnCheckServiceITensionMax();
   OnCheckServiceIIITensionMax();
   OnCheckSevereServiceIIITensionMax();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecGirderFinalPropertyPage::OnSetActive()
{
   // 2017 crosswalk chapter 5 reorg
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   GetDlgItem(IDC_GPERM)->SetWindowText(CString(_T("Stress Limits at Service Limit State after Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.2"), _T("5.9.2.3.2")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}


void CSpecGirderFinalPropertyPage::OnCheckServiceITensileStress()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_I_TENSION);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_I_TENSION);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);

   OnCheckServiceITensionMax();
}

void CSpecGirderFinalPropertyPage::OnCheckServiceITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->IsWindowEnabled() ? pchk->GetCheck() == BST_CHECKED : FALSE;

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderFinalPropertyPage::OnCheckServiceIIITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderFinalPropertyPage::OnCheckSevereServiceIIITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SEVERE_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SEVERE_SERVICE_III_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderFinalPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

