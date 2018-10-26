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

// SpecHaulingErectionPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecHaulingErectionPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecHaulingErectionPage property page

IMPLEMENT_DYNCREATE(CSpecHaulingErectionPage, CPropertyPage)

CSpecHaulingErectionPage::CSpecHaulingErectionPage() : CPropertyPage(CSpecHaulingErectionPage::IDD,IDS_HAULING_ERECTION)
{
	//{{AFX_DATA_INIT(CSpecHaulingErectionPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecHaulingErectionPage::~CSpecHaulingErectionPage()
{
}

void CSpecHaulingErectionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecHaulingErectionPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeHaulingData(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      CEdit* pnote = (CEdit*)GetDlgItem(IDC_ENABLE_NOTE);
      if (!m_IsHaulingEnabled)
      {
         HideControls(true);
         pnote->SetWindowText("Hauling Check is Disabled on Design Tab");
      }
      else
      {
         HideControls(false);
         pnote->SetWindowText("Hauling Check is Enabled on Design Tab");

	      DoCheckMax();

         int method = GetCheckedRadioButton(IDC_LUMPSUM_METHOD,IDC_PERAXLE_METHOD);
         if ( method == IDC_LUMPSUM_METHOD )
            OnLumpSumMethod();
         else
            OnPerAxleMethod();

      }
   }
}


BEGIN_MESSAGE_MAP(CSpecHaulingErectionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecHaulingErectionPage)
	ON_BN_CLICKED(IDC_CHECK_HAULING_TENS_MAX, OnCheckHaulingTensMax)
	ON_BN_CLICKED(IDC_LUMPSUM_METHOD, OnLumpSumMethod)
	ON_BN_CLICKED(IDC_PERAXLE_METHOD, OnPerAxleMethod)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecHaulingErectionPage message handlers
LRESULT CSpecHaulingErectionPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_HAULING_AND_ERECTION_TAB );
   return TRUE;
}


void CSpecHaulingErectionPage::OnCheckHaulingTensMax() 
{
	DoCheckMax();
	
}

void CSpecHaulingErectionPage::DoCheckMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENS_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_HAULING_TENS_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HAULING_TENS_MAX_UNITS);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

BOOL CSpecHaulingErectionPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingErectionPage::OnLumpSumMethod() 
{
   EnableLumpSumMethod(TRUE);
}

void CSpecHaulingErectionPage::OnPerAxleMethod() 
{
   EnableLumpSumMethod(FALSE);
}

void CSpecHaulingErectionPage::EnableLumpSumMethod(BOOL bEnable)
{
   GetDlgItem(IDC_ROLL_STIFFNESS)->EnableWindow(bEnable);
   GetDlgItem(IDC_ROLL_STIFFNESS_UNITS)->EnableWindow(bEnable);

   GetDlgItem(IDC_AXLE_WEIGHT)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_WEIGHT_UNITS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_STIFFNESS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_STIFFNESS_UNITS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_MIN_ROLL_STIFFNESS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_MIN_ROLL_STIFFNESS_UNITS)->EnableWindow(!bEnable);
}

BOOL CSpecHaulingErectionPage::EnableWindows(HWND hwnd,LPARAM lParam)
{
   ::EnableWindow(hwnd,(BOOL)lParam);
   return TRUE;
}

void CSpecHaulingErectionPage::HideControls(bool hide)
{
   bool enable = !hide;

   // disable all of the windows
   EnumChildWindows(GetSafeHwnd(),CSpecHaulingErectionPage::EnableWindows,(LPARAM)enable);


   // if controls are enabled, then get the lump sup/per axle roll stiffness parameters enabled connrctly
   if (enable)
   {
      int method = GetCheckedRadioButton(IDC_LUMPSUM_METHOD,IDC_PERAXLE_METHOD);
      if ( method == IDC_LUMPSUM_METHOD )
         OnLumpSumMethod();
      else
         OnPerAxleMethod();
   }

   // always enable these two notes
   GetDlgItem(IDC_ENABLE_NOTE)->EnableWindow(TRUE);
   GetDlgItem(IDC_LOSSES_NOTE)->EnableWindow(TRUE);
}

HBRUSH CSpecHaulingErectionPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   if (pWnd->GetDlgCtrlID() == IDC_ENABLE_NOTE)
   {
      pDC->SetTextColor(RGB(255, 0, 0));
   }	

	return hbr;
}
