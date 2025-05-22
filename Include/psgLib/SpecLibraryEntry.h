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

#include "PsgLibLib.h"

#include <PsgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <System\SubjectT.h>

#include <type_traits>

// forward declarations
class SpecLibraryEntryImpl;
class IEAFDisplayUnits;
class RefactoredSpecLibraryParameters;
class rptChapter;

struct SpecificationCriteria;
struct SectionPropertiesCriteria;
struct ThermalMovementCriteria;
struct LimitStateConcreteStrengthCriteria;
struct StrandSlopeCriteria;
struct HoldDownCriteria;
struct PlantHandlingCriteria;
struct EndZoneCriteria;
struct HarpedStrandDesignCriteria;
struct PrestressedElementCriteria;
struct LiftingCriteria;
//struct HaulingCriteria;
struct PrincipalTensionStressCriteria;
struct ClosureJointCriteria;
struct CreepCriteria;
struct PrestressLossCriteria;
struct LiveLoadDistributionCriteria;
struct DeadLoadDistributionCriteria;
struct HaunchCriteria;
struct StrandStressCriteria;
struct TendonStressCriteria;
struct DuctSizeCriteria;
struct LiveLoadDeflectionCriteria;
struct LimitsCriteria;
//struct SlabOffsetCriteria;
struct InterfaceShearCriteria;
struct LiveLoadCriteria;
struct TransferLengthCriteria;
struct BottomFlangeClearanceCriteria;
struct GirderInclinationCriteria;
struct ShearCapacityCriteria;
struct MomentCapacityCriteria;
struct BearingCriteria;

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CSpecMainSheet;
class SpecLibraryEntry;
class SpecLibraryEntryObserver;
namespace PGS {namespace Library{class DifferenceItem;};};
struct SlabOffsetCriteria;
struct HaulingCriteria;

#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>;


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
   prestressed girder design.

   This class is implemented using the PIMPL idiom to minimize recompling
   when the individual criteria items changes. Only those parts of code
   that depend on a specific type of criteria will recompile when that
   criteria definition changes

LOG
   rdp : 09.17.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS SpecLibraryEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>
{
   // the dialog is our friend.
   friend CSpecMainSheet;

public:
   SpecLibraryEntry();
   SpecLibraryEntry(const SpecLibraryEntry& rOther);
   virtual ~SpecLibraryEntry();

   SpecLibraryEntry& operator=(const SpecLibraryEntry& rOther);

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
   bool Compare(const SpecLibraryEntry& rOther, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const SpecLibraryEntry& rOther,bool bConsiderName=false) const;

   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //////////////////////////////////////
   //
   // General Specification Properties
   //
   //////////////////////////////////////

   const SpecificationCriteria& GetSpecificationCriteria() const;
   void SetSpecificationCriteria(const SpecificationCriteria& criteria);

   const ThermalMovementCriteria& GetThermalMovementCriteria() const;
   void SetThermalMovementCriteria(const ThermalMovementCriteria& criteria);

   const SectionPropertiesCriteria& GetSectionPropertiesCriteria() const;
   void SetSectionPropertiesCriteria(const SectionPropertiesCriteria& criteria);

   //////////////////////////////////////
   //
   // Design and Spec Checking
   //
   //////////////////////////////////////

   const StrandSlopeCriteria& GetStrandSlopeCriteria() const;
   void SetStrandSlopeCriteria(const StrandSlopeCriteria& criteria);

   const HoldDownCriteria& GetHoldDownCriteria() const;
   void SetHoldDownCriteria(const HoldDownCriteria& criteria);

   const PlantHandlingCriteria& GetPlantHandlingCriteria() const;
   void SetPlantHandlingCriteria(const PlantHandlingCriteria& criteria);

   const EndZoneCriteria& GetEndZoneCriteria() const;
   void SetEndZoneCriteria(const EndZoneCriteria& criteria);

   const SlabOffsetCriteria& GetSlabOffsetCriteria() const;
   void SetSlabOffsetCriteria(const SlabOffsetCriteria& criteria);

   const LiftingCriteria& GetLiftingCriteria() const;
   void SetLiftingCriteria(const LiftingCriteria& criteria);

   const HaulingCriteria& GetHaulingCriteria() const;
   void SetHaulingCriteria(const HaulingCriteria& criteria);

   const LiveLoadDeflectionCriteria& GetLiveLoadDeflectionCriteria() const;
   void SetLiveLoadDeflectionCriteria(const LiveLoadDeflectionCriteria& criteria);

   const BottomFlangeClearanceCriteria& GetBottomFlangeClearanceCriteria() const;
   void SetBottomFlangeClearanceCriteria(const BottomFlangeClearanceCriteria& criteria);

   const GirderInclinationCriteria& GetGirderInclinationCriteria() const;
   void SetGirderInclinationCriteria(const GirderInclinationCriteria& criteria);

   const HarpedStrandDesignCriteria& GetHarpedStrandDesignCriteria() const;
   void SetHarpedStrandDesignCriteria(const HarpedStrandDesignCriteria& criteria);

   const LimitStateConcreteStrengthCriteria& GetLimitStateConcreteStrengthCriteria() const;
   void SetLimitStateConcreteStrengthCriteria(const LimitStateConcreteStrengthCriteria& criteria);

   //////////////////////////////////////
   //
   // Precast Elements
   //
   //////////////////////////////////////

   const PrestressedElementCriteria& GetPrestressedElementCriteria() const;
   void SetPrestressedElementCriteria(const PrestressedElementCriteria& criteria);

   const PrincipalTensionStressCriteria& GetPrincipalTensionStressCriteria() const;
   void SetPrincipalTensionStressCriteria(const PrincipalTensionStressCriteria& criteria);

   //////////////////////////////////////
   //
   // Closure Joints
   //
   //////////////////////////////////////
    
   const ClosureJointCriteria& GetClosureJointCriteria() const;
   void SetClosureJointCriteria(const ClosureJointCriteria& criteria);


   //////////////////////////////////////
   //
   // Strands
   //
   //////////////////////////////////////

   const StrandStressCriteria& GetStrandStressCriteria() const;
   void SetStrandStressCriteria(const StrandStressCriteria& criteria);

   const TendonStressCriteria& GetTendonStressCriteria() const;
   void SetTendonStressCriteria(const TendonStressCriteria& criteria);

   const TransferLengthCriteria& GetTransferLengthCriteria() const;
   void SetTransferLengthCriteria(const TransferLengthCriteria& criteria);

   const DuctSizeCriteria& GetDuctSizeCriteria() const;
   void SetDuctSizeCriteria(const DuctSizeCriteria& criteria);

   //////////////////////////////////////
   //
   // Loads
   //
   //////////////////////////////////////

   const LiveLoadCriteria& GetLiveLoadCriteria() const;
   void SetLiveLoadCriteria(const LiveLoadCriteria& criteria);

   const LiveLoadDistributionCriteria& GetLiveLoadDistributionCriteria() const;
   void SetLiveLoadDistributionCriteria(const LiveLoadDistributionCriteria& criteria);

   const DeadLoadDistributionCriteria& GetDeadLoadDistributionCriteria() const;
   void SetDeadLoadDistributionCriteria(const DeadLoadDistributionCriteria& criteria);

   const HaunchCriteria& GetHaunchCriteria() const;
   void SetHaunchCriteria(const HaunchCriteria& criteria);

   //////////////////////////////////////
   //
   // Moment Capacity Parameters
   //
   //////////////////////////////////////
   
   const MomentCapacityCriteria& GetMomentCapacityCriteria() const;
   void SetMomentCapacityCriteria(const MomentCapacityCriteria& criteria);

   //////////////////////////////////////
   //
   // Shear Capacity Parameters
   //
   //////////////////////////////////////
   
   const ShearCapacityCriteria& GetShearCapacityCriteria() const;
   void SetShearCapacityCriteria(const ShearCapacityCriteria& criteria);

   const InterfaceShearCriteria& GetInterfaceShearCriteria() const;
   void SetInterfaceShearCriteria(const InterfaceShearCriteria& criteria);

   //////////////////////////////////////
   //
   // Creep and Camber Parameters
   //
   //////////////////////////////////////

   const CreepCriteria& GetCreepCriteria() const;
   void SetCreepCriteria(const CreepCriteria& criteria);

   //////////////////////////////////////
   //
   // Prestress Loss Parameters
   //
   //////////////////////////////////////

   const PrestressLossCriteria& GetPrestressLossCriteria() const;
   void SetPrestressLossCriteria(const PrestressLossCriteria& criteria);

   //////////////////////////////////////
   //
   // Limits Parameters
   //
   //////////////////////////////////////

   const LimitsCriteria& GetLimitsCriteria() const;
   void SetLimitsCriteria(const LimitsCriteria& criteria);

   //////////////////////////////////////
   //
   // Bearings Parameters
   //
   //////////////////////////////////////
   const BearingCriteria& GetBearingCriteria() const;
   void SetBearingCriteria(const BearingCriteria& criteria);

   void Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   ////////////////////////////////////////
   //
   // Legacy Parameters
   //
   ////////////////////////////////////////

   // Returns parameters that were once in an older version of this object.
   // These methods are obsolete and should only be used by the Project Agent
   // to get data the was at one time in the library and is now part of the
   // project data

   /// @brief Returns an object that represents parameters that where once part of this library entry
   /// but have since been refactored into a different part of the program (typically these parameters
   /// become user input). The ProjectAgent evaluates the returned object and updates user input
   /// parameters as needed
   const RefactoredSpecLibraryParameters& GetRefactoredParameters() const;

private:
   std::unique_ptr<SpecLibraryEntryImpl> m_pImpl;
};
