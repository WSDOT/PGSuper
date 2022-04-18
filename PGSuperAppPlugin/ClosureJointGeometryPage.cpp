///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "resource.h"
#include "ClosureJointGeometryPage.h"

#include "TemporarySupportDlg.h"
#include "PierDetailsDlg.h"
#include "TimelineEventDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "PGSuperColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



void CSegmentConnectionComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   COLORREF oldTextColor = dc.GetTextColor();
   COLORREF oldBkColor = dc.GetBkColor();

   CString lpszText;
   GetLBText(lpDrawItemStruct->itemID, lpszText);

   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
      (lpDrawItemStruct->itemState & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));

      // Tell the parent page to update the girder image
      CClosureJointGeometryPage* pParent = (CClosureJointGeometryPage*)GetParent();
      if (pParent->m_bIsPier)
      {
         pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)GetItemData(lpDrawItemStruct->itemID);
         pParent->UpdateConnectionPicture(connectionType);
      }
      else
      {
         pgsTypes::TempSupportSegmentConnectionType connectionType = (pgsTypes::TempSupportSegmentConnectionType)GetItemData(lpDrawItemStruct->itemID);
         pParent->UpdateConnectionPicture(connectionType);
      }
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_WINDOW));
   }

   dc.DrawText(lpszText, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

   if (lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
   }

   dc.SetTextColor(oldTextColor);
   dc.SetBkColor(oldBkColor);

   dc.Detach();
}


void CClosureJointBearingOffsetMeasureComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   COLORREF oldTextColor = dc.GetTextColor();
   COLORREF oldBkColor = dc.GetBkColor();

   CString lpszText;
   GetLBText(lpDrawItemStruct->itemID, lpszText);

   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
      (lpDrawItemStruct->itemState & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));

      // Tell the parent page to update the girder image
      ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)GetItemData(lpDrawItemStruct->itemID);
      CClosureJointGeometryPage* pParent = (CClosureJointGeometryPage*)GetParent();
      pParent->UpdateConnectionPicture(brgOffsetType);
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_WINDOW));
   }

   dc.DrawText(lpszText, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

   if (lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
   }

   dc.SetTextColor(oldTextColor);
   dc.SetBkColor(oldBkColor);

   dc.Detach();
}


void CClosureJointEndDistanceMeasureComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   COLORREF oldTextColor = dc.GetTextColor();
   COLORREF oldBkColor = dc.GetBkColor();

   CString lpszText;
   GetLBText(lpDrawItemStruct->itemID, lpszText);

   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
      (lpDrawItemStruct->itemState & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));

      // Tell the parent page to update the girder image
      ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)GetItemData(lpDrawItemStruct->itemID);
      CClosureJointGeometryPage* pParent = (CClosureJointGeometryPage*)GetParent();
      pParent->UpdateConnectionPicture(endType);
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_WINDOW));
   }

   dc.DrawText(lpszText, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

   if (lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
   }

   dc.SetTextColor(oldTextColor);
   dc.SetBkColor(oldBkColor);

   dc.Detach();
}

// CClosureJointGeometryPage dialog

IMPLEMENT_DYNAMIC(CClosureJointGeometryPage, CPropertyPage)

CClosureJointGeometryPage::CClosureJointGeometryPage()
	: CPropertyPage(CClosureJointGeometryPage::IDD)
{
   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
   m_ClosureID = INVALID_ID;
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

   const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(0);
   if ( pClosureJoint )
   {
      m_ClosureKey = pClosureJoint->GetClosureKey();
      m_ClosureID = pClosureJoint->GetID();
   }
}

void CClosureJointGeometryPage::Init(const CPierData2* pPierData)
{
   m_bIsPier = true;
   m_strSupportLabel = _T("Pier");

   // we are forcing both sides of the pier to be the same so just use Ahead face
   pPierData->GetGirderEndDistance(pgsTypes::Ahead,&m_EndDistance,&m_EndDistanceMeasurementType);
   pPierData->GetBearingOffset(pgsTypes::Ahead,&m_BearingOffset,&m_BearingOffsetMeasurementType);

   m_DiaphragmWidth = pPierData->GetDiaphragmWidth(pgsTypes::Back) + pPierData->GetDiaphragmWidth(pgsTypes::Ahead);
   m_DiaphragmWidth = Max(m_DiaphragmWidth, -1.0);
   m_DiaphragmHeight = pPierData->GetDiaphragmHeight(pgsTypes::Back);

   const CClosureJointData* pClosureJoint = pPierData->GetClosureJoint(0);
   if ( pClosureJoint )
   {
      m_ClosureKey = pClosureJoint->GetClosureKey();
      m_ClosureID = pClosureJoint->GetID();
   }
}

void CClosureJointGeometryPage::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // for access to metafile resources

	CPropertyPage::DoDataExchange(pDX);

   DDX_Control(pDX, IDC_CONNECTION_TYPE, m_cbSegmentConnection);
   DDX_Control(pDX, IDC_BEARING_OFFSET_MEASURE, m_cbBearingOffsetMeasure);
   DDX_Control(pDX, IDC_END_DISTANCE_MEASURE, m_cbEndDistMeasure);
   
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strImageName;
   if ( m_bIsPier )
   {
      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      strImageName = GetImageName(pParent->m_pPier->GetSegmentConnectionType(),m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      strImageName = GetImageName(pParent->m_pTS->GetConnectionType(),m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
   }
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,strImageName, _T("Metafile") );

   DDX_UnitValueAndTag(pDX, IDC_END_DISTANCE, IDC_END_DISTANCE_T, m_EndDistance, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_DISTANCE, m_EndDistance, pDisplayUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX, IDC_BEARING_OFFSET, IDC_BEARING_OFFSET_T, m_BearingOffset, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BEARING_OFFSET, m_BearingOffset, pDisplayUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   if ( m_bIsPier )
   {
      DDX_KeywordUnitValueAndTag(pDX, IDC_WIDTH, IDC_WIDTH_UNIT, _T("Compute"), m_DiaphragmWidth, pDisplayUnits->GetComponentDimUnit() );
      DDX_KeywordUnitValueAndTag(pDX, IDC_HEIGHT, IDC_HEIGHT_UNIT, _T("Compute"), m_DiaphragmHeight, pDisplayUnits->GetComponentDimUnit() );
   }

   if ( pDX->m_bSaveAndValidate )
   {
      // copy the page local data to the bridge model on the property sheet
      bool bCheckTimeline = false;
      CTimelineEvent* pClosureEvent = nullptr;
      CTimelineManager* pTimelineMgr = GetTimelineManager();
      if ( m_bIsPier )
      {
         CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
         pParent->m_pPier->SetGirderEndDistance(pgsTypes::Ahead,m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pPier->SetBearingOffset(pgsTypes::Ahead,m_BearingOffset,m_BearingOffsetMeasurementType);
         pParent->m_pPier->SetGirderEndDistance(pgsTypes::Back,m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pPier->SetBearingOffset(pgsTypes::Back,m_BearingOffset,m_BearingOffsetMeasurementType);

         pParent->m_pPier->SetDiaphragmHeight(pgsTypes::Back,m_DiaphragmHeight);
         pParent->m_pPier->SetDiaphragmHeight(pgsTypes::Ahead,m_DiaphragmHeight);
         pParent->m_pPier->SetDiaphragmWidth(pgsTypes::Back, m_DiaphragmWidth < 0 ? -1 : m_DiaphragmWidth/2);
         pParent->m_pPier->SetDiaphragmWidth(pgsTypes::Ahead, m_DiaphragmWidth < 0 ? -1 : m_DiaphragmWidth/2);
         pParent->m_pPier->SetDiaphragmLoadType(pgsTypes::Back,ConnectionLibraryEntry::ApplyAtBearingCenterline);
         pParent->m_pPier->SetDiaphragmLoadType(pgsTypes::Ahead,ConnectionLibraryEntry::ApplyAtBearingCenterline);

         bCheckTimeline = ::IsSegmentContinuousOverPier(pParent->m_pPier->GetSegmentConnectionType()) ? false : true;

         if (bCheckTimeline)
         {
            auto cjID = pParent->m_pPier->GetClosureJoint(0)->GetID();
            auto eventID = pTimelineMgr->GetCastClosureJointEventID(cjID);
            pClosureEvent = pTimelineMgr->GetEventByID(eventID);
            ATLASSERT(pClosureEvent->GetCastClosureJointActivity().IsEnabled());
         }
      }
      else
      {
         CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

         pParent->m_pTS->SetGirderEndDistance(m_EndDistance,m_EndDistanceMeasurementType);
         pParent->m_pTS->SetBearingOffset(m_BearingOffset,m_BearingOffsetMeasurementType);

         bCheckTimeline = pParent->m_pTS->GetConnectionType() == pgsTypes::tsctClosureJoint ? true : false;

         if (bCheckTimeline)
         {
            auto cjID = pParent->m_pTS->GetClosureJoint(0)->GetID();
            auto eventID = pTimelineMgr->GetCastClosureJointEventID(cjID);
            pClosureEvent = pTimelineMgr->GetEventByID(eventID);
            ATLASSERT(pClosureEvent->GetCastClosureJointActivity().IsEnabled());
         }
      }

      if (bCheckTimeline)
      {
         int result = pTimelineMgr->Validate();
         if (sysFlags<Uint32>::IsSet(result, TLM_CLOSURE_JOINT_ERROR))
         {
            pDX->PrepareCtrl(IDC_EVENT);
            auto strError = pTimelineMgr->GetErrorMessage(result);
            CString strMsg;
            strMsg.Format(_T("%s\r\n\r\nPlease correct the closure joint installation event."), strError.c_str());
            AfxMessageBox(strMsg, MB_OK | MB_ICONERROR);
            pDX->Fail();
         }

         // we could have added something to the event that makes it's duration change... set the event and force the timeline to adjust
         result = pTimelineMgr->ValidateEvent(pClosureEvent);
         if (result != TLM_SUCCESS)
         {
            auto strError = pTimelineMgr->GetErrorMessage(result);
            CString strMsg;
            strMsg.Format(_T("%s\r\n\r\nShould the timeline be adjusted to accomodate the closure joint installation event?"), strError.c_str());
            if (AfxMessageBox(strMsg, MB_YESNO) == IDYES)
            {
               result = pTimelineMgr->SetEventByID(pClosureEvent->GetID(), *pClosureEvent, true); // use the reference version... the pointer version of this function deletes the old event, which is the event we are inserting
               ATLASSERT(result == TLM_SUCCESS);
            }
            else
            {
               pDX->Fail();
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CClosureJointGeometryPage, CPropertyPage)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE,   OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_CONNECTION_TYPE,        OnConnectionTypeChanged)
   ON_CBN_SELCHANGE(IDC_EVENT,                  OnInstallationStageChanged)
   ON_CBN_DROPDOWN(IDC_EVENT,                   OnInstallationStageChanging)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CClosureJointGeometryPage message handlers

BOOL CClosureJointGeometryPage::OnInitDialog()
{
   FillConnectionTypeComboBox();
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();

   FillEventList();

   CPropertyPage::OnInitDialog();

   if ( m_bIsPier )
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pgsTypes::PierSegmentConnectionType connectionType = pParent->m_pPier->GetSegmentConnectionType();

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx,IDC_CONNECTION_TYPE,connectionType);

      if ( connectionType == pgsTypes::psctContinousClosureJoint ||
           connectionType == pgsTypes::psctIntegralClosureJoint )
      {
         EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastClosureJointEventIndex(m_ClosureID);
         pcbEvent->SetCurSel((int)eventIdx);
      }

      GetDlgItem(IDC_EVENT_NOTE)->SetWindowText(_T("NOTE: Changes to the Installation Event apply to all closure joints at this pier."));
   }
   else
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

      pgsTypes::TempSupportSegmentConnectionType connectionType = pParent->m_pTS->GetConnectionType();
      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx,IDC_CONNECTION_TYPE,connectionType);

      if ( connectionType == pgsTypes::tsctClosureJoint )
      {
         ATLASSERT(m_ClosureID != INVALID_ID);
         EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastClosureJointEventIndex(m_ClosureID);
         pcbEvent->SetCurSel((int)eventIdx);
      }
      else
      {
         ATLASSERT(m_ClosureID == INVALID_ID);
         pcbEvent->SetCurSel(0);
      }

      GetDlgItem(IDC_DIAPHRAGM_GROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HEIGHT_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HEIGHT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HEIGHT_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_WIDTH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_WIDTH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_WIDTH_UNIT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DIAPHRAGM_NOTE)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_EVENT_NOTE)->SetWindowText(_T("NOTE: Changes to the Installation Event apply to all closure joints at this temporary support."));
   }

   OnConnectionTypeChanged();

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

   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
   EventIndexType castClosureEventIdx = (EventIndexType)pcbEvent->GetCurSel();

   int showWindow;
   if ( m_bIsPier )
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctIntegralClosureJoint ? SW_SHOW : SW_HIDE);

      if ( connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctIntegralClosureJoint )
      {
         CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
         pParent->m_pPier->SetSegmentConnectionType(connectionType,castClosureEventIdx);
         CClosureJointData* pClosure = pParent->m_pPier->GetClosureJoint(0);
         ATLASSERT(pClosure);
         m_ClosureID = pClosure->GetID();
      }
      else
      {
         // segment connection type changed in such a way that there is no longer a closure
         CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
         pParent->m_pPier->SetSegmentConnectionType(connectionType,INVALID_INDEX);
         m_ClosureID = INVALID_ID;
      }
   }
   else
   {
      pgsTypes::TempSupportSegmentConnectionType connectionType = (pgsTypes::TempSupportSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      showWindow = (connectionType == pgsTypes::tsctClosureJoint ? SW_SHOW : SW_HIDE);

      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      if ( connectionType == pgsTypes::tsctClosureJoint )
      {
         pParent->m_pTS->SetConnectionType(connectionType,castClosureEventIdx);
         CClosureJointData* pClosure = pParent->m_pTS->GetClosureJoint(0);
         m_ClosureID = pClosure->GetID();
      }
      else
      {
         ATLASSERT(connectionType == pgsTypes::tsctContinuousSegment);
         pParent->m_pTS->SetConnectionType(connectionType,INVALID_INDEX);
         m_ClosureID = INVALID_ID;
      }
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
   GetDlgItem(IDC_EVENT_NOTE)->ShowWindow(showWindow);
}

void CClosureJointGeometryPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   curSel = pcbConnectionType->GetCurSel();

   CString strImageName;
   if ( m_bIsPier )
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }
   else
   {
      pgsTypes::TempSupportSegmentConnectionType connectionType = (pgsTypes::TempSupportSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }

	m_ConnectionPicture.SetImage(strImageName, _T("Metafile") );
}

void CClosureJointGeometryPage::UpdateConnectionPicture(pgsTypes::TempSupportSegmentConnectionType connectionType)
{
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CString strImageName = GetImageName(connectionType, brgOffsetType, endType);
   m_ConnectionPicture.SetImage(strImageName, _T("Metafile"));
}

void CClosureJointGeometryPage::UpdateConnectionPicture(pgsTypes::PierSegmentConnectionType connectionType)
{
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CString strImageName = GetImageName(connectionType, brgOffsetType, endType);
   m_ConnectionPicture.SetImage(strImageName, _T("Metafile"));
}

void CClosureJointGeometryPage::UpdateConnectionPicture(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType)
{
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType endType = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   curSel = pcbConnectionType->GetCurSel();

   CString strImageName;
   if (m_bIsPier)
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }
   else
   {
      pgsTypes::TempSupportSegmentConnectionType connectionType = (pgsTypes::TempSupportSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }

   m_ConnectionPicture.SetImage(strImageName, _T("Metafile"));
}

void CClosureJointGeometryPage::UpdateConnectionPicture(ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CComboBox* pcbBrg = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   int curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   curSel = pcbConnectionType->GetCurSel();

   CString strImageName;
   if (m_bIsPier)
   {
      pgsTypes::PierSegmentConnectionType connectionType = (pgsTypes::PierSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }
   else
   {
      pgsTypes::TempSupportSegmentConnectionType connectionType = (pgsTypes::TempSupportSegmentConnectionType)pcbConnectionType->GetItemData(curSel);
      strImageName = GetImageName(connectionType, brgOffsetType, endType);
   }

   m_ConnectionPicture.SetImage(strImageName, _T("Metafile"));
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
      pCB->SetItemData(pCB->AddString(_T("Closure Joint")),(DWORD_PTR)pgsTypes::tsctClosureJoint);

      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      if ( pParent->m_pTS->GetSupportType() != pgsTypes::StrongBack )
      {
         pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),(DWORD_PTR)pgsTypes::tsctContinuousSegment);
      }
   }

   if ( cursel != CB_ERR )
   {
      cursel = pCB->SetCurSel(cursel);
   }

   if ( cursel == CB_ERR )
   {
      pCB->SetCurSel(0);
   }
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

HBRUSH CClosureJointGeometryPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hBrush = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CClosureJointGeometryPage::GetImageName(pgsTypes::TempSupportSegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CString strName;
   if ( connectionType == pgsTypes::tsctClosureJoint )
   {
      if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("TS_CLOSURE_BRGALONGGDR_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("TS_CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("TS_CLOSURE_BRGALONGGDR_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("TS_CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMPIER");
            break;
         }
      }
      else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("TS_CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("TS_CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("TS_CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("TS_CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
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
            strName = _T("PIER_CLOSURE_BRGALONGGDR_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("PIER_CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("PIER_CLOSURE_BRGALONGGDR_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("PIER_CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMPIER");
            break;
         }
      }
      else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("PIER_CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("PIER_CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("PIER_CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("PIER_CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
            break;
         }
      }
   }
   else
   {
      strName = _T("PIER_CONTINUOUS_SEGMENT");
   }

   return strName;
}

BOOL CClosureJointGeometryPage::OnSetActive()
{
   if ( m_bIsPier )
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

      CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
      pgsTypes::PierSegmentConnectionType connectionType = pParent->m_pPier->GetSegmentConnectionType();
      if ( connectionType == pgsTypes::psctContinousClosureJoint || connectionType == pgsTypes::psctIntegralClosureJoint )
      {
         EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastClosureJointEventIndex(m_ClosureID);
         pcbEvent->SetCurSel((int)eventIdx);
      }
   }
   else
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pgsTypes::TempSupportSegmentConnectionType connectionType = pParent->m_pTS->GetConnectionType();

      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
      if ( connectionType == pgsTypes::tsctClosureJoint )
      {
         const CClosureJointData* pClosureJoint = pParent->m_pTS->GetClosureJoint(0);
         m_ClosureKey = pClosureJoint->GetClosureKey();
         m_ClosureID = pClosureJoint->GetID();

         ATLASSERT(m_ClosureID != INVALID_ID);
         EventIndexType eventIdx = pParent->m_BridgeDesc.GetTimelineManager()->GetCastClosureJointEventIndex(m_ClosureID);
         pcbEvent->SetCurSel((int)eventIdx);
      }
      else
      {
         m_ClosureKey = CClosureKey();
         m_ClosureID = INVALID_ID;
         pcbEvent->SetCurSel(0);
      }

      // This is a temporary support... we need to update the temporary support type because it could
      // have changed since this dialog page was created
      FillConnectionTypeComboBox();
      OnConnectionTypeChanged();
   }

   FillEventList();

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
   {
      pcbEvent->SetCurSel(selIdx);
   }
   else
   {
      pcbEvent->SetCurSel(0);
   }
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
   EventIndexType eventIdx = (IndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      if ( eventIdx == INVALID_INDEX )
      {
         pCB->SetCurSel((int)m_PrevEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CTimelineManager* pTimelineMgr = GetTimelineManager();

   ATLASSERT(m_ClosureID != INVALID_ID);
   pTimelineMgr->SetCastClosureJointEventByIndex(m_ClosureID,eventIdx);
   pCB->SetCurSel((int)eventIdx);
}

EventIndexType CClosureJointGeometryPage::CreateEvent()
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();

   CTimelineEventDlg dlg(*pTimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType idx;
      pTimelineMgr->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&idx);
      return idx;
  }

   return INVALID_INDEX;
}

CTimelineManager* CClosureJointGeometryPage::GetTimelineManager()
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
   return pTimelineMgr;
}

void CClosureJointGeometryPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), m_bIsPier ? IDH_PIERDETAILS_CONNECTIONS : IDH_TSDETAILS_CONNECTION );
}
