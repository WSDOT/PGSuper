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

#pragma once


#include "psgLibLib.h"

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS HoldDownCriteria
{
   enum class Type
   {
      Total,
      PerStrand
   };

   bool    bCheck = false;
   bool    bDesign = false;
   Type    type = Type::Total;
   Float64 force_limit = ConvertToSysUnits(45.0, WBFL::Units::Measure::Kip); // maximum hold down force permitted (could be total force or per stand force depending on type)
   Float64 friction = 0.0; // percentage increase in computed hold down force to account for friction. Phold_down = Pjack(slope)(1+friction)

   bool operator==(const HoldDownCriteria& other) const;
   bool operator!=(const HoldDownCriteria& other) const;
   bool Compare(const HoldDownCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

inline constexpr auto operator+(HoldDownCriteria::Type a) noexcept { return std::underlying_type<HoldDownCriteria::Type>::type(a); }

