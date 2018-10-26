///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\PoiMap.h>

#include <EAF\EAFInterfaceCache.h>

#include <boost\shared_ptr.hpp>

#include <WBFLFem2d.h>

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
   public IPrestressStresses,
   public ICamber,
   public IContraflexurePoints,
   public IContinuity,
   public IBridgeDescriptionEventSink,
   public ISpecificationEventSink,
   public ILoadModifiersEventSink
{
public:
   CAnalysisAgentImp()
   {
      m_Level = 0;
      m_NextPoi = 0;
      m_DefaultAnalysisType = pgsTypes::Simple;
   }

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
   COM_INTERFACE_ENTRY(IPrestressStresses)
   COM_INTERFACE_ENTRY(ICamber)
   COM_INTERFACE_ENTRY(IContraflexurePoints)
   COM_INTERFACE_ENTRY(IContinuity)
   COM_INTERFACE_ENTRY(IBridgeDescriptionEventSink)
   COM_INTERFACE_ENTRY(ISpecificationEventSink)
   COM_INTERFACE_ENTRY(ILoadModifiersEventSink)
   COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CAnalysisAgentImp)
END_CONNECTION_POINT_MAP()

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidInformationalError;
   StatusCallbackIDType m_scidVSRatio;


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
   virtual pgsTypes::Stage GetGirderDeadLoadStage(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual pgsTypes::Stage GetGirderDeadLoadStage(GirderIndexType gdrLineIdx);
   virtual void GetGirderSelfWeightLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad);
   virtual Float64 GetTrafficBarrierLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetSidewalkLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual bool HasPedestrianLoad();
   virtual bool HasPedestrianLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual bool HasSidewalkLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetPedestrianLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);

   virtual void GetOverlayLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<OverlayLoad>* pOverlayLoads);

   virtual void GetCantileverSlabLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);
   virtual void GetIntermediateDiaphragmLoads(pgsTypes::Stage stage, SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<DiaphragmLoad>* pLoads);
   virtual void GetEndDiaphragmLoads(SpanIndexType spanIdx,GirderIndexType gdrIdx, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2);

   virtual bool HasShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx); // checks for load in adjacent continuous beams as well as current beam
   virtual void GetShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<ShearKeyLoad>* pLoads);

   virtual std::string GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex);
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType);
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex);

private:
   virtual void GetMainSpanSlabLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<SlabLoad>* pSlabLoads);
   virtual void GetMainSpanOverlayLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<OverlayLoad>* pOverlayLoads);
   virtual void GetMainSpanShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<ShearKeyLoad>* pLoads);

// IProductForces
public:
   virtual sysSectionValue GetShear(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat);
   virtual Float64 GetMoment(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat);
   virtual Float64 GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat);
   virtual Float64 GetRotation(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat);
   virtual Float64 GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat);
   virtual void GetStress(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot);

   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi);
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config);

   virtual void GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,long* pMminTruck = NULL,long* pMmaxTruck = NULL);
   virtual void GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,long* pMminTruck = NULL,long* pMmaxTruck = NULL);
   virtual void GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,long* pMinConfig = NULL,long* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,long* pMinConfig = NULL,long* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,long* pMinConfig,long* pMaxConfig);
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,long* pMinConfig,long* pMaxConfig);
   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,long* pMinConfig = NULL,long* pMaxConfig = NULL);
   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,long* pMinConfig = NULL,long* pMaxConfig = NULL);
   virtual void GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,long* pTopMinConfig=NULL,long* pTopMaxConfig=NULL,long* pBotMinConfig=NULL,long* pBotMaxConfig=NULL);

   virtual std::vector<std::string> GetVehicleNames(pgsTypes::LiveLoadType llType,GirderIndexType gdr);
   virtual void GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = NULL,
                                          AxleConfiguration* pMinRightAxleConfig = NULL,
                                          AxleConfiguration* pMaxLeftAxleConfig = NULL,
                                          AxleConfiguration* pMaxRightAxleConfig = NULL);
   virtual void GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadReaction(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=NULL,AxleConfiguration* pMaxAxleConfigTop=NULL,AxleConfiguration* pMinAxleConfigBot=NULL,AxleConfiguration* pMaxAxleConfigBot=NULL);

   virtual void GetDeflLiveLoadDisplacement(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,Float64* pDmin,Float64* pDmax);

   virtual Float64 GetDesignSlabPadMomentAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi);
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi);
   virtual void GetDesignSlabPadStressAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot);

   virtual void DumpAnalysisModels(GirderIndexType girderLineIdx);

   // IProductForces2
public:
   virtual std::vector<sysSectionValue> GetShear(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat);
   virtual std::vector<Float64> GetMoment(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat);
   virtual std::vector<Float64> GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat);
   virtual std::vector<Float64> GetRotation(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat);
   virtual void GetStress(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   virtual void GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<long>* pMminTruck = NULL,std::vector<long>* pMmaxTruck = NULL);
   virtual void GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<long>* pMminTruck = NULL,std::vector<long>* pMmaxTruck = NULL);
   virtual void GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<long>* pMinConfig = NULL,std::vector<long>* pMaxConfig = NULL);
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<long>* pMinConfig = NULL,std::vector<long>* pMaxConfig = NULL);
   virtual void GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<long>* pTopMinIndex=NULL,std::vector<long>* pTopMaxIndex=NULL,std::vector<long>* pBotMinIndex=NULL,std::vector<long>* pBotMaxIndex=NULL);

   virtual void GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = NULL);
   virtual void GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL);
   virtual void GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigTop=NULL,std::vector<AxleConfiguration>* pMinAxleConfigBot=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigBot=NULL);

// ICombinedForces
public:
   virtual sysSectionValue GetShear(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat);
   virtual Float64 GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat);
   virtual Float64 GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat);
   virtual Float64 GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat);
   virtual void GetStress(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot);

   virtual void GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax);
   virtual void GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pVmin,sysSectionValue* pVmax);
   virtual void GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax);
   virtual void GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax);
   virtual void GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax);

// ICombinedForces2
public:
   virtual std::vector<sysSectionValue> GetShear(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat);
   virtual std::vector<Float64> GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat);
   virtual std::vector<Float64> GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat);
   virtual void GetStress(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   virtual void GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax);
   virtual void GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax);
   virtual void GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax);
   virtual void GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax);

// ILimitStateForces
public:
   virtual void GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   virtual void GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax);
   virtual void GetDesignStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,BridgeAnalysisType bat,Float64* pMin,Float64* pMax);
   virtual void GetConcurrentShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax);
   virtual void GetViMmax(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,double* pVi,double* pMmax);
   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState ls,const pgsPointOfInterest& poi,BridgeAnalysisType bat);

// ILimitStateForces2
public:
   virtual void GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax);
   virtual void GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual void GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat);

// IPrestressStresses
public:
   virtual Float64 GetStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc);
   virtual Float64 GetStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e);
   virtual Float64 GetStressPerStrand(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc);
   virtual Float64 GetDesignStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config);
   virtual std::vector<Float64> GetStress(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc);

// ICamber
public:
   virtual Uint32 GetCreepMethod();
   virtual Float64 GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr, const GDRCONFIG& config,CreepPeriod creepPeriod, Int16 constructionRate);
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings);
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings);
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings);
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings);
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate);
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi);
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config);
   virtual Float64 GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi);
   virtual Float64 GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi, const GDRCONFIG& config);
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

   virtual void GetHarpedStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMl,Float64* pMr,Float64* pNl,Float64* pNr,Float64* pXl,Float64* pXr);
   virtual void GetTempStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMxferL,Float64* pMxferR,Float64* pMremoveL,Float64* pMremoveR);
   virtual void GetStraightStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,std::vector< std::pair<Float64,Float64> >* loads);

// IContraflexurePoints
public:
   virtual void GetContraflexurePoints(SpanIndexType span,GirderIndexType gdr,double* cfPoints,Uint32* nPoints);

// IContinuity
public:
   virtual bool IsContinuityFullyEffective(GirderIndexType girderline);
   virtual double GetContinuityStressLevel(PierIndexType pier,GirderIndexType gdr);

// IBridgeDescriptionEventSink
public:
   virtual HRESULT OnBridgeChanged();
   virtual HRESULT OnGirderFamilyChanged();
   virtual HRESULT OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint);
   virtual HRESULT OnLiveLoadChanged();
   virtual HRESULT OnLiveLoadNameChanged(const char* strOldName,const char* strNewName);

// ISpecificationEventSink
public:
   virtual HRESULT OnSpecificationChanged();
   virtual HRESULT OnAnalysisTypeChanged();

// ILoadModifersEventSink
public:
   virtual HRESULT OnLoadModifiersChanged();

public:
   // this methods need to be public so the PGSuperLoadCombinationResponse object use them
   void GetVehicularLoad(ILBAMModel* pModel,LiveLoadModelType llType,VehicleIndexType vehicleIndex,IVehicularLoad** pVehicle);
   void CreateAxleConfig(ILBAMModel* pModel,ILiveLoadConfiguration* pConfig,AxleConfiguration* pAxles);
   pgsTypes::LimitState GetLimitStateFromLoadCombination(CComBSTR bstrLoadCombination);

private:
   DECLARE_AGENT_DATA;
   Uint16 m_Level;
   DWORD m_dwBridgeDescCookie;
   DWORD m_dwSpecCookie;
   DWORD m_dwLoadModifierCookie;
   CComPtr<ILongArray> m_LBAMPoi;   // array for LBAM poi
   CComPtr<ILBAMLRFDFactory3> m_LBAMUtility;
   CComPtr<IUnitServer> m_UnitServer;

   PoiIDType m_NextPoi;

   pgsTypes::AnalysisType m_DefaultAnalysisType;

   // casting yard models
   struct cyModelData
   {
      pgsTypes::Stage Stage;
      CComPtr<IFem2dModel> Model;
      pgsPoiMap PoiMap;
      cyModelData& operator=(const cyModelData& other)
      {
         Stage = other.Stage;
         Model = other.Model;
         PoiMap = other.PoiMap;
         return *this;
      };
   };
   typedef std::map<SpanGirderHashType,cyModelData> cyGirderModels;
   cyGirderModels m_CastingYardModels;

   // lbam model of structure at bridge site
   struct ModelData
   {
      CAnalysisAgentImp* m_pParent;

      // Model and response engines for the bridge site force analysis
      CComPtr<ILBAMModel>               m_Model;
      CComPtr<ILBAMModel>               m_ContinuousModel;
      CComPtr<ILBAMModelEnveloper>      m_MinModelEnveloper;
      CComPtr<ILBAMModelEnveloper>      m_MaxModelEnveloper;

      // Index is SimpleSpan or ContinuousSpan
      CComPtr<ILoadGroupResponse>       pLoadGroupResponse[2]; // Girder, Traffic Barrier, etc
      CComPtr<ILoadCaseResponse>        pLoadCaseResponse[2];  // DC, DW, etc
      CComPtr<IContraflexureResponse>   pContraflexureResponse[2]; 

      // first index is force effect: fetFx, fetFy, fetMz
      // second index is optimization: optMinimize, optMaximize
      CComPtr<ILoadGroupResponse>       pMinLoadGroupResponseEnvelope[3][2]; // Girder, Traffic Barrier, etc
      CComPtr<ILoadGroupResponse>       pMaxLoadGroupResponseEnvelope[3][2]; // Girder, Traffic Barrier, etc
      CComPtr<ILoadCaseResponse>        pMinLoadCaseResponseEnvelope[3][2];  // DC, DW, etc
      CComPtr<ILoadCaseResponse>        pMaxLoadCaseResponseEnvelope[3][2];  // DC, DW, etc

      // Index is SimpleSpan, ContinuousSpan, MinSimpleContinousEnvelope, MaxSimpleContinuousEnvelope
      CComPtr<ILiveLoadModelResponse>   pLiveLoadResponse[4];  // LL+IM
      CComPtr<ILoadCombinationResponse> pLoadComboResponse[4]; // Service I, Strength I, etc

      // Index is SimpleSpan or ContinuousSpan
      CComPtr<IConcurrentLoadCombinationResponse> pConcurrentComboResponse[2];
      
      // Index is SimpleSpan, ContinuousSpan, MinSimpleContinousEnvelope, MaxSimpleContinuousEnvelope
      CComPtr<IEnvelopedVehicularResponse> pVehicularResponse[4];

      // response engines for the deflection analysis
      CComPtr<ILoadGroupResponse>          pDeflLoadGroupResponse;
      CComPtr<ILiveLoadModelResponse>      pDeflLiveLoadResponse;
      CComPtr<IEnvelopedVehicularResponse> pDeflEnvelopedVehicularResponse;

      pgsPoiMap PoiMap;

      ModelData(CAnalysisAgentImp *pParent);
      ModelData(const ModelData& other);
      ~ModelData();
      void CreateAnalysisEngine(ILBAMModel* theModel,BridgeAnalysisType bat,ILBAMAnalysisEngine** ppEngine);
      void AddContinuousModel(ILBAMModel* pContModel);
      void operator=(const ModelData& other);
   };

   typedef std::map<GirderIndexType,ModelData> Models; // Key is girder line index
   Models m_BridgeSiteModels;

   std::map<SpanGirderHashType,Float64> m_TrafficBarrierLoad;
   std::map<SpanGirderHashType,Float64> m_SidewalkLoad;

   // Camber models
   std::map<SpanGirderHashType,CREEPCOEFFICIENTDETAILS> m_CreepCoefficientDetails[2][6]; // key is span/girder hash, index to array is [Construction Rate][CreepPeriod]

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
   typedef std::map<SpanGirderHashType,CamberModelData> CamberModels;
   CamberModels m_PrestressDeflectionModels;
   CamberModels m_InitialTempPrestressDeflectionModels;
   CamberModels m_ReleaseTempPrestressDeflectionModels;

   void ValidateCamberModels(SpanIndexType span,GirderIndexType gdr);
   void BuildCamberModel(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData);
   void BuildTempCamberModel(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData);
   void InvalidateCamberModels();
   CamberModelData GetPrestressDeflectionModel(SpanIndexType span,GirderIndexType gdr,CamberModels& models);


   void ValidateAnalysisModels(SpanIndexType span,GirderIndexType gdr);
   void Invalidate(bool clearStatus=true);
   void DoCastingYardAnalysis(SpanIndexType span,GirderIndexType gdr,IProgress* pProgress);

   ModelData* GetModelData(GirderIndexType gdr);
   void BuildBridgeSiteModel(GirderIndexType gdr);
   Int32 AddPointOfInterest(ModelData* pModelData,const pgsPointOfInterest& poi);
   virtual void AddPoiStressPoints(const pgsPointOfInterest& poi,IStage* pStage,IPOIStressPoints* pPOIStressPoints);

   cyModelData BuildCastingYardModels(SpanIndexType span,GirderIndexType gdr,IProgress* pProgress);
   PoiIDType AddCyPointOfInterest(const pgsPointOfInterest& poi);

   long GetStressPointIndex(pgsTypes::StressLocation loc);
   CComBSTR GetLoadCaseName(LoadingCombination combo);
   CComBSTR GetStageName(ProductForceType type);
   CComBSTR GetLoadCombinationName(pgsTypes::LimitState ls);
   CComBSTR GetLoadCombinationName(pgsTypes::LiveLoadType llt); // load combo name for LBAM
   CComBSTR GetLiveLoadName(pgsTypes::LiveLoadType llt); // friendly live load name


   void BuildBridgeSiteModel(GirderIndexType gdr,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel);
   void ApplySelfWeightLoad(ILBAMModel* pModel, GirderIndexType gdr);
   void ApplyDiaphragmLoad(ILBAMModel* pModel, GirderIndexType gdr);
   void ApplySlabLoad(ILBAMModel* pModel, GirderIndexType gdr );
   void ApplyOverlayLoad(ILBAMModel* pModel, GirderIndexType gdr);
   void ApplyShearKeyLoad(ILBAMModel* pModel, GirderIndexType gdr);
   void GetRailingSystemLoadFraction(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64* pFraLeft,Float64* pFraRight);
   void ApplyTrafficBarrierAndSidewalkLoad(ILBAMModel* pModel,GirderIndexType gdr);
   void ApplyLiveLoadModel(ILBAMModel* pModel,GirderIndexType gdr);
   void AddUserLiveLoads(ILBAMModel* pModel,GirderIndexType gdr,pgsTypes::LiveLoadType llType,std::vector<std::string>& libraryLoads,ILibrary* pLibrary, ILiveLoads* pLiveLoads,IVehicularLoads* pVehicles,IVehicularLoads* pPedVehicles);
   void ApplyUserDefinedLoads(ILBAMModel* pModel,GirderIndexType gdr);
   void ApplyLiveLoadDistributionFactors(GirderIndexType gdr,bool bContinuous,IContraflexureResponse* pContraflexureResponse,ILBAMModel* pModel);
   void ConfigureLoadCombinations(ILBAMModel* pModel);
   void ApplyEndDiaphragmLoads(ILBAMModel* pModel,CComBSTR& bstrStage,CComBSTR& bstrLoadGroup, SpanIndexType span,GirderIndexType gdr );
   void ApplyIntermediateDiaphragmLoads(ILBAMModel* pModel, SpanIndexType span,GirderIndexType gdr );

   Float64 GetPedestrianLiveLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void AddHL93LiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,double IMtruck,double IMlane);
   void AddLegalLiveLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,double IMtruck,double IMlane);
   void AddNotionalRatingLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,double IMtruck,double IMlane);
   void AddSHVLoad(ILBAMModel* pModel,ILibrary* pLibrary,pgsTypes::LiveLoadType llType,double IMtruck,double IMlane);
   void AddPedestrianLoad(const std::string& strLLName,double wPedLL,IVehicularLoads* pVehicles);
   void AddUserTruck(const std::string& strLLName,ILibrary* pLibrary,double IMtruck,double IMlane,IVehicularLoads* pVehicles);
   void AddDummyLiveLoad(IVehicularLoads* pVehicles);

   void GetSectionResults(cyGirderModels& models,const pgsPointOfInterest& poi,Float64* pFx,Float64* pFy,Float64* pMz,Float64* pDx,Float64* pDy,Float64* pRz);
   Float64 GetSectionStress(cyGirderModels& models,pgsTypes::StressLocation loc,const pgsPointOfInterest& poi);
   Float64 GetReactions(cyGirderModels& model,PierIndexType pier,GirderIndexType gdr);

   void GetLiveLoadSectionResults(const pgsPointOfInterest& poi,bamSectionResults* pMinResults,bamSectionResults* pMaxResults);
   void GetLiveLoadReactions(PierIndexType pier,GirderIndexType gdr,bamReaction* pMin,bamReaction* pMax);
   void GetDeflLiveLoadSectionResults(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,bamSectionResults* pMinResults,bamSectionResults* pMaxResults);
   cyModelData* GetModelData(cyGirderModels& models,SpanIndexType span,GirderIndexType gdr);

   void GetCreepDeflection(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );
   void GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz );

   void GetD_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetD_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz);

   void GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,bool bRelativeToBearings,Float64* pDy,Float64* pRz);
   void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz);
   void GetInitialCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,Float64* pDy,Float64* pRz);
   void GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);
   void GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& pInitTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz);

   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz);
   void GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);
   void GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz);
   void GetDesignSlabPadDeflectionAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz);


   Float64 GetConcreteStrengthAtTimeOfLoading(SpanIndexType span,GirderIndexType gdr,LoadingEvent le);
   Float64 GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config,LoadingEvent le);
   LoadingEvent GetLoadingEvent(CreepPeriod creepPeriod);

   enum SpanType { PinPin, PinFix, FixPin, FixFix };
   SpanType GetSpanType(SpanIndexType span,bool bContinuous);
   void AddDistributionFactors(IDistributionFactors* factors,double length,double gpM,double gnM,double gV,double gR,double gF);
   Uint32 GetCfPointsInRange(IDblArray* cfLocs, double spanStart, double spanEnd, double* ptsInrg);
   void ApplyLLDF_PinPin(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_PinFix(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_FixPin(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_FixFix(SpanIndexType spanIdx,GirderIndexType gdrIdx,IDblArray* cf_locs,IDistributionFactors* distFactors);
   void ApplyLLDF_Support(PierIndexType pierIdx,GirderIndexType gdrIdx,ISupports* supports);

   void GetEngine(ModelData* pModelData,bool bContinuous,ILBAMAnalysisEngine** pEngine);

   rkPPPartUniformLoad GetDesignSlabPadModel(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi);
   Float64 GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::Stage stage);
   BridgeAnalysisType GetBridgeAnalysisType();

   ModelData* UpdateLBAMPois(const std::vector<pgsPointOfInterest>& vPoi);
   void RenameLiveLoad(ILBAMModel* pModel,pgsTypes::LiveLoadType llType,const char* strOldName,const char* strNewName);
   void GetModel(ModelData* pModelData,BridgeAnalysisType bat,ILBAMModel** ppModel);

   void GetLiveLoadModel(pgsTypes::LiveLoadType llType,GirderIndexType gdrIdx,ILiveLoadModel** ppLiveLoadModel);

   DistributionFactorType GetLiveLoadDistributionFactorType(pgsTypes::LiveLoadType llType);
};

#endif //__ANALYSISAGENT_H_
