// SpecMomentResistanceFactorsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecMomentResistanceFactorsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecMomentResistanceFactorsPropertyPage

IMPLEMENT_DYNAMIC(CSpecMomentResistanceFactorsPropertyPage, CMFCPropertyPage)

CSpecMomentResistanceFactorsPropertyPage::CSpecMomentResistanceFactorsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_MOMENT_RESISTANCE_FACTORS)
{
}

CSpecMomentResistanceFactorsPropertyPage::~CSpecMomentResistanceFactorsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecMomentResistanceFactorsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecMomentResistanceFactorsPropertyPage message handlers

void CSpecMomentResistanceFactorsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeMomentResistanceFactorsData(pDX);
}


BOOL CSpecMomentResistanceFactorsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecMomentResistanceFactorsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_SCLOSURE)->SetWindowText(CString(_T("Closure Joint (LRFD 5.5.4.2, ")) + pDad->LrfdCw8th(_T("5.14.1.3.2d"), _T("5.12.3.4.2d")) + _T(")"));

   return __super::OnSetActive();
}

void CSpecMomentResistanceFactorsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
