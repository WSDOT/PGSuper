///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
END_MESSAGE_MAP()

void CSpecClosurePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_CLOSURE_JOINTS);
}

// CSpecClosurePage message handlers
BOOL CSpecClosurePage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CSpecClosurePage::OnSetActive()
{
   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CWnd* pGrp = GetDlgItem(IDC_FATIGUE_GROUP);
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   if (pDad->m_Entry.GetSpecificationType() < lrfdVersionMgr::FourthEditionWith2009Interims)
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
   GetDlgItem(IDC_GTEMP)->SetWindowText(CString(_T("Stress Limits for Temporary Stresses before Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.1"), _T("5.9.2.3.1")) + _T(", ") + pDad->LrfdCw8th(_T("5.14.1.3.2d"),_T("5.12.3.4.2d")) + _T(")"));
   GetDlgItem(IDC_GPERM)->SetWindowText(CString(_T("Stress Limits at Service Limit State after Losses (LRFD ")) + pDad->LrfdCw8th(_T("5.9.4.2"), _T("5.9.2.3.2")) + _T(", ") + pDad->LrfdCw8th(_T("5.14.1.3.2d"),_T("5.12.3.4.2d")) + _T(")"));


   return CPropertyPage::OnSetActive();
}
