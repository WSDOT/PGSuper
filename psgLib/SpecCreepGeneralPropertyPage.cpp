// SpecCreepGeneralPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecCreepGeneralPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecCreepGeneralPropertyPage

IMPLEMENT_DYNAMIC(CSpecCreepGeneralPropertyPage, CMFCPropertyPage)

CSpecCreepGeneralPropertyPage::CSpecCreepGeneralPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_CREEP_GENERAL)
{
}

CSpecCreepGeneralPropertyPage::~CSpecCreepGeneralPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecCreepGeneralPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecCreepGeneralPropertyPage message handlers

void CSpecCreepGeneralPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeCreepGeneralData(pDX);
}


BOOL CSpecCreepGeneralPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecCreepGeneralPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
