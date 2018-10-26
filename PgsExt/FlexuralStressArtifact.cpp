///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
pgsFlexuralStressArtifact::pgsFlexuralStressArtifact()
{
   m_bIsAltTensileStressApplicable = false;
   m_FcReqd = -99999;
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

//======================== OPERATIONS =======================================
void pgsFlexuralStressArtifact::SetPrestressEffects(Float64 fTop,Float64 fBot)
{
   m_fTopPrestress = fTop;
   m_fBotPrestress = fBot;
}

void pgsFlexuralStressArtifact::GetPrestressEffects(Float64* pfTop,Float64* pfBot) const
{
   *pfTop = m_fTopPrestress;
   *pfBot = m_fBotPrestress;
}

void pgsFlexuralStressArtifact::SetExternalEffects(Float64 fTop,Float64 fBot)
{
   m_fTopExternal = fTop;
   m_fBotExternal = fBot;
}

void pgsFlexuralStressArtifact::GetExternalEffects(Float64* pfTop,Float64* pfBot) const
{
   *pfTop = m_fTopExternal;
   *pfBot = m_fBotExternal;
}

void pgsFlexuralStressArtifact::SetDemand(Float64 fTop,Float64 fBot)
{
   m_fTopDemand = fTop;
   m_fBotDemand = fBot;
}

void pgsFlexuralStressArtifact::GetDemand(Float64* pfTop,Float64* pfBot) const
{
   *pfTop = m_fTopDemand;
   *pfBot = m_fBotDemand;
}

void pgsFlexuralStressArtifact::SetCapacity(Float64 fAllowable,pgsTypes::StressType stressType)
{
   m_fAllowableStress = fAllowable;
   m_StressType = stressType;
}

Float64 pgsFlexuralStressArtifact::GetCapacity() const
{
   return m_fAllowableStress;
}

pgsTypes::StressType pgsFlexuralStressArtifact::GetStressType() const
{
   return m_StressType;
}

void pgsFlexuralStressArtifact::SetRequiredConcreteStrength(double fcReqd)
{
   m_FcReqd = fcReqd;
}

double pgsFlexuralStressArtifact::GetRequiredConcreteStrength() const
{
   return m_FcReqd;
}

void pgsFlexuralStressArtifact::IsAlternativeTensileStressApplicable(bool bApplicable)
{
   m_bIsAltTensileStressApplicable = bApplicable;
}

bool pgsFlexuralStressArtifact::IsAlternativeTensileStressApplicable() const
{
   return m_bIsAltTensileStressApplicable;
}

void pgsFlexuralStressArtifact::SetAlternativeTensileStressParameters(double Yna,double At,double T,double As,double fAllow)
{
   m_Yna = Yna;
   m_At = At;
   m_T = T;
   m_As = As;
   m_fAltAllowableStress = fAllow;
}

void pgsFlexuralStressArtifact::GetAlternativeTensileStressParameters(double* Yna,double* At,double* T,double* As) const
{
   *Yna = m_Yna;
   *At = m_At;
   *T = m_T;
   *As = m_As;
}

bool pgsFlexuralStressArtifact::TopPassed() const
{
   bool bPassed = true;
   Float64 fTop, fBot;
   GetDemand(&fTop,&fBot);

   if ( m_StressType == pgsTypes::Compression )
   {
      if ( (fTop < m_fAllowableStress && !IsEqual(m_fAllowableStress,fTop,0.001))  )
      {
         bPassed = false;
      }
   }
   else
   {
      bPassed = TensionPassed(fTop);
   }

   return bPassed;
}

bool pgsFlexuralStressArtifact::BottomPassed() const
{
   bool bPassed = true;
   Float64 fTop, fBot;
   GetDemand(&fTop,&fBot);

   if ( m_StressType == pgsTypes::Compression )
   {
      if ( (fBot < m_fAllowableStress && !IsEqual(m_fAllowableStress,fBot,0.001)) )
      {
         bPassed = false;
      }
   }
   else
   {
      bPassed = TensionPassed(fBot);
   }

   return bPassed;
}

bool pgsFlexuralStressArtifact::TensionPassed(double fTens) const
{
   bool bPassed = true;
   if ( (m_fAllowableStress < fTens && !IsEqual(m_fAllowableStress,fTens,0.001) ) )
   {
      bPassed = false;
   }

   if ( m_bIsAltTensileStressApplicable )
   {
      bPassed = true;
      if ( (m_fAltAllowableStress < fTens && !IsEqual(m_fAltAllowableStress,fTens,0.001) ) )
      {
         bPassed = false;
      }
   }

   return bPassed;
}

bool pgsFlexuralStressArtifact::Passed() const
{
   // Casting Yard,               Tension,     top and bottom
   // Casting Yard,               Compression, top and bottom
   // Temporary Strand Removal,   Tension,     top and bottom
   // Temporary Strand Removal,   Compression, top and bottom
   // Bridge Site 1,              Tension,     top and bottom
   // Bridge Site 1,              Compression, top and bottom
   // Bridge Site 2,              Compression, top and bottom
   // Bridge Site 3, Service I,   Compression, top and bottom
   // Bridge Site 3, Service IA,  Compression, top and bottom
   // Bridge Site 3, Service III, Tension,     bottom

   bool bPassed = false;
   switch( m_Key.GetStage() )
   {
   case pgsTypes::CastingYard:
//   case pgsTypes::GirderPlacement:
   case pgsTypes::TemporaryStrandRemoval:
   case pgsTypes::BridgeSite1:
   case pgsTypes::BridgeSite2:
      bPassed = (TopPassed() && BottomPassed());
      break;

   case pgsTypes::BridgeSite3:
      switch( m_Key.GetLimitState() )
      {
      case pgsTypes::ServiceI:
      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
         bPassed = (TopPassed() && BottomPassed());
         break;

      case pgsTypes::ServiceIII:
      bPassed = BottomPassed();
         break;
      }
      break;

      default:
         ATLASSERT(false);
   }
   return bPassed;
}

 //======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsFlexuralStressArtifact::MakeCopy(const pgsFlexuralStressArtifact& rOther)
{
   m_fTopPrestress    = rOther.m_fTopPrestress;
   m_fBotPrestress    = rOther.m_fBotPrestress;
   m_fTopExternal     = rOther.m_fTopExternal;
   m_fBotExternal     = rOther.m_fBotExternal;
   m_fTopDemand       = rOther.m_fTopDemand;
   m_fBotDemand       = rOther.m_fBotDemand;
   m_fAllowableStress = rOther.m_fAllowableStress;
   m_fAltAllowableStress = rOther.m_fAltAllowableStress;
   m_StressType       = rOther.m_StressType;
   m_bIsAltTensileStressApplicable = rOther.m_bIsAltTensileStressApplicable;
   m_Yna = rOther.m_Yna;
   m_At  = rOther.m_At;
   m_T   = rOther.m_T;
   m_As  = rOther.m_As;
   m_FcReqd = rOther.m_FcReqd;
   m_Key = rOther.m_Key;
}

void pgsFlexuralStressArtifact::MakeAssignment(const pgsFlexuralStressArtifact& rOther)
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
