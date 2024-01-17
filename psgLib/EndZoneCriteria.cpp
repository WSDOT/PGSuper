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
#include <psgLib\EndZoneCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool EndZoneCriteria::operator==(const EndZoneCriteria& other) const
{
   return !operator!=(other);
}

bool EndZoneCriteria::operator!=(const EndZoneCriteria& other) const
{
   return bCheckConfinement != other.bCheckConfinement or
      bDesignConfinement != other.bDesignConfinement or
      bCheckSplitting != other.bCheckSplitting or
      bDesignSplitting != other.bDesignSplitting or 
      !::IsEqual(SplittingZoneLengthFactor, other.SplittingZoneLengthFactor);
}

bool EndZoneCriteria::Compare(const EndZoneCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Splitting and Confinement requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void EndZoneCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("End Zone (Splitting and Confinement) Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (bCheckSplitting)
   {
      if (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= WBFL::LRFD::BDSManager::GetEdition())
      {
         *pPara << _T("Splitting zone length: h/") << SplittingZoneLengthFactor << _T(" (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) << _T(")") << rptNewLine;
      }
      else
      {
         *pPara << _T("Bursting zone length: h/") << SplittingZoneLengthFactor << _T(" (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) << _T(")") << rptNewLine;
      }
   }
   else
   {
      if (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= WBFL::LRFD::BDSManager::GetEdition())
      {
         *pPara << _T("Splitting checks (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) << _T(") are disabled.") << rptNewLine;
      }
      else
      {
         *pPara << _T("Bursting checks (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) << _T(") are disabled.") << rptNewLine;
      }
   }

   if (bCheckConfinement)
   {
      *pPara << _T("Confinement checks (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.2"), _T("5.9.4.4.2")) << _T(") are enabled.") << rptNewLine;
   }
   else
   {
      *pPara << _T("Confinement checks (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.10.10.2"), _T("5.9.4.4.2")) << _T(") are disabled.") << rptNewLine;
   }
}

void EndZoneCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("EndZoneCriteria"), 1.0);
   pSave->Property(_T("CheckSplitting"), bCheckSplitting);
   pSave->Property(_T("DesignSplitting"), bDesignSplitting);
   pSave->Property(_T("SplittingZoneLengthFactor"), SplittingZoneLengthFactor);
   pSave->Property(_T("CheckConfinement"), bCheckConfinement);
   pSave->Property(_T("DesignConfinement"), bDesignConfinement);
   pSave->EndUnit();
}

void EndZoneCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("EndZoneCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("CheckSplitting"), &bCheckSplitting)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DesignSplitting"), &bDesignSplitting)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("SplittingZoneLengthFactor"), &SplittingZoneLengthFactor)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("CheckConfinement"), &bCheckConfinement)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("DesignConfinement"), &bDesignConfinement)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
