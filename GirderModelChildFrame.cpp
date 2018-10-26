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

// GirderModelChildFrame.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "GirderModelChildFrame.h"
#include "GirderModelSectionView.h"
#include "GirderModelElevationView.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\BridgeDescription2.h>

#include "PGSuperTypes.h"
#include "SectionCutDlg.h"
#include "SectionCutDlgEx.h"
#include "GirderViewPrintJob.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include "htmlhelp\HelpTopics.hh"

#include <PgsExt\InsertDeleteLoad.h>

#include <WBFLDManip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame

IMPLEMENT_DYNCREATE(CGirderModelChildFrame, CSplitChildFrame)

CGirderModelChildFrame::CGirderModelChildFrame():
m_CurrentCutLocation(0),
m_EventIndex(0),
m_GirderKey(ALL_GROUPS,0),
m_bIsAfterFirstUpdate(false)
{
}

CGirderModelChildFrame::~CGirderModelChildFrame()
{
}

BOOL CGirderModelChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CSplitChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_GIRDERMODELEDITOR);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CGirderModelChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CGirderModelChildFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_CBN_SELCHANGE(IDC_SELEVENT, OnSelectEvent)
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMoment)
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SPAN, OnGroupChanged )
   ON_COMMAND(IDC_SECTION_CUT, OnSectionCut )
	ON_BN_CLICKED(IDC_SYNC, OnSync)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HELP, OnCommandHelp)
   ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame message handlers
void CGirderModelChildFrame::SelectGirder(const CGirderKey& girderKey,bool bDoUpdate)
{
   if ( m_GirderKey.groupIndex == INVALID_INDEX && m_GirderKey.girderIndex == INVALID_INDEX )
   {
      m_GirderKey = girderKey;
      UpdateBar();

      if ( bDoUpdate )
      {
   	   UpdateViews();
      }
   }
   else
   {
      ATLASSERT(false);
   }
}

const CGirderKey& CGirderModelChildFrame::GetSelection() const
{
   return m_GirderKey;
}

bool CGirderModelChildFrame::DoSyncWithBridgeModelView()
{
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   return (pBtn->GetCheck() == 0 ? false : true);
}

void CGirderModelChildFrame::RefreshGirderLabeling()
{
   UpdateBar();
}

CRuntimeClass* CGirderModelChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CGirderModelSectionView);
}

Float64 CGirderModelChildFrame::GetTopFrameFraction() const
{
   return 0.4;
}

BOOL CGirderModelChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
   if ( !__super::OnCreateClient(lpcs,pContext) )
   {
      return FALSE;
   }

   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   CDocTemplate* pDocTemplate = pDoc->GetDocTemplate();
   ASSERT( pDocTemplate->IsKindOf(RUNTIME_CLASS(CEAFDocTemplate)) );

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)pDocTemplate;
   CGirderKey girderKey( *(CGirderKey*)pTemplate->GetViewCreationData() );

   if ( girderKey.groupIndex != ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS)
   {
      m_GirderKey = girderKey;
   }

   return TRUE;
}

int CGirderModelChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }
	
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if ( !m_SettingsBar.Create( this, IDD_GIRDER_ELEVATION_BAR, CBRS_TOP, IDD_GIRDER_ELEVATION_BAR) )
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}

   m_SettingsBar.SetBarStyle(m_SettingsBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	
   // point load tool
   CComPtr<iTool> point_load_tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&point_load_tool);
   point_load_tool->SetID(IDC_POINT_LOAD_DRAG);
   point_load_tool->SetToolTipText(_T("Drag me onto girder to create a point load"));

   CComQIPtr<iToolIcon, &IID_iToolIcon> pti(point_load_tool);
   HRESULT hr = pti->SetIcon(::AfxGetInstanceHandle(), IDI_POINT_LOAD);
   ATLASSERT(SUCCEEDED(hr));

   m_SettingsBar.AddTool(point_load_tool);

   // distributed load tool
   CComPtr<iTool> distributed_load_tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&distributed_load_tool);
   distributed_load_tool->SetID(IDC_DISTRIBUTED_LOAD_DRAG);
   distributed_load_tool->SetToolTipText(_T("Drag me onto girder to create a distributed load"));

   CComQIPtr<iToolIcon, &IID_iToolIcon> dti(distributed_load_tool);
   hr = dti->SetIcon(::AfxGetInstanceHandle(), IDI_DISTRIBUTED_LOAD);
   ATLASSERT(SUCCEEDED(hr));

   m_SettingsBar.AddTool(distributed_load_tool);

   // moment load tool
   // Used only with PGSuper (not used for PGSplice)
   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CComPtr<iTool> moment_load_tool;
      ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&moment_load_tool);
      moment_load_tool->SetID(IDC_MOMENT_LOAD_DRAG);
      moment_load_tool->SetToolTipText(_T("Drag me onto girder to create a moment load"));
      
      CComQIPtr<iToolIcon, &IID_iToolIcon> mti(moment_load_tool);
      hr = mti->SetIcon(::AfxGetInstanceHandle(), IDI_MOMENT_LOAD);
      ATLASSERT(SUCCEEDED(hr));

      m_SettingsBar.AddTool(moment_load_tool);
   }
   else
   {
      m_SettingsBar.GetDlgItem(IDC_MOMENT_LOAD_DRAG)->ShowWindow(SW_HIDE);
   }

   // sets the check state of the sync button
   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   pBtn->SetCheck( settings & IDG_SV_SYNC_GIRDER ? TRUE : FALSE);

   if ( DoSyncWithBridgeModelView() ) 
   {
      // Sync only if we can
      CSelection selection = pDoc->GetSelection();
      if ( selection.Type == CSelection::Girder && selection.GroupIdx != ALL_GROUPS && selection.GirderIdx != ALL_GIRDERS)
      {
	      m_GirderKey.groupIndex = selection.GroupIdx;
    	   m_GirderKey.girderIndex = selection.GirderIdx;
      }
   }

   UpdateMaxCutLocation();
   m_CurrentCutLocation = m_MaxCutLocation/2;

   FillEventComboBox();
   UpdateBar();

	return 0;
}

void CGirderModelChildFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_GIRDERLABELFORMATCHANGED )
   {
      RefreshGirderLabeling();
   }
   else if ( lHint == HINT_SELECTIONCHANGED )
   {
      CSelection* pSelection = (CSelection*)pHint;
      if( (pSelection->Type == CSelection::Girder || pSelection->Type == CSelection::Segment) && DoSyncWithBridgeModelView() )
      {
         if ( m_GirderKey.groupIndex != pSelection->GroupIdx || m_GirderKey.girderIndex != pSelection->GirderIdx )
         {
            if (0 <= pSelection->GroupIdx || 0 <= pSelection->GirderIdx )
            {
               CGirderKey girderKey(pSelection->GroupIdx,pSelection->GirderIdx);
               SelectGirder(girderKey,false);
            }
         }
      }
   }
   else if ( lHint == HINT_BRIDGECHANGED || lHint == HINT_UNITSCHANGED )
   {
      UpdateBar();
   }

   m_bIsAfterFirstUpdate = true;
}


CGirderModelElevationView* CGirderModelChildFrame::GetGirderModelElevationView() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane does the "AssertValid" thing, the module state has to be correct
   CWnd* pwnd = m_SplitterWnd.GetPane(0, 0);
   CGirderModelElevationView* pvw = dynamic_cast<CGirderModelElevationView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

CGirderModelSectionView* CGirderModelChildFrame::GetGirderModelSectionView() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane does the "AssertValid" thing, the module state has to be correct
   CWnd* pwnd = m_SplitterWnd.GetPane(1, 0);
   CGirderModelSectionView* pvw = dynamic_cast<CGirderModelSectionView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

void CGirderModelChildFrame::UpdateViews()
{
   GetGirderModelElevationView()->OnUpdate(NULL,0,NULL);
   GetGirderModelSectionView()->OnUpdate(NULL,0,NULL);
}

void CGirderModelChildFrame::UpdateCutLocation(const pgsPointOfInterest& poi)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   m_CurrentCutLocation = pPoi->ConvertPoiToGirderlineCoordinate(poi);
   UpdateBar();
   GetGirderModelSectionView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
   GetGirderModelElevationView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
}

void CGirderModelChildFrame::UpdateBar()
{
   CComboBox* pcbGroup    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   CComboBox* pcbGirder   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   CStatic* pwndCutLocation = (CStatic*)m_SettingsBar.GetDlgItem(IDC_SECTION_CUT);
   ASSERT(pcbGroup);
   ASSERT(pcbGirder);
   ASSERT(pwndCutLocation);
   
   CEAFDocument* pDoc = EAFGetDocument();
   CString strGroupLabel( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) ? _T("Span") : _T("Group") );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // make sure controls are in sync with actual data groups
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   // refill group control
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   CString strLabel;
   strLabel.Format(_T("All %ss"), strGroupLabel);
   int idx = pcbGroup->AddString(strLabel);
   pcbGroup->SetItemData(idx,(DWORD_PTR)ALL_GROUPS);


   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      strLabel.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      idx = pcbGroup->AddString(strLabel);
      pcbGroup->SetItemData(idx,(DWORD_PTR)grpIdx);
   }

   curSel = pcbGroup->SetCurSel(curSel);
   
   if ( curSel == CB_ERR )
   {
      // Default to group 0 if not set
      curSel = pcbGroup->SetCurSel(0);
      ATLASSERT(curSel != CB_ERR);
      m_GirderKey.groupIndex = (GroupIndexType)(pcbGroup->GetItemData(curSel));
   }

   // girders
   GroupIndexType startGroupIdx = (m_GirderKey.groupIndex == ALL_GROUPS ? 0 : m_GirderKey.groupIndex );
   GroupIndexType endGroupIdx   = (m_GirderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   GirderIndexType nGirders = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      nGirders = Max(nGirders,pGroup->GetGirderCount());
   }

   curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      CString strLabel;
      strLabel.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString(strLabel);
   }
   
   curSel = pcbGirder->SetCurSel(curSel);

   if ( curSel == CB_ERR )
   {
      // Default to girder 0 if not set
      curSel = pcbGirder->SetCurSel(0);
      ATLASSERT(curSel != CB_ERR);
      m_GirderKey.girderIndex = 0;
   }

   UpdateMaxCutLocation();
   if ( m_MaxCutLocation < m_CurrentCutLocation )
   {
      m_CurrentCutLocation = m_MaxCutLocation;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString msg;
   msg.Format(_T("Section Cut: %s"),FormatDimension(m_CurrentCutLocation,pDisplayUnits->GetSpanLengthUnit()));

   pwndCutLocation->SetWindowText(msg);
   pwndCutLocation->EnableWindow();

   // frame title
   OnUpdateFrameTitle(TRUE);
}

void CGirderModelChildFrame::UpdateMaxCutLocation()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // Update the section cut range
   ATLASSERT(m_GirderKey.girderIndex != ALL_GIRDERS);
   if ( m_GirderKey.groupIndex == ALL_GROUPS )
   {
      // sum the length of all the girders
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      m_MaxCutLocation = 0;
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         Float64 lg = pBridge->GetGirderLayoutLength(CGirderKey(grpIdx,m_GirderKey.girderIndex));

         // if this is the first group, deduct the distance from the pier line to the end of the girder
         // from the girder layout length (basically coverting from
         // girder path length to girder length)
         if ( grpIdx == 0 )
         {
            CSegmentKey segmentKey(grpIdx,m_GirderKey.girderIndex,0);
            Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
            Float64 endDist   = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 start_offset = brgOffset - endDist;
            lg -= start_offset;
         }

         // if this is the last group, deduct the distance from the last pier line to the end of the girder
         // from the girder layout length (basically coverting from
         // girder path length to girder length)
         if ( grpIdx == nGroups-1 )
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,m_GirderKey.girderIndex));
            CSegmentKey segmentKey(grpIdx,m_GirderKey.girderIndex,nSegments-1);
            Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
            Float64 endDist   = pBridge->GetSegmentEndEndDistance(segmentKey);
            Float64 end_offset = brgOffset - endDist;
            lg -= end_offset;
         }

         m_MaxCutLocation += lg;
      }
   }
   else
   {
      m_MaxCutLocation = pBridge->GetGirderLength(m_GirderKey);
   }
}

void CGirderModelChildFrame::OnGirderChanged()
{
   CComboBox* pcbGirder   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   m_GirderKey.girderIndex = pcbGirder->GetCurSel();

   UpdateBar();
   UpdateViews();

   if ( DoSyncWithBridgeModelView() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::OnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   m_GirderKey.groupIndex = (GroupIndexType)pcbGroup->GetItemData(pcbGroup->GetCurSel());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType startGroupIdx = (m_GirderKey.groupIndex == INVALID_INDEX ? 0 : m_GirderKey.groupIndex);
   GroupIndexType endGroupIdx   = (m_GirderKey.groupIndex == INVALID_INDEX ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      m_GirderKey.girderIndex = Min(m_GirderKey.girderIndex,nGirders-1);
   }

   UpdateBar();
   UpdateViews();

   if ( DoSyncWithBridgeModelView() ) 
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::OnSelectEvent() 
{
   CComboBox* pcbEvents = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELEVENT);
   int curSel = pcbEvents->GetCurSel();
   m_EventIndex = (EventIndexType)pcbEvents->GetItemData(curSel);

   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::OnSectionCut()
{
   ShowCutDlg();
}

void CGirderModelChildFrame::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CSectionCutDlgEx dlg(pBroker,m_GirderKey,GetCutLocation());
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetPOI());
   }
}

Float64 CGirderModelChildFrame::GetMinCutLocation()
{
   return 0.0;
}

Float64 CGirderModelChildFrame::GetMaxCutLocation()
{
   return m_MaxCutLocation;
}

Float64 CGirderModelChildFrame::GetCurrentCutLocation() 
{
   return m_CurrentCutLocation;
}

void CGirderModelChildFrame::CutAt(Float64 Xgl)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pgsPointOfInterest poi = pPoi->ConvertGirderlineCoordinateToPoi(m_GirderKey.girderIndex,Xgl);
   if ( poi.GetID() == INVALID_ID )
   {
      // make sure we are at an actual poi
      poi = pPoi->GetNearestPointOfInterest(poi.GetSegmentKey(),poi.GetDistFromStart());
   }

   UpdateCutLocation(poi);
}

void CGirderModelChildFrame::CutAtNext()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pgsPointOfInterest currentPoi = GetCutLocation();
   pgsPointOfInterest poi = pPoi->GetNextPointOfInterest(currentPoi.GetID(),POI_ERECTED_SEGMENT);
   if ( poi.GetID() != INVALID_ID )
   {
      Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);
      CutAt(Xgl);
   }
}

void CGirderModelChildFrame::CutAtPrev()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pgsPointOfInterest currentPoi = GetCutLocation();
   pgsPointOfInterest poi = pPoi->GetPrevPointOfInterest(currentPoi.GetID(),POI_ERECTED_SEGMENT);
   if ( poi.GetID() != INVALID_ID )
   {
      Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);
      CutAt(Xgl);
   }
}

pgsPointOfInterest CGirderModelChildFrame::GetCutLocation()
{
   return GetGirderModelElevationView()->GetCutLocation();
}

void CGirderModelChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CGirderModelChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CGirderModelChildFrame::DoFilePrint(bool direct) 
{
   CGirderModelElevationView* pElevationView = GetGirderModelElevationView();
   CGirderModelSectionView*   pSectionView   = GetGirderModelSectionView();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   // create a print job and do it
   CGirderViewPrintJob pj(pElevationView, pSectionView, this, pBroker);
   pj.OnFilePrint(direct);
}

void CGirderModelChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg;
      if ( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS  )
      {
         msg.Format(_T("Girder View - %s"), GIRDER_LABEL(m_GirderKey));
      }
      else
      {
         msg.Format(_T("%s"),_T("Girder View"));
      }

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}

void CGirderModelChildFrame::OnAddPointload()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ATLASSERT( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS  ); // if we are adding a point load, a girder better be selected

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);

   // set data to that of view
   CPointLoadData data;
   data.m_SpanKey.spanIndex = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan()->GetIndex();
   data.m_SpanKey.girderIndex = m_GirderKey.girderIndex;

   EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   if ( m_EventIndex != liveLoadEventIdx)
   {
      data.m_EventIndex = m_EventIndex;
   }
   else
   {
      data.m_LoadCase = UserLoads::LL_IM;
   }

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditPointLoadDlg dlg(data,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : NULL);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CGirderModelChildFrame::OnAddDistributedLoad() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ATLASSERT( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS  ); // if we are adding a point load, a girder better be selected

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);

   // set data to that of view
   CDistributedLoadData data;
   data.m_SpanKey.spanIndex = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan()->GetIndex();
   data.m_SpanKey.girderIndex = m_GirderKey.girderIndex;

   EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   if ( m_EventIndex != liveLoadEventIdx)
   {
      data.m_EventIndex = m_EventIndex;
   }
   else
   {
      data.m_LoadCase = UserLoads::LL_IM;
   }

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditDistributedLoadDlg dlg(data,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : NULL);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CGirderModelChildFrame::OnAddMoment() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);

   // set data to that of view
   CMomentLoadData data;
   data.m_SpanKey.spanIndex = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan()->GetIndex();
   data.m_SpanKey.girderIndex = m_GirderKey.girderIndex;

   EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   if ( m_EventIndex != liveLoadEventIdx)
   {
      data.m_EventIndex = m_EventIndex;
   }
   else
   {
      data.m_LoadCase = UserLoads::LL_IM;
   }

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditMomentLoadDlg dlg(data,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : NULL);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

LRESULT CGirderModelChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_VIEW );
   return TRUE;
}

void CGirderModelChildFrame::OnSync() 
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   UINT settings = pDoc->GetGirderEditorSettings();

   if ( DoSyncWithBridgeModelView() )
   {
      settings |= IDG_SV_SYNC_GIRDER;
      pDoc->SelectGirder(m_GirderKey);
   }
   else
   {
      settings &= ~IDG_SV_SYNC_GIRDER;
   }

   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnSetFocus(CWnd* pOldWnd)
{
   __super::OnSetFocus(pOldWnd);

   if ( m_bIsAfterFirstUpdate && DoSyncWithBridgeModelView() ) 
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::FillEventComboBox()
{
   CComboBox* pcbEvents = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELEVENT);

   int sel = pcbEvents->GetCurSel();
   pcbEvents->ResetContent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
   EventIndexType firstEventIdx = pTimelineMgr->GetFirstSegmentErectionEventIndex();
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = firstEventIdx; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      CString strLabel;
      strLabel.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());
      int idx = pcbEvents->AddString(strLabel);
      pcbEvents->SetItemData(idx,(DWORD_PTR)eventIdx);
   }
   pcbEvents->SetCurSel( sel == CB_ERR ? 0 : sel );
   m_EventIndex = (EventIndexType)pcbEvents->GetItemData(pcbEvents->GetCurSel());
}