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

#pragma once

// SYSTEM INCLUDES
//
#include <vector>
#include <PGSuperTypes.h>
#include "Details.h"
#include <pgsExt\CamberMultipliers.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class WBFL::System::SectionValue;
class pgsPointOfInterest;
struct StressCheckTask;

// MISCELLANEOUS
//


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
   LinearLoad() :
      Xstart(0), Xend(0), Wstart(0), Wend(0) {}
   LinearLoad(Float64 Xstart, Float64 Xend, Float64 Wstart, Float64 Wend) :
      Xstart(Xstart), Xend(Xend), Wstart(Wstart), Wend(Wend) {}

   Float64 Xstart; // start location of the load, in segment coordinates
   Float64 Xend;   // end location of the load, in segment coordinates
   Float64 Wstart; // magnitude of load at start location (Force/Length)
   Float64 Wend;   // magnitude of load at end location (Force/Length)
} LinearLoad;

typedef struct ConcentratedLoad
{
   ConcentratedLoad() :Loc(0), Load(0) {}
   ConcentratedLoad(Float64 Loc, Float64 Load) : Loc(Loc), Load(Load) {}
   Float64 Loc;  // measured from left end of segment if precast or left end of span if cast-in-place
   Float64 Load;
   bool operator<(const ConcentratedLoad& other) const { return Loc < other.Loc; }
} ConcentratedLoad;

typedef ConcentratedLoad DiaphragmLoad;

struct OverlayLoad : public LinearLoad
{
   OverlayLoad() : LinearLoad(), StartWcc(0), EndWcc(0) {}
   OverlayLoad(Float64 Xstart, Float64 Xend, Float64 Wstart, Float64 Wend, Float64 StartWcc, Float64 EndWcc) :
      LinearLoad(Xstart, Xend, Wstart, Wend), StartWcc(StartWcc), EndWcc(EndWcc) {}

   Float64 StartWcc; // curb to curb width
   Float64 EndWcc; // curb to curb width
};

typedef OverlayLoad ConstructionLoad;
typedef LinearLoad SegmentLoad;
typedef LinearLoad ClosureJointLoad;
typedef LinearLoad LongitudinalJointLoad;

typedef struct SlabLoad
{
   IndexType DeckCastingRegionIdx; // index of the deck casting region
   Float64 Loc;          // measured from left girder end. Location where this load is defined
   Float64 MainSlabLoad; // if used with SIP, only the cast portion of the slab
   Float64 PanelLoad;    // Weight of SIP deck panels
   Float64 PadLoad;      // Haunch load. Zero if negative depth
   Float64 HaunchDepth;  // Assumed haunch depth. Will be negative if geometry makes it so
   Float64 SlabDepth;
   Float64 Station;
   Float64 Offset;
   Float64 GirderChordElevation;
   Float64 TopSlabElevation;
   Float64 TopGirderElevation; // includes factored camber
   Float64 AssumedExcessCamber;    // factored by % factor in project criteria
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
   EquivPretensionLoad() { memset((void*)this, 0, sizeof(EquivPretensionLoad)); }
   Float64 Xs; // location of start of load from start of segment for uniform loads or location of load from start of segment for point load
   Float64 Xe; // location of end of load from start of segment for uniform loads (not used with point loads)
   Float64 P;  // magnitude of equivalent axial load
   Float64 ex; // lateral eccentricty
   Float64 eye; // vertical eccentricty at end of girder
   Float64 eyh; // vertical eccentricity at harp point
   Float64 eprime;
   Float64 PrecamberAtLoadPoint; // precamber at the location this load is applied
   Float64 Precamber; // total precamber
   Float64 b; // distance between end of girder and harp point
   Float64 Ls; // length of segment
   Float64 N;  // magnitude of equivalent vertical load
   Float64 Mx; // magnitude of equivalent moment about the x-axis (vertical bending)
   Float64 My; // magnitude of equivalent moment about the y-axis (lateral bending)
   Float64 wy; // magnitude of equivalent uniform vertical load from Xs to Xe (vertical bending)
} EquivPretensionLoad;


// Simple span bearing reactions can occur at back/ahead. Continuous and pier reactions are at mid
typedef enum PierReactionFaceType 
{
   rftBack, 
   rftMid, 
   rftAhead
} PierReactionFaceType;

struct ReactionLocation
{
   PierIndexType        PierIdx;   // Index of the pier where reactions are reported
   PierReactionFaceType Face;      // Face of pier that reaction applies to
   CGirderKey           GirderKey; // GirderKey for the girder that provides the reaction
   std::_tstring        PierLabel; // Label (Abutment 1, Pier 2, etc)
};

typedef struct REACTION
{
   REACTION():Fx(0),Fy(0),Mz(0){}
   Float64 Fx;
   Float64 Fy;
   Float64 Mz;

   REACTION& operator+=(const REACTION& r)
   {
      Fx += r.Fx;
      Fy += r.Fy;
      Mz += r.Mz;
      return *this;
   };

   REACTION& operator-=(const REACTION& r)
   {
      Fx -= r.Fx;
      Fy -= r.Fy;
      Mz -= r.Mz;
      return *this;
   };

   REACTION& operator*=(Float64 v)
   {
      Fx *= v;
      Fy *= v;
      Mz *= v;
      return *this;
   };

   REACTION& operator/=(Float64 v)
   {
      Fx /= v;
      Fy /= v;
      Mz /= v;
      return *this;
   };

   bool operator==(const REACTION& r) const
   {
      return ::IsEqual(Fx,r.Fx) && ::IsEqual(Fy,r.Fy) && ::IsEqual(Mz,r.Mz) ? true : false;
   }

   bool operator!=(const REACTION& r) const
   {
      return !operator==(r);
   }

} REACTION;


inline REACTION operator+(const REACTION& r1,const REACTION& r2)
{
   REACTION r;
   r.Fx = r1.Fx + r2.Fx;
   r.Fy = r1.Fy + r2.Fy;
   r.Mz = r1.Mz + r2.Mz;
   return r;
}

inline REACTION operator-(const REACTION& r1,const REACTION& r2)
{
   REACTION r;
   r.Fx = r1.Fx - r2.Fx;
   r.Fy = r1.Fy - r2.Fy;
   r.Mz = r1.Mz - r2.Mz;
   return r;
}


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
   virtual LPCTSTR GetProductLoadName(pgsTypes::ProductForceType pfType) const = 0;
   virtual LPCTSTR GetLoadCombinationName(LoadingCombinationType loadCombo) const = 0;

   // Returns true if axial results should be reported.
   // Axial results are always available, however in some cases they are just a bunch
   // of zeros and it doesn't make sense to report them. Axial results should be
   // reported when there are physical piers or post-tensioning in the bridge model
   virtual bool ReportAxialResults() const = 0;

   // product loads applied to a segment including the closure joint load at the right end of the segment
   virtual void GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) const = 0;

   // gets the equivalent pretension forces. If strandType is pgsTypes::Temporary, bTempStrandInstallation is used to determine of the equivalent loads
   // are for the installation or removal interval. intervalIdx can be release or erection interval index (usually release), if INVALID_INDEX, assume release interval
   virtual std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation=true,IntervalIndexType intervalIdx=INVALID_INDEX) const = 0;
   virtual std::vector<EquivPretensionLoad> GetEquivSegmentPostTensionLoads(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx=INVALID_INDEX) const = 0;
   virtual Float64 GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSidewalkLoad(const CSegmentKey& segmentKey) const = 0;
   virtual bool HasPedestrianLoad() const = 0;
   virtual bool HasPedestrianLoad(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetPedestrianLoad(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual bool HasSidewalkLoad(const CGirderKey& girderKey) const = 0;
   virtual void GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad, Float64* pFraExtLeft, Float64* pFraIntLeft, Float64* pFraExtRight,Float64* pFraIntRight) const = 0;
   virtual void GetSidewalkLoadFraction(const CSegmentKey& segmentKey, Float64* pSidewalkLoad, Float64* pFraLeft,Float64* pFraRight) const = 0;

   // overlay loads
   virtual void GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const = 0;

   // construction loads
   virtual bool HasConstructionLoad(const CGirderKey& girderKey) const = 0;
   virtual void GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const = 0;

   // slab loads
   virtual void GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const = 0;

   // Returns the difference in moment between the slab moment for the current value of slab offset
   // and the passed design input values. Adjustment is positive if the input slab offset is greater than the current value
   virtual void GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 Fillet, std::vector<SlabLoad>* pSlabLoads) const = 0;

   // point loads due to portion of slab out on cantilever. 
   // + force is up, + moment is ccw.
   virtual void GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const = 0;
   
   // point loads due to portion of slab pad out on cantilever. 
   // + force is up, + moment is ccw.
   virtual void GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const = 0;

   // diaphragm loads
   // + force is up, + moment is ccw.
   virtual void GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const = 0;
   virtual void GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const = 0;
   // At abutments, the diaphragm load can be offset from the CL Bearing. The load is defined as an equivalent force/moment system depending on how the loading is specified
   // if the girder end distance is long enough to be modeled as a cantilever, the load applied on or at the tip the cantilever depending on its definition. The moment
   // arm parameter is the distance from the vertical load (P) to the CL Bearing.
   virtual void GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, PIER_DIAPHRAGM_LOAD_DETAILS* pBackSide, PIER_DIAPHRAGM_LOAD_DETAILS* pAheadSide) const = 0;

   virtual std::vector<std::_tstring> GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey) const = 0;

   // Shear Key load (applied from girder library entry)
   virtual bool HasShearKeyLoad(const CGirderKey& girderKey) const = 0;
   virtual void GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const = 0;

   virtual bool HasLongitudinalJointLoad() const = 0;
   virtual void GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const = 0;

   virtual std::_tstring GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const = 0;
   virtual pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const = 0;
   virtual VehicleIndexType GetVehicleCount(pgsTypes::LiveLoadType llType) const = 0;
   virtual Float64 GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const = 0;

   // get list of product loads for a given load combo tpe
   virtual std::vector<pgsTypes::ProductForceType> GetProductForcesForCombo(LoadingCombinationType combo) const = 0;

   // get list of pertinent product loads for a given girder. use this when creating reports
   virtual std::vector<pgsTypes::ProductForceType> GetProductForcesForGirder(const CGirderKey& girderKey) const = 0;
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
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) const = 0;
   virtual pgsTypes::BridgeAnalysisType GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) const = 0;

   virtual Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false,bool bIncludePrecamber=false,bool bIncludePreErectionUnrecov=true) const = 0;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false, bool bIncludePrecamber=false, bool bIncludePreErectionUnrecov=true) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const = 0;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck = nullptr,VehicleIndexType* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = nullptr,VehicleIndexType* pMaxConfig = nullptr) const = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const = 0;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig=nullptr,VehicleIndexType* pTopMaxConfig=nullptr,VehicleIndexType* pBotMinConfig=nullptr,VehicleIndexType* pBotMaxConfig=nullptr) const = 0;

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax,
                                          AxleConfiguration* pMinLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMinRightAxleConfig = nullptr,
                                          AxleConfiguration* pMaxLeftAxleConfig = nullptr,
                                          AxleConfiguration* pMaxRightAxleConfig = nullptr) const = 0;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop=nullptr,AxleConfiguration* pMaxAxleConfigTop=nullptr,AxleConfiguration* pMinAxleConfigBot=nullptr,AxleConfiguration* pMaxAxleConfigBot=nullptr) const = 0;

   // This is the deflection due to self weight of the girder, when the girder
   // is in the casting yard, supported at the storage support points. Modulus
   // of elasticity is equal to Eci.  This differs from GetDeflection above
   // in that the span length L is the span length of the girder instead of the
   // girder length usually used in the casting yard.
   virtual Float64 GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Deflection due to LRFD optional design live load (LRFD 3.6.1.3.2)
   enum DeflectionLiveLoadType { DesignTruckAlone, Design25PlusLane, DeflectionLiveLoadEnvelope }; 
   virtual void GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const = 0;

   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const = 0;

   // returns the difference in deflection between the slab deflection for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const = 0;

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab 
   // and the input value.
   virtual void GetDesignSlabStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const = 0;

   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const = 0;

   // returns the difference in deflection between the slab pad deflection for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   virtual Float64 GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const = 0;

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   virtual void GetDesignSlabPadStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const = 0;

   virtual void DumpAnalysisModels(GirderIndexType gdrIdx) const = 0;

   virtual void GetDeckShrinkageStresses(const pgsPointOfInterest& poi, pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,Float64* pftop,Float64* pfbot) const = 0;
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
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false, bool bIncludePrecamber=false,bool bIncludePreErectionUnrecov=true) const = 0;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment=false, bool bIncludePrecamber=false,bool bIncludePreErectionUnrecov=true) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const = 0;

   virtual void GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck = nullptr,std::vector<VehicleIndexType>* pMmaxTruck = nullptr) const = 0;
   virtual void GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const = 0;
   virtual void GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const = 0;
   virtual void GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex=nullptr,std::vector<VehicleIndexType>* pTopMaxIndex=nullptr,std::vector<VehicleIndexType>* pBotMinIndex=nullptr,std::vector<VehicleIndexType>* pBotMaxIndex=nullptr) const = 0;

   virtual void GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax,
                                          std::vector<AxleConfiguration>* pMinLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMinRightAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxLeftAxleConfig = nullptr,
                                          std::vector<AxleConfiguration>* pMaxRightAxleConfig = nullptr) const = 0;
   virtual void GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigurationTop=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigurationTop=nullptr,std::vector<AxleConfiguration>* pMinAxleConfigurationBot=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfigurationBot=nullptr) const = 0;

   // Function returns permanent deflection caused by girder dead load and modulus stiffening at storage. Values are adjusted for support location for given interval
   enum sagInterval { sagHauling,sagErection };
   virtual std::vector<Float64> GetUnrecoverableGirderDeflectionFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const = 0;
   virtual std::vector<Float64> GetUnrecoverableGirderRotationFromStorage(sagInterval interval,pgsTypes::BridgeAnalysisType bat,const PoiList& vPoi) const=0;

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
   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber=false,bool bIncludePreErectionUnrecov=true) const = 0;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false, bool bIncludePreErectionUnrecov=true) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const = 0;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const = 0;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeShear,WBFL::System::SectionValue* pVmin,WBFL::System::SectionValue* pVmax) const = 0;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMmin,Float64* pMmax) const = 0;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const = 0;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const = 0;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const = 0;
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
   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false, bool bIncludePrecamber = false, bool bIncludePreErectionUnrecov=true) const = 0;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType combo, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false, bool bIncludePrecamber = false, bool bIncludePreErectionUnrecov=true) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const = 0;

   virtual void GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const = 0;
   virtual void GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact, std::vector<WBFL::System::SectionValue>* pVmin,std::vector<WBFL::System::SectionValue>* pVmax) const = 0;
   virtual void GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const = 0;
   virtual void GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const = 0;
   virtual void GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const = 0;
   virtual void GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const = 0;
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
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const = 0;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const = 0;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const = 0;
   virtual void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, Float64* pMin, Float64* pMax) const = 0;
   virtual void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, Float64* pMin, Float64* pMax) const = 0;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, Float64* pMin,Float64* pMax) const = 0;
   // Warning: Reactions for limit states are not for public consumption: They make no sense. However, they are useful for computing uplift for other calculations. Use at your own peril.
   virtual void GetLSReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,Float64* pMin,Float64* pMax) const = 0;

   virtual void GetDesignStress(const StressCheckTask& task, const pgsPointOfInterest& poi,pgsTypes::StressLocation loc, const GDRCONFIG* pConfig,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const = 0;

   virtual void GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,WBFL::System::SectionValue* pMin,WBFL::System::SectionValue* pMax) const = 0;
   virtual void GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const = 0;

   virtual Float64 GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) const = 0;

   // Return true if limit state is to be reported
   virtual bool IsStrengthIIApplicable(const CGirderKey& girderKey) const = 0;
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
   virtual void GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const = 0;
   virtual void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<WBFL::System::SectionValue>* pMin,std::vector<WBFL::System::SectionValue>* pMax) const = 0;
   virtual void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const = 0;
   virtual void GetDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, bool bIncludeLiveLoad, bool bIncludeElevationAdjustment, bool bIncludePrecamber,bool bIncludePreErectionUnrecov, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const = 0;
   virtual void GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const = 0;
   virtual void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,bool bIncludePrecamber,bool bIncludePreErectionUnrecov,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const = 0;

   virtual std::vector<Float64> GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const = 0;
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
   virtual bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz) = 0;

   // creates a uniform load
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) = 0;
   virtual bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy) = 0;

   // creates a constant initial strain load between two POI.
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) = 0;
   virtual bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r) = 0;

   virtual Float64 GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual WBFL::System::SectionValue GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual Float64 GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment=false) const = 0;
   virtual Float64 GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual Float64 GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const = 0;

   virtual std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<WBFL::System::SectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment = false) const = 0;
   virtual std::vector<Float64> GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment = false) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const = 0;

   // Gets segment end reactions prior to the segment being erected. Reactions are taken to be zero after the segment
   // as been erected
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const = 0;

   // Gets the reaction at a pier or temporary support. Reactions are taken to be zero prior to segments being erected and
   // supported. Temporary support reactions are taken to be zero after the support is removed
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
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
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,bool bIncludeLiveLoad,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLocation,bool bIncludeLiveLoad, pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx,Float64* pfTop,Float64* pfBot) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation loc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pStress) const = 0;
   virtual void GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pvfTop,std::vector<Float64>* pvfBot) const = 0;
   virtual Float64 GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 P,Float64 ex,Float64 ey) const = 0;
   virtual Float64 GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation loc) const = 0;
   virtual Float64 GetDesignStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation loc, const GDRCONFIG& config, bool bIncludeLiveLoad, pgsTypes::LimitState limitState) const = 0;
   virtual void GetDesignStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc, const GDRCONFIG& config, bool bIncludeLiveLoad, pgsTypes::LimitState limitState,Float64* pfTop,Float64* pfBot) const = 0;
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

   virtual Uint32 GetCreepMethod() const = 0;

   // Returns the creep multiplier used to scale the instantious deflection
   // constructionRate indicates if it is rapid or normal construction
   virtual Float64 GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns details of creep coefficient calculations.
   virtual CREEPCOEFFICIENTDETAILS GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate, const GDRCONFIG* pConfig = nullptr) const = 0;

   virtual std::shared_ptr<const lrfdCreepCoefficient> GetGirderCreepModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual std::shared_ptr<const lrfdCreepCoefficient2005> GetDeckCreepModel(IndexType deckCastingRegionIdx) const = 0;
    
   // Deflection caused by permanent prestressing alone (Harped and Straight Strands only).
   // Deflections are computed using the concrete properties at release.
   // Deflections are measured relative to the support location defined by datum
   // Returns Y deflection
   virtual Float64 GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig=nullptr) const = 0;
   // Returns X and Y deflection
   virtual void GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy) const = 0;

   // Deflection caused by temporary prestressing alone when prestressed stressed at casting.
   // Deflections are computed using the concrete properties at release.
   // Deflections are measured relative to the support location defined by datum
   // Returns Y deflection
   virtual Float64 GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const = 0;
   // Returns X and Y deflection
   virtual void GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy) const = 0;

   // Returns the amount the girder deflects when the temporary strands are
   // removed.  Deflections are computed using 28day concrete properties.
   // Returns Y deflection
   virtual Float64 GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;
   // Returns X and Y deflection
   virtual void GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy) const = 0;

   virtual Float64 GetInitialCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the deflection due to creep at the end of a creep period
   virtual Float64 GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual void GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig,Float64* pDcreep,Float64* pRcreep) const = 0;

   // Returns the lateral deflection due to creep at the end of a creep period
   virtual Float64 GetXCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the amount the girder deflects when the deck is
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects when the deck panels are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects when the shear keys are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects when the longitudinal joints are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetLongitudinalJointDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the amount the girder deflects when the construction load is
   // applied.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects when the diaphragms are
   // cast.  Deflections are computed using 28day concrete properties.
   virtual Float64 GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects when user loads are applied.
   // Camber is Cumulative through stages.
   virtual Float64 GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the amount the girder deflects slab+barrier+overlay loads are applied.
   // Camber is Cumulative through stages.
   virtual Float64 GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;

   // This is the negative value of the amount the girder will deflect after the slab is placed (if there is no slab, after the diaphragms are cast)
   virtual Float64 GetScreedCamber(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetScreedCamberUnfactored(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;

   // This is the camber that remains in the girder after the slab pour
   // It is equal to the Total camber less the screed camber.
   virtual Float64 GetExcessCamber(const pgsPointOfInterest& poi,Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetExcessCamberEx(const pgsPointOfInterest& poi, Int16 time, Float64* pDy, Float64* pCy,const GDRCONFIG* pConfig = nullptr) const = 0;

   // This is the rotation in the girder that renames after the slab pour
   // It is equal to the rotation due to total camber, less the rotation for screed camber
   virtual Float64 GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;

   // This is the camber in the girder, just prior to slab casting.
   virtual Float64 GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetDCamberForGirderScheduleUnfactored(const pgsPointOfInterest& poi,Int16 time, const GDRCONFIG* pConfig=nullptr) const = 0;

   virtual void GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi, Int16 time, Float64* pUpperBound,Float64* pAvg,Float64* pLowerBound,const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual void GetDCamberForGirderScheduleUnfactoredEx(const pgsPointOfInterest& poi, Int16 time, Float64* pUpperBound, Float64* pAvg, Float64* pLowerBound, const GDRCONFIG* pConfig = nullptr) const = 0;

   // This is the factor that the min timing camber is multiplied by the compute the lower bound camber
   virtual Float64 GetLowerBoundCamberVariabilityFactor()const = 0;

   // Camber multipliers for final factored camber
   virtual CamberMultipliers GetCamberMultipliers(const CSegmentKey& segmentKey) const = 0;

   // Returns true if at least on of the segments in the girder has precamber
   virtual bool HasPrecamber(const CGirderKey& girderKey) const = 0;

   // returns the total precamber
   virtual Float64 GetPrecamber(const CSegmentKey& segmentKey) const = 0;

   // returns the precamber at a POI, adjusted for the measurement datum
   virtual Float64 GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum) const = 0;
   virtual void GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, Float64* pDprecamber, Float64* pRprecamber) const = 0;
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
   virtual void GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const = 0;
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
   virtual bool IsContinuityFullyEffective(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) const = 0;
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
   // Returns true if live load reactions on bearings include impact
   virtual bool BearingLiveLoadReactionsIncludeImpact() const = 0;

   // Returns a list of piers where bearing reactions are available
   virtual std::vector<PierIndexType> GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const = 0;

   // Returns the bearing reaction due to a product load
   virtual Float64 GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;

   // Returns the max and min live load reaction at the specified reaction location along with the corresponding rotation
   virtual void GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const = 0;

   // Returns the max and min live load rotation at the specified reaction location along with the corresponding reaction
   virtual void GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                   Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                   VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const = 0;

   // Returns the bearing reaction due to a load combination (DC, DW, etc)
   virtual Float64 GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType combo,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;

   // Returns the per-girder min/max reaction at the specified reaction location
   virtual void GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                   Float64* pRmin,Float64* pRmax) const = 0;

   // Returns the bearing reaction due to a limit state combintation.
   virtual void GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                             pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                             Float64* pRmin,Float64* pRmax) const = 0;
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
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ) const = 0;

   // Returns true of the specified location is in the precompressed tensile zone
   virtual void IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const = 0;

   // Returns true if the deck is "precompressed". The deck is considered precompressed if
   // it experiences direct stresses due to post-tensioning applied after the deck
   // has become composite
   virtual bool IsDeckPrecompressed(const CGirderKey& girderKey) const = 0;
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
   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const = 0;

   virtual void GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const = 0;
   virtual void GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const = 0;

   // Gets the reaction at a pier or temporary support. Reactions are taken to be zero prior to segments being erected and
   // supported. Temporary support reactions are taken to be zero after the support is removed
   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;

   virtual REACTION GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;
   virtual std::vector<REACTION> GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const = 0;

   // Live load reactions are per lane only. LLDF's were removed in 11/2019
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,REACTION* pRmin,REACTION* pRmax,AxleConfiguration* pMinAxleConfig=nullptr,AxleConfiguration* pMaxAxleConfig=nullptr) const = 0;
   virtual void GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig=nullptr,std::vector<AxleConfiguration>* pMaxAxleConfig=nullptr) const = 0;

   // Note that reactions are per lane in GetLiveLoadReaction, except for the case of pedestrian load, where they are distributed per-girder
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,REACTION* pRmin,REACTION* pRmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinConfig = nullptr,std::vector<VehicleIndexType>* pMaxConfig = nullptr) const = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx = nullptr,VehicleIndexType* pMaxVehIdx = nullptr) const = 0;
   virtual void GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx = nullptr,std::vector<VehicleIndexType>* pMaxVehIdx = nullptr) const = 0;

   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const = 0;
   virtual void GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const = 0;
};

/*****************************************************************************
INTERFACE
   IDeformedGirderGeometry

DESCRIPTION
   Interface for obtaining elevations of deformed bridge
*****************************************************************************/
// {CB6DDE05-1618-468E-AE8B-30031F938D71}
DEFINE_GUID(IID_IDeformedGirderGeometry,
   0xcb6dde05,0x1618,0x468e,0xae,0x8b,0x30,0x3,0x1f,0x93,0x8d,0x71);
interface IDeformedGirderGeometry : public IUnknown
{
   // Functions to get elevations of the deformed structure
   // 
   // Returns the top of girder elevation at the centerline girder FOR DESIGN FOR PGSUPER MODELS WITH ADIM INPUT ONLY. 
   // If pConfig is nullptr, the slab offset and excess camber from thebridge model are used, otherwise the slab offset from the config
   // is used and the excess camber is computed using the supplied configuration. 
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the top of girder elevation for the left, center, and right edges of the girder at the specified poi at the time of 
   // the Geometry Control Event (GCE).
   //  The elevation takes into account slab offsets and excess camber. Direction defines a tranverse line passing through poi. 
   // Left and Right elevations are computed where the transverse line intersects the edges of the girder. 
   // If pDirection is nullptr, the transverse line is taken to be normal to the girder
   virtual void GetTopGirderElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const = 0;

   // Function below is only valid for direct haunch input. 
   virtual void GetTopGirderElevationEx(const pgsPointOfInterest& poi,IntervalIndexType interval,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const = 0;

   // Finished elevation, ONLY for NO-DECK girders at time of the GCE
   // Returns the finished top of girder elevation for the left, center, and right edges of the girder at the specified poi. The elevation takes into
   // account elevation adjustments and excess camber. Direction defines a tranverse line passing through poi. Left and Right elevations are computed
   // where the transverse line intersects the edges of the girder. If pDirection is nullptr, the transverse line is taken to be normal to the girder.
   // The depth of the overlay is included if applied at or before the GCE (future overlays are not included), otherwise this method is the same
   // as GetTopGirderElevation for no-deck bridges
   virtual void GetFinishedElevation(const pgsPointOfInterest& poi,IDirection* pDirection,Float64* pLeft,Float64* pCenter,Float64* pRight) const = 0;

   // Finished elevation for direct haunch input. Elevation can only be checked at CL girder because this is a hard point where the haunch depth is input. 
   // Haunch is pliable at left & right locations, so we return haunch depth at left/center/right to be checked against fillet dimension.
   virtual Float64 GetFinishedElevation(const pgsPointOfInterest& poi,IntervalIndexType interval,Float64* pLftHaunch,Float64* pCtrHaunch,Float64* pRgtHaunch) const = 0;
};
