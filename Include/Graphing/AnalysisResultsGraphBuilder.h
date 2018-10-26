///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <GraphicsLib\GraphXY.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PointOfInterest.h>

class CAnalysisResultsGraphController;
class CAnalysisResultsGraphDefinition;
class CAnalysisResultsGraphDefinitions;
class arvPhysicalConverter;


enum ActionType 
{
   actionMoment, 
   actionShear, 
   actionDisplacement, 
   actionStress
};

enum GraphType 
{ 
   graphCombined, 
   graphLiveLoad, 
   graphVehicularLiveLoad,
   graphLimitState, 
   graphProduct,
   graphPrestress, 
   graphPostTension,
   graphDemand, 
   graphAllowable, 
   graphCapacity,
   graphMinCapacity
};

class GRAPHINGCLASS CAnalysisResultsGraphBuilder : public CGirderGraphBuilderBase
{
public:
   CAnalysisResultsGraphBuilder();
   CAnalysisResultsGraphBuilder(const CAnalysisResultsGraphBuilder& other);
   virtual ~CAnalysisResultsGraphBuilder();

   virtual CGraphBuilder* Clone();

   void UpdateGraphDefinitions();
   std::vector<std::pair<CString,IDType>> GetLoadCaseNames(IntervalIndexType intervalIdx,ActionType actionType);

   void DumpLBAM();

protected:
   virtual CGirderGraphControllerBase* CreateGraphController();
   virtual int InitGraphController(CWnd* pParent,UINT nID);
   virtual bool UpdateNow();

   CAnalysisResultsGraphDefinitions* m_pGraphDefinitions;

   DECLARE_MESSAGE_MAP()

   std::vector<IntervalIndexType> AddTSRemovalIntervals(IntervalIndexType loadingIntervalIdx,const std::vector<IntervalIndexType>& vIntervals,const std::vector<IntervalIndexType>& vTSRIntervals);

   void UpdateYAxisUnits(ActionType actionType);
   void UpdateXAxisTitle(IntervalIndexType intervalIdx);
   void UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,ActionType actionType);
   void UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,ActionType actionType);
  
   void InitializeGraph(const CAnalysisResultsGraphDefinition& graphDef,ActionType actionType,IntervalIndexType intervalIdx,bool bIsFinalShear,IndexType* pDataSeriesID,pgsTypes::BridgeAnalysisType* pBAT,IndexType* pAnalysisTypeCount);

   void CombinedLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void LiveLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void VehicularLiveLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void ProductLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType actionType,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear=false);
   void PrestressLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals);
   void CyStressCapacityGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals);
   void PostTensionLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals);
   void LimitStateLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear=false);

   pgsTypes::AnalysisType GetAnalysisType();
};
