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

// SYSTEM INCLUDES
//
#include <vector>
#include <PGSuperTypes.h>
#include "Details.h"

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class sysSectionValue;
class pgsPointOfInterest;

// MISCELLANEOUS
//

typedef enum ProductForceType 
{ 
   // externaly applied loads
   pftGirder,
   pftConstruction,
   pftSlab, 
   pftSlabPad, 
   pftSlabPanel, 
   pftDiaphragm, 
   pftOverlay,
   pftSidewalk,
   pftTrafficBarrier, 
   pftUserDC, 
   pftUserDW, 
   pftUserLLIM,
   pftShearKey,

   // pseudo externally applied loads (really internal loads)
   pftPretension,       // P*e for pretension
   pftPostTensioning,   // P*e for post-tension
   pftSecondaryEffects,

   // time-depending effects
   pftCreep,
   pftShrinkage,
   pftRelaxation,

   // special cases
   pftOverlayRating,

   pftProductForceTypeCount
} ProductForceType;

typedef enum LoadingCombinationType
{ 
   lcDC, 
   lcDW,        // total DW
   lcDWRating,  // DW for loading rating
   lcDWp,       // DW for permanent loads only
   lcDWf,       // DW for future loads only
   lcCR,
   lcSH,
   lcRE,
   lcPS,
   lcLoadingCombinationTypeCount
} LoadingCombinationType;

typedef enum ResultsType 
{ 
   rtCumulative, 
   rtIncremental 
} ResultsType;

struct AxlePlacement
{
   Float64 Weight;
   Float64 Location;
};

typedef std::vector<AxlePlacement> AxleConfiguration;

typedef struct LinearLoad
{
   Float64 StartLoc; // measured from left end of girder
   Float64 EndLoc;   // measured from left end of girder
   Float64 wStart;
   Float64 wEnd;
} LinearLoad;

typedef struct DiaphragmLoad
{
   Float64 Loc;  // measured from left end of segment if precast or left end of span if cast-in-place
   Float64 Load;
   bool operator<(const DiaphragmLoad& other) const { return Loc < other.Loc; }
} DiaphragmLoad;

struct OverlayLoad : public LinearLoad
{
   Float64 StartWcc; // curb to curb width
   Float64 EndWcc; // curb to curb width
};

typedef OverlayLoad ConstructionLoad;
typedef LinearLoad GirderLoad;

typedef struct SlabLoad
{
   Float64 Loc;          // measured from left girder end. Location where this load is defined
   Float64 MainSlabLoad; // if used with SIP, only the cast portion of the slab
   Float64 PanelLoad;    // Weight of SIP deck panels
   Float64 PadLoad;
} SlabLoad;

typedef struct ShearKeyLoad
{
   Float64 StartLoc;       // Measured from left end of girder
   Float64 EndLoc; 
   Float64 UniformLoad;    // Portion of load independent of joint
   Float64 StartJW;        // Joint width
   Float64 StartJointLoad;
   Float64 EndJW;          // Curb to curb width
   Float64 EndJointLoad;
} ShearKeyLoad;

typedef struct EquivPretensionLoad
{
   Float64 Xs; // location from start of segment
   Float64 P;  // magnitude of equivalent vertical load
   Float64 M;  // magnitude of equivalent moment
} EquivPretensionLoad;

/*****************************************************************************
INTERFACE
   IProductLoads

   Interface to product load definitions

DESCRIPTION
   Product loads are things like girder self-weight, traffic barrier loads,
   etc.  These are the loads in the product domain. Live loads are per lane and 
   are the envelope of the individual vehicular loads. Vehicular loads are
   per girder and represent the envelope of each individual vehicular load.
*****************************************************************************/
// {033FA7EE-DAEF-42e0-8F09-0197C7071810}
DEFINE_GUID(IID_IProductLoads, 
0x33fa7ee, 0xdaef, 0x42e0, 0x8f, 0x9, 0x1, 0x97, 0xc7, 0x7, 0x18, 0x10);
interface IProductLoads : IUnknown
{
   virtual LPCTSTR GetProductLoadName(ProductForceType pfType) = 0;
   virtual LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo) = 0;
   virtual LPCTSTR GetLimitStateName(pgsTypes::LimitState limitState) = 0;

   // product loads applied to girder
   virtual void GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad) = 0;
   virtual Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) = 0;
   virtual bool HasPedestrianLoad() = 0;
   virtual bool HasPedestrianLoad(const CGirderKey& girderKey) = 0;
   virtual Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) = 0;
   virtual Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) = 0;
   virtual bool HasSidewalkLoad(const CGirderKey& girderKey) = 0;
   virtual void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,
                                              Float64* pFraExtLeft, Float64* pFraIntLeft,
                                              Float64* pFraExtRight,Float64* pFraIntRight)=0;
   virtual void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad,
                                        Float64* pFraLeft,Float64* pFraRight)=0;


   // overlay loads
   virtual void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads)=0;

   // construction loads
   virtual void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads)=0;

   // slab loads
   virtual void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads)=0;

   // point loads due to portion of slab out on cantilever. 
   // + force is up, + moment is ccw.
   virtual void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)=0;
   
   // point loads due to portion of slab pad out on cantilever. 
   // + force is up, + moment is ccw.
   virtual void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)=0;

   // diaphragm loads
   // + force is up, + moment is ccw.
   virtual void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) = 0;
   virtual void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) = 0;
   virtual void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead) = 0;

   virtual std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey) = 0;

   // Shear Key load (applied from girder library entry)
   virtual bool HasShearKeyLoad(const CGirderKey& girderKey) = 0;
   virtual void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads)=0;

   virtual std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex) = 0;
   virtual pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex) = 0;
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) = 0;
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex) = 0;

   // Equivalent loads for pretensioning
   virtual std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;
};


/*****************************************************************************
INTERFACE
   IProductForces

   Interface to structural analysis results for product loads.

DESCRIPTION
   Interface to structural analysis results for product loads.
   Product loads are things like girder self-weight, traffic barrier loads,
   etc.  These are the loads in the product domain. Live loads are per lane and 
   are the envelope of the individual vehicular loads. Vehicular loads are
   per girder and represent the envelope of each individual vehicular load.
*****************************************************************************/
// {3BACF776-6A93-11d2-883E-006097C68A9C}
DEFINE_GUID(IID_IProductForces, 
0x3bacf776, 0x6a93, 0x11d2, 0x88, 0x3e, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IProductForces : IUnknown
{
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) = 0;
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) = 0;

   virtual Float64 GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) = 0;

   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck = NULL,VehicleIndexType* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) = 0;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=NULL,VehicleIndexType* pTopMaxConfig=NULL,VehicleIndexType* pBotMinConfig=NULL,VehicleIndexType* pBotMaxConfig=NULL) = 0;

   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = NULL,
                                          AxleConfiguration* pMinRightAxleConfig = NULL,
                                          AxleConfiguration* pMaxLeftAxleConfig = NULL,
                                          AxleConfiguration* pMaxRightAxleConfig = NULL) = 0;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=NULL,AxleConfiguration* pMaxAxleConfigTop=NULL,AxleConfiguration* pMinAxleConfigBot=NULL,AxleConfiguration* pMaxAxleConfigBot=NULL) = 0;

   // This is the deflection due to self weight of the girder, when the girder
   // is in the casting yard, supported on its final bearing points. Modulus
   // of elasticity is equal to Eci.  This differs from GetDeflection above
   // in that the span length L is the span length of the girder instead of the
   // girder length usually used in the casting yard.
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Deflection due to LRFD optional design live load (LRFD 3.6.1.3.2)
   enum DeflectionLiveLoadType { DesignTruckAlone, Design25PlusLane, DeflectionLiveLoadEnvelope }; 
   virtual void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) = 0;

   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in deflection between the slab deflection for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab 
   // and the input value.
   virtual void GetDesignSlabStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot) = 0;

   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in deflection between the slab pad deflection for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   virtual void GetDesignSlabPadStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot) = 0;

   virtual void DumpAnalysisModels(GirderIndexType gdrIdx) = 0;

   virtual void GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot) = 0;
};


/*****************************************************************************
INTERFACE
   IProductForces2

   Interface to structural analysis results for product loads.

DESCRIPTION
   Interface to structural analysis results for product loads.
   Product loads are things like girder self-weight, traffic barrier loads,
   etc.  These are the loads in the product domain. Live loads are per lane and 
   are the envelope of the individual vehicular loads. Vehicular loads are
   per girder and represent the envelope of each individual vehicular load.
*****************************************************************************/
// {E51FF04B-B046-43b9-8696-62AB606D2F04}
DEFINE_GUID(IID_IProductForces2, 
0xe51ff04b, 0xb046, 0x43b9, 0x86, 0x96, 0x62, 0xab, 0x60, 0x6d, 0x2f, 0x4);
interface IProductForces2 : IUnknown
{
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) = 0;

   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = NULL,std::vector<VehicleIndexType>* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=NULL,std::vector<VehicleIndexType>* pTopMaxIndex=NULL,std::vector<VehicleIndexType>* pBotMinIndex=NULL,std::vector<VehicleIndexType>* pBotMaxIndex=NULL) = 0;

   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = NULL) = 0;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMinAxleConfigurationBot=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationBot=NULL) = 0;
};

/*****************************************************************************
INTERFACE
   ICombinedForces

   Interface to structural analysis results for load combinations.

DESCRIPTION
   Interface to structural analysis results for load combinations.
   Note that in this case load combinations is an LRFD term meaning
   the combination of product loads.  DC, DW, etc are load combinations.
   Live load results are per girder
*****************************************************************************/
// {F32A1BFE-7727-11d2-8851-006097C68A9C}
DEFINE_GUID(IID_ICombinedForces, 
0xf32a1bfe, 0x7727, 0x11d2, 0x88, 0x51, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface ICombinedForces : IUnknown
{
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) = 0;

   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) = 0;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeShear,sysSectionValue* pVmin,sysSectionValue* pVmax) = 0;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) = 0;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) = 0;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) = 0;
};

/*****************************************************************************
INTERFACE
   ICombinedForces2

   Interface to structural analysis results for load combinations.

DESCRIPTION
   Interface to structural analysis results for load combinations.
   Note that in this case load combinations is an LRFD term meaning
   the combination of product loads.  DC, DW, etc are load combinations.
   Live loads results are per girder
*****************************************************************************/
// {D8619CFF-B2DA-4f37-9D34-A3CDA17600A7}
DEFINE_GUID(IID_ICombinedForces2, 
0xd8619cff, 0xb2da, 0x4f37, 0x9d, 0x34, 0xa3, 0xcd, 0xa1, 0x76, 0x0, 0xa7);
interface ICombinedForces2 : IUnknown
{
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) = 0;

   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) = 0;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact, std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax) = 0;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) = 0;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) = 0;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) = 0;
};

/*****************************************************************************
INTERFACE
   ILimitStateForces

   Interface to structural analysis results for limit states.

DESCRIPTION
   Interface to structural analysis results for limit states.
*****************************************************************************/
// {D458BB7A-797C-11d2-8853-006097C68A9C}
DEFINE_GUID(IID_ILimitStateForces, 
0xd458bb7a, 0x797c, 0x11d2, 0x88, 0x53, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface ILimitStateForces : IUnknown
{
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) = 0;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,Float64* pMin,Float64* pMax) = 0;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,Float64* pMin,Float64* pMax) = 0;
   virtual void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,Float64* pMin,Float64* pMax) = 0;

   virtual void GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;

   virtual void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) = 0;
   virtual void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) = 0;

   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) = 0;

   // Return true if limit state is to be reported
   virtual bool IsStrengthIIApplicable(const CGirderKey& girderKey) = 0;
};


/*****************************************************************************
INTERFACE
   ILimitStateForces2

   Interface to structural analysis results for limit states.

DESCRIPTION
   Interface to structural analysis results for limit states.
*****************************************************************************/
// {DEEDBFFC-E236-4a24-A78A-D507B96573A3}
DEFINE_GUID(IID_ILimitStateForces2, 
0xdeedbffc, 0xe236, 0x4a24, 0xa7, 0x8a, 0xd5, 0x7, 0xb9, 0x65, 0x73, 0xa3);
interface ILimitStateForces2 : IUnknown
{
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax) = 0;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;
   virtual void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;

   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat) = 0;
};

/*****************************************************************************
INTERFACE
   IExternalLoading

DESCRIPTION
   Interface for defining load groups, and loadings for those groups, for
   external clients.

   Load groups can be added to previously defined load combinations.

   This is different from user defined loads in that use defined loads are
   created by the end user of the software through the user interface. These
   loads are created programatically and may or may not be included in the
   load cases and limit state combinations.
*****************************************************************************/
// {88ECE0F2-6265-4467-B888-D830D293C712}
DEFINE_GUID(IID_IExternalLoading, 
0x88ece0f2, 0x6265, 0x4467, 0xb8, 0x88, 0xd8, 0x30, 0xd2, 0x93, 0xc7, 0x12);
interface IExternalLoading : IUnknown
{
   // creates a new loading. this is essentially creating a user defined product load
   // returns true if successful
   virtual bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName) = 0;

   // adds a loading to a load combination. returns true if successful
   virtual bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo) = 0;

   // creates a concentrated load
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) = 0;
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) = 0;

   // creates a uniform load
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) = 0;
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) = 0;

   // creates a constant initial strain load between two POI.
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) = 0;
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) = 0;

   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual sysSectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual Float64 GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) = 0;

   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) = 0;
};

/*****************************************************************************
INTERFACE
   IPretensionStresses

   Interface to get the stresses caused by pretensioned strands.

DESCRIPTION
   Interface to get the stresses caused by pretensioned strands.
*****************************************************************************/
// {FDCC4ED6-7D9B-11d2-8857-006097C68A9C}
DEFINE_GUID(IID_IPretensionStresses, 
0xfdcc4ed6, 0x7d9b, 0x11d2, 0x88, 0x57, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IPretensionStresses : IUnknown
{
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc) = 0;
   virtual std::vector<Float64> GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc) = 0;
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e) = 0;
   virtual Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) = 0;
   virtual Float64 GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config) = 0;
};

/*****************************************************************************
INTERFACE
   ICamber

   Interface to get camber information.

DESCRIPTION
   Interface to get camber information.
*****************************************************************************/
// {6D835B88-87BB-11d2-887F-006097C68A9C}
DEFINE_GUID(IID_ICamber, 
0x6d835b88, 0x87bb, 0x11d2, 0x88, 0x7f, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface ICamber : IUnknown
{
   enum CreepPeriod
   {
      cpReleaseToDiaphragm, // load applied at release and creep evaluated immediately before time of diaphragam placement and temporary strand removal
      cpReleaseToDeck,      // load applied at release and creep evaluated immediately before time of deck placement and application of superimposed dead loads
      cpReleaseToFinal,     // load applied at release and creep evaluated at end of creep period

      cpDiaphragmToDeck,    // load applied immediately after diaphragm placement and creep evaluated immediately before deck placement
      cpDiaphragmToFinal,   // load applied immediately after diaphragm placement and creep evaluated at end of creep period

      cpDeckToFinal         // load applied immediately after deck placement and creep evaluated at end of creep period
   };

   enum LoadingEvent
   { 
      leRelease,    // prestress release
      leDiaphragm,  // diaphragms are cast and temporary strands removed
      leDeck        // deck is cast and superimposed dead loads are applied
   };

   virtual Uint32 GetCreepMethod() = 0;

   // Returns the creep multiplier used to scale the instantious deflection
   // constructionRate indicates if it is rapid or normal construction
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate) = 0;
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate) = 0;

   // Returns details of creep coefficient calculations.
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate) = 0;
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate) = 0;

   // Deflection caused by prestressing alone (Harped and Straight Strands only).
   // Deflections are computed using the concrete properties at release.
   // If bRelativeToBearings is true, the deflection is relative to the bearings
   // otherwise it is relative to the end of the girder
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings) = 0;
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings) = 0;

   // Deflection caused by temporary prestressing alone when stressed at casting.
   // Deflections are computed using the concrete properties at release.
   // If bRelativeToBearings is true, the deflection is relative to the bearings
   // otherwise it is relative to the end of the girder
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings) = 0;
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings) = 0;

   // Returns the amount the girder deflects when the temporary strands are
   // removed.  Deflections are computed using 28day concrete properties.
   // Returns 0 if the strands where not placed in the girder at casting
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Returns the deflection due to creep at the end of a creep period
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate) = 0;
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate) = 0;

   // Returns the amount the girder deflects when the deck is
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Returns the amount the girder deflects when the deck panels are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Returns the amount the girder deflects when the diaphragms are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Returns the amount the girder deflects when user loads are applied.
   // Camber is cummulative through stages.
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config) = 0;

   // Returns the amount the girder deflects slab+barrier+overlay loads are applied.
   // Camber is cummulative through stages.
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Deflection due to slab, diaphragms and user-defined loads.
   // This is the amount the girder will deflect downwards as the slab is placed.
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   virtual Float64 GetSidlDeflection(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // This is the camber that remains in the girder after the slab pour
   // It is equal to the Total camber less the screed camber.
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,Int16 time) = 0;
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time) = 0;

   // This is the rotation in the girder that renames after the slab pour
   // It is equal to the rotation due to total camber, less the rotation for screed camber
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time) = 0;
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time) = 0;

   // This is the camber that remains in the girder after the slab pour
   // It is equal to the Total camber less the slab deflection.
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time) = 0;
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time) = 0; 

   // This is the factor that the min timing camber is multiplied by the compute the lower bound camber
   virtual Float64 GetLowerBoundCamberVariabilityFactor()const = 0;
};


/*****************************************************************************
INTERFACE
   IContraflexurePoints

   Interface to get contraflexure points

DESCRIPTION
   Interface to get contraflexure points due to a uniform load on the structure
*****************************************************************************/
// {A07DC9A0-1DFA-4a7c-B852-ABCAF719736D}
DEFINE_GUID(IID_IContraflexurePoints, 
0xa07dc9a0, 0x1dfa, 0x4a7c, 0xb8, 0x52, 0xab, 0xca, 0xf7, 0x19, 0x73, 0x6d);
interface IContraflexurePoints : IUnknown
{
   virtual void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) = 0;
};


/*****************************************************************************
INTERFACE
   IContinuity

   Interface to get information about continuity in the structure

DESCRIPTION
   Interface to get information about continuity in the structure
*****************************************************************************/
// {02731714-537F-43d2-8024-B7024E8F5274}
DEFINE_GUID(IID_IContinuity, 
0x2731714, 0x537f, 0x43d2, 0x80, 0x24, 0xb7, 0x2, 0x4e, 0x8f, 0x52, 0x74);
interface IContinuity : IUnknown
{
   virtual bool IsContinuityFullyEffective(const CGirderKey& girderKey) = 0;
   virtual Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) = 0;
};

/*****************************************************************************
INTERFACE
   IBearingDesign

   Interface to get information about bearing design results

DESCRIPTION
   Interface to get information about bearing design results
*****************************************************************************/
// {DACEC889-2B86-46e9-8B47-0875BC9B19A5}
DEFINE_GUID(IID_IBearingDesign, 
0xdacec889, 0x2b86, 0x46e9, 0x8b, 0x47, 0x8, 0x75, 0xbc, 0x9b, 0x19, 0xa5);
interface IBearingDesign : IUnknown
{
   // Determine if bearing reactions are available for the girder in question. Function will return true if girder has a
   // simply supported boundary connection. Passed-in bools will be true for simple-supported ends.
   virtual bool AreBearingReactionsAvailable(IntervalIndexType intervalIdx,const CGirderKey& girderKey, bool* pBleft, bool* pBright)=0;

   // From IProductForces
   virtual void GetBearingProductReaction(IntervalIndexType intervalIdx,ProductForceType type,const CGirderKey& girderKey,
                                          pgsTypes::BridgeAnalysisType bat, ResultsType cmbtype, Float64* pLftEnd,Float64* pRgtEnd)=0;

   virtual void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                           Float64* pLeftRmin,Float64* pLeftRmax,Float64* pLeftTmin,Float64* pLeftTmax,
                                           Float64* pRightRmin,Float64* pRightRmax,Float64* pRightTmin,Float64* pRightTmax,
                                           VehicleIndexType* pLeftMinVehIdx = NULL,VehicleIndexType* pLeftMaxVehIdx = NULL,
                                           VehicleIndexType* pRightMinVehIdx = NULL,VehicleIndexType* pRightMaxVehIdx = NULL)=0;

   virtual void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                           Float64* pLeftTmin,Float64* pLeftTmax,Float64* pLeftRmin,Float64* pLeftRmax,
                                           Float64* pRightTmin,Float64* pRightTmax,Float64* pRightRmin,Float64* pRightRmax,
                                           VehicleIndexType* pLeftMinVehIdx = NULL,VehicleIndexType* pLeftMaxVehIdx = NULL,
                                           VehicleIndexType* pRightMinVehIdx = NULL,VehicleIndexType* pRightMaxVehIdx = NULL)=0;
   // From ICombinedForces
   virtual void GetBearingCombinedReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,const CGirderKey& girderKey,
                                           pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pLftEnd,Float64* pRgtEnd)=0;

   virtual void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                   Float64* pLeftRmin, Float64* pLeftRmax, 
                                                   Float64* pRightRmin,Float64* pRightRmax)=0;

   // From ILimitStateForces
   virtual void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const CGirderKey& girderKey,
                                             pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                             Float64* pLeftRmin, Float64* pLeftRmax, 
                                             Float64* pRightRmin,Float64* pRightRmax)=0;
};

/*****************************************************************************
INTERFACE
   IPrecompressedTensileZone

   Interface to get information about the precompressed tensile zone

DESCRIPTION
   Interface to get information about the precompressed tensile zone
*****************************************************************************/
// {1A7DFED9-6DE6-4bd1-8A41-D603CFD53C14}
DEFINE_GUID(IID_IPrecompressedTensileZone, 
0x1a7dfed9, 0x6de6, 0x4bd1, 0x8a, 0x41, 0xd6, 0x3, 0xcf, 0xd5, 0x3c, 0x14);
interface IPrecompressedTensileZone : IUnknown
{
   // Returns true of the specified location is in the precompressed tensile zone
   virtual bool IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation) = 0;

   // Returns true of the specified location is in the precompressed tensile zone
   virtual bool IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation,const GDRCONFIG* pConfig) = 0;

   // Returns true if the deck is "precompressed". The deck is considered precompressed if
   // it experiences direct stresses due to post-tensioning applied after the deck
   // has become composite
   virtual bool IsDeckPrecompressed(const CGirderKey& girderKey) = 0;
};

/*****************************************************************************
INTERFACE
   IReactions

DESCRIPTION
   Interface to get reactions
*****************************************************************************/
// {B1EA30D1-527C-4a44-AEA7-BE3F06145AC9}
DEFINE_GUID(IID_IReactions, 
0xb1ea30d1, 0x527c, 0x4a44, 0xae, 0xa7, 0xbe, 0x3f, 0x6, 0x14, 0x5a, 0xc9);
interface IReactions : IUnknown
{
   // Gets segment end reactions prior to the segment being erected. Reactions are taken to be zero after the segment
   // as been erected
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) = 0;

   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) = 0;

   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax) = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax) = 0;

   // Gets the reaction at a pier or temporary support. Reactions are taken to be zero prior to segments being erected and
   // supported. Temporary support reactions are taken to be zero after the support is removed
   virtual Float64 GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) = 0;

   virtual Float64 GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) = 0;
   virtual std::vector<Float64> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) = 0;

   virtual void GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,Float64* pRmin,Float64* pRmax) = 0;
   virtual void GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) = 0;

   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;

   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinConfig = NULL,std::vector<VehicleIndexType>* pMaxConfig = NULL) = 0;

   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) = 0;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) = 0;
};
