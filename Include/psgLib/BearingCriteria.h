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


class rptChapter;
class IEAFDisplayUnits;
class SpecLibraryEntryImpl;
namespace PGS {namespace Library{class DifferenceItem;};};

struct PSGLIBCLASS BearingCriteria
{
   bool bAlertTaperedSolePlateRequirement = true; ///< Option to enable/disable tapered sole plat check
   Float64 TaperedSolePlateInclinationThreshold = 0.01; ///< Inclination threshold per LRFD 14.8.2 (radian)
   bool bUseImpactForBearingReactions = false; ///< See LRFD 14.4.1
   bool bCheck = false;
   pgsTypes::BearingDesignMethod BearingDesignMethod = pgsTypes::BearingDesignMethod::MethodB; ///< AASHTO Bearing Design Method
   Float64 MinimumElastomerShearModulus = WBFL::Units::ConvertToSysUnits(0.080, WBFL::Units::Measure::KSI); ///< See LRFD 14.7.5.2
   Float64 MaximumElastomerShearModulus = WBFL::Units::ConvertToSysUnits(0.175, WBFL::Units::Measure::KSI); ///< See LRFD 14.7.5.2
   bool bRequiredIntermediateElastomerThickness = false;
   Float64 RequiredIntermediateElastomerThickness = WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Inch);
   bool bMinimumTotalBearingHeight = false;
   Float64 MinimumTotalBearingHeight = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch);
   bool bMinimumBearingEdgeToGirderEdgeDistance = false;
   Float64 MinimumBearingEdgeToGirderEdgeDistance = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch);
   bool bMaximumBearingEdgeToGirderEdgeDistance = false;
   Float64 MaximumBearingEdgeToGirderEdgeDistance = WBFL::Units::ConvertToSysUnits(9.0, WBFL::Units::Measure::Inch);
   bool bRequiredBearingEdgeToGirderEdgeDistance = false;
   Float64 RequiredBearingEdgeToGirderEdgeDistance = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch);
   Float64 MaximumLiveLoadDeflection = WBFL::Units::ConvertToSysUnits(0.125, WBFL::Units::Measure::Inch);
   bool bMaximumTotalLoad = false;
   Float64 MaximumTotalLoad = WBFL::Units::ConvertToSysUnits(800.0, WBFL::Units::Measure::Kip);


   bool operator==(const BearingCriteria& other) const;
   bool operator!=(const BearingCriteria& other) const;
   bool Compare(const BearingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences,bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void Save(WBFL::System::IStructuredSave* pSave) const;
   void Load(WBFL::System::IStructuredLoad* pLoad);
};

