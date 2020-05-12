// SpecClosureFinalPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecClosureFinalPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecClosureFinalPropertyPage

IMPLEMENT_DYNAMIC(CSpecClosureFinalPropertyPage, CMFCPropertyPage)

CSpecClosureFinalPropertyPage::CSpecClosureFinalPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_CLOSURE_FINAL)
{
}

CSpecClosureFinalPropertyPage::~CSpecClosureFinalPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecClosureFinalPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecClosureFinalPropertyPage message handlers

void CSpecClosureFinalPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeClosureFinalData(pDX);
}


BOOL CSpecClosureFinalPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecClosureFinalPropertyPage::OnSetActive()
{
   // 2017 crosswalk chapter 5 reorg
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   GetDlgItem(IDC_GPERM)->SetWindowText(CString(_T("Stress Limits at Service Limit State after Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.2"), _T("5.9.2.3.2")) + _T(", ") + pDad->LrfdCw8th(_T("5.14.1.3.2d"), _T("5.12.3.4.2d")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecClosureFinalPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

