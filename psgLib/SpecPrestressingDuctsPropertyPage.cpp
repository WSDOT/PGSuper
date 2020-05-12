// SpecPrestressingDuctsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecPrestressingDuctsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecPrestressingDuctsPropertyPage

IMPLEMENT_DYNAMIC(CSpecPrestressingDuctsPropertyPage, CMFCPropertyPage)

CSpecPrestressingDuctsPropertyPage::CSpecPrestressingDuctsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_PRESTRESSING_DUCTS)
{
}

CSpecPrestressingDuctsPropertyPage::~CSpecPrestressingDuctsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecPrestressingDuctsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecPrestressingDuctsPropertyPage message handlers

void CSpecPrestressingDuctsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecPrestressingDuctsPropertyPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangePrestressLimitsDuctsData(pDX);
}


BOOL CSpecPrestressingDuctsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecPrestressingDuctsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
