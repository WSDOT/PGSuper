///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PRESTRESS_H_
#define INCLUDED_IFACE_PRESTRESS_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#include <Details.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;
class matPsStrand;
class rptChapter;
interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IPretensionForce

   Interface to prestress force and stress information

DESCRIPTION
   Interface to prestress force and stress information.  This is computed
   information and not the raw input data.
*****************************************************************************/
// {381E19E0-6E82-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IPretensionForce, 
0x381e19e0, 0x6e82, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IPretensionForce : IUnknown
{
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const = 0;
   virtual Float64 GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands) const = 0;

   virtual Float64 GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;
   virtual Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded,bool bUHPC, const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,bool bUHPC,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the governing hold down force based on the average harped strand slope. The slope associated with
   // the hold down force is returned through the pSlope parameter. The poi where the max hold down force occurs is returned through pPoi
   virtual Float64 GetHoldDownForce(const CSegmentKey& segmentKey,bool bTotalForce=true,Float64* pSlope=nullptr,pgsPointOfInterest* pPoi=nullptr,const GDRCONFIG* pConfig = nullptr) const = 0;

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const = 0;

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const = 0;

   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects) const = 0;

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects) const = 0;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects) const = 0;

   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, VehicleIndexType vehicleIndex = INVALID_INDEX) const = 0;
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, VehicleIndexType vehicleIndex = INVALID_INDEX) const = 0;

   virtual void GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config, Float64* pLowerBound, Float64* pUpperBound) const = 0;
};


/*****************************************************************************
INTERFACE
   IPosttensionForce

   Interface to post-tension force and stress information

DESCRIPTION
   Interface to post-tension force and stress information.  This is computed
   information and not the raw input data.
*****************************************************************************/
// {4E2D92B7-73B1-4dcd-8450-A4D18ED9F2B4}
DEFINE_GUID(IID_IPosttensionForce, 
0x4e2d92b7, 0x73b1, 0x4dcd, 0x84, 0x50, 0xa4, 0xd1, 0x8e, 0xd9, 0xf2, 0xb4);
interface IPosttensionForce : IUnknown
{
   // Returns the maximum jacking force for a given number of strands
   virtual Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands) const = 0;
   virtual Float64 GetGirderTendonPjackMax(const CGirderKey& girderKey,const matPsStrand& strand,StrandIndexType nStrands) const = 0;
   virtual Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, StrandIndexType nStrands) const = 0;
   virtual Float64 GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, const matPsStrand& strand, StrandIndexType nStrands) const = 0;

   // Returns the force in the tendon at jacking including the effect of friction. 
   // If bIncludeAnchorSet, anchor set losses are taken into account as well. This force
   // is based on the actual values and not the values computed using the average friction and anchor set loss
   virtual Float64 GetGirderTendonInitialForce(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const = 0;
   virtual Float64 GetGirderTendonInitialStress(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const = 0;
   virtual Float64 GetGirderTendonAverageInitialForce(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual Float64 GetGirderTendonAverageInitialStress(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   virtual Float64 GetSegmentTendonInitialForce(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const = 0;
   virtual Float64 GetSegmentTendonInitialStress(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const = 0;
   virtual Float64 GetSegmentTendonAverageInitialForce(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonAverageInitialStress(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const = 0;

   // returns the force in a tendon in a particular interval. (use ALL_DUCTS for all tendons)
   // if bIncludeMinLiveLoad is true, the force related to the elastic stress due to the minimum limit state live load is included in the tendon force.
   // if bIncludeMaxLiveLoad is true, the force related to the elastic stress due to the maximum limit state live load is included in the tendon force.
   // if both bIncludeMinLiveLoad and bIncludeMaxLiveLoad are true, the live load that maximizes the tendon force is used.
   // these values are based on the average friction and anchor set loss being applied to fpj.
   virtual Float64 GetGirderTendonForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad = false, bool bIncludeMaxLiveLoad = false, pgsTypes::LimitState limitState = pgsTypes::ServiceIII, VehicleIndexType vehicleIdx = INVALID_INDEX) const = 0;
   virtual Float64 GetSegmentTendonForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad = false, bool bIncludeMaxLiveLoad = false, pgsTypes::LimitState limitState = pgsTypes::ServiceIII, VehicleIndexType vehicleIdx = INVALID_INDEX) const = 0;

   // returns the effective stress in a tendon in a particular interval.
   // if bIncludeMinLiveLoad is true, the elastic stress due to the minimum limit state live load is included in the tendon stress.
   // if bIncludeMaxLiveLoad is true, the elastic stress due to the maximum limit state live load is included in the tendon stress.
   // if both bIncludeMinLiveLoad and bIncludeMaxLiveLoad are true, the live load stress that maximizes the tendon stress is used.
   // these values are based on the average friction and anchor set loss being applied to fpj.
   virtual Float64 GetGirderTendonStress(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad = false, bool bIncludeMaxLiveLoad = false, pgsTypes::LimitState limitState = pgsTypes::ServiceIII, VehicleIndexType vehicleIdx = INVALID_INDEX) const = 0;
   virtual Float64 GetSegmentTendonStress(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad = false, bool bIncludeMaxLiveLoad = false, pgsTypes::LimitState limitState = pgsTypes::ServiceIII, VehicleIndexType vehicleIdx = INVALID_INDEX) const = 0;

   // returns the vertical component of the post-tension force
   virtual Float64 GetGirderTendonVerticalForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonVerticalForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, DuctIndexType ductIdx) const = 0;
};

/*****************************************************************************
INTERFACE
   ILosses

   Interface to get losses.

DESCRIPTION
   Interface to get losses. The losses returned by this interface
   are the effective prestress losses. When gross section properties are 
   used, any elastic gain/losses are included.
*****************************************************************************/
// {03D91150-6DBB-11d2-8EE9-006097DF3C68}
DEFINE_GUID(IID_ILosses, 
0x3d91150, 0x6dbb, 0x11d2, 0x8e, 0xe9, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ILosses : IUnknown
{
   // Returns the details of the prestress loss calculation for losses computed up to and including
   // intervalIdx. Loses may be computed beyond this interval as well, however they are only
   // guarenteed to be computed up to and including the specified interval. An intervalIdx of
   // INVALID_INDEX means that losses are computed through all intervals
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx = INVALID_INDEX) const = 0;

   // Returns the name of the load case use to analyze creep, shrinkage, and creep restraining forces
   // use one of the TIMESTEP_XXX constants
   virtual CString GetRestrainingLoadName(IntervalIndexType intervalIdx,int loadType) const = 0;

   // losses based on current input
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) const = 0;

   // losses based on a girder configuration and slab offset
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) const = 0;
   virtual const LOSSDETAILS* GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx=INVALID_INDEX) const = 0;
   virtual void ClearDesignLosses() = 0;

   virtual void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const = 0;

   // Returns the effective prestress loss... The effective losses are time depenent losses + elastic effects
   // effective losses = fpj - fpe
   virtual Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig,bool bIncludeElasticEffects) const = 0;

   // Returns the time-dependent losses only. They do not include elastic effects
   virtual Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the change in prestress due to elastic effects including elastic shortening
   // and externally applied loads. Elastic gain due to live load is not included except for
   // the "WithLiveLoad" versions
   virtual Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx = INVALID_INDEX, const GDRCONFIG* pConfig = nullptr) const = 0;

   virtual Float64 GetGirderTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetGirderTendonAnchorSetZoneLength(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const = 0;
   virtual Float64 GetSegmentTendonAnchorSetZoneLength(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const = 0;
   virtual Float64 GetGirderTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const = 0;
   virtual Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const = 0;
   virtual Float64 GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual Float64 GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const = 0;
   virtual Float64 GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const = 0;

   // Return true if elastic gains and/or deck shrinkage should be included in the losses
   virtual bool AreElasticGainsApplicable() const = 0;
   virtual bool IsDeckShrinkageApplicable() const = 0;
};

#endif // INCLUDED_IFACE_PRESTRESS_H_

