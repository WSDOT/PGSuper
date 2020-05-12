// SpecPrestressingOptionsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecPrestressingOptionsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecPrestressingOptionsPropertyPage

IMPLEMENT_DYNAMIC(CSpecPrestressingOptionsPropertyPage, CMFCPropertyPage)

CSpecPrestressingOptionsPropertyPage::CSpecPrestressingOptionsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_PRESTRESSING_OPTIONS)
{
}

CSpecPrestressingOptionsPropertyPage::~CSpecPrestressingOptionsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecPrestressingOptionsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecPrestressingOptionsPropertyPage message handlers

void CSpecPrestressingOptionsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangePrestressLimitsOptionsData(pDX);
}


BOOL CSpecPrestressingOptionsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecPrestressingOptionsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
