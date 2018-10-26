///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
// LegalRatingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "LegalRatingPage.h"
#include "RatingOptionsDlg.h"
#include <MFCTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include "HtmlHelp\HelpTopics.hh"

// CLegalRatingPage dialog

IMPLEMENT_DYNAMIC(CLegalRatingPage, CPropertyPage)

CLegalRatingPage::CLegalRatingPage()
	: CPropertyPage(CLegalRatingPage::IDD)
{
}

CLegalRatingPage::~CLegalRatingPage()
{
}

void CLegalRatingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      m_Data.RoutineNames.clear();
      int idx;
      int nItems = m_ctlRoutineLL.GetCount();
      for (idx = 0; idx < nItems; idx++)
      {
         int check_state = m_ctlRoutineLL.GetCheck(idx);
         if (check_state == 1)
         {
            CString str;
            m_ctlRoutineLL.GetText(idx, str);
            m_Data.RoutineNames.push_back( std::_tstring(str));
         }
      }

      m_Data.SpecialNames.clear();
      nItems = m_ctlSpecialLL.GetCount();
      for (idx = 0; idx < nItems; idx++)
      {
         int check_state = m_ctlSpecialLL.GetCheck(idx);
         if (check_state == 1)
         {
            CString str;
            m_ctlSpecialLL.GetText(idx, str);
            m_Data.SpecialNames.push_back( std::_tstring(str));
         }
      }
   }

	DDX_Control(pDX, IDC_ROUTINE_LIVELOAD, m_ctlRoutineLL);
	DDX_Control(pDX, IDC_SPECIAL_LIVELOAD, m_ctlSpecialLL);

   DDX_Percentage(pDX,IDC_ROUTINE_TRUCK_IMPACT,m_Data.IM_Truck_Routine);
   DDX_Percentage(pDX,IDC_ROUTINE_LANE_IMPACT, m_Data.IM_Lane_Routine);

   DDX_Percentage(pDX,IDC_SPECIAL_TRUCK_IMPACT,m_Data.IM_Truck_Special);
   DDX_Percentage(pDX,IDC_SPECIAL_LANE_IMPACT, m_Data.IM_Lane_Special);

   DDX_Text(pDX,IDC_STRENGTH_I_DC,m_Data.StrengthI_DC);
   DDX_Text(pDX,IDC_STRENGTH_I_DW,m_Data.StrengthI_DW);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_ROUTINE,_T("Compute"),m_Data.StrengthI_LL_Routine);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_SPECIAL,_T("Compute"),m_Data.StrengthI_LL_Special);
   
   DDX_Text(pDX,IDC_SERVICE_III_DC,m_Data.ServiceIII_DC);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_Data.ServiceIII_DW);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL_ROUTINE,_T("Compute"),m_Data.ServiceIII_LL_Routine);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL_SPECIAL,_T("Compute"),m_Data.ServiceIII_LL_Special);

   DDX_Check_Bool(pDX,IDC_RATE_FOR_STRESS, m_Data.bRateForStress);
   DDX_Check_Bool(pDX,IDC_RATE_FOR_SHEAR,  m_Data.bRateForShear);
   DDX_Check_Bool(pDX,IDC_EXCLUDE_LANELOAD,m_Data.bExcludeLaneLoad);

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);
   DDX_UnitValueAndTag(pDX,IDC_ALLOWABLE_TENSION,IDC_ALLOWABLE_TENSION_UNIT,m_Data.AllowableTensionCoefficient,pDisplayUnits->GetTensionCoefficientUnit());

   CString tag = pDisplayUnits->GetUnitMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )");
   DDX_Text(pDX,IDC_ALLOWABLE_TENSION_UNIT,tag);
}


BEGIN_MESSAGE_MAP(CLegalRatingPage, CPropertyPage)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CLegalRatingPage message handlers

BOOL CLegalRatingPage::OnInitDialog()
{
   EnableToolTips();

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
   for (std::vector<std::_tstring>::reverse_iterator iter = m_Data.RoutineNames.rbegin(); iter != m_Data.RoutineNames.rend(); iter++)
   {
      LPCTSTR str = iter->c_str();
      int idx = m_ctlRoutineLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlRoutineLL.SetCheck( idx, 1 );
         m_ctlRoutineLL.SetTopIndex(idx);
      }
   }

   for (std::vector<std::_tstring>::reverse_iterator iter = m_Data.SpecialNames.rbegin(); iter != m_Data.SpecialNames.rend(); iter++)
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

BOOL CLegalRatingPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   pgsTypes::LimitState limit_state;
   bool bIsLoadFactorTip = false;

   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);

      switch(nID)
      {
      case IDC_ROUTINE_TRUCK_IMPACT:
      case IDC_SPECIAL_TRUCK_IMPACT:
         m_strTip = _T("MBE 6A.4.4.3 Normal Conditions IM = 33%\rMBE C6A.4.4.3 For spans greater than 40 ft:\rSmooth riding surface at approaches, bridge deck, and expansion joints IM = 10%\rMinor surface deviations ar depressions IM = 20%");
         break;

      case IDC_RATE_FOR_STRESS:
         m_strTip = _T("This is an optional rating consideration. See MBE 6A.5.4.2.2a");
         break;

      case IDC_RATE_FOR_SHEAR:
         m_strTip = _T("In-service concrete bridges that show no visible signs of shear distress need not be checked for shear when rating for legal loads (MBE 6A.5.9)");
         break;

      case IDC_STRENGTH_I_LL_ROUTINE:
         limit_state = pgsTypes::StrengthI_LegalRoutine;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL_ROUTINE:
         limit_state = pgsTypes::ServiceIII_LegalRoutine;
         bIsLoadFactorTip = true;
         break;

      case IDC_STRENGTH_I_LL_SPECIAL:
         limit_state = pgsTypes::StrengthI_LegalSpecial;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL_SPECIAL:
         limit_state = pgsTypes::ServiceIII_LegalSpecial;
         bIsLoadFactorTip = true;
         break;

      default:
         return FALSE;
      }

      if ( bIsLoadFactorTip )
      {
         CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
         pParent->GetLoadFactorToolTip(m_strTip,limit_state);
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

BOOL CLegalRatingPage::OnSetActive()
{
   if ( !CPropertyPage::OnSetActive() )
      return FALSE;

   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2( broker, ILibrary, pLib );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( pParent->m_GeneralPage.m_Data.CriteriaName.c_str() );

   CDataExchange dx(this,false);
   Float64 gLL = -1;

   const CLiveLoadFactorModel& routine = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Routine);
   if ( !routine.AllowUserOverride() )
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_ROUTINE,_T("Compute"),gLL);
      DDX_Keyword(&dx,IDC_SERVICE_III_LL_ROUTINE,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_ROUTINE)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_III_LL_ROUTINE)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_STRENGTH_I_LL_ROUTINE)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_III_LL_ROUTINE)->EnableWindow(TRUE);
   }


   const CLiveLoadFactorModel& special = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Special);
   if ( !special.AllowUserOverride() )
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_SPECIAL,_T("Compute"),gLL);
      DDX_Keyword(&dx,IDC_SERVICE_III_LL_SPECIAL,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_SPECIAL)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_III_LL_SPECIAL)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_STRENGTH_I_LL_SPECIAL)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_III_LL_SPECIAL)->EnableWindow(TRUE);
   }

   return TRUE;
}

void CLegalRatingPage::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_RATING_LEGAL_TAB );
}