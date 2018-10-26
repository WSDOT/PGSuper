///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// SpecCastingYardPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecCastingYardPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecCastingYardPage property page

IMPLEMENT_DYNCREATE(CSpecCastingYardPage, CPropertyPage)

CSpecCastingYardPage::CSpecCastingYardPage() : CPropertyPage(CSpecCastingYardPage::IDD,IDS_SPEC_CASTING_YARD)
{
	//{{AFX_DATA_INIT(CSpecCastingYardPage)
	//}}AFX_DATA_INIT
}

CSpecCastingYardPage::~CSpecCastingYardPage()
{
}

void CSpecCastingYardPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecCastingYardPage)
	DDX_Control(pDX, IDC_STATIC_SLOPE_07, m_StaticSlope07);
	DDX_Control(pDX, IDC_STATIC_SLOPE_06, m_StaticSlope06);
	DDX_Control(pDX, IDC_STATIC_SLOPE_05, m_StaticSlope05);
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeCyData(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      DoCheckStrandSlope();
      DoCheckHoldDown();
      DoCheckMax();
      DoCheckAnchorage();

      CEdit* pnote = (CEdit*)GetDlgItem(IDC_SS_NOTE);
      if (!m_DoCheckStrandSlope)
      {
         pnote->SetWindowText(_T("Strand Slope Check is Disabled on Design Tab"));
      }
      else
      {
         pnote->SetWindowText(_T("Strand Slope Check is Enabled on Design Tab"));
      }

      pnote = (CEdit*)GetDlgItem(IDC_HD_NOTE);
      if (!m_DoCheckHoldDown)
      {
         pnote->SetWindowText(_T("Strand Hold Down Check is Disabled on Design Tab"));
      }
      else
      {
         pnote->SetWindowText(_T("Strand Hold Down Check is Enabled on Design Tab"));
      }

      pnote = (CEdit*)GetDlgItem(IDC_ANCHORAGE_NOTE);
      if (!m_DoCheckAnchorage)
      {
         pnote->SetWindowText(_T("Anchorage Check (5.10.10) is Disabled on Design Tab"));
      }
      else
      {
         pnote->SetWindowText(_T("Anchorage Check (5.10.10) is Enabled on Design Tab"));
      }
   }
}


BEGIN_MESSAGE_MAP(CSpecCastingYardPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecCastingYardPage)
	ON_BN_CLICKED(IDC_CHECK_NORMAL_MAX_MAX2, OnCheckMax)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecCastingYardPage message handlers
BOOL CSpecCastingYardPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   DoCheckHoldDown();
   DoCheckStrandSlope();
	DoCheckMax();
   DoCheckAnchorage();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecCastingYardPage::DoCheckStrandSlope()
{
   BOOL ischk = m_DoCheckStrandSlope;

   CWnd* pwnd = GetDlgItem(IDC_STRAND_SLOPE_05);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_STRAND_SLOPE_06);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_STRAND_SLOPE_07);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}


void CSpecCastingYardPage::DoCheckHoldDown()
{
   BOOL ischk = m_DoCheckHoldDown;

   CWnd* pwnd = GetDlgItem(IDC_HOLD_DOWN_FORCE);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HOLD_DOWN_FORCE_UNITS);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CSpecCastingYardPage::DoCheckAnchorage()
{
   BOOL ischk = m_DoCheckAnchorage;

   CWnd* pwnd = GetDlgItem(IDC_N);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

LRESULT CSpecCastingYardPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CASTING_YARD_TAB );
   return TRUE;
}


void CSpecCastingYardPage::OnCheckMax() 
{
	DoCheckMax();
}


void CSpecCastingYardPage::DoCheckMax()
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

HBRUSH CSpecCastingYardPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   if (pWnd->GetDlgCtrlID() == IDC_SS_NOTE || 
       pWnd->GetDlgCtrlID() == IDC_HD_NOTE ||
       pWnd->GetDlgCtrlID() == IDC_ANCHORAGE_NOTE)
   {
      pDC->SetTextColor(RGB(255, 0, 0));
   }

	return hbr;
}
