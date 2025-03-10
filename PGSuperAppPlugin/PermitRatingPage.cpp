///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// PermitRatingPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "PermitRatingPage.h"
#include "RatingOptionsDlg.h"

#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

// CPermitRatingPage dialog

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPermitRatingPage, CPropertyPage)

CPermitRatingPage::CPermitRatingPage()
	: CPropertyPage(CPermitRatingPage::IDD)
{

}

CPermitRatingPage::~CPermitRatingPage()
{
}

void CPermitRatingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      m_Data.RoutinePermitNames.clear();
      int idx;
      int nItems = m_ctlRoutineLL.GetCount();
      for (idx = 0; idx < nItems; idx++)
      {
         int check_state = m_ctlRoutineLL.GetCheck(idx);
         if (check_state == 1)
         {
            CString str;
            m_ctlRoutineLL.GetText(idx, str);
            m_Data.RoutinePermitNames.push_back( std::_tstring(str));
         }
      }

      m_Data.SpecialPermitNames.clear();
      nItems = m_ctlSpecialLL.GetCount();
      for (idx = 0; idx < nItems; idx++)
      {
         int check_state = m_ctlSpecialLL.GetCheck(idx);
         if (check_state == 1)
         {
            CString str;
            m_ctlSpecialLL.GetText(idx, str);
            m_Data.SpecialPermitNames.push_back( std::_tstring(str));
         }
      }
   }

	DDX_Control(pDX, IDC_PERMIT_LIVELOAD,  m_ctlRoutineLL);
	DDX_Control(pDX, IDC_PERMIT_LIVELOAD2, m_ctlSpecialLL);

   DDX_CBEnum(pDX,IDC_PERMIT_TYPE,m_Data.SpecialPermitType); // need this before load factors

   DDX_Percentage(pDX,IDC_PERMIT_TRUCK_IMPACT,m_Data.IM_Truck_Routine);
   DDX_Percentage(pDX,IDC_PERMIT_LANE_IMPACT, m_Data.IM_Lane_Routine);

   DDX_Percentage(pDX,IDC_PERMIT_TRUCK_IMPACT2,m_Data.IM_Truck_Special);
   DDX_Percentage(pDX,IDC_PERMIT_LANE_IMPACT2, m_Data.IM_Lane_Special);

   DDX_Text(pDX,IDC_STRENGTH_II_DC,m_Data.StrengthII_DC);
   DDX_Text(pDX,IDC_STRENGTH_II_DW,m_Data.StrengthII_DW);
   DDX_Keyword(pDX,IDC_STRENGTH_II_LL_PERMIT, _T("Compute"),m_Data.StrengthII_LL_Routine);
   DDX_Keyword(pDX,IDC_STRENGTH_II_LL_SPECIAL,_T("Compute"),m_Data.StrengthII_LL_Special);
   DDX_Text(pDX,IDC_STRENGTH_II_CR,m_Data.StrengthII_CR);
   DDX_Text(pDX,IDC_STRENGTH_II_SH,m_Data.StrengthII_SH);
   DDX_Text(pDX,IDC_STRENGTH_II_PS,m_Data.StrengthII_PS);
   
   DDX_Text(pDX,IDC_SERVICE_I_DC,m_Data.ServiceI_DC);
   DDX_Text(pDX,IDC_SERVICE_I_DW,m_Data.ServiceI_DW);
   DDX_Keyword(pDX,IDC_SERVICE_I_LL_PERMIT, _T("Compute"),m_Data.ServiceI_LL_Routine);
   DDX_Keyword(pDX,IDC_SERVICE_I_LL_SPECIAL,_T("Compute"),m_Data.ServiceI_LL_Special);
   DDX_Text(pDX,IDC_SERVICE_I_CR,m_Data.ServiceI_CR);
   DDX_Text(pDX,IDC_SERVICE_I_SH,m_Data.ServiceI_SH);
   DDX_Text(pDX,IDC_SERVICE_I_PS,m_Data.ServiceI_PS);
   
   DDX_Text(pDX,IDC_SERVICE_III_DC,m_Data.ServiceIII_DC);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_Data.ServiceIII_DW);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL_PERMIT, _T("Compute"),m_Data.ServiceIII_LL_Routine);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL_SPECIAL,_T("Compute"),m_Data.ServiceIII_LL_Special);
   DDX_Text(pDX,IDC_SERVICE_III_CR,m_Data.ServiceIII_CR);
   DDX_Text(pDX,IDC_SERVICE_III_SH,m_Data.ServiceIII_SH);
   DDX_Text(pDX,IDC_SERVICE_III_PS,m_Data.ServiceIII_PS);

   DDX_Check_Bool(pDX,IDC_RATE_FOR_STRESS, m_Data.bRateForStress);
   DDX_Check_Bool(pDX,IDC_RATE_FOR_SHEAR,m_Data.bRateForShear);
   DDX_Check_Bool(pDX,IDC_CHECK_YIELDING,m_Data.bCheckReinforcementYielding);

   DDX_Text(pDX,IDC_FY_COEFFICIENT,m_Data.YieldStressCoefficient);
   DDV_Range(pDX,mfcDDV::LE,mfcDDV::GE,m_Data.YieldStressCoefficient,0.0,1.0);

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);
   DDX_UnitValueAndTag(pDX,IDC_ALLOWABLE_TENSION,IDC_ALLOWABLE_TENSION_UNIT,m_Data.AllowableTensionCoefficient,pDisplayUnits->GetTensionCoefficientUnit());

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pDisplayUnits->GetUnitMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pDisplayUnits->GetUnitMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }
   DDX_Text(pDX,IDC_ALLOWABLE_TENSION_UNIT,tag);

   DDX_Check_Bool(pDX, IDC_CHECK_TENSION_MAX, m_Data.bLimitTensileStress);
   DDX_UnitValueAndTag(pDX, IDC_TENSION_MAX, IDC_TENSION_MAX_UNIT, m_Data.MaxTensileStress, pDisplayUnits->GetStressUnit());
}


BEGIN_MESSAGE_MAP(CPermitRatingPage, CPropertyPage)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_BN_CLICKED(IDC_CHECK_YIELDING, &CPermitRatingPage::OnCheckYieldingClicked)
   ON_CBN_SELCHANGE(IDC_PERMIT_TYPE, &CPermitRatingPage::OnPermitTypeChanged)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_RATE_FOR_STRESS, &CPermitRatingPage::OnRateForStressChanged)
   ON_BN_CLICKED(IDC_CHECK_TENSION_MAX, &CPermitRatingPage::OnMaxTensionStressChanged)
END_MESSAGE_MAP()


// CPermitRatingPage message handlers

BOOL CPermitRatingPage::OnInitDialog()
{
   VERIFY(EnableToolTips());

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_PERMIT_TYPE);
   int idx = pCB->AddString(_T("Single Trip with escort"));
   pCB->SetItemData(idx,(DWORD_PTR)pgsTypes::ptSingleTripWithEscort);

   idx = pCB->AddString(_T("Single Trip with other traffic"));
   pCB->SetItemData(idx,(DWORD_PTR)pgsTypes::ptSingleTripWithTraffic);

   idx = pCB->AddString(_T("Multiple Trips (< 100) with other traffic"));
   pCB->SetItemData(idx,(DWORD_PTR)pgsTypes::ptMultipleTripWithTraffic);

   CPropertyPage::OnInitDialog();

   m_ctlRoutineLL.SetCheckStyle( BS_AUTOCHECKBOX );
   m_ctlSpecialLL.SetCheckStyle( BS_AUTOCHECKBOX );

   // fill the live load list boxes here
   for (std::vector<std::_tstring>::iterator iter = m_AllNames.begin(); iter != m_AllNames.end(); iter++)
   {
      LPCTSTR str = iter->c_str();
      m_ctlRoutineLL.AddString(str);
      m_ctlSpecialLL.AddString(str);
   }

   // Set the check marks for the various loads
   for (std::vector<std::_tstring>::reverse_iterator iter = m_Data.RoutinePermitNames.rbegin(); iter != m_Data.RoutinePermitNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlRoutineLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlRoutineLL.SetCheck( idx, 1 );
         m_ctlRoutineLL.SetTopIndex(idx);
      }
   }

   for (std::vector<std::_tstring>::reverse_iterator iter = m_Data.SpecialPermitNames.rbegin(); iter != m_Data.SpecialPermitNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlSpecialLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlSpecialLL.SetCheck( idx, 1 );
         m_ctlSpecialLL.SetTopIndex(idx);
      }
   }


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPermitRatingPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   pgsTypes::LimitState limit_state;
   pgsTypes::SpecialPermitType specialPermitType = GetSpecialPermitType();
   bool bIsLoadFactorTip = false;

   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);

      switch(nID)
      {
      case IDC_PERMIT_TRUCK_IMPACT:
         m_strTip = _T("MBE 6A.4.5.5 For slow moving vehicles (<= 10 mph) IM = 0%\rMBE 6A.4.4.3 Normal Conditions IM = 33%\rMBE C6A.4.4.3 For spans greater than 40 ft:\rSmooth riding surface at approaches, bridge deck, and expansion joints IM = 10%\rMinor surface deviations or depressions IM = 20%");
         break;

      case IDC_STRENGTH_II_LL_PERMIT:
         limit_state = pgsTypes::StrengthII_PermitRoutine;
         bIsLoadFactorTip = true;
         break;

      case IDC_STRENGTH_II_LL_SPECIAL:
         limit_state = pgsTypes::StrengthII_PermitSpecial;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_I_LL_PERMIT:
         limit_state = pgsTypes::ServiceI_PermitRoutine;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_I_LL_SPECIAL:
         limit_state = pgsTypes::ServiceI_PermitSpecial;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL_PERMIT:
         limit_state = pgsTypes::ServiceIII_PermitRoutine;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL_SPECIAL:
         limit_state = pgsTypes::ServiceIII_PermitSpecial;
         bIsLoadFactorTip = true;
         break;

      default:
         return FALSE;
      }

      if ( bIsLoadFactorTip )
      {
         CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
         pParent->GetLoadFactorToolTip(m_strTip,limit_state,specialPermitType);
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CPermitRatingPage::OnCheckYieldingClicked()
{
   BOOL bEnable = FALSE;
   if ( IsDlgButtonChecked(IDC_CHECK_YIELDING) )
   {
      bEnable = TRUE;
   }

   GetDlgItem(IDC_FY_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_FY_COEFFICIENT)->EnableWindow(bEnable);
   GetDlgItem(IDC_FY)->EnableWindow(bEnable);
}

void CPermitRatingPage::OnRateForStressChanged()
{
   BOOL bEnable = FALSE;
   if ( IsDlgButtonChecked(IDC_RATE_FOR_STRESS) )
   {
      bEnable = TRUE;
   }

   GetDlgItem(IDC_ALLOWABLE_TENSION_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ALLOWABLE_TENSION)->EnableWindow(bEnable);
   GetDlgItem(IDC_ALLOWABLE_TENSION_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_CHECK_TENSION_MAX)->EnableWindow(bEnable);
   GetDlgItem(IDC_TENSION_MAX)->EnableWindow(bEnable);
   GetDlgItem(IDC_TENSION_MAX_UNIT)->EnableWindow(bEnable);

   OnMaxTensionStressChanged();
}

void CPermitRatingPage::OnMaxTensionStressChanged()
{
   BOOL bEnable = FALSE;
   if (IsDlgButtonChecked(IDC_CHECK_TENSION_MAX) && GetDlgItem(IDC_CHECK_TENSION_MAX)->IsWindowEnabled())
   {
      bEnable = TRUE;
   }
   GetDlgItem(IDC_TENSION_MAX)->EnableWindow(bEnable);
   GetDlgItem(IDC_TENSION_MAX_UNIT)->EnableWindow(bEnable);
}

pgsTypes::SpecialPermitType CPermitRatingPage::GetSpecialPermitType()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_PERMIT_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::SpecialPermitType permitType = (pgsTypes::SpecialPermitType)pCB->GetItemData(curSel);
   return permitType;
}

void CPermitRatingPage::OnPermitTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_PERMIT_TYPE);
   int curSel = pCB->GetCurSel();
   pgsTypes::SpecialPermitType permitType = (pgsTypes::SpecialPermitType)pCB->GetItemData(curSel);

   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2( broker, ILibrary, pLib );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( pParent->m_GeneralPage.m_Data.CriteriaName.c_str() );

   bool bAllowUserOverride;
   if ( pRatingEntry->GetSpecificationVersion() < WBFL::LRFD::MBEManager::Edition::SecondEditionWith2013Interims )
   {
      const CLiveLoadFactorModel& permit = pRatingEntry->GetLiveLoadFactorModel(permitType);
      bAllowUserOverride = permit.AllowUserOverride();
   }
   else
   {
      const CLiveLoadFactorModel2& permit = pRatingEntry->GetLiveLoadFactorModel2(permitType);
      bAllowUserOverride = permit.AllowUserOverride();
   }

   GetDlgItem(IDC_STRENGTH_II_LL_SPECIAL)->EnableWindow(bAllowUserOverride ? TRUE : FALSE);
   GetDlgItem(IDC_SERVICE_I_LL_SPECIAL)->EnableWindow(bAllowUserOverride ? TRUE : FALSE);
   GetDlgItem(IDC_SERVICE_III_LL_SPECIAL)->EnableWindow(bAllowUserOverride ? TRUE : FALSE);
}

BOOL CPermitRatingPage::OnSetActive()
{
   if ( !CPropertyPage::OnSetActive() )
      return FALSE;

   OnPermitTypeChanged();
   OnCheckYieldingClicked();
   OnRateForStressChanged();

   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2( broker, ILibrary, pLib );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( pParent->m_GeneralPage.m_Data.CriteriaName.c_str() );

   CDataExchange dx(this,false);
   Float64 gLL = -1;

   bool bAllowUserOverride;
   if ( pRatingEntry->GetSpecificationVersion() < WBFL::LRFD::MBEManager::Edition::SecondEditionWith2013Interims )
   {
      const CLiveLoadFactorModel& permit = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      bAllowUserOverride = permit.AllowUserOverride();
   }
   else
   {
      const CLiveLoadFactorModel2& permit = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
      bAllowUserOverride = permit.AllowUserOverride();
   }

   if ( bAllowUserOverride )
   {
      GetDlgItem(IDC_STRENGTH_II_LL_PERMIT)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_I_LL_PERMIT)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_III_LL_PERMIT)->EnableWindow(TRUE);
   }
   else
   {
      DDX_Keyword(&dx,IDC_STRENGTH_II_LL_PERMIT,_T("Compute"),gLL);
      DDX_Keyword(&dx,IDC_SERVICE_I_LL_PERMIT,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_II_LL_PERMIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_I_LL_PERMIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_III_LL_PERMIT)->EnableWindow(FALSE);
   }

   GET_IFACE2(broker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      GetDlgItem(IDC_STRENGTH_II_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SERVICE_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SERVICE_III_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS_LABEL)->ShowWindow(SW_HIDE);
   }

   return TRUE;
}

void CPermitRatingPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_RATING_PERMIT_TAB );
}
