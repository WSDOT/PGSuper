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

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS PrincipalTensionStressCriteria
{
   pgsTypes::PrincipalTensileStressMethod Method = pgsTypes::ptsmLRFD;
   Float64 Coefficient = WBFL::Units::ConvertToSysUnits(0.110, WBFL::Units::Measure::SqrtKSI); ///< coefficient in front of sqrt(f'c)
   Float64 TendonNearnessFactor = 1.5; ///< used to define if a tendon is "near" the section being evaluated... This is a number of outside duct diameters from section
   Float64 FcThreshold = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI); ///< minimum f'c to trigger principal stress check for non-post tensioned beams. If -1, it means principal tension stress check applies to all f'c
   Float64 UngroutedMultiplier = 1.0; ///< Multiplier * Duct Diameter to be subtracted from web for UNGROUTED ducts, if elevation near duct
   Float64 GroutedMultiplier = 0.0;   ///< Multiplier * Duct Diameter to be subtracted from web for GROUTED ducts, if elevation near duct

   bool operator==(const PrincipalTensionStressCriteria& other) const;
   bool operator!=(const PrincipalTensionStressCriteria& other) const;
   bool Compare(const PrincipalTensionStressCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

