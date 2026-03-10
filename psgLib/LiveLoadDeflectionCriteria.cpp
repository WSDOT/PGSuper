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
#include <PsgLib\LiveLoadDeflectionCriteria.h>
#include <PsgLib/DifferenceItem.h>

bool LiveLoadDeflectionCriteria::operator==(const LiveLoadDeflectionCriteria& other) const
{
   return !operator!=(other);
}

bool LiveLoadDeflectionCriteria::operator!=(const LiveLoadDeflectionCriteria& other) const
{
   return bCheck != other.bCheck or !::IsEqual(DeflectionLimit, other.DeflectionLimit);
}

bool LiveLoadDeflectionCriteria::Compare(const LiveLoadDeflectionCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<PGS::Library::DifferenceStringItem>(_T("Live Load Deflection Check Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void LiveLoadDeflectionCriteria::Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Live Load Deflection Criteria") << rptNewLine;
   
   pPara = new rptParagraph;
   *pChapter << pPara;

   if (bCheck)
   {
      *pPara << _T("Live Load Deflection Limit: ") << Sub2(_T("L"), _T("span")) << _T("/") << DeflectionLimit << rptNewLine;
   }
   else
   {
      *pPara << _T("Live Load Deflection Limit not evaluated") << rptNewLine;
   }
}

void LiveLoadDeflectionCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LiveLoadDeflectionCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("DeflectionLimit"), DeflectionLimit);
   pSave->EndUnit();
}

void LiveLoadDeflectionCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LiveLoadDeflectionCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DeflectionLimit"), &DeflectionLimit)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
