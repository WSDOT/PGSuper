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
#include <psgLib/HarpedStrandDesignCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool HarpedStrandDesignCriteria::Compare(const HarpedStrandDesignCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (StrandFillType != other.StrandFillType)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Harped Strand Design Strategies are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void HarpedStrandDesignCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Harped Strand Design Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Harped strands design") << rptNewLine;
   if (StrandFillType == ftGridOrder)
   {
      *pPara << _T(" - Add strands using the permanent strand fill order") << rptNewLine;
   }
   else if (StrandFillType == ftMinimizeHarping)
   {
      *pPara << _T(" - Minimize number of harped strands (if necessary, straight strands may be traded for harped strands to control top tension)") << rptNewLine;
   }
   else
   {
      CHECK(StrandFillType == ftDirectFill); // ftDirectFill is the only other option
      CHECK(false); // but it is not a valid option in the spec library entry
   }
}

void HarpedStrandDesignCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("HarpedStrandDesignCriteria"), 1.0);

   pSave->Property(_T("StrandFillType"), (long)StrandFillType);

   pSave->EndUnit(); // HarpedStrandDesignCriteria
}

void HarpedStrandDesignCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("HarpedStrandDesignCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   long value;
   if(!pLoad->Property(_T("StrandFillType"),&value)) THROW_LOAD(InvalidFileFormat, pLoad);
   StrandFillType = (arDesignStrandFillType)value;

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // HarpedStrandDesignCriteria
}
