///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\StrandSlopeArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStrandSlopeArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStrandSlopeArtifact::pgsStrandSlopeArtifact()
{
}

pgsStrandSlopeArtifact::pgsStrandSlopeArtifact(const pgsStrandSlopeArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStrandSlopeArtifact::~pgsStrandSlopeArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStrandSlopeArtifact& pgsStrandSlopeArtifact::operator=(const pgsStrandSlopeArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsStrandSlopeArtifact::SetCapacity(Float64 capacity)
{
   m_Capacity = capacity;
}

Float64 pgsStrandSlopeArtifact::GetCapacity() const
{
   return m_Capacity;
}

void pgsStrandSlopeArtifact::SetDemand(Float64 demand)
{
   m_Demand = demand;
}

Float64 pgsStrandSlopeArtifact::GetDemand() const
{
   return m_Demand;
}

void pgsStrandSlopeArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsStrandSlopeArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

bool pgsStrandSlopeArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
   {
      return true;
   }

   if ( 1.0/m_Capacity < 1.0/m_Demand && !IsEqual(m_Capacity,m_Demand) )
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
void pgsStrandSlopeArtifact::MakeCopy(const pgsStrandSlopeArtifact& rOther)
{
   m_bIsApplicable = rOther.m_bIsApplicable;
   m_Demand        = rOther.m_Demand;
   m_Capacity      = rOther.m_Capacity;
}

void pgsStrandSlopeArtifact::MakeAssignment(const pgsStrandSlopeArtifact& rOther)
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
bool pgsStrandSlopeArtifact::AssertValid() const
{
   return true;
}

void pgsStrandSlopeArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStrandSlopeArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsStrandSlopeArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsStrandSlopeArtifact");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsStrandSlopeArtifact");

   TESTME_EPILOG("pgsStrandSlopeArtifact");
}
#endif // _UNITTEST
