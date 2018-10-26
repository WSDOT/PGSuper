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

#ifndef INCLUDED_IFACE_ANALYSISRESULTS_H_
#define INCLUDED_IFACE_ANALYSISRESULTS_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

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

enum BridgeAnalysisType
{ 
   SimpleSpan, 
   ContinuousSpan, 
   MinSimpleContinuousEnvelope, 
   MaxSimpleContinuousEnvelope 
};

enum ProductForceType 
{ 
   pftGirder,
   pftSlab, 
   pftSlabPanel, 
   pftDiaphragm, 
   pftOverlay,
   pftSidewalk,
   pftTrafficBarrier, 
   pftUserDC, 
   pftUserDW, 
   pftUserLLIM,
   pftShearKey
};

enum LoadingCombination
{ 
   lcDC, 
   lcDW 
};

enum CombinationType 
{ 
   ctCummulative, 
   ctIncremental 
};

struct AxlePlacement
{
   double Weight;
   double Location;
};

typedef std::vector<AxlePlacement> AxleConfiguration;

typedef struct GirderLoad
{
   Float64 StartLoc; // measured from left end of girder
   Float64 EndLoc;   // measured from left end of girder
   Float64 wStart;
   Float64 wEnd;
} GirderLoad;

typedef struct DiaphragmLoad
{
   Float64 Loc;  // measured from left end of girder
   Float64 Load;
   bool operator<(const DiaphragmLoad& other) const { return Loc < other.Loc; }
} DiaphragmLoad;

typedef struct OverlayLoad
{
   Float64 StartLoc;// measured from left end of girder
   Float64 EndLoc;// measured from left end of girder
   Float64 StartWcc; // curb to curb width
   Float64 StartLoad;
   Float64 EndWcc; // curb to curb width
   Float64 EndLoad;
} OverlayLoad;

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
   virtual pgsTypes::Stage GetGirderDeadLoadStage(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual pgsTypes::Stage GetGirderDeadLoadStage(GirderIndexType gdrLineIdx) = 0;

   // product loads applied to girder
   virtual void GetGirderSelfWeightLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad)=0;
   virtual Float64 GetTrafficBarrierLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)=0;
   virtual Float64 GetSidewalkLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx)=0;
   virtual bool HasPedestrianLoad() = 0;
   virtual bool HasPedestrianLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual Float64 GetPedestrianLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual bool HasSidewalkLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;

   // overlay loads
   virtual void GetOverlayLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<OverlayLoad>* pOverlayLoads)=0;

   // slab loads
   virtual void GetMainSpanSlabLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<SlabLoad>* pSlabLoads)=0;
   // point loads due to portion of slab out on cantilever. 
   // + force is up, + moment is ccw.
   virtual void GetCantileverSlabLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)=0;

   // diaphragm loads
   // + force is up, + moment is ccw.
   virtual void GetIntermediateDiaphragmLoads(pgsTypes::Stage stage, SpanIndexType spanIdx,GirderIndexType gdrIdx, std::vector<DiaphragmLoad>* pLoads)=0;
   virtual void GetEndDiaphragmLoads(SpanIndexType spanIdx,GirderIndexType gdrIdx, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)=0;

   virtual std::vector<std::string> GetVehicleNames(pgsTypes::LiveLoadType llType,GirderIndexType gdr) = 0;

   // Shear Key load (applied from girder library entry)
   virtual bool HasShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
   virtual void GetShearKeyLoad(SpanIndexType spanIdx,GirderIndexType gdrIdx,std::vector<ShearKeyLoad>* pLoads)=0;

   virtual std::string GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex) = 0;
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) = 0;
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex) = 0;
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
   virtual sysSectionValue GetShear(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat) = 0;
   virtual Float64 GetMoment(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat) = 0;
   virtual Float64 GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat) = 0;
   virtual Float64 GetRotation(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat) = 0;
   virtual Float64 GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat) = 0;
   virtual void GetStress(pgsTypes::Stage stage,ProductForceType type,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot) = 0;

   virtual void GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,long* pMminTruck = NULL,long* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,long* pMminTruck = NULL,long* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,long* pMinConfig = NULL,long* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,long* pMinConfig = NULL,long* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,long* pMinConfig,long* pMaxConfig) = 0;
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,long* pMinConfig,long* pMaxConfig) = 0;
   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,long* pMinConfig = NULL,long* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,long* pMinConfig = NULL,long* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,long* pTopMinConfig=NULL,long* pTopMaxConfig=NULL,long* pBotMinConfig=NULL,long* pBotMaxConfig=NULL) = 0;

   virtual void GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = NULL,
                                          AxleConfiguration* pMinRightAxleConfig = NULL,
                                          AxleConfiguration* pMaxLeftAxleConfig = NULL,
                                          AxleConfiguration* pMaxRightAxleConfig = NULL) = 0;
   virtual void GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadReaction(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=NULL,AxleConfiguration* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=NULL,AxleConfiguration* pMaxAxleConfigTop=NULL,AxleConfiguration* pMinAxleConfigBot=NULL,AxleConfiguration* pMaxAxleConfigBot=NULL) = 0;

   // This is the deflection due to self weight of the girder, when the girder
   // is in the casting yard, supported on its final bearing points. Modulus
   // of elasticity is equal to Eci.  This differs from GetDisplacement above
   // in that the span length L is the span length of the girder instead of the
   // girder length usually used in the casting yard.
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   // Deflection due to LRFD optional design live load (LRFD 3.6.1.3.2)
   enum DeflectionLiveLoadType { DesignTruckAlone, Design25PlusLane, DeflectionLiveLoadEnvelope }; 
   virtual void GetDeflLiveLoadDisplacement(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,Float64* pDmin,Float64* pDmax) = 0;

   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadMomentAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in deflection between the slab pad deflection for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi) = 0;

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   virtual void GetDesignSlabPadStressAdjustment(double fcgdr,double startSlabOffset,double endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot) = 0;

   virtual void DumpAnalysisModels(GirderIndexType girderLineIdx) = 0;
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
   virtual std::vector<sysSectionValue> GetShear(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat) = 0;
   virtual std::vector<Float64> GetMoment(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat) = 0;
   virtual std::vector<Float64> GetDisplacement(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat) = 0;
   virtual std::vector<Float64> GetRotation(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat) = 0;
   virtual void GetStress(pgsTypes::Stage stage,ProductForceType type,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) = 0;

   virtual void GetLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<long>* pMminTruck = NULL,std::vector<long>* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<long>* pMminTruck = NULL,std::vector<long>* pMmaxTruck = NULL) = 0;
   virtual void GetLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<long>* pMinConfig = NULL,std::vector<long>* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadRotation(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<long>* pMinConfig = NULL,std::vector<long>* pMaxConfig = NULL) = 0;
   virtual void GetLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<long>* pTopMinIndex=NULL,std::vector<long>* pTopMaxIndex=NULL,std::vector<long>* pBotMinIndex=NULL,std::vector<long>* pBotMaxIndex=NULL) = 0;

   virtual void GetVehicularLiveLoadMoment(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadShear(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = NULL,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = NULL) = 0;
   virtual void GetVehicularLiveLoadDisplacement(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadRotation(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=NULL,std::vector<AxleConfiguration>* pMaxAxleConfig=NULL) = 0;
   virtual void GetVehicularLiveLoadStress(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationTop=NULL,std::vector<AxleConfiguration>* pMinAxleConfigurationBot=NULL,std::vector<AxleConfiguration>* pMaxAxleConfigurationBot=NULL) = 0;
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
   virtual sysSectionValue GetShear(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual Float64 GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual Float64 GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual Float64 GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual void GetStress(LoadingCombination combo,pgsTypes::Stage stage,const pgsPointOfInterest& poi,CombinationType type,BridgeAnalysisType bat,Float64* pfTop,Float64* pfBot) = 0;

   virtual void GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) = 0;
   virtual void GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pVmin,sysSectionValue* pVmax) = 0;
   virtual void GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) = 0;
   virtual void GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) = 0;
   virtual void GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) = 0;
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
   virtual std::vector<sysSectionValue> GetShear(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual std::vector<Float64> GetMoment(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual std::vector<Float64> GetDisplacement(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual void GetStress(LoadingCombination combo,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,CombinationType type,BridgeAnalysisType bat,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) = 0;

   virtual void GetCombinedLiveLoadMoment(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) = 0;
   virtual void GetCombinedLiveLoadShear(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax) = 0;
   virtual void GetCombinedLiveLoadDisplacement(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) = 0;
   virtual void GetCombinedLiveLoadStress(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) = 0;
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
   virtual void GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) = 0;
   virtual void GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;
   virtual void GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;
   virtual void GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;
   virtual void GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) = 0;

   virtual void GetDesignStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,BridgeAnalysisType bat,Float64* pMin,Float64* pMax) = 0;

   virtual void GetConcurrentShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) = 0;
   virtual void GetViMmax(pgsTypes::LimitState ls,pgsTypes::Stage stage,const pgsPointOfInterest& poi,BridgeAnalysisType bat,double* pVi,double* pMmax) = 0;

   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState ls,const pgsPointOfInterest& poi,BridgeAnalysisType bat) = 0;
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
   virtual void GetShear(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax) = 0;
   virtual void GetMoment(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;
   virtual void GetDisplacement(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;
   virtual void GetStress(pgsTypes::LimitState ls,pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc,bool bIncludePrestress,BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) = 0;

   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,BridgeAnalysisType bat) = 0;
};

/*****************************************************************************
INTERFACE
   IPrestressStresses

   Interface to get the stresses caused by prestressing.

DESCRIPTION
   Interface to get the stresses caused by prestressing.
*****************************************************************************/
// {FDCC4ED6-7D9B-11d2-8857-006097C68A9C}
DEFINE_GUID(IID_IPrestressStresses, 
0xfdcc4ed6, 0x7d9b, 0x11d2, 0x88, 0x57, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IPrestressStresses : IUnknown
{
   virtual Float64 GetStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc) = 0;
   virtual Float64 GetStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 e) = 0;
   virtual Float64 GetStressPerStrand(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) = 0;

   virtual Float64 GetDesignStress(pgsTypes::Stage stage,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,const GDRCONFIG& config) = 0;

   virtual std::vector<Float64> GetStress(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation loc) = 0;
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
   virtual Float64 GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate) = 0;
   virtual Float64 GetCreepCoefficient(SpanIndexType span,GirderIndexType gdr, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate) = 0;

   // Returns details of creep coefficient calculations.
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr, CreepPeriod creepPeriod, Int16 constructionRate) = 0;
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(SpanIndexType span,GirderIndexType gdr, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate) = 0;

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
   virtual Float64 GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetUserLoadDeflection(pgsTypes::Stage, const pgsPointOfInterest& poi, const GDRCONFIG& config) = 0;

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
   // It is equal to the Total camber less the slab displacement.
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time) = 0;
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time) = 0; 

   virtual void GetHarpedStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMl,Float64* pMr,Float64* pNl,Float64* pNr,Float64* pXl,Float64* pXr) = 0;
   virtual void GetTempStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,Float64* pMxferL,Float64* pMxferR,Float64* pMremoveL,Float64* pMremoveR) = 0;
   virtual void GetStraightStrandEquivLoading(SpanIndexType span,GirderIndexType gdr,std::vector< std::pair<Float64,Float64> >* loads) = 0;
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
   virtual void GetContraflexurePoints(SpanIndexType span,GirderIndexType gdr,double* cfPoints,Uint32* nPoints) = 0;
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
   virtual bool IsContinuityFullyEffective(GirderIndexType girderline) = 0;
   virtual double GetContinuityStressLevel(PierIndexType pier,GirderIndexType gdr) = 0;
};

#endif // INCLUDED_IFACE_ANALYSISRESULTS_H_

