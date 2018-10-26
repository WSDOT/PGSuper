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
// EditMomentLoadDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>


#include <System\Tokenizer.h>

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditMomentLoadDlg dialog


CEditMomentLoadDlg::CEditMomentLoadDlg(const CMomentLoadData& load,const CTimelineManager* pTimelineMgr,CWnd* pParent /*=nullptr*/):
	CDialog(CEditMomentLoadDlg::IDD, pParent),
   m_Load(load),
   m_TimelineMgr(*pTimelineMgr)
{
	//{{AFX_DATA_INIT(CEditMomentLoadDlg)
	//}}AFX_DATA_INIT
   EAFGetBroker(&m_pBroker);

   m_EventID = m_TimelineMgr.FindUserLoadEventID(m_Load.m_ID);

   m_bWasNewEventCreated = false;
}


void CEditMomentLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   if ( !pDX->m_bSaveAndValidate )
   {
      CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_END);
      m_LocationIdx = IsZero(m_Load.m_Location) ? 0 : 1;
   }

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditMomentLoadDlg)
	DDX_Control(pDX, IDC_SPAN_LENGTH_CTRL, m_SpanLengthCtrl);
	DDX_Control(pDX, IDC_GIRDERS, m_GirderCB);
	DDX_Control(pDX, IDC_SPANS, m_SpanCB);
	//}}AFX_DATA_MAP
	DDX_String(pDX, IDC_DESCRIPTION, m_Load.m_Description);
   DDX_CBIndex(pDX,IDC_GIRDER_END,m_LocationIdx);

   // magnitude is easy part
   DDX_UnitValueAndTag( pDX, IDC_MAGNITUDE, IDC_MAGNITUDE_UNITS, m_Load.m_Magnitude, pDisplayUnits->GetMomentUnit() );

   DDX_CBItemData(pDX,IDC_EVENT,m_EventID);

   // other values need to be done manually
   if (pDX->m_bSaveAndValidate)
   {
      int ival;
      DDX_CBIndex(pDX,IDC_LOADCASE,ival);
      m_Load.m_LoadCase = UserLoads::GetLoadCase(ival);

      EventIDType liveLoadEventID = m_TimelineMgr.GetLiveLoadEventID();

      if ( m_Load.m_LoadCase == UserLoads::LL_IM && m_EventID != liveLoadEventID )
      {
         AfxMessageBox(_T("The LL+IM load case can only be used in the events when live load is defined.\n\nChange the Load Case or Event."));
         pDX->PrepareCtrl(IDC_LOADCASE);
         pDX->Fail();
      }

      if ( m_TimelineMgr.GetEventIndex(m_EventID) < m_TimelineMgr.GetFirstSegmentErectionEventIndex() )
      {
         AfxMessageBox(_T("User defined loads can only be applied at the bridge site"));
         pDX->PrepareCtrl(IDC_EVENT);
         pDX->Fail();
      }

      ival = m_SpanCB.GetCurSel();

      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;

      if (ival == m_SpanCB.GetCount()-1)
      {
         spanIdx = ALL_SPANS;
      }
      else
      {
         spanIdx = ival;
      }

      ival = m_GirderCB.GetCurSel();
      if (ival == m_GirderCB.GetCount()-1 )
      {
         gdrIdx = ALL_GIRDERS;
      }
      else
      {
         gdrIdx = ival;
      }

      m_Load.m_SpanKey.spanIndex = spanIdx;
      m_Load.m_SpanKey.girderIndex = gdrIdx;

      // this must always be a fractional measure at the start or end of the girder
      if ( m_LocationIdx == 0 )
      {
         m_Load.m_Location = 0;
         m_Load.m_Fractional = true;
      }
      else
      {
         m_Load.m_Location = 1.0;
         m_Load.m_Fractional = true;
      }
   }
}


BEGIN_MESSAGE_MAP(CEditMomentLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CEditMomentLoadDlg)
	ON_CBN_SELCHANGE(IDC_LOADCASE, OnEditchangeLoadcase)
	ON_CBN_SELCHANGE(IDC_SPANS, OnEditchangeSpans)
	ON_CBN_SELCHANGE(IDC_GIRDERS, OnEditchangeGirders)
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditMomentLoadDlg message handlers

BOOL CEditMomentLoadDlg::OnInitDialog() 
{
	// fill up controls
   // events, load cases
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOADCASE);
   for (Int32 ilc=0; ilc<UserLoads::GetNumLoadCases(); ilc++)
   {
      CString str(UserLoads::GetLoadCaseName(ilc).c_str());
      pCB->AddString(str);
   }

   pCB->SetCurSel(m_Load.m_LoadCase);

   FillEventList();
   if ( m_EventID == INVALID_ID )
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
      pcbEvent->SetCurSel(0);
      m_EventID = (EventIDType)pcbEvent->GetItemData(0);
   }

   CDialog::OnInitDialog();

   m_WasLiveLoad = m_Load.m_LoadCase == UserLoads::LL_IM;

//   UpdateEventLoadCase(true);

   // spans, girders
   GET_IFACE(IBridge, pBridge);
   SpanIndexType nSpans   = pBridge->GetSpanCount();

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CString str;
      str.Format(_T("Span %d"), LABEL_SPAN(spanIdx));
      m_SpanCB.AddString(str);
   }

   m_SpanCB.AddString(_T("All Spans"));


   SpanIndexType spanIdx  = m_Load.m_SpanKey.spanIndex;
   GirderIndexType gdrIdx = m_Load.m_SpanKey.girderIndex;

   if (spanIdx == ALL_SPANS)
   {
      m_SpanCB.SetCurSel((int)nSpans);
   }
   else
   {
      if ( 0 <= spanIdx && spanIdx < nSpans)
      {
         m_SpanCB.SetCurSel((int)spanIdx);
      }
      else
      {
         ::AfxMessageBox(_T("Warning - The Span for this load is out of range. Resetting to Span 1"));
         m_Load.m_SpanKey.spanIndex = 0;
         m_Load.m_SpanKey.girderIndex = gdrIdx;
         m_SpanCB.SetCurSel((int)spanIdx);
      }
   }

   UpdateGirderList();

   if (gdrIdx==ALL_GIRDERS)
   {
      m_GirderCB.SetCurSel(m_GirderCB.GetCount()-1);
   }
   else
   {
      if (0 <= gdrIdx && gdrIdx < GirderIndexType(m_GirderCB.GetCount()-1) )
      {
         m_GirderCB.SetCurSel((int)gdrIdx);
      }
      else
      {
         m_Load.m_SpanKey.girderIndex = 0;

         CString strMsg;
         strMsg.Format(_T("Warning - The Girder for this load is out of range. Resetting to Girder %s"),LABEL_GIRDER(m_Load.m_SpanKey.girderIndex));
         ::AfxMessageBox(strMsg);
         m_GirderCB.SetCurSel((int)m_Load.m_SpanKey.girderIndex);
      }
   }

   //UpdateEventLoadCase();
   UpdateSpanLength();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_MOMENT_LOAD),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditMomentLoadDlg::UpdateEventLoadCase(bool isInitial)
{
   CComboBox* pcbLoadCase = (CComboBox*)GetDlgItem(IDC_LOADCASE);
   CComboBox* pcbEvent    = (CComboBox*)GetDlgItem(IDC_EVENT);

   EventIndexType castDeckEventIdx      = m_TimelineMgr.GetCastDeckEventIndex();
   EventIndexType railingSystemEventIdx = m_TimelineMgr.GetRailingSystemLoadEventIndex();
   EventIndexType liveLoadEventIdx      = m_TimelineMgr.GetLiveLoadEventIndex();

   if(pcbLoadCase->GetCurSel() == UserLoads::LL_IM)
   {
      pcbEvent->ResetContent();
      const CTimelineEvent* pTimelineEvent = m_TimelineMgr.GetEventByIndex(liveLoadEventIdx);
      CString strEvent;
      strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(liveLoadEventIdx),pTimelineEvent->GetDescription());
      int idx = pcbEvent->AddString(strEvent);
      pcbEvent->SetItemData(idx,DWORD_PTR(pTimelineEvent->GetID()));
      pcbEvent->SetCurSel(0);
      pcbEvent->EnableWindow(FALSE);

      m_WasLiveLoad = true;
   }
   else
   {
      if (isInitial || m_WasLiveLoad)
      {
         pcbEvent->ResetContent();
         const CTimelineEvent* pTimelineEvent = m_TimelineMgr.GetEventByIndex(castDeckEventIdx);
         CString strEvent;
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(castDeckEventIdx),pTimelineEvent->GetDescription());
         int idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(pTimelineEvent->GetID()));

         pTimelineEvent = m_TimelineMgr.GetEventByIndex(railingSystemEventIdx);
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(railingSystemEventIdx),pTimelineEvent->GetDescription());
         idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(pTimelineEvent->GetID()));

         pcbEvent->EnableWindow(TRUE);

         if (isInitial)
         {
            EventIDType castDeckEventID      = m_TimelineMgr.GetCastDeckEventID();
            EventIDType railingSystemEventID = m_TimelineMgr.GetRailingSystemLoadEventID();
            EventIDType liveLoadEventID      = m_TimelineMgr.GetLiveLoadEventID();
            if ( m_EventID == castDeckEventID )
            {
               pcbEvent->SetCurSel(0);
            }
            else if ( m_EventID == railingSystemEventID )
            {
               pcbEvent->SetCurSel(1);
            }
            else
            {
               pcbEvent->SetCurSel(0);
               m_EventID = castDeckEventID;
            }
         }
         else
         {
            pcbEvent->SetCurSel(0);
         }
      }
  
      m_WasLiveLoad = false;
   }
}

void CEditMomentLoadDlg::OnEditchangeLoadcase() 
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      UpdateEventLoadCase();
   }
}

void CEditMomentLoadDlg::OnEditchangeSpans() 
{
   UpdateGirderList();
   UpdateSpanLength();
}

void CEditMomentLoadDlg::OnEditchangeGirders() 
{
   UpdateSpanLength();
}

void CEditMomentLoadDlg::UpdateSpanLength() 
{
	int spn = m_SpanCB.GetCurSel();
	int gdr = m_GirderCB.GetCurSel();

   if (spn == m_SpanCB.GetCount()-1 || gdr == m_GirderCB.GetCount()-1)
   {
      m_SpanLengthCtrl.SetWindowText(_T("Span Length = N/A"));
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridge, pBridge);
      CSegmentKey segmentKey(spn,gdr,0);
      Float64 span_length = pBridge->GetSegmentSpanLength(segmentKey);
      CString strLabel;
      strLabel.Format(_T("Span Length = %s"),FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit(),false));
      m_SpanLengthCtrl.SetWindowText(strLabel);
   }
}

void CEditMomentLoadDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_MOMENT_LOADS );
}

void CEditMomentLoadDlg::UpdateGirderList()
{
   GET_IFACE(IBridge, pBridge);
   SpanIndexType spanIdx = m_SpanCB.GetCurSel();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   int curSel = m_GirderCB.GetCurSel();
   BOOL bAllSelected = (curSel == m_GirderCB.GetCount()-1);
   m_GirderCB.ResetContent();

   GirderIndexType nMaxGirders = 9999;
   if ( spanIdx == nSpans )
   {
      // loading applies to all spans
      // need to find the span with the fewest girders
      for ( SpanIndexType i = 0; i < nSpans; i++ )
      {
         GirderIndexType cGirders = pBridge->GetGirderCount(i);
         if ( cGirders < nMaxGirders )
         {
            nMaxGirders = cGirders;
            spanIdx = i;
         }
      }
   }
   else
   {
      nMaxGirders = pBridge->GetGirderCount(spanIdx);
   }

   GirderIndexType nGirders = nMaxGirders;

   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      CString str;
      str.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
      m_GirderCB.AddString(str);
   }

    m_GirderCB.AddString(_T("All Girders"));
    if ( curSel != CB_ERR )
    {
       if ( bAllSelected )
       {
         m_GirderCB.SetCurSel( m_GirderCB.GetCount()-1 );
       }
       else
       {
          if ( m_GirderCB.GetCount()-1 == curSel )
          {
             curSel = 0;
          }

         curSel = m_GirderCB.SetCurSel( curSel );
       }
    }

    if ( curSel == CB_ERR )
    {
       m_GirderCB.SetCurSel(0);
    }
}

void CEditMomentLoadDlg::FillEventList()
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      UpdateEventLoadCase(true);
   }
   else
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

      int selEventIdx = pcbEvent->GetCurSel();

      pcbEvent->ResetContent();

      EventIndexType nEvents = m_TimelineMgr.GetEventCount();
      bool bValidEvent = false;
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {
         const CTimelineEvent* pTimelineEvent = m_TimelineMgr.GetEventByIndex(eventIdx);
         if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
         {
            bValidEvent = true;
         }

         if ( bValidEvent )
         {
            CString label;
            label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

            EventIDType eventID = pTimelineEvent->GetID();
            pcbEvent->SetItemData(pcbEvent->AddString(label),eventID);
         }
      }

      CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
      pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

      if ( selEventIdx != CB_ERR )
      {
         pcbEvent->SetCurSel(selEventIdx);
      }
      else
      {
         pcbEvent->SetCurSel(0);
      }
   }
}

void CEditMomentLoadDlg::OnEventChanging()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pcbEvent->GetCurSel();
}

void CEditMomentLoadDlg::OnEventChanged()
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
      int curSel = pCB->GetCurSel();
      EventIDType id = (EventIDType)pCB->GetItemData(curSel);
      if ( id == CREATE_TIMELINE_EVENT )
      {
         EventIndexType idx = CreateEvent();
         if ( idx != INVALID_INDEX )
         {
            FillEventList();
            EventIndexType firstEventIdx = m_TimelineMgr.GetFirstSegmentErectionEventIndex();
            int curSel = (int)(idx - firstEventIdx);
            pCB->SetCurSel(curSel);
         }
         else
         {
            pCB->SetCurSel(m_PrevEventIdx);
         }
      }
   }
}

EventIndexType CEditMomentLoadDlg::CreateEvent()
{
   CTimelineEventDlg dlg(m_TimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      m_bWasNewEventCreated = true;
      EventIndexType eventIdx;
      m_TimelineMgr.AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      return eventIdx;
   }

   return INVALID_INDEX;
}
