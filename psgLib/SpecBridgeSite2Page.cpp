///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// SpecBridgeSite2Page.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecBridgeSite2Page.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"
#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite2Page property page

IMPLEMENT_DYNCREATE(CSpecBridgeSite2Page, CPropertyPage)

CSpecBridgeSite2Page::CSpecBridgeSite2Page() : CPropertyPage(CSpecBridgeSite2Page::IDD)
{
	//{{AFX_DATA_INIT(CSpecBridgeSite2Page)
	//}}AFX_DATA_INIT
}

CSpecBridgeSite2Page::~CSpecBridgeSite2Page()
{
}

void CSpecBridgeSite2Page::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecBridgeSite2Page)
	DDX_Control(pDX, IDC_DIST_TRAFFIC_BARRIER_SPIN, m_TrafficSpin);
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeBs2Data(pDX);
}


BEGIN_MESSAGE_MAP(CSpecBridgeSite2Page, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecBridgeSite2Page)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_CHECK_TENSION, &CSpecBridgeSite2Page::OnBnClickedCheckTension)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite2Page message handlers

BOOL CSpecBridgeSite2Page::OnInitDialog() 
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   lrfdVersionMgr::Version version = pDad->GetSpecVersion();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EFF_FLANGE_WIDTH);
   pCB->AddString(_T("in accordance with LRFD 4.6.2.6"));

   if ( version < lrfdVersionMgr::FourthEditionWith2008Interims )
      pCB->AddString(_T("using the tributary width"));

   pCB = (CComboBox*)GetDlgItem(IDC_DIST_TRAFFIC_BARRIER_BASIS);
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

   OnBnClickedCheckTension();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecBridgeSite2Page::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGESITE_2_TAB );
   return TRUE;
}

void CSpecBridgeSite2Page::OnBnClickedCheckTension()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = (IsDlgButtonChecked(IDC_CHECK_TENSION) == BST_CHECKED ? TRUE : FALSE);
   GetDlgItem(IDC_NORMAL_MAX_SQRT3)->EnableWindow(bEnable);
   GetDlgItem(IDC_CYS_TENS_BYLINE2)->EnableWindow(bEnable);
   GetDlgItem(IDC_CHECK_NORMAL_MAX_MAX3)->EnableWindow(bEnable);
   GetDlgItem(IDC_NORMAL_MAX_MAX3)->EnableWindow(bEnable);
}
