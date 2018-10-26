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

pgsStirrupCheckAtZonesArtifactKey::pgsStirrupCheckAtZonesArtifactKey(ZoneIndexType zoneNum)
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
   {
      MakeAssignment( rOther );
   }

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
   m_StartAvs = 0;
   m_EndAvs = 0;
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

pgsTypes::SplittingDirection pgsSplittingZoneArtifact::GetSplittingDirection() const
{
   return m_SplittingDirection;
}

void pgsSplittingZoneArtifact::SetSplittingDirection(pgsTypes::SplittingDirection sd)
{
   m_SplittingDirection = sd;
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

bool pgsSplittingZoneArtifact::Passed() const
{
   return StartPassed() && EndPassed();
}

// Start
Float64 pgsSplittingZoneArtifact::GetStartH() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartH;
}

void pgsSplittingZoneArtifact::SetStartH(Float64 h)
{
   ATLASSERT(m_IsApplicable);
   m_StartH = h;
}

Float64 pgsSplittingZoneArtifact::GetStartSplittingZoneLength() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartSplittingZoneLength;
}

void pgsSplittingZoneArtifact::SetStartSplittingZoneLength(Float64 bzl)
{
   ATLASSERT(m_IsApplicable);
   m_StartSplittingZoneLength = bzl;
}

Float64 pgsSplittingZoneArtifact::GetStartFs() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartFs;
}

void pgsSplittingZoneArtifact::SetStartFs(Float64 fs)
{
   ATLASSERT(m_IsApplicable);
   m_StartFs = fs;
}

void pgsSplittingZoneArtifact::SetStartAvs(Float64 avs)
{
   ATLASSERT(m_IsApplicable);
   m_StartAvs = avs;
}

Float64 pgsSplittingZoneArtifact::GetStartAvs() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartAvs;
}

Float64 pgsSplittingZoneArtifact::GetStartAps() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartAps;
}

void pgsSplittingZoneArtifact::SetStartAps(Float64 aps)
{
   ATLASSERT(m_IsApplicable);
   m_StartAps = aps;
}

Float64 pgsSplittingZoneArtifact::GetStartFpj() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartFpj;
}

void pgsSplittingZoneArtifact::SetStartFpj(Float64 fpj)
{
   ATLASSERT(m_IsApplicable);
   m_StartFpj = fpj;
}

Float64 pgsSplittingZoneArtifact::GetStartLossesAfterTransfer() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartdFpT;
}

void pgsSplittingZoneArtifact::SetStartLossesAfterTransfer(Float64 dFpT)
{
   m_StartdFpT = dFpT;
}

Float64 pgsSplittingZoneArtifact::GetStartSplittingForce() const
{
   ATLASSERT(m_IsApplicable);
   Float64 P = 0.04*m_StartAps*(m_StartFpj - m_StartdFpT);
   return P;
}


Float64 pgsSplittingZoneArtifact::GetStartSplittingResistance() const
{
   ATLASSERT(m_IsApplicable);
   return m_StartPr;
}

void pgsSplittingZoneArtifact::SetStartSplittingResistance(Float64 p)
{
   ATLASSERT(m_IsApplicable);
   m_StartPr = p;
}


bool pgsSplittingZoneArtifact::StartPassed() const
{
   if (m_IsApplicable)
   {
      return GetStartSplittingForce() <= m_StartPr;
   }
   else
   {
      return true;
   }
}

// End
Float64 pgsSplittingZoneArtifact::GetEndH() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndH;
}

void pgsSplittingZoneArtifact::SetEndH(Float64 h)
{
   ATLASSERT(m_IsApplicable);
   m_EndH = h;
}

Float64 pgsSplittingZoneArtifact::GetEndSplittingZoneLength() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndSplittingZoneLength;
}

void pgsSplittingZoneArtifact::SetEndSplittingZoneLength(Float64 bzl)
{
   ATLASSERT(m_IsApplicable);
   m_EndSplittingZoneLength = bzl;
}

Float64 pgsSplittingZoneArtifact::GetEndFs() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndFs;
}

void pgsSplittingZoneArtifact::SetEndFs(Float64 fs)
{
   ATLASSERT(m_IsApplicable);
   m_EndFs = fs;
}

void pgsSplittingZoneArtifact::SetEndAvs(Float64 avs)
{
   ATLASSERT(m_IsApplicable);
   m_EndAvs = avs;
}

Float64 pgsSplittingZoneArtifact::GetEndAvs() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndAvs;
}

Float64 pgsSplittingZoneArtifact::GetEndAps() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndAps;
}

void pgsSplittingZoneArtifact::SetEndAps(Float64 aps)
{
   ATLASSERT(m_IsApplicable);
   m_EndAps = aps;
}

Float64 pgsSplittingZoneArtifact::GetEndFpj() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndFpj;
}

void pgsSplittingZoneArtifact::SetEndFpj(Float64 fpj)
{
   ATLASSERT(m_IsApplicable);
   m_EndFpj = fpj;
}

Float64 pgsSplittingZoneArtifact::GetEndLossesAfterTransfer() const
{
   ATLASSERT(m_IsApplicable);
   return m_EnddFpT;
}

void pgsSplittingZoneArtifact::SetEndLossesAfterTransfer(Float64 dFpT)
{
   m_EnddFpT = dFpT;
}

Float64 pgsSplittingZoneArtifact::GetEndSplittingForce() const
{
   ATLASSERT(m_IsApplicable);
   Float64 P = 0.04*m_EndAps*(m_EndFpj - m_EnddFpT);
   return P;
}


Float64 pgsSplittingZoneArtifact::GetEndSplittingResistance() const
{
   ATLASSERT(m_IsApplicable);
   return m_EndPr;
}

void pgsSplittingZoneArtifact::SetEndSplittingResistance(Float64 p)
{
   ATLASSERT(m_IsApplicable);
   m_EndPr = p;
}


bool pgsSplittingZoneArtifact::EndPassed() const
{
   if (m_IsApplicable)
   {
      return m_EndPr >= GetEndSplittingForce();
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
   m_SplittingZoneLengthFactor = rOther.m_SplittingZoneLengthFactor;
   m_SplittingDirection = rOther.m_SplittingDirection;

   m_StartSplittingZoneLength = rOther.m_StartSplittingZoneLength;
   m_StartH                  = rOther.m_StartH;
   m_StartAps                = rOther.m_StartAps;
   m_StartFpj                = rOther.m_StartFpj;
   m_StartdFpT               = rOther.m_StartdFpT;
   m_StartAvs                = rOther.m_StartAvs;
   m_StartFs                 = rOther.m_StartFs;
   m_StartPr                 = rOther.m_StartPr;

   m_EndSplittingZoneLength = rOther.m_EndSplittingZoneLength;
   m_EndH                  = rOther.m_EndH;
   m_EndAps                = rOther.m_EndAps;
   m_EndFpj                = rOther.m_EndFpj;
   m_EnddFpT               = rOther.m_EnddFpT;
   m_EndAvs                = rOther.m_EndAvs;
   m_EndFs                 = rOther.m_EndFs;
   m_EndPr                 = rOther.m_EndPr;
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
   m_pStartRebar = NULL;
   m_pMinRebar = NULL;
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
bool pgsConfinementArtifact::IsApplicable() const 
{
   return m_IsApplicable;
}

void pgsConfinementArtifact::SetApplicability(bool isAp)
{ 
   m_IsApplicable = isAp;
}

const matRebar* pgsConfinementArtifact::GetMinBar() const 
{
   return m_pMinRebar;
}

void pgsConfinementArtifact::SetMinBar(const matRebar* pBar)
{ 
   m_pMinRebar = pBar;
}

Float64 pgsConfinementArtifact::GetSMax() const 
{
   return m_SMax;
}

void pgsConfinementArtifact::SetSMax(Float64 smax)
{
   m_SMax = smax;
}

Float64 pgsConfinementArtifact::GetZoneLengthFactor() const
{
   return m_ZoneLengthFactor;
}

void pgsConfinementArtifact::SetZoneLengthFactor(Float64 fac)
{
   m_ZoneLengthFactor = fac;
}

Float64 pgsConfinementArtifact::GetStartProvidedZoneLength() const 
{
   return m_StartProvidedZoneLength;
}

void pgsConfinementArtifact::SetStartProvidedZoneLength(Float64 zl)
{
   m_StartProvidedZoneLength = zl;
}

Float64 pgsConfinementArtifact::GetStartRequiredZoneLength() const 
{
   return m_StartRequiredZoneLength;
}

void pgsConfinementArtifact::SetStartRequiredZoneLength(Float64 zl)
{
   m_StartRequiredZoneLength=zl;
}

const matRebar* pgsConfinementArtifact::GetStartBar() const 
{
   return m_pStartRebar;
}

void pgsConfinementArtifact::SetStartBar(const matRebar* pRebar)
{ 
   m_pStartRebar = pRebar;
}

Float64 pgsConfinementArtifact::GetStartS() const 
{
   return m_StartS;
}

void pgsConfinementArtifact::SetStartS(Float64 s) 
{
   m_StartS = s;
}

Float64 pgsConfinementArtifact::GetStartd() const
{
   return m_Startd;
}

void pgsConfinementArtifact::SetStartd(Float64 d)
{
   m_Startd = d;
}

Float64 pgsConfinementArtifact::GetEndProvidedZoneLength() const 
{
   return m_EndProvidedZoneLength;
}

void pgsConfinementArtifact::SetEndProvidedZoneLength(Float64 zl)
{
   m_EndProvidedZoneLength = zl;
}

Float64 pgsConfinementArtifact::GetEndRequiredZoneLength() const 
{
   return m_EndRequiredZoneLength;
}

void pgsConfinementArtifact::SetEndRequiredZoneLength(Float64 zl)
{
   m_EndRequiredZoneLength=zl;
}

const matRebar* pgsConfinementArtifact::GetEndBar() const 
{
   return m_pEndRebar;
}

void pgsConfinementArtifact::SetEndBar(const matRebar* pRebar)
{ 
   m_pEndRebar = pRebar;
}

Float64 pgsConfinementArtifact::GetEndS() const 
{
   return m_EndS;
}

void pgsConfinementArtifact::SetEndS(Float64 s) 
{
   m_EndS = s;
}

Float64 pgsConfinementArtifact::GetEndd() const
{
   return m_Endd;
}

void pgsConfinementArtifact::SetEndd(Float64 d)
{
   m_Endd = d;
}

bool pgsConfinementArtifact::StartPassed() const
{
   if ( !m_IsApplicable )
   {
      return true;
   }

   const Float64 tol = 1.0e-6;

   // Zone length
   if (m_StartProvidedZoneLength+tol < m_StartRequiredZoneLength)
   {
      return false;
   }

   // min bar size
   if (m_pMinRebar != NULL)
   {
      if (m_pStartRebar == NULL)
      {
         return false;
      }

      if (m_pStartRebar->GetNominalDimension() < m_pMinRebar->GetNominalDimension())
      {
         return false;
      }
   }
      
   // spacing
   if (GetStartS() > GetSMax()+tol)
   {
      return false;
   }

   return true;
}

bool pgsConfinementArtifact::EndPassed() const
{
   if ( !m_IsApplicable )
   {
      return true;
   }

   const Float64 tol = 1.0e-6;

   // Zone length
   if (m_EndProvidedZoneLength+tol < m_EndRequiredZoneLength)
   {
      return false;
   }

   // min bar size
   if (m_pMinRebar != NULL)
   {
      if (m_pEndRebar == NULL)
      {
         return false;
      }

      if (m_pEndRebar->GetNominalDimension() < m_pMinRebar->GetNominalDimension())
      {
         return false;
      }
   }
      
   // spacing
   if (GetEndS() > GetSMax()+tol)
   {
      return false;
   }

   return true;
}

bool pgsConfinementArtifact::Passed() const
{
   return StartPassed() && EndPassed();
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
   m_pMinRebar = rOther.m_pMinRebar;
   m_SMax = rOther.m_SMax;

   m_ZoneLengthFactor = rOther.m_ZoneLengthFactor;

   m_pStartRebar = rOther.m_pStartRebar;
   m_StartProvidedZoneLength = rOther.m_StartProvidedZoneLength;
   m_StartRequiredZoneLength = rOther.m_StartRequiredZoneLength;
   m_StartS = rOther.m_StartS;
   m_Startd = rOther.m_Startd;

   m_pEndRebar = rOther.m_pEndRebar;
   m_EndProvidedZoneLength = rOther.m_EndProvidedZoneLength;
   m_EndRequiredZoneLength = rOther.m_EndRequiredZoneLength;
   m_EndS = rOther.m_EndS;
   m_Endd = rOther.m_Endd;
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
/*
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
*/