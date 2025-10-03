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

#include "StdAfx.h"
#include "WSDOTReinforcement.h"
#include <AgentTools.h>

#include <IFace\Bridge.h>
#include <IFace/PointOfInterest.h>

#include <Plugins\BeamFamilyCLSID.h>


int CWSDOTReinforcement::GetWSDOTReinforcementDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, 
    CLSID& familyCLSID, Float64* pz1Spacing, Float64* pz1Length, Float64* pz2Spacing, Float64* pz2Length, Float64* pz3Spacing, Float64* pz3Length) const
{
    GET_IFACE2(pBroker, IStirrupGeometry, pStirrupGeometry);
    if (!pStirrupGeometry->AreStirrupZonesSymmetrical(segmentKey))
    {
        return STIRRUP_ERROR_SYMMETRIC;
    }

    // Check if the number of zones is consistent with the girder schedule
    ZoneIndexType nZones = pStirrupGeometry->GetPrimaryZoneCount(segmentKey); // this is total number of zones
    nZones = nZones / 2 + 1; // this is the input number of zones (and it must be symmetric)
    if (nZones != 5 && nZones != 6)
    {
        return STIRRUP_ERROR_ZONES;
    }

    // Check first zone... it must be 1-1/2" long with 1-1/2" spacing... one space
    WBFL::Materials::Rebar::Size barSize;
    Float64 count, spacing;
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, 0, &barSize, &count, &spacing);

    Float64 zoneStart, zoneEnd;
    pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, 0, &zoneStart, &zoneEnd);
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, 0, &barSize, &count, &spacing);
    Float64 zoneLength = zoneEnd - zoneStart;
    Float64 v = zoneLength / spacing;

    if (barSize != WBFL::Materials::Rebar::Size::bs5)
    {
        return STIRRUP_ERROR_BARSIZE;
    }

    if (!IsEqual(v, 1.0) && !IsEqual(spacing, WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch)))
    {
        return STIRRUP_ERROR_STARTZONE;
    }

    // So far, so good... start figuring out the V values

    // Zone 1 (V1 & V2)
    pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, 1, &zoneStart, &zoneEnd);
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, 1, &barSize, &count, &spacing);
    zoneLength = zoneEnd - zoneStart;
    v = zoneLength / spacing;
    if (!IsEqual(v, Round(v)))
    {
        return STIRRUP_ERROR_ZONES;
    }

    if (barSize != WBFL::Materials::Rebar::Size::bs5)
    {
        return STIRRUP_ERROR_BARSIZE;
    }

    *pz1Spacing = spacing;
    *pz1Length = zoneLength;

    // Zone 2 (V3 & V4)
    pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, 2, &zoneStart, &zoneEnd);
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, 2, &barSize, &count, &spacing);
    zoneLength = zoneEnd - zoneStart;
    v = zoneLength / spacing;
    if (!IsEqual(v, Round(v)))
    {
        return STIRRUP_ERROR_ZONES;
    }

    if (barSize != WBFL::Materials::Rebar::Size::bs5)
    {
        return STIRRUP_ERROR_BARSIZE;
    }

    *pz2Spacing = spacing;
    *pz2Length = zoneLength;

    // Zone 3 (either V6 or V5 & V6);
    Float64 v6Spacing;
    if (nZones == 6)
    {
        // this small zone that is labeled V6 on the stirrup layout is modeled
        pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, 3, &zoneStart, &zoneEnd);
        pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, 3, &barSize, &count, &spacing);
        zoneLength = zoneEnd - zoneStart;
        v = zoneLength / spacing;
        if (!IsEqual(v, Round(v)))
        {
            return STIRRUP_ERROR_ZONES;
        }

        if (familyCLSID == CLSID_UBeamFamily)
        {
            if (barSize != WBFL::Materials::Rebar::Size::bs4)
            {
                return STIRRUP_ERROR_BARSIZE;
            }
        }
        else
        {
            if (barSize != WBFL::Materials::Rebar::Size::bs5)
            {
                return STIRRUP_ERROR_BARSIZE;
            }
        }

        if (!IsEqual(zoneLength, spacing))
        {
            return STIRRUP_ERROR_V6;
        }

        v6Spacing = spacing;
    }

    ZoneIndexType zoneIdx = (nZones == 5 ? 3 : 4);
    pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, zoneIdx, &zoneStart, &zoneEnd);
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, zoneIdx, &barSize, &count, &spacing);
    zoneLength = zoneEnd - zoneStart;
    v = zoneLength / spacing;
    if (!IsEqual(v, Round(v)))
    {
        return STIRRUP_ERROR_ZONES;
    }

    if (familyCLSID == CLSID_UBeamFamily)
    {
        if (barSize != WBFL::Materials::Rebar::Size::bs4)
        {
            return STIRRUP_ERROR_BARSIZE;
        }
    }
    else
    {
        if (barSize != WBFL::Materials::Rebar::Size::bs5)
        {
            return STIRRUP_ERROR_BARSIZE;
        }
    }

    *pz3Spacing = spacing;
    *pz3Length = zoneLength;

    if (nZones == 6 && !IsEqual(spacing, v6Spacing))
    {
        return STIRRUP_ERROR_V6;
    }

    zoneIdx = (nZones == 5 ? 4 : 5);
    pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey, zoneIdx, &barSize, &count, &spacing);

    if (familyCLSID == CLSID_WFBeamFamily || familyCLSID == CLSID_UBeamFamily)
    {
        if (!IsEqual(spacing, WBFL::Units::ConvertToSysUnits(18.0, WBFL::Units::Measure::Inch)))
        {
            return STIRRUP_ERROR_LASTZONE;
        }
    }
    else if (familyCLSID == CLSID_SlabBeamFamily)
    {
        GET_IFACE2(pBroker, IPointOfInterest, pPoi);
        pgsPointOfInterest poi = pPoi->GetPointOfInterest(segmentKey, 0.0);

        GET_IFACE2(pBroker, IGirder, pGirder);
        Float64 H = pGirder->GetHeight(poi);

        Float64 lastZoneSpacing = Min(H - WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch), WBFL::Units::ConvertToSysUnits(18.0, WBFL::Units::Measure::Inch));
        if (!IsEqual(spacing, lastZoneSpacing))
        {
            return STIRRUP_ERROR_LASTZONE;
        }
    }
    else
    {
        if (!IsEqual(spacing, WBFL::Units::ConvertToSysUnits(9.0, WBFL::Units::Measure::Inch)))
        {
            return STIRRUP_ERROR_LASTZONE;
        }
    }


    if (familyCLSID == CLSID_UBeamFamily || familyCLSID == CLSID_SlabBeamFamily)
    {
        if (barSize != WBFL::Materials::Rebar::Size::bs4)
        {
            return STIRRUP_ERROR_BARSIZE;
        }
    }
    else
    {
        if (barSize != WBFL::Materials::Rebar::Size::bs5)
        {
            return STIRRUP_ERROR_BARSIZE;
        }
    }

    return STIRRUP_ERROR_NONE;
}