// SpecShearResistanceFactorsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecShearResistanceFactorsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecShearResistanceFactorsPropertyPage

IMPLEMENT_DYNAMIC(CSpecShearResistanceFactorsPropertyPage, CMFCPropertyPage)

CSpecShearResistanceFactorsPropertyPage::CSpecShearResistanceFactorsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_SHEAR_RESISTANCE_FACTORS)
{
}

CSpecShearResistanceFactorsPropertyPage::~CSpecShearResistanceFactorsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecShearResistanceFactorsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecShearResistanceFactorsPropertyPage message handlers

void CSpecShearResistanceFactorsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeShearResistanceFactorsData(pDX);
}


BOOL CSpecShearResistanceFactorsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecShearResistanceFactorsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_SCLOSURE)->SetWindowText(CString(_T("Closure Joint (LRFD 5.5.4.2, ")) + pDad->LrfdCw8th(_T("5.14.1.3.2d"), _T("5.12.3.4.2d")) + _T(")"));

   return __super::OnSetActive();
}

void CSpecShearResistanceFactorsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
