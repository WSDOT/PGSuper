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
#include <psgLib/MomentCapacityCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

MomentCapacityCriteria::MomentCapacityCriteria()
{
   bIncludeStrandForNegMoment = false;
   bConsiderReinforcementStrainLimit = false;
   nMomentCapacitySlices = 10;
   bIncludeRebar = false;
   bIncludeNoncompositeMomentsForNegMomentDesign = true;

   OverReinforcedMomentCapacity = 0;

   ModulusOfRuptureCoefficient[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(0.37, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.20, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(0.17, WBFL::Units::Measure::SqrtKSI);

   PhiTensionPS[pgsTypes::Normal] = 1.00;
   PhiTensionRC[pgsTypes::Normal] = 0.90;
   PhiTensionSpliced[pgsTypes::Normal] = 1.00;
   PhiCompression[pgsTypes::Normal] = 0.75;

   PhiTensionPS[pgsTypes::SandLightweight] = 1.00;
   PhiTensionRC[pgsTypes::SandLightweight] = 0.90;
   PhiTensionSpliced[pgsTypes::SandLightweight] = 1.00;
   PhiCompression[pgsTypes::SandLightweight] = 0.75;

   PhiTensionPS[pgsTypes::AllLightweight] = 1.00;
   PhiTensionRC[pgsTypes::AllLightweight] = 0.90;
   PhiTensionSpliced[pgsTypes::AllLightweight] = 1.00;
   PhiCompression[pgsTypes::AllLightweight] = 0.75;

   PhiTensionPS[pgsTypes::PCI_UHPC] = 1.00;
   PhiTensionRC[pgsTypes::PCI_UHPC] = 0.90;
   PhiTensionSpliced[pgsTypes::PCI_UHPC] = 1.00;
   PhiCompression[pgsTypes::PCI_UHPC] = 0.75;

   // These don't make sense since UHPC uses a variable phi based on ductility ratio.
   // The variable phi does have min/max values so this might be able to be used for that in the future
   // At this time, these are just placeholder values. There aren't any UI elements to modify them
   // and they aren't used in the main program.
   PhiTensionPS[pgsTypes::UHPC] = 0.90;
   PhiTensionRC[pgsTypes::UHPC] = 0.90;
   PhiTensionSpliced[pgsTypes::UHPC] = 1.00;
   PhiCompression[pgsTypes::UHPC] = 0.75;

   PhiClosureJoint[pgsTypes::Normal] = 0.95;
   PhiClosureJoint[pgsTypes::SandLightweight] = 0.90;
   PhiClosureJoint[pgsTypes::AllLightweight] = 0.90;
   PhiClosureJoint[pgsTypes::PCI_UHPC] = 0.95;
   PhiClosureJoint[pgsTypes::UHPC] = 0.95; // UHPC doesn't have a fixed phi for flexure
}

bool MomentCapacityCriteria::Compare(const MomentCapacityCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if ((impl.GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims &&  OverReinforcedMomentCapacity != other.OverReinforcedMomentCapacity) ||
      bIncludeStrandForNegMoment != other.bIncludeStrandForNegMoment ||
      bConsiderReinforcementStrainLimit != other.bConsiderReinforcementStrainLimit ||
      nMomentCapacitySlices != other.nMomentCapacitySlices ||
      bIncludeRebar != other.bIncludeRebar ||
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::Normal], other.ModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
      (impl.GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims && !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight])))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Moment Capacity parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   bool bPhiFactors = true;
   for (int i = 0; i < pgsTypes::ConcreteTypeCount && bPhiFactors == true; i++)
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if (concreteType == pgsTypes::AllLightweight && WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= impl.GetSpecificationCriteria().GetEdition() )
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and those parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if (!::IsEqual(PhiTensionPS[concreteType], other.PhiTensionPS[concreteType]) ||
          !::IsEqual(PhiTensionRC[concreteType], other.PhiTensionRC[concreteType]) ||
          !::IsEqual(PhiTensionSpliced[concreteType], other.PhiTensionSpliced[concreteType]) ||
          !::IsEqual(PhiCompression[concreteType], other.PhiCompression[concreteType]) ||
          !::IsEqual(PhiClosureJoint[concreteType], other.PhiClosureJoint[concreteType]))
      {
         bPhiFactors = false;
      }
   }
   if (!bPhiFactors)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Moment Resistance Factors are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (bIncludeNoncompositeMomentsForNegMomentDesign != other.bIncludeNoncompositeMomentsForNegMomentDesign)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Negative Moment Capacity parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void MomentCapacityCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Moment Capacity Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims)
   {
      if (OverReinforcedMomentCapacity == 0)
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with LRFD C5.7.3.3.1") << rptNewLine;
      }
      else
      {
         *pPara << _T("Capacity of over reinforced sections computed in accordance with WSDOT Bridge Design Manual") << rptNewLine;
      }
   }

   *pPara << _T("Girder longitudinal mild reinforcement is");
   if (!bIncludeRebar) *pPara << _T(" not");
   *pPara << _T(" included in the capacity analysis") << rptNewLine;
   
   *pPara << _T("Reinforcement strain limits are");
   if (!bConsiderReinforcementStrainLimit) *pPara << _T(" not");
   *pPara << _T(" included in the capacity analysis") << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Modulus of rupture for cracking moment (LRFD 5.4.2.6, 5.6.3.3)") << rptNewLine;
   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      *pPara << _T("Normal weight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::Normal]) << symbol(lambda) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) << symbol(lambda) << RPT_SQRT_FC << rptNewLine;
   }
   else
   {
      *pPara << _T("Normal weight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::Normal]) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("Sand Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("All Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) << RPT_SQRT_FC << RPT_FC << rptNewLine;
   }

   *pPara << rptNewLine;

   *pPara << _T("Resistance factors (LRFD 5.5.4.2)") << rptNewLine;
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4);
   *pPara << pTable << rptNewLine;
   (*pTable)(0, 1) << _T("Normal weight concrete");
   (*pTable)(0, 2) << _T("Lightweight concrete");
   (*pTable)(0, 3) << _T("PCI UHPC concrete");

   (*pTable)(1, 0) << _T("Tension-controlled (Reinforced)");
   (*pTable)(1, 1) << PhiTensionRC[pgsTypes::Normal];
   (*pTable)(1, 2) << PhiTensionRC[pgsTypes::SandLightweight];
   (*pTable)(1, 3) << PhiTensionRC[pgsTypes::PCI_UHPC];

   (*pTable)(2, 0) << _T("Tension-controlled (Prestressed)");
   (*pTable)(2, 1) << PhiTensionPS[pgsTypes::Normal];
   (*pTable)(2, 2) << PhiTensionPS[pgsTypes::SandLightweight];
   (*pTable)(2, 3) << PhiTensionPS[pgsTypes::PCI_UHPC];

   (*pTable)(3, 0) << _T("Tension-controlled (Spliced)");
   (*pTable)(3, 1) << PhiTensionSpliced[pgsTypes::Normal];
   (*pTable)(3, 2) << PhiTensionSpliced[pgsTypes::SandLightweight];
   (*pTable)(3, 3) << PhiTensionSpliced[pgsTypes::PCI_UHPC];

   (*pTable)(4, 0) << _T("Compression-controlled");
   (*pTable)(4, 1) << PhiCompression[pgsTypes::Normal];
   (*pTable)(4, 2) << PhiCompression[pgsTypes::SandLightweight];
   (*pTable)(4, 3) << PhiCompression[pgsTypes::PCI_UHPC];

   *pPara << rptNewLine;

   *pPara << _T("Closure joint resistance factors (LRFD 5.5.4.2, 5.12.3.4.2d)") << rptNewLine;
   pTable = rptStyleManager::CreateDefaultTable(3);
   (*pTable)(0, 1) << _T("Normal weight concrete");
   (*pTable)(0, 2) << _T("Lightweight concrete");

   (*pTable)(1, 0) << _T("Fully bonded tendons");
   (*pTable)(1, 1) << PhiClosureJoint[pgsTypes::Normal];
   (*pTable)(1, 2) << PhiClosureJoint[pgsTypes::SandLightweight];

   *pPara << pTable << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Strands are");
   if (!bIncludeStrandForNegMoment) *pPara << _T(" not");
   *pPara << _T(" included in the negative moment capacity analysis") << rptNewLine;

   *pPara << _T("Noncomposite moments are");
   if (!bIncludeNoncompositeMomentsForNegMomentDesign) *pPara << _T(" not");
   *pPara << _T(" included in the negative moment, Mu") << rptNewLine;
}

void MomentCapacityCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("MomentCapacityCriteria"), 1.0);

      pSave->Property(_T("OverReinforcedMomentCapacity"), OverReinforcedMomentCapacity);
      pSave->Property(_T("IncludeStrandForNegMoment"), bIncludeStrandForNegMoment);
      pSave->Property(_T("IncludeRebarForCapacity"), bIncludeRebar);
      pSave->Property(_T("ConsiderReinforcementStrainLimit"), bConsiderReinforcementStrainLimit);
      pSave->Property(_T("MomentCapacitySliceCount"), nMomentCapacitySlices);
      pSave->Property(_T("bIncludeNoncompositeMomentsForNegMomentDesign"), bIncludeNoncompositeMomentsForNegMomentDesign);

      pSave->BeginUnit(_T("ModulusOfRupture"), 1.0);
         pSave->Property(_T("Normal"), ModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"), ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"), ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // ModulusOfRupture
      
      pSave->BeginUnit(_T("ResistanceFactor"), 1.0);
         pSave->BeginUnit(_T("NormalWeight"), 1.0);
            pSave->Property(_T("TensionControlled_RC"), PhiTensionRC[pgsTypes::Normal]);
            pSave->Property(_T("TensionControlled_PS"), PhiTensionPS[pgsTypes::Normal]);
            pSave->Property(_T("TensionControlled_Spliced"), PhiTensionSpliced[pgsTypes::Normal]);
            pSave->Property(_T("CompressionControlled"), PhiCompression[pgsTypes::Normal]);
         pSave->EndUnit(); // NormalWeight
         
         pSave->BeginUnit(_T("AllLightweight"), 1.0);
            pSave->Property(_T("TensionControlled_RC"), PhiTensionRC[pgsTypes::AllLightweight]);
            pSave->Property(_T("TensionControlled_PS"), PhiTensionPS[pgsTypes::AllLightweight]);
            pSave->Property(_T("TensionControlled_Spliced"), PhiTensionSpliced[pgsTypes::AllLightweight]);
            pSave->Property(_T("CompressionControlled"), PhiCompression[pgsTypes::AllLightweight]);
         pSave->EndUnit(); // AllLightweight

         pSave->BeginUnit(_T("SandLightweight"), 1.0);
            pSave->Property(_T("TensionControlled_RC"), PhiTensionRC[pgsTypes::SandLightweight]);
            pSave->Property(_T("TensionControlled_PS"), PhiTensionPS[pgsTypes::SandLightweight]);
            pSave->Property(_T("TensionControlled_Spliced"), PhiTensionSpliced[pgsTypes::SandLightweight]);
            pSave->Property(_T("CompressionControlled"), PhiCompression[pgsTypes::SandLightweight]);
         pSave->EndUnit(); // SandLightweight
      
      pSave->EndUnit(); // ResistanceFactor

      pSave->BeginUnit(_T("ClosureJointResistanceFactor"), 1.0);
         pSave->BeginUnit(_T("NormalWeight"), 1.0);
            pSave->Property(_T("FullyBondedTendons"), PhiClosureJoint[pgsTypes::Normal]);
         pSave->EndUnit(); // NormalWeight
         pSave->BeginUnit(_T("AllLightweight"), 1.0);
            pSave->Property(_T("FullyBondedTendons"), PhiClosureJoint[pgsTypes::AllLightweight]);
         pSave->EndUnit(); // AllLightweight
         pSave->BeginUnit(_T("SandLightweight"), 1.0);
            pSave->Property(_T("FullyBondedTendons"), PhiClosureJoint[pgsTypes::SandLightweight]);
         pSave->EndUnit(); // SandLightweight
      pSave->EndUnit(); // ClosureJointResistanceFactor

   pSave->EndUnit(); // MomentCapacityCriteria
}

void MomentCapacityCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("MomentCapacityCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if(!pLoad->Property(_T("OverReinforcedMomentCapacity"), &OverReinforcedMomentCapacity)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("IncludeStrandForNegMoment"), &bIncludeStrandForNegMoment)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("IncludeRebarForCapacity"), &bIncludeRebar)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("ConsiderReinforcementStrainLimit"), &bConsiderReinforcementStrainLimit)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("MomentCapacitySliceCount"), &nMomentCapacitySlices)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("bIncludeNoncompositeMomentsForNegMomentDesign"), &bIncludeNoncompositeMomentsForNegMomentDesign)) THROW_LOAD(InvalidFileFormat, pLoad);
   
   if(!pLoad->BeginUnit(_T("ModulusOfRupture"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("Normal"), &ModulusOfRuptureCoefficient[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("AllLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("SandLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad); // ModulusOfRupture

   if(!pLoad->BeginUnit(_T("ResistanceFactor"))) THROW_LOAD(InvalidFileFormat, pLoad);
   
   if(!pLoad->BeginUnit(_T("NormalWeight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_RC"), &PhiTensionRC[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_PS"), &PhiTensionPS[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_Spliced"), &PhiTensionSpliced[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionControlled"), &PhiCompression[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad);; // NormalWeight

   if(!pLoad->BeginUnit(_T("AllLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_RC"), &PhiTensionRC[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_PS"), &PhiTensionPS[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_Spliced"), &PhiTensionSpliced[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionControlled"), &PhiCompression[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad); // AllLightweight

   if(!pLoad->BeginUnit(_T("SandLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_RC"), &PhiTensionRC[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_PS"), &PhiTensionPS[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("TensionControlled_Spliced"), &PhiTensionSpliced[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionControlled"), &PhiCompression[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad);; // SandLightweight

   if(!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad); // ResistanceFactor

   if(!pLoad->BeginUnit(_T("ClosureJointResistanceFactor"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if(!pLoad->BeginUnit(_T("NormalWeight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("FullyBondedTendons"), &PhiClosureJoint[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);; // NormalWeight

   if(!pLoad->BeginUnit(_T("AllLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("FullyBondedTendons"), &PhiClosureJoint[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);; // AllLightweight

   if(!pLoad->BeginUnit(_T("SandLightweight"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("FullyBondedTendons"), &PhiClosureJoint[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);; // SandLightweight

   if(!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad); // ClosureJointResistanceFactor

   if (!pLoad->EndUnit())  THROW_LOAD(InvalidFileFormat, pLoad); // MomentCapacityCriteria
}

Float64 MomentCapacityCriteria::GetModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   PRECONDITION(!IsUHPC(type));
   return ModulusOfRuptureCoefficient[type];
}

void MomentCapacityCriteria::GetResistanceFactors(pgsTypes::ConcreteType type, Float64* phiTensionPS, Float64* phiTensionRC, Float64* phiTensionSpliced, Float64* phiCompression) const
{
   PRECONDITION(type != pgsTypes::UHPC); // the values for UHPC are just placeholders at this time... there isn't a UI to modify them and they aren't used in the main program
   *phiTensionPS = PhiTensionPS[type];
   *phiTensionRC = PhiTensionRC[type];
   *phiTensionSpliced = PhiTensionSpliced[type];
   *phiCompression = PhiCompression[type];
}

Float64 MomentCapacityCriteria::GetClosureJointResistanceFactor(pgsTypes::ConcreteType type) const
{
   return PhiClosureJoint[type];
}
