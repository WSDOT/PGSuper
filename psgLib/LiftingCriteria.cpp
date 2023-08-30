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
#include <psgLib\LiftingCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include "SpecLibraryEntryImpl.h"
#include <EAF/EAFDisplayUnits.h>

LiftingCriteria::LiftingCriteria()
{
   TensionStressLimitWithoutReinforcement.Coefficient = WBFL::Units::ConvertToSysUnits(0.0948, WBFL::Units::Measure::SqrtKSI);
   TensionStressLimitWithoutReinforcement.bHasMaxValue = true;
   TensionStressLimitWithoutReinforcement.MaxValue = WBFL::Units::ConvertToSysUnits(0.200, WBFL::Units::Measure::KSI);

   TensionStressLimitWithReinforcement.Coefficient = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
   TensionStressLimitWithReinforcement.bHasMaxValue = false;

   ModulusOfRuptureCoefficient[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.21, WBFL::Units::Measure::SqrtKSI);
   ModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(0.18, WBFL::Units::Measure::SqrtKSI);
}

bool LiftingCriteria::Compare(const LiftingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (bCheck != other.bCheck
      or
      bDesign != other.bDesign
      or
      !IsEqual(MinPickPoint, other.MinPickPoint)
      or
      !IsEqual(PickPointAccuracy, other.PickPointAccuracy)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Lifting Check/Design Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if(!IsEqual(FsCracking,other.FsCracking)
      or
      !IsEqual(FsFailure,other.FsFailure)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Lifting Factors of Safety are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }


   if (!::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::Normal], other.ModulusOfRuptureCoefficient[pgsTypes::Normal])
      or
      !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight])
      or
      (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= impl.GetSpecificationCriteria().GetEdition() ? !::IsEqual(ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], other.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) : false)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Modulus of Rupture for Cracking Moment During Lifting are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(ImpactUp, other.ImpactUp) or
      !::IsEqual(ImpactDown, other.ImpactDown) or
      !::IsEqual(PickPointHeight, other.PickPointHeight) or
      !::IsEqual(LiftingLoopTolerance, other.LiftingLoopTolerance) or
      !::IsEqual(SweepTolerance, other.SweepTolerance) or
      !::IsEqual(MinCableInclination, other.MinCableInclination) or
      !::IsEqual(CamberMultiplier, other.CamberMultiplier) or
      WindLoadType != other.WindLoadType or
      !::IsEqual(WindLoad, other.WindLoad)
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Lifting Analysis Parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(CompressionStressCoefficient_GlobalStress, other.CompressionStressCoefficient_GlobalStress) or
      !::IsEqual(CompressionStressCoefficient_PeakStress, other.CompressionStressCoefficient_PeakStress) or
      TensionStressLimitWithReinforcement != other.TensionStressLimitWithReinforcement or
      TensionStressLimitWithoutReinforcement != other.TensionStressLimitWithoutReinforcement
      )
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Lifting Concrete Stresses Limits are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }


   return bSame;
}

void LiftingCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Lifting Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), true);
   INIT_UV_PROTOTYPE(rptVelocityUnitValue, speed, pDisplayUnits->GetVelocityUnit(), true);
   INIT_UV_PROTOTYPE(rptPressureUnitValue, pressure, pDisplayUnits->GetWindPressureUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false);

   *pPara << _T("Factors of Safety") << rptNewLine;
   *pPara << _T("- Cracking F.S. = ") << FsCracking << rptNewLine;
   *pPara << _T("- Failure F.S. = ") << FsFailure << rptNewLine;

   *pPara << _T("Impact Factors") << rptNewLine;
   *pPara << _T("- Upward   = ") << percentage.SetValue(ImpactUp) << rptNewLine;
   *pPara << _T("- Downward = ") << percentage.SetValue(ImpactDown) << rptNewLine;

   *pPara << _T("Height of pick point above top of girder = ") << dim.SetValue(PickPointHeight) << rptNewLine;
   *pPara << _T("Lifting loop placement tolerance = ") << dim.SetValue(LiftingLoopTolerance) << rptNewLine;

   if (pDisplayUnits->GetUnitMode() == eafTypes::umSI)
   {
      *pPara << _T("Sweep tolerance = ") << SweepTolerance << _T("m/m") << rptNewLine;
   }
   else
   {
      INT x = (INT)::RoundOff((1.0 / (SweepTolerance * 120.0)), 1.0);
      *pPara << _T("Sweep tolerance = (1/") << x << _T(" in) per 10 ft") << rptNewLine;
   }

   *pPara << _T("Min. angle of inclination of lifting cables = ") << angle.SetValue(MinCableInclination) << rptNewLine;

   *pPara << _T("Wind Loading") << rptNewLine;
   if (WindLoadType == WBFL::Stability::WindLoadType::Speed)
   {
      *pPara << _T("- Wind Speed = ") << speed.SetValue(WindLoad) << rptNewLine;
   }
   else
   {
      *pPara << _T("- Wind Pressure = ") << pressure.SetValue(WindLoad) << rptNewLine;
   }

   *pPara << rptNewLine;
    
   *pPara << _T("Modulus of rupture") << rptNewLine;
   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition())
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


   *pPara << _T("Concrete Stress Limits") << rptNewLine;
   *pPara << _T("- Compressive Stress (General) = ") << CompressionStressCoefficient_GlobalStress << RPT_FCI << rptNewLine;
   *pPara << _T("- Compressive Stress (With Lateral Bending) = ") << CompressionStressCoefficient_PeakStress << RPT_FCI << rptNewLine;

   *pPara << rptNewLine;

   *pPara << _T("Tension Stress Limits") << rptNewLine;
   *pPara << _T("Without Reinforcement - "); TensionStressLimitWithoutReinforcement.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci);
   *pPara << rptNewLine;
   *pPara << _T("With Reinforcement - "); TensionStressLimitWithReinforcement.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci);
}

void LiftingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LiftingCriteria"), 1.0);
   pSave->Property(_T("bCheck"), bCheck);
   pSave->Property(_T("bDesign"), bDesign);
   pSave->Property(_T("PickPointHeight"), PickPointHeight);
   pSave->Property(_T("LiftingLoopTolerance"), LiftingLoopTolerance);
   pSave->Property(_T("MinCableInclination"), MinCableInclination);
   pSave->Property(_T("MaxGirderSweep"), SweepTolerance);
   pSave->Property(_T("ImpactUp"), ImpactUp);
   pSave->Property(_T("ImpactDown"), ImpactDown);
   pSave->Property(_T("CamberMultiplier"), CamberMultiplier);
   pSave->Property(_T("WindLoadType"), +WindLoadType);
   pSave->Property(_T("WindLoad"), WindLoad);

   pSave->BeginUnit(_T("ModulusOfRuptureCoefficient"), 1.0);
   pSave->Property(_T("Normal"), ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::Normal]);
   pSave->Property(_T("SandLightweight"), ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::SandLightweight]);
   pSave->Property(_T("AllLightweight"), ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::AllLightweight]);
   pSave->EndUnit(); // ModulusOfRuptureCoefficient

   pSave->Property(_T("MinPickPoint"), MinPickPoint);
   pSave->Property(_T("PickPointAccuracy"), PickPointAccuracy);
   
   pSave->Property(_T("CompressionStressCoefficient_GlobalStress"), CompressionStressCoefficient_GlobalStress);
   pSave->Property(_T("CompressionStressCoefficient_PeakStress"), CompressionStressCoefficient_PeakStress);

   TensionStressLimitWithoutReinforcement.Save(_T("TensionStressLimitWithoutReinforcement"), pSave);

   TensionStressLimitWithReinforcement.Save(_T("TensionStressLimitWithReinforcement"), pSave);

   pSave->Property(_T("FsCracking"), FsCracking);
   pSave->Property(_T("FsFailure"), FsFailure);

   pSave->EndUnit();
}

void LiftingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LiftingCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("bCheck"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("bDesign"), &bDesign)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("PickPointHeight"), &PickPointHeight)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("LiftingLoopTolerance"), &LiftingLoopTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MinCableInclination"), &MinCableInclination)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxGirderSweep"), &SweepTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("ImpactUp"), &ImpactUp)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("ImpactDown"), &ImpactDown)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CamberMultiplier"), &CamberMultiplier)) THROW_LOAD(InvalidFileFormat, pLoad);
   Int16 value;
   if (!pLoad->Property(_T("WindLoadType"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   WindLoadType = (WBFL::Stability::WindLoadType)value;
   if (!pLoad->Property(_T("WindLoad"), &WindLoad)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->BeginUnit(_T("ModulusOfRuptureCoefficient"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Normal"), &ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::Normal])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SandLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::SandLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("AllLightweight"), &ModulusOfRuptureCoefficient[pgsTypes::ConcreteType::AllLightweight])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("MinPickPoint"), &MinPickPoint)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("PickPointAccuracy"), &PickPointAccuracy)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CompressionStressCoefficient_GlobalStress"), &CompressionStressCoefficient_GlobalStress)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("CompressionStressCoefficient_PeakStress"), &CompressionStressCoefficient_PeakStress)) THROW_LOAD(InvalidFileFormat, pLoad);

   TensionStressLimitWithoutReinforcement.Load(_T("TensionStressLimitWithoutReinforcement"),pLoad);

   TensionStressLimitWithReinforcement.Load(_T("TensionStressLimitWithReinforcement"),pLoad);

   if (!pLoad->Property(_T("FsCracking"), &FsCracking)) THROW_LOAD(InvalidFileFormat,pLoad);
   if (!pLoad->Property(_T("FsFailure"), &FsFailure)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
