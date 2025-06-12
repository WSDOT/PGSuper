///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "SegmentModelManager.h"
#include "GirderModelManager.h"


#include <Math/LinearFunction.h>


#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DistributionFactors.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
class CAnalysisAgentImp : public WBFL::EAF::Agent,
   public IProductLoads,
   public IProductForces,
   public IProductForces2,
   public ICombinedForces,
   public ICombinedForces2,
   public ILimitStateForces,
   public ILimitStateForces2,
   public IExternalLoading,
   public IPretensionStresses,
   public ICamber,
   public IContraflexurePoints,
   public IContinuity,
   public IBearingDesign,
   public IPrecompressedTensileZone,
   public IReactions,
   public IDeformedGirderGeometry,
   public IBridgeDescriptionEventSink,
   public ISpecificationEventSink,
   public IRatingSpecificationEventSink,
   public ILoadModifiersEventSink,
   public ILossParametersEventSink
{
public:
   CAnalysisAgentImp();

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidVSRatio;

#if defined _USE_MULTITHREADING
   CThreadManager m_ThreadManager;
#endif

// Agent
public:
   std::_tstring GetName() const override { return _T("AnalysisAgent"); }
   bool RegisterInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;

// IProductLoads
public:
   LPCTSTR GetProductLoadName(pgsTypes::ProductForceType pfType) const override;
   LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo) const override;
   bool ReportAxialResults() const override;
   void GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) const override;
   std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX) const override;
   std::vector<EquivPretensionLoad> GetEquivSegmentPostTensionLoads(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx = INVALID_INDEX) const override;
   Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const override;
   Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) const override;
   bool HasPedestrianLoad() const override;
   bool HasPedestrianLoad(const CGirderKey& girderKey) const override;
   bool HasSidewalkLoad(const CGirderKey& girderKey) const override;
   Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) const override;
   Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const override;
   void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad, Float64* pFraExtLeft, Float64* pFraIntLeft, Float64* pFraExtRight,Float64* pFraIntRight) const override;
   void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad, Float64* pFraLeft,Float64* pFraRight) const override;
   void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const override;
   bool HasConstructionLoad(const CGirderKey& girderKey) const override;
   void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const override;
   void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const override;
   void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 Fillet, std::vector<SlabLoad>* pSlabLoads) const override;
   void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const override;
   void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const override;
   void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const override;
   void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const override;
   void GetPierDiaphragmLoads(PierIndexType pierIdx, GirderIndexType gdrIdx, PIER_DIAPHRAGM_LOAD_DETAILS* pBackSide, PIER_DIAPHRAGM_LOAD_DETAILS* pAheadSide) const override;
   bool HasShearKeyLoad(const CGirderKey& girderKey) const override; // checks for load in adjacent continuous beams as well as current beam
   void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const override;
   bool HasLongitudinalJointLoad() const override;
   void GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const override;
   std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) const override;
   Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType, const CGirderKey& segmentKey) const override;
   std::vector<pgsTypes::ProductForceType> GetProductForcesForCombo(LoadingCombinationType combo) const override;
   std::vector<pgsTypes::ProductForceType> GetProductForcesForGirder(const CGirderKey& girderKey) const override;

// IProductForces
public:
   pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) const override;
   pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) const override;
   Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   Float64 GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;

   Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;

   void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const override;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const override;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const override;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const override;
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=nullptr,VehicleIndexType* pTopMaxConfig=nullptr,VehicleIndexType* pBotMinConfig=nullptr,VehicleIndexType* pBotMaxConfig=nullptr) const override;

   void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,
                                  AxleConfiguration* pMinLeftAxleConfig = nullptr,
                                  AxleConfiguration* pMinRightAxleConfig = nullptr,
                                  AxleConfiguration* pMaxLeftAxleConfig = nullptr,
                                  AxleConfiguration* pMaxRightAxleConfig = nullptr) const override;
   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=nullptr,AxleConfiguration* pMaxAxleConfigTop=nullptr,AxleConfiguration* pMinAxleConfigBot=nullptr,AxleConfiguration* pMaxAxleConfigBot=nullptr) const override;

   void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const override;

   Float64 GetDesignSlabMomentAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const override;
   Float64 GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;
   void GetDesignSlabStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const override;

   Float64 GetDesignSlabPadMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;
   Float64 GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;

   void DumpAnalysisModels(GirderIndexType gdrIdx) const override;

   std::pair<Float64,Float64> GetDeckShrinkageStresses(const pgsPointOfInterest& poi, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation,const GDRCONFIG* pConfig = nullptr) const override;

// IProductForces2
public:
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;

   void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const override;
   void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const override;
   void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=nullptr,std::vector<VehicleIndexType>* pTopMaxIndex=nullptr,std::vector<VehicleIndexType>* pBotMinIndex=nullptr,std::vector<VehicleIndexType>* pBotMaxIndex=nullptr) const override;

   void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,
                                  std::vector<AxleConfiguration>* pMinLeftAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMinRightAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMaxLeftAxleConfig = nullptr,
                                  std::vector<AxleConfiguration>* pMaxRightAxleConfig = nullptr) const override;
   void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMinAxleConfigBot=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigBot=nullptr) const override;
   // Function returns permanent deflection caused by girder dead load and modulus stiffening at storage. Values are adjusted for support location for given interval
   std::vector<Float64> GetUnrecoverableGirderDeflectionFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const override;
   std::vector<Float64> GetUnrecoverableGirderXDeflectionFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const override;
   std::vector<Float64> GetUnrecoverableGirderRotationFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const override;

// ICombinedForces
public:
   Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   Float64 GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;

   void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const override;
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax) const override;
   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const override;
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const override;
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const override;
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const override;

// ICombinedForces2
public:
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;

   void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const override;
   void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax) const override;
   void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const override;
   void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const override;
   void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const override;
   void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const override;

// ILimitStateForces
public:
   void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const override;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, Float64* pMin, Float64* pMax) const override;
   void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, Float64* pMin, Float64* pMax) const override;
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,bool bIncludePrecamber,bool bIncludePreErectionUnrecov,Float64* pMin,Float64* pMax) const override;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax) const override;
   void GetLSReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const override;
   void GetDesignStress(const StressCheckTask& task, const pgsPointOfInterest& poi,pgsTypes::StressLocation loc, const GDRCONFIG* pConfig,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const override;
   void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const override;
   Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) const override;
   bool IsStrengthIIApplicable(const CGirderKey& girderKey) const override;

// ILimitStateForces2
public:
   void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const override;
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const override;
   void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const override;
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const override;

// IExternalLoading
public:
   bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName) override;
   bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo) override;
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) override;
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) override;
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) override;
   bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) override;
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) override;
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) override;
   Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   Float64 GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false) const override;
   Float64 GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false) const override;
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false) const override;
   std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false) const override;
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;
   void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

// IPretensionStresses
public:
   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig=nullptr) const override;
   std::pair<Float64,Float64> GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig=nullptr) const override;
   void GetStress(IntervalIndexType intervalIdx,const PoiList& vPoi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pStresses) const override;
   void GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pvfTop, std::vector<Float64>* pvfBot) const override;
   Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P, const WBFL::Geometry::Point2d& ecc) const override;
   Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) const override;

// ICamber
public:
   Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate, const GDRCONFIG* pConfig = nullptr) const override;
   CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,const GDRCONFIG* pConfig=nullptr) const override;
   std::shared_ptr<const WBFL::LRFD::CreepCoefficient> GetGirderCreepModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;
   std::shared_ptr<const WBFL::LRFD::CreepCoefficient2005> GetDeckCreepModel(IndexType deckCastingRegionIdx) const override;
   Float64 GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   void GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx,Float64* pDy) const override;
   Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy) const override;
   Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy) const override;
   Float64 GetInitialCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDy, Float64* pRz) const override;
   Float64 GetXCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetLongitudinalJointDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetScreedCamber(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetScreedCamberUnfactored(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetExcessCamber(const pgsPointOfInterest& poi, pgsTypes::CreepTime time,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetExcessCamberEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, Float64* pDy, Float64* pCy, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi, pgsTypes::CreepTime time,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi, pgsTypes::CreepTime time,const GDRCONFIG* pConfig=nullptr) const override;
   Float64 GetDCamberForGirderScheduleUnfactored(const pgsPointOfInterest& poi, pgsTypes::CreepTime time,const GDRCONFIG* pConfig=nullptr) const override;
   void GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, Float64* pUpperBound, Float64* pAvg, Float64* pLowerBound, const GDRCONFIG* pConfig = nullptr) const override;
   void GetDCamberForGirderScheduleUnfactoredEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, Float64* pUpperBound, Float64* pAvg, Float64* pLowerBound, const GDRCONFIG* pConfig = nullptr) const override;
   Float64 GetLowerBoundCamberVariabilityFactor()const override;
   CamberMultipliers GetCamberMultipliers(const CSegmentKey& segmentKey) const override;
   bool HasPrecamber(const CGirderKey& girderKey) const override;
   Float64 GetPrecamber(const CSegmentKey& segmentKey) const override;
   Float64 GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum) const override;
   void GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, Float64* pDprecamber, Float64* pRprecamber) const override;

// IContraflexurePoints
public:
   void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const override;

// IBearingDesign
public:
   bool BearingLiveLoadReactionsIncludeImpact() const override;

   std::vector<PierIndexType> GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const override;

   Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

   void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                        pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                        Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                        VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;

   void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                        pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                        Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                        VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;

   Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

   void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                   Float64* pRmin,Float64* pRmax) const override;

   void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                     Float64* pRmin,Float64* pRmax) const override;

// IContinuity
public:
   bool IsContinuityFullyEffective(const CGirderKey& girderKey) const override;
   Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) const override;

// IPrecompressedTensileZone
public:
   void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ) const override;
   void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation, const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const override;
   bool IsDeckPrecompressed(const CGirderKey& girderKey) const override;

// IReactions
public:
   void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,REACTION* pRmin,REACTION* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,REACTION* pRmin,REACTION* pRmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) const override;
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;
   void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) const override;
   void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const override;
   void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const override;

   // IDeformedGirderGeometry
   Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig = nullptr) const override;
   void GetTopGirderElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   void GetTopGirderElevationEx(const pgsPointOfInterest& poi,IntervalIndexType interval,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   void GetFinishedElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   Float64 GetFinishedElevation(const pgsPointOfInterest& poi,IntervalIndexType interval,Float64* pLftHaunch,Float64* pCtrHaunch,Float64* pRgtHaunch) const override;

// IBridgeDescriptionEventSink
public:
   HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   HRESULT OnGirderFamilyChanged() override;
   HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   HRESULT OnLiveLoadChanged() override;
   HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   HRESULT OnConstructionLoadChanged() override;

// ISpecificationEventSink
public:
   HRESULT OnSpecificationChanged() override;
   HRESULT OnAnalysisTypeChanged() override;

// IRatingSpecificationEventSink
public:
   HRESULT OnRatingSpecificationChanged() override;

// ILoadModifersEventSink
public:
   HRESULT OnLoadModifiersChanged() override;

// ILossParametersEventSink
public:
   HRESULT OnLossParametersChanged() override;

private:
   EAF_DECLARE_AGENT_DATA;
   DECLARE_LOGFILE;

   Uint16 m_Level;
   IDType m_BridgeDescCookie;
   IDType m_SpecCookie;
   IDType m_RatingSpecCookie;
   IDType m_LoadModifierCookie;
   IDType m_LossParametersCookie;

   void InitializeAnalysis(const PoiList& vPoi) const;
   mutable std::set<GirderIndexType> m_ExternalLoadState;
   std::unique_ptr<CSegmentModelManager> m_pSegmentModelManager;
   std::unique_ptr<CGirderModelManager>  m_pGirderModelManager;
   static UINT DeleteSegmentModelManager(LPVOID pParam);
   static UINT DeleteGirderModelManager(LPVOID pParam);

   // Creep models
   mutable std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS> m_CreepCoefficientDetails[2][6]; // key is span/girder hash, index to array is [Construction Rate][CreepPeriod]
   mutable std::map<CSegmentKey, std::shared_ptr<WBFL::LRFD::CreepCoefficient>> m_GirderCreepModels; 
   mutable std::map<IndexType, std::shared_ptr<WBFL::LRFD::CreepCoefficient2005>> m_DeckCreepModels; // key is deck casting region

   // camber models
   struct CamberModelData
   {
      CComPtr<IFem2dModel> Model;
      pgsPoiPairMap PoiMap;
      CamberModelData& operator=(const CamberModelData& other)
      {
         Model = other.Model;
         PoiMap = other.PoiMap;
         return *this;
      };
   };
   typedef std::map<CSegmentKey,CamberModelData> CamberModels;
   mutable CamberModels m_PrestressDeflectionModels;
   mutable CamberModels m_InitialTempPrestressDeflectionModels;
   mutable CamberModels m_ReleaseTempPrestressDeflectionModels;

   mutable GDRCONFIG m_CacheConfig;
   mutable CamberModelData m_CacheConfig_PrestressDeflectionModel;
   mutable CamberModelData m_CacheConfig_InitialTempPrestressDeflectionModels;
   mutable CamberModelData m_CacheConfig_ReleaseTempPrestressDeflectionModels;
   void InvalidateCache();

   void ValidateCamberModels(const CSegmentKey& segmentKey) const;
   void ValidateCamberModels(const GDRCONFIG* pConfig) const;

   // using pConfig == nullptr means use the current configuration

   CamberModelData BuildCamberModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig=nullptr) const;
   void BuildTempCamberModel(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData) const;
   void InvalidateCamberModels();
   CamberModelData GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models) const;

   mutable GDRCONFIG m_SlabOffsetDesignCacheConfig;
   mutable CamberModelData m_CacheConfig_SlabOffsetDesignModel;

   void InValidateSlabOffsetDesignModel();
   void ValidateSlabOffsetDesignModel(const GDRCONFIG* pConfig) const;
   void BuildSlabOffsetDesignModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,CamberModelData* pModelData) const;
   Float64 GetDesignMomentAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const;
   void GetDesignDeflectionAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;

   std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX) const;

   void Invalidate(bool clearStatus=true);

   // adds points of interest to a camber model
   PoiIDPairType AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi) const;

   void GetCreepDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_CIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_SIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, pgsTypes::CreepTime constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;


   void GetD_Deck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_Deck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;

   void GetPermanentPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy,Float64* pRz) const;
   void GetPrestressDeflectionFromModel(const pgsPointOfInterest& poi, CamberModelData& modelData, pgsTypes::StrandType strandType, pgsTypes::PrestressDeflectionDatum datum, Float64* pDx, Float64* pDy, Float64* pRz) const;

   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData, Float64* pDx, Float64* pDy,Float64* pRz) const;

   void GetScreedCamberEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, const GDRCONFIG* pConfig,bool applyFactors,Float64* pDy,Float64* pRz) const;

   Float64 GetExcessCamberEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time,const GDRCONFIG* pConfig,bool applyFactors, Float64* pDy, Float64* pCy) const;
   void GetExcessCamberEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime time,bool applyFactors,Float64* pDd,Float64* pDr,Float64* pCd,Float64* pCr,Float64* pEd,Float64* pEr) const;

   Float64 GetExcessCamberRotationEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, const GDRCONFIG* pConfig, bool applyFactors) const;

   Float64 GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi, pgsTypes::CreepTime time, const GDRCONFIG* pConfig, bool applyFactor) const;
   void GetDCamberForGirderScheduleEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData, pgsTypes::CreepTime time,bool applyFactors,Float64* pDy,Float64* pRz) const;

   CamberMultipliers GetCamberMultipliersEx(const CSegmentKey& segmentKey, bool applyFactors) const;
   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc) const;
   void GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz) const;
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetLongitudinalJointDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64* pDy, Float64* pRz) const;
   void GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   
   void GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const;
   void GetDesignSlabPadStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const;

   Float64 GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey, LoadingEvent le,const GDRCONFIG* pConfig=nullptr) const;
   LoadingEvent GetLoadingEvent(CreepPeriod creepPeriod) const;

   Float64 GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,IntervalIndexType intervalIdx) const;

   void GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;
   void GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   void GetTimeStepStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;
   void GetElasticStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const;

   void GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;
   void GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const;

   void GetTiltedGirderLateralStresses(const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, pgsTypes::StressLocation topLocation, pgsTypes::StressLocation botLocation, std::vector<Float64>* pfTop, std::vector<Float64>* pfBot) const;

   std::vector<Float64> GetTimeStepPrestressAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;
   std::vector<Float64> GetTimeStepPrestressMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const;

   void ApplyPrecamberElevationAdjustment(IntervalIndexType intervalIdx, const PoiList& vPoi, std::vector<Float64>* pDeflection1, std::vector<Float64>* pDeflection2) const;
   void ApplyPrecamberRotationAdjustment(IntervalIndexType intervalIdx, const PoiList& vPoi, std::vector<Float64>* pRotation1, std::vector<Float64>* pRotation2) const;
   void ApplyElevationAdjustment(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2) const;
   void ApplyRotationAdjustment(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<Float64>* pRotation1,std::vector<Float64>* pRotation2) const;

   std::shared_ptr<WBFL::Math::LinearFunction> GetUnrecoverableDeflectionVariables(sagInterval sagint,pgsTypes::BridgeAnalysisType bat,IntervalIndexType storageIntervalIdx,const CSegmentKey& segmentKey,Float64* pDeflectionFactor) const;

   // Girder elevations for A dimension haunch input, and for direct haunch input are computed differently
   void GetTopGirderElevation4ADim(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const;
   void GetTopGirderElevationEx4DirectHaunch(const pgsPointOfInterest& poi,IntervalIndexType interval,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const;
   Float64 GetTopCLGirderElevationEx4DirectHaunch(const pgsPointOfInterest& poi,IntervalIndexType interval) const;

   // Girder chord must be adjusted so that zero deflection occurs at segment chord datums for elevation comp's. These functions 
   // compute and cache the adjustment
   Float64 GetElevationDeflectionAdjustment(const pgsPointOfInterest& poi) const;
   const WBFL::Math::LinearFunction& GetElevationDeflectionAdjustmentFunction(const CSegmentKey& segmentKey) const;
   void ValidateElevationDeflectionAdjustment(const CSegmentKey& girderKey) const;
   mutable std::map<CSegmentKey,WBFL::Math::LinearFunction> m_ElevationDeflectionAdjustmentFunctions; // linear functions that represent the adjustment

   void ComputeTimeDependentEffects(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const;

   void IsDeckInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,bool* pbTopPTZ,bool* pbBotPTZ) const;
   void IsGirderInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const;

   std::pair<Float64,Float64> GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation) const;

   void GetRawPrecamber(const pgsPointOfInterest& poi, Float64 Ls,Float64* pDprecamber,Float64* pRprecamber) const;
   IntervalIndexType GetErectionInterval(const PoiList& vPoi) const;
   IntervalIndexType GetStorageInterval(const PoiList& vPoi) const;
   IntervalIndexType GetHaulingInterval(const PoiList& vPoi) const;
};
