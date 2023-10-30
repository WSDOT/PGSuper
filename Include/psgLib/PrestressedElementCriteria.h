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
#include "TensionStressLimit.h"

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS PrestressedElementCriteria
{
   Float64 CompressionStressCoefficient_BeforeLosses = 0.45;
   //TensionStressLimit TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses; // this case is explicitly defined as N/A in Table 5.9.2.3.1b-1
   TensionStressLimit TensionStressLimit_WithReinforcement_BeforeLosses{ WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI),false,0.0 }; // Table 5.9.2.3.1b-1 does not consider PTZ for this requirement
   TensionStressLimit TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses{ WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI),true,WBFL::Units::ConvertToSysUnits(0.200,WBFL::Units::Measure::KSI)};
   //TensionStressLimit TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses; // this case is not defined in Table 5.9.2.3.1b-1

   Float64 CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses = 0.60;
   Float64 CompressionStressCoefficient_AllLoads_AfterLosses = 0.60;

   // This is an optional tension check, not in the LRFD, but used by TxDOT and CalTrans (zero tension under dead load only)
   bool bCheckFinalServiceITension = false; // false is consistent with the original features of the program (this feature didn't originally exists, so defaulting to false makes old files unchanged)
   TensionStressLimit TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses{ 0.0,false,0.0 };

   TensionStressLimit TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses{ 0.0,false,WBFL::Units::ConvertToSysUnits(0.200,WBFL::Units::Measure::KSI) };
   TensionStressLimit TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses{ 0.0,false,WBFL::Units::ConvertToSysUnits(02300,WBFL::Units::Measure::KSI) };

   Float64 CompressionStressCoefficient_Fatigue = 0.40;

   // PGSuper-only stress limits for temporary loading conditions
   bool bCheckTemporaryStresses = true; // true is consistent with the original default value
   Float64 CompressionStressCoefficient_TemporaryStrandRemoval = 0.45;
   TensionStressLimit TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval{ WBFL::Units::ConvertToSysUnits(0.19, WBFL::Units::Measure::SqrtKSI), false, WBFL::Units::ConvertToSysUnits(0.2, WBFL::Units::Measure::KSI) };
   TensionStressLimit TensionStressLimit_WithReinforcement_TemporaryStrandRemoval{ WBFL::Units::ConvertToSysUnits(0.24, WBFL::Units::Measure::SqrtKSI), false, WBFL::Units::ConvertToSysUnits(0.2, WBFL::Units::Measure::KSI) };

   Float64 CompressionStressCoefficient_AfterDeckPlacement = 0.6;
   TensionStressLimit TensionStressLimit_AfterDeckPlacement{0.0,false,WBFL::Units::ConvertToSysUnits(0.2, WBFL::Units::Measure::KSI) };

   bool Compare(const PrestressedElementCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};
