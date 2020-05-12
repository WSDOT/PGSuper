// SpecClosureTemporaryPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecClosureTemporaryPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecClosureTemporaryPropertyPage

IMPLEMENT_DYNAMIC(CSpecClosureTemporaryPropertyPage, CMFCPropertyPage)

CSpecClosureTemporaryPropertyPage::CSpecClosureTemporaryPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_CLOSURE_TEMPORARY)
{
}

CSpecClosureTemporaryPropertyPage::~CSpecClosureTemporaryPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecClosureTemporaryPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecClosureTemporaryPropertyPage message handlers

void CSpecClosureTemporaryPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeClosureTemporaryData(pDX);
}


BOOL CSpecClosureTemporaryPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecClosureTemporaryPropertyPage::OnSetActive()
{
   // 2017 crosswalk chapter 5 reorg
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   GetDlgItem(IDC_GTEMP)->SetWindowText(CString(_T("Stress Limits for Temporary Stresses before Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.1"), _T("5.9.2.3.1")) + _T(", ") + pDad->LrfdCw8th(_T("5.14.1.3.2d"), _T("5.12.3.4.2d")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecClosureTemporaryPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

