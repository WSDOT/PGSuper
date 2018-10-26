///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\DeflectionCheckArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDeflectionCheckArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsDeflectionCheckArtifact::pgsDeflectionCheckArtifact()
{
}

pgsDeflectionCheckArtifact::pgsDeflectionCheckArtifact(const pgsDeflectionCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsDeflectionCheckArtifact::~pgsDeflectionCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsDeflectionCheckArtifact& pgsDeflectionCheckArtifact::operator=(const pgsDeflectionCheckArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsDeflectionCheckArtifact::SetAllowableSpanRatio(Float64 val)
{
   m_AllowableSpanRatio = val;
}

Float64 pgsDeflectionCheckArtifact::GetAllowableSpanRatio() const
{
   return m_AllowableSpanRatio;
}

void pgsDeflectionCheckArtifact::SetCapacity(Float64 capacity)
{
   m_Capacity = capacity;
}

Float64 pgsDeflectionCheckArtifact::GetCapacity() const
{
   return m_Capacity;
}

void pgsDeflectionCheckArtifact::SetDemand(Float64 min,Float64 max)
{
   m_MinDemand = min;
   m_MaxDemand = max;
}

void pgsDeflectionCheckArtifact::GetDemand(Float64* pMin,Float64* pMax) const
{
   *pMin = m_MinDemand;
   *pMax = m_MaxDemand;
}

void pgsDeflectionCheckArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsDeflectionCheckArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

bool pgsDeflectionCheckArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
      return true;

   if ( m_MinDemand < -m_Capacity || m_Capacity < m_MaxDemand )
      return false;

   return true;
}

 //======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsDeflectionCheckArtifact::MakeCopy(const pgsDeflectionCheckArtifact& rOther)
{
   m_bIsApplicable      = rOther.m_bIsApplicable;
   m_MinDemand          = rOther.m_MinDemand;
   m_MaxDemand          = rOther.m_MaxDemand;
   m_Capacity           = rOther.m_Capacity;
   m_AllowableSpanRatio = rOther.m_AllowableSpanRatio;
}

void pgsDeflectionCheckArtifact::MakeAssignment(const pgsDeflectionCheckArtifact& rOther)
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
bool pgsDeflectionCheckArtifact::AssertValid() const
{
   return true;
}

void pgsDeflectionCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsDeflectionCheckArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsDeflectionCheckArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsDeflectionCheckArtifact");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsDeflectionCheckArtifact");

   TESTME_EPILOG("pgsDeflectionCheckArtifact");
}
#endif // _UNITTEST
