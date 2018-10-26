///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// SpecBridgeSitePage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecBridgeSitePage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite3Page property page

IMPLEMENT_DYNCREATE(CSpecBridgeSite3Page, CPropertyPage)

CSpecBridgeSite3Page::CSpecBridgeSite3Page() : CPropertyPage(CSpecBridgeSite3Page::IDD)
{
	//{{AFX_DATA_INIT(CSpecBridgeSite3Page)
	//}}AFX_DATA_INIT
}

CSpecBridgeSite3Page::~CSpecBridgeSite3Page()
{
}

void CSpecBridgeSite3Page::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecBridgeSite3Page)
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeBsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecBridgeSite3Page, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecBridgeSite3Page)
	ON_BN_CLICKED(IDC_CHECK_NORMAL_MAX_MAX, OnCheckNormalMaxMax)
	ON_BN_CLICKED(IDC_CHECK_EXTREME_MAX_MAX, OnCheckExtremeMaxMax)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite3Page message handlers

void CSpecBridgeSite3Page::OnCheckNormalMaxMax() 
{
	DoCheckNormalMaxMax();
}

void CSpecBridgeSite3Page::OnCheckExtremeMaxMax() 
{
	DoCheckExtremeMaxMax();
}

void CSpecBridgeSite3Page::DoCheckNormalMaxMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_NORMAL_MAX_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_NORMAL_MAX_MAX_UNITS);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

void CSpecBridgeSite3Page::DoCheckExtremeMaxMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_EXTREME_MAX_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_EXTREME_MAX_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_EXTREME_MAX_MAX_UNITS);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}


BOOL CSpecBridgeSite3Page::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	DoCheckNormalMaxMax();
	DoCheckExtremeMaxMax();

   CWnd* pWnd = GetDlgItem(IDC_FATIGUE_LABEL);
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   if ( pDad->m_Entry.GetSpecificationType() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pWnd->SetWindowText(_T("Service IA (Live Load Plus One-Half of Permanent Loads) (LRFD 5.9.4.2.1)"));
   }
   else
   {
      pWnd->SetWindowText(_T("Fatigue I (Live Load Plus One-Half of Permanent Loads) (LRFD 5.5.3.1)"));
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// CGirderMiscPage message handlers
LRESULT CSpecBridgeSite3Page::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGE_SITE_TAB );
   return TRUE;
}

