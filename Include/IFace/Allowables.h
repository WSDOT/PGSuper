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
#include <PgsExt\Keys.h>

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
   virtual Float64 GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;

   virtual Float64 GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;
   virtual void GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) = 0;

   // Returns the allowable Compression stress at the specified locations along the girder
   virtual std::vector<Float64> GetGirderAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the allowable Compression stress at the specified locations along the deck
   virtual std::vector<Float64> GetDeckAllowableCompressionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the allowable tensile stress at the specified locations along the girder. If bWithBondedReinforcement is true, the high allowable tension is returned if it is applicable.
   // If bInPrecompressedTensileZone is true, the allowable tensile stress for the precompressed tensile zone in closure joints is returned
   virtual std::vector<Float64> GetGirderAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) = 0;

   // Returns the allowable tensile stress at the specified locations along the girder. If bWithBondedReinforcement is true, the high allowable tension is returned if it is applicable.
   virtual std::vector<Float64> GetDeckAllowableTensionStress(const std::vector<pgsPointOfInterest>& vPoi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondededReinforcement) = 0;


   // Returns the allowable Compression stress in a girder segment at the specified location
   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the allowable Compression stress in a closure joint at the specified location
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the allowable Compression stress in the deck at the specified location
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   // Returns the allowable Compression stress in a girder segment at the specified location for a specified concrete strength
   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) = 0;

   // Returns the allowable Compression stress in a closure joint at the specified location for a specified concrete strength
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) = 0;

   // Returns the allowable Compression stress in the deck at the specified location for a specified concrete strength
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc) = 0;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement) = 0;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement) = 0;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement) = 0;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) = 0;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,Float64 fc,bool bWithBondedReinforcement) = 0;

   virtual Float64 GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls) = 0;

   virtual void GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) = 0;
   virtual void GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) = 0;
   virtual void GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) = 0;

   // Returns a vector of limit states that are to be spec-checked in for service stresses
   virtual std::vector<pgsTypes::LimitState> GetStressCheckLimitStates() = 0;

   // Returns true if the stress check is applicable to this interval, limit state, and stress type
   virtual bool IsStressCheckApplicable(const CGirderKey& girderKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::StressType stressType) = 0;

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


/*****************************************************************************
INTERFACE
   IDuctLimits

   Interface to access duct limits criteria

DESCRIPTION
   Interface to access debond limits criteria
*****************************************************************************/
// {C99249A5-87C6-4fdf-AC1A-502E50FAEABC}
DEFINE_GUID(IID_IDuctLimits, 
0xc99249a5, 0x87c6, 0x4fdf, 0xac, 0x1a, 0x50, 0x2e, 0x50, 0xfa, 0xea, 0xbc);
interface IDuctLimits : IUnknown
{
   virtual Float64 GetRadiusOfCurvatureLimit(const CGirderKey& girderKey) = 0;
   virtual Float64 GetTendonAreaLimit(const CGirderKey& girderKey) = 0;
   virtual Float64 GetDuctSizeLimit(const CGirderKey& girderKey) = 0;
};

