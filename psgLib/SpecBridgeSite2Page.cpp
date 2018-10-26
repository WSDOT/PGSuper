///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite2Page message handlers

BOOL CSpecBridgeSite2Page::OnInitDialog() 
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   lrfdVersionMgr::Version version = pDad->GetSpecVersion();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EFF_FLANGE_WIDTH);
   pCB->AddString("in accordance with LRFD 4.6.2.6");

   if ( version < lrfdVersionMgr::FourthEditionWith2008Interims )
      pCB->AddString("using the tributary width");

   pCB = (CComboBox*)GetDlgItem(IDC_DIST_TRAFFIC_BARRIER_BASIS);
   int index = pCB->AddString("girders");
   pCB->SetItemData(index,pgsTypes::tbdGirder);
   index = pCB->AddString("mating surfaces");
   pCB->SetItemData(index,pgsTypes::tbdMatingSurface);
   index = pCB->AddString("webs");
   pCB->SetItemData(index,pgsTypes::tbdWebs);

   pCB = (CComboBox*)GetDlgItem(IDC_OVERLAY_DISTR);
   pCB->AddString("Uniformly Among All Girders [LRFD 4.6.2.2.1]");
   pCB->AddString("Using Tributary Width");

   CPropertyPage::OnInitDialog();

   m_TrafficSpin.SetRange(0,100);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecBridgeSite2Page::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGESITE_2_TAB );
   return TRUE;
}
