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
#include <psgLib\GirderInclinationCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool GirderInclinationCriteria::operator==(const GirderInclinationCriteria& other) const
{
   return !operator!=(other);
}

bool GirderInclinationCriteria::operator!=(const GirderInclinationCriteria& other) const
{
   return bCheck != other.bCheck or !::IsEqual(FS, other.FS);
}

bool GirderInclinationCriteria::Compare(const GirderInclinationCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Inclined Girder Check Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void GirderInclinationCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   if (bCheck)
   {
      *pPara << _T("Factor of safety: ") << FS << rptNewLine;
   }
   else
   {
      *pPara << _T("Girder inclination not checked") << rptNewLine;
   }
}

void GirderInclinationCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("GirderInclinationCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("FS"), FS);
   pSave->EndUnit();
}

void GirderInclinationCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("GirderInclinationCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("FS"), &FS)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
