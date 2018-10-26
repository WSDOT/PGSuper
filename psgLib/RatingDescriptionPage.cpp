///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// RatingDescriptionPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "RatingDescriptionPage.h"
#include "RatingDialog.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRatingDescriptionPage property page

IMPLEMENT_DYNCREATE(CRatingDescriptionPage, CPropertyPage)

CRatingDescriptionPage::CRatingDescriptionPage() : CPropertyPage(CRatingDescriptionPage::IDD)
{
	//{{AFX_DATA_INIT(CRatingDescriptionPage)
	//}}AFX_DATA_INIT
}

CRatingDescriptionPage::~CRatingDescriptionPage()
{
}

void CRatingDescriptionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRatingDescriptionPage)
	//}}AFX_DATA_MAP

   CRatingDialog* pDad = (CRatingDialog*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDescriptionData(pDX);

}


BEGIN_MESSAGE_MAP(CRatingDescriptionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRatingDescriptionPage)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_SPECIFICATION,OnSpecificationChanged)
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRatingDescriptionPage message handlers
void CRatingDescriptionPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_LOAD_RATING_CRITERIA );
}

BOOL CRatingDescriptionPage::OnInitDialog() 
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx;
   for ( int i = 1; i < (int)lrfrVersionMgr::LastVersion; i++ )
   {
      idx = pSpec->AddString(lrfrVersionMgr::GetVersionString((lrfrVersionMgr::Version)(i)));
      pSpec->SetItemData(idx,(DWORD)(i));
   }

   CPropertyPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

lrfrVersionMgr::Version CRatingDescriptionPage::GetSpecVersion()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   return (lrfrVersionMgr::Version)(pSpec->GetItemData(idx));
}

void CRatingDescriptionPage::OnSpecificationChanged()
{
   CRatingDialog* pParent = (CRatingDialog*)GetParent();
   pParent->UpdatePageLayout();
}