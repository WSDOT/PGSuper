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

// SpecLossPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecLossPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLossPage property page

IMPLEMENT_DYNCREATE(CSpecLossPage, CPropertyPage)

CSpecLossPage::CSpecLossPage() : CPropertyPage(CSpecLossPage::IDD,IDS_SPEC_LOSS)
{
	//{{AFX_DATA_INIT(CSpecLossPage)
	//}}AFX_DATA_INIT
}

CSpecLossPage::~CSpecLossPage()
{
}

void CSpecLossPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecLossPage)
	//}}AFX_DATA_MAP
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLossData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecLossPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLossPage)
	ON_CBN_SELCHANGE(IDC_LOSS_METHOD, OnLossMethodChanged)
	ON_CBN_SELCHANGE(IDC_SHIPPING_LOSS_METHOD, OnShippingLossMethodChanged)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLossPage message handlers

void CSpecLossPage::OnLossMethodChanged() 
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   int method = pBox->GetCurSel();

   if ( 0 <= method && method < 5 )
   {
      EnableShippingLosses(m_SpecVersion <= lrfdVersionMgr::ThirdEdition2004 ? TRUE : FALSE);
      EnableRefinedShippingTime(lrfdVersionMgr::ThirdEdition2004 < m_SpecVersion && (method == 0 || method == 1 || method == 2) ? TRUE : FALSE);
      EnableApproximateShippingTime(lrfdVersionMgr::ThirdEdition2004 < m_SpecVersion && (method == 3 || method == 4) ? TRUE : FALSE);
      EnableGeneralLumpSum(FALSE);
   }
   else
   {
      EnableShippingLosses(FALSE);
      EnableRefinedShippingTime(FALSE);
      EnableApproximateShippingTime(FALSE);
      EnableGeneralLumpSum(TRUE);
   }
}

#define ENABLE_WINDOW(x) pWnd = GetDlgItem(x); pWnd->EnableWindow(bEnable); pWnd->ShowWindow(bEnable ? SW_SHOW : SW_HIDE)
void CSpecLossPage::EnableShippingLosses(BOOL bEnable)
{
   CWnd* pWnd;

   ENABLE_WINDOW(IDC_SHIPPING_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING);
   ENABLE_WINDOW(IDC_SHIPPING_TAG);

   ENABLE_WINDOW(IDC_SHIPPING_LOSS_METHOD);
}

void CSpecLossPage::EnableGeneralLumpSum(BOOL bEnable)
{
   CWnd* pWnd;

   ENABLE_WINDOW(IDC_BEFORE_XFER_LABEL);
   ENABLE_WINDOW(IDC_BEFORE_XFER);
   ENABLE_WINDOW(IDC_BEFORE_XFER_TAG);

   ENABLE_WINDOW(IDC_AFTER_XFER_LABEL);
   ENABLE_WINDOW(IDC_AFTER_XFER);
   ENABLE_WINDOW(IDC_AFTER_XFER_TAG);

   ENABLE_WINDOW(IDC_LIFTING_LABEL);
   ENABLE_WINDOW(IDC_LIFTING);
   ENABLE_WINDOW(IDC_LIFTING_TAG);

   ENABLE_WINDOW(IDC_SHIPPING2_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING2);
   ENABLE_WINDOW(IDC_SHIPPING2_TAG);

   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL_LABEL);
   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL);
   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL_TAG);

   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL_LABEL);
   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL);
   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL_TAG);

   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT_LABEL);
   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT);
   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT_TAG);

   ENABLE_WINDOW(IDC_FINAL_LABEL);
   ENABLE_WINDOW(IDC_FINAL);
   ENABLE_WINDOW(IDC_FINAL_TAG);
}

void CSpecLossPage::EnableRefinedShippingTime(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_SHIPPING_TIME_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING_TIME);
   ENABLE_WINDOW(IDC_SHIPPING_TIME_TAG);
}

void CSpecLossPage::EnableApproximateShippingTime(BOOL bEnable)
{
   CWnd* pWnd;
   ENABLE_WINDOW(IDC_APPROXIMATE_SHIPPING_TIME_NOTE);
}

BOOL CSpecLossPage::OnInitDialog() 
{
   // Fill up combo box for loss methods
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_LOSS_METHOD);
   pBox->AddString("Refined Estimate per LRFD 5.9.5.4");
   pBox->AddString("Refined Estimate per WSDOT Bridge Design Manual");
   pBox->AddString("Refined Estimate per TxDOT Bridge Design Manual");
   pBox->AddString("Approximate Lump Sum per LRFD 5.9.5.3");
   pBox->AddString("Approximate Lump Sum per WSDOT Bridge Design Manual");
   pBox->AddString("General Lump Sum");
   pBox->SetCurSel(0);

   pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   pBox->AddString("Use a lump sum loss for shipping");
   pBox->AddString("Use a percentage of the final losses for shipping");
   pBox->SetCurSel(0);

   CPropertyPage::OnInitDialog();


   OnLossMethodChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecLossPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_LOSSES );
   return TRUE;
}

BOOL CSpecLossPage::IsFractionalShippingLoss()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_SHIPPING_LOSS_METHOD);
   int curSel = pBox->GetCurSel();

   return curSel == 1 ? TRUE : FALSE;
}

void CSpecLossPage::OnShippingLossMethodChanged() 
{
   CWnd* pTag = GetDlgItem(IDC_SHIPPING_TAG);

	if ( IsFractionalShippingLoss() )
   {
      pTag->SetWindowText("%");
   }
   else
   {
      CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
      bool bUnitsSI = (pDad->m_Mode == libUnitsMode::UNITS_SI ? true : false);
      const unitPressure& unit = (bUnitsSI ? unitMeasure::MPa : unitMeasure::KSI);
      pTag->SetWindowText(unit.UnitTag().c_str());
   }
}

BOOL CSpecLossPage::OnSetActive() 
{

   // if this is third edition or earlier, enable the shipping loss controls
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   m_SpecVersion = pDad->m_Entry.GetSpecificationType();

   OnLossMethodChanged();

   return CPropertyPage::OnSetActive();
}
