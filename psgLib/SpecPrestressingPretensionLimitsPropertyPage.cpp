// SpecPrestressingPretensionLimitsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecPrestressingPretensionLimitsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecPrestressingPretensionLimitsPropertyPage

IMPLEMENT_DYNAMIC(CSpecPrestressingPretensionLimitsPropertyPage, CMFCPropertyPage)

CSpecPrestressingPretensionLimitsPropertyPage::CSpecPrestressingPretensionLimitsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_PRESTRESSING_PS_LIMITS)
{
}

CSpecPrestressingPretensionLimitsPropertyPage::~CSpecPrestressingPretensionLimitsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecPrestressingPretensionLimitsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_PS_AT_JACKING, OnPsChecked)
   ON_BN_CLICKED(IDC_CHECK_PS_BEFORE_TRANSFER, OnPsChecked)
   ON_BN_CLICKED(IDC_CHECK_PS_AFTER_TRANSFER, OnCheckPsAfterTransfer)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecPrestressingPretensionLimitsPropertyPage message handlers

void CSpecPrestressingPretensionLimitsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangePrestressLimitsPretensionData(pDX);
}


BOOL CSpecPrestressingPretensionLimitsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnPsChecked();
   OnCheckPsAfterTransfer();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecPrestressingPretensionLimitsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // deal with 2017 crosswalk
   CWnd* pWnd = GetDlgItem(IDC_GPRES);
   pWnd->SetWindowText(CString(_T("Stress Limits for Prestressing (")) + pDad->LrfdCw8th(_T("5.9.3"), _T("5.9.2.2")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecPrestressingPretensionLimitsPropertyPage::OnPsChecked()
{
   EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AT_JACKING), IDC_PS_AT_JACKING_SR, IDC_PS_AT_JACKING_LR);
   EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_BEFORE_TRANSFER), IDC_PS_BEFORE_TRANSFER_SR, IDC_PS_BEFORE_TRANSFER_LR);
}

void CSpecPrestressingPretensionLimitsPropertyPage::EnableControls(BOOL bEnable, UINT nSR, UINT nLR)
{
   GetDlgItem(nSR)->EnableWindow(bEnable);
   GetDlgItem(nLR)->EnableWindow(bEnable);
}

void CSpecPrestressingPretensionLimitsPropertyPage::OnCheckPsAfterTransfer()
{
   EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AFTER_TRANSFER), IDC_PS_AFTER_TRANSFER_SR, IDC_PS_AFTER_TRANSFER_LR);
}

void CSpecPrestressingPretensionLimitsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
