///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// ClosureJointGeometryPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\Resource.h"
#include "ClosureJointGeometryPage.h"

#include "TemporarySupportDlg.h"
#include "PierDetailsDlg.h"
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "PGSuperColors.h"

// CClosureJointGeometryPage dialog

IMPLEMENT_DYNAMIC(CClosureJointGeometryPage, CPropertyPage)

CClosureJointGeometryPage::CClosureJointGeometryPage()
	: CPropertyPage(CClosureJointGeometryPage::IDD)
{
   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
}

CClosureJointGeometryPage::~CClosureJointGeometryPage()
{
}

void CClosureJointGeometryPage::Init(const CTemporarySupportData* pTS)
{
   m_bIsPier = false;
   m_strSupportLabel = _T("Temporary Support");

   pTS->GetGirderEndDistance(&m_EndDistance,&m_EndDistanceMeasurementType);
   pTS->GetBearingOffset(&m_BearingOffset,&m_BearingOffsetMeasurementType);
   m_SupportWidth = pTS->GetSupportWidth();
   m_TSConnectionType = pTS->GetConnectionType();

   const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(0);
   if ( pClosureJoint )
   {
      IDType closureID = pClosureJoint->GetID();

      const CBridgeDescription2* pBridge = pTS->GetSpan()->GetBridgeDescription();
      const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();
      m_ClosureJointEventIndex = pTimelineMgr->GetCastClosureJointEventIndex(closureID);
   
      m_ClosureKey = pClosureJoint->GetClosureKey();
   }
   else
   {
      m_ClosureJointEventIndex = INVALID_INDEX;
   }

   m_tsSupportType = pTS->GetSupportType();
}

void CClosureJointGeometryPage::Init(const CPierData2* pPierData)
{
   m_bIsPier = true;
   m_strSupportLabel = _T("Pier");

   // we are forcing both sides of the pier to be the same so just use Ahead face
   pPierData->GetGirderEndDistance(pgsTypes::Ahead,&m_EndDistance,&m_EndDistanceMeasurementType);
   pPierData->GetBearingOffset(pgsTypes::Ahead,&m_BearingOffset,&m_BearingOffsetMeasurementType);
   m_SupportWidth = pPierData->GetSupportWidth(pgsTypes::Ahead) + pPierData->GetSupportWidth(pgsTypes::Back);

   m_SegmentConnectionType = pPierData->GetSegmentConnectionType();
   m_PierConnectionType = pPierData->GetPierConnectionType();

   const CClosureJointData* pClosureJoint = pPierData->GetClosureJoint(0);
   if ( pClosureJoint )
   {
      IDType closureID = pClosureJoint->GetID();

      const CBridgeDescription2* pBridge = pPierData->GetBridgeDescription();
      const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();
      m_ClosureJointEventIndex = pTimelineMgr->GetCastClosureJointEventIndex(closureID);

      m_ClosureKey = pClosureJoint->GetClosureKey();
   }
   else
   {
      m_ClosureJointEventIndex = INVALID_INDEX;
   }
}

void CClosureJointGeometryPage::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // for access to metafile resources

	CPropertyPage::DoDataExchange(pDX);

   DDX_Control(pDX,IDC_BOUNDARY_CONDITIONS,m_cbBoundaryCondition);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strImageName;
   if ( m_bIsPier )
   {
      strImageName = GetImageName(m_SegmentConnectionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
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
      // Boundary Conditions
      DDX_CBItemData(pDX,IDC_BOUNDARY_CONDITIONS,m_PierConnectionType);

      DDX_CBItemData(pDX,IDC_CONNECTION_TYPE,m_SegmentConnectionType);
      if ( m_SegmentConnectionType == pgsTypes::psctContinousClosureJoint || m_SegmentConnectionType == pgsTypes::psctIntegralClosureJoint )
      {
         DDX_CBItemData(pDX,IDC_EVENT,m_ClosureJointEventIndex);

         if ( pDX->m_bSaveAndValidate && m_ClosureJointEventIndex == INVALID_INDEX )
         {
            pDX->PrepareCtrl(IDC_EVENT);
            AfxMessageBox(_T("The closure joint installation event must be defined."));
            pDX->Fail();
         }
      }
   }
   else
   {
      DDX_CBItemData(pDX,IDC_CONNECTION_TYPE,m_TSConnectionType);
      if ( m_TSConnectionType == pgsTypes::sctClosureJoint )
      {
         DDX_CBItemData(pDX,IDC_EVENT,m_ClosureJointEventIndex);

         if ( pDX->m_bSaveAndValidate && m_ClosureJointEventIndex == INVALID_INDEX )
         {
            pDX->PrepareCtrl(IDC_EVENT);
            AfxMessageBox(_T("The closure joint installation event must be defined."));
            pDX->Fail();
         }
      }
   }

   if ( pDX->m_bSaveAndValidate )
   {
      // copy the page local data to the bridge model on the property sheet
      if ( m_bIsPier )
      {
         CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
         pParent->m_pPier->SetGirderEndDistance(pgsTypes::Ahead,m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pPier->SetBearingOffset(pgsTypes::Ahead,m_BearingOffset,m_BearingOffsetMeasurementType);
         pParent->m_pPier->SetGirderEndDistance(pgsTypes::Back,m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pPier->SetBearingOffset(pgsTypes::Back,m_BearingOffset,m_BearingOffsetMeasurementType);
         pParent->m_pPier->SetSupportWidth(pgsTypes::Ahead,m_SupportWidth/2);
         pParent->m_pPier->SetSupportWidth(pgsTypes::Back,m_SupportWidth/2);
         pParent->m_pPier->SetSegmentConnectionType(m_SegmentConnectionType,m_ClosureJointEventIndex);
         pParent->m_pPier->SetPierConnectionType(m_PierConnectionType);
      }
      else
      {
         CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

         pParent->m_pTS->SetGirderEndDistance(m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pTS->SetBearingOffset(m_BearingOffset,m_BearingOffsetMeasurementType);
         pParent->m_pTS->SetSupportWidth(m_SupportWidth);
         pParent->m_pTS->SetConnectionType(m_TSConnectionType,m_ClosureJointEventIndex);
         pParent->m_pTS->SetSupportType(m_tsSupportType);
      }
   }
}


BEGIN_MESSAGE_MAP(CClosureJointGeometryPage, CPropertyPage)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_CONNECTION_TYPE,OnConnectionTypeChanged)
   ON_CBN_SELCHANGE(IDC_EVENT, OnInstallationStageChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnInstallationStageChanging)
END_MESSAGE_MAP()


// CClosureJointGeometryPage message handlers

BOOL CClosureJointGeometryPage::OnInitDialog()
{
   FillConnectionTypeComboBox();
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();
   FillBoundaryConditionComboBox();

   FillEventList();

   CPropertyPage::OnInitDialog();

   OnConnectionTypeChanged();

   if ( m_bIsPier )
   {
      m_cbBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);
   }
   else
   {
      GetDlgItem(IDC_BOUNDARY_CONDITION_LABEL)->ShowWindow(SW_HIDE);
      m_cbBoundaryCondition.ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosureJointGeometryPage::OnEndDistanceMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CClosureJointGeometryPage::OnBearingOffsetMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CClosureJointGeometryPage::OnConnectionTypeChanged()
{
   UpdateConnectionPicture();

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int curSel = pcbConnectionType->GetCurSel();

   int showWindow;
   if ( m_bIsPier )
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctIntegralClosureJoint ? SW_SHOW : SW_HIDE);
   }
   else
   {
      pgsTypes::SegmentConnectionType connectionType = (pgsTypes::SegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::sctClosureJoint ? SW_SHOW : SW_HIDE);
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

void CClosureJointGeometryPage::UpdateConnectionPicture()
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

void CClosureJointGeometryPage::FillConnectionTypeComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int cursel = pCB->GetCurSel();

   pCB->ResetContent();

   if ( m_bIsPier )
   {
      pCB->SetItemData(pCB->AddString(_T("Continuous Closure Joint")),(DWORD_PTR)pgsTypes::psctContinousClosureJoint);
      pCB->SetItemData(pCB->AddString(_T("Integral Closure Joint")),  (DWORD_PTR)pgsTypes::psctIntegralClosureJoint);
      pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),     (DWORD_PTR)pgsTypes::psctContinuousSegment);
      pCB->SetItemData(pCB->AddString(_T("Integral Segment")),       (DWORD_PTR)pgsTypes::psctIntegralSegment);
   }
   else
   {
      pCB->SetItemData(pCB->AddString(_T("Closure Joint")),(DWORD_PTR)pgsTypes::sctClosureJoint);

      if ( m_tsSupportType != pgsTypes::StrongBack )
         pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),(DWORD_PTR)pgsTypes::sctContinuousSegment);
   }

   if ( cursel != CB_ERR )
      cursel = pCB->SetCurSel(cursel);

   if ( cursel == CB_ERR )
      pCB->SetCurSel(0);
}

void CClosureJointGeometryPage::FillBearingOffsetComboBox()
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

void CClosureJointGeometryPage::FillEndDistanceComboBox()
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

void CClosureJointGeometryPage::FillBoundaryConditionComboBox()
{
   if ( m_bIsPier )
   {
      CBoundaryConditionComboBox* pcbBoundaryConditions = (CBoundaryConditionComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);

      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      PierIndexType pierIdx = pParent->m_pPier->GetIndex();

      std::vector<pgsTypes::PierConnectionType> connections( pParent->m_pPier->GetBridgeDescription()->GetPierConnectionTypes(pierIdx) );

      pcbBoundaryConditions->Initialize(connections);
   }
}

HBRUSH CClosureJointGeometryPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hBrush = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CClosureJointGeometryPage::GetImageName(pgsTypes::SegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CString strName;
   if ( connectionType == pgsTypes::sctClosureJoint )
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

CString CClosureJointGeometryPage::GetImageName(pgsTypes::PierSegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
#pragma Reminder("UPDATE: need correct image") // or re-work the images so the same images work for both forms of this dialog page
   CString strName;
   if ( connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctIntegralClosureJoint )
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

BOOL CClosureJointGeometryPage::OnSetActive()
{
   if ( !m_bIsPier )
   {
      // This is a temporary support... we need to update the temporary support type because it could
      // have changed since this dialog page was created
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      m_tsSupportType = pParent->m_pTS->GetSupportType();
      FillConnectionTypeComboBox();
      OnConnectionTypeChanged();
   }

   return CPropertyPage::OnSetActive();
}

void CClosureJointGeometryPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int selIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

   CTimelineManager* pTimelineMgr;
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
   }

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

   if ( selIdx != CB_ERR )
      pcbEvent->SetCurSel(selIdx);
}


void CClosureJointGeometryPage::OnInstallationStageChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CClosureJointGeometryPage::OnInstallationStageChanged()
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
         pIBridgeDesc->SetCastClosureJointEventByIndex(m_ClosureKey.groupIndex,m_ClosureKey.segmentIndex,eventIdx);

         FillEventList();

         pCB->SetCurSel((int)idx);
         m_ClosureJointEventIndex = idx;
      }
      else
      {
         pCB->SetCurSel((int)m_PrevEventIdx);
      }
   }
}

EventIndexType CClosureJointGeometryPage::CreateEvent()
{
   CTimelineManager* pTimelineMgr;
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
   }

   CTimelineEventDlg dlg(pTimelineMgr,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType idx;
      pTimelineMgr->AddTimelineEvent(dlg.m_TimelineEvent,true,&idx);
      return idx;
  }

   return INVALID_INDEX;
}