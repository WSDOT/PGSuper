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
#include <psgLib\StrandStressCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

StrandStressCriteria::StrandStressCriteria()
{
   bCheckStrandStress[+CheckStage::AtJacking] = false;
   bCheckStrandStress[+CheckStage::BeforeTransfer] = true;
   bCheckStrandStress[+CheckStage::AfterTransfer] = false;
   bCheckStrandStress[+CheckStage::AfterAllLosses] = true;

   StrandStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved] = 0.72;
   StrandStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation] = 0.78;
   StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::StressRelieved] = 0.70;
   StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::LowRelaxation] = 0.75;
   StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::StressRelieved] = 0.70;
   StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::LowRelaxation] = 0.74;
   StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved] = 0.80;
   StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation] = 0.80;
}

bool StrandStressCriteria::operator==(const StrandStressCriteria& other) const
{
   bool bPSAtJacking = true;
   if (bCheckStrandStress[+CheckStage::AtJacking] != other.bCheckStrandStress[+CheckStage::AtJacking] ||
      (bCheckStrandStress[+CheckStage::AtJacking] == true && (!::IsEqual(StrandStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved], other.StrandStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved]) ||
         !::IsEqual(StrandStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation], other.StrandStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation]))))
   {
      bPSAtJacking = false;
   }

   bool bPSBeforeXfer = true;
   if (bCheckStrandStress[+CheckStage::BeforeTransfer] != other.bCheckStrandStress[+CheckStage::BeforeTransfer] ||
      (bCheckStrandStress[+CheckStage::BeforeTransfer] == true && (!::IsEqual(StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::StressRelieved], other.StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::StressRelieved]) ||
         !::IsEqual(StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::LowRelaxation], other.StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::LowRelaxation]))))
   {
      bPSBeforeXfer = false;
   }

   bool bPSAfterXfer = true;
   if (bCheckStrandStress[+CheckStage::AfterTransfer] != other.bCheckStrandStress[+CheckStage::AfterTransfer] ||
      (bCheckStrandStress[+CheckStage::AfterTransfer] == true && (!::IsEqual(StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::StressRelieved], other.StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::StressRelieved]) ||
         !::IsEqual(StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::LowRelaxation], other.StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::LowRelaxation]))))
   {
      bPSAfterXfer = false;
   }

   bool bPSFinal = true;
   if (bCheckStrandStress[+CheckStage::AfterAllLosses] != other.bCheckStrandStress[+CheckStage::AfterAllLosses] ||
      (bCheckStrandStress[+CheckStage::AfterAllLosses] == true && (!::IsEqual(StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved], other.StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved]) ||
         !::IsEqual(StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation], other.StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation]))))
   {
      bPSFinal = false;
   }

   return bPSAtJacking and bPSBeforeXfer and bPSAfterXfer and bPSFinal;
}

bool StrandStressCriteria::operator!=(const StrandStressCriteria& other) const
{
   return !operator==(other);
}

bool StrandStressCriteria::Compare(const StrandStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if(operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Stress Limits for Prestressing are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void StrandStressCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stress Limits for Prestressing Strands") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (CheckStrandStress(CheckStage::AtJacking))
   {
      *pPara << _T("Stress Limit at Jacking") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << GetStrandStressCoefficient(CheckStage::AtJacking, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << GetStrandStressCoefficient(CheckStage::AtJacking, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << rptNewLine;
   }

   if (CheckStrandStress(CheckStage::BeforeTransfer))
   {
      *pPara << _T("Stress Limit Immediately Prior to Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::BeforeTransfer, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::BeforeTransfer, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << rptNewLine;
   }

   if (CheckStrandStress(CheckStage::AfterTransfer))
   {
      *pPara << _T("Stress Limit Immediately After Transfer") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterTransfer, StrandType::StressRelieved) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterTransfer, StrandType::LowRelaxation) << RPT_STRESS(_T("pu")) << rptNewLine;
      *pPara << rptNewLine;
   }

   if (CheckStrandStress(CheckStage::AfterAllLosses))
   {
      *pPara << rptNewLine;
      *pPara << _T("Stress Limit at service limit state after all losses") << rptNewLine;
      *pPara << _T("- Stress Relieved Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterAllLosses, StrandType::StressRelieved) << RPT_STRESS(_T("py")) << rptNewLine;
      *pPara << _T("- Low Relaxation Strand = ") << GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterAllLosses, StrandType::LowRelaxation) << RPT_STRESS(_T("py")) << rptNewLine;
      *pPara << rptNewLine;
   }
}

void StrandStressCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("StrandStressCriteria"), 1.0);
   pSave->Property(_T("CheckStrandStressAtJacking"), bCheckStrandStress[+CheckStage::AtJacking]);
   pSave->Property(_T("Coeff_AtJacking_StressRel"), StrandStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"), StrandStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation]);

   pSave->Property(_T("CheckStrandStressBeforeTransfer"), bCheckStrandStress[+CheckStage::BeforeTransfer]);
   pSave->Property(_T("Coeff_BeforeTransfer_StressRel"), StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_BeforeTransfer_LowRelax"), StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::LowRelaxation]);

   pSave->Property(_T("CheckStrandStressAfterTransfer"), bCheckStrandStress[+CheckStage::AfterTransfer]);
   pSave->Property(_T("Coeff_AfterTransfer_StressRel"), StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AfterTransfer_LowRelax"), StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::LowRelaxation]);

   pSave->Property(_T("CheckStrandStressAfterAllLosses"), bCheckStrandStress[+CheckStage::AfterAllLosses]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"), StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"), StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation]);
   pSave->EndUnit();
}

void StrandStressCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("StrandStressCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CheckStrandStressAtJacking"), &bCheckStrandStress[+CheckStage::AtJacking])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"), &StrandStressCoeff[+CheckStage::AtJacking][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"), &StrandStressCoeff[+CheckStage::AtJacking][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);


   if (!pLoad->Property(_T("CheckStrandStressBeforeTransfer"), &bCheckStrandStress[+CheckStage::BeforeTransfer])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_BeforeTransfer_StressRel"), &StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_BeforeTransfer_LowRelax"), &StrandStressCoeff[+CheckStage::BeforeTransfer][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);


   if (!pLoad->Property(_T("CheckStrandStressAfterTransfer"), &bCheckStrandStress[+CheckStage::AfterTransfer])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AfterTransfer_StressRel"), &StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AfterTransfer_LowRelax"), &StrandStressCoeff[+CheckStage::AfterTransfer][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);


   if (!pLoad->Property(_T("CheckStrandStressAfterAllLosses"), &bCheckStrandStress[+CheckStage::AfterAllLosses])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"), &StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::StressRelieved])) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"), &StrandStressCoeff[+CheckStage::AfterAllLosses][+StrandType::LowRelaxation])) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

bool StrandStressCriteria::CheckStrandStress(CheckStage stage) const
{
   return bCheckStrandStress[+stage];
}

Float64 StrandStressCriteria::GetStrandStressCoefficient(CheckStage stage, StrandType strandType) const
{
   return StrandStressCoeff[+stage][+strandType];
}
