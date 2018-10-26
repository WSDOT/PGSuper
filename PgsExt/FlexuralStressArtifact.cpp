///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsFlexuralStressArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsFlexuralStressArtifact::pgsFlexuralStressArtifact():
m_FcReqd(-99999)
{
   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = false;
      m_fPretension[stressLocation]   = 0.0;
      m_fPosttension[stressLocation]  = 0.0;
      m_fExternal[stressLocation]     = 0.0;
      m_fDemand[stressLocation]       = 0.0;
      m_fAllowable[stressLocation]    = 0.0;
      m_bIsInPTZ[stressLocation]      = false;
   }

   for ( int i = 0; i < 2; i++ )
   {
      m_Yna[i]                           = 0.0;
      m_At[i]                            = 0.0;
      m_T[i]                             = 0.0;
      m_AsProvided[i]                    = 0.0;
      m_AsRequired[i]                    = 0.0;
      m_fAltAllowableStress[i]           = 0.0;
      m_bIsAltTensileStressApplicable[i] = false;
   }
}

pgsFlexuralStressArtifact::pgsFlexuralStressArtifact(const pgsPointOfInterest& poi):
m_Poi(poi),
m_FcReqd(-99999)
{
   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = false;
      m_fPretension[stressLocation]   = 0.0;
      m_fPosttension[stressLocation]  = 0.0;
      m_fExternal[stressLocation]     = 0.0;
      m_fDemand[stressLocation]       = 0.0;
      m_fAllowable[stressLocation]    = 0.0;
      m_bIsInPTZ[stressLocation]      = false;
   }

   for ( int i = 0; i < 2; i++ )
   {
      m_Yna[i]                           = 0.0;
      m_At[i]                            = 0.0;
      m_T[i]                             = 0.0;
      m_AsProvided[i]                    = 0.0;
      m_AsRequired[i]                    = 0.0;
      m_fAltAllowableStress[i]           = 0.0;
      m_bIsAltTensileStressApplicable[i] = false;
   }
}

pgsFlexuralStressArtifact::pgsFlexuralStressArtifact(const pgsFlexuralStressArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsFlexuralStressArtifact::~pgsFlexuralStressArtifact()
{
}

//======================== OPERATORS  =======================================
pgsFlexuralStressArtifact& pgsFlexuralStressArtifact::operator=(const pgsFlexuralStressArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsFlexuralStressArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& pgsFlexuralStressArtifact::GetPointOfInterest() const
{
   return m_Poi;
}

bool pgsFlexuralStressArtifact::operator<(const pgsFlexuralStressArtifact& rOther) const
{
   return m_Poi < rOther.m_Poi;
}

void pgsFlexuralStressArtifact::SetLimitState(pgsTypes::LimitState limitState)
{
   m_LimitState = limitState;
}

pgsTypes::LimitState pgsFlexuralStressArtifact::GetLimitState() const
{
   return m_LimitState;
}

void pgsFlexuralStressArtifact::SetStressType(pgsTypes::StressType type)
{
   m_StressType = type;
}

pgsTypes::StressType pgsFlexuralStressArtifact::GetStressType() const
{
   return m_StressType;
}

void pgsFlexuralStressArtifact::IsApplicable(pgsTypes::StressLocation stressLocation,bool bIsApplicable)
{
   m_bIsApplicable[stressLocation] = bIsApplicable;
}

bool pgsFlexuralStressArtifact::IsApplicable(pgsTypes::StressLocation stressLocation) const
{
   return m_bIsApplicable[stressLocation];
}

void pgsFlexuralStressArtifact::SetPretensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPS)
{
   m_fPretension[stressLocation] = fPS;
}

Float64 pgsFlexuralStressArtifact::GetPretensionEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fPretension[stressLocation];
}

void pgsFlexuralStressArtifact::SetPosttensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPT)
{
   m_fPosttension[stressLocation] = fPT;
}

Float64 pgsFlexuralStressArtifact::GetPosttensionEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fPosttension[stressLocation];
}

void pgsFlexuralStressArtifact::SetExternalEffects(pgsTypes::StressLocation stressLocation,Float64 f)
{
   m_fExternal[stressLocation] = f;
}

Float64 pgsFlexuralStressArtifact::GetExternalEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fExternal[stressLocation];
}

void pgsFlexuralStressArtifact::SetDemand(pgsTypes::StressLocation stressLocation,Float64 fDemand)
{
   m_fDemand[stressLocation] = fDemand;
}

Float64 pgsFlexuralStressArtifact::GetDemand(pgsTypes::StressLocation stressLocation) const
{
   return m_fDemand[stressLocation];
}

void pgsFlexuralStressArtifact::IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation,bool bIsInPTZ)
{
   m_bIsInPTZ[stressLocation] = bIsInPTZ;
}

bool pgsFlexuralStressArtifact::IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation) const
{
   return m_bIsInPTZ[stressLocation];
}

void pgsFlexuralStressArtifact::SetCapacity(pgsTypes::StressLocation stressLocation,Float64 fAllowable)
{
   m_fAllowable[stressLocation] = fAllowable;
}

Float64 pgsFlexuralStressArtifact::GetCapacity(pgsTypes::StressLocation stressLocation) const
{
   return m_fAllowable[stressLocation];
}

void pgsFlexuralStressArtifact::SetRequiredConcreteStrength(Float64 fcReqd)
{
   m_FcReqd = fcReqd;
}

Float64 pgsFlexuralStressArtifact::GetRequiredConcreteStrength() const
{
   return m_FcReqd;
}

void pgsFlexuralStressArtifact::SetAlternativeTensileStressParameters(pgsTypes::GirderFace face,Float64 Yna,Float64 At,Float64 T,Float64 AsProvided,Float64 AsRequired,Float64 fHigherAllow)
{
   m_Yna[face]                 = Yna;
   m_At[face]                  = At;
   m_T[face]                   = T;
   m_AsProvided[face]          = AsProvided;
   m_AsRequired[face]          = AsRequired;
   m_fAltAllowableStress[face] = fHigherAllow;
}

void pgsFlexuralStressArtifact::GetAlternativeTensileStressParameters(pgsTypes::GirderFace face,Float64* Yna,Float64* At,Float64* T,Float64* AsProvided,Float64* AsRequired) const
{
   *Yna        = m_Yna[face];
   *At         = m_At[face];
   *T          = m_T[face];
   *AsProvided = m_AsProvided[face];
   *AsRequired = m_AsRequired[face];
}

Float64 pgsFlexuralStressArtifact::GetAlternativeAllowableTensileStress(pgsTypes::GirderFace face) const
{
   return m_fAltAllowableStress[face];
}

bool pgsFlexuralStressArtifact::WasWithRebarAllowableStressUsed(pgsTypes::GirderFace face) const
{
   // If na<0.0, then section was in compression
   return (0.0 < m_Yna[face]) && (m_AsRequired[face] <= m_AsProvided[face]) ? true : false;
}


bool pgsFlexuralStressArtifact::TopPassed() const
{
   bool bPassed = true;
   Float64 fTop = GetDemand(pgsTypes::TopGirder);
   Float64 fBot = GetDemand(pgsTypes::BottomGirder);

   bPassed = StressedPassed(fTop,pgsTypes::GirderTop);

   return bPassed;
}

bool pgsFlexuralStressArtifact::BottomPassed() const
{
   bool bPassed = true;
   Float64 fTop = GetDemand(pgsTypes::TopGirder);
   Float64 fBot = GetDemand(pgsTypes::BottomGirder);

   bPassed = StressedPassed(fBot,pgsTypes::GirderBottom);

   return bPassed;
}

bool pgsFlexuralStressArtifact::StressedPassed(Float64 fStress,pgsTypes::GirderFace face) const
{
   if ( m_StressType == pgsTypes::Compression )
   {
      if ( face == pgsTypes::GirderTop && m_bIsApplicable[pgsTypes::TopGirder] )
      {
         if ( (fStress < m_fAllowable[pgsTypes::TopGirder] && !IsEqual(m_fAllowable[pgsTypes::TopGirder],fStress,0.001)) )
         {
            return false;
         }
      }

      if ( face == pgsTypes::GirderBottom && m_bIsApplicable[pgsTypes::BottomGirder] )
      {
         if ( (fStress < m_fAllowable[pgsTypes::BottomGirder] && !IsEqual(m_fAllowable[pgsTypes::BottomGirder],fStress,0.001)) )
         {
            return false;
         }
      }

      // If top and bottom passed or if top and bottom are not applicable
      return true;
   }
   else
   {
      if (m_AsRequired <= m_AsProvided)
      {
        // If we have adequate rebar, we can use higher limit
         return TensionPassedWithRebar(fStress,face); 
      }
      else
      {
         return TensionPassedWithoutRebar(fStress,face);
      }
   }

   ATLASSERT(false); // should never get here
   return false;
}

bool pgsFlexuralStressArtifact::TensionPassedWithRebar(Float64 fTens,pgsTypes::GirderFace face) const
{
   if ( face == pgsTypes::GirderTop && m_bIsApplicable[pgsTypes::TopGirder] )
   {
      if ( (m_fAltAllowableStress[pgsTypes::GirderTop] < fTens && !IsEqual(m_fAltAllowableStress[pgsTypes::GirderTop],fTens,0.001) ) )
      {
         return false;
      }
   }

   if ( face == pgsTypes::GirderBottom && m_bIsApplicable[pgsTypes::BottomGirder]  )
   {
      if ( (m_fAltAllowableStress[pgsTypes::GirderBottom] < fTens && !IsEqual(m_fAltAllowableStress[pgsTypes::GirderBottom],fTens,0.001) ) )
      {
         return false;
      }
   }

   return true;
}

bool pgsFlexuralStressArtifact::TensionPassedWithoutRebar(Float64 fTens,pgsTypes::GirderFace face) const
{
   if ( face == pgsTypes::GirderTop && m_bIsApplicable[pgsTypes::TopGirder] )
   {
      if ( ( m_fAllowable[pgsTypes::TopGirder] < fTens && !IsEqual(m_fAllowable[pgsTypes::TopGirder],fTens,0.001)) )
      {
         return false;
      }
   }

   if ( face == pgsTypes::GirderBottom && m_bIsApplicable[pgsTypes::BottomGirder] )
   {
      if ( (m_fAllowable[pgsTypes::BottomGirder] < fTens && !IsEqual(m_fAllowable[pgsTypes::BottomGirder],fTens,0.001)) )
      {
         return false;
      }
   }

   // if top and bottom are applicable and both pass... or
   // if top and bottom are not applicable... it can't fail so it must be pass
   return true;
}

bool pgsFlexuralStressArtifact::Passed() const
{
   if ( !TopPassed() )
      return false;

   if ( !BottomPassed() )
      return false;

   return true;
}

Float64 pgsFlexuralStressArtifact::GetCDRatio() const
{
   Float64 cdr_top = GetTopCDRatio();
   Float64 cdr_bot = GetBottomCDRatio();

   Float64 cdr;

   if ( cdr_top < 0 && cdr_bot < 0)
   {
      // top and bottom are both N/A or SKIP
      // Figure out which to return
      if ( cdr_top == cdr_bot )
         return cdr_top;

      if ( m_bIsApplicable[pgsTypes::TopGirder] && !m_bIsApplicable[pgsTypes::BottomGirder] )
         return cdr_top;
      else if ( !m_bIsApplicable[pgsTypes::TopGirder] && m_bIsApplicable[pgsTypes::BottomGirder] )
         return cdr_bot;
      else
         return Min(cdr_top,cdr_bot);
   }
   else if ( 0 <= cdr_top && cdr_bot < 0 )
   {
      // Top only
      cdr = cdr_top;
   }
   else if ( cdr_top < 0 && 0 <= cdr_bot )
   {
      // Bottom only
      cdr = cdr_bot;
   }
   else
   {
      // controlling of top and bottom
      cdr = Min(cdr_top,cdr_bot);
   }

   // deal with case where round-off gives a CD Ratio of 1.0
   // but the spec check doesn't actually pass
   if ( IsEqual(cdr,1.0) && !Passed() )
      cdr = 0.99;

   return cdr;
}

Float64 pgsFlexuralStressArtifact::GetCDRatio(Float64 c,Float64 d) const
{
   if ( ::BinarySign(c) != ::BinarySign(d) )
      return CDR_SKIP; // skip CD Ratio if signs aren't the same

   if ( IsZero(c) && IsZero(d) )
      return CDR_NA; // CD Ratio isn't applicable for 0/0

   if ( IsZero(d) )
      return CDR_INF; // CD Ratio isn't applicable if demand is 0.

   Float64 cdr = c/d;

   // if the actual CD Ratio exceeds the constant that identifies
   // infinity, tweek the value so that 10+ is reported
   if ( CDR_INF <= cdr )
   {
      cdr = CDR_LARGE;
   }

   return cdr;
}

Float64 pgsFlexuralStressArtifact::GetTopCDRatio() const
{
   if ( !m_bIsApplicable[pgsTypes::TopGirder] )
      return CDR_NA;

   Float64 d = m_fDemand[pgsTypes::TopGirder];
   Float64 c;
   if ( WasWithRebarAllowableStressUsed(pgsTypes::GirderTop) )
   {
      c = m_fAltAllowableStress[pgsTypes::GirderTop];
   }
   else
   {
      c = m_fAllowable[pgsTypes::TopGirder];
   }

   return GetCDRatio(c,d);
}

Float64 pgsFlexuralStressArtifact::GetBottomCDRatio() const
{
   if ( !m_bIsApplicable[pgsTypes::BottomGirder] )
      return CDR_NA;

   Float64 d = m_fDemand[pgsTypes::BottomGirder];
   Float64 c;
   if ( WasWithRebarAllowableStressUsed(pgsTypes::GirderBottom) )
   {
      c = m_fAltAllowableStress[pgsTypes::GirderBottom];
   }
   else
   {
      c = m_fAllowable[pgsTypes::BottomGirder];
   }

   return GetCDRatio(c,d);
}

void pgsFlexuralStressArtifact::MakeCopy(const pgsFlexuralStressArtifact& rOther)
{
   m_Poi                 = rOther.m_Poi;
   m_LimitState          = rOther.m_LimitState;
   m_StressType          = rOther.m_StressType;
   m_FcReqd              = rOther.m_FcReqd;

   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = rOther.m_bIsApplicable[stressLocation];
      m_fPretension[stressLocation]   = rOther.m_fPretension[stressLocation];
      m_fPosttension[stressLocation]  = rOther.m_fPosttension[stressLocation];
      m_fExternal[stressLocation]     = rOther.m_fExternal[stressLocation];
      m_fDemand[stressLocation]       = rOther.m_fDemand[stressLocation];
      m_fAllowable[stressLocation]    = rOther.m_fAllowable[stressLocation];
      m_bIsInPTZ[stressLocation]      = rOther.m_bIsInPTZ[stressLocation];
   }

   for ( int i = 0; i < 2; i++ )
   {
      m_Yna[i]                 = rOther.m_Yna[i];
      m_At[i]                  = rOther.m_At[i];
      m_T[i]                   = rOther.m_T[i];
      m_AsProvided[i]          = rOther.m_AsProvided[i];
      m_AsRequired[i]          = rOther.m_AsRequired[i];
      m_fAltAllowableStress[i] = rOther.m_fAltAllowableStress[i];
   }
}

void pgsFlexuralStressArtifact::MakeAssignment(const pgsFlexuralStressArtifact& rOther)
{
   MakeCopy( rOther );
}
