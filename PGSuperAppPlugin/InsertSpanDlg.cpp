///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// InsertSpanDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "InsertSpanDlg.h"
#include "TimelineEventDlg.h"
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInsertSpanDlg dialog


CInsertSpanDlg::CInsertSpanDlg(const CBridgeDescription2* pBridgeDesc,CWnd* pParent /*=nullptr*/)
	: CDialog(CInsertSpanDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInsertSpanDlg)
	//}}AFX_DATA_INIT
   m_RefPierIdx = INVALID_INDEX;
   m_SpanLength = ::ConvertToSysUnits(100.0,unitMeasure::Feet);
   m_bCreateNewGroup = false;
   m_pBridgeDesc = pBridgeDesc;
   m_EventIndex = 0;
}


void CInsertSpanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertSpanDlg)
	//}}AFX_DATA_MAP

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueGreaterThanZero(pDX,IDC_SPAN_LENGTH,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());

   DDX_CBIndex(pDX,IDC_LOCATION,m_LocationIdx);
   DDX_Check_Bool(pDX,IDC_NEW_GROUP,m_bCreateNewGroup);
   DDX_CBItemData(pDX,IDC_EVENT,m_EventIndex);

   if ( pDX->m_bSaveAndValidate )
   {
      m_RefPierIdx = m_Keys[m_LocationIdx].first;
      m_PierFace   = m_Keys[m_LocationIdx].second;
   }
}


BEGIN_MESSAGE_MAP(CInsertSpanDlg, CDialog)
	//{{AFX_MSG_MAP(CInsertSpanDlg)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_LOCATION,&CInsertSpanDlg::OnPierChanged)
   ON_CBN_SELCHANGE(IDC_EVENT, &CInsertSpanDlg::OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, &CInsertSpanDlg::OnEventChanging)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_NEW_GROUP, &CInsertSpanDlg::OnBnClickedNewGroup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDlg message handlers

BOOL CInsertSpanDlg::OnInitDialog() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pcbPiers = (CComboBox*)GetDlgItem(IDC_LOCATION);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);

      if ( !pPier->HasCantilever() )
      {
         CString strType(pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));

         CString strItem;
         strItem.Format(_T("Before %s %d\n"),strType,LABEL_PIER(pierIdx));
         m_Keys.emplace_back(pierIdx,pgsTypes::Back);
         pcbPiers->AddString(strItem);

         strItem.Format(_T("After %s %d\n"),strType,LABEL_PIER(pierIdx));
         m_Keys.emplace_back(pierIdx,pgsTypes::Ahead);
         pcbPiers->AddString(strItem);
      }
   }

   // Use the current selection to guide the defaults
   GET_IFACE2(pBroker,ISelection,pSelection);
   PierIndexType selectedPierIdx = pSelection->GetSelectedPier();
   SpanIndexType selectedSpanIdx = pSelection->GetSelectedSpan();

   if ( selectedPierIdx != INVALID_INDEX )
   {
      m_RefPierIdx = selectedPierIdx;
   }
   else if ( selectedSpanIdx != INVALID_INDEX )
   {
      m_RefPierIdx = (PierIndexType)selectedSpanIdx;
   }
   else
   {
      m_RefPierIdx = INVALID_INDEX;
   }

   if ( m_RefPierIdx == INVALID_INDEX )
   {
      m_LocationIdx = pcbPiers->GetCount()-1;
   }
   else
   {
      m_LocationIdx = (int)(2*m_RefPierIdx+1);
   }

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      m_bCreateNewGroup = true;
      GetDlgItem(IDC_NEW_GROUP)->ShowWindow(SW_HIDE);

      m_EventIndex = 0;
      GetDlgItem(IDC_EVENT_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EVENT)->ShowWindow(SW_HIDE);
   }

   FillEventList();

   // Use adjacent span as the default span length
   const CSpanData2* pSpan;
   if ( selectedSpanIdx != INVALID_INDEX )
   {
      pSpan = m_pBridgeDesc->GetSpan(selectedSpanIdx);
   }
   else
   {
      if ( m_RefPierIdx == INVALID_INDEX )
      {
         pSpan = m_pBridgeDesc->GetSpan(m_pBridgeDesc->GetSpanCount()-1);
      }
      else
      {
         pSpan = m_pBridgeDesc->GetSpan(m_RefPierIdx-1);
         if ( pSpan == nullptr )
         {
            pSpan = m_pBridgeDesc->GetSpan(m_RefPierIdx);
         }
      }
   }
   m_SpanLength = pSpan->GetSpanLength();

   CDialog::OnInitDialog();

   OnPierChanged();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertSpanDlg::OnHelp() 
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_INSERT_SPAN);
}

void CInsertSpanDlg::OnPierChanged()
{
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      // If this is a spliced girder project, a new group can only be created if
      // the pier where the span is going to be inserted is on the boundary of a group

      CComboBox* pLocation = (CComboBox*)GetDlgItem(IDC_LOCATION);
      int location = pLocation->GetCurSel();
      PierIndexType pierIdx = m_Keys[location].first;
      pgsTypes::PierFaceType pierFace = m_Keys[location].second;

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);

      const CGirderGroupData* pPrevGroup = nullptr;
      const CGirderGroupData* pNextGroup = nullptr;

      if ( pPier->GetPrevSpan() )
         pPrevGroup = m_pBridgeDesc->GetGirderGroup(pPier->GetPrevSpan());

      if ( pPier->GetNextSpan() )
         pNextGroup = m_pBridgeDesc->GetGirderGroup(pPier->GetNextSpan());

      ATLASSERT(pPrevGroup != nullptr || pNextGroup != nullptr);
      CWnd* pGroup = GetDlgItem(IDC_NEW_GROUP);
      if ( pPrevGroup != pNextGroup )
      {
         // on boundary of group
         pGroup->EnableWindow();
         CheckDlgButton(IDC_NEW_GROUP,m_bCreateNewGroup ? BST_CHECKED : BST_UNCHECKED);
      }
      else
      {
         // not on boundary of group
         pGroup->EnableWindow(FALSE);
         CheckDlgButton(IDC_NEW_GROUP,BST_UNCHECKED);
      }
   }
}

void CInsertSpanDlg::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CInsertSpanDlg::OnEventChanged()
{
#pragma Reminder("UPDATE: this dialog needs to work on a local bridge model... and use the local timeline manager")
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   if ( pCB->GetItemData(curSel) == CREATE_TIMELINE_EVENT )
   {
      EventIndexType eventIdx = CreateEvent();
      
      if ( eventIdx != INVALID_INDEX )
      {
         m_EventIndex = eventIdx;
         FillEventList();
         pCB->SetCurSel((int)eventIdx);
      }
      else
      {
         pCB->SetCurSel(m_PrevEventIdx);
      }
   }
}

void CInsertSpanDlg::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int eventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

#pragma Reminder("UPDATE: this dialog needs to work on a local bridge model... and use the local timeline manager")
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

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( eventIdx != CB_ERR )
   {
      pcbEvent->SetCurSel(eventIdx);
   }
   else
   {
      pcbEvent->SetCurSel(0);
   }
}

EventIndexType CInsertSpanDlg::CreateEvent()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CTimelineEventDlg dlg(*pTimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      return pIBridgeDesc->AddTimelineEvent(*dlg.m_pTimelineEvent);
  }

   return INVALID_INDEX;
}

void CInsertSpanDlg::OnBnClickedNewGroup()
{
   m_bCreateNewGroup = IsDlgButtonChecked(IDC_NEW_GROUP) == BST_CHECKED;
}
