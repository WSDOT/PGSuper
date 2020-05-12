// SpecGeneralDescriptionPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGeneralDescriptionPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecGeneralDescriptionPropertyPage

IMPLEMENT_DYNAMIC(CSpecGeneralDescriptionPropertyPage, CMFCPropertyPage)

CSpecGeneralDescriptionPropertyPage::CSpecGeneralDescriptionPropertyPage() : 
   CMFCPropertyPage(IDD_SPEC_GENERAL_DESCRIPTION)
{
}

CSpecGeneralDescriptionPropertyPage::~CSpecGeneralDescriptionPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGeneralDescriptionPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGeneralDescriptionPropertyPage message handlers

void CSpecGeneralDescriptionPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGeneralDescriptionData(pDX);

}

void CSpecGeneralDescriptionPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
