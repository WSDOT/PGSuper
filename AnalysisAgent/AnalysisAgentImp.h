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

// AnalysisAgentImp.h : Declaration of the CAnalysisAgent

#ifndef __ANALYSISAGENT_H_
#define __ANALYSISAGENT_H_

#include "resource.h"       // main symbols

#include "SegmentModelManager.h"
#include "GirderModelManager.h"

#include <EAF\EAFInterfaceCache.h>

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
   virtual LPCTSTR GetProductLoadName(pgsTypes::ProductForceType pfType) override;
   virtual LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo) override;
   virtual bool ReportAxialResults() override;
   virtual void GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) override;
   virtual std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX) override;
   virtual Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) override;
   virtual Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) override;
   virtual bool HasPedestrianLoad() override;
   virtual bool HasPedestrianLoad(const CGirderKey& girderKey) override;
   virtual bool HasSidewalkLoad(const CGirderKey& girderKey) override;
   virtual Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) override;
   virtual Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) override;
   virtual void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,
                                              Float64* pFraExtLeft, Float64* pFraIntLeft,
                                              Float64* pFraExtRight,Float64* pFraIntRight) override;
   virtual void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad,
                                        Float64* pFraLeft,Float64* pFraRight) override;

   virtual void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) override;
   virtual bool HasConstructionLoad(const CGirderKey& girderKey) override;
   virtual void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) override;
   virtual void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) override;

   virtual void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 Fillet, std::vector<SlabLoad>* pSlabLoads) override;

   virtual void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) override;
   virtual void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) override;
   virtual void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) override;
   virtual void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) override;
   virtual void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead) override;

   virtual bool HasShearKeyLoad(const CGirderKey& girderKey) override; // checks for load in adjacent continuous beams as well as current beam
   virtual void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) override;

   virtual std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) override;
   virtual pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) override;
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) override;
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) override;

// IProductForces
public:
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) override;
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) override;
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) override;

   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) override;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=nullptr,VehicleIndexType* pTopMaxConfig=nullptr,VehicleIndexType* pBotMinConfig=nullptr,VehicleIndexType* pBotMaxConfig=nullptr) override;

   virtual std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& segmentKey) override;
   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMinRightAxleConfig = nullptr,
                                          AxleConfiguration* pMaxLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMaxRightAxleConfig = nullptr) override;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=nullptr,AxleConfiguration* pMaxAxleConfigTop=nullptr,AxleConfiguration* pMinAxleConfigBot=nullptr,AxleConfiguration* pMaxAxleConfigBot=nullptr) override;

   virtual void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) override;

   virtual Float64 GetDesignSlabMomentAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) override;
   virtual Float64 GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) override;
   virtual void GetDesignSlabStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) override;

   virtual Float64 GetDesignSlabPadMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) override;
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) override;

   virtual void DumpAnalysisModels(GirderIndexType gdrIdx) override;

   virtual void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot) override;

// IProductForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) override;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) override;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) override;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) override;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=nullptr,std::vector<VehicleIndexType>* pTopMaxIndex=nullptr,std::vector<VehicleIndexType>* pBotMinIndex=nullptr,std::vector<VehicleIndexType>* pBotMaxIndex=nullptr) override;

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = nullptr) override;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigTop=nullptr,std::vector<AxleConfiguration>* pMinAxleConfigBot=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigBot=nullptr) override;

// ICombinedForces
public:
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) override;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) override;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax) override;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) override;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) override;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) override;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) override;

// ICombinedForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) override;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) override;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax) override;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) override;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) override;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) override;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) override;

// ILimitStateForces
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) override;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) override;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) override;
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,Float64* pMin,Float64* pMax) override;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,Float64* pMin,Float64* pMax) override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax) override;
   virtual void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) override;
   virtual void GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc, const GDRCONFIG* pConfig,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) override;
   virtual void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) override;
   virtual void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) override;
   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) override;
   virtual bool IsStrengthIIApplicable(const CGirderKey& girderKey) override;

// ILimitStateForces2
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) override;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax) override;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) override;
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax) override;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax) override;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) override;
   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat) override;

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
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) override;
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) override;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) override;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) override;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) override;
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;

// IPretensionStresses
public:
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad) override;
   virtual void GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad,Float64* pfTop,Float64* pfBot) override;
   virtual std::vector<Float64> GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad) override;
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e) override;
   virtual Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) override;
   virtual Float64 GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config,bool bIncludeLiveLoad) override;

// ICamber
public:
   virtual Uint32 GetCreepMethod() override;
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate, const GDRCONFIG* pConfig = nullptr) override;
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) override;
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig =nullptr) override;
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) override;
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig=nullptr) override;

   virtual Float64 GetLowerBoundCamberVariabilityFactor()const override;

   virtual CamberMultipliers GetCamberMultipliers(const CSegmentKey& segmentKey) override;

// IContraflexurePoints
public:
   virtual void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) override;

// IBearingDesign
public:
   virtual std::vector<PierIndexType> GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey) override;

   virtual Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;

   virtual void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) override;

   virtual void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) override;

   virtual Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;

   virtual void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                           Float64* pRmin,Float64* pRmax) override;

   virtual void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                             pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                             Float64* pRmin,Float64* pRmax) override;

// IContinuity
public:
   virtual bool IsContinuityFullyEffective(const CGirderKey& girderKey) override;
   virtual Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) override;

// IPrecompressedTensileZone
public:
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ) override;
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation, const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) override;
   virtual bool IsDeckPrecompressed(const CGirderKey& girderKey) override;

// IReactions
public:
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) override;
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) override;
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax) override;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax) override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) override;
   virtual void GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,REACTION* pRmin,REACTION* pRmax) override;
   virtual void GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax) override;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,REACTION* pRmin,REACTION* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) override;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,REACTION* pRmin,REACTION* pRmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) override;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) override;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) override;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) override;

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

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecCookie;
   DWORD m_dwRatingSpecCookie;
   DWORD m_dwLoadModifierCookie;
   DWORD m_dwLossParametersCookie;

   void InitializeAnalysis(const std::vector<pgsPointOfInterest>& vPoi);
   std::set<GirderIndexType> m_ExternalLoadState;
   std::unique_ptr<CSegmentModelManager> m_pSegmentModelManager;
   std::unique_ptr<CGirderModelManager>  m_pGirderModelManager;
   static UINT DeleteSegmentModelManager(LPVOID pParam);
   static UINT DeleteGirderModelManager(LPVOID pParam);

   // Camber models
   std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS> m_CreepCoefficientDetails[2][6]; // key is span/girder hash, index to array is [Construction Rate][CreepPeriod]

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
   CamberModels m_PrestressDeflectionModels;
   CamberModels m_InitialTempPrestressDeflectionModels;
   CamberModels m_ReleaseTempPrestressDeflectionModels;

   GDRCONFIG m_CacheConfig;
   CamberModelData m_CacheConfig_PrestressDeflectionModel;
   CamberModelData m_CacheConfig_InitialTempPrestressDeflectionModels;
   CamberModelData m_CacheConfig_ReleaseTempPrestressDeflectionModels;

   void ValidateCamberModels(const CSegmentKey& segmentKey);
   void ValidateCamberModels(const GDRCONFIG* pConfig);

   CamberModelData BuildCamberModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig=nullptr);
   void BuildTempCamberModel(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData);
   void InvalidateCamberModels();
   CamberModelData GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models);

   GDRCONFIG m_SlabOffsetDesignCacheConfig;
   CamberModelData m_CacheConfig_SlabOffsetDesignModel;

   void InValidateSlabOffsetDesignModel();
   void ValidateSlabOffsetDesignModel(const GDRCONFIG* pConfig);
   void BuildSlabOffsetDesignModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,CamberModelData* pModelData);
   Float64 GetDesignMomentAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig);
   void GetDesignDeflectionAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);

   std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX);

   void Invalidate(bool clearStatus=true);

   // adds points of interest to a camber model
   PoiIDPairType AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi);

#pragma Reminder("UPDATE - need to elminate duplicate methods") // instead of bUseConfig... look at pConfig == nullptr

   void GetCreepDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );


   void GetD_Deck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_Deck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);

   void GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   void GetPrestressDeflectionFromModel(const pgsPointOfInterest& poi, CamberModelData& modelData, LoadCaseIDType lcid, pgsTypes::PrestressDeflectionDatum datum, Float64* pDy, Float64* pRz);

   //void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz);

   void GetInitialCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,Float64* pDy,Float64* pRz);

   void GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetScreedCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);

   void GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   //void GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);

   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   //void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   //void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);

   //void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc);
   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc);
   //void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz);
   void GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz);
   //void GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetShearKeyDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetConstructionLoadDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   //void GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   
   void GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   void GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz);
   void GetDesignSlabPadStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot);

   Float64 GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey, LoadingEvent le);
   Float64 GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config, LoadingEvent le);
   LoadingEvent GetLoadingEvent(CreepPeriod creepPeriod);

   Float64 GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,IntervalIndexType intervalIdx);

   void GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);
   void GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   void GetTimeStepStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);
   void GetElasticStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   void GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax);

   std::vector<Float64> GetTimeStepPrestressAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   std::vector<Float64> GetTimeStepPrestressMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);

   void ApplyElevationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2);
   void ApplyRotationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2);

   void ComputeTimeDependentEffects(const CGirderKey& girderKey,IntervalIndexType intervalIdx);

   void IsDeckInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,bool* pbTopPTZ,bool* pbBotPTZ);
   void IsGirderInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ);

   void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr,Float64* pftop,Float64* pfbot);

   std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> GetSegmentPreviousSupportLocations(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);
};

#endif //__ANALYSISAGENT_H_
