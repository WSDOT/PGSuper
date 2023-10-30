///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// SpecClosurePage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecClosurePage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/ClosureJointCriteria.h>
#include <psgLib/PrestressedElementCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CSpecClosurePage dialog

IMPLEMENT_DYNAMIC(CSpecClosurePage, CPropertyPage)

CSpecClosurePage::CSpecClosurePage()
	: CPropertyPage(CSpecClosurePage::IDD)
{

}

CSpecClosurePage::~CSpecClosurePage()
{
}

void CSpecClosurePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeClosureData(pDX);
}

BEGIN_MESSAGE_MAP(CSpecClosurePage, CPropertyPage)
   ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_I_TENSION_MAX,OnCheckServiceITensionMax)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_PTZ_TENSION_MAX,OnCheckServicePtzTensionMax)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_PTZ_TENSION_WITH_REBAR_MAX,OnCheckServicePtzTensionMaxWithRebar)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_III_TENSION_MAX,OnCheckServiceTensionMax)
   ON_BN_CLICKED(IDC_CHECK_SERVICE_III_TENSION_WITH_REBAR_MAX,OnCheckServiceTensionMaxWithRebar)

END_MESSAGE_MAP()

void CSpecClosurePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_CLOSURE_JOINTS);
}

// CSpecClosurePage message handlers
BOOL CSpecClosurePage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   OnCheckServiceITensionMax();
   OnCheckServicePtzTensionMax();
   OnCheckServicePtzTensionMaxWithRebar();
   OnCheckServiceTensionMax();
   OnCheckServiceTensionMaxWithRebar();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecClosurePage::OnSetActive()
{
   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CWnd* pGrp = GetDlgItem(IDC_FATIGUE_GROUP);
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
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

   // A bit TRICKY here. Settings for closure joint are sync'd with precast element. ExchangeClosureData sets value, but we have to enable/disable controls here.
   // Note that we are setting to prestressed element, not closure. This is because DoDataExchange can be called after this function
   BOOL bEnable = pDad->m_Entry.GetPrestressedElementCriteria().bCheckFinalServiceITension ? TRUE:FALSE;
   if (bEnable)
   {
      GetDlgItem(IDC_STATIC_SERVICE_I_TENSION)->SetWindowText(_T("Tensile Stress (Effective Prestress + Permanent Loads) (Enabled on Precast Elements tab)"));
   }
   else
   {
      GetDlgItem(IDC_STATIC_SERVICE_I_TENSION)->SetWindowText(_T("Tensile Stress (Effective Prestress + Permanent Loads) (Disabled on Precast Elements tab)"));
   }

   GetDlgItem(IDC_STATIC_SERVICE_I_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVICE_I_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVICE_I_TENSION_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_SERVICE_I_TENSION_MAX)->EnableWindow(bEnable);

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_GTEMP)->SetWindowText(CString(_T("Stress Limits for Temporary Stresses before Losses (LRFD ")) + WBFL::LRFD::LrfdCw8th(_T("5.9.4.1"), _T("5.9.2.3.1")) + _T(", ") + WBFL::LRFD::LrfdCw8th(_T("5.14.1.3.2d"),_T("5.12.3.4.2d")) + _T(")"));
   GetDlgItem(IDC_GPERM)->SetWindowText(CString(_T("Stress Limits at Service Limit State after Losses (LRFD ")) + WBFL::LRFD::LrfdCw8th(_T("5.9.4.2"), _T("5.9.2.3.2")) + _T(", ") + WBFL::LRFD::LrfdCw8th(_T("5.14.1.3.2d"),_T("5.12.3.4.2d")) + _T(")"));

   OnCheckServiceITensionMax();
   OnCheckServicePtzTensionMax();
   OnCheckServicePtzTensionMaxWithRebar();
   OnCheckServiceTensionMax();
   OnCheckServiceTensionMaxWithRebar();

   return CPropertyPage::OnSetActive();
}

void CSpecClosurePage::OnCheckServiceITensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   // Need also to check main setting
   pchk = (CButton*)GetDlgItem(IDC_STATIC_SERVICE_I_TENSION);
   ASSERT(pchk);
   ischk &= pchk->IsWindowEnabled();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_I_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecClosurePage::OnCheckServicePtzTensionMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_PTZ_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_PTZ_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_PTZ_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecClosurePage::OnCheckServicePtzTensionMaxWithRebar()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_PTZ_TENSION_WITH_REBAR_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_PTZ_TENSION_WITH_REBAR_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_PTZ_TENSION_WITH_REBAR_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecClosurePage::OnCheckServiceTensionMax()
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

void CSpecClosurePage::OnCheckServiceTensionMaxWithRebar()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_SERVICE_III_TENSION_WITH_REBAR_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_WITH_REBAR_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_SERVICE_III_TENSION_WITH_REBAR_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}
