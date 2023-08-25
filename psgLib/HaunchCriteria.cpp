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
#include <psgLib/HaunchCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool HaunchCriteria::Compare(const HaunchCriteria& other, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (HaunchLoadComputationType != other.HaunchLoadComputationType ||
      (HaunchLoadComputationType == pgsTypes::hlcDetailedAnalysis && !::IsEqual(HaunchLoadCamberTolerance, other.HaunchLoadCamberTolerance)))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Haunch Loads are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (HaunchLoadComputationType == pgsTypes::hlcDetailedAnalysis && !::IsEqual(HaunchLoadCamberFactor, other.HaunchLoadCamberFactor))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Haunch Loads Camber Factors are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (HaunchAnalysisSectionPropertiesType != other.HaunchAnalysisSectionPropertiesType ||
      (HaunchAnalysisSectionPropertiesType == pgsTypes::hspDetailedDescription && !::IsEqual(HaunchLoadCamberTolerance, other.HaunchLoadCamberTolerance)))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Method using haunch geometry to compute composite section properties is different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void HaunchCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Haunch And Assumed Excess Camber Criteria");

   pPara = new rptParagraph;
   *pChapter << pPara;

   // Loading first
   if (HaunchLoadComputationType == pgsTypes::hlcZeroCamber)
   {
      *pPara << _T("Haunch dead load is computed assuming that the top of the girder is flat (Zero assumed excess camber)") << rptNewLine;
   }
   else if (HaunchLoadComputationType == pgsTypes::hlcDetailedAnalysis)
   {
      *pPara << _T("Haunch dead load is computed using detailed input. When explicit haunch depths are input directly, dead load is based on those depths. When haunch depth is defined by slab offets and assumed camber, depths along girder are computed assuming that the top of the girder is parabolic with the parabola defined by the user-input assumed excess camber.") << rptNewLine;
      *pPara << _T("- Use ") << HaunchLoadCamberFactor * 100 << _T(" % of assumed excess camber when computing haunch dead load.") << rptNewLine;
   }
   else
   {
      CHECK(false); // new method?
   }

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (HaunchAnalysisSectionPropertiesType == pgsTypes::hspZeroHaunch)
   {
      *pPara << _T("Composite section properties and capacities are computed ignoring the haunch depth)") << rptNewLine;
   }
   else if (HaunchAnalysisSectionPropertiesType == pgsTypes::hspConstFilletDepth)
   {
      *pPara << _T("Composite section properties and capacities are computed assuming a Constant Haunch Depth equal to the Fillet value") << rptNewLine;
   }
   else if (HaunchAnalysisSectionPropertiesType == pgsTypes::hspDetailedDescription)
   {
      *pPara << _T("Composite section properties and capacities are computed using detailed input. When explicit haunch depths are input directly, dead load is based on those depths. When haunch depth is defined by slab offets and assumed camber, assume a Parabolically varying Haunch Depth defined by the roadway geometry and assumed excess camber ") << rptNewLine;
   }
   else
   {
      CHECK(false); // new method?
   }

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Allowable tolerance between assumed and computed excess camber = ") << dim.SetValue(HaunchLoadCamberTolerance) << rptNewLine;
}

void HaunchCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("HaunchLoadCriteria"), 1.0);
   pSave->Property(_T("HaunchLoadComputationType"), HaunchLoadComputationType);
   pSave->Property(_T("HaunchLoadCamberTolerance"), HaunchLoadCamberTolerance);
   pSave->Property(_T("HaunchLoadCamberFactor"), HaunchLoadCamberFactor);
   pSave->Property(_T("HaunchAnalysisSectionPropertiesType"), HaunchAnalysisSectionPropertiesType);
   pSave->EndUnit();
}

void HaunchCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("HaunchLoadCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   int value;
   if (!pLoad->Property(_T("HaunchLoadComputationType"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   HaunchLoadComputationType = (pgsTypes::HaunchLoadComputationType)value;

   if (!pLoad->Property(_T("HaunchLoadCamberTolerance"), &HaunchLoadCamberTolerance)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("HaunchLoadCamberFactor"), &HaunchLoadCamberFactor)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("HaunchAnalysisSectionPropertiesType"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   HaunchAnalysisSectionPropertiesType = (pgsTypes::HaunchAnalysisSectionPropertiesType)value;

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
