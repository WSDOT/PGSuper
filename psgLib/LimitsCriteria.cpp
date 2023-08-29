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
#include <psgLib/LimitsCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

LimitsCriteria::LimitsCriteria()
{
   bCheckStirrupSpacingCompatibility = true;
   bCheckSag = true;
   SagCamber = pgsTypes::SagCamber::LowerBoundCamber;

   MaxSlabFc[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxSegmentFci[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(7.5, WBFL::Units::Measure::KSI);
   MaxSegmentFc[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI);
   MaxClosureFci[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxClosureFc[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   MaxConcreteUnitWeight[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::LbfPerFeet3);
   MaxConcreteAggSize[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   MaxSlabFc[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxSegmentFci[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(7.5, WBFL::Units::Measure::KSI);
   MaxSegmentFc[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(9.0, WBFL::Units::Measure::KSI);
   MaxClosureFci[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxClosureFc[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   MaxConcreteUnitWeight[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(125., WBFL::Units::Measure::LbfPerFeet3);
   MaxConcreteAggSize[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   MaxSlabFc[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxSegmentFci[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(7.5, WBFL::Units::Measure::KSI);
   MaxSegmentFc[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(9.0, WBFL::Units::Measure::KSI);
   MaxClosureFci[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   MaxClosureFc[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   MaxConcreteUnitWeight[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(125., WBFL::Units::Measure::LbfPerFeet3);
   MaxConcreteAggSize[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   // Not using these limits for UHPCs yet
   //MaxSlabFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //MaxSegmentFci[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI);
   //MaxSegmentFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(20.0, WBFL::Units::Measure::KSI);
   //MaxClosureFci[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //MaxClosureFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   //MaxConcreteUnitWeight[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::LbfPerFeet3);
   //MaxConcreteAggSize[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   //MaxSlabFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //MaxSegmentFci[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(14.0, WBFL::Units::Measure::KSI);
   //MaxSegmentFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(20.0, WBFL::Units::Measure::KSI);
   //MaxClosureFci[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //MaxClosureFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   //MaxConcreteUnitWeight[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::LbfPerFeet3);
   //MaxConcreteAggSize[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);
}

bool LimitsCriteria::Compare(const LimitsCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (bCheckStirrupSpacingCompatibility != other.bCheckStirrupSpacingCompatibility ||
      (bCheckSag != other.bCheckSag || (bCheckSag == true && SagCamber != other.SagCamber)))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("General Warnings are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   bool bConcreteLimits = true;
   auto count = MaxSlabFc.size();
   for (int i = 0; i < count && bConcreteLimits == true; i++)
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if (concreteType == pgsTypes::AllLightweight && WBFL::LRFD::LRFDVersionMgr::Version::SeventhEditionWith2016Interims <= impl.GetSpecificationCriteria().GetEdition())
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and those parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if (!::IsEqual(MaxSlabFc[concreteType], other.MaxSlabFc[concreteType]) ||
         !::IsEqual(MaxSegmentFci[concreteType], other.MaxSegmentFci[concreteType]) ||
         !::IsEqual(MaxSegmentFc[concreteType], other.MaxSegmentFc[concreteType]) ||
         !::IsEqual(MaxClosureFci[concreteType], other.MaxClosureFci[concreteType]) ||
         !::IsEqual(MaxClosureFc[concreteType], other.MaxClosureFc[concreteType]) ||
         !::IsEqual(MaxConcreteUnitWeight[concreteType], other.MaxConcreteUnitWeight[concreteType]) ||
         !::IsEqual(MaxConcreteAggSize[concreteType], other.MaxConcreteAggSize[concreteType]))
      {
         bConcreteLimits = false;
      }
   }

   if (!bConcreteLimits)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Concrete Limits are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void LimitsCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
}

void LimitsCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LimitsCriteria"), 1.0);

   pSave->BeginUnit(_T("Limits"), 1.0);

   pSave->BeginUnit(_T("Normal"), 1.0);
   pSave->Property(_T("MaxSlabFc"), MaxSlabFc[pgsTypes::Normal]);
   pSave->Property(_T("MaxGirderFci"), MaxSegmentFci[pgsTypes::Normal]);
   pSave->Property(_T("MaxGirderFc"), MaxSegmentFc[pgsTypes::Normal]);
   pSave->Property(_T("MaxClosureFci"), MaxClosureFci[pgsTypes::Normal]);
   pSave->Property(_T("MaxClosureFc"), MaxClosureFc[pgsTypes::Normal]);
   pSave->Property(_T("MaxConcreteUnitWeight"), MaxConcreteUnitWeight[pgsTypes::Normal]);
   pSave->Property(_T("MaxConcreteAggSize"), MaxConcreteAggSize[pgsTypes::Normal]);
   pSave->EndUnit(); // Normal;

   pSave->BeginUnit(_T("AllLightweight"), 1.0);
   pSave->Property(_T("MaxSlabFc"), MaxSlabFc[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxGirderFci"), MaxSegmentFci[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxGirderFc"), MaxSegmentFc[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxClosureFci"), MaxClosureFci[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxClosureFc"), MaxClosureFc[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxConcreteUnitWeight"), MaxConcreteUnitWeight[pgsTypes::AllLightweight]);
   pSave->Property(_T("MaxConcreteAggSize"), MaxConcreteAggSize[pgsTypes::AllLightweight]);
   pSave->EndUnit(); // AllLightweight;

   pSave->BeginUnit(_T("SandLightweight"), 1.0);
   pSave->Property(_T("MaxSlabFc"), MaxSlabFc[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxGirderFci"), MaxSegmentFci[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxGirderFc"), MaxSegmentFc[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxClosureFci"), MaxClosureFci[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxClosureFc"), MaxClosureFc[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxConcreteUnitWeight"), MaxConcreteUnitWeight[pgsTypes::SandLightweight]);
   pSave->Property(_T("MaxConcreteAggSize"), MaxConcreteAggSize[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // SandLightweight;

   pSave->EndUnit(); // Limits

   pSave->BeginUnit(_T("Warnings"), 1.0);
   pSave->Property(_T("bCheckStirrupSpacingCompatibility"), bCheckStirrupSpacingCompatibility);
   pSave->Property(_T("bCheckSag"), bCheckSag);
   pSave->Property(_T("SagCamber"), (Int16)SagCamber);
   pSave->EndUnit(); // Warnings

   pSave->EndUnit();
}

void LimitsCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LimitsCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->BeginUnit(_T("Limits"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->BeginUnit(_T("Normal"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxSlabFc"), &MaxSlabFc[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFci"), &MaxSegmentFci[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFc"), &MaxSegmentFc[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFci"), &MaxClosureFci[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFc"), &MaxClosureFc[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &MaxConcreteUnitWeight[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteAggSize"), &MaxConcreteAggSize[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Normal

   if (!pLoad->BeginUnit(_T("AllLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxSlabFc"), &MaxSlabFc[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFci"), &MaxSegmentFci[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFc"), &MaxSegmentFc[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFci"), &MaxClosureFci[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFc"), &MaxClosureFc[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &MaxConcreteUnitWeight[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteAggSize"), &MaxConcreteAggSize[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // AllLightweight

   if (!pLoad->BeginUnit(_T("SandLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxSlabFc"), &MaxSlabFc[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFci"), &MaxSegmentFci[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderFc"), &MaxSegmentFc[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFci"), &MaxClosureFci[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxClosureFc"), &MaxClosureFc[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteUnitWeight"), &MaxConcreteUnitWeight[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxConcreteAggSize"), &MaxConcreteAggSize[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);  // SandLightweight

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Limits

   if (!pLoad->BeginUnit(_T("Warnings"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bCheckStirrupSpacingCompatibility"), &bCheckStirrupSpacingCompatibility)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bCheckSag"), &bCheckSag)) THROW_LOAD(InvalidFileFormat, pLoad);
   Int16 value;
   if (!pLoad->Property(_T("SagCamber"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   SagCamber = (pgsTypes::SagCamber)value;
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Warnings

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
