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

// SpecDeadLoadsPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecDeadLoadsPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CSpecDeadLoadsPage dialog

IMPLEMENT_DYNAMIC(CSpecDeadLoadsPage, CPropertyPage)

CSpecDeadLoadsPage::CSpecDeadLoadsPage()
	: CPropertyPage(CSpecDeadLoadsPage::IDD)
{

}

CSpecDeadLoadsPage::~CSpecDeadLoadsPage()
{
}

void CSpecDeadLoadsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DIST_TRAFFIC_BARRIER_SPIN, m_TrafficSpin);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDeadLoadsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecDeadLoadsPage, CPropertyPage)
   ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()


// CSpecDeadLoadsPage message handlers

BOOL CSpecDeadLoadsPage::OnInitDialog()
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

   CPropertyPage::OnInitDialog();

   m_TrafficSpin.SetRange(0,100);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDeadLoadsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_DEAD_LOADS );
}

