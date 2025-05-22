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
#include <PsgLib\StrandSlopeCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <PsgLib/DifferenceItem.h>

bool StrandSlopeCriteria::operator==(const StrandSlopeCriteria& other) const
{
   return !operator!=(other);
}

bool StrandSlopeCriteria::operator!=(const StrandSlopeCriteria& other) const
{
   return bCheck != other.bCheck or
      bDesign != other.bDesign or
      !::IsEqual(MaxSlope05, other.MaxSlope05) or
      !::IsEqual(MaxSlope06, other.MaxSlope06) or
      !::IsEqual(MaxSlope07, other.MaxSlope07);
}

bool StrandSlopeCriteria::Compare(const StrandSlopeCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Strand Slope requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

Float64 StrandSlopeCriteria::GetStrandSlopeLimit(WBFL::Materials::PsStrand::Size strandSize) const
{
   Float64 strand_slope_limit = 0.0;
   if (strandSize == WBFL::Materials::PsStrand::Size::D1778)
   {
      strand_slope_limit = MaxSlope07;
   }
   else if (strandSize == WBFL::Materials::PsStrand::Size::D1524)
   {
      strand_slope_limit = MaxSlope06;
   }
   else
   {
      strand_slope_limit = MaxSlope05;
   }

   return strand_slope_limit;
}

void StrandSlopeCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Harped Strand Slope Limits") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   
   if (bCheck)
   {
      *pPara << _T("Max. slope for 0.5\" ") << symbol(phi) << _T(" strands = 1:") << MaxSlope05 << rptNewLine;
      *pPara << _T("Max. slope for 0.6\" ") << symbol(phi) << _T(" strands = 1:") << MaxSlope06 << rptNewLine;
      *pPara << _T("Max. slope for 0.7\" ") << symbol(phi) << _T(" strands = 1:") << MaxSlope07 << rptNewLine;
   }
   else
   {
      *pPara << _T("Max. Strand slope is not checked") << rptNewLine;
   }
}

void StrandSlopeCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("StrandSlopeCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("Design"), bDesign);
   pSave->Property(_T("MaxSlope05"), MaxSlope05);
   pSave->Property(_T("MaxSlope06"), MaxSlope06);
   pSave->Property(_T("MaxSlope07"), MaxSlope07);
   pSave->EndUnit();
}

void StrandSlopeCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("StrandSlopeCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Design"), &bDesign)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("MaxSlope05"), &MaxSlope05)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxSlope06"), &MaxSlope06)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MaxSlope07"), &MaxSlope07)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

