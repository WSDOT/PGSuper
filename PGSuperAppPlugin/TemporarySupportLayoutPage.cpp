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

// TemporarySupportLayoutPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"
#include "TemporarySupportLayoutPage.h"
#include "TimelineEventDlg.h"
#include "SelectItemDlg.h"

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>

// CTemporarySupportLayoutPage dialog

IMPLEMENT_DYNAMIC(CTemporarySupportLayoutPage, CPropertyPage)

CTemporarySupportLayoutPage::CTemporarySupportLayoutPage()
	: CPropertyPage(CTemporarySupportLayoutPage::IDD)
{
   m_Station = 0.0;
   m_strOrientation = _T("NORMAL");
   m_Type = pgsTypes::ErectionTower;
   m_ErectionEventIndex = 0;
   m_RemovalEventIndex  = 0;
}

CTemporarySupportLayoutPage::~CTemporarySupportLayoutPage()
{
}

void CTemporarySupportLayoutPage::Init(const CTemporarySupportData* pTS)
{
   m_Station        = pTS->GetStation();
   m_strOrientation = pTS->GetOrientation();
   m_Type           = pTS->GetSupportType();

   SupportIDType tsID = pTS->GetID();

   const CSpanData2* pSpanData = pTS->GetSpan();
   const CBridgeDescription2* pBridge = pSpanData->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();
   pTimelineMgr->GetTempSupportEvents(tsID,&m_ErectionEventIndex,&m_RemovalEventIndex);
}

void CTemporarySupportLayoutPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

	CPropertyPage::DoDataExchange(pDX);

   DDX_Station(pDX,IDC_STATION,m_Station,pDisplayUnits->GetStationFormat());
   DDX_String(pDX,IDC_ORIENTATION,m_strOrientation);

   DDX_CBEnum(pDX,IDC_TS_TYPE,m_Type);

   DDX_CBItemData(pDX,IDC_ERECTION_EVENT,m_ErectionEventIndex);
   DDX_CBItemData(pDX,IDC_REMOVAL_EVENT,m_RemovalEventIndex);

   if ( pDX->m_bSaveAndValidate )
   {
      if ( pParent->m_BridgeDesc.GetTimelineManager()->GetEventCount() <= m_ErectionEventIndex )
      {
         pDX->PrepareCtrl(IDC_ERECTION_EVENT);
         AfxMessageBox(_T("Select an erection event"));
         pDX->Fail();
      }

      if ( pParent->m_BridgeDesc.GetTimelineManager()->GetEventCount() <= m_RemovalEventIndex )
      {
         pDX->PrepareCtrl(IDC_REMOVAL_EVENT);
         AfxMessageBox(_T("Select an removal event"));
         pDX->Fail();
      }
#pragma Reminder("Validate temporary support orientation")
      //pDX->PrepareEditCtrl(IDC_ORIENTATION);
      //GET_IFACE2(pBroker,IBridge,pBridge);
      //Float64 skewAngle;
      //bool bSuccess = pBridge->GetSkewAngle(m_Station,m_strOrientation.c_str(),&skewAngle);
      //if ( !bSuccess )
      //{
      //   AfxMessageBox(_T("Invalid pier orientation"));
      //   pDX->Fail();
      //}
      //else if ( bSuccess && IsLT(fabs(skewAngle),0.0) || IsGE(MAX_SKEW_ANGLE,fabs(skewAngle)) )
      //{
      //   AfxMessageBox(_T("Pier skew must be less than 88°\r\nPier skew is measured from the alignment normal"),MB_OK | MB_ICONSTOP);
      //   pDX->Fail();
      //}

      CBridgeDescription2* pBridgeDesc = pParent->GetBridgeDescription();
      if ( !pBridgeDesc->IsOnBridge(m_Station) )
      {
         pDX->PrepareEditCtrl(IDC_STATION);
         AfxMessageBox(_T("Temporary support must be between the end piers"),MB_OK | MB_ICONSTOP);
         pDX->Fail();
      }

      PierIndexType pierIdx = pBridgeDesc->IsPierLocation(m_Station);
      if ( pierIdx != INVALID_INDEX )
      {
         pDX->PrepareEditCtrl(IDC_STATION);
         CString strMsg;
         strMsg.Format(_T("Temporary support cannot be at the same location as %s %d"),pBridgeDesc->GetPier(pierIdx)->IsAbutment() ? _T("Abutment") : _T("Pier"),LABEL_PIER(pierIdx));
         AfxMessageBox(strMsg,MB_OK | MB_ICONSTOP);
         pDX->Fail();
      }

      SupportIndexType tsIdx = pBridgeDesc->IsTemporarySupportLocation(m_Station);
      if ( tsIdx != INVALID_INDEX && tsIdx != pParent->m_pTS->GetIndex() )
      {
         pDX->PrepareEditCtrl(IDC_STATION);
         CString strMsg;
         strMsg.Format(_T("Temporary support cannot be at the same location as Temporary Support %d"),LABEL_TEMPORARY_SUPPORT(tsIdx));
         AfxMessageBox(strMsg,MB_OK | MB_ICONSTOP);
         pDX->Fail();
      }

      // copy the local page data in the bridge model owned by the parent property sheet
      pParent->m_pTS->SetOrientation(m_strOrientation.c_str());
      pParent->m_pTS->SetSupportType(m_Type);
      pBridgeDesc->GetTimelineManager()->SetTempSupportEvents(pParent->m_pTS->GetID(),m_ErectionEventIndex,m_RemovalEventIndex);

      // don't use pParent->m_pTS->SetStation().... we have to move the TS within the bridge model
      if ( !IsEqual(pParent->m_pTS->GetStation(),m_Station) )
      {
         pBridgeDesc->MoveTemporarySupport(pParent->m_pTS->GetIndex(),m_Station);
      }
   }
}


BEGIN_MESSAGE_MAP(CTemporarySupportLayoutPage, CPropertyPage)
   ON_CBN_SELCHANGE(IDC_TS_TYPE, &CTemporarySupportLayoutPage::OnSupportTypeChanged)
   ON_CBN_SELCHANGE(IDC_ERECTION_EVENT, &CTemporarySupportLayoutPage::OnErectionEventChanged)
   ON_CBN_DROPDOWN(IDC_ERECTION_EVENT, &CTemporarySupportLayoutPage::OnErectionEventChanging)
   ON_CBN_SELCHANGE(IDC_REMOVAL_EVENT, &CTemporarySupportLayoutPage::OnRemovalEventChanged)
   ON_CBN_DROPDOWN(IDC_REMOVAL_EVENT, &CTemporarySupportLayoutPage::OnRemovalEventChanging)
END_MESSAGE_MAP()


// CTemporarySupportLayoutPage message handlers

BOOL CTemporarySupportLayoutPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TS_TYPE);
   pCB->SetItemData(pCB->AddString(_T("Erection Tower")), (DWORD_PTR)pgsTypes::ErectionTower);
   pCB->SetItemData(pCB->AddString(_T("Strong Back")),    (DWORD_PTR)pgsTypes::StrongBack);

   FillEventList();

   CPropertyPage::OnInitDialog();

   CString fmt;
   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   GetDlgItem(IDC_ORIENTATION_FORMAT)->SetWindowText( fmt );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTemporarySupportLayoutPage::OnSupportTypeChanged()
{
   // TODO: Add your control notification handler code here
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TS_TYPE);
   int cursel = pCB->GetCurSel();
   if ( cursel == CB_ERR )
      return;

   pgsTypes::TemporarySupportType type = (pgsTypes::TemporarySupportType)pCB->GetItemData(cursel);

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   if ( type == pgsTypes::StrongBack && pParent->m_pTS->GetConnectionType() == pgsTypes::sctContinuousSegment )
   {
#pragma Reminder("UPDATE: add option to create an event on the fly")
      // Adding this option will make this UI element consistent with all the other UI elements that
      // ask the user to select an event
      CSelectItemDlg dlg;
      dlg.m_strLabel = _T("Precast segments must be spliced with a closure joint at strong back supports. Select the event when the closure joints are cast.");
      dlg.m_strTitle = _T("Select Event");
      dlg.m_ItemIdx = 0;

      CString strItems;
      CTimelineManager* pTimelineMgr = pParent->m_BridgeDesc.GetTimelineManager();
      EventIndexType nEvents = pTimelineMgr->GetEventCount();
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {
         const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

         CString strItem;
         strItem.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

         if ( eventIdx != 0 )
            strItems += _T("\n");

         strItems += strItem;
      }

      dlg.m_strItems = strItems;
      EventIndexType castClosureEventIndex;
      if ( dlg.DoModal() == IDOK )
      {
         castClosureEventIndex = dlg.m_ItemIdx;
      }
      else
      {
         return;
      }

      pParent->m_pTS->SetConnectionType(pgsTypes::sctClosureJoint,castClosureEventIndex);
   }
}

void CTemporarySupportLayoutPage::FillEventList()
{
   CComboBox* pcbErect = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   CComboBox* pcbRemove = (CComboBox*)GetDlgItem(IDC_REMOVAL_EVENT);

   int erectIdx = pcbErect->GetCurSel();
   int removeIdx = pcbRemove->GetCurSel();

   pcbErect->ResetContent();
   pcbRemove->ResetContent();

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   const CBridgeDescription2* pBridge = pParent->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbErect->SetItemData(pcbErect->AddString(label),eventIdx);
      pcbRemove->SetItemData(pcbRemove->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbErect->SetItemData(pcbErect->AddString(strNewEvent),CREATE_TIMELINE_EVENT);
   pcbRemove->SetItemData(pcbRemove->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( erectIdx != CB_ERR )
      pcbErect->SetCurSel(erectIdx);

   if ( removeIdx != CB_ERR )
      pcbRemove->SetCurSel(removeIdx);
}

void CTemporarySupportLayoutPage::OnErectionEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   m_PrevErectionEventIdx = pCB->GetCurSel();
}

void CTemporarySupportLayoutPage::OnErectionEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ERECTION_EVENT);
   int curSel = pCB->GetCurSel();
   if ( pCB->GetItemData(curSel) == CREATE_TIMELINE_EVENT )
   {
      EventIndexType eventIdx = CreateEvent();
      
      if ( eventIdx != INVALID_INDEX )
      {
         m_ErectionEventIndex = eventIdx;
         FillEventList();
         pCB->SetCurSel((int)eventIdx);
      }
      else
      {
         pCB->SetCurSel(m_PrevErectionEventIdx);
      }
   }
}

void CTemporarySupportLayoutPage::OnRemovalEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REMOVAL_EVENT);
   m_PrevRemovalEventIdx = pCB->GetCurSel();
}

void CTemporarySupportLayoutPage::OnRemovalEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REMOVAL_EVENT);
   int curSel = pCB->GetCurSel();
   if ( pCB->GetItemData(curSel) == CREATE_TIMELINE_EVENT )
   {
      EventIndexType eventIdx = CreateEvent();
      
      if ( eventIdx != INVALID_INDEX )
      {
         m_RemovalEventIndex = eventIdx;
         FillEventList();
         pCB->SetCurSel((int)eventIdx);
      }
      else
      {
         pCB->SetCurSel((int)m_RemovalEventIndex);
      }
   }
}

EventIndexType CTemporarySupportLayoutPage::CreateEvent()
{
   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->GetBridgeDescription();
   CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();

   CTimelineEventDlg dlg(pTimelineMgr,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType idx;
      pTimelineMgr->AddTimelineEvent(dlg.m_TimelineEvent,true,&idx);
      return idx;
  }

   return INVALID_INDEX;
}