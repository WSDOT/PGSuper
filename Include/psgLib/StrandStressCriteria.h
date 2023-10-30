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

#pragma once


#include "psgLibLib.h"
#include <array>

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS StrandStressCriteria
{
   enum class CheckStage
   {
      AtJacking = 0,
      BeforeTransfer = 1,
      AfterTransfer = 2,
      AfterAllLosses = 3
   };

   enum class StrandType
   {
      StressRelieved,
      LowRelaxation
   };

   std::array<bool, 4> bCheckStrandStress; // Access array with CheckStage enum
   std::array<std::array<Float64, 2>, 4> StrandStressCoeff; // Access array with CheckStage and StrandType enums

   StrandStressCriteria();
   bool operator==(const StrandStressCriteria& other) const;
   bool operator!=(const StrandStressCriteria& other) const;
   bool Compare(const StrandStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   bool CheckStrandStress(CheckStage stage) const;
   Float64 GetStrandStressCoefficient(CheckStage stage, StrandType strandType) const;
};

inline constexpr auto operator+(StrandStressCriteria::CheckStage a) noexcept { return std::underlying_type<StrandStressCriteria::CheckStage>::type(a); }
inline constexpr auto operator+(StrandStressCriteria::StrandType a) noexcept { return std::underlying_type<StrandStressCriteria::StrandType>::type(a); }
