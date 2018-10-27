///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
// EditDistributedLoadDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditDistributedLoadDlg.h"

#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include <System\Tokenizer.h>

#include <PgsExt\BridgeDescription2.h>
#include "PGSuperAppPlugin\TimelineEventDlg.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditDistributedLoadDlg dialog


CEditDistributedLoadDlg::CEditDistributedLoadDlg(const CDistributedLoadData& load,const CTimelineManager* pTimelineMgr,CWnd* pParent /*=nullptr*/)
	: CDialog(CEditDistributedLoadDlg::IDD, pParent),
   m_Load(load),
   m_TimelineMgr(*pTimelineMgr)
{
	//{{AFX_DATA_INIT(CEditDistributedLoadDlg)
	//}}AFX_DATA_INIT
   EAFGetBroker(&m_pBroker);

   m_EventID = m_TimelineMgr.FindUserLoadEventID(m_Load.m_ID);

   m_bWasNewEventCreated = false;
}


void CEditDistributedLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditDistributedLoadDlg)
	DDX_Control(pDX, IDC_SPAN_LENGTH_CTRL, m_SpanLengthCtrl);
	DDX_Control(pDX, IDC_LOCATION_UNITS, m_LocationUnitCtrl);
	DDX_Control(pDX, IDC_RIGHT_LOCATION, m_RightLocationCtrl);
	DDX_Control(pDX, IDC_LEFT_LOCATION, m_LeftLocationCtrl);
	DDX_Control(pDX, IDC_LOADTYPE, m_LoadTypeCB);
	DDX_Control(pDX, IDC_FRACTIONAL, m_FractionalCtrl);
	DDX_Control(pDX, IDC_GIRDERS, m_GirderCB);
	DDX_Control(pDX, IDC_SPANS, m_SpanCB);
	//}}AFX_DATA_MAP
	DDX_String(pDX, IDC_DESCRIPTION, m_Load.m_Description);

   // magnitude is easy part
   DDX_UnitValueAndTag( pDX, IDC_LEFT_MAGNITUDE, IDC_MAGNITUDE_UNITS,  m_Load.m_WStart, pDisplayUnits->GetForcePerLengthUnit() );
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_MAGNITUDE,IDC_MAGNITUDE_UNITS2, m_Load.m_WEnd,   pDisplayUnits->GetForcePerLengthUnit() );

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

      m_Load.m_SpanKey.spanIndex   = spanIdx;
      m_Load.m_SpanKey.girderIndex = gdrIdx;

      // first check if load is uniform or trapezoidal (much more work for trapezoidal)
      m_Load.m_Type = UserLoads::GetDistributedLoadType(m_LoadTypeCB.GetCurSel());

      if (m_Load.m_Type != UserLoads::Uniform)
      {
         // location takes some effort
         Float64 lft_locval, rgt_locval;
         CString str;
         m_LeftLocationCtrl.GetWindowText(str);
         if (!sysTokenizer::ParseDouble(str, &lft_locval))
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
            ::AfxMessageBox(_T("Please enter a number"));
            pDX->Fail();
         }

         m_RightLocationCtrl.GetWindowText(str);
         if (!sysTokenizer::ParseDouble(str, &rgt_locval))
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
            ::AfxMessageBox(_T("Please enter a number"));
            pDX->Fail();
         }

         if (lft_locval >= rgt_locval)
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
            ::AfxMessageBox(_T("Invalid Value: The left location value must be greater than the right location value"));
            pDX->Fail();
         }

         m_Load.m_Fractional = m_FractionalCtrl.GetCheck()!=FALSE;

         if (m_Load.m_Fractional)
         {
            if (0.0 <= lft_locval && lft_locval <= 1.0)
            {
               m_Load.m_StartLocation = lft_locval;
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Fractional values must range from 0.0 to 1.0"));
               pDX->Fail();
            }

            if (0.0 <= rgt_locval && rgt_locval <= 1.0)
            {
               m_Load.m_EndLocation = rgt_locval;
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Fractional values must range from 0.0 to 1.0"));
               pDX->Fail();
            }
         }
         else
         {
            if (0.0 <= lft_locval)
            {
               m_Load.m_StartLocation = ::ConvertToSysUnits(lft_locval, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure );
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Location values must be zero or greater"));
               pDX->Fail();
            }

            if (0.0 <= rgt_locval)
            {
               m_Load.m_EndLocation = ::ConvertToSysUnits(rgt_locval, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Location values must be zero or greater"));
               pDX->Fail();
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CEditDistributedLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CEditDistributedLoadDlg)
	ON_BN_CLICKED(IDC_FRACTIONAL, OnFractional)
	ON_CBN_SELCHANGE(IDC_LOADCASE, OnEditchangeLoadcase)
	ON_CBN_SELCHANGE(IDC_LOADTYPE, OnEditchangeLoadtype)
	ON_CBN_SELCHANGE(IDC_SPANS, OnEditchangeSpans)
	ON_CBN_SELCHANGE(IDC_GIRDERS, OnEditchangeGirders)
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditDistributedLoadDlg message handlers

BOOL CEditDistributedLoadDlg::OnInitDialog() 
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

   //UpdateEventLoadCase(true);

   // load types
   for (Int32 ilt=0; ilt<UserLoads::GetNumDistributedLoadTypes(); ilt++)
   {
      CString str(UserLoads::GetDistributedLoadTypeName(ilt).c_str());
      m_LoadTypeCB.AddString(str);
   }

   m_LoadTypeCB.SetCurSel(m_Load.m_Type);

   // spans, girders
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

    if (m_Load.m_SpanKey.spanIndex == ALL_SPANS)
    {
       m_SpanCB.SetCurSel((int)nSpans);
    }
    else
    {
      if ( 0 <= m_Load.m_SpanKey.spanIndex && m_Load.m_SpanKey.spanIndex < nSpans)
      {
         m_SpanCB.SetCurSel((int)m_Load.m_SpanKey.spanIndex);
      }
      else
      {
         ::AfxMessageBox(_T("Warning - The span for this load is out of range. Resetting to Span 1"));

         m_Load.m_SpanKey.spanIndex = 0;
         m_SpanCB.SetCurSel((int)m_Load.m_SpanKey.spanIndex);
      }
    }

   UpdateGirderList();


    if (m_Load.m_SpanKey.girderIndex == ALL_GIRDERS)
    {
       m_GirderCB.SetCurSel( m_GirderCB.GetCount()-1 );
    }
    else
    {
      if (0 <= m_Load.m_SpanKey.girderIndex && m_Load.m_SpanKey.girderIndex < GirderIndexType(m_GirderCB.GetCount()-1) )
      {
         m_GirderCB.SetCurSel((int)m_Load.m_SpanKey.girderIndex);
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
   
   m_FractionalCtrl.SetCheck(m_Load.m_Fractional);

   if (m_Load.m_Fractional)
   {
      sysNumericFormatTool tool;
      m_LeftLocationCtrl.SetWindowText(tool.AsString(m_Load.m_StartLocation).c_str());
      m_RightLocationCtrl.SetWindowText(tool.AsString(m_Load.m_EndLocation).c_str());
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      CString strLeftLoad;
      strLeftLoad.Format(_T("%s"),FormatDimension(m_Load.m_StartLocation,pDisplayUnits->GetSpanLengthUnit(),false));
      m_LeftLocationCtrl.SetWindowText(strLeftLoad);

      CString strRightLoad;
      strRightLoad.Format(_T("%s"),FormatDimension(m_Load.m_EndLocation,pDisplayUnits->GetSpanLengthUnit(),false));
      m_RightLocationCtrl.SetWindowText(strRightLoad);
   }

   UpdateLocationUnit();
   UpdateLoadType();
   UpdateSpanLength();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_DISTRIBUTED_LOAD),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditDistributedLoadDlg::UpdateLocationUnit()
{
	int chk = m_FractionalCtrl.GetCheck();

   if (chk)
   {
      m_LocationUnitCtrl.SetWindowText(_T("[0.0 - 1.0]"));
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnit);
      m_LocationUnitCtrl.SetWindowText(pDisplayUnit->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   }
}

void CEditDistributedLoadDlg::UpdateEventLoadCase(bool isInitial)
{
   CComboBox* pcbLoadCase = (CComboBox*)GetDlgItem(IDC_LOADCASE);
   CComboBox* pcbEvent    = (CComboBox*)GetDlgItem(IDC_EVENT);

   EventIndexType castDiaphragmEventIdx = m_TimelineMgr.GetIntermediateDiaphragmsLoadEventIndex();
   EventIndexType castLongitudinalJointEventIdx = m_TimelineMgr.GetCastLongitudinalJointEventIndex();
   EventIndexType castDeckEventIdx      = m_TimelineMgr.GetCastDeckEventIndex();
   EventIndexType noncompositeLoadEventIdx;
   if (castDeckEventIdx == INVALID_INDEX)
   {
      if (castLongitudinalJointEventIdx == INVALID_INDEX)
      {
         noncompositeLoadEventIdx = castDiaphragmEventIdx;
      }
      else
      {
         noncompositeLoadEventIdx = castLongitudinalJointEventIdx;
      }
   }
   else
   {
      noncompositeLoadEventIdx = castDeckEventIdx;
   }

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
         const CTimelineEvent* pTimelineEvent = m_TimelineMgr.GetEventByIndex(noncompositeLoadEventIdx);
         CString strEvent;
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(noncompositeLoadEventIdx),pTimelineEvent->GetDescription());
         int idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(pTimelineEvent->GetID()));

         pTimelineEvent = m_TimelineMgr.GetEventByIndex(railingSystemEventIdx);
         strEvent.Format(_T("Event %d: %s"),LABEL_EVENT(railingSystemEventIdx),pTimelineEvent->GetDescription());
         idx = pcbEvent->AddString(strEvent);
         pcbEvent->SetItemData(idx,DWORD_PTR(pTimelineEvent->GetID()));

         pcbEvent->EnableWindow(TRUE);

         if (isInitial)
         {
            EventIDType noncompositeLoadEventID      = m_TimelineMgr.GetEventByIndex(noncompositeLoadEventIdx)->GetID();
            EventIDType railingSystemEventID = m_TimelineMgr.GetRailingSystemLoadEventID();
            EventIDType liveLoadEventID      = m_TimelineMgr.GetLiveLoadEventID();
            if ( m_EventID == noncompositeLoadEventID)
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
               m_EventID = noncompositeLoadEventID;
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

void CEditDistributedLoadDlg::OnFractional() 
{
   UpdateLocationUnit();
}

void CEditDistributedLoadDlg::OnEditchangeLoadcase() 
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      UpdateEventLoadCase();
   }
}

void CEditDistributedLoadDlg::OnEditchangeLoadtype() 
{
   UpdateLoadType();
}

void CEditDistributedLoadDlg::UpdateLoadType()
{
   int sel = m_LoadTypeCB.GetCurSel();

   // need to hide a bunch of stuff if uniform
   int cmdshw = (UserLoads::GetDistributedLoadType(sel) == UserLoads::Uniform)? SW_HIDE : SW_SHOW;

   CWnd* pst = this->GetDlgItem(IDC_LEFT_ENDSTATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_END_STATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_MAGNITUDE);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_MAGNITUDE);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_MAGNITUDE_UNITS);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LOCATION_STATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LEFT_LOCATION);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_LOCATION);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LOCATION_UNITS);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_FRACTIONAL);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LOAD_LOCATION_NOTE);
   pst->ShowWindow(cmdshw);

   // units for uniform
   int cmdswt = cmdshw==SW_HIDE? SW_SHOW:SW_HIDE;
   pst = this->GetDlgItem(IDC_MAGNITUDE_UNITS2);
   pst->ShowWindow(cmdswt);
}


void CEditDistributedLoadDlg::OnEditchangeSpans() 
{
   UpdateGirderList();
   UpdateSpanLength();
}

void CEditDistributedLoadDlg::OnEditchangeGirders() 
{
   UpdateSpanLength();
}

void CEditDistributedLoadDlg::UpdateSpanLength() 
{
	int spanIdx = m_SpanCB.GetCurSel();
	int gdrIdx  = m_GirderCB.GetCurSel();

   if (spanIdx == m_SpanCB.GetCount()-1 || gdrIdx == m_GirderCB.GetCount()-1)
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

void CEditDistributedLoadDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_DISTRIBUTED_LOADS );
}

void CEditDistributedLoadDlg::UpdateGirderList()
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

void CEditDistributedLoadDlg::FillEventList()
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

EventIndexType CEditDistributedLoadDlg::CreateEvent()
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

void CEditDistributedLoadDlg::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CEditDistributedLoadDlg::OnEventChanged()
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