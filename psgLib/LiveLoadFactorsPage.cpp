///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// LiveLoadFactorsPage.cpp : implementation file
//

#include "stdafx.h"
#include "LiveLoadFactorsPage.h"
#include "RatingDialog.h"
#include "..\htmlhelp\HelpTopics.hh"

// CLiveLoadFactorsPage dialog

IMPLEMENT_DYNAMIC(CLiveLoadFactorsPage, CPropertyPage)

CLiveLoadFactorsPage::CLiveLoadFactorsPage(UINT idd,LPCTSTR strTitle,pgsTypes::LoadRatingType ratingType)
	: CPropertyPage(idd)
{
   m_RatingType = ratingType;
   m_SpecialPermitType = (pgsTypes::SpecialPermitType)(-1); // bogus value so we know this is not for a special permit rating

	m_psp.pszTitle = strTitle;
  	m_psp.dwFlags |= PSP_USETITLE;
}

CLiveLoadFactorsPage::CLiveLoadFactorsPage(UINT idd,LPCTSTR strTitle,pgsTypes::LoadRatingType ratingType,pgsTypes::SpecialPermitType permitType)
	: CPropertyPage(idd)
{
   m_RatingType = ratingType;
   m_SpecialPermitType = permitType;

	m_psp.pszTitle = strTitle;
  	m_psp.dwFlags |= PSP_USETITLE;
}

CLiveLoadFactorsPage::~CLiveLoadFactorsPage()
{
}

void CLiveLoadFactorsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CRatingDialog* pParent = (CRatingDialog*)GetParent();

   if ( m_RatingType == pgsTypes::lrPermit_Special )
      pParent->ExchangeLoadFactorData(pDX,m_SpecialPermitType);
   else
      pParent->ExchangeLoadFactorData(pDX,m_RatingType);
}


BEGIN_MESSAGE_MAP(CLiveLoadFactorsPage, CPropertyPage)
   ON_CBN_SELCHANGE(IDC_LL_METHOD, &CLiveLoadFactorsPage::OnLiveLoadFactorTypeChanged)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


// CLiveLoadFactorsPage message handlers

BOOL CLiveLoadFactorsPage::OnInitDialog()
{
   CRatingDialog* pParent = (CRatingDialog*)GetParent();

   CComboBox* pcbLLMethod = (CComboBox*)GetDlgItem(IDC_LL_METHOD);
   int idx = pcbLLMethod->AddString(_T("Single Value"));
   pcbLLMethod->SetItemData(idx,(DWORD_PTR)pgsTypes::gllSingleValue);
   idx = pcbLLMethod->AddString(_T("Stepped"));
   pcbLLMethod->SetItemData(idx,(DWORD_PTR)pgsTypes::gllStepped);
   idx = pcbLLMethod->AddString(_T("Linear"));
   pcbLLMethod->SetItemData(idx,(DWORD_PTR)pgsTypes::gllLinear);
   idx = pcbLLMethod->AddString(_T("Bilinear"));
   pcbLLMethod->SetItemData(idx,(DWORD_PTR)pgsTypes::gllBilinear);

   if ( pParent->m_RatingDescriptionPage.GetSpecVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
      idx = pcbLLMethod->AddString(_T("Bilinear with vehicle weight"));
   else
      idx = pcbLLMethod->AddString(_T("Bilinear with permit weight ratio"));
   pcbLLMethod->SetItemData(idx,(DWORD_PTR)pgsTypes::gllBilinearWithWeight);

   CComboBox* pcbOptions = (CComboBox*)GetDlgItem(IDC_INTERPOLATE);
   idx = pcbOptions->AddString(_T("Interpolate between ADTT"));
   pcbOptions->SetItemData(idx,(DWORD_PTR)pgsTypes::gllmInterpolate);
   idx = pcbOptions->AddString(_T("Round up ADTT"));
   pcbOptions->SetItemData(idx,(DWORD_PTR)pgsTypes::gllmRoundUp);

   CPropertyPage::OnInitDialog();

   OnLiveLoadFactorTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLiveLoadFactorsPage::OnLiveLoadFactorTypeChanged()
{
   CRatingDialog* pParent = (CRatingDialog*)GetParent();

   CComboBox* pcbLLMethod = (CComboBox*)GetDlgItem(IDC_LL_METHOD);
   int curSel =  pcbLLMethod->GetCurSel();
   pgsTypes::LiveLoadFactorType gllType = (pgsTypes::LiveLoadFactorType)(pcbLLMethod->GetItemData(curSel));

   BOOL bVehicleWeightLabel = FALSE;
   BOOL bLowerVehicleWeightLabel = FALSE;
   BOOL bLowerVehicleWeight = FALSE;
   BOOL bLowerVehicleWeightUnit = FALSE;
   BOOL bUpperVehicleWeightLabel = FALSE;
   BOOL bUpperVehicleWeight = FALSE;
   BOOL bUpperVehicleWeightUnit = FALSE;
   BOOL bADTT1Label = FALSE;
   BOOL bADTT1 = FALSE;
   BOOL bADTT2Label = FALSE;
   BOOL bADTT2 = FALSE;
   BOOL bADTT3Label = FALSE;
   BOOL bADTT3 = FALSE;
   BOOL bADTT4Label = FALSE;
   BOOL bgllLower1Label = FALSE;
   BOOL bgllLower1 = FALSE;
   BOOL bgllLower2Label = FALSE;
   BOOL bgllLower2 = FALSE;
   BOOL bgllLower3Label = FALSE;
   BOOL bgllLower3 = FALSE;
   BOOL bgllLower4Label = FALSE;
   BOOL bgllLower4 = FALSE;

   BOOL bgllMiddle1Label = FALSE;
   BOOL bgllMiddle1 = FALSE;
   BOOL bgllMiddle2Label = FALSE;
   BOOL bgllMiddle2 = FALSE;
   BOOL bgllMiddle3Label = FALSE;
   BOOL bgllMiddle3 = FALSE;
   BOOL bgllMiddle4Label = FALSE;
   BOOL bgllMiddle4 = FALSE;

   BOOL bgllUpper1Label = FALSE;
   BOOL bgllUpper1 = FALSE;
   BOOL bgllUpper2Label = FALSE;
   BOOL bgllUpper2 = FALSE;
   BOOL bgllUpper3Label = FALSE;
   BOOL bgllUpper3 = FALSE;
   BOOL bgllUpper4Label = FALSE;
   BOOL bgllUpper4 = FALSE;
   BOOL bgllService1 = FALSE;
   BOOL bgllService2 = FALSE;
   BOOL bgllService3 = FALSE;
   BOOL bgllService4 = FALSE;
   BOOL bInterpolate = FALSE;

   switch ( gllType )
   {
   case pgsTypes::gllSingleValue:
      bgllLower1Label = TRUE;
      bgllLower1      = TRUE;
      bgllService1    = TRUE;
      break;

   case pgsTypes::gllStepped:
      bADTT1Label     = TRUE;
      bADTT1          = TRUE;
      bADTT2Label     = TRUE;
      GetDlgItem(IDC_ADTT2_LABEL)->SetWindowText(_T("Otherwise"));
      bgllLower1Label = TRUE;
      bgllLower1      = TRUE;
      bgllService1    = TRUE;
      bgllLower2Label = TRUE;
      bgllLower2      = TRUE;
      bgllService2    = TRUE;
      bADTT4Label     = TRUE;
      bgllLower4Label = TRUE;
      bgllLower4      = TRUE;
      bgllService4    = TRUE;
      break;

   case pgsTypes::gllLinear:
      bADTT1Label     = TRUE;
      bADTT1          = TRUE;
      bADTT2Label     = TRUE;
      bADTT2          = TRUE;
      GetDlgItem(IDC_ADTT1_LABEL)->SetWindowText(_T("ADTT <="));
      GetDlgItem(IDC_ADTT2_LABEL)->SetWindowText(_T("ADTT >="));
      bgllLower1Label = TRUE;
      bgllLower1      = TRUE;
      bgllService1    = TRUE;
      bgllLower2Label = TRUE;
      bgllLower2      = TRUE;
      bgllService2    = TRUE;
      bADTT4Label     = TRUE;
      bgllLower4Label = TRUE;
      bgllLower4      = TRUE;
      bgllService4    = TRUE;
      bInterpolate = TRUE;
      break;

   case pgsTypes::gllBilinear:
      bADTT1Label = TRUE;
      bADTT1 = TRUE;
      bADTT2Label = TRUE;
      GetDlgItem(IDC_ADTT1_LABEL)->SetWindowText(_T("ADTT <"));
      GetDlgItem(IDC_ADTT2_LABEL)->SetWindowText(_T("ADTT ="));
      GetDlgItem(IDC_ADTT3_LABEL)->SetWindowText(_T("ADTT >"));
      bADTT2 = TRUE;
      bADTT3Label = TRUE;
      bADTT3 = TRUE;
      bADTT4Label = TRUE;
      bgllLower1Label = TRUE;
      bgllLower1 = TRUE;
      bgllService1    = TRUE;
      bgllLower2Label = TRUE;
      bgllLower2 = TRUE;
      bgllService2    = TRUE;
      bgllLower3Label = TRUE;
      bgllLower3 = TRUE;
      bgllService3    = TRUE;
      bgllLower4Label = TRUE;
      bgllLower4 = TRUE;
      bgllService4    = TRUE;
      bInterpolate = TRUE;
      break;

   case pgsTypes::gllBilinearWithWeight:
      bVehicleWeightLabel = TRUE;
      bLowerVehicleWeightLabel = TRUE;
      bLowerVehicleWeight = TRUE;
      bLowerVehicleWeightUnit = TRUE;
      bUpperVehicleWeightLabel = TRUE;
      bUpperVehicleWeight = TRUE;
      bUpperVehicleWeightUnit = TRUE;
      bADTT1Label = TRUE;
      bADTT1 = TRUE;
      bADTT2Label = TRUE;
      GetDlgItem(IDC_ADTT2_LABEL)->SetWindowText(_T("ADTT ="));
      bADTT2 = TRUE;
      bADTT3Label = TRUE;
      bADTT3 = TRUE;
      bADTT4Label = TRUE;
      bgllLower1Label = TRUE;
      bgllLower1 = TRUE;
      bgllService1    = TRUE;
      bgllLower2Label = TRUE;
      bgllLower2 = TRUE;
      bgllService2    = TRUE;
      bgllLower3Label = TRUE;
      bgllLower3 = TRUE;
      bgllService3    = TRUE;
      bgllLower4Label = TRUE;
      bgllLower4 = TRUE;
      bgllService4    = TRUE;

      if ( lrfrVersionMgr::SecondEditionWith2013Interims <= pParent->m_RatingDescriptionPage.GetSpecVersion() )
      {
         bgllMiddle1Label = TRUE;
         bgllMiddle1 = TRUE;
         bgllMiddle2Label = TRUE;
         bgllMiddle2 = TRUE;
         bgllMiddle3Label = TRUE;
         bgllMiddle3 = TRUE;
         bgllMiddle4Label = TRUE;
         bgllMiddle4 = TRUE;
      }

      bgllUpper1Label = TRUE;
      bgllUpper1 = TRUE;
      bgllUpper2Label = TRUE;
      bgllUpper2 = TRUE;
      bgllUpper3Label = TRUE;
      bgllUpper3 = TRUE;
      bgllUpper4Label = TRUE;
      bgllUpper4 = TRUE;
      bInterpolate = TRUE;
      break;

   default:
      ATLASSERT(false); // is there a new live load factor type???
   }

   GetDlgItem(IDC_VEHICLE_WEIGHT_LABEL)->ShowWindow(bVehicleWeightLabel ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LOWER_VEHICLE_WEIGHT_LABEL)->ShowWindow(bLowerVehicleWeightLabel ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LOWER_VEHICLE_WEIGHT)->ShowWindow(bLowerVehicleWeight ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LOWER_VEHICLE_WEIGHT_UNIT)->ShowWindow(bLowerVehicleWeightUnit ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_UPPER_VEHICLE_WEIGHT_LABEL)->ShowWindow(bUpperVehicleWeightLabel ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_UPPER_VEHICLE_WEIGHT)->ShowWindow(bUpperVehicleWeight ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_UPPER_VEHICLE_WEIGHT_UNIT)->ShowWindow(bUpperVehicleWeightUnit ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_ADTT1_LABEL)->ShowWindow(bADTT1Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ADTT2_LABEL)->ShowWindow(bADTT2Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ADTT3_LABEL)->ShowWindow(bADTT3Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ADTT4_LABEL)->ShowWindow(bADTT4Label ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_ADTT1)->ShowWindow(bADTT1 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ADTT2)->ShowWindow(bADTT2 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ADTT3)->ShowWindow(bADTT3 ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LF_LOWER1_LABEL)->ShowWindow(bgllLower1Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER2_LABEL)->ShowWindow(bgllLower2Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER3_LABEL)->ShowWindow(bgllLower3Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER4_LABEL)->ShowWindow(bgllLower4Label ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LF_LOWER1)->ShowWindow(bgllLower1 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER2)->ShowWindow(bgllLower2 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER3)->ShowWindow(bgllLower3 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_LOWER4)->ShowWindow(bgllLower4 ? SW_SHOW : SW_HIDE);

   if ( lrfrVersionMgr::SecondEditionWith2013Interims <= pParent->m_RatingDescriptionPage.GetSpecVersion() )
   {
      GetDlgItem(IDC_LF_MIDDLE1_LABEL)->ShowWindow(bgllMiddle1Label ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE2_LABEL)->ShowWindow(bgllMiddle2Label ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE3_LABEL)->ShowWindow(bgllMiddle3Label ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE4_LABEL)->ShowWindow(bgllMiddle4Label ? SW_SHOW : SW_HIDE);

      GetDlgItem(IDC_LF_MIDDLE1)->ShowWindow(bgllMiddle1 ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE2)->ShowWindow(bgllMiddle2 ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE3)->ShowWindow(bgllMiddle3 ? SW_SHOW : SW_HIDE);
      GetDlgItem(IDC_LF_MIDDLE4)->ShowWindow(bgllMiddle4 ? SW_SHOW : SW_HIDE);
   }

   GetDlgItem(IDC_LF_UPPER1_LABEL)->ShowWindow(bgllUpper1Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER2_LABEL)->ShowWindow(bgllUpper2Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER3_LABEL)->ShowWindow(bgllUpper3Label ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER4_LABEL)->ShowWindow(bgllUpper4Label ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LF_UPPER1)->ShowWindow(bgllUpper1 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER2)->ShowWindow(bgllUpper2 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER3)->ShowWindow(bgllUpper3 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_UPPER4)->ShowWindow(bgllUpper4 ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LF_SERVICE_1)->ShowWindow(bgllService1 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_2)->ShowWindow(bgllService2 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_3)->ShowWindow(bgllService3 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_4)->ShowWindow(bgllService4 ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_LF_SERVICE_1_LABEL)->ShowWindow(bgllService1 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_2_LABEL)->ShowWindow(bgllService2 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_3_LABEL)->ShowWindow(bgllService3 ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_LF_SERVICE_4_LABEL)->ShowWindow(bgllService4 ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_INTERPOLATE)->ShowWindow( bInterpolate ? SW_SHOW : SW_HIDE );
}

LRESULT CLiveLoadFactorsPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_LIVE_LOAD_FACTORS );
   return TRUE;
}
