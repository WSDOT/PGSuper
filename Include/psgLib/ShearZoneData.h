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
#include <PsgLib\ShearZoneData2.h>
#include <MathEx.h>
#include <StrData.h>
#include <Materials/Rebar.h>

class WBFL::System::IStructuredLoad;
class WBFL::System::IStructuredSave;
class CShearData;

/*****************************************************************************
CLASS 
   CShearZoneData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CShearZoneData
{
   friend CShearData; // only or friend can see legacy data

public:
   Uint32  ZoneNum;
   WBFL::Materials::Rebar::Size VertBarSize;
   Float64 BarSpacing;
   Float64 ZoneLength;
   Float64 nVertBars;
   Float64 nHorzInterfaceBars;
   WBFL::Materials::Rebar::Size ConfinementBarSize;

private:
   // These values are used only for CShearData version < 9
   WBFL::Materials::Rebar::Size  legacy_HorzBarSize;
   Uint32          legacy_nHorzBars;

   // GROUP: LIFECYCLE
public:
   CShearZoneData();
   CShearZoneData(const CShearZoneData& rOther) = default;
   ~CShearZoneData() = default;

   CShearZoneData& operator = (const CShearZoneData& rOther) = default;
   bool operator == (const CShearZoneData& rOther) const;
   bool operator != (const CShearZoneData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(WBFL::System::IStructuredLoad* pStrLoad, bool bConvertToShearDataVersion9, 
                WBFL::Materials::Rebar::Size ConfinementBarSize,Uint32 NumConfinementZones, 
                bool bDoStirrupsEngageDeck);

	HRESULT Save(WBFL::System::IStructuredSave* pStrSave);

   CShearZoneData2 Convert() const;
};

class PSGLIBCLASS ShearZoneDataLess
{
public:
   bool operator()(const CShearZoneData& a, const CShearZoneData& b)
   {
      return a.ZoneNum < b.ZoneNum;
   }
};
