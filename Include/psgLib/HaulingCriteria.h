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
#include <psgLib/TensionStressLimit.h>
#include <Stability/StabilityTypes.h>
#include <array>

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS KDOTHaulingCriteria
{
   Float64 OverhangGFactor = 3.0;
   Float64 InteriorGFactor = 1.0;

   Float64 CompressionStressLimitCoefficient = 0.6;
   TensionStressLimit TensionStressLimitWithoutReinforcement{ WBFL::Units::ConvertToSysUnits(0.0948, WBFL::Units::Measure::SqrtKSI) ,true, WBFL::Units::ConvertToSysUnits(0.200, WBFL::Units::Measure::KSI) };
   TensionStressLimit TensionStressLimitWithReinforcement{ WBFL::Units::ConvertToSysUnits(0.18, WBFL::Units::Measure::SqrtKSI) ,false, WBFL::Units::ConvertToSysUnits(0.200, WBFL::Units::Measure::KSI) };

   bool Compare(const KDOTHaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;
   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;
   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

struct PSGLIBCLASS WSDOTHaulingCriteria
{
   Float64 FsCracking = 1.0;
   Float64 FsFailure = 1.5;

   std::array<Float64, 3> ModulusOfRuptureCoefficient // pgsTypes::ConcreteType is the array index, pgsTypes::PCI_UHPC and pgsTypes::UHPC are not valid
   {
      WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI), // Normal
      WBFL::Units::ConvertToSysUnits(0.18,WBFL::Units::Measure::SqrtKSI), // AllLightweight
      WBFL::Units::ConvertToSysUnits(0.21,WBFL::Units::Measure::SqrtKSI)  // SandLightweight
   };

   WBFL::Stability::HaulingImpact ImpactUsage = WBFL::Stability::HaulingImpact::NormalCrown;
   Float64 ImpactUp = 0.0;
   Float64 ImpactDown = 0.0;

   Float64 RoadwayCrownSlope = 0.0;
   Float64 RoadwaySuperelevation = 0.06;
   Float64 SweepTolerance = WBFL::Units::ConvertToSysUnits(1. / 8., WBFL::Units::Measure::Inch) / WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Feet);
   Float64 SweepGrowth = 0.0; // PCI's value is 1.0", but we've never used this before so we'll default to 0.0. PCI models sweep at hauling as (1/8" per 10 ft + 1"). The 1" is the sweep growth
   Float64 SupportPlacementTolerance = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch);
   Float64 CamberMultiplier = 1.0; // multiplier for direct camber

   WBFL::Stability::WindLoadType WindLoadType = WBFL::Stability::WindLoadType::Speed;
   Float64 WindLoad = 0.0;

   WBFL::Stability::CFType CentrifugalForceType = WBFL::Stability::CFType::Favorable;
   Float64 HaulingSpeed = 0.0;
   Float64 TurningRadius = WBFL::Units::ConvertToSysUnits(1000, WBFL::Units::Measure::Feet);

   Float64 CompressionStressCoefficient_GlobalStress = 0.60;
   Float64 CompressionStressCoefficient_PeakStress = 0.60;
   std::array<TensionStressLimit, 2> TensionStressLimitWithReinforcement // array index is WBFL::Stability::HaulingSlope
   {
      TensionStressLimit{WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI),false,0.0},
      TensionStressLimit{WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI),false,0.0}
   };

   std::array<TensionStressLimit, 2> TensionStressLimitWithoutReinforcement // array index is WBFL::Stability::HaulingSlope
   {
      TensionStressLimit{WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI),true,WBFL::Units::ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)},
      TensionStressLimit{WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI),false,WBFL::Units::ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)}
   };

   bool Compare(const WSDOTHaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;
   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;
   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

struct PSGLIBCLASS HaulingCriteria
{
   bool    bCheck = true;
   bool    bDesign = true;
   pgsTypes::HaulingAnalysisMethod AnalysisMethod = pgsTypes::HaulingAnalysisMethod::WSDOT;

   Float64 MinBunkPoint = -1; // -1 = H from end of girder
   Float64 BunkPointAccuracy = WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Feet);
   bool    bUseMinBunkPointLimit = true; // When true, the bunk point location is limited to (MinBunkPointLimitFactor)*(Girder Length) - KDOT only
   Float64 MinBunkPointLimitFactor = 0.1; // Minimum buck point is taken not less than (MinBunkPointLimitFactor)*(Girder Length)

   WSDOTHaulingCriteria WSDOT;
   KDOTHaulingCriteria KDOT;


   bool Compare(const HaulingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

