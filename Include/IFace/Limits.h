///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PsgLib/Keys.h>
#include <PsgLib/PointOfInterest.h>
#include <PsgLib/TensionStressLimit.h>

class IEAFDisplayUnits;
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

/// @brief Interface that provides stress checking tasks and intervals
class IStressCheck
{
public:
   /// @brief Returns stress check tasks for girders. This function calls GetStressCheckTasks for each segment.
   /// @param girderKey 
   /// @param bDesign indicates if the tasks are for design or specification checking
   /// @return 
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CGirderKey& girderKey,bool bDesign = false) const = 0;

   /// @brief Returns stress check tasks for segments
   /// @param segmentKey 
   /// @param bDesign  indicates if the tasks are for design or specification checking. Temporary strand removal task is always included for design.
   /// @return 
   virtual std::vector<StressCheckTask> GetStressCheckTasks(const CSegmentKey& segmentKey,bool bDesign = false) const = 0;

   /// @brief Returns the intervals when stress checks occur
   /// @param girderKey 
   /// @param bDesign  indicates if the tasks are for design or specification checking
   /// @return 
   virtual std::vector<IntervalIndexType> GetStressCheckIntervals(const CGirderKey& girderKey, bool bDesign = false) const = 0;
};

/*****************************************************************************
INTERFACE
   IStrandStressLimit

   Interface to get the stress limits on prestressing strand.

DESCRIPTION
   Interface to get the stress limits on prestressing strand stresses.
   Different versions of the LRFD have different requirements for when
   strand stresses must be checked. Use the CheckStressXXX methods
   to determine when the strand stresses must be checked.
   Based on LRFD Table 5.9.2.2-1 (pre2017: 5.9.3-1)
*****************************************************************************/
// {82EA97B0-6EB2-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IStrandStressLimit, 
0x82ea97b0, 0x6eb2, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);

/// @brief Interface to get the stress limits for prestressing strands.
/// Different versions of the LRFD have different requirements for when
/// strand stresses must be checked.Use the CheckStressXXX methods
/// to determine when the strand stresses must be checked.
/// Based on LRFD Table 5.9.2.2 - 1 (pre2017: 5.9.3 - 1)
class IStrandStressLimit
{
public:
   /// @brief Returns true if strand stresses are to be checked at jacking
   virtual bool CheckStrandStressAtJacking() const = 0;

   /// @brief Returns true if strand stresses are to be check immediately before prestress transfer (release)
   virtual bool CheckStrandStressBeforeXfer() const = 0;

   /// @brief Returns true if strand stresses are to be check immediately after prestress transfer (release)
   virtual bool CheckStrandStressAfterXfer() const = 0;

   /// @brief Returns true if strand stresses are to be check after all losses have occurred
   virtual bool CheckStrandStressAfterLosses() const = 0;

   /// @brief Returns the strand stress limit at jacking
   virtual Float64 GetStrandStressLimitAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   /// @brief Returns the strand stress limit immediately prior to prestress transfer (release)
   virtual Float64 GetStrandStressLimitBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   /// @brief Returns the strand stress limit immediately after prestress transfer (release)
   virtual Float64 GetStrandStressLimitAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;

   /// @brief Returns the strand stress limit after all losses have occurred
   virtual Float64 GetStrandStressLimitAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;
};


/*****************************************************************************
INTERFACE
   ITendonStressLimit

   Interface to get the tendon stress limit.

DESCRIPTION
   Interface to get the tendon stress limit. Based on LRFD Table 5.9.2.2-1 (pre2017: 5.9.3-1)
*****************************************************************************/
// {FC5C901A-C65B-4d10-98A8-3B01EEA86044}
DEFINE_GUID(IID_ITendonStressLimit, 
0xfc5c901a, 0xc65b, 0x4d10, 0x98, 0xa8, 0x3b, 0x1, 0xee, 0xa8, 0x60, 0x44);

/// @brief Interface to get tendon stress limits. Based on LRFD Table 5.9.2.2-1 (pre2017: Table 5.9.3-1).
/// Segment Tendons are PT tendons installed at the precasting plant. Girder Tendons are PT tendons
/// installed on site and connect multiple segments.
class ITendonStressLimit
{
public:
   /// @brief Returns true if tendon stresses are limited at jacking
   virtual bool CheckTendonStressAtJacking() const = 0;

   /// @brief Returns true if tendon stresses are limited prior to seating
   virtual bool CheckTendonStressPriorToSeating() const = 0;

   /// @brief Returns the segment tendon stress limit at jacking. Only valid if CheckTendonStressAtJacking returns true.
   virtual Float64 GetSegmentTendonStressLimitAtJacking(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the segment tendon stress limit prior to seating. Only valid if CheckTendonStressPriorToSeating returns true.
   virtual Float64 GetSegmentTendonStressLimitPriorToSeating(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the segment tendon stress limit after anchor set at the anchorage
   virtual Float64 GetSegmentTendonStressLimitAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the segment tendon stress limit after anchor set at locations other than the anchorage
   virtual Float64 GetSegmentTendonStressLimitAfterAnchorSet(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the segment tendon stress limit after losses
   virtual Float64 GetSegmentTendonStressLimitAfterLosses(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the segment tendon stress limit at jacking. Only valid if CheckTendonStressAtJacking returns true.
   virtual Float64 GetSegmentTendonStressLimitCoefficientAtJacking(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the segment tendon stress limit prior to seating. Only valid if CheckTendonStressPriorToSeating returns true.
   virtual Float64 GetSegmentTendonStressLimitCoefficientPriorToSeating(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the segment tendon stress limit after anchor set at the anchorage
   virtual Float64 GetSegmentTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the segment tendon stress limit after anchor set at locations other than the anchorage
   virtual Float64 GetSegmentTendonStressLimitCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpy for the segment tendon stress limit after losses
   virtual Float64 GetSegmentTendonStressLimitCoefficientAfterLosses(const CSegmentKey& segmentKey) const = 0;

   /// @brief Returns the girder tendon stress limit at jacking. Only valid if CheckTendonStressAtJacking returns true.
   virtual Float64 GetGirderTendonStressLimitAtJacking(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the girder tendon stress limit prior to seating. Only valid if CheckTendonStressPriorToSeating returns true.
   virtual Float64 GetGirderTendonStressLimitPriorToSeating(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the girder tendon stress limit after anchor set at the anchorage
   virtual Float64 GetGirderTendonStressLimitAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the girder tendon stress limit after anchor set at locations other than the anchorage
   virtual Float64 GetGirderTendonStressLimitAfterAnchorSet(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the girder tendon stress limit after losses
   virtual Float64 GetGirderTendonStressLimitAfterLosses(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the girder tendon stress limit at jacking. Only valid if CheckTendonStressAtJacking returns true.
   virtual Float64 GetGirderTendonStressLimitCoefficientAtJacking(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the girder tendon stress limit prior to seating. Only valid if CheckTendonStressPriorToSeating returns true.
   virtual Float64 GetGirderTendonStressLimitCoefficientPriorToSeating(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the girder tendon stress limit after anchor set at the anchorage
   virtual Float64 GetGirderTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpu for the girder tendon stress limit after anchor set at locations other than the anchorage
   virtual Float64 GetGirderTendonStressLimitCoefficientAfterAnchorSet(const CGirderKey& girderKey) const = 0;
   /// @brief Returns the coefficient multiplied with fpy for the girder tendon stress limit after losses
   virtual Float64 GetGirderTendonStressLimitCoefficientAfterLosses(const CGirderKey& girderKey) const = 0;
};

/*****************************************************************************
INTERFACE
   IConcreteStressLimits

DESCRIPTION
   Interface to get concrete stresses limits.

   Concrete stress limits vary by position. Some agencies limit the
   tension stress in the top of the girder near the supports. This will
   be supported in the future. For now, stress limits are constant
   over the length of a segment so dummy POI can be passed into this function
   so long as the segment key is valid.
*****************************************************************************/
// {8D24A46E-7DAD-11d2-8857-006097C68A9C}
DEFINE_GUID(IID_IConcreteStressLimits, 
0x8d24a46e, 0x7dad, 0x11d2, 0x88, 0x57, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);

/// @brief Interface to get concrete stress limits.
/// In general, concrete stress limits can vary by position. Some agencies limit the
/// tension stress in the top of the girder near the supports. This will
/// be supported in the future. For now, stress limits are constant
/// over the length of a segment so dummy POI can be passed into these functions
/// so long as the segment key is valid. Though it is recommended to use actual POI
/// if available to making implementation of per POI stress limits easier in the future.
class IConcreteStressLimits
{
public:
   /// @brief Returns the concrete compression stress limit
   /// @param poi location where the stress limit is requested
   /// @param stressLocation location in the girder section
   /// @param task stress checking task
   /// @return The stress limit
   virtual Float64 GetConcreteCompressionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const = 0;

   /// @brief Returns the concrete tension stress limit
   /// @param poi location where the stress limit is requested
   /// @param stressLocation location in the girder section
   /// @param task stress checking task
   /// @param bWithBondedReinforcement if true, there is adequate bonded reinforcement at the section to use the alternative stress limit
   /// @param bInPrecompressedTensileZone if true, the poi and stressLocation are in the precompressed tensile zone
   /// @return The stress limit
   virtual Float64 GetConcreteTensionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;

   /// @brief Reports the segment concrete compression stress limit
   virtual void ReportSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;

   /// @brief Reports the segment concrete tension stress limit
   virtual void ReportSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi,const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;

   /// @brief Reports the closure joint concrete compression stress limit
   virtual void ReportClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;

   /// @brief Reports the closure joint concrete tension stress limit
   virtual void ReportClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;

   /// @brief Returns the concrete tension stress limit for load rating
   /// @param ratingType Type of load rating analysis
   /// @param poi Location where stress limit is requested. This method determine if poi is in a precast segment or closure joint
   /// @param stressLocation location in the girder section
   /// @return The stress limit
   virtual Float64 GetConcreteTensionStressLimit(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const = 0;

   /// @brief Returns the coefficient multiplied with concrete strength for concrete compression stress limit
   /// @param poi Location where stress limit is requested. This method determine if poi is in a precast segment or closure joint
   /// @param stressLocation location in the girder section
   /// @param task stress checking task
   /// @return The compression stress limit coefficient
   virtual Float64 GetConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const = 0;

   /// @brief Returns the parameters for determining the concrete tension stress limit
   /// @param poi Location where stress limit is requested. This method determine if poi is in a precast segment or closure joint
   /// @param stressLocation location in the girder section
   /// @param task stress checking task
   /// @param bWithBondedReinforcement if true, there is adequate bonded reinforcement at the section to use the alternative stress limit
   /// @param bInPrecompressedTensileZone if true, the poi and stressLocation are in the precompressed tensile zone
   /// @return The tension stress limit parameters
   virtual TensionStressLimit GetConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;

   /// @brief Returns the compression stress limit at the specified locations along the girder
   virtual std::vector<Float64> GetGirderConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const = 0;

   /// @brief Returns the compression stress limit at the specified locations along the deck
   virtual std::vector<Float64> GetDeckConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const = 0;

   /// @brief Returns the tensile stress limit at the specified locations along the girder. If bWithBondedReinforcement is true, the alternative tension stress limit is returned if it is applicable.
   /// If bInPrecompressedTensileZone is true, the alternative tensile stress limit for the precompressed tensile zone in closure joints is returned
   virtual std::vector<Float64> GetGirderConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const = 0;

   /// @brief Returns the allowable tensile stress at the specified locations along the girder. If bWithBondedReinforcement is true, the high allowable tension is returned if it is applicable.
   virtual std::vector<Float64> GetDeckConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const = 0;


   /// @brief Returns the allowable Compression stress in a girder segment at the specified location
   virtual Float64 GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   /// @brief Returns the compression stress limit in a closure joint at the specified location
   virtual Float64 GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   /// @brief Returns the compression stress limit in the deck at the specified location
   virtual Float64 GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   /// @brief Returns the compression stress limit in a girder segment at the specified location for a specified concrete strength
   virtual Float64 GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   /// @brief Returns the compression stress limit in a closure joint at the specified location for a specified concrete strength
   virtual Float64 GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   /// @brief Returns the compression stress limit in the deck at the specified location for a specified concrete strength
   virtual Float64 GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const = 0;

   /// @brief Returns the tension stress limit in a girder segment at the specified location
   virtual Float64 GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement) const = 0;
   /// @brief Returns the tension stress limit in a closure joint at the specified location
   virtual Float64 GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement, bool bInPrecompressedTensileZone) const = 0;
   /// @brief Returns the tension stress limit in the deck at the specified location
   virtual Float64 GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, bool bWithBondedReinforcement) const = 0;

   /// @brief Returns the tension stress limit in a girder segment at the specified location for the specified concrete strength
   virtual Float64 GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const = 0;
   /// @brief Returns the tension stress limit in a closure joint at the specified location for the specified concrete strength
   virtual Float64 GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;
   /// @brief Returns the tension stress limit in the deck at the specified location for the specified concrete strength
   virtual Float64 GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const = 0;

   /// @brief Returns the compression stress limit coefficient for a girder segment at the specified location
   virtual Float64 GetSegmentConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;
   /// @brief Returns the compression stress limit coefficient for a closure joint at the specified location
   virtual Float64 GetClosureJointConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;
   /// @brief Returns the compression stress limit coefficient for the deck at the specified location
   virtual Float64 GetDeckConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const = 0;

   /// @brief Returns the tension stress limit parameters for a girder segment at the specified location
   virtual TensionStressLimit GetSegmentConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const = 0;
   /// @brief Returns the tension stress limit parameters for a closure joint at the specified location
   virtual TensionStressLimit GetClosureJointConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const = 0;
   /// @brief Returns the tension stress limit parameters for the deck at the specified location
   virtual TensionStressLimit GetDeckConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const = 0;

   /// @brief Returns true if the stress check is applicable for the segment and task
   virtual bool IsConcreteStressLimitApplicable(const CSegmentKey& segmentKey, const StressCheckTask& task) const = 0;

   /// @brief Returns true if the stress check is applicable to any segment in the girder
   virtual bool IsConcreteStressLimitApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const = 0;

   /// @brief Returns true if the allowable tension stress in the specified interval has a "with bonded reinforcement"
   /// option. If bInPTZ is true, the result is for the precompressed tensile zone, otherwise it is for areas other than the
   /// precompressed tensile zone. If bSegment is true the result is for a precast segment and segmentKey is for that segment
   /// otherwise it is for a closure joint and segmentKey is the closure key
   virtual bool HasConcreteTensionStressLimitWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const = 0;

   /// @brief Max cover to always be used in resisting tensile forces to compute higher tensile limit. Note that HasConcreteTensionStressLimitWithRebarOption should
   /// be called prior to insure that the cover value can be used.
   virtual Float64 GetMaxCoverToUseHigherTensionStressLimit() const = 0;

   /// @brief Returns true if the girder stress checks are to include intermediate, temporary loading conditions
   virtual bool CheckTemporaryStresses() const = 0;

   /// @brief Returns true if tension stresses due to final dead load are to be evaluated
   virtual bool CheckFinalDeadLoadTensionStress() const = 0;

   /// @brief Returns the segment concrete tension stress limit for principal tension stress in the web
   virtual Float64 GetSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey) const = 0;
   /// @brief Reports the segment concrete tension stress limit for principal tension stress in the web
   virtual void ReportSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;
   /// @brief Returns the closure joint concrete tension stress limit for principal tension stress in the web
   virtual Float64 GetClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey) const = 0;
   /// @brief Reports the closure joint concrete tension stress limit for principal tension stress in the web
   virtual void ReportClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;
   /// @brief Returns the concrete principal tension stress limit at a poi.
   virtual Float64 GetConcreteWebPrincipalTensionStressLimit(const pgsPointOfInterest& poi) const = 0;
   /// @brief Returns the coefficient multiplied with sqrt(f'c) for the concrete principal tension stress limit
   virtual Float64 GetConcreteWebPrincipalTensionStressLimitCoefficient() const = 0;
   /// @brief Returns the f'c threshold when the principal tension stress limit is applicable. A threshold of f'c = 0.0 means the stress limit is always applicable.
   virtual Float64 GetPrincipalTensileStressFcThreshold() const = 0;
   /// @brief Returns the concrete strength required to satisfy the tension stress limit
   virtual Float64 GetPrincipalTensileStressRequiredConcreteStrength(const pgsPointOfInterest& poi, Float64 stress) const = 0;

   /// @brief Returns the coefficient multiplied with PCI UHPC concrete cracking strength for the tension stress limit
   virtual Float64 GetPCIUHPCTensionStressLimitCoefficient() const = 0;
   /// @brief Returns the coefficient multiplied with UHPC ft,cr for the tension stress limit
   virtual Float64 GetUHPCTensionStressLimitCoefficient(const CSegmentKey& segmentKey) const = 0;
   /// @brief Returns the tension stress modifier factor for tension stress in the Fatigue I limit state
   virtual Float64 GetUHPCFatigueTensionStressLimitModifier() const = 0;

   /// @brief the required concrete strength to satisfy the concrete stress limit
   virtual Float64 ComputeRequiredConcreteStrength(const pgsPointOfInterest& poi, pgsTypes::StressLocation stressLocation, Float64 stressDemand, const StressCheckTask& task,bool bWithBondedReinforcement, bool bInPrecompressedTensileZone) const = 0;

   /// @brief Returns the name used for concrete stress limit names (concrete strength or cracking strength)
   virtual std::_tstring GetConcreteStressLimitParameterName(pgsTypes::StressType stressType, pgsTypes::ConcreteType concreteType) const = 0;

   /// @brief Returns the concrete strength increase factor for slow curing concretes per LRFD 5.12.3.2.5
   virtual Float64 GetSlowCuringConcreteStrengthFactor(pgsTypes::LimitState limitState, pgsTypes::ConcreteType type, Float64 age) const = 0;
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
class IDebondLimits
{
public:
   virtual bool CheckMaxDebondedStrands(const CSegmentKey& segmentKey) const = 0; // returns true if Max Debonded Strands is to be checked
   virtual Float64 GetMaxDebondedStrands(const CSegmentKey& segmentKey) const = 0;  // % of total
   virtual Float64 GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) const = 0; // % of total in row
   virtual void GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey,StrandIndexType* p10orLess,StrandIndexType* pn10orMore,StrandIndexType* pn10orMore_07Strand,bool* pbCheckMax,Float64* pMaxFraction) const = 0;
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
class IDuctLimits
{
public:
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

