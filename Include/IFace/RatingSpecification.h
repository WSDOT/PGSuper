///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
   // returns true if a load rating is to be performed along with the analysis
   virtual bool AlwaysLoadRate() const = 0;

   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const = 0;
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable) = 0;

   virtual std::_tstring GetRatingSpecification() const = 0;
   virtual void SetRatingSpecification(const std::_tstring& spec) = 0;

   virtual void SetADTT(Int16 adtt) = 0;
   virtual Int16 GetADTT() const = 0; // < 0 = Unknown

   virtual void IncludePedestrianLiveLoad(bool bInclude) = 0;
   virtual bool IncludePedestrianLiveLoad() const = 0;

   virtual void SetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) = 0;
   virtual void GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const = 0;
   virtual Float64 GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx) const = 0;
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

   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL) = 0;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) const = 0;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) const = 0;

   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t) = 0;
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType) const = 0;
   virtual Float64 GetAllowableTension(pgsTypes::LoadRatingType ratingType,SpanIndexType spanIdx,GirderIndexType gdrIdx) const = 0;

   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress) = 0;
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType) const = 0;

   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear) = 0;
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType) const = 0;

   // Per last paragraph in MBE 6A.4.4.2.1a, the lane load may be excluded and the 0.75 truck factor taken as 1.0
   // for ADTT < 500 and in the Engineer's judgement it is warranted
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude) = 0;
   virtual bool ExcludeLegalLoadLaneLoading() const = 0;

   // returns fraction of yield stress that reinforcement can be stressed to during
   // a permit load rating evaluation MBE 6A.5.4.2.2b
   virtual void SetYieldStressLimitCoefficient(Float64 x) = 0;
   virtual Float64 GetYieldStressLimitCoefficient() const = 0;

   // Permit type for rating for special/limited crossing permit vehicle
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type) = 0;
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType() const = 0;
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
