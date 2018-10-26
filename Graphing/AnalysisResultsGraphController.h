///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#define GRAPH_MODE_INTERVAL  0
#define GRAPH_MODE_LOADING   1

class CAnalysisResultsGraphController : public CGirderGraphControllerBase
{
public:
   CAnalysisResultsGraphController();
   DECLARE_DYNCREATE(CAnalysisResultsGraphController);

   // returns one of the GRAPH_MODE_xxx constants
   int GetGraphMode();

   ActionType GetActionType();
   ResultsType GetResultsType();
   bool PlotStresses(pgsTypes::StressLocation stressLocation);

   IntervalIndexType GetInterval();
   std::vector<IntervalIndexType> GetSelectedIntervals();

   bool IncludeElevationAdjustment();

   pgsTypes::AnalysisType GetAnalysisType();

   IndexType GetMaxGraphCount();
   IndexType GetGraphCount();

   IDType SelectedGraphIndexToGraphID(IndexType graphIdx);

protected:

   virtual BOOL OnInitDialog();

	//{{AFX_MSG(CAnalysisResultsGraphController)
   afx_msg void OnModeChanged();
   afx_msg void OnActionChanged();
   afx_msg void OnDropDownChanged();
   afx_msg void OnSelectListChanged();
   afx_msg void OnPlotTypeClicked();
   afx_msg void OnStress();
   afx_msg void OnElevAdjustment();
   afx_msg void OnAnalysisTypeClicked();
   //}}AFX_MSG

   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

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
   void UpdateAnalysisType();
   void UpdateListInfo();
   void UpdateResultsType();

   IntervalIndexType GetFirstInterval();
   IntervalIndexType GetLastInterval();

   // conotrl variables
   int                    m_GraphMode;
   ActionType             m_ActionType;
   pgsTypes::AnalysisType m_AnalysisType;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG

   friend CAnalysisResultsGraphBuilder;
};