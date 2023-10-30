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

struct PSGLIBCLASS ClosureJointCriteria
{
   Float64 CompressionStressCoefficient_BeforeLosses = 0.60;
   TensionStressLimit TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses{ 0.0,false,0.0 };
   TensionStressLimit TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses{ WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI),false,0.0 };
   TensionStressLimit TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses{ 0.0,false,0.0 };
   TensionStressLimit TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses{ WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI),false,0.0 };

   Float64 CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses = 0.45;
   Float64 CompressionStressCoefficient_AllLoads_AfterLosses = 0.60;

   // This is an optional tension check, not in the LRFD, but used by TxDOT and CalTrans (zero tension under dead load only)
   // Note that the bool below appears to be independent of the same in PrestressedElementCriteria, but it is forced to be synchronized by the UI. 
   // Do not make this independent unless you are willing to update creation of the new limit state in the specagent
   bool bCheckFinalServiceITension = false; // false is consistent with the original features of the program (this feature didn't originally exists, so defaulting to false makes old files unchanged)
   TensionStressLimit TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses{ 0.0,false,0.0 };

   TensionStressLimit TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses{ 0.0,false,0.0 };
   TensionStressLimit TensionStressLimit_InPTZ_WithReinforcement_AfterLosses{ WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI),true,  WBFL::Units::ConvertToSysUnits(0.30,WBFL::Units::Measure::KSI) };
   TensionStressLimit TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses{ 0.0,false,0.0 };
   TensionStressLimit TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses{ WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI),false,0.0 };

   Float64 CompressionStressCoefficient_Fatigue = 0.40;

   bool Compare(const ClosureJointCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};
