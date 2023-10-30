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
#include <Materials/PsStrand.h>
class pgsLibraryEntryDifferenceItem;

class rptChapter;
interface IEAFDisplayUnits;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS StrandSlopeCriteria
{
   bool    bCheck = false;
   bool    bDesign = false;
   Float64 MaxSlope05 = 6;  /// < max strand slope for 0.5" diameter strands
   Float64 MaxSlope06 = 8;  /// < max strand slope for 0.6" diameter strands
   Float64 MaxSlope07 = 10; /// < max strand slope for 0.7" diameter strands

   bool operator==(const StrandSlopeCriteria& other) const;
   bool operator!=(const StrandSlopeCriteria& other) const;
   bool Compare(const StrandSlopeCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   Float64 GetStrandSlopeLimit(WBFL::Materials::PsStrand::Size strandSize) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

