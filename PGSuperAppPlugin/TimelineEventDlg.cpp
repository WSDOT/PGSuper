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
// EventDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "TimelineEventDlg.h"
#include "ErectPiersDlg.h"
#include "ErectSegmentsDlg.h"
#include "RemoveTempSupportsDlg.h"
#include "CastClosureJointDlg.h"
#include "ApplyLoadsDlg.h"
#include "ConstructSegmentsDlg.h"
#include "StressTendonDlg.h"
#include "CastDeckDlg.h"
#include "CastLongitudinalJointsDlg.h"
#include "GeometryControlDlg.h"

#include <EAF\EAFDocument.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>

#include <PgsExt\Helpers.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// CTimelineEventDlg dialog

IMPLEMENT_DYNAMIC(CTimelineEventDlg, CDialog)

CTimelineEventDlg::CTimelineEventDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bEditEvent,EventIndexType minEventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CTimelineEventDlg::IDD, pParent)
{
   m_bEdit = bEditEvent;
   m_bReadOnly = bReadOnly;
   m_TimelineManager = timelineMgr;
   m_EventIndex = eventIdx;
   m_MinEventIdx = minEventIdx;
}

CTimelineEventDlg::~CTimelineEventDlg()
{
}

bool CTimelineEventDlg::UpdateTimelineManager(const CTimelineManager& timelineMgr)
{
   // Get the event that was changed and validate it against the original timeline
   const CTimelineEvent* pNewEvent = timelineMgr.GetEventByIndex(m_EventIndex);
   int result = timelineMgr.ValidateEvent(pNewEvent);

   if ( result != TLM_SUCCESS )
   {
      // The event that was changed doesn't fit... as the user what to do about it
      CString strProblem = timelineMgr.GetErrorMessage(result).c_str();
      CString strRemedy(_T("The timeline will be adjusted to accomodate this event."));

      CString strMsg;
      strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
      if ( AfxMessageBox(strMsg,MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL )
      {
         return false; // problem not fixed...
      }
   }
   m_TimelineManager = timelineMgr; // timelineMgr should have a properly adjusted timeline
   m_pTimelineEvent = m_TimelineManager.GetEventByIndex(m_EventIndex);
   return true;
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
      m_pTimelineEvent->SetDescription(description);

      Float64 day;
      DDX_Text(pDX,IDC_DAY,day);
      
      if ( day < 0 )
      {
         pDX->PrepareEditCtrl(IDC_DAY);
         AfxMessageBox(_T("The day when this event occurs must be zero or more"),MB_OK);
         pDX->Fail();
      }

      m_pTimelineEvent->SetDay(day);

      m_EventIndex = m_TimelineManager.GetEventIndex(m_pTimelineEvent->GetID());
   }
   else
   {
      CString description = m_pTimelineEvent->GetDescription();
      DDX_Text(pDX,IDC_DESCRIPTION,description);

      Float64 day = m_pTimelineEvent->GetDay();
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
   ON_COMMAND(ID_ACTIVITIES_CASTDECK, &CTimelineEventDlg::OnCastDeck)
   ON_COMMAND(ID_ACTIVITIES_CASTLONGITUDINALJOINTS, &CTimelineEventDlg::OnCastLongitudinalJoints)
   ON_COMMAND(ID_ACTIVITIES_APPLYLOADS,&CTimelineEventDlg::OnApplyLoads)
   ON_COMMAND(ID_ACTIVITIES_GEOMETRYCONTROLEVENT,&CTimelineEventDlg::OnGeometryControl)
   ON_COMMAND(ID_ACTIVITIES_STRESSTENDON,&CTimelineEventDlg::OnStressTendons)
   ON_BN_CLICKED(ID_HELP, &CTimelineEventDlg::OnHelp)
END_MESSAGE_MAP()

BOOL CTimelineEventDlg::OnInitDialog()
{
   // must set m_pTimelineEvent before calling CDialog::OnInitDialog
   if ( m_EventIndex == INVALID_INDEX )
   {
      m_pTimelineEvent = new CTimelineEvent();
      m_TimelineManager.AppendTimelineEvent(m_pTimelineEvent,&m_EventIndex);
   }
   else
   {
      // this dialog is being used to edit an existing event
      CString strTitle;
      strTitle.Format(_T("Edit Event %d"),LABEL_EVENT(m_EventIndex));
      SetWindowText(strTitle);
      m_pTimelineEvent = m_TimelineManager.GetEventByIndex(m_EventIndex);
   }

   m_Grid.SubclassDlgItem(IDC_ACTIVITY_GRID, this);
   m_Grid.CustomInit(m_bReadOnly);

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

      EventIndexType nEvents = m_TimelineManager.GetEventCount();
      for ( EventIndexType eventIdx = (m_MinEventIdx == INVALID_INDEX ? 0 : m_MinEventIdx); eventIdx < nEvents; eventIdx++ )
      {
         const CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(eventIdx);
         CString strEventIndex;
         strEventIndex.Format(_T("%lld"), LABEL_EVENT(eventIdx) );

         int item = (int)(eventIdx - (m_MinEventIdx == INVALID_INDEX ? 0 : m_MinEventIdx));
         m_TimelineEventList.InsertItem(LVIF_TEXT,item,strEventIndex,0,0,0,0);

         CString strDay;
         strDay.Format(_T("%d"),(int)pTimelineEvent->GetDay());
         m_TimelineEventList.SetItemText(item,1,strDay);
         m_TimelineEventList.SetItemText(item,2,pTimelineEvent->GetDescription());
      }

      m_TimelineEventList.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
      m_TimelineEventList.SetColumnWidth(1,LVSCW_AUTOSIZE_USEHEADER);
      m_TimelineEventList.SetColumnWidth(2,LVSCW_AUTOSIZE);
   }

   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_DAY)->EnableWindow(FALSE);
      GetDlgItem(IDC_DESCRIPTION)->EnableWindow(FALSE);

      GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_REMOVE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);

      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTimelineEventDlg::UpdateAddButton()
{
   m_btnAdd.Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   CString strErectPiers;
   CString strConstructSegments;
   CString strErectSegments;
   if ( pDocType->IsPGSuperDocument() )
   {
      strErectPiers        = _T("Erect Piers");
      strConstructSegments = _T("Construct Girders");
      strErectSegments     = _T("Erect Girders");
   }
   else
   {
      strErectPiers        = _T("Erect Piers/Temporary Supports");
      strConstructSegments = _T("Construct Segments");
      strErectSegments     = _T("Erect Segments");
   }

   // Keep the activities in a somewhat logical sequence
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_ERECT_PIERS,strErectPiers,MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_CONSTRUCTSEGMENT,strConstructSegments,MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_ERECT_SEGMENTS,strErectSegments,MF_ENABLED);

   if ( pDocType->IsPGSpliceDocument() )
   {
      m_btnAdd.AddMenuItem(ID_ACTIVITIES_CASTCLOSUREJOINTS,_T("Cast Closure Joints"),MF_ENABLED);
      m_btnAdd.AddMenuItem(ID_ACTIVITIES_STRESSTENDON,_T("Stress Tendons"),MF_ENABLED);
      m_btnAdd.AddMenuItem(ID_ACTIVITIES_REMOVE_TS,_T("Remove Temporary Supports"),MF_ENABLED);
   }

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   if (pIBridgeDesc->GetBridgeDescription()->HasStructuralLongitudinalJoints())
   {
      m_btnAdd.AddMenuItem(ID_ACTIVITIES_CASTLONGITUDINALJOINTS, _T("Cast Longitudinal Joints"), MF_ENABLED);
   }
   CString strName(GetCastDeckEventName(pIBridgeDesc->GetDeckDescription()->GetDeckType()));
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_CASTDECK,strName,MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_APPLYLOADS,_T("Apply Loads"),MF_ENABLED);
   m_btnAdd.AddMenuItem(ID_ACTIVITIES_GEOMETRYCONTROLEVENT,_T("Geometry Control Event"),MF_ENABLED);
}

void CTimelineEventDlg::OnRemoveActivities()
{
   m_Grid.RemoveActivity();
}

template <class T>
bool EditEvent(CTimelineEventDlg* pTimelineEventDlg,T* pActivityDlg)
{
   if ( pActivityDlg->DoModal() == IDOK ) // so the dialog
   {
      // user pressed OK
      // check the update the timeline... if UpdateTimelineManager returns true
      // it was successful, otherwise, go back to the dialog
      pTimelineEventDlg->m_EventIndex = pActivityDlg->m_EventIndex;
      while ( pTimelineEventDlg->UpdateTimelineManager(pActivityDlg->m_TimelineMgr) == false )
      {
         if ( pActivityDlg->DoModal() == IDCANCEL )
         {
            // user canceled the edit
            return false;
         }
      }
      // timeline has been updated
      return true;
   }
   return false; // user pressed cancel
}

void CTimelineEventDlg::OnConstructSegments()
{
   UpdateData();
   CConstructSegmentsDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnErectPiers()
{
   UpdateData();
   CErectPiersDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnErectSegments()
{
   UpdateData();
   CErectSegmentsDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnRemoveTempSupports()
{
   UpdateData();
   CRemoveTempSupportsDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnCastClosureJoints()
{
   UpdateData();
   CCastClosureJointDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnCastDeck()
{
   UpdateData();
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   pgsTypes::SupportedDeckType deckType = pIBridgeDesc->GetDeckDescription()->GetDeckType();
   CString strName(GetCastDeckEventName(deckType));

   CCastDeckDlg dlg(strName,m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnCastLongitudinalJoints()
{
   UpdateData();
   CCastLongitudinalJointsDlg dlg(m_TimelineManager, m_EventIndex, m_bReadOnly);
   if (EditEvent(this, &dlg))
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnApplyLoads()
{
   UpdateData();
   CApplyLoadsDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnGeometryControl()
{
   UpdateData();
   CGeometryControlDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if (EditEvent(this,&dlg))
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnStressTendons()
{
   UpdateData();
   CStressTendonDlg dlg(m_TimelineManager,m_EventIndex,m_bReadOnly);
   if ( EditEvent(this,&dlg) )
   {
      m_Grid.Refresh();
   }
}

void CTimelineEventDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),m_bEdit ? IDH_TIMELINE_EVENT : IDH_TIMELINE_CREATE_EVENT);
}
