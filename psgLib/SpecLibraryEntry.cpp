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

#include "StdAfx.h"
#include "resource.h"
#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#include "SpecLibraryEntryImpl.h"
#include "SpecMainSheet.h"

#include <EAF\EAFApp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   SpecLibraryEntry
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
SpecLibraryEntry::SpecLibraryEntry()
{
   m_pImpl = std::make_unique<SpecLibraryEntryImpl>();
}

SpecLibraryEntry::SpecLibraryEntry(const SpecLibraryEntry& rOther)
{
   *this = rOther;
}

SpecLibraryEntry::~SpecLibraryEntry() = default;

SpecLibraryEntry& SpecLibraryEntry::operator=(const SpecLibraryEntry& rOther)
{
   WBFL::Library::LibraryEntry::operator=(rOther);
   ISupportIcon::operator=(rOther);
   WBFL::System::SubjectT<SpecLibraryEntryObserver, SpecLibraryEntry>::operator=(rOther);
   m_pImpl = std::make_unique<SpecLibraryEntryImpl>(*(rOther.m_pImpl));
   return *this;
}


bool SpecLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   SpecLibraryEntry tmp(*this);
   if ( 0 < GetRefCount() )
   {
      tmp.AddRef();
   }

   CSpecMainSheet dlg(tmp, IDS_SPEC_SHEET, allowEditing);
   dlg.SetActivePage(nPage);
   INT_PTR i = dlg.DoModal();

   if ( 0 < GetRefCount() )
   {
      tmp.Release();
   }

   if (i==IDOK)
   {
      *this = tmp;
      return true;
   }

   return false;
}

HICON  SpecLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPECIFICATION_ENTRY) );
}

bool SpecLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   return m_pImpl->SaveMe(this,pSave);
}

bool SpecLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   return m_pImpl->LoadMe(this,pLoad);
}

bool SpecLibraryEntry::IsEqual(const SpecLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool SpecLibraryEntry::Compare(const SpecLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   bMustRename = false;
   if (considerName && GetName() != rOther.GetName())
   {
      if (bReturnOnFirstDifference) { ATLASSERT(vDifferences.size() == 0); return false; }
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Name"), GetName().c_str(), rOther.GetName().c_str()));
   }

   return m_pImpl->Compare(rOther.m_pImpl.get(), vDifferences, bReturnOnFirstDifference);
}

const SpecificationCriteria& SpecLibraryEntry::GetSpecificationCriteria() const
{
   return m_pImpl->GetSpecificationCriteria();
}

void SpecLibraryEntry::SetSpecificationCriteria(const SpecificationCriteria& criteria)
{
   m_pImpl->SetSpecificationCriteria(criteria);
}

const SectionPropertiesCriteria& SpecLibraryEntry::GetSectionPropertiesCriteria() const
{
   return m_pImpl->GetSectionPropertiesCriteria();
}

void SpecLibraryEntry::SetSectionPropertiesCriteria(const SectionPropertiesCriteria& criteria)
{
   m_pImpl->SetSectionPropertiesCriteria(criteria);
}

const StrandSlopeCriteria& SpecLibraryEntry::GetStrandSlopeCriteria() const
{
   return m_pImpl->GetStrandSlopeCriteria();
}

void SpecLibraryEntry::SetStrandSlopeCriteria(const StrandSlopeCriteria& criteria)
{
   m_pImpl->SetStrandSlopeCriteria(criteria);
}

const HoldDownCriteria& SpecLibraryEntry::GetHoldDownCriteria() const
{
   return m_pImpl->GetHoldDownCriteria();
}

void SpecLibraryEntry::SetHoldDownCriteria(const HoldDownCriteria& criteria)
{
   m_pImpl->SetHoldDownCriteria(criteria);
}

const PlantHandlingCriteria& SpecLibraryEntry::GetPlantHandlingCriteria() const
{
   return m_pImpl->GetPlantHandlingCriteria();
}

void SpecLibraryEntry::SetPlantHandlingCriteria(const PlantHandlingCriteria& criteria)
{
   m_pImpl->SetPlantHandlingCriteria(criteria);
}

const EndZoneCriteria& SpecLibraryEntry::GetEndZoneCriteria() const
{
   return m_pImpl->GetEndZoneCriteria();
}

void SpecLibraryEntry::SetEndZoneCriteria(const EndZoneCriteria& criteria)
{
   m_pImpl->SetEndZoneCriteria(criteria);
}

const SlabOffsetCriteria& SpecLibraryEntry::GetSlabOffsetCriteria() const
{
   return m_pImpl->GetSlabOffsetCriteria();
}

void SpecLibraryEntry::SetSlabOffsetCriteria(const SlabOffsetCriteria& criteria)
{
   m_pImpl->SetSlabOffsetCriteria(criteria);
}

const ShearCapacityCriteria& SpecLibraryEntry::GetShearCapacityCriteria() const
{
   return m_pImpl->GetShearCapacityCriteria();
}

void SpecLibraryEntry::SetShearCapacityCriteria(const ShearCapacityCriteria& criteria)
{
   m_pImpl->SetShearCapacityCriteria(criteria);
}

const InterfaceShearCriteria& SpecLibraryEntry::GetInterfaceShearCriteria() const
{
   return m_pImpl->GetInterfaceShearCriteria();
}

void SpecLibraryEntry::SetInterfaceShearCriteria(const InterfaceShearCriteria& criteria)
{
   m_pImpl->SetInterfaceShearCriteria(criteria);
}

const LiftingCriteria& SpecLibraryEntry::GetLiftingCriteria() const
{
   return m_pImpl->GetLiftingCriteria();
}

void SpecLibraryEntry::SetLiftingCriteria(const LiftingCriteria& criteria)
{
   m_pImpl->SetLiftingCriteria(criteria);
}

const HaulingCriteria& SpecLibraryEntry::GetHaulingCriteria() const
{
   return m_pImpl->GetHaulingCriteria();
}

void SpecLibraryEntry::SetHaulingCriteria(const HaulingCriteria& criteria)
{
   m_pImpl->SetHaulingCriteria(criteria);
}

const PrincipalTensionStressCriteria& SpecLibraryEntry::GetPrincipalTensionStressCriteria() const
{
   return m_pImpl->GetPrincipalTensionStressCriteria();
}

void SpecLibraryEntry::SetPrincipalTensionStressCriteria(const PrincipalTensionStressCriteria& criteria)
{
   m_pImpl->SetPrincipalTensionStressCriteria(criteria);
}

const MomentCapacityCriteria& SpecLibraryEntry::GetMomentCapacityCriteria() const
{
   return m_pImpl->GetMomentCapacityCriteria();
}

void SpecLibraryEntry::SetMomentCapacityCriteria(const MomentCapacityCriteria& criteria)
{
   m_pImpl->SetMomentCapacityCriteria(criteria);
}

const StrandStressCriteria& SpecLibraryEntry::GetStrandStressCriteria() const
{
   return m_pImpl->GetStrandStressCriteria();
}

void SpecLibraryEntry::SetStrandStressCriteria(const StrandStressCriteria& criteria)
{
   m_pImpl->SetStrandStressCriteria(criteria);
}

const TendonStressCriteria& SpecLibraryEntry::GetTendonStressCriteria() const
{
   return m_pImpl->GetTendonStressCriteria();
}

void SpecLibraryEntry::SetTendonStressCriteria(const TendonStressCriteria& criteria)
{
   m_pImpl->SetTendonStressCriteria(criteria);
}

const CreepCriteria& SpecLibraryEntry::GetCreepCriteria() const
{
   return m_pImpl->GetCreepCriteria();
}

void SpecLibraryEntry::SetCreepCriteria(const CreepCriteria& criteria)
{
   m_pImpl->SetCreepCriteria(criteria);
}
const LimitsCriteria& SpecLibraryEntry::GetLimitsCriteria() const
{
   return m_pImpl->GetLimitsCriteria();
}

void SpecLibraryEntry::SetLimitsCriteria(const LimitsCriteria& criteria)
{
   m_pImpl->SetLimitsCriteria(criteria);
}

const PrestressLossCriteria& SpecLibraryEntry::GetPrestressLossCriteria() const
{
   return m_pImpl->GetPrestressLossCriteria();;
}

void SpecLibraryEntry::SetPrestressLossCriteria(const PrestressLossCriteria& criteria)
{
   m_pImpl->SetPrestressLossCriteria(criteria);
}

const LiveLoadDistributionCriteria& SpecLibraryEntry::GetLiveLoadDistributionCriteria() const
{
   return m_pImpl->GetLiveLoadDistributionCriteria();
}

void SpecLibraryEntry::SetLiveLoadDistributionCriteria(const LiveLoadDistributionCriteria& criteria)
{
   m_pImpl->SetLiveLoadDistributionCriteria(criteria);
}

const DeadLoadDistributionCriteria& SpecLibraryEntry::GetDeadLoadDistributionCriteria() const
{
   return m_pImpl->GetDeadLoadDistributionCriteria();;
}

void SpecLibraryEntry::SetDeadLoadDistributionCriteria(const DeadLoadDistributionCriteria& criteria)
{
   m_pImpl->SetDeadLoadDistributionCriteria(criteria);
}

const HaunchCriteria& SpecLibraryEntry::GetHaunchCriteria() const
{
   return m_pImpl->GetHaunchCriteria();;
}

void SpecLibraryEntry::SetHaunchCriteria(const HaunchCriteria& criteria)
{
   m_pImpl->SetHaunchCriteria(criteria);
}

const LiveLoadDeflectionCriteria& SpecLibraryEntry::GetLiveLoadDeflectionCriteria() const
{
   return m_pImpl->GetLiveLoadDeflectionCriteria();
}

void SpecLibraryEntry::SetLiveLoadDeflectionCriteria(const LiveLoadDeflectionCriteria& criteria)
{
   m_pImpl->SetLiveLoadDeflectionCriteria(criteria);
}

const LiveLoadCriteria& SpecLibraryEntry::GetLiveLoadCriteria() const
{
   return m_pImpl->GetLiveLoadCriteria();
} 

void SpecLibraryEntry::SetLiveLoadCriteria(const LiveLoadCriteria& criteria)
{
   m_pImpl->SetLiveLoadCriteria(criteria);
}

const TransferLengthCriteria& SpecLibraryEntry::GetTransferLengthCriteria() const
{
   return m_pImpl->GetTransferLengthCriteria();
}

void SpecLibraryEntry::SetTransferLengthCriteria(const TransferLengthCriteria& criteria)
{
   m_pImpl->SetTransferLengthCriteria(criteria);
}

const DuctSizeCriteria& SpecLibraryEntry::GetDuctSizeCriteria() const
{
   return m_pImpl->GetDuctSizeCriteria();
}

void SpecLibraryEntry::SetDuctSizeCriteria(const DuctSizeCriteria& criteria)
{
   m_pImpl->SetDuctSizeCriteria(criteria);
}

const ClosureJointCriteria& SpecLibraryEntry::GetClosureJointCriteria() const
{
   return m_pImpl->GetClosureJointCriteria();
}

void SpecLibraryEntry::SetClosureJointCriteria(const ClosureJointCriteria& criteria)
{
   m_pImpl->SetClosureJointCriteria(criteria);
}

const BottomFlangeClearanceCriteria& SpecLibraryEntry::GetBottomFlangeClearanceCriteria() const
{
   return m_pImpl->GetBottomFlangeClearanceCriteria();
}

void SpecLibraryEntry::SetBottomFlangeClearanceCriteria(const BottomFlangeClearanceCriteria& criteria)
{
   m_pImpl->SetBottomFlangeClearanceCriteria(criteria);
}

const GirderInclinationCriteria& SpecLibraryEntry::GetGirderInclinationCriteria() const
{
   return m_pImpl->GetGirderInclinationCriteria();
}

void SpecLibraryEntry::SetGirderInclinationCriteria(const GirderInclinationCriteria& criteria)
{
   m_pImpl->SetGirderInclinationCriteria(criteria);
}

const HarpedStrandDesignCriteria& SpecLibraryEntry::GetHarpedStrandDesignCriteria() const
{
   return m_pImpl->GetHarpedStrandDesignCriteria();
}

void SpecLibraryEntry::SetHarpedStrandDesignCriteria(const HarpedStrandDesignCriteria& criteria)
{
   m_pImpl->SetHarpedStrandDesignCriteria(criteria);
}

const LimitStateConcreteStrengthCriteria& SpecLibraryEntry::GetLimitStateConcreteStrengthCriteria() const
{
   return m_pImpl->GetLimitStateConcreteStrengthCriteria();
}

void SpecLibraryEntry::SetLimitStateConcreteStrengthCriteria(const LimitStateConcreteStrengthCriteria& criteria)
{
   m_pImpl->SetLimitStateConcreteStrengthCriteria(criteria);
}

const PrestressedElementCriteria& SpecLibraryEntry::GetPrestressedElementCriteria() const
{
   return m_pImpl->GetPrestressedElementCriteria();
}

void SpecLibraryEntry::SetPrestressedElementCriteria(const PrestressedElementCriteria& criteria)
{
   m_pImpl->SetPrestressedElementCriteria(criteria);
}

const BearingCriteria& SpecLibraryEntry::GetBearingCriteria() const
{
   return m_pImpl->GetBearingCriteria();
}

void SpecLibraryEntry::SetBearingCriteria(const BearingCriteria& criteria)
{
   m_pImpl->SetBearingCriteria(criteria);
}

void SpecLibraryEntry::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << Bold(_T("Name: ")) << GetName() << rptNewLine;

   m_pImpl->Report(pChapter, pDisplayUnits);
}

const RefactoredSpecLibraryParameters& SpecLibraryEntry::GetRefactoredParameters() const
{
   return m_pImpl->GetRefactoredParameters();
}
