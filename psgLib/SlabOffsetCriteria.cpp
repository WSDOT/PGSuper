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
#include <psgLib\SlabOffsetCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool SlabOffsetCriteria::Compare(const SlabOffsetCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (bCheck != other.bCheck or bDesign != other.bDesign or RoundingMethod != other.RoundingMethod or !IsEqual(SlabOffsetTolerance, other.SlabOffsetTolerance))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Slab Offset (\"A\" Dimension) Check/Design Options are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }


   if (!::IsEqual(FinishedElevationTolerance, other.FinishedElevationTolerance))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Finished/Design Elevation Tolerances are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void SlabOffsetCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if (bCheck)
   {
      if (RoundingMethod == pgsTypes::sormRoundNearest)
      {
         *pPara << _T("Haunch Geometry check is enabled and the Required Slab Offset is rounded to the nearest ") << dim.SetValue(SlabOffsetTolerance) << rptNewLine;
      }
      else
      {
         *pPara << _T("Haunch Geometry check is enabled and the Required Slab Offset is rounded up to the nearest ") << dim.SetValue(SlabOffsetTolerance) << rptNewLine;
      }
   }
   else
   {
      *pPara << _T("Haunch Geometry check is disabled.") << rptNewLine;
   }

   *pPara << _T("Finished elevation tolerance for no-deck bridges : ") << dim.SetValue(FinishedElevationTolerance) << rptNewLine;
}

void SlabOffsetCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("SlabOffsetCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("Design"), bDesign);
   pSave->Property(_T("RoundingMethod"), RoundingMethod);
   pSave->Property(_T("Tolerance"), SlabOffsetTolerance);
   pSave->EndUnit();

   pSave->BeginUnit(_T("FinishedElevation"), 1.0);
   pSave->Property(_T("Tolerance"), FinishedElevationTolerance);
   pSave->EndUnit();
}

void SlabOffsetCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("SlabOffsetCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Design"), &bDesign)) THROW_LOAD(InvalidFileFormat, pLoad);

   int method;
   if (!pLoad->Property(_T("RoundingMethod"), &method)) THROW_LOAD(InvalidFileFormat, pLoad);
   RoundingMethod = (pgsTypes::SlabOffsetRoundingMethod)method;

   if (!pLoad->Property(_T("Tolerance"), &SlabOffsetTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // SlabOffsetCriteria

   if (!pLoad->BeginUnit(_T("FinishedElevation"))) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Tolerance"), &FinishedElevationTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); //FinishedElevation
}

Float64 SlabOffsetCriteria::RoundSlabOffset(Float64 slabOffset) const
{
   // Round slab offset using specified method and tolerance
   Float64 rounded_slab_offset = slabOffset;
   if (!::IsZero(SlabOffsetTolerance))
   {
      if (RoundingMethod == pgsTypes::sormRoundNearest)
      {
         rounded_slab_offset = RoundOff(slabOffset, SlabOffsetTolerance);
      }
      else if (RoundingMethod == pgsTypes::sormRoundUp)
      {
         rounded_slab_offset = CeilOff(slabOffset, SlabOffsetTolerance);
      }
      else
      {
         ATLASSERT(false); // new method??
      }
   }

   return rounded_slab_offset;
}
