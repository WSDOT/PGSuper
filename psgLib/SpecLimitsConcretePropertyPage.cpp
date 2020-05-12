// SpecLimitsConcretePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLimitsConcretePropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecLimitsConcretePropertyPage

IMPLEMENT_DYNAMIC(CSpecLimitsConcretePropertyPage, CMFCPropertyPage)

CSpecLimitsConcretePropertyPage::CSpecLimitsConcretePropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIMITS_CONCRETE)
{
}

CSpecLimitsConcretePropertyPage::~CSpecLimitsConcretePropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLimitsConcretePropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLimitsConcretePropertyPage message handlers

void CSpecLimitsConcretePropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLimitsConcreteData(pDX);
}


BOOL CSpecLimitsConcretePropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecLimitsConcretePropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   return __super::OnSetActive();
}

void CSpecLimitsConcretePropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
