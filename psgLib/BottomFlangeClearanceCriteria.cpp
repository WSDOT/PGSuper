///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <psgLib\BottomFlangeClearanceCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool BottomFlangeClearanceCriteria::operator==(const BottomFlangeClearanceCriteria& other) const
{
   return !operator!=(other);
}

bool BottomFlangeClearanceCriteria::operator!=(const BottomFlangeClearanceCriteria& other) const
{
   return bCheck != other.bCheck or !::IsEqual(MinClearance, other.MinClearance);
}

bool BottomFlangeClearanceCriteria::Compare(const BottomFlangeClearanceCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Bottom Flange Clearance Check Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void BottomFlangeClearanceCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if (bCheck)
   {
      *pPara << _T("Minimum bottom flange clearance: ") << dim.SetValue(MinClearance) << rptNewLine;
   }
   else
   {
      *pPara << _T("Bottom flange clearance not checked") << rptNewLine;
   }
}

void BottomFlangeClearanceCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("BottomFlangeClearanceCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("MinClearance"), MinClearance);
   pSave->EndUnit();
}

void BottomFlangeClearanceCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("BottomFlangeClearanceCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MinClearance"), &MinClearance)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
