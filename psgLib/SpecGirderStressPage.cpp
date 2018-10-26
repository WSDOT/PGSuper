///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// SpecGirderStressPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecGirderStressPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"


// CSpecGirderStressPage dialog

IMPLEMENT_DYNAMIC(CSpecGirderStressPage, CPropertyPage)

CSpecGirderStressPage::CSpecGirderStressPage()
	: CPropertyPage(CSpecGirderStressPage::IDD)
{

}

CSpecGirderStressPage::~CSpecGirderStressPage()
{
}

void CSpecGirderStressPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeGirderData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecGirderStressPage, CPropertyPage)
	ON_BN_CLICKED(IDC_CHECK_RELEASE_TENSION_MAX, OnCheckReleaseTensionMax)
	ON_BN_CLICKED(IDC_CHECK_TS_REMOVAL_TENSION_MAX, OnCheckTSRemovalTensionMax)
   ON_BN_CLICKED(IDC_CHECK_AFTER_DECK_TENSION_MAX, OnCheckAfterDeckTensionMax)

	ON_BN_CLICKED(IDC_CHECK_SERVICE_TENSION_MAX, OnCheckServiceTensionMax)
	ON_BN_CLICKED(IDC_CHECK_SEVERE_SERVICE_TENSION_MAX, OnCheckSevereServiceTensionMax)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


// CSpecGirderStressPage message handlers

BOOL CSpecGirderStressPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

	OnCheckReleaseTensionMax();
	OnCheckTSRemovalTensionMax();
   OnCheckAfterDeckTensionMax();
	OnCheckServiceTensionMax();
	OnCheckSevereServiceTensionMax();

   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CWnd* pGrp = GetDlgItem(IDC_FATIGUE_GROUP);
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   if ( pDad->m_Entry.GetSpecificationType() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pGrp->SetWindowText(_T("Allowable Concrete Stress for Fatigue (LRFD 5.9.4.2.1)"));
      pWnd->SetWindowText(_T("Service IA (Live Load Plus One-Half of Permanent Loads)"));
   }
   else
   {
      pGrp->SetWindowText(_T("Allowable Concrete Stress at Fatigue Limit State (LRFD 5.5.3.1)"));
      pWnd->SetWindowText(_T("Fatigue I plus one-half the sum of effective prestress and permanent loads"));
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecGirderStressPage::OnCheckReleaseTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_RELEASE_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_RELEASE_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_RELEASE_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckTSRemovalTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_TS_REMOVAL_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckAfterDeckTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_AFTER_DECK_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_AFTER_DECK_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_AFTER_DECK_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckServiceTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckSevereServiceTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SEVERE_SERVICE_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SEVERE_SERVICE_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SEVERE_SERVICE_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

#pragma Reminder("UPDATE: need correct help context id")
LRESULT CSpecGirderStressPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGESITE_2_TAB );
   return TRUE;
}
