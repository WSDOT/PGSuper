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

#include <StrData.h>
#include <Materials/Rebar.h>


class WBFL::System::IStructuredLoad;
class WBFL::System::IStructuredSave;

/*****************************************************************************
CLASS 
   CHorizontalInterfaceZoneData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CHorizontalInterfaceZoneData
{
public:
   ZoneIndexType ZoneNum;
   Float64 ZoneLength;
   Float64 BarSpacing;
   WBFL::Materials::Rebar::Size BarSize;
   Float64 nBars;

   CHorizontalInterfaceZoneData();
   CHorizontalInterfaceZoneData(const CHorizontalInterfaceZoneData& rOther) = default;
   ~CHorizontalInterfaceZoneData();

   CHorizontalInterfaceZoneData& operator = (const CHorizontalInterfaceZoneData& rOther) = default;
   bool operator == (const CHorizontalInterfaceZoneData& rOther) const;
   bool operator != (const CHorizontalInterfaceZoneData& rOther) const;

	  HRESULT Load(WBFL::System::IStructuredLoad* pStrLoad);
	  HRESULT Save(WBFL::System::IStructuredSave* pStrSave);
};
