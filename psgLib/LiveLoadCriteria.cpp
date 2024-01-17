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
#include <psgLib\LiveLoadCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>


bool LiveLoadCriteria::Compare(const LiveLoadCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (bIncludeDualTandem != other.bIncludeDualTandem)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Dual Design Tandem setting is different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (!::IsEqual(MinSidewalkWidth, other.MinSidewalkWidth)
    or
    !::IsEqual(PedestrianLoad, other.PedestrianLoad))
   {
      bSame = false;
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void LiveLoadCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetSidewalkPressureUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Live Load Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Dual design tandem (low boy) is ");
   if (!bIncludeDualTandem) *pPara << _T("not ");
   *pPara << _T(" evaluated (LRFD C3.6.1.1.1)") << rptNewLine;
   
   *pPara << _T("Pedestrian live load: ") << stress.SetValue(PedestrianLoad) << rptNewLine;
   *pPara << _T("Minimum width of sidewalk to apply pedestrian live load: ") << dim.SetValue(MinSidewalkWidth) << rptNewLine;
}

void LiveLoadCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LiveLoadCriteria"), 1.0);
   pSave->Property(_T("bIncludeDualTandem"), bIncludeDualTandem);
   pSave->Property(_T("MinSidewalkWidth"), MinSidewalkWidth);
   pSave->Property(_T("PedestrianLoad"), PedestrianLoad);
   pSave->EndUnit();
}

void LiveLoadCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LiveLoadCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("bIncludeDualTandem"), &bIncludeDualTandem)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("MinSidewalkWidth"), &MinSidewalkWidth)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("PedestrianLoad"), &PedestrianLoad)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
