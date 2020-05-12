// SpecCreepExcessCamberPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecCreepExcessCamberPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecCreepExcessCamberPropertyPage

IMPLEMENT_DYNAMIC(CSpecCreepExcessCamberPropertyPage, CMFCPropertyPage)

CSpecCreepExcessCamberPropertyPage::CSpecCreepExcessCamberPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_CREEP_EXCESS_CAMBER)
{
}

CSpecCreepExcessCamberPropertyPage::~CSpecCreepExcessCamberPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecCreepExcessCamberPropertyPage, CMFCPropertyPage)
   ON_CBN_SELCHANGE(IDC_HAUNCH_COMP_CB, OnChangeHaunch)
   ON_CBN_SELCHANGE(IDC_HAUNCH_COMP_PROPS_CB, OnChangeHaunch)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecCreepExcessCamberPropertyPage message handlers

void CSpecCreepExcessCamberPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeCreepExcessCamberData(pDX);
}


BOOL CSpecCreepExcessCamberPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnChangeHaunch();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecCreepExcessCamberPropertyPage::OnChangeHaunch()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_COMP_CB);
   int idxl = pBox->GetCurSel();
   BOOL enablel = idxl == (int)pgsTypes::hlcDetailedAnalysis;

   GetDlgItem(IDC_HAUNCH_FACTOR)->EnableWindow(enablel);
   GetDlgItem(IDC_HAUNCH_FACTOR1)->EnableWindow(enablel);
   GetDlgItem(IDC_HAUNCH_FACTOR2)->EnableWindow(enablel);

   pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_COMP_PROPS_CB);
   int idxp = pBox->GetCurSel();
   BOOL enablep = enablel || idxp == (int)pgsTypes::hspDetailedDescription;

   GetDlgItem(IDC_HAUNCH_TOLER_STATIC)->EnableWindow(enablep);
   GetDlgItem(IDC_HAUNCH_TOLER_UNIT)->EnableWindow(enablep);
   GetDlgItem(IDC_HAUNCH_TOLER)->EnableWindow(enablep);
}

void CSpecCreepExcessCamberPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
