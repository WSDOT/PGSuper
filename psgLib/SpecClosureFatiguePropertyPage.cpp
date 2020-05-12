// SpecClosureFatiguePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecClosureFatiguePropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>

// CSpecClosureFatiguePropertyPage

IMPLEMENT_DYNAMIC(CSpecClosureFatiguePropertyPage, CMFCPropertyPage)

CSpecClosureFatiguePropertyPage::CSpecClosureFatiguePropertyPage() :
   CMFCPropertyPage(IDD_SPEC_CLOSURE_FATIGUE)
{
}

CSpecClosureFatiguePropertyPage::~CSpecClosureFatiguePropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecClosureFatiguePropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecClosureFatiguePropertyPage message handlers

void CSpecClosureFatiguePropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeClosureFatigueData(pDX);
}


BOOL CSpecClosureFatiguePropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecClosureFatiguePropertyPage::OnSetActive()
{
   // 2017 crosswalk chapter 5 reorg
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CWnd* pGrp = GetDlgItem(IDC_FATIGUE_GROUP);
   if (pDad->m_Entry.GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims)
   {
      pGrp->SetWindowText(_T("Stress limit at Fatigue Limit State (LRFD 5.9.4.2.1)"));
      pWnd->SetWindowText(_T("Service IA (Live Load Plus One-Half of Permanent Loads)"));
   }
   else
   {
      pGrp->SetWindowText(_T("Stress limit for Fatigue (LRFD 5.5.3.1)"));
      pWnd->SetWindowText(_T("Fatigue I plus one-half the sum of effective prestress and permanent loads"));
   }

   return CMFCPropertyPage::OnSetActive();
}

void CSpecClosureFatiguePropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

