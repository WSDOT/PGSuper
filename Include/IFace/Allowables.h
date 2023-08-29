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

#include <PGSuperTypes.h>
#include <PgsExt\Keys.h>
#include <PgsExt\PointOfInterest.h>

#include <psgLib/TensionStressLimit.h>

interface IEAFDisplayUnits;
class pgsSegmentArtifact;

/*****************************************************************************
INTERFACE
IStressCheck

DESCRIPTION
Interface to get specification related information

*****************************************************************************/
// {DFA2B573-DE2F-44D0-B32E-4BAE1DF9CEAF}
DEFINE_GUID(IID_IStressCheck,
   0xdfa2b573, 0xde2f, 0x44d0, 0xb3, 0x2e, 0x4b, 0xae, 0x1d, 0xf9, 0xce, 0xaf);

interface IStressCheck : IUnknown
{
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CGirderKey& girderKey,bool bDesign = false) const = 0;
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CSegmentKey& segmentKey,bool bDesign = false) const = 0;
   virtual std::vector<IntervalIndexType> GetStressCheckIntervals(const CGirderKey& girderKey, bool bDesign = false) const = 0;
};

/*****************************************************************************
INTERFACE
   IAllowableStrandStress

   Interface to get the allowable prestressing strand stresses.

DESCRIPTION
   Interface to get the allowable prestressing strand stresses.
   Different versions of the LRFD have different requirements for when
   strand stresses must be checked. Use the CheckStresXXX methods
   to determine when the strand stresses must be checked.
   Based on LRFD Table 5.9.2.2-1 (pre2017: 5.9.3-1)
*****************************************************************************/
// {82EA97B0-6EB2-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IAllowableStrandStress, 
0x82ea97b0, 0x6eb2, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IAllowableStrandStress : IUnknown
{
   // Returns true if strand stresses are to be checked at jacking
   virtual bool CheckStressAtJacking() const = 0;

   // Returns true if strand stresses are to be check immediately before prestress transfer (release)
   virtual bool CheckStressBeforeXfer() const = 0;

   // Returns true if strand stresses are to be check immediately after prestress transfer (release)
   virtual bool CheckStressAfterXfer() const = 0;

   // Returns true if strand stresses are to be check after all losses have occured
   virtual bool CheckStressAfterLosses() const = 0;

   // Returns the allowable strand stress at jacking
   virtual Float64 GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   // Returns the allowable strand stress immediately prior to prestress transfer (release)
   virtual Float64 GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   // Returns the allowable strand stress immediately after prestress transfer (release)
   virtual Float64 GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   // Returns the allowable strand stress after all losses have occurred
   virtual Float64 GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;
};


/*****************************************************************************
INTERFACE
   IAllowableTendonStress

   Interface to get the allowable tendon stresses.

DESCRIPTION
   Interface to get the allowable tendon stresses. Based on LRFD Table 5.9.2.2-1 (pre2017: 5.9.3-1)
*****************************************************************************/
// {FC5C901A-C65B-4d10-98A8-3B01EEA86044}
DEFINE_GUID(IID_IAllowableTendonStress, 
0xfc5c901a, 0xc65b, 0x4d10, 0x98, 0xa8, 0x3b, 0x1, 0xee, 0xa8, 0x60, 0x44);
interface IAllowableTendonStress : IUnknown
{
   virtual bool CheckTendonStressAtJacking() const = 0;
   virtual bool CheckTendonStressPriorToSeating() const = 0;

   virtual Float64 GetSegmentTendonAllowableAtJacking(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowablePriorToSeating(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableAfterAnchorSet(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableAfterLosses(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableCoefficientAtJacking(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableCoefficientPriorToSeating(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAllowableCoefficientAfterLosses(const CSegmentKey& segmentKey) const = 0;

   virtual Float64 GetGirderTendonAllowableAtJacking(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowablePriorToSeating(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableAfterAnchorSet(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableAfterLosses(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableCoefficientAtJacking(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableCoefficientPriorToSeating(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterAnchorSet(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAllowableCoefficientAfterLosses(const CGirderKey& girderKey) const = 0;
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
   // Allowable stress for design
   virtual Float64 GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const = 0;
   virtual Float64 GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;
   virtual void ReportSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual void ReportSegmentAllowableTensionStress(const pgsPointOfInterest& poi,const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual void ReportClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual void ReportClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;

   // Allowable stress for load rating
   virtual Float64 GetAllowableTensionStress(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const = 0;

   virtual Float64 GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const = 0;
   virtual TensionStressLimit GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;

   // Returns the allowable Compression stress at the specified locations along the girder
   virtual std::vector<Float64> GetGirderAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const = 0;

   // Returns the allowable Compression stress at the specified locations along the deck
   virtual std::vector<Float64> GetDeckAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const = 0;

   // Returns the allowable tensile stress at the specified locations along the girder. If bWithBondedReinforcement is true, the high allowable tension is returned if it is applicable.
   // If bInPrecompressedTensileZone is true, the allowable tensile stress for the precompressed tensile zone in closure joints is returned
   virtual std::vector<Float64> GetGirderAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const = 0;

   // Returns the allowable tensile stress at the specified locations along the girder. If bWithBondedReinforcement is true, the high allowable tension is returned if it is applicable.
   virtual std::vector<Float64> GetDeckAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const = 0;


   // Returns the allowable Compression stress in a girder segment at the specified location
   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   // Returns the allowable Compression stress in a closure joint at the specified location
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   // Returns the allowable Compression stress in the deck at the specified location
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   // Returns the allowable Compression stress in a girder segment at the specified location for a specified concrete strength
   virtual Float64 GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   // Returns the allowable Compression stress in a closure joint at the specified location for a specified concrete strength
   virtual Float64 GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   // Returns the allowable Compression stress in the deck at the specified location for a specified concrete strength
   virtual Float64 GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement) const = 0;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement, bool bInPrecompressedTensileZone) const = 0;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement) const = 0;

   virtual Float64 GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const = 0;
   virtual Float64 GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;
   virtual Float64 GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const = 0;

   virtual Float64 GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;
   virtual Float64 GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;
   virtual Float64 GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   virtual TensionStressLimit GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const = 0;
   virtual TensionStressLimit GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;
   virtual TensionStressLimit GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const = 0;

   // Returns true if the stress check is applicable to this interval, limit state, and stress type
   virtual bool IsStressCheckApplicable(const CSegmentKey& segmentKey, const StressCheckTask& task) const = 0;

   // Returns true if the stress check is applicable to any segment in the girder
   virtual bool IsStressCheckApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const = 0;

   // Returns true if the allowable tension stress in the specified interval has a "with bonded reinforcement"
   // option. If bInPTZ is true, the result is for the precompressed tensile zone, otherwise it is for areas other than the
   // precompressed tensile zone. If bSegment is true the result is for a precast segment and segmentKey is for that segment
   // otherwise it is for a closure joint and segmentKey is the closure key
   virtual bool HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const = 0;

   // returns true if the girder stress checks are to include intermediate, temporary
   // loading conditions
   virtual bool CheckTemporaryStresses() const = 0;

   // returns true if tension stresses due to final dead load are to be evaluated
   virtual bool CheckFinalDeadLoadTensionStress() const = 0;

   virtual Float64 GetAllowableSegmentPrincipalWebTensionStress(const CSegmentKey& segmentKey) const = 0;
   virtual void ReportAllowableSegmentPrincipalWebTensionStress(const CSegmentKey& segmentKey, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual Float64 GetAllowableClosureJointPrincipalWebTensionStress(const CClosureKey& closureKey) const = 0;
   virtual void ReportAllowableClosureJointPrincipalWebTensionStress(const CClosureKey& closureKey, rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual Float64 GetAllowablePrincipalWebTensionStress(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAllowablePrincipalWebTensionStressCoefficient() const = 0;
   virtual Float64 GetPrincipalTensileStressFcThreshold() const = 0;
   virtual Float64 GetPrincipalTensileStressRequiredConcreteStrength(const pgsPointOfInterest& poi, Float64 stress) const = 0;

   virtual Float64 GetAllowablePCIUHPCTensionStressLimitCoefficient() const = 0;
   virtual Float64 GetAllowableUHPCTensionStressLimitCoefficient(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetAllowableUHPCFatigueTensionStressLimitModifier() const = 0;

   virtual Float64 ComputeRequiredConcreteStrength(const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation, Float64 stressDemand, const StressCheckTask& task,bool bWithBondedReinforcement, bool bInPrecompressedTensileZone) const = 0;
   virtual std::_tstring GetAllowableStressParameterName(pgsTypes::StressType stressType, pgsTypes::ConcreteType concreteType) const = 0;
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
   virtual bool CheckMaxDebondedStrands(const CSegmentKey& segmentKey) const = 0; // returns true if Max Debonded Strands is to be checked
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) const = 0;  // % of total
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) const = 0; // % of total in row
   virtual void GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey,StrandIndexType* p10orLess,StrandIndexType* pNS,bool* pbCheckMax,Float64* pMaxFraction) const = 0;
   virtual void    GetMaxDebondLength(const CSegmentKey& segmentKey,Float64* pLen, pgsTypes::DebondLengthControl* pControl) const = 0;

   // Get the criteria for determining the minimum distance between termination of debond sections
   virtual void GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey, Float64* pndb, bool* pbUseMinDistance, Float64* pMinDistance) const = 0;

   // Returns the minimum distance between the termination of debond sections
   virtual Float64 GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey) const = 0;

   // returns true if we are checking for debonding symmetry
   virtual bool CheckDebondingSymmetry(const CSegmentKey& segmentKey) const = 0;

   // returns true if we are checking for adjacent debonded strands
   virtual bool CheckAdjacentDebonding(const CSegmentKey& segmentKey) const = 0;

   // returns true if we are checking for debonded strands within the web width projections
   virtual bool CheckDebondingInWebWidthProjections(const CSegmentKey& segmentKey) const = 0;

   // Returns the true if the exterior strands in the specified row are required to be bonded
   virtual bool IsExteriorStrandBondingRequiredInRow(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, RowIndexType rowIdx) const = 0;
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
   virtual Float64 GetSegmentTendonRadiusOfCurvatureLimit(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonAreaLimit(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentTendonDuctSizeLimit(const CSegmentKey& segmentKey) const = 0;

   virtual Float64 GetGirderTendonRadiusOfCurvatureLimit(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonAreaLimit(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetGirderTendonDuctSizeLimit(const CGirderKey& girderKey) const = 0;

   virtual Float64 GetTendonAreaLimit(pgsTypes::StrandInstallationType installationType) const = 0;

   virtual Float64 GetSegmentDuctDeductionFactor(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetGirderDuctDeductionFactor(const CGirderKey& girderKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx) const = 0;
};

