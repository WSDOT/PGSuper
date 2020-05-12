// SpecShearInterfaceShearPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecShearInterfaceShearPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecShearInterfaceShearPropertyPage

IMPLEMENT_DYNAMIC(CSpecShearInterfaceShearPropertyPage, CMFCPropertyPage)

CSpecShearInterfaceShearPropertyPage::CSpecShearInterfaceShearPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_SHEAR_INTERFACE)
{
}

CSpecShearInterfaceShearPropertyPage::~CSpecShearInterfaceShearPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecShearInterfaceShearPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecShearInterfaceShearPropertyPage message handlers

void CSpecShearInterfaceShearPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeShearInterfaceData(pDX);
}


BOOL CSpecShearInterfaceShearPropertyPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SHEAR_FLOW_METHOD);
   pCB->AddString(_T("using the LRFD simplified method: vui = Vu/(Wtf dv)"));
   pCB->AddString(_T("using the classical shear flow method: vui = (Vu Q)/(I Wtf)"));

   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecShearInterfaceShearPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_SHIS)->SetWindowText(CString(_T("LRFD ")) + pDad->LrfdCw8th(_T("5.8.4.2"), _T("5.7.4.5")) + _T(" Spacing of interface shear connectors shall not exceed"));
   GetDlgItem(IDC_USE_DECK_FOR_PC)->SetWindowText(CString(_T("Use the deck weight for the permanent net compressive force normal to the shear plane, (Pc, LRFD Eq. ")) + pDad->LrfdCw8th(_T("5.8.4.1-3"), _T("5.7.4.3-3")) + _T(")"));

   // this text only applies for 7th Edition, 2014 and later
   if (WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= pDad->GetSpecVersion())
   {
      GetDlgItem(IDC_DEPTH_OF_UNIT)->ShowWindow(SW_SHOW);
   }
   else
   {
      GetDlgItem(IDC_DEPTH_OF_UNIT)->ShowWindow(SW_HIDE);
   }


   return __super::OnSetActive();
}

void CSpecShearInterfaceShearPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
