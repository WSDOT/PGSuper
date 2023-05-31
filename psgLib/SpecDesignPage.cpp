///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <EAF\EAFDocument.h>

#include "SpecMainSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage dialog
IMPLEMENT_DYNCREATE(CSpecDesignPage, CPropertyPage)


CSpecDesignPage::CSpecDesignPage(CWnd* pParent /*=nullptr*/)
	: CPropertyPage(CSpecDesignPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDesignPage)
   //}}AFX_DATA_INIT
}


void CSpecDesignPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);

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
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE, &CSpecDesignPage::OnBnClickedCheckBottomFlangeClearance)
   ON_BN_CLICKED(IDC_CHECK_INCLINDED_GIRDER, &CSpecDesignPage::OnBnClickedCheckInclindedGirder)
   ON_BN_CLICKED(IDC_LL_DEFLECTION, &CSpecDesignPage::OnBnClickedLlDeflection)
   ON_BN_CLICKED(IDC_CHECK_HANDLING_WEIGHT, &CSpecDesignPage::OnBnClickedCheckHandlingWeight)
   ON_BN_CLICKED(IDC_FC1, &CSpecDesignPage::OnFcTypeChanged)
   ON_BN_CLICKED(IDC_FC2, &CSpecDesignPage::OnFcTypeChanged)
   ON_BN_CLICKED(IDC_DESIGN_A, &CSpecDesignPage::OnDesignA)
   ON_BN_CLICKED(IDC_USE_90_DAY_STRENGTH, &CSpecDesignPage::OnBnClicked90DayStrength)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage message handlers
BOOL CSpecDesignPage::OnInitDialog()
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();
	
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

   CComboBox* pcbHDFT = (CComboBox*)GetDlgItem(IDC_HOLD_DOWN_FORCE_TYPE);
   pcbHDFT->AddString(_T("Total Hold Down Force"));
   pcbHDFT->AddString(_T("Hold Down Force per Strand"));

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

   OnBnClickedLlDeflection();

   OnBnClickedCheckHandlingWeight();

   OnBnClicked90DayStrength();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// Don't allow design if check is disabled.
inline void CheckDesignCtrl(int idc, int idd, int list[], CWnd* pme)
{
	CButton* pbut = (CButton*)pme->GetDlgItem(idc);
   BOOL ischk = pbut->GetCheck()!=0 ? TRUE : FALSE;
 	CButton* pbut2 = (CButton*)pme->GetDlgItem(idd);
   if (ischk==FALSE)
   {
      pbut2->SetCheck(0);
      pbut2->EnableWindow(FALSE);
   }
   else
   {
      pbut2->EnableWindow(TRUE);
   }

   // enable/disable controls in aux list
   for(int* p=list; *p!=-1; p++)
   {
      int id = *p;
      CWnd* pwnd = pme->GetDlgItem(id);
      pwnd->EnableWindow(ischk);
   }
}

void CSpecDesignPage::OnCheckA() 
{
   int list[]={ IDC_ELEVATION_TOLERANCE_LABEL,IDC_ELEVATION_TOLERANCE,IDC_ELEVATION_TOLERANCE_UNIT,IDC_ELEVATION_TOLERANCE_NOTE, -1};

   CheckDesignCtrl(IDC_CHECK_A, IDC_DESIGN_A, list, this);

   OnDesignA();
}

void CSpecDesignPage::OnDesignA()
{
   int list[] = { IDC_A_ROUNDING_EDIT, IDC_A_ROUNDING_UNIT, -1 };
   CheckDesignCtrl(IDC_DESIGN_A, IDC_A_ROUNDING_CB, list, this);
}


void CSpecDesignPage::OnCheckHauling() 
{
   int list[]={IDC_MIN_TRUCK_SUPPORT_LABEL,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,IDC_STATIC_H,
               IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_LABEL,IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,
               IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_UNIT,IDC_IS_SUPPORT_LESS_THAN,IDC_SUPPORT_LESS_THAN,
               IDC_SUPPORT_LESS_THAN_UNIT,IDC_STATIC_K,-1};

   CheckDesignCtrl(IDC_CHECK_HAULING, IDC_DESIGN_HAULING, list, this);
}

void CSpecDesignPage::OnCheckHd() 
{
   int list[]={ IDC_HOLD_DOWN_FORCE_TYPE,IDC_HOLD_DOWN_FORCE,IDC_HOLD_DOWN_FORCE_UNITS,IDC_FRICTION_LABEL,IDC_FRICTION,IDC_FRICTION_UNIT,-1};

   CheckDesignCtrl(IDC_CHECK_HD, IDC_DESIGN_HD, list, this);
}

void CSpecDesignPage::OnCheckLifting() 
{
   int list[]={IDC_MIN_LIFTING_POINT_LABEL,IDC_MIN_LIFTING_POINT,IDC_MIN_LIFTING_POINT_UNIT,IDC_LIFTING_POINT_LOCATION_ACCURACY_LABEL,IDC_STATIC_L,IDC_LIFTING_POINT_LOCATION_ACCURACY,IDC_LIFTING_POINT_LOCATION_ACCURACY_UNIT,-1};

   CheckDesignCtrl(IDC_CHECK_LIFTING, IDC_DESIGN_LIFTING, list, this);
}

void CSpecDesignPage::OnCheckSlope() 
{
   int list[]={IDC_STATIC_SLOPE_05,IDC_STRAND_SLOPE_05,IDC_STATIC_SLOPE_06,IDC_STRAND_SLOPE_06,IDC_STATIC_SLOPE_07,
               IDC_STRAND_SLOPE_07,-1};
   CheckDesignCtrl(IDC_CHECK_SLOPE, IDC_DESIGN_SLOPE, list, this);
}

void CSpecDesignPage::OnCheckSplitting() 
{
   int list[]={IDC_STATIC_SPL,IDC_STATIC_SH,IDC_N,-1};
   CheckDesignCtrl(IDC_CHECK_SPLITTING, IDC_DESIGN_SPLITTING, list, this);
}

void CSpecDesignPage::OnCheckConfinement() 
{
   int list[]={-1};

   CheckDesignCtrl(IDC_CHECK_CONFINEMENT, IDC_DESIGN_CONFINEMENT, list, this);
}


void CSpecDesignPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_DESIGN );
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
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE);
   GetDlgItem(IDC_CLEARANCE_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE_UNIT)->EnableWindow(bEnable);
}

void CSpecDesignPage::OnBnClickedCheckInclindedGirder()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_INCLINDED_GIRDER);
   GetDlgItem(IDC_INCLINDED_GIRDER_FS_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_INCLINDED_GIRDER_FS)->EnableWindow(bEnable);
}

BOOL CSpecDesignPage::OnSetActive()
{
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();

   // deal with 2017 crosswalk
   CWnd* pWnd = GetDlgItem(IDC_SSPLITTING);
   pWnd->SetWindowText(CString(_T("Splitting Resistance (")) + pDad->LrfdCw8th(_T("5.10.10.1"),_T("5.9.4.4.1")) + _T(")"));

   pWnd = GetDlgItem(IDC_SCONFINEMENT);
   pWnd->SetWindowText(CString(_T("Confinement Reinforcement (")) + pDad->LrfdCw8th(_T("5.10.10.2"),_T("5.9.4.4.2")) + _T(")"));

   pWnd = GetDlgItem(IDC_FC1);
   pWnd->SetWindowText(CString(_T("Use fc at the time of loading (")) + pDad->LrfdCw8th(_T("5.14.1.3.2d and 5.14.1.3.3"),_T("5.12.3.4.2d and 5.12.3.4.3")) + _T(")"));
   pWnd->EnableWindow(pDad->m_Entry.GetLossMethod() == pgsTypes::TIME_STEP ? TRUE : FALSE); // only an option for time-step analysis

   pWnd = GetDlgItem(IDC_90_DAY_STRENGTH_LABEL);
   pWnd->SetWindowText(CString(_T("% of f'c for stress combinations after 90 days for slow curing concretes (")) + pDad->LrfdCw8th(_T("5.14.1.2.5"), _T("5.12.3.2.5")) + _T(")"));

   if (pDad->GetSpecVersion() < lrfdVersionMgr::NinthEdition2020)
   {
      pWnd = GetDlgItem(IDC_LIFTING_GROUP);
      pWnd->SetWindowText(_T("Lifting Stability Check/Design Options"));

      pWnd = GetDlgItem(IDC_HAULING_GROUP);
      pWnd->SetWindowText(_T("Hauling Stability Check/Design Options"));
   }
   else
   {
      // requirement is new in LRFD 9th Edition so add spec reference
      pWnd = GetDlgItem(IDC_LIFTING_GROUP);
      pWnd->SetWindowText(_T("Lifting Stability Check/Design Options (LRFD 5.5.4.3)"));

      pWnd = GetDlgItem(IDC_HAULING_GROUP);
      pWnd->SetWindowText(_T("Hauling Stability Check/Design Options (LRFD 5.5.4.3)"));
   }

   return CPropertyPage::OnSetActive();
}

void CSpecDesignPage::OnBnClickedLlDeflection()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_LL_DEFLECTION);
   GetDlgItem(IDC_LL_DEF_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_DEFLECTION_LIMIT)->EnableWindow(bEnable);
}

void CSpecDesignPage::OnBnClickedCheckHandlingWeight()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_HANDLING_WEIGHT);
   GetDlgItem(IDC_HANDLING_WEIGHT)->EnableWindow(bEnable);
   GetDlgItem(IDC_HANDLING_WEIGHT_UNIT)->EnableWindow(bEnable);
}

void CSpecDesignPage::OnFcTypeChanged()
{
   OnBnClicked90DayStrength();
}

void CSpecDesignPage::OnBnClicked90DayStrength()
{
   BOOL bCorrectFcSetting = IsDlgButtonChecked(IDC_FC2);
   BOOL bIsUsed = IsDlgButtonChecked(IDC_USE_90_DAY_STRENGTH);
   BOOL bEnable = (bIsUsed && bCorrectFcSetting);

   GetDlgItem(IDC_USE_90_DAY_STRENGTH)->EnableWindow(bCorrectFcSetting);
   GetDlgItem(IDC_90_DAY_STRENGTH_FACTOR)->EnableWindow(bEnable);
   GetDlgItem(IDC_90_DAY_STRENGTH_LABEL)->EnableWindow(bEnable);
}