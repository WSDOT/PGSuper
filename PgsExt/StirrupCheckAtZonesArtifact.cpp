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

#include <PGSExt\StirrupCheckAtZonesArtifact.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStirrupCheckAtZonesArtifactKey
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStirrupCheckAtZonesArtifactKey::pgsStirrupCheckAtZonesArtifactKey()
{
}

pgsStirrupCheckAtZonesArtifactKey::pgsStirrupCheckAtZonesArtifactKey(Uint32 zoneNum)
{
   m_ZoneNum = zoneNum;
}

pgsStirrupCheckAtZonesArtifactKey::pgsStirrupCheckAtZonesArtifactKey(const pgsStirrupCheckAtZonesArtifactKey& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupCheckAtZonesArtifactKey::~pgsStirrupCheckAtZonesArtifactKey()
{
}

//======================== OPERATORS  =======================================
pgsStirrupCheckAtZonesArtifactKey& pgsStirrupCheckAtZonesArtifactKey::operator = (const pgsStirrupCheckAtZonesArtifactKey& rOther)
{
   if ( this != &rOther )
      MakeAssignment( rOther );

   return *this;
}

bool pgsStirrupCheckAtZonesArtifactKey::operator<(const pgsStirrupCheckAtZonesArtifactKey& rOther) const
{
   return m_ZoneNum < rOther.m_ZoneNum;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStirrupCheckAtZonesArtifactKey::MakeCopy(const pgsStirrupCheckAtZonesArtifactKey& rOther)
{
   m_ZoneNum         = rOther.m_ZoneNum;
}

void pgsStirrupCheckAtZonesArtifactKey::MakeAssignment(const pgsStirrupCheckAtZonesArtifactKey& rOther)
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
bool pgsStirrupCheckAtZonesArtifactKey::AssertValid() const
{
   return true;
}

void pgsStirrupCheckAtZonesArtifactKey::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStirrupCheckAtZonesArtifactKey" << endl;
   os << "m_ZoneNum"<< m_ZoneNum << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsStirrupCheckAtZonesArtifactKey::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsStirrupCheckAtZonesArtifactKey");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsStirrupCheckAtZonesArtifactKey");

   TESTME_EPILOG("pgsStirrupCheckAtZonesArtifactKey");
}
#endif // _UNITTEST





/****************************************************************************
CLASS
   pgsSplittingZoneArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsSplittingZoneArtifact::pgsSplittingZoneArtifact()
{
   m_IsApplicable=false;
   m_Avs = 0;
   m_SplittingDirection = pgsTypes::sdVertical;
}

pgsSplittingZoneArtifact::pgsSplittingZoneArtifact(const pgsSplittingZoneArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSplittingZoneArtifact::~pgsSplittingZoneArtifact()
{
}

//======================== OPERATORS  =======================================
pgsSplittingZoneArtifact& pgsSplittingZoneArtifact::operator= (const pgsSplittingZoneArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

//======================== OPERATIONS =======================================
bool pgsSplittingZoneArtifact::GetIsApplicable() const
{
   return m_IsApplicable;
}

void pgsSplittingZoneArtifact::SetIsApplicable(bool isApp)
{
   m_IsApplicable = isApp;
}

Float64 pgsSplittingZoneArtifact::GetH() const
{
   ATLASSERT(m_IsApplicable);
   return m_H;
}

void pgsSplittingZoneArtifact::SetH(Float64 h)
{
   ATLASSERT(m_IsApplicable);
   m_H = h;
}

Float64 pgsSplittingZoneArtifact::GetSplittingZoneLength() const
{
   ATLASSERT(m_IsApplicable);
   return m_SplittingZoneLength;
}

void pgsSplittingZoneArtifact::SetSplittingZoneLength(Float64 bzl)
{
   ATLASSERT(m_IsApplicable);
   m_SplittingZoneLength = bzl;
}

Float64 pgsSplittingZoneArtifact::GetSplittingZoneLengthFactor() const
{
   ATLASSERT(m_IsApplicable);
   return m_SplittingZoneLengthFactor;
}

void pgsSplittingZoneArtifact::SetSplittingZoneLengthFactor(Float64 bzlf)
{
   ATLASSERT(m_IsApplicable);
   m_SplittingZoneLengthFactor = bzlf;
}

Float64 pgsSplittingZoneArtifact::GetFs() const
{
   ATLASSERT(m_IsApplicable);
   return m_Fs;
}

void pgsSplittingZoneArtifact::SetFs(Float64 fs)
{
   ATLASSERT(m_IsApplicable);
   m_Fs = fs;
}

pgsTypes::SplittingDirection pgsSplittingZoneArtifact::GetSplittingDirection() const
{
   return m_SplittingDirection;
}

void pgsSplittingZoneArtifact::SetSplittingDirection(pgsTypes::SplittingDirection sd)
{
   m_SplittingDirection = sd;
}

void pgsSplittingZoneArtifact::SetAvs(Float64 avs)
{
   ATLASSERT(m_IsApplicable);
   m_Avs = avs;
}

Float64 pgsSplittingZoneArtifact::GetAvs() const
{
   ATLASSERT(m_IsApplicable);
   return m_Avs;
}

Float64 pgsSplittingZoneArtifact::GetAps() const
{
   ATLASSERT(m_IsApplicable);
   return m_Aps;
}

void pgsSplittingZoneArtifact::SetAps(Float64 aps)
{
   ATLASSERT(m_IsApplicable);
   m_Aps = aps;
}

Float64 pgsSplittingZoneArtifact::GetFpj() const
{
   ATLASSERT(m_IsApplicable);
   return m_Fpj;
}

void pgsSplittingZoneArtifact::SetFpj(Float64 fpj)
{
   ATLASSERT(m_IsApplicable);
   m_Fpj = fpj;
}

Float64 pgsSplittingZoneArtifact::GetLossesAfterTransfer() const
{
   ATLASSERT(m_IsApplicable);
   return m_dFpT;
}

void pgsSplittingZoneArtifact::SetLossesAfterTransfer(double dFpT)
{
   m_dFpT = dFpT;
}

Float64 pgsSplittingZoneArtifact::GetSplittingForce() const
{
   ATLASSERT(m_IsApplicable);
   double P = 0.04*m_Aps*(m_Fpj - m_dFpT);
   return P;
}


Float64 pgsSplittingZoneArtifact::GetSplittingResistance() const
{
   ATLASSERT(m_IsApplicable);
   return m_Pr;
}

void pgsSplittingZoneArtifact::SetSplittingResistance(Float64 p)
{
   ATLASSERT(m_IsApplicable);
   m_Pr = p;
}


bool pgsSplittingZoneArtifact::Passed() const
{
   if (m_IsApplicable)
   {
      return m_Pr >= GetSplittingForce();
   }
   else
   {
      return true;
   }
}

//======================== ACCESS     =======================================

//======================== INQUIRY    =======================================

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsSplittingZoneArtifact::MakeCopy(const pgsSplittingZoneArtifact& rOther)
{
   m_IsApplicable = rOther.m_IsApplicable;
   m_SplittingZoneLength = rOther.m_SplittingZoneLength;
   m_SplittingZoneLengthFactor = rOther.m_SplittingZoneLengthFactor;
   m_H                  = rOther.m_H;
   m_Aps                = rOther.m_Aps;
   m_Fpj                = rOther.m_Fpj;
   m_dFpT               = rOther.m_dFpT;
   m_Avs                = rOther.m_Avs;
   m_Fs                 = rOther.m_Fs;
   m_Pr                 = rOther.m_Pr;
   m_SplittingDirection = rOther.m_SplittingDirection;
}

void pgsSplittingZoneArtifact::MakeAssignment(const pgsSplittingZoneArtifact& rOther)
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


/****************************************************************************
CLASS
   pgsConfinementArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsConfinementArtifact::pgsConfinementArtifact():
m_IsApplicable(false)
{
}

pgsConfinementArtifact::pgsConfinementArtifact(const pgsConfinementArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsConfinementArtifact::~pgsConfinementArtifact()
{
}

//======================== OPERATORS  =======================================
pgsConfinementArtifact& pgsConfinementArtifact::operator= (const pgsConfinementArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

//======================== OPERATIONS =======================================

bool pgsConfinementArtifact::Passed() const
{
   if ( !m_IsApplicable )
      return true;

   // min bar size
   if (m_MinBarSize>0)
   {
      if (m_BarSize==0)
         return false;

      lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
      CHECK(pool!=0);
      const matRebar* bs = pool->GetRebar(m_BarSize);
      CHECK(bs!=0);
      const matRebar* mnbs = pool->GetRebar(m_MinBarSize);
      CHECK(mnbs!=0);

      if (bs->GetNominalDimension() < mnbs->GetNominalDimension())
         return false;
   }
      
   // spacing
   const Float64 tol = 1.0e-6;
   if (GetS() > GetSMax()+tol)
      return false;

   return true;
}

//======================== ACCESS     =======================================

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsConfinementArtifact::AssertValid() const
{
   return true;
}

void pgsConfinementArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsConfinementArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsConfinementArtifact::MakeCopy(const pgsConfinementArtifact& rOther)
{
   m_IsApplicable = rOther.m_IsApplicable;
   m_ApplicableZoneLength = rOther.m_ApplicableZoneLength;
   m_ZoneEnd = rOther.m_ZoneEnd;
   m_BarSize = rOther.m_BarSize;
   m_S = rOther.m_S;
   m_MinBarSize = rOther.m_MinBarSize;
   m_SMax = rOther.m_SMax;
}

void pgsConfinementArtifact::MakeAssignment(const pgsConfinementArtifact& rOther)
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



/****************************************************************************
CLASS
   pgsStirrupCheckAtZonesArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStirrupCheckAtZonesArtifact::pgsStirrupCheckAtZonesArtifact()
{
}

pgsStirrupCheckAtZonesArtifact::pgsStirrupCheckAtZonesArtifact(const pgsStirrupCheckAtZonesArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupCheckAtZonesArtifact::~pgsStirrupCheckAtZonesArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStirrupCheckAtZonesArtifact& pgsStirrupCheckAtZonesArtifact::operator= (const pgsStirrupCheckAtZonesArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

void pgsStirrupCheckAtZonesArtifact::SetConfinementArtifact(const pgsConfinementArtifact& artifact)
{
   m_ConfinementArtifact = artifact;
}

const pgsConfinementArtifact* pgsStirrupCheckAtZonesArtifact::GetConfinementArtifact() const
{
   return &m_ConfinementArtifact;
}

bool pgsStirrupCheckAtZonesArtifact::Passed() const
{
   if (!m_ConfinementArtifact.Passed())
      return false;

   return true;
}

//======================== ACCESS     =======================================

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsStirrupCheckAtZonesArtifact::AssertValid() const
{
   return true;
}

void pgsStirrupCheckAtZonesArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStirrupCheckAtZonesArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStirrupCheckAtZonesArtifact::MakeCopy(const pgsStirrupCheckAtZonesArtifact& rOther)
{
   m_ConfinementArtifact =  rOther.m_ConfinementArtifact;
}

void pgsStirrupCheckAtZonesArtifact::MakeAssignment(const pgsStirrupCheckAtZonesArtifact& rOther)
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
