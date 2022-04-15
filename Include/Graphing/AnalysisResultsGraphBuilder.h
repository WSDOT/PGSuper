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
#include <Graphing\GirderGraphBuilderBase.h>
#include <Graphing\GraphingTypes.h>

class CAnalysisResultsGraphDefinition; // use a forward declaration here. We don't want to include the header file for a non-exported class
class CAnalysisResultsGraphDefinitions;
class grGraphColor;

class GRAPHINGCLASS CAnalysisResultsGraphBuilder : public CGirderGraphBuilderBase
{
public:
   CAnalysisResultsGraphBuilder();
   CAnalysisResultsGraphBuilder(const CAnalysisResultsGraphBuilder& other);
   virtual ~CAnalysisResultsGraphBuilder();

   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;

   virtual CGraphBuilder* Clone() const override;

   void UpdateGraphDefinitions(const CGirderKey& girderKey);
   std::vector<std::pair<std::_tstring,IDType>> GetLoadings(IntervalIndexType intervalIdx,ActionType actionType);

   GraphType GetGraphType(IDType graphID);

   virtual void CreateViewController(IEAFViewController** ppController) override;

   void DumpLBAM();

   COLORREF GetGraphColor(IndexType graphIdx,IntervalIndexType intervalIdx);

   void ExportGraphData();

protected:
   std::unique_ptr<grGraphColor> m_pGraphColor;
   std::set<IndexType> m_UsedDataLabels; // keeps track of the graph data labels that have already been used so we don't get duplicates in the legend

   void Init();

   virtual CGirderGraphControllerBase* CreateGraphController() override;
   virtual bool UpdateNow() override;

   void UpdateYAxisUnits();
   void UpdateXAxisTitle();
   void UpdateGraphTitle();
   void UpdateGraphData();

   CString GetDataLabel(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx);

   void InitializeGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, ActionType actionType, IntervalIndexType intervalIdx, bool bIsFinalShear, std::array<IndexType, 4>* pDataSeriesID, std::array<pgsTypes::BridgeAnalysisType, 4>* pBat, std::array<pgsTypes::StressLocation, 4>* pStressLocations, IndexType* pAnalysisTypeCount);

   void ProductLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear);
   void CombinedLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear);
   void LimitStateLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear);

   void LiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear);
   void VehicularLiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear);

   void ProductReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx);
   void CombinedReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx);
   void VehicularLiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey);
   void LiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey);


   void CyStressCapacityGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);
   void DeckShrinkageStressGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);

   void RatingFactorGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals);
   void PrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals);
   void TimeStepPrincipalWebStressLiveLoadGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals);
   void TimeStepPrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals);
   void TimeStepProductLoadPrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals);

   virtual void GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx) override;
   virtual DWORD GetDrawBeamStyle() const override;

   std::unique_ptr<CAnalysisResultsGraphDefinitions> m_pGraphDefinitions;

   DECLARE_MESSAGE_MAP()

   std::vector<IntervalIndexType> AddTSRemovalIntervals(IntervalIndexType loadingIntervalIdx,const std::vector<IntervalIndexType>& vIntervals,const std::vector<IntervalIndexType>& vTSRIntervals);
   pgsTypes::AnalysisType GetAnalysisType();

   Float64 m_GroupOffset;

   void GetSecondaryXValues(const PoiList& vPoi,const std::vector<Float64>& xVals,PoiList* pPoi,std::vector<Float64>* pXvalues);
   void GetSegmentXValues(const CGirderKey& girderKey,SegmentIndexType segIdx,IntervalIndexType intervalIdx,std::vector<CSegmentKey>* pSegments,std::vector<Float64>* pLeftXVals,std::vector<Float64>* pRightXVals);
   void GetSupportXValues(const CGirderKey& girderKey,bool bIncludeTemporarySupports,std::vector<Float64>* pXVals,std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>* pSupports);
};
