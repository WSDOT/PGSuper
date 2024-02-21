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

class rptChapter;
interface IEAFDisplayUnits;
class pgsLibraryEntryDifferenceItem;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS LiveLoadDistributionCriteria
{
   pgsTypes::LiveLoadDistributionFactorMethod LldfMethod = pgsTypes::LiveLoadDistributionFactorMethod::LRFD;
   bool bIgnoreSkewReductionForMoment = false;

   // "fudge" factors for computing live load distribution factors
   Float64 MaxAngularDeviationBetweenGirders = WBFL::Units::ConvertToSysUnits(5.0, WBFL::Units::Measure::Degree); // maximum angle between girders in order to consider them "parallel"
   Float64 MinGirderStiffnessRatio = 0.90; // minimum allowable value for EImin/EImax in order to consider girders to have "approximately the same stiffness"
   Float64 GirderSpacingLocation = 0.75; // fractional location in the girder span where the girder spacing is measured
   // for purposes of computing live load distribution factors

   bool bLimitDistributionFactorsToLanesBeams = false;
   bool bExteriorBeamLiveLoadDistributionGTInteriorBeam = false;

   bool bUseRigidMethod = false; // if true, the rigid method is always used with Type a, e, and k cross section for exterior beam LLDF

   bool operator==(const LiveLoadDistributionCriteria& other) const;
   bool operator!=(const LiveLoadDistributionCriteria& other) const;
   bool Compare(const LiveLoadDistributionCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

