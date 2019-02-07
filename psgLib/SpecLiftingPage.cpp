///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <EAF\EAFDocument.h>

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
}


BEGIN_MESSAGE_MAP(CSpecLiftingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLiftingPage)
	ON_BN_CLICKED(IDC_CHECK_LIFTING_TENSION_MAX, OnCheckLiftingNormalMaxMax)
	ON_BN_CLICKED(ID_HELP,OnHelp)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_WIND_TYPE, &CSpecLiftingPage::OnCbnSelchangeWindType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLiftingPage message handlers

void CSpecLiftingPage::OnCheckLiftingNormalMaxMax() 
{
	DoCheckMax();
}

void CSpecLiftingPage::DoCheckMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_LIFTING_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_LIFTING_TENSION_MAX);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_LIFTING_TENSION_MAX_UNIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

BOOL CSpecLiftingPage::OnInitDialog() 
{
   CComboBox* pcbStresses = (CComboBox*)GetDlgItem(IDC_STRESSES);
   pcbStresses->SetItemData(pcbStresses->AddString(_T("Include girder stability equilibrium angle in stress calculations")),(DWORD_PTR)true);
   pcbStresses->SetItemData(pcbStresses->AddString(_T("Ignore girder stability equilibrium angle in stress calculations")),(DWORD_PTR)false);

   CComboBox* pcbWind = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   pcbWind->SetItemData(pcbWind->AddString(_T("Pressure")),(DWORD_PTR)pgsTypes::Pressure);
   pcbWind->SetItemData(pcbWind->AddString(_T("Speed")),   (DWORD_PTR)pgsTypes::Speed);

   CPropertyPage::OnInitDialog();
	
	DoCheckMax();
   OnCbnSelchangeWindType();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecLiftingPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_LIFTING );
}

BOOL CSpecLiftingPage::OnSetActive()
{
   // Disable controls if hauling not enabled
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   BOOL enableChild = pDad->m_Entry.IsLiftingAnalysisEnabled() ? TRUE : FALSE;
   EnableControls(enableChild);

   if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= pDad->m_Entry.GetSpecificationType() )
   {
      GetDlgItem(IDC_SLWC_FR_TXT)->SetWindowText(_T("Lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_TXT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_HIDE);
   }
   else
   {
      GetDlgItem(IDC_SLWC_FR_TXT)->SetWindowText(_T("Sand lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_TXT)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_SHOW);
   }

   return CPropertyPage::OnSetActive();
}

inline BOOL CALLBACK EnableChildWindow(HWND hwnd,LPARAM lParam)
{
   ::EnableWindow(hwnd,(int)lParam);
   return TRUE;
}

void CSpecLiftingPage::EnableControls(BOOL bEnable)
{
   EnumChildWindows(GetSafeHwnd(),EnableChildWindow,bEnable);
}

void CSpecLiftingPage::OnCbnSelchangeWindType()
{
   // TODO: Add your control notification handler code here
   CComboBox* pcbWindType = (CComboBox*)GetDlgItem(IDC_WIND_TYPE);
   int curSel = pcbWindType->GetCurSel();
   pgsTypes::WindType windType = (pgsTypes::WindType)pcbWindType->GetItemData(curSel);
   CDataExchange dx(this,false);
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDispUnits = pApp->GetDisplayUnits();
   if ( windType == pgsTypes::Speed )
   {
      DDX_Tag(&dx,IDC_WIND_LOAD_UNIT,pDispUnits->Velocity);
   }
   else
   {
      DDX_Tag(&dx,IDC_WIND_LOAD_UNIT,pDispUnits->WindPressure);
   }
}
