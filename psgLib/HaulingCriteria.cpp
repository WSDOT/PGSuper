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
#include <psgLib/HaulingCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

bool KDOTHaulingCriteria::Compare(const KDOTHaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (!::IsEqual(OverhangGFactor, other.OverhangGFactor) ||
      !::IsEqual(InteriorGFactor, other.InteriorGFactor))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling Dynamic Load Factors are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!IsEqual(CompressionStressLimitCoefficient, other.CompressionStressLimitCoefficient)
      or
      TensionStressLimitWithoutReinforcement != other.TensionStressLimitWithoutReinforcement
      or
      TensionStressLimitWithReinforcement != other.TensionStressLimitWithReinforcement)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling concrete stress limits are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void KDOTHaulingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("KDOTCriteria"), 1.0);

   pSave->Property(_T("OverhangGFactor"), OverhangGFactor);
   pSave->Property(_T("InteriorGFactor"), InteriorGFactor);
   pSave->Property(_T("CompressionStressLimitCoefficient"), CompressionStressLimitCoefficient);

   TensionStressLimitWithoutReinforcement.Save(_T("TensionStressLimitWithoutReinforcement"),pSave);
   TensionStressLimitWithReinforcement.Save(_T("TensionStressLimitWithReinforcement"), pSave);

   pSave->EndUnit();
}

void KDOTHaulingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   if (!pLoad->BeginUnit(_T("KDOTCriteria"))) THROW_LOAD(InvalidFileFormat,pLoad);

   if (!pLoad->Property(_T("OverhangGFactor"), &OverhangGFactor)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("InteriorGFactor"), &InteriorGFactor)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CompressionStressLimitCoefficient"), &CompressionStressLimitCoefficient)) THROW_LOAD(InvalidFileFormat, pLoad);

   TensionStressLimitWithoutReinforcement.Load(_T("TensionStressLimitWithoutReinforcement"),pLoad);

   TensionStressLimitWithReinforcement.Load(_T("TensionStressLimitWithReinforcement"),pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

void KDOTHaulingCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Hauling Criteria - KDOT Method") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Dynamic 'G' Factors") << rptNewLine;
   *pPara << _T("- In Cantilever Region = ") << OverhangGFactor << rptNewLine;
   *pPara << _T("- Between Bunk Points = ") << InteriorGFactor << rptNewLine;

   *pPara << _T("Concrete Stress Limits - Hauling") << rptNewLine;
   *pPara << _T("- Compressive Stress = ") << CompressionStressLimitCoefficient << RPT_FC << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = "); TensionStressLimitWithoutReinforcement.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = "); TensionStressLimitWithReinforcement.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
}

void WSDOTHaulingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("WSDOTHaulingCriteria"), 1.0);
   pSave->Property(_T("FsCracking"), FsCracking);
   pSave->Property(_T("FsFailure"), FsFailure);

   pSave->BeginUnit(_T("ModulusOfRupture"), 1.0);
      pSave->Property(_T("Normal"), ModulusOfRuptureCoefficient[pgsTypes::Normal]);
      pSave->Property(_T("SandLightweight"), ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->Property(_T("AllLightweight"), ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
   pSave->EndUnit(); // Modulus of Rupture

   pSave->Property(_T("ImpactUsage"), +ImpactUsage);
   pSave->Property(_T("ImpactUp"), ImpactUp);
   pSave->Property(_T("ImpactDown"), ImpactDown);

   pSave->Property(_T("RoadwayCrownSlope"), RoadwayCrownSlope);
   pSave->Property(_T("RoadwaySuperelevation"), RoadwaySuperelevation);
   pSave->Property(_T("MaxGirderSweep"), SweepTolerance);
   pSave->Property(_T("SweepGrowth"), SweepGrowth);
   pSave->Property(_T("SupportPlacementTolerance"), SupportPlacementTolerance);
   pSave->Property(_T("CamberMultiplier"), CamberMultiplier);

   pSave->Property(_T("WindLoadType"), +WindLoadType);
   pSave->Property(_T("WindLoad"), WindLoad);

   pSave->Property(_T("CentrifugalForceType"), +CentrifugalForceType);
   pSave->Property(_T("HaulingSpeed"), HaulingSpeed);
   pSave->Property(_T("TurningRadius"), TurningRadius);

   pSave->Property(_T("CompressionStressCoefficient_GlobalStress"), CompressionStressCoefficient_GlobalStress);
   pSave->Property(_T("CompressionStressCoefficient_PeakStress"), CompressionStressCoefficient_PeakStress);

   TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Save(_T("TensionStressLimitWithReinforcement_CrownSlope"), pSave);

   TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Save(_T("TensionStressLimitWithReinforcement_Superelevation"), pSave);

   TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Save(_T("TensionStressLimitWithoutReinforcement_CrownSlope"), pSave);

   TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Save(_T("TensionStressLimitWithoutReinforcement_Superelevation"), pSave);

   pSave->EndUnit();
}

void WSDOTHaulingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   if (!pLoad->BeginUnit(_T("WSDOTHaulingCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("FsCracking"), &FsCracking)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("FsFailure"), &FsFailure)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->BeginUnit(_T("ModulusOfRupture"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Normal"), &ModulusOfRuptureCoefficient[pgsTypes::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Modulus of Rupture

   Int16 value;
   if (!pLoad->Property(_T("ImpactUsage"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);  
   ImpactUsage = (WBFL::Stability::HaulingImpact)value;
   if (!pLoad->Property(_T("ImpactUp"), &ImpactUp)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("ImpactDown"), &ImpactDown)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("RoadwayCrownSlope"), &RoadwayCrownSlope)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("RoadwaySuperelevation"), &RoadwaySuperelevation)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderSweep"), &SweepTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SweepGrowth"), &SweepGrowth)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SupportPlacementTolerance"), &SupportPlacementTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CamberMultiplier"), &CamberMultiplier)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("WindLoadType"), &value)) THROW_LOAD(InvalidFileFormat, pLoad); 
   WindLoadType = (WBFL::Stability::WindLoadType)value;
   if (!pLoad->Property(_T("WindLoad"), &WindLoad)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CentrifugalForceType"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   CentrifugalForceType = (WBFL::Stability::CFType)value;
   if (!pLoad->Property(_T("HaulingSpeed"), &HaulingSpeed)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("TurningRadius"), &TurningRadius)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CompressionStressCoefficient_GlobalStress"), &CompressionStressCoefficient_GlobalStress)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CompressionStressCoefficient_PeakStress"), &CompressionStressCoefficient_PeakStress)) THROW_LOAD(InvalidFileFormat, pLoad);

   TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Load(_T("TensionStressLimitWithReinforcement_CrownSlope"),pLoad);

   TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Load(_T("TensionStressLimitWithReinforcement_Superelevation"),pLoad);

   TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Load(_T("TensionStressLimitWithoutReinforcement_CrownSlope"),pLoad);

   TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Load(_T("TensionStressLimitWithoutReinforcement_Superelevation"),pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

void WSDOTHaulingCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Hauling Criteria - WSDOT Method") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_UV_PROTOTYPE(rptMomentPerAngleUnitValue, spring, pDisplayUnits->GetMomentPerAngleUnit(), true);
   INIT_UV_PROTOTYPE(rptVelocityUnitValue, speed, pDisplayUnits->GetVelocityUnit(), true);
   INIT_UV_PROTOTYPE(rptPressureUnitValue, pressure, pDisplayUnits->GetWindPressureUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false);

   *pPara << _T("Factors of Safety") << rptNewLine;
   *pPara << _T("- Cracking F.S. = ") << FsCracking << rptNewLine;
   *pPara << _T("- Failure & Roll over F.S. = ") << FsFailure << rptNewLine;

   *pPara << _T("Impact Factors") << rptNewLine;
   *pPara << _T("- Upward   = ") << percentage.SetValue(ImpactUp) << rptNewLine;
   *pPara << _T("- Downward = ") << percentage.SetValue(ImpactDown) << rptNewLine;

   if (pDisplayUnits->GetUnitMode() == eafTypes::umSI)
   {
      *pPara << _T("Normal Crown Slope = ") << RoadwayCrownSlope << _T(" m/m") << rptNewLine;
      *pPara << _T("Max. Superelevation = ") << RoadwaySuperelevation << _T(" m/m") << rptNewLine;
      *pPara << _T("Sweep tolerance = ") << SweepTolerance << _T(" m/m") << rptNewLine;
   }
   else
   {
      *pPara << _T("Normal Crown Slope = ") << RoadwayCrownSlope << _T(" ft/ft") << rptNewLine;
      *pPara << _T("Max. Superelevation = ") << RoadwaySuperelevation << _T(" ft/ft") << rptNewLine;
      INT x = (INT)::RoundOff((1.0 / (SweepTolerance * 120.0)), 1.0);
      *pPara << _T("Sweep tolerance = (1/") << x << _T(" in) per 10 ft") << rptNewLine;
   }

   *pPara << _T("Support placement lateral tolerance = ") << dim.SetValue(SupportPlacementTolerance) << rptNewLine;

   *pPara << _T("Wind Loading") << rptNewLine;
   if (WindLoadType == WBFL::Stability::WindLoadType::Speed)
   {
      *pPara << _T("- Wind Speed = ") << speed.SetValue(WindLoad) << rptNewLine;
   }
   else
   {
      *pPara << _T("- Wind Pressure = ") << pressure.SetValue(WindLoad) << rptNewLine;
   }

   *pPara << _T("Centrifugal Force") << rptNewLine;
   if (CentrifugalForceType == WBFL::Stability::CFType::Adverse)
   {
      *pPara << _T("- Force is adverse") << rptNewLine;
   }
   else
   {
      *pPara << _T("- Force is favorable") << rptNewLine;
   }
   *pPara << _T("- Hauling Speed = ") << speed.SetValue(HaulingSpeed) << rptNewLine;
   *pPara << _T("- Turning Radius = ") << dim2.SetValue(TurningRadius) << rptNewLine;


   *pPara << _T("Modulus of rupture") << rptNewLine;
   if (WBFL::LRFD::LRFDVersionMgr::Version::SeventhEditionWith2016Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion())
   {
      *pPara << _T("- Normal weight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::Normal]) << symbol(lambda) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("- Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) << symbol(lambda) << RPT_SQRT_FC << rptNewLine;
   }
   else
   {
      *pPara << _T("- Normal weight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::Normal]) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("- Sand Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) << RPT_SQRT_FC << rptNewLine;
      *pPara << _T("- All Lightweight concrete: ") << tension_coefficient.SetValue(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) << RPT_SQRT_FC << RPT_FC << rptNewLine;
   }

   *pPara << _T("Concrete Stress Limits - Hauling - Normal Crown Slope") << rptNewLine;
   *pPara << _T("- Compressive Stress (General) = ") << CompressionStressCoefficient_GlobalStress << RPT_FC << rptNewLine;
   *pPara << _T("- Compressive Stress (With lateral bending) = ") << CompressionStressCoefficient_PeakStress << RPT_FC << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = "); TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = "); TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;

   *pPara << _T(" Concrete Stress Limits - Hauling - Maximum Superelevation") << rptNewLine;
   *pPara << _T("- Compressive Stress (General) = ") << CompressionStressCoefficient_GlobalStress << RPT_FC << rptNewLine;
   *pPara << _T("- Compressive Stress (With lateral bending) = ") << CompressionStressCoefficient_PeakStress << RPT_FC << rptNewLine;
   *pPara << _T("- Tensile Stress (w/o mild rebar) = "); TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("- Tensile Stress (w/  mild rebar) = "); TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
}

bool WSDOTHaulingCriteria::Compare(const WSDOTHaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (!::IsEqual(FsCracking, other.FsCracking) ||
      !::IsEqual(FsFailure, other.FsFailure))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling Factors of Safety are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::Normal], other.ModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
      (impl.GetSpecificationCriteria().GetEdition() <= WBFL::LRFD::LRFDVersionMgr::Version::SeventhEditionWith2016Interims ? !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) : false)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Modulus of Rupture for Cracking Moment During Hauling are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(ImpactUp, other.ImpactUp) ||
      !::IsEqual(ImpactDown, other.ImpactDown) ||
      ImpactUsage != other.ImpactUsage ||
      !::IsEqual(RoadwayCrownSlope, other.RoadwayCrownSlope) ||
      !::IsEqual(RoadwaySuperelevation, other.RoadwaySuperelevation) ||
      !::IsEqual(SweepTolerance, other.SweepTolerance) ||
      !::IsEqual(SweepGrowth, other.SweepGrowth) ||
      !::IsEqual(SupportPlacementTolerance, other.SupportPlacementTolerance) ||
      !::IsEqual(CamberMultiplier, other.CamberMultiplier) ||
      WindLoadType != other.WindLoadType ||
      !::IsEqual(WindLoad, other.WindLoad) ||
      CentrifugalForceType != other.CentrifugalForceType ||
      !::IsEqual(HaulingSpeed, other.HaulingSpeed) ||
      !::IsEqual(TurningRadius, other.TurningRadius)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling Analysis Parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

bool HaulingCriteria::Compare(const HaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (bCheck != other.bCheck ||
      bDesign != other.bDesign ||
      !::IsEqual(MinBunkPoint, other.MinBunkPoint) ||
      !::IsEqual(BunkPointAccuracy, other.BunkPointAccuracy) ||
      (bUseMinBunkPointLimit != other.bUseMinBunkPointLimit || (bUseMinBunkPointLimit == true && !::IsEqual(MinBunkPointLimitFactor, other.MinBunkPointLimitFactor))))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling Check/Design Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if(AnalysisMethod != other.AnalysisMethod)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hauling Analysis Methods are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT)
   {
      if (!WSDOT.Compare(other.WSDOT,impl,vDifferences,bReturnOnFirstDifference))
      {
         bSame = false;
         if (bReturnOnFirstDifference) return false;
      }
   }
   else
   {
      if (!KDOT.Compare(other.KDOT,impl,vDifferences,bReturnOnFirstDifference))
      {
         bSame = false;
         if (bReturnOnFirstDifference) return false;
      }
   }

   return bSame;
}

void HaulingCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim2, pDisplayUnits->GetSpanLengthUnit(), true);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Minimum support location = ");
   if (this->MinBunkPoint < 0)
      *pPara << _T("H from end of girder");
   else
      *pPara << dim2.SetValue(MinBunkPoint) << _T(" from end of the girder");

   if (bUseMinBunkPointLimit)
   {
      *pPara << _T(", but not less than (") << MinBunkPointLimitFactor << _T(")(girder length)");
   }
   *pPara << rptNewLine;

   if (AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT)
      WSDOT.Report(pChapter, pDisplayUnits);
   else
      KDOT.Report(pChapter, pDisplayUnits);
}

void HaulingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("HaulingCriteria"), 1.0);

   pSave->Property(_T("bCheck"),bCheck);
   pSave->Property(_T("bDesign"), bDesign);
   pSave->Property(_T("AnalysisMethod"), (Int16)AnalysisMethod);
   pSave->Property(_T("MinBunkPoint"), MinBunkPoint);
   pSave->Property(_T("BunkPointAccuracy"), BunkPointAccuracy);
   pSave->Property(_T("bUseMinBunkPointLimit"), bUseMinBunkPointLimit);
   pSave->Property(_T("MinBunkPointLimitFactor"), MinBunkPointLimitFactor);

   WSDOT.Save(pSave);
   KDOT.Save(pSave);

   pSave->EndUnit();
}

void HaulingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("HaulingCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("bCheck"), &bCheck)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("bDesign"), &bDesign)) THROW_LOAD(InvalidFileFormat,pLoad);
   
   Int16 value;
   if (!pLoad->Property(_T("AnalysisMethod"), &value)) THROW_LOAD(InvalidFileFormat,pLoad);
   AnalysisMethod = (pgsTypes::HaulingAnalysisMethod)value;

   if (!pLoad->Property(_T("MinBunkPoint"), &MinBunkPoint)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("BunkPointAccuracy"), &BunkPointAccuracy)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("bUseMinBunkPointLimit"), &bUseMinBunkPointLimit)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("MinBunkPointLimitFactor"), &MinBunkPointLimitFactor)) THROW_LOAD(InvalidFileFormat, pLoad);

   WSDOT.Load(pLoad);
   KDOT.Load(pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
