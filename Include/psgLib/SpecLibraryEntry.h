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

#ifndef INCLUDED_PSGLIB_SPECLIBRARYENTRY_H_
#define INCLUDED_PSGLIB_SPECLIBRARYENTRY_H_

// SYSTEM INCLUDES
//
#include <PGSuperTypes.h>

// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <psgLib\OldHaulTruck.h>

#include <System\SubjectT.h>

#include <Lrfd\VersionMgr.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsLibraryEntryDifferenceItem;
class CSpecMainSheet;
class SpecLibraryEntry;
class SpecLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>;

#define LOSSES_AASHTO_REFINED       0
#define LOSSES_AASHTO_LUMPSUM       1
#define LOSSES_WSDOT_LUMPSUM        4 // same as PPR = 1.0 in aashto eqn's
#define LOSSES_AASHTO_LUMPSUM_2005  5 // 2005 AASHTO code
#define LOSSES_AASHTO_REFINED_2005  6 // 2005 AASHTO code
#define LOSSES_WSDOT_LUMPSUM_2005   7 // 2005 AASHTO, WSDOT (includes initial relaxation loss)
#define LOSSES_WSDOT_REFINED_2005   8 // 2005 AASHTO, WSDOT (includes initial relaxation loss)
#define LOSSES_WSDOT_REFINED        9
#define LOSSES_TXDOT_REFINED_2004   10 // TxDOT's May, 09 decision is to use refined losses from AASHTO 2004
#define LOSSES_TXDOT_REFINED_2013   11 // TxDOT's Method based on Report No. FHWA/TX-12/0-6374-2
#define LOSSES_TIME_STEP            12 // Losses are computed with a time-step method

#define CSS_AT_JACKING       0
#define CSS_BEFORE_TRANSFER  1
#define CSS_AFTER_TRANSFER   2
#define CSS_AFTER_ALL_LOSSES 3
#define CSS_PRIOR_TO_SEATING 1
#define CSS_ANCHORAGES_AFTER_SEATING 2
#define CSS_ELSEWHERE_AFTER_SEATING 4

#define STRESS_REL 0
#define LOW_RELAX  1

// constants for relaxation loss method for LRFD 2005, refined method
#define RLM_SIMPLIFIED  0
#define RLM_REFINED     1
#define RLM_LUMPSUM     2

// constants for time dependent models
#define TDM_AASHTO    0
#define TDM_ACI209    1
#define TDM_CEBFIP    2

// hold down force type
#define HOLD_DOWN_TOTAL 0
#define HOLD_DOWN_PER_STRAND 1

// MISCELLANEOUS
//

#define SPEC_PAGE_DESCRIPTION 0
#define SPEC_PAGE_DESIGN      1
#define SPEC_PAGE_SEGMENT     2
#define SPEC_PAGE_CLOSURE     3
#define SPEC_PAGE_STRANDS     4
#define SPEC_PAGE_LIFTING     5
#define SPEC_PAGE_HAULING     6
#define SPEC_PAGE_LOADS       7
#define SPEC_PAGE_MOMENT      8
#define SPEC_PAGE_SHEAR       9
#define SPEC_PAGE_CREEP      10
#define SPEC_PAGE_LOSSES     11
#define SPEC_PAGE_LIMITS     12


/*****************************************************************************
CLASS 
   SpecLibraryEntryObserver

   A pure virtual entry class for observing Specification entries.


DESCRIPTION
   This class may be used to describe observe Specification entries in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS SpecLibraryEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(SpecLibraryEntry& subject, Int32 hint)=0;
};


/*****************************************************************************
CLASS 
   SpecLibraryEntry

   Library entry class for a parameterized specification


DESCRIPTION
   This class encapsulates all specification information required for
   prestressed girder design

LOG
   rdp : 09.17.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS SpecLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>
{
   // the dialog is our friend.
   friend CSpecMainSheet;
public:

   SpecLibraryEntry();
   SpecLibraryEntry(const SpecLibraryEntry& rOther);
   virtual ~SpecLibraryEntry();

   SpecLibraryEntry& operator = (const SpecLibraryEntry& rOther);

   //////////////////////////////////////
   // General
   //////////////////////////////////////

   // Causes the editing dialog to be displayed. if allowEditing is false
   // the dialog is opened in a read-only mode
   virtual bool Edit(bool allowEditing,int nPage=0);

   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const SpecLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const SpecLibraryEntry& rOther,bool bConsiderName=false) const;

   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //////////////////////////////////////
   //
   // General Specification Properties
   //
   //////////////////////////////////////

   pgsTypes::AnalysisType GetAnalysisType() const;

   void UseCurrentSpecification(bool bUseCurrent);
   bool UseCurrentSpecification() const;

   // Set/Get specification type we are based on
   void SetSpecificationType(lrfdVersionMgr::Version type);
   lrfdVersionMgr::Version GetSpecificationType() const;

   // Set/Get specification Units we are based on
   void SetSpecificationUnits(lrfdVersionMgr::Units Units);
   lrfdVersionMgr::Units GetSpecificationUnits() const;

   // Set/Get string to describe specification
   void SetDescription(LPCTSTR name);
   std::_tstring GetDescription(bool bApplySymbolSubstitution=true) const;

   // Set/Get the method of computing section properties
   void SetSectionPropertyMode(pgsTypes::SectionPropertyMode mode);
   pgsTypes::SectionPropertyMode GetSectionPropertyMode() const;

   // Set/Get the method for computing effective flange width
   void SetEffectiveFlangeWidthMethod(pgsTypes::EffectiveFlangeWidthMethod efwMethod);
   pgsTypes::EffectiveFlangeWidthMethod GetEffectiveFlangeWidthMethod() const;

   //////////////////////////////////////
   //
   // Design and Spec Checking
   //
   //////////////////////////////////////

   // Set/Get maximum strand slope for 0.5", 0.6", and 0.6" strands. If bool value is false,
   // then slope values do not need to be checked and slope values are 
   // undefined.
   void GetMaxStrandSlope(bool* doCheck, bool* doDesign, Float64* slope05, Float64* slope06,Float64* slope07) const;
   void SetMaxStrandSlope(bool doCheck, bool doDesign, Float64 slope05=0.0, Float64 slope06=0.0,Float64 slope07=0.0);

   //  Set/Get maximum allowable force to hold down strand bundles at harp point.
   //  If doCheck is false, then hold down forces do not need to be 
   //  checked and hold down force value is undefined.
   void GetHoldDownForce(bool* doCheck, bool* doDesign, int* holdDownForceType,Float64* force,Float64* pFriction) const;
   void SetHoldDownForce(bool doCheck, bool doDesign, int holdDownForceType=HOLD_DOWN_TOTAL,Float64 force=0.0,Float64 friction=0.0);

   // Set/Get the maximum girder weight for plant handling
   void GetPlantHandlingWeightLimit(bool* pbDoCheck, Float64* pLimit) const;
   void SetPlantHandlingWeightLimit(bool bDoCheck, Float64 limit);

   // Splitting zone length h/n (h/4 or h/5) per LRFD 5.9.4.4.1 (pre2017: 5.10.10.1)
   void SetSplittingZoneLengthFactor(Float64 n);
   Float64 GetSplittingZoneLengthFactor() const;

   // Set/Get parameter for splitting reinforcement for UHPC
   void SetUHPCStrengthAtFirstCrack(Float64 f1);
   Float64 GetUHPCStrengthAtFirstCrack() const;

   // Get/Set the parameter that determines if the slab offset ("A" Dimension) and finished elevations
   // are checked
   void EnableSlabOffsetCheck(bool enable);
   bool IsSlabOffsetCheckEnabled() const;

   // Get/Set the parameter that determines if the slab offset ("A" Dimension)
   // is designed
   void EnableSlabOffsetDesign(bool enable);
   bool IsSlabOffsetDesignEnabled() const;

   // Enable check for lifting
   void EnableLiftingCheck(bool enable);
   bool IsLiftingAnalysisEnabled() const;

   // Enable design for lifting
   void EnableLiftingDesign(bool enable);
   bool IsLiftingDesignEnabled() const;

   // Enable check for hauling
   void EnableHaulingCheck(bool enable);
   bool IsHaulingAnalysisEnabled() const;

   // Enable design for hauling
   void EnableHaulingDesign(bool enable);
   bool IsHaulingDesignEnabled() const;

   // Enable check for splitting resistance 5.9.4.4.1 (pre2017: 5.10.10.1)
   void EnableSplittingCheck(bool enable);
   bool IsSplittingCheckEnabled() const;

   // Enable design for splitting resistance 5.9.4.4.1 (pre2017: 5.10.10.1)
   void EnableSplittingDesign(bool enable);
   bool IsSplittingDesignEnabled() const;

   // Enable check for adequate confinement reinforcement 5.9.4.4.2 (pre2017: 5.10.10.2)
   void EnableConfinementCheck(bool enable);
   bool IsConfinementCheckEnabled() const;

   // Enable design for adequate confinement reinforcement 5.9.4.4.2 (pre2017: 5.10.10.2)
   void EnableConfinementDesign(bool enable);
   bool IsConfinementDesignEnabled() const;

   // Set/Get method for filling the strand pattern during design
   void SetDesignStrandFillType(arDesignStrandFillType type);
   arDesignStrandFillType GetDesignStrandFillType() const;

   // Determine if we want to evaluate deflection due to live load ala LRFD 2.5.2.5.2
   bool GetDoEvaluateLLDeflection() const;
   void SetDoEvaluateLLDeflection(bool doit);

   // Set/Get span deflection limit criteria. Limit is Span / value.
   Float64 GetLLDeflectionLimit() const;
   void SetLLDeflectionLimit(Float64 limit);

   // Set/Get the minimum location for the lift point
   void SetMininumLiftingPointLocation(Float64 x); // < 0 means use Hg
   Float64 GetMininumLiftingPointLocation() const;

   // Set/Get the accuracy of the lift point placement during automated design
   void SetLiftingPointLocationAccuracy(Float64 x);
   Float64 GetLiftingPointLocationAccuracy() const;

   // Set/Get the minimum location of the truck support location
   void SetMininumTruckSupportLocation(Float64 x); // < 0 means use Hg
   Float64 GetMininumTruckSupportLocation() const;

   // Set/Get the accuracy of the truck support location during automated design
   void SetTruckSupportLocationAccuracy(Float64 x);
   Float64 GetTruckSupportLocationAccuracy() const;

   // Set/Get concrete strength type
   void SetLimitStateConcreteStrength(pgsTypes::LimitStateConcreteStrength lsFc);
   pgsTypes::LimitStateConcreteStrength GetLimitStateConcreteStrength() const;
   void Use90DayStrengthForSlowCuringConcrete(bool bUse, Float64 factor);
   void Use90DayStrengthForSlowCuringConcrete(bool* pbUse, Float64* pfactor) const;

   // Set/Get parameters for checking clear space between adjacent bottom flanges
   void CheckBottomFlangeClearance(bool bCheck);
   bool CheckBottomFlangeClearance() const;
   void SetMinBottomFlangeClearance(Float64 Cmin);
   Float64 GetMinBottomFlangeClearance() const;

   // Set/Get parameters for checking maximum inclinations of tilted girders
   void CheckGirderInclination(bool bCheck);
   bool CheckGirderInclination() const;
   void SetGirderInclinationFactorOfSafety(Float64 fs);
   Float64 GetGirderInclinationFactorOfSafety() const;

   // Set/Get rounding parameters for required slab offset
   void SetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod method, Float64 tolerance);
   void GetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod* pMethod, Float64* pTolerance) const;

   //////////////////////////////////////
   //
   // Precast Elements
   //
   //////////////////////////////////////

   // Set/Get the maximum allowable concrete compression stress at release as a factor times f'ci
   Float64 GetAtReleaseCompressionStressFactor() const;
   void SetAtReleaseCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tension stress at release as a factor times sqrt(f'ci)
   Float64 GetAtReleaseTensionStressFactor() const;
   void SetAtReleaseTensionStressFactor(Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress at release.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetAtReleaseMaximumTensionStress(bool* bIsApplicable, Float64* maxStress) const;
   void SetAtReleaseMaximumTensionStress(bool bIsApplicable, Float64 maxStress);

   // Set/Get the maximum allowable concrete tension stress at release as a factor times sqrt(f'ci)
   // when adequate mild rebar is provided
   Float64 GetAtReleaseTensionStressFactorWithRebar() const;
   void SetAtReleaseTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at erection as a factor times f'c
   Float64 GetErectionCompressionStressFactor() const;
   void SetErectionCompressionStressFactor(Float64 stress);

   // Set/Get the maxumum allowable concrete tension stress at erection as a factor times sqrt(f'c)
   Float64 GetErectionTensionStressFactor() const;
   void SetErectionTensionStressFactor(Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress at erection.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetErectionMaximumTensionStress(bool* bIsApplicable, Float64* maxStress) const;
   void SetErectionMaximumTensionStress(bool bIsApplicable, Float64 maxStress);
   
   // Set/Get the maximum allowable concrete compressive stress after temporary strand removal 
   // as a factor times f'c
   Float64 GetTempStrandRemovalCompressionStressFactor() const;
   void SetTempStrandRemovalCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tension stress after temporary strand removal 
   // as a factor times sqrt(f'c)
   Float64 GetTempStrandRemovalTensionStressFactor() const;
   void SetTempStrandRemovalTensionStressFactor(Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress after temporary strand removal.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetTempStrandRemovalMaximumTensionStress(bool* bIsApplicable, Float64* maxStress) const;
   void SetTempStrandRemovalMaximumTensionStress(bool bIsApplicable, Float64 maxStress);

   // Set/Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the temporary strand removal with rebar
   Float64 GetTempStrandRemovalTensionStressFactorWithRebar() const;
   void SetTempStrandRemovalTensionStressFactorWithRebar(Float64 stress);

   //------------------------------------------------------------------------
   // Set/Get flag that indicates if stresses during temporary loading conditions
   // are to be checked (basically, Bridge Site 1 stresses)
   void CheckTemporaryStresses(bool bCheck);
   bool CheckTemporaryStresses() const;

   //------------------------------------------------------------------------
   void CheckFinalTensionPermanentLoadStresses(bool bCheck);
   bool CheckFinalTensionPermanentLoadStresses() const;

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the bridge site stage 2
   Float64 GetFinalTensionPermanentLoadsStressFactor() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete at the bridge site stage 2
   void SetFinalTensionPermanentLoadsStressFactor(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // at the bridge site stage 2
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetFinalTensionPermanentLoadStressFactor(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete at the bridge site stage 2
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetFinalTensionPermanentLoadStressFactor(bool doCheck, Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at the serivce limit state,
   // without live load, as a factor times f'c
   Float64 GetFinalWithoutLiveLoadCompressionStressFactor() const;
   void SetFinalWithoutLiveLoadCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at the service limit state,
   // with live load, as a factor times f'c
   Float64 GetFinalWithLiveLoadCompressionStressFactor() const;
   void SetFinalWithLiveLoadCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tension stress after the service limit state
   // as a factor times sqrt(f'c). Use the EXPOSURE_xxx constants
   Float64 GetFinalTensionStressFactor(int exposureCondition) const;
   void SetFinalTensionStressFactor(int exposureCondition,Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress at the service limit state.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetFinalTensionStressFactor(int exposureCondition,bool* bIsApplicable, Float64* maxStress) const;
   void SetFinalTensionStressFactor(int exposureCondition,bool bIsApplicable, Float64 maxStress);

   // Set/Get the maximum allowable concrete compressive stress at the fatigue limit state,
   // as a factor times f'c
   Float64 GetFatigueCompressionStressFactor() const;
   void SetFatigueCompressionStressFactor(Float64 stress);

   // Set/Get the method and tensile stress limit coefficient for principal tensile stress in webs
   void SetPrincipalTensileStressInWebsParameters(pgsTypes::PrincipalTensileStressMethod principalTensileStressMethod, Float64 principalTensionCoefficient, Float64 ductNearnessFactor, 
                                                  Float64 principalTensileStressUngroutedMultiplier, Float64 principalTensileStressGroutedMultiplier, Float64 principalTensileStressFcThreshold);
   void GetPrincipalTensileStressInWebsParameters(pgsTypes::PrincipalTensileStressMethod* pPrincipalTensileStressMethod, Float64* pPrincipalTensionCoefficient,Float64* pDuctNearnessFactor, 
                                                  Float64* pPrincipalTensileStressUngroutedMultiplier, Float64* pPrincipalTensileStressGroutedMultiplier, Float64* principalTensileStressFcThreshold) const;

   //////////////////////////////////////
   //
   // Closure Joints
   //
   //////////////////////////////////////

   // Set/Get the maximum allowable concrete compressive stress at stressing in a closure joint as a factor times f'ci
   Float64 GetAtStressingCompressingStressFactor() const;
   void SetAtStressingCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at stressing in a closure joint as a factor times sqrt(f'ci)
   // for a section in the precompressed tensile zone without minimum bonded auxiliary reinforcment
   Float64 GetAtStressingPrecompressedTensileZoneTensionStressFactor() const;
   void SetAtStressingPrecompressedTensileZoneTensionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at stressing in a closure joint as a factor times sqrt(f'ci)
   // for a section in the precompressed tensile zone with minimum bonded auxiliary reinforcment
   Float64 GetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar() const;
   void SetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at stressing in a closure joint as a factor times sqrt(f'ci)
   // for a section other than in the precompressed tensile zone without minimum bonded auxiliary reinforcment
   Float64 GetAtStressingOtherLocationTensionStressFactor() const;
   void SetAtStressingOtherLocationTensileZoneTensionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at stressing in a closure joint as a factor times sqrt(f'ci)
   // for a section other than in the precompressed tensile zone with minimum bonded auxiliary reinforcment
   Float64 GetAtStressingOtherLocationTensionStressFactorWithRebar() const;
   void SetAtStressingOtherLocationTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at service limit state in a closure joint as a factor times f'ci
   Float64 GetAtServiceCompressingStressFactor() const;
   void SetAtServiceCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at service limit state in a closure joint as a factor times f'ci
   Float64 GetAtServiceWithLiveLoadCompressingStressFactor() const;
   void SetAtServiceWithLiveLoadCompressionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at service limit state in a closure joint as a factor times sqrt(f'ci)
   // for a section in the precompressed tensile zone without minimum bonded auxiliary reinforcment
   Float64 GetAtServicePrecompressedTensileZoneTensionStressFactor() const;
   void SetAtServicePrecompressedTensileZoneTensionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at service limit state in a closure joint as a factor times sqrt(f'ci)
   // for a section in the precompressed tensile zone with minimum bonded auxiliary reinforcment
   Float64 GetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar() const;
   void SetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at service limit state in a closure joint as a factor times sqrt(f'ci)
   // for a section other than in the precompressed tensile zone without minimum bonded auxiliary reinforcment
   Float64 GetAtServiceOtherLocationTensionStressFactor() const;
   void SetAtServiceOtherLocationTensileZoneTensionStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tensile stress at service limit state in a closure joint as a factor times sqrt(f'ci)
   // for a section other than in the precompressed tensile zone with minimum bonded auxiliary reinforcment
   Float64 GetAtServiceOtherLocationTensionStressFactorWithRebar() const;
   void SetAtServiceOtherLocationTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the maximum allowable concrete compressive stress at the fatigue limit state,
   // as a factor times f'c
   Float64 GetClosureFatigueCompressionStressFactor() const;
   void SetClosureFatigueCompressionStressFactor(Float64 stress);

   //////////////////////////////////////
   //
   // Strands
   //
   //////////////////////////////////////

   // Set/Get parameter indicating if strand stresses should be evaluated
   // during a particular stage. Use one of the CSS_xxx constants
   void CheckStrandStress(UINT stage,bool bCheck);
   bool CheckStrandStress(UINT stage) const;

   // Set/Get the allowable strand stress as a coefficient times fpu
   // Use one of the CSS_xxx constants for stage and LOW_RELAX or STRESS_REL
   // for strandType
   Float64 GetStrandStressCoefficient(UINT stage,UINT strandType) const;
   void SetStrandStressCoefficient(UINT stage,UINT strandType, Float64 coeff);

   // Returns true if tendon stresses are to be check at jacking
   bool CheckTendonStressAtJacking() const;

   // Returns true if tendon stresses are to be check just prior to seating
   bool CheckTendonStressPriorToSeating() const;

   // Returns the allowable tendon stress as a coefficient times fpu
   // Use one of the CSS_xxx constants for stage and LOW_RELAX or STRESS_REL
   // for strandType
   Float64 GetTendonStressCoefficient(UINT stage,UINT strandType) const;

   // Set/Get parameter that indicates if straigh strands are allowed to be extended
   void AllowStraightStrandExtensions(bool bAllow);
   bool AllowStraightStrandExtensions() const;

   // Set/Get the method for computing prestress transfer length
   pgsTypes::PrestressTransferComputationType GetPrestressTransferComputationType() const;
   void SetPrestressTransferComputationType(pgsTypes::PrestressTransferComputationType type);

   // Set/Get duct area ratio (LRFD 5.4.6.2)
   void GetDuctAreaRatio(Float64* pPush,Float64* pPull) const;
   void SetDuctAreaRatio(Float64 push,Float64 pull);

   // Set/Get duct diameter ratio (LRFD 5.4.6.2)
   Float64 GetDuctDiameterRatio() const;
   void SetDuctDiameterRatio(Float64 dr);

   //////////////////////////////////////
   //
   // Lifting Parameters
   //
   //////////////////////////////////////

   // Set/Get minimum factor of safety against cracking for lifting
   Float64 GetCrackingFOSLifting() const;
   void SetCrackingFOSLifting(Float64 fs);

   // Set/Get minimum factor of safety against failure for lifting
   Float64 GetLiftingFailureFOS() const;
   void SetLiftingFailureFOS(Float64 fs);

   // Set/Get upward impact for lifting
   Float64 GetLiftingUpwardImpactFactor() const;
   void SetLiftingUpwardImpactFactor(Float64 impact);

   // Set/Get downward impact for lifting in the casting yard
   Float64 GetLiftingDownwardImpactFactor() const;
   void SetLiftingDownwardImpactFactor(Float64 impact);

   // Set/Get the max allowable compressive concrete stress for lifting as a factor times f'ci
   Float64 GetLiftingCompressionGlobalStressFactor() const;
   void SetLiftingCompressionGlobalStressFactor(Float64 stress);

   Float64 GetLiftingCompressionPeakStressFactor() const;
   void SetLiftingCompressionPeakStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tension stress during lifting as a factor times sqrt(f'ci)
   Float64 GetLiftingTensionStressFactor() const;
   void SetLiftingTensionStressFactor(Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress during lifting.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetLiftingMaximumTensionStress(bool* bIsApplicable, Float64* maxStress) const;
   void SetLiftingMaximumTensionStress(bool bIsApplicable, Float64 maxStress);

   // Set/Get the maximum allowable concrete tension stress during lifting as a factor times sqrt(f'ci)
   // when adequate mild rebar is provided
   Float64 GetLiftingTensionStressFactorWithRebar() const;
   void SetLiftingTensionStressFactorWithRebar(Float64 stress);

   // Set/Get the pick point height.
   Float64 GetPickPointHeight() const;
   void SetPickPointHeight(Float64 hgt);

   // Set/Get the lifting loop placement tolerance
   Float64 GetLiftingLoopTolerance() const;
   void SetLiftingLoopTolerance(Float64 hgt);

   // Set/Get the minimum lifting cable inclination angle measured from horizontal
   Float64 GetMinCableInclination() const;
   void SetMinCableInclination(Float64 angle);

   // Set/Get the maximum girder sweep tolerance for lifting
   Float64 GetLiftingMaximumGirderSweepTolerance() const;
   void SetLiftingMaximumGirderSweepTolerance(Float64 sweep);

   // Set/Get the coefficient used to compute the modulus of rupture of concrete for lifting as a factor times sqrt(f'c)
   Float64 GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType type) const;
   void SetLiftingModulusOfRuptureFactor(Float64 fr,pgsTypes::ConcreteType type);

   Float64 GetLiftingCamberMultiplier() const;
   void SetLiftingCamberMultiplier(Float64 m);

   pgsTypes::WindType GetLiftingWindType() const;
   void SetLiftingWindType(pgsTypes::WindType windType);

   // wind load is either pressure or speed depending on the wind type
   Float64 GetLiftingWindLoad() const;
   void SetLiftingWindLoad(Float64 wl);

   //////////////////////////////////////
   //
   // Hauling/Shipping Parameters
   //
   //////////////////////////////////////

   // General hauling parameters

   // Set/Get the hauling analysis method
   void SetHaulingAnalysisMethod(pgsTypes::HaulingAnalysisMethod method);
   pgsTypes::HaulingAnalysisMethod GetHaulingAnalysisMethod() const;

   // Set/Get upward impact for hauling
   Float64 GetHaulingUpwardImpactFactor() const;
   void SetHaulingUpwardImpactFactor(Float64 impact);

   // Set/Get downward impact for hauling
   Float64 GetHaulingDownwardImpactFactor() const;
   void SetHaulingDownwardImpactFactor(Float64 impact);

   // Set/Get the maximum girder sweep tolerance for Hauling
   Float64 GetHaulingMaximumGirderSweepTolerance() const;
   void SetHaulingMaximumGirderSweepTolerance(Float64 sweep);

   // Set/Get the the sweep growth for hauling.
   // PCI models sweep at hauling as (1/8" per 10 ft + 1")
   // The 1" is the sweep growth
   Float64 GetHaulingSweepGrowth() const;
   void SetHaulingSweepGrowth(Float64 sweepGrowth);

   // Set/Get the maximum lateral tolerance for hauling support placement
   Float64 GetHaulingSupportPlacementTolerance() const;
   void SetHaulingSupportPlacementTolerance(Float64 tol);

   Float64 GetHaulingCamberMultiplier() const;
   void SetHaulingCamberMultiplier(Float64 m);

   const COldHaulTruck* GetOldHaulTruck() const;

   void SetHaulingImpactUsage(pgsTypes::HaulingImpact impactUsage);
   pgsTypes::HaulingImpact GetHaulingImpactUsage() const;

   Float64 GetRoadwayCrownSlope() const;
   void SetRoadwayCrownSlope(Float64 slope);

   // Set/Get max expected roadway superelevation angle during hauling
   Float64 GetRoadwaySuperelevation() const;
   void SetRoadwaySuperelevation(Float64 dist);

   // Set/Get the max allowable compressive concrete stress for global stresses for hauling as a factor times f'c
   Float64 GetHaulingCompressionGlobalStressFactor() const;
   void SetHaulingCompressionGlobalStressFactor(Float64 stress);

   // Set/Get the max allowable compressive concrete stress for peak stresses for hauling as a factor times f'c
   Float64 GetHaulingCompressionPeakStressFactor() const;
   void SetHaulingCompressionPeakStressFactor(Float64 stress);

   // Set/Get the maximum allowable concrete tension stress during during hauling as a factor times sqrt(f'ci)
   Float64 GetHaulingTensionStressFactor(pgsTypes::HaulingSlope slope) const;
   void SetHaulingTensionStressFactor(pgsTypes::HaulingSlope slope,Float64 stress);

   // Set/Get the absolute maximum allowable concrete tension stress during hauling.
   // If bIsApplicable is false the allowable concrete tension stress is not limited
   // and maxStress is undefined
   void GetHaulingMaximumTensionStress(pgsTypes::HaulingSlope slope,bool* bIsApplicable, Float64* maxStress) const;
   void SetHaulingMaximumTensionStress(pgsTypes::HaulingSlope slope,bool bIsApplicable, Float64 maxStress);

   // Set/Get the maximum allowable concrete tension stress during hauling as a factor times sqrt(f'c)
   // when adequate mild rebar is provided
   Float64 GetHaulingTensionStressFactorWithRebar(pgsTypes::HaulingSlope slope) const;
   void SetHaulingTensionStressFactorWithRebar(pgsTypes::HaulingSlope slope,Float64 stress);

   // Set/Get minimum factor of safety against cracking for hauling
   Float64 GetHaulingCrackingFOS() const;
   void SetHaulingCrackingFOS(Float64 fs);

   // Set/Get minimum factor of safety against failure for hauling
   Float64 GetHaulingFailureFOS() const;
   void SetHaulingFailureFOS(Float64 fs);

   // Set/Get the coefficient used to compute the modulus of rupture of concrete during hauling as a factor times sqrt(f'c)
   Float64 GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType type) const;
   void SetHaulingModulusOfRuptureFactor(Float64 fr,pgsTypes::ConcreteType type);

   pgsTypes::WindType GetHaulingWindType() const;
   void SetHaulingWindType(pgsTypes::WindType windType);

   // wind load is either pressure or speed depending on the wind type
   Float64 GetHaulingWindLoad() const;
   void SetHaulingWindLoad(Float64 wl);

   pgsTypes::CFType GetCentrifugalForceType() const;
   void SetCentrifugalForceType(pgsTypes::CFType cfType);

   Float64 GetHaulingSpeed() const;
   void SetHaulingSpeed(Float64 v);

   Float64 GetTurningRadius() const;
   void SetTurningRadius(Float64 r);

   //
   // Values used for KDOT method only
   //
   void SetUseMinTruckSupportLocationFactor(bool factor);
   bool GetUseMinTruckSupportLocationFactor() const;
   void SetMinTruckSupportLocationFactor(Float64 factor);
   Float64 GetMinTruckSupportLocationFactor() const;

   void SetOverhangGFactor(Float64 factor);
   Float64 GetOverhangGFactor() const;
   void SetInteriorGFactor(Float64 factor);
   Float64 GetInteriorGFactor() const;

   //////////////////////////////////////
   //
   // Loads
   //
   //////////////////////////////////////

   // Set/Get the maximum number of girders that traffic barrier loads may be
   // distributed over.
   GirderIndexType GetMaxGirdersDistTrafficBarrier() const;
   void SetMaxGirdersDistTrafficBarrier(GirderIndexType num);

   // Set/Get the traffic barrier load distribution type
   pgsTypes::TrafficBarrierDistribution GetTrafficBarrierDistributionType() const;
   void SetTrafficBarrierDistibutionType(pgsTypes::TrafficBarrierDistribution tbd);

   // Set/Get the overlay load distribution type
   pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() const;
   void SetOverlayLoadDistributionType(pgsTypes::OverlayLoadDistributionType type);

   // Set/Get the how the haunch load is computed wrt girder camber
   pgsTypes::HaunchLoadComputationType GetHaunchLoadComputationType() const;
   void SetHaunchLoadComputationType(pgsTypes::HaunchLoadComputationType type);

   // set/get solution tolerance between camber used to compute haunch load and computed excess camber.
   // This value is only used if HaunchLoadComputationType==hspAccountForCamber or HaunchAnalysisSectionPropertiesType==hspDetailedDescription
   Float64 GetHaunchLoadCamberTolerance() const;
   void SetHaunchLoadCamberTolerance(Float64 tol);

   // Set/get factor applied to assumed excess camber when computing haunch loads
   // This value is only used if HaunchLoadComputationType==hlcDetailedAnalysis && slab offset input
   // Valid values are 0.0< to <=1.0
   Float64 GetHaunchLoadCamberFactor() const;
   void SetHaunchLoadCamberFactor(Float64 tol);

   // Set/Get the how the haunch is used when computing composite section properties
   pgsTypes::HaunchAnalysisSectionPropertiesType GetHaunchAnalysisSectionPropertiesType() const;
   void SetHaunchAnalysisSectionPropertiesType(pgsTypes::HaunchAnalysisSectionPropertiesType type);

   // Set/Get the magnitude of the pedestrian live load
   Float64 GetPedestrianLiveLoad() const;
   void SetPedestrianLiveLoad(Float64 w);

   // Set/Get the minimum width of sidewalk on which pedestrian live load is applied
   Float64 GetMinSidewalkWidth() const;
   void SetMinSidewalkWidth(Float64 Wmin);

   // Set/Get the method of compute live load distribution factors.
   // Use one of the LLDF_XXXX constants
   Int16 GetLiveLoadDistributionMethod() const;
   void SetLiveLoadDistributionMethod(Int16 method);

   // Set/Get a flag indicating if the skew reduction factor for moment LLDF
   // is to be ignored
   void IgnoreSkewReductionForMoment(bool bIgnore);
   bool IgnoreSkewReductionForMoment() const;

   // Set/Get decision to impose a lower limit on distribution factors
   // If true, live load distribution factors are never taken less than
   // the number of lanes divided by the number of girders
   void LimitDistributionFactorsToLanesBeams(bool bInclude);
   bool LimitDistributionFactorsToLanesBeams() const;

   // Set/Get over-arching rule that exterior live load distribution factors
   // may not be less than adjacent interior factors
   void SetExteriorLiveLoadDistributionGTAdjacentInteriorRule(bool bValue);
   bool GetExteriorLiveLoadDistributionGTAdjacentInteriorRule() const;

   // Set/Get maxumum angular deviation between girders
   // This parameter is used to determine if girders are approximately parallel
   // per LRFD 4.6.2.2.1
   void SetMaxAngularDeviationBetweenGirders(Float64 angle);
   Float64 GetMaxAngularDeviationBetweenGirders() const;

   // Set/Get the minimum girder stiffness ratio. The girder stiffness ratio
   // is the ratio of EI for adjacent girders. This parameter is used to 
   // determine if girders have approximately the same stiffness per LRFD 4.6.2.2.1
   void SetMinGirderStiffnessRatio(Float64 r);
   Float64 GetMinGirderStiffnessRatio() const;

   // Set/Get the location along a span where the spacing between girders is
   // computed. Distribution factors are computed per span based on a single 
   // spacing.
   void SetLLDFGirderSpacingLocation(Float64 fra);
   Float64 GetLLDFGirderSpacingLocation() const;

   // Set/Get rigid method option
   void UseRigidMethod(bool bUseRigidMethod);
   bool UseRigidMethod() const;

   // Set/Get inclusion of HL93 low boy, tandem vehicle
   void IncludeDualTandem(bool bInclude);
   bool IncludeDualTandem() const;

   //////////////////////////////////////
   //
   // Moment Capacity Parameters
   //
   //////////////////////////////////////

   // Set/Get the method for computing over reinforced section moment capacity.
   // If true, over reinforced moment capacity computed per LRFD C5.7.3.3.1 (method removed in 2005)
   // otherwise computed by WSDOT method
   bool GetLRFDOverreinforcedMomentCapacity() const;
   void SetLRFDOverreinforcedMomentCapacity(bool bSet);

   // Set/Get a parameter that indicates if girder rebar is included in moment capacity calculations.
   // If true, girder mild reinforcement is included in the moment capacity calculations.
   void IncludeRebarForMoment(bool bInclude);
   bool IncludeRebarForMoment() const;
   
   // Set/Get a parameter that indicates if the strain limits specified in the relavent material
   // standards are considered when computing moment capacity. If considered, and the strains are
   // exceeded when the concrete crushing strain is at 0.003, the capacity is recomputed based
   // on the limiting strain.
   void ConsiderReinforcementStrainLimitForMomentCapacity(bool bConsider);
   bool ConsiderReinforcementStrainLimitForMomentCapacity() const;

   void SetSliceCountForMomentCapacity(IndexType nSlices);
   IndexType GetSliceCountForMomentCapacity() const;
   
   // Set/Get a paramter that indicates if pretensioned strands are included in negative moment capacity calculations.
   // If true, the pretensioned strands are included in negative moment capacity calculuations
   void IncludeStrandForNegativeMoment(bool bInclude);
   bool IncludeStrandForNegativeMoment() const;

   // Set/Get the coefficient for computing modulus of rupture for moment capacity analysis
   void SetFlexureModulusOfRuptureCoefficient(pgsTypes::ConcreteType type,Float64 fr);
   Float64 GetFlexureModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const;

   // Set/Get the moment capacity resistance factors
   void SetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64 phiTensionPS,Float64 phiTensionRC,Float64 phiTensionSpliced,Float64 phiCompression);
   void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const;

   void SetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type,Float64 phi);
   Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const;

   // Set/Get parameter that indicates if non-composite moments are included in negative moment (deck)
   // design. If true, Mu for deck design includes moments on the non-composite section.
   void IncludeNoncompositeMomentsForNegMomentDesign(bool bInclude);
   bool IncludeNoncompositeMomentsForNegMomentDesign() const;

   //////////////////////////////////////
   //
   // Shear Capacity Parameters
   //
   //////////////////////////////////////

   // Set/Get the shear capacity calculation method
   void SetShearCapacityMethod(pgsTypes::ShearCapacityMethod method);
   pgsTypes::ShearCapacityMethod GetShearCapacityMethod() const;

   // Set/Get flag indicating if the net tensile strain computed per LRFD Eq. 5.7.3.4.2-4 should be limited to non-negative numbers
   void LimitNetTensionStrainToPositiveValues(bool bLimit);
   bool LimitNetTensionStrainToPositiveValues() const;

   // Set/Get the coefficient for computing modulus of rupture for shear capacity analysis
   void SetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type,Float64 fr);
   Float64 GetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const;

   // Set/Get the shear capacity resistance factors
   void SetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type,Float64 phi);
   Float64 GetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type) const;

   void SetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type,Float64 phi);
   Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const;

   // Longitudinal Reinforcement for shear

   // Set/Get a parameter that indicates if girder rebar is included in longitudinal reinforcement
   // for shear calculations.
   // If true, girder mild reinforcement is included in the longitindal reinforcement for shear calculations.
   void IncludeRebarForShear(bool bInclude);
   bool IncludeRebarForShear() const;

   // Set/Get Max allowable stirrup spacing for girder.
   // Stirrup spacing limitations are given in LRFD 5.7.2.6 (pre2017: 5.8.2.7) in the form of
   // Smax = k*dv <= s (5.8.2.7-1 Smax = 0.8dv <= 48")
   // K1 and S1 are for equation 5.7.2.6-1
   // K2 and S2 are for equation 5.7.2.6-2
   void SetMaxStirrupSpacing(Float64 K1,Float64 S1,Float64 K2,Float64 S2);
   void GetMaxStirrupSpacing(Float64* pK1,Float64* pS1,Float64* pK2,Float64* pS2) const;

   // Set/Get the concrete curing method
   // Use one of the CURING_xxx constants
   int GetCuringMethod() const;
   void SetCuringMethod(int method);

   // Set/Get the longitudinal reinforcement for shear calculation method.
   // Use one of the LRSH_XXXX constants
   Int16 GetLongReinfShearMethod() const;
   void SetLongReinfShearMethod(Int16 method);

   // Horizontal Interface Shear

   // Set/Get the shear flow calculation method
   void SetShearFlowMethod(pgsTypes::ShearFlowMethod method);
   pgsTypes::ShearFlowMethod GetShearFlowMethod() const;

   void SetMaxInterfaceShearConnectionSpacing(Float64 sMax);
   Float64 GetMaxInterfaceShearConnectorSpacing() const;

   // Set/Get parameter that indicates if the weight of cast in place
   // deck is used for the permanent net compressive force normal to 
   // the shear plane.
   // if true, Pc for LRFD5.7.4.3 is computed otherwise it is taken as 0.0.
   void UseDeckWeightForPermanentNetCompressiveForce(bool bUse);
   bool UseDeckWeightForPermanentNetCompressiveForce() const;

   //////////////////////////////////////
   //
   // Creep and Camber Parameters
   //
   //////////////////////////////////////

   // Set/Get the creep analysis method. 
   // Use one of the CREEP_xxx constants
   // NOTE: The option to select a creep calculation method was
   // removed with LRFD 2006 as WSDOT dropped its creep method
   // in favor of the LRFD method. This parameter is only applicable
   // with the LRFD code is 2005 or earlier.
   int GetCreepMethod() const;
   void SetCreepMethod(int method);

   // Set/Get the creep factor for the WSDOT creep method (BDM 6.1.2c.2)
   // NOTE: The option to select a creep calculation method was
   // removed with LRFD 2006 as WSDOT dropped its creep method
   // in favor of the LRFD method. This parameter is only applicable
   // with the LRFD code is 2005 or earlier.
   Float64 GetCreepFactor() const;
   void SetCreepFactor(Float64 cf);

   // Get/Set the time from strand stressing to prestress transfer
   // Used for computing relaxation losses prior to prestress transfer.
   // NOTE: this is only applicable to non-time step loss methods
   Float64 GetXferTime() const;
   void SetXferTime(Float64 time);

   // Set/Get the number of days from prestress release until temporary strand
   // removal (or diaphragm loading for structures without temporary strands).
   // NOTE: this is only applicable to non-time step loss methods
   Float64 GetCreepDuration1Min() const;
   Float64 GetCreepDuration1Max() const;
   void SetCreepDuration1(Float64 min,Float64 max);

   // Set/Get the number of days from prestress release until the slab
   // is acting composite with the girder.
   // NOTE: this is only applicable to non-time step loss methods
   Float64 GetCreepDuration2Min() const;
   Float64 GetCreepDuration2Max() const;
   void SetCreepDuration2(Float64 min,Float64 max);

   // Set/Get the total creep duration.
   // NOTE: this is only applicable to non-time step loss methods
   void SetTotalCreepDuration(Float64 duration);
   Float64 GetTotalCreepDuration() const;

   //------------------------------------------------------------------------
   //  Variability between upper and lower bound camber, stored in decimal percent
   void SetCamberVariability(Float64 var);
   Float64 GetCamberVariability() const;

   void CheckGirderSag(bool bCheck);
   bool CheckGirderSag() const;

   pgsTypes::SagCamberType GetSagCamberType() const;
   void SetSagCamberType(pgsTypes::SagCamberType type);

   // Set/Get the curing method time adjustment factor
   // Form LRFD, 1 day of steam curing = 7 days of moist curing
   void SetCuringMethodTimeAdjustmentFactor(Float64 f);
   Float64 GetCuringMethodTimeAdjustmentFactor() const;

   //////////////////////////////////////
   //
   // Prestress Loss Parameters
   //
   //////////////////////////////////////

   // Set/Get method for computing losses. 
   // Use one of the LOSSES_xxx constants
   int GetLossMethod() const;
   void SetLossMethod(int method);

   // Set/Get the time-dependent model type for time-step loss calculations
   // Use on of the TDM_xxx constants
   int GetTimeDependentModel() const;
   void SetTimeDependentModel(int model);

   //------------------------------------------------------------------------
   // Returns the losses before prestress xfer for a lump sum method
   Float64 GetBeforeXferLosses() const;
   void SetBeforeXferLosses(Float64 loss);

   //------------------------------------------------------------------------
   // Returns the losses after prestress xfer for a lump sum method
   Float64 GetAfterXferLosses() const;
   void SetAfterXferLosses(Float64 loss);

   Float64 GetLiftingLosses() const;
   void SetLiftingLosses(Float64 loss);

   // Set/Get the shipping losses for lump sum loss methods or for a method
   // that does not support computing losses at shipping.
   Float64 GetShippingLosses() const;
   void SetShippingLosses(Float64 loss);

   // Set/Get the time when shipping occurs. Used when shipping losses are 
   // computed by the LRFD refined method after LRFD 2005
   void SetShippingTime(Float64 time);
   Float64 GetShippingTime() const;

   //------------------------------------------------------------------------
   Float64 GetBeforeTempStrandRemovalLosses() const;
   void SetBeforeTempStrandRemovalLosses(Float64 loss);

   //------------------------------------------------------------------------
   Float64 GetAfterTempStrandRemovalLosses() const;
   void SetAfterTempStrandRemovalLosses(Float64 loss);

   //------------------------------------------------------------------------
   Float64 GetAfterDeckPlacementLosses() const;
   void SetAfterDeckPlacementLosses(Float64 loss);

   //------------------------------------------------------------------------
   Float64 GetAfterSIDLLosses() const;
   void SetAfterSIDLLosses(Float64 loss);

   //------------------------------------------------------------------------
   // Returns the final losses for a lump sum method
   Float64 GetFinalLosses() const;
   void SetFinalLosses(Float64 loss);

   //------------------------------------------------------------------------
   // Returns the anchor set
   bool UpdatePTParameters() const;
   Float64 GetAnchorSet() const;
   void SetAnchorSet(Float64 dset);

   Float64 GetWobbleFrictionCoefficient() const;
   void SetWobbleFrictionCoefficient(Float64 K);

   Float64 GetFrictionCoefficient() const;
   void SetFrictionCoefficient(Float64 u);

   // Set/Get the method for computing relaxation losses.
   // Use one of the RLM_xxx constants
   void SetRelaxationLossMethod(Int16 method);
   Int16 GetRelaxationLossMethod() const;

   // Set/Get load effectiveness for elastic gains due to the main slab dead load
   // (including deck panels if present)
   Float64 GetSlabElasticGain() const;
   void SetSlabElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to the slab haunch dead load
   Float64 GetSlabPadElasticGain() const;
   void SetSlabPadElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to the diaphragm dead load
   Float64 GetDiaphragmElasticGain() const;
   void SetDiaphragmElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to user defined DC loads
   // applied to the non-composite section
   Float64 GetUserLoadBeforeDeckDCElasticGain() const;
   void SetUserLoadBeforeDeckDCElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to user defined DC loads
   // applied to the composite section
   Float64 GetUserLoadAfterDeckDCElasticGain() const;
   void SetUserLoadAfterDeckDCElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to user defined DW loads
   // applied to the non-composite section
   Float64 GetUserLoadBeforeDeckDWElasticGain() const;
   void SetUserLoadBeforeDeckDWElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to user defined DW loads
   // applied to the composite section
   Float64 GetUserLoadAfterDeckDWElasticGain() const;
   void SetUserLoadAfterDeckDWElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to the railing system dead load
   Float64 GetRailingSystemElasticGain() const;
   void SetRailingSystemElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to the overlay dead load
   Float64 GetOverlayElasticGain() const;
   void SetOverlayElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to deck shrinkage
   Float64 GetDeckShrinkageElasticGain() const;
   void SetDeckShrinkageElasticGain(Float64 f);

   // Set/Get load effectiveness for elastic gains due to live load
   Float64 GetLiveLoadElasticGain() const;
   void SetLiveLoadElasticGain(Float64 f);

   //------------------------------------------------------------------------
   // Get/Set a FCGP_XXXX constant to determine the method used to compute fcgp
   // for losses. This option is only used for the TxDOT 2013 losses method
   void SetFcgpComputationMethod(Int16 method);
   Int16 GetFcgpComputationMethod() const;

   //------------------------------------------------------------------------
   // Determine if elastic gains or deck shrinkage are applicable
   bool AreElasticGainsApplicable() const;
   bool IsDeckShrinkageApplicable() const;


   //////////////////////////////////////
   //
   // Limits Parameters
   //
   //////////////////////////////////////

   // Set/Get the threshold value for slab concrete strength for which the user be warned
   // that it is excess. A warning will be issued for any value greater than the threshold.
   void SetMaxSlabFc(pgsTypes::ConcreteType type,Float64 fc);
   Float64 GetMaxSlabFc(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for specified 28-day segment strength for which the user be warned
   // that it is excess. A warning will be issued for any value greater than the threshold.
   void SetMaxSegmentFc(pgsTypes::ConcreteType type,Float64 fc);
   Float64 GetMaxSegmentFc(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for specified segment strength at initial loading for which the user be warned
   // that it is excess. A warning will be issued for any value greater than the threshold.
   void SetMaxSegmentFci(pgsTypes::ConcreteType type,Float64 fci);
   Float64 GetMaxSegmentFci(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for specified 28-day closure joint strength for which the user be warned
   // that it is excess. A warning will be issued for any value greater than the threshold.
   void SetMaxClosureFc(pgsTypes::ConcreteType type,Float64 fc);
   Float64 GetMaxClosureFc(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for specified closure joint strength at initial loading for which the user be warned
   // that it is excess. A warning will be issued for any value greater than the threshold.
   void SetMaxClosureFci(pgsTypes::ConcreteType type,Float64 fci);
   Float64 GetMaxClosureFci(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for concrete unit weight for which the user will be warned
   // that it is excessive. A warning will be issued for any value greater than the threshold.
   void SetMaxConcreteUnitWeight(pgsTypes::ConcreteType type,Float64 wc);
   Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType type) const;

   // Set/Get the threshold value for concrete maximum aggregate size for which the user will be warned
   // that it is excessive. A warning will be issued for any value greater than the threshold.
   void SetMaxConcreteAggSize(pgsTypes::ConcreteType type,Float64 agg);
   Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType type) const;

   // Returns true if the load factors were loaded from an old library
   // and the main application needs to be updated with these values
   bool UpdateLoadFactors() const;

   void GetDCLoadFactors(pgsTypes::LimitState ls,Float64* pDCmin,Float64* pDCmax) const;
   void GetDWLoadFactors(pgsTypes::LimitState ls,Float64* pDWmin,Float64* pDWmax) const;
   void GetLLIMLoadFactors(pgsTypes::LimitState ls,Float64* pLLIMmin,Float64* pLLIMmax) const;
   
   void SetDCLoadFactors(pgsTypes::LimitState ls,Float64 DCmin,Float64 DCmax);
   void SetDWLoadFactors(pgsTypes::LimitState ls,Float64 DWmin,Float64 DWmax);
   void SetLLIMLoadFactors(pgsTypes::LimitState ls,Float64 LLIMmin,Float64 LLIMmax);

   void SetDoCheckStirrupSpacingCompatibility(bool doCheck);
   bool GetDoCheckStirrupSpacingCompatibility() const;

   void SetFinishedElevationTolerance(Float64 tol);
   Float64 GetFinishedElevationTolerance() const;

   //////////////////////////////////////
   //
   // Bearings Parameters
   //
   //////////////////////////////////////
   void AlertTaperedSolePlateRequirement(bool bAlert);
   bool AlertTaperedSolePlateRequirement() const;
   void SetTaperedSolePlateInclinationThreshold(Float64 threshold);
   Float64 GetTaperedSolePlateInclinationThreshold() const;
   void UseImpactForBearingReactions(bool bUse);
   bool UseImpactForBearingReactions() const;

   ////////////////////////////////////////
   //
   // Legacy Functions
   //
   ////////////////////////////////////////

   // Returns parameters that were once in an older version of this object.
   // These methods are obsolete and should only be used by the Project Agent
   // to get data the was at one time in the library and is now part of the
   // project data

   // Gets parameter that indicates if the range of applicability for the LRFD
   // live load distribution factors is to be ignored.
   bool IgnoreRangeOfApplicabilityRequirements() const;

   // set version of these methods are obsolete and should be removed
   void IgnoreRangeOfApplicabilityRequirements(bool bIgnore);


protected:
   void MakeCopy(const SpecLibraryEntry& rOther);
   void MakeAssignment(const SpecLibraryEntry& rOther);

private:

   // general
   bool m_bUseCurrentSpecification;
   lrfdVersionMgr::Version m_SpecificationType;
   lrfdVersionMgr::Units m_SpecificationUnits;
   std::_tstring m_Description;
   pgsTypes::SectionPropertyMode m_SectionPropertyMode;

   // casting yard
   bool    m_DoCheckStrandSlope;
   bool    m_DoDesignStrandSlope;
   Float64 m_MaxSlope05;
   Float64 m_MaxSlope06;
   Float64 m_MaxSlope07;

   bool    m_DoCheckHoldDown;
   bool    m_DoDesignHoldDown;
   int     m_HoldDownForceType; // one of the HOLD_DOWN_XXX constants
   Float64 m_HoldDownForce;
   Float64 m_HoldDownFriction;

   bool m_bCheckHandlingWeightLimit;
   Float64 m_HandlingWeightLimit;

   bool    m_DoCheckSplitting;    // 5.9.4.4 (pre2017: 5.10.10)
   bool    m_DoCheckConfinement;  // 5.9.4.4
   bool    m_DoDesignSplitting;   // 5.9.4.4
   bool    m_DoDesignConfinement; // 5.9.4.4


   Float64 m_CyLiftingCrackFs;
   Float64 m_CyLiftingFailFs;
   Float64 m_CyCompStressServ;
   Float64 m_LiftingCompressionStressCoefficient_GlobalStress;
   Float64 m_LiftingCompressionStressCoefficient_PeakStress;

   Float64 m_CyTensStressServ;
   bool    m_CyDoTensStressServMax;
   Float64 m_CyTensStressServMax;

   Float64 m_CyTensStressServWithRebar;
   Float64 m_TensStressLiftingWithRebar;
   std::array<Float64, 2> m_TensStressHaulingWithRebar; // array index is pgsTypes::HaulingSlope

   Float64 m_CyTensStressLifting;
   bool    m_CyDoTensStressLiftingMax;
   Float64 m_CyTensStressLiftingMax;

   bool    m_EnableLiftingCheck;
   bool    m_EnableLiftingDesign;
   Float64 m_PickPointHeight;
   Float64 m_LiftingLoopTolerance;
   Float64 m_MinCableInclination;
   Float64 m_MaxGirderSweepLifting;
   Float64 m_LiftingUpwardImpact;
   Float64 m_LiftingDownwardImpact;
   int     m_CuringMethod;
   Float64 m_LiftingCamberMultiplier; // multilplier for direct camber
   pgsTypes::WindType m_LiftingWindType;
   Float64 m_LiftingWindLoad;
   Float64 m_SplittingZoneLengthFactor;

   // hauling
   bool    m_EnableHaulingCheck;
   bool    m_EnableHaulingDesign;
   pgsTypes::HaulingAnalysisMethod m_HaulingAnalysisMethod;

   Float64 m_HaulingCrackFs;
   Float64 m_HaulingRollFs;

   Float64 m_HaulingUpwardImpact;
   Float64 m_HaulingDownwardImpact;

   Float64 m_HaulingCamberMultiplier; // multilplier for direct camber

   bool m_bHasOldHaulTruck; // if true, an old spec library entry was read and the hauling truck information is stored in m_OldHaulTruck
   COldHaulTruck m_OldHaulTruck;
   pgsTypes::HaulingImpact m_HaulingImpactUsage;
   Float64 m_RoadwayCrownSlope;
   Float64 m_RoadwaySuperelevation;
   Float64 m_MaxGirderSweepHauling;
   Float64 m_HaulingSweepGrowth;
   Float64 m_HaulingSupportPlacementTolerance;

   Float64 m_GlobalCompStressHauling;
   Float64 m_PeakCompStressHauling;
   std::array<Float64, 2> m_TensStressHauling; // aray index is pgsTypes::HaulingSlope
   std::array<bool, 2>    m_DoTensStressHaulingMax;
   std::array<Float64, 2> m_TensStressHaulingMax;

   std::array<Float64, 3> m_HaulingModulusOfRuptureCoefficient; // pgsTypes::ConcreteType is the array index, pgsTypes::PCI_UHPC and pgsTypes::UHPC are not valid
   std::array<Float64, 3> m_LiftingModulusOfRuptureCoefficient; // pgsTypes::ConcreteType is the array index, pgsTypes::PCI_UHPC and pgsTypes::UHPC are not valid

   Float64 m_MinLiftPoint;
   Float64 m_LiftPointAccuracy;
   Float64 m_MinHaulPoint;
   Float64 m_HaulPointAccuracy;

   pgsTypes::WindType m_HaulingWindType;
   Float64 m_HaulingWindLoad;

   pgsTypes::CFType m_CentrifugalForceType;
   Float64 m_HaulingSpeed;
   Float64 m_TurningRadius;

   // Used for KDOT only
   bool    m_UseMinTruckSupportLocationFactor;
   Float64 m_MinTruckSupportLocationFactor;
   Float64 m_OverhangGFactor;
   Float64 m_InteriorGFactor;

   // temporary strand removal
   Float64 m_TempStrandRemovalCompStress;
   Float64 m_TempStrandRemovalTensStress;
   bool    m_TempStrandRemovalDoTensStressMax;
   Float64 m_TempStrandRemovalTensStressMax;
   Float64 m_TempStrandRemovalTensStressWithRebar;

   // bridge site 1
   bool m_bCheckTemporaryStresses; // indicates if limit state stresses are checked for temporary loading conditions
   Float64 m_Bs1CompStress;
   Float64 m_Bs1TensStress;
   bool    m_Bs1DoTensStressMax;
   Float64 m_Bs1TensStressMax;

   // bridge site 2
   Float64 m_Bs2CompStress;
   bool    m_bCheckBs2Tension;
   Float64 m_Bs2TensStress;
   bool    m_Bs2DoTensStressMax;
   Float64 m_Bs2TensStressMax;
   pgsTypes::TrafficBarrierDistribution m_TrafficBarrierDistributionType;
   GirderIndexType  m_Bs2MaxGirdersTrafficBarrier;
   pgsTypes::OverlayLoadDistributionType m_OverlayLoadDistribution;
   pgsTypes::HaunchLoadComputationType m_HaunchLoadComputationType;
   Float64 m_HaunchLoadCamberTolerance;
   Float64 m_HaunchLoadCamberFactor;
   pgsTypes:: HaunchAnalysisSectionPropertiesType m_HaunchAnalysisSectionPropertiesType;

   // bridge site 3
   Float64 m_Bs3CompStressServ;
   Float64 m_Bs3CompStressService1A;
   Float64 m_Bs3TensStressServNc;
   bool    m_Bs3DoTensStressServNcMax;
   Float64 m_Bs3TensStressServNcMax;
   Float64 m_Bs3TensStressServSc;
   bool    m_Bs3DoTensStressServScMax;
   Float64 m_Bs3TensStressServScMax;
   bool    m_Bs3IgnoreRangeOfApplicability;  // this will only be found in library entries older than version 29
   int     m_Bs3LRFDOverReinforcedMomentCapacity;
   bool    m_bIncludeRebar_Moment;
   bool    m_bIncludeStrand_NegMoment;
   bool    m_bConsiderReinforcementStrainLimit;
   IndexType m_nMomentCapacitySlices; // user defined number of slices for strain compatibility analysis (constrained to be between 10 and 100)
   std::array<Float64, 3>  m_FlexureModulusOfRuptureCoefficient; // index is pgsTypes::ConcreteType enum (PCI UHPC is not valid)
   std::array<Float64, 3>  m_ShearModulusOfRuptureCoefficient;   // index is pgsTypes::ConcreteType enum (PCI UHPC is not valid)
   bool m_bLimitNetTensionStrainToPositiveValues; // when true, es from LRFD Eq 5.7.3.4.2-4 is taken to be zero if it is computed as a negative value

   pgsTypes::PrincipalTensileStressMethod m_PrincipalTensileStressMethod;
   Float64 m_PrincipalTensileStressCoefficient;
   Float64 m_PrincipalTensileStressTendonNearnessFactor; // used to define if a tendon is "near" the section being evaluated... This is a number of outside duct diameters from section
   Float64 m_PrincipalTensileStressFcThreshold; // minimum f'c to trigger principal stress check for non-post tensioned beams
   Float64 m_PrincipalTensileStressUngroutedMultiplier; // Multiplier * Duct Diameter to be subtracted from web for  UNGROUTED ducts, if elevation near duct
   Float64 m_PrincipalTensileStressGroutedMultiplier;   // Multiplier * Duct Diameter to be subtracted from web for  GROUTED ducts, if elevation near duct

   void DeterminePrincipalStressDuctDeductionMultiplier();


   // Closure Joint Allowable Stresses
   Float64 m_ClosureCompStressAtStressing;
   Float64 m_ClosureTensStressPTZAtStressing; // in precompressed tensile zone
   Float64 m_ClosureTensStressPTZWithRebarAtStressing;
   Float64 m_ClosureTensStressAtStressing; // in other areas
   Float64 m_ClosureTensStressWithRebarAtStressing;
   Float64 m_ClosureCompStressAtService;
   Float64 m_ClosureCompStressWithLiveLoadAtService;
   Float64 m_ClosureTensStressPTZAtService;
   Float64 m_ClosureTensStressPTZWithRebarAtService;
   Float64 m_ClosureTensStressAtService;
   Float64 m_ClosureTensStressWithRebarAtService;
   Float64 m_ClosureCompStressFatigue;


   // Creep
   int     m_CreepMethod;
   Float64 m_XferTime;
   Float64 m_CreepFactor;
   Float64 m_CreepDuration1Min;
   Float64 m_CreepDuration1Max;
   Float64 m_CreepDuration2Min;
   Float64 m_CreepDuration2Max;
   Float64 m_TotalCreepDuration;

   Float64 m_CamberVariability; // Variability between upper and lower bound camber, stored in decimal percent
   bool m_bCheckSag; // evaluate girder camber and dead load deflections and check for sag potential
   pgsTypes::SagCamberType m_SagCamberType; // indicates the camber used to detect girder sag potential

   // Losses
   int     m_LossMethod;
   int     m_TimeDependentModel;
   Float64 m_FinalLosses;
   Float64 m_LiftingLosses;
   Float64 m_ShippingLosses;  // if between -1.0 and 0, shipping loss is fraction of final loss. Fraction is abs(m_ShippingLoss)
   Float64 m_BeforeXferLosses;
   Float64 m_AfterXferLosses;
   Float64 m_ShippingTime;
   Float64 m_BeforeTempStrandRemovalLosses;
   Float64 m_AfterTempStrandRemovalLosses;
   Float64 m_AfterDeckPlacementLosses;
   Float64 m_AfterSIDLLosses;

   bool m_bUpdatePTParameters;
   Float64 m_Dset; // anchor set
   Float64 m_WobbleFriction; // wobble friction, K
   Float64 m_FrictionCoefficient; // mu

   Float64 m_SlabElasticGain;
   Float64 m_SlabPadElasticGain;
   Float64 m_DiaphragmElasticGain;
   Float64 m_UserDCElasticGainBS1;
   Float64 m_UserDWElasticGainBS1;
   Float64 m_UserDCElasticGainBS2;
   Float64 m_UserDWElasticGainBS2;
   Float64 m_RailingSystemElasticGain;
   Float64 m_OverlayElasticGain;
   Float64 m_SlabShrinkageElasticGain;
   Float64 m_LiveLoadElasticGain;

   // Live Load Distribution Factors
   int m_LldfMethod;
   bool m_bIgnoreSkewReductionForMoment;

   // Longitudinal reinforcement shear capacity
   int m_LongReinfShearMethod;
   bool m_bIncludeRebar_Shear;

   // Strand stress coefficients
   bool m_bCheckStrandStress[4];
   Float64 m_StrandStressCoeff[4][2];

   // Tendon Stress Coefficients
   bool m_bCheckTendonStressAtJacking;
   bool m_bCheckTendonStressPriorToSeating;
   Float64 m_TendonStressCoeff[5][2];

   Float64 m_DuctAreaPushRatio;
   Float64 m_DuctAreaPullRatio;
   Float64 m_DuctDiameterRatio;

   // live load deflection
   bool m_bDoEvaluateDeflection;
   Float64 m_DeflectionLimit;

   pgsTypes::AnalysisType m_AnalysisType; // this data will be in old library entries (version < 28)

   // Concrete limits
   // limiting the size of this array to 3 for NWC and 2 kinds of LWC.. these parameters aren't used for UHPCs yet
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxSlabFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxSegmentFci;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxSegmentFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxClosureFci;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxClosureFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxConcreteUnitWeight;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> m_MaxConcreteAggSize;

   bool m_bUpdateLoadFactors; // true if the load factors are from an old library entry
   std::array<Float64, 6> m_DCmin;   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   std::array<Float64, 6> m_DWmin;
   std::array<Float64, 6> m_LLIMmin;
   std::array<Float64, 6> m_DCmax;
   std::array<Float64, 6> m_DWmax;
   std::array<Float64, 6> m_LLIMmax;

   // Warning checks
   bool m_DoCheckStirrupSpacingCompatibility;
   
   bool m_EnableSlabOffsetCheck;
   bool m_EnableSlabOffsetDesign;

   arDesignStrandFillType m_DesignStrandFillType;
   pgsTypes::EffectiveFlangeWidthMethod m_EffFlangeWidthMethod;

   pgsTypes::ShearFlowMethod m_ShearFlowMethod;
   Float64 m_MaxInterfaceShearConnectorSpacing;
   bool m_bUseDeckWeightForPc;

   std::array<Float64, 2> m_StirrupSpacingCoefficient;
   std::array<Float64, 2> m_MaxStirrupSpacing;

   pgsTypes::ShearCapacityMethod m_ShearCapacityMethod;

   Float64 m_CuringMethodTimeAdjustmentFactor;

   Float64 m_MinSidewalkWidth; // sidewalk must be greater that this width for ped load to apply
   Float64 m_PedestrianLoad; // magnitude of pedestrian load (F/L^2)
   
   // "fudge" factors for computing live load distribution factors
   Float64 m_MaxAngularDeviationBetweenGirders; // maximum angle between girders in order to consider them "parallel"
   Float64 m_MinGirderStiffnessRatio; // minimum allowable value for EImin/EImax in order to consider girders to have "approximately the same stiffness"
   Float64 m_LLDFGirderSpacingLocation; // fractional location in the girder span where the girder spacing is measured
                                       // for purposes of computing live load distribution factors

   bool m_bIncludeDualTandem; // if true, the dual tandem loading from LRFD C3.6.1.3.1 is included in the HL93 model

   bool m_LimitDistributionFactorsToLanesBeams; 
   bool m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule;

   bool m_bUseRigidMethod; // if true, the rigid method is always used with Type a, e, and k cross section for exterior beam LLDF

   pgsTypes::PrestressTransferComputationType m_PrestressTransferComputationType;

   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiFlexureTensionPS; // tension controlled, prestressed
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiFlexureTensionRC; // tension controlled, reinforced
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiFlexureTensionSpliced; // tension controlled, spliced girders
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiFlexureCompression;
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiShear;
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiShearDebonded;

   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiClosureJointFlexure;
   std::array<Float64, pgsTypes::ConcreteTypeCount> m_PhiClosureJointShear;

   Int16 m_RelaxationLossMethod;  // method for computing relaxation losses for LRFD 2005 and later, refined method
   Int16 m_FcgpComputationMethod; // method for computing fcgp for losses. only used for txdot 2013
   bool m_bIncludeForNegMoment;

   bool m_bAllowStraightStrandExtensions;

   bool m_bCheckBottomFlangeClearance;
   Float64 m_Cmin;

   bool m_bCheckGirderInclination;
   Float64 m_InclinedGirder_FSmax;

   pgsTypes::SlabOffsetRoundingMethod m_SlabOffsetRoundingMethod;
   Float64 m_SlabOffsetRoundingTolerance;

   pgsTypes::LimitStateConcreteStrength m_LimitStateConcreteStrength;
   bool m_bUse90DayConcreteStrength;
   Float64 m_90DayConcreteStrengthFactor;

   Float64 m_FinishedElevationTolerance; // tolerance between finished and design roadway surface elevation for no-deck bridges

   // bearing reactions
   bool m_bAlertTaperedSolePlateRequirement;
   Float64 m_TaperedSolePlateInclinationThreshold;
   bool m_bUseImpactForBearingReactions;
};

#endif // INCLUDED_PSGLIB_SPECLIBRARYENTRY_H_
