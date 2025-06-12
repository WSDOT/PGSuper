///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// ApplyLoadsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ApplyLoadsDlg.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\DeckDescription2.h>



// CApplyLoadsDlg dialog

IMPLEMENT_DYNAMIC(CApplyLoadsDlg, CDialog)

CApplyLoadsDlg::CApplyLoadsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CApplyLoadsDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;
}

CApplyLoadsDlg::~CApplyLoadsDlg()
{
}

void CApplyLoadsDlg::InitalizeCheckBox(CDataExchange* pDX,EventIndexType eventIdx,UINT nIDC)
{
   int value;
   if ( eventIdx == m_EventIndex )
   {
      value = BST_CHECKED;
   }
   else if ( eventIdx == INVALID_INDEX )
   {
      value = BST_UNCHECKED;
   }
   else
   {
      CButton* pCheckBox = (CButton*)GetDlgItem(nIDC);
      pCheckBox->SetButtonStyle(BS_AUTO3STATE);
      value = BST_INDETERMINATE;
   }
   DDX_Check(pDX,nIDC,value);
}

void CApplyLoadsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_SOURCE_LIST,m_lbSource);
   DDX_Control(pDX,IDC_TARGET_LIST,m_lbTarget);

   if ( pDX->m_bSaveAndValidate )
   {
      int value;
      DDX_Check(pDX, IDC_INTERMEDIATE_DIAPHRAGMS, value);
      if (value == BST_CHECKED)
      {
         m_TimelineMgr.SetIntermediateDiaphragmsLoadEventByIndex(m_EventIndex);
      }
      else if (value == BST_UNCHECKED)
      {
         m_TimelineMgr.SetIntermediateDiaphragmsLoadEventByIndex(INVALID_INDEX);
      }

      DDX_Check(pDX,IDC_RAILING_SYSTEM,value);
      if ( value == BST_CHECKED )
      {
         m_TimelineMgr.SetRailingSystemLoadEventByIndex(m_EventIndex);
      }
      else if ( value == BST_UNCHECKED )
      {
         m_TimelineMgr.SetRailingSystemLoadEventByIndex(INVALID_INDEX);
      }

      DDX_Check(pDX,IDC_OVERLAY,value);
      if ( value == BST_CHECKED )
      {
         m_TimelineMgr.SetOverlayLoadEventByIndex(m_EventIndex);
      }
      else if ( value == BST_UNCHECKED )
      {
         m_TimelineMgr.SetOverlayLoadEventByIndex(INVALID_INDEX);
      }

      DDX_Check(pDX,IDC_LIVELOAD,value);
      if ( value == BST_CHECKED )
      {
         m_TimelineMgr.SetLiveLoadEventByIndex(m_EventIndex);
      }
      else if ( value == BST_UNCHECKED )
      {
         m_TimelineMgr.SetLiveLoadEventByIndex(INVALID_INDEX);
      }

      DDX_Check(pDX,IDC_LOAD_RATING,value);
      if ( value == BST_CHECKED )
      {
         m_TimelineMgr.SetLoadRatingEventByIndex(m_EventIndex);
      }
      else if ( value == BST_UNCHECKED )
      {
         m_TimelineMgr.SetLoadRatingEventByIndex(INVALID_INDEX);
      }

      int nItems = m_lbTarget.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbTarget.GetItemDataPtr(itemIdx);
         LoadIDType loadID = pItemData->m_ID;

         m_TimelineMgr.SetUserLoadEventByIndex(loadID,m_EventIndex);
      }

      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            continue;
         }

         LoadIDType loadID = pItemData->m_ID;

         m_TimelineMgr.SetUserLoadEventByIndex(loadID,INVALID_INDEX);
      }
   }
   else
   {
      EventIndexType intermediateDiaphragmsEventIdx = m_TimelineMgr.GetIntermediateDiaphragmsLoadEventIndex();
      InitalizeCheckBox(pDX, intermediateDiaphragmsEventIdx, IDC_INTERMEDIATE_DIAPHRAGMS);

      EventIndexType railingSystemEventIdx = m_TimelineMgr.GetRailingSystemLoadEventIndex();
      InitalizeCheckBox(pDX,railingSystemEventIdx,IDC_RAILING_SYSTEM);

      EventIndexType overlayEventIdx = m_TimelineMgr.GetOverlayLoadEventIndex();
      InitalizeCheckBox(pDX,overlayEventIdx,IDC_OVERLAY);

      EventIndexType liveLoadEventIdx = m_TimelineMgr.GetLiveLoadEventIndex();
      InitalizeCheckBox(pDX,liveLoadEventIdx,IDC_LIVELOAD);

      EventIndexType loadRatingEventIdx = m_TimelineMgr.GetLoadRatingEventIndex();
      InitalizeCheckBox(pDX,loadRatingEventIdx,IDC_LOAD_RATING);
   }
}


BEGIN_MESSAGE_MAP(CApplyLoadsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CApplyLoadsDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CApplyLoadsDlg::OnMoveToSourceList)
   ON_BN_CLICKED(ID_HELP, &CApplyLoadsDlg::OnHelp)
END_MESSAGE_MAP()


// CApplyLoadsDlg message handlers

BOOL CApplyLoadsDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::ID,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::ID,&m_lbSource);

   CDialog::OnInitDialog();

   FillLists();

   CString strNote(_T(""));
   const CDeckDescription2* pDeck = m_TimelineMgr.GetBridgeDescription()->GetDeckDescription();
   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth )
   {
      GetDlgItem(IDC_OVERLAY)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY_NOTE)->EnableWindow(FALSE);
      strNote = _T("(Bridge does not have an overlay)");
   }
   CWnd* pWnd = GetDlgItem(IDC_OVERLAY_NOTE);
   pWnd->SetWindowText(strNote);


   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_INTERMEDIATE_DIAPHRAGMS)->EnableWindow(FALSE);
      GetDlgItem(IDC_RAILING_SYSTEM)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY)->EnableWindow(FALSE);
      GetDlgItem(IDC_OVERLAY_NOTE)->EnableWindow(FALSE);
      GetDlgItem(IDC_LIVELOAD)->EnableWindow(FALSE);
      GetDlgItem(IDC_LOAD_RATING)->EnableWindow(FALSE);
      GetDlgItem(IDC_SOURCE_LIST)->EnableWindow(FALSE);
      GetDlgItem(IDC_TARGET_LIST)->EnableWindow(FALSE);
      GetDlgItem(IDC_MOVE_LEFT)->EnableWindow(FALSE);
      GetDlgItem(IDC_MOVE_RIGHT)->EnableWindow(FALSE);

      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CApplyLoadsDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CApplyLoadsDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CApplyLoadsDlg::FillLists()
{
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   IndexType nLoads = pUserDefinedLoads->GetPointLoadCount();
   for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CPointLoadData* pLoad = pUserDefinedLoads->GetPointLoad(loadIdx);
      EventIndexType loadEventIdx = this->m_TimelineMgr.FindUserLoadEventIndex(pLoad->m_ID);

      CString strLoad = GetLoadDescription(pLoad);
      if ( loadEventIdx == m_EventIndex )
      {
         // load applied during this event
         m_lbTarget.AddItem(strLoad,CTimelineItemDataPtr::Used,pLoad->m_ID);
      }
      else
      {
         // load not applied during this event
         CTimelineItemDataPtr::State state = (loadEventIdx == INVALID_INDEX ? CTimelineItemDataPtr::Unused : CTimelineItemDataPtr::Used);
         m_lbSource.AddItem(strLoad,state,pLoad->m_ID);
      }
   }

   nLoads = pUserDefinedLoads->GetDistributedLoadCount();
   for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CDistributedLoadData* pLoad = pUserDefinedLoads->GetDistributedLoad(loadIdx);
      EventIndexType loadEventIdx = this->m_TimelineMgr.FindUserLoadEventIndex(pLoad->m_ID);

      CString strLoad = GetLoadDescription(pLoad);
      if ( loadEventIdx == m_EventIndex )
      {
         // load applied during this event
         m_lbTarget.AddItem(strLoad,CTimelineItemDataPtr::Used,pLoad->m_ID);
      }
      else
      {
         // load not applied during this event
         CTimelineItemDataPtr::State state = (loadEventIdx == INVALID_INDEX ? CTimelineItemDataPtr::Unused : CTimelineItemDataPtr::Used);
         m_lbSource.AddItem(strLoad,state,pLoad->m_ID);
      }
   }

   nLoads = pUserDefinedLoads->GetMomentLoadCount();
   for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CMomentLoadData* pLoad = pUserDefinedLoads->GetMomentLoad(loadIdx);
      EventIndexType loadEventIdx = this->m_TimelineMgr.FindUserLoadEventIndex(pLoad->m_ID);

      CString strLoad = GetLoadDescription(pLoad);
      if ( loadEventIdx == m_EventIndex )
      {
         // load applied during this event
         m_lbTarget.AddItem(strLoad,CTimelineItemDataPtr::Used,pLoad->m_ID);
      }
      else
      {
         // load not applied during this event
         CTimelineItemDataPtr::State state = (loadEventIdx == INVALID_INDEX ? CTimelineItemDataPtr::Unused : CTimelineItemDataPtr::Used);
         m_lbSource.AddItem(strLoad,state,pLoad->m_ID);
      }
   }
}

void CApplyLoadsDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_APPLY_LOADS);
}
