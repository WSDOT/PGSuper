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

#pragma once


#include "psgLibLib.h"
#include <array>

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS TendonStressCriteria
{
   enum class CheckStage
   {
      AtJacking = 0,
      PriorToSeating = 1,
      AtAnchoragesAfterSeating = 2,
      AfterAllLosses = 3,
      ElsewhereAfterSeating = 4,
   };

   enum class StrandType
   {
      StressRelieved,
      LowRelaxation
   };

   bool bCheckAtJacking = false;
   bool bCheckPriorToSeating = true;
   std::array<std::array<Float64, 2>, 5> TendonStressCoeff; // Access array with CheckStage and StrandType enums

   TendonStressCriteria();
   bool operator==(const TendonStressCriteria& other) const;
   bool operator!=(const TendonStressCriteria& other) const;
   bool Compare(const TendonStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   Float64 GetTendonStressCoefficient(CheckStage stage, StrandType strandType) const;
};

inline constexpr auto operator+(TendonStressCriteria::CheckStage a) noexcept { return std::underlying_type<TendonStressCriteria::CheckStage>::type(a); }
inline constexpr auto operator+(TendonStressCriteria::StrandType a) noexcept { return std::underlying_type<TendonStressCriteria::StrandType>::type(a); }
