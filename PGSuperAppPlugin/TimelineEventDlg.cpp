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
// EventDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "TimelineEventDlg.h"
#include "ErectPiersDlg.h"
#include "ErectSegmentsDlg.h"
#include "RemoveTempSupportsDlg.h"
#include "CastClosureJointDlg.h"
#include "ApplyLoadsDlg.h"
#include "ConstructSegmentsDlg.h"
#include "StressTendonDlg.h"




// CTimelineEventDlg dialog

IMPLEMENT_DYNAMIC(CTimelineEventDlg, CDialog)

CTimelineEventDlg::CTimelineEventDlg(const CTimelineManager* pTimelineMgr,BOOL bEditEvent,CWnd* pParent /*=NULL*/)
	: CDialog(CTimelineEventDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr)
{
   m_bEdit = bEditEvent;
}

CTimelineEventDlg::~CTimelineEventDlg()
{
}

void CTimelineEventDlg::DoDataExchange(CDataExchange* pDX)
{
   DDX_Control(pDX,IDC_ADD,m_btnAdd);

	CDialog::DoDataExchange(pDX);

   // need to exchange data with event

   if ( pDX->m_bSaveAndValidate )
   {
      CString description;
      DDX_Text(pDX,IDC_DESCRIPTION,description);
      m_TimelineEvent.SetDescription(description);

      Float64 day;
      DDX_Text(pDX,IDC_DAY,day);
      m_TimelineEvent.SetDay(day);

      // check for duplicate data if the event manager does not have a event with this ID (pTimelineEvent == NULL)
      // and when there is a event with this ID and the day is changing... otherwise, a event with ID
      // is already defined and it's day isn't changing so there wont be duplicates
      const CTimelineEvent* pTimelineEvent = m_pTimelineMgr->GetEventByID(m_TimelineEvent.GetID());
      bool bCheckDay = (pTimelineEvent == NULL ? true : (pTimelineEvent->GetDay() != day ? true : false));
      if ( bCheckDay && m_pTimelineMgr->HasEvent(day) )
      {
         pDX->PrepareEditCtrl(IDC_DAY);
         CString strMsg;
         strMsg.Format(_T("An event is already defined at day %d"),(int)day);
         AfxMessageBox(strMsg);
         pDX->Fail();
      }

#pragma Reminder("UPDATE: event elapsed time validation")
      // if this is a new event, then pTimelineEvent is null. we need a way to validate the duration of this event
      //if ( pTimelineEvent )
      //{
      //   EventIndexType eventIdx = m_pTimelineMgr->GetEventIndex(pTimelineEvent->GetID());
      //   Float64 duration = m_pTimelineMgr->GetDuration(eventIdx);
      //   if ( duration < m_TimelineEvent.GetMinElapsedTime() )
      //   {
      //      pDX->PrepareEditCtrl(IDC_DAY);
      //      CString strMsg;
      //      strMsg.Format(_T("The activities in this event take longer than the duration of the event. Reduce the amount of time for the activities in this event or make the next event start at a later time."));
      //      AfxMessageBox(strMsg);
      //      pDX->Fail();
      //   }
      //}
   }
   else
   {
      CString description = m_TimelineEvent.GetDescription();
      DDX_Text(pDX,IDC_DESCRIPTION,description);

      Float64 day = m_TimelineEvent.GetDay();
      DDX_Text(pDX,IDC_DAY,day);
   }
}


BEGIN_MESSAGE_MAP(CTimelineEventDlg, CDialog)
   ON_BN_CLICKED(IDC_REMOVE, &CTimelineEventDlg::OnRemoveActivities)
   ON_COMMAND(ID_ACTIVITIES_CONSTRUCTSEGMENT,&CTimelineEventDlg::OnConstructSegments)
   ON_COMMAND(ID_ACTIVITIES_ERECT_PIERS,&CTimelineEventDlg::OnErectPiers)
   ON_COMMAND(ID_ACTIVITIES_ERECT_SEGMENTS,&CTimelineEventDlg::OnErectSegments)
   ON_COMMAND(ID_ACTIVITIES_REMOVE_TS,&CTimelineEventDlg::OnRemoveTempSupports)
   ON_COMMAND(ID_ACTIVITIES_CASTCLOSUREJOINTS,&CTimelineEventDlg::OnCastClosureJoints)
   ON_COMMAND(ID_ACTIVITIES_CASTDECK,&CTimelineEventDlg::OnCastDeck)
   ON_COMMAND(ID_ACTIVITIES_APPLYLOADS,&CTimelineEventDlg::OnApplyLoads)
   ON_COMMAND(ID_ACTIVITIES_STRESSTENDON,&CTimelineEventDlg::OnStressTendons)
   ON_BN_CLICKED(ID_HELP, &CTimelineEventDlg::OnHelp)
END_MESSAGE_MAP()

BOOL CTimelineEventDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_ACTIVITY_GRID, this);
   m_Grid.CustomInit();

   CDialog::OnInitDialog();

   m_btnAdd.SetSplit(FALSE);
   UpdateAddButton();

   m_Grid.Refresh();

   if ( !m_bEdit )
   {
      // We are creating a new event
      SetWindowText(_T("Create Event"));

      // Hide the activities gird and related buttons
      m_btnAdd.ShowWindow(SW_HIDE);
      m_Grid.ShowWindow(SW_HIDE);
      GetDlgItem(IDC_REMOVE)->ShowWindow(SW_HIDE);
      
      // change the title of the Activities label
      GetDlgItem(IDC_ACTIVITIES_LABEL)->SetWindowText(_T("Previously defined events"));

      CRect rect;
      m_Grid.GetWindowRect(&rect);
      ScreenToClient(&rect);

      CRect rOK;
      GetDlgItem(IDOK)->GetWindowRect(&rOK);
      ScreenToClient(&rOK);
      rect.bottom = rOK.top - 7;

      // create a list control to display the previously defined events
      m_TimelineEventList.CreateEx(WS_EX_STATICEDGE,WS_CHILD | WS_VISIBLE | LVS_REPORT,rect,this,9999);
      m_TimelineEventList.InsertColumn(0,_T("Event"));
      m_TimelineEventList.InsertColumn(1,_T("Day"));
      m_TimelineEventList.InsertColumn(2,_T("Description"));

      EventIndexType nEvents = m_pTimelineMgr->GetEventCount();
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {

         const CTimelineEvent* pTimelineEvent = m_pTimelineMgr->GetEventByIndex(eventIdx);
         CString strEventIndex;
         strEventIndex.Format(_T("%lld"), LABEL_EVENT(eventIdx) );
         m_TimelineEventList.InsertItem(LVIF_TEXT,(int)eventIdx,strEventIndex,0,0,0,0);

         CString strDay;
         strDay.Format(_T("%d"),(int)pTimelineEvent->GetDay());
         m_TimelineEventList.SetItemText((int)eventIdx,1,strDay);
         m_TimelineEventList.SetItemText((int)eventIdx,2,pTimelineEvent->GetDescription());
      }

      m_TimelineEventList.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
      m_TimelineEventList.SetColumnWidth(1,LVSCW_AUTOSIZE_USEHEADER);
      m_TimelineEventList.SetColumnWidth(2,LVSCW_AUTOSIZE);
   }

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTimelineEventDlg::UpdateAddButton()
{
   m_btnAdd.Clear();

   // Keep the activities in a somewhat logical sequence


   UINT flags = 0;

   flags = (m_TimelineEvent.GetConstructSegmentsActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_CONSTRUCTSEGMENT,_T("Construct Segments"),flags);

   flags = (m_TimelineEvent.GetErectPiersActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_ERECT_PIERS,_T("Erect Piers/Temporary Supports"),flags);

   flags = (m_TimelineEvent.GetErectSegmentsActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_ERECT_SEGMENTS,_T("Erect Segments"),flags);

   flags = (m_TimelineEvent.GetCastClosureJointActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_CASTCLOSUREJOINTS,_T("Cast Closure Joints"),flags);

   flags = (m_TimelineEvent.GetStressTendonActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_STRESSTENDON,_T("Stress Tendons"),flags);

   flags = (m_TimelineEvent.GetRemoveTempSupportsActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_REMOVE_TS,_T("Remove Temporary Supports"),flags);

   flags = (m_TimelineEvent.GetCastDeckActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_CASTDECK,_T("Cast Deck"),flags);

   flags = (m_TimelineEvent.GetApplyLoadActivity().IsEnabled() ? MF_DISABLED : MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_APPLYLOADS,_T("Apply Loads"),flags);
}

void CTimelineEventDlg::OnRemoveActivities()
{
   m_Grid.RemoveActivity();
   UpdateAddButton();
}

void CTimelineEventDlg::OnConstructSegments()
{
   CConstructSegmentsDlg dlg(m_pTimelineMgr);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetConstructSegmentsActivity(dlg.m_ConstructSegments);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnErectPiers()
{
   CErectPiersDlg dlg(m_pTimelineMgr,m_EventIndex);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetErectPiersActivity(dlg.m_ErectPiers);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnErectSegments()
{
   CErectSegmentsDlg dlg(m_pTimelineMgr,m_EventIndex);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetErectSegmentsActivity(dlg.m_ErectSegments);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnRemoveTempSupports()
{
   CRemoveTempSupportsDlg dlg(m_pTimelineMgr,m_EventIndex);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetRemoveTempSupportsActivity(dlg.m_RemoveTempSupports);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnCastClosureJoints()
{
   CCastClosureJointDlg dlg(m_pTimelineMgr);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetCastClosureJointActivity(dlg.m_CastClosureJoints);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnCastDeck()
{
   m_TimelineEvent.GetCastDeckActivity().Enable(true);
   m_Grid.Refresh();
   UpdateAddButton();
}

void CTimelineEventDlg::OnApplyLoads()
{
   CApplyLoadsDlg dlg;
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetApplyLoadActivity(dlg.m_ApplyLoads);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnStressTendons()
{
   CStressTendonDlg dlg(m_pTimelineMgr,m_EventIndex);
   if ( dlg.DoModal() == IDOK )
   {
      m_TimelineEvent.SetStressTendonActivity(dlg.m_StressTendonActivity);
      m_Grid.Refresh();
      UpdateAddButton();
   }
}

void CTimelineEventDlg::OnHelp()
{
   // TODO: Add your control notification handler code here
#pragma Reminder("IMPLEMENT")
   AfxMessageBox(_T("Implement"));
}
