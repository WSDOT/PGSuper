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

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;

struct PSGLIBCLASS HaunchCriteria
{
   pgsTypes::HaunchLoadComputationType HaunchLoadComputationType = pgsTypes::hlcZeroCamber;
   Float64 HaunchLoadCamberTolerance = WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Inch); ///< This value is only used if HaunchLoadComputationType==hspAccountForCamber or HaunchAnalysisSectionPropertiesType==hspDetailedDescription

   // This value is only used if HaunchLoadComputationType == hlcDetailedAnalysis && slab offset input
   // Valid values are 0.0< to <=1.0
   Float64 HaunchLoadCamberFactor = 1.0;

   pgsTypes::HaunchAnalysisSectionPropertiesType HaunchAnalysisSectionPropertiesType = pgsTypes::hspZeroHaunch;

   bool Compare(const HaunchCriteria& other, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};
