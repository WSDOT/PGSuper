///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <PGSuperTypes.h>
#include <PgsExt\SegmentKey.h>

class pgsPointOfInterest;

/*****************************************************************************
INTERFACE
   IAllowableStrandStress

   Interface to get the allowable prestressing strand stresses.

DESCRIPTION
   Interface to get the allowable prestressing strand stresses.
   Different versions of the LRFD have different requirements for when
   strand stresses must be checked. Use the CheckStresXXX methods
   to determine when the strand stresses must be checked.
   Based on LRFD Table 5.9.3-1
*****************************************************************************/
// {82EA97B0-6EB2-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IAllowableStrandStress, 
0x82ea97b0, 0x6eb2, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IAllowableStrandStress : IUnknown
{
   // Returns true if strand stresses are to be checked at jacking
   virtual bool CheckStressAtJacking() = 0;

   // Returns true if strand stresses are to be check immediately before prestress transfer (release)
   virtual bool CheckStressBeforeXfer() = 0;

   // Returns true if strand stresses are to be check immediately after prestress transfer (release)
   virtual bool CheckStressAfterXfer() = 0;

   // Returns true if strand stresses are to be check after all losses have occured
   virtual bool CheckStressAfterLosses() = 0;

   // Returns the allowable strand stress at jacking
   virtual Float64 GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;

   // Returns the allowable strand stress immediately prior to prestress transfer (release)
   virtual Float64 GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;

   // Returns the allowable strand stress immediately after prestress transfer (release)
   virtual Float64 GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;

   // Returns the allowable strand stress after all losses have occured
   virtual Float64 GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) = 0;
};


/*****************************************************************************
INTERFACE
   IAllowableTendonStress

   Interface to get the allowable tendon stresses.

DESCRIPTION
   Interface to get the allowable tendon stresses. Based on LRFD Table 5.9.3-1
*****************************************************************************/
// {FC5C901A-C65B-4d10-98A8-3B01EEA86044}
DEFINE_GUID(IID_IAllowableTendonStress, 
0xfc5c901a, 0xc65b, 0x4d10, 0x98, 0xa8, 0x3b, 0x1, 0xee, 0xa8, 0x60, 0x44);
interface IAllowableTendonStress : IUnknown
{
   virtual bool CheckTendonStressAtJacking() = 0;
   virtual bool CheckTendonStressPriorToSeating() = 0;
   virtual Float64 GetAllowableAtJacking(const CGirderKey& girderKey) = 0;
   virtual Float64 GetAllowablePriorToSeating(const CGirderKey& girderKey) = 0;
   virtual Float64 GetAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) = 0;
   virtual Float64 GetAllowableAfterAnchorSet(const CGirderKey& girderKey) = 0;
   virtual Float64 GetAllowableAfterLosses(const CGirderKey& girderKey) = 0;
};

/*****************************************************************************
INTERFACE
   IAllowableConcreteStress

DESCRIPTION
   Interface to allowable concrete stresses.

   Allowable stresses are allowed to vary by position. Some agencies limit the
   tension stress in the top of the girder near the supports. This will
   be supported in the future. For now, allowable stresses are constant
   over the length of a segment so dummy POI can be passed into this function
   so long as the segment key is valid.
*****************************************************************************/
// {8D24A46E-7DAD-11d2-8857-006097C68A9C}
DEFINE_GUID(IID_IAllowableConcreteStress, 
0x8d24a46e, 0x7dad, 0x11d2, 0x88, 0x57, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IAllowableConcreteStress : IUnknown
{
   // Returns the allowable concrete stress at a point of interest. 
   // If bWithBondedReinforcement is true, the allowable tensile stress for the "with bonded reinforcement" 
   // case is returned. bWithBondedReinforcement is ignored if the stress type is pgsTypes::Compression 
   // and intervals that occur after shipping. If bInPrecompressedTensileZone is true and the stress type
   // is pgsTypes::Tension, the allowable stress for the precompressed tensile zone is returned, otherwise
   // the allowable for locations other than the precompressed tensile zone is returned.
   // The stressLocation parameter is used to discriminate between requests for girder allowables and requests for deck allowables
   virtual Float64 GetAllowableStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType type,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;
   virtual std::vector<Float64> GetAllowableStress(const std::vector<pgsPointOfInterest>& vPoi, pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType type,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;
   virtual Float64 GetAllowableStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;

   // Returns the coefficient for allowable compressive stress (x*fc)
   virtual Float64 GetAllowableCompressiveStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the coefficient for allowable tension stress (x*sqrt(fc)), a boolean value indicating if the allowable tension stress has a maximum value
   // and the maxiumum value
   virtual void GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) = 0;

   // Returns a vector of limit states that are to be spec-checked in for service stresses
   virtual std::vector<pgsTypes::LimitState> GetStressCheckLimitStates() = 0;

   // Returns true if the stress check is applicable to this interval, limit state, and stress type
   virtual bool IsStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType) = 0;

   // Returns true if the the allowable tension stress in the specified interval has a "with bonded reinforcement"
   // option. If bInPTZ is true, the result is for the precompressed tensile zone, otherwise it is for areas other than the
   // precompressed tensile zone. If bSegment is true the result is for a precast segment and segmentKey is for that segment
   // otherwise it is for a closure joint and segmentKey is the closure key
   virtual bool HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) = 0;
};


/*****************************************************************************
INTERFACE
   IDebondLimits

   Interface to access debond limits criteria

DESCRIPTION
   Interface to access debond limits criteria
*****************************************************************************/
// {34C607AB-62D4-43a6-AB8A-6CC66BC8C932}
DEFINE_GUID(IID_IDebondLimits, 
0x34c607ab, 0x62d4, 0x43a6, 0xab, 0x8a, 0x6c, 0xc6, 0x6b, 0xc8, 0xc9, 0x32);
interface IDebondLimits : IUnknown
{
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) = 0;  // % of total
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) = 0; // % of total in row
   virtual Float64 GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey) = 0; // % of total debonded
   virtual StrandIndexType GetMaxNumDebondedStrandsPerSection(const CSegmentKey& segmentKey) = 0; 
   virtual void    GetMaxDebondLength(const CSegmentKey& segmentKey,Float64* pLen, pgsTypes::DebondLengthControl* pControl) = 0; 
   virtual Float64 GetMinDebondSectionDistance(const CSegmentKey& segmentKey) = 0; 
};

