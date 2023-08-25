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
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

bool SectionPropertiesCriteria::Compare(const SectionPropertiesCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (SectionPropertyMode != other.SectionPropertyMode || EffectiveFlangeWidthMethod != other.EffectiveFlangeWidthMethod)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Section Properties are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void SectionPropertiesCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
    
   if (SectionPropertyMode == pgsTypes::spmGross)
   {
      *pPara << Bold(_T("Section Properties: ")) << _T("Gross") << rptNewLine;
   }
   else
   {
      *pPara << Bold(_T("Section Properties: ")) << _T("Transformed") << rptNewLine;
   }

   pPara = new rptParagraph;
   *pChapter << pPara;
   if (EffectiveFlangeWidthMethod == pgsTypes::efwmLRFD)
   {
      *pPara << Bold(_T("Effective Flange Width: ")) << _T("computed in accordance with LRFD 4.6.2.6") << rptNewLine;
   }
   else
   {
      *pPara << Bold(_T("Effective Flange Width: ")) << _T("computed using tributary width") << rptNewLine;
   }


}

void SectionPropertiesCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("SectionPropertiesCriteria"), 1.0);
   pSave->Property(_T("SectionPropertyMode"), SectionPropertyMode);
   pSave->Property(_T("EffectiveFlangeWidthMethod"), EffectiveFlangeWidthMethod);
   pSave->EndUnit();
}

void SectionPropertiesCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("SectionPropertiesCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   int value;
   if (!pLoad->Property(_T("SectionPropertyMode"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   SectionPropertyMode = (pgsTypes::SectionPropertyMode)value;

   if (!pLoad->Property(_T("EffectiveFlangeWidthMethod"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   EffectiveFlangeWidthMethod = (pgsTypes::EffectiveFlangeWidthMethod)value;

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
