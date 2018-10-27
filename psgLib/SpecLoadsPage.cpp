///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// SpecLoadsPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecLoadsPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>

// CSpecLoadsPage dialog

IMPLEMENT_DYNAMIC(CSpecLoadsPage, CPropertyPage)

CSpecLoadsPage::CSpecLoadsPage()
	: CPropertyPage(CSpecLoadsPage::IDD)
{

}

CSpecLoadsPage::~CSpecLoadsPage()
{
}

void CSpecLoadsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DIST_TRAFFIC_BARRIER_SPIN, m_TrafficSpin);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLoadsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecLoadsPage, CPropertyPage)
   ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_CBN_SELCHANGE(IDC_HAUNCH_COMP_CB, &CSpecLoadsPage::OnCbnSelchangeHaunchCompCb)
END_MESSAGE_MAP()


// CSpecLoadsPage message handlers

BOOL CSpecLoadsPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DIST_TRAFFIC_BARRIER_BASIS);
   int index = pCB->AddString(_T("nearest girders"));
   pCB->SetItemData(index,pgsTypes::tbdGirder);
   index = pCB->AddString(_T("nearest mating surfaces"));
   pCB->SetItemData(index,pgsTypes::tbdMatingSurface);
   index = pCB->AddString(_T("nearest webs"));
   pCB->SetItemData(index,pgsTypes::tbdWebs);

   pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_DISTR);
   pCB->AddString(_T("Uniformly Among All Girders [LRFD 4.6.2.2.1]"));
   pCB->AddString(_T("Using Tributary Width"));

   pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   pCB->AddString(_T("Compute in accordance with LRFD 4.6.2.2"));
   pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual"));
   pCB->AddString(_T("Compute in accordance with TxDOT Bridge Design Manual"));

   CPropertyPage::OnInitDialog();

   m_TrafficSpin.SetRange(0,100);

   OnCbnSelchangeHaunchCompCb();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLoadsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LOADS );
}

void CSpecLoadsPage::OnCbnSelchangeHaunchCompCb()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_HAUNCH_COMP_CB);
   int idx = pBox->GetCurSel();
   BOOL enable = idx==(int)pgsTypes::hlcAccountForCamber;

   GetDlgItem(IDC_HAUNCH_TOLER_STATIC)->EnableWindow(enable);
   GetDlgItem(IDC_HAUNCH_TOLER_UNIT)->EnableWindow(enable);
   GetDlgItem(IDC_HAUNCH_TOLER)->EnableWindow(enable);

   GetDlgItem(IDC_HAUNCH_FACTOR)->EnableWindow(enable);
   GetDlgItem(IDC_HAUNCH_FACTOR1)->EnableWindow(enable);
   GetDlgItem(IDC_HAUNCH_FACTOR2)->EnableWindow(enable);
}
