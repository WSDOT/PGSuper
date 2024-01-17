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
#include <array>

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS MomentCapacityCriteria
{
   bool bIncludeRebar; ///< if true, longitudinal girder rebar are included when computing moment capacity
   bool bIncludeStrandForNegMoment; ///< if true, prestressing strands are included when computing negative moment capacity

   // indicates if the strain limits specified in the relevant material
   // standards are considered when computing moment capacity. If considered, and the strains are
   // exceeded when the concrete crushing strain is at 0.003, the capacity is recomputed based
   // on the limiting strain
   bool bConsiderReinforcementStrainLimit;

   bool bIncludeNoncompositeMomentsForNegMomentDesign; // if true, noncomposite moments are included in the negative moment design moment, otherwise only loads applied to the composite section are used

   int OverReinforcedMomentCapacity;///< 0 = over reinforced moment capacity computed per LRFD C5.7.3.3.1 (method removed in LRFD 2005) otherwise computed by pre-2005 WSDOT method


   IndexType nMomentCapacitySlices; // user defined number of slices for strain compatibility analysis (constrained to be between 10 and 100)
   std::array<Float64, 3>  ModulusOfRuptureCoefficient; // index is pgsTypes::ConcreteType enum (UHPCs are not valid)

   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiTensionPS; // tension controlled, prestressed
   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiTensionRC; // tension controlled, reinforced
   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiTensionSpliced; // tension controlled, spliced girders
   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiCompression;

   std::array<Float64, pgsTypes::ConcreteTypeCount> PhiClosureJoint;

   MomentCapacityCriteria();

   bool Compare(const MomentCapacityCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);

   Float64 GetModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const;
   void GetResistanceFactors(pgsTypes::ConcreteType type, Float64* phiTensionPS, Float64* phiTensionRC, Float64* phiTensionSpliced, Float64* phiCompression) const;
   Float64 GetClosureJointResistanceFactor(pgsTypes::ConcreteType type) const;
};
