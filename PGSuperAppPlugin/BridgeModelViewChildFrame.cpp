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

// cfSplitChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDocBase.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "BridgeSectionView.h"
#include "AlignmentPlanView.h"
#include "AlignmentProfileView.h"
#include "BridgeViewPrintJob.h"
#include "StationCutDlg.h"
#include "SelectItemDlg.h"
#include "MainFrm.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\EditByUI.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include "EditBoundaryConditions.h"

#include <BridgeModelViewController.h>
#include "BridgeModelViewControllerImp.h"

#include "InsertSpanDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame

IMPLEMENT_DYNCREATE(CBridgeModelViewChildFrame, CSplitChildFrame)

CBridgeModelViewChildFrame::CBridgeModelViewChildFrame()
{
   m_bCutLocationInitialized = false;
   m_CurrentCutLocation = 0;
   m_bSelecting = false;

   CEAFViewControllerFactory::Init(this);
}

CBridgeModelViewChildFrame::~CBridgeModelViewChildFrame()
{
}

BOOL CBridgeModelViewChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   // force this window to be maximized (not sure why WS_VISIBLE is required)
   cs.style |= WS_MAXIMIZE | WS_VISIBLE;

   return __super::PreCreateWindow(cs);
}

BOOL CBridgeModelViewChildFrame::Create(LPCTSTR lpszClassName,
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
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_BRIDGEMODELEDITOR);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CBridgeModelViewChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CBridgeModelViewChildFrame)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_SPAN,   OnEditSpan)
	ON_COMMAND(ID_EDIT_PIER,   OnEditPier)
	ON_COMMAND(ID_VIEW_GIRDER, OnViewGirder)
	ON_COMMAND(ID_DELETE_PIER, OnDeletePier)
	ON_UPDATE_COMMAND_UI(ID_DELETE_PIER, OnUpdateDeletePier)
	ON_COMMAND(ID_DELETE_SPAN, OnDeleteSpan)
	ON_UPDATE_COMMAND_UI(ID_DELETE_SPAN, OnUpdateDeleteSpan)
	ON_COMMAND(ID_INSERT_SPAN, OnInsertSpan)
	ON_COMMAND(ID_INSERT_PIER, OnInsertPier)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnBoundaryCondition)
   ON_UPDATE_COMMAND_UI_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnUpdateBoundaryCondition)
   ON_COMMAND_RANGE(IDM_ERECTION_TOWER,IDM_STRONG_BACK,OnTemporarySupportType)
   ON_UPDATE_COMMAND_UI_RANGE(IDM_ERECTION_TOWER, IDM_STRONG_BACK, OnUpdateTemporarySupportType)
   ON_MESSAGE(WM_HELP, OnCommandHelp)
   ON_NOTIFY(UDN_DELTAPOS, IDC_START_GROUP_SPIN, &CBridgeModelViewChildFrame::OnStartGroupChanged)
   ON_NOTIFY(UDN_DELTAPOS, IDC_END_GROUP_SPIN, &CBridgeModelViewChildFrame::OnEndGroupChanged)
   ON_CONTROL_RANGE(BN_CLICKED,IDC_BRIDGE,IDC_ALIGNMENT,OnViewModeChanged)
   ON_BN_CLICKED(IDC_NORTH, &CBridgeModelViewChildFrame::OnNorth)
   ON_UPDATE_COMMAND_UI(IDC_NORTH, &CBridgeModelViewChildFrame::OnUpdateNorth)
   ON_BN_CLICKED(IDC_LABELS, &CBridgeModelViewChildFrame::OnShowLabels)
   ON_UPDATE_COMMAND_UI(IDC_LABELS, &CBridgeModelViewChildFrame::OnUpdateShowLabels)
   ON_BN_CLICKED(IDC_DIMENSIONS, &CBridgeModelViewChildFrame::OnDimensions)
   ON_UPDATE_COMMAND_UI(IDC_DIMENSIONS, &CBridgeModelViewChildFrame::OnUpdateDimensions)
   ON_BN_CLICKED(IDC_SHOW_BRIDGE, &CBridgeModelViewChildFrame::OnBridge)
   ON_UPDATE_COMMAND_UI(IDC_SHOW_BRIDGE, &CBridgeModelViewChildFrame::OnUpdateBridge)
   ON_BN_CLICKED(IDC_SCHEMATIC, &CBridgeModelViewChildFrame::OnSchematic)
   ON_UPDATE_COMMAND_UI(IDC_SCHEMATIC, &CBridgeModelViewChildFrame::OnUpdateSchematic)
   ON_BN_CLICKED(IDC_RW_CS, &CBridgeModelViewChildFrame::OnRwCrossSection)
   ON_UPDATE_COMMAND_UI(IDC_RW_CS, &CBridgeModelViewChildFrame::OnUpdateRwCrossSection)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame message handlers
CRuntimeClass* CBridgeModelViewChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CBridgeSectionView);
}

Float64 CBridgeModelViewChildFrame::GetTopFrameFraction() const
{
   return 0.5;
}

void CBridgeModelViewChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CBridgeModelViewChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CBridgeModelViewChildFrame::DoFilePrint(bool direct)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   // create a print job and do it
   CBridgeViewPrintJob pj(this,GetUpperView(), GetLowerView(), pBroker);
   pj.OnFilePrint(direct);
}

CBridgePlanView* CBridgeModelViewChildFrame::GetBridgePlanView() 
{
   CView* pView = GetUpperView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CBridgePlanView)) )
   {
      return (CBridgePlanView*)pView;
   }
   return nullptr;
}

CBridgeSectionView* CBridgeModelViewChildFrame::GetBridgeSectionView() 
{
   CView* pView = GetLowerView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CBridgeSectionView)) )
   {
      return (CBridgeSectionView*)pView;
   }
   return nullptr;
}

CAlignmentPlanView* CBridgeModelViewChildFrame::GetAlignmentPlanView()
{
   CView* pView = GetUpperView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CAlignmentPlanView)) )
   {
      return (CAlignmentPlanView*)pView;
   }
   return nullptr;
}

CAlignmentProfileView* CBridgeModelViewChildFrame::GetAlignmentProfileView()
{
   CView* pView = GetLowerView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CAlignmentProfileView)) )
   {
      return (CAlignmentProfileView*)pView;
   }
   return nullptr;
}

CBridgeViewPane* CBridgeModelViewChildFrame::GetUpperView()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   ASSERT_KINDOF(CBridgeViewPane,m_SplitterWnd.GetPane(0,0));
   return (CBridgeViewPane*)m_SplitterWnd.GetPane(0, 0);
}

CBridgeViewPane* CBridgeModelViewChildFrame::GetLowerView()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   ASSERT_KINDOF(CBridgeViewPane,m_SplitterWnd.GetPane(1,0));
   return (CBridgeViewPane*)m_SplitterWnd.GetPane(1, 0);
}

void CBridgeModelViewChildFrame::InitGroupRange()
{
   // Can't get to the broker, and thus the bridge information in OnCreate, so we need a method
   // that can be called later to initalize the span range for viewing

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();
   CString strType;
   if (!isPGSuper)
   {
      strType = _T("Groups");
      m_SettingsBar.GetDlgItem(IDC_GROUP_RANGE_LABEL)->SetWindowText(_T("View Groups"));
   }
   else
   {
      strType = _T("Spans");
      m_SettingsBar.GetDlgItem(IDC_GROUP_RANGE_LABEL)->SetWindowText(_T("View Spans"));
   }

   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_GROUP_SPIN);
   CSpinButtonCtrl* pEndSpinner   = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_GROUP_SPIN);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   CBridgePlanView* pPlanView = GetBridgePlanView();

   // indexes here are from 0 to nGroups-1
   GroupIndexType startGroupIdx, endGroupIdx;
   pPlanView->GetGroupRange(&startGroupIdx,&endGroupIdx);

   startGroupIdx = (startGroupIdx == ALL_GROUPS ? 0         : startGroupIdx);
   endGroupIdx   = (endGroupIdx   == ALL_GROUPS ? nGroups-1 : endGroupIdx);

   // convert from starting display span #
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType displayStartingGroupNum;
   if (isPGSuper)
   {
      displayStartingGroupNum = pBridgeDesc->GetDisplayStartingPierNumber();
   }
   else
   {
      displayStartingGroupNum = 1;
   }

   GroupIndexType startDisplayIndx = startGroupIdx + displayStartingGroupNum;
   GroupIndexType endDisplayIndx   = endGroupIdx + displayStartingGroupNum;

   pStartSpinner->SetRange32((int)startDisplayIndx,(int)(nGroups - 1 + displayStartingGroupNum));
   pEndSpinner->SetRange32((int)startDisplayIndx,(int)(nGroups - 1 + displayStartingGroupNum));

   pStartSpinner->SetPos32((int)startDisplayIndx);
   pEndSpinner->SetPos32((int)endDisplayIndx);

   CString str;
   if (pDocType->IsPGSpliceDocument())
   {
      str.Format(_T("of %d Groups"), nGroups);
   }
   else
   {
      str.Format(_T("of (%d - %d)"), startDisplayIndx, endDisplayIndx);
   }

   m_SettingsBar.GetDlgItem(IDC_GROUP_COUNT)->SetWindowText(str);
}

void CBridgeModelViewChildFrame::SetViewMode(CBridgeModelViewChildFrame::ViewMode viewMode)
{
   INT nIDC = (viewMode == Bridge ? IDC_BRIDGE : IDC_ALIGNMENT);
   m_SettingsBar.CheckRadioButton(IDC_BRIDGE, IDC_ALIGNMENT, nIDC);
   OnViewModeChanged(nIDC);
}

CBridgeModelViewChildFrame::ViewMode CBridgeModelViewChildFrame::GetViewMode() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_BRIDGE) ? Bridge : Alignment;
}

void CBridgeModelViewChildFrame::NorthUp(bool bNorthUp)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT bridge_settings = pDoc->GetBridgeEditorSettings();
   if (bNorthUp)
   {
      sysFlags<UINT>::Set(&bridge_settings, IDB_PV_NORTH_UP);
   }
   else
   {
      sysFlags<UINT>::Clear(&bridge_settings, IDB_PV_NORTH_UP);
   }
   pDoc->SetBridgeEditorSettings(bridge_settings);

   UINT alignment_settings = pDoc->GetAlignmentEditorSettings();
   if (sysFlags<UINT>::IsSet(bridge_settings, IDB_PV_NORTH_UP))
   {
      sysFlags<UINT>::Set(&alignment_settings, IDA_AP_NORTH_UP);
   }
   else
   {
      sysFlags<UINT>::Clear(&alignment_settings, IDA_AP_NORTH_UP);
   }
   pDoc->SetAlignmentEditorSettings(alignment_settings);
}

bool CBridgeModelViewChildFrame::NorthUp() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_NORTH) ? true : false;
}

void CBridgeModelViewChildFrame::ShowLabels(bool bShowLabels)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();
   if (bShowLabels)
   {
      sysFlags<UINT>::Set(&settings, IDB_PV_LABEL_PIERS);
      sysFlags<UINT>::Set(&settings, IDB_PV_LABEL_GIRDERS);
      sysFlags<UINT>::Set(&settings, IDB_PV_LABEL_BEARINGS);
      sysFlags<UINT>::Set(&settings, IDB_CS_LABEL_GIRDERS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDB_PV_LABEL_PIERS);
      sysFlags<UINT>::Clear(&settings, IDB_PV_LABEL_GIRDERS);
      sysFlags<UINT>::Clear(&settings, IDB_PV_LABEL_BEARINGS);
      sysFlags<UINT>::Clear(&settings, IDB_CS_LABEL_GIRDERS);
   }

   pDoc->SetBridgeEditorSettings(settings);

}

bool CBridgeModelViewChildFrame::ShowLabels() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_LABELS) ? true : false;
}

void CBridgeModelViewChildFrame::ShowDimensions(bool bShowDimensions)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();
   if (bShowDimensions)
   {
      sysFlags<UINT>::Set(&settings, IDB_CS_SHOW_DIMENSIONS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDB_CS_SHOW_DIMENSIONS);
   }
   pDoc->SetBridgeEditorSettings(settings);
}

bool CBridgeModelViewChildFrame::ShowDimensions() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_DIMENSIONS) ? true : false;
}

void CBridgeModelViewChildFrame::ShowBridge(bool bShowBridge)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if (bShowBridge)
   {
      sysFlags<UINT>::Set(&settings, IDA_AP_DRAW_BRIDGE);
      sysFlags<UINT>::Set(&settings, IDP_AP_DRAW_BRIDGE);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDA_AP_DRAW_BRIDGE);
      sysFlags<UINT>::Clear(&settings, IDP_AP_DRAW_BRIDGE);
   }

   pDoc->SetAlignmentEditorSettings(settings);
}

bool CBridgeModelViewChildFrame::ShowBridge() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_SHOW_BRIDGE) ? true : false;
}

void CBridgeModelViewChildFrame::Schematic(bool bSchematic)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if (bSchematic)
   {
      sysFlags<UINT>::Set(&settings, IDP_AP_DRAW_ISOTROPIC);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDP_AP_DRAW_ISOTROPIC);
   }
   pDoc->SetAlignmentEditorSettings(settings);
}

bool CBridgeModelViewChildFrame::Schematic() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_SCHEMATIC) ? true : false;
}

void CBridgeModelViewChildFrame::ShowRwCrossSection(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDB_CS_DRAW_RW_CS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDB_CS_DRAW_RW_CS);
   }
   pDoc->SetBridgeEditorSettings(settings);
}

bool CBridgeModelViewChildFrame::ShowRwCrossSection() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_RW_CS) ? true : false;
}

#if defined _DEBUG
void CBridgeModelViewChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSplitChildFrame::AssertValid();
}

void CBridgeModelViewChildFrame::Dump(CDumpContext& dc) const
{
   CSplitChildFrame::Dump(dc);
}
#endif 

int CBridgeModelViewChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	this->SetWindowText(_T("Bridge View"));

   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HINSTANCE hInstance = AfxGetInstanceHandle();
      if ( !m_SettingsBar.Create( this, IDD_BRIDGEVIEW_CONTROLS, CBRS_TOP, IDD_BRIDGEVIEW_CONTROLS) )
	   {
		   TRACE0("Failed to create control bar\n");
		   return -1;      // fail to create
	   }

      m_SettingsBar.SetBarStyle(m_SettingsBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

      m_SettingsBar.CheckRadioButton(IDC_BRIDGE,IDC_ALIGNMENT,IDC_BRIDGE);

      CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_NORTH);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_NORTH), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);

      pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_LABELS);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_LABELS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);

      pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_DIMENSIONS);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_DIMENSIONS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);

      pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SHOW_BRIDGE);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_BRIDGE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);

      pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SCHEMATIC);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCHEMATIC_PROFILE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);

      pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_RW_CS);
      pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_RW_CS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
      m_SettingsBar.AddTooltip(pBtn);
   }

   return 0;
}

void CBridgeModelViewChildFrame::CutAt(Float64 X)
{
   UpdateCutLocation(X);
}

Float64 CBridgeModelViewChildFrame::GetNextCutStation(Float64 direction)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // if control key is down... move the section cut

   Float64 station = GetCurrentCutLocation();

   SpanIndexType spanIdx;
   if ( !pBridge->GetSpan(station,&spanIdx) )
   {
      return station;
   }

   Float64 back_pier = pBridge->GetPierStation(spanIdx);
   Float64 ahead_pier = pBridge->GetPierStation(spanIdx+1);
   Float64 span_length = ahead_pier - back_pier;
   Float64 inc = span_length/10;

   station = station + direction*inc;

   return station;
}

void CBridgeModelViewChildFrame::CutAtNext()
{
   CutAt(GetNextCutStation(1));
}

void CBridgeModelViewChildFrame::CutAtPrev()
{
   CutAt(GetNextCutStation(-1));
}

void CBridgeModelViewChildFrame::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   Float64 startStation, endStation;
   GetCutRange(&startStation, &endStation);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CStationCutDlg dlg(m_CurrentCutLocation,startStation,endStation,bUnitsSI);
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetValue());
   }
}

void CBridgeModelViewChildFrame::GetCutRange(Float64* pMin, Float64* pMax)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);

   GroupIndexType startGroupIdx, endGroupIdx;
   GetBridgePlanView()->GetGroupRange(&startGroupIdx,&endGroupIdx);

   SpanIndexType startSpanIdx, dummySpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(startGroupIdx, &startSpanIdx, &dummySpanIdx);
   pBridge->GetGirderGroupSpans(endGroupIdx, &dummySpanIdx, &endSpanIdx);
   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx = (PierIndexType)(endSpanIdx + 1);

   *pMin = pBridge->GetPierStation(startPierIdx);
   *pMax = pBridge->GetPierStation(endPierIdx);

   if (startPierIdx == 0)
   {
      // adjust for start cantilever if present
      SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
      GroupIndexType grpIdx = 0;
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      Float64 Lc = 0;
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         Float64 lc = pBridge->GetCantileverLength(startSpanIdx, gdrIdx, pgsTypes::metStart);
         Lc = Max(Lc, lc);
      }

      *pMin -= Lc;
   }

   PierIndexType nPiers = pBridge->GetPierCount();
   if (endPierIdx == nPiers - 1)
   {
      // adjust for end cantilever if present
      SpanIndexType endSpanIdx = (SpanIndexType)(endPierIdx - 1);
      GroupIndexType grpIdx = pBridge->GetGirderGroupCount() - 1;
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      Float64 Lc = 0;
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         Float64 lc = pBridge->GetCantileverLength(endSpanIdx, gdrIdx, pgsTypes::metEnd);
         Lc = Max(Lc, lc);
      }

      *pMax += Lc;
   }
}

void CBridgeModelViewChildFrame::UpdateCutLocation(Float64 cut)
{
   Float64 startStation, endStation;
   GetCutRange(&startStation, &endStation);
   m_CurrentCutLocation = ForceIntoRange(startStation, cut, endStation);

   GetUpperView()->OnUpdate(nullptr, HINT_BRIDGEVIEWSECTIONCUTCHANGED, nullptr);
   GetLowerView()->OnUpdate(nullptr, HINT_BRIDGEVIEWSECTIONCUTCHANGED, nullptr);
}
   
Float64 CBridgeModelViewChildFrame::GetCurrentCutLocation()
{
   if ( !m_bCutLocationInitialized )
   {
      Float64 startStation, endStation;
      GetCutRange(&startStation, &endStation);
      m_CurrentCutLocation = 0.5*(startStation+endStation);
      m_bCutLocationInitialized = true;
   }

   return m_CurrentCutLocation;
}

void CBridgeModelViewChildFrame::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   CEAFViewControllerFactory::CreateViewController(&stdController);

   CComObject<CBridgeModelViewController>* pController;
   CComObject<CBridgeModelViewController>::CreateInstance(&pController);
   pController->Init(this,stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

void CBridgeModelViewChildFrame::OnViewGirder() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSegmentKey segmentKey;

   if ( GetBridgePlanView()->GetSelectedGirder(&segmentKey) || GetBridgePlanView()->GetSelectedSegment(&segmentKey) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->OnViewGirderEditor();
   }
}

void CBridgeModelViewChildFrame::OnEditSpan() 
{
   SpanIndexType spanIdx;
   if ( GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->EditSpanDescription(spanIdx,ESD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::OnEditPier() 
{
   PierIndexType pierIdx;
   if ( GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->EditPierDescription(pierIdx,EPD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::SelectSpan(SpanIndexType spanIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectSpan(spanIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectPier(PierIndexType pierIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectPier(pierIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectGirder(const CGirderKey& girderKey)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectGirder(girderKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectSegment(const CSegmentKey& segmentKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectSegment(segmentKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectClosureJoint(const CSegmentKey& closureKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(closureKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectClosureJoint(closureKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectTemporarySupport(SupportIDType tsID)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectTemporarySupport(tsID);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectDeck()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectDeck();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectAlignment()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectAlignment();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectTrafficBarrier(pgsTypes::TrafficBarrierOrientation orientation)
{
   if (m_bSelecting)
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectTrafficBarrier(orientation);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::ClearSelection()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->ClearSelection();
}

LRESULT CBridgeModelViewChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGE_VIEW );
   return TRUE;
}

void CBridgeModelViewChildFrame::OnDeletePier() 
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->DeletePier(pierIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeletePier(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType pierIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnDeleteSpan() 
{
   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
      pDoc->DeleteSpan(spanIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeleteSpan(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnInsertSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType face = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIndex;

         CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,face,span_length,bCreateNewGroup,eventIdx);
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnInsertPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType pierFace = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIndex;

         CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,pierFace,span_length,bCreateNewGroup,eventIdx);

         if ( pierFace == pgsTypes::Back )
         {
            // move the pier selection ahead one pier so that it seems to
            // stay with the currently selected pier
            pView->SelectPier(pierIdx+1,true);
         }
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

EventIndexType GetDefaultCastClosureJointEvent(IBridgeDescription* pIBridgeDesc,const CGirderGroupData* pGroup)
{
   // A new closure joint is being created. We need to supply the bridge model with the event index
   // for when the closure joint is cast. Since the user wasn't prompted (this is a response
   // handler for a right-click context menu) the first-best guess is to use the closure casting
   // event for another closure joint in this girder.

   GroupIndexType grpIdx = pGroup->GetIndex();

   EventIndexType newClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx, 0);
   if (newClosureEventIdx == INVALID_INDEX)
   {
      // there isn't another closure joint in this group with a valid event index
      // the next best guess is to use the event just before the event when the first 
      // tendon is installed.
      EventIndexType eventIdx = INVALID_INDEX;
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         CGirderKey girderKey(pGirder->GetGirderKey());
         const CPTData* pPT = pGirder->GetPostTensioning();
         DuctIndexType nDucts = pPT->GetDuctCount();
         for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
         {
            EventIndexType stressTendonEventIdx = pIBridgeDesc->GetStressTendonEventIndex(girderKey, ductIdx);
            eventIdx = Min(eventIdx, stressTendonEventIdx);
         }
      }

      if (eventIdx != INVALID_INDEX)
      {
         newClosureEventIdx = eventIdx - 1;
      }
   }

   if (newClosureEventIdx == INVALID_INDEX)
   {
      // the last best guess is to cast the closure when the segments are installed
      EventIndexType eventIdx = 0;
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            EventIndexType erectSegmentEventIdx = pIBridgeDesc->GetSegmentErectionEventIndex(segmentKey);
            eventIdx = Max(eventIdx, erectSegmentEventIdx);
         }
      }

      if (eventIdx != INVALID_INDEX)
      {
         newClosureEventIdx = eventIdx;
      }
   }

   // if this fires then there is a use case that hasn't been considered
   ATLASSERT(newClosureEventIdx != INVALID_INDEX);

   return newClosureEventIdx;
}

void CBridgeModelViewChildFrame::OnBoundaryCondition(UINT nIDC)
{
   CBridgePlanView* pView = GetBridgePlanView();
   PierIndexType pierIdx;
   if ( pView->GetSelectedPier(&pierIdx) )
   {
      pgsTypes::BoundaryConditionType newBoundaryConditionType;
      pgsTypes::PierSegmentConnectionType newSegmentConnectionType;
      switch( nIDC )
      {
         case IDM_HINGE:                          newBoundaryConditionType = pgsTypes::bctHinge;                        break;
         case IDM_ROLLER:                         newBoundaryConditionType = pgsTypes::bctRoller;                       break;
         case IDM_CONTINUOUS_AFTERDECK:           newBoundaryConditionType = pgsTypes::bctContinuousAfterDeck;          break;
         case IDM_CONTINUOUS_BEFOREDECK:          newBoundaryConditionType = pgsTypes::bctContinuousBeforeDeck;         break;
         case IDM_INTEGRAL_AFTERDECK:             newBoundaryConditionType = pgsTypes::bctIntegralAfterDeck;            break;
         case IDM_INTEGRAL_BEFOREDECK:            newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeck;           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   newBoundaryConditionType = pgsTypes::bctIntegralAfterDeckHingeBack;   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeckHingeBack;  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  newBoundaryConditionType = pgsTypes::bctIntegralAfterDeckHingeAhead;  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeckHingeAhead; break;
         case IDM_CONTINUOUS_CLOSURE:             newSegmentConnectionType = pgsTypes::psctContinousClosureJoint;  break;
         case IDM_INTEGRAL_CLOSURE:               newSegmentConnectionType = pgsTypes::psctIntegralClosureJoint;   break;
         case IDM_CONTINUOUS_SEGMENT_AT_PIER:     newSegmentConnectionType = pgsTypes::psctContinuousSegment;     break;
         case IDM_INTEGRAL_SEGMENT_AT_PIER:       newSegmentConnectionType = pgsTypes::psctIntegralSegment;       break;

         default: ATLASSERT(false); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      txnEditBoundaryConditions* pTxn;

      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType oldConnectionType = pPier->GetBoundaryConditionType();
         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,newBoundaryConditionType);
      }
      else
      {
         pgsTypes::PierSegmentConnectionType oldConnectionType = pPier->GetSegmentConnectionType();
         EventIndexType oldClosureEventIdx = INVALID_INDEX;
         EventIndexType newClosureEventIdx = INVALID_INDEX;
         const CGirderGroupData* pGroup = pPier->GetGirderGroup(pgsTypes::Back); // this is an interior pier so back/ahead are the same
         GroupIndexType grpIdx = pGroup->GetIndex();
         if ( oldConnectionType == pgsTypes::psctContinousClosureJoint || oldConnectionType == pgsTypes::psctIntegralClosureJoint )
         {
            IndexType closureIdx = pPier->GetClosureJoint(0)->GetIndex();
            oldClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx,closureIdx);
         }

         if ( newSegmentConnectionType == pgsTypes::psctContinousClosureJoint || newSegmentConnectionType == pgsTypes::psctIntegralClosureJoint )
         {
            // A new closure joint is being created. We need to supply the bridge model with the event index
            // for when the closure joint is cast. Since the user wasn't prompted (this is a response
            // handler for a right-click context menu) the first-best guess is to use the closure casting
            // event for another closure joint in this girder.
            newClosureEventIdx = GetDefaultCastClosureJointEvent(pIBridgeDesc, pGroup);
         }

         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,oldClosureEventIdx,newSegmentConnectionType,newClosureEventIdx);
      }
      txnTxnManager::GetInstance()->Execute(pTxn);
   }

   SupportIndexType tsIdx;
   if (pView->GetSelectedTemporarySupport(&tsIdx))
   {
      pgsTypes::TempSupportSegmentConnectionType newConnectionType;
      switch (nIDC)
      {
      case IDM_CONTINUOUS_CLOSURE:  newConnectionType = pgsTypes::tsctClosureJoint;         break;
      case IDM_CONTINUOUS_SEGMENT:  newConnectionType = pgsTypes::tsctContinuousSegment;    break;
      default: ATLASSERT(false); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      txnEditBoundaryConditions* pTxn;

      const CTemporarySupportData* pTS = pIBridgeDesc->GetTemporarySupport(tsIdx);
      pgsTypes::TemporarySupportType supportType = pTS->GetSupportType();
      pgsTypes::TempSupportSegmentConnectionType oldConnectionType = pTS->GetConnectionType();
      EventIndexType oldClosureEventIdx = INVALID_INDEX;
      EventIndexType newClosureEventIdx = INVALID_INDEX;
      const CGirderGroupData* pGroup = pTS->GetSpan()->GetPier(pgsTypes::metStart)->GetGirderGroup(pgsTypes::Ahead);
      GroupIndexType grpIdx = pGroup->GetIndex();
      if (oldConnectionType == pgsTypes::tsctClosureJoint)
      {
         IndexType closureIdx = pTS->GetClosureJoint(0)->GetIndex();
         oldClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx, closureIdx);
      }

      if (newConnectionType == pgsTypes::tsctClosureJoint)
      {
         // A new closure joint is being created. We need to supply the bridge model with the event index
         // for when the closure joint is cast. Since the user wasn't prompted (this is a response
         // handler for a right-click context menu) the first-best guess is to use the closure casting
         // event for another closure joint in this girder.
         newClosureEventIdx = GetDefaultCastClosureJointEvent(pIBridgeDesc, pGroup);
      }

      pTxn = new txnEditBoundaryConditions(tsIdx, supportType, oldConnectionType, oldClosureEventIdx, supportType, newConnectionType, newClosureEventIdx);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CBridgeModelViewChildFrame::OnUpdateBoundaryCondition(CCmdUI* pCmdUI)
{
   CBridgePlanView* pView = GetBridgePlanView();
   PierIndexType pierIdx;
   if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType boundaryConditionType = pPier->GetBoundaryConditionType();
         switch( pCmdUI->m_nID )
         {
            case IDM_HINGE:                          pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctHinge);                        break;
            case IDM_ROLLER:                         pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctRoller);                       break;
            case IDM_CONTINUOUS_AFTERDECK:           pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctContinuousAfterDeck);          break;
            case IDM_CONTINUOUS_BEFOREDECK:          pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctContinuousBeforeDeck);         break;
            case IDM_INTEGRAL_AFTERDECK:             pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeck);            break;
            case IDM_INTEGRAL_BEFOREDECK:            pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeck);           break;
            case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack);   break;
            case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack);  break;
            case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead);  break;
            case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead); break;
            default: ATLASSERT(false); // is there a new boundary condition type?
         }
      }
      else
      {
         pgsTypes::PierSegmentConnectionType segmentConnectionType = pPier->GetSegmentConnectionType();
         switch( pCmdUI->m_nID )
         {
            case IDM_CONTINUOUS_CLOSURE:             pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinousClosureJoint);  break;
            case IDM_INTEGRAL_CLOSURE:               pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralClosureJoint);   break;
            case IDM_CONTINUOUS_SEGMENT_AT_PIER:     pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinuousSegment);     break;
            case IDM_INTEGRAL_SEGMENT_AT_PIER:       pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralSegment);       break;
            default: ATLASSERT(false); // is there a new connection type?
         }
      }
   }

   SupportIndexType tsIdx;
   if (pView->GetSelectedTemporarySupport(&tsIdx))
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CTemporarySupportData* pTS = pIBridgeDesc->GetTemporarySupport(tsIdx);
      pgsTypes::TempSupportSegmentConnectionType segmentConnectionType = pTS->GetConnectionType();
      switch (pCmdUI->m_nID)
      {
      case IDM_CONTINUOUS_CLOSURE: pCmdUI->SetCheck(segmentConnectionType == pgsTypes::tsctClosureJoint); break;
      case IDM_CONTINUOUS_SEGMENT: pCmdUI->SetCheck(segmentConnectionType == pgsTypes::tsctContinuousSegment); break;
      default: ATLASSERT(false); // is there a new connection type?
      }
   }
}

void CBridgeModelViewChildFrame::OnTemporarySupportType(UINT nIDC)
{
   CBridgePlanView* pView = GetBridgePlanView();
   SupportIndexType tsIdx;
   if (pView->GetSelectedTemporarySupport(&tsIdx))
   {
      pgsTypes::TemporarySupportType newSupportType;
      switch (nIDC)
      {
      case IDM_ERECTION_TOWER:  newSupportType = pgsTypes::ErectionTower;         break;
      case IDM_STRONG_BACK:  newSupportType = pgsTypes::StrongBack;    break;
      default: ATLASSERT(false); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CTemporarySupportData* pTS = pIBridgeDesc->GetTemporarySupport(tsIdx);

      txnEditBoundaryConditions* pTxn;
      pgsTypes::TemporarySupportType oldSupportType = pTS->GetSupportType();
      pgsTypes::TempSupportSegmentConnectionType oldConnectionType = pTS->GetConnectionType();
      if (oldConnectionType == pgsTypes::tsctClosureJoint)
      {
         // if there is a closure joint we can toggle between erection tower and strong back
         pTxn = new txnEditBoundaryConditions(tsIdx, oldSupportType, newSupportType);
      }
      else
      {
         pgsTypes::TempSupportSegmentConnectionType newConnectionType;
         switch (nIDC)
         {
         case IDM_ERECTION_TOWER: newConnectionType = oldConnectionType; break;
         case IDM_STRONG_BACK: newConnectionType = pgsTypes::tsctClosureJoint; break; // strong backs must hae a closure joint type connection
         default: ATLASSERT(false); // is there a new type?
         }

         EventIndexType oldClosureEventIdx = INVALID_INDEX;
         EventIndexType newClosureEventIdx = INVALID_INDEX;
         const CGirderGroupData* pGroup = pTS->GetSpan()->GetPier(pgsTypes::metStart)->GetGirderGroup(pgsTypes::Ahead);
         GroupIndexType grpIdx = pGroup->GetIndex();
         if (oldConnectionType == pgsTypes::tsctClosureJoint)
         {
            IndexType closureIdx = pTS->GetClosureJoint(0)->GetIndex();
            oldClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx, closureIdx);
         }

         if (newConnectionType == pgsTypes::tsctClosureJoint)
         {
            // A new closure joint is being created. We need to supply the bridge model with the event index
            // for when the closure joint is cast. Since the user wasn't prompted (this is a response
            // handler for a right-click context menu) the first-best guess is to use the closure casting
            // event for another closure joint in this girder.
            newClosureEventIdx = GetDefaultCastClosureJointEvent(pIBridgeDesc, pGroup);
         }

         pTxn = new txnEditBoundaryConditions(tsIdx, oldSupportType, oldConnectionType, oldClosureEventIdx, newSupportType, newConnectionType, newClosureEventIdx);
      }
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CBridgeModelViewChildFrame::OnUpdateTemporarySupportType(CCmdUI* pCmdUI)
{
   CBridgePlanView* pView = GetBridgePlanView();
   SupportIndexType tsIdx;
   if (pView->GetSelectedTemporarySupport(&tsIdx))
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CTemporarySupportData* pTS = pIBridgeDesc->GetTemporarySupport(tsIdx);
      pgsTypes::TemporarySupportType type = pTS->GetSupportType();
      switch (pCmdUI->m_nID)
      {
      case IDM_ERECTION_TOWER: pCmdUI->SetCheck(type == pgsTypes::ErectionTower); break;
      case IDM_STRONG_BACK: pCmdUI->SetCheck(type == pgsTypes::StrongBack); break;
      default: ATLASSERT(false); // is there a new connection type?
      }
   }
}

void CBridgeModelViewChildFrame::OnStartGroupChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   *pResult = 0;

   // convert from starting display span #
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType displayStartingPierNum = pBridgeDesc->GetDisplayStartingPierNumber();

   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_GROUP_SPIN);
   int start, end;
   pStartSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   GroupIndexType newStartGroupIdx = newPos - displayStartingPierNum; // from display index to internal

   CBridgePlanView* pPlanView = GetBridgePlanView();

   GroupIndexType startGroupIdx, endGroupIdx;
   pPlanView->GetGroupRange(&startGroupIdx,&endGroupIdx);

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_GROUP_SPIN);
   if ( endGroupIdx <= newStartGroupIdx )
   {
      // new start group is greater than end group
      // force position to be the same
      endGroupIdx = newStartGroupIdx;

      pEndSpinner->SetPos32((int)(endGroupIdx + displayStartingPierNum));
   }

   pPlanView->SetGroupRange(newStartGroupIdx,endGroupIdx);
}

void CBridgeModelViewChildFrame::OnEndGroupChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   *pResult = 0;

   // convert from starting display span #
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType displayStartingPierNum = pBridgeDesc->GetDisplayStartingPierNumber();

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_GROUP_SPIN);
   int start, end;
   pEndSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   GroupIndexType newEndGroupIdx = newPos - displayStartingPierNum; // convert from display to internal indices

   CBridgePlanView* pPlanView = GetBridgePlanView();

   GroupIndexType startGroupIdx, endGroupIdx;
   pPlanView->GetGroupRange(&startGroupIdx,&endGroupIdx);
   
   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_GROUP_SPIN);
   if ( newEndGroupIdx <= startGroupIdx )
   {
      // new end group is less than start group
      // force position to be the same
      startGroupIdx = newEndGroupIdx;

      pStartSpinner->SetPos32((int)startGroupIdx+1);
   }

   pPlanView->SetGroupRange(startGroupIdx,newEndGroupIdx);
}

void CBridgeModelViewChildFrame::OnViewModeChanged(UINT nIDC)
{
   int show = (nIDC == IDC_BRIDGE ? SW_SHOW : SW_HIDE);
   m_SettingsBar.GetDlgItem(IDC_GROUP_RANGE_LABEL)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_START_GROUP_SPIN)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_START_GROUP_EDIT)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_GROUP_RANGE_TO)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_END_GROUP_SPIN)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_END_GROUP_EDIT)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_GROUP_COUNT)->ShowWindow(show);

   AFX_MANAGE_STATE(AfxGetAppModuleState());
   if ( nIDC == IDC_BRIDGE )
   {
      m_SplitterWnd.ReplaceView(0,0,RUNTIME_CLASS(CBridgePlanView));
      m_SplitterWnd.ReplaceView(1,0,RUNTIME_CLASS(CBridgeSectionView));
   }
   else
   {
      m_SplitterWnd.ReplaceView(0,0,RUNTIME_CLASS(CAlignmentPlanView));
      m_SplitterWnd.ReplaceView(1,0,RUNTIME_CLASS(CAlignmentProfileView));
   }
}

void CBridgeModelViewChildFrame::OnUpdateNorth(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
 
   UINT bridge_settings = pDoc->GetBridgeEditorSettings();
   UINT alignment_settings = pDoc->GetAlignmentEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(bridge_settings,IDB_PV_NORTH_UP) || sysFlags<UINT>::IsSet(alignment_settings, IDA_AP_NORTH_UP)  ? BST_CHECKED : BST_UNCHECKED);
}

void CBridgeModelViewChildFrame::OnNorth()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT bridge_settings = pDoc->GetBridgeEditorSettings();
   sysFlags<UINT>::Toggle(&bridge_settings, IDB_PV_NORTH_UP);
   pDoc->SetBridgeEditorSettings(bridge_settings);

   UINT alignment_settings = pDoc->GetAlignmentEditorSettings();
   if (sysFlags<UINT>::IsSet(bridge_settings, IDB_PV_NORTH_UP))
   {
      sysFlags<UINT>::Set(&alignment_settings, IDA_AP_NORTH_UP);
   }
   else
   {
      sysFlags<UINT>::Clear(&alignment_settings, IDA_AP_NORTH_UP);
   }
   pDoc->SetAlignmentEditorSettings(alignment_settings);
}

void CBridgeModelViewChildFrame::OnUpdateShowLabels(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDB_PV_LABEL_PIERS) || sysFlags<UINT>::IsSet(settings, IDB_PV_LABEL_GIRDERS) || sysFlags<UINT>::IsSet(settings, IDB_PV_LABEL_BEARINGS) || sysFlags<UINT>::IsSet(settings, IDB_CS_LABEL_GIRDERS) ? BST_CHECKED : BST_UNCHECKED);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnShowLabels()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      sysFlags<UINT>::Toggle(&settings, IDB_PV_LABEL_PIERS);
      sysFlags<UINT>::Toggle(&settings, IDB_PV_LABEL_GIRDERS);
      sysFlags<UINT>::Toggle(&settings, IDB_PV_LABEL_BEARINGS);
      sysFlags<UINT>::Toggle(&settings, IDB_CS_LABEL_GIRDERS);
      pDoc->SetBridgeEditorSettings(settings);
   }
}

void CBridgeModelViewChildFrame::OnUpdateDimensions(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDB_CS_SHOW_DIMENSIONS) ? BST_CHECKED : BST_UNCHECKED);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnDimensions()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      sysFlags<UINT>::Toggle(&settings, IDB_CS_SHOW_DIMENSIONS);
      pDoc->SetBridgeEditorSettings(settings);
   }
}

void CBridgeModelViewChildFrame::OnUpdateBridge(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      UINT settings = pDoc->GetAlignmentEditorSettings();
      pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDA_AP_DRAW_BRIDGE) || sysFlags<UINT>::IsSet(settings, IDP_AP_DRAW_BRIDGE) ? BST_CHECKED : BST_UNCHECKED);
      pCmdUI->Enable(TRUE);
   }
}

void CBridgeModelViewChildFrame::OnBridge()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_ALIGNMENT)
   {
      UINT settings = pDoc->GetAlignmentEditorSettings();
      sysFlags<UINT>::Toggle(&settings, IDA_AP_DRAW_BRIDGE);
      sysFlags<UINT>::Toggle(&settings, IDP_AP_DRAW_BRIDGE);
      pDoc->SetAlignmentEditorSettings(settings);
   }
}

void CBridgeModelViewChildFrame::OnRwCrossSection()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      sysFlags<UINT>::Toggle(&settings, IDB_CS_DRAW_RW_CS);
      pDoc->SetBridgeEditorSettings(settings);
   }
}

void CBridgeModelViewChildFrame::OnUpdateRwCrossSection(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      UINT settings = pDoc->GetBridgeEditorSettings();
      pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDB_CS_DRAW_RW_CS) ? BST_CHECKED : BST_UNCHECKED);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnUpdateSchematic(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_BRIDGE)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      UINT settings = pDoc->GetAlignmentEditorSettings();
      pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDP_AP_DRAW_ISOTROPIC) ? BST_UNCHECKED : BST_CHECKED);
      pCmdUI->Enable(TRUE);
   }
}

void CBridgeModelViewChildFrame::OnSchematic()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   if (m_SettingsBar.GetCheckedRadioButton(IDC_BRIDGE, IDC_ALIGNMENT) == IDC_ALIGNMENT)
   {
      UINT settings = pDoc->GetAlignmentEditorSettings();
      sysFlags<UINT>::Toggle(&settings, IDP_AP_DRAW_ISOTROPIC);
      pDoc->SetAlignmentEditorSettings(settings);
   }
}