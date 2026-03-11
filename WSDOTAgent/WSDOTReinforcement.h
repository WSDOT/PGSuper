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

#pragma once

#include <PsgLib\Keys.h>

namespace WBFL { namespace EAF { class Broker; }; };


#define STIRRUP_ERROR_NONE        0
#define STIRRUP_ERROR            -1
#define STIRRUP_ERROR_BARSIZE    -2
#define STIRRUP_ERROR_ZONES      -3
#define STIRRUP_ERROR_SYMMETRIC  -4
#define STIRRUP_ERROR_STARTZONE  -5
#define STIRRUP_ERROR_LASTZONE   -6
#define STIRRUP_ERROR_V6         -7


class CWSDOTReinforcement
{
public:
	int GetWSDOTReinforcementDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, CLSID& familyCLSID,
		WBFL::Materials::Rebar::Size* pz1BarSize, Float64* pz1Spacing, Float64* pz1Length, WBFL::Materials::Rebar::Size* pz2BarSize, 
		Float64* pz2Spacing, Float64* pz2Length, WBFL::Materials::Rebar::Size* pz3BarSize, Float64* pz3Spacing, Float64* pz3Length) const;
};