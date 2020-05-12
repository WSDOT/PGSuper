// SpecDeadLoadOverlayPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDeadLoadOverlayPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDeadLoadOverlayPropertyPage

IMPLEMENT_DYNAMIC(CSpecDeadLoadOverlayPropertyPage, CMFCPropertyPage)

CSpecDeadLoadOverlayPropertyPage::CSpecDeadLoadOverlayPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DEAD_LOAD_OVERLAY)
{
}

CSpecDeadLoadOverlayPropertyPage::~CSpecDeadLoadOverlayPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDeadLoadOverlayPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDeadLoadOverlayPropertyPage message handlers

void CSpecDeadLoadOverlayPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDeadLoadOverlayData(pDX);
}


BOOL CSpecDeadLoadOverlayPropertyPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_DISTR);
   pCB->AddString(_T("Uniformly Among All Girders [LRFD 4.6.2.2.1]"));
   pCB->AddString(_T("Using Tributary Width"));

   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDeadLoadOverlayPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
