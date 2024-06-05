///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
   void DumpAnalysisModels(GirderIndexType gdrLineIdx) const;

   // IProduct Loads
   CComBSTR GetLoadGroupName(pgsTypes::ProductForceType type) const;
   void GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoad,std::vector<ClosureJointLoad>* pClosureJointLoads) const;
   Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const;
   Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) const;
   bool HasPedestrianLoad() const;
   bool HasPedestrianLoad(const CGirderKey& girderKey) const;
   bool HasSidewalkLoad(const CGirderKey& girderKey) const;
   Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) const;
   Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const;
   void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,
                                              Float64* pFraExtLeft, Float64* pFraIntLeft,
                                              Float64* pFraExtRight,Float64* pFraIntRight) const;
   void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad, Float64* pFraLeft,Float64* pFraRight) const;

   void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const;
   bool HasConstructionLoad(const CGirderKey& girderKey) const;
   void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const;
   void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const;

   void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 AssumedExcessCamber, std::vector<SlabLoad>* pSlabLoads) const;

   void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const;
   void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const;
   void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const;
   void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const;
   void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, PIER_DIAPHRAGM_LOAD_DETAILS* pBackSide, PIER_DIAPHRAGM_LOAD_DETAILS* pAheadSide) const;
   void GetClosureJointLoads(const CClosureKey& closureKey,std::vector<ClosureJointLoad>* pLoads) const;

   bool HasShearKeyLoad(const CGirderKey& girderKey) const; // checks for load in adjacent continuous beams as well as current beam
   void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const;

   bool HasLongitudinalJointLoad() const;
   void GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const;

   std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const;
   pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const;
   VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) const;
   Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const;


   // IProductForces
   Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   Float64 GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const;
   Float64 GetReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, PierIndexType pierIdx, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;
   REACTION GetReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, SupportIndexType supportIdx, pgsTypes::SupportType supportType, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;

   void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const;
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const;
   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const;
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pier,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const;
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=nullptr,VehicleIndexType* pTopMaxConfig=nullptr,VehicleIndexType* pBotMinConfig=nullptr,VehicleIndexType* pBotMaxConfig=nullptr) const;

   void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,
                                  AxleConfiguration* pMinLeftAxleConfig = nullptr,
                                  AxleConfiguration* pMinRightAxleConfig = nullptr,
                                  AxleConfiguration* pMaxLeftAxleConfig = nullptr,
                                  AxleConfiguration* pMaxRightAxleConfig = nullptr) const;
   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=nullptr,AxleConfiguration* pMaxAxleConfigTop=nullptr,AxleConfiguration* pMinAxleConfigBot=nullptr,AxleConfiguration* pMaxAxleConfigBot=nullptr) const;
   void GetDeflLiveLoadDeflection(IProductForces::DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const;

   // returns fTop,fBottom
   std::pair<Float64,Float64> GetDeckShrinkageStresses(const pgsPointOfInterest& poi, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation, const GDRCONFIG* pConfig = nullptr) const;

   // IProductForce2
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const;
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const;
   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const;
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const;
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=nullptr,std::vector<VehicleIndexType>* pTopMaxIndex=nullptr,std::vector<VehicleIndexType>* pBotMinIndex=nullptr,std::vector<VehicleIndexType>* pBotMaxIndex=nullptr) const;

   void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,
                                  std::vector<AxleConfiguration>* pMinLeftAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMinRightAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMaxLeftAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMaxRightAxleConfig = nullptr) const;
   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const;
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx, pgsTypes::LiveLoadType llType, VehicleIndexType vehicleIdx, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact, bool bIncludeLLDF, pgsTypes::StressLocation topLocation, pgsTypes::StressLocation botLocation, std::vector<Float64>* pfTopMin, std::vector<Float64>* pfTopMax, std::vector<Float64>* pfBotMin, std::vector<Float64>* pfBotMax, std::vector<AxleConfiguration>* pMinAxleConfigurationTop = nullptr, std::vector<AxleConfiguration>* pMaxAxleConfigurationTop = nullptr, std::vector<AxleConfiguration>* pMinAxleConfigurationBot = nullptr, std::vector<AxleConfiguration>* pMaxAxleConfigurationBot = nullptr) const;

   // ICombinedForces
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const;
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax) const;
   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const;
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const;
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const;
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const;

   // ICombinedForces2
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludePreErectionUnrecov) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludePreErectionUnrecov) const;
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const;
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact, std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax) const;
   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const;
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const;
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const;
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const;

   // ILimitStateForces
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const;
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,Float64* pMin,Float64* pMax) const;
   void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax) const;
   void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const;
   void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const;

   // ILimitStateForces2
   void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludePreErectionUnrecov,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;

   std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const;

   // IExternalLoading
   bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName);
   bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;
   std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;

   // IContraflexurePoints
   void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const;

   // IReactions - support functions for analysis agent
   std::vector<REACTION> GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;
   std::vector<REACTION> GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;
   void GM_GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax) const;
   void GM_GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const;
   void GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const;
   void GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const;
   void GM_GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const;
   void GM_GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const;

   void GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const;

private:
   void GM_GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const;

   // IBearingDesign
public:
   Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;

   void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const;

   void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                   Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                   VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const;


   Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;

   void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                           Float64* pRmin,Float64* pRmax) const;

   void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                     Float64* pRmin,Float64* pRmax) const;


   void ChangeLiveLoadName(LPCTSTR strOldName,LPCTSTR strNewName);

   // stress in the girder due to prestressing
   std::pair<Float64, Float64> GetStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig = nullptr) const;
   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation,Float64 P, const WBFL::Geometry::Point2d& ecc, const GDRCONFIG* pConfig = nullptr) const;
   Float64 GetStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::SectionPropertyType spType, pgsTypes::StressLocation stressLocation, Float64 P, const WBFL::Geometry::Point2d& ecc, const GDRCONFIG* pConfig = nullptr) const;
   Float64 GetStressFromSegmentPT(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation, DuctIndexType ductIdx) const;
   Float64 GetStressFromGirderPT(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation, DuctIndexType ductIdx) const;

   // this methods need to be public so the PGSuperLoadCombinationResponse object use them
   void GetVehicularLoad(ILBAMModel* pModel,LiveLoadModelType llType,VehicleIndexType vehicleIdx,IVehicularLoad** pVehicle) const;
   void CreateAxleConfig(ILBAMModel* pModel,ILiveLoadConfiguration* pConfig,AxleConfiguration* pAxles) const;
   pgsTypes::LimitState GetLimitStateFromLoadCombination(CComBSTR bstrLoadCombination) const;
   std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey) const;

private:
	DECLARE_SHARED_LOGFILE;
   IBroker* m_pBroker; // must be a weak reference (this is the agent's pointer and it is a weak refernece)
   StatusGroupIDType m_StatusGroupID;
   mutable CComPtr<IIDArray> m_LBAMPoi;   // array for LBAM poi (this will be a problem for concurrency)
   CComPtr<ILBAMLRFDFactory3> m_LBAMUtility;
   CComPtr<IUnitServer> m_UnitServer;

   mutable PoiIDType m_NextPoi;

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
   pgsTypes::ProductForceType GetProductForceType(const CComBSTR& bstrLoadGroup) const;

   StatusCallbackIDType m_scidInformationalError;
   StatusCallbackIDType m_scidBridgeDescriptionError;

   // There is one girder model for each girder line in the bridge model.
   mutable std::map<GirderIndexType,CGirderModelData> m_GirderModels;
   CGirderModelData* GetGirderModel(GirderIndexType gdrLineIdx) const;
   CGirderModelData* GetGirderModel(GirderIndexType gdrLineIdx,pgsTypes::BridgeAnalysisType bat) const;
   void ValidateGirderModels(const CGirderKey& girderKey) const;
   GirderIndexType GetGirderLineIndex(const CGirderKey& girderKey) const;

   void CheckGirderEndGeometry(IBridge* pBridge,const CGirderKey& girderKey) const;
   void BuildModel(GirderIndexType gdrLineIdx) const;
   void BuildModel(GirderIndexType gdrLineIdx,pgsTypes::BridgeAnalysisType bat) const;
   void BuildLBAM(GirderIndexType gdr,bool bContinuousModel,IContraflexureResponse* pContraflexureResponse,IContraflexureResponse* pDeflContraflexureResponse,ILBAMModel* pModel) const;
   void CreateLBAMStages(GirderIndexType gdrLineIdx,ILBAMModel* pModel) const;
   void CreateLBAMSpans(GirderIndexType gdrLineIdx,bool bContinuousModel,const WBFL::LRFD::LoadModifier& loadModifier,ILBAMModel* pModel) const;
   void CreateLBAMSupport(GirderIndexType gdrLineIdx,bool bContinuousModel,const WBFL::LRFD::LoadModifier& loadModifier,const CPierData2* pPier,ISupport** ppSupport) const;
   void CreateLBAMSuperstructureMembers(GirderIndexType gdrLineIdx,bool bContinuousModel,WBFL::LRFD::LoadModifier& loadModifier,ILBAMModel* pModel) const;
   void CreateLBAMSuperstructureMember(Float64 length,const std::vector<SuperstructureMemberData>& vData,ISuperstructureMember** ppMbr) const;
   BoundaryConditionType GetLBAMBoundaryConditions(bool bContinuous,const CPierData2* pPier) const;
   void GetLBAMBoundaryConditions(bool bContinuous,const CTimelineManager* pTimelineMgr,GroupIndexType grpIdx,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,ISuperstructureMember* pSSMbr) const;
   void ApplySelfWeightLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;
   MemberIDType ApplyClosureJointSelfWeightLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType mbrID,const CClosureKey& closureKey,const std::vector<ClosureJointLoad>& vClosureJointLoads) const;
   void ApplyDiaphragmLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;
   void ApplySlabLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;
   void ApplyOverlayLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx,bool bContinuousModel) const;
   void ApplyConstructionLoad(ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;
   void ApplyShearKeyLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType, GirderIndexType gdrLineIdx) const;
   void ApplyLongitudinalJointLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType, GirderIndexType gdrLineIdx) const;
   void ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdr,bool bContinuousModel) const;
   void ComputeSidewalksBarriersLoadFractions() const;
   void ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdrLineIdx) const;
   void AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdrLineIdx,pgsTypes::LiveLoadType llType,std::vector<std::_tstring>& libraryLoads,ILibrary* pLibrary, ILiveLoads* pLiveLoads,IVehicularLoads* pVehicles) const;
   void ApplyUserDefinedLoads(ILBAMModel* pModel,GirderIndexType gdrLineIdx) const;
   void ApplyPostTensionDeformation(ILBAMModel* pModel,GirderIndexType gdrLineIdx) const;
   void ApplyLiveLoadDistributionFactors(GirderIndexType gdrLineIdx,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel) const;
   void ConfigureLoadCombinations(ILBAMModel* pModel) const;
   void ApplyDiaphragmLoadsAtPiers(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;
   void ApplyIntermediateDiaphragmLoads(ILBAMModel* pModel, pgsTypes::AnalysisType analysisType,GirderIndexType gdrLineIdx) const;

   void GetMainSpanSlabLoadEx(const CSegmentKey& segmentKey, bool doCondense, bool useDesignValues , Float64 Astart, Float64 Aend, Float64 AssumedExcessCamber,  std::vector<SlabLoad>* pSlabLoads) const;

   typedef struct PostTensionStrainLoad
   {
      PostTensionStrainLoad(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,Float64 Xstart,Float64 Xend,Float64 eStart,Float64 eEnd,Float64 rStart,Float64 rEnd) :
         startSpanIdx(startSpanIdx),endSpanIdx(endSpanIdx),Xstart(Xstart),Xend(Xend),eStart(eStart),eEnd(eEnd),rStart(rStart),rEnd(rEnd)
      {
          ATLASSERT(startSpanIdx < endSpanIdx || (startSpanIdx == endSpanIdx && Xstart < Xend));
      }
      SpanIndexType startSpanIdx, endSpanIdx;
      Float64 Xstart, Xend;
      Float64 eStart, eEnd;
      Float64 rStart, rEnd;
   } PostTensionStrainLoad;
   void GetPostTensionDeformationLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,std::vector<PostTensionStrainLoad>& strainLoads) const;


   Float64 GetPedestrianLiveLoad(const CSpanKey& spanKey) const;
   Float64 GetPedestrianLiveLoad(const CSegmentKey& segmentKey) const;
   void AddHL93LiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const;
   void AddFatigueLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const;
   void AddDeflectionLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane) const;
   void AddLegalLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const;
   void AddNotionalRatingLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,Float64 IMtruck,Float64 IMlane) const;
   void AddEmergencyLiveLoad(ILBAMModel* pModel, ILibrary* pLibrary, pgsTypes::LiveLoadType llType, Float64 IMtruck, Float64 IMlane) const;
   void AddSHVLoad(ILBAMModel* pModel, ILibrary* pLibrary, pgsTypes::LiveLoadType llType, Float64 IMtruck, Float64 IMlane) const;
   void AddPedestrianLoad(const std::_tstring& strLLName,Float64 wPedLL,IVehicularLoads* pVehicles) const;
   void AddUserTruck(const std::_tstring& strLLName,ILibrary* pLibrary,Float64 IMtruck,Float64 IMlane,IVehicularLoads* pVehicles) const;
   void AddDummyLiveLoad(IVehicularLoads* pVehicles) const;

   void GetLiveLoadModel(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,ILiveLoadModel** ppLiveLoadModel) const;
   DistributionFactorType GetLiveLoadDistributionFactorType(pgsTypes::LiveLoadType llType) const;

   // LLDF Support Methods
   enum SpanType { PinPin, PinFix, FixPin, FixFix };
   SpanType GetSpanType(const CSpanKey& spanKey,bool bContinuous) const;
   void AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gV,Float64 gR,
                               Float64 gFM,Float64 gFV,Float64 gD, Float64 gPedes) const;
   void AddDistributionFactors(IDistributionFactors* factors,Float64 length,Float64 gpM,Float64 gnM,Float64 gVStart,Float64 gVEnd,Float64 gR,
                               Float64 gFM,Float64 gFVStart,Float64 gFVEnd,Float64 gD, Float64 gPedes) const;
   IndexType GetCfPointsInRange(IDblArray* cfLocs, Float64 spanStart, Float64 spanEnd, Float64* ptsInrg) const;
   void ApplyLLDF_PinPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const;
   void ApplyLLDF_PinFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const;
   void ApplyLLDF_FixPin(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const;
   void ApplyLLDF_FixFix(const CSpanKey& spanKey,IDblArray* cf_locs,IDistributionFactors* distFactors) const;
   void ApplyLLDF_Support(const CSpanKey& spanKey,pgsTypes::MemberEndType endType,ISupports* supports,ITemporarySupports* tempSupports) const;

   void GetLoadGroupName(pgsTypes::StrandType strandType,CComBSTR& bstrLoadGroupX,CComBSTR& bstrLoadGroupY) const;

   CComBSTR GetLBAMStageName(IntervalIndexType intervalIdx) const;
   IntervalIndexType GetIntervalFromLBAMStageName(const pgsPointOfInterest& poi, BSTR bstrStage) const;


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
   mutable SidewalkTrafficBarrierLoadMap  m_SidewalkTrafficBarrierLoads;

   SupportIDType GetPierID(PierIndexType pierIdx) const;
   SupportIDType GetTemporarySupportID(SupportIndexType tsIdx) const;
   SupportIndexType GetTemporarySupportIndex(SupportIDType tsID) const;

   // Returns the temporary support IDs used in the LBAM model. Not all permanent piers use these temporary
   // supports so the LBAM model is not guarenteed to have temporary supports with these IDs. 
   // Use GetPierSupportIDs to get the IDs need to get data from the LBAM
   void GetPierTemporarySupportIDs(PierIndexType pierIdx,SupportIDType* pBackID,SupportIDType* pAheadID) const;

   // Gets the correct support IDs for getting reaction data from the LBAM 
   void GetPierSupportIDs(const ReactionLocation& location, SupportIDType* pBackID, SupportIDType* pAheadID) const;

   void GetEngine(CGirderModelData* pModelData,bool bContinuous,ILBAMAnalysisEngine** pEngine) const;
   PoiIDType AddPointOfInterest(CGirderModelData* pModelData,const pgsPointOfInterest& poi) const;
   void AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints) const;


   void GetCantileverSlabLoads(const CSegmentKey& segmentKey, Float64* pP1, Float64* pP2) const;

   // Applies linear distributed load to the LBAM members that represent a segment. Returns the ID of the next superstructure member to be loaded.
   // ssmbrID is the ID of the first superstructure to load
   // segmentKey identifies the segment that is being loaded
   // vLoads is a vector of loads
   MemberIDType ApplyDistributedLoadsToSegment(IntervalIndexType intervalIdx,ILBAMModel* pModel,pgsTypes::AnalysisType analysisType,MemberIDType ssmbrID,const CSegmentKey& segmentKey,const std::vector<LinearLoad>& vLoads,BSTR bstrStage,BSTR bstrLoadGroup) const;

   void GetSlabLoad(const std::vector<SlabLoad>& vBasicSlabLoads, IndexType castingRegionIdx, std::vector<LinearLoad>& vSlabLoads, std::vector<LinearLoad>& vHaunchLoads, std::vector<LinearLoad>& vPanelLoads) const;
   void GetMainSpanOverlayLoad(const CSegmentKey& segmentKey, std::vector<OverlayLoad>* pOverlayLoads) const;
   void GetMainSpanShearKeyLoad(const CSegmentKey& segmentKey, std::vector<ShearKeyLoad>* pLoads) const;
   void GetMainSpanLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const;
   void GetMainConstructionLoad(const CSegmentKey& segmentKey, std::vector<ConstructionLoad>* pConstructionLoads) const;

   CComBSTR GetLoadGroupNameForUserLoad(IUserDefinedLoads::UserDefinedLoadCase lc) const;
   CComBSTR GetLoadCombinationName(pgsTypes::LimitState limitState) const;
   CComBSTR GetLiveLoadName(pgsTypes::LiveLoadType llt) const;

   void AddLoadCase(ILoadCases* loadCases, BSTR name, BSTR description) const;
   HRESULT AddLoadGroup(ILoadGroups* loadGroups, BSTR name, BSTR description) const;

   // Returns the ID of the first LBAM superstructure member that models this segment
   MemberIDType GetFirstSuperstructureMemberID(const CSegmentKey& segmentKey) const;

   // Returns the number of superstructure members used to model the segment
   IndexType GetSuperstructureMemberCount(const CSegmentKey& segmentKey) const;

   // Returns the number of superstructure members used in the LBAM model at this pier
   IndexType GetSuperstructureMemberCount(const CPierData2* pPier,GirderIndexType gdrIdx) const;

   // Returns the number of superstructure members used in the LBAM model at this temporary support
   IndexType GetSuperstructureMemberCount(const CTemporarySupportData* pTS,GirderIndexType gdrIdx) const;

   // Converts a location in segment coordinates to a position in the LBAM
   void GetPosition(ILBAMModel* pLBAMModel,const CSegmentKey& segmentKey,Float64 Xs,MemberType* pMbrType,MemberIDType* pMbrID,Float64* pX) const;

   // Converts a location in segment coordinates to a load position in the LBAM
   void GetLoadPosition(ILBAMModel* pLBAMModel,const CSegmentKey& segmentKey,Float64 Xs,bool bLoadCantilevers,MemberType* pMbrType,MemberIDType* pMbrID,Float64* pXmbr) const;

   // Implementation functions and data for IBearingDesign
   void ApplyOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage, const CComBSTR& bstrLoadGroup,
                              MemberIDType mbrID,Float64 Pstart, Float64 Xstart,Float64 Pend, Float64 Xend, IPointLoads* pointLoads) const;
   void SaveOverhangPointLoads(const CSegmentKey& segmentKey,pgsTypes::AnalysisType analysisType,const CComBSTR& bstrStage,const CComBSTR& bstrLoadGroup,Float64 Pstart,Float64 Pend) const;

   bool GetOverhangPointLoads(const CSegmentKey& segmentKey, pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx, pgsTypes::ProductForceType type,
                              ResultsType cmbtype, Float64* pPStart, Float64* pPEnd) const;


   typedef std::set<COverhangLoadData> OverhangLoadSet;
   mutable OverhangLoadSet m_OverhangLoadSet[2]; // index is pgsTypes::AnalysisType (only Simple or Continuous)


   void RenameLiveLoad(ILBAMModel* pModel,pgsTypes::LiveLoadType llType,LPCTSTR strOldName,LPCTSTR strNewName);
   CGirderModelData* UpdateLBAMPois(const PoiList& vPoi) const;
   void GetLBAM(CGirderModelData* pModelData,pgsTypes::BridgeAnalysisType bat,ILBAMModel** ppModel) const;
   bool CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(ILBAMModel* pModel,const pgsPoiMap& poiMap,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   void ConfigureLBAMPoisForReactions(const CGirderKey& girderKey, SupportIndexType supportIdx, pgsTypes::SupportType supportType) const;
   IndexType GetStressPointIndex(pgsTypes::StressLocation loc) const;
   CComBSTR GetLoadCaseName(LoadingCombinationType combo) const;
   bool GetLoadCaseTypeFromName(const CComBSTR& name, LoadingCombinationType* pCombo) const;

   // Returns two points of interest that represent where the segment is supported at erection
   // These are the "hard" supports which would be first, permanent piers, second erection towers, third strongbacks
   std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>>  GetSegmentErectionSupportLocations(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const;

   // a method to get a consistent list of POIs for linear load applications
   void GetLinearLoadPointsOfInterest(const CSegmentKey& segmentKey, PoiList* pvPoi) const;

   REACTION GetBearingReaction(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, SupportIDType supportID, const CGirderKey& girderKey, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const;

#if defined _DEBUG
   void VerifyAnalysisType() const;
#endif

   bool VerifyPoi(const PoiList& vPoi) const; // all POI must be for the same girder
};
