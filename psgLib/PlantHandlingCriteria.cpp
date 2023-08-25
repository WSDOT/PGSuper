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
#include <psgLib\PlantHandlingCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool PlantHandlingCriteria::operator==(const PlantHandlingCriteria& other) const
{
   return !operator!=(other);
}

bool PlantHandlingCriteria::operator!=(const PlantHandlingCriteria& other) const
{
   return bCheck != other.bCheck or !::IsEqual(WeightLimit, other.WeightLimit);
}

bool PlantHandlingCriteria::Compare(const PlantHandlingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Handling Weight Limit requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void PlantHandlingCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Plant Handling Weight Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   
   if (bCheck)
   {
      *pPara << _T("Maximum girder weight for plant handling = ") << force.SetValue(WeightLimit) << rptNewLine;
   }
   else
   {
      *pPara << _T("Maximum girder weight for plant handling is not checked") << rptNewLine;
   }
}

void PlantHandlingCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("PlantHandlingCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("WeightLimit"), WeightLimit);
   pSave->EndUnit();
}

void PlantHandlingCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("PlantHandlingCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("WeightLimit"), &WeightLimit)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

