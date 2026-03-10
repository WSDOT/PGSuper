///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// SpecStrandPage.cpp : implementation file
//

#include "stdafx.h"
#include <PsgLib\PsgLib.h>
#include "SpecPSLimitPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>


/////////////////////////////////////////////////////////////////////////////
// CSpecStrandPage property page

IMPLEMENT_DYNCREATE(CSpecStrandPage, CPropertyPage)

CSpecStrandPage::CSpecStrandPage() : CPropertyPage(CSpecStrandPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecStrandPage)
	//}}AFX_DATA_INIT
}

CSpecStrandPage::~CSpecStrandPage()
{
}

void CSpecStrandPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecStrandPage)
	//}}AFX_DATA_MAP
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeStrandData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecStrandPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecStrandPage)
	ON_BN_CLICKED(IDC_CHECK_PS_AT_JACKING, OnPsChecked)
	ON_BN_CLICKED(IDC_CHECK_PS_BEFORE_TRANSFER, OnPsChecked)
	ON_BN_CLICKED(IDC_CHECK_PT_AT_JACKING, OnPtChecked)
	ON_BN_CLICKED(IDC_CHECK_PT_BEFORE_TRANSFER, OnPtChecked)
	ON_BN_CLICKED(IDC_CHECK_PS_AFTER_TRANSFER, OnCheckPsAfterTransfer)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecStrandPage message handlers

BOOL CSpecStrandPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   OnPsChecked();
   OnPtChecked();
   OnCheckPsAfterTransfer();

   // Capture the window rectangle for the Pull Method ratio
   // so we can restore the control to it's original position
   // if it gets moved for the 10th Edition of LRFD
   GetDlgItem(IDC_PULL_METHOD)->GetWindowRect(&ratio_rect);
   ScreenToClient(ratio_rect);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecStrandPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_PRESTRESSING );
}

void CSpecStrandPage::OnPsChecked() 
{
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AT_JACKING),IDC_PS_AT_JACKING_SR,IDC_PS_AT_JACKING_LR);
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_BEFORE_TRANSFER),IDC_PS_BEFORE_TRANSFER_SR,IDC_PS_BEFORE_TRANSFER_LR);
}

void CSpecStrandPage::OnPtChecked() 
{
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PT_AT_JACKING),IDC_PT_AT_JACKING_SR,IDC_PT_AT_JACKING_LR);
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PT_BEFORE_TRANSFER),IDC_PT_BEFORE_TRANSFER_SR,IDC_PT_BEFORE_TRANSFER_LR);
}

void CSpecStrandPage::EnableControls(BOOL bEnable,UINT nSR,UINT nLR)
{
   GetDlgItem(nSR)->EnableWindow(bEnable);
   GetDlgItem(nLR)->EnableWindow(bEnable);
}

void CSpecStrandPage::OnCheckPsAfterTransfer() 
{
	EnableControls(IsDlgButtonChecked(IDC_CHECK_PS_AFTER_TRANSFER),IDC_PS_AFTER_TRANSFER_SR,IDC_PS_AFTER_TRANSFER_LR);
}

BOOL CSpecStrandPage::OnSetActive()
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();

   // deal with 2017 crosswalk
   CWnd* pWnd = GetDlgItem(IDC_GPRES);
   pWnd->SetWindowText(CString(_T("Stress Limits for Prestressing (")) + WBFL::LRFD::LrfdCw8th(_T("5.9.3"),_T("5.9.2.2")) + _T(")"));

   pWnd = GetDlgItem(IDC_GPOST);
   pWnd->SetWindowText(CString(_T("Stress Limits for Post-tensioning (")) + WBFL::LRFD::LrfdCw8th(_T("5.9.3"),_T("5.9.2.2")) + _T(")"));

   if (WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= pDad->GetSpecVersion())
   {
      GetDlgItem(IDC_DUCT_SIZE_LABEL)->SetWindowText(_T("Nominal diameter of duct shall not exceed "));
   }
   else
   {
      GetDlgItem(IDC_DUCT_SIZE_LABEL)->SetWindowText(_T("Outside diameter of duct shall not exceed "));
   }

   int nCmdShow = WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= pDad->GetSpecVersion() ? SW_HIDE : SW_SHOW;
   GetDlgItem(IDC_PUSH_METHOD)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_PUSH_METHOD_DESC)->ShowWindow(nCmdShow);
   //GetDlgItem(IDC_PULL_METHOD)->ShowWindow(nCmdShow); // Always show this control
   GetDlgItem(IDC_PULL_METHOD_DESC)->ShowWindow(nCmdShow);

   if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= pDad->GetSpecVersion())
   {
	  CRect rect;
	  GetDlgItem(IDC_DUCT_AREA_RATIO_DESC)->GetWindowRect(&rect);
	  ScreenToClient(rect);
	  GetDlgItem(IDC_PULL_METHOD)->SetWindowPos(nullptr, rect.right + 5, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
   }
   else
   {
	  GetDlgItem(IDC_PULL_METHOD)->SetWindowPos(nullptr, ratio_rect.left, ratio_rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
   }

   return CPropertyPage::OnSetActive();
}
