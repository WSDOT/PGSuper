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
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>
#include <boost\algorithm\string\replace.hpp>

bool SpecificationCriteria::Compare(const SpecificationCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;

   if (Description != other.Description)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Description"), Description.c_str(), other.Description.c_str()));
      if (bReturnOnFirstDifference) return false;
   }

   if (bUseCurrentSpecification != other.bUseCurrentSpecification || Edition != other.Edition)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Basis is different"), WBFL::LRFD::LRFDVersionMgr::GetVersionString(Edition), WBFL::LRFD::LRFDVersionMgr::GetVersionString(other.Edition)));
      if (bReturnOnFirstDifference) return false;
   }

   if (WBFL::LRFD::LRFDVersionMgr::Version::ThirdEditionWith2006Interims < Edition && Units != other.Units)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Specification Units Systems are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void SpecificationCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << Bold(_T("Description: ")) << GetDescription() << rptNewLine;
   *pPara << Bold(_T("Based on: ")) << WBFL::LRFD::LRFDVersionMgr::GetCodeString() << _T(", ") << WBFL::LRFD::LRFDVersionMgr::GetVersionString();

   if (Units == WBFL::LRFD::LRFDVersionMgr::Units::SI)
   {
      *pPara << _T(" - SI Units") << rptNewLine;
   }
   else
   {
      *pPara << _T(" - US Units") << rptNewLine;
   }
}

void SpecificationCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("SpecificationCriteria"), 1.0);

   pSave->Property(_T("Description"), Description.c_str());

   pSave->Property(_T("UseCurrentSpecification"), bUseCurrentSpecification);
   pSave->Property(_T("Edition"), WBFL::LRFD::LRFDVersionMgr::GetVersionString(Edition, true));

   if (Units == WBFL::LRFD::LRFDVersionMgr::Units::SI)
   {
      pSave->Property(_T("SpecificationUnits"), _T("SI"));
   }
   else if (Units == WBFL::LRFD::LRFDVersionMgr::Units::US)
   {
      pSave->Property(_T("SpecificationUnits"), _T("US"));
   }
   else
   {
      CHECK(false); // should never get here
   }

   pSave->EndUnit();
}

void SpecificationCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("SpecificationCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("Description"), &Description)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("UseCurrentSpecification"), &bUseCurrentSpecification)) THROW_LOAD(InvalidFileFormat, pLoad);

   std::_tstring tmp;
   if (pLoad->Property(_T("Edition"), &tmp))
   {
      try
      {
         Edition = WBFL::LRFD::LRFDVersionMgr::GetVersion(tmp.c_str());
      }
      catch (...)
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }


   if (pLoad->Property(_T("SpecificationUnits"), &tmp))
   {
      if (tmp == _T("SI"))
      {
         Units = WBFL::LRFD::LRFDVersionMgr::Units::SI;
      }
      else if (tmp == _T("US"))
      {
         Units = WBFL::LRFD::LRFDVersionMgr::Units::US;
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }


   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}

std::_tstring SpecificationCriteria::GetDescription(bool bApplySymbolSubstitution) const
{
   if (bApplySymbolSubstitution)
   {
      std::_tstring description(Description);
      std::_tstring strSubstitute(WBFL::LRFD::LRFDVersionMgr::GetCodeString());
      strSubstitute += _T(", ");
      strSubstitute += WBFL::LRFD::LRFDVersionMgr::GetVersionString();
      boost::replace_all(description, _T("%BDS%"), strSubstitute);
      return description;
   }
   else
   {
      return Description;
   }
}

WBFL::LRFD::LRFDVersionMgr::Version SpecificationCriteria::GetEdition() const
{
   if (bUseCurrentSpecification)
   {
      return WBFL::LRFD::LRFDVersionMgr::GetLatestVersion();
   }
   else
   {
      return Edition;
   }
}
