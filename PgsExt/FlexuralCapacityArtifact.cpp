///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PgsExt\FlexuralCapacityArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsFlexuralCapacityArtifact
****************************************************************************/


pgsFlexuralCapacityArtifact::pgsFlexuralCapacityArtifact(bool bPositiveMoment)
{
   m_bPositiveMoment = bPositiveMoment;
   m_cde = 0;
   m_cdeMax = 0.42;
   m_Mu = 0;
   m_Mr = 0;
   m_MrMin = 0;
}

pgsFlexuralCapacityArtifact::pgsFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsFlexuralCapacityArtifact::~pgsFlexuralCapacityArtifact()
{
}

//======================== OPERATORS  =======================================
pgsFlexuralCapacityArtifact& pgsFlexuralCapacityArtifact::operator=(const pgsFlexuralCapacityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsFlexuralCapacityArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& pgsFlexuralCapacityArtifact::GetPointOfInterest() const
{
   return m_Poi;
}

void pgsFlexuralCapacityArtifact::SetMaxReinforcementRatio(Float64 cde)
{
   m_cde = cde;
}

Float64 pgsFlexuralCapacityArtifact::GetMaxReinforcementRatio() const
{
   return m_cde;
}

void pgsFlexuralCapacityArtifact::SetMaxReinforcementRatioLimit(Float64 cdeMax)
{
   m_cdeMax = cdeMax;
}

Float64 pgsFlexuralCapacityArtifact::GetMaxReinforcementRatioLimit() const
{
   return m_cdeMax;
}

void pgsFlexuralCapacityArtifact::SetMinCapacity(Float64 MrMin)
{
   m_MrMin = MrMin;
}

Float64 pgsFlexuralCapacityArtifact::GetMinCapacity() const
{
   return m_MrMin;
}

void pgsFlexuralCapacityArtifact::SetDemand(Float64 Mu)
{
   m_Mu = IsZero(Mu) ? 0 : Mu;
}

Float64 pgsFlexuralCapacityArtifact::GetDemand() const
{
   return m_Mu;
}

void pgsFlexuralCapacityArtifact::SetCapacity(Float64 Mr)
{
   m_Mr = Mr;
}

Float64 pgsFlexuralCapacityArtifact::GetCapacity() const
{
   return m_Mr;
}

bool pgsFlexuralCapacityArtifact::IsOverReinforced() const
{
   return ( m_cde > m_cdeMax ) ? true : false;
}

bool pgsFlexuralCapacityArtifact::IsUnderReinforced() const
{
   if ( m_bPositiveMoment )
   {
      return IsLT(m_Mr,m_MrMin) ? true : false;
   }
   else
   {
      return IsGT(m_MrMin,m_Mr) ? true : false;
   }
}

bool pgsFlexuralCapacityArtifact::CapacityPassed() const
{
   if ( m_bPositiveMoment )
   {
      return IsLE(m_Mu,m_Mr) ? true : false;
   }
   else
   {
      return IsGE(m_Mr,m_Mu) ? true : false;
   }
}

bool pgsFlexuralCapacityArtifact::Passed() const
{
   if ( IsOverReinforced() )
   {
      return false;
   }

   if ( IsUnderReinforced() )
   {
      return false;
   }

   if ( !CapacityPassed() )
   {
      return false;
   }

   return true;
}

void pgsFlexuralCapacityArtifact::MakeCopy(const pgsFlexuralCapacityArtifact& rOther)
{
   m_Poi    = rOther.m_Poi;
   m_cde    = rOther.m_cde;
   m_cdeMax = rOther.m_cdeMax;
   m_MrMin  = rOther.m_MrMin;
   m_Mu     = rOther.m_Mu;
   m_Mr     = rOther.m_Mr;
   m_bPositiveMoment = rOther.m_bPositiveMoment;
}

void pgsFlexuralCapacityArtifact::MakeAssignment(const pgsFlexuralCapacityArtifact& rOther)
{
   MakeCopy( rOther );
}
