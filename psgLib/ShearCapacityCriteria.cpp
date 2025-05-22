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
#include "StdAfx.h"
#include <PsgLib/ShearCapacityCriteria.h>
#include <PsgLib/DifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

ShearCapacityCriteria::ShearCapacityCriteria()
{
   CapacityMethod = pgsTypes::scmBTEquations;
   bLimitNetTensionStrainToPositiveValues = false;
   bIncludeRebar = false;

   ModulusOfRuptureCoefficient[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(0.20, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.20, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(0.17, WBFL::Units::Measure::SqrtKSI);

   StirrupSpacingCoefficient[0] = 0.8;
   StirrupSpacingCoefficient[1] = 0.4;
   MaxStirrupSpacing[0] = WBFL::Units::ConvertToSysUnits(24.0, WBFL::Units::Measure::Inch);
   MaxStirrupSpacing[1] = WBFL::Units::ConvertToSysUnits(12.0, WBFL::Units::Measure::Inch);

   Phi[pgsTypes::Normal] = 0.9;
   Phi[pgsTypes::SandLightweight] = 0.7;
   Phi[pgsTypes::AllLightweight] = 0.7;
   Phi[pgsTypes::PCI_UHPC] = 0.9;
   Phi[pgsTypes::UHPC] = 0.9;

   PhiDebonded[pgsTypes::Normal] = 0.85; // set defaults to 8th edition
   PhiDebonded[pgsTypes::SandLightweight] = 0.85;
   PhiDebonded[pgsTypes::AllLightweight] = 0.85;
   PhiDebonded[pgsTypes::PCI_UHPC] = 0.9; // PCI UHPC has 0.85, but this is going to get sunset, so we are going to set it to the AASHTO UHPC GS value
   PhiDebonded[pgsTypes::UHPC] = 0.9;

   PhiClosureJoint[pgsTypes::Normal] = 0.90;
   PhiClosureJoint[pgsTypes::SandLightweight] = 0.70;
   PhiClosureJoint[pgsTypes::AllLightweight] = 0.70;
   PhiClosureJoint[pgsTypes::PCI_UHPC] = 0.90;
   PhiClosureJoint[pgsTypes::UHPC] = 0.90;
}

bool ShearCapacityCriteria::Compare(const ShearCapacityCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (CapacityMethod != other.CapacityMethod ||
      bLimitNetTensionStrainToPositiveValues != other.bLimitNetTensionStrainToPositiveValues
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Shear Capacity parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }


   if (!::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::Normal], other.ModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Shear modulus of rupture parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }
      
   bool bPhiFactors = true;
   for (int i = 0; i < pgsTypes::ConcreteTypeCount && bPhiFactors == true; i++)
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if (concreteType == pgsTypes::AllLightweight && WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= impl.GetSpecificationCriteria().GetEdition())
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and those parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if (!::IsEqual(Phi[concreteType], other.Phi[concreteType]))
      {
         bPhiFactors = false;
      }

      if (!::IsEqual(PhiDebonded[concreteType], other.PhiDebonded[concreteType]))
      {
         bPhiFactors = false;
      }

      if (!::IsEqual(PhiClosureJoint[concreteType], other.PhiClosureJoint[concreteType]))
      {
         bPhiFactors = false;
      }
   }

   if (!bPhiFactors)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Shear Resistance Factors are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(StirrupSpacingCoefficient[0], other.StirrupSpacingCoefficient[0]) ||
      !::IsEqual(StirrupSpacingCoefficient[1], other.StirrupSpacingCoefficient[1]) ||
      !::IsEqual(MaxStirrupSpacing[0], other.MaxStirrupSpacing[0]) ||
      !::IsEqual(MaxStirrupSpacing[1], other.MaxStirrupSpacing[1]))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Minimum Spacing of Transverse Reinforcement requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (LongitudinalReinforcementForShearMethod != other.LongitudinalReinforcementForShearMethod ||
      bIncludeRebar != other.bIncludeRebar)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Longitudinal Reinforcement for Shear requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void ShearCapacityCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Shear Capacity Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   switch (CapacityMethod)
   {
   case pgsTypes::scmBTEquations:
      *pPara << _T("Shear capacity computed in accordance with LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.8.3.4.2"), _T("5.7.3.4.2")) << _T(" (General method)") << rptNewLine;
      break;

   case pgsTypes::scmVciVcw:
      *pPara << _T("Shear capacity computed in accordance with LRFD 5.8.3.4.3 (Vci, Vcw method)") << rptNewLine;
      break;

   case pgsTypes::scmBTTables:
      *pPara << _T("Shear capacity computed in accordance with LRFD B5.1 (Beta-Theta Tables)") << rptNewLine;
      break;

   case pgsTypes::scmWSDOT2001:
      *pPara << _T("Shear capacity computed in accordance with WSDOT Bridge Design Manual (June 2001)") << rptNewLine;
      break;

   case pgsTypes::scmWSDOT2007:
      *pPara << _T("Shear capacity computed in accordance with WSDOT Bridge Design Manual (August 2007)") << rptNewLine;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   bool bAfter1999 = WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= WBFL::LRFD::BDSManager::GetEdition() ? true : false;
   std::_tstring strFcCoefficient(bAfter1999 ? _T("0.125") : _T("0.1"));
   Float64 k1 = StirrupSpacingCoefficient[0];
   Float64 k2 = StirrupSpacingCoefficient[1];
   Float64 s1 = MaxStirrupSpacing[0];
   Float64 s2 = MaxStirrupSpacing[1];
   *pPara << _T("Maximum Spacing of Transverse Reinforcement (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) << _T(")") << rptNewLine;
   if (bAfter1999)
   {
      *pPara << _T("Eqn ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) << _T("-1: If ") << italic(ON) << Sub2(_T("v"), _T("u")) << italic(OFF) << _T(" < ") << strFcCoefficient << RPT_FC << _T(" then ") << Sub2(_T("S"), _T("max")) << _T(" = ") << k1 << Sub2(_T("d"), _T("v")) << symbol(LTE) << dim.SetValue(s1) << rptNewLine;
      *pPara << _T("Eqn ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) << _T("-2: If ") << italic(ON) << Sub2(_T("v"), _T("u")) << italic(OFF) << _T(" ") << symbol(GTE) << _T(" ") << strFcCoefficient << RPT_FC << _T(" then ") << Sub2(_T("S"), _T("max")) << _T(" = ") << k2 << Sub2(_T("d"), _T("v")) << symbol(LTE) << dim.SetValue(s2) << rptNewLine;
   }
   else
   {
      *pPara << _T("Eqn ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) << _T("-1: If ") << italic(ON) << Sub2(_T("V"), _T("u")) << italic(OFF) << _T(" < ") << strFcCoefficient << RPT_FC << Sub2(_T("b"), _T("v")) << Sub2(_T("d"), _T("v")) << _T(" then ") << Sub2(_T("S"), _T("max")) << _T(" = ") << k1 << Sub2(_T("d"), _T("v")) << symbol(LTE) << dim.SetValue(s1) << rptNewLine;
      *pPara << _T("Eqn ") << WBFL::LRFD::LrfdCw8th(_T("5.8.2.7"), _T("5.7.2.6")) << _T("-2: If ") << italic(ON) << Sub2(_T("V"), _T("u")) << italic(OFF) << _T(" ") << symbol(GTE) << _T(" ") << strFcCoefficient << RPT_FC << Sub2(_T("b"), _T("v")) << Sub2(_T("d"), _T("v")) << _T(" then ") << Sub2(_T("S"), _T("max")) << _T(" = ") << k2 << Sub2(_T("d"), _T("v")) << symbol(LTE) << dim.SetValue(s2) << rptNewLine;
   }

   if (LongitudinalReinforcementForShearMethod == pgsTypes::LongitudinalReinforcementForShearMethod::LRFD)
   {
      *pPara << _T("Longitudinal reinforcement requirements computed in accordance with LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.8.3.5"), _T("5.7.3.5")) << rptNewLine;
   }
   else
   {
      CHECK(false); // should never get here - WSDOT method has been rescinded
      *pPara << _T("Longitudinal reinforcement requirements computed in accordance with WSDOT Bridge Design Manual") << rptNewLine;
   }
}

void ShearCapacityCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("ShearCapacityCriteria"), 1.0);
 
   pSave->Property(_T("ShearCapacityMethod"), (Int16)CapacityMethod);
   pSave->Property(_T("LimitNetTensionStrainToPositiveValues"), bLimitNetTensionStrainToPositiveValues);


   pSave->BeginUnit(_T("ResistanceFactor"), 1.0);
      pSave->Property(_T("Normal"), Phi[pgsTypes::Normal]);
      pSave->Property(_T("AllLightweight"), Phi[pgsTypes::AllLightweight]);
      pSave->Property(_T("SandLightweight"),Phi[pgsTypes::SandLightweight]);
      pSave->Property(_T("PCI_UHPC"), Phi[pgsTypes::PCI_UHPC]);
      pSave->Property(_T("UHPC"), Phi[pgsTypes::UHPC]);
   pSave->EndUnit(); // ResistanceFactor

   pSave->BeginUnit(_T("ResistanceFactorDebonded"), 1.0);
      pSave->Property(_T("Normal"), PhiDebonded[pgsTypes::Normal]);
      pSave->Property(_T("AllLightweight"), PhiDebonded[pgsTypes::AllLightweight]);
      pSave->Property(_T("SandLightweight"), PhiDebonded[pgsTypes::SandLightweight]);
      pSave->Property(_T("PCI_UHPC"), PhiDebonded[pgsTypes::PCI_UHPC]);
      pSave->Property(_T("UHPC"), PhiDebonded[pgsTypes::UHPC]);
   pSave->EndUnit(); // ResistanceFactorDebonded

   pSave->BeginUnit(_T("ClosureJointResistanceFactor"), 1.0);
      pSave->Property(_T("Normal"), PhiClosureJoint[pgsTypes::Normal]);
      pSave->Property(_T("AllLightweight"), PhiClosureJoint[pgsTypes::AllLightweight]);
      pSave->Property(_T("SandLightweight"), PhiClosureJoint[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // ResistanceFactor

   pSave->BeginUnit(_T("ModulusOfRupture"), 1.0);
      pSave->Property(_T("Normal"), ModulusOfRuptureCoefficient[pgsTypes::Normal]);
      pSave->Property(_T("AllLightweight"), ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
      pSave->Property(_T("SandLightweight"), ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
   pSave->EndUnit(); // ModulusOfRupture

   pSave->BeginUnit(_T("StirrupSpacing"), 1.0);
      pSave->Property(_T("StirrupSpacingCoefficient1"), StirrupSpacingCoefficient[0]);
      pSave->Property(_T("MaxStirrupSpacing1"), MaxStirrupSpacing[0]);
      pSave->Property(_T("StirrupSpacingCoefficient2"), StirrupSpacingCoefficient[1]);
      pSave->Property(_T("MaxStirrupSpacing2"), MaxStirrupSpacing[1]);
   pSave->EndUnit(); // StirrupSpacing

   pSave->BeginUnit(_T("LongitudinalReinforcementForShear"));
      pSave->Property(_T("LongitudinalReinforcementForShearMethod"), (Int16)LongitudinalReinforcementForShearMethod);
      pSave->Property(_T("IncludeRebarForCapacity"), bIncludeRebar);
   pSave->EndUnit();

   pSave->EndUnit(); // ShearCapacityCriteria
}

void ShearCapacityCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("ShearCapacityCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   Int16 temp;
   if (!pLoad->Property(_T("ShearCapacityMethod"), &temp)) THROW_LOAD(InvalidFileFormat,pLoad);
   CapacityMethod = (pgsTypes::ShearCapacityMethod)temp;

   if (!pLoad->Property(_T("LimitNetTensionStrainToPositiveValues"), &bLimitNetTensionStrainToPositiveValues)) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("ResistanceFactor"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("Normal"), &Phi[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &Phi[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &Phi[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("PCI_UHPC"), &Phi[pgsTypes::PCI_UHPC])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("UHPC"), &Phi[pgsTypes::UHPC])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("ResistanceFactorDebonded"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("Normal"), &PhiDebonded[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &PhiDebonded[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &PhiDebonded[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("PCI_UHPC"), &PhiDebonded[pgsTypes::PCI_UHPC])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("UHPC"), &PhiDebonded[pgsTypes::UHPC])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("ClosureJointResistanceFactor"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("Normal"), &PhiClosureJoint[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &PhiClosureJoint[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &PhiClosureJoint[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("ModulusOfRupture"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("Normal"), &ModulusOfRuptureCoefficient[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("StirrupSpacing"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("StirrupSpacingCoefficient1"), &StirrupSpacingCoefficient[0])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("MaxStirrupSpacing1"), &MaxStirrupSpacing[0])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("StirrupSpacingCoefficient2"), &StirrupSpacingCoefficient[1])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("MaxStirrupSpacing2"), &MaxStirrupSpacing[1])) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->BeginUnit(_T("LongitudinalReinforcementForShear"))) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("LongitudinalReinforcementForShearMethod"), &temp)) THROW_LOAD(InvalidFileFormat,pLoad);
   LongitudinalReinforcementForShearMethod = (pgsTypes::LongitudinalReinforcementForShearMethod)temp;
   if (!pLoad->Property(_T("IncludeRebarForCapacity"), &bIncludeRebar)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

Float64 ShearCapacityCriteria::GetResistanceFactor(pgsTypes::ConcreteType concreteType, bool bIsDebonded, WBFL::LRFD::BDSManager::Edition edition) const
{
   Float64 phi = -99999;
   if (bIsDebonded && WBFL::LRFD::BDSManager::Edition::EighthEdition2017 <= edition)
      phi = PhiDebonded[concreteType];
   else
      phi = Phi[concreteType];

   return phi;
}

Float64 ShearCapacityCriteria::GetClosureJointResistanceFactor(pgsTypes::ConcreteType concreteType) const
{
   return PhiClosureJoint[concreteType];
}
