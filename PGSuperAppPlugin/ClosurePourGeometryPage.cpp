///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// ClosurePourGeometryPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\Resource.h"
#include "ClosurePourGeometryPage.h"

#include "TemporarySupportDlg.h"
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosurePourData.h>

#include "PGSuperColors.h"

// CClosurePourGeometryPage dialog

IMPLEMENT_DYNAMIC(CClosurePourGeometryPage, CPropertyPage)

CClosurePourGeometryPage::CClosurePourGeometryPage()
	: CPropertyPage(CClosurePourGeometryPage::IDD)
{
   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
}

CClosurePourGeometryPage::~CClosurePourGeometryPage()
{
}

void CClosurePourGeometryPage::Init(const CTemporarySupportData& tsData)
{
   m_bIsPier = false;
   m_strSupportLabel = _T("Temporary Support");

   tsData.GetGirderEndDistance(&m_EndDistance,&m_EndDistanceMeasurementType);
   tsData.GetBearingOffset(&m_BearingOffset,&m_BearingOffsetMeasurementType);
   m_SupportWidth = tsData.GetSupportWidth();
   m_TSConnectionType = tsData.GetConnectionType();

   const CClosurePourData* pClosurePour = tsData.GetClosurePour(0);
   if ( pClosurePour )
   {
      const CPrecastSegmentData* pLeftSegment = pClosurePour->GetLeftSegment();
      SegmentIDType segID = pLeftSegment->GetID();

      const CBridgeDescription2* pBridge = pLeftSegment->GetGirder()->GetPier(pgsTypes::metStart)->GetBridgeDescription();
      const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();
      m_ClosurePourEventIndex = pTimelineMgr->GetCastClosurePourEventIndex(segID);
   
      m_ClosureKey = pClosurePour->GetClosureKey();
   }
   else
   {
      m_ClosurePourEventIndex = INVALID_INDEX;
   }

   m_tsSupportType = tsData.GetSupportType();
}

void CClosurePourGeometryPage::Init(const CPierData2* pPierData)
{
   m_bIsPier = true;
   m_strSupportLabel = _T("Pier");

   // we are forcing both sides of the pier to be the same so just use Ahead face
   pPierData->GetGirderEndDistance(pgsTypes::Ahead,&m_EndDistance,&m_EndDistanceMeasurementType);
   pPierData->GetBearingOffset(pgsTypes::Ahead,&m_BearingOffset,&m_BearingOffsetMeasurementType);
   m_SupportWidth = pPierData->GetSupportWidth(pgsTypes::Ahead);

   m_PierConnectionType = pPierData->GetSegmentConnectionType();

   const CClosurePourData* pClosurePour = pPierData->GetClosurePour(0);
   if ( pClosurePour )
   {
      const CPrecastSegmentData* pLeftSegment = pClosurePour->GetLeftSegment();
      SegmentIDType segID = pLeftSegment->GetID();

      const CBridgeDescription2* pBridge = pLeftSegment->GetGirder()->GetPier(pgsTypes::metStart)->GetBridgeDescription();
      const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();
      m_ClosurePourEventIndex = pTimelineMgr->GetCastClosurePourEventIndex(segID);

      m_ClosureKey = pClosurePour->GetClosureKey();
   }
   else
   {
      m_ClosurePourEventIndex = INVALID_INDEX;
   }
}

void CClosurePourGeometryPage::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // for access to metafile resources

	CPropertyPage::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strImageName;
   if ( m_bIsPier )
   {
      strImageName = GetImageName(m_PierConnectionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
   }
   else
   {
      strImageName = GetImageName(m_TSConnectionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
   }
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,strImageName, _T("Metafile") );

   DDX_UnitValueAndTag(pDX, IDC_END_DISTANCE, IDC_END_DISTANCE_T, m_EndDistance, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_DISTANCE, m_EndDistance, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BEARING_OFFSET, IDC_BEARING_OFFSET_T, m_BearingOffset, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BEARING_OFFSET, m_BearingOffset, pDisplayUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX, IDC_SUPPORT_WIDTH, IDC_SUPPORT_WIDTH_T, m_SupportWidth, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_SUPPORT_WIDTH, m_SupportWidth, pDisplayUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   if ( m_bIsPier )
   {
      DDX_CBItemData(pDX,IDC_CONNECTION_TYPE,m_PierConnectionType);
      if ( m_PierConnectionType == pgsTypes::psctContinousClosurePour || m_PierConnectionType == pgsTypes::psctIntegralClosurePour )
      {
         DDX_CBItemData(pDX,IDC_EVENT,m_ClosurePourEventIndex);

         if ( pDX->m_bSaveAndValidate && m_ClosurePourEventIndex == INVALID_INDEX )
         {
            pDX->PrepareCtrl(IDC_EVENT);
            AfxMessageBox(_T("The closure pour installation event must be defined."));
            pDX->Fail();
         }
      }
   }
   else
   {
      DDX_CBItemData(pDX,IDC_CONNECTION_TYPE,m_TSConnectionType);
      if ( m_TSConnectionType == pgsTypes::sctClosurePour )
      {
         DDX_CBItemData(pDX,IDC_EVENT,m_ClosurePourEventIndex);

         if ( pDX->m_bSaveAndValidate && m_ClosurePourEventIndex == INVALID_INDEX )
         {
            pDX->PrepareCtrl(IDC_EVENT);
            AfxMessageBox(_T("The closure pour installation event must be defined."));
            pDX->Fail();
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CClosurePourGeometryPage, CPropertyPage)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_CONNECTION_TYPE,OnConnectionTypeChanged)
   ON_CBN_SELCHANGE(IDC_EVENT, OnInstallationStageChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnInstallationStageChanging)
END_MESSAGE_MAP()


// CClosurePourGeometryPage message handlers

BOOL CClosurePourGeometryPage::OnInitDialog()
{
   FillConnectionTypeComboBox();
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();

   FillEventList();

   CPropertyPage::OnInitDialog();

   OnConnectionTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosurePourGeometryPage::OnEndDistanceMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CClosurePourGeometryPage::OnBearingOffsetMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CClosurePourGeometryPage::OnConnectionTypeChanged()
{
   UpdateConnectionPicture();

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int curSel = pcbConnectionType->GetCurSel();

   int showWindow;
   if ( m_bIsPier )
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::psctContinousClosurePour || connectionType == pgsTypes::psctIntegralClosurePour ? SW_SHOW : SW_HIDE);
   }
   else
   {
      pgsTypes::SegmentConnectionType connectionType = (pgsTypes::SegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::sctClosurePour ? SW_SHOW : SW_HIDE);
   }

   GetDlgItem(IDC_BEARING_OFFSET_LABEL)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET_T)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET_MEASURE)->ShowWindow(showWindow);

   GetDlgItem(IDC_END_DISTANCE_LABEL)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE_T)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE_MEASURE)->ShowWindow(showWindow);

   GetDlgItem(IDC_EVENT)->ShowWindow(showWindow);
   GetDlgItem(IDC_EVENT_LABEL)->ShowWindow(showWindow);
}

void CClosurePourGeometryPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType ems = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType bms = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   curSel = pcbConnectionType->GetCurSel();

   CString strImageName;
   if ( m_bIsPier )
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType,bms,ems);
   }
   else
   {
      pgsTypes::SegmentConnectionType connectionType = (pgsTypes::SegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType,bms,ems);
   }

	m_ConnectionPicture.SetImage(strImageName, _T("Metafile") );
}

void CClosurePourGeometryPage::FillConnectionTypeComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int cursel = pCB->GetCurSel();

   pCB->ResetContent();

   if ( m_bIsPier )
   {
      pCB->SetItemData(pCB->AddString(_T("Continuous Closure Pour")),(DWORD_PTR)pgsTypes::psctContinousClosurePour);
      pCB->SetItemData(pCB->AddString(_T("Integral Closure Pour")),  (DWORD_PTR)pgsTypes::psctIntegralClosurePour);
      pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),     (DWORD_PTR)pgsTypes::psctContinuousSegment);
      pCB->SetItemData(pCB->AddString(_T("Integral Segment")),       (DWORD_PTR)pgsTypes::psctIntegralSegment);
   }
   else
   {
      pCB->SetItemData(pCB->AddString(_T("Closure Pour")),(DWORD_PTR)pgsTypes::sctClosurePour);

      if ( m_tsSupportType != pgsTypes::StrongBack )
         pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),(DWORD_PTR)pgsTypes::sctContinuousSegment);
   }

   if ( cursel != CB_ERR )
      cursel = pCB->SetCurSel(cursel);

   if ( cursel == CB_ERR )
      pCB->SetCurSel(0);
}

void CClosurePourGeometryPage::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   CString strLabel;
   strLabel.Format(_T("Measured Normal to %s Line"),m_strSupportLabel);
   int idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   idx = pCB->AddString(_T("Measured Along Centerline Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::AlongGirder));
}

void CClosurePourGeometryPage::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured from CL Bearing, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(_T("Measured from and normal to CL Bearing"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   CString strLabel;
   strLabel.Format(_T("Measured from %s Line, Along Girder"),m_strSupportLabel);
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   strLabel.Format(_T("Measured from and normal to %s Line"),m_strSupportLabel);
   idx = pCB->AddString(strLabel);
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

HBRUSH CClosurePourGeometryPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hBrush = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CClosurePourGeometryPage::GetImageName(pgsTypes::SegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CString strName;
   if ( connectionType == pgsTypes::sctClosurePour )
   {
      if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMPIER");
            break;
         }
      }
      else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
            break;
         }
      }
   }
   else
   {
      strName = _T("TS_CONTINUOUS_SEGMENT");
   }

   return strName;
}

CString CClosurePourGeometryPage::GetImageName(pgsTypes::PierSegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
#pragma Reminder("UPDATE: need correct image") // or re-work the images so the same images work for both forms of this dialog page
   CString strName;
   if ( connectionType == pgsTypes::psctContinousClosurePour || connectionType == pgsTypes::psctIntegralClosurePour )
   {
      if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMPIER");
            break;
         }
      }
      else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
            break;
         }
      }
   }
   else
   {
      strName = _T("TS_CONTINUOUS_SEGMENT");
   }

   return strName;
}

BOOL CClosurePourGeometryPage::OnSetActive()
{
   if ( !m_bIsPier )
   {
      // This is a temporary support... we need to update the temporary support type because it could
      // have changed since this dialog page was created
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      m_tsSupportType = pParent->GetTemporarySupportType();
      FillConnectionTypeComboBox();
      OnConnectionTypeChanged();
   }

   return CPropertyPage::OnSetActive();
}

void CClosurePourGeometryPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int eventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( eventIdx != CB_ERR )
      pcbEvent->SetCurSel(eventIdx);
}


void CClosurePourGeometryPage::OnInstallationStageChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CClosurePourGeometryPage::OnInstallationStageChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType idx = (IndexType)pCB->GetItemData(curSel);
   if ( idx == CREATE_TIMELINE_EVENT )
   {
      EventIndexType eventIdx = CreateEvent();
      if (eventIdx != INVALID_INDEX)
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

         ATLASSERT(m_ClosureKey.groupIndex   != INVALID_INDEX);
         ATLASSERT(m_ClosureKey.segmentIndex != INVALID_INDEX);
         pIBridgeDesc->SetCastClosurePourEventByIndex(m_ClosureKey.groupIndex,m_ClosureKey.segmentIndex,eventIdx);

         FillEventList();

         pCB->SetCurSel((int)idx);
         m_ClosurePourEventIndex = idx;
      }
      else
      {
         pCB->SetCurSel(m_PrevEventIdx);
      }
   }
}

EventIndexType CClosurePourGeometryPage::CreateEvent()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CTimelineEventDlg dlg(pTimelineMgr,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      return pIBridgeDesc->AddTimelineEvent(dlg.m_TimelineEvent);
  }

   return INVALID_INDEX;
}