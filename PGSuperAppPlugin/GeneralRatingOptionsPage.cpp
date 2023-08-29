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

// GeneralRatingOptionsPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"

#include "GeneralRatingOptionsPage.h"
#include "RatingOptionsDlg.h"
#include "TimelineEventDlg.h"

#include <IFace\Project.h>
#include <EAF\EAFDocument.h>

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

   EventIndexType loadRatingEventIdx;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBItemData(pDX, IDC_LOAD_RATING_EVENT, loadRatingEventIdx );
      m_Data.TimelineMgr.SetLoadRatingEventByIndex(loadRatingEventIdx);
   }
   else
   {
      loadRatingEventIdx = m_Data.TimelineMgr.GetLoadRatingEventIndex();
      DDX_CBItemData(pDX, IDC_LOAD_RATING_EVENT, loadRatingEventIdx );
   }

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
   ON_CBN_DROPDOWN(IDC_LOAD_RATING_EVENT, OnLoadRatingEventChanging)
   ON_CBN_SELCHANGE(IDC_LOAD_RATING_EVENT, OnLoadRatingEventChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CGeneralRatingOptionsPage message handlers
BOOL CGeneralRatingOptionsPage::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   BOOL bEnable = pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? TRUE : FALSE;
   GetDlgItem(IDC_LOAD_RATING_EVENT)->EnableWindow(bEnable);

   EnableToolTips();

   CComboBox* pcbRatingSpec = (CComboBox*)GetDlgItem( IDC_RATING_CRITERIA );

   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_RatingSpecs.begin(); iter < m_RatingSpecs.end(); iter++ )
   {
      CString spec( (*iter).c_str() );
      pcbRatingSpec->AddString(spec);
   }

   FillLoadRatingEventComboBox();
   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGeneralRatingOptionsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_RATING_GENERAL_TAB );
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
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CGeneralRatingOptionsPage::FillLoadRatingEventComboBox()
{
   CComboBox* pcbLoadRatingEvents = (CComboBox*)GetDlgItem(IDC_LOAD_RATING_EVENT);
   int curSel = pcbLoadRatingEvents->GetCurSel();
   pcbLoadRatingEvents->ResetContent();

   EventIndexType firstEventIdx = m_Data.TimelineMgr.GetLiveLoadEventIndex();
   EventIndexType nEvents = m_Data.TimelineMgr.GetEventCount();
   for ( EventIndexType eventIdx = firstEventIdx; eventIdx < nEvents; eventIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),m_Data.TimelineMgr.GetEventByIndex(eventIdx)->GetDescription());
      int idx = pcbLoadRatingEvents->AddString(strLabel);
      pcbLoadRatingEvents->SetItemData(idx,(DWORD_PTR)eventIdx);
   }
   int idx = pcbLoadRatingEvents->AddString(_T("Create Event..."));
   pcbLoadRatingEvents->SetItemData(idx,(DWORD_PTR)CREATE_TIMELINE_EVENT);

   if ( pcbLoadRatingEvents->SetCurSel(curSel) == CB_ERR )
   {
      pcbLoadRatingEvents->SetCurSel(pcbLoadRatingEvents->GetCount()-2);
   }
}

void CGeneralRatingOptionsPage::OnLoadRatingEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOAD_RATING_EVENT);
   m_PrevLoadRatingEventIdx = pCB->GetCurSel();
}

void CGeneralRatingOptionsPage::OnLoadRatingEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOAD_RATING_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (EventIndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      FillLoadRatingEventComboBox();
   }

   if (eventIdx == INVALID_INDEX)
   {
      // event creation was canceled... restore the original selection
      pCB->SetCurSel((int)m_PrevLoadRatingEventIdx);
   }
   else
   {
      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx,IDC_LOAD_RATING_EVENT,eventIdx);
   }
}

EventIndexType CGeneralRatingOptionsPage::CreateEvent()
{
   CTimelineEventDlg dlg(m_Data.TimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = m_Data.TimelineMgr.AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      return eventIdx;
  }

   return INVALID_INDEX;
}
