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

#if !defined INCLUDED_SYSTEM_SUBJECTT_H_
#include <System\SubjectT.h>
#endif

#if !defined INCLUDED_LRFD_VERSIONMGR_H_
#include <Lrfd\VersionMgr.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CSpecMainSheet;
class SpecLibraryEntry;
class SpecLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>;


#define AT_JACKING       0
#define BEFORE_TRANSFER  1
#define AFTER_TRANSFER   2
#define AFTER_ALL_LOSSES 3

#define STRESS_REL 0
#define LOW_RELAX  1

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   SpecLibraryEntryObserver

   A pure virtual entry class for observing Specification entries.


DESCRIPTION
   This class may be used to describe observe Specification entries in a library.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   virtual void Update(SpecLibraryEntry* pSubject, Int32 hint)=0;
};


/*****************************************************************************
CLASS 
   SpecLibraryEntry

   Library entry class for a parameterized specification


DESCRIPTION
   This class encapsulates all specification information required for
   prestressed girder design


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 09.17.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS SpecLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>
{
   // the dialog is our friend.
   friend CSpecMainSheet;
public:

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   SpecLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   SpecLibraryEntry(const SpecLibraryEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~SpecLibraryEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   SpecLibraryEntry& operator = (const SpecLibraryEntry& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Equality - test if two entries are equal. Ignore names by default
   virtual bool IsEqual(const SpecLibraryEntry& rOther, bool considerName=false) const;

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Set specification type we are based on
   void SetSpecificationType(lrfdVersionMgr::Version type);

   //------------------------------------------------------------------------
   // Get specification type we are based on
   lrfdVersionMgr::Version GetSpecificationType() const;

   //------------------------------------------------------------------------
   // Set specification Units we are based on
   void SetSpecificationUnits(lrfdVersionMgr::Units Units);

   //------------------------------------------------------------------------
   // Get specification Units we are based on
   lrfdVersionMgr::Units GetSpecificationUnits() const;

   //------------------------------------------------------------------------
   // Set string to describe specification
   void SetDescription(const char* name);

   //------------------------------------------------------------------------
   // Get string to describe specification
   std::string GetDescription() const;

   //------------------------------------------------------------------------
   // Get Max strand slope for 0.5 and 0.6" strands. If bool value is false,
   // then slope values do not need to be checked and slope values are 
   // undefined.
   void GetMaxStrandSlope(bool* doCheck, bool* doDesign, Float64* slope05, Float64* slope06,Float64* slope07) const;

   //------------------------------------------------------------------------
   // Set Max strand slope for 0.5 and 0.6" strands. If bool value is false,
   // then slope values do not need to be checked and slope values are 
   // undefined.
   void SetMaxStrandSlope(bool doCheck, bool doDesign, Float64 slope05=0.0, Float64 slope06=0.0,Float64 slope07=0.0);

   //------------------------------------------------------------------------
   //  Get Max allowable force to hold down strand bundles at harp point.
   //  If doCheck is false, then hold down forces do not need to be 
   //  checked and hold down force value is undefined.
   void GetHoldDownForce(bool* doCheck, bool* doDesign, Float64* force) const;

   //------------------------------------------------------------------------
   //  Set Max allowable force to hold down strand bundles at harp point.
   //  If bool value is false, then hold down forces do not need to be 
   //  checked and hold down force value is undefined.
   void SetHoldDownForce(bool doCheck, bool doDesign, Float64 force=0.0);

   //------------------------------------------------------------------------
   // Enable check for anchorage splitting and confinement 5.10.10
   void EnableAnchorageCheck(bool enable);
   bool IsAnchorageCheckEnabled() const;

   //------------------------------------------------------------------------
   // Get Max allowable stirrup spacing for girder.
   Float64 GetMaxStirrupSpacing() const;

   //------------------------------------------------------------------------
   // Set Max allowable stirrup spacing for girder.
   void SetMaxStirrupSpacing(Float64 space);

   //------------------------------------------------------------------------
   // Enable check and design for lifting in casting yard
   void EnableLiftingCheck(bool enable);
   bool IsLiftingCheckEnabled() const;

   void EnableLiftingDesign(bool enable);
   bool IsLiftingDesignEnabled() const;

   //------------------------------------------------------------------------
   // Get minimum factor of safety against cracking for Lifting in the 
   // casting yard
   Float64 GetCyLiftingCrackFs() const;

   //------------------------------------------------------------------------
   // Set minimum factor of safety against cracking for Lifting in the 
   // casting yard
   void SetCyLiftingCrackFs(Float64 fs);

   //------------------------------------------------------------------------
   // Get minimum factor of safety against failure for Lifting in the 
   // casting yard
   Float64 GetCyLiftingFailFs() const;

   //------------------------------------------------------------------------
   // Get upward impact for lifting in the casting yard
   Float64 GetCyLiftingUpwardImpact() const;

   //------------------------------------------------------------------------
   // Set upward impact for lifting in the casting yard
   void SetCyLiftingUpwardImpact(Float64 impact);

   //------------------------------------------------------------------------
   // Get downward impact for lifting in the casting yard
   Float64 GetCyLiftingDownwardImpact() const;

   //------------------------------------------------------------------------
   // Set downward impact for lifting in the casting yard
   void SetCyLiftingDownwardImpact(Float64 impact);

   // Splitting zone length h/n (h/4 or h/5) per LRFD 5.10.10.1
   void SetSplittingZoneLengthFactor(Float64 n);
   Float64 GetSplittingZoneLengthFactor() const;

   //------------------------------------------------------------------------
   // Enable check and design for hauling in casting yard
   void EnableHaulingCheck(bool enable);
   bool IsHaulingCheckEnabled() const;

   void EnableHaulingDesign(bool enable);
   bool IsHaulingDesignEnabled() const;

   //------------------------------------------------------------------------
   // Get upward impact for hauling
   Float64 GetHaulingUpwardImpact() const;

   //------------------------------------------------------------------------
   // Set upward impact for hauling
   void SetHaulingUpwardImpact(Float64 impact);

   //------------------------------------------------------------------------
   // Get downward impact for hauling
   Float64 GetHaulingDownwardImpact() const;

   //------------------------------------------------------------------------
   // Get downward impact for hauling
   void SetHaulingDownwardImpact(Float64 impact);

   //------------------------------------------------------------------------
   // Set minimum factor of safety against failure for Lifting in the 
   // casting yard
   void SetCyLiftingFailFs(Float64 fs);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for service loading
   // in the casting yard as a factor times f'ci
   Float64 GetCyCompStressService() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for service loading
   // in the casting yard as a factor times f'ci
   void SetCyCompStressService(Float64 stress);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for Lifting loading
   // in the casting yard as a factor times f'ci
   Float64 GetCyCompStressLifting() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for Lifting loading
   // in the casting yard as a factor times f'ci
   void SetCyCompStressLifting(Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the casting yard stage for service
   Float64 GetCyMaxConcreteTens() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete at the casting yard stage for service
   void SetCyMaxConcreteTens(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // at the casting yard stage  for service
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetCyAbsMaxConcreteTens(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete at the casting yard stage for service
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetCyAbsMaxConcreteTens(bool doCheck, Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the casting yard stage for lifting
   Float64 GetCyMaxConcreteTensLifting() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete at the casting yard stage for lifting
   void SetCyMaxConcreteTensLifting(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // at the casting yard stage  for lifting
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetCyAbsMaxConcreteTensLifting(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete at the casting yard stage for lifting
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetCyAbsMaxConcreteTensLifting(bool doCheck, Float64 stress);

   //------------------------------------------------------------------------
   int GetCuringMethod() const;

   //------------------------------------------------------------------------
   void SetCuringMethod(int method);

   //------------------------------------------------------------------------
   // Get the pick point height.
   Float64 GetPickPointHeight() const;

   //------------------------------------------------------------------------
   // Set the pick point height.
   void SetPickPointHeight(Float64 hgt);

   //------------------------------------------------------------------------
   // Get the lifting loop placement tolerance
   Float64 GetLiftingLoopTolerance() const;

   //------------------------------------------------------------------------
   // Set the lifting loop placement tolerance
   void SetLiftingLoopTolerance(Float64 hgt);

   //------------------------------------------------------------------------
   // Get the minimum lifting cable inclination angle measured from horizontal
   Float64 GetMinCableInclination() const;

   //------------------------------------------------------------------------
   // Set the min lifting cable inclination angle measured from horizontal
   void SetMinCableInclination(Float64 angle);

   //------------------------------------------------------------------------
   // Get the max girder sweep tolerance for lifting
   Float64 GetMaxGirderSweepLifting() const;

   //------------------------------------------------------------------------
   // Set the max girder sweep tolerance for lifting
   void SetMaxGirderSweepLifting(Float64 sweep);

   //------------------------------------------------------------------------
   // Get the max girder sweep tolerance for Hauling
   Float64 GetMaxGirderSweepHauling() const;

   //------------------------------------------------------------------------
   // Set the max girder sweep tolerance for Hauling
   void SetMaxGirderSweepHauling(Float64 sweep);

   //------------------------------------------------------------------------
   // Get the max allowable distance between hauling supports
   Float64 GetHaulingSupportDistance() const;

   //------------------------------------------------------------------------
   // Set the max allowable distance between hauling supports
   void SetHaulingSupportDistance(Float64 d);

   Float64 GetMaxHaulingOverhang() const;
   void SetMaxHaulingOverhang(double oh);

   //------------------------------------------------------------------------
   // Get the max lateral tolerance for hauling support placement
   Float64 GetHaulingSupportPlacementTolerance() const;

   //------------------------------------------------------------------------
   // Set the max lateral tolerance for hauling support placement
   void SetHaulingSupportPlacementTolerance(Float64 tol);

   //------------------------------------------------------------------------
   // Get the percent the radius of stability is to be increased for camber
   Float64 GetHaulingCamberPercentEstimate() const;

   //------------------------------------------------------------------------
   // Set the percent the radius of stability is to be increased for camber
   void SetHaulingCamberPercentEstimate(Float64 per);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for transporation
   // to the site as a factor times f'c
   Float64 GetHaulingCompStress() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for transporation
   // to the site as a factor times f'c
   void SetHaulingCompStress(Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   //  for Hauling
   Float64 GetMaxConcreteTensHauling() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // for Hauling
   void SetMaxConcreteTensHauling(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // for Hauling
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetAbsMaxConcreteTensHauling(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete for Hauling
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetAbsMaxConcreteTensHauling(bool doCheck, Float64 stress);

   //------------------------------------------------------------------------
   // Get minimum factor of safety against cracking 
   // during transporation to the bridge site
   Float64 GetHaulingCrackFs() const;

   //------------------------------------------------------------------------
   // Set minimum factor of safety against cracking 
   // during transporation to the bridge site
   void SetHaulingCrackFs(Float64 fs);

   //------------------------------------------------------------------------
   // Get minimum factor of safety against failure 
   // during transporation to the bridge site
   Float64 GetHaulingFailFs() const;

   //------------------------------------------------------------------------
   // Set minimum factor of safety against failure 
   // during transporation to the bridge site
   void SetHaulingFailFs(Float64 fs);

   int GetTruckRollStiffnessMethod() const;
   void SetTruckRollStiffnessMethod(int method);
   Float64 GetAxleWeightLimit() const;
   void SetAxleWeightLimit(double limit);
   Float64 GetAxleStiffness() const;
   void SetAxleStiffness(double stiffness);
   Float64 GetMinRollStiffness() const;
   void SetMinRollStiffness(double stiffness);

   //------------------------------------------------------------------------
   // Get truck roll stiffness
   Float64 GetTruckRollStiffness() const;

   //------------------------------------------------------------------------
   // Set truck roll stiffness
   void SetTruckRollStiffness(Float64 stiff);

   //------------------------------------------------------------------------
   // Get height of bottom of girder above roadway during transport
   Float64 GetTruckGirderHeight() const;

   //------------------------------------------------------------------------
   // Set height of bottom of girder above roadway during transport
   void SetTruckGirderHeight(Float64 height);

   //------------------------------------------------------------------------
   // Get height of truck roll center above roadway.
   Float64 GetTruckRollCenterHeight() const;

   //------------------------------------------------------------------------
   // Get height of truck roll center above roadway.
   void SetTruckRollCenterHeight(Float64 height);

   //------------------------------------------------------------------------
   // Get center-to-center distance between truck tires
   Float64 GetTruckAxleWidth() const;

   //------------------------------------------------------------------------
   // Set center-to-center distance between truck tires
   void SetTruckAxleWidth(Float64 dist);

   //------------------------------------------------------------------------
   // Get max expected roadway superelevation angle during hauling
   Float64 GetRoadwaySuperelevation() const;

   //------------------------------------------------------------------------
   // Set max expected roadway superelevation angle during hauling
   void SetRoadwaySuperelevation(Float64 dist);

   //------------------------------------------------------------------------
   // Get minimum factor of safety against cracking for erection
   Float64 GetErectionCrackFs() const;

   //------------------------------------------------------------------------
   // Set minimum factor of safety against cracking for erection
   void SetErectionCrackFs(Float64 fs);

   //------------------------------------------------------------------------
   // Get minimum factor of safety against failure for erection
   Float64 GetErectionFailFs() const;

   //------------------------------------------------------------------------
   // Set minimum factor of safety against failure for erection
   void SetErectionFailFs(Float64 fs);

   void SetHaulingModulusOfRuptureCoefficient(double fr);
   double GetHaulingModulusOfRuptureCoefficient() const;

   void SetLiftingModulusOfRuptureCoefficient(double fr);
   double GetLiftingModulusOfRuptureCoefficient() const;

   void SetMaxGirderWeight(Float64 wgt);
   Float64 GetMaxGirderWeight() const;

   //------------------------------------------------------------------------
   // Set/Get coefficient for max concrete stress when adequate mild rebar is provided
   Float64 GetCyMaxConcreteTensWithRebar() const;
   void SetCyMaxConcreteTensWithRebar(Float64 stress);
   Float64 GetMaxConcreteTensWithRebarLifting() const;
   void SetMaxConcreteTensWithRebarLifting(Float64 stress);
   Float64 GetMaxConcreteTensWithRebarHauling() const;
   void SetMaxConcreteTensWithRebarHauling(Float64 stress);



   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for service loading
   // at temporary strand removal as a factor times f'c
   Float64 GetTempStrandRemovalCompStress() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for  loading
   // at temporary strand removal as a factor times f'c
   void SetTempStrandRemovalCompStress(Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the temporary strand removal
   Float64 GetTempStrandRemovalMaxConcreteTens() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete at the temporary strand removal
   void SetTempStrandRemovalMaxConcreteTens(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // at the temporary strand removal
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetTempStrandRemovalAbsMaxConcreteTens(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete at the temporary strand removal
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetTempStrandRemovalAbsMaxConcreteTens(bool doCheck, Float64 stress);


   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for service loading
   // at bridge site 1 as a factor times f'c
   Float64 GetBs1CompStress() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for  loading
   // at bridge site 1 as a factor times f'c
   void SetBs1CompStress(Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // at the bridge site stage 1
   Float64 GetBs1MaxConcreteTens() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete at the bridge site stage 1
   void SetBs1MaxConcreteTens(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // at the bridge site stage 1
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetBs1AbsMaxConcreteTens(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete at the bridge site stage 1
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetBs1AbsMaxConcreteTens(bool doCheck, Float64 stress);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for  loading
   // at bridge site 2 as a factor times f'c
   Float64 GetBs2CompStress() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for  loading
   // at bridge site 2 as a factor times f'c
   void SetBs2CompStress(Float64 stress);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for service loading
   // at bridge site 3 as a factor times f'c
   Float64 GetBs3CompStressService() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for service loading
   // at bridge site 3 as a factor times f'c
   void SetBs3CompStressService(Float64 stress);

   //------------------------------------------------------------------------
   // Get the max allowable compressive concrete stress for the live load plus
   // one-half permanent loads at bridge site 3 as a factor times f'c 
   // This is also used for the Fatigue I limit state (lrfd 5.5.3.1)
   Float64 GetBs3CompStressService1A() const;

   //------------------------------------------------------------------------
   // Set the max allowable compressive concrete stress for the live load plus
   // one-half permanent loads at bridge site 3 as a factor times f'c
   void SetBs3CompStressService1A(Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // for normal exposure conditions at the bridge site stage 3
   Float64 GetBs3MaxConcreteTensNc() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete for normal exposure conditions at the bridge site stage 3
   void SetBs3MaxConcreteTensNc(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // for normal exposure conditions at the bridge site stage 3
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void GetBs3AbsMaxConcreteTensNc(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete for normal exposure conditions at the bridge site stage 3
   // If the bool is false, this check is not made and the stress value is undefined.
   void SetBs3AbsMaxConcreteTensNc(bool doCheck, Float64 stress);

   //------------------------------------------------------------------------
   // Get the factor * sqrt(f'c) to determine allowable tensile stress in concrete
   // for severe corrosive conditions  at the bridge site stage 3
   Float64 GetBs3MaxConcreteTensSc() const;

   //------------------------------------------------------------------------
   // Set the factor * sqrt(f'c) to determine allowable tensile stress in 
   // concrete for severe corrosive conditions
   void SetBs3MaxConcreteTensSc(Float64 stress);

   //------------------------------------------------------------------------
   // Get the absolute maximum allowable tensile stress in concrete
   // for severe corrosive conditions. If the bool is false, this check is not
   // made and the stress value is undefined.
   void GetBs3AbsMaxConcreteTensSc(bool* doCheck, Float64* stress) const;

   //------------------------------------------------------------------------
   // Set the absolute maximum allowable tensile stress in 
   // concrete for severe corrosive conditions.  
   // If the bool is false, this check is not made and the stress value is 
   // undefined.
   void SetBs3AbsMaxConcreteTensSc(bool doCheck, Float64 stress);

   // If true, over reinforced moment capacity computed per LRFD C5.7.3.3.1
   // otherwise computed by WSDOT method
   void SetBs3LRFDOverreinforcedMomentCapacity(bool bSet);
   bool GetBs3LRFDOverreinforcedMomentCapacity() const;

   //------------------------------------------------------------------------
   // Get the maximum number of girders that traffic barrier loads may be
   // distributed over.
   GirderIndexType GetMaxGirdersDistTrafficBarrier() const;

   //------------------------------------------------------------------------
   // Set the maximum number of girders that traffic barrier loads may be
   // distributed over.
   void SetMaxGirdersDistTrafficBarrier(GirderIndexType num);

   void SetTrafficBarrierDistibutionType(pgsTypes::TrafficBarrierDistribution tbd);
   pgsTypes::TrafficBarrierDistribution GetTrafficBarrierDistributionType() const;

   //------------------------------------------------------------------------
   // Get how overlay loads are distributed to each girder
   pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() const;

   //------------------------------------------------------------------------
   void SetCreepMethod(int method);

   //------------------------------------------------------------------------
   int GetCreepMethod() const;

   //------------------------------------------------------------------------
   void SetXferTime(double time);

   //------------------------------------------------------------------------
   Float64 GetXferTime() const;

   //------------------------------------------------------------------------
   // Returns the creep factor.
   Float64 GetCreepFactor() const;

   //------------------------------------------------------------------------
   // Sets the creep factor.
   void SetCreepFactor(Float64 cf);

   //------------------------------------------------------------------------
   // Returns the number of days from prestress release until temporary strand
   // removal (or diaphragm loading).
   Float64 GetCreepDuration1Min() const;
   Float64 GetCreepDuration1Max() const;

   //------------------------------------------------------------------------
   void SetCreepDuration1(Float64 min,Float64 max);

   //------------------------------------------------------------------------
   // Returns the number of days from prestress release until the slab
   // is acting composite with the girder.
   Float64 GetCreepDuration2Min() const;
   Float64 GetCreepDuration2Max() const;

   //------------------------------------------------------------------------
   void SetCreepDuration2(Float64 min,Float64 max);

   void SetTotalCreepDuration(Float64 duration);
   Float64 GetTotalCreepDuration() const;

   //------------------------------------------------------------------------
   // Returns the method for computing losses. The return value will be
   // one of the LOSSES_XXX constants
   int GetLossMethod() const;

   //------------------------------------------------------------------------
   // Sets the method for computing losses
   void SetLossMethod(int method);

   //------------------------------------------------------------------------
   // Returns the losses before prestress xfer for a lump sum method
   double GetBeforeXferLosses() const;
   void SetBeforeXferLosses(double loss);

   //------------------------------------------------------------------------
   // Returns the losses after prestress xfer for a lump sum method
   double GetAfterXferLosses() const;
   void SetAfterXferLosses(double loss);

   double GetLiftingLosses() const;
   void SetLiftingLosses(double loss);

   //------------------------------------------------------------------------
   // Returns the shipping losses for a lump sum method or for a method
   // that does not support computing losses at shipping.
   double GetShippingLosses() const;
   void SetShippingLosses(double loss);

   //------------------------------------------------------------------------
   // Set/get the time when shipping occurs. Used when shipping losses are 
   // computed by the LRFD refined method after LRFD 2005
   void SetShippingTime(double time);
   double GetShippingTime() const;

   //------------------------------------------------------------------------
   double GetBeforeTempStrandRemovalLosses() const;
   void SetBeforeTempStrandRemovalLosses(double loss);

   //------------------------------------------------------------------------
   double GetAfterTempStrandRemovalLosses() const;
   void SetAfterTempStrandRemovalLosses(double loss);

   //------------------------------------------------------------------------
   double GetAfterDeckPlacementLosses() const;
   void SetAfterDeckPlacementLosses(double loss);

   //------------------------------------------------------------------------
   // Returns the final losses for a lump sum method
   double GetFinalLosses() const;
   void SetFinalLosses(double loss);

   //------------------------------------------------------------------------
   // Returns the anchor set
   double GetAnchorSet() const;
   void SetAnchorSet(double dset);

   double GetWobbleFrictionCoefficient() const;
   void SetWobbleFrictionCoefficient(double K);

   double GetFrictionCoefficient() const;
   void SetFrictionCoefficient(double u);


   //------------------------------------------------------------------------
   // Returns a LLDF_XXXX constant for the live load distribution factor
   // calculation method
   Int16 GetLiveLoadDistributionMethod() const;

   //------------------------------------------------------------------------
   // Sets the live load distribution factor calculation method. Use on of the
   // LLDF_XXXX constants.
   void SetLiveLoadDistributionMethod(Int16 method);

   //------------------------------------------------------------------------
   // Returns a LRSH_XXXX constant for the method for determining 
   // longitudinal reinforcement shear capacity calculation method
   Int16 GetLongReinfShearMethod() const;

   //------------------------------------------------------------------------
   // Sets the method for determining longitudinal reinforcement shear capacity
   // Uses the LRSH_XXXX constants.
   void SetLongReinfShearMethod(Int16 method);


   void IgnoreRangeOfApplicabilityRequirements(bool bIgnore);
   bool IgnoreRangeOfApplicabilityRequirements() const;

   void CheckStrandStress(UINT stage,bool bCheck);
   bool CheckStrandStress(UINT stage) const;
   void SetStrandStressCoefficient(UINT stage,UINT strandType, double coeff);
   double GetStrandStressCoefficient(UINT stage,UINT strandType) const;

   //------------------------------------------------------------------------
   // Determine if we want to evaluate deflection due to live load ala LRFD 2.5.2.5.2
   bool GetDoEvaluateLLDeflection() const;
   void SetDoEvaluateLLDeflection(bool doit);

   //------------------------------------------------------------------------
   // Set/Get span deflection limit criteria. Limit is Span / value.
   double GetLLDeflectionLimit() const;
   void SetLLDeflectionLimit(double limit);

//  All debonding criteria was moved to the girder library for version 27
// 
//   void SetMaxDebondStrands(Float64 max);
//   Float64 GetMaxDebondStrands() const;
//   void SetMaxDebondStrandsPerRow(Float64 max);
//   Float64 GetMaxDebondStrandsPerRow() const;
//   void SetMaxDebondStrandsPerSection(long nMax,double fMax);
//   void GetMaxDebondStrandsPerSection(long* nMax,double* fMax) const;
//   void SetDefaultDebondLength(Float64 l);
//   Float64 GetDefaultDebondLength() const;

   void IncludeRebarForMoment(bool bInclude);
   bool IncludeRebarForMoment() const;

   void IncludeRebarForShear(bool bInclude);
   bool IncludeRebarForShear() const;

   pgsTypes::AnalysisType GetAnalysisType() const;

   void SetFlexureModulusOfRuptureCoefficient(double fr);
   double GetFlexureModulusOfRuptureCoefficient() const;
   
   void SetShearModulusOfRuptureCoefficient(double fr);
   double GetShearModulusOfRuptureCoefficient() const;

   void SetMaxSlabFc(double fc);
   double GetMaxSlabFc() const;
   void SetMaxGirderFc(double fc);
   double GetMaxGirderFc() const;
   void SetMaxGirderFci(double fci);
   double GetMaxGirderFci() const;
   void SetMaxConcreteUnitWeight(double wc);
   double GetMaxConcreteUnitWeight() const;
   void SetMaxConcreteAggSize(double agg);
   double GetMaxConcreteAggSize() const;

   void GetDCLoadFactors(pgsTypes::LimitState ls,double* pDCmin,double* pDCmax) const;
   void GetDWLoadFactors(pgsTypes::LimitState ls,double* pDWmin,double* pDWmax) const;
   void GetLLIMLoadFactors(pgsTypes::LimitState ls,double* pLLIMmin,double* pLLIMmax) const;
   
   void SetDCLoadFactors(pgsTypes::LimitState ls,double DCmin,double DCmax);
   void SetDWLoadFactors(pgsTypes::LimitState ls,double DWmin,double DWmax);
   void SetLLIMLoadFactors(pgsTypes::LimitState ls,double LLIMmin,double LLIMmax);

   //------------------------------------------------------------------------
   // Enable check and design for "A" dimension (Slab Offset
   void EnableSlabOffsetCheck(bool enable);
   bool IsSlabOffsetCheckEnabled() const;

   void EnableSlabOffsetDesign(bool enable);
   bool IsSlabOffsetDesignEnabled() const;

   void SetDesignStrandFillType(arDesignStrandFillType type);
   arDesignStrandFillType GetDesignStrandFillType() const;

   void SetEffectiveFlangeWidthMethod(pgsTypes::EffectiveFlangeWidthMethod efwMethod);
   pgsTypes::EffectiveFlangeWidthMethod GetEffectiveFlangeWidthMethod() const;

   void SetShearFlowMethod(ShearFlowMethod method);
   ShearFlowMethod GetShearFlowMethod() const;

   void SetShearCapacityMethod(ShearCapacityMethod method);
   ShearCapacityMethod GetShearCapacityMethod() const;

   void SetCuringMethodTimeAdjustmentFactor(double f);
   double GetCuringMethodTimeAdjustmentFactor() const;

   void SetMininumTruckSupportLocation(double x);
   double GetMininumTruckSupportLocation() const;
   void SetTruckSupportLocationAccuracy(double x);
   double GetTruckSupportLocationAccuracy() const;

   void SetMininumLiftingPointLocation(double x);
   double GetMininumLiftingPointLocation() const;
   void SetLiftingPointLocationAccuracy(double x);
   double GetLiftingPointLocationAccuracy() const;

   void SetPedestrianLiveLoad(double w);
   double GetPedestrianLiveLoad() const;
   void SetMinSidewalkWidth(double Wmin);
   double GetMinSidewalkWidth() const;

   void SetMaxAngularDeviationBetweenGirders(double angle);
   double GetMaxAngularDeviationBetweenGirders() const;
   void SetMinGirderStiffnessRatio(double r);
   double GetMinGirderStiffnessRatio() const;
   void SetLLDFGirderSpacingLocation(double fra);
   double GetLLDFGirderSpacingLocation() const;

   // impose a lower limit on distribution factors?
   void LimitDistributionFactorsToLanesBeams(bool bInclude);
   bool LimitDistributionFactorsToLanesBeams() const;

   // method for computing transfer length
   pgsTypes::PrestressTransferComputationType GetPrestressTransferComputationType() const;
   void SetPrestressTransferComputationType(pgsTypes::PrestressTransferComputationType type);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const SpecLibraryEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const SpecLibraryEntry& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   // general
   lrfdVersionMgr::Version m_SpecificationType;
   lrfdVersionMgr::Units m_SpecificationUnits;
   std::string m_Description;

   // casting yard
   bool    m_DoCheckStrandSlope;
   bool    m_DoDesignStrandSlope;
   Float64 m_MaxSlope05;
   Float64 m_MaxSlope06;
   Float64 m_MaxSlope07;

   bool    m_DoCheckHoldDown;
   bool    m_DoDesignHoldDown;
   Float64 m_HoldDownForce;
   Float64 m_MaxStirrupSpacing;

   bool    m_DoCheckAnchorage; // 5.10.10

   Float64 m_CyLiftingCrackFs;
   Float64 m_CyLiftingFailFs;
   Float64 m_CyCompStressServ;
   Float64 m_CyCompStressLifting;

   Float64 m_CyTensStressServ;
   bool    m_CyDoTensStressServMax;
   Float64 m_CyTensStressServMax;

   Float64 m_CyTensStressServWithRebar;
   Float64 m_TensStressLiftingWithRebar;
   Float64 m_TensStressHaulingWithRebar;

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

   Float64 m_SplittingZoneLengthFactor;

   // hauling
   bool    m_EnableHaulingCheck;
   bool    m_EnableHaulingDesign;
   Float64 m_HeHaulingCrackFs;
   Float64 m_HeHaulingRollFs;
   
   int     m_TruckRollStiffnessMethod;
   Float64 m_TruckRollStiffness;
   Float64 m_AxleWeightLimit;
   Float64 m_AxleStiffness;
   Float64 m_MinRollStiffness;

   Float64 m_TruckGirderHeight;
   Float64 m_TruckRollCenterHeight;
   Float64 m_TruckAxleWidth;
   Float64 m_HeErectionCrackFs;
   Float64 m_HeErectionFailFs;
   Float64 m_RoadwaySuperelevation;
   Float64 m_MaxGirderSweepHauling;
   Float64 m_HaulingSupportDistance;
   Float64 m_HaulingSupportPlacementTolerance;
   Float64 m_HaulingCamberPercentEstimate;
   Float64 m_HaulingUpwardImpact;
   Float64 m_HaulingDownwardImpact;

   Float64 m_CompStressHauling;
   Float64 m_TensStressHauling;
   bool    m_DoTensStressHaulingMax;
   Float64 m_TensStressHaulingMax;

   Float64 m_MaxHaulingOverhang;
   Float64 m_MaxGirderWgt;

   double  m_HaulingModulusOfRuptureCoefficient;
   double  m_LiftingModulusOfRuptureCoefficient;

   double m_MinLiftPoint;
   double m_LiftPointAccuracy;
   double m_MinHaulPoint;
   double m_HaulPointAccuracy;


   // temporary strand removal
   Float64 m_TempStrandRemovalCompStress;
   Float64 m_TempStrandRemovalTensStress;
   bool    m_TempStrandRemovalDoTensStressMax;
   Float64 m_TempStrandRemovalTensStressMax;

   // bridge site 1
   Float64 m_Bs1CompStress;
   Float64 m_Bs1TensStress;
   bool    m_Bs1DoTensStressMax;
   Float64 m_Bs1TensStressMax;

   // bridge site 2
   Float64 m_Bs2CompStress;
   pgsTypes::TrafficBarrierDistribution m_TrafficBarrierDistributionType;
   GirderIndexType  m_Bs2MaxGirdersTrafficBarrier;
   GirderIndexType  m_Bs2MaxGirdersUtility;
   pgsTypes::OverlayLoadDistributionType m_OverlayLoadDistribution;

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
   double  m_FlexureModulusOfRuptureCoefficient;
   double  m_ShearModulusOfRuptureCoefficient;

   // Creep
   int     m_CreepMethod;
   Float64 m_XferTime;
   Float64 m_CreepFactor;
   Float64 m_CreepDuration1Min;
   Float64 m_CreepDuration1Max;
   Float64 m_CreepDuration2Min;
   Float64 m_CreepDuration2Max;
   Float64 m_TotalCreepDuration;

   // Losses
   int    m_LossMethod;
   double m_FinalLosses;
   double m_LiftingLosses;
   double m_ShippingLosses;  // if between -1.0 and 0, shipping loss is fraction of final loss. Fraction is abs(m_ShippingLoss)
   double m_BeforeXferLosses;
   double m_AfterXferLosses;
   double m_ShippingTime;
   double m_BeforeTempStrandRemovalLosses;
   double m_AfterTempStrandRemovalLosses;
   double m_AfterDeckPlacementLosses;

   double m_Dset; // anchor set
   double m_WobbleFriction; // wobble friction, K
   double m_FrictionCoefficient; // mu

   // Live Load Distribution Factors
   int m_LldfMethod;

   // Longitudinal reinforcement shear capacity
   int m_LongReinfShearMethod;
   bool m_bIncludeRebar_Shear;

   // Strand stress coefficients
   bool m_bCheckStrandStress[4];
   double m_StrandStressCoeff[4][2];

   // live load deflection
   bool m_bDoEvaluateDeflection;
   double m_DeflectionLimit;

   pgsTypes::AnalysisType m_AnalysisType; // this data will be in old library entries (version < 28)

   double m_MaxSlabFc;
   double m_MaxGirderFci;
   double m_MaxGirderFc;
   double m_MaxConcreteUnitWeight;
   double m_MaxConcreteAggSize;

   double m_DCmin[6];   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   double m_DWmin[6];
   double m_LLIMmin[6];
   double m_DCmax[6];
   double m_DWmax[6];
   double m_LLIMmax[6];
   
   bool m_EnableSlabOffsetCheck;
   bool m_EnableSlabOffsetDesign;

   arDesignStrandFillType m_DesignStrandFillType;
   pgsTypes::EffectiveFlangeWidthMethod m_EffFlangeWidthMethod;

   ShearFlowMethod m_ShearFlowMethod;

   ShearCapacityMethod m_ShearCapacityMethod;

   double m_CuringMethodTimeAdjustmentFactor;

   double m_MinSidewalkWidth; // sidewalk must be greater that this width for ped load to apply
   double m_PedestrianLoad; // magnitude of pedestrian load (F/L^2)
   
   // "fudge" factors for computing live load distribution factors
   double m_MaxAngularDeviationBetweenGirders; // maximum angle between girders in order to consider them "parallel"
   double m_MinGirderStiffnessRatio; // minimum allowable value for EImin/EImax in order to consider girders to have "approximately the same stiffness"
   double m_LLDFGirderSpacingLocation; // fractional location in the girder span where the girder spacing is measured
                                       // for purposes of computing live load distribution factors

   bool m_LimitDistributionFactorsToLanesBeams; 

   pgsTypes::PrestressTransferComputationType m_PrestressTransferComputationType;
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PSGLIB_SPECLIBRARYENTRY_H_
