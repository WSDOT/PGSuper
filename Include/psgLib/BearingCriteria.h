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
#include <EngTools/BearingDesignCriteria.h>


class rptChapter;
class IEAFDisplayUnits;
class SpecLibraryEntryImpl;

struct PSGLIBCLASS BearingCriteria : public WBFL::EngTools::BearingProjectCriteria
{
    BearingCriteria()
    {
        bRequiredIntermediateElastomerThickness = true;
        bMinimumTotalBearingHeight = true;
        bMinimumBearingEdgeToGirderEdgeDistance = true;
        bMaximumBearingEdgeToGirderEdgeDistance = true;
        bRequiredBearingEdgeToGirderEdgeDistance = true;
        bMaximumTotalLoad = true;
    }

    bool bCheck = true;

    bool bAlertTaperedSolePlateRequirement = true; ///< Option to enable/disable tapered sole plate check
    Float64 TaperedSolePlateInclinationThreshold = 0.01; ///< Inclination threshold per LRFD 14.8.2 (radian)
    bool bUseImpactForBearingReactions = false; ///< See LRFD 14.4.1



    bool operator==(const BearingCriteria& other) const;
    bool operator!=(const BearingCriteria& other) const;
    bool Compare(const BearingCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const;

   void Report(rptChapter* pChapter, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

    void Save(WBFL::System::IStructuredSave* pSave) const;
    void Load(WBFL::System::IStructuredLoad* pLoad);
};

