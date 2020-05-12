// SpecLiveLoadPedestrianPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLiveLoadPedestrianPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecLiveLoadPedestrianPropertyPage

IMPLEMENT_DYNAMIC(CSpecLiveLoadPedestrianPropertyPage, CMFCPropertyPage)

CSpecLiveLoadPedestrianPropertyPage::CSpecLiveLoadPedestrianPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIVE_LOAD_PEDESTRIAN)
{
}

CSpecLiveLoadPedestrianPropertyPage::~CSpecLiveLoadPedestrianPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLiveLoadPedestrianPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLiveLoadPedestrianPropertyPage message handlers

void CSpecLiveLoadPedestrianPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLiveLoadPedestrianData(pDX);
}


BOOL CSpecLiveLoadPedestrianPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();


   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiveLoadPedestrianPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
