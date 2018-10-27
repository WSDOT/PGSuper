///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
   m_IsApplicable = false;
   m_SplittingDirection = pgsTypes::sdVertical;
   m_SplittingZoneLengthFactor = 0;

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      m_SplittingZoneLength[endType] = 0;
      m_H[endType] = 0;
      m_Avs[endType] = 0;
      m_Fs[endType] = 0;
      m_Pr[endType] = 0;

      for (int j = 0; j < 3; j++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
         m_Aps[endType][strandType] = 0;
         m_Fpj[endType][strandType] = 0;
         m_dFpT[endType][strandType] = 0;
      }
   }
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
   return Passed(pgsTypes::metStart) && Passed(pgsTypes::metEnd);
}

Float64 pgsSplittingZoneArtifact::GetH(pgsTypes::MemberEndType endType) const
{
   ATLASSERT(m_IsApplicable);
   return m_H[endType];
}

void pgsSplittingZoneArtifact::SetH(pgsTypes::MemberEndType endType,Float64 h)
{
   ATLASSERT(m_IsApplicable);
   m_H[endType] = h;
}

Float64 pgsSplittingZoneArtifact::GetSplittingZoneLength(pgsTypes::MemberEndType endType) const
{
   ATLASSERT(m_IsApplicable);
   return m_SplittingZoneLength[endType];
}

void pgsSplittingZoneArtifact::SetSplittingZoneLength(pgsTypes::MemberEndType endType,Float64 bzl)
{
   ATLASSERT(m_IsApplicable);
   m_SplittingZoneLength[endType] = bzl;
}

Float64 pgsSplittingZoneArtifact::GetFs(pgsTypes::MemberEndType endType) const
{
   ATLASSERT(m_IsApplicable);
   return m_Fs[endType];
}

void pgsSplittingZoneArtifact::SetFs(pgsTypes::MemberEndType endType,Float64 fs)
{
   ATLASSERT(m_IsApplicable);
   m_Fs[endType] = fs;
}

void pgsSplittingZoneArtifact::SetAvs(pgsTypes::MemberEndType endType,Float64 avs)
{
   ATLASSERT(m_IsApplicable);
   m_Avs[endType] = avs;
}

Float64 pgsSplittingZoneArtifact::GetAvs(pgsTypes::MemberEndType endType) const
{
   ATLASSERT(m_IsApplicable);
   return m_Avs[endType];
}

Float64 pgsSplittingZoneArtifact::GetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   ATLASSERT(m_IsApplicable);
   if (strandType == pgsTypes::Permanent)
   {
      return m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped];
   }
   else
   {
      return m_Aps[endType][strandType];
   }
}

void pgsSplittingZoneArtifact::SetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 aps)
{
   ATLASSERT(m_IsApplicable);
   m_Aps[endType][strandType] = aps;
}

Float64 pgsSplittingZoneArtifact::GetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   ATLASSERT(m_IsApplicable);
   if (strandType == pgsTypes::Permanent)
   {
      if (IsZero(m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]))
      {
         return 0;
      }
      else
      {
         return (m_Fpj[endType][pgsTypes::Straight] * m_Aps[endType][pgsTypes::Straight] + m_Fpj[endType][pgsTypes::Harped] * m_Aps[endType][pgsTypes::Harped]) / (m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]);
      }
   }
   else
   {
      return m_Fpj[endType][strandType];
   }
}

void pgsSplittingZoneArtifact::SetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 fpj)
{
   ATLASSERT(m_IsApplicable);
   m_Fpj[endType][strandType] = fpj;
}

Float64 pgsSplittingZoneArtifact::GetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   ATLASSERT(m_IsApplicable);
   if (strandType == pgsTypes::Permanent)
   {
      if (IsZero(m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]))
      {
         return 0;
      }
      else
      {
         return (m_dFpT[endType][pgsTypes::Straight] * m_Aps[endType][pgsTypes::Straight] + m_dFpT[endType][pgsTypes::Harped] * m_Aps[endType][pgsTypes::Harped]) / (m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]);
      }
   }
   else
   {
      return m_dFpT[endType][strandType];
   }
}

void pgsSplittingZoneArtifact::SetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 dFpT)
{
   m_dFpT[endType][strandType] = dFpT;
}

Float64 pgsSplittingZoneArtifact::GetSplittingForce(pgsTypes::MemberEndType endType, pgsTypes::StrandType strandType) const
{
   ATLASSERT(m_IsApplicable);
   if (strandType == pgsTypes::Permanent)
   {
      Float64 Ps = GetSplittingForce(endType, pgsTypes::Straight);
      Float64 Ph = GetSplittingForce(endType, pgsTypes::Harped);
      return Ps + Ph;
   }
   else
   {
      Float64 P = 0.04*m_Aps[endType][strandType]*(m_Fpj[endType][strandType] - m_dFpT[endType][strandType]);
      return P;
   }
}

Float64 pgsSplittingZoneArtifact::GetTotalSplittingForce(pgsTypes::MemberEndType endType) const
{
   Float64 Ps = GetSplittingForce(endType, pgsTypes::Straight);
   Float64 Ph = GetSplittingForce(endType, pgsTypes::Harped);
   Float64 Pt = GetSplittingForce(endType, pgsTypes::Temporary);
   return Ps + Ph + Pt;
}

Float64 pgsSplittingZoneArtifact::GetSplittingResistance(pgsTypes::MemberEndType endType) const
{
   ATLASSERT(m_IsApplicable);
   return m_Pr[endType];
}

void pgsSplittingZoneArtifact::SetSplittingResistance(pgsTypes::MemberEndType endType,Float64 p)
{
   ATLASSERT(m_IsApplicable);
   m_Pr[endType] = p;
}


bool pgsSplittingZoneArtifact::Passed(pgsTypes::MemberEndType endType) const
{
   if (m_IsApplicable)
   {
      return GetTotalSplittingForce(endType) <= m_Pr[endType];
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

   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      m_SplittingZoneLength[endType] = rOther.m_SplittingZoneLength[endType];
      m_H[endType] = rOther.m_H[endType];
      m_Avs[endType] = rOther.m_Avs[endType];
      m_Fs[endType] = rOther.m_Fs[endType];
      m_Pr[endType] = rOther.m_Pr[endType];

      for (int j = 0; j < 3; j++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
         m_Aps[endType][strandType] = rOther.m_Aps[endType][strandType];
         m_Fpj[endType][strandType] = rOther.m_Fpj[endType][strandType];
         m_dFpT[endType][strandType] = rOther.m_dFpT[endType][strandType];
      }
   }
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
   m_pStartRebar = nullptr;
   m_pMinRebar = nullptr;
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
   if (m_pMinRebar != nullptr)
   {
      if (m_pStartRebar == nullptr)
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
   if (m_pMinRebar != nullptr)
   {
      if (m_pEndRebar == nullptr)
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