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

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS LiftingCriteria
{
   bool    bCheck = true;
   bool    bDesign = true;
   Float64 PickPointHeight = 0.0;
   Float64 LiftingLoopTolerance = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch);
   Float64 MinCableInclination = WBFL::Units::ConvertToSysUnits(90., WBFL::Units::Measure::Degree);
   Float64 SweepTolerance = WBFL::Units::ConvertToSysUnits(1. / 16., WBFL::Units::Measure::Inch) / WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Feet);
   Float64 ImpactUp = 0.0;
   Float64 ImpactDown = 0.0;
   Float64 CamberMultiplier = 1.0; // multiplier for direct camber
   WBFL::Stability::WindLoadType WindLoadType = WBFL::Stability::WindLoadType::Speed;
   Float64 WindLoad = 0.0;

   std::array<Float64, 3> ModulusOfRuptureCoefficient; // pgsTypes::ConcreteType is the array index, pgsTypes::PCI_UHPC and pgsTypes::UHPC are not valid
   Float64 MinPickPoint = -1; // minimum distance from end of beam to pick point. -1 means H from end of the beam
   Float64 PickPointAccuracy = WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Feet);

   Float64 CompressionStressCoefficient_GlobalStress = 0.65;
   Float64 CompressionStressCoefficient_PeakStress = 0.70;
   TensionStressLimit TensionStressLimitWithoutReinforcement;
   TensionStressLimit TensionStressLimitWithReinforcement;

   Float64 FsCracking = 1.0;
   Float64 FsFailure = 1.5;

   LiftingCriteria();

   bool Compare(const LiftingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

