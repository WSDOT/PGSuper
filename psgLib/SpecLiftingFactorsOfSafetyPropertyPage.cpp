// SpecLiftingFactorsOfSafetyPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiftingFactorsOfSafetyPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecLiftingFactorsOfSafetyPropertyPage

IMPLEMENT_DYNAMIC(CSpecLiftingFactorsOfSafetyPropertyPage, CMFCPropertyPage)

CSpecLiftingFactorsOfSafetyPropertyPage::CSpecLiftingFactorsOfSafetyPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIFTING_FOS)
{
}

CSpecLiftingFactorsOfSafetyPropertyPage::~CSpecLiftingFactorsOfSafetyPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiftingFactorsOfSafetyPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiftingFactorsOfSafetyPropertyPage message handlers

void CSpecLiftingFactorsOfSafetyPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiftingFactorsOfSafetyData(pDX);
}


BOOL CSpecLiftingFactorsOfSafetyPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiftingFactorsOfSafetyPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
