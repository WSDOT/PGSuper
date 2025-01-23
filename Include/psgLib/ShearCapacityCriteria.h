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

struct PSGLIBCLASS ShearCapacityCriteria
{
   pgsTypes::ShearCapacityMethod CapacityMethod = pgsTypes::scmBTEquations; // LRFD 5.7.3.4.2
   bool bLimitNetTensionStrainToPositiveValues = false; // when true, es from LRFD Eq 5.7.3.4.2-4 is taken to be zero if it is computed as a negative value
   std::array<Float64, 3>  ModulusOfRuptureCoefficient;   // index is pgsTypes::ConcreteType enum (PCI UHPC is not valid), LRFD 5.4.2.6

   std::array<Float64, pgsTypes::ConcreteTypeCount> Phi;
   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiDebonded;
   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiClosureJoint;

   // Maximum spacing of transverse reinforcement, LRFD 5.7.2.6
   std::array<Float64, 2> MaxStirrupSpacing; // Index 0 for vu < 0.125 f'c, Index 1 vu >= 0.125f'c
   std::array<Float64, 2> StirrupSpacingCoefficient; // Index 0 for vu < 0.125 f'c, Index 1 vu >= 0.125f'c

   // Longitudinal reinforcement shear capacity
   pgsTypes::LongitudinalReinforcementForShearMethod LongitudinalReinforcementForShearMethod = pgsTypes::LongitudinalReinforcementForShearMethod::LRFD;
   bool bIncludeRebar;

   ShearCapacityCriteria();

   bool Compare(const ShearCapacityCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   Float64 GetResistanceFactor(pgsTypes::ConcreteType concreteType, bool bIsDebonded, WBFL::LRFD::BDSManager::Edition edition) const;
   Float64 GetClosureJointResistanceFactor(pgsTypes::ConcreteType concreteType) const;
};
