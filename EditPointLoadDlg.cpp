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


CEditPointLoadDlg::CEditPointLoadDlg(const CPointLoadData& load,CWnd* pParent /*=NULL*/):
	CDialog(CEditPointLoadDlg::IDD, pParent),
   m_Load(load)
{
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

   DDX_CBItemData(pDX,IDC_EVENT,m_Load.m_EventIdx);

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

      if ( m_Load.m_LoadCase == UserLoads::LL_IM && m_Load.m_EventIdx < liveLoadEventIdx )
      {
         AfxMessageBox(_T("The LL+IM load case can only be used in the events when live load is defined.\n\nChange the Load Case or Event."));
         pDX->PrepareCtrl(IDC_LOADCASE);
         pDX->Fail();
      }

      ival = m_SpanCB.GetCurSel();

      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;

      if (ival == m_SpanCB.GetCount()-1)
         spanIdx = ALL_GROUPS;
      else
         spanIdx = ival;

      ival = m_GirderCB.GetCurSel();
      if (ival == m_GirderCB.GetCount()-1 )
         gdrIdx = ALL_GIRDERS;
      else
         gdrIdx = ival;


      m_Load.m_SpanGirderKey.spanIndex   = spanIdx;
      m_Load.m_SpanGirderKey.girderIndex = gdrIdx;

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
   if ( m_Load.m_EventIdx == INVALID_INDEX )
   {
      CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);
      pcbEvent->SetCurSel(0);
      m_Load.m_EventIdx = (EventIndexType)pcbEvent->GetItemData(0);
   }

	CDialog::OnInitDialog();

   m_WasLiveLoad = m_Load.m_LoadCase == UserLoads::LL_IM;

   // groups, girders
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      CString str;
      str.Format(_T("Span %d"), LABEL_SPAN(spanIdx));
      m_SpanCB.AddString(str);
   }

   m_SpanCB.AddString(_T("All Spans"));

    if (m_Load.m_SpanGirderKey.spanIndex == ALL_SPANS)
    {
       m_SpanCB.SetCurSel((int)nSpans);
    }
    else
    {
      if ( 0 <= m_Load.m_SpanGirderKey.spanIndex && m_Load.m_SpanGirderKey.spanIndex < nSpans)
      {
         m_SpanCB.SetCurSel((int)m_Load.m_SpanGirderKey.spanIndex);
      }
      else
      {
         ::AfxMessageBox(_T("Warning - The span for this load is out of range. Resetting to Span 1"));

         m_Load.m_SpanGirderKey.spanIndex = 0;
         m_SpanCB.SetCurSel((int)m_Load.m_SpanGirderKey.spanIndex);
      }
    }

   UpdateGirderList();

    if (m_Load.m_SpanGirderKey.girderIndex == ALL_GIRDERS)
    {
       m_GirderCB.SetCurSel( m_GirderCB.GetCount()-1 );
    }
    else
    {
      if (0 <= m_Load.m_SpanGirderKey.girderIndex && m_Load.m_SpanGirderKey.girderIndex < GirderIndexType(m_GirderCB.GetCount()-1) )
      {
         m_GirderCB.SetCurSel((int)m_Load.m_SpanGirderKey.girderIndex);
      }
      else
      {
         m_Load.m_SpanGirderKey.girderIndex = 0;

         CString strMsg;
         strMsg.Format(_T("Warning - The Girder for this load is out of range. Resetting to Girder %s"),LABEL_GIRDER(m_Load.m_SpanGirderKey.girderIndex));
         ::AfxMessageBox(strMsg);

         m_GirderCB.SetCurSel((int)m_Load.m_SpanGirderKey.girderIndex);
      }
    }

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
      int idx = pcbEvent->AddString(pTimelineEvent->GetDescription());
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
         int idx = pcbEvent->AddString(pTimelineEvent->GetDescription());
         pcbEvent->SetItemData(idx,DWORD_PTR(castDeckEventIdx));

         pTimelineEvent = pIBridgeDesc->GetEventByIndex(railingSystemEventIdx);
         idx = pcbEvent->AddString(pTimelineEvent->GetDescription());
         pcbEvent->SetItemData(idx,DWORD_PTR(railingSystemEventIdx));

         pcbEvent->EnableWindow(TRUE);

         if (isInitial)
         {
            if ( m_Load.m_EventIdx == castDeckEventIdx )
               pcbEvent->SetCurSel(0);
            else if ( m_Load.m_EventIdx == railingSystemEventIdx )
               pcbEvent->SetCurSel(1);
            else
            {
               pcbEvent->SetCurSel(0);
               m_Load.m_EventIdx = castDeckEventIdx;
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
	int spanIdx = m_SpanCB.GetCurSel();
	int gdrIdx  = m_GirderCB.GetCurSel();

   if (spanIdx == m_SpanCB.GetCount()-1 || spanIdx == m_SpanCB.GetCount()-1)
   {
      CString str(_T("Span Length = N/A"));
      m_SpanLengthCtrl.SetWindowText(str);
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridge, pBridge);
      Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);
      CString str;
      str.Format(_T("Span Length = %s"),FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit()));
      m_SpanLengthCtrl.SetWindowText(str);
   }
}

void CEditPointLoadDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_EDIT_POINT_LOADS );
}

void CEditPointLoadDlg::UpdateGirderList()
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx = (SpanIndexType)m_SpanCB.GetCurSel();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();

   int curSel = m_GirderCB.GetCurSel();
   BOOL bAllSelected = (curSel == m_GirderCB.GetCount()-1);
   m_GirderCB.ResetContent();

   GirderIndexType nMaxGirders = 9999; // this is the maximum number of girders that can be listed
                                       // in the combo box (not the maximum number of girders)
   if ( spanIdx == nSpans )
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
             curSel = 0;

         curSel = m_GirderCB.SetCurSel( curSel );
       }
    }

    if ( curSel == CB_ERR )
       m_GirderCB.SetCurSel(0);
}

void CEditPointLoadDlg::FillEventList()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

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

      if ( selEventIdx != CB_ERR )
      {
         pcbEvent->SetCurSel(selEventIdx);
         m_Load.m_EventIdx = (EventIndexType)pcbEvent->GetItemData(selEventIdx);
      }
   }
}

EventIndexType CEditPointLoadDlg::CreateEvent()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CTimelineEventDlg dlg(pTimelineMgr,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      return pIBridgeDesc->AddTimelineEvent(dlg.m_TimelineEvent);
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