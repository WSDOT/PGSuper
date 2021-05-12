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

#pragma once

#include <Graphing\AnalysisResultsGraphBuilder.h>
#include "GirderGraphControllerBase.h"
#include <Graphing\GraphingTypes.h>

#include <IFace\AnalysisResults.h>
#include <MfcTools\WideDropDownComboBox.h>

class CAnalysisResultsGraphController : public CGirderGraphControllerBase
{
public:
   CAnalysisResultsGraphController();
   DECLARE_DYNCREATE(CAnalysisResultsGraphController);

   // returns one of the GRAPH_MODE_xxx constants
   enum GraphModeType {Interval, Loading};
   void SetGraphMode(GraphModeType mode);
   GraphModeType GetGraphMode() const;

   void SetResultsType(ResultsType resultsType);
   ResultsType GetResultsType() const;

   std::vector<ActionType> GetActionTypes() const;
   LPCTSTR GetActionName(ActionType action) const;

   void SetActionType(ActionType actionType);
   ActionType GetActionType() const;

   void SetAnalysisType(pgsTypes::AnalysisType analysisType);
   pgsTypes::AnalysisType GetAnalysisType() const;

   void PlotStresses(pgsTypes::StressLocation stressLocation, bool bPlot);
   bool PlotStresses(pgsTypes::StressLocation stressLocation) const;

   // These methods are for the drop down list box used to
   // select either the interval that loads are being graphed for
   // of a load to graph over multiple intervals
   IndexType GetGraphTypeCount() const;
   CString GetGraphType(IndexType idx) const;
   void SelectGraphType(IndexType idx);
   void SelectGraphType(LPCTSTR lpszType);

   // These methods are for the list box for each individual graph
   // Graphs depend on the current graphing mode
   IndexType GetGraphCount() const;
   IndexType GetSelectedGraphCount() const;
   std::vector<IndexType> GetSelectedGraphs() const;
   CString GetGraphName(IndexType idx) const;
   void SelectGraph(IndexType idx);
   void SelectGraphs(const std::vector<IndexType>& vGraphs);
   void SelectGraph(LPCTSTR lpszGraphName);
   void SelectGraphs(const std::vector<CString>& vGraphs);

   IntervalIndexType GetInterval() const;
   std::vector<IntervalIndexType> GetSelectedIntervals() const;

   void IncludeElevationAdjustment(bool bInclude);
   bool IncludeElevationAdjustment() const;

   void IncludePrecamber(bool bInclude);
   bool IncludePrecamber() const;

   IDType SelectedGraphIndexToGraphID(IndexType graphIdx);

protected:
   CWideDropDownComboBox m_cbDropList;

   virtual void DoDataExchange(CDataExchange* pDX);
   virtual BOOL OnInitDialog() override;

	//{{AFX_MSG(CAnalysisResultsGraphController)
   afx_msg void OnModeChanged();
   afx_msg void OnActionChanged();
   afx_msg void OnDropDownChanged();
   afx_msg void OnSelectListChanged();
   afx_msg void OnPlotTypeClicked();
   afx_msg void OnStress();
   afx_msg void OnElevAdjustment();
   afx_msg void OnPrecamber();
   afx_msg void OnAnalysisTypeClicked();
   //}}AFX_MSG

   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

	DECLARE_MESSAGE_MAP()

   void FillModeCtrl();
   void FillActionTypeCtrl();
   void FillDropListCtrl(bool bRetainSelection);
   void FillDropListCtrl_Intervals(bool bRetainSelection);
   void FillDropListCtrl_Loadings(bool bRetainSelection);
   void FillSelectListCtrl(bool bRetainSelection);
   void FillSelectListCtrl_Intervals(bool bRetainSelection);
   void FillSelectListCtrl_Loadings(bool bRetainSelection);

   void UpdateStressControls();
   void UpdateElevAdjustment();
   void UpdatePrecamberAdjustment();
   void UpdateAnalysisType();
   void UpdateListInfo();
   void UpdateResultsType();

   IntervalIndexType GetFirstInterval() const;
   IntervalIndexType GetLastInterval() const;

   bool m_bHasStructuralDeck;

   IntervalIndexType m_LiveLoadIntervalIdx;
   IntervalIndexType m_LoadRatingIntervalIdx;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG

   friend CAnalysisResultsGraphBuilder;
};