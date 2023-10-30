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
#include <psgLib\HoldDownCriteria.h>
#include <EAF/EAFDisplayUnits.h>
#include <psgLib/LibraryEntryDifferenceItem.h>

bool HoldDownCriteria::operator==(const HoldDownCriteria& other) const
{
   return !operator!=(other);
}

bool HoldDownCriteria::operator!=(const HoldDownCriteria& other) const
{
   return bCheck != other.bCheck or
      bDesign != other.bDesign or
      type != other.type or
      !::IsEqual(force_limit, other.force_limit) or
      !::IsEqual(friction, other.friction);
}

bool HoldDownCriteria::Compare(const HoldDownCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (operator!=(other))
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Hold Down Force requirements are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void HoldDownCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Harped Strand Hold Down Force Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (bCheck)
   {
      if (type == HoldDownCriteria::Type::Total)
      {
         *pPara << _T("Maximum hold down force = ");
      }
      else
      {
         CHECK(type == HoldDownCriteria::Type::PerStrand);
         *pPara << _T("Maximum hold down force per strand = ");
      }
      *pPara << force.SetValue(force_limit) << rptNewLine;
      *pPara << _T("The hold down force includes ") << percentage.SetValue(friction) << _T(" friction") << rptNewLine;
   }
   else
   {
      *pPara << _T("Hold down force is not checked.") << rptNewLine;
   }
}

void HoldDownCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("HoldDownCriteria"), 1.0);
   pSave->Property(_T("Check"), bCheck);
   pSave->Property(_T("Design"), bDesign);
   pSave->Property(_T("ForceType"), +type);
   pSave->Property(_T("ForceLimit"), force_limit);
   pSave->Property(_T("Friction"), friction);
   pSave->EndUnit();
}

void HoldDownCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("HoldDownCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Check"), &bCheck)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Design"), &bDesign)) THROW_LOAD(InvalidFileFormat, pLoad);

   int hdft;
   if (!pLoad->Property(_T("ForceType"), &hdft))
   {
         THROW_LOAD(InvalidFileFormat, pLoad);
   }
   type = (HoldDownCriteria::Type)hdft;

   if (!pLoad->Property(_T("ForceLimit"), &force_limit)) THROW_LOAD(InvalidFileFormat, pLoad);
   if (!pLoad->Property(_T("Friction"), &friction)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

