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
#include <psgLib/ClosureJointCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool ClosureJointCriteria::Compare(const ClosureJointCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   bool bServiceITension = (bCheckFinalServiceITension == other.bCheckFinalServiceITension);
   if (bServiceITension && TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses != other.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses)
   {
      bServiceITension = false;
   }

   if (!::IsEqual(CompressionStressCoefficient_BeforeLosses, other.CompressionStressCoefficient_BeforeLosses) 
      or
      TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses != other.TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses
      or
      TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses != other.TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses
      or
      !bServiceITension
      or
      TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses != other.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses
      or
      TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses != other.TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Closure Joint Stress Limits for Temporary Stresses before Losses are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses, other.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses)
      or
      !::IsEqual(CompressionStressCoefficient_AllLoads_AfterLosses, other.CompressionStressCoefficient_AllLoads_AfterLosses)
      or
      TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses != other.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses
      or
      TensionStressLimit_InPTZ_WithReinforcement_AfterLosses != other.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses
      or
      TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses != other.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses
      or
      TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses != other.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Closure Joint Stress Limits at Service Limit State after Losses are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(CompressionStressCoefficient_Fatigue, other.CompressionStressCoefficient_Fatigue))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Closure Joint Allowable Concrete Stress at Fatigue Limit State are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void ClosureJointCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Closure Joint Criteria") << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits for Temporary Stresses before Losses (LRFD 5.9.4.1, 5.14.1.3.2d)") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Compressive stress") << rptNewLine;
   *pPara << CompressionStressCoefficient_BeforeLosses << RPT_FCI << rptNewLine;
   *pPara << _T("Tensile stress") << rptNewLine;
   *pPara << _T("- Longitudinal Stresses through Joints in the Precompressed Tensile Zone") << rptNewLine;
   *pPara << _T("  * Joints with minimum bonded auxiliary reinforcement through the joints, which is sufficient to carry the calculated tensile force at a stress of 0.5") << RPT_FY << _T("; with internal tendons or external tendons :");
   TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;
   
   *pPara << _T("  * Joints without the minimum bonded auxiliary reinforcement through the joints : ");
   TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;

   *pPara << _T("- Stresses in Other Areas") << rptNewLine;
   *pPara << _T("  * For areas without bonded nonprestressed reinforcement : ");
   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;
   *pPara << _T("  * In areas with bonded reinforcement (reinforcing bars or prestressing steel) sufficient to resist the tensile force in the concrete computed assuming an uncracked section, where reinforcement is proportioned using a stress of 0.5") << RPT_FY << _T(", not to exceed 30.0 ksi : ");
   TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fci); *pPara << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits at Service Limit State after Losses (LRFD 5.9.4.2, 5.14.1.3.2d)") << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Compressive stress") << rptNewLine;
   *pPara << _T("- Due to the sum of effective prestress and permanent loads : ") << CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses << RPT_FC << rptNewLine;
   *pPara << _T("- Due to the sum of effective prestress, permanent loads, and transient loads : ") << CompressionStressCoefficient_AllLoads_AfterLosses << RPT_FC << rptNewLine;
   *pPara << _T("Tension stress") << rptNewLine;

   if (bCheckFinalServiceITension)
   {
      *pPara << _T(" - (Optional, Enabled) Tension due to the sum of effective prestress and permanent loads : "); TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Report(pPara,pDisplayUnits,TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   }

   *pPara << _T("- Longitudinal Stresses through Joints in the Precompressed Tensile Zone") << rptNewLine;
   *pPara << _T("  * Joints with minimum bonded auxiliary reinforcement through the joints, which is sufficient to carry the calculated tensile force at a stress of 0.5") << RPT_FY << _T("; with internal tendons or external tendons :");
   TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("  * Joints without the minimum bonded auxiliary reinforcement through the joints : ");
   TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("- Stresses in Other Areas") << rptNewLine;
   *pPara << _T("  * For areas without bonded nonprestressed reinforcement : ");
   TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;
   *pPara << _T("  * In areas with bonded reinforcement (reinforcing bars or prestressing steel) sufficient to resist the tensile force in the concrete computed assuming an uncracked section, where reinforcement is proportioned using a stress of 0.5") << RPT_FY << _T(", not to exceed 30.0 ksi : ");
   TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc); *pPara << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits for Fatigue Limit State (LRFD 5.5.3.1)") << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Fatigue I (live load plus one-half of permanent loads) : ") << CompressionStressCoefficient_Fatigue << RPT_FC << rptNewLine;
}

void ClosureJointCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("ClosureJointCriteria"), 1.0);

   pSave->BeginUnit(_T("BeforeLosses"), 1.0);
   
   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_BeforeLosses);
   
   TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Save(_T("TensionStressLimit_InPTZ_WithoutReinforcement"), pSave);

   TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Save(_T("TensionStressLimit_InPTZ_WithReinforcement"), pSave);

   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Save(_T("TensionStressLimit_OtherAreas_WithoutReinforcement"), pSave);

   TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Save(_T("TensionStressLimit_OtherAreas_WithReinforcement"), pSave);

   pSave->EndUnit(); // BeforeLosses

   // Version 2 added OptionalFinalTensionStressLimit
   pSave->BeginUnit(_T("AfterLosses"), 2.0);

   pSave->Property(_T("CompressionStressCoefficient_PermanentLoadsOnly"), CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);
   pSave->Property(_T("CompressionStressCoefficient_AllLoads"), CompressionStressCoefficient_AllLoads_AfterLosses);

   TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Save(_T("TensionStressLimit_InPTZ_WithoutReinforcement"), pSave);

   TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Save(_T("TensionStressLimit_InPTZ_WithReinforcement"), pSave);

   TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Save(_T("TensionStressLimit_OtherAreas_WithoutReinforcement"), pSave);

   TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Save(_T("TensionStressLimit_OtherAreas_WithReinforcement"), pSave);

   pSave->BeginUnit(_T("OptionalFinalTensionStressLimit"),1.0);
   pSave->Property(_T("bCheckFinalServiceITension"),bCheckFinalServiceITension);
   TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Save(_T("TensionStressLimit_ServiceI_PermanentLoadsOnly"),pSave);
   pSave->EndUnit(); // OptionalFinalTensionStressLimit

   pSave->EndUnit(); // AfterLosses

   pSave->BeginUnit(_T("Fatigue"), 1.0);
   pSave->Property(_T("CompressionStressCoefficient"), CompressionStressCoefficient_Fatigue);
   pSave->EndUnit(); // Fatigue

   pSave->EndUnit(); // ClosureJointCriteria
}

void ClosureJointCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("ClosureJointCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->BeginUnit(_T("BeforeLosses")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_BeforeLosses))  THROW_LOAD(InvalidFileFormat, pLoad);

   TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Load(_T("TensionStressLimit_InPTZ_WithoutReinforcement"),pLoad);

   TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Load(_T("TensionStressLimit_InPTZ_WithReinforcement"),pLoad);

   TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Load(_T("TensionStressLimit_OtherAreas_WithoutReinforcement"),pLoad);

   TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Load(_T("TensionStressLimit_OtherAreas_WithReinforcement"),pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // BeforeLosses

   if (!pLoad->BeginUnit(_T("AfterLosses"))) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CompressionStressCoefficient_PermanentLoadsOnly"), &CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CompressionStressCoefficient_AllLoads"), &CompressionStressCoefficient_AllLoads_AfterLosses)) THROW_LOAD(InvalidFileFormat, pLoad);

   TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Load(_T("TensionStressLimit_InPTZ_WithoutReinforcement"),pLoad);

   TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Load(_T("TensionStressLimit_InPTZ_WithReinforcement"),pLoad);

   TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Load(_T("TensionStressLimit_OtherAreas_WithoutReinforcement"),pLoad);

   TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Load(_T("TensionStressLimit_OtherAreas_WithReinforcement"),pLoad);

   if (pLoad->GetVersion() > 1.0)
   {
      if (!pLoad->BeginUnit(_T("OptionalFinalTensionStressLimit"))) THROW_LOAD(InvalidFileFormat,pLoad);
      if (!pLoad->Property(_T("bCheckFinalServiceITension"),&bCheckFinalServiceITension)) THROW_LOAD(InvalidFileFormat,pLoad);
      TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Load(_T("TensionStressLimit_ServiceI_PermanentLoadsOnly"),pLoad);
      if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat,pLoad); // OptionalFinalTensionStressLimit
   }

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // AfterLosses

   if (!pLoad->BeginUnit(_T("Fatigue"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CompressionStressCoefficient"), &CompressionStressCoefficient_Fatigue)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // Fatigue

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // ClosureJointCriteria
}
