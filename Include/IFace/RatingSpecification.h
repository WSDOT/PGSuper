///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <WbflTypes.h>

#include <psglib\librarymanager.h>

#include <IFace\AnalysisResults.h> // for AxleConfiguration

/*****************************************************************************
INTERFACE
   IRatingSpecification

   Interface to manipulating the rating specification input

DESCRIPTION
   Interface to manipulating the rating specification input
*****************************************************************************/
// {10198680-3270-4adb-A768-A6C01069D9C0}
DEFINE_GUID(IID_IRatingSpecification, 
0x10198680, 0x3270, 0x4adb, 0xa7, 0x68, 0xa6, 0xc0, 0x10, 0x69, 0xd9, 0xc0);
interface IRatingSpecification : IUnknown
{
   // returns true if any one of the rating types is enabled
   virtual bool IsRatingEnabled() const = 0;

   // returns true if a specific rating type is enabled
   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const = 0;
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable) = 0;

   virtual std::_tstring GetRatingSpecification() const = 0;
   virtual void SetRatingSpecification(const std::_tstring& spec) = 0;

   virtual void SetADTT(Int16 adtt) = 0;
   virtual Int16 GetADTT() const = 0; // < 0 = Unknown

   virtual void IncludePedestrianLiveLoad(bool bInclude) = 0;
   virtual bool IncludePedestrianLiveLoad() const = 0;

   virtual void SetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) = 0;
   virtual void GetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const = 0;
   virtual Float64 GetGirderConditionFactor(const CSegmentKey& segmentKey) const = 0;
   virtual void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) = 0;
   virtual void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const = 0;
   virtual Float64 GetDeckConditionFactor() const = 0;
   
   virtual void SetSystemFactorFlexure(Float64 sysFactor) = 0;
   virtual Float64 GetSystemFactorFlexure() const = 0;
   
   virtual void SetSystemFactorShear(Float64 sysFactor) = 0;
   virtual Float64 GetSystemFactorShear() const = 0;

   virtual void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC) = 0;
   virtual Float64 GetDeadLoadFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW) = 0;
   virtual Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR) = 0;
   virtual Float64 GetCreepFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH) = 0;
   virtual Float64 GetShrinkageFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetRelaxationFactor(pgsTypes::LimitState ls,Float64 gRE) = 0;
   virtual Float64 GetRelaxationFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetSecondaryEffectsFactor(pgsTypes::LimitState ls,Float64 gPS) = 0;
   virtual Float64 GetSecondaryEffectsFactor(pgsTypes::LimitState ls) const = 0;

   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL) = 0;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) const = 0;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) const = 0;

   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t,bool bLimitStress,Float64 fMax) = 0;
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,bool* pbLimitStress,Float64* pfMax) const = 0;

   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress) = 0;
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType) const = 0;

   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear) = 0;
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType) const = 0;

   // Per last paragraph in MBE 6A.4.4.2.1a, the lane load may be excluded and the 0.75 truck factor taken as 1.0
   // for ADTT < 500 and in the Engineer's judgement it is warranted
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude) = 0;
   virtual bool ExcludeLegalLoadLaneLoading() const = 0;

   virtual void CheckYieldStress(pgsTypes::LoadRatingType ratingType,bool bCheckYieldStress) = 0;
   virtual bool CheckYieldStress(pgsTypes::LoadRatingType ratingType) const = 0;

   // returns fraction of yield stress that reinforcement can be stressed to during
   // a permit load rating evaluation MBE 6A.5.4.2.2b
   virtual void SetYieldStressLimitCoefficient(Float64 x) = 0;
   virtual Float64 GetYieldStressLimitCoefficient() const = 0;

   // Permit type for rating for special/limited crossing permit vehicle
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type) = 0;
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType() const = 0;

   // Gets the Strength/Service Live Load factor for a permit rating type when the load factor is based on 
   // the weight of the vehicle on the bridge (MBE 6A.4.5.4.2a-1)
   // Use these methods if GetLiveLoadFactor(ls,true) returns a value that is less than zero. This means
   // the load factor is computed based on the weight of the vehicle on the bridge.
   virtual Float64 GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig) const = 0;
   virtual Float64 GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType) const = 0;

   // Returns the Strength/Service load factor for reactions at the specified pier.
   // The vehicle index can be INVALID_INDEX which means the live load factor is that for the
   // controlling live load reaction case
   virtual Float64 GetReactionStrengthLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const = 0;
   virtual Float64 GetReactionServiceLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const = 0;
};

/*****************************************************************************
INTERFACE
   IRatingSpecificationEventSink

   Callback interface for rating specification input change events.

DESCRIPTION
   Callback interface for rating specification input change events.
*****************************************************************************/
// {D100AF07-38DA-4baa-8726-F1EE9A52D037}
DEFINE_GUID(IID_IRatingSpecificationEventSink, 
0xd100af07, 0x38da, 0x4baa, 0x87, 0x26, 0xf1, 0xee, 0x9a, 0x52, 0xd0, 0x37);
interface IRatingSpecificationEventSink : IUnknown
{
   virtual HRESULT OnRatingSpecificationChanged() = 0;
};
