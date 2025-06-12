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
#include <PsgLib\LongRebarInstance.h>


/****************************************************************************
CLASS
   pgsLongRebarInstance
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLongRebarInstance::pgsLongRebarInstance():
m_pRebar(0),
m_MinCutoffLength(0.0)
{
}


pgsLongRebarInstance::pgsLongRebarInstance(const WBFL::Geometry::Point2d& rloc,
                                           const WBFL::Materials::Rebar* pRebar,
                                           Float64 minCutoffLength):
m_Location(rloc),
m_pRebar(pRebar),
m_MinCutoffLength(minCutoffLength)
{
}


const WBFL::Geometry::Point2d& pgsLongRebarInstance::GetLocation() const
{
   return m_Location;
}

void pgsLongRebarInstance::SetLocation(const WBFL::Geometry::Point2d& loc)
{
   m_Location = loc;
}

const WBFL::Materials::Rebar* pgsLongRebarInstance::GetRebar() const
{
   return m_pRebar;
}
void pgsLongRebarInstance::SetRebar(const WBFL::Materials::Rebar* pRebar)
{
   ATLASSERT(pRebar!=nullptr);
   m_pRebar = pRebar;
}

Float64 pgsLongRebarInstance::GetMinCutoffLength() const
{
   ATLASSERT(0.0 < m_MinCutoffLength);
   return m_MinCutoffLength;
}

void pgsLongRebarInstance::SetMinCutoffLength(Float64 length)
{
   m_MinCutoffLength = length;
}
