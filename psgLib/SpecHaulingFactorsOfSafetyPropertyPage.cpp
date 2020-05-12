// SpecHaulingFactorsOfSafetyPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecHaulingFactorsOfSafetyPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecHaulingFactorsOfSafetyPropertyPage

IMPLEMENT_DYNAMIC(CSpecHaulingFactorsOfSafetyPropertyPage, CMFCPropertyPage)

CSpecHaulingFactorsOfSafetyPropertyPage::CSpecHaulingFactorsOfSafetyPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_HAULING_WSDOT_FOS)
{
}

CSpecHaulingFactorsOfSafetyPropertyPage::~CSpecHaulingFactorsOfSafetyPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecHaulingFactorsOfSafetyPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecHaulingFactorsOfSafetyPropertyPage message handlers

void CSpecHaulingFactorsOfSafetyPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeHaulingFactorsOfSafetyData(pDX);
}


BOOL CSpecHaulingFactorsOfSafetyPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingFactorsOfSafetyPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
