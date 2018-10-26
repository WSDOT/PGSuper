///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "AnalysisResultsGraphController.h"
#include <Graphing\GraphingTypes.h>
#include <Graphing\AnalysisResultsGraphBuilder.h>

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <Hints.h>

IMPLEMENT_DYNCREATE(CAnalysisResultsGraphController,CGirderGraphControllerBase)

CAnalysisResultsGraphController::CAnalysisResultsGraphController():
CGirderGraphControllerBase(true/*false*//*exclude ALL_GROUPS*/),
m_GraphMode(GRAPH_MODE_LOADING),
m_ActionType(actionMoment),
m_AnalysisType(pgsTypes::Simple)
{
}

int CAnalysisResultsGraphController::GetGraphMode()
{
   return m_GraphMode;
}

BEGIN_MESSAGE_MAP(CAnalysisResultsGraphController, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CAnalysisResultsGraphController)
   ON_CBN_SELCHANGE( IDC_MODE, OnModeChanged )
   ON_CBN_SELCHANGE( IDC_ACTION, OnActionChanged )
   ON_CBN_SELCHANGE( IDC_DROP_LIST, OnDropDownChanged )
   ON_LBN_SELCHANGE( IDC_SELECT_LIST, OnSelectListChanged )

   ON_BN_CLICKED(IDC_INCREMENTAL,OnPlotTypeClicked)
   ON_BN_CLICKED(IDC_CUMULATIVE,OnPlotTypeClicked)

   ON_BN_CLICKED(IDC_TOP_GIRDER, OnStress)
   ON_BN_CLICKED(IDC_BOTTOM_GIRDER, OnStress)
   ON_BN_CLICKED(IDC_TOP_DECK, OnStress)
   ON_BN_CLICKED(IDC_BOTTOM_DECK, OnStress)

   ON_BN_CLICKED(IDC_ELEV_ADJUSTMENT,OnElevAdjustment)

   ON_BN_CLICKED(IDC_SIMPLE,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE2,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE3,OnAnalysisTypeClicked)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAnalysisResultsGraphController::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();

   FillModeCtrl();
   FillActionTypeCtrl();
   FillDropListCtrl(false);
   FillSelectListCtrl(false);

   CheckRadioButton(IDC_INCREMENTAL,IDC_CUMULATIVE,IDC_CUMULATIVE);
   CheckDlgButton(IDC_TOP_GIRDER,BST_CHECKED);
   CheckDlgButton(IDC_BOTTOM_GIRDER,BST_CHECKED);

   UpdateListInfo();
   UpdateStressControls();
   UpdateAnalysisType();
   UpdateElevAdjustment();

   OnModeChanged();

   return TRUE;
}

ActionType CAnalysisResultsGraphController::GetActionType()
{
   return m_ActionType;
}

ResultsType CAnalysisResultsGraphController::GetResultsType()
{
   return IsDlgButtonChecked(IDC_INCREMENTAL) == BST_CHECKED ? rtIncremental : rtCumulative;
}

bool CAnalysisResultsGraphController::PlotStresses(pgsTypes::StressLocation stressLocation)
{
   UINT nID[4] = {IDC_BOTTOM_GIRDER,IDC_TOP_GIRDER,IDC_BOTTOM_DECK,IDC_TOP_DECK};
   return IsDlgButtonChecked(nID[stressLocation]) == BST_CHECKED ? true : false;
}

IntervalIndexType CAnalysisResultsGraphController::GetInterval()
{
   if ( m_GraphMode == GRAPH_MODE_INTERVAL )
   {
      return INVALID_INDEX; // graphing multiple intervals
   }

   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   int curSel = pcbIntervals->GetCurSel();
   IntervalIndexType intervalIdx = (IntervalIndexType)pcbIntervals->GetItemData(curSel);
   return intervalIdx;
}

std::vector<IntervalIndexType> CAnalysisResultsGraphController::GetSelectedIntervals()
{
   std::vector<IntervalIndexType> vIntervals;

   if (m_GraphMode == GRAPH_MODE_LOADING )
   {
      vIntervals.push_back(GetInterval());
   }
   else
   {
      CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_SELECT_LIST);

      // capture the current selection
      int selCount = plbIntervals->GetSelCount();
      CArray<int,int> selItemIndices;
      selItemIndices.SetSize(selCount);
      plbIntervals->GetSelItems(selCount,selItemIndices.GetData());

      int i;
      for ( i = 0; i < selCount; i++ )
      {
         IntervalIndexType intervalIdx = (IntervalIndexType)plbIntervals->GetItemData(selItemIndices[i]);
         vIntervals.push_back(intervalIdx);
      }
   }

   return vIntervals;
}

bool CAnalysisResultsGraphController::IncludeElevationAdjustment()
{
   return IsDlgButtonChecked(IDC_ELEV_ADJUSTMENT) == BST_CHECKED ? true : false;
}

pgsTypes::AnalysisType CAnalysisResultsGraphController::GetAnalysisType()
{
   return m_AnalysisType;
}

IDType CAnalysisResultsGraphController::SelectedGraphIndexToGraphID(IndexType graphIdx)
{
   if (m_GraphMode == GRAPH_MODE_LOADING )
   {
      CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);

      CArray<int,int> array;
      array.SetSize(graphIdx+1);

      plbLoading->GetSelItems((int)(graphIdx+1),(LPINT)array.GetData());

      int idx = array[graphIdx];

      IDType id = (IDType)plbLoading->GetItemData(idx);

      return id;
   }
   else
   {
      CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
      int curSel = pcbLoading->GetCurSel();
      IDType id = (IDType)pcbLoading->GetItemData(curSel);
      return id;
   }
}

IndexType CAnalysisResultsGraphController::GetMaxGraphCount()
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   return (IndexType)(plbSelectList->GetCount());
}

IndexType CAnalysisResultsGraphController::GetGraphCount()
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   IndexType count = plbSelectList->GetSelCount();

   return count;
}

void CAnalysisResultsGraphController::OnModeChanged()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   int curSel = pcbMode->GetCurSel();
   if ( m_GraphMode != curSel )
   {
      m_GraphMode = curSel;
      FillDropListCtrl(false);
      FillSelectListCtrl(false);
      UpdateListInfo();
      UpdateGraph();
   }
}

void CAnalysisResultsGraphController::OnActionChanged()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();
   ActionType actionType = ActionType(pcbAction->GetItemData(curSel));
   if ( m_ActionType == actionType )
   {
      // action type didn't change
      return;
   }

   m_ActionType = actionType;

   UpdateElevAdjustment();
   UpdateStressControls();

   // the loads that are available to plot for a particular action depend on the
   // current action type... update the loading list for this new action
   if ( GetGraphMode() == GRAPH_MODE_LOADING )
   {
      FillSelectListCtrl(true);
   }
   else
   {
      FillDropListCtrl(true);
   }

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnDropDownChanged()
{
   if ( GetGraphMode() == GRAPH_MODE_LOADING )
   {
      FillSelectListCtrl(true);
   }
   else
   {
      UpdateResultsType();
      FillDropListCtrl(true);
   }

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnSelectListChanged()
{
   if ( GetGraphMode() == GRAPH_MODE_LOADING) 
   {
      UpdateResultsType();
   }

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnPlotTypeClicked()
{
   UpdateGraph();
}

void CAnalysisResultsGraphController::OnStress()
{
   UpdateGraph();
}

void CAnalysisResultsGraphController::OnElevAdjustment()
{
   UpdateGraph();
}

void CAnalysisResultsGraphController::OnAnalysisTypeClicked()
{
   int id = GetCheckedRadioButton(IDC_SIMPLE,IDC_SIMPLE3);
   pgsTypes::AnalysisType analysisType;
   switch (id)
   {
   case IDC_SIMPLE:
      analysisType = pgsTypes::Simple;
      break;

   case IDC_SIMPLE2:
      analysisType = pgsTypes::Continuous;
      break;

   case IDC_SIMPLE3:
      analysisType = pgsTypes::Envelope;
      break;

   default:
      ATLASSERT(false); // is there a new analysis type?
   }

   if ( m_AnalysisType == analysisType )
   {
      return;
   }

   m_AnalysisType = analysisType;

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);

   if ( 
        lHint == HINT_LIVELOADCHANGED || 
        lHint == HINT_BRIDGECHANGED   ||
        lHint == HINT_SPECCHANGED     ||
        lHint == HINT_RATINGSPECCHANGED
      )
   {
      ((CAnalysisResultsGraphBuilder*)GetGraphBuilder())->UpdateGraphDefinitions();
      FillDropListCtrl(true);
      FillSelectListCtrl(true);
      UpdateGraph();
   }
}

void CAnalysisResultsGraphController::FillModeCtrl()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   pcbMode->AddString(_T("Plot by Interval"));
   pcbMode->AddString(_T("Plot by Loading"));
   pcbMode->SetCurSel(m_GraphMode);
}

void CAnalysisResultsGraphController::FillActionTypeCtrl()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();

   pcbAction->ResetContent();

   pcbAction->SetItemData(pcbAction->AddString(_T("Moment")),     actionMoment);
   pcbAction->SetItemData(pcbAction->AddString(_T("Shear")),      actionShear);
   pcbAction->SetItemData(pcbAction->AddString(_T("Deflection")), actionDeflection);
   pcbAction->SetItemData(pcbAction->AddString(_T("Rotation")),   actionRotation);
   pcbAction->SetItemData(pcbAction->AddString(_T("Stress")),     actionStress);
   pcbAction->SetItemData(pcbAction->AddString(_T("Reaction")),   actionReaction);

   if ( curSel == CB_ERR || pcbAction->GetCount() <= curSel)
   {
      pcbAction->SetCurSel(0);
   }
   else
   {
      pcbAction->SetCurSel(curSel);
   }

   m_ActionType = (ActionType)pcbAction->GetItemData(pcbAction->GetCurSel());
}

void CAnalysisResultsGraphController::FillDropListCtrl(bool bRetainSelection)
{
   // NOTE: the two calls in this method are supposed to look backwards
   // in Interval mode, the drop down list contains loadings and visa-versa
   if ( GetGraphMode() == GRAPH_MODE_INTERVAL )
   {
      FillDropListCtrl_Loadings(bRetainSelection);
   }
   else
   {
      FillDropListCtrl_Intervals(bRetainSelection);
   }
}

void CAnalysisResultsGraphController::FillDropListCtrl_Intervals(bool bRetainSelection)
{
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_DROP_LIST);

   int curSel = pcbIntervals->GetCurSel();

   pcbIntervals->ResetContent();

   CGirderKey girderKey(GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(girderKey,intervalIdx));
      int idx = pcbIntervals->AddString(strInterval);
      pcbIntervals->SetItemData(idx,intervalIdx);
   }

   if ( bRetainSelection )
   {
      curSel = pcbIntervals->SetCurSel(curSel);
      if ( curSel == CB_ERR )
      {
         pcbIntervals->SetCurSel(0);
      }
   }
   else
   {
      pcbIntervals->SetCurSel(0);
   }
}

void CAnalysisResultsGraphController::FillDropListCtrl_Loadings(bool bRetainSelection)
{
   CComboBox* pcbLoadings = (CComboBox*)GetDlgItem(IDC_DROP_LIST);

   int curSel = pcbLoadings->GetCurSel();

   pcbLoadings->ResetContent();

   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();
   
   std::vector<std::pair<std::_tstring,IDType>> vLoadings( pGraphBuilder->GetLoadings(INVALID_INDEX,GetActionType()) );
   std::vector<std::pair<std::_tstring,IDType>>::iterator iter(vLoadings.begin());
   std::vector<std::pair<std::_tstring,IDType>>::iterator end(vLoadings.end());
   for ( ; iter != end; iter++ )
   {
      std::_tstring& lcName( (*iter).first );
      IDType  graphID   = (*iter).second;
      
      int idx = pcbLoadings->AddString(lcName.c_str());
      pcbLoadings->SetItemData(idx,(DWORD_PTR)graphID);
   }

   if ( bRetainSelection )
   {
      curSel = pcbLoadings->SetCurSel(curSel);
      if ( curSel == CB_ERR )
      {
         pcbLoadings->SetCurSel(0);
      }
   }
   else
   {
      pcbLoadings->SetCurSel(0);
   }
}

void CAnalysisResultsGraphController::FillSelectListCtrl(bool bRetainSelection)
{
   if ( GetGraphMode() == GRAPH_MODE_INTERVAL )
   {
      GetDlgItem(IDC_SELECT_LIST_TITLE)->SetWindowText(_T("Interval"));
      FillSelectListCtrl_Intervals(bRetainSelection);
   }
   else
   {
      GetDlgItem(IDC_SELECT_LIST_TITLE)->SetWindowText(_T("Loading"));
      FillSelectListCtrl_Loadings(bRetainSelection);
   }
}

void CAnalysisResultsGraphController::FillSelectListCtrl_Intervals(bool bRetainSelection)
{
   CListBox* plbIntervals = (CListBox*)GetDlgItem(IDC_SELECT_LIST);

   // capture the current selection
   int selCount = 0;
   CStringArray selItems;
   if ( bRetainSelection )
   {
      selCount = plbIntervals->GetSelCount();
      CArray<int,int> selItemIndices;
      selItemIndices.SetSize(selCount);
      plbIntervals->GetSelItems(selCount,selItemIndices.GetData());

      int i;
      for ( i = 0; i < selCount; i++ )
      {
         CString strItem;
         plbIntervals->GetText(selItemIndices[i],strItem);
         selItems.Add( strItem );
      }
   }

   // clear the control
   plbIntervals->ResetContent();

   CGirderKey girderKey(GetGirderKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      CString str;
      CGirderKey thisGirderKey(girderKey);
      if ( thisGirderKey.groupIndex == ALL_GROUPS )
      {
         thisGirderKey.groupIndex = 0;
      }

      str.Format(_T("%d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(thisGirderKey,intervalIdx));
      int idx = plbIntervals->AddString(str);
      plbIntervals->SetItemData(idx,intervalIdx);
   }

   // reselect anything that was previously selected
   if ( bRetainSelection )
   {
      for ( int i = 0; i < selCount; i++ )
      {
         CString strItem = selItems[i];
         int idx = plbIntervals->FindStringExact(-1,strItem);
         if ( idx != LB_ERR )
         {
            plbIntervals->SetSel(idx);
         }
      }
   }
}

void CAnalysisResultsGraphController::FillSelectListCtrl_Loadings(bool bRetainSelection)
{
   CListBox* plbLoadings = (CListBox*)GetDlgItem(IDC_SELECT_LIST);

   // capture the current selection
   int selCount = 0;
   CStringArray selItems;
   if ( bRetainSelection )
   {
      selCount = plbLoadings->GetSelCount();
      CArray<int,int> selItemIndices;
      selItemIndices.SetSize(selCount);
      plbLoadings->GetSelItems(selCount,selItemIndices.GetData());

      int i;
      for ( i = 0; i < selCount; i++ )
      {
         CString strItem;
         plbLoadings->GetText(selItemIndices[i],strItem);
         selItems.Add( strItem );
      }
   }

   // clear the control
   plbLoadings->ResetContent();

   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();
   
   std::vector<std::pair<std::_tstring,IDType>> vLoadings( pGraphBuilder->GetLoadings(GetInterval(),GetActionType()) );

   std::vector<std::pair<std::_tstring,IDType>>::iterator iter(vLoadings.begin());
   std::vector<std::pair<std::_tstring,IDType>>::iterator end(vLoadings.end());
   for ( ; iter != end; iter++ )
   {
      std::_tstring& lcName( (*iter).first );
      IDType   graphID = (*iter).second;
      
      int idx = plbLoadings->AddString(lcName.c_str());
      plbLoadings->SetItemData(idx,(DWORD_PTR)graphID);
   }

   if ( bRetainSelection )
   {
      // reselect anything that was previously selected
      for ( int i = 0; i < selCount; i++ )
      {
         CString strItem = selItems[i];
         int idx = plbLoadings->FindStringExact(-1,strItem);
         if ( idx != LB_ERR )
         {
            plbLoadings->SetSel(idx);
         }
      }
   }
}

void CAnalysisResultsGraphController::UpdateStressControls()
{
   // only show the stress check boxes if the action type is actionStress
   int nCmdShow = (m_ActionType == actionStress ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_GIRDER   )->ShowWindow(nCmdShow);
   GetDlgItem(IDC_BOTTOM_GIRDER)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_TOP_DECK     )->ShowWindow(nCmdShow);
   GetDlgItem(IDC_BOTTOM_DECK  )->ShowWindow(nCmdShow);
}

void CAnalysisResultsGraphController::UpdateListInfo()
{
   CString strType(GetGraphMode() == GRAPH_MODE_INTERVAL ? _T("intervals") : _T("loadings"));
   CString strHint;
   strHint.Format(_T("Hold CTRL key to select multiple %s\nHold SHIFT key to select range of %s"),strType,strType);
   GetDlgItem(IDC_LIST_INFO)->SetWindowText(strHint);
}

bool IsCumulativeOnlyGraphType(GraphType graphType)
{
   return (graphType == graphLimitState ||
           graphType == graphDemand     ||
           graphType == graphAllowable  ||
           graphType == graphCapacity   ||
           graphType == graphMinCapacity) ? true : false;
}

void CAnalysisResultsGraphController::UpdateResultsType()
{
   bool bCumulativeOnly = false;
   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();
   if ( GetGraphMode() == GRAPH_MODE_INTERVAL )
   {
      IDType graphID = SelectedGraphIndexToGraphID(0);
      GraphType graphType = pGraphBuilder->GetGraphType(graphID);
      bCumulativeOnly = IsCumulativeOnlyGraphType(graphType);
   }
   else
   {
      IndexType nGraphs = GetGraphCount();
      for ( IndexType graphIdx = 0; graphIdx < nGraphs; graphIdx++ )
      {
         IDType graphID = SelectedGraphIndexToGraphID(graphIdx);
         GraphType graphType = pGraphBuilder->GetGraphType(graphID);
         if ( IsCumulativeOnlyGraphType(graphType) )
         {
            bCumulativeOnly = true;
         }
      }
   }

   if ( bCumulativeOnly )
   {
      CheckRadioButton(IDC_INCREMENTAL,IDC_CUMULATIVE,IDC_CUMULATIVE);
      GetDlgItem(IDC_INCREMENTAL)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_INCREMENTAL)->EnableWindow(TRUE);
   }
}

void CAnalysisResultsGraphController::UpdateElevAdjustment()
{
   ActionType actionType = GetActionType();
   if ( actionType == actionDeflection || actionType == actionRotation )
   {
      if ( actionType == actionDeflection )
      {
         GetDlgItem(IDC_ELEV_ADJUSTMENT)->SetWindowText(_T("Include Elevation Adjustment"));
      }
      else
      {
         GetDlgItem(IDC_ELEV_ADJUSTMENT)->SetWindowText(_T("Include Slope Adjustment"));
      }

      GetDlgItem(IDC_ELEV_ADJUSTMENT)->ShowWindow(SW_SHOW);
   }
   else
   {
      GetDlgItem(IDC_ELEV_ADJUSTMENT)->ShowWindow(SW_HIDE);
   }
}

void CAnalysisResultsGraphController::UpdateAnalysisType()
{
   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      GetDlgItem(IDC_ANALYSISTYPE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SIMPLE)->ShowWindow(SW_HIDE);  // Simple Span
      GetDlgItem(IDC_SIMPLE2)->ShowWindow(SW_HIDE); // Simple Spans made Continuous
      GetDlgItem(IDC_SIMPLE3)->ShowWindow(SW_HIDE); // Envelope
      m_AnalysisType = pgsTypes::Continuous;
   }
   else
   {
      GET_IFACE(ISpecification,pSpec);
      m_AnalysisType = pSpec->GetAnalysisType();

      int idx;
      switch( m_AnalysisType )
      {
      case pgsTypes::Simple:
         idx = IDC_SIMPLE;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(FALSE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(FALSE);
         break;

      case pgsTypes::Continuous:
         idx = IDC_SIMPLE2;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(FALSE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(FALSE);
         break;

      case pgsTypes::Envelope:
         idx = IDC_SIMPLE3;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(TRUE);
         break;

      default:
         ATLASSERT(false); // is there a new analysis type?
      }

      CheckRadioButton(IDC_SIMPLE,IDC_SIMPLE3,idx );
   }
}

IntervalIndexType CAnalysisResultsGraphController::GetFirstInterval()
{
   GET_IFACE(IIntervals,pIntervals);
   CGirderKey girderKey(GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // want the first segment release interval... which is one interval after strand stressing
   return pIntervals->GetFirstStressStrandInterval(girderKey)+1;
}

IntervalIndexType CAnalysisResultsGraphController::GetLastInterval()
{
   GET_IFACE(IIntervals,pIntervals);
   CGirderKey girderKey(GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }
   return pIntervals->GetIntervalCount(girderKey) - 1;
}

#ifdef _DEBUG
void CAnalysisResultsGraphController::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CAnalysisResultsGraphController::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
