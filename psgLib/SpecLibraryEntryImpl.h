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

// SYSTEM INCLUDES
//
#include <PGSuperTypes.h>

// PROJECT INCLUDES
//

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <psgLib/RefactoredSpecLibraryParameters.h>

#include <System\SubjectT.h>

#include <type_traits>

#include <psgLib/SpecificationCriteria.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/HoldDownCriteria.h>
#include <psgLib/StrandSlopeCriteria.h>
#include <psgLib/PlantHandlingCriteria.h>
#include <psgLib/EndZoneCriteria.h>
#include <psgLib/SlabOffsetCriteria.h>
#include <psgLib/LiveLoadDeflectionCriteria.h>
#include <psgLib/BottomFlangeClearanceCriteria.h>
#include <psgLib/GirderInclinationCriteria.h>
#include <psgLib/PrincipalTensionStressCriteria.h>
#include <psgLib/BearingCriteria.h>
#include <psgLib/LiveLoadCriteria.h>
#include <psgLib/LiveLoadDistributionCriteria.h>
#include <psgLib/DuctSizeCriteria.h>
#include <psgLib/TransferLengthCriteria.h>
#include <psgLib/StrandStressCriteria.h>
#include <psgLib/TendonStressCriteria.h>
#include <psgLib/DeadLoadDistributionCriteria.h>
#include <psgLib/HaunchCriteria.h>
#include <psgLib/CreepCriteria.h>
#include <psgLib/InterfaceShearCriteria.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/MomentCapacityCriteria.h>
#include <psgLib/LiftingCriteria.h>
#include <psgLib/HaulingCriteria.h>
#include <psgLib/LimitsCriteria.h>
#include <psgLib/ClosureJointCriteria.h>
#include <psgLib/LimitStateConcreteStrengthCriteria.h>
#include <psgLib/HarpedStrandDesignCriteria.h>
#include <psgLib/PrestressedElementCriteria.h>
#include <psgLib/PrestressLossCriteria.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsLibraryEntryDifferenceItem;
class CSpecMainSheet;


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

/// @brief Implementation class for SpecLibraryEntry
/// This is an implementation of the Pimpl idiom
class SpecLibraryEntryImpl
{
   // the dialog is our friend.
   friend CSpecMainSheet;
public:
   SpecLibraryEntryImpl();
   SpecLibraryEntryImpl(const SpecLibraryEntryImpl& rOther) = default;
   virtual ~SpecLibraryEntryImpl() = default;

   SpecLibraryEntryImpl& operator=(const SpecLibraryEntryImpl& rOther) = default;

   //////////////////////////////////////
   // General
   //////////////////////////////////////

   // Save to structured storage
   bool SaveMe(WBFL::Library::LibraryEntry* pParent,WBFL::System::IStructuredSave* pSave);

   // Load from structured storage
   bool LoadMe(WBFL::Library::LibraryEntry* pParent,WBFL::System::IStructuredLoad* pLoad);

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const SpecLibraryEntryImpl* pOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   //////////////////////////////////////
   //
   // General Specification Properties
   //
   //////////////////////////////////////

   const SpecificationCriteria& GetSpecificationCriteria() const;
   void SetSpecificationCriteria(const SpecificationCriteria& criteria);

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

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

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

   SpecificationCriteria m_SpecificationCriteria;
   SectionPropertiesCriteria m_SectionPropertiesCriteria;
   LimitStateConcreteStrengthCriteria m_LimitStateConcreteStrengthCriteria;
   StrandSlopeCriteria m_StrandSlopeCriteria;
   HoldDownCriteria m_HoldDownCriteria;
   PlantHandlingCriteria m_PlantHandlingCriteria;
   EndZoneCriteria m_EndZoneCriteria;
   HarpedStrandDesignCriteria m_HarpedStrandDesignCriteria;
   PrestressedElementCriteria m_PrestressedElementCriteria;
   LiftingCriteria m_LiftingCriteria;
   HaulingCriteria m_HaulingCriteria;
   PrincipalTensionStressCriteria m_PrincipalTensionStressCriteria;
   ClosureJointCriteria m_ClosureJointCriteria;
   CreepCriteria m_CreepCriteria;
   PrestressLossCriteria m_PrestressLossCriteria;
   LiveLoadDistributionCriteria m_LiveLoadDistributionCriteria;
   DeadLoadDistributionCriteria m_DeadLoadDistributionCriteria;
   HaunchCriteria m_HaunchCriteria;
   StrandStressCriteria m_StrandStressCriteria;
   TendonStressCriteria m_TendonStressCriteria;
   DuctSizeCriteria m_DuctSizeCriteria;
   LiveLoadDeflectionCriteria m_LiveLoadDeflectionCriteria;
   LimitsCriteria m_LimitsCriteria;
   SlabOffsetCriteria m_SlabOffsetCriteria;
   InterfaceShearCriteria m_InterfaceShearCriteria;
   LiveLoadCriteria m_LiveLoadCriteria;
   TransferLengthCriteria m_TransferLengthCriteria;
   BottomFlangeClearanceCriteria m_BottomFlangeClearanceCriteria;
   GirderInclinationCriteria m_GirderInclinationCriteria;
   ShearCapacityCriteria m_ShearCapacityCriteria;
   MomentCapacityCriteria m_MomentCapacityCriteria;
   BearingCriteria m_BearingCriteria;


   //
   // Data and methods for reading legacy data blocks (pre version 83)
   //
   bool LegacyLoadMe(WBFL::Library::LibraryEntry* pParent, WBFL::System::IStructuredLoad* pLoad); // loads pre-version 83 data
   void DeterminePrincipalStressDuctDeductionMultiplier(); // used when loading legacy data for principal tension stresses

   // Manages refactored data
   RefactoredSpecLibraryParameters m_RefactoredParameters;
};
