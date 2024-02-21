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
#include <psgLib/LimitStateConcreteStrengthCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
#include <EAF/EAFDisplayUnits.h>

bool LimitStateConcreteStrengthCriteria::Compare(const LimitStateConcreteStrengthCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (LimitStateConcreteStrength != other.LimitStateConcreteStrength ||
      bUse90DayConcreteStrength != other.bUse90DayConcreteStrength ||
      !::IsEqual(SlowCuringConcreteStrengthFactor, other.SlowCuringConcreteStrengthFactor))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Concrete Strength for Limit State Evaluations are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void LimitStateConcreteStrengthCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Concrete Strength for Limit State Evaluations") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   
   switch (LimitStateConcreteStrength)
   {
   case pgsTypes::lscStrengthAtTimeOfLoading:
      *pPara << _T("Use ") << RPT_FC << _T(" at the time of loading (LRFD 5.14.1.3.3, 5.14.3.2d)") << rptNewLine;
      break;

   case pgsTypes::lscSpecifiedStrength:
      *pPara << _T("Use specified ") << RPT_FCI << _T(" and ") << RPT_FC << rptNewLine;
      if (bUse90DayConcreteStrength)
         *pPara << _T("Use ") << percentage.SetValue(SlowCuringConcreteStrengthFactor) << _T(" of ") << RPT_FC << _T(" for stress combinations after 90 days for slow curing concretes (LRFD 5.12.3.2.5)") << rptNewLine;
      break;
   default:
      CHECK(false); // should never get here
   }
   
}

void LimitStateConcreteStrengthCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("LimitStateConcreteStrengthCriteria"), 1.0);

   pSave->Property(_T("LimitStateConcreteStrength"), LimitStateConcreteStrength);
   pSave->Property(_T("Use90DayConcreteStrength"), bUse90DayConcreteStrength);
   pSave->Property(_T("SlowCuringConcreteStrengthFactor"), SlowCuringConcreteStrengthFactor);

   pSave->EndUnit(); // LimitStateConcreteStrengthCriteria
}

void LimitStateConcreteStrengthCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("LimitStateConcreteStrengthCriteria"))) THROW_LOAD(InvalidFileFormat, pLoad);

   long value;
   if (!pLoad->Property(_T("LimitStateConcreteStrength"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   LimitStateConcreteStrength = (pgsTypes::LimitStateConcreteStrength)value;

   if(!pLoad->Property(_T("Use90DayConcreteStrength"), &bUse90DayConcreteStrength)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SlowCuringConcreteStrengthFactor"), &SlowCuringConcreteStrengthFactor)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad); // LimitStateConcreteStrengthCriteria
}
