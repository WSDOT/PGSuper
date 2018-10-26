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

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>

// CTemporarySupportLayoutPage dialog

IMPLEMENT_DYNAMIC(CTemporarySupportLayoutPage, CPropertyPage)

CTemporarySupportLayoutPage::CTemporarySupportLayoutPage()
	: CPropertyPage(CTemporarySupportLayoutPage::IDD)
{
}

CTemporarySupportLayoutPage::~CTemporarySupportLayoutPage()
{
}

void CTemporarySupportLayoutPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

	CPropertyPage::DoDataExchange(pDX);

   if ( !pDX->m_bSaveAndValidate )
   {
      m_Station        = pParent->m_TemporarySupport.GetStation();
      m_strOrientation = pParent->m_TemporarySupport.GetOrientation();
      m_Type           = pParent->m_TemporarySupport.GetSupportType();
   }

   DDX_Station(pDX,IDC_STATION,m_Station,pDisplayUnits->GetStationFormat());
   DDX_String(pDX,IDC_ORIENTATION,m_strOrientation);

   DDX_CBEnum(pDX,IDC_TS_TYPE,m_Type);

   DDX_CBItemData(pDX,IDC_ERECTION_EVENT,pParent->m_ErectionEventIndex);
   DDX_CBItemData(pDX,IDC_REMOVAL_EVENT,pParent->m_RemovalEventIndex);

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

      const CBridgeDescription2* pBridgeDesc = pParent->m_pBridgeDesc;
      if ( !pBridgeDesc->IsOnBridge(m_Station) )
      {
         pDX->PrepareEditCtrl(IDC_STATION);
         AfxMessageBox(_T("Temporary support must be between the end piers"),MB_OK | MB_ICONSTOP);
         pDX->Fail();
      }

      pParent->m_TemporarySupport.SetStation(m_Station);
      pParent->m_TemporarySupport.SetOrientation(m_strOrientation.c_str());
      pParent->m_TemporarySupport.SetSupportType(m_Type);
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
   if ( type == pgsTypes::StrongBack && pParent->m_TemporarySupport.GetConnectionType() == pgsTypes::sctContinuousSegment )
   {
      AfxMessageBox(_T("Precast segments must be spliced with a closure pour at strong back supports. Connection type changed to Closure Pour"),MB_OK);
      pParent->m_TemporarySupport.SetConnectionType(pgsTypes::sctClosurePour);
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
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      EventIndexType eventIdx = CreateEvent();
      
      if ( eventIdx != INVALID_INDEX )
      {
         pParent->m_ErectionEventIndex = eventIdx;
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
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      EventIndexType eventIdx = CreateEvent();
      
      if ( eventIdx != INVALID_INDEX )
      {
         pParent->m_RemovalEventIndex = eventIdx;
         FillEventList();
         pCB->SetCurSel((int)eventIdx);
      }
      else
      {
         pCB->SetCurSel((int)pParent->m_RemovalEventIndex);
      }
   }
}

EventIndexType CTemporarySupportLayoutPage::CreateEvent()
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