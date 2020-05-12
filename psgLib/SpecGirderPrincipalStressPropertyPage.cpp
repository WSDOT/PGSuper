// SpecGirderPrincipalStressPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGirderPrincipalStressPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecGirderPrincipalStressPropertyPage

IMPLEMENT_DYNAMIC(CSpecGirderPrincipalStressPropertyPage, CMFCPropertyPage)

CSpecGirderPrincipalStressPropertyPage::CSpecGirderPrincipalStressPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_GIRDER_PRINCIPAL_STRESS)
{
}

CSpecGirderPrincipalStressPropertyPage::~CSpecGirderPrincipalStressPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGirderPrincipalStressPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGirderPrincipalStressPropertyPage message handlers

void CSpecGirderPrincipalStressPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGirderPrincipalStressData(pDX);
}


BOOL CSpecGirderPrincipalStressPropertyPage::OnInitDialog()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD);
   pcb->AddString(_T("AASHTO LRFD Equation 5.9.2.3.3-1"));
   pcb->AddString(_T("NCHRP Report 849 Equation 3.8"));

   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecGirderPrincipalStressPropertyPage::OnSetActive()
{
   // If we hide these, there is nothing to show on the page
   // There is no harm showning the controls, they just aren't used

   //// This was not in the LRFD before 8th Edition 2017
   //CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   //int nShow = (pDad->m_Entry.GetSpecificationType() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017 ? SW_HIDE : SW_SHOW);
   //GetDlgItem(IDC_PRINCIPAL_TENSION_GROUP)->ShowWindow(nShow);
   //GetDlgItem(IDC_PRINCIPAL_TENSION_LABEL)->ShowWindow(nShow);
   //GetDlgItem(IDC_PRINCIPAL_TENSION)->ShowWindow(nShow);
   //GetDlgItem(IDC_PRINCIPAL_TENSION_UNIT)->ShowWindow(nShow);
   //GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD_LABEL)->ShowWindow(nShow);
   //GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD)->ShowWindow(nShow);
   return CMFCPropertyPage::OnSetActive();
}

void CSpecGirderPrincipalStressPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

