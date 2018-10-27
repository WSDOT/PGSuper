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
END_MESSAGE_MAP()


// CSpecLiveLoadsPage message handlers

BOOL CSpecLiveLoadsPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LLDF);
   pCB->AddString(_T("Compute in accordance with LRFD 4.6.2.2"));
   pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual"));
   pCB->AddString(_T("Compute in accordance with TxDOT Bridge Design Manual"));

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiveLoadsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LIVE_LOADS );
}
