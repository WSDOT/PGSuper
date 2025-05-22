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
#include <array>

class rptChapter;
class IEAFDisplayUnits;
class SpecLibraryEntryImpl;
namespace PGS {namespace Library{class DifferenceItem;};};


struct PSGLIBCLASS LimitsCriteria
{
   bool bCheckStirrupSpacingCompatibility;

   bool bCheckSag; // evaluate girder camber and dead load deflections and check for sag potential
   pgsTypes::SagCamber SagCamber; // indicates the camber used to detect girder sag potential

   // Concrete limits
   // limiting the size of this array to 3 for NWC and 2 kinds of LWC.. these parameters aren't used for UHPCs yet
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxSlabFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxSegmentFci;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxSegmentFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxClosureFci;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxClosureFc;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxConcreteUnitWeight;
   std::array<Float64, 3/*pgsTypes::ConcreteTypeCount*/> MaxConcreteAggSize;

   LimitsCriteria();

   bool Compare(const LimitsCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};
