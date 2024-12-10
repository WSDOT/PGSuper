///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// *this program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// *this program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with *this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SpecLibraryEntryImpl.h"

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>
#include <Units\Convert.h>

#include <MathEx.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

// During the development of PGSplice, there was an overlap in version numbers between the
// 2.9 and 3.0 branches. It is ok for loads to fail for 44.0 <= version <= MAX_OVERLAP_VERSION.
#define MAX_OVERLAP_VERSION 53.0 // overlap of data blocks between PGS 2.9 and 3.0 end with *this version

// The develop (patches) branch started at version 64. We need to make room so
// the version number can increment. Jump our version number to 70.
// The version number has progressed past version 70
#define CURRENT_VERSION 84.0 

/****************************************************************************
CLASS
   SpecLibraryEntryImpl
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
SpecLibraryEntryImpl::SpecLibraryEntryImpl()
{
}

bool SpecLibraryEntryImpl::SaveMe(WBFL::Library::LibraryEntry* pParent,WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("SpecificationLibraryEntry"), CURRENT_VERSION);
   pSave->Property(_T("Name"),pParent->GetName().c_str());
   
   // Starting with SpecificationLibraryEntry data block unit 83, 
   // all of the project criteria data is now modeled in "Criteria" objects.
   m_SpecificationCriteria.Save(pSave);
   m_SectionPropertiesCriteria.Save(pSave);
   m_ThermalMovementCriteria.Save(pSave);
   m_HoldDownCriteria.Save(pSave);
   m_StrandSlopeCriteria.Save(pSave);
   m_EndZoneCriteria.Save(pSave);
   m_PlantHandlingCriteria.Save(pSave);
   m_LiveLoadDeflectionCriteria.Save(pSave);
   m_BottomFlangeClearanceCriteria.Save(pSave);
   m_GirderInclinationCriteria.Save(pSave);
   m_SlabOffsetCriteria.Save(pSave);
   m_HarpedStrandDesignCriteria.Save(pSave);
   m_LimitStateConcreteStrengthCriteria.Save(pSave);
   m_PrestressedElementCriteria.Save(pSave);
   m_PrincipalTensionStressCriteria.Save(pSave);
   m_ClosureJointCriteria.Save(pSave);
   m_StrandStressCriteria.Save(pSave);
   m_TendonStressCriteria.Save(pSave);
   m_TransferLengthCriteria.Save(pSave);
   m_DuctSizeCriteria.Save(pSave);
   m_LiftingCriteria.Save(pSave);
   m_HaulingCriteria.Save(pSave);
   m_DeadLoadDistributionCriteria.Save(pSave);
   m_LiveLoadCriteria.Save(pSave);
   m_LiveLoadDistributionCriteria.Save(pSave);
   m_MomentCapacityCriteria.Save(pSave);
   m_ShearCapacityCriteria.Save(pSave);
   m_InterfaceShearCriteria.Save(pSave);
   m_CreepCriteria.Save(pSave);
   m_HaunchCriteria.Save(pSave);
   m_PrestressLossCriteria.Save(pSave);
   m_LimitsCriteria.Save(pSave);
   m_BearingCriteria.Save(pSave);

   pSave->EndUnit();

   return true;
}

bool SpecLibraryEntryImpl::LoadMe(WBFL::Library::LibraryEntry* pParent,WBFL::System::IStructuredLoad* pLoad)
{
   // *this data block was completely changed in version 83
   // Version 83 introduced the "Criteria" structs that encapsulates the various
   // criteria that make up the ProjectCriteria. Pre-version 83 loads are handled
   // with the LegacyLoadMe function.

   if(pLoad->BeginUnit(_T("SpecificationLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (version < 1.0 || CURRENT_VERSION < version)
      {
         THROW_LOAD(BadVersion,pLoad);
      }

      if (version < 83)
      {
         LegacyLoadMe(pParent,pLoad);
      }
      else
      {
         std::_tstring name;
         if(pLoad->Property(_T("Name"),&name))
         {
            pParent->SetName(name.c_str());
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_SpecificationCriteria.Load(pLoad);
         m_SectionPropertiesCriteria.Load(pLoad);
         if (version > 83)
         {
             m_ThermalMovementCriteria.Load(pLoad);
         }
         m_HoldDownCriteria.Load(pLoad);
         m_StrandSlopeCriteria.Load(pLoad);
         m_EndZoneCriteria.Load(pLoad);
         m_PlantHandlingCriteria.Load(pLoad);
         m_LiveLoadDeflectionCriteria.Load(pLoad);
         m_BottomFlangeClearanceCriteria.Load(pLoad);
         m_GirderInclinationCriteria.Load(pLoad);
         m_SlabOffsetCriteria.Load(pLoad);
         m_HarpedStrandDesignCriteria.Load(pLoad);
         m_LimitStateConcreteStrengthCriteria.Load(pLoad);
         m_PrestressedElementCriteria.Load(pLoad);
         m_PrincipalTensionStressCriteria.Load(pLoad);
         m_ClosureJointCriteria.Load(pLoad);
         m_StrandStressCriteria.Load(pLoad);
         m_TendonStressCriteria.Load(pLoad);
         m_TransferLengthCriteria.Load(pLoad);
         m_DuctSizeCriteria.Load(pLoad);
         m_LiftingCriteria.Load(pLoad);
         m_HaulingCriteria.Load(pLoad);
         m_DeadLoadDistributionCriteria.Load(pLoad);
         m_LiveLoadCriteria.Load(pLoad);
         m_LiveLoadDistributionCriteria.Load(pLoad);
         m_MomentCapacityCriteria.Load(pLoad);
         m_ShearCapacityCriteria.Load(pLoad);
         m_InterfaceShearCriteria.Load(pLoad);
         m_CreepCriteria.Load(pLoad);
         m_HaunchCriteria.Load(pLoad);
         m_PrestressLossCriteria.Load(pLoad);
         m_LimitsCriteria.Load(pLoad);
         m_BearingCriteria.Load(pLoad);

         if(!pLoad->EndUnit())
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
   }

   // sometimes the user may toggle between several loss methods. if they select time step and change the time dependent model
   // and then change to a different method, the time dependent model is saved in its last state. *this makes the default time dependent
   // model be the last used value. Change it to the true default value here
   if (m_PrestressLossCriteria.LossMethod != PrestressLossCriteria::LossMethodType::TIME_STEP)
   {
      m_PrestressLossCriteria.TimeDependentConcreteModel = PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO;
   }

   return true;
}

bool SpecLibraryEntryImpl::Compare(const SpecLibraryEntryImpl* pOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   if(!m_SpecificationCriteria.Compare(pOther->m_SpecificationCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_ThermalMovementCriteria.Compare(pOther->m_ThermalMovementCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_HoldDownCriteria.Compare(pOther->m_HoldDownCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_StrandSlopeCriteria.Compare(pOther->m_StrandSlopeCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_PlantHandlingCriteria.Compare(pOther->m_PlantHandlingCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_EndZoneCriteria.Compare(pOther->m_EndZoneCriteria, *this, vDifferences, bReturnOnFirstDifference) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_LiftingCriteria.Compare(pOther->m_LiftingCriteria, *this, vDifferences, bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_HaulingCriteria.Compare(pOther->m_HaulingCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_SlabOffsetCriteria.Compare(pOther->m_SlabOffsetCriteria,*this,vDifferences,bReturnOnFirstDifference) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_LiveLoadDeflectionCriteria.Compare(pOther->m_LiveLoadDeflectionCriteria,*this,vDifferences, bReturnOnFirstDifference) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_BottomFlangeClearanceCriteria.Compare(pOther->m_BottomFlangeClearanceCriteria, *this,vDifferences, bReturnOnFirstDifference) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_GirderInclinationCriteria.Compare(pOther->m_GirderInclinationCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_HarpedStrandDesignCriteria.Compare(pOther->m_HarpedStrandDesignCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_LimitStateConcreteStrengthCriteria.Compare(pOther->m_LimitStateConcreteStrengthCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_PrestressedElementCriteria.Compare(pOther->m_PrestressedElementCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_PrincipalTensionStressCriteria.Compare(pOther->m_PrincipalTensionStressCriteria, *this,vDifferences, bReturnOnFirstDifference) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_ClosureJointCriteria.Compare(pOther->m_ClosureJointCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if ( !m_StrandStressCriteria.Compare(pOther->m_StrandStressCriteria, *this,vDifferences, bReturnOnFirstDifference ) )
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_TendonStressCriteria.Compare(pOther->m_TendonStressCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_TransferLengthCriteria.Compare(pOther->m_TransferLengthCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_DuctSizeCriteria.Compare(pOther->m_DuctSizeCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_DeadLoadDistributionCriteria.Compare(pOther->m_DeadLoadDistributionCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_LiveLoadDistributionCriteria.Compare(pOther->m_LiveLoadDistributionCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_LiveLoadCriteria.Compare(pOther->m_LiveLoadCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_MomentCapacityCriteria.Compare(pOther->m_MomentCapacityCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_ShearCapacityCriteria.Compare(pOther->m_ShearCapacityCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }
   
   if(!m_InterfaceShearCriteria.Compare(pOther->m_InterfaceShearCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_CreepCriteria.Compare(pOther->m_CreepCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_PrestressLossCriteria.Compare(pOther->m_PrestressLossCriteria, *this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if (!m_LimitsCriteria.Compare(pOther->m_LimitsCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }

   if(!m_BearingCriteria.Compare(pOther->m_BearingCriteria,*this,vDifferences,bReturnOnFirstDifference))
   {
      RETURN_ON_DIFFERENCE;
   }
      
   return vDifferences.size() == 0 ? true : false;
}

const SpecificationCriteria& SpecLibraryEntryImpl::GetSpecificationCriteria() const
{
   return m_SpecificationCriteria;
}

void SpecLibraryEntryImpl::SetSpecificationCriteria(const SpecificationCriteria& criteria)
{
   m_SpecificationCriteria = criteria;
}

const ThermalMovementCriteria& SpecLibraryEntryImpl::GetThermalMovementCriteria() const
{
    return m_ThermalMovementCriteria;
}

void SpecLibraryEntryImpl::SetThermalMovementCriteria(const ThermalMovementCriteria& criteria)
{
    m_ThermalMovementCriteria = criteria;
}

const SectionPropertiesCriteria& SpecLibraryEntryImpl::GetSectionPropertiesCriteria() const
{
   return m_SectionPropertiesCriteria;
}

void SpecLibraryEntryImpl::SetSectionPropertiesCriteria(const SectionPropertiesCriteria& criteria)
{
   m_SectionPropertiesCriteria = criteria;
}

const StrandSlopeCriteria& SpecLibraryEntryImpl::GetStrandSlopeCriteria() const
{
   return m_StrandSlopeCriteria;
}

void SpecLibraryEntryImpl::SetStrandSlopeCriteria(const StrandSlopeCriteria& criteria)
{
   m_StrandSlopeCriteria = criteria;
   if (m_StrandSlopeCriteria.bCheck == false)
      m_StrandSlopeCriteria.bDesign = false; // don't allow design without checking
}

const HoldDownCriteria& SpecLibraryEntryImpl::GetHoldDownCriteria() const
{
   return m_HoldDownCriteria;
}

void SpecLibraryEntryImpl::SetHoldDownCriteria(const HoldDownCriteria& criteria)
{
   m_HoldDownCriteria = criteria;
   if (m_HoldDownCriteria.bCheck == false)
      m_HoldDownCriteria.bDesign = false; // don't allow design without checking
}

const PlantHandlingCriteria& SpecLibraryEntryImpl::GetPlantHandlingCriteria() const
{
   return m_PlantHandlingCriteria;
}

void SpecLibraryEntryImpl::SetPlantHandlingCriteria(const PlantHandlingCriteria& criteria)
{
   m_PlantHandlingCriteria = criteria;
}

const EndZoneCriteria& SpecLibraryEntryImpl::GetEndZoneCriteria() const
{
   return m_EndZoneCriteria;
}

void SpecLibraryEntryImpl::SetEndZoneCriteria(const EndZoneCriteria& criteria)
{
   m_EndZoneCriteria = criteria;
}

const SlabOffsetCriteria& SpecLibraryEntryImpl::GetSlabOffsetCriteria() const
{
   return m_SlabOffsetCriteria;
}

void SpecLibraryEntryImpl::SetSlabOffsetCriteria(const SlabOffsetCriteria& criteria)
{
   m_SlabOffsetCriteria = criteria;
   if (m_SlabOffsetCriteria.bCheck == false)
      m_SlabOffsetCriteria.bDesign = false; // don't allow design without checking
}

const ShearCapacityCriteria& SpecLibraryEntryImpl::GetShearCapacityCriteria() const
{
   return m_ShearCapacityCriteria;
}

void SpecLibraryEntryImpl::SetShearCapacityCriteria(const ShearCapacityCriteria& criteria)
{
   m_ShearCapacityCriteria = criteria;
}

const InterfaceShearCriteria& SpecLibraryEntryImpl::GetInterfaceShearCriteria() const
{
   return m_InterfaceShearCriteria;
}

void SpecLibraryEntryImpl::SetInterfaceShearCriteria(const InterfaceShearCriteria& criteria)
{
   m_InterfaceShearCriteria = criteria;
}

const LiftingCriteria& SpecLibraryEntryImpl::GetLiftingCriteria() const
{
   return m_LiftingCriteria;
}

void SpecLibraryEntryImpl::SetLiftingCriteria(const LiftingCriteria& criteria)
{
   m_LiftingCriteria = criteria;
}

const HaulingCriteria& SpecLibraryEntryImpl::GetHaulingCriteria() const
{
   return m_HaulingCriteria;
}

void SpecLibraryEntryImpl::SetHaulingCriteria(const HaulingCriteria& criteria)
{
   m_HaulingCriteria = criteria;
}

const PrincipalTensionStressCriteria& SpecLibraryEntryImpl::GetPrincipalTensionStressCriteria() const
{
   return m_PrincipalTensionStressCriteria;
}

void SpecLibraryEntryImpl::SetPrincipalTensionStressCriteria(const PrincipalTensionStressCriteria& criteria)
{
   m_PrincipalTensionStressCriteria = criteria;
}

const MomentCapacityCriteria& SpecLibraryEntryImpl::GetMomentCapacityCriteria() const
{
   return m_MomentCapacityCriteria;
}

void SpecLibraryEntryImpl::SetMomentCapacityCriteria(const MomentCapacityCriteria& criteria)
{
   m_MomentCapacityCriteria = criteria;
}

const StrandStressCriteria& SpecLibraryEntryImpl::GetStrandStressCriteria() const
{
   return m_StrandStressCriteria;
}

void SpecLibraryEntryImpl::SetStrandStressCriteria(const StrandStressCriteria& criteria)
{
   m_StrandStressCriteria = criteria;
}

const TendonStressCriteria& SpecLibraryEntryImpl::GetTendonStressCriteria() const
{
   return m_TendonStressCriteria;
}

void SpecLibraryEntryImpl::SetTendonStressCriteria(const TendonStressCriteria& criteria)
{
   m_TendonStressCriteria = criteria;
}

const CreepCriteria& SpecLibraryEntryImpl::GetCreepCriteria() const
{
   return m_CreepCriteria;
}

void SpecLibraryEntryImpl::SetCreepCriteria(const CreepCriteria& criteria)
{
   m_CreepCriteria = criteria;
}
const LimitsCriteria& SpecLibraryEntryImpl::GetLimitsCriteria() const
{
   return m_LimitsCriteria;
}

void SpecLibraryEntryImpl::SetLimitsCriteria(const LimitsCriteria& criteria)
{
   m_LimitsCriteria = criteria;
}

const PrestressLossCriteria& SpecLibraryEntryImpl::GetPrestressLossCriteria() const
{
   return m_PrestressLossCriteria;
}

void SpecLibraryEntryImpl::SetPrestressLossCriteria(const PrestressLossCriteria& criteria)
{
   m_PrestressLossCriteria = criteria;
}

const LiveLoadDistributionCriteria& SpecLibraryEntryImpl::GetLiveLoadDistributionCriteria() const
{
   return m_LiveLoadDistributionCriteria;
}

void SpecLibraryEntryImpl::SetLiveLoadDistributionCriteria(const LiveLoadDistributionCriteria& criteria)
{
   m_LiveLoadDistributionCriteria = criteria;
}

const DeadLoadDistributionCriteria& SpecLibraryEntryImpl::GetDeadLoadDistributionCriteria() const
{
   return m_DeadLoadDistributionCriteria;
}

void SpecLibraryEntryImpl::SetDeadLoadDistributionCriteria(const DeadLoadDistributionCriteria& criteria)
{
   m_DeadLoadDistributionCriteria = criteria;
}

const HaunchCriteria& SpecLibraryEntryImpl::GetHaunchCriteria() const
{
   return m_HaunchCriteria;
}

void SpecLibraryEntryImpl::SetHaunchCriteria(const HaunchCriteria& criteria)
{
   m_HaunchCriteria = criteria;
}

const LiveLoadDeflectionCriteria& SpecLibraryEntryImpl::GetLiveLoadDeflectionCriteria() const
{
   return m_LiveLoadDeflectionCriteria;
}

void SpecLibraryEntryImpl::SetLiveLoadDeflectionCriteria(const LiveLoadDeflectionCriteria& criteria)
{
   PRECONDITION(0.0 < criteria.DeflectionLimit);
   m_LiveLoadDeflectionCriteria = criteria;
}

const LiveLoadCriteria& SpecLibraryEntryImpl::GetLiveLoadCriteria() const
{
   return m_LiveLoadCriteria;
} 

void SpecLibraryEntryImpl::SetLiveLoadCriteria(const LiveLoadCriteria& criteria)
{
   m_LiveLoadCriteria = criteria;
}

const TransferLengthCriteria& SpecLibraryEntryImpl::GetTransferLengthCriteria() const
{
   return m_TransferLengthCriteria;
}

void SpecLibraryEntryImpl::SetTransferLengthCriteria(const TransferLengthCriteria& criteria)
{
   m_TransferLengthCriteria = criteria;
}

const DuctSizeCriteria& SpecLibraryEntryImpl::GetDuctSizeCriteria() const
{
   return m_DuctSizeCriteria;
}

void SpecLibraryEntryImpl::SetDuctSizeCriteria(const DuctSizeCriteria& criteria)
{
   m_DuctSizeCriteria = criteria;
}

const ClosureJointCriteria& SpecLibraryEntryImpl::GetClosureJointCriteria() const
{
   return m_ClosureJointCriteria;
}

void SpecLibraryEntryImpl::SetClosureJointCriteria(const ClosureJointCriteria& criteria)
{
   m_ClosureJointCriteria = criteria;
}

const BottomFlangeClearanceCriteria& SpecLibraryEntryImpl::GetBottomFlangeClearanceCriteria() const
{
   return m_BottomFlangeClearanceCriteria;
}

void SpecLibraryEntryImpl::SetBottomFlangeClearanceCriteria(const BottomFlangeClearanceCriteria& criteria)
{
   m_BottomFlangeClearanceCriteria = criteria;
}

const GirderInclinationCriteria& SpecLibraryEntryImpl::GetGirderInclinationCriteria() const
{
   return m_GirderInclinationCriteria;
}

void SpecLibraryEntryImpl::SetGirderInclinationCriteria(const GirderInclinationCriteria& criteria)
{
   m_GirderInclinationCriteria = criteria;
}

const HarpedStrandDesignCriteria& SpecLibraryEntryImpl::GetHarpedStrandDesignCriteria() const
{
   return m_HarpedStrandDesignCriteria;
}

void SpecLibraryEntryImpl::SetHarpedStrandDesignCriteria(const HarpedStrandDesignCriteria& criteria)
{
   m_HarpedStrandDesignCriteria = criteria;
}

const LimitStateConcreteStrengthCriteria& SpecLibraryEntryImpl::GetLimitStateConcreteStrengthCriteria() const
{
   return m_LimitStateConcreteStrengthCriteria;
}

void SpecLibraryEntryImpl::SetLimitStateConcreteStrengthCriteria(const LimitStateConcreteStrengthCriteria& criteria)
{
   m_LimitStateConcreteStrengthCriteria = criteria;
}

const PrestressedElementCriteria& SpecLibraryEntryImpl::GetPrestressedElementCriteria() const
{
   return m_PrestressedElementCriteria;
}

void SpecLibraryEntryImpl::SetPrestressedElementCriteria(const PrestressedElementCriteria& criteria)
{
   m_PrestressedElementCriteria = criteria;
}

const BearingCriteria& SpecLibraryEntryImpl::GetBearingCriteria() const
{
   return m_BearingCriteria;
}

void SpecLibraryEntryImpl::SetBearingCriteria(const BearingCriteria& criteria)
{
   m_BearingCriteria = criteria;
}

void SpecLibraryEntryImpl::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   m_SpecificationCriteria.Report(pChapter,pDisplayUnits);
   m_SectionPropertiesCriteria.Report(pChapter,pDisplayUnits);
   m_ThermalMovementCriteria.Report(pChapter, pDisplayUnits);

   m_HoldDownCriteria.Report(pChapter, pDisplayUnits);
   m_StrandSlopeCriteria.Report(pChapter, pDisplayUnits);
   m_EndZoneCriteria.Report(pChapter, pDisplayUnits);
   m_PlantHandlingCriteria.Report(pChapter, pDisplayUnits);

   m_LiveLoadDeflectionCriteria.Report(pChapter, pDisplayUnits);
   m_BottomFlangeClearanceCriteria.Report(pChapter, pDisplayUnits);
   m_GirderInclinationCriteria.Report(pChapter, pDisplayUnits);
   m_SlabOffsetCriteria.Report(pChapter, pDisplayUnits);
   m_HarpedStrandDesignCriteria.Report(pChapter, pDisplayUnits);
   m_LimitStateConcreteStrengthCriteria.Report(pChapter,pDisplayUnits);

   m_PrestressedElementCriteria.Report(pChapter,pDisplayUnits);
   m_PrincipalTensionStressCriteria.Report(pChapter, pDisplayUnits);

   m_ClosureJointCriteria.Report(pChapter, pDisplayUnits);
   
   m_StrandStressCriteria.Report(pChapter, pDisplayUnits);
   m_TendonStressCriteria.Report(pChapter, pDisplayUnits);
   m_TransferLengthCriteria.Report(pChapter, pDisplayUnits);
   m_DuctSizeCriteria.Report(pChapter, pDisplayUnits);

   m_LiftingCriteria.Report(pChapter,pDisplayUnits);
   m_HaulingCriteria.Report(pChapter,pDisplayUnits);

   m_DeadLoadDistributionCriteria.Report(pChapter, pDisplayUnits);

   m_LiveLoadCriteria.Report(pChapter, pDisplayUnits);
   m_LiveLoadDistributionCriteria.Report(pChapter, pDisplayUnits);

   m_MomentCapacityCriteria.Report(pChapter, pDisplayUnits);
   m_ShearCapacityCriteria.Report(pChapter, pDisplayUnits);
   m_InterfaceShearCriteria.Report(pChapter, pDisplayUnits);

   m_CreepCriteria.Report(pChapter,pDisplayUnits);
   m_HaunchCriteria.Report(pChapter, pDisplayUnits);

   m_PrestressLossCriteria.Report(pChapter,pDisplayUnits);
   m_LimitsCriteria.Report(pChapter,pDisplayUnits);
   m_BearingCriteria.Report(pChapter,pDisplayUnits);
}

const RefactoredSpecLibraryParameters& SpecLibraryEntryImpl::GetRefactoredParameters() const
{
   return m_RefactoredParameters;
}

void SpecLibraryEntryImpl::DeterminePrincipalStressDuctDeductionMultiplier()
{
   // Before 2nd Edition, 2000 interims
   // LRFD 5.8.2.9
   // "In determining the web width at a particular level, the diameter of ungrouted ducts or
   // one-half the diameter of grouted ducts at that level shall be subtracted from the web width"
   //
   // 2nd Edition, 2003 interims
   // "In determining the web width at a particular level, one-half the diameter of ungrouted ducts or
   // one-quarter the diameter of grouted ducts at that level shall be subtracted from the web width"
   //
   // 9th Edition, 2020
   // LRFD 5.7.2.8
   // The paragraph quoted above has been removed and the following is now the definition of bv
   // "bv = ... for grouted ducts, no modification is necessary. For ungrouted ducts, reduce bv by the diameter of the duct"

   auto edition = m_SpecificationCriteria.GetEdition();
   if (WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= edition)
   {
      // 9th Edition, 2020 and later
      m_PrincipalTensionStressCriteria.UngroutedMultiplier = 1.0;
      m_PrincipalTensionStressCriteria.GroutedMultiplier = 0.0;
   }
   else if (edition < WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims)
   {
      m_PrincipalTensionStressCriteria.UngroutedMultiplier = 1.0;
      m_PrincipalTensionStressCriteria.GroutedMultiplier = 0.5;
   }
   else
   {
      // 2nd Edition, 2000 interims to 9th Edition 2020
      m_PrincipalTensionStressCriteria.UngroutedMultiplier = 0.50;
      m_PrincipalTensionStressCriteria.GroutedMultiplier = 0.25;
   }
}

// *this is the LoadMe function prior to SpecificationLibraryEntry data unit version 83.
// *this function is used to load old files. For reference the legacy "SaveMe" is provided
// below as commented out code.
bool SpecLibraryEntryImpl::LegacyLoadMe(WBFL::Library::LibraryEntry* pParent, WBFL::System::IStructuredLoad* pLoad)
{
   // The implementation of *this library entry was changed to use the "Criteria" objects starting with
   // version 83. *this was the code for loading the library entry data for data block version 82 and earlier
   auto version = pLoad->GetVersion();
   PRECONDITION(version < 83);

   Int16 temp;
   pgsTypes::ShearCapacityMethod shear_capacity_method = m_ShearCapacityCriteria.CapacityMethod; // used as a temporary storage for shear capacity method before file version 18

   std::_tstring name;
   if (pLoad->Property(_T("Name"), &name))
   {
      pParent->SetName(name.c_str());
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (pLoad->Property(_T("Description"), &name))
   {
      m_SpecificationCriteria.Description = name;
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (76 < version)
   {
      // added in version 77
      if (!pLoad->Property(_T("UseCurrentSpecification"), &m_SpecificationCriteria.bUseCurrentSpecification))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      // if loading an older file, set *this to false because it wasn't an option
      // and we don't want to change the specification.
      // the default for new entires is true so we have to force the value to false here
      m_SpecificationCriteria.bUseCurrentSpecification = false;
   }

   std::_tstring tmp;
   if (pLoad->Property(_T("SpecificationType"), &tmp))
   {
      try
      {
         m_SpecificationCriteria.Edition = WBFL::LRFD::BDSManager::GetEdition(tmp.c_str());
      }
      catch (...)
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }


   if (pLoad->Property(_T("SpecificationUnits"), &tmp))
   {
      if (tmp == _T("SiUnitsSpec"))
      {
         m_SpecificationCriteria.Units = WBFL::LRFD::BDSManager::Units::SI;
      }
      else if (tmp == _T("UsUnitsSpec"))
      {
         m_SpecificationCriteria.Units = WBFL::LRFD::BDSManager::Units::US;
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (50 <= version)
   {
      // added in version 50
      int value;
      if (!pLoad->Property(_T("SectionPropertyType"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_SectionPropertiesCriteria.SectionPropertyMode = (pgsTypes::SectionPropertyMode)value;
   }

   if (!pLoad->Property(_T("DoCheckStrandSlope"), &m_StrandSlopeCriteria.bCheck))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (version < 15)
   {
      m_StrandSlopeCriteria.bDesign = m_StrandSlopeCriteria.bCheck;
   }
   else
   {
      if (!pLoad->Property(_T("DoDesignStrandSlope"), &m_StrandSlopeCriteria.bDesign))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("MaxSlope05"), &m_StrandSlopeCriteria.MaxSlope05))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("MaxSlope06"), &m_StrandSlopeCriteria.MaxSlope06))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (35 <= version)
   {
      if (!pLoad->Property(_T("MaxSlope07"), &m_StrandSlopeCriteria.MaxSlope07))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("DoCheckHoldDown"), &m_HoldDownCriteria.bCheck))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (version < 15)
   {
      m_HoldDownCriteria.bDesign = m_HoldDownCriteria.bCheck;
   }
   else
   {
      if (!pLoad->Property(_T("DoDesignHoldDown"), &m_HoldDownCriteria.bDesign))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (69 < version)
   {
      // added in version 70
      int type;
      if (!pLoad->Property(_T("HoldDownForceType"), &type))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HoldDownCriteria.type = (HoldDownCriteria::Type)type;
   }

   if (!pLoad->Property(_T("HoldDownForce"), &m_HoldDownCriteria.force_limit))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (69 < version)
   {
      // added in version 70
      if (!pLoad->Property(_T("HoldDownFriction"), &m_HoldDownCriteria.friction))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (32 < version && version < 39)
   {
      // added in version 33 and removed in version 38
      bool check_anchor;
      if (!pLoad->Property(_T("DoCheckAnchorage"), &check_anchor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_EndZoneCriteria.bCheckSplitting = check_anchor;
      m_EndZoneCriteria.bCheckConfinement = check_anchor;
      m_EndZoneCriteria.bDesignSplitting = check_anchor;
      m_EndZoneCriteria.bDesignConfinement = check_anchor;
   }
   else if (39 <= version)
   {
      if (!pLoad->Property(_T("DoCheckSplitting"), &m_EndZoneCriteria.bCheckSplitting))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoDesignSplitting"), &m_EndZoneCriteria.bDesignSplitting))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoCheckConfinement"), &m_EndZoneCriteria.bCheckConfinement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoDesignConfinement"), &m_EndZoneCriteria.bDesignConfinement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (69 < version)
   {
      // added in version 70
      if (!pLoad->Property(_T("DoCheckHandlingWeightLimit"), &m_PlantHandlingCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("HandlingWeightLimit"), &m_PlantHandlingCriteria.WeightLimit))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 46 || version == 50)
   {
      // removed in version 46
      // also used in version 50 (on the PGSplice, v3.0 branch)
      Float64 maxStirrupSpacing;
      if (!pLoad->Property(_T("MaxStirrupSpacing"), &maxStirrupSpacing))
         THROW_LOAD(InvalidFileFormat, pLoad);

      m_ShearCapacityCriteria.MaxStirrupSpacing[0] = maxStirrupSpacing;

      if (m_SpecificationCriteria.Units == WBFL::LRFD::BDSManager::Units::SI)
      {
         // default value in SI units is 300mm
         // default value is US units is 12"
         // 12" = 305mm
         m_ShearCapacityCriteria.MaxStirrupSpacing[1] = WBFL::Units::ConvertToSysUnits(300.0, WBFL::Units::Measure::Millimeter);
      }

   }
   else
   {
      // added in version 46 or 51 (PGSplice, v3.0 branch)
      if (!pLoad->Property(_T("StirrupSpacingCoefficient1"), &m_ShearCapacityCriteria.StirrupSpacingCoefficient[0]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxStirrupSpacing1"), &m_ShearCapacityCriteria.MaxStirrupSpacing[0]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("StirrupSpacingCoefficient2"), &m_ShearCapacityCriteria.StirrupSpacingCoefficient[1]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxStirrupSpacing2"), &m_ShearCapacityCriteria.MaxStirrupSpacing[1]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("CyLiftingCrackFs"), &m_LiftingCriteria.FsCracking))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyLiftingFailFs"), &m_LiftingCriteria.FsFailure))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyCompStressServ"), &m_PrestressedElementCriteria.CompressionStressCoefficient_BeforeLosses))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (version < 65)
   {
      if (!pLoad->Property(_T("CyCompStressLifting"), &m_LiftingCriteria.CompressionStressCoefficient_GlobalStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      if (!pLoad->Property(_T("CyGlobalCompStressLifting"), &m_LiftingCriteria.CompressionStressCoefficient_GlobalStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CyPeakCompStressLifting"), &m_LiftingCriteria.CompressionStressCoefficient_PeakStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

   }

   if (!pLoad->Property(_T("CyTensStressServ"), &m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyDoTensStressServMax"), &m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.bHasMaxValue))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyTensStressServMax"), &m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.MaxValue))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyTensStressLifting"), &m_LiftingCriteria.TensionStressLimitWithoutReinforcement.Coefficient))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyDoTensStressLiftingMax"), &m_LiftingCriteria.TensionStressLimitWithoutReinforcement.bHasMaxValue))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CyTensStressLiftingMax"), &m_LiftingCriteria.TensionStressLimitWithoutReinforcement.MaxValue))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   // at version 27 we moved debonding to the girder library. Data here is ignored
   if (6.0 <= version && version < 27)
   {
      Float64 debond_junk;
      if (!pLoad->Property(_T("MaxDebondStrands"), &debond_junk))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxDebondStrandsPerRow"), &debond_junk))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxNumDebondedStrandsPerSection"), &debond_junk))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxDebondedStrandsPerSection"), &debond_junk))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DefaultDebondLength"), &debond_junk))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (1.4 <= version)
   {
      if (!pLoad->Property(_T("BurstingZoneLengthFactor"), &m_EndZoneCriteria.SplittingZoneLengthFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (74 < version && version < 82)
   {
      // removed in version 82
      Float64 dummy; // gobble up the value since it isn't useful anymore
      if (!pLoad->Property(_T("UHPCStregthAtFirstCrack"), &dummy))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("LiftingUpwardImpact"), &m_LiftingCriteria.ImpactUp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("LiftingDownwardImpact"), &m_LiftingCriteria.ImpactDown))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("HaulingUpwardImpact"), &m_HaulingCriteria.WSDOT.ImpactUp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("HaulingDownwardImpact"), &m_HaulingCriteria.WSDOT.ImpactDown))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("CuringMethod"), &temp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }
   m_CreepCriteria.CuringMethod = (pgsTypes::CuringMethod)temp;

   if (version < 11)
   {
      m_LiftingCriteria.bCheck = true;
      m_LiftingCriteria.bDesign = true;
   }
   else
   {
      if (!pLoad->Property(_T("EnableLiftingCheck"), &m_LiftingCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 15)
      {
         m_LiftingCriteria.bDesign = true;
      }
      else
      {
         if (!pLoad->Property(_T("EnableLiftingDesign"), &m_LiftingCriteria.bDesign))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   if (!pLoad->Property(_T("PickPointHeight"), &m_LiftingCriteria.PickPointHeight))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("LiftingLoopTolerance"), &m_LiftingCriteria.LiftingLoopTolerance))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("MinCableInclination"), &m_LiftingCriteria.MinCableInclination))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("MaxGirderSweepLifting"), &m_LiftingCriteria.SweepTolerance))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (55 < version)
   {
      // added in version 56, removed in version 65
      if (version < 65)
      {
         Int32 temp;
         if (!pLoad->Property(_T("LiftingCamberMethod"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         //m_LiftingCamberMethod = (pgsTypes::CamberMethod)temp; ignore value

         Float64 liftingCamberPrecentEstimate;
         if (!pLoad->Property(_T("LiftingCamberPercentEstimate"), &liftingCamberPrecentEstimate/*&m_LiftingCamberPercentEstimate*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (57 < version)
      {
         // added in version 58
         if (!pLoad->Property(_T("LiftingCamberMultiplier"), &m_LiftingCriteria.CamberMultiplier))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("LiftingWindType"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_LiftingCriteria.WindLoadType = (WBFL::Stability::WindLoadType)temp;

      if (!pLoad->Property(_T("LiftingWindLoad"), &m_LiftingCriteria.WindLoad))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 59)
      {
         bool bPlumbGirder;
         if (!pLoad->Property(_T("LiftingStressesPlumbGirder"), &bPlumbGirder))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         //m_bComputeLiftingStressesAtEquilibriumAngle = !bPlumbGirder; removed in version 65
      }
      else if (version < 65)
      {
         // removed in version 65
         bool bComputeLiftingStressAtEquilibriumAngle;
         if (!pLoad->Property(_T("LiftingStressesEquilibriumAngle"), &bComputeLiftingStressAtEquilibriumAngle/*&m_bComputeLiftingStressesAtEquilibriumAngle*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   if (version < 11)
   {
      m_HaulingCriteria.bCheck = true;
      m_HaulingCriteria.bDesign = true;
   }
   else
   {
      if (!pLoad->Property(_T("EnableHaulingCheck"), &m_HaulingCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 15)
      {
         m_HaulingCriteria.bDesign = true;
      }
      else
      {
         if (!pLoad->Property(_T("EnableHaulingDesign"), &m_HaulingCriteria.bDesign))
            THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 43)
   {
      m_HaulingCriteria.AnalysisMethod = pgsTypes::HaulingAnalysisMethod::WSDOT;
   }
   else
   {
      Int32 tmp;
      if (!pLoad->Property(_T("HaulingAnalysisMethod"), &tmp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_HaulingCriteria.AnalysisMethod = (pgsTypes::HaulingAnalysisMethod)tmp;
   }

   if (!pLoad->Property(_T("MaxGirderSweepHauling"), &m_HaulingCriteria.WSDOT.SweepTolerance))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (65 < version)
   {
      // added version 66
      if (!pLoad->Property(_T("SweepGrowthHauling"), &m_HaulingCriteria.WSDOT.SweepGrowth))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 56)
   {
      m_RefactoredParameters.m_bHasOldHaulTruck = true;

      // removed in version 56
      if (!pLoad->Property(_T("HaulingSupportDistance"), &m_RefactoredParameters.m_OldHaulTruck.m_Lmax))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (2.0 <= version)
      {
         if (!pLoad->Property(_T("MaxHaulingOverhang"), &m_RefactoredParameters.m_OldHaulTruck.m_MaxOH))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         m_RefactoredParameters.m_OldHaulTruck.m_MaxOH = WBFL::Units::ConvertToSysUnits(15.0, WBFL::Units::Measure::Feet);
      }
   }

   if (!pLoad->Property(_T("HaulingSupportPlacementTolerance"), &m_HaulingCriteria.WSDOT.SupportPlacementTolerance))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (55 < version)
   {
      // added in version 56, removed version 65
      if (version < 65)
      {
         Int32 temp;
         if (!pLoad->Property(_T("HaulingCamberMethod"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         //m_HaulingCamberMethod = (pgsTypes::CamberMethod)temp;
      }

   }

   // removed in version 65
   if (version < 65)
   {
      Float64 haulingCamberPrecentEstimate;
      if (!pLoad->Property(_T("HaulingCamberPercentEstimate"), &haulingCamberPrecentEstimate/*&m_HaulingCamberPercentEstimate*/))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   // Removed in version 65
   //if ( version < 56 )
   //{
      //// in version 56 *this value changed from a whole percentage to a fraction
      //// e.g. 2% became 0.02
      //m_HaulingCamberPercentEstimate /= 100;
   //}

   if (57 < version)
   {
      // added in version 50
      if (!pLoad->Property(_T("HaulingCamberMultiplier"), &m_HaulingCriteria.WSDOT.CamberMultiplier))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (55 < version)
   {
      Int32 temp;
      if (!pLoad->Property(_T("HaulingWindType"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.WSDOT.WindLoadType = (WBFL::Stability::WindLoadType)temp;

      if (!pLoad->Property(_T("HaulingWindLoad"), &m_HaulingCriteria.WSDOT.WindLoad))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CentrifugalForceType"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.WSDOT.CentrifugalForceType = (WBFL::Stability::CFType)temp;

      if (!pLoad->Property(_T("HaulingSpeed"), &m_HaulingCriteria.WSDOT.HaulingSpeed))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TurningRadius"), &m_HaulingCriteria.WSDOT.TurningRadius))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (58 < version && version < 65)
      {
         // added in version 59, removed in version 65
         bool bComputeHaulingStressAtEquilibriumAngle;
         if (!pLoad->Property(_T("HaulingStressesEquilibriumAngle"), &bComputeHaulingStressAtEquilibriumAngle/*&m_bComputeHaulingStressesAtEquilibriumAngle*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   if (version < 65)
   {
      if (!pLoad->Property(_T("CompStressHauling"), &m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.KDOT.CompressionStressLimitCoefficient = m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress;
   }
   else
   {
      if (!pLoad->Property(_T("GlobalCompStressHauling"), &m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.KDOT.CompressionStressLimitCoefficient = m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress;

      if (!pLoad->Property(_T("PeakCompStressHauling"), &m_HaulingCriteria.WSDOT.CompressionStressCoefficient_PeakStress))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 56)
   {
      if (!pLoad->Property(_T("TensStressHauling"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoTensStressHaulingMax"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensStressHaulingMax"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement = m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope];
   }
   else
   {
      if (!pLoad->Property(_T("TensStressHaulingNormalCrown"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoTensStressHaulingMaxNormalCrown"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensStressHaulingMaxNormalCrown"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement = m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope];

      if (!pLoad->Property(_T("TensStressHaulingMaxSuper"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DoTensStressHaulingMaxMaxSuper"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensStressHaulingMaxMaxSuper"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("HeHaulingCrackFs"), &m_HaulingCriteria.WSDOT.FsCracking))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (!pLoad->Property(_T("HeHaulingFailFs"), &m_HaulingCriteria.WSDOT.FsFailure))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (55 < version)
   {
      // added in version 56
      Int32 temp;
      if (!pLoad->Property(_T("HaulingImpactUsage"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_HaulingCriteria.WSDOT.ImpactUsage = (WBFL::Stability::HaulingImpact)temp;

      if (!pLoad->Property(_T("RoadwayCrownSlope"), &m_HaulingCriteria.WSDOT.RoadwayCrownSlope))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

   }

   if (!pLoad->Property(_T("RoadwaySuperelevation"), &m_HaulingCriteria.WSDOT.RoadwaySuperelevation))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (version < 56)
   {
      // removed in version 56
      if (version < 1.9)
      {
         if (!pLoad->Property(_T("TruckRollStiffness"), &m_RefactoredParameters.m_OldHaulTruck.m_TruckRollStiffness))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_RefactoredParameters.m_OldHaulTruck.m_TruckRollStiffnessMethod = 0;
      }
      else
      {
         long method;
         if (!pLoad->Property(_T("TruckRollStiffnessMethod"), &method))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_RefactoredParameters.m_OldHaulTruck.m_TruckRollStiffnessMethod = (int)method;

         if (!pLoad->Property(_T("TruckRollStiffness"), &m_RefactoredParameters.m_OldHaulTruck.m_TruckRollStiffness))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AxleWeightLimit"), &m_RefactoredParameters.m_OldHaulTruck.m_AxleWeightLimit))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AxleStiffness"), &m_RefactoredParameters.m_OldHaulTruck.m_AxleStiffness))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("MinRollStiffness"), &m_RefactoredParameters.m_OldHaulTruck.m_MinRollStiffness))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("TruckGirderHeight"), &m_RefactoredParameters.m_OldHaulTruck.m_Hbg))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TruckRollCenterHeight"), &m_RefactoredParameters.m_OldHaulTruck.m_Hrc))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TruckAxleWidth"), &m_RefactoredParameters.m_OldHaulTruck.m_Wcc))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 55.0)
   {
      // removed in version 55.0 (*this parameters are never used)
      Float64 value;
      if (!pLoad->Property(_T("HeErectionCrackFs"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("HeErectionFailFs"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (1.3 <= version)
   {
      if (version < 56)
      {
         // removed in vesrion 56
         if (!pLoad->Property(_T("MaxGirderWgt"), &m_RefactoredParameters.m_OldHaulTruck.m_MaxWeight))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }
   else
   {
      m_RefactoredParameters.m_OldHaulTruck.m_MaxWeight = WBFL::Units::ConvertToSysUnits(200, WBFL::Units::Measure::Kip);
   }

   if (52 < version)
   {
      // Added at version 53
      if (!pLoad->Property(_T("LimitStateConcreteStrength"), (long*)&m_LimitStateConcreteStrengthCriteria.LimitStateConcreteStrength))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (71 < version)
   {
      // added in version 72
      if (!pLoad->Property(_T("Use90DayConcreteStrength"), &m_LimitStateConcreteStrengthCriteria.bUse90DayConcreteStrength))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SlowCuringConcreteStrengthFactor"), &m_LimitStateConcreteStrengthCriteria.SlowCuringConcreteStrengthFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 37)
   {
      // changed in version 37
      if (12 <= version)
      {
         if (!pLoad->Property(_T("HaulingModulusOfRuptureCoefficient"), &m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (20 <= version)
      {
         if (!pLoad->Property(_T("LiftingModulusOfRuptureCoefficient"), &m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   if (25 <= version)
   {
      // Added at version 25
      if (!pLoad->Property(_T("MinLiftingPointLocation"), &m_LiftingCriteria.MinPickPoint))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("LiftingPointLocationAccuracy"), &m_LiftingCriteria.PickPointAccuracy))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MinHaulingSupportLocation"), &m_HaulingCriteria.MinBunkPoint))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("HaulingSupportLocationAccuracy"), &m_HaulingCriteria.BunkPointAccuracy))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (43 <= version)
   {
      // KDOT values
      if (!pLoad->Property(_T("UseMinTruckSupportLocationFactor"), &m_HaulingCriteria.bUseMinBunkPointLimit))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MinTruckSupportLocationFactor"), &m_HaulingCriteria.MinBunkPointLimitFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("OverhangGFactor"), &m_HaulingCriteria.KDOT.OverhangGFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("InteriorGFactor"), &m_HaulingCriteria.KDOT.InteriorGFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (3.2 < version)
   {
      if (!pLoad->Property(_T("CastingYardTensileStressLimitWithMildRebar"), &m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("LiftingTensileStressLimitWithMildRebar"), &m_LiftingCriteria.TensionStressLimitWithReinforcement.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 56)
      {
         if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebar"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebarNormalCrown"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebarMaxSuper"), &m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }
   else
   {
      m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses.Coefficient = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
      m_LiftingCriteria.TensionStressLimitWithReinforcement.Coefficient = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
      m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
      m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
   }
   m_HaulingCriteria.KDOT.TensionStressLimitWithReinforcement = m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope];

   // deal with version 1.1
   // in *this version we added compressive and tensile stress limits for BSS 1 & 2
   // in version 1.0, limits were for all BS stages.
   if (version == 1.0)
   {
      if (!pLoad->Property(_T("BsCompStressServ"), &m_PrestressedElementCriteria.CompressionStressCoefficient_AllLoads_AfterLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsCompStressService1A"), &m_PrestressedElementCriteria.CompressionStressCoefficient_Fatigue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 tmp;
      if (!pLoad->Property(_T("BsCompStressService1B"), &tmp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      // stage 1 and 2 are permanent loads only
      m_PrestressedElementCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses = tmp;
      m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement = tmp;

      if (!pLoad->Property(_T("BsTensStressServNc"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsDoTensStressServNcMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsTensStressServNcMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement = m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses;

      if (!pLoad->Property(_T("BsTensStressServSc"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsDoTensStressServScMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsTensStressServScMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("BsMaxGirdersTrafficBarrier"), &m_DeadLoadDistributionCriteria.MaxGirdersTrafficBarrier))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 55)
      {
         // *this parameter never used... removed in version 55
         Float64 value;
         if (!pLoad->Property(_T("BsMaxGirdersUtility"), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      m_PrestressedElementCriteria.CompressionStressCoefficient_TemporaryStrandRemoval = m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement;
      m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval = m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement;
      m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval = m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses;
   }
   else if (1.1 <= version)
   {
      if (29 < version)
      {
         // added in version 30
         if (!pLoad->Property(_T("TempStrandRemovalCompStress"), &m_PrestressedElementCriteria.CompressionStressCoefficient_TemporaryStrandRemoval))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("TempStrandRemovalTensStress"), &m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("TempStrandRemovalDoTensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.bHasMaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("TempStrandRemovalTensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.MaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (48 < version)
         {
            // added version 49
            if (!pLoad->Property(_T("TempStrandRemovalTensStressWithRebar"), &m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Coefficient))
            {
               m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval = m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses; // make sure it is set to the defalut value
               if (MAX_OVERLAP_VERSION < version)
               {
                  // *this was added in version 49 for PGSuper version 2.9.
                  // At the same time, PGSuper 3.0 was being built. The data block version was
                  // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                  // then the data file format is invalid.
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
         }

         if (46 < version)
         {
            // added in version 47
            if (!pLoad->Property(_T("CheckTemporaryStresses"), &m_PrestressedElementCriteria.bCheckTemporaryStresses))
            {
               m_PrestressedElementCriteria.bCheckTemporaryStresses = true; // make sure it is set to the default value
               if (MAX_OVERLAP_VERSION < version)
               {
                  // *this was added in version 47 for PGSuper version 2.9.
                  // At the same time, PGSuper 3.0 was being built. The data block version was
                  // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                  // then the data file format is invalid.
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
         }

         // for the following 5 items, the m_ was removed from the keyword in version 30
         if (!pLoad->Property(_T("Bs1CompStress"), &m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("Bs1TensStress"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("Bs1DoTensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.bHasMaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("Bs1TensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.MaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("Bs2CompStress"), &m_PrestressedElementCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (46 < version)
         {
            bool bOKToFail = false;
            if (version <= MAX_OVERLAP_VERSION)
            {
               // *this was added in version 45 for PGSuper version 2.9.
               // At the same time, PGSuper 3.0 was being built. The data block version was
               // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
               // then the data file format is invalid.
               bOKToFail = true;
            }

            // added in version 47
            bool bSucceeded = pLoad->Property(_T("CheckBs2Tension"), &m_PrestressedElementCriteria.bCheckFinalServiceITension);
            if (bSucceeded)
            {
               if (!pLoad->Property(_T("Bs2TensStress"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Coefficient))
                  THROW_LOAD(InvalidFileFormat, pLoad);

               if (!pLoad->Property(_T("Bs2DoTensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.bHasMaxValue))
                  THROW_LOAD(InvalidFileFormat, pLoad);

               if (!pLoad->Property(_T("Bs2TensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.MaxValue))
                  THROW_LOAD(InvalidFileFormat, pLoad);
            }
            else
            {
               if (!bOKToFail)
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
         }
      }
      else
      {
         if (!pLoad->Property(_T("m_Bs1CompStress"), &m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs1TensStress"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs1DoTensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.bHasMaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs1TensStressMax"), &m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.MaxValue))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs2CompStress"), &m_PrestressedElementCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_PrestressedElementCriteria.CompressionStressCoefficient_TemporaryStrandRemoval = m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement;
         m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval = m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement;
         m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval = m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses;
      }

      if (version < 5.0)
      {
         // *this parameters were removed.... just gobble them up if *this is an older file
         Float64 dummy;
         if (!pLoad->Property(_T("m_Bs2TensStress"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs2DoTensStressMax"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("m_Bs2TensStressMax"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (36 <= version)
      {
         Int16 value;
         if (!pLoad->Property(_T("Bs2TrafficBarrierDistributionType"), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_DeadLoadDistributionCriteria.TrafficBarrierDistribution = (pgsTypes::TrafficBarrierDistribution)value;
      }

      if (!pLoad->Property(_T("Bs2MaxGirdersTrafficBarrier"), &m_DeadLoadDistributionCriteria.MaxGirdersTrafficBarrier))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 55)
      {
         // *this parameter was never used. removed in version 55
         Float64 value;
         if (!pLoad->Property(_T("Bs2MaxGirdersUtility"), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (33.0 < version)
      {
         long oldt;
         if (!pLoad->Property(_T("OverlayLoadDistribution"), &oldt))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_DeadLoadDistributionCriteria.OverlayDistribution = (pgsTypes::OverlayLoadDistributionType)oldt == pgsTypes::olDistributeTributaryWidth ?
            pgsTypes::olDistributeTributaryWidth : pgsTypes::olDistributeEvenly;
      }

      if (53.0 < version)
      {
         Int32 hlct;
         if (!pLoad->Property(_T("HaunchLoadComputationType"), &hlct))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_HaunchCriteria.HaunchLoadComputationType = (pgsTypes::HaunchLoadComputationType)hlct;

         if (!pLoad->Property(_T("HaunchLoadCamberTolerance"), &m_HaunchCriteria.HaunchLoadCamberTolerance))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (59 < version)
      {
         if (!pLoad->Property(_T("HaunchLoadCamberFactor"), &m_HaunchCriteria.HaunchLoadCamberFactor))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (62 < version)
      {
         Int32 hlct;
         if (!pLoad->Property(_T("HaunchAnalysisComputationType"), &hlct))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_HaunchCriteria.HaunchAnalysisSectionPropertiesType = (pgsTypes::HaunchAnalysisSectionPropertiesType)hlct;
      }

      if (!pLoad->Property(_T("Bs3CompStressServ"), &m_PrestressedElementCriteria.CompressionStressCoefficient_AllLoads_AfterLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3CompStressService1A"), &m_PrestressedElementCriteria.CompressionStressCoefficient_Fatigue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3TensStressServNc"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3DoTensStressServNcMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3TensStressServNcMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3TensStressServSc"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3DoTensStressServScMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.bHasMaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Bs3TensStressServScMax"), &m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.MaxValue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (75 < version)
      {
         // added in version 76
         Int16 value;
         if (!pLoad->Property(_T("PrincipalTensileStressMethod"), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_PrincipalTensionStressCriteria.Method = (pgsTypes::PrincipalTensileStressMethod)value;

         if (!pLoad->Property(_T("PrincipalTensileStressCoefficient"), &m_PrincipalTensionStressCriteria.Coefficient))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (76 < version)
         {
            // added in version 77
            if (!pLoad->Property(_T("PrincipalTensileStressTendonNearnessFactor"), &m_PrincipalTensionStressCriteria.TendonNearnessFactor))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if (77 < version)
         {
            // added in version 78
            if (!pLoad->Property(_T("PrincipalTensileStressFcThreshold"), &m_PrincipalTensionStressCriteria.FcThreshold))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if (79 < version)
         {
            // added in version 78
            if (!pLoad->Property(_T("PrincipalTensileStressUngroutedMultiplier"), &m_PrincipalTensionStressCriteria.UngroutedMultiplier))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("PrincipalTensileStressGroutedMultiplier"), &m_PrincipalTensionStressCriteria.GroutedMultiplier))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            // Older versions did not load the duct multipliers -> they were spec dependent. Use the spec version to determine the value
            DeterminePrincipalStressDuctDeductionMultiplier();
         }
      }
      else
      {
         DeterminePrincipalStressDuctDeductionMultiplier();
      }
   }

   if (1.4 <= version)
   {
      if (version < 29)
      {
         // removed in version 29
         if (!pLoad->Property(_T("Bs3IgnoreRangeOfApplicability"), &m_RefactoredParameters.m_bIgnoreRangeOfApplicability))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      // *this parameter removed in version 18
      if (version < 18)
      {
         if (!pLoad->Property(_T("Bs3LRFDShear"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         switch (temp)
         {
         case 0: // scmGeneral -> pgsTypes::scmBTTables
            shear_capacity_method = pgsTypes::scmBTTables;
            break;

         case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
            shear_capacity_method = pgsTypes::scmWSDOT2001;
            break;

         case 2: // scmSimplified -> pgsTypes::scmVciVcw
            shear_capacity_method = pgsTypes::scmVciVcw;
            break;

         default:
            ATLASSERT(false);
         }
      }
   }
   else
   {
      shear_capacity_method = pgsTypes::scmBTTables;
   }

   if (version < 37)
   {
      if (1.8 <= version)
      {
         if (!pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_MomentCapacityCriteria.OverReinforcedMomentCapacity = temp;
      }

      if (7.0 <= version)
      {
         if (!pLoad->Property(_T("IncludeRebar_MomentCapacity"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_MomentCapacityCriteria.bIncludeRebar = (temp == 0 ? false : true);
      }
   }
   else
   {
      if (!pLoad->BeginUnit(_T("MomentCapacity")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 mc_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_MomentCapacityCriteria.OverReinforcedMomentCapacity = temp;

      // added in version 4
      if (3 < mc_version)
      {
         if (!pLoad->Property(_T("IncludeStrandForNegMoment"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_MomentCapacityCriteria.bIncludeStrandForNegMoment = (temp == 0 ? false : true);
      }

      if (!pLoad->Property(_T("IncludeRebarForCapacity"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_MomentCapacityCriteria.bIncludeRebar = (temp == 0 ? false : true);


      if (4 < mc_version) // added in version 5
      {
         if (!pLoad->Property(_T("ConsiderReinforcementStrainLimit"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_MomentCapacityCriteria.bConsiderReinforcementStrainLimit = (temp == 0 ? false : true);

         if (!pLoad->Property(_T("MomentCapacitySliceCount"), &m_MomentCapacityCriteria.nMomentCapacitySlices))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (2 <= mc_version)
      {
         if (!pLoad->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"), &temp)) // added version 2 of *this data block
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_MomentCapacityCriteria.bIncludeNoncompositeMomentsForNegMomentDesign = (temp == 0 ? false : true);
      }

      if (mc_version < 3)
      {
         if (!pLoad->BeginUnit(_T("ReductionFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         // fixed spelling error in version 3 of MomentCapacity data block
         if (!pLoad->BeginUnit(_T("ResistanceFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->BeginUnit(_T("NormalWeight")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_RC"), &m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_PS"), &m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 nwc_version = pLoad->GetVersion();
      if (1.0 < nwc_version)
      {
         if (!pLoad->Property(_T("TensionControlled_Spliced"), &m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("CompressionControlled"), &m_MomentCapacityCriteria.PhiCompression[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // NormalWeight
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("AllLightweight")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_RC"), &m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_PS"), &m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 alc_version = pLoad->GetVersion();
      if (1.0 < alc_version)
      {
         if (!pLoad->Property(_T("TensionControlled_Spliced"), &m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::AllLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("CompressionControlled"), &m_MomentCapacityCriteria.PhiCompression[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // AllLightweight
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->BeginUnit(_T("SandLightweight")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_RC"), &m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TensionControlled_PS"), &m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 slc_version = pLoad->GetVersion();
      if (1.0 < slc_version)
      {
         if (!pLoad->Property(_T("TensionControlled_Spliced"), &m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::SandLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("CompressionControlled"), &m_MomentCapacityCriteria.PhiCompression[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // SandLightweight
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // ResistanceFactor
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (2 < mc_version)
      {
         // added ClosureJointResistanceFactor in version 3 of the MomentCapacity data block
         if (!pLoad->BeginUnit(_T("ClosureJointResistanceFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->BeginUnit(_T("NormalWeight")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("FullyBondedTendons"), &m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->EndUnit()) // NormalWeight
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->BeginUnit(_T("AllLightweight")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("FullyBondedTendons"), &m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::AllLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->EndUnit()) // AllLightweight
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->BeginUnit(_T("SandLightweight")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("FullyBondedTendons"), &m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::SandLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->EndUnit()) // SandLightweight
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->EndUnit()) // ClosureJointResistanceFactor
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit()) // MomentCapacity
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }


   if (version < 37)
   {
      if (9.0 <= version)
      {
         if (!pLoad->Property(_T("ModulusOfRuptureCoefficient"), &m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (18 <= version)
      {
         // added in version 18
         if (!pLoad->Property(_T("ShearModulusOfRuptureCoefficient"), &m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }
   else
   {
      if (!pLoad->BeginUnit(_T("ModulusOfRuptureCoefficient")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("Moment")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 moment_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("Normal"), &m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AllLightweight"), &m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SandLightweight"), &m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1 < moment_version && moment_version < 3)
      {
         // added in version 2, removed in version 3
         // we are throwing out *this value because it is no longer relevant
         Float64 dummy;
         if (!pLoad->Property(_T("UHPC"), &dummy/*&m_FlexureModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit()) // Moment
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("Shear")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 shear_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("Normal"), &m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AllLightweight"), &m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SandLightweight"), &m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1 < shear_version && version < 3)
      {
         // added in version 2, removed in version 3
         // we are throwing out *this value because it is no longer relevant
         Float64 dummy;
         if (!pLoad->Property(_T("UHPC"), &dummy/*&m_ShearModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit()) // Shear
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("Lifting")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 lifting_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("Normal"), &m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AllLightweight"), &m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SandLightweight"), &m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1 < lifting_version && lifting_version < 3)
      {
         // added in version 2, removed in version 3
         Float64 dummy;
         if (!pLoad->Property(_T("UHPC"), &dummy/*&m_LiftingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit()) // Lifting
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->BeginUnit(_T("Shipping")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 shipping_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("Normal"), &m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AllLightweight"), &m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SandLightweight"), &m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1 < shipping_version && shipping_version < 3)
      {
         // added in version 2, removed in version 3
         Float64 dummy;
         if (!pLoad->Property(_T("UHPC"), &dummy/*&m_HaulingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit()) // Shipping
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // ModulusOfRuptureCoefficient
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (74 < version && version < 82)
   {
      Float64 dummy;
      if (!pLoad->Property(_T("UHPCFiberShearStrength"), &dummy/*&m_UHPCFiberShearStrength*/)) // added in version 75, removed in version 82
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->Property(_T("BsLldfMethod"), &temp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }
   m_LiveLoadDistributionCriteria.LldfMethod = (pgsTypes::LiveLoadDistributionFactorMethod)temp;

   if (73 < version)
   {
      // added in version 74
      if (!pLoad->Property(_T("IgnoreSkewReductionForMoment"), &m_LiveLoadDistributionCriteria.bIgnoreSkewReductionForMoment))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (28 < version)
   {
      // added in version 29
      if (!pLoad->Property(_T("MaxAngularDeviationBetweenGirders"), &m_LiveLoadDistributionCriteria.MaxAngularDeviationBetweenGirders))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MinGirderStiffnessRatio"), &m_LiveLoadDistributionCriteria.MinGirderStiffnessRatio))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("LLDFGirderSpacingLocation"), &m_LiveLoadDistributionCriteria.GirderSpacingLocation))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (69 < version)
   {
      // added in version 70
      if (!pLoad->Property(_T("UseRigidMethod"), &m_LiveLoadDistributionCriteria.bUseRigidMethod))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (60 < version)
   {
      // added in version 61
      if (!pLoad->Property(_T("IncludeDualTandem"), &m_LiveLoadCriteria.bIncludeDualTandem))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (30 < version)
   {
      if (!pLoad->Property(_T("LimitDistributionFactorsToLanesBeams"), &m_LiveLoadDistributionCriteria.bLimitDistributionFactorsToLanesBeams))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (80 < version)
   {
      if (!pLoad->Property(_T("ExteriorLiveLoadDistributionGTAdjacentInteriorRule"), &m_LiveLoadDistributionCriteria.bExteriorBeamLiveLoadDistributionGTInteriorBeam))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 37)
   {
      // moved below in version 37

      // added longitudinal reinforcement for shear method to version 1.2 (*this was the only change)
      if (1.2 <= version)
      {
         if (!pLoad->Property(_T("LongReinfShearMethod"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod = (pgsTypes::LongitudinalReinforcementForShearMethod)temp;

         // WSDOT method has been rescinded
         m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod = pgsTypes::LongitudinalReinforcementForShearMethod::LRFD;
      }
      else
      {
         m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod = pgsTypes::LongitudinalReinforcementForShearMethod::LRFD;
      }


      if (7.0 <= version)
      {
         if (!pLoad->Property(_T("IncludeRebar_Shear"), &temp))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         m_ShearCapacityCriteria.bIncludeRebar = (temp == 0 ? false : true);
      }
   }

   // *this parameter is no longer used, just gobble it up
   if (!pLoad->Property(_T("CreepMethod"), &temp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   // *this parameter is no longer used, just gobble it up
   if (!pLoad->Property(_T("CreepFactor"), &temp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (3.0 <= version)
   {
      if (!pLoad->Property(_T("CreepDuration1Min"), &m_CreepCriteria.CreepDuration1Min))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CreepDuration1Max"), &m_CreepCriteria.CreepDuration1Max))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CreepDuration2Min"), &m_CreepCriteria.CreepDuration2Min))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CreepDuration2Max"), &m_CreepCriteria.CreepDuration2Max))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("XferTime"), &m_CreepCriteria.XferTime))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (3.1 <= version && version < 3.2)
      {
         if (!pLoad->Property(_T("NoncompositeCreepDuration"), &m_CreepCriteria.TotalCreepDuration))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      if (3.2 <= version)
      {
         if (!pLoad->Property(_T("TotalCreepDuration"), &m_CreepCriteria.TotalCreepDuration))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }
   else if (1.6 <= version && version < 3.0)
   {
      if (!pLoad->Property(_T("CreepDuration1"), &m_CreepCriteria.CreepDuration1Min))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_CreepCriteria.CreepDuration1Max = m_CreepCriteria.CreepDuration1Min;

      if (!pLoad->Property(_T("CreepDuration2"), &m_CreepCriteria.CreepDuration2Min))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_CreepCriteria.CreepDuration2Max = m_CreepCriteria.CreepDuration2Min;

      if (!pLoad->Property(_T("XferTime"), &m_CreepCriteria.XferTime))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      // only one creep duration was entered - need to make some assumptions here
      if (!pLoad->Property(_T("CreepDuration"), &m_CreepCriteria.CreepDuration2Min))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_CreepCriteria.CreepDuration2Max = m_CreepCriteria.CreepDuration2Min;

      if (!pLoad->Property(_T("XferTime"), &m_CreepCriteria.XferTime))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 duration_days = WBFL::Units::ConvertFromSysUnits(m_CreepCriteria.CreepDuration2Min, WBFL::Units::Measure::Day);
      Float64 xfer_days = WBFL::Units::ConvertFromSysUnits(m_CreepCriteria.XferTime, WBFL::Units::Measure::Day);
      if (duration_days - 30 > xfer_days)
      {
         m_CreepCriteria.CreepDuration1Min = WBFL::Units::ConvertToSysUnits(duration_days - 30, WBFL::Units::Measure::Day);
         m_CreepCriteria.CreepDuration1Max = m_CreepCriteria.CreepDuration1Min;
      }
      else
      {
         m_CreepCriteria.CreepDuration1Min = (m_CreepCriteria.CreepDuration2Min + m_CreepCriteria.XferTime) / 2.0;
         m_CreepCriteria.CreepDuration1Max = m_CreepCriteria.CreepDuration1Min;
      }
   }

   if (44 <= version)
   {
      if (!pLoad->Property(_T("CamberVariability"), &m_CreepCriteria.CamberVariability))
      {
         m_CreepCriteria.CamberVariability = 0.50; // the call failed, make sure the variable is set to its default value (it gets changed in the call)

         // it is ok to fail it version is 50 or less
         if (version < 50)
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   if (!pLoad->Property(_T("LossMethod"), &temp))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   m_PrestressLossCriteria.LossMethod = (decltype(m_PrestressLossCriteria.LossMethod))temp;
   m_RefactoredParameters.m_bUpdateLumpSumLosses = m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::GENERAL_LUMPSUM ? true : false;

   if (50 <= version)
   {
      // added in version 50
      if (!pLoad->Property(_T("TimeDependentModel"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_PrestressLossCriteria.TimeDependentConcreteModel = (decltype(m_PrestressLossCriteria.TimeDependentConcreteModel))temp;
   }

   if (50 <= version)
   {
      if (!pLoad->Property(_T("ShippingLosses"), &m_PrestressLossCriteria.ShippingLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_RefactoredParameters.m_ShippingLosses = m_PrestressLossCriteria.ShippingLosses;

      if (!pLoad->Property(_T("ShippingTime"), &m_PrestressLossCriteria.ShippingTime))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      if (!pLoad->Property(_T("FinalLosses"), &m_RefactoredParameters.m_FinalLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ShippingLosses"), &m_PrestressLossCriteria.ShippingLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_RefactoredParameters.m_ShippingLosses = m_PrestressLossCriteria.ShippingLosses;

      if (!pLoad->Property(_T("BeforeXferLosses"), &m_RefactoredParameters.m_BeforeXferLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AfterXferLosses"), &m_RefactoredParameters.m_AfterXferLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (13.0 <= version)
      {
         if (!pLoad->Property(_T("ShippingTime"), &m_PrestressLossCriteria.ShippingTime))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (22 <= version)
      {
         if (!pLoad->Property(_T("LiftingLosses"), &m_RefactoredParameters.m_LiftingLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("BeforeTempStrandRemovalLosses"), &m_RefactoredParameters.m_BeforeTempStrandRemovalLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AfterTempStrandRemovalLosses"), &m_RefactoredParameters.m_AfterTempStrandRemovalLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AfterDeckPlacementLosses"), &m_RefactoredParameters.m_AfterDeckPlacementLosses))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (38 <= version)
         {
            if (!pLoad->Property(_T("AfterSIDLLosses"), &m_RefactoredParameters.m_AfterSIDLLosses))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            m_RefactoredParameters.m_AfterSIDLLosses = m_RefactoredParameters.m_AfterDeckPlacementLosses;
         }
      }
      else
      {
         m_RefactoredParameters.m_LiftingLosses = m_RefactoredParameters.m_AfterXferLosses;
         m_RefactoredParameters.m_BeforeTempStrandRemovalLosses = (m_RefactoredParameters.m_ShippingLosses < 0.0) ? m_RefactoredParameters.m_LiftingLosses : m_RefactoredParameters.m_ShippingLosses;
         m_RefactoredParameters.m_AfterTempStrandRemovalLosses = m_RefactoredParameters.m_BeforeTempStrandRemovalLosses;
         m_RefactoredParameters.m_AfterDeckPlacementLosses = m_RefactoredParameters.m_FinalLosses;
         m_RefactoredParameters.m_AfterSIDLLosses = m_RefactoredParameters.m_FinalLosses;
      }
   }

   if (19 <= version)
   {
      if (!pLoad->Property(_T("CuringMethodFactor"), &m_CreepCriteria.CuringMethodTimeAdjustmentFactor))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   // added in version 1.6
   if (1.5 <= version)
   {
      if (!pLoad->Property(_T("CheckStrandStressAtJacking"), &m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AtJacking]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->Property(_T("CheckStrandStressBeforeTransfer"), &m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::BeforeTransfer]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_BeforeTransfer_StressRel"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_BeforeTransfer_LowRelax"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->Property(_T("CheckStrandStressAfterTransfer"), &m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterTransfer]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterTransfer_StressRel"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterTransfer_LowRelax"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->Property(_T("CheckStrandStressAfterAllLosses"), &m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterAllLosses]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"), &m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

   }
   else
   {
      switch (m_SpecificationCriteria.Edition)
      {
      case WBFL::LRFD::BDSManager::Edition::FirstEdition1994:
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AtJacking] = true;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::BeforeTransfer] = false;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterTransfer] = true;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterAllLosses] = true;
         break;

      case WBFL::LRFD::BDSManager::Edition::FirstEditionWith1996Interims:
      case WBFL::LRFD::BDSManager::Edition::FirstEditionWith1997Interims:
      case WBFL::LRFD::BDSManager::Edition::SecondEdition1998:
      case WBFL::LRFD::BDSManager::Edition::SecondEditionWith1999Interims:
      case WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims:
      case WBFL::LRFD::BDSManager::Edition::SecondEditionWith2001Interims:
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AtJacking] = false;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::BeforeTransfer] = true;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterTransfer] = false;
         m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterAllLosses] = true;
         break;
      }
   }

   if (50 <= version)
   {
      // added in version 50
      if (!pLoad->Property(_T("CheckTendonStressAtJacking"), &m_TendonStressCriteria.bCheckAtJacking))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CheckTendonStressPriorToSeating"), &m_TendonStressCriteria.bCheckPriorToSeating))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_PriorToSeating_StressRel"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_PriorToSeating_LowRelax"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::StressRelieved]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"), &m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::LowRelaxation]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (22 < version && version < 50)
   {
      // added in version 23 and removed in version 50
      m_RefactoredParameters.m_bUpdatePTParameters = true;
      if (!pLoad->Property(_T("AnchorSet"), &m_RefactoredParameters.m_Dset))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("WobbleFriction"), &m_RefactoredParameters.m_WobbleFriction))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("CoefficientOfFriction"), &m_RefactoredParameters.m_FrictionCoefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (39 < version)
   {
      // added in version 40
      int rlm;
      if (!pLoad->Property(_T("RelaxationLossMethod"), &rlm))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_PrestressLossCriteria.RelaxationLossMethod = (decltype(m_PrestressLossCriteria.RelaxationLossMethod))rlm;

      if (!pLoad->Property(_T("SlabElasticGain"), &m_PrestressLossCriteria.SlabElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("HaunchElasticGain"), &m_PrestressLossCriteria.SlabPadElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("DiaphragmElasticGain"), &m_PrestressLossCriteria.DiaphragmElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("UserDCElasticGainBS1"), &m_PrestressLossCriteria.UserDCElasticGain_BeforeDeckPlacement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("UserDWElasticGainBS1"), &m_PrestressLossCriteria.UserDWElasticGain_BeforeDeckPlacement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("UserDCElasticGainBS2"), &m_PrestressLossCriteria.UserDCElasticGain_AfterDeckPlacement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("UserDWElasticGainBS2"), &m_PrestressLossCriteria.UserDWElasticGain_AfterDeckPlacement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("RailingSystemElasticGain"), &m_PrestressLossCriteria.RailingSystemElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("OverlayElasticGain"), &m_PrestressLossCriteria.OverlayElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SlabShrinkageElasticGain"), &m_PrestressLossCriteria.SlabShrinkageElasticGain))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (41 < version)
      {
         // added version 42
         if (!pLoad->Property(_T("LiveLoadElasticGain"), &m_PrestressLossCriteria.LiveLoadElasticGain))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (44 < version)
      {
         if (m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013)
         {
            Int16 value;
            if (!pLoad->Property(_T("FcgpComputationMethod"), &value))
            {
               if (MAX_OVERLAP_VERSION < version)
               {
                  // *this was added in version 45 for PGSuper version 2.9.
                  // At the same time, PGSuper 3.0 was being built. The data block version was
                  // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                  // then the data file format is invalid.
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
            m_PrestressLossCriteria.FcgpComputationMethod = (decltype(m_PrestressLossCriteria.FcgpComputationMethod))value;
         }
      }
   }

   // added in version 1.7
   if (1.6 < version)
   {
      if (!pLoad->Property(_T("CheckLiveLoadDeflection"), &m_LiveLoadDeflectionCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("LiveLoadDeflectionLimit"), &m_LiveLoadDeflectionCriteria.DeflectionLimit))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

   }
   else
   {
      m_LiveLoadDeflectionCriteria.bCheck = true;
      m_LiveLoadDeflectionCriteria.DeflectionLimit = 800.0;
   }

   // added in version 8.0 and removed in version 28
   if (8.0 <= version && version < 28)
   {
      long value;
      if (!pLoad->Property(_T("AnalysisType"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_RefactoredParameters.m_AnalysisType = (pgsTypes::AnalysisType)(value);
      m_RefactoredParameters.m_bHasAnalysisType = true;
   }

   if (10.0 <= version && version < 37)
   {
      if (!pLoad->Property(_T("MaxSlabFc"), &m_LimitsCriteria.MaxSlabFc[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFci"), &m_LimitsCriteria.MaxSegmentFci[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFc"), &m_LimitsCriteria.MaxSegmentFc[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxConcreteAggSize"), &m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else if (37 <= version)
   {
      if (!pLoad->BeginUnit(_T("Limits")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("Normal")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 normal_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("MaxSlabFc"), &m_LimitsCriteria.MaxSlabFc[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFci"), &m_LimitsCriteria.MaxSegmentFci[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFc"), &m_LimitsCriteria.MaxSegmentFc[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1.0 < normal_version) // added in version 2.0
      {
         if (!pLoad->Property(_T("MaxClosureFci"), &m_LimitsCriteria.MaxClosureFci[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("MaxClosureFc"), &m_LimitsCriteria.MaxClosureFc[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxConcreteAggSize"), &m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // Normal
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }


      if (!pLoad->BeginUnit(_T("AllLightweight")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 alw_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("MaxSlabFc"), &m_LimitsCriteria.MaxSlabFc[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFci"), &m_LimitsCriteria.MaxSegmentFci[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFc"), &m_LimitsCriteria.MaxSegmentFc[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1.0 < alw_version) // added in version 2.0
      {
         if (!pLoad->Property(_T("MaxClosureFci"), &m_LimitsCriteria.MaxClosureFci[pgsTypes::AllLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("MaxClosureFc"), &m_LimitsCriteria.MaxClosureFc[pgsTypes::AllLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxConcreteAggSize"), &m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // AllLightweight
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->BeginUnit(_T("SandLightweight")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 slw_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("MaxSlabFc"), &m_LimitsCriteria.MaxSlabFc[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFci"), &m_LimitsCriteria.MaxSegmentFci[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxGirderFc"), &m_LimitsCriteria.MaxSegmentFc[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (1.0 < slw_version) // added in version 2.0
      {
         if (!pLoad->Property(_T("MaxClosureFci"), &m_LimitsCriteria.MaxClosureFci[pgsTypes::SandLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("MaxClosureFc"), &m_LimitsCriteria.MaxClosureFc[pgsTypes::SandLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MaxConcreteAggSize"), &m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // SandLightweight
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // Limits
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (44 <= version)
   {
      bool bOKToFail = false;
      if (version <= MAX_OVERLAP_VERSION)
      {
         // *this was added in version 45 for PGSuper version 2.9.
         // At the same time, PGSuper 3.0 was being built. The data block version was
         // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
         // then the data file format is invalid.
         bOKToFail = true;
      }

      bool bBeginUnit = pLoad->BeginUnit(_T("Warnings"));
      if (!bBeginUnit && !bOKToFail)
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (bBeginUnit)
      {
         // reading the unit start successfully... keep going
         if (!pLoad->Property(_T("DoCheckStirrupSpacingCompatibility"), &m_LimitsCriteria.bCheckStirrupSpacingCompatibility))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         Float64 warningsVersion = pLoad->GetVersion();
         if (2 <= warningsVersion)
         {
            if (!pLoad->Property(_T("CheckGirderSag"), &m_LimitsCriteria.bCheckSag))
               THROW_LOAD(InvalidFileFormat, pLoad);

            int value;
            if (!pLoad->Property(_T("SagCamberType"), &value))
               THROW_LOAD(InvalidFileFormat, pLoad);

            m_LimitsCriteria.SagCamber = (pgsTypes::SagCamber)value;
         }

         if (!pLoad->EndUnit()) // Warnings
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }

   // Added in 14.0, removed in version 41
   if (14.0 <= version && version < 41)
   {
      std::_tstring strLimitState[] = { _T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI") };

      pLoad->BeginUnit(_T("LoadFactors"));
      Float64 lf_version = pLoad->GetVersion();
      int nLimitStates = 4;
      if (2 <= lf_version)
      {
         nLimitStates += 1; // added Strength II
      }

      if (3 <= lf_version)
      {
         nLimitStates += 1; // added Fatigue I
      }

      for (int i = 0; i < nLimitStates; i++)
      {
         pLoad->BeginUnit(strLimitState[i].c_str());

         pLoad->Property(_T("DCmin"), &m_RefactoredParameters.m_DCmin[i]);
         pLoad->Property(_T("DCmax"), &m_RefactoredParameters.m_DCmax[i]);
         pLoad->Property(_T("DWmin"), &m_RefactoredParameters.m_DWmin[i]);
         pLoad->Property(_T("DWmax"), &m_RefactoredParameters.m_DWmax[i]);
         pLoad->Property(_T("LLIMmin"), &m_RefactoredParameters.m_LLIMmin[i]);
         pLoad->Property(_T("LLIMmax"), &m_RefactoredParameters.m_LLIMmax[i]);

         pLoad->EndUnit();

         m_RefactoredParameters.m_bUpdateLoadFactors = true; // need to update load factors in PGS with these values
      }
      pLoad->EndUnit();
   }

   // added in version 15
   if (version < 15)
   {
      m_SlabOffsetCriteria.bCheck = true;
      m_SlabOffsetCriteria.bDesign = true;
   }
   else
   {
      if (!pLoad->Property(_T("EnableSlabOffsetCheck"), &m_SlabOffsetCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("EnableSlabOffsetDesign"), &m_SlabOffsetCriteria.bDesign))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 15)
   {
      m_HarpedStrandDesignCriteria.StrandFillType = ftMinimizeHarping;
   }
   else
   {
      long ftype;
      if (!pLoad->Property(_T("DesignStrandFillType"), &ftype))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_HarpedStrandDesignCriteria.StrandFillType = ftype == (long)ftGridOrder ? ftGridOrder : ftMinimizeHarping;
   }

   if (16 <= version)
   {
      long value;
      if (!pLoad->Property(_T("EffectiveFlangeWidthMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_SectionPropertiesCriteria.EffectiveFlangeWidthMethod = (pgsTypes::EffectiveFlangeWidthMethod)(value);
   }

   // only a valid input between data block version 17 and 23
   // *this input is no longer used, so just gobble it up
   if (17 <= version && version <= 23)
   {
      long value;
      if (!pLoad->Property(_T("SlabOffsetMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (18 <= version && version < 37)
   {
      long value;
      if (!pLoad->Property(_T("ShearFlowMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_InterfaceShearCriteria.ShearFlowMethod = (pgsTypes::ShearFlowMethod)(value);

      // MaxInterfaceShearConnectorSpacing wasn't available in *this version of the input
      // the default value is 48". Set the value to match the spec
      if (m_SpecificationCriteria.Edition < WBFL::LRFD::BDSManager::Edition::SeventhEdition2014)
      {
         if (m_SpecificationCriteria.Units == WBFL::LRFD::BDSManager::Units::US)
         {
            m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(24.0, WBFL::Units::Measure::Inch);
         }
         else
         {
            m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(0.6, WBFL::Units::Measure::Meter);
         }
      }

      if (!pLoad->Property(_T("ShearCapacityMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      switch (value)
      {
      case 0: // scmGeneral -> pgsTypes::scmBTEquations
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
         break;

      case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmWSDOT2001;
         break;

      case 2: // scmSimplified -> pgsTypes::scmVciVcw
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmVciVcw;
         break;

      default:
         //ATLASSERT(false); do nothing...
         // if value is 3 or 4 it is the correct stuff
         m_ShearCapacityCriteria.CapacityMethod = (pgsTypes::ShearCapacityMethod)value;
      }

      // The general method from the 2007 spec becomes the tables method in the 2008 spec
      // make that adjustment here
      if (m_SpecificationCriteria.Edition < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims && m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmBTEquations)
      {
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTTables;
      }

      // if *this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
      if (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= m_SpecificationCriteria.Edition && m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmWSDOT2007)
      {
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
      }
   }
   else if (version < 18)
   {
      // *this is before Version 18... shear capacity method was read above in a now obsolete parameter
      // translate that parameter here
      m_ShearCapacityCriteria.CapacityMethod = (pgsTypes::ShearCapacityMethod)shear_capacity_method;
   }
   else
   {
      ATLASSERT(37 <= version);
      if (!pLoad->BeginUnit(_T("Shear")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      Float64 shear_version = pLoad->GetVersion();

      if (!pLoad->Property(_T("LongReinfShearMethod"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod = (pgsTypes::LongitudinalReinforcementForShearMethod)temp;

      // WSDOT method has been rescinded
      m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod = pgsTypes::LongitudinalReinforcementForShearMethod::LRFD;

      if (!pLoad->Property(_T("IncludeRebarForCapacity"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_ShearCapacityCriteria.bIncludeRebar = (temp == 0 ? false : true);

      long value;
      if (!pLoad->Property(_T("ShearFlowMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      m_InterfaceShearCriteria.ShearFlowMethod = (pgsTypes::ShearFlowMethod)(value);

      if (1 < shear_version)
      {
         // NOTE: PGSuper 2.9 changed the version number of *this data block to 2
         // PGSuper 3.0 was using version 2 at the same time. There will be files
         // that were created with PGSuper 2.9 that will be read by PGSuper 3.0.
         // In those cases the following Property will fail. That's ok. Just move
         // on. If it succeeds, the MaxInterfaceShearConnectorSpacing information is available
         // so read it.
         // For *this reason the return value from *this pLoad->Property is not check. Everything
         // is OK if it passes or fails
         // 
         // ***** NOTE ***** DO NOT TEST THE RETULT VALUE
         pLoad->Property(_T("MaxInterfaceShearConnectorSpacing"), &m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing);
      }
      else
      {
         // prior to 7th Edition 2014 max spacing was 24 inches... 
         if (m_SpecificationCriteria.Edition < WBFL::LRFD::BDSManager::Edition::SeventhEdition2014)
         {
            if (m_SpecificationCriteria.Units == WBFL::LRFD::BDSManager::Units::US)
            {
               m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(24.0, WBFL::Units::Measure::Inch);
            }
            else
            {
               m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(0.6, WBFL::Units::Measure::Meter);
            }
         }
      }

      if (2 < shear_version)
      {
         if (!pLoad->Property(_T("UseDeckWeightForPc"), &m_InterfaceShearCriteria.bUseDeckWeightForPc)) // added in version 3 of Shear data block
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("ShearCapacityMethod"), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      switch (value)
      {
      case 0: // scmGeneral -> pgsTypes::scmBTEquations
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
         break;

      case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmWSDOT2001;
         break;

      case 2: // scmSimplified -> pgsTypes::scmVciVcw
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmVciVcw;
         break;

      default:
         //ATLASSERT(false); do nothing...
         // if value is 3 or 4 it is the correct stuff
         m_ShearCapacityCriteria.CapacityMethod = (pgsTypes::ShearCapacityMethod)value;
      }

      // The general method from the 2007 spec becomes the tables method in the 2008 spec
      // make that adjustment here
      if (m_SpecificationCriteria.Edition < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims && m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmBTEquations)
      {
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTTables;
      }

      // if *this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
      if (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= m_SpecificationCriteria.Edition && m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmWSDOT2007)
      {
         m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
      }

      if (3 < shear_version)
      {
         if (!pLoad->Property(_T("LimitNetTensionStrainToPositiveValues"), &m_ShearCapacityCriteria.bLimitNetTensionStrainToPositiveValues)) // added in version 4 of *this data block
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      // Fixed misspelling in version 2 of the Shear data block
      if (shear_version < 2)
      {
         // before version 2 it was named ReductionFactor
         if (!pLoad->BeginUnit(_T("ReductionFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else if (shear_version == 2)
      {
         // for version 2, because of the overlap, it can be ReductionFactor or ResistanceFactor
         if (!pLoad->BeginUnit(_T("ReductionFactor")) && !pLoad->BeginUnit(_T("ResistanceFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         ATLASSERT(2 < shear_version);
         // greater than version 2, ResistanceFactor
         if (!pLoad->BeginUnit(_T("ResistanceFactor")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("Normal"), &m_ShearCapacityCriteria.Phi[pgsTypes::Normal]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AllLightweight"), &m_ShearCapacityCriteria.Phi[pgsTypes::AllLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SandLightweight"), &m_ShearCapacityCriteria.Phi[pgsTypes::SandLightweight]))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // ResistanceFactor
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (64 <= version)
      {
         if (!pLoad->BeginUnit(_T("ResistanceFactorDebonded")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         Float64 data_block_version = pLoad->GetVersion();

         if (!pLoad->Property(_T("Normal"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::Normal]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AllLightweight"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::AllLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("SandLightweight"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::SandLightweight]))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (1 < data_block_version)
         {
            // added version 2 of *this data block
            if (!pLoad->Property(_T("PCI_UHPC"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::PCI_UHPC]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("UHPC"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::UHPC])
               &&
               !pLoad->Property(_T("FHWA_UHPC"), &m_ShearCapacityCriteria.PhiDebonded[pgsTypes::UHPC]))
            {
               // Early versions of *this property used FHWA_UHPC. If UHPC fails, try FHWA_UHPC
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if (!pLoad->EndUnit()) // ResistanceFactorDebonded
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      // Added ClosureJointResistanceFactor in version 2 of the Shear data block
      if (1 < shear_version)
      {
         // NOTE: PGSuper 2.9 changed the version number of *this data block to 2
         // PGSuper 3.0 was using version 2 at the same time. There will be files
         // that were created with PGSuper 2.9 that will be read by PGSuper 3.0.
         // In those cases the following BeginUnit will fail. That's ok. Just move
         // on. If it succeeds, the ClosureJointResistanceFactor information is available
         // so read it
         if (pLoad->BeginUnit(_T("ClosureJointResistanceFactor")))
         {
            if (!pLoad->Property(_T("Normal"), &m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::Normal]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("AllLightweight"), &m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::AllLightweight]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("SandLightweight"), &m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::SandLightweight]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->EndUnit()) // ClosureJointResistanceFactor
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
      }

      if (!pLoad->EndUnit()) // Shear
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (version < 26)
   {
      if (m_SpecificationCriteria.Units == WBFL::LRFD::BDSManager::Units::SI)
      {
         m_LiveLoadCriteria.PedestrianLoad = WBFL::Units::ConvertToSysUnits(3.6e-3, WBFL::Units::Measure::MPa);
         m_LiveLoadCriteria.MinSidewalkWidth = WBFL::Units::ConvertToSysUnits(600., WBFL::Units::Measure::Millimeter);
      }
      else
      {
         m_LiveLoadCriteria.PedestrianLoad = WBFL::Units::ConvertToSysUnits(0.075, WBFL::Units::Measure::KSF);
         m_LiveLoadCriteria.MinSidewalkWidth = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Feet);
      }
   }
   else
   {
      // added in version 26
      if (!pLoad->Property(_T("PedestrianLoad"), &m_LiveLoadCriteria.PedestrianLoad))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("MinSidewalkWidth"), &m_LiveLoadCriteria.MinSidewalkWidth))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }


   if (32 <= version)
   {

      if (!pLoad->Property(_T("PrestressTransferComputationType"), &temp))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      m_TransferLengthCriteria.CalculationMethod = (temp == 0 ? pgsTypes::TransferLengthCalculationMethod::MinuteValue : pgsTypes::TransferLengthCalculationMethod::Specification);
   }


   if (38 <= version)
   {
      // *this was removed in version 83 - straight strands are always allowed to be extended
      bool bBeginUnit = pLoad->BeginUnit(_T("StrandExtensions"));

      if (bBeginUnit)
      {
         bool bDummy; // load the old data and gobble it up
         if (!pLoad->Property(_T("AllowStraightStrandExtensions"), &bDummy))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         pLoad->EndUnit();
      }
   }

   if (52 <= version)
   {
      // added in version 52
      bool bBeginUnit = pLoad->BeginUnit(_T("DuctSize"));
      if (bBeginUnit)
      {
         if (!pLoad->Property(_T("DuctAreaPushRatio"), &m_DuctSizeCriteria.DuctAreaPushRatio))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("DuctAreaPullRatio"), &m_DuctSizeCriteria.DuctAreaPullRatio))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("DuctDiameterRatio"), &m_DuctSizeCriteria.DuctDiameterRatio))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         pLoad->EndUnit(); // DuctSize
      }
   }


   if (50 <= version)
   {
      // added version 50
      pLoad->BeginUnit(_T("ClosureJoint"));
      if (!pLoad->Property(_T("ClosureCompStressAtStressing"), &m_ClosureJointCriteria.CompressionStressCoefficient_BeforeLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressPTZAtStressing"), &m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressPTZWithRebarAtStressing"), &m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressAtStressing"), &m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressWithRebarAtStressing"), &m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureCompStressAtService"), &m_ClosureJointCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureCompStressWithLiveLoadAtService"), &m_ClosureJointCriteria.CompressionStressCoefficient_AllLoads_AfterLosses))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressPTZAtService"), &m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressPTZWithRebarAtService"), &m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressAtService"), &m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureTensStressWithRebarAtService"), &m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Coefficient))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("ClosureCompStressFatigue"), &m_ClosureJointCriteria.CompressionStressCoefficient_Fatigue))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      pLoad->EndUnit(); // Closure Joint
   }


   // added in version 48
   if (47 < version)
   {
      bool bOKToFail = false;
      if (version <= MAX_OVERLAP_VERSION)
      {
         // *this was added in version 48 for PGSuper version 2.9.
         // At the same time, PGSuper 3.0 was being built. The data block version was
         // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
         // then the data file format is invalid.
         bOKToFail = true;
      }

      bool bSucceeded = pLoad->Property(_T("CheckBottomFlangeClearance"), &m_BottomFlangeClearanceCriteria.bCheck);
      if (bSucceeded)
      {
         if (!pLoad->Property(_T("MinBottomFlangeClearance"), &m_BottomFlangeClearanceCriteria.MinClearance))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         if (!bOKToFail)
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
   }


   // added in version 57
   if (56 < version)
   {
      if (!pLoad->Property(_T("CheckGirderInclination"), &m_GirderInclinationCriteria.bCheck))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (version < 71)
      {
         // removed in version 71
         Float64 value; // waste the value
         if (!pLoad->Property(_T("InclindedGirder_BrgPadDeduction"), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->Property(_T("InclindedGirder_FSmax"), &m_GirderInclinationCriteria.FS))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }


   // added in version 62
   if (61 < version)
   {
      if (!pLoad->Property(_T("FinishedElevationTolerance"), &m_SlabOffsetCriteria.FinishedElevationTolerance))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   // slab offset rounding option added in version 73
   if (72 < version)
   {
      if (!pLoad->Property(_T("SlabOffsetRoundingMethod"), (long*)&m_SlabOffsetCriteria.RoundingMethod))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("SlabOffsetRoundingTolerance"), &m_SlabOffsetCriteria.SlabOffsetTolerance))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      // *this was the hard-coded default for versions before 5.x
      m_SlabOffsetCriteria.RoundingMethod = pgsTypes::sormRoundNearest;
      m_SlabOffsetCriteria.SlabOffsetTolerance = m_SpecificationCriteria.Units == WBFL::LRFD::BDSManager::Units::US ? WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch) : WBFL::Units::ConvertToSysUnits(5.0, WBFL::Units::Measure::Millimeter);
   }

   // Bearings was added in verison 79
   if (78 < version)
   {
      if (!pLoad->BeginUnit(_T("Bearings")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("AlertTaperedSolePlateRequirement"), &m_BearingCriteria.bAlertTaperedSolePlateRequirement))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("TaperedSolePlaneInclinationThreshold"), &m_BearingCriteria.TaperedSolePlateInclinationThreshold))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->Property(_T("UseImpactForBearingReactions"), &m_BearingCriteria.bUseImpactForBearingReactions))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (!pLoad->EndUnit()) // Bearings
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (!pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if (version < 72)
   {
      if (m_PrestressLossCriteria.LossMethod != PrestressLossCriteria::LossMethodType::TIME_STEP && m_LimitStateConcreteStrengthCriteria.LimitStateConcreteStrength == pgsTypes::lscStrengthAtTimeOfLoading)
      {
         // LimitStateConcreteStrength was only used for time step losses before version 72
         // now it is always used
         // strength at time of loading only a valid option for time step analysis... change it to a valid value
         m_LimitStateConcreteStrengthCriteria.LimitStateConcreteStrength = pgsTypes::lscSpecifiedStrength;
      }
   }

   return true;
}

/*
* *this the SaveMe method prior to Version 83 of the SpecificationLibraryEntry data unit.
* The entire SpecLibraryEntryImpl class was refactored to use "Criteria" objects and the data
* unit was incremented to version 83. *this is the SaveMe method prior to that and it
* is retained here as commented out code for future reference for knowing how
* older versions of files where saved
bool SpecLibraryEntryImpl::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("SpecificationLibraryEntry"), CURRENT_VERSION);
   pSave->Property(_T("Name"), *this->GetName().c_str());
   pSave->Property(_T("Description"), *this->GetDescription(false).c_str());

   pSave->Property(_T("UseCurrentSpecification"), m_bUseCurrentSpecification); // added in version 77
   pSave->Property(_T("SpecificationType"), WBFL::LRFD::BDSManager::GetEditionAsString(m_SpecificationType, true));

   if (m_SpecificationUnits == WBFL::LRFD::BDSManager::Units::SI)
   {
      pSave->Property(_T("SpecificationUnits"), _T("SiUnitsSpec"));
   }
   else if (m_SpecificationUnits == WBFL::LRFD::BDSManager::Units::US)
   {
      pSave->Property(_T("SpecificationUnits"), _T("UsUnitsSpec"));
   }
   else
   {
      ASSERT(0);
   }

   pSave->Property(_T("SectionPropertyType"), m_SectionPropertyMode); // added version 50

   pSave->Property(_T("DoCheckStrandSlope"), m_DoCheckStrandSlope);
   pSave->Property(_T("DoDesignStrandSlope"), m_DoDesignStrandSlope);
   pSave->Property(_T("MaxSlope05"), m_MaxSlope05);
   pSave->Property(_T("MaxSlope06"), m_MaxSlope06);
   pSave->Property(_T("MaxSlope07"), m_MaxSlope07); // added in version 35
   pSave->Property(_T("DoCheckHoldDown"), m_DoCheckHoldDown);
   pSave->Property(_T("DoDesignHoldDown"), m_DoDesignHoldDown);
   pSave->Property(_T("HoldDownForceType"), m_HoldDownForceType); // added in version 70
   pSave->Property(_T("HoldDownForce"), m_HoldDownForce);
   pSave->Property(_T("HoldDownFriction"), m_HoldDownFriction); // added in version 70
   pSave->Property(_T("DoCheckSplitting"), m_DoCheckSplitting);
   pSave->Property(_T("DoDesignSplitting"), m_DoDesignSplitting);
   pSave->Property(_T("DoCheckConfinement"), m_DoCheckConfinement);
   pSave->Property(_T("DoDesignConfinement"), m_DoDesignConfinement);

   pSave->Property(_T("DoCheckHandlingWeightLimit"), m_bCheckHandlingWeightLimit); // added version 70
   pSave->Property(_T("HandlingWeightLimit"), m_HandlingWeightLimit); // added version 70

   //pSave->Property(_T("MaxStirrupSpacing"), m_MaxStirrupSpacing); // removed in version 46
   pSave->Property(_T("StirrupSpacingCoefficient1"), m_StirrupSpacingCoefficient[0]); // added in version 46
   pSave->Property(_T("MaxStirrupSpacing1"), m_MaxStirrupSpacing[0]); // added in version 46
   pSave->Property(_T("StirrupSpacingCoefficient2"), m_StirrupSpacingCoefficient[1]); // added in version 46
   pSave->Property(_T("MaxStirrupSpacing2"), m_MaxStirrupSpacing[1]); // added in version 46
   pSave->Property(_T("CyLiftingCrackFs"), m_CyLiftingCrackFs);
   pSave->Property(_T("CyLiftingFailFs"), m_CyLiftingFailFs);
   pSave->Property(_T("CyCompStressServ"), m_CyCompStressServ);
   //pSave->Property(_T("CyCompStressLifting"), m_LiftingCompressionStressCoefficient_GlobalStress); // removed in version 65
   pSave->Property(_T("CyGlobalCompStressLifting"), m_LiftingCompressionStressCoefficient_GlobalStress); // added in version 65
   pSave->Property(_T("CyPeakCompStressLifting"), m_LiftingCompressionStressCoefficient_PeakStress); // added in version 65
   pSave->Property(_T("CyTensStressServ"), m_CyTensStressServ);
   pSave->Property(_T("CyDoTensStressServMax"), m_CyDoTensStressServMax);
   pSave->Property(_T("CyTensStressServMax"), m_CyTensStressServMax);
   pSave->Property(_T("CyTensStressLifting"), m_CyTensStressLifting);
   pSave->Property(_T("CyDoTensStressLiftingMax"), m_CyDoTensStressLiftingMax);
   pSave->Property(_T("CyTensStressLiftingMax"), m_CyTensStressLiftingMax);
   pSave->Property(_T("BurstingZoneLengthFactor"), m_SplittingZoneLengthFactor);
   //pSave->Property(_T("UHPCStregthAtFirstCrack"), m_UHPCStregthAtFirstCrack); // added in version 75, removed version 82
   pSave->Property(_T("LiftingUpwardImpact"), m_LiftingUpwardImpact);
   pSave->Property(_T("LiftingDownwardImpact"), m_LiftingDownwardImpact);
   pSave->Property(_T("HaulingUpwardImpact"), m_HaulingUpwardImpact);
   pSave->Property(_T("HaulingDownwardImpact"), m_HaulingDownwardImpact);
   pSave->Property(_T("CuringMethod"), (Int16)m_CuringMethod);
   pSave->Property(_T("EnableLiftingCheck"), m_EnableLiftingCheck);
   pSave->Property(_T("EnableLiftingDesign"), m_EnableLiftingDesign);
   pSave->Property(_T("PickPointHeight"), m_PickPointHeight);
   pSave->Property(_T("LiftingLoopTolerance"), m_LiftingLoopTolerance);
   pSave->Property(_T("MinCableInclination"), m_MinCableInclination);
   pSave->Property(_T("MaxGirderSweepLifting"), m_MaxGirderSweepLifting);
   //pSave->Property(_T("LiftingCamberMethod"),(Int32)m_LiftingCamberMethod); // added version 56, removed in version 65
   //pSave->Property(_T("LiftingCamberPercentEstimate"),m_LiftingCamberPercentEstimate); // added version 56, removed in version 65
   pSave->Property(_T("LiftingCamberMultiplier"), m_LiftingCamberMultiplier); // added version 58
   pSave->Property(_T("LiftingWindType"), (Int32)m_LiftingWindType); // added in version 56
   pSave->Property(_T("LiftingWindLoad"), m_LiftingWindLoad); // added in version 56
   //pSave->Property(_T("LiftingStressesPlumbGirder"), m_LiftingStressesPlumbGirder); // added in version 56, removed 59
   //pSave->Property(_T("LiftingStressesEquilibriumAngle"), m_bComputeLiftingStressesAtEquilibriumAngle); // added in version 59, removed in version 65
   pSave->Property(_T("EnableHaulingCheck"), m_EnableHaulingCheck);
   pSave->Property(_T("EnableHaulingDesign"), m_EnableHaulingDesign);
   pSave->Property(_T("HaulingAnalysisMethod"), (Int32)m_HaulingAnalysisMethod); // added version 43
   pSave->Property(_T("MaxGirderSweepHauling"), m_MaxGirderSweepHauling);
   pSave->Property(_T("SweepGrowthHauling"), m_HaulingSweepGrowth); // added version 66
   //pSave->Property(_T("HaulingSupportDistance"),m_HaulingSupportDistance); // removed version 56
   //pSave->Property(_T("MaxHaulingOverhang"), m_MaxHaulingOverhang); // removed version 56
   pSave->Property(_T("HaulingSupportPlacementTolerance"), m_HaulingSupportPlacementTolerance);
   //pSave->Property(_T("HaulingCamberMethod"),(Int32)m_HaulingCamberMethod); // added version 56, removed in version 65
   //pSave->Property(_T("HaulingCamberPercentEstimate"),m_HaulingCamberPercentEstimate); // added version 56, removed in version 65
   pSave->Property(_T("HaulingCamberMultiplier"), m_HaulingCamberMultiplier); // added version 58
   pSave->Property(_T("HaulingWindType"), (Int32)m_HaulingWindType); // added in version 56
   pSave->Property(_T("HaulingWindLoad"), m_HaulingWindLoad); // added in version 56
   pSave->Property(_T("CentrifugalForceType"), (Int32)m_CentrifugalForceType); // added in version 56
   pSave->Property(_T("HaulingSpeed"), m_HaulingSpeed); // added in version 56
   pSave->Property(_T("TurningRadius"), m_TurningRadius); // added in version 56
   //pSave->Property(_T("HaulingStressesEquilibriumAngle"), m_bComputeHaulingStressesAtEquilibriumAngle); // added in version 59, removed in version 65
   //pSave->Property(_T("CompStressHauling"), m_CompStressHauling); // removed in version 65
   pSave->Property(_T("GlobalCompStressHauling"), m_GlobalCompStressHauling); // added in version 65
   pSave->Property(_T("PeakCompStressHauling"), m_PeakCompStressHauling); // added in version 65

   //pSave->Property(_T("TensStressHauling"),m_TensStressHauling); // removed in version 56
   //pSave->Property(_T("DoTensStressHaulingMax"),m_DoTensStressHaulingMax);  // removed in version 56
   //pSave->Property(_T("TensStressHaulingMax"), m_TensStressHaulingMax); // removed in version 56

   pSave->Property(_T("TensStressHaulingNormalCrown"), m_TensStressHauling[pgsTypes::CrownSlope]); // added in version 56
   pSave->Property(_T("DoTensStressHaulingMaxNormalCrown"), m_DoTensStressHaulingMax[pgsTypes::CrownSlope]);  // added in version 56
   pSave->Property(_T("TensStressHaulingMaxNormalCrown"), m_TensStressHaulingMax[pgsTypes::CrownSlope]); // added in version 56

   pSave->Property(_T("TensStressHaulingMaxSuper"), m_TensStressHauling[pgsTypes::Superelevation]); // added in version 56
   pSave->Property(_T("DoTensStressHaulingMaxMaxSuper"), m_DoTensStressHaulingMax[pgsTypes::Superelevation]);  // added in version 56
   pSave->Property(_T("TensStressHaulingMaxMaxSuper"), m_TensStressHaulingMax[pgsTypes::Superelevation]); // added in version 56

   pSave->Property(_T("HeHaulingCrackFs"), m_HaulingCrackFs);
   pSave->Property(_T("HeHaulingFailFs"), m_HaulingRollFs);
   pSave->Property(_T("HaulingImpactUsage"), (Int32)m_HaulingImpactUsage); // added in version 56
   pSave->Property(_T("RoadwayCrownSlope"), m_RoadwayCrownSlope); // added in version 56
   pSave->Property(_T("RoadwaySuperelevation"), m_RoadwaySuperelevation);

   //pSave->Property(_T("TruckRollStiffnessMethod"), (long)m_TruckRollStiffnessMethod); // removed version 56
   //pSave->Property(_T("TruckRollStiffness"), m_TruckRollStiffness); // removed version 56
   //pSave->Property(_T("AxleWeightLimit"), m_AxleWeightLimit); // removed version 56
   //pSave->Property(_T("AxleStiffness"),m_AxleStiffness); // removed version 56
   //pSave->Property(_T("MinRollStiffness"),m_MinRollStiffness); // removed version 56
   //pSave->Property(_T("TruckGirderHeight"), m_TruckGirderHeight); // removed version 56
   //pSave->Property(_T("TruckRollCenterHeight"), m_TruckRollCenterHeight); // removed version 56
   //pSave->Property(_T("TruckAxleWidth"), m_TruckAxleWidth); // removed version 56
   //pSave->Property(_T("HeErectionCrackFs"), m_HeErectionCrackFs); // removed in version 55.0
   //pSave->Property(_T("HeErectionFailFs"), m_HeErectionFailFs); // removed in version 55.0
   //pSave->Property(_T("MaxGirderWgt"),m_MaxGirderWgt); // removed version 56

   // Added at version 53
   pSave->Property(_T("LimitStateConcreteStrength"), m_LimitStateConcreteStrength);

   // Added in version 72
   pSave->Property(_T("Use90DayConcreteStrength"), m_bUse90DayConcreteStrength);
   pSave->Property(_T("SlowCuringConcreteStrengthFactor"), m_90DayConcreteStrengthFactor);

   // modified in version 37 (see below)
   //pSave->Property(_T("HaulingModulusOfRuptureCoefficient"),m_HaulingModulusOfRuptureCoefficient); // added for version 12.0
   //pSave->Property(_T("LiftingModulusOfRuptureCoefficient"),m_LiftingModulusOfRuptureCoefficient); // added for version 20.0


   // Added at version 25
   pSave->Property(_T("MinLiftingPointLocation"), m_MinLiftPoint);
   pSave->Property(_T("LiftingPointLocationAccuracy"), m_LiftPointAccuracy);
   pSave->Property(_T("MinHaulingSupportLocation"), m_MinHaulPoint);
   pSave->Property(_T("HaulingSupportLocationAccuracy"), m_HaulPointAccuracy);

   // KDOT values added at version 43
   pSave->Property(_T("UseMinTruckSupportLocationFactor"), m_UseMinTruckSupportLocationFactor);
   pSave->Property(_T("MinTruckSupportLocationFactor"), m_MinTruckSupportLocationFactor);
   pSave->Property(_T("OverhangGFactor"), m_OverhangGFactor);
   pSave->Property(_T("InteriorGFactor"), m_InteriorGFactor);

   // Added at version 4.0
   pSave->Property(_T("CastingYardTensileStressLimitWithMildRebar"), m_CyTensStressServWithRebar);
   pSave->Property(_T("LiftingTensileStressLimitWithMildRebar"), m_TensStressLiftingWithRebar);
   //pSave->Property(_T("HaulingTensileStressLimitWithMildRebar"),m_TensStressHaulingWithRebar); // removed in version 56
   pSave->Property(_T("HaulingTensileStressLimitWithMildRebarNormalCrown"), m_TensStressHaulingWithRebar[pgsTypes::CrownSlope]); // added in version 56
   pSave->Property(_T("HaulingTensileStressLimitWithMildRebarMaxSuper"), m_TensStressHaulingWithRebar[pgsTypes::Superelevation]); // added in version 56

   // Added at version 30
   pSave->Property(_T("TempStrandRemovalCompStress"), m_TempStrandRemovalCompStress);
   pSave->Property(_T("TempStrandRemovalTensStress"), m_TempStrandRemovalTensStress);
   pSave->Property(_T("TempStrandRemovalDoTensStressMax"), m_TempStrandRemovalDoTensStressMax);
   pSave->Property(_T("TempStrandRemovalTensStressMax"), m_TempStrandRemovalTensStressMax);
   pSave->Property(_T("TempStrandRemovalTensStressWithRebar"), m_TempStrandRemovalTensStressWithRebar); // added version 49

   pSave->Property(_T("CheckTemporaryStresses"), m_bCheckTemporaryStresses); // added in version 47
   pSave->Property(_T("Bs1CompStress"), m_Bs1CompStress); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStress"), m_Bs1TensStress); // removed m_ in version 30
   pSave->Property(_T("Bs1DoTensStressMax"), m_Bs1DoTensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStressMax"), m_Bs1TensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs2CompStress"), m_Bs2CompStress); // removed m_ in version 30

   pSave->Property(_T("CheckBs2Tension"), m_bCheckBs2Tension); // added in version 47
   pSave->Property(_T("Bs2TensStress"), m_Bs2TensStress); // added in version 47
   pSave->Property(_T("Bs2DoTensStressMax"), m_Bs2DoTensStressMax); // added in version 47
   pSave->Property(_T("Bs2TensStressMax"), m_Bs2TensStressMax); // added in version 47

   pSave->Property(_T("Bs2TrafficBarrierDistributionType"), (Int16)m_TrafficBarrierDistributionType); // added in version 36
   pSave->Property(_T("Bs2MaxGirdersTrafficBarrier"), m_Bs2MaxGirdersTrafficBarrier);
   //pSave->Property(_T("Bs2MaxGirdersUtility"), m_Bs2MaxGirdersUtility); // removed in version 55
   pSave->Property(_T("OverlayLoadDistribution"), (Int32)m_OverlayLoadDistribution); // added in version 34
   pSave->Property(_T("HaunchLoadComputationType"), (Int32)m_HaunchLoadComputationType); // added in version 54
   pSave->Property(_T("HaunchLoadCamberTolerance"), m_HaunchLoadCamberTolerance);        // ""
   pSave->Property(_T("HaunchLoadCamberFactor"), m_HaunchLoadCamberFactor);  // added in version 61
   pSave->Property(_T("HaunchAnalysisComputationType"), (Int32)m_HaunchAnalysisSectionPropertiesType); // added in version 63
   pSave->Property(_T("Bs3CompStressServ"), m_Bs3CompStressServ);
   pSave->Property(_T("Bs3CompStressService1A"), m_Bs3CompStressService1A);
   pSave->Property(_T("Bs3TensStressServNc"), m_Bs3TensStressServNc);
   pSave->Property(_T("Bs3DoTensStressServNcMax"), m_Bs3DoTensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServNcMax"), m_Bs3TensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServSc"), m_Bs3TensStressServSc);
   pSave->Property(_T("Bs3DoTensStressServScMax"), m_Bs3DoTensStressServScMax);
   pSave->Property(_T("Bs3TensStressServScMax"), m_Bs3TensStressServScMax);

   // added in version 76
   pSave->Property(_T("PrincipalTensileStressMethod"), m_PrincipalTensileStressMethod);
   pSave->Property(_T("PrincipalTensileStressCoefficient"), m_PrincipalTensileStressCoefficient);
   pSave->Property(_T("PrincipalTensileStressTendonNearnessFactor"), m_PrincipalTensileStressTendonNearnessFactor); // added in version 77
   pSave->Property(_T("PrincipalTensileStressFcThreshold"), m_PrincipalTensileStressFcThreshold); // added in version 78
   pSave->Property(_T("PrincipalTensileStressUngroutedMultiplier"), m_PrincipalTensileStressUngroutedMultiplier); // added in version 80
   pSave->Property(_T("PrincipalTensileStressGroutedMultiplier"), m_PrincipalTensileStressGroutedMultiplier);     //   ""   ""  ""    80


   // removed in version 29
   // pSave->Property(_T("Bs3IgnoreRangeOfApplicability"), m_Bs3IgnoreRangeOfApplicability);

   // moved into MomentCapacity data block (version 37)
   //pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
   //pSave->Property(_T("IncludeRebar_MomentCapacity"),m_bIncludeRebar_Moment); // added for version 7.0

   // added in version 37
   pSave->BeginUnit(_T("MomentCapacity"), 5.0);
   pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"), (Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
   pSave->Property(_T("IncludeStrandForNegMoment"), m_bIncludeStrand_NegMoment); // added in version 4 of *this data block
   pSave->Property(_T("IncludeRebarForCapacity"), m_bIncludeRebar_Moment);
   pSave->Property(_T("ConsiderReinforcementStrainLimit"), m_bConsiderReinforcementStrainLimit); // added in version 5 of *this data block
   pSave->Property(_T("MomentCapacitySliceCount"), m_nMomentCapacitySlices); // added in version 5 of *this data block
   pSave->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"), m_bIncludeForNegMoment); // added version 2 of *this data block
   pSave->BeginUnit(_T("ResistanceFactor"), 1.0);
   pSave->BeginUnit(_T("NormalWeight"), 2.0);
   pSave->Property(_T("TensionControlled_RC"), m_PhiFlexureTensionRC[pgsTypes::Normal]);
   pSave->Property(_T("TensionControlled_PS"), m_PhiFlexureTensionPS[pgsTypes::Normal]);
   pSave->Property(_T("TensionControlled_Spliced"), m_PhiFlexureTensionSpliced[pgsTypes::Normal]);
   pSave->Property(_T("CompressionControlled"), m_PhiFlexureCompression[pgsTypes::Normal]);
   pSave->EndUnit(); // NormalWeight
   pSave->BeginUnit(_T("AllLightweight"), 2.0);
   pSave->Property(_T("TensionControlled_RC"), m_PhiFlexureTensionRC[pgsTypes::AllLightweight]);
   pSave->Property(_T("TensionControlled_PS"), m_PhiFlexureTensionPS[pgsTypes::AllLightweight]);
   pSave->Property(_T("TensionControlled_Spliced"), m_PhiFlexureTensionSpliced[pgsTypes::AllLightweight]);
   pSave->Property(_T("CompressionControlled"), m_PhiFlexureCompression[pgsTypes::AllLightweight]);
   pSave->EndUnit(); // AllLightweight
   pSave->BeginUnit(_T("SandLightweight"), 2.0);
   pSave->Property(_T("TensionControlled_RC"), m_PhiFlexureTensionRC[pgsTypes::SandLightweight]);
   pSave->Property(_T("TensionControlled_PS"), m_PhiFlexureTensionPS[pgsTypes::SandLightweight]);
   pSave->Property(_T("TensionControlled_Spliced"), m_PhiFlexureTensionSpliced[pgsTypes::SandLightweight]);
   pSave->Property(_T("CompressionControlled"), m_PhiFlexureCompression[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // SandLightweight
   pSave->EndUnit(); // ResistanceFactor

   // added ClosureJointResistanceFactor in version 3.0 of MomentCapacity data block
   pSave->BeginUnit(_T("ClosureJointResistanceFactor"), 1.0);
   pSave->BeginUnit(_T("NormalWeight"), 1.0);
   pSave->Property(_T("FullyBondedTendons"), m_PhiClosureJointFlexure[pgsTypes::Normal]);
   pSave->EndUnit(); // NormalWeight
   pSave->BeginUnit(_T("AllLightweight"), 1.0);
   pSave->Property(_T("FullyBondedTendons"), m_PhiClosureJointFlexure[pgsTypes::AllLightweight]);
   pSave->EndUnit(); // AllLightweight
   pSave->BeginUnit(_T("SandLightweight"), 1.0);
   pSave->Property(_T("FullyBondedTendons"), m_PhiClosureJointFlexure[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // SandLightweight
   pSave->EndUnit(); // ClosureJointResistanceFactor
   pSave->EndUnit(); // MomentCapacity

   // changed in version 37
   //pSave->Property(_T("ModulusOfRuptureCoefficient"),m_FlexureModulusOfRuptureCoefficient); // added for version 9.0
   //pSave->Property(_T("ShearModulusOfRuptureCoefficient"),m_ShearModulusOfRuptureCoefficient); // added for version 18
   pSave->BeginUnit(_T("ModulusOfRuptureCoefficient"), 1.0);
   pSave->BeginUnit(_T("Moment"), 3.0);
   pSave->Property(_T("Normal"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
   //pSave->Property(_T("UHPC"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2 // removed in version 3
   pSave->EndUnit(); // Moment
   pSave->BeginUnit(_T("Shear"), 3.0);
   pSave->Property(_T("Normal"), m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
   //pSave->Property(_T("UHPC"), m_ShearModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2 // removed in version 3
   pSave->EndUnit(); // Shear
   pSave->BeginUnit(_T("Lifting"), 3.0);
   pSave->Property(_T("Normal"), m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
   //pSave->Property(_T("UHPC"), m_LiftingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2, removed in version 3
   pSave->EndUnit(); // Lifting
   pSave->BeginUnit(_T("Shipping"), 3.0);
   pSave->Property(_T("Normal"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
   //pSave->Property(_T("UHPC"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2, removed in version 3
   pSave->EndUnit(); // Shipping
   pSave->EndUnit(); // ModulusOfRuptureCoefficient

   //pSave->Property(_T("UHPCFiberShearStrength"), m_UHPCFiberShearStrength); // added in version 75, removed in version 82

   pSave->Property(_T("BsLldfMethod"), (Int16)m_LldfMethod);  // added LLDF_TXDOT for version 21.0
   pSave->Property(_T("IgnoreSkewReductionForMoment"), m_bIgnoreSkewReductionForMoment); // added in version 74
   // added in version 29
   pSave->Property(_T("MaxAngularDeviationBetweenGirders"), m_MaxAngularDeviationBetweenGirders);
   pSave->Property(_T("MinGirderStiffnessRatio"), m_MinGirderStiffnessRatio);
   pSave->Property(_T("LLDFGirderSpacingLocation"), m_LLDFGirderSpacingLocation);
   pSave->Property(_T("UseRigidMethod"), m_bUseRigidMethod); // added in version 70

   pSave->Property(_T("IncludeDualTandem"), m_bIncludeDualTandem); // added in version 61

   // added in version 31
   pSave->Property(_T("LimitDistributionFactorsToLanesBeams"), m_LimitDistributionFactorsToLanesBeams);

   // added in version 81
   pSave->Property(_T("ExteriorLiveLoadDistributionGTAdjacentInteriorRule"), m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule);

   // moved into Shear block in version 37
   //pSave->Property(_T("LongReinfShearMethod"),(Int16)m_LongReinfShearMethod); // added for version 1.2
   //pSave->Property(_T("IncludeRebar_Shear"),m_bIncludeRebar_Shear); // added for version 7.0
   pSave->Property(_T("CreepMethod"), (Int16)m_CreepMethod);
   pSave->Property(_T("CreepFactor"), m_CreepFactor);
   pSave->Property(_T("CreepDuration1Min"), m_CreepDuration1Min);
   pSave->Property(_T("CreepDuration1Max"), m_CreepDuration1Max);
   pSave->Property(_T("CreepDuration2Min"), m_CreepDuration2Min);
   pSave->Property(_T("CreepDuration2Max"), m_CreepDuration2Max);
   pSave->Property(_T("XferTime"), m_XferTime);
   pSave->Property(_T("TotalCreepDuration"), m_TotalCreepDuration);
   pSave->Property(_T("CamberVariability"), m_CamberVariability);

   pSave->Property(_T("LossMethod"), (Int16)m_LossMethod);
   pSave->Property(_T("TimeDependentModel"), (Int16)m_TimeDependentModel); // added in version 50
   //pSave->Property(_T("FinalLosses"),m_FinalLosses); // removed version 50
   pSave->Property(_T("ShippingLosses"), m_ShippingLosses);
   //pSave->Property(_T("BeforeXferLosses"),m_BeforeXferLosses);// removed version 50
   //pSave->Property(_T("AfterXferLosses"),m_AfterXferLosses);// removed version 50
   pSave->Property(_T("ShippingTime"), m_ShippingTime);

   // added in version 22
   //pSave->Property(_T("LiftingLosses"),m_LiftingLosses);// removed version 50
   //pSave->Property(_T("BeforeTempStrandRemovalLosses"),m_BeforeTempStrandRemovalLosses);// removed version 50
   //pSave->Property(_T("AfterTempStrandRemovalLosses"),m_AfterTempStrandRemovalLosses);// removed version 50
   //pSave->Property(_T("AfterDeckPlacementLosses"),m_AfterDeckPlacementLosses);// removed version 50
   //pSave->Property(_T("AfterSIDLLosses"),m_AfterSIDLLosses); // added in version 38// removed version 50


   pSave->Property(_T("CuringMethodFactor"), m_CuringMethodTimeAdjustmentFactor);

   // Added in version 1.5
   pSave->Property(_T("CheckStrandStressAtJacking"), m_bCheckStrandStress[CSS_AT_JACKING]);
   pSave->Property(_T("Coeff_AtJacking_StressRel"), m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"), m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressBeforeTransfer"), m_bCheckStrandStress[CSS_BEFORE_TRANSFER]);
   pSave->Property(_T("Coeff_BeforeTransfer_StressRel"), m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_BeforeTransfer_LowRelax"), m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterTransfer"), m_bCheckStrandStress[CSS_AFTER_TRANSFER]);
   pSave->Property(_T("Coeff_AfterTransfer_StressRel"), m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterTransfer_LowRelax"), m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterAllLosses"), m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"), m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"), m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);

   // added in version 50
   pSave->Property(_T("CheckTendonStressAtJacking"), m_bCheckTendonStressAtJacking);
   pSave->Property(_T("CheckTendonStressPriorToSeating"), m_bCheckTendonStressPriorToSeating);
   pSave->Property(_T("Coeff_AtJacking_StressRel"), m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"), m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]);
   pSave->Property(_T("Coeff_PriorToSeating_StressRel"), m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_PriorToSeating_LowRelax"), m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"), m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"), m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"), m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"), m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"), m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"), m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);


   // added in version 23, removed version 50
   //pSave->Property(_T("AnchorSet"),m_Dset);
   //pSave->Property(_T("WobbleFriction"),m_WobbleFriction);
   //pSave->Property(_T("CoefficientOfFriction"),m_FrictionCoefficient);

   // added in version 40
   pSave->Property(_T("RelaxationLossMethod"), m_RelaxationLossMethod);
   pSave->Property(_T("SlabElasticGain"), m_SlabElasticGain);
   pSave->Property(_T("HaunchElasticGain"), m_SlabPadElasticGain);
   pSave->Property(_T("DiaphragmElasticGain"), m_DiaphragmElasticGain);
   pSave->Property(_T("UserDCElasticGainBS1"), m_UserDCElasticGainBS1);
   pSave->Property(_T("UserDWElasticGainBS1"), m_UserDWElasticGainBS1);
   pSave->Property(_T("UserDCElasticGainBS2"), m_UserDCElasticGainBS2);
   pSave->Property(_T("UserDWElasticGainBS2"), m_UserDWElasticGainBS2);
   pSave->Property(_T("RailingSystemElasticGain"), m_RailingSystemElasticGain);
   pSave->Property(_T("OverlayElasticGain"), m_OverlayElasticGain);
   pSave->Property(_T("SlabShrinkageElasticGain"), m_SlabShrinkageElasticGain);
   pSave->Property(_T("LiveLoadElasticGain"), m_LiveLoadElasticGain); // added in version 42

   // added in version 45
   if (m_LossMethod == LOSSES_TXDOT_REFINED_2013)
   {
      pSave->Property(_T("FcgpComputationMethod"), m_FcgpComputationMethod);
   }

   // Added in 1.7
   pSave->Property(_T("CheckLiveLoadDeflection"), m_bDoEvaluateDeflection);
   pSave->Property(_T("LiveLoadDeflectionLimit"), m_DeflectionLimit);

   //   // Added in 8.0, removed in version 28
   //   pSave->Property(_T("AnalysisType"),(long)m_AnalysisType);

      // Added in 10.0, updated in version 37
   pSave->BeginUnit(_T("Limits"), 1.0);
   pSave->BeginUnit(_T("Normal"), 2.0);
   pSave->Property(_T("MaxSlabFc"), m_MaxSlabFc[pgsTypes::Normal]);
   pSave->Property(_T("MaxGirderFci"), m_MaxSegmentFci[pgsTypes::Normal]);
   pSave->Property(_T("MaxGirderFc"), m_MaxSegmentFc[pgsTypes::Normal]);
   pSave->Property(_T("MaxClosureFci"), m_MaxClosureFci[pgsTypes::Normal]); // added in version 2.0
   pSave->Property(_T("MaxClosureFc"), m_MaxClosureFc[pgsTypes::Normal]); // added in version 2.0
   pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::Normal]);
   pSave->Property(_T("MaxConcreteAggSize"), m_MaxConcreteAggSize[pgsTypes::Normal]);
   pSave->EndUnit(); // Normal;
   pSave->BeginUnit(_T("AllLightweight"), 2.0);
   pSave->Property(_T("MaxSlabFc"), m_MaxSlabFc[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxGirderFci"), m_MaxSegmentFci[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxGirderFc"), m_MaxSegmentFc[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxClosureFci"), m_MaxClosureFci[pgsTypes::AllLightweight]); // added in version 2.0
   pSave->Property(_T("MaxClosureFc"), m_MaxClosureFc[pgsTypes::AllLightweight]); // added in version 2.0
   pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxConcreteAggSize"), m_MaxConcreteAggSize[pgsTypes::AllLightweight]);
   pSave->EndUnit(); // AllLightweight;
   pSave->BeginUnit(_T("SandLightweight"), 2.0);
   pSave->Property(_T("MaxSlabFc"), m_MaxSlabFc[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxGirderFci"), m_MaxSegmentFci[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxGirderFc"), m_MaxSegmentFc[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxClosureFci"), m_MaxClosureFci[pgsTypes::SandLightweight]); // added in version 2.0
   pSave->Property(_T("MaxClosureFc"), m_MaxClosureFc[pgsTypes::SandLightweight]); // added in version 2.0
   pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxConcreteAggSize"), m_MaxConcreteAggSize[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // SandLightweight;
   pSave->EndUnit(); // Limits

   // Added in version 44
   pSave->BeginUnit(_T("Warnings"), 2.0);
   pSave->Property(_T("DoCheckStirrupSpacingCompatibility"), m_DoCheckStirrupSpacingCompatibility);
   pSave->Property(_T("CheckGirderSag"), m_bCheckSag); // added in version 2
   pSave->Property(_T("SagCamberType"), m_SagCamberType); // added in version 2
   pSave->EndUnit(); // Warnings

   // Added in 14.0 removed in version 41
   //std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};

   //pSave->BeginUnit(_T("LoadFactors"),3.0);
   //int nLimitStates = sizeof(strLimitState)/sizeof(std::_tstring); // Added StrengthII in version 26
   //for ( int i = 0; i < nLimitStates; i++ )
   //{
   //   pSave->BeginUnit(strLimitState[i].c_str(),1.0);
   //   
   //   pSave->Property(_T("DCmin"),  m_DCmin[i]);
   //   pSave->Property(_T("DCmax"),  m_DCmax[i]);
   //   pSave->Property(_T("DWmin"),  m_DWmin[i]);
   //   pSave->Property(_T("DWmax"),  m_DWmax[i]);
   //   pSave->Property(_T("LLIMmin"),m_LLIMmin[i]);
   //   pSave->Property(_T("LLIMmax"),m_LLIMmax[i]);

   //   pSave->EndUnit();
   //}
   //pSave->EndUnit();

   // added in version 15
   pSave->Property(_T("EnableSlabOffsetCheck"), m_EnableSlabOffsetCheck);
   pSave->Property(_T("EnableSlabOffsetDesign"), m_EnableSlabOffsetDesign);

   pSave->Property(_T("DesignStrandFillType"), (long)m_DesignStrandFillType);

   // added in version 16
   pSave->Property(_T("EffectiveFlangeWidthMethod"), (long)m_EffFlangeWidthMethod);

   //   // added in version 17, removed version 24
   //   pSave->Property(_T("SlabOffsetMethod"),(long)m_SlabOffsetMethod);

      // reconfigured in version 37 and added Phi
   pSave->BeginUnit(_T("Shear"), 4.0);
   // moved here in version 37
   pSave->Property(_T("LongReinfShearMethod"), (Int16)m_LongReinfShearMethod); // added for version 1.2

   // moved here in version 37
   pSave->Property(_T("IncludeRebarForCapacity"), m_bIncludeRebar_Shear); // added for version 7.0

   // added in version 18 (moved into datablock in version 37)
   pSave->Property(_T("ShearFlowMethod"), (long)m_ShearFlowMethod);
   pSave->Property(_T("MaxInterfaceShearConnectorSpacing"), m_MaxInterfaceShearConnectorSpacing);
   pSave->Property(_T("UseDeckWeightForPc"), m_bUseDeckWeightForPc); // added in version 3 of Shear data block
   pSave->Property(_T("ShearCapacityMethod"), (long)m_ShearCapacityMethod);
   pSave->Property(_T("LimitNetTensionStrainToPositiveValues"), m_bLimitNetTensionStrainToPositiveValues); // added in version 4 of *this data block

   // added in version 37
   pSave->BeginUnit(_T("ResistanceFactor"), 1.0);
   pSave->Property(_T("Normal"), m_PhiShear[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_PhiShear[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_PhiShear[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // ResistanceFactor

   // added in version 64
   pSave->BeginUnit(_T("ResistanceFactorDebonded"), 2.0);
   pSave->Property(_T("Normal"), m_PhiShearDebonded[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_PhiShearDebonded[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_PhiShearDebonded[pgsTypes::SandLightweight]);
   pSave->Property(_T("PCI_UHPC"), m_PhiShearDebonded[pgsTypes::PCI_UHPC]); // added v2
   pSave->Property(_T("UHPC"), m_PhiShearDebonded[pgsTypes::UHPC]); // added v2
   pSave->EndUnit(); // ResistanceFactorDebonded

   // Added ClosureJointResistanceFactor in version 2 of Shear data block
   pSave->BeginUnit(_T("ClosureJointResistanceFactor"), 1.0);
   pSave->Property(_T("Normal"), m_PhiClosureJointShear[pgsTypes::Normal]);
   pSave->Property(_T("AllLightweight"), m_PhiClosureJointShear[pgsTypes::AllLightweight]);
   pSave->Property(_T("SandLightweight"), m_PhiClosureJointShear[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // ResistanceFactor
   pSave->EndUnit(); // Shear

   // added in version 26
   pSave->Property(_T("PedestrianLoad"), m_PedestrianLoad);
   pSave->Property(_T("MinSidewalkWidth"), m_MinSidewalkWidth);

   // added in vers 32
   pSave->Property(_T("PrestressTransferComputationType"), (long)m_PrestressTransferComputationType);

   pSave->BeginUnit(_T("StrandExtensions"), 1.0);
   pSave->Property(_T("AllowStraightStrandExtensions"), m_bAllowStraightStrandExtensions);
   pSave->EndUnit(); // StrandExtensions

   // added in version 52
   pSave->BeginUnit(_T("DuctSize"), 1.0);
   pSave->Property(_T("DuctAreaPushRatio"), m_DuctAreaPushRatio);
   pSave->Property(_T("DuctAreaPullRatio"), m_DuctAreaPullRatio);
   pSave->Property(_T("DuctDiameterRatio"), m_DuctDiameterRatio);
   pSave->EndUnit(); // DuctSize

   // added version 50
   pSave->BeginUnit(_T("ClosureJoint"), 1.0);
   pSave->Property(_T("ClosureCompStressAtStressing"), m_ClosureCompStressAtStressing);
   pSave->Property(_T("ClosureTensStressPTZAtStressing"), m_ClosureTensStressPTZAtStressing);
   pSave->Property(_T("ClosureTensStressPTZWithRebarAtStressing"), m_ClosureTensStressPTZWithRebarAtStressing);
   pSave->Property(_T("ClosureTensStressAtStressing"), m_ClosureTensStressAtStressing);
   pSave->Property(_T("ClosureTensStressWithRebarAtStressing"), m_ClosureTensStressWithRebarAtStressing);
   pSave->Property(_T("ClosureCompStressAtService"), m_ClosureCompStressAtService);
   pSave->Property(_T("ClosureCompStressWithLiveLoadAtService"), m_ClosureCompStressWithLiveLoadAtService);
   pSave->Property(_T("ClosureTensStressPTZAtService"), m_ClosureTensStressPTZAtService);
   pSave->Property(_T("ClosureTensStressPTZWithRebarAtService"), m_ClosureTensStressPTZWithRebarAtService);
   pSave->Property(_T("ClosureTensStressAtService"), m_ClosureTensStressAtService);
   pSave->Property(_T("ClosureTensStressWithRebarAtService"), m_ClosureTensStressWithRebarAtService);
   pSave->Property(_T("ClosureCompStressFatigue"), m_ClosureCompStressFatigue);
   pSave->EndUnit(); // Closure Joint

   // added in version 48
   pSave->Property(_T("CheckBottomFlangeClearance"), m_bCheckBottomFlangeClearance);
   pSave->Property(_T("MinBottomFlangeClearance"), m_Cmin);

   // added in version 57
   pSave->Property(_T("CheckGirderInclination"), m_bCheckGirderInclination);
   //pSave->Property(_T("InclindedGirder_BrgPadDeduction"), m_InclinedGirder_BrgPadDeduction); // removed in version 71
   pSave->Property(_T("InclindedGirder_FSmax"), m_InclinedGirder_FSmax);

   // added in version 62
   pSave->Property(_T("FinishedElevationTolerance"), m_FinishedElevationTolerance);

   // added in version 73
   pSave->Property(_T("SlabOffsetRoundingMethod"), (long)m_SlabOffsetRoundingMethod);
   pSave->Property(_T("SlabOffsetRoundingTolerance"), m_SlabOffsetRoundingTolerance);

   // added in version 79
   pSave->BeginUnit(_T("Bearings"), 1.0);
   pSave->Property(_T("AlertTaperedSolePlateRequirement"), m_bAlertTaperedSolePlateRequirement);
   pSave->Property(_T("TaperedSolePlaneInclinationThreshold"), m_TaperedSolePlateInclinationThreshold);
   pSave->Property(_T("UseImpactForBearingReactions"), m_bUseImpactForBearingReactions);
   pSave->EndUnit(); // Bearings

   pSave->EndUnit();

   return true;
}
*/