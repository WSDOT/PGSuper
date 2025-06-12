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
#include <PsgLib/TendonStressCriteria.h>
#include <PsgLib/DifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

TendonStressCriteria::TendonStressCriteria()
{
   bCheckAtJacking = false;
   bCheckPriorToSeating = true;

   TendonStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved] = 0.76;
   TendonStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation] = 0.80;
   TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::StressRelieved] = 0.90;
   TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::LowRelaxation] = 0.90;
   TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::StressRelieved] = 0.70;
   TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::LowRelaxation] = 0.70;
   TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::StressRelieved] = 0.70;
   TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::LowRelaxation] = 0.74;
   TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved] = 0.80;
   TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation] = 0.80;
}

bool TendonStressCriteria::operator==(const TendonStressCriteria& other) const
{
   bool bPTAtJacking = true;
   if (bCheckAtJacking != other.bCheckAtJacking ||
      (bCheckAtJacking == true && (!::IsEqual(TendonStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved], other.TendonStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved]) ||
         !::IsEqual(TendonStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation], other.TendonStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation]))))
   {
      bPTAtJacking = false;
   }

   bool bPTPriorToSeating = true;
   if (bCheckPriorToSeating != other.bCheckPriorToSeating ||
      (bCheckPriorToSeating == true && (!::IsEqual(TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::StressRelieved], other.TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::StressRelieved]) ||
         !::IsEqual(TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::LowRelaxation], other.TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::LowRelaxation]))))
   {
      bPTPriorToSeating = false;
   }

   bool bPTAfterSeating = true;
   if (!::IsEqual(TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::StressRelieved], other.TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::StressRelieved]) ||
      !::IsEqual(TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::LowRelaxation], other.TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::LowRelaxation]))
   {
      bPTAfterSeating = false;
   }

   bool bPTElsewhereAfterSeating = true;
   if (!::IsEqual(TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::StressRelieved], other.TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::StressRelieved]) ||
      !::IsEqual(TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::LowRelaxation], other.TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::LowRelaxation]))
   {
      bPTElsewhereAfterSeating = false;
   }

   bool bPTFinal = true;
   if (!::IsEqual(TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved], other.TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved]) ||
      !::IsEqual(TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation], other.TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation]))
   {
      bPTFinal = false;
   }

   return (bPTAtJacking and bPTPriorToSeating and bPTAfterSeating and bPTElsewhereAfterSeating and bPTFinal);
}

bool TendonStressCriteria::operator!=(const TendonStressCriteria& other) const
{
   return !operator==(other);
}

bool TendonStressCriteria::Compare(const TendonStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if(operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Stress Limits for Post-tensioning are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void TendonStressCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits for Post-Tensioned Tendons") << rptNewLine;

   pPara = new rptParagraph;
    *pChapter << pPara;

    if (bCheckAtJacking)
    {
       *pPara << rptNewLine;
       *pPara << _T("Stress Limit at Jacking") << rptNewLine;
       *pPara << _T("- Stress Relieved Strand = ") << GetTendonStressCoefficient(CheckStage::AtJacking, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
       *pPara << _T("- Low Relaxation Strand = ") << GetTendonStressCoefficient(CheckStage::AtJacking, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;
    }

    if (bCheckPriorToSeating)
    {
       *pPara << rptNewLine;
       *pPara << _T("Stress Limit Immediately Prior to Seating") << rptNewLine;
       *pPara << _T("- Stress Relieved Strand = ") << GetTendonStressCoefficient(CheckStage::PriorToSeating, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
       *pPara << _T("- Low Relaxation Strand = ") << GetTendonStressCoefficient(CheckStage::PriorToSeating, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;
    }

   *pPara << rptNewLine;
   *pPara << _T("Stress Limit at anchorages immediately after anchor set") << rptNewLine;
   *pPara << _T("- Stress Relieved Strand = ") << GetTendonStressCoefficient(CheckStage::AtAnchoragesAfterSeating, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
   *pPara << _T("- Low Relaxation Strand = ") << GetTendonStressCoefficient(CheckStage::AtAnchoragesAfterSeating, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Stress Limit at at anchor set elsewhere along the length of the member away from anchorages") << rptNewLine;
   *pPara << _T("- Stress Relieved Strand = ") << GetTendonStressCoefficient(CheckStage::ElsewhereAfterSeating, StrandType::StressRelieved) << RPT_STRESS(_T("py")) << rptNewLine;
   *pPara << _T("- Low Relaxation Strand = ") << GetTendonStressCoefficient(CheckStage::ElsewhereAfterSeating, StrandType::LowRelaxation) << RPT_STRESS(_T("py")) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Stress Limit at service limit state after all losses") << rptNewLine;
   *pPara << _T("- Stress Relieved Strand = ") << GetTendonStressCoefficient(CheckStage::AfterAllLosses, StrandType::StressRelieved) << RPT_STRESS(_T("py")) << rptNewLine;
   *pPara << _T("- Low Relaxation Strand = ") << GetTendonStressCoefficient(CheckStage::AfterAllLosses, StrandType::LowRelaxation) << RPT_STRESS(_T("py")) << rptNewLine;
}

void TendonStressCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("TendonStressCriteria"), 1.0);
   pSave->Property(_T("CheckTendonStressAtJacking"), bCheckAtJacking);
   pSave->Property(_T("CheckTendonStressPriorToSeating"), bCheckPriorToSeating);
   pSave->Property(_T("Coeff_AtJacking_StressRel"), TendonStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"), TendonStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation]);
   pSave->Property(_T("Coeff_PriorToSeating_StressRel"), TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_PriorToSeating_LowRelax"), TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::LowRelaxation]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"), TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"), TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::LowRelaxation]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"), TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"), TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::LowRelaxation]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"), TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"), TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation]);
   pSave->EndUnit();
}

void TendonStressCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("TendonStressCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CheckTendonStressAtJacking"), &bCheckAtJacking)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CheckTendonStressPriorToSeating"), &bCheckPriorToSeating)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"), &TendonStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"), &TendonStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Coeff_PriorToSeating_StressRel"), &TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_PriorToSeating_LowRelax"), &TendonStressCoeff[+CheckStage::PriorToSeating][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"), &TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"), &TendonStressCoeff[+CheckStage::AtAnchoragesAfterSeating][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"), &TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"), &TendonStressCoeff[+CheckStage::ElsewhereAfterSeating][+StrandType::LowRelaxation]))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"), &TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"), &TendonStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

Float64 TendonStressCriteria::GetTendonStressCoefficient(CheckStage stage, StrandType strandType) const
{
   return TendonStressCoeff[+stage][+strandType];
}
