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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\LongRebarInstance.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

pgsLongRebarInstance::pgsLongRebarInstance(const pgsLongRebarInstance& rOther)
{
   MakeCopy(rOther);
}

pgsLongRebarInstance::~pgsLongRebarInstance()
{
}

//======================== OPERATORS  =======================================
pgsLongRebarInstance& pgsLongRebarInstance::operator= (const pgsLongRebarInstance& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

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



//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLongRebarInstance::MakeCopy(const pgsLongRebarInstance& rOther)
{
    m_Location = rOther.m_Location;
    m_pRebar = rOther.m_pRebar;
    m_MinCutoffLength = rOther.m_MinCutoffLength;
}

void pgsLongRebarInstance::MakeAssignment(const pgsLongRebarInstance& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
