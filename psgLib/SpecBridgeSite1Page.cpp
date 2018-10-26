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

// SpecBridgeSite1Page.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecBridgeSite1Page.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite1Page property page

IMPLEMENT_DYNCREATE(CSpecBridgeSite1Page, CPropertyPage)

CSpecBridgeSite1Page::CSpecBridgeSite1Page() : CPropertyPage(CSpecBridgeSite1Page::IDD)
{
	//{{AFX_DATA_INIT(CSpecBridgeSite1Page)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecBridgeSite1Page::~CSpecBridgeSite1Page()
{
}

void CSpecBridgeSite1Page::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecBridgeSite1Page)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeBs1Data(pDX);

}


BEGIN_MESSAGE_MAP(CSpecBridgeSite1Page, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecBridgeSite1Page)
	ON_BN_CLICKED(IDC_CHECK_NORMAL_MAX_MAX2, OnCheckNormalMaxMax2)
	ON_BN_CLICKED(IDC_CHECK_NORMAL_MAX_MAX3, OnCheckNormalMaxMax3)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite1Page message handlers

BOOL CSpecBridgeSite1Page::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	DoCheckMaxMax();
	DoCheckMaxMax3();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecBridgeSite1Page::OnCheckNormalMaxMax2() 
{
	DoCheckMaxMax();
}

void CSpecBridgeSite1Page::OnCheckNormalMaxMax3() 
{
	DoCheckMaxMax3();
}

void CSpecBridgeSite1Page::DoCheckMaxMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_NORMAL_MAX_MAX2);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX2);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX_UNITS2);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecBridgeSite1Page::DoCheckMaxMax3()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_NORMAL_MAX_MAX3);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX3);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX_UNITS3);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

LRESULT CSpecBridgeSite1Page::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGESITE_1_TAB );
   return TRUE;
}
