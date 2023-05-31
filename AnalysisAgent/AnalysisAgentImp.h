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

// AnalysisAgentImp.h : Declaration of the CAnalysisAgent

#ifndef __ANALYSISAGENT_H_
#define __ANALYSISAGENT_H_

#include "CLSID.h"

#include "resource.h"       // main symbols

#include "SegmentModelManager.h"
#include "GirderModelManager.h"

#include <EAF\EAFInterfaceCache.h>
#include <Math/LinearFunction.h>

#if defined _USE_MULTITHREADING
#include <PgsExt\ThreadManager.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
class ATL_NO_VTABLE CAnalysisAgentImp : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CAnalysisAgentImp, &CLSID_AnalysisAgent>,
   public IConnectionPointContainerImpl<CAnalysisAgentImp>,
   public IAgentEx,
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

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_ANALYSISAGENT)
DECLARE_NOT_AGGREGATABLE(CAnalysisAgentImp)

BEGIN_COM_MAP(CAnalysisAgentImp)
   COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
   COM_INTERFACE_ENTRY(IProductLoads)
   COM_INTERFACE_ENTRY(IProductForces)
   COM_INTERFACE_ENTRY(IProductForces2)
   COM_INTERFACE_ENTRY(ICombinedForces)
   COM_INTERFACE_ENTRY(ICombinedForces2)
   COM_INTERFACE_ENTRY(ILimitStateForces)
   COM_INTERFACE_ENTRY(ILimitStateForces2)
   COM_INTERFACE_ENTRY(IExternalLoading)
   COM_INTERFACE_ENTRY(IPretensionStresses)
   COM_INTERFACE_ENTRY(ICamber)
   COM_INTERFACE_ENTRY(IContraflexurePoints)
   COM_INTERFACE_ENTRY(IContinuity)
   COM_INTERFACE_ENTRY(IBearingDesign)
   COM_INTERFACE_ENTRY(IPrecompressedTensileZone)
   COM_INTERFACE_ENTRY(IReactions)
   COM_INTERFACE_ENTRY(IDeformedGirderGeometry)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(IRatingSpecificationEventSink)
   COM_INTERFACE_ENTRY(ILoadModifiersEventSink)
   COM_INTERFACE_ENTRY(ILossParametersEventSink)
   COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CAnalysisAgentImp)
END_CONNECTION_POINT_MAP()

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidVSRatio;

#if defined _USE_MULTITHREADING
   CThreadManager m_ThreadManager;
#endif

// IAgentEx
public:
   STDMETHOD(SetBroker)(IBroker* pBroker);
   STDMETHOD(RegInterfaces)();
   STDMETHOD(Init)();
   STDMETHOD(Reset)();
   STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

// IProductLoads
public:
   virtual LPCTSTR GetProductLoadName(pgsTypes::ProductForceType pfType) const override;
   virtual LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo) const override;
   virtual bool ReportAxialResults() const override;
   virtual void GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) const override;
   virtual std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX) const override;
   virtual std::vector<EquivPretensionLoad> GetEquivSegmentPostTensionLoads(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx = INVALID_INDEX) const override;
   virtual Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) const override;
   virtual bool HasPedestrianLoad() const override;
   virtual bool HasPedestrianLoad(const CGirderKey& girderKey) const override;
   virtual bool HasSidewalkLoad(const CGirderKey& girderKey) const override;
   virtual Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const override;
   virtual void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad, Float64* pFraExtLeft, Float64* pFraIntLeft, Float64* pFraExtRight,Float64* pFraIntRight) const override;
   virtual void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad, Float64* pFraLeft,Float64* pFraRight) const override;
   virtual void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const override;
   virtual bool HasConstructionLoad(const CGirderKey& girderKey) const override;
   virtual void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const override;
   virtual void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const override;
   virtual void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 Fillet, std::vector<SlabLoad>* pSlabLoads) const override;
   virtual void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const override;
   virtual void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const override;
   virtual void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const override;
   virtual void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const override;
   virtual void GetPierDiaphragmLoads(PierIndexType pierIdx, GirderIndexType gdrIdx, PIER_DIAPHRAGM_LOAD_DETAILS* pBackSide, PIER_DIAPHRAGM_LOAD_DETAILS* pAheadSide) const override;
   virtual bool HasShearKeyLoad(const CGirderKey& girderKey) const override; // checks for load in adjacent continuous beams as well as current beam
   virtual void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const override;
   virtual bool HasLongitudinalJointLoad() const override;
   virtual void GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const override;
   virtual std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   virtual pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) const override;
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const override;
   virtual std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType, const CGirderKey& segmentKey) const override;
   virtual std::vector<pgsTypes::ProductForceType> GetProductForcesForCombo(LoadingCombinationType combo) const override;
   virtual std::vector<pgsTypes::ProductForceType> GetProductForcesForGirder(const CGirderKey& girderKey) const override;

// IProductForces
public:
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) const override;
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) const override;
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;

   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const override;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=nullptr,VehicleIndexType* pTopMaxConfig=nullptr,VehicleIndexType* pBotMinConfig=nullptr,VehicleIndexType* pBotMaxConfig=nullptr) const override;

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMinRightAxleConfig = nullptr,
                                          AxleConfiguration* pMaxLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMaxRightAxleConfig = nullptr) const override;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=nullptr,AxleConfiguration* pMaxAxleConfigTop=nullptr,AxleConfiguration* pMinAxleConfigBot=nullptr,AxleConfiguration* pMaxAxleConfigBot=nullptr) const override;

   virtual void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const override;

   virtual Float64 GetDesignSlabMomentAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const override;
   virtual Float64 GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;
   virtual void GetDesignSlabStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const override;

   virtual Float64 GetDesignSlabPadMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const override;

   virtual void DumpAnalysisModels(GirderIndexType gdrIdx) const override;

   virtual void GetDeckShrinkageStresses(const pgsPointOfInterest& poi, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation,Float64* pftop,Float64* pfbot) const override;

// IProductForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType type, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const override;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const override;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=nullptr,std::vector<VehicleIndexType>* pTopMaxIndex=nullptr,std::vector<VehicleIndexType>* pBotMinIndex=nullptr,std::vector<VehicleIndexType>* pBotMaxIndex=nullptr) const override;

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = nullptr) const override;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMinAxleConfigBot=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigBot=nullptr) const override;
   // Function returns permanent deflection caused by girder dead load and modulus stiffening at storage. Values are adjusted for support location for given interval
   std::vector<Float64> GetUnrecoverableGirderDeflectionFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const override;
   std::vector<Float64> GetUnrecoverableGirderRotationFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const override;

// ICombinedForces
public:
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const override;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax) const override;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const override;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const override;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const override;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const override;

// ICombinedForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false,bool bIncludePreErectionUnrecov=true) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const override;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax) const override;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const override;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const override;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const override;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const override;

// ILimitStateForces
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const override;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   virtual void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, Float64* pMin, Float64* pMax) const override;
   virtual void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, Float64* pMin, Float64* pMax) const override;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,bool bIncludePrecamber,bool bIncludePreErectionUnrecov,Float64* pMin,Float64* pMax) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax) const override;
   virtual void GetLSReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const override;
   virtual void GetDesignStress(const StressCheckTask& task, const pgsPointOfInterest& poi,pgsTypes::StressLocation loc, const GDRCONFIG* pConfig,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const override;
   virtual void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const override;
   virtual void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const override;
   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) const override;
   virtual bool IsStrengthIIApplicable(const CGirderKey& girderKey) const override;

// ILimitStateForces2
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const override;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   virtual void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const override;
   virtual void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const override;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const override;
   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const override;

// IExternalLoading
public:
   virtual bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName) override;
   virtual bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo) override;
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) override;
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) override;
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) override;
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) override;
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) override;
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) override;
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false) const override;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const override;
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false) const override;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const override;
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

// IPretensionStresses
public:
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,Float64* pfTop,Float64* pfBot) const override;
   virtual void GetStress(IntervalIndexType intervalIdx,const PoiList& vPoi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pStresses) const override;
   virtual void GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pvfTop, std::vector<Float64>* pvfBot) const override;
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 ex,Float64 ey) const override;
   virtual Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) const override;
   virtual Float64 GetDesignStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation loc, const GDRCONFIG& config, bool bIncludeLiveLoad, pgsTypes::LimitState limitState) const override;
   virtual void GetDesignStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, const GDRCONFIG& config, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, Float64* pfTop, Float64* pfBot) const override;

// ICamber
public:
   virtual Uint32 GetCreepMethod() const override;
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate, const GDRCONFIG* pConfig = nullptr) const override;
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate,const GDRCONFIG* pConfig=nullptr) const override;
   virtual std::shared_ptr<const lrfdCreepCoefficient> GetGirderCreepModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;
   virtual std::shared_ptr<const lrfdCreepCoefficient2005> GetDeckCreepModel(IndexType deckCastingRegionIdx) const override;
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx,Float64* pDy) const override;
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy) const override;
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy) const override;
   virtual Float64 GetInitialCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDy, Float64* pRz) const override;
   virtual Float64 GetXCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetLongitudinalJointDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetScreedCamberUnfactored(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetExcessCamberEx(const pgsPointOfInterest& poi, Int16 time, Float64* pDy, Float64* pCy, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) const override;
   virtual Float64 GetDCamberForGirderScheduleUnfactored(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) const override;
   virtual void GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi, Int16 time, Float64* pUpperBound, Float64* pAvg, Float64* pLowerBound, const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetDCamberForGirderScheduleUnfactoredEx(const pgsPointOfInterest& poi, Int16 time, Float64* pUpperBound, Float64* pAvg, Float64* pLowerBound, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetLowerBoundCamberVariabilityFactor()const override;
   virtual CamberMultipliers GetCamberMultipliers(const CSegmentKey& segmentKey) const override;
   virtual bool HasPrecamber(const CGirderKey& girderKey) const override;
   virtual Float64 GetPrecamber(const CSegmentKey& segmentKey) const override;
   virtual Float64 GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum) const override;
   virtual void GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, Float64* pDprecamber, Float64* pRprecamber) const override;

// IContraflexurePoints
public:
   virtual void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const override;

// IBearingDesign
public:
   virtual bool BearingLiveLoadReactionsIncludeImpact() const override;

   virtual std::vector<PierIndexType> GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const override;

   virtual Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

   virtual void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;

   virtual void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;

   virtual Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;

   virtual void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                           Float64* pRmin,Float64* pRmax) const override;

   virtual void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                             pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                             Float64* pRmin,Float64* pRmax) const override;

// IContinuity
public:
   virtual bool IsContinuityFullyEffective(const CGirderKey& girderKey) const override;
   virtual Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) const override;

// IPrecompressedTensileZone
public:
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ) const override;
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation, const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const override;
   virtual bool IsDeckPrecompressed(const CGirderKey& girderKey) const override;

// IReactions
public:
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const override;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,REACTION* pRmin,REACTION* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const override;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,REACTION* pRmin,REACTION* pRmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) const override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) const override;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const override;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const override;

   // IDeformedGirderGeometry
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig = nullptr) const override;
   virtual void GetTopGirderElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   virtual void GetTopGirderElevationEx(const pgsPointOfInterest& poi,IntervalIndexType interval,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   virtual void GetFinishedElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const override;
   virtual Float64 GetFinishedElevation(const pgsPointOfInterest& poi,IntervalIndexType interval,Float64* pLftHaunch,Float64* pCtrHaunch,Float64* pRgtHaunch) const override;

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint) override;
   virtual HRESULT OnGirderFamilyChanged() override;
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint) override;
   virtual HRESULT OnLiveLoadChanged() override;
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName) override;
   virtual HRESULT OnConstructionLoadChanged() override;

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged() override;
   virtual HRESULT OnAnalysisTypeChanged() override;

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged() override;

// ILoadModifersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged() override;

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged() override;

private:
   DECLARE_EAF_AGENT_DATA;
   DECLARE_LOGFILE;

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecCookie;
   DWORD m_dwRatingSpecCookie;
   DWORD m_dwLoadModifierCookie;
   DWORD m_dwLossParametersCookie;

   void InitializeAnalysis(const PoiList& vPoi) const;
   mutable std::set<GirderIndexType> m_ExternalLoadState;
   std::unique_ptr<CSegmentModelManager> m_pSegmentModelManager;
   std::unique_ptr<CGirderModelManager>  m_pGirderModelManager;
   static UINT DeleteSegmentModelManager(LPVOID pParam);
   static UINT DeleteGirderModelManager(LPVOID pParam);

   // Creep models
   mutable std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS> m_CreepCoefficientDetails[2][6]; // key is span/girder hash, index to array is [Construction Rate][CreepPeriod]
   mutable std::map<CSegmentKey, std::shared_ptr<lrfdCreepCoefficient>> m_GirderCreepModels; 
   mutable std::map<IndexType, std::shared_ptr<lrfdCreepCoefficient2005>> m_DeckCreepModels; // key is deck casting region

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

   void GetCreepDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_CIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_SIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;
   void GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const;


   void GetD_Deck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_Deck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;
   void GetD_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const;

   void GetPermanentPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy,Float64* pRz) const;
   void GetPrestressDeflectionFromModel(const pgsPointOfInterest& poi, CamberModelData& modelData, pgsTypes::StrandType strandType, pgsTypes::PrestressDeflectionDatum datum, Float64* pDx, Float64* pDy, Float64* pRz) const;

   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum, Float64* pDx, Float64* pDy,Float64* pRz) const;
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData, Float64* pDx, Float64* pDy,Float64* pRz) const;

   void GetScreedCamberEx(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig,bool applyFactors,Float64* pDy,Float64* pRz) const;

   Float64 GetExcessCamberEx(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig,bool applyFactors, Float64* pDy, Float64* pCy) const;
   void GetExcessCamberEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,bool applyFactors,Float64* pDd,Float64* pDr,Float64* pCd,Float64* pCr,Float64* pEd,Float64* pEr) const;

   Float64 GetExcessCamberRotationEx(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig, bool applyFactors) const;

   Float64 GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig, bool applyFactor) const;
   void GetDCamberForGirderScheduleEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,bool applyFactors,Float64* pDy,Float64* pRz) const;

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

   void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr, pgsTypes::StressLocation topStressLocation, pgsTypes::StressLocation botStressLocation,Float64* pftop,Float64* pfbot) const;

   void GetRawPrecamber(const pgsPointOfInterest& poi, Float64 Ls,Float64* pDprecamber,Float64* pRprecamber) const;
   IntervalIndexType GetErectionInterval(const PoiList& vPoi) const;
   IntervalIndexType GetStorageInterval(const PoiList& vPoi) const;
   IntervalIndexType GetHaulingInterval(const PoiList& vPoi) const;
};

#endif //__ANALYSISAGENT_H_
