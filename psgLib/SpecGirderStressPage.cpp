///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
   ON_BN_CLICKED(IDC_CHECK_SERVICE_I_TENSION,OnCheckServiceITensileStress)
	ON_BN_CLICKED(IDC_CHECK_RELEASE_TENSION_MAX, OnCheckReleaseTensionMax)
	ON_BN_CLICKED(IDC_CHECK_TS_REMOVAL_TENSION_MAX, OnCheckTSRemovalTensionMax)
   ON_BN_CLICKED(IDC_CHECK_AFTER_DECK_TENSION_MAX, OnCheckAfterDeckTensionMax)

   ON_BN_CLICKED(IDC_CHECK_SERVICE_I_TENSION_MAX, OnCheckServiceITensionMax)
	ON_BN_CLICKED(IDC_CHECK_SERVICE_III_TENSION_MAX, OnCheckServiceIIITensionMax)
	ON_BN_CLICKED(IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX, OnCheckSevereServiceIIITensionMax)

   ON_BN_CLICKED(IDC_CHECK_TEMPORARY_STRESSES, OnCheckTemporaryStresses)
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()


// CSpecGirderStressPage message handlers

BOOL CSpecGirderStressPage::OnInitDialog()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD);
   pcb->AddString(_T("AASHTO LRFD Equation 5.9.2.3.3-1"));
   pcb->AddString(_T("NCHRP Report 849 Equation 3.8"));

   CPropertyPage::OnInitDialog();

   OnCheckServiceITensileStress();
	OnCheckReleaseTensionMax();
	OnCheckTSRemovalTensionMax();
   OnCheckAfterDeckTensionMax();
	OnCheckServiceITensionMax();
	OnCheckServiceIIITensionMax();
	OnCheckSevereServiceIIITensionMax();
   OnCheckTemporaryStresses();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecGirderStressPage::OnSetActive()
{
   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CWnd* pGrp = GetDlgItem(IDC_FATIGUE_GROUP);
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   if ( pDad->m_Entry.GetSpecificationType() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pGrp->SetWindowText(_T("Stress limit at Fatigue Limit State (LRFD 5.9.4.2.1)"));
      pWnd->SetWindowText(_T("Service IA (Live Load Plus One-Half of Permanent Loads)"));
   }
   else
   {
      pGrp->SetWindowText(_T("Stress limit for Fatigue (LRFD 5.5.3.1)"));
      pWnd->SetWindowText(_T("Fatigue I plus one-half the sum of effective prestress and permanent loads"));
   }


   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_GTEMP)->SetWindowText(CString(_T("Stress Limits for Temporary Stresses before Losses (LRFD ")) +  pDad->LrfdCw8th(_T("5.9.4.1"),_T("5.9.2.3.1")) + _T(")"));
   GetDlgItem(IDC_GPERM)->SetWindowText(CString(_T("Stress Limits at Service Limit State after Losses (LRFD ")) +  pDad->LrfdCw8th(_T("5.9.4.2"),_T("5.9.2.3.2")) + _T(")"));

   // This was not in the LRFD before 8th Edition 2017
   int nShow = (pDad->m_Entry.GetSpecificationType() < lrfdVersionMgr::EighthEdition2017 ? SW_HIDE : SW_SHOW);
   GetDlgItem(IDC_PRINCIPAL_TENSION_GROUP)->ShowWindow(nShow);
   GetDlgItem(IDC_PRINCIPAL_TENSION_LABEL)->ShowWindow(nShow);
   GetDlgItem(IDC_PRINCIPAL_TENSION)->ShowWindow(nShow);
   GetDlgItem(IDC_PRINCIPAL_TENSION_UNIT)->ShowWindow(nShow);
   GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD_LABEL)->ShowWindow(nShow);
   GetDlgItem(IDC_PRINCIPAL_TENSION_METHOD)->ShowWindow(nShow);

   return CPropertyPage::OnSetActive();
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

void CSpecGirderStressPage::OnCheckServiceITensileStress()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_I_TENSION);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_I_TENSION);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckServiceITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckServiceIIITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckSevereServiceIIITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SEVERE_SERVICE_III_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SEVERE_SERVICE_III_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecGirderStressPage::OnCheckTemporaryStresses()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_TEMPORARY_STRESSES);
   BOOL bEnable = pchk->GetCheck() == BST_CHECKED;

   GetDlgItem(IDC_TEMP_STRAND_REMOVAL_GROUP)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_COMPRESS_FC)->EnableWindow(bEnable);

   GetDlgItem(IDC_TS_REMOVAL_TENSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_REMOVAL_TENSION_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_CHECK_TS_REMOVAL_TENSION_MAX)->EnableWindow(bEnable);
   BOOL bEnable2 = bEnable;
   if ( IsDlgButtonChecked(IDC_CHECK_TS_REMOVAL_TENSION_MAX) != BST_CHECKED )
   {
      bEnable2 = FALSE;
   }
   GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX)->EnableWindow(bEnable2);
   GetDlgItem(IDC_TS_REMOVAL_TENSION_MAX_UNIT)->EnableWindow(bEnable2);

   GetDlgItem(IDC_TS_TENSION_WITH_REBAR_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_TENSION_WITH_REBAR)->EnableWindow(bEnable);
   GetDlgItem(IDC_TS_TENSION_WITH_REBAR_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_AFTER_DECK_GROUP)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_COMPRESSION_FC)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_AFTER_DECK_TENSION_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_CHECK_AFTER_DECK_TENSION_MAX)->EnableWindow(bEnable);
   BOOL bEnable3 = bEnable;
   if ( IsDlgButtonChecked(IDC_CHECK_AFTER_DECK_TENSION_MAX) != BST_CHECKED )
   {
      bEnable3 = FALSE;
   }
   GetDlgItem(IDC_AFTER_DECK_TENSION_MAX)->EnableWindow(bEnable3);
   GetDlgItem(IDC_AFTER_DECK_TENSION_MAX_UNIT)->EnableWindow(bEnable3);
}

void CSpecGirderStressPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_PRESTRESS_ELEMENTS );
}
