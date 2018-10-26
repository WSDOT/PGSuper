///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
// EditPointLoadDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditPointLoadDlg.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#include <System\Tokenizer.h>
#include <..\htmlhelp\HelpTopics.hh>

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditPointLoadDlg dialog


CEditPointLoadDlg::CEditPointLoadDlg(const CPointLoadData& load,const CTimelineManager* pTimelineMgr,CWnd* pParent /*=NULL*/):
	CDialog(CEditPointLoadDlg::IDD, pParent),
   m_Load(load),
   m_TimelineMgr(*pTimelineMgr)
{
   m_bWasNewEventCreated = false;

	//{{AFX_DATA_INIT(CEditPointLoadDlg)
	//}}AFX_DATA_INIT
   EAFGetBroker(&m_pBroker);
}


void CEditPointLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditPointLoadDlg)
	DDX_Control(pDX, IDC_SPAN_LENGTH_CTRL, m_SpanLengthCtrl);
	DDX_Control(pDX, IDC_LOCATION, m_LocationCtrl);
	DDX_Control(pDX, IDC_LOCATION_UNITS, m_LocationUnitCtrl);
	DDX_Control(pDX, IDC_FRACTIONAL, m_FractionalCtrl);
	DDX_Control(pDX, IDC_GIRDERS, m_GirderCB);
	DDX_Control(pDX, IDC_SPANS, m_SpanCB);
	//}}AFX_DATA_MAP

   DDX_String(pDX, IDC_DESCRIPTION, m_Load.m_Description);

   // magnitude is easy part
   DDX_UnitValueAndTag( pDX, IDC_MAGNITUDE, IDC_MAGNITUDE_UNITS, m_Load.m_Magnitude, pDisplayUnits->GetGeneralForceUnit() );

   DDX_CBItemData(pDX,IDC_EVENT,m_Load.m_EventIndex);

   // other values need to be done manually
   if (pDX->m_bSaveAndValidate)
   {
      int ival;
      DDX_CBIndex(pDX,IDC_LOADCASE,ival);
      m_Load.m_LoadCase = UserLoads::GetLoadCase(ival);

      GET_IFACE(IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
      EventIndexType liveLoadEventIdx = pTimelineMgr->GetLiveLoadEventIndex();

      if ( m_Load.m_LoadCase == UserLoads::LL_IM && m_Load.m_EventIndex != liveLoadEventIdx )
      {
         AfxMessageBox(_T("The LL+IM load case can only be used in the events when live load is defined.\n\nChange the Load Case or Event."));
         pDX->PrepareCtrl(IDC_LOADCASE);
         pDX->Fail();
      }

      if ( m_Load.m_EventIndex < pTimelineMgr->GetFirstSegmentErectionEventIndex() )
      {
         AfxMessageBox(_T("User defined loads can only be applied at the bridge site"));
         pDX->PrepareCtrl(IDC_EVENT);
         pDX->Fail();
      }

      ival = m_SpanCB.GetCurSel();
      m_Load.m_SpanKey.spanIndex = m_Spans[ival].spanIdx;
      m_Load.m_bLoadOnCantilever[pgsTypes::metStart] = m_Spans[ival].bStartCantilever;
      m_Load.m_bLoadOnCantilever[pgsTypes::metEnd]   = m_Spans[ival].bEndCantilever;

      GirderIndexType gdrIdx;
      ival = m_GirderCB.GetCurSel();
      if (ival == m_GirderCB.GetCount()-1 )
      {
         gdrIdx = ALL_GIRDERS;
      }
      else
      {
         gdrIdx = ival;
      }

      m_Load.m_SpanKey.girderIndex = gdrIdx;
      

      // location takes some effort
      Float64 locval;
      CString str;
      m_LocationCtrl.GetWindowText(str);
      if (!sysTokenizer::ParseDouble(str, &locval))
      {
      	HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
         ::AfxMessageBox(_T("Please enter a number"));
         pDX->Fail();
      }

      m_Load.m_Fractional = m_FractionalCtrl.GetCheck()!=FALSE;

      if (m_Load.m_Fractional)
      {
         if (0.0 <= locval && locval <= 1.0)
         {
            m_Load.m_Location = locval;
         }
         else
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
            ::AfxMessageBox(_T("Invalid Value: Fractional values must range from 0.0 to 1.0"));
            pDX->Fail();
         }
      }
      else
      {
         if (0.0 <= locval)
         {
            m_Load.m_Location = ::ConvertToSysUnits(locval, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
         }
         else
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
            ::AfxMessageBox(_T("Invalid Value: Location values must be zero or greater"));
            pDX->Fail();
         }
      }
   }
   else
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
      if ( nSpans <= m_Load.m_SpanKey.spanIndex && m_Load.m_SpanKey.spanIndex != ALL_SPANS )
      {
         ::AfxMessageBox(_T("Warning - The span for this load is out of range. Resetting to Span 1"));
         m_Load.m_SpanKey.spanIndex = 0;
         m_Load.m_bLoadOnCantilever[pgsTypes::metStart] = false;
         m_Load.m_bLoadOnCantilever[pgsTypes::metEnd]   = false;
      }

      if ( m_Load.m_bLoadOnCantilever[pgsTypes::metStart] || m_Load.m_bLoadOnCantilever[pgsTypes::metEnd] )
      {
         ATLASSERT(m_Load.m_SpanKey.spanIndex == 0 || m_Load.m_SpanKey.spanIndex == nSpans-1);
         const CSpanData2* pSpan = pBridgeDesc->GetSpan(m_Load.m_SpanKey.spanIndex);
         if ( (m_Load.m_SpanKey.spanIndex == 0 && !pSpan->GetPrevPier()->HasCantilever())
              ||
              (m_Load.m_SpanKey.spanIndex == nSpans-1 && !pSpan->GetNextPier()->HasCantilever())
            )
         {
            ::AfxMessageBox(_T("Warning - The span for this loading does not have a cantilever. Moving the load into the span"));
            m_Load.m_bLoadOnCantilever[pgsTypes::metStart] = false;
            m_Load.m_bLoadOnCantilever[pgsTypes::metEnd]   = false;
         }
      }

      int i = 0;
      std::vector<SpanType>::iterator iter(m_Spans.begin());
      std::vector<SpanType>::iterator end(m_Spans.end());
      for ( ; iter != end; iter++, i++ )
      {
         SpanType& spanType(*iter);
         if ( spanType.spanIdx == m_Load.m_SpanKey.spanIndex && 
            spanType.bStartCantilever == m_Load.m_bLoadOnCantilever[pgsTypes::metStart] &&
            spanType.bEndCantilever == m_Load.m_bLoadOnCantilever[pgsTypes::metEnd] )
         {
            m_SpanCB.SetCurSel(i);
            break;
         }
      }
      ATLASSERT(m_SpanCB.GetCurSel() != CB_ERR);

      const CSpanData2* pSpan = pBridgeDesc->GetSpan(m_Load.m_SpanKey.spanIndex);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType nGirders = pGroup->GetGirderCount();

      if ( nGirders <= m_Load.m_SpanKey.girderIndex && m_Load.m_SpanKey.girderIndex != ALL_GIRDERS )
      {
         m_Load.m_SpanKey.girderIndex = 0;

         CString strMsg;
         strMsg.Format(_T("Warning - The Girder for this load is out of range. Resetting to Girder %s"),LABEL_GIRDER(m_Load.m_SpanKey.girderIndex));
         ::AfxMessageBox(strMsg);
      }

      DDX_CBIndex(pDX,IDC_GIRDERS,m_Load.m_SpanKey.girderIndex);
   }
}


BEGIN_MESSAGE_MAP(CEditPointLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CEditPointLoadDlg)
	ON_BN_CLICKED(IDC_FRACTIONAL, OnFractional)
	ON_CBN_SELCHANGE(IDC_LOADCASE, OnEditchangeLoadcase)
	ON_CBN_SELCHANGE(IDC_SPANS, OnEditchangeSpans)
	ON_CBN_SELCHANGE(IDC_GIRDERS, OnEditchangeGirders)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditPointLoadDlg message handlers

BOOL CEditPointLoadDlg::OnInitDialog() 
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
   if ( m_Load.m_EventIndex == INVALID_INDEX )
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
      pcbEvent->SetCurSel(0);
      m_Load.m_EventIndex = (EventIndexType)pcbEvent->GetItemData(0);
   }

   UpdateSpanList();
   UpdateGirderList();

	CDialog::OnInitDialog();

   m_WasLiveLoad = m_Load.m_LoadCase == UserLoads::LL_IM;

   // location
   m_FractionalCtrl.SetCheck(m_Load.m_Fractional);

   if (m_Load.m_Fractional)
   {
      sysNumericFormatTool tool;
      m_LocationCtrl.SetWindowText(tool.AsString(m_Load.m_Location).c_str());
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      CString strLocation;
      strLocation.Format(_T("%s"),::FormatDimension(m_Load.m_Location,pDisplayUnits->GetSpanLengthUnit(),false));
      m_LocationCtrl.SetWindowText(strLocation);
   }

   UpdateLocationUnit();
   UpdateSpanLength();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_POINT_LOAD),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPointLoadDlg::OnFractional() 
{
   UpdateLocationUnit();
}

void CEditPointLoadDlg::UpdateLocationUnit()
{
	int chk = m_FractionalCtrl.GetCheck();
   if (chk)
   {
      m_LocationUnitCtrl.SetWindowText(_T("[0.0 - 1.0]"));
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      m_LocationUnitCtrl.SetWindowText(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   }
}

void CEditPointLoadDlg::UpdateEventLoadCase(bool isInitial)
{
   CComboBox* pcbLoadCase = (CComboBox*)GetDlgItem(IDC_LOADCASE);
   CComboBox* pcbEvent    = (CComboBox*)GetDlgItem(IDC_EVENT);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   EventIndexType railingSystemEventIdx = pIBridgeDesc->GetRailingSystemLoadEventIndex();
   EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();

   if(pcbLoadCase->GetCurSel() == UserLoads::LL_IM)
   {
      pcbEvent->ResetContent();
      const CTimelineEvent* pTimelineEvent = pIBridgeDesc->GetEventByIndex(liveLoadEventIdx);
      CString strEvent;
      strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(liveLoadEventIdx),pTimelineEvent->GetDescription());
      int idx = pcbEvent->AddString(strEvent);
      pcbEvent->SetItemData(idx,DWORD_PTR(liveLoadEventIdx));
      pcbEvent->SetCurSel(0);
      pcbEvent->EnableWindow(FALSE);

      m_WasLiveLoad = true;
   }
   else
   {
      if (isInitial || m_WasLiveLoad)
      {
         pcbEvent->ResetContent();
         const CTimelineEvent* pTimelineEvent = pIBridgeDesc->GetEventByIndex(castDeckEventIdx);
         CString strEvent;
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(castDeckEventIdx),pTimelineEvent->GetDescription());
         int idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(castDeckEventIdx));

         pTimelineEvent = pIBridgeDesc->GetEventByIndex(railingSystemEventIdx);
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(railingSystemEventIdx),pTimelineEvent->GetDescription());
         idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(railingSystemEventIdx));

         pcbEvent->EnableWindow(TRUE);

         if (isInitial)
         {
            if ( m_Load.m_EventIndex == castDeckEventIdx )
            {
               pcbEvent->SetCurSel(0);
            }
            else if ( m_Load.m_EventIndex == railingSystemEventIdx )
            {
               pcbEvent->SetCurSel(1);
            }
            else
            {
               pcbEvent->SetCurSel(0);
               m_Load.m_EventIndex = castDeckEventIdx;
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

void CEditPointLoadDlg::OnEditchangeLoadcase() 
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      UpdateEventLoadCase();
   }
}

void CEditPointLoadDlg::OnEditchangeSpans() 
{
   UpdateGirderList();
   UpdateSpanLength();
}

void CEditPointLoadDlg::OnEditchangeGirders() 
{
   UpdateSpanLength();
}

void CEditPointLoadDlg::UpdateSpanLength() 
{
   CComboBox* pcbSpans = (CComboBox*)GetDlgItem(IDC_SPANS);

   int idx = pcbSpans->GetCurSel();
   SpanIndexType spanIdx = m_Spans[idx].spanIdx;

   int gdrIdx  = m_GirderCB.GetCurSel();

   if (spanIdx == ALL_SPANS || gdrIdx == m_GirderCB.GetCount()-1)
   {
      CString str(_T("Span Length = N/A"));
      m_SpanLengthCtrl.SetWindowText(str);
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridge, pBridge);
      if ( m_Spans[idx].bStartCantilever || m_Spans[idx].bEndCantilever )
      {
         Float64 cantilever_length = pBridge->GetCantileverLength(spanIdx,gdrIdx,(m_Spans[idx].bStartCantilever ? pgsTypes::metStart : pgsTypes::metEnd));
         CString str;
         str.Format(_T("Cantilever Length = %s"),FormatDimension(cantilever_length,pDisplayUnits->GetSpanLengthUnit()));
         m_SpanLengthCtrl.SetWindowText(str);
      }
      else
      {
         Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);
         CString str;
         str.Format(_T("Span Length = %s"),FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit()));
         m_SpanLengthCtrl.SetWindowText(str);
      }
   }
}

void CEditPointLoadDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_EDIT_POINT_LOADS );
}

void CEditPointLoadDlg::UpdateSpanList()
{
   CComboBox* pcbSpans = (CComboBox*)GetDlgItem(IDC_SPANS);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      if ( spanIdx == 0 )
      {
         const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
         const CPierData2* pPier = pSpan->GetPrevPier();
         if ( pPier->HasCantilever() )
         {
            CString str;
            str.Format(_T("Span %d Start Cantilever"), LABEL_SPAN(spanIdx));
            pcbSpans->AddString(str);
            SpanType spanType;
            spanType.spanIdx = spanIdx;
            spanType.bStartCantilever = true;
            spanType.bEndCantilever = false;
            m_Spans.push_back(spanType);
         }
      }

      CString str;
      str.Format(_T("Span %d"), LABEL_SPAN(spanIdx));
      pcbSpans->AddString(str);
      SpanType spanType;
      spanType.spanIdx = spanIdx;
      spanType.bStartCantilever = false;
      spanType.bEndCantilever = false;
      m_Spans.push_back(spanType);

      if ( spanIdx == nSpans-1 )
      {
         const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
         const CPierData2* pPier = pSpan->GetNextPier();
         if ( pPier->HasCantilever() )
         {
            CString str;
            str.Format(_T("Span %d End Cantilever"), LABEL_SPAN(spanIdx));
            pcbSpans->AddString(str);
            SpanType spanType;
            spanType.spanIdx = spanIdx;
            spanType.bStartCantilever = false;
            spanType.bEndCantilever = true;
            m_Spans.push_back(spanType);
         }
      }
   }

   pcbSpans->AddString(_T("All Spans"));
   SpanType spanType;
   spanType.spanIdx = ALL_SPANS;
   spanType.bStartCantilever = false;
   spanType.bEndCantilever = false;
   m_Spans.push_back(spanType);
}

void CEditPointLoadDlg::UpdateGirderList()
{
   CComboBox* pcbSpans = (CComboBox*)GetDlgItem(IDC_SPANS);
   CComboBox* pcbGirders = (CComboBox*)GetDlgItem(IDC_GIRDERS);

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   int idx = pcbSpans->GetCurSel();
   if ( idx == CB_ERR )
   {
      idx = 0;
   }
   SpanIndexType spanIdx = m_Spans[idx].spanIdx;
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();

   int curSel = pcbGirders->GetCurSel();
   BOOL bAllSelected = (curSel == pcbGirders->GetCount()-1);
   pcbGirders->ResetContent();

   GirderIndexType nMaxGirders = 9999; // this is the maximum number of girders that can be listed
                                       // in the combo box (not the maximum number of girders)
   if ( spanIdx == ALL_SPANS )
   {
      // loading applies to all spans
      // need to find the span with the fewest girders
      for ( GroupIndexType i = 0; i < nGroups; i++ )
      {
         GirderIndexType cGirders = pBridgeDesc->GetGirderGroup(i)->GetGirderCount();
         if ( cGirders < nMaxGirders )
         {
            nMaxGirders = cGirders;
         }
      }
   }
   else
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      nMaxGirders = pGroup->GetGirderCount();
   }

   ATLASSERT(nMaxGirders != 9999);

   for (GirderIndexType gdrIdx = 0; gdrIdx < nMaxGirders; gdrIdx++)
   {
      CString str;
      str.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
      pcbGirders->AddString(str);
   }

    pcbGirders->AddString(_T("All Girders"));
    if ( curSel != CB_ERR )
    {
       if ( bAllSelected )
       {
         pcbGirders->SetCurSel( m_GirderCB.GetCount()-1 );
       }
       else
       {
          if ( pcbGirders->GetCount()-1 == curSel )
          {
             curSel = 0;
          }

         curSel = pcbGirders->SetCurSel( curSel );
       }
    }

    if ( curSel == CB_ERR )
    {
       pcbGirders->SetCurSel(0);
    }
}

void CEditPointLoadDlg::FillEventList()
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

      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

      EventIndexType nEvents = pTimelineMgr->GetEventCount();
      bool bValidEvent = false;
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {
         const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
         if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
         {
            bValidEvent = true;
         }

         if ( bValidEvent )
         {
            CString label;
            label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

            pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
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

EventIndexType CEditPointLoadDlg::CreateEvent()
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

void CEditPointLoadDlg::OnEventChanging()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pcbEvent->GetCurSel();
}

void CEditPointLoadDlg::OnEventChanged()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pcbEvent->GetCurSel();
   EventIndexType idx = (IndexType)pcbEvent->GetItemData(curSel);
   if ( idx == CREATE_TIMELINE_EVENT )
   {
      idx = CreateEvent();
      if ( idx != INVALID_INDEX )
      {
         FillEventList();
         pcbEvent->SetCurSel((int)idx);
      }
      else
      {
         pcbEvent->SetCurSel(m_PrevEventIdx);
      }
   }
}
