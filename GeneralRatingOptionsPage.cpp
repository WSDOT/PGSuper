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

// GeneralRatingOptionsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"

#include "GeneralRatingOptionsPage.h"
#include "RatingOptionsDlg.h"
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CGeneralRatingOptionsPage, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// CGeneralRatingOptionsPage property page

CGeneralRatingOptionsPage::CGeneralRatingOptionsPage()
	: CPropertyPage(CGeneralRatingOptionsPage::IDD)
{
}

CGeneralRatingOptionsPage::~CGeneralRatingOptionsPage()
{
}

void CGeneralRatingOptionsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_CBStringExactCase(pDX, IDC_RATING_CRITERIA, m_Data.CriteriaName);

   DDX_Text(pDX,   IDC_SYSTEM_FACTOR_FLEXURE, m_Data.SystemFactorFlexure);
   DDX_Text(pDX,   IDC_SYSTEM_FACTOR_SHEAR,   m_Data.SystemFactorShear);
   DDX_Keyword(pDX,IDC_ADTT,_T("Unknown"),m_Data.ADTT);

   DDX_Check_Bool(pDX, IDC_PEDESTRIAN, m_Data.bIncludePedestrianLiveLoad);

   DDX_Check_Bool(pDX, IDC_DESIGN, m_Data.bDesignRating);
   DDX_Check_Bool(pDX, IDC_LEGAL,  m_Data.bLegalRating);
   DDX_Check_Bool(pDX, IDC_PERMIT, m_Data.bPermitRating);

}

BEGIN_MESSAGE_MAP(CGeneralRatingOptionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGeneralRatingOptionsPage)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CGeneralRatingOptionsPage message handlers
BOOL CGeneralRatingOptionsPage::OnInitDialog()
{
   EnableToolTips();

   CComboBox* pcbRatingSpec = (CComboBox*)GetDlgItem( IDC_RATING_CRITERIA );

   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_RatingSpecs.begin(); iter < m_RatingSpecs.end(); iter++ )
   {
      CString spec( (*iter).c_str() );
      pcbRatingSpec->AddString(spec);
   }

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGeneralRatingOptionsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_RATING_GENERAL_TAB );
}

BOOL CGeneralRatingOptionsPage::OnApply()
{
   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   if ( m_Data.bLegalRating && pParent->m_LegalPage.m_Data.RoutineNames.size() == 0 && pParent->m_LegalPage.m_Data.SpecialNames.size() == 0 )
   {
      int result = AfxChoose(_T("Load Rating Specification"),_T("Legal Load Rating is selected, however live loads are not defined."),_T("Select Live Loads\nSkip Legal Load Rating"));
      if ( result == 0 )
      {
         pParent->SetActivePage(&(pParent->m_LegalPage));
         return FALSE;
      }
      else
      {
         m_Data.bLegalRating = false;
      }
   }

   if ( m_Data.bPermitRating && pParent->m_PermitPage.m_Data.RoutinePermitNames.size() == 0 && pParent->m_PermitPage.m_Data.SpecialPermitNames.size() == 0 )
   {
      int result = AfxChoose(_T("Load Rating Specification"),_T("Permit Load Rating is selected, however live loads are not defined."),_T("Select Live Loads\nSkip Permit Load Rating"));
      if ( result == 0 )
      {
         pParent->SetActivePage(&(pParent->m_PermitPage));
         return FALSE;
      }
      else
      {
         m_Data.bPermitRating = false;
      }
   }

   return CPropertyPage::OnApply();
}

BOOL CGeneralRatingOptionsPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);

      switch(nID)
      {
      case IDC_ADTT:
         m_strTip = _T("Enter an ADTT value or enter the keyword Unknown");
         break;

      case IDC_PEDESTRIAN:
         m_strTip = _T("MBE 6A.2.3.4 - Pedestrian loads on sidewalks need not be considered simultaneously with vehicluar loads when load rating a bridge unless the Engineer has reason to expect that significant pedestrian loading will coincide with maximum vehicular loading");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}
