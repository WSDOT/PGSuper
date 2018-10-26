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

// SpecLiftingPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecLiftingPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLiftingPage property page

IMPLEMENT_DYNCREATE(CSpecLiftingPage, CPropertyPage)

CSpecLiftingPage::CSpecLiftingPage() : CPropertyPage(CSpecLiftingPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecLiftingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecLiftingPage::~CSpecLiftingPage()
{
}

void CSpecLiftingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecLiftingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLiftingData(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      CEdit* pnote = (CEdit*)GetDlgItem(IDC_LIFTING_NOTE);
      if (!m_IsLiftingEnabled)
      {
         HideLiftingControls(true);
         pnote->SetWindowText("Lifting Check is Disabled on Design Tab");
      }
      else
      {
         HideLiftingControls(false);
         pnote->SetWindowText("Lifting Check is Enabled on Design Tab");

	      DoCheckMax();
      }
   }
}


BEGIN_MESSAGE_MAP(CSpecLiftingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLiftingPage)
	ON_BN_CLICKED(IDC_CHECK_LIFTING_NORMAL_MAX_MAX, OnCheckLiftingNormalMaxMax)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLiftingPage message handlers

void CSpecLiftingPage::OnCheckLiftingNormalMaxMax() 
{
	DoCheckMax();
}

void CSpecLiftingPage::DoCheckMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_LIFTING_NORMAL_MAX_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_LIFTING_NORMAL_MAX_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_LIFTING_NORMAL_MAX_MAX_UNITS);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

BOOL CSpecLiftingPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecLiftingPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_LIFTING );
   return TRUE;
}


BOOL CSpecLiftingPage::EnableWindows(HWND hwnd,LPARAM lParam)
{
   ::EnableWindow(hwnd,(BOOL)lParam);
   return TRUE;
}

void CSpecLiftingPage::HideLiftingControls(bool hide)
{
   bool enable = !hide;

   // disable all of the windows
   EnumChildWindows(GetSafeHwnd(),CSpecLiftingPage::EnableWindows,(LPARAM)enable);
   GetDlgItem(IDC_LIFTING_NOTE)->EnableWindow(TRUE); // always enable the note at the top of the page
}


HBRUSH CSpecLiftingPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   if (pWnd->GetDlgCtrlID() == IDC_LIFTING_NOTE)
   {
      pDC->SetTextColor(RGB(255, 0, 0));
   }	

	return hbr;
}
