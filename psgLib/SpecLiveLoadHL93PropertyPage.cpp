// SpecLiveLoadHL93PropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiveLoadHL93PropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecLiveLoadHL93PropertyPage

IMPLEMENT_DYNAMIC(CSpecLiveLoadHL93PropertyPage, CMFCPropertyPage)

CSpecLiveLoadHL93PropertyPage::CSpecLiveLoadHL93PropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIVE_LOAD_HL93)
{
}

CSpecLiveLoadHL93PropertyPage::~CSpecLiveLoadHL93PropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiveLoadHL93PropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiveLoadHL93PropertyPage message handlers

void CSpecLiveLoadHL93PropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiveLoadHL93Data(pDX);
}


BOOL CSpecLiveLoadHL93PropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiveLoadHL93PropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
