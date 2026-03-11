///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "DebondResults.h"
#include <AgentTools.h>

#include <IFace\Bridge.h>
#include <IFace/PointOfInterest.h>

#include <Plugins\BeamFamilyCLSID.h>


int CDebondResults::GetDebondDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, std::vector<DebondInformation>& debondInfo) const
{
    GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);
    if (!pStrandGeometry->IsDebondingSymmetric(segmentKey))
    {
        return DEBOND_ERROR_SYMMETRIC;
    }

    // fail if not symmetric
    SectionIndexType nSections = pStrandGeometry->GetNumDebondSections(segmentKey, pgsTypes::metStart, pgsTypes::Straight);
    for (SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++)
    {
        DebondInformation dbInfo;
        dbInfo.Length = pStrandGeometry->GetDebondSection(segmentKey, pgsTypes::metStart, sectionIdx, pgsTypes::Straight);

        dbInfo.Strands = pStrandGeometry->GetDebondedStrandsAtSection(segmentKey, pgsTypes::metStart, sectionIdx, pgsTypes::Straight);

        debondInfo.push_back(dbInfo);
    }

    return DEBOND_ERROR_NONE;
}