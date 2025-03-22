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
#include <psgLib/PrestressedElementCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool PrestressedElementCriteria::Compare(const PrestressedElementCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (!::IsEqual(CompressionStressCoefficient_BeforeLosses, other.CompressionStressCoefficient_BeforeLosses)
      or
      TensionStressLimit_WithReinforcement_BeforeLosses != other.TensionStressLimit_WithReinforcement_BeforeLosses
      or
      TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses != other.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Stress Limits for Temporary Stresses are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if(!IsEqual(MaxCoverToUseHigherTensionStressLimit, other.MaxCoverToUseHigherTensionStressLimit))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Cover Limits for Temporary Stresses are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   bool bServiceITension = (bCheckFinalServiceITension == other.bCheckFinalServiceITension);
   if (bServiceITension && TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses != other.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses)
   {
      bServiceITension = false;
   }

   if (!::IsEqual(CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses, other.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses)
      or
      !::IsEqual(CompressionStressCoefficient_AllLoads_AfterLosses, other.CompressionStressCoefficient_AllLoads_AfterLosses)
      or
      !bServiceITension
      or
      TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses != other.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses
      or
      TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses != other.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Stress Limits at Service Limit State are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(CompressionStressCoefficient_Fatigue, other.CompressionStressCoefficient_Fatigue))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Allowable Concrete Stress at Fatigue Limit State are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   bool bTempStrandRemovalStresses = true;
   bool bDeckPlacementStresses = true;
   if (bCheckTemporaryStresses != other.bCheckTemporaryStresses)
   {
      if (!::IsEqual(CompressionStressCoefficient_TemporaryStrandRemoval, other.CompressionStressCoefficient_TemporaryStrandRemoval)
         or
         TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval != other.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval
         or
         TensionStressLimit_WithReinforcement_TemporaryStrandRemoval != other.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval)
      {
         bTempStrandRemovalStresses = false;
      }

      if (!::IsEqual(CompressionStressCoefficient_AfterDeckPlacement, other.CompressionStressCoefficient_AfterDeckPlacement)
         or
         TensionStressLimit_AfterDeckPlacement != other.TensionStressLimit_AfterDeckPlacement)
      {
         bDeckPlacementStresses = false;
      }
   }

   if (bCheckTemporaryStresses != other.bCheckTemporaryStresses ||
      (bCheckTemporaryStresses == true && (!bTempStrandRemovalStresses || !bDeckPlacementStresses)))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Stress Limits for Temporary Loading Conditions are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void PrestressedElementCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits for Concrete - Prestressed Members, LRFD 5.9.2.3") << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Concrete Stress Limits - Temporary Stresses ")<< WBFL::LRFD::LrfdLosses10th(WBFL::LRFD::ltTemporary) << _T("- LRFD 5.9.2.3.1") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("- Compressive stress : ") << CompressionStressCoefficient_BeforeLosses << RPT_FCI << rptNewLine;
   *pPara << _T("- Tension stress in areas other than the precompressed tensile zone and without bonded reinforcement : "); TensionStressLimit_WithReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;
   *pPara << _T("- Tension stress in areas with bonded reinforcement (reinforcing bars or prestress steel sufficient to resist the tensile force in the concrete computed assuming an uncracked section, where reinforcement is proportioned using a stress of 0.5") << RPT_FY << _T(", not to exceed 30.0 ksi : ");
   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;
   if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition())
   {
      *pPara << _T("Maximum cover needed for reinforcement bars to resist tensile force required to use higher stress limit : ") << dim.SetValue(MaxCoverToUseHigherTensionStressLimit) << rptNewLine;
   }
   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Concrete Stress Limits - Stresses at Service Limit State") << WBFL::LRFD::LrfdLosses10th(WBFL::LRFD::ltService) << _T("(LRFD 5.9.2.3.2") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T(" - Compressive stress due to the sum of effective prestress and permanent loads : ") << CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses << RPT_FC << rptNewLine;
   *pPara << _T(" - Compressive stress due to the sum of effective prestress, permanent loads, and transient loads : ") << CompressionStressCoefficient_AllLoads_AfterLosses << RPT_FC << rptNewLine;
   *pPara << _T(" - Tension in the Precompressed Tensile Zone, Assuming Uncracked Sections") << rptNewLine;
   *pPara << _T("   * For components with bonded prestressing tendons or reinforcement that are subjected to not worse than moderate corrosion conditions : "); TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("   * For components with bonded prestressing tendons or reinforcement that are subjected to severe corrosive conditions : "); TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;

   if (bCheckFinalServiceITension)
   {
      *pPara << _T(" - (Optional, Enabled) Tension due to the sum of effective prestress and permanent loads : "); TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   }

   if (bCheckTemporaryStresses)
   {
      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pPara;
      *pPara << _T("Concrete Stress Limits for Temporary Loading Conditions (Optional) (PGSuper only)") << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("Stress Limits immediately after Temporary Strand Removal") << rptNewLine;
      *pPara << _T("Compression Stress : ") << CompressionStressCoefficient_TemporaryStrandRemoval << RPT_FC << rptNewLine;
      *pPara << _T("Tension Stress") << rptNewLine;
      *pPara << _T("- In areas other than the precompressed tensile zone : "); TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
      *pPara << _T("- In areas with sufficient bonded reinforcement : "); TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
      *pPara << rptNewLine;
      *pPara << _T("Stress Limits immediately after Deck Placement") << rptNewLine;
      *pPara << _T("Compression Stress : ") << CompressionStressCoefficient_AfterDeckPlacement << RPT_FC << rptNewLine;
      *pPara << _T("Tension Stress : "); TensionStressLimit_AfterDeckPlacement.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   }


   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Concrete Stress Limits - Fatigue Limit State, LRFD 5.5.3.1") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("For prestressed components in other than segmentally constructed bridges, the compressive stress due to the Fatigue I load combination plus one-half the sum of the unfactored effective prestress and permanent loads shall not exceed ")
      << CompressionStressCoefficient_Fatigue << RPT_FC << _T(" after losses") << rptNewLine;
}

void PrestressedElementCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("PrestressedElementCriteria"), 2.0);

   pSave->BeginUnit(_T("BeforeLosses"), 1.0);

   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_BeforeLosses);
   TensionStressLimit_WithReinforcement_BeforeLosses.Save(_T("TensionStressLimit_WithReinforcement"), pSave);

   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Save(_T("TensionStressLimit_WithoutReinforcement"), pSave);

   pSave->Property(_T("MaxCoverToUseHigherTensionStressLimit"), MaxCoverToUseHigherTensionStressLimit);

   pSave->EndUnit(); // Before losses

   pSave->BeginUnit(_T("AfterLosses"), 1.0);

   pSave->Property(_T("CompressionStressCoefficient_PermanentLoadsOnly"),CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);
   pSave->Property(_T("CompressionStressCoefficient_AllLoads"), CompressionStressCoefficient_AllLoads_AfterLosses);
   TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Save(_T("TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions"), pSave);
   TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Save(_T("TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions"), pSave);

   pSave->BeginUnit(_T("OptionalFinalTensionStressLimit"), 1.0);
   pSave->Property(_T("bCheckFinalServiceITension"), bCheckFinalServiceITension);
   TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Save(_T("TensionStressLimit_ServiceI_PermanentLoadsOnly"), pSave);
   pSave->EndUnit(); // OptionalFinalTensionStressLimit

   pSave->EndUnit(); // Afterlosses

   pSave->BeginUnit(_T("Fatigue"), 1.0);
   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_Fatigue);
   pSave->EndUnit(); // CompressionStressCoefficient

   pSave->BeginUnit(_T("OptionalTemporaryStressLimits"), 1.0);
   pSave->Property(_T("bCheckTemporaryStresses"), bCheckTemporaryStresses);
   pSave->BeginUnit(_T("AfterTemporaryStrandRemoval"), 1.0);
   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_TemporaryStrandRemoval);
   TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Save(_T("TensionStressLimit_WithoutReinforcement"), pSave);
   TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Save(_T("TensionStressLimit_WithReinforcement"), pSave);
   pSave->EndUnit(); // AfterTemporaryStrandRemoval

   pSave->BeginUnit(_T("AfterDeckPlacement"), 1.0);
   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_AfterDeckPlacement);
   TensionStressLimit_AfterDeckPlacement.Save(_T("TensionStressLimit"), pSave);
   pSave->EndUnit(); // AfterDeckPlacement
   pSave->EndUnit(); // OptionalTemporaryStressLimits

   pSave->EndUnit(); // PrestressedElementCriteria
}

void PrestressedElementCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("PrestressedElementCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   Float64 elversion = pLoad->GetVersion();

   if (!pLoad->BeginUnit(_T("BeforeLosses")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_BeforeLosses)) THROW_LOAD(InvalidFileFormat, pLoad);
   TensionStressLimit_WithReinforcement_BeforeLosses.Load(_T("TensionStressLimit_WithReinforcement"),pLoad);
   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Load(_T("TensionStressLimit_WithoutReinforcement"),pLoad);

   if (elversion > 1)
   {
      if (!pLoad->Property(_T("MaxCoverToUseHigherTensionStressLimit"), &MaxCoverToUseHigherTensionStressLimit)) THROW_LOAD(InvalidFileFormat, pLoad);
   }

   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Before losses

   if(!pLoad->BeginUnit(_T("AfterLosses"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if(!pLoad->Property(_T("CompressionStressCoefficient_PermanentLoadsOnly"), &CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionStressCoefficient_AllLoads"), &CompressionStressCoefficient_AllLoads_AfterLosses)) THROW_LOAD(InvalidFileFormat, pLoad);
   TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Load(_T("TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions"),pLoad);
   TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Load(_T("TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions"),pLoad);

   if(!pLoad->BeginUnit(_T("OptionalFinalTensionStressLimit"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("bCheckFinalServiceITension"), &bCheckFinalServiceITension)) THROW_LOAD(InvalidFileFormat, pLoad);
   TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Load(_T("TensionStressLimit_ServiceI_PermanentLoadsOnly"),pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // OptionalFinalTensionStressLimit

   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Afterlosses

   if(!pLoad->BeginUnit(_T("Fatigue"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_Fatigue)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // CompressionStressCoefficient

   if(!pLoad->BeginUnit(_T("OptionalTemporaryStressLimits"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("bCheckTemporaryStresses"), &bCheckTemporaryStresses)) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->BeginUnit(_T("AfterTemporaryStrandRemoval"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_TemporaryStrandRemoval)) THROW_LOAD(InvalidFileFormat, pLoad);
   TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Load(_T("TensionStressLimit_WithoutReinforcement"),pLoad);
   TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Load(_T("TensionStressLimit_WithReinforcement"),pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // AfterTemporaryStrandRemoval

   if(!pLoad->BeginUnit(_T("AfterDeckPlacement"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if(!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_AfterDeckPlacement)) THROW_LOAD(InvalidFileFormat, pLoad);
   TensionStressLimit_AfterDeckPlacement.Load(_T("TensionStressLimit"),pLoad);
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // AfterDeckPlacement
   if(!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // OptionalTemporaryStressLimits

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // PrestressedElementCriteria
}
