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

struct PSGLIBCLASS CreepCriteria
{
   Float64 XferTime = WBFL::Units::ConvertToSysUnits(24, WBFL::Units::Measure::Hour);  ///< Used for computing relaxation losses prior to prestress transfer. This is only applicable to non-time step loss methods

   Float64 CreepDuration1Min = WBFL::Units::ConvertToSysUnits(10, WBFL::Units::Measure::Day); ///< Number of days from prestress release until temporary strand removal (or diaphragm loading for structures without temporary strands). This is only applicable to non-time step loss methods
   Float64 CreepDuration1Max = WBFL::Units::ConvertToSysUnits(40, WBFL::Units::Measure::Day);
   Float64 CreepDuration2Min = WBFL::Units::ConvertToSysUnits(90, WBFL::Units::Measure::Day); ///< Number of days from prestress release until the slab is acting composite with the girder. This is only applicable to non-time step loss methods
   Float64 CreepDuration2Max = WBFL::Units::ConvertToSysUnits(120, WBFL::Units::Measure::Day);
   Float64 TotalCreepDuration = WBFL::Units::ConvertToSysUnits(2000, WBFL::Units::Measure::Day); ///< This is only applicable to non-time step loss methods

   Float64 CamberVariability = 0.5; ///< Variability between upper and lower bound camber, stored in decimal percent

   pgsTypes::CuringMethod CuringMethod = pgsTypes::CuringMethod::Accelerated;
   Float64 CuringMethodTimeAdjustmentFactor = 7.0; ///< From LRFD, 1 day of steam curing = 7 days of moist curing

   bool Compare(const CreepCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   Float64 GetCreepDuration1(pgsTypes::CreepTime time) const;
   Float64 GetCreepDuration2(pgsTypes::CreepTime time) const;
};
