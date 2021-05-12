///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <PgsExt\HoldDownForceArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsHoldDownForceArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHoldDownForceArtifact::pgsHoldDownForceArtifact()
{
}

pgsHoldDownForceArtifact::pgsHoldDownForceArtifact(const pgsHoldDownForceArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHoldDownForceArtifact::~pgsHoldDownForceArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHoldDownForceArtifact& pgsHoldDownForceArtifact::operator=(const pgsHoldDownForceArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsHoldDownForceArtifact::SetCapacity(Float64 capacity)
{
   m_Capacity = capacity;
}

Float64 pgsHoldDownForceArtifact::GetCapacity() const
{
   return m_Capacity;
}

void pgsHoldDownForceArtifact::SetDemand(Float64 demand)
{
   m_Demand = demand;
}

Float64 pgsHoldDownForceArtifact::GetDemand() const
{
   return m_Demand;
}

void pgsHoldDownForceArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsHoldDownForceArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

bool pgsHoldDownForceArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
   {
      return true;
   }

   if ( m_Capacity < m_Demand )
   {
      return false;
   }

   return true;
}

 //======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHoldDownForceArtifact::MakeCopy(const pgsHoldDownForceArtifact& rOther)
{
   m_bIsApplicable = rOther.m_bIsApplicable;
   m_Demand        = rOther.m_Demand;
   m_Capacity      = rOther.m_Capacity;
}

void pgsHoldDownForceArtifact::MakeAssignment(const pgsHoldDownForceArtifact& rOther)
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsHoldDownForceArtifact::AssertValid() const
{
   return true;
}

void pgsHoldDownForceArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHoldDownForceArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsHoldDownForceArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsHoldDownForceArtifact");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsHoldDownForceArtifact");

   TESTME_EPILOG("pgsHoldDownForceArtifact");
}
#endif // _UNITTEST
