///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "SegmentAnalysisResultsGraphController.h"
#include <Graphing\GraphingTypes.h>
#include <Graphing\SegmentAnalysisResultsGraphBuilder.h>

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>

#include <Hints.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CSegmentAnalysisResultsGraphController,CSegmentGraphControllerBase)

CSegmentAnalysisResultsGraphController::CSegmentAnalysisResultsGraphController():
CSegmentGraphControllerBase(false)//*exclude ALL_GROUPS*/)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
}

void CSegmentAnalysisResultsGraphController::SetGraphMode(CSegmentAnalysisResultsGraphController::GraphModeType mode)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_MODE);
   pCB->SetCurSel((CSegmentAnalysisResultsGraphController::GraphModeType)(pCB->GetItemData(0)) == mode ? 0 : 1);
   FillDropListCtrl(false);
   FillSelectListCtrl(false);
   FillActionTypeCtrl();
   UpdateListInfo();
   OnIntervalsChanged();
   UpdateGraph();
}

CSegmentAnalysisResultsGraphController::GraphModeType CSegmentAnalysisResultsGraphController::GetGraphMode() const
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_MODE);
   int curSel = pCB->GetCurSel();
   return (CSegmentAnalysisResultsGraphController::GraphModeType)(pCB->GetItemData(curSel));
}

BEGIN_MESSAGE_MAP(CSegmentAnalysisResultsGraphController, CSegmentGraphControllerBase)
	//{{AFX_MSG_MAP(CSegmentAnalysisResultsGraphController)
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

   ON_BN_CLICKED(IDC_SIMPLE,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE2,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE3,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_PRECAMBER,OnUnrecoverableDefl)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CSegmentAnalysisResultsGraphController::DoDataExchange(CDataExchange* pDX)
{
   DDX_Control(pDX, IDC_DROP_LIST, m_cbDropList);

   CSegmentGraphControllerBase::DoDataExchange(pDX);
}

BOOL CSegmentAnalysisResultsGraphController::OnInitDialog()
{
   CSegmentGraphControllerBase::OnInitDialog();

   FillModeCtrl();
   FillActionTypeCtrl();
   FillDropListCtrl(false);
   FillSelectListCtrl(false);

   CheckRadioButton(IDC_INCREMENTAL,IDC_CUMULATIVE,IDC_CUMULATIVE);
   CheckDlgButton(IDC_TOP_GIRDER,BST_CHECKED);
   CheckDlgButton(IDC_BOTTOM_GIRDER,BST_CHECKED);
   CheckDlgButton(IDC_PRECAMBER,BST_CHECKED);

   UpdateListInfo();
   UpdateStressControls();
   UpdateUnrecoverableDeflAdjustment();

   return TRUE;
}

void CSegmentAnalysisResultsGraphController::SetResultsType(ResultsType resultsType)
{
   CheckRadioButton(IDC_INCREMENTAL, IDC_CUMULATIVE, (resultsType == rtIncremental ? IDC_INCREMENTAL : IDC_CUMULATIVE));
   UpdateGraph();
}

ResultsType CSegmentAnalysisResultsGraphController::GetResultsType() const
{
   return IsDlgButtonChecked(IDC_INCREMENTAL) == BST_CHECKED ? rtIncremental : rtCumulative;
}

void CSegmentAnalysisResultsGraphController::SetActionType(ActionType actionType)
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int nActions = pcbAction->GetCount();
   for (int idx = 0; idx < nActions; idx++)
   {
      if ((ActionType)(pcbAction->GetItemData(idx)) == actionType)
      {
         pcbAction->SetCurSel(idx);

         UpdateStressControls();
         UpdateResultsType();
         UpdateUnrecoverableDeflAdjustment();

         // the loads that are available to plot for a particular action depend on the
         // current action type... update the loading list for this new action
         if (GetGraphMode() == Loading)
         {
            FillSelectListCtrl(true);
         }
         else
         {
            FillDropListCtrl(true);
         }

         UpdateGraph();

         return;
      }
   }

}

ActionType CSegmentAnalysisResultsGraphController::GetActionType() const
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);
   int curSel = pcbAction->GetCurSel();
   return (ActionType)(pcbAction->GetItemData(curSel));
}

void CSegmentAnalysisResultsGraphController::PlotStresses(pgsTypes::StressLocation stressLocation, bool bPlot)
{
   static UINT nIDC[2] = { IDC_BOTTOM_GIRDER,IDC_TOP_GIRDER };
   if (PlotStresses(stressLocation) != bPlot)
   {
      CheckDlgButton(nIDC[stressLocation], bPlot ? BST_CHECKED : BST_UNCHECKED);
      UpdateGraph();
   }
}

bool CSegmentAnalysisResultsGraphController::PlotStresses(pgsTypes::StressLocation stressLocation) const
{
   static UINT nIDC[2] = {IDC_BOTTOM_GIRDER,IDC_TOP_GIRDER};
   return IsDlgButtonChecked(nIDC[stressLocation]) == BST_CHECKED ? true : false;
}

IntervalIndexType CSegmentAnalysisResultsGraphController::GetInterval() const
{
   if ( GetGraphMode() == Interval )
   {
      return INVALID_INDEX; // graphing multiple intervals
   }

   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   int curSel = pcbIntervals->GetCurSel();
   IntervalIndexType intervalIdx;
   if (curSel != CB_ERR)
   {
      intervalIdx = (IntervalIndexType)pcbIntervals->GetItemData(curSel);
   }
   else
   {
      intervalIdx = GetFirstInterval(); // this is selected by default at startup
   }

   return intervalIdx;
}

std::vector<IntervalIndexType> CSegmentAnalysisResultsGraphController::GetSelectedIntervals() const
{
   std::vector<IntervalIndexType> vIntervals;

   if (GetGraphMode() == Loading )
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

void CSegmentAnalysisResultsGraphController::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   int nIDC = IDC_SIMPLE + (int)(analysisType);
   CheckRadioButton(IDC_SIMPLE, IDC_SIMPLE3, nIDC);
   UpdateGraph();
}

pgsTypes::AnalysisType CSegmentAnalysisResultsGraphController::GetAnalysisType() const
{
   int nIDC = GetCheckedRadioButton(IDC_SIMPLE, IDC_SIMPLE3);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   return (pgsTypes::AnalysisType)(nIDC - IDC_SIMPLE);
}

IDType CSegmentAnalysisResultsGraphController::SelectedGraphIndexToGraphID(IndexType graphIdx)
{
   if (GetGraphMode() == Loading )
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

bool CSegmentAnalysisResultsGraphController::ShowBeamBelowGraph() const
{
   return true;
}

IndexType CSegmentAnalysisResultsGraphController::GetGraphTypeCount() const
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   return pcbLoading->GetCount();
}

CString CSegmentAnalysisResultsGraphController::GetGraphType(IndexType idx) const
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   CString strType;
   pcbLoading->GetLBText((int)idx, strType);
   return strType;
}

void CSegmentAnalysisResultsGraphController::SelectGraphType(IndexType idx)
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   pcbLoading->SetCurSel((int)idx);

   if (GetGraphMode() == Loading)
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

void CSegmentAnalysisResultsGraphController::SelectGraphType(LPCTSTR lpszType)
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   int idx = pcbLoading->FindStringExact(-1,lpszType);
   SelectGraphType(idx);
}

IndexType CSegmentAnalysisResultsGraphController::GetGraphCount() const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   return (IndexType)(plbSelectList->GetCount());
}

IndexType CSegmentAnalysisResultsGraphController::GetSelectedGraphCount() const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   IndexType count = plbSelectList->GetSelCount();

   return count;
}

std::vector<IndexType> CSegmentAnalysisResultsGraphController::GetSelectedGraphs() const
{
   std::vector<IndexType> vSelectedGraphs;
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   int count = plbSelectList->GetSelCount();
   CArray<int, int> selectedItems;
   selectedItems.SetSize(count);
   plbSelectList->GetSelItems(count, selectedItems.GetData());
   ATLASSERT(count == selectedItems.GetCount());
   for (int i = 0; i < count; i++)
   {
      vSelectedGraphs.push_back(selectedItems.GetAt(i));
   }

   return vSelectedGraphs;
}

CString CSegmentAnalysisResultsGraphController::GetGraphName(IndexType idx) const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   CString str;
   plbSelectList->GetText((int)idx, str);
   return str;
}

void CSegmentAnalysisResultsGraphController::SelectGraph(IndexType idx)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   plbLoading->SetSel((int)idx);

   UpdateResultsType();
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::SelectGraphs(const std::vector<IndexType>& vGraphs)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   plbLoading->SelItemRange(FALSE,0,plbLoading->GetCount()-1); // unselect evertyhing
   for (const auto& idx : vGraphs)
   {
      plbLoading->SetSel((int)idx);
   }

   UpdateResultsType();
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::SelectGraph(LPCTSTR lpszGraphName)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   int idx = plbLoading->FindStringExact(-1, lpszGraphName);
   if (idx != LB_ERR)
   {
      SelectGraph(idx);
   }
}

void CSegmentAnalysisResultsGraphController::SelectGraphs(const std::vector<CString>& vGraphs)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   plbLoading->SelItemRange(FALSE, 0, plbLoading->GetCount() - 1); // unselect evertyhing
   for (const auto& strGraph : vGraphs)
   {
      int idx = plbLoading->FindStringExact(-1, strGraph);
      if (idx != LB_ERR)
      {
         plbLoading->SetSel(idx);
      }
   }

   UpdateResultsType();
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnModeChanged()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   int curSel = pcbMode->GetCurSel();
   GraphModeType mode = (GraphModeType)(pcbMode->GetItemData(curSel));
   SetGraphMode(mode);
}

void CSegmentAnalysisResultsGraphController::OnActionChanged()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();
   ActionType actionType = ActionType(pcbAction->GetItemData(curSel));
   SetActionType(actionType);
}

void CSegmentAnalysisResultsGraphController::OnDropDownChanged()
{
   if ( GetGraphMode() == Loading )
   {
      FillActionTypeCtrl();
      FillSelectListCtrl(true);
      OnIntervalsChanged();
   }
   else
   {
      UpdateResultsType();
      FillDropListCtrl(true);
   }

   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnSelectListChanged()
{
   if ( GetGraphMode() == Loading) 
   {
      UpdateResultsType();
   }
   else
   {
      OnIntervalsChanged();
   }

   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnPlotTypeClicked()
{
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnStress()
{
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnAnalysisTypeClicked()
{
   int nIDC = GetCheckedRadioButton(IDC_SIMPLE,IDC_SIMPLE3);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   pgsTypes::AnalysisType analysisType = (pgsTypes::AnalysisType)(nIDC - IDC_SIMPLE);
   SetAnalysisType(analysisType);
}

void CSegmentAnalysisResultsGraphController::OnIntervalsChanged()
{
   UpdateUnrecoverableDeflAdjustment();
}

void CSegmentAnalysisResultsGraphController::OnUnrecoverableDefl()
{
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CSegmentGraphControllerBase::OnUpdate(pSender,lHint,pHint);

   if ( 
        lHint == HINT_BRIDGECHANGED   ||
        lHint == HINT_SPECCHANGED   
      )
   {
      OnSegmentChanged();
   }
}

void CSegmentAnalysisResultsGraphController::OnGroupChanged()
{
   OnSegmentChanged();
}

void CSegmentAnalysisResultsGraphController::OnGirderChanged()
{
   OnSegmentChanged();
}

void CSegmentAnalysisResultsGraphController::OnSegmentChanged()
{
   const CSegmentKey& segmentKey = GetSegmentKey();
   ((CSegmentAnalysisResultsGraphBuilder*)GetGraphBuilder())->UpdateGraphDefinitions(segmentKey);
   FillDropListCtrl(true);
   FillSelectListCtrl(true);
   UpdateGraph();
}

void CSegmentAnalysisResultsGraphController::FillModeCtrl()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   pcbMode->SetItemData(pcbMode->AddString(_T("Plot by Interval")),(DWORD_PTR)Interval);
   pcbMode->SetItemData(pcbMode->AddString(_T("Plot by Loading")),(DWORD_PTR)Loading);
   pcbMode->SetCurSel(1);
}

std::vector<ActionType> CSegmentAnalysisResultsGraphController::GetActionTypes() const
{
   std::vector<ActionType> vActions;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   if (pProductLoads->ReportAxialResults())
   {
      vActions.push_back(actionAxial);
   }

   vActions.push_back(actionShear);
   vActions.push_back(actionMoment);

   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() || pBridge->HasTiltedGirders())
   {
      vActions.push_back(actionXDeflection);
      vActions.push_back(actionDeflection);
   }
   else
   {
      vActions.push_back(actionDeflection);
   }

   vActions.push_back(actionRotation);
   vActions.push_back(actionStress);
   vActions.push_back(actionReaction);

   GET_IFACE(ISpecification,pSpec);
   ISpecification::PrincipalWebStressCheckType pwscType = pSpec->GetPrincipalWebStressCheckType(CSegmentKey(INVALID_INDEX, INVALID_INDEX, INVALID_INDEX));

   return vActions;
}

LPCTSTR CSegmentAnalysisResultsGraphController::GetActionName(ActionType action) const
{
   if (action == actionDeflection)
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridge, pBridge);
      if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() || pBridge->HasTiltedGirders())
      {
         return _T("Deflection Y");
      }
   }

   static LPCTSTR actions[] =
   {
      _T("Axial"),
      _T("Shear"),
      _T("Moment"),
      _T("Reaction"),
      _T("Deflection"),
      _T("Deflection X"),
      _T("Rotation"),
      _T("Stress")
   };

#if defined _DEBUG
   switch(action)
   {
   case actionAxial:
   case actionShear:
   case actionMoment:
   case actionReaction:
   case actionDeflection:
   case actionXDeflection:
   case actionRotation:
   case actionStress:
      break;
   default:
      ATLASSERT(false); // is there a new action type?
   };
#endif

   return actions[action];
}

void CSegmentAnalysisResultsGraphController::FillActionTypeCtrl()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();

   pcbAction->ResetContent();

   std::vector<ActionType> vActions = GetActionTypes();
   for (const auto& action : vActions)
   {
      pcbAction->SetItemData(pcbAction->AddString(GetActionName(action)), action);
   }

   if ( curSel == CB_ERR || pcbAction->GetCount() <= curSel)
   {
      pcbAction->SetCurSel(std::find(vActions.cbegin(),vActions.cend(),actionAxial) != vActions.cend() ? 2 : 1);
   }
   else
   {
      pcbAction->SetCurSel(curSel);
   }
}

void CSegmentAnalysisResultsGraphController::FillDropListCtrl(bool bRetainSelection)
{
   // NOTE: the two calls in this method are supposed to look backwards
   // in Interval mode, the drop down list contains loadings and visa-versa
   if ( GetGraphMode() == Interval )
   {
      FillDropListCtrl_Loadings(bRetainSelection);
   }
   else
   {
      FillDropListCtrl_Intervals(bRetainSelection);
   }
}

void CSegmentAnalysisResultsGraphController::FillDropListCtrl_Intervals(bool bRetainSelection)
{
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_DROP_LIST);

   int curSel = pcbIntervals->GetCurSel();

   pcbIntervals->ResetContent();

   CSegmentKey segmentKey(GetSegmentKey());
   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      segmentKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx = GetLastInterval();
   for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
   {
      if (intervalIdx == haulSegmentIntervalIdx || !pIntervals->IsHaulSegmentInterval(intervalIdx))
      {
         CString strInterval;
         strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));

         if (intervalIdx == erectSegmentIntervalIdx)
         {
            strInterval += _T(" - Current Segment");
         }

         int idx = pcbIntervals->AddString(strInterval);
         pcbIntervals->SetItemData(idx,intervalIdx);
      }
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

void CSegmentAnalysisResultsGraphController::FillDropListCtrl_Loadings(bool bRetainSelection)
{
   CComboBox* pcbLoadings = (CComboBox*)GetDlgItem(IDC_DROP_LIST);

   int curSel = pcbLoadings->GetCurSel();
   CString strCurSel;
   pcbLoadings->GetLBText(curSel,strCurSel);

   pcbLoadings->ResetContent();

   CSegmentAnalysisResultsGraphBuilder* pGraphBuilder = (CSegmentAnalysisResultsGraphBuilder*)GetGraphBuilder();
   
   ActionType actionType = GetActionType();
   std::vector<std::pair<std::_tstring,IDType>> vLoadings( pGraphBuilder->GetLoadings(INVALID_INDEX,actionType) );
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
      curSel = pcbLoadings->FindStringExact(0,strCurSel);
      curSel = (curSel == CB_ERR ? 0 : curSel);
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

void CSegmentAnalysisResultsGraphController::FillSelectListCtrl(bool bRetainSelection)
{
   if ( GetGraphMode() == Interval )
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

void CSegmentAnalysisResultsGraphController::FillSelectListCtrl_Intervals(bool bRetainSelection)
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

   CSegmentKey segmentKey(GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      if (intervalIdx == haulSegmentIntervalIdx || !pIntervals->IsHaulSegmentInterval(intervalIdx))
      {
         CString str;
         CSegmentKey thisSegmentKey(segmentKey);
         if (thisSegmentKey.groupIndex == ALL_GROUPS)
         {
            thisSegmentKey.groupIndex = 0;
         }

         str.Format(_T("%d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));

         if (intervalIdx == erectSegmentIntervalIdx)
         {
            str += _T(" - Current Segment");
         }

         int idx = plbIntervals->AddString(str);
         plbIntervals->SetItemData(idx,intervalIdx);
      }
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

void CSegmentAnalysisResultsGraphController::FillSelectListCtrl_Loadings(bool bRetainSelection)
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

   CSegmentAnalysisResultsGraphBuilder* pGraphBuilder = (CSegmentAnalysisResultsGraphBuilder*)GetGraphBuilder();
   
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

void CSegmentAnalysisResultsGraphController::UpdateStressControls()
{
   // only show the stress check boxes if the action type is actionStress
   int nCmdShow = (GetActionType() == actionStress ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_GIRDER   )->ShowWindow(nCmdShow);
   GetDlgItem(IDC_BOTTOM_GIRDER)->ShowWindow(nCmdShow);
}

void CSegmentAnalysisResultsGraphController::UpdateListInfo()
{
   CString strType(GetGraphMode() == Interval ? _T("intervals") : _T("loadings"));
   CString strHint;
   strHint.Format(_T("Hold CTRL key to select multiple %s\nHold SHIFT key to select range of %s"),strType,strType);
   GetDlgItem(IDC_LIST_INFO)->SetWindowText(strHint);
}

static bool IsCumulativeOnlyGraphType(GraphType graphType)
{
   return (graphType == graphLimitState ||
           graphType == graphDemand     ||
           graphType == graphAllowable  ||
           graphType == graphCapacity   ||
           graphType == graphMinCapacity ||
           graphType == graphLoadRating) ? true : false;
}


bool CSegmentAnalysisResultsGraphController::IncludeUnrecoverableDefl(IntervalIndexType interval) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectInterval = pIntervals->GetErectSegmentInterval(GetSegmentKey());
   IntervalIndexType haulInterval = pIntervals->GetHaulSegmentInterval(GetSegmentKey());

   if (interval < haulInterval)
   {
      return true;
   }
   else
   {
      CWnd* pWnd = GetDlgItem(IDC_PRECAMBER);
      return pWnd->IsWindowVisible() && IsDlgButtonChecked(IDC_PRECAMBER) == BST_CHECKED;
   }
}

void CSegmentAnalysisResultsGraphController::UpdateUnrecoverableDeflAdjustment()
{
   CWnd* pWnd = GetDlgItem(IDC_PRECAMBER);

   // Only show for deflection after after hauling
   bool doShow(false);

   ActionType actionType = GetActionType();
   if (actionType == actionDeflection || actionType == actionRotation)
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectInterval = pIntervals->GetErectSegmentInterval(GetSegmentKey());
      IntervalIndexType haulInterval = pIntervals->GetHaulSegmentInterval(GetSegmentKey());

      std::vector<IntervalIndexType> intervals(GetSelectedIntervals());
      for (const auto& interval : intervals)
      {
         if (interval >= haulInterval)
         {
            doShow = true;
            break;
         }
      }
   }

   pWnd->ShowWindow(doShow ? SW_SHOW : SW_HIDE);
}

void CSegmentAnalysisResultsGraphController::UpdateResultsType()
{
   // always set to show here because the controls get hidden for load rating graph
   GetDlgItem(IDC_INCREMENTAL)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_CUMULATIVE)->ShowWindow(SW_SHOW);

   bool bCumulativeOnly = false;
   CSegmentAnalysisResultsGraphBuilder* pGraphBuilder = (CSegmentAnalysisResultsGraphBuilder*)GetGraphBuilder();
   if ( GetGraphMode() == Interval )
   {
      IDType graphID = SelectedGraphIndexToGraphID(0);
      GraphType graphType = pGraphBuilder->GetGraphType(graphID);
      bCumulativeOnly = IsCumulativeOnlyGraphType(graphType);
   }
   else
   {
      IndexType nGraphs = GetSelectedGraphCount();
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
      GetDlgItem(IDC_CUMULATIVE)->EnableWindow(TRUE);
   }
   else
   {
      GetDlgItem(IDC_INCREMENTAL)->EnableWindow(TRUE);
      GetDlgItem(IDC_CUMULATIVE)->EnableWindow(TRUE);
   }
}

IntervalIndexType CSegmentAnalysisResultsGraphController::GetFirstInterval() const
{
   GET_IFACE(IIntervals,pIntervals);
   CSegmentKey segmentKey(GetSegmentKey());
   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      segmentKey.groupIndex = 0;
   }

   return pIntervals->GetPrestressReleaseInterval(segmentKey);
}

IntervalIndexType CSegmentAnalysisResultsGraphController::GetLastInterval() const
{
   GET_IFACE(IIntervals,pIntervals);
   CSegmentKey segmentKey(GetSegmentKey());
   if (segmentKey.groupIndex == ALL_GROUPS)
   {
      segmentKey.groupIndex = 0;
   }

   // Tough call here, but we need to have overlap with the total bridge analysis view. Get last segment erection interval
   CGirderKey girderKey(segmentKey.groupIndex,segmentKey.girderIndex);
   return pIntervals->GetLastSegmentErectionInterval(girderKey);

   // return pIntervals->GetErectSegmentInterval(segmentKey);
}

#ifdef _DEBUG
void CSegmentAnalysisResultsGraphController::AssertValid() const
{
	CSegmentGraphControllerBase::AssertValid();
}

void CSegmentAnalysisResultsGraphController::Dump(CDumpContext& dc) const
{
	CSegmentGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
