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

#include <Graphing\GraphingExp.h>
#include <Graphing\SegmentGraphBuilderBase.h>
#include <Graphing\GraphingTypes.h>

// Create a constant for a fake product load type so we can view unrecoverable deflection due to girder load at hauling and erection
#define PL_UNRECOVERABLE 1959

class CSegmentAnalysisResultsGraphDefinition; // use a forward declaration here. We don't want to include the header file for a non-exported class
class CSegmentAnalysisResultsGraphDefinitions;
class grGraphColor;

class GRAPHINGCLASS CSegmentAnalysisResultsGraphBuilder : public CSegmentGraphBuilderBase
{
public:
   CSegmentAnalysisResultsGraphBuilder();
   CSegmentAnalysisResultsGraphBuilder(const CSegmentAnalysisResultsGraphBuilder& other);
   virtual ~CSegmentAnalysisResultsGraphBuilder();

   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;

   virtual CGraphBuilder* Clone() const override;

   void UpdateGraphDefinitions(const CSegmentKey& segmentKey);
   std::vector<std::pair<std::_tstring,IDType>> GetLoadings(IntervalIndexType intervalIdx,ActionType actionType);

   GraphType GetGraphType(IDType graphID);

   virtual void CreateViewController(IEAFViewController** ppController) override;

   COLORREF GetGraphColor(IndexType graphIdx,IntervalIndexType intervalIdx);

protected:
   std::unique_ptr<grGraphColor> m_pGraphColor;
   std::set<IndexType> m_UsedDataLabels; // keeps track of the graph data labels that have already been used so we don't get duplicates in the legend

   void Init();

   virtual CSegmentGraphControllerBase* CreateGraphController() override;
   virtual bool UpdateNow() override;

   void UpdateYAxisUnits();
   void UpdateXAxisTitle();
   void UpdateGraphTitle();
   void UpdateGraphData();

   pgsTypes::AnalysisType GetAnalysisType();
   bool IncludeUnrecoverableDefl(IntervalIndexType interval);

   CString GetDataLabel(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx);

   void InitializeGraph(IndexType graphIdx, const CSegmentAnalysisResultsGraphDefinition& graphDef, ActionType actionType, IntervalIndexType intervalIdx, std::array<IndexType, 4>* pDataSeriesID, std::array<pgsTypes::BridgeAnalysisType, 4>* pBat, std::array<pgsTypes::StressLocation, 4>* pStressLocations, IndexType* pAnalysisTypeCount);

   void ProductLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);
   void CombinedLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);
   void LimitStateLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);

   void CyStressCapacityGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);

   void ProductReactionGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey);
   void CombinedReactionGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey);

   virtual void GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx) override;
   virtual DWORD GetDrawBeamStyle() const override;

   std::unique_ptr<CSegmentAnalysisResultsGraphDefinitions> m_pGraphDefinitions;

   DECLARE_MESSAGE_MAP()

   void GetSegmentXValues(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,std::vector<Float64>* pLeftXVals,std::vector<Float64>* pRightXVals);
};
