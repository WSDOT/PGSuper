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
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierLocationPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_SLAB_OFFSET_TYPE, m_ctrlSlabOffsetType);
   DDX_Control(pDX, IDC_BACK_SLAB_OFFSET,  m_ctrlBackSlabOffset);
   DDX_Control(pDX, IDC_AHEAD_SLAB_OFFSET, m_ctrlAheadSlabOffset);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_Station(pDX,IDC_STATION,m_Station,pDisplayUnits->GetStationFormat());

   DDX_CBItemData(pDX,IDC_MOVE_PIER,m_MovePierOption);

   DDX_String(pDX,IDC_ORIENTATION,m_strOrientation);

   DDX_UnitValueAndTag(pDX,IDC_BACK_SLAB_OFFSET,  IDC_BACK_SLAB_OFFSET_UNIT,  m_SlabOffset[pgsTypes::Back],  pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_AHEAD_SLAB_OFFSET, IDC_AHEAD_SLAB_OFFSET_UNIT, m_SlabOffset[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());

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

      pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
      if (deckType != pgsTypes::sdtNone && pParent->m_pPier->HasSlabOffset())
      {
         Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();
         CString strMinValError;
         strMinValError.Format(_T("Slab Offset must be greater or equal to slab depth (%s)"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));

         if (::IsLT(m_SlabOffset[pgsTypes::Back], minSlabOffset))
         {
            pDX->PrepareCtrl(IDC_BACK_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         if (::IsLT(m_SlabOffset[pgsTypes::Ahead], minSlabOffset))
         {
            pDX->PrepareCtrl(IDC_AHEAD_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         // Copy the temporary data from this dialog in to actual bridge model
         if ( pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBridge )
         {
            if ( pParent->m_pPier->GetGirderGroup(pgsTypes::Back) )
            {
               pParent->m_BridgeDesc.SetSlabOffset(m_SlabOffset[pgsTypes::Back]);
            }
            else
            {
               pParent->m_BridgeDesc.SetSlabOffset(m_SlabOffset[pgsTypes::Ahead]);
            }
         }
         else if ( pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBearingLine )
         {
            if ( m_PierFaceCount == 1 )
            {
               // the UI has only one input parameter
               ATLASSERT(pParent->m_pPier->IsAbutment());
               if ( pParent->m_pPier->GetPrevSpan() == nullptr )
               {
                  // we are at the start of the bridge so the data is in the ahead controls
                  pParent->m_pPier->SetSlabOffset(pgsTypes::Ahead,m_SlabOffset[pgsTypes::Ahead]);
               }
               else
               {
                  // we are at the end of the bridge so the data is in the back controls
                  pParent->m_pPier->SetSlabOffset(pgsTypes::Back,m_SlabOffset[pgsTypes::Back]);
               }
            }
            else
            {
               pParent->m_pPier->SetSlabOffset(pgsTypes::Back,m_SlabOffset[pgsTypes::Back]);
               pParent->m_pPier->SetSlabOffset(pgsTypes::Ahead,m_SlabOffset[pgsTypes::Ahead]);
            }
         }
         else
         {
            ATLASSERT(pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotSegment);
            if ( m_InitialSlabOffsetType != pgsTypes::sotSegment )
            {
               // Slab offset started off as Bridge or Pier and now it is Girder... this means the
               // slab offset at this Pier applies to all girders/segments
               ATLASSERT(IsEqual(m_SlabOffset[pgsTypes::Back], m_SlabOffset[pgsTypes::Ahead]));
               std::function<void(CPrecastSegmentData*, void*)> fn = UpdateSlabOffset;
               pParent->m_BridgeDesc.ForEachSegment(fn, (void*)&m_SlabOffset);
            }
         }
      }
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
   ON_CBN_SELCHANGE(IDC_SLAB_OFFSET_TYPE, OnChangeSlabOffset)
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

	CPropertyPage::OnInitDialog();

   // move options are not available until the station changes
   UpdateMoveOptionList();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();

   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   if (deckType == pgsTypes::sdtNone)
   {
      // no deck, no slab offset
      m_ctrlSlabOffsetType.EnableWindow(FALSE);

      GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);
      m_ctrlBackSlabOffset.EnableWindow(FALSE);
      GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->EnableWindow(FALSE);
      m_ctrlAheadSlabOffset.EnableWindow(FALSE);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->EnableWindow(FALSE);
   }
   else
   {
      if (m_InitialSlabOffsetType == pgsTypes::sotBridge )
      {
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBridge,bIsPGSuper));
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine,bIsPGSuper));
      }
      else if ( m_InitialSlabOffsetType == pgsTypes::sotSegment )
      {
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotSegment,bIsPGSuper));
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine,bIsPGSuper));
      }
      else
      {
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBearingLine,bIsPGSuper));
         m_ctrlSlabOffsetType.AddString(GetSlabOffsetTypeAsString(pgsTypes::sotBridge,bIsPGSuper));
      }

      m_ctrlSlabOffsetType.SetCurSel(0);

      m_ctrlBackSlabOffset.ShowDefaultWhenDisabled(TRUE);
      m_ctrlAheadSlabOffset.ShowDefaultWhenDisabled(TRUE);
   }

   EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetPierErectionEventIndex(m_PierID);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_ERECTION_EVENT,eventIdx);

   CString strPierType(pParent->m_pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));

   CString strGroupLabel;
   strGroupLabel.Format(_T("%s Line"),strPierType);
   GetDlgItem(IDC_LINE_GROUP)->SetWindowText(strGroupLabel);

   CString strPierLabel;
   strPierLabel.Format(_T("%s %d"),strPierType,LABEL_PIER(m_PierIdx));
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

   UpdateSlabOffsetWindowState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierLocationPage::Init(const CPierData2* pPier)
{
   const CBridgeDescription2* pBridgeDesc = pPier->GetBridgeDescription();

   m_PierIdx = pPier->GetIndex();
   m_PierID  = pPier->GetID();

   m_InitialSlabOffsetType = pBridgeDesc->GetSlabOffsetType();

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

   const CGirderGroupData* pBackGroup = pPier->GetGirderGroup(pgsTypes::Back);
   const CGirderGroupData* pAheadGroup = pPier->GetGirderGroup(pgsTypes::Ahead);
   if (pBackGroup)
   {
      m_SlabOffset[pgsTypes::Back] = pPier->GetSlabOffset(pgsTypes::Back, m_InitialSlabOffsetType == pgsTypes::sotBearingLine ? false : true);

      if (!pAheadGroup)
      {
         m_SlabOffset[pgsTypes::Ahead] = m_SlabOffset[pgsTypes::Back]; // fill with decent values so we don't have garbage
      }
   }

   if (pAheadGroup)
   {
      m_SlabOffset[pgsTypes::Ahead] = pPier->GetSlabOffset(pgsTypes::Ahead, m_InitialSlabOffsetType == pgsTypes::sotBearingLine ? false : true);

      if (!pBackGroup)
      {
         m_SlabOffset[pgsTypes::Back] = m_SlabOffset[pgsTypes::Ahead]; // fill with decent values so we don't have garbage
      }
   }
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

   UnitModeType unitMode = pDisplayUnits->GetStationFormat().GetUnitOfMeasure() == unitStationFormat::Feet ? umUS : umSI;
   const unitLength& displayUnit = (unitMode == umUS ? unitMeasure::Feet : unitMeasure::Meter);

   CWnd* pWnd = GetDlgItem(IDC_STATION);
   CString strStation;
   pWnd->GetWindowText(strStation);
   HRESULT hr = m_objStation->FromString(CComBSTR(strStation),unitMode);
   if ( SUCCEEDED(hr) )
   {
      m_objStation->get_Value(pStation);
      *pStation = ::ConvertToSysUnits( *pStation, displayUnit );
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
   CString strName = (pParent->m_pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));


   CWnd* pMove = GetDlgItem(IDC_MOVE_LABEL);
   CString strMove;
   strMove.Format(_T("Move %s %d from %s to %s"),
                  strName,
                  LABEL_PIER(m_PierIdx),
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
      strOptions[nOptions++].Format(_T("Adjust the length of Spans %d and %d"),
                                    LABEL_SPAN(m_PierIdx),LABEL_SPAN(m_PierIdx+1));
   }

   if ( m_PierIdx == 0 && toStation < m_NextPierStation )
   {
      // adjust length of first span only
      options[nOptions] = pgsTypes::AdjustNextSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d by moving %s %d"),
                                       LABEL_SPAN(m_PierIdx),strName,LABEL_SPAN(m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx));
      }
   }
   else if ( m_PierIdx == m_nSpans && m_PrevPierStation < toStation )
   {
      // adjust length of last span only
      options[nOptions] = pgsTypes::AdjustPrevSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d by moving %s %d"),
                                       LABEL_SPAN(m_PierIdx-1),strName,LABEL_SPAN(m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }
   }
   else if ( 0 < m_PierIdx && m_PierIdx < m_nSpans )
   {
      if ( m_PrevPierStation < toStation )
      {
         // adjust length of previous span only
         options[nOptions] = pgsTypes::AdjustPrevSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }

      if ( toStation < m_NextPierStation )
      {
         // adjust length of next span only
         options[nOptions] = pgsTypes::AdjustNextSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
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

void CPierLocationPage::OnChangeSlabOffset()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   if ( m_InitialSlabOffsetType == pgsTypes::sotBridge || m_InitialSlabOffsetType == pgsTypes::sotBearingLine )
   {
      // if slab offset was bridge or pier when the dialog was created, toggle between
      // bridge and pier mode
      if (slabOffsetType == pgsTypes::sotBearingLine)
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBridge);
      }
      else
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBearingLine);
      }
   }
   else
   {
      // if slab offset was girder when the dialog was created, toggle between
      // girder and pier
      ATLASSERT(m_InitialSlabOffsetType == pgsTypes::sotSegment);
      if ( slabOffsetType == pgsTypes::sotBearingLine )
      {
         // going from pier to girder so both ends of girder are the same. default values can be shown
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotSegment);
         m_ctrlBackSlabOffset.ShowDefaultWhenDisabled(TRUE);
         m_ctrlAheadSlabOffset.ShowDefaultWhenDisabled(TRUE);
      }
      else
      {
         pParent->m_BridgeDesc.SetSlabOffsetType(pgsTypes::sotBearingLine);
      }
   }

   if ( slabOffsetType == pgsTypes::sotBearingLine && pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotBridge )
   {
      // going from span-by-span to one for the entire bridge
      // need to check the span values and if they are different, ask the user which one is to
      // be used for the entire bridge

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      // get current values out of the controls
      std::array<Float64,2> slabOffset;
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_BACK_SLAB_OFFSET,  IDC_BACK_SLAB_OFFSET_UNIT,  slabOffset[pgsTypes::Back],  pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_AHEAD_SLAB_OFFSET, IDC_AHEAD_SLAB_OFFSET_UNIT, slabOffset[pgsTypes::Ahead], pDisplayUnits->GetComponentDimUnit());

      // take start value as default
      Float64 slab_offset = slabOffset[pgsTypes::Back];

      // check if start/end values are equal
      if ( m_PierFaceCount == 2 && !IsEqual(slabOffset[pgsTypes::Back],slabOffset[pgsTypes::Ahead]) )
      {
         // nope... make the user select which slab offset to use
         CSelectItemDlg dlg;
         dlg.m_ItemIdx = 0;
         dlg.m_strTitle = _T("Select Slab Offset");
         dlg.m_strLabel = _T("A single slab offset will be used for the entire bridge. Select a value.");
         
         CString strItems;
         strItems.Format(_T("Back side of pier (%s)\nAhead side of pier (%s)"),
                         ::FormatDimension(slabOffset[pgsTypes::Back],pDisplayUnits->GetComponentDimUnit()),
                         ::FormatDimension(slabOffset[pgsTypes::Ahead],  pDisplayUnits->GetComponentDimUnit()));

         dlg.m_strItems = strItems;
         if ( dlg.DoModal() == IDOK )
         {
            if (dlg.m_ItemIdx == 0)
            {
               slab_offset = slabOffset[pgsTypes::Back];
            }
            else
            {
               slab_offset = slabOffset[pgsTypes::Ahead];
            }
         }
         else
         {
            // user cancelled... get the heck outta here
            return;
         }
      }

      // put the data back in the controls
      dx.m_bSaveAndValidate = FALSE;
      DDX_UnitValueAndTag(&dx, IDC_BACK_SLAB_OFFSET,  IDC_BACK_SLAB_OFFSET_UNIT,  slab_offset, pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_AHEAD_SLAB_OFFSET, IDC_AHEAD_SLAB_OFFSET_UNIT, slab_offset, pDisplayUnits->GetComponentDimUnit());
   }

   UpdateSlabOffsetWindowState();

   return;
}

void CPierLocationPage::UpdateSlabOffsetWindowState()
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

   BOOL bEnable = TRUE;
   if ( slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotSegment )
   {
      bEnable = FALSE;
   }

   m_ctrlBackSlabOffset.EnableWindow(bEnable);
   m_ctrlAheadSlabOffset.EnableWindow(bEnable);
}


BOOL CPierLocationPage::OnSetActive()
{
   BOOL bResult = CPropertyPage::OnSetActive();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   m_PierFaceCount = pParent->m_pPier->IsBoundaryPier() && !pParent->m_pPier->IsAbutment() ? 2 : 1;

   if (pParent->m_pPier->IsAbutment())
   {
      ATLASSERT(m_PierFaceCount == 1);
      if (pParent->m_pPier->GetPrevSpan() == nullptr)
      {
         GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_BACK_SLAB_OFFSET)->ShowWindow(SW_HIDE);
         GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

         CRect rOffset, rUnit;
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
      int nShowCmd = IsSegmentContinuousOverPier(pParent->m_pPier->GetSegmentConnectionType()) ? SW_HIDE : SW_SHOW;
      GetDlgItem(IDC_SLAB_OFFSET_GROUP)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_SLAB_OFFSET_TYPE)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_BACK_SLAB_OFFSET_LABEL)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_BACK_SLAB_OFFSET)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_BACK_SLAB_OFFSET_UNIT)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_LABEL)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET)->ShowWindow(nShowCmd);
      GetDlgItem(IDC_AHEAD_SLAB_OFFSET_UNIT)->ShowWindow(nShowCmd);
   }

   return bResult;
}
