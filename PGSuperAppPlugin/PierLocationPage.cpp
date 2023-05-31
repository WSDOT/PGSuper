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

// PierLocationPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "PGSuperUnits.h"
#include "PierLocationPage.h"
#include "PierDetailsDlg.h"
#include "TimelineEventDlg.h"
#include "EditHaunchDlg.h"
#include "Utilities.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TimelineManager.h>

#include "SelectItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPierLocationPage property page

IMPLEMENT_DYNCREATE(CPierLocationPage, CPropertyPage)

CPierLocationPage::CPierLocationPage() : CPropertyPage(CPierLocationPage::IDD)
{
	//{{AFX_DATA_INIT(CPierLocationPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_MovePierOption = pgsTypes::MoveBridge;

   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
   ASSERT(SUCCEEDED(hr));
}

CPierLocationPage::~CPierLocationPage()
{
}

void CPierLocationPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierLocationPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_BACK_SLAB_OFFSET,  m_ctrlBackSlabOffset);
   DDX_Control(pDX, IDC_AHEAD_SLAB_OFFSET, m_ctrlAheadSlabOffset);

   // Set unit text regardless of input type
   DDX_Tag(pDX,IDC_BACK_SLAB_OFFSET_UNIT,pDisplayUnits->GetComponentDimUnit());
   DDX_Tag(pDX,IDC_AHEAD_SLAB_OFFSET_UNIT,pDisplayUnits->GetComponentDimUnit());

   DDX_Station(pDX,IDC_STATION,m_Station,pDisplayUnits->GetStationFormat());

   DDX_CBItemData(pDX,IDC_MOVE_PIER,m_MovePierOption);

   DDX_String(pDX,IDC_ORIENTATION,m_strOrientation);

   if ( pDX->m_bSaveAndValidate )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      pDX->PrepareEditCtrl(IDC_ORIENTATION);
      Float64 skewAngle;
      bool bSuccess = pBridge->GetSkewAngle(m_Station,m_strOrientation.c_str(),&skewAngle);
      if ( !bSuccess )
      {
         AfxMessageBox(_T("Invalid pier orientation"));
         pDX->Fail();
      }
      else if ( bSuccess && IsLT(fabs(skewAngle),0.0) || IsGE(MAX_SKEW_ANGLE,fabs(skewAngle)) )
      {
         AfxMessageBox(_T("Pier skew must be less than 88°\r\nPier skew is measured from the alignment normal"));
         pDX->Fail();
      }

      pParent->m_BridgeDesc.MovePier(pParent->m_pPier->GetIndex(), m_Station, m_MovePierOption);
      pParent->m_pPier->SetOrientation(m_strOrientation.c_str());

      UpdateHaunchAndCamberData(pDX);
            }
            else
            {
      UpdateHaunchAndCamberControls();
   }
}


BEGIN_MESSAGE_MAP(CPierLocationPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierLocationPage)
	ON_EN_CHANGE(IDC_STATION, OnChangeStation)
	ON_EN_KILLFOCUS(IDC_STATION, OnKillfocusStation)
	ON_CBN_SETFOCUS(IDC_MOVE_PIER, OnSetfocusMovePier)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_ERECTION_EVENT, OnErectionStageChanged)
   ON_CBN_DROPDOWN(IDC_ERECTION_EVENT, OnErectionStageChanging)
   ON_WM_CTLCOLOR()
   ON_BN_CLICKED(IDC_EDIT_HAUNCH_BUTTON,&OnBnClickedEditHaunchButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierLocationPage message handlers

BOOL CPierLocationPage::OnInitDialog() 
{
   FillEventList();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_ERECTION_EVENT)->EnableWindow(FALSE);
   }

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

	CPropertyPage::OnInitDialog();

   // move options are not available until the station changes
   UpdateMoveOptionList();


   EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetPierErectionEventIndex(m_PierID);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_ERECTION_EVENT,eventIdx);

   CString strPierType(pParent->m_pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));

   CString strGroupLabel;
   strGroupLabel.Format(_T("%s Line"),strPierType);
   GetDlgItem(IDC_LINE_GROUP)->SetWindowText(strGroupLabel);

   CString strPierLabel;
   strPierLabel.Format(_T("%s"),LABEL_PIER_EX(pParent->m_pPier->IsAbutment(), m_PierIdx));
   GetDlgItem(IDC_PIER_LABEL)->SetWindowText(strPierLabel);

   CString strStationLocation;
   strStationLocation.Format(_T("Station and Orientation defines the %s Line"),strPierType);
   GetDlgItem(IDC_STATION_LOCATION_LABEL)->SetWindowText(strStationLocation);
	
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CString fmt;
   fmt.LoadString( IS_SI_UNITS(pDisplayUnits) ? IDS_DLG_STATIONFMT_SI : IDS_DLG_STATIONFMT_US );
   GetDlgItem(IDC_STATION_FORMAT)->SetWindowText( fmt );

   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   GetDlgItem(IDC_ORIENTATION_FORMAT)->SetWindowText( fmt );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierLocationPage::Init(const CPierData2* pPier)
{
   const CBridgeDescription2* pBridgeDesc = pPier->GetBridgeDescription();

   m_PierIdx = pPier->GetIndex();
   m_PierID  = pPier->GetID();

   m_nSpans = pBridgeDesc->GetSpanCount();

   m_Station = pPier->GetStation();
   m_FromStation = m_Station; // keep a copy of this for the "move" note

   if ( m_PierIdx != 0 )
   {
      m_PrevPierStation = pBridgeDesc->GetPier(m_PierIdx-1)->GetStation();
   }
   else
   {
      m_PrevPierStation = -DBL_MAX;
   }

   if ( m_PierIdx != m_nSpans )
   {
      m_NextPierStation = pBridgeDesc->GetPier(m_PierIdx+1)->GetStation();
   }
   else
   {
      m_NextPierStation = -DBL_MAX;
   }

   m_strOrientation = pPier->GetOrientation();
}

void CPierLocationPage::OnChangeStation() 
{
   UpdateMoveOptionList();
}

BOOL CPierLocationPage::IsValidStation(Float64* pStation)
{
   BOOL bResult = TRUE;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   UnitModeType unitMode = pDisplayUnits->GetStationFormat().GetUnitOfMeasure() == WBFL::Units::StationFormat::UnitOfMeasure::Feet ? umUS : umSI;
   const WBFL::Units::Length& displayUnit = (unitMode == umUS ? WBFL::Units::Measure::Feet : WBFL::Units::Measure::Meter);

   CWnd* pWnd = GetDlgItem(IDC_STATION);
   CString strStation;
   pWnd->GetWindowText(strStation);
   HRESULT hr = m_objStation->FromString(CComBSTR(strStation),unitMode);
   if ( SUCCEEDED(hr) )
   {
      m_objStation->get_Value(pStation);
      *pStation = WBFL::Units::ConvertToSysUnits( *pStation, displayUnit );
      bResult = TRUE;
   }
   else
   {
      bResult = FALSE;
   }

   return bResult;
}

void CPierLocationPage::UpdateMoveOptionList()
{
   Float64 toStation;
   BOOL bIsValid = IsValidStation(&toStation);
   if ( bIsValid == FALSE )
   {
      CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_MOVE_PIER);
      pOptions->EnableWindow(FALSE);

      CEdit* pEdit = (CEdit*)GetDlgItem(IDC_STATION);
      pEdit->SetSel(-1,0);
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // get the current selection
   CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_MOVE_PIER);
   int curSel = Max(pOptions->GetCurSel(),0);

   pOptions->ResetContent();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CWnd* pMove = GetDlgItem(IDC_MOVE_LABEL);
   CString strMove;
   strMove.Format(_T("Move %s from %s to %s"),
                  LABEL_PIER_EX(pParent->m_pPier->IsAbutment(), m_PierIdx),
                  FormatStation(pDisplayUnits->GetStationFormat(),m_FromStation),
                  FormatStation(pDisplayUnits->GetStationFormat(),toStation)
                  );
   pMove->SetWindowText(strMove);

   CString strOptions[4];
   pgsTypes::MovePierOption options[4];
   strOptions[0].Format(_T("Move bridge retaining all span lengths"));
   options[0] = pgsTypes::MoveBridge;

   int nOptions = 1;

   if ( 1 < m_nSpans &&  // must have more than one span... moving an interior pier
        m_PierIdx != 0 && m_PierIdx != m_nSpans &&  // can't be first or last pier
        m_PrevPierStation < toStation && toStation < m_NextPierStation ) // can't move pier beyond adjacent piers
   {
      options[nOptions] = pgsTypes::AdjustAdjacentSpans;
      strOptions[nOptions++].Format(_T("Adjust the length of Spans %s and %s"),
                                    LABEL_SPAN(m_PierIdx),LABEL_SPAN(m_PierIdx+1));
   }

   if ( m_PierIdx == 0 && toStation < m_NextPierStation )
   {
      // adjust length of first span only
      options[nOptions] = pgsTypes::AdjustNextSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s by moving %s"),
                                       LABEL_SPAN(m_PierIdx), LABEL_PIER_EX(pParent->m_pPier->IsAbutment(), m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx));
      }
   }
   else if ( m_PierIdx == m_nSpans && m_PrevPierStation < toStation )
   {
      // adjust length of last span only
      options[nOptions] = pgsTypes::AdjustPrevSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s by moving %s"),
                                       LABEL_SPAN(m_PierIdx-1), LABEL_PIER_EX(pParent->m_pPier->IsAbutment(), m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }
   }
   else if ( 0 < m_PierIdx && m_PierIdx < m_nSpans )
   {
      if ( m_PrevPierStation < toStation )
      {
         // adjust length of previous span only
         options[nOptions] = pgsTypes::AdjustPrevSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }

      if ( toStation < m_NextPierStation )
      {
         // adjust length of next span only
         options[nOptions] = pgsTypes::AdjustNextSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %s, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx));
      }
   }

   for ( int i = 0; i < nOptions; i++ )
   {
      int idx = pOptions->AddString(strOptions[i]); 
      pOptions->SetItemData(idx,(DWORD)options[i]);
   }
   int result = pOptions->SetCurSel(curSel);
   if ( result == CB_ERR )
   {
      pOptions->SetCurSel(0);
   }

   int nShow = IsEqual(m_FromStation,toStation) ? SW_HIDE : SW_SHOW;
   GetDlgItem(IDC_MOVE_PIER)->EnableWindow(TRUE);
   GetDlgItem(IDC_MOVE_PIER)->ShowWindow(nShow);
   GetDlgItem(IDC_MOVE_LABEL)->ShowWindow(nShow);
}

void CPierLocationPage::OnKillfocusStation() 
{
   UpdateMoveOptionList();
}

void CPierLocationPage::OnSetfocusMovePier() 
{
   UpdateMoveOptionList();
}

void CPierLocationPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PIERDETAILS_GENERAL );
}

HBRUSH CPierLocationPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hbr = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);

   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_STATION:
      {
         Float64 toStation;
         if ( IsValidStation(&toStation) )
         {
            pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
         }
         else
         {
            pDC->SetTextColor(RED);
         }
      }
      break;

   };

   return hbr;
}

void CPierLocationPage::FillEventList()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CComboBox* pcbErect = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);

   int erectIdx = pcbErect->GetCurSel();

   pcbErect->ResetContent();

   const CTimelineManager* pTimelineMgr = pParent->GetBridgeDescription()->GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbErect->SetItemData(pcbErect->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbErect->SetItemData(pcbErect->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( erectIdx != CB_ERR )
   {
      pcbErect->SetCurSel(erectIdx);
   }
   else
   {
      pcbErect->SetCurSel(0);
   }
}

void CPierLocationPage::OnErectionStageChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CPierLocationPage::OnErectionStageChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (IndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      if ( eventIdx == INVALID_INDEX )
      {
         pCB->SetCurSel(m_PrevEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pParent->m_BridgeDesc.GetTimelineManager()->SetPierErectionEventByIndex(pParent->m_pPier->GetIndex(),eventIdx);
   pCB->SetCurSel((int)eventIdx);
}

EventIndexType CPierLocationPage::CreateEvent()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CTimelineManager* pTimelineMgr = pParent->GetBridgeDescription()->GetTimelineManager();

   CTimelineEventDlg dlg(*pTimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType idx;
      pTimelineMgr->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&idx);
      return idx;
  }

   return INVALID_INDEX;
}

void CPierLocationPage::UpdateHaunchAndCamberControls()
{
   // Function takes bridge data and puts into dialog controls
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   m_PierFaceCount = 0;

   pgsTypes::HaunchInputDepthType inputType = pParent->m_BridgeDesc.GetHaunchInputDepthType();

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      // No deck, no haunch
      DisableHaunchAndCamberControls();
   }
   else
   {
      // First deal with visibility of controls. 
      if (inputType == pgsTypes::hidACamber && !pParent->m_pPier->HasSlabOffset())
      {
         DisableHaunchAndCamberControls();
      }
      else
      {
         if (pParent->m_pPier->IsAbutment())
         {
            m_PierFaceCount = 1;

            if (pParent->m_pPier->GetPrevSpan() == nullptr)
            {
               GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_BACK_SLAB_OFFSET)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

               CRect rOffset,rUnit;
               GetDlgItem(IDC_BACK_SLAB_OFFSET)->GetWindowRect(&rOffset);
               GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->GetWindowRect(&rUnit);

               ScreenToClient(&rOffset);
               ScreenToClient(&rUnit);

               GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->MoveWindow(rOffset);
               GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->MoveWindow(rUnit);
            }
            else
            {
               GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->ShowWindow(SW_HIDE);
               GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);
      }
   }
         else if (pParent->m_pPier->IsInteriorPier())
   {
            // when segments are continuous over a pier, there isn't an "A" dimension at the pier
            int nShowCmd;
            if (inputType == pgsTypes::hidACamber && IsSegmentContinuousOverPier(pParent->m_pPier->GetSegmentConnectionType()) )
      {
               m_PierFaceCount = 1;
               nShowCmd = SW_HIDE;
      }
      else
      {
               m_PierFaceCount = 2;
               nShowCmd = SW_SHOW;
      }

            GetDlgItem(IDC_SLAB_OFFSET_GROUP)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_BACK_SLAB_OFFSET)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->ShowWindow(nShowCmd);
            GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->ShowWindow(nShowCmd);
   }

         GetDlgItem(IDC_EDIT_HAUNCH_BUTTON)->EnableWindow(TRUE);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
         GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);

         if (inputType == pgsTypes::hidACamber)
         {
            // Input is via slab offset
            pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
            GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Slab Offset (\"A\" Dimension)"));

            BOOL bEnable = TRUE;
            if (slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotSegment)
            {
               bEnable = FALSE;
            }

            m_ctrlBackSlabOffset.EnableWindow(bEnable);
            m_ctrlAheadSlabOffset.EnableWindow(bEnable);

            // Now that visibility is established, set values in control text
            // Put whole bridge value into disabled controls if needed
            CString strSlabOffset;
            if (slabOffsetType == pgsTypes::sotBridge)
      {
               Float64 allBridgeA = pParent->m_BridgeDesc.GetSlabOffset();
               strSlabOffset.Format(_T("%s"),FormatDimension(allBridgeA,pDisplayUnits->GetComponentDimUnit(),false));
               m_ctrlAheadSlabOffset.SetWindowText(strSlabOffset);
               m_ctrlBackSlabOffset.SetWindowText(strSlabOffset);
            }
            else if (slabOffsetType == pgsTypes::sotSegment)
            {
               m_ctrlAheadSlabOffset.SetWindowText(_T(""));
               m_ctrlBackSlabOffset.SetWindowText(_T(""));
            }
            else if (slabOffsetType == pgsTypes::sotBearingLine)
            {
               if (m_PierFaceCount == 1)
               {
                  // the UI has only one input parameter
                  ATLASSERT(pParent->m_pPier->IsAbutment());
                  if (pParent->m_pPier->GetPrevSpan() == nullptr)
                  {
                     // We are at the start of the bridge so the data is in the ahead controls
                     Float64 aheadSlabOffset = pParent->m_pPier->GetSlabOffset(pgsTypes::Ahead);
                     strSlabOffset.Format(_T("%s"),FormatDimension(aheadSlabOffset,pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlBackSlabOffset.SetWindowText(_T(""));
                     m_ctrlAheadSlabOffset.SetWindowText(strSlabOffset);
                  }
                  else
                  {
                     // we are at the end of the bridge so the data is in the back controls
                     Float64 backSlabOffset = pParent->m_pPier->GetSlabOffset(pgsTypes::Back);
                     strSlabOffset.Format(_T("%s"),FormatDimension(backSlabOffset,pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlAheadSlabOffset.SetWindowText(_T(""));
                     m_ctrlBackSlabOffset.SetWindowText(strSlabOffset);
                  }
               }
               else
               {
                  Float64 backSlabOffset = pParent->m_pPier->GetSlabOffset(pgsTypes::Back);
                  strSlabOffset.Format(_T("%s"),FormatDimension(backSlabOffset,pDisplayUnits->GetComponentDimUnit(),false));
                  m_ctrlBackSlabOffset.SetWindowText(strSlabOffset);

                  Float64 aheadSlabOffset = pParent->m_pPier->GetSlabOffset(pgsTypes::Ahead);
                  strSlabOffset.Format(_T("%s"),FormatDimension(aheadSlabOffset,pDisplayUnits->GetComponentDimUnit(),false));
                  m_ctrlAheadSlabOffset.SetWindowText(strSlabOffset);
               }
            }
            else
            {
               ATLASSERT(0); // ??
            }
         } // end A Camber
         else
         {
            // Direct input of haunch depths
            if (inputType == pgsTypes::hidHaunchDirectly)
            {
               GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Haunch Depth"));
            }
            else
         {
               GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Haunch+Slab Depth"));
            }

            const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();
            Float64 Tdeck;
            if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
            {
               Tdeck = pDeck->GrossDepth + pDeck->PanelDepth;
            }
            else
            {
               Tdeck = pDeck->GrossDepth;
            }

            pgsTypes::HaunchInputLocationType haunchInputLocationType = pParent->m_BridgeDesc.GetHaunchInputLocationType();
            pgsTypes::HaunchLayoutType haunchLayoutType = pParent->m_BridgeDesc.GetHaunchLayoutType();
            pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pParent->m_BridgeDesc.GetHaunchInputDistributionType();

            bool bHasBackHaunch(true),bHasAheadHaunch(true);
            if (m_PierFaceCount == 1)
            {
               ATLASSERT(pParent->m_pPier->IsAbutment());
               if (pParent->m_pPier->GetPrevSpan() == nullptr)
               {
                  bHasBackHaunch = false;
         }
         else
         {
                  bHasAheadHaunch = false;
               }
            }

            BOOL bEnableBack(FALSE),bEnableAhead(FALSE);
            if (haunchLayoutType == pgsTypes::hltAlongSpans && haunchInputLocationType == pgsTypes::hilSame4AllGirders && haunchInputDistributionType == pgsTypes::hidAtEnds)
            {
               // only case where we might allow editing of data
               bEnableBack = bHasBackHaunch ? TRUE : FALSE;
               bEnableAhead = bHasAheadHaunch ? TRUE : FALSE;
            }

            m_ctrlBackSlabOffset.EnableWindow(bEnableBack);
            m_ctrlAheadSlabOffset.EnableWindow(bEnableAhead);

            m_ctrlAheadSlabOffset.SetWindowText(_T(""));
            m_ctrlBackSlabOffset.SetWindowText(_T(""));

            // Now that visibility is established, set values in control text
            Float64 haunchDepth;
            CString strHaunchVal;
            if (haunchInputLocationType == pgsTypes::hilSame4Bridge && haunchLayoutType == pgsTypes::hltAlongSpans)
            {
               // Put whole bridge value into disabled controls if needed
               std::vector<Float64> allBridgeHaunches = pParent->m_BridgeDesc.GetDirectHaunchDepths();
               if (haunchInputDistributionType == pgsTypes::hidAtEnds)
               {
                  ATLASSERT(allBridgeHaunches.size() == 2);
                  if (bHasBackHaunch)
                  {
                     haunchDepth = allBridgeHaunches.back() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
                     strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth, pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlBackSlabOffset.SetWindowText(strHaunchVal);
                  }

                  if (bHasAheadHaunch)
                  {
                     haunchDepth = allBridgeHaunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
                     strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlAheadSlabOffset.SetWindowText(strHaunchVal);
                  }
               }
               else if (haunchInputDistributionType == pgsTypes::hidUniform)
               {
                  ATLASSERT(allBridgeHaunches.size() == 1);
                  haunchDepth = allBridgeHaunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);

                  if (bHasBackHaunch)
                  {
                     strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlBackSlabOffset.SetWindowText(strHaunchVal);
                  }

                  if (bHasAheadHaunch)
                  {
                     strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
                     m_ctrlAheadSlabOffset.SetWindowText(strHaunchVal);
                  }
         }
      }
            else if (bEnableBack || bEnableAhead)
            {
               if (bEnableBack)
               {
                  // we are at the end of the bridge so the data is in the back controls
                  std::vector<Float64> backHaunches = pParent->m_pPier->GetPrevSpan()->GetDirectHaunchDepths(0);
                  ATLASSERT(backHaunches.size() == 2);
                  haunchDepth = backHaunches.back() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
                  strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
                  m_ctrlBackSlabOffset.SetWindowText(strHaunchVal);
               }

               if (bEnableAhead)
               {
                  std::vector<Float64> aheadHaunches = pParent->m_pPier->GetNextSpan()->GetDirectHaunchDepths(0);
                  ATLASSERT(aheadHaunches.size() == 2);
                  haunchDepth = aheadHaunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
                  strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
                  m_ctrlAheadSlabOffset.SetWindowText(strHaunchVal);
               }
            }
         }  // end direct haunch
   }
   }
}

void CPierLocationPage::DisableHaunchAndCamberControls()
{
   GetDlgItem(IDC_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);
   GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
   m_ctrlBackSlabOffset.EnableWindow(FALSE);
   m_ctrlBackSlabOffset.SetWindowText(_T(""));
   GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->EnableWindow(FALSE);

   GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
   m_ctrlAheadSlabOffset.ShowWindow(SW_HIDE);
   GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON)->EnableWindow(FALSE);
}

void CPierLocationPage::UpdateHaunchAndCamberData(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      return; // No deck, no haunch. Go no further
   }
   else
   {
      pgsTypes::HaunchInputDepthType inputType = pParent->m_BridgeDesc.GetHaunchInputDepthType();
      if (inputType == pgsTypes::hidACamber)
      {
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
         if (slabOffsetType == pgsTypes::sotBearingLine)
         {
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

            Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();
            CString strMinValError;
            strMinValError.Format(_T("Slab Offset must be greater or equal to slab depth (%s)"),FormatDimension(minSlabOffset,pDisplayUnits->GetComponentDimUnit()));

            // Determine which values are needed
            bool bHasBack(true),bHasAhead(true);
            if (m_PierFaceCount == 1)
            {
               ATLASSERT(pParent->m_pPier->IsAbutment());
               if (pParent->m_pPier->GetPrevSpan() == nullptr)
               {
                  bHasBack = false;
               }
               else
               {
                  bHasAhead = false;
               }
            }

            // Get current values out of the controls
            CDataExchange dx(this,TRUE);
            Float64 slabOffset;
            if (bHasBack)
            {
               DDX_UnitValueAndTag(&dx,IDC_BACK_SLAB_OFFSET,IDC_BACK_SLAB_OFFSET_UNIT,slabOffset,pDisplayUnits->GetComponentDimUnit());
               if (::IsLT(slabOffset,minSlabOffset))
   {
                  pDX->PrepareCtrl(IDC_BACK_SLAB_OFFSET);
                  AfxMessageBox(strMinValError);
                  pDX->Fail();
   }

               pParent->m_pPier->SetSlabOffset(pgsTypes::Back,slabOffset);
            }

            if (bHasAhead)
            {
               DDX_UnitValueAndTag(&dx,IDC_AHEAD_SLAB_OFFSET,IDC_AHEAD_SLAB_OFFSET_UNIT,slabOffset,pDisplayUnits->GetComponentDimUnit());

               if (::IsLT(slabOffset,minSlabOffset))
               {
                  pDX->PrepareCtrl(IDC_AHEAD_SLAB_OFFSET);
                  AfxMessageBox(strMinValError);
                  pDX->Fail();
               }

               pParent->m_pPier->SetSlabOffset(pgsTypes::Ahead,slabOffset);
            }
         }
      }
      else
      {
         // Direct input of haunch
         pgsTypes::HaunchInputLocationType haunchInputLocationType = pParent->m_BridgeDesc.GetHaunchInputLocationType();
         pgsTypes::HaunchLayoutType haunchLayoutType = pParent->m_BridgeDesc.GetHaunchLayoutType();
         pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pParent->m_BridgeDesc.GetHaunchInputDistributionType();

         if (haunchLayoutType == pgsTypes::hltAlongSpans && haunchInputLocationType == pgsTypes::hilSame4AllGirders && haunchInputDistributionType == pgsTypes::hidAtEnds)
         {
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

            const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();
            Float64 Tdeck;
            if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
   {
               Tdeck = pDeck->GrossDepth + pDeck->PanelDepth;
            }
            else
      {
               Tdeck = pDeck->GrossDepth;
            }

            Float64 minHaunch = pParent->m_BridgeDesc.GetMinimumAllowableHaunchDepth(inputType);

            CString strMinValError;
            if (inputType == pgsTypes::hidHaunchPlusSlabDirectly)
            {
               strMinValError.Format(_T("Haunch Depth must be greater or equal to fillet (%s)"),FormatDimension(minHaunch,pDisplayUnits->GetComponentDimUnit()));
            }
            else
            {
               strMinValError.Format(_T("Haunch+Slab Depth must be greater or equal to deck depth+fillet (%s)"),FormatDimension(minHaunch,pDisplayUnits->GetComponentDimUnit()));
            }

            // Determine which values are needed
            bool bHasBack(true),bHasAhead(true);
            if (m_PierFaceCount == 1)
            {
               ATLASSERT(pParent->m_pPier->IsAbutment());
               if (pParent->m_pPier->GetPrevSpan() == nullptr)
               {
                  bHasBack = false;
      }
      else
      {
                  bHasAhead = false;
               }
            }

            // Get current values out of the controls
            CDataExchange dx(this,TRUE);
            Float64 haunchDepth;
            if (bHasBack)
            {
               DDX_UnitValueAndTag(&dx,IDC_BACK_SLAB_OFFSET,IDC_BACK_SLAB_OFFSET_UNIT,haunchDepth,pDisplayUnits->GetComponentDimUnit());
               if (::IsLT(haunchDepth,minHaunch))
               {
                  pDX->PrepareCtrl(IDC_BACK_SLAB_OFFSET);
                  AfxMessageBox(strMinValError);
                  pDX->Fail();
      }

               haunchDepth -= (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
               std::vector<Float64> haunchDepths = pParent->m_pPier->GetPrevSpan()->GetDirectHaunchDepths(0);
               haunchDepths.back() = haunchDepth;
               pParent->m_pPier->GetPrevSpan()->SetDirectHaunchDepths(haunchDepths);
   }

            if (bHasAhead)
            {
               DDX_UnitValueAndTag(&dx,IDC_AHEAD_SLAB_OFFSET,IDC_AHEAD_SLAB_OFFSET_UNIT,haunchDepth,pDisplayUnits->GetComponentDimUnit());

               if (::IsLT(haunchDepth,minHaunch))
   {
                  pDX->PrepareCtrl(IDC_AHEAD_SLAB_OFFSET);
                  AfxMessageBox(strMinValError);
                  pDX->Fail();
               }

               haunchDepth -= (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
               std::vector<Float64> haunchDepths = pParent->m_pPier->GetNextSpan()->GetDirectHaunchDepths(0);
               haunchDepths.front() = haunchDepth;
               pParent->m_pPier->GetNextSpan()->SetDirectHaunchDepths(haunchDepths);
            }
         }
      }
   }
}

BOOL CPierLocationPage::OnSetActive()
{
   BOOL bResult = CPropertyPage::OnSetActive();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();


   return bResult;
}

void CPierLocationPage::OnBnClickedEditHaunchButton()
{
   // Need to validate main dialog data before we go into haunch dialog
   try
   {
      if (TRUE != UpdateData(TRUE))
      {
         return;
      }
   }
   catch (...)
   {
      ATLASSERT(0);
      return;
   }

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   CEditHaunchDlg dlg(&(pParent->m_BridgeDesc));
   if (dlg.DoModal() == IDOK)
   {
      // Cannot copy entire bridge description here because this dialog has hooks into pointers withing bridgedescr.
      // Use function to copy haunch and slab offset data
      pParent->m_BridgeDesc.CopyHaunchSettings(dlg.m_BridgeDesc);

      UpdateHaunchAndCamberControls();
   }
}
