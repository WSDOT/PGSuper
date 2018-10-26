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

// ApplyLoadsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ApplyLoadsDlg.h"

#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>

#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>

// CApplyLoadsDlg dialog

IMPLEMENT_DYNAMIC(CApplyLoadsDlg, CDialog)

CApplyLoadsDlg::CApplyLoadsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CApplyLoadsDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
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
   DDX_Control(pDX, IDC_USER_LOADS, m_ctrlUserLoads);

   if ( pDX->m_bSaveAndValidate )
   {
      int value;
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
   }
   else
   {
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
END_MESSAGE_MAP()


// CApplyLoadsDlg message handlers

BOOL CApplyLoadsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   InitUserLoads();

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

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CApplyLoadsDlg::InitUserLoads()
{
   m_ctrlUserLoads.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

   int st;
   st = m_ctrlUserLoads.InsertColumn(0,_T("Type"));
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(1,_T("Load Case"));
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(2,_T("Location"));
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(3,_T("Magnitude"));
   ATLASSERT(st!=-1);
   st = m_ctrlUserLoads.InsertColumn(4,_T("Description"));
   ATLASSERT(st!=-1);

   int rowIdx = 0;
   CApplyLoadActivity& applyLoads = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetApplyLoadActivity();
   IndexType nLoads = applyLoads.GetUserLoadCount();
   for ( IndexType idx = 0; idx < nLoads; idx++, rowIdx++ )
   {
      LoadIDType loadID = applyLoads.GetUserLoadID(idx);

      UserLoads::UserLoadType loadType = UserLoads::GetUserLoadTypeFromID(loadID);
      switch(loadType)
      {
      case UserLoads::Distributed:
         AddDistributedLoad(rowIdx,loadID);
         break;

      case UserLoads::Point:
         AddPointLoad(rowIdx,loadID);
         break;

      case UserLoads::Moment:
         AddMomentLoad(rowIdx,loadID);
         break;

      default:
         ATLASSERT(false); // should never get here
      }
   }

   int nItems = m_ctrlUserLoads.GetItemCount();
   int nCol = m_ctrlUserLoads.GetHeaderCtrl()->GetItemCount();
   if ( nItems == 0 )
   {
      for ( int i = 0; i < nCol; i++ )
      {
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE_USEHEADER);
      }
   }
   else
   {
      for ( int i = 0; i < nCol; i++ )
      {
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE_USEHEADER);
         int cx1 = m_ctrlUserLoads.GetColumnWidth(i);
         m_ctrlUserLoads.SetColumnWidth(i,LVSCW_AUTOSIZE);
         int cx2 = m_ctrlUserLoads.GetColumnWidth(i);
         m_ctrlUserLoads.SetColumnWidth(i,Max(cx1,cx2));
      }
   }
}

void CApplyLoadsDlg::AddDistributedLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData* pLoadData = pUserDefinedLoads->FindDistributedLoad(loadID);
   ATLASSERT(pLoadData != NULL);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Distributed"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if (pLoadData->m_Type == UserLoads::Uniform)
   {
      m_ctrlUserLoads.SetItemText(rowIdx, 0, _T("Uniform"));
   }
   else
   {
      m_ctrlUserLoads.SetItemText(rowIdx, 0, _T("Trapezoidal"));
   }

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(pLoadData->m_LoadCase).c_str());

   CString strSpan;
   if ( pLoadData->m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      strSpan.Format(_T("Span %d"),LABEL_SPAN(pLoadData->m_SpanKey.spanIndex));
   }

   CString strGirder;
   if ( pLoadData->m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(pLoadData->m_SpanKey.girderIndex));
   }

   CString strLocation;
   if ( pLoadData->m_Type == UserLoads::Uniform )
   {
      strLocation.Format(_T("%s"),_T("Entire Span"));
   }
   else
   {
      if (pLoadData->m_Fractional)
      {
         strLocation.Format(_T("%s - %s"),FormatPercentage(pLoadData->m_StartLocation,false),FormatPercentage(pLoadData->m_EndLocation));
      }
      else
      {
         strLocation.Format(_T("%s - %s"),FormatDimension(pLoadData->m_StartLocation,pDisplayUnits->GetSpanLengthUnit(),false),FormatDimension(pLoadData->m_EndLocation,pDisplayUnits->GetSpanLengthUnit()));
      }
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   if (pLoadData->m_Type == UserLoads::Uniform)
   {
      strMagnitude.Format(_T("%s"),FormatDimension(pLoadData->m_WStart,pDisplayUnits->GetForcePerLengthUnit()));
   }
   else
   {
      strMagnitude.Format(_T("%s - %s"),FormatDimension(pLoadData->m_WStart,pDisplayUnits->GetForcePerLengthUnit(),false),FormatDimension(pLoadData->m_WEnd,pDisplayUnits->GetForcePerLengthUnit()));
   }

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, pLoadData->m_Description.c_str());
}

void CApplyLoadsDlg::AddPointLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData* pLoadData = pUserDefinedLoads->FindPointLoad(loadID);
   ATLASSERT(pLoadData != NULL);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Point"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(pLoadData->m_LoadCase).c_str());

   CString strSpan;
   if ( pLoadData->m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      strSpan.Format(_T("Span %d"),LABEL_SPAN(pLoadData->m_SpanKey.spanIndex));
   }

   CString strGirder;
   if ( pLoadData->m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(pLoadData->m_SpanKey.girderIndex));
   }

   CString strLocation;
   if (pLoadData->m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(pLoadData->m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(pLoadData->m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(pLoadData->m_Magnitude,pDisplayUnits->GetGeneralForceUnit()));

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, pLoadData->m_Description.c_str());
}

void CApplyLoadsDlg::AddMomentLoad(int rowIdx,LoadIDType loadID)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData* pLoadData = pUserDefinedLoads->FindMomentLoad(loadID);
   ATLASSERT(pLoadData != NULL);

   m_ctrlUserLoads.InsertItem(rowIdx, _T("Moment"));

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   m_ctrlUserLoads.SetItemText(rowIdx, 1, UserLoads::GetLoadCaseName(pLoadData->m_LoadCase).c_str());

   CString strSpan;
   if ( pLoadData->m_SpanKey.spanIndex == ALL_SPANS )
   {
      strSpan.Format(_T("%s"),_T("All Spans"));
   }
   else
   {
      strSpan.Format(_T("Span %d"),LABEL_SPAN(pLoadData->m_SpanKey.spanIndex));
   }

   CString strGirder;
   if ( pLoadData->m_SpanKey.girderIndex == ALL_GIRDERS )
   {
      strGirder.Format(_T("%s"),_T("All Girders"));
   }
   else
   {
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(pLoadData->m_SpanKey.girderIndex));
   }

   CString strLocation;
   if (pLoadData->m_Fractional)
   {
      strLocation.Format(_T("%s"),FormatPercentage(pLoadData->m_Location));
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(pLoadData->m_Location,pDisplayUnits->GetSpanLengthUnit()));
   }

   CString strLabel;
   strLabel.Format(_T("%s, %s, %s"),strSpan,strGirder,strLocation);

   m_ctrlUserLoads.SetItemText(rowIdx,2,strLabel);

   CString strMagnitude;
   strMagnitude.Format(_T("%s"),FormatDimension(pLoadData->m_Magnitude,pDisplayUnits->GetMomentUnit()));

   m_ctrlUserLoads.SetItemText(rowIdx, 3, strMagnitude);
   m_ctrlUserLoads.SetItemText(rowIdx, 4, pLoadData->m_Description.c_str());
}
