// SpecPrestressingPostTensionLimitsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecPrestressingPostTensionLimitsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecPrestressingPostTensionLimitsPropertyPage

IMPLEMENT_DYNAMIC(CSpecPrestressingPostTensionLimitsPropertyPage, CMFCPropertyPage)

CSpecPrestressingPostTensionLimitsPropertyPage::CSpecPrestressingPostTensionLimitsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_PRESTRESSING_PT_LIMITS)
{
}

CSpecPrestressingPostTensionLimitsPropertyPage::~CSpecPrestressingPostTensionLimitsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecPrestressingPostTensionLimitsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_PT_AT_JACKING, OnPtChecked)
   ON_BN_CLICKED(IDC_CHECK_PT_BEFORE_TRANSFER, OnPtChecked)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecPrestressingPostTensionLimitsPropertyPage message handlers

void CSpecPrestressingPostTensionLimitsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangePrestressLimitsPostTensionData(pDX);
}


BOOL CSpecPrestressingPostTensionLimitsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnPtChecked();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecPrestressingPostTensionLimitsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // deal with 2017 crosswalk
   CWnd* pWnd = GetDlgItem(IDC_GPOST);
   pWnd->SetWindowText(CString(_T("Stress Limits for Post-tensioning (")) + pDad->LrfdCw8th(_T("5.9.3"), _T("5.9.2.2")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecPrestressingPostTensionLimitsPropertyPage::OnPtChecked()
{
   EnableControls(IsDlgButtonChecked(IDC_CHECK_PT_AT_JACKING), IDC_PT_AT_JACKING_SR, IDC_PT_AT_JACKING_LR);
   EnableControls(IsDlgButtonChecked(IDC_CHECK_PT_BEFORE_TRANSFER), IDC_PT_BEFORE_TRANSFER_SR, IDC_PT_BEFORE_TRANSFER_LR);
}

void CSpecPrestressingPostTensionLimitsPropertyPage::EnableControls(BOOL bEnable, UINT nSR, UINT nLR)
{
   GetDlgItem(nSR)->EnableWindow(bEnable);
   GetDlgItem(nLR)->EnableWindow(bEnable);
}

void CSpecPrestressingPostTensionLimitsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
