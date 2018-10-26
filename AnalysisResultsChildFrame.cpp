///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// AnalysisResultsChildFrame.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "pgsuperDoc.h"
#include "AnalysisResultsChildFrame.h"
#include "AnalysisResultsView.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <PgsExt\PointOfInterest.h>
#include "PGSuperTypes.h"
#include "PGSuperColors.h"
#include "htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

COLORREF clrTruck[] = 
{
   SLATEGREY,
   DARKSLATEGREY,
   MIDNIGHTBLUE,
   CORNFLOWERBLUE,
   SLATEBLUE,
   SALMON1,
   ORANGE1,
   DARKORANGE1,
   ORANGERED1,
   DEEPPINK1,
   HOTPINK1,
   MAGENTA1,
   PLUM1,
   PURPLE1
};

long nTruckColors = sizeof(clrTruck)/sizeof(COLORREF);

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsChildFrame

IMPLEMENT_DYNCREATE(CAnalysisResultsChildFrame, CEAFOutputChildFrame)

CAnalysisResultsChildFrame::CAnalysisResultsChildFrame():
m_GirderIdx(0),
m_SpanIdx(0),
m_Grid(true),
m_Action(actionMoment),
m_Stage(pgsTypes::CastingYard),
m_AnalysisType(pgsTypes::Simple)
{
}

CAnalysisResultsChildFrame::~CAnalysisResultsChildFrame()
{
}

BOOL CAnalysisResultsChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
#if defined _EAF_USING_MFC_FEATURE_PACK
   // If MFC Feature pack is used, we are using tabbed MDI windows so we don't want
   // the system menu or the minimize and maximize boxes
   dwStyle &= ~(WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
#endif

   BOOL bResult = CEAFOutputChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_ANALYSISRESULTS);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CAnalysisResultsChildFrame, CEAFOutputChildFrame)
	//{{AFX_MSG_MAP(CAnalysisResultsChildFrame)
	ON_WM_CREATE()
   ON_CBN_SELCHANGE( IDC_GIRDER   , OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SPAN     , OnSpanChanged )
   ON_CBN_SELCHANGE( IDC_STAGE    , OnStageChanged )
   ON_LBN_SELCHANGE( IDC_LOAD_CASE, OnLoadCaseChanged )
   ON_CBN_SELCHANGE( IDC_ACTION   , OnActionChanged )
   ON_BN_CLICKED(IDC_GRID, OnGridClicked)
//   ON_COMMAND_RANGE(IDC_SIMPLE,IDC_SIMPLE3,OnAnalysisTypeClicked) // for some reason this is causing a crash in release mode
   ON_BN_CLICKED(IDC_SIMPLE,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE2,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE3,OnAnalysisTypeClicked)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsChildFrame message handlers
void CAnalysisResultsChildFrame::SelectSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   CComboBox* pcbSpans = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   ASSERT(pcbSpans);

   int nItems = pcbSpans->GetCount();
   for ( int i = 0; i < nItems; i++ )
   {
      if ( (SpanIndexType)pcbSpans->GetItemData(i) == spanIdx )
      {
         pcbSpans->SetCurSel(i);
         OnSpanChanged();
         break;
      }
   }

   CComboBox* pcbGirders = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   pcbGirders->SetCurSel((int)gdrIdx);
   OnGirderChanged();
}

SpanIndexType CAnalysisResultsChildFrame::GetSpanIdx() const             
{
   return m_SpanIdx;
}

GirderIndexType CAnalysisResultsChildFrame::GetGirderIdx() const 
{
   return m_GirderIdx;
}

pgsTypes::Stage  CAnalysisResultsChildFrame::GetStage() const     
{
   return m_Stage;
}

pgsTypes::AnalysisType CAnalysisResultsChildFrame::GetAnalysisType() const
{
   return m_AnalysisType;
}

int CAnalysisResultsChildFrame::GetGraphCount() const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int count = pload_case_ctrl->GetSelCount();

   return count;
}

GraphType CAnalysisResultsChildFrame::GetGraphType(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   return def.m_GraphType;
}

pgsTypes::LimitState  CAnalysisResultsChildFrame::GetLimitState(int graphIdx) const   
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);

#if defined _DEBUG
   GraphType graph_type = def.m_GraphType;
   ASSERT(graph_type == graphLimitState || 
          graph_type == graphDemand     || 
          graph_type == graphAllowable  || 
          graph_type == graphCapacity   ||
          graph_type == graphMinCapacity);
#endif

   return def.m_LoadType.LimitStateType;
}

ProductForceType CAnalysisResultsChildFrame::GetProductLoadCase(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   
#if defined _DEBUG
   GraphType graph_type = def.m_GraphType;
   ASSERT(graph_type == graphProduct);
#endif

   return def.m_LoadType.ProductLoadType;
}

LoadingCombination CAnalysisResultsChildFrame::GetCombinedLoadCase(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);

#if defined _DEBUG
   GraphType graph_type = def.m_GraphType;
   ASSERT(graph_type == graphCombined);
#endif

   return def.m_LoadType.CombinedLoadType;
}

COLORREF CAnalysisResultsChildFrame::GetGraphColor(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   return def.m_Color;
}

CString CAnalysisResultsChildFrame::GetGraphDataLabel(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   return def.m_Name;
}

pgsTypes::LiveLoadType CAnalysisResultsChildFrame::GetLiveLoadType(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   return def.m_LoadType.LiveLoadType;
}

VehicleIndexType CAnalysisResultsChildFrame::GetVehicleIndex(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   int id = SelectedGraphIndexToGraphID(graphIdx);
   const CAnalysisResultsGraphDefinition& def = m_GraphDefinitions.GetGraphDefinition(id);
   return def.m_VehicleIndex;
}

int CAnalysisResultsChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEAFOutputChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#if defined _EAF_USING_MFC_FEATURE_PACK
	if ( !m_SettingsBar.Create( _T("Title"), this, FALSE, IDD_ANALYSIS_RESULTS_BAR, CBRS_LEFT, IDD_ANALYSIS_RESULTS_BAR) )
#else
	if ( !m_SettingsBar.Create( this, IDD_ANALYSIS_RESULTS_BAR, CBRS_LEFT, IDD_ANALYSIS_RESULTS_BAR) )
#endif
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}
   }

#if defined _EAF_USING_MFC_FEATURE_PACK
   EnableDocking(CBRS_ALIGN_LEFT);
   m_SettingsBar.EnableDocking(CBRS_ALIGN_LEFT);
   m_SettingsBar.DockToFrameWindow(CBRS_ALIGN_LEFT);
#endif
	
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // fill stage bar since it won't change
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(0,0,pgsTypes::Temporary);

   CComboBox* pstg_ctrl = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   ASSERT(pstg_ctrl!=0);
   int idx = pstg_ctrl->AddString(_T("Casting Yard"));
   pstg_ctrl->SetItemData(idx,pgsTypes::CastingYard);

   idx = pstg_ctrl->AddString(_T("Girder Placement"));
   pstg_ctrl->SetItemData(idx,pgsTypes::GirderPlacement);

   if ( 0 < NtMax )
   {
      idx = pstg_ctrl->AddString(_T("Temporary Strand Removal"));
      pstg_ctrl->SetItemData(idx,pgsTypes::TemporaryStrandRemoval);
   }

   idx = pstg_ctrl->AddString(_T("Deck and Diaphragm Placement (Bridge Site 1)"));
   pstg_ctrl->SetItemData(idx,pgsTypes::BridgeSite1);

   idx = pstg_ctrl->AddString(_T("Superimposed Dead Loads (Bridge Site 2)"));
   pstg_ctrl->SetItemData(idx,pgsTypes::BridgeSite2);

   idx = pstg_ctrl->AddString(_T("Final with Live Load (Bridge Site 3)"));
   pstg_ctrl->SetItemData(idx,pgsTypes::BridgeSite3);

   pstg_ctrl->SetCurSel(0);

   // fill action bar since it won't change
   FillActionCtrl();

   // grid buttton
   CButton* pgrid_btn = (CButton*)m_SettingsBar.GetDlgItem(IDC_GRID);
   ASSERT(pgrid_btn!=0);
   pgrid_btn->SetCheck(m_Grid ? TRUE : FALSE);

   // Initialize the analysis type... match the "default"

   GET_IFACE2(pBroker,ISpecification,pSpec);
   m_AnalysisType = pSpec->GetAnalysisType();

   switch( m_AnalysisType )
   {
   case pgsTypes::Simple:
      idx = IDC_SIMPLE;
      break;

   case pgsTypes::Continuous:
      idx = IDC_SIMPLE2;
      break;

   case pgsTypes::Envelope:
      idx = IDC_SIMPLE3;
      break;

   default:
      ATLASSERT(false);
   }

   m_SettingsBar.CheckRadioButton(IDC_SIMPLE,IDC_SIMPLE3,idx );

   // load cases - dependent on stage
   CreateGraphDefinitions(); // must do this before filling the load control

   // fill control
   FillLoadCtrl();
   CString lc_name = m_GraphDefinitions.GetDefaultLoadCase(m_Stage);

   // select default item in list
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   ASSERT(pload_case_ctrl);
   idx = pload_case_ctrl->FindStringExact(0, lc_name);
   if ( idx == LB_ERR )
      idx = 0;
   pload_case_ctrl->SetCurSel(idx);

   return 0;
}

CAnalysisResultsView* CAnalysisResultsChildFrame::GetAnalysisResultsView() const
{
   CWnd* pwnd = GetActiveView();
   CAnalysisResultsView* pvw = (CAnalysisResultsView*)pwnd;
   ASSERT( pvw->IsKindOf(RUNTIME_CLASS(CAnalysisResultsView)) );
   ATLASSERT(pvw);
   return pvw;
}

void CAnalysisResultsChildFrame::UpdateViews()
{
   CAnalysisResultsView* pView = GetAnalysisResultsView();
   pView->UpdateFromBar();
}

void CAnalysisResultsChildFrame::Update(LPARAM lHint)
{
   UpdateBar();

   if ( 
        lHint == HINT_LIVELOADCHANGED || 
        lHint == HINT_BRIDGECHANGED   ||   // traffic barrier changed
        lHint == HINT_SPECCHANGED
      )
   {
      CreateGraphDefinitions();
      FillLoadCtrl();
   }
}

void CAnalysisResultsChildFrame::UpdateBar()
{
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   CComboBox* pstage_ctrl    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   CListBox* pload_case_ctrl = (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   CComboBox* paction_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_ACTION);
   ASSERT(pgirder_ctrl);
   ASSERT(pstage_ctrl);
   ASSERT(pload_case_ctrl);
   ASSERT(paction_ctrl);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);

   FillSpanCtrl();

   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   if ( pDoc == NULL )
      return;

   // girders
   GirderIndexType num_girders;
   if (m_SpanIdx == ALL_GIRDERS )
   {
      // "all" spans - get max number of girders
      num_girders = 0;
      SpanIndexType nSpans = pBridge->GetSpanCount();
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
         num_girders = _cpp_max(num_girders,nGirders);
      }
   }
   else
   {
      // get number of girders for this span
      num_girders = pBridge->GetGirderCount(m_SpanIdx);
   }

   ASSERT(num_girders!=0);
   int sel = pgirder_ctrl->GetCurSel();
   if (sel == CB_ERR) 
   {
      CSelection selection = pDoc->GetSelection();
      sel = (int)selection.GirderIdx;
      if ( sel == ALL_GIRDERS )
         sel = 0;

      if (num_girders <= GirderIndexType(sel) )
         sel = int(num_girders-1);
   }

   pgirder_ctrl->ResetContent();
   CString csv;
   for (GirderIndexType i=0; i<num_girders; i++)
   {
      csv.Format(_T("Girder %s"), LABEL_GIRDER(i));
      pgirder_ctrl->AddString(csv);
   }
   if (sel < int(num_girders))
      pgirder_ctrl->SetCurSel(sel);
   else
      pgirder_ctrl->SetCurSel(0);

   m_GirderIdx = pgirder_ctrl->GetCurSel();
   ASSERT(m_GirderIdx!=CB_ERR);
   if (m_GirderIdx == CB_ERR) 
      m_GirderIdx = 0;
}

void CAnalysisResultsChildFrame::OnGirderChanged()
{
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   ASSERT(pgirder_ctrl);

   int girdidx = pgirder_ctrl->GetCurSel();
//   ASSERT(girdidx!=CB_ERR);
   if (girdidx == CB_ERR) 
   {
      m_GirderIdx=0;
      pgirder_ctrl->SetCurSel(0);
   }
   else if (GirderIndexType(girdidx) != m_GirderIdx)
   {
      m_GirderIdx = GirderIndexType(girdidx);
   }

   CreateGraphDefinitions();
   FillLoadCtrl();
   UpdateViews();
}

void CAnalysisResultsChildFrame::OnSpanChanged()
{
   UpdateBar();
   UpdateViews();
}

void CAnalysisResultsChildFrame::OnStageChanged()
{
   // stages
   pgsTypes::Stage  stage;
   bool changed = false;

   CComboBox* pstage_ctrl    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   CListBox* pload_case_ctrl = (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   ASSERT(pstage_ctrl);
   ASSERT(pload_case_ctrl);

   int idx = pstage_ctrl->GetCurSel();
   ASSERT(idx!=CB_ERR);
   if (idx==CB_ERR) 
   {
      stage = pgsTypes::CastingYard;
      pstage_ctrl->SetCurSel(0);
   }
   else
   {
      stage = (pgsTypes::Stage)(pstage_ctrl->GetItemData(idx));
   }

   if (stage != m_Stage)
   {
      m_Stage = stage;

      // load cases - dependent on stage
      CString lc_name;
      idx = pload_case_ctrl->GetCurSel();
      if (idx==CB_ERR)
      {
         ASSERT(0);
         lc_name = m_GraphDefinitions.GetDefaultLoadCase(m_Stage);
      }
      else
         pload_case_ctrl->GetText(idx, lc_name);

//      FillSpanCtrl();

      // fill load control
      FillLoadCtrl();

      idx = pload_case_ctrl->FindStringExact(0, lc_name);
      if (idx==LB_ERR)
      {
         lc_name = m_GraphDefinitions.GetDefaultLoadCase(m_Stage);
         idx = pload_case_ctrl->FindStringExact(0, lc_name);

         if ( idx == LB_ERR )
            idx = 0;
      }
      pload_case_ctrl->SetCurSel(idx);

      UpdateBar();
      UpdateViews();
   }
}

void CAnalysisResultsChildFrame::OnLoadCaseChanged()
{
   UpdateBar();
   UpdateViews();
}

void CAnalysisResultsChildFrame::OnActionChanged()
{
   // action
   CComboBox* paction_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_ACTION);
   ASSERT(paction_ctrl);

   bool bActionChanged = false;
   int idx = paction_ctrl->GetCurSel();
   if (idx==0)
   {
      if (m_Action != actionMoment)
      {
         m_Action = actionMoment;
         bActionChanged = true;
      }
   }
   else if (idx==1)
   {
      if (m_Action != actionShear)
      {
         m_Action = actionShear;
         bActionChanged = true;
      }
   }
   else if (idx==2)
   {
      if (m_Action != actionDisplacement)
      {
         m_Action = actionDisplacement;
         bActionChanged = true;
      }
   }
   else if (idx==3)
   {
      if (m_Action != actionStress)
      {
         m_Action = actionStress;
         bActionChanged = true;
      }
   }
   else
   {
      ASSERT(0);
      m_Action = actionMoment;
      bActionChanged = true;
   }

   UpdateBar();

   if ( bActionChanged )
   {
      // load cases - dependent on action
      CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
      CString lc_name;
      idx = pload_case_ctrl->GetCurSel();
      if (idx == LB_ERR)
         lc_name = m_GraphDefinitions.GetDefaultLoadCase(m_Stage);
      else
         pload_case_ctrl->GetText(idx, lc_name);


      // fill load control
      FillLoadCtrl();

      idx = pload_case_ctrl->FindStringExact(0, lc_name);
      if (idx == LB_ERR)
      {
         lc_name = m_GraphDefinitions.GetDefaultLoadCase(m_Stage);
         idx = pload_case_ctrl->FindStringExact(0, lc_name);

         if ( idx == LB_ERR )
            idx = 0;
      }
      pload_case_ctrl->SetCurSel(idx);

      UpdateViews();
   }
}

void CAnalysisResultsChildFrame::OnGridClicked()
{
   m_Grid = !m_Grid;
   GetAnalysisResultsView()->UpdateGrid();
}

void CAnalysisResultsChildFrame::OnAnalysisTypeClicked()
{
   int id = m_SettingsBar.GetCheckedRadioButton(IDC_SIMPLE,IDC_SIMPLE3);
   pgsTypes::AnalysisType at;
   switch (id )
   {
   case IDC_SIMPLE:
      at = pgsTypes::Simple;
      break;

   case IDC_SIMPLE2:
      at = pgsTypes::Continuous;
      break;

   case IDC_SIMPLE3:
      at = pgsTypes::Envelope;
      break;

   default:
      ATLASSERT(false);
   }

   if ( at != m_AnalysisType )
   {
      m_AnalysisType = at;

      FillActionCtrl();
      UpdateViews();
   }
}

void CAnalysisResultsChildFrame::FillLoadCtrl()
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);
   ASSERT(pload_case_ctrl);

   int selCount = pload_case_ctrl->GetSelCount();
   CArray<int,int> selItemIndices;
   selItemIndices.SetSize(selCount);
   pload_case_ctrl->GetSelItems(selCount,selItemIndices.GetData());

   CStringArray selItems;
   int i;
   for ( i = 0; i < selCount; i++ )
   {
      CString strItem;
      pload_case_ctrl->GetText(selItemIndices[i],strItem);
      selItems.Add( strItem );
   }


   // refill control
   pload_case_ctrl->ResetContent();
   
   std::vector< std::pair<CString,int> > lcNames = m_GraphDefinitions.GetLoadCaseNames(m_Stage,m_Action);

   std::vector< std::pair<CString,int> >::iterator iter;
   for ( iter = lcNames.begin(); iter != lcNames.end(); iter++ )
   {
      CString lcName = (*iter).first;
      int     lcID   = (*iter).second;
      
      int idx = pload_case_ctrl->AddString(lcName);
      pload_case_ctrl->SetItemData(idx,lcID);
   }

   for ( i = 0; i < selCount; i++ )
   {
      CString strItem = selItems[i];
      int idx = pload_case_ctrl->FindString(-1,strItem);
      if ( idx != LB_ERR )
         pload_case_ctrl->SetSel(idx);
   }
}

void CAnalysisResultsChildFrame::FillSpanCtrl()
{
   CComboBox* pspan_ctrl     = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);

   SpanIndexType num_spans = pBridge->GetSpanCount();

   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   if ( pDoc == NULL )
      return;

   int sel = pspan_ctrl->GetCurSel();
   if (sel == CB_ERR)
   {
      CSelection selection = pDoc->GetSelection();
      sel = (int)selection.SpanIdx;
      if ( sel == ALL_SPANS )
         sel = 0;

      if (num_spans <= SpanIndexType(sel))
         sel = (int)(num_spans-1);
   }

   SpanIndexType selSpanIdx = SpanIndexType(pspan_ctrl->GetItemData(sel));

   pspan_ctrl->ResetContent();
   CString csv;
   int idx = -1;

   if ( m_Stage != pgsTypes::CastingYard )
   {
      idx = pspan_ctrl->AddString(_T("All"));
      pspan_ctrl->SetItemData(idx,ALL_SPANS);
   }

   for (SpanIndexType i=0; i<num_spans; i++)
   {
      csv.Format(_T("Span %i"), LABEL_SPAN(i));
      idx = pspan_ctrl->AddString(csv);
      pspan_ctrl->SetItemData(idx,i);

      if (i == selSpanIdx )
         sel = idx;
   }

   if (selSpanIdx < num_spans)
      pspan_ctrl->SetCurSel(sel);
   else
      pspan_ctrl->SetCurSel(0);

   idx = pspan_ctrl->GetCurSel();
   m_SpanIdx = (SpanIndexType)pspan_ctrl->GetItemData(idx);
}

void CAnalysisResultsChildFrame::FillActionCtrl()
{
   CComboBox* paction_ctrl = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_ACTION);

   int sel = paction_ctrl->GetCurSel();

   paction_ctrl->ResetContent();

   ASSERT(paction_ctrl!=0);
   paction_ctrl->AddString(_T("Moment"));
   paction_ctrl->AddString(_T("Shear"));
   paction_ctrl->AddString(_T("Displacement"));
   paction_ctrl->AddString(_T("Stress"));

   if ( sel == CB_ERR || paction_ctrl->GetCount() <= sel)
      paction_ctrl->SetCurSel(0);
   else
      paction_ctrl->SetCurSel(sel);

   OnActionChanged();
}

void CAnalysisResultsChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if ( bAddToTitle )
   {
      // TRICKY: we expect our view to provide is with text
      CView* pView = this->GetActiveView();
      if ( pView )
      {
         CString name;
         pView->GetWindowText(name);
		   AfxSetWindowText(m_hWnd, name);
      }
   }
}

int CAnalysisResultsChildFrame::SelectedGraphIndexToGraphID(int graphIdx) const
{
   CListBox* pload_case_ctrl= (CListBox*)m_SettingsBar.GetDlgItem(IDC_LOAD_CASE);

   CArray<int,int> array;
   array.SetSize(graphIdx+1);

   pload_case_ctrl->GetSelItems(graphIdx+1,array.GetData());

   int idx = array[graphIdx];

   int id = (int)pload_case_ctrl->GetItemData(idx);

   return id;
}

LRESULT CAnalysisResultsChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_ANALYSIS_RESULTS_VIEW );
   return TRUE;
}

void CAnalysisResultsChildFrame::CreateGraphDefinitions()
{
   m_GraphDefinitions.Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   int graphID = 0;

   SpanIndexType span = GetSpanIdx();
   GirderIndexType gdr  = GetGirderIdx();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   // determine if there are temporary strands or pedestrian load for any of the girders
   // for this sppan
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   SpanIndexType startSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpanIdx = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpanIdx+1);
   bool bTempStrand = false;
   bool bPedLoading = false;
   bool bSidewalk   = false;
   bool bShearKey   = false;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx < endSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      bPedLoading |= pProductLoads->HasPedestrianLoad(spanIdx,gdrIdx);
      bSidewalk   |= pProductLoads->HasSidewalkLoad(spanIdx,gdrIdx);
      bShearKey   |= pProductLoads->HasShearKeyLoad(spanIdx,gdrIdx);

      StrandIndexType NtMax = pStrandGeom->GetMaxStrands(spanIdx,gdrIdx,pgsTypes::Temporary);
      bTempStrand |= ( 0 < NtMax );
   }

   // Product Load Cases
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Girder"),         pftGirder,        true,  true,  false, false, false, false, ACTIONS_ALL,BROWN) );

   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Construction"),           pftConstruction,  false, false, false, true,  false, false, ACTIONS_ALL,INDIANRED) );
   }

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Slab"),           pftSlab,          false, false, false, true,  false, false, ACTIONS_ALL,SALMON) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Haunch"),       pftSlabPad,       false, false, false, true,  false, false, ACTIONS_ALL,SALMON) );

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Slab Panels"),           pftSlabPanel,          false, false, false, true,  false, false, ACTIONS_ALL,RED4) );
   }

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Diaphragm"),      pftDiaphragm,     false, false, false, true,  false, false, ACTIONS_ALL,ORANGE) );

   if (bShearKey)
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Shear Key"),      pftShearKey,      false, false, false, true,  false, false, ACTIONS_ALL,DARKSEAGREEN) );

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Traffic Barrier"),pftTrafficBarrier,false, false, false, false, true,  false, ACTIONS_ALL,TAN) );

   if ( bSidewalk )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Sidewalk"),pftSidewalk,false, false, false, false, true,  false, ACTIONS_ALL,PERU) );

   bool bFutureOverlay = pBridge->IsFutureOverlay();
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Overlay"),        pftOverlay,       false, false, false, false, !bFutureOverlay, bFutureOverlay, ACTIONS_ALL,VIOLET) );

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Prestress"),                                 graphPrestress,DODGERBLUE) );

   // User Defined Static Loads
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User DC"),        pftUserDC,        false, false, false, true,  true,  true,  ACTIONS_ALL,CORAL) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User DW"),        pftUserDW,        false, false, false, true,  true,  true,  ACTIONS_ALL,GOLD) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User Live Load"), pftUserLLIM,      false, false, false, false, false, true,  ACTIONS_ALL,MAROON) );

   // Individual Truck Responses

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   std::vector<pgsTypes::LiveLoadType> vLiveLoadTypes;
   vLiveLoadTypes.push_back(pgsTypes::lltDesign);
   vLiveLoadTypes.push_back(pgsTypes::lltFatigue);

   if ( bPermit )
      vLiveLoadTypes.push_back(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      vLiveLoadTypes.push_back(pgsTypes::lltLegalRating_Routine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      vLiveLoadTypes.push_back(pgsTypes::lltLegalRating_Special);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      vLiveLoadTypes.push_back(pgsTypes::lltPermitRating_Routine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      vLiveLoadTypes.push_back(pgsTypes::lltPermitRating_Special);

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = vLiveLoadTypes.begin(); iter != vLiveLoadTypes.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      std::_tstring strBase;
      switch(llType)
      {
      case pgsTypes::lltDesign:
         strBase = _T("Design");
         break;

      case pgsTypes::lltFatigue:
         strBase = _T("Fatigue");
         break;

      case pgsTypes::lltPermit:
         strBase = _T("Permit");
         break;

      case pgsTypes::lltLegalRating_Routine:
         strBase = _T("Legal Rating (Routine)");
         break;

      case pgsTypes::lltLegalRating_Special:
         strBase = _T("Legal Rating (Special)");
         break;

      case pgsTypes::lltPermitRating_Routine:
         strBase = _T("Permit Rating (Routine)");
         break;

      case pgsTypes::lltPermitRating_Special:
         strBase = _T("Permit Rating (Special)");
         break;

      default:
         ATLASSERT(false); // should never get here
      }

      std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,gdr);
      std::vector<std::_tstring>::iterator iter;
      VehicleIndexType vehicleIndex = 0;
      long colorIdx = 0;
      int action;
      switch(llType)
      {
      case pgsTypes::lltDesign:
      case pgsTypes::lltFatigue:
      case pgsTypes::lltLegalRating_Routine:
      case pgsTypes::lltLegalRating_Special:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DISPLACEMENT;
         break;

      default:
         ATLASSERT(false);
      }

      for ( iter = strLLNames.begin(); iter != strLLNames.end(); iter++, vehicleIndex++ )
      {
         std::_tstring strName = *iter;

         // skip the dummy live load
         if ( strName == _T("No Live Load Defined") )
            continue;

         std::_tstring strLLName = strBase + _T(" - ") + strName;
         CAnalysisResultsGraphDefinition def(graphID++,
                                             CString(strLLName.c_str()),
                                             llType,
                                             vehicleIndex,
                                             false, false, false, false, false, true,  // only bridge site 3
                                             action,
                                             clrTruck[colorIdx++]);

         m_GraphDefinitions.AddGraphDefinition(def);

         if ( nTruckColors <= colorIdx )
            colorIdx = 0;
      }

      std::_tstring strLLName = strBase + _T(" - LL+IM");
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, CString(strLLName.c_str()), llType,  -1, false, false, false, false, false, true,  ACTIONS_ALL, MAGENTA) );
   }



   // Combined Results
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("DC"),             lcDC,           true,  true, true, true,  true,  true,  ACTIONS_ALL,                RED) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("DW"),             lcDW,           false, false, false, true,  true,  true,  ACTIONS_ALL,                GOLDENROD) );


   if ( bPedLoading )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("PL"),   pgsTypes::lltPedestrian,           false, false, false, false, false, true,  ACTIONS_ALL, FIREBRICK) );
   }

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Design)"),   pgsTypes::lltDesign,           false, false, false, false, false, true,  ACTIONS_ALL,                MAGENTA) );

   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Fatigue)"),   pgsTypes::lltFatigue,           false, false, false, false, false, true,  ACTIONS_ALL,           PURPLE) );
   }

   if (bPermit)
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit)"),   pgsTypes::lltPermit,           false, false, false, false, false, true,  ACTIONS_FORCE_DISPLACEMENT, MEDIUMPURPLE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Routine)"),   pgsTypes::lltLegalRating_Routine,           false, false, false, false, false, true,  ACTIONS_ALL,                HOTPINK) );

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Special)"),   pgsTypes::lltLegalRating_Special,           false, false, false, false, false, true,  ACTIONS_ALL,                DARKSEAGREEN4) );

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Routine)"),   pgsTypes::lltPermitRating_Routine,           false, false, false, false, false, true,  ACTIONS_ALL,                TOMATO) );

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Special)"),   pgsTypes::lltPermitRating_Special,           false, false, false, false, false, true,  ACTIONS_ALL,                DARKORANGE) );

   // Limit States and Capacities
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I (Design)"),            pgsTypes::ServiceI,                 true,  true, true,  true,  true,  true,  ACTIONS_STRESS_ONLY,  SLATEBLUE) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA (Design)"),           pgsTypes::ServiceIA,                false, false, false, false, false, true,  ACTIONS_STRESS_ONLY,  NAVY) );

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III (Design)"),          pgsTypes::ServiceIII,               false, false, false, false, false, true,  ACTIONS_STRESS_ONLY,  ROYALBLUE) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design)"),           pgsTypes::StrengthI,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, CADETBLUE) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design)"),  pgsTypes::StrengthI, graphCapacity, false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   CADETBLUE) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I"),           pgsTypes::FatigueI,                false, false, false, false, false, true,  ACTIONS_STRESS_ONLY,  CHOCOLATE) );

   if ( bPermit )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Permit)"),          pgsTypes::StrengthII,               false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, BROWN) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Permit)"), pgsTypes::StrengthII,graphCapacity, false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   BROWN) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Inventory)"),            pgsTypes::StrengthI_Inventory,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, DEEPPINK4) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Inventory)"),  pgsTypes::StrengthI_Inventory, graphCapacity,  false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   DEEPPINK4) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Operating)"),            pgsTypes::StrengthI_Operating,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, THISTLE) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Operating)"),  pgsTypes::StrengthI_Operating, graphCapacity,  false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   THISTLE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Routine)"),            pgsTypes::StrengthI_LegalRoutine,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, SLATEBLUE) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Legal Rating, Routine)"),  pgsTypes::StrengthI_LegalRoutine, graphCapacity,  false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   SLATEBLUE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Special)"),            pgsTypes::StrengthI_LegalSpecial,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, DARKORCHID) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Special)"),  pgsTypes::StrengthI_LegalSpecial, graphCapacity, false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   DARKORCHID) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Routine Permit Rating)"),            pgsTypes::StrengthII_PermitRoutine,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, MEDIUMSLATEBLUE) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Routine Permit Rating)"),  pgsTypes::StrengthII_PermitRoutine, graphCapacity,  false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   MEDIUMSLATEBLUE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Special Permit Rating)"),            pgsTypes::StrengthII_PermitSpecial,                false, false, false, false, false, true,  ACTIONS_MOMENT_SHEAR, DARKSLATEGRAY) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Special Permit Rating)"),  pgsTypes::StrengthII_PermitSpecial, graphCapacity,  false, false, false, false, false, true,  ACTIONS_SHEAR_ONLY,   DARKSLATEGRAY) );
   }

   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Moment Capacity"),      pgsTypes::StrengthI, graphCapacity, false, false, false, false, false, true,  ACTIONS_MOMENT_ONLY,  CHOCOLATE) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Min Moment Capacity (Design)"),  pgsTypes::StrengthI, graphMinCapacity, false, false, false, false, false, true,  ACTIONS_MOMENT_ONLY,  SADDLEBROWN) );

   if ( bPermit )
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Min Moment Capacity (Permit)"),  pgsTypes::StrengthII, graphMinCapacity, false, false, false, false, false, true,  ACTIONS_MOMENT_ONLY,  SADDLEBROWN) );

   // Demand and Allowable
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Demand (Design)"),     pgsTypes::ServiceI,  graphDemand,    true,  false,  true,  true,  true,  true, RGB(139, 26, 26)) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Allowable (Design)"),  pgsTypes::ServiceI,  graphAllowable, true,  false,  true,  true,  true,  true, RGB(139, 26, 26)) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Demand (Design)"),    pgsTypes::ServiceIA, graphDemand,    false, false, false, false, false, true, RGB(255, 69,  0)) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Allowable (Design)"), pgsTypes::ServiceIA, graphAllowable, false, false, false, false, false, true, RGB(255, 69,  0)) );
   }
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design)"),   pgsTypes::ServiceIII,graphDemand,    false, false, false, false, false, true, RGB(205, 16,118)) );
   m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Design)"),pgsTypes::ServiceIII,graphAllowable, false, false, false, false, false, true, RGB(205, 16,118)) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Demand"),    pgsTypes::FatigueI, graphDemand,    false, false, false, false, false, true, RGB(255, 69,  0)) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Allowable"), pgsTypes::FatigueI, graphAllowable, false, false, false, false, false, true, RGB(255, 69,  0)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design Rating, Inventory)"),   pgsTypes::ServiceIII_Inventory,graphDemand,    false, false, false, false, false, true, RGB(118,16,205)) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Design Rating, Inventory)"),pgsTypes::ServiceIII_Inventory,graphAllowable, false, false, false, false, false, true, RGB(118,16,205)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Routine)"),   pgsTypes::ServiceIII_LegalRoutine,graphDemand,    false, false, false, false, false, true, RGB( 16,205,118)) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Legal Rating, Routine)"),pgsTypes::ServiceIII_LegalRoutine,graphAllowable, false, false, false, false, false, true, RGB( 16,205,118)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Special)"),   pgsTypes::ServiceIII_LegalSpecial,graphDemand,    false, false, false, false, false, true, RGB(205, 16,118)) );
      m_GraphDefinitions.AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Legal Rating, Special)"),pgsTypes::ServiceIII_LegalSpecial,graphAllowable,  false, false, false, false, false, true, RGB(205, 16,118)) );
   }
}

#ifdef _DEBUG
void CAnalysisResultsChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CEAFOutputChildFrame::AssertValid();
}

void CAnalysisResultsChildFrame::Dump(CDumpContext& dc) const
{
	CEAFOutputChildFrame::Dump(dc);
}
#endif //_DEBUG
