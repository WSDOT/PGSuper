///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\PoiArtifactKey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsFlexuralStressArtifactKey
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsFlexuralStressArtifactKey::pgsFlexuralStressArtifactKey()
{
}

pgsFlexuralStressArtifactKey::pgsFlexuralStressArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType stress,Float64 distFromStart)
{
   m_Stage = stage;
   m_LimitState = ls;
   m_StressType = stress;
   m_DistFromStart = distFromStart;
}

pgsFlexuralStressArtifactKey::pgsFlexuralStressArtifactKey(const pgsFlexuralStressArtifactKey& rOther)
{
   MakeCopy(rOther);
}

pgsFlexuralStressArtifactKey::~pgsFlexuralStressArtifactKey()
{
}

//======================== OPERATORS  =======================================
pgsFlexuralStressArtifactKey& pgsFlexuralStressArtifactKey::operator = (const pgsFlexuralStressArtifactKey& rOther)
{
   if ( this != &rOther )
      MakeAssignment( rOther );

   return *this;
}

bool pgsFlexuralStressArtifactKey::operator<(const pgsFlexuralStressArtifactKey& rOther) const
{
   if ( m_Stage < rOther.m_Stage )
      return true;

   if ( m_Stage > rOther.m_Stage )
      return false;

   if ( m_LimitState < rOther.m_LimitState )
      return true;

   if ( m_LimitState > rOther.m_LimitState )
      return false;

   if ( m_StressType < rOther.m_StressType )
      return true;

   if ( m_StressType > rOther.m_StressType )
      return false;

   if ( m_DistFromStart < rOther.m_DistFromStart )
      return true;

   if ( IsEqual(m_DistFromStart,rOther.m_DistFromStart) || m_DistFromStart > rOther.m_DistFromStart )
      return false;

   return true;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsFlexuralStressArtifactKey::MakeCopy(const pgsFlexuralStressArtifactKey& rOther)
{
   m_Stage         = rOther.m_Stage;
   m_LimitState    = rOther.m_LimitState;
   m_StressType    = rOther.m_StressType;
   m_DistFromStart = rOther.m_DistFromStart;
}

void pgsFlexuralStressArtifactKey::MakeAssignment(const pgsFlexuralStressArtifactKey& rOther)
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
bool pgsFlexuralStressArtifactKey::AssertValid() const
{
   return true;
}

void pgsFlexuralStressArtifactKey::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsFlexuralStressArtifactKey" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsFlexuralStressArtifactKey::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsFlexuralStressArtifactKey");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsFlexuralStressArtifactKey");

   TESTME_EPILOG("pgsFlexuralStressArtifactKey");
}
#endif // _UNITTEST


/****************************************************************************
CLASS
   pgsPoiArtifactKey
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPoiArtifactKey::pgsPoiArtifactKey()
{
}

pgsPoiArtifactKey::pgsPoiArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 distFromStart)
{
   m_Stage = stage;
   m_LimitState = ls;
   m_DistFromStart = distFromStart;
}

pgsPoiArtifactKey::pgsPoiArtifactKey(const pgsPoiArtifactKey& rOther)
{
   MakeCopy(rOther);
}

pgsPoiArtifactKey::~pgsPoiArtifactKey()
{
}

//======================== OPERATORS  =======================================
pgsPoiArtifactKey& pgsPoiArtifactKey::operator = (const pgsPoiArtifactKey& rOther)
{
   if ( this != &rOther )
      MakeAssignment( rOther );

   return *this;
}

bool pgsPoiArtifactKey::operator<(const pgsPoiArtifactKey& rOther) const
{
   if ( m_Stage < rOther.m_Stage )
      return true;

   if ( m_Stage > rOther.m_Stage )
      return false;

   if ( m_LimitState < rOther.m_LimitState )
      return true;

   if ( m_LimitState > rOther.m_LimitState )
      return false;

   if ( IsEqual(m_DistFromStart,rOther.m_DistFromStart) || m_DistFromStart > rOther.m_DistFromStart )
      return false;

   return true;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPoiArtifactKey::MakeCopy(const pgsPoiArtifactKey& rOther)
{
   m_Stage         = rOther.m_Stage;
   m_LimitState    = rOther.m_LimitState;
   m_DistFromStart = rOther.m_DistFromStart;
}

void pgsPoiArtifactKey::MakeAssignment(const pgsPoiArtifactKey& rOther)
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
bool pgsPoiArtifactKey::AssertValid() const
{
   return true;
}

void pgsPoiArtifactKey::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsPoiArtifactKey" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsPoiArtifactKey::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsPoiArtifactKey");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsPoiArtifactKey");

   TESTME_EPILOG("pgsPoiArtifactKey");
}
#endif // _UNITTEST



/****************************************************************************
CLASS
   pgsFlexuralCapacityArtifactKey
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsFlexuralCapacityArtifactKey::pgsFlexuralCapacityArtifactKey():
pgsPoiArtifactKey()
{
}

pgsFlexuralCapacityArtifactKey::pgsFlexuralCapacityArtifactKey(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 distFromStart):
pgsPoiArtifactKey(stage,ls,distFromStart)
{
}

pgsFlexuralCapacityArtifactKey::pgsFlexuralCapacityArtifactKey(const pgsFlexuralCapacityArtifactKey& rOther)
{
   MakeCopy(rOther);
}

pgsFlexuralCapacityArtifactKey::~pgsFlexuralCapacityArtifactKey()
{
}

//======================== OPERATORS  =======================================
pgsFlexuralCapacityArtifactKey& pgsFlexuralCapacityArtifactKey::operator = (const pgsFlexuralCapacityArtifactKey& rOther)
{
   if ( this != &rOther )
      MakeAssignment( rOther );

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsFlexuralCapacityArtifactKey::MakeCopy(const pgsFlexuralCapacityArtifactKey& rOther)
{
   pgsPoiArtifactKey::MakeCopy(rOther);
}

void pgsFlexuralCapacityArtifactKey::MakeAssignment(const pgsFlexuralCapacityArtifactKey& rOther)
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
bool pgsFlexuralCapacityArtifactKey::AssertValid() const
{
   pgsPoiArtifactKey::AssertValid();
   return true;
}

void pgsFlexuralCapacityArtifactKey::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsFlexuralCapacityArtifactKey" << endl;
   pgsPoiArtifactKey::Dump(os);
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsFlexuralCapacityArtifactKey::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsFlexuralCapacityArtifactKey");

   pgsPoiArtifactKey::TestMe(rlog);

   TESTME_EPILOG("pgsFlexuralCapacityArtifactKey");
}
#endif // _UNITTEST


