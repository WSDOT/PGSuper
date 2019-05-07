///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// SpecLiveLoadsPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecLiveLoadsPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CSpecLiveLoadsPage dialog

IMPLEMENT_DYNAMIC(CSpecLiveLoadsPage, CPropertyPage)

CSpecLiveLoadsPage::CSpecLiveLoadsPage()
	: CPropertyPage(CSpecLiveLoadsPage::IDD)
{

}

CSpecLiveLoadsPage::~CSpecLiveLoadsPage()
{
}

void CSpecLiveLoadsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLiveLoadsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecLiveLoadsPage, CPropertyPage)
   ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_CBN_SELCHANGE(IDC_LLDF, &CSpecLiveLoadsPage::OnCbnSelchangeLldf)
END_MESSAGE_MAP()


// CSpecLiveLoadsPage message handlers

BOOL CSpecLiveLoadsPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   pCB->AddString(_T("Compute in accordance with LRFD 4.6.2.2"));
   pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual"));
   pCB->AddString(_T("Compute in accordance with TxDOT Bridge Design Manual"));

   CPropertyPage::OnInitDialog();

   OnCbnSelchangeLldf();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecLiveLoadsPage::OnSetActive()
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   int nCmdShow = (lrfdVersionMgr::SeventhEdition2014 <= pDad->m_Entry.GetSpecificationType()) ? SW_SHOW : SW_HIDE;
   GetDlgItem(IDC_RIGID_METHOD)->ShowWindow(nCmdShow);
   return CPropertyPage::OnSetActive();
}

void CSpecLiveLoadsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LIVE_LOADS );
}

void CSpecLiveLoadsPage::OnCbnSelchangeLldf()
{
   // TODO: Add your control notification handler code here
   CWnd* pOption = GetDlgItem(IDC_RIGID_METHOD);
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   auto curSel = pCB->GetCurSel();
   pOption->ShowWindow(curSel == 0 ? SW_SHOW : SW_HIDE);
   // this option is never used by WSDOT or TxDOT methods
}
