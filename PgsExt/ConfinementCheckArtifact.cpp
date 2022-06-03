///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\ConfinementCheckArtifact.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsConfinementCheckArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsConfinementCheckArtifact::pgsConfinementCheckArtifact():
m_IsApplicable(false)
{
   m_pStartRebar = nullptr;
   m_pMinRebar = nullptr;
}

pgsConfinementCheckArtifact::~pgsConfinementCheckArtifact()
{
}

bool pgsConfinementCheckArtifact::IsApplicable() const 
{
   return m_IsApplicable;
}

void pgsConfinementCheckArtifact::SetApplicability(bool isAp)
{ 
   m_IsApplicable = isAp;
}

const matRebar* pgsConfinementCheckArtifact::GetMinBar() const 
{
   return m_pMinRebar;
}

void pgsConfinementCheckArtifact::SetMinBar(const matRebar* pBar)
{ 
   m_pMinRebar = pBar;
}

Float64 pgsConfinementCheckArtifact::GetSMax() const 
{
   return m_SMax;
}

void pgsConfinementCheckArtifact::SetSMax(Float64 smax)
{
   m_SMax = smax;
}

Float64 pgsConfinementCheckArtifact::GetZoneLengthFactor() const
{
   return m_ZoneLengthFactor;
}

void pgsConfinementCheckArtifact::SetZoneLengthFactor(Float64 fac)
{
   m_ZoneLengthFactor = fac;
}

Float64 pgsConfinementCheckArtifact::GetStartProvidedZoneLength() const 
{
   return m_StartProvidedZoneLength;
}

void pgsConfinementCheckArtifact::SetStartProvidedZoneLength(Float64 zl)
{
   m_StartProvidedZoneLength = zl;
}

Float64 pgsConfinementCheckArtifact::GetStartRequiredZoneLength() const 
{
   return m_StartRequiredZoneLength;
}

void pgsConfinementCheckArtifact::SetStartRequiredZoneLength(Float64 zl)
{
   m_StartRequiredZoneLength=zl;
}

const matRebar* pgsConfinementCheckArtifact::GetStartBar() const 
{
   return m_pStartRebar;
}

void pgsConfinementCheckArtifact::SetStartBar(const matRebar* pRebar)
{ 
   m_pStartRebar = pRebar;
}

Float64 pgsConfinementCheckArtifact::GetStartS() const 
{
   return m_StartS;
}

void pgsConfinementCheckArtifact::SetStartS(Float64 s) 
{
   m_StartS = s;
}

Float64 pgsConfinementCheckArtifact::GetStartd() const
{
   return m_Startd;
}

void pgsConfinementCheckArtifact::SetStartd(Float64 d)
{
   m_Startd = d;
}

Float64 pgsConfinementCheckArtifact::GetEndProvidedZoneLength() const 
{
   return m_EndProvidedZoneLength;
}

void pgsConfinementCheckArtifact::SetEndProvidedZoneLength(Float64 zl)
{
   m_EndProvidedZoneLength = zl;
}

Float64 pgsConfinementCheckArtifact::GetEndRequiredZoneLength() const 
{
   return m_EndRequiredZoneLength;
}

void pgsConfinementCheckArtifact::SetEndRequiredZoneLength(Float64 zl)
{
   m_EndRequiredZoneLength=zl;
}

const matRebar* pgsConfinementCheckArtifact::GetEndBar() const 
{
   return m_pEndRebar;
}

void pgsConfinementCheckArtifact::SetEndBar(const matRebar* pRebar)
{ 
   m_pEndRebar = pRebar;
}

Float64 pgsConfinementCheckArtifact::GetEndS() const 
{
   return m_EndS;
}

void pgsConfinementCheckArtifact::SetEndS(Float64 s) 
{
   m_EndS = s;
}

Float64 pgsConfinementCheckArtifact::GetEndd() const
{
   return m_Endd;
}

void pgsConfinementCheckArtifact::SetEndd(Float64 d)
{
   m_Endd = d;
}

bool pgsConfinementCheckArtifact::StartPassed() const
{
   if ( !m_IsApplicable )
   {
      return true;
   }

   const Float64 tol = 1.0e-6;

   // Zone length
   if (::IsLT(m_StartProvidedZoneLength,m_StartRequiredZoneLength,tol))
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
   if (::IsLT(GetSMax(), GetStartS(), tol))
   {
      return false;
   }

   return true;
}

bool pgsConfinementCheckArtifact::EndPassed() const
{
   if ( !m_IsApplicable )
   {
      return true;
   }

   const Float64 tol = 1.0e-6;

   // Zone length
   if (::IsLT(m_EndProvidedZoneLength,m_EndRequiredZoneLength,tol))
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
   if (::IsLT(GetSMax(),GetEndS(),tol))
   {
      return false;
   }

   return true;
}

bool pgsConfinementCheckArtifact::Passed() const
{
   return StartPassed() && EndPassed();
}
