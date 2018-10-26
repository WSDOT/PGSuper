///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// SpecDescrPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecDescrPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDescrPage property page

IMPLEMENT_DYNCREATE(CSpecDescrPage, CPropertyPage)

CSpecDescrPage::CSpecDescrPage() : CPropertyPage(CSpecDescrPage::IDD,IDS_SPEC_DESCRIPTION)
{
	//{{AFX_DATA_INIT(CSpecDescrPage)
	//}}AFX_DATA_INIT
}

CSpecDescrPage::~CSpecDescrPage()
{
}

void CSpecDescrPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecDescrPage)
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDescrData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecDescrPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDescrPage)
	ON_WM_CANCELMODE()
    ON_CBN_SELCHANGE(IDC_SPECIFICATION,OnSpecificationChanged)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDescrPage message handlers
LRESULT CSpecDescrPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPECIFICATION_DESCRIPTION_TAB );
   return TRUE;
}

BOOL CSpecDescrPage::OnInitDialog() 
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx;
   idx = pSpec->AddString("1st Edition, 1994");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FirstEdition1994);

   idx = pSpec->AddString("1st Edition, 1994 with 1996 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FirstEditionWith1996Interims);

   idx = pSpec->AddString("1st Edition, 1994 with 1996 and 1997 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FirstEditionWith1997Interims);

   idx = pSpec->AddString("2nd Edition, 1998");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEdition1998);

   idx = pSpec->AddString("2nd Edition, 1998 with 1999 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEditionWith1999Interims);

   idx = pSpec->AddString("2nd Edition, 1998 with 1999 - 2000 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEditionWith2000Interims);

   idx = pSpec->AddString("2nd Edition, 1998 with 1999 - 2001 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEditionWith2001Interims);

   idx = pSpec->AddString("2nd Edition, 1998 with 1999 - 2002 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEditionWith2002Interims);

   idx = pSpec->AddString("2nd Edition, 1998 with 1999 - 2003 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::SecondEditionWith2003Interims);

   idx = pSpec->AddString("3rd Edition, 2004");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::ThirdEdition2004);

   idx = pSpec->AddString("3rd Edition, 2004 with 2005 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::ThirdEditionWith2005Interims);

   idx = pSpec->AddString("3rd Edition, 2004 with 2005 - 2006 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::ThirdEditionWith2006Interims);

   idx = pSpec->AddString("4th Edition, 2007");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FourthEdition2007);

   idx = pSpec->AddString("4th Edition, 2007 with 2008 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FourthEditionWith2008Interims);

   idx = pSpec->AddString("4th Edition, 2007 with 2008 - 2009 interim provisions");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FourthEditionWith2009Interims);

   idx = pSpec->AddString("5th Edition, 2010");
   pSpec->SetItemData(idx,(DWORD)lrfdVersionMgr::FifthEdition2010);


   CPropertyPage::OnInitDialog();

   // enable/disable si setting for before 2007
   OnSpecificationChanged();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDescrPage::OnCancelMode() 
{
	CPropertyPage::OnCancelMode();
}

lrfdVersionMgr::Version CSpecDescrPage::GetSpecVersion()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   return (lrfdVersionMgr::Version)(pSpec->GetItemData(idx));
}

void CSpecDescrPage::OnSpecificationChanged()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   DWORD id = pSpec->GetItemData(idx);

   BOOL enable_si = TRUE;
   if (id > (DWORD)lrfdVersionMgr::ThirdEditionWith2006Interims)
   {
      CheckRadioButton(IDC_SPEC_UNITS_SI,IDC_SPEC_UNITS_US,IDC_SPEC_UNITS_US);
      enable_si = FALSE;
   }

   CButton* pSi = (CButton*)GetDlgItem(IDC_SPEC_UNITS_SI);
   pSi->EnableWindow(enable_si);
}