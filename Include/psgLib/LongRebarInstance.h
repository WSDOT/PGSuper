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
#include <Materials/Rebar.h>
#include <GeomModel/Primitives.h>

/*****************************************************************************
CLASS 
   pgsLongRebarInstance

   Instance of a rebar in a cross section.


DESCRIPTION
   Instance of a rebar in a cross section.

LOG
   rdp : 05.31.1999 : Created file
*****************************************************************************/

class PSGLIBCLASS pgsLongRebarInstance
{
public:
   pgsLongRebarInstance();
   pgsLongRebarInstance(const WBFL::Geometry::Point2d& rloc, const WBFL::Materials::Rebar* pRebar, Float64 minCutoffLength);
   pgsLongRebarInstance(const pgsLongRebarInstance& rOther) = default;
   ~pgsLongRebarInstance() = default;

   pgsLongRebarInstance& operator = (const pgsLongRebarInstance& rOther) = default;

   // Location in section where rebar occurs
   const WBFL::Geometry::Point2d& GetLocation() const;
   void SetLocation(const WBFL::Geometry::Point2d& loc);

   // a pointer to this rebar's material
   const WBFL::Materials::Rebar* GetRebar() const;
   void SetRebar(const WBFL::Materials::Rebar* pRebar);

   // Nearest Distance from cut location to either end of the bar
   Float64 GetMinCutoffLength() const;
   void SetMinCutoffLength(Float64 length);

private:
   // GROUP: DATA MEMBERS
      WBFL::Geometry::Point2d       m_Location;
      const WBFL::Materials::Rebar* m_pRebar;
      Float64         m_MinCutoffLength; // Distance from cut location
};
