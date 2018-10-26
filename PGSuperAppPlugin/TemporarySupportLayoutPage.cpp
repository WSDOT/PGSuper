///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <EAF\EAFDocument.h>

// CTemporarySupportLayoutPage dialog

IMPLEMENT_DYNAMIC(CTemporarySupportLayoutPage, CPropertyPage)

CTemporarySupportLayoutPage::CTemporarySupportLayoutPage()
	: CPropertyPage(CTemporarySupportLayoutPage::IDD)
{
   m_Station = 0.0;
   m_strOrientation = _T("NORMAL");
   m_Type = pgsTypes::ErectionTower;
   m_ElevAdjustment = 0;
}

CTemporarySupportLayoutPage::~CTemporarySupportLayoutPage()
{
}

void CTemporarySupportLayoutPage::Init(const CTemporarySupportData* pTS)
{
   m_Station        = pTS->GetStation();
   m_strOrientation = pTS->GetOrientation();
   m_Type           = pTS->GetSupportType();
   m_ElevAdjustment = pTS->GetElevationAdjustment();
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

   DDX_UnitValueAndTag(pDX,IDC_ADJUSTMENT,IDC_ADJUSTMENT_UNIT,m_ElevAdjustment,pDisplayUnits->GetComponentDimUnit());

   if ( pDX->m_bSaveAndValidate )
   {
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

      pParent->m_pTS->SetElevationAdjustment(m_ElevAdjustment);

      // don't use pParent->m_pTS->SetStation().... we have to move the TS within the bridge model
      if ( !IsEqual(pParent->m_pTS->GetStation(),m_Station) )
      {
         // this deletes the temporary support so pParent->m_pTS becomes invalid
         // the index of the "new" temporary support is returned
         SupportIndexType tsIdx = pBridgeDesc->MoveTemporarySupport(pParent->m_pTS->GetIndex(),m_Station);

         pParent->m_pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
         ATLASSERT(pParent->m_pTS != nullptr);
      }

      CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
      int result = pTimelineMgr->Validate();
      if (result != TLM_SUCCESS)
      {
         pDX->PrepareCtrl(IDC_REMOVAL_EVENT);
         CString strMsg = pTimelineMgr->GetErrorMessage(result);
         strMsg += _T("\r\n\r\nPlease correct the temporary support removal event.");
         AfxMessageBox(strMsg, MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CTemporarySupportLayoutPage, CPropertyPage)
   ON_CBN_SELCHANGE(IDC_TS_TYPE, &CTemporarySupportLayoutPage::OnSupportTypeChanged)
   ON_CBN_SELCHANGE(IDC_ERECTION_EVENT, &CTemporarySupportLayoutPage::OnErectionEventChanged)
   ON_CBN_DROPDOWN(IDC_ERECTION_EVENT, &CTemporarySupportLayoutPage::OnErectionEventChanging)
   ON_CBN_SELCHANGE(IDC_REMOVAL_EVENT, &CTemporarySupportLayoutPage::OnRemovalEventChanged)
   ON_CBN_DROPDOWN(IDC_REMOVAL_EVENT, &CTemporarySupportLayoutPage::OnRemovalEventChanging)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CTemporarySupportLayoutPage message handlers

BOOL CTemporarySupportLayoutPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_TS_TYPE);
   pCB->SetItemData(pCB->AddString(_T("Erection Tower")), (DWORD_PTR)pgsTypes::ErectionTower);
   pCB->SetItemData(pCB->AddString(_T("Strong Back")),    (DWORD_PTR)pgsTypes::StrongBack);

   FillEventList();

   CPropertyPage::OnInitDialog();

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   EventIndexType erectionEventIdx, removalEventIdx;
   pParent->m_BridgeDesc.GetTimelineManager()->GetTempSupportEvents(pParent->m_pTS->GetID(),&erectionEventIdx,&removalEventIdx);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_ERECTION_EVENT,erectionEventIdx);
   DDX_CBItemData(&dx,IDC_REMOVAL_EVENT,removalEventIdx);

   CString fmt;
   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   GetDlgItem(IDC_ORIENTATION_FORMAT)->SetWindowText( fmt );

   OnSupportTypeChanged();

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

   // Hide the support elevation adjustment input if this is a strongback
   UINT showWnd = SW_SHOW;
   if ( type == pgsTypes::StrongBack )
   {
      showWnd = SW_HIDE;
   }
   GetDlgItem(IDC_ADJUSTMENT_LABEL)->ShowWindow(showWnd);
   GetDlgItem(IDC_ADJUSTMENT)->ShowWindow(showWnd);
   GetDlgItem(IDC_ADJUSTMENT_UNIT)->ShowWindow(showWnd);

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   pParent->m_pTS->SetSupportType(type);

   if ( type == pgsTypes::StrongBack && pParent->m_pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment )
   {
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
         {
            strItems += _T("\n");
         }

         strItems += strItem;
      }

      strItems += _T("\nCreate Event...");

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

      if ( castClosureEventIndex == nEvents )
      {
         castClosureEventIndex = CreateEvent();
         if ( castClosureEventIndex == INVALID_INDEX )
         {
            return;
         }
      }

      pParent->m_pTS->SetConnectionType(pgsTypes::tsctClosureJoint,castClosureEventIndex);
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
   {
      pcbErect->SetCurSel(erectIdx);
   }
   else
   {
      pcbErect->SetCurSel(0);
   }

   if ( removeIdx != CB_ERR )
   {
      pcbRemove->SetCurSel(removeIdx);
   }
   else
   {
      pcbRemove->SetCurSel(0);
   }
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
   EventIndexType eventIdx = pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      if ( eventIdx == INVALID_INDEX )
      {
         pCB->SetCurSel(m_PrevErectionEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   EventIndexType erectionEventIdx, removalEventIdx;
   pParent->m_BridgeDesc.GetTimelineManager()->GetTempSupportEvents(pParent->m_pTS->GetID(),&erectionEventIdx,&removalEventIdx);
   pParent->m_BridgeDesc.GetTimelineManager()->SetTempSupportEvents(pParent->m_pTS->GetID(),eventIdx,removalEventIdx);
   pCB->SetCurSel((int)eventIdx);
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
   EventIndexType eventIdx = pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      if ( eventIdx == INVALID_INDEX )
      {
         pCB->SetCurSel(m_PrevRemovalEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   EventIndexType erectionEventIdx, removalEventIdx;
   pParent->m_BridgeDesc.GetTimelineManager()->GetTempSupportEvents(pParent->m_pTS->GetID(),&erectionEventIdx,&removalEventIdx);
   pParent->m_BridgeDesc.GetTimelineManager()->SetTempSupportEvents(pParent->m_pTS->GetID(),erectionEventIdx,eventIdx);
   pCB->SetCurSel((int)eventIdx);

}

EventIndexType CTemporarySupportLayoutPage::CreateEvent()
{
   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->GetBridgeDescription();
   CTimelineManager* pTimelineMgr = pBridge->GetTimelineManager();

   CTimelineEventDlg dlg(*pTimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType idx;
      pTimelineMgr->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&idx);
      return idx;
  }

   return INVALID_INDEX;
}

BOOL CTemporarySupportLayoutPage::OnSetActive()
{
   FillEventList();

   return CPropertyPage::OnSetActive();
}

void CTemporarySupportLayoutPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_TSDETAILS_GENERAL );
}
