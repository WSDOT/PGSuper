///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
   virtual LPCTSTR GetProductLoadName(pgsTypes::ProductForceType pfType);
   virtual LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo);
   virtual LPCTSTR GetLimitStateName(pgsTypes::LimitState limitState);
   virtual bool ReportAxialResults();
   virtual void GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad);
   virtual std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true);
   virtual Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey);
   virtual Float64 GetSidewalkLoad(const CSegmentKey& segmentKey);
   virtual bool HasPedestrianLoad();
   virtual bool HasPedestrianLoad(const CGirderKey& girderKey);
   virtual bool HasSidewalkLoad(const CGirderKey& girderKey);
   virtual Float64 GetPedestrianLoad(const CSegmentKey& segmentKey);
   virtual Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation);
   virtual void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,
                                              Float64* pFraExtLeft, Float64* pFraIntLeft,
                                              Float64* pFraExtRight,Float64* pFraIntRight);
   virtual void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad,
                                        Float64* pFraLeft,Float64* pFraRight);

   virtual void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads);
   virtual bool HasConstructionLoad(const CGirderKey& girderKey);
   virtual void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads);
   virtual void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads);

   virtual void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 Fillet, std::vector<SlabLoad>* pSlabLoads);

   virtual void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);
   virtual void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);
   virtual void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads);
   virtual void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads);
   virtual void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead);

   virtual bool HasShearKeyLoad(const CGirderKey& girderKey); // checks for load in adjacent continuous beams as well as current beam
   virtual void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads);

   virtual std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx);
   virtual pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx);
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType);
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx);

// IProductForces
public:
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization);
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization);
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);

   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi);
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config);

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL);
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL);
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL);
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig);
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig);
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=NULL,VehicleIndexType* pTopMaxConfig=NULL,VehicleIndexType* pBotMinConfig=NULL,VehicleIndexType* pBotMaxConfig=NULL);

   virtual std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& segmentKey);
   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = NULL,
                                          AxleConfiguration* pMinRightAxleConfig = NULL,
                                          AxleConfiguration* pMaxLeftAxleConfig = NULL,
                                          AxleConfiguration* pMaxRightAxleConfig = NULL);
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=NULL,AxleConfiguration* pMaxAxleConfigTop=NULL,AxleConfiguration* pMinAxleConfigBot=NULL,AxleConfiguration* pMaxAxleConfigBot=NULL);

   virtual void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax);

   virtual Float64 GetDesignSlabMomentAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi);
   virtual Float64 GetDesignSlabDeflectionAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi);
   virtual void GetDesignSlabStressAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot);

   virtual Float64 GetDesignSlabPadMomentAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi);
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi);

   virtual void DumpAnalysisModels(GirderIndexType gdrIdx);

   virtual void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot);

// IProductForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL);
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL);
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL);
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL);
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=NULL,std::vector<VehicleIndexType>* pTopMaxIndex=NULL,std::vector<VehicleIndexType>* pBotMinIndex=NULL,std::vector<VehicleIndexType>* pBotMaxIndex=NULL);

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = NULL);
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigTop=NULL,std::vector<AxleConfiguration>* pMinAxleConfigBot=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigBot=NULL);

// ICombinedForces
public:
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax);
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax);
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax);
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax);
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax);
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax);

// ICombinedForces2
public:
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax);
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax);
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax);
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax);
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax);

// ILimitStateForces
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,Float64* pMin,Float64* pMax);
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,Float64* pMin,Float64* pMax);
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax);
   virtual void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax);
   virtual void GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   virtual void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax);
   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat);
   virtual bool IsStrengthIIApplicable(const CGirderKey& girderKey);

// ILimitStateForces2
public:
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax);
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat);

// IExternalLoading
public:
   virtual bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName);
   virtual bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo);
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType);
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment);
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment);
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright);
   virtual Float64 GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   virtual std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);

// IPretensionStresses
public:
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad);
   virtual void GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad,Float64* pfTop,Float64* pfBot);
   virtual std::vector<Float64> GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad);
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e);
   virtual Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc);
   virtual Float64 GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config,bool bIncludeLiveLoad);

// ICamber
public:
   virtual Uint32 GetCreepMethod();
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, const GDRCONFIG& config,CreepPeriod creepPeriod, Int16 constructionRate);
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum);
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi);
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config);
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetSidlDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi);
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,Int16 time);
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time);
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time);
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time);
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time);
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time);

   virtual Float64 GetLowerBoundCamberVariabilityFactor()const;

   virtual CamberMultipliers GetCamberMultipliers(const CSegmentKey& segmentKey);

// IContraflexurePoints
public:
   virtual void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints);

// IBearingDesign
public:
   virtual std::vector<PierIndexType> GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey);

   virtual Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);

   virtual void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx = NULL,VehicleIndexType* pMaxVehIdx = NULL);

   virtual void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx = NULL,VehicleIndexType* pMaxVehIdx = NULL);

   virtual Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);

   virtual void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                           Float64* pRmin,Float64* pRmax);

   virtual void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                             pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                             Float64* pRmin,Float64* pRmax);

// IContinuity
public:
   virtual bool IsContinuityFullyEffective(const CGirderKey& girderKey);
   virtual Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey);

// IPrecompressedTensileZone
public:
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ);
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation, const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ);
   virtual bool IsDeckPrecompressed(const CGirderKey& girderKey);

// IReactions
public:
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright);
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright);
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax);
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax);
   virtual Float64 GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   virtual std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   virtual Float64 GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   virtual std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType);
   virtual void GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,Float64* pRmin,Float64* pRmax);
   virtual void GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinVehIdx = NULL,VehicleIndexType* pMaxVehIdx = NULL);
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx = NULL,std::vector<VehicleIndexType>* pMaxVehIdx = NULL);
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = NULL,VehicleIndexType* pMaxVehIdx = NULL);
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx = NULL,std::vector<VehicleIndexType>* pMaxVehIdx = NULL);
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax);
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged(CBridgeChangedHint* pHint);
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName);
   virtual HRESULT OnConstructionLoadChanged();

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// IRatingSpecificationEventSink
public:
   virtual HRESULT OnRatingSpecificationChanged();

// ILoadModifersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged();

// ILossParametersEventSink
public:
   virtual HRESULT OnLossParametersChanged();

private:
   DECLARE_EAF_AGENT_DATA;

   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecCookie;
   DWORD m_dwRatingSpecCookie;
   DWORD m_dwLoadModifierCookie;
   DWORD m_dwLossParametersCookie;

   std::auto_ptr<CSegmentModelManager> m_pSegmentModelManager;
   std::auto_ptr<CGirderModelManager>  m_pGirderModelManager;
   static UINT DeleteSegmentModelManager(LPVOID pParam);
   static UINT DeleteGirderModelManager(LPVOID pParam);

   // Camber models
   std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS> m_CreepCoefficientDetails[2][6]; // key is span/girder hash, index to array is [Construction Rate][CreepPeriod]

   // camber models
   struct CamberModelData
   {
      CComPtr<IFem2dModel> Model;
      pgsPoiMap PoiMap;
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
   void ValidateCamberModels(const GDRCONFIG& config);
   void BuildCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData);
   void BuildTempCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData);
   void InvalidateCamberModels();
   CamberModelData GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models);

   GDRCONFIG m_SlabOffsetDesignCacheConfig;
   CamberModelData m_CacheConfig_SlabOffsetDesignModel;

   void InValidateSlabOffsetDesignModel();
   void ValidateSlabOffsetDesignModel(const GDRCONFIG& config);
   void BuildSlabOffsetDesignModel(const CSegmentKey& segmentKey,const GDRCONFIG& config,CamberModelData* pModelData);
   Float64 GetDesignMomentAdjustment(LoadCaseIDType lcid, const GDRCONFIG& config,const pgsPointOfInterest& poi);
   void GetDesignDeflectionAdjustment(LoadCaseIDType lcid, const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);

   std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true);

   void Invalidate(bool clearStatus=true);

   // adds points of interest to a camber model
   PoiIDType AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi);

   void GetCreepDeflection(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz );


   void GetD_Deck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_Deck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);

   void GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz);
   void GetInitialCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,Float64* pDy,Float64* pRz);
   void GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);

   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc);
   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc);
   void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz);
   void GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz);
   void GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz);
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetShearKeyDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetConstructionLoadDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetDesignSlabDeflectionAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDesignSlabPadDeflectionAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDesignSlabPadStressAdjustment(const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot);

   Float64 GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey,LoadingEvent le);
   Float64 GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config,LoadingEvent le);
   LoadingEvent GetLoadingEvent(CreepPeriod creepPeriod);

   Float64 GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx);

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
};

#endif //__ANALYSISAGENT_H_
