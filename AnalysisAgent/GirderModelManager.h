///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "GirderModelData.h"
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include "ProductLoadMap.h"

interface ILibrary;
interface ILiveLoads;

class CPierData2;
class CTemporarySupportData;
class CTimelineManager;
class CPrecastSegmentData;
class CParabolicDuctGeometry;
class CLinearDuctGeometry;
class COffsetDuctGeometry;

class COverhangLoadData
{
public:
   CSegmentKey segmentKey;
   CComBSTR bstrStage;
   CComBSTR bstrLoadGroup;

   Float64 PStart;
   Float64 PEnd;

   COverhangLoadData(const CSegmentKey& segmentKey, const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup, Float64 PStart, Float64 PEnd);
   bool operator == (const COverhangLoadData& rOther) const;
   bool operator < (const COverhangLoadData& rOther) const;
};


// CGirderModelManager
//
// Manages the creation, access, and storage of analysis models of spliced girder.
// This are the models of the assempeld girder and are modeled with the LBAM.

class CGirderModelManager
{
public:
   CGirderModelManager(SHARED_LOGFILE lf,IBroker* pBroker,StatusGroupIDType statusGroupID);

   void Clear();
   void DumpAnalysisModels(GirderIndexType gdrLineIdx);

   // IProduct Loads
   CComBSTR GetLoadGroupName(ProductForceType type);
   void GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad);
   Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey);
   Float64 GetSidewalkLoad(const CSegmentKey& segmentKey);
   bool HasPedestrianLoad();
   bool HasPedestrianLoad(const CGirderKey& girderKey);
   bool HasSidewalkLoad(const CGirderKey& girderKey);
   Float64 GetPedestrianLoad(const CSegmentKey& segmentKey);
   Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation);
   void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,
                                              Float64* pFraExtLeft, Float64* pFraIntLeft,
                                              Float64* pFraExtRight,Float64* pFraIntRight);
   void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad,
                                        Float64* pFraLeft,Float64* pFraRight);

   void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads);
   void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads);
   void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads);

   void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);
   void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);
   void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads);
   void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads);
   void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx,Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead);

   bool HasShearKeyLoad(const CGirderKey& girderKey); // checks for load in adjacent continuous beams as well as current beam
   void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads);

   std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex);
   pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex);
   VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType);
   Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex);

   void GetEquivPostTensionLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,std::vector<EquivPostTensionPointLoad>& ptLoads,std::vector<EquivPostTensionDistributedLoad>& distLoads,std::vector<EquivPostTensionMomentLoad>& momentLoads);
   void GetEquivPostTensionLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,const CLinearDuctGeometry& ductGeometry,Float64 P,std::vector<EquivPostTensionPointLoad>& ptLoads,std::vector<EquivPostTensionDistributedLoad>& distLoads,std::vector<EquivPostTensionMomentLoad>& momentLoads);
   void GetEquivPostTensionLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,const CParabolicDuctGeometry& ductGeometry,Float64 P,std::vector<EquivPostTensionPointLoad>& ptLoads,std::vector<EquivPostTensionDistributedLoad>& distLoads,std::vector<EquivPostTensionMomentLoad>& momentLoads);


   // IProductForces
   Float64 GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   sysSectionValue GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   Float64 GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   Float64 GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   Float64 GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);
   Float64 GetReaction(IntervalIndexType intervalIdx,ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);

   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL);
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL);
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL);
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL);
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig);
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig);
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=NULL,VehicleIndexType* pTopMaxConfig=NULL,VehicleIndexType* pBotMinConfig=NULL,VehicleIndexType* pBotMaxConfig=NULL);

   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                  AxleConfiguration* pMinLeftAxleConfig = NULL,
                                  AxleConfiguration* pMinRightAxleConfig = NULL,
                                  AxleConfiguration* pMaxLeftAxleConfig = NULL,
                                  AxleConfiguration* pMaxRightAxleConfig = NULL);
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=NULL,AxleConfiguration* pMaxAxleConfigTop=NULL,AxleConfiguration* pMinAxleConfigBot=NULL,AxleConfiguration* pMaxAxleConfigBot=NULL);
   void GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax);

   void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot);

   // IProductForce2
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL);
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL);
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=NULL,std::vector<VehicleIndexType>* pTopMaxIndex=NULL,std::vector<VehicleIndexType>* pBotMinIndex=NULL,std::vector<VehicleIndexType>* pBotMaxIndex=NULL);

   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                  std::vector<AxleConfiguration>* pMinLeftAxleConfig = NULL,
                                  std::vector<AxleConfiguration>* pMinRightAxleConfig = NULL,
                                  std::vector<AxleConfiguration>* pMaxLeftAxleConfig = NULL,
                                  std::vector<AxleConfiguration>* pMaxRightAxleConfig = NULL);
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMinAxleConfigurationBot=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationBot=NULL);

   // ICombinedForces
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax);
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax);
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax);
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax);
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax);

   // ICombinedForces2
   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax);
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact, std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax);
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax);
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax);

   // ILimitStateForces
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,Float64* pMin,Float64* pMax);
   void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax);
   void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax);

   // ILimitStateForces2
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax);
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax);

   std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat);

   // IExternalLoading
   bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName);
   bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   Float64 GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   // IContraflexurePoints
   void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints);

   // IReactions
   std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   void GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);
   void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);


   // IBearingDesign
   void GetBearingProductReaction(IntervalIndexType intervalIdx,ProductForceType type,const CGirderKey& girderKey,
                                  pgsTypes::BridgeAnalysisType bat, ResultsType cmbtype, Float64* pLftEnd,Float64* pRgtEnd);

   void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                   Float64* pLeftRmin,Float64* pLeftRmax,Float64* pLeftTmin,Float64* pLeftTmax,
                                   Float64* pRightRmin,Float64* pRightRmax,Float64* pRightTmin,Float64* pRightTmax,
                                   VehicleIndexType* pLeftMinVehIdx = NULL,VehicleIndexType* pLeftMaxVehIdx = NULL,
                                   VehicleIndexType* pRightMinVehIdx = NULL,VehicleIndexType* pRightMaxVehIdx = NULL);

   void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                   Float64* pLeftTmin,Float64* pLeftTmax,Float64* pLeftRmin,Float64* pLeftRmax,
                                   Float64* pRightTmin,Float64* pRightTmax,Float64* pRightRmin,Float64* pRightRmax,
                                   VehicleIndexType* pLeftMinVehIdx = NULL,VehicleIndexType* pLeftMaxVehIdx = NULL,
                                   VehicleIndexType* pRightMinVehIdx = NULL,VehicleIndexType* pRightMaxVehIdx = NULL);

   void GetBearingCombinedReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,const CGirderKey& girderKey,
                                   pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pLftEnd,Float64* pRgtEnd);

   void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                           Float64* pLeftRmin, Float64* pLeftRmax, 
                                           Float64* pRightRmin,Float64* pRightRmax);

   void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const CGirderKey& girderKey,
                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                     Float64* pLeftRmin, Float64* pLeftRmax, 
                                     Float64* pRightRmin,Float64* pRightRmax);




   void ChangeLiveLoadName(LPCTSTR strOldName,LPCTSTR strNewName);

   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation);
   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 e);
   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx);

   // this methods need to be public so the PGSuperLoadCombinationResponse object use them
   void GetVehicularLoad(ILBAMModel* pModel,LiveLoadModelType llType,VehicleIndexType vehicleIndex,IVehicularLoad** pVehicle);
   void CreateAxleConfig(ILBAMModel* pModel,ILiveLoadConfiguration* pConfig,AxleConfiguration* pAxles);
   pgsTypes::LimitState GetLimitStateFromLoadCombination(CComBSTR bstrLoadCombination);
   std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey);

private:
	DECLARE_SHARED_LOGFILE;
   IBroker* m_pBroker; // must be a weak reference (this is the agent's pointer and it is a weak refernece)
   StatusGroupIDType m_StatusGroupID;
   CComPtr<IIDArray> m_LBAMPoi;   // array for LBAM poi
   CComPtr<ILBAMLRFDFactory3> m_LBAMUtility;
   CComPtr<IUnitServer> m_UnitServer;

   PoiIDType m_NextPoi;

   // useful struct's
   typedef struct SuperstructureMemberData
   {
      CComBSTR stage;
      Float64 ea;
      Float64 ei;
      Float64 ea_defl;
      Float64 ei_defl;
   } SuperstructureMemberData;

   CProductLoadMap m_ProductLoadMap;
   ProductForceType GetProductForceType(const CComBSTR& bstrLoadGroup);

   StatusCallbackIDType m_scidInformationalError;
   StatusCallbackIDType m_scidBridgeDescriptionError;

   // There is one girder model for each girder line in the bridge model.
   std::map<GirderIndexType,CGirderModelData> m_GirderModels;
   CGirderModelData* GetGirderModel(GirderIndexType gdrLineIdx);
   void ValidateGirderModels(const CGirderKey& girderKey);
   GirderIndexType GetGirderLineIndex(const CGirderKey& girderKey);

   void CheckGirderEndGeometry(IBridge* pBridge,const CGirderKey& girderKey);
   void BuildModel(GirderIndexType gdrLineIdx);
   void BuildLBAM(GirderIndexType gdr,bool bContinuousModel,IContraflexureResponse* pContraflexureResponse,IContraflexureResponse* pDeflContraflexureResponse,ILBAMModel* pModel);
   void CreateLBAMStages(GirderIndexType gdrLineIdx,ILBAMModel* pModel);
   void CreateLBAMSpans(GirderIndexType gdrLineIdx,bool bContinuousModel,lrfdLoadModifier& loadModifier,ILBAMModel* pModel);
   void CreateLBAMSuperstructureMembers(GirderIndexType gdrLineIdx,bool bContinuousModel,lrfdLoadModifier& loadModifier,ILBAMModel* pModel);
   void CreateLBAMSuperstructureMember(Float64 length,const std::vector<SuperstructureMemberData>& vData,ISuperstructureMember** ppMbr);
   BoundaryConditionType GetLBAMBoundaryConditions(bool bContinuous,const CPierData2* pPier);
   void GetLBAMBoundaryConditions(bool bContinuous,const CTimelineManager* pTimelineMgr,GroupIndexType grpIdx,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,ISuperstructureMember* pSSMbr);
   void ApplySelfWeightLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplyDiaphragmLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplySlabLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplyOverlayLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx,bool bContinuousModel);
   void ApplyConstructionLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplyShearKeyLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel);
   void ComputeSidewalksBarriersLoadFractions();
   void ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdrLineIdx);
   void AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdrLineIdx,pgsTypes::LiveLoadType llType,std::vector<std::_tstring>& libraryLoads,ILibrary* pLibrary, ILiveLoads* pLiveLoads,IVehicularLoads* pVehicles);
   void ApplyUserDefinedLoads(ILBAMModel* pModel,GirderIndexType gdrLineIdx);
   void ApplyEquivalentPretensionForce(CGirderModelData* pModelData);
   void ApplyEquivalentPretensionForce(ILBAMModel* pModel,GirderIndexType gdrLineIdx);
   void ApplyEquivalentPostTensionForce(ILBAMModel* pModel,GirderIndexType gdrLineIdx);
   void ApplyLiveLoadDistributionFactors(GirderIndexType gdrLineIdx,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel);
   void ConfigureLoadCombinations(ILBAMModel* pModel);
   void ApplyDiaphragmLoadsAtPiers(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);
   void ApplyIntermediateDiaphragmLoads(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx);


   Float64 GetPedestrianLiveLoad(const CSpanKey& spanKey);
   void AddHL93LiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane);
   void AddFatigueLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane);
   void AddDeflectionLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane);
   void AddLegalLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane);
   void AddNotionalRatingLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane);
   void AddSHVLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane);
   void AddPedestrianLoad(const std::_tstring& strLLName,Float64 wPedLL,IVehicularLoads* pVehicles);
   void AddUserTruck(const std::_tstring& strLLName,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane,IVehicularLoads* pVehicles);
   void AddDummyLiveLoad(IVehicularLoads* pVehicles);

   void GetLiveLoadModel(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,ILiveLoadModel** ppLiveLoadModel);
   DistributionFactorType GetLiveLoadDistributionFactorType(pgsTypes::LiveLoadType llType);

   // LLDF Support Methods
   enum SpanType { PinPin, PinFix, FixPin, FixFix };
   SpanType GetSpanType(const CSpanKey& spanKey,bool bContinuous);
   void AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gV,Float64 gR,
                               Float64 gFM,Float64 gFV,Float64 gD, Float64 gPedes);
   void AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gVStart,Float64 gVEnd,Float64 gR,
                               Float64 gFM,Float64 gFVStart,Float64 gFVEnd,Float64 gD, Float64 gPedes);
   IndexType GetCfPointsInRange(IDblArray* cfLocs, Float64 spanStart, Float64 spanEnd, Float64* ptsInrg);
   void ApplyLLDF_PinPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_PinFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_FixPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_FixFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_Support(const CSpanKey& spanKey,pgsTypes::MemberEndType endType,ISupports* supports);

   CComBSTR GetLoadGroupName(pgsTypes::StrandType strandType);

   CComBSTR GetLBAMStageName(IntervalIndexType intervalIdx);
   IntervalIndexType GetIntervalFromLBAMStageName(BSTR bstrStage);


   struct SidewalkTrafficBarrierLoad
   {
      Float64 m_SidewalkLoad; // total load from both sidewalks
      Float64 m_BarrierLoad; //  total load from both barriers

      // Fractions of total barrier/sw weight that go to girder in question
      Float64 m_LeftExtBarrierFraction;
      Float64 m_LeftIntBarrierFraction;
      Float64 m_LeftSidewalkFraction;
      Float64 m_RightExtBarrierFraction;
      Float64 m_RightIntBarrierFraction;
      Float64 m_RightSidewalkFraction;
   };

   typedef std::map<CSegmentKey,SidewalkTrafficBarrierLoad> SidewalkTrafficBarrierLoadMap;
   typedef SidewalkTrafficBarrierLoadMap::iterator SidewalkTrafficBarrierLoadIterator;
   SidewalkTrafficBarrierLoadMap  m_SidewalkTrafficBarrierLoads;

   SupportIDType GetPierID(PierIndexType pierIdx);
   SupportIDType GetTemporarySupportID(SupportIndexType tsIdx);
   SupportIndexType GetTemporarySupportIndex(SupportIDType tsID);
   void GetPierTemporarySupportIDs(PierIndexType pierIdx,SupportIDType* pBackID,SupportIDType* pAheadID);

   void GetEngine(CGirderModelData* pModelData,bool bContinuous,ILBAMAnalysisEngine** pEngine);
   PoiIDType AddPointOfInterest(CGirderModelData* pModelData,const pgsPointOfInterest& poi);
   void AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints);


   void GetCantileverSlabLoads(const CSegmentKey& segmentKey, Float64* pP1, Float64* pP2);

   // Applies linear distributed loads to an LBAM member. Returns the ID of the next superstructure member to be loaded.
   // ssmbrID is the ID of the first superstructure to load
   // segmentKey identifies the segment that is being loaded
   // vLoads is a vector of loads
   MemberIDType ApplyDistributedLoads(IntervalIndexType intervalIdx,ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType ssmbrID,const CSegmentKey& segmentKey,const std::vector<LinearLoad>& vLoads,BSTR bstrStage,BSTR bstrLoadGroup);

   void GetSlabLoad(const CSegmentKey& segmentKey, std::vector<LinearLoad>& vSlabLoads, std::vector<LinearLoad>& vHaunchLoads, std::vector<LinearLoad>& vPanelLoads);
   void GetMainSpanOverlayLoad(const CSegmentKey& segmentKey, std::vector<OverlayLoad>* pOverlayLoads);
   void GetMainSpanShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads);
   void GetMainConstructionLoad(const CSegmentKey& segmentKey, std::vector<ConstructionLoad>* pConstructionLoads);

   CComBSTR GetLoadGroupNameForUserLoad(IUserDefinedLoads::UserDefinedLoadCase lc);
   CComBSTR GetLoadCombinationName(pgsTypes::LimitState limitState);
   CComBSTR GetLoadCombinationName(pgsTypes::LiveLoadType llt);
   CComBSTR GetLiveLoadName(pgsTypes::LiveLoadType llt);

   void AddLoadCase(ILoadCases* loadCases, BSTR name, BSTR description);
   HRESULT AddLoadGroup(ILoadGroups* loadGroups, BSTR name, BSTR description);

   // Returns the ID of the first LBAM superstructure member that models this segment
   MemberIDType GetFirstSuperstructureMemberID(const CSegmentKey& segmentKey);

   // Returns the number of superstructure members used to model the segment
   IndexType GetSuperstructureMemberCount(const CSegmentKey& segmentKey);

   // Returns the number of superstructure members used in the LBAM model at this pier
   IndexType GetSuperstructureMemberCount(const CPierData2* pPier);

   // Returns the number of superstructure members used in the LBAM model at this temporary support
   IndexType GetSuperstructureMemberCount(const CTemporarySupportData* pTS);

   // Converts a location in segment coordinates to a position in the LBAM
   void GetPosition(ILBAMModel* pModel,const CSegmentKey& segmentKey,Float64 Xs,MemberType* pMbrType,MemberIDType* pMbrID,Float64* pX);

   // Implementation functions and data for IBearingDesign
   void ApplyOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup,
                              MemberIDType mbrID,Float64 Pstart, Float64 Xstart,Float64 Pend, Float64 Xend, IPointLoads* pointLoads);
   void SaveOverhangPointLoads(const CSegmentKey& segmentKey,pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage,const CComBSTR& bstrLoadGroup,Float64 Pstart,Float64 Pend);

   bool GetOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx, ProductForceType type,
                              ResultsType cmbtype, Float64* pPStart, Float64* pPEnd);


   typedef std::set<COverhangLoadData> OverhangLoadSet;
   typedef OverhangLoadSet::iterator   OverhangLoadIterator;
   OverhangLoadSet m_OverhangLoadSet[2]; // index is pgsTypes::AnalysisType (only Simple or Continuous)


   void RenameLiveLoad(ILBAMModel* pModel,pgsTypes::LiveLoadType llType,LPCTSTR strOldName,LPCTSTR strNewName);
   CGirderModelData* UpdateLBAMPois(const std::vector<pgsPointOfInterest>& vPoi);
   void GetLBAM(CGirderModelData* pModelData,pgsTypes::BridgeAnalysisType bat,ILBAMModel** ppModel);
   bool CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   void ConfigureLBAMPoisForReactions(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::BridgeAnalysisType bat,bool bLiveLoadReaction);
   CollectionIndexType GetStressPointIndex(pgsTypes::StressLocation loc);
   CComBSTR GetLoadCaseName(LoadingCombinationType combo);
   bool GetLoadCaseTypeFromName(const CComBSTR& name, LoadingCombinationType* pCombo);

#if defined _DEBUG
   void VerifyAnalysisType();
#endif
};
