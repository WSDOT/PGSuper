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
#include <psgLib\DuctSizeCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

bool DuctSizeCriteria::operator==(const DuctSizeCriteria& other) const
{
   return !operator!=(other);
}

bool DuctSizeCriteria::operator!=(const DuctSizeCriteria& other) const
{
   return !::IsEqual(DuctAreaPushRatio, other.DuctAreaPushRatio) or
          !::IsEqual(DuctAreaPullRatio, other.DuctAreaPullRatio) or
          !::IsEqual(DuctDiameterRatio, other.DuctDiameterRatio);
}

bool DuctSizeCriteria::Compare(const DuctSizeCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if(operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Size of Ducts parameters are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void DuctSizeCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Size of Ducts (LRFD 5.4.6.2)") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   
   *pPara << _T("Maximum ratio of inside area of duct to net area of prestressing steel:") << rptNewLine;
   *pPara << DuctAreaPushRatio << _T(" for strands placed by the push method") << rptNewLine;
   *pPara << DuctAreaPullRatio << _T(" for strands placed by the pull method") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Nominal diameter of ducts shall not exceed ") << DuctDiameterRatio << _T(" times the least gross concrete thickness at the duct") << rptNewLine;
}

void DuctSizeCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("DuctSizeCriteria"), 1.0);
   pSave->Property(_T("DuctAreaPushRatio"), DuctAreaPushRatio);
   pSave->Property(_T("DuctAreaPullRatio"), DuctAreaPullRatio);
   pSave->Property(_T("DuctDiameterRatio"), DuctDiameterRatio);
   pSave->EndUnit();
}

void DuctSizeCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("DuctSizeCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("DuctAreaPushRatio"), &DuctAreaPushRatio)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DuctAreaPullRatio"), &DuctAreaPullRatio)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DuctDiameterRatio"), &DuctDiameterRatio)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
