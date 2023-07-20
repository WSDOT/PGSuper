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
// DesignRatingPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "DesignRatingPage.h"
#include "RatingOptionsDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

// CDesignRatingPage dialog

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
IMPLEMENT_DYNAMIC(CDesignRatingPage, CPropertyPage)

CDesignRatingPage::CDesignRatingPage()
	: CPropertyPage(CDesignRatingPage::IDD)
{
}

CDesignRatingPage::~CDesignRatingPage()
{
}

void CDesignRatingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Text(pDX,IDC_STRENGTH_I_DC,m_Data.StrengthI_DC);
   DDX_Text(pDX,IDC_STRENGTH_I_DW,m_Data.StrengthI_DW);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_INVENTORY,_T("Compute"),m_Data.StrengthI_LL_Inventory);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_OPERATING,_T("Compute"),m_Data.StrengthI_LL_Operating);
   DDX_Text(pDX,IDC_STRENGTH_I_CR,m_Data.StrengthI_CR);
   DDX_Text(pDX,IDC_STRENGTH_I_SH,m_Data.StrengthI_SH);
   DDX_Text(pDX,IDC_STRENGTH_I_PS,m_Data.StrengthI_PS);
   
   DDX_Text(pDX,IDC_SERVICE_III_DC,m_Data.ServiceIII_DC);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_Data.ServiceIII_DW);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL,_T("Compute"),m_Data.ServiceIII_LL);
   DDX_Text(pDX,IDC_SERVICE_III_CR,m_Data.ServiceIII_CR);
   DDX_Text(pDX,IDC_SERVICE_III_SH,m_Data.ServiceIII_SH);
   DDX_Text(pDX,IDC_SERVICE_III_PS,m_Data.ServiceIII_PS);

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);
   DDX_UnitValueAndTag(pDX,IDC_ALLOWABLE_TENSION,IDC_ALLOWABLE_TENSION_UNIT,m_Data.AllowableTensionCoefficient,pDisplayUnits->GetTensionCoefficientUnit());
   CString tag;
   if ( WBFL::LRFD::LRFDVersionMgr::GetVersion() < WBFL::LRFD::LRFDVersionMgr::Version::SeventhEditionWith2016Interims )
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

   DDX_Check_Bool(pDX, IDC_RATE_FOR_SHEAR, m_Data.bRateForShear);
}


BEGIN_MESSAGE_MAP(CDesignRatingPage, CPropertyPage)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_CHECK_TENSION_MAX, &CDesignRatingPage::OnMaxTensionStressChanged)
END_MESSAGE_MAP()


// CDesignRatingPage message handlers
BOOL CDesignRatingPage::OnInitDialog()
{
   VERIFY(EnableToolTips());

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDesignRatingPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   WATCH(_T("CDesignRatingPage::OnToolTipNotify"));
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
      case IDC_RATE_FOR_SHEAR:
         m_strTip = _T("In-service concrete bridges that show no visible signs of shear distress need not be checked for shear when rating for the design loads (MBE 6A.5.9)");
         break;

      case IDC_STRENGTH_I_LL_INVENTORY:
         limit_state = pgsTypes::StrengthI_Inventory;
         bIsLoadFactorTip = true;
         break;

      case IDC_STRENGTH_I_LL_OPERATING:
         limit_state = pgsTypes::StrengthI_Operating;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL:
         limit_state = pgsTypes::ServiceIII_Inventory;
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
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CDesignRatingPage::OnMaxTensionStressChanged()
{
   BOOL bEnable = FALSE;
   if (IsDlgButtonChecked(IDC_CHECK_TENSION_MAX))
   {
      bEnable = TRUE;
   }
   GetDlgItem(IDC_TENSION_MAX)->EnableWindow(bEnable);
   GetDlgItem(IDC_TENSION_MAX_UNIT)->EnableWindow(bEnable);
}

BOOL CDesignRatingPage::OnSetActive()
{
   if ( !CPropertyPage::OnSetActive() )
      return FALSE;

   OnMaxTensionStressChanged();

   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2( broker, ILibrary, pLib );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( pParent->m_GeneralPage.m_Data.CriteriaName.c_str() );

   bool bAllowUserOverride;
   if ( pRatingEntry->GetSpecificationVersion() < WBFL::LRFD::LRFRVersionMgr::Version::SecondEditionWith2013Interims )
   {
      const CLiveLoadFactorModel& inventory = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory);
      bAllowUserOverride = inventory.AllowUserOverride();
   }
   else
   {
      const CLiveLoadFactorModel2& inventory = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrDesign_Inventory);
      bAllowUserOverride = inventory.AllowUserOverride();
   }

   CDataExchange dx(this,false);
   Float64 gLL = -1;
   if ( bAllowUserOverride )
   {
      GetDlgItem(IDC_STRENGTH_I_LL_INVENTORY)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_III_LL)->EnableWindow(TRUE);
   }
   else
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_INVENTORY,_T("Compute"),gLL);
      DDX_Keyword(&dx,IDC_SERVICE_III_LL,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_INVENTORY)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_III_LL)->EnableWindow(FALSE);
   }


   if ( pRatingEntry->GetSpecificationVersion() < WBFL::LRFD::LRFRVersionMgr::Version::SecondEditionWith2013Interims )
   {
      const CLiveLoadFactorModel& operating = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating);
      bAllowUserOverride = operating.AllowUserOverride();
   }
   else
   {
      const CLiveLoadFactorModel2& operating = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrDesign_Operating);
      bAllowUserOverride = operating.AllowUserOverride();
   }

   if ( bAllowUserOverride )
   {
      GetDlgItem(IDC_STRENGTH_I_LL_OPERATING)->EnableWindow(TRUE);
   }
   else
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_OPERATING,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_OPERATING)->EnableWindow(FALSE);
   }

   GET_IFACE2(broker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_STRENGTH_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS_LABEL)->ShowWindow(SW_HIDE);

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

void CDesignRatingPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_RATING_DESIGN_TAB );
}