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


#include "PsgLibLib.h"

class rptChapter;
class IEAFDisplayUnits;
class SpecLibraryEntryImpl;
namespace PGS {namespace Library{class DifferenceItem;};};


struct PSGLIBCLASS PlantHandlingCriteria
{
   bool    bCheck = false;
   Float64 WeightLimit = ConvertToSysUnits(130, WBFL::Units::Measure::Kip);

   bool operator==(const PlantHandlingCriteria& other) const;
   bool operator!=(const PlantHandlingCriteria& other) const;
   bool Compare(const PlantHandlingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

