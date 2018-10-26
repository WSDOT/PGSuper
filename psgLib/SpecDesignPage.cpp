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

// SpecDesignPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecDesignPage.h"

#include <EAF\EAFApp.h>

#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage dialog
IMPLEMENT_DYNCREATE(CSpecDesignPage, CPropertyPage)


CSpecDesignPage::CSpecDesignPage(CWnd* pParent /*=NULL*/)
	: CPropertyPage(CSpecDesignPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDesignPage)
   //}}AFX_DATA_INIT
}


void CSpecDesignPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   // dad is a friend of the entry. use him to transfer data.
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();

   pDad->ExchangeDesignData(pDX);

	//{{AFX_DATA_MAP(CSpecDesignPage)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpecDesignPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDesignPage)
	ON_BN_CLICKED(IDC_CHECK_A, OnCheckA)
	ON_BN_CLICKED(IDC_CHECK_HAULING, OnCheckHauling)
	ON_BN_CLICKED(IDC_CHECK_HD, OnCheckHd)
	ON_BN_CLICKED(IDC_CHECK_LIFTING, OnCheckLifting)
	ON_BN_CLICKED(IDC_CHECK_SLOPE, OnCheckSlope)
	ON_BN_CLICKED(IDC_CHECK_SPLITTING, OnCheckSplitting)
	ON_BN_CLICKED(IDC_CHECK_CONFINEMENT, OnCheckConfinement)
   ON_BN_CLICKED(IDC_IS_SUPPORT_LESS_THAN,OnBnClickedIsSupportLessThan)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
   ON_BN_CLICKED(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE, &CSpecDesignPage::OnBnClickedCheckBottomFlangeClearance)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage message handlers
BOOL CSpecDesignPage::OnInitDialog()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();
	
   // set statics for strand slope
   CString sl05, sl06, sl07;
   if (pApp->GetUnitsMode() == eafTypes::umSI)
   {
      VERIFY(sl05.LoadString(IDS_SLOPE_O5_SI));
      VERIFY(sl06.LoadString(IDS_SLOPE_O6_SI));
      VERIFY(sl07.LoadString(IDS_SLOPE_O7_SI));
   }
   else
   {
      VERIFY(sl05.LoadString(IDS_SLOPE_O5_US));
      VERIFY(sl06.LoadString(IDS_SLOPE_O6_US));
      VERIFY(sl07.LoadString(IDS_SLOPE_O7_US));
   }

   CPropertyPage::OnInitDialog();

   OnCheckA();
   OnCheckHauling();
   OnCheckHd();
   OnCheckLifting();
   OnCheckSlope();
   OnCheckSplitting();
   OnCheckConfinement();

   OnBnClickedIsSupportLessThan();

   OnBnClickedCheckBottomFlangeClearance();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// Don't allow design if check is disabled.
inline void CheckDesignCtrl(int idc, int idd, CWnd* pme)
{
	CButton* pbut = (CButton*)pme->GetDlgItem(idc);
 	CButton* pbut2 = (CButton*)pme->GetDlgItem(idd);
   if (pbut->GetCheck()==0)
   {
      pbut2->SetCheck(0);
      pbut2->EnableWindow(FALSE);
   }
   else
   {
      pbut2->EnableWindow(TRUE);
   }

}

void CSpecDesignPage::OnCheckA() 
{
   CheckDesignCtrl(IDC_CHECK_A, IDC_DESIGN_A, this);
}

void CSpecDesignPage::OnCheckHauling() 
{
   CheckDesignCtrl(IDC_CHECK_HAULING, IDC_DESIGN_HAULING, this);
}

void CSpecDesignPage::OnCheckHd() 
{
   CheckDesignCtrl(IDC_CHECK_HD, IDC_DESIGN_HD, this);

}

void CSpecDesignPage::OnCheckLifting() 
{
   CheckDesignCtrl(IDC_CHECK_LIFTING, IDC_DESIGN_LIFTING, this);
}

void CSpecDesignPage::OnCheckSlope() 
{
   CheckDesignCtrl(IDC_CHECK_SLOPE, IDC_DESIGN_SLOPE, this);
}

void CSpecDesignPage::OnCheckSplitting() 
{
   CheckDesignCtrl(IDC_CHECK_SPLITTING, IDC_DESIGN_SPLITTING, this);
}

void CSpecDesignPage::OnCheckConfinement() 
{
   CheckDesignCtrl(IDC_CHECK_CONFINEMENT, IDC_DESIGN_CONFINEMENT, this);
}


LRESULT CSpecDesignPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_DESIGN );
   return TRUE;
}

void CSpecDesignPage::OnBnClickedIsSupportLessThan()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_IS_SUPPORT_LESS_THAN);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);

   pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN_UNIT);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CSpecDesignPage::OnBnClickedCheckBottomFlangeClearance()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE);
   GetDlgItem(IDC_CLEARANCE_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE_UNIT)->EnableWindow(bEnable);
}
