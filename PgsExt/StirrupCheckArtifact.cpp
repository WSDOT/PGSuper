///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <PgsExt\StirrupCheckArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStirrupCheckArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStirrupCheckArtifact::pgsStirrupCheckArtifact()
{
}

pgsStirrupCheckArtifact::pgsStirrupCheckArtifact(const pgsStirrupCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupCheckArtifact::~pgsStirrupCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStirrupCheckArtifact& pgsStirrupCheckArtifact::operator= (const pgsStirrupCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsStirrupCheckArtifact::AddStirrupCheckAtPoisArtifact(const pgsStirrupCheckAtPoisArtifactKey& key,
                                                  const pgsStirrupCheckAtPoisArtifact& artifact)
{
   m_StirrupCheckAtPoisArtifacts.insert(std::make_pair(key,artifact));
}

const pgsStirrupCheckAtPoisArtifact* pgsStirrupCheckArtifact::GetStirrupCheckAtPoisArtifact(const pgsStirrupCheckAtPoisArtifactKey& key) const
{
   std::map<pgsStirrupCheckAtPoisArtifactKey,pgsStirrupCheckAtPoisArtifact>::const_iterator found;
   found = m_StirrupCheckAtPoisArtifacts.find( key );
   if ( found == m_StirrupCheckAtPoisArtifacts.end() )
      return 0;

   return &(*found).second;
}

void pgsStirrupCheckArtifact::SetConfinementArtifact(const pgsConfinementArtifact& artifact)
{
   m_ConfinementArtifact = artifact;
}

const pgsConfinementArtifact& pgsStirrupCheckArtifact::GetConfinementArtifact() const
{
   return m_ConfinementArtifact;
}

pgsSplittingZoneArtifact* pgsStirrupCheckArtifact::GetSplittingZoneArtifact()
{
   return &m_SplittingZoneArtifact;
}

const pgsSplittingZoneArtifact* pgsStirrupCheckArtifact::GetSplittingZoneArtifact() const
{
   return &m_SplittingZoneArtifact;
}

void pgsStirrupCheckArtifact::Clear()
{
   m_StirrupCheckAtPoisArtifacts.clear();
}

bool pgsStirrupCheckArtifact::Passed() const
{
   std::map<pgsStirrupCheckAtPoisArtifactKey,pgsStirrupCheckAtPoisArtifact>::const_iterator i3;
   for ( i3 = m_StirrupCheckAtPoisArtifacts.begin(); i3 != m_StirrupCheckAtPoisArtifacts.end(); i3++ )
   {
      const std::pair<pgsStirrupCheckAtPoisArtifactKey,pgsStirrupCheckAtPoisArtifact>& artifact = *i3;
      if (!artifact.second.Passed())
         return false;
   }

   bool bPassed = true;

   bPassed &= m_SplittingZoneArtifact.Passed();

   bPassed &= m_ConfinementArtifact.Passed();

   return bPassed;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStirrupCheckArtifact::MakeCopy(const pgsStirrupCheckArtifact& rOther)
{
   m_StirrupCheckAtPoisArtifacts   = rOther.m_StirrupCheckAtPoisArtifacts;
   m_Fy                            = rOther.m_Fy;
   m_Fc                            = rOther.m_Fc;
   m_ConfinementArtifact           = rOther.m_ConfinementArtifact;
   m_SplittingZoneArtifact         = rOther.m_SplittingZoneArtifact;
}

void pgsStirrupCheckArtifact::MakeAssignment(const pgsStirrupCheckArtifact& rOther)
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
bool pgsStirrupCheckArtifact::AssertValid() const
{
   return true;
}

void pgsStirrupCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStirrupCheckArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsStirrupCheckArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsStirrupCheckArtifact");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsStirrupCheckArtifact");

   TESTME_EPILOG("StirrupCheckArtifact");
}
#endif // _UNITTEST