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

#include "stdafx.h"
#include "resource.h"
#include "AnalysisResultsGraphController.h"
#include <Graphs/GraphTypes.h>
#include <Graphs/AnalysisResultsGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>

#include <EAF\EAFDocument.h>
#include <Hints.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CAnalysisResultsGraphController,CGirderGraphControllerBase)

CAnalysisResultsGraphController::CAnalysisResultsGraphController():
CGirderGraphControllerBase(true/*false*//*exclude ALL_GROUPS*/)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   m_LiveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
}

void CAnalysisResultsGraphController::SetGraphMode(CAnalysisResultsGraphController::GraphModeType mode)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_MODE);
   pCB->SetCurSel((CAnalysisResultsGraphController::GraphModeType)(pCB->GetItemData(0)) == mode ? 0 : 1);
   FillDropListCtrl(false);
   FillSelectListCtrl(false);
   FillActionTypeCtrl();
   UpdateListInfo();
   OnIntervalsChanged();
   UpdateGraph();
}

CAnalysisResultsGraphController::GraphModeType CAnalysisResultsGraphController::GetGraphMode() const
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_MODE);
   int curSel = pCB->GetCurSel();
   return (CAnalysisResultsGraphController::GraphModeType)(pCB->GetItemData(curSel));
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

   ON_BN_CLICKED(IDC_ELEV_ADJUSTMENT, OnElevAdjustment)
   ON_BN_CLICKED(IDC_PRECAMBER, OnUnrecoverableDefl)

   ON_BN_CLICKED(IDC_SIMPLE,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE2,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE3,OnAnalysisTypeClicked)

   ON_BN_CLICKED(IDC_EXPORT_GRAPH_BTN,OnGraphExportClicked)
   ON_UPDATE_COMMAND_UI(IDC_EXPORT_GRAPH_BTN,OnCommandUIGraphExport)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CAnalysisResultsGraphController::DoDataExchange(CDataExchange* pDX)
{
   DDX_Control(pDX, IDC_DROP_LIST, m_cbDropList);

   CGirderGraphControllerBase::DoDataExchange(pDX);
}

BOOL CAnalysisResultsGraphController::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();

   GET_IFACE(IBridge, pBridge);
   m_bHasStructuralDeck = IsStructuralDeck(pBridge->GetDeckType());

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
   UpdateAnalysisType();
   UpdateElevAdjustment();
   UpdateUnrecoverableDeflAdjustment();

   return TRUE;
}

void CAnalysisResultsGraphController::SetResultsType(ResultsType resultsType)
{
   CheckRadioButton(IDC_INCREMENTAL, IDC_CUMULATIVE, (resultsType == rtIncremental ? IDC_INCREMENTAL : IDC_CUMULATIVE));
   UpdateGraph();
}

ResultsType CAnalysisResultsGraphController::GetResultsType() const
{
   return IsDlgButtonChecked(IDC_INCREMENTAL) == BST_CHECKED ? rtIncremental : rtCumulative;
}

void CAnalysisResultsGraphController::SetActionType(ActionType actionType)
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int nActions = pcbAction->GetCount();
   for (int idx = 0; idx < nActions; idx++)
   {
      if ((ActionType)(pcbAction->GetItemData(idx)) == actionType)
      {
         pcbAction->SetCurSel(idx);

         UpdateElevAdjustment();
         UpdateUnrecoverableDeflAdjustment();
         UpdateStressControls();
         UpdateResultsType();

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

ActionType CAnalysisResultsGraphController::GetActionType() const
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);
   int curSel = pcbAction->GetCurSel();
   return (ActionType)(pcbAction->GetItemData(curSel));
}

void CAnalysisResultsGraphController::PlotStresses(pgsTypes::StressLocation stressLocation, bool bPlot)
{
   static UINT nIDC[4] = { IDC_BOTTOM_GIRDER,IDC_TOP_GIRDER,IDC_BOTTOM_DECK,IDC_TOP_DECK };
   if (PlotStresses(stressLocation) != bPlot)
   {
      CheckDlgButton(nIDC[stressLocation], bPlot ? BST_CHECKED : BST_UNCHECKED);
      UpdateGraph();
   }
}

bool CAnalysisResultsGraphController::PlotStresses(pgsTypes::StressLocation stressLocation) const
{
   static UINT nIDC[4] = {IDC_BOTTOM_GIRDER,IDC_TOP_GIRDER,IDC_BOTTOM_DECK,IDC_TOP_DECK};
   return IsDlgButtonChecked(nIDC[stressLocation]) == BST_CHECKED ? true : false;
}

IntervalIndexType CAnalysisResultsGraphController::GetInterval() const
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

std::vector<IntervalIndexType> CAnalysisResultsGraphController::GetSelectedIntervals() const
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

void CAnalysisResultsGraphController::IncludeElevationAdjustment(bool bInclude)
{
   CheckDlgButton(IDC_ELEV_ADJUSTMENT, bInclude ? BST_CHECKED : BST_UNCHECKED);
   UpdateGraph();
}

bool CAnalysisResultsGraphController::IncludeElevationAdjustment() const
{
   return IsDlgButtonChecked(IDC_ELEV_ADJUSTMENT) == BST_CHECKED ? true : false;
}

void CAnalysisResultsGraphController::IncludeUnrecoverableDefl(bool bInclude)
{
   CheckDlgButton(IDC_PRECAMBER, bInclude ? BST_CHECKED : BST_UNCHECKED);
   UpdateGraph();
}

bool CAnalysisResultsGraphController::IncludeUnrecoverableDefl() const
{
   return IsDlgButtonChecked(IDC_PRECAMBER) == BST_CHECKED ? true : false;
}

void CAnalysisResultsGraphController::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   int nIDC = IDC_SIMPLE + (int)(analysisType);
   CheckRadioButton(IDC_SIMPLE, IDC_SIMPLE3, nIDC);
   UpdateGraph();
}

pgsTypes::AnalysisType CAnalysisResultsGraphController::GetAnalysisType() const
{
   int nIDC = GetCheckedRadioButton(IDC_SIMPLE, IDC_SIMPLE3);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   return (pgsTypes::AnalysisType)(nIDC - IDC_SIMPLE);
}

IDType CAnalysisResultsGraphController::SelectedGraphIndexToGraphID(IndexType graphIdx)
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

bool CAnalysisResultsGraphController::ShowBeamBelowGraph() const
{
   return true;
}

IndexType CAnalysisResultsGraphController::GetGraphTypeCount() const
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   return pcbLoading->GetCount();
}

CString CAnalysisResultsGraphController::GetGraphType(IndexType idx) const
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   CString strType;
   pcbLoading->GetLBText((int)idx, strType);
   return strType;
}

void CAnalysisResultsGraphController::SelectGraphType(IndexType idx)
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

void CAnalysisResultsGraphController::SelectGraphType(LPCTSTR lpszType)
{
   CComboBox* pcbLoading = (CComboBox*)GetDlgItem(IDC_DROP_LIST);
   int idx = pcbLoading->FindStringExact(-1,lpszType);
   SelectGraphType(idx);
}

IndexType CAnalysisResultsGraphController::GetGraphCount() const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   return (IndexType)(plbSelectList->GetCount());
}

IndexType CAnalysisResultsGraphController::GetSelectedGraphCount() const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   IndexType count = plbSelectList->GetSelCount();

   return count;
}

std::vector<IndexType> CAnalysisResultsGraphController::GetSelectedGraphs() const
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

CString CAnalysisResultsGraphController::GetGraphName(IndexType idx) const
{
   CListBox* plbSelectList = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   CString str;
   plbSelectList->GetText((int)idx, str);
   return str;
}

void CAnalysisResultsGraphController::SelectGraph(IndexType idx)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   plbLoading->SetSel((int)idx);

   UpdateResultsType();
   UpdateGraph();
}

void CAnalysisResultsGraphController::SelectGraphs(const std::vector<IndexType>& vGraphs)
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

void CAnalysisResultsGraphController::SelectGraph(LPCTSTR lpszGraphName)
{
   CListBox* plbLoading = (CListBox*)GetDlgItem(IDC_SELECT_LIST);
   int idx = plbLoading->FindStringExact(-1, lpszGraphName);
   if (idx != LB_ERR)
   {
      SelectGraph(idx);
   }
}

void CAnalysisResultsGraphController::SelectGraphs(const std::vector<CString>& vGraphs)
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

void CAnalysisResultsGraphController::OnModeChanged()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   int curSel = pcbMode->GetCurSel();
   GraphModeType mode = (GraphModeType)(pcbMode->GetItemData(curSel));
   SetGraphMode(mode);
}

void CAnalysisResultsGraphController::OnActionChanged()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();
   ActionType actionType = ActionType(pcbAction->GetItemData(curSel));
   SetActionType(actionType);
}

void CAnalysisResultsGraphController::OnDropDownChanged()
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

void CAnalysisResultsGraphController::OnSelectListChanged()
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

void CAnalysisResultsGraphController::OnUnrecoverableDefl()
{
   UpdateGraph();
}

void CAnalysisResultsGraphController::OnAnalysisTypeClicked()
{
   int nIDC = GetCheckedRadioButton(IDC_SIMPLE,IDC_SIMPLE3);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   pgsTypes::AnalysisType analysisType = (pgsTypes::AnalysisType)(nIDC - IDC_SIMPLE);
   SetAnalysisType(analysisType);
}

void CAnalysisResultsGraphController::OnIntervalsChanged()
{
   UpdateUnrecoverableDeflAdjustment();
}

void CAnalysisResultsGraphController::OnGraphExportClicked()
{
   // Build default file name
   CString strProjectFileNameNoPath = CExportGraphXYTool::GetTruncatedFileName();

   const CGirderKey& girderKey = GetGirderKey();
   CString girderName = GIRDER_LABEL(girderKey);

   ActionType action = GetActionType();
   CString actionName = GetActionName(action);

   CString strDefaultFileName = strProjectFileNameNoPath + _T("_") + girderName + _T("_") + actionName;
   strDefaultFileName.Replace(' ','_'); // prefer not to have spaces or ,'s in file names
   strDefaultFileName.Replace(',','_');

   ((CAnalysisResultsGraphBuilder*)GetGraphBuilder())->ExportGraphData(strDefaultFileName);
}

// this has to be implemented otherwise button will not be enabled.
void CAnalysisResultsGraphController::OnCommandUIGraphExport(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}

void CAnalysisResultsGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);

   GET_IFACE(IIntervals, pIntervals);
   m_LiveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   m_LoadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   if ( 
        lHint == HINT_LIVELOADCHANGED || 
        lHint == HINT_BRIDGECHANGED   ||
        lHint == HINT_SPECCHANGED     ||
        lHint == HINT_RATINGSPECCHANGED
      )
   {
      OnGirderChanged();
   }
   else if ( lHint == HINT_ANALYSISTYPECHANGED )
   {
      UpdateAnalysisType();
   }
}

void CAnalysisResultsGraphController::OnGroupChanged()
{
   OnGirderChanged();
}

void CAnalysisResultsGraphController::OnGirderChanged()
{
   const CGirderKey& girderKey = GetGirderKey();
   ((CAnalysisResultsGraphBuilder*)GetGraphBuilder())->UpdateGraphDefinitions(girderKey);
   FillDropListCtrl(true);
   FillSelectListCtrl(true);
   UpdateGraph();
}

void CAnalysisResultsGraphController::FillModeCtrl()
{
   CComboBox* pcbMode = (CComboBox*)GetDlgItem(IDC_MODE);
   pcbMode->SetItemData(pcbMode->AddString(_T("Plot by Interval")),(DWORD_PTR)Interval);
   pcbMode->SetItemData(pcbMode->AddString(_T("Plot by Loading")),(DWORD_PTR)Loading);
   pcbMode->SetCurSel(1);
}

std::vector<ActionType> CAnalysisResultsGraphController::GetActionTypes() const
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

   IntervalIndexType interval = GetInterval();
   if (ISpecification::pwcNotApplicable != pwscType && (m_LiveLoadIntervalIdx <= interval || ISpecification::pwcNCHRPTimeStepMethod == pwscType)) // time step method covers all intervals
   {
      vActions.push_back(actionPrincipalWebStress);
   }

   if (interval == m_LoadRatingIntervalIdx && GetGraphMode() == Loading)
   {
      // rating factors are only applicable for "By Loading" results the interval when load rating occurs
      vActions.push_back(actionLoadRating);
   }

   return vActions;
}

LPCTSTR CAnalysisResultsGraphController::GetActionName(ActionType action) const
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
      _T("Stress"),
      _T("Rating Factor"),
      _T("Principal Web Stress")
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
   case actionLoadRating:
   case actionPrincipalWebStress:
      break;
   default:
      ATLASSERT(false); // is there a new action type?
   };
#endif

   return actions[action];
}

void CAnalysisResultsGraphController::FillActionTypeCtrl()
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

void CAnalysisResultsGraphController::FillDropListCtrl(bool bRetainSelection)
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
      // skip hauling intervals
      if (!pIntervals->IsHaulSegmentInterval(intervalIdx))
      {
         CString strInterval;
         strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
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

void CAnalysisResultsGraphController::FillDropListCtrl_Loadings(bool bRetainSelection)
{
   CComboBox* pcbLoadings = (CComboBox*)GetDlgItem(IDC_DROP_LIST);

   int curSel = pcbLoadings->GetCurSel();
   CString strCurSel;
   pcbLoadings->GetLBText(curSel,strCurSel);

   pcbLoadings->ResetContent();

   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();
   
   ActionType actionType = GetActionType();
   if (actionType == actionLoadRating)
   {
      // load rating is not an option for results by loading
      actionType = actionMoment;
   }
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

void CAnalysisResultsGraphController::FillSelectListCtrl(bool bRetainSelection)
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

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstIntervalIdx = GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = GetLastInterval();
   for (IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++)
   {
      // skip hauling intervals
      if (!pIntervals->IsHaulSegmentInterval(intervalIdx))
      {
         CString str;
         str.Format(_T("%d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
         int idx = plbIntervals->AddString(str);
         plbIntervals->SetItemData(idx,intervalIdx);

         // reselect anything that was previously selected
         if (bRetainSelection)
         {
            for (int i = 0; i < selCount; i++)
            {
               CString strItem = selItems[i];
               int idx = plbIntervals->FindStringExact(-1,strItem);
               if (idx != LB_ERR)
               {
                  plbIntervals->SetSel(idx);
               }
            }
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
   int nCmdShow = (GetActionType() == actionStress ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_GIRDER   )->ShowWindow(nCmdShow);
   GetDlgItem(IDC_BOTTOM_GIRDER)->ShowWindow(nCmdShow);

   if (nCmdShow == SW_SHOW && !m_bHasStructuralDeck)
   {
      // don't show top/bottom deck controls if we don't have a structural deck
      nCmdShow = SW_HIDE;
   }
   GetDlgItem(IDC_TOP_DECK     )->ShowWindow(nCmdShow);
   GetDlgItem(IDC_BOTTOM_DECK  )->ShowWindow(nCmdShow);
}

void CAnalysisResultsGraphController::UpdateListInfo()
{
   CString strType(GetGraphMode() == Interval ? _T("intervals") : _T("loadings"));
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
           graphType == graphMinCapacity ||
           graphType == graphLoadRating) ? true : false;
}

void CAnalysisResultsGraphController::UpdateResultsType()
{
   if (GetActionType() == actionLoadRating || GetActionType() == actionPrincipalWebStress)
   {
      GetDlgItem(IDC_INCREMENTAL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CUMULATIVE)->ShowWindow(SW_HIDE);
      return;
   }

   // always set to show here because the controls get hidden for load rating graph
   GetDlgItem(IDC_INCREMENTAL)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_CUMULATIVE)->ShowWindow(SW_SHOW);

   bool bCumulativeOnly = false;
   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();
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

void CAnalysisResultsGraphController::UpdateElevAdjustment()
{
   CWnd* pWnd = GetDlgItem(IDC_ELEV_ADJUSTMENT);
   GET_IFACE(IDocumentType, pDocType);
   if (pDocType->IsPGSuperDocument())
   {
      // elevation adjustment doesn't apply to PGSuper
      pWnd->ShowWindow(SW_HIDE);
      return;
   }

   ActionType actionType = GetActionType();
   if ( actionType == actionDeflection || actionType == actionRotation )
   {
      if ( actionType == actionDeflection )
      {
         pWnd->SetWindowText(_T("Include Elevation Adjustment"));
      }
      else
      {
         pWnd->SetWindowText(_T("Include Slope Adjustment"));
      }

      pWnd->ShowWindow(SW_SHOW);
   }
   else
   {
      pWnd->ShowWindow(SW_HIDE);
   }
}

void CAnalysisResultsGraphController::UpdateUnrecoverableDeflAdjustment()
{
   CWnd* pWnd = GetDlgItem(IDC_PRECAMBER);

   // Only show for deflection
   ActionType actionType = GetActionType();
   if (actionType == actionDeflection || actionType == actionRotation)
   {
      pWnd->ShowWindow(SW_SHOW);

      // This is not an option if a pre-erection interval is selected
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectInterval = pIntervals->GetFirstSegmentErectionInterval(GetGirderKey());

      bool hasPreErection(false);
      std::vector<IntervalIndexType> intervals(GetSelectedIntervals());
      for (const auto& interval : intervals)
      {
         if (interval < erectInterval)
         {
            hasPreErection = true;
         }
      }

      if (hasPreErection)
      {
         CheckDlgButton(IDC_PRECAMBER,BST_CHECKED);
         pWnd->EnableWindow(FALSE);
      }
      else
      {
         pWnd->EnableWindow(TRUE);
      }
   }
   else
   {
      pWnd->ShowWindow(SW_HIDE);
   }

}

void CAnalysisResultsGraphController::UpdateAnalysisType()
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      GetDlgItem(IDC_ANALYSISTYPE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SIMPLE)->ShowWindow(SW_HIDE);  // Simple Span
      GetDlgItem(IDC_SIMPLE2)->ShowWindow(SW_HIDE); // Simple Spans made Continuous
      GetDlgItem(IDC_SIMPLE3)->ShowWindow(SW_HIDE); // Envelope
      ATLASSERT(analysisType == pgsTypes::Continuous);
      CheckRadioButton(IDC_SIMPLE, IDC_SIMPLE3, IDC_SIMPLE2);
      ATLASSERT(GetAnalysisType() == pgsTypes::Continuous);
   }
   else
   {
      int idx;
      switch( analysisType )
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

IntervalIndexType CAnalysisResultsGraphController::GetFirstInterval() const
{
   GET_IFACE(IIntervals,pIntervals);
   CGirderKey girderKey(GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // we start at last erection
   return pIntervals->GetFirstSegmentErectionInterval(girderKey);
}

IntervalIndexType CAnalysisResultsGraphController::GetLastInterval() const
{
   GET_IFACE(IIntervals,pIntervals);
   return pIntervals->GetIntervalCount() - 1;
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
