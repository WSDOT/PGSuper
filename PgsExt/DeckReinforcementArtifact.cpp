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
#include <PgsExt\DeckReinforcementArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDeckReinforcementCheckAtPoisArtifact
****************************************************************************/
pgsDeckReinforcementCheckAtPoisArtifact::pgsDeckReinforcementCheckAtPoisArtifact(const pgsPointOfInterest& POI, Float64 DeckTensileStress, Float64 AreaCIPDeck, Float64 AreaReinforcement, Float64 deckModulusRupture, bool Passed, bool IsApplicable) :
   m_POI(POI), m_DeckTensileStress(DeckTensileStress), m_AreaCIPDeck(AreaCIPDeck), m_AreaReinforcement(AreaReinforcement), m_DeckModulusRupture(deckModulusRupture), m_Passed(Passed), m_IsApplicable(IsApplicable)
{
}

const pgsPointOfInterest& pgsDeckReinforcementCheckAtPoisArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

Float64 pgsDeckReinforcementCheckAtPoisArtifact::GetDeckTensileStress() const
{
   return m_DeckTensileStress;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetDeckTensileStress(Float64 stress)
{
   m_DeckTensileStress = stress;
}

Float64 pgsDeckReinforcementCheckAtPoisArtifact::GetAreaCIPDeck() const
{
   return m_AreaCIPDeck;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetAreaCIPDeck(Float64 area)
{
   m_AreaCIPDeck = area;
}

Float64 pgsDeckReinforcementCheckAtPoisArtifact::GetAreaReinforcement() const
{
   return m_AreaReinforcement;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetAreaReinforcement(Float64 area)
{
   m_AreaReinforcement = area;
}

Float64 pgsDeckReinforcementCheckAtPoisArtifact::GetDeckModulusRupture() const
{
   return m_DeckModulusRupture;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetDeckModulusRupture(Float64 mod)
{
   m_DeckModulusRupture = mod;
}

bool pgsDeckReinforcementCheckAtPoisArtifact::IsApplicable() const
{
   return m_IsApplicable;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetApplicability(bool isApplicable)
{
   m_IsApplicable = isApplicable;
}

bool pgsDeckReinforcementCheckAtPoisArtifact::Passed() const
{
   return m_Passed;
}

void pgsDeckReinforcementCheckAtPoisArtifact::SetPassed(bool passed)
{
   m_Passed = passed;
}

/****************************************************************************
CLASS
   pgsDeckReinforcementCheckArtifact
****************************************************************************/

pgsDeckReinforcementCheckArtifact::pgsDeckReinforcementCheckArtifact()
{
}

pgsDeckReinforcementCheckArtifact::pgsDeckReinforcementCheckArtifact(const pgsDeckReinforcementCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsDeckReinforcementCheckArtifact::~pgsDeckReinforcementCheckArtifact()
{
}

pgsDeckReinforcementCheckArtifact& pgsDeckReinforcementCheckArtifact::operator= (const pgsDeckReinforcementCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsDeckReinforcementCheckArtifact::AddDeckReinforcementCheckAtPoisArtifact(const pgsDeckReinforcementCheckAtPoisArtifact& artifact)
{
   m_DeckReinforcementCheckAtPoisArtifacts.push_back(artifact);
}

IndexType pgsDeckReinforcementCheckArtifact::GetDeckReinforcementCheckAtPoisArtifactCount() const
{
   return m_DeckReinforcementCheckAtPoisArtifacts.size();
}

const pgsDeckReinforcementCheckAtPoisArtifact* pgsDeckReinforcementCheckArtifact::GetDeckReinforcementCheckAtPoisArtifact(IndexType index) const
{
   return &m_DeckReinforcementCheckAtPoisArtifacts[index]; 
}

void pgsDeckReinforcementCheckArtifact::Clear()
{
   m_DeckReinforcementCheckAtPoisArtifacts.clear();
}

void pgsDeckReinforcementCheckArtifact::SetPhiFactor(Float64 factor)
{
   m_PhiFactor = factor;
}

Float64 pgsDeckReinforcementCheckArtifact::GetPhiFactor() const
{
   return m_PhiFactor;
}

bool pgsDeckReinforcementCheckArtifact::IsApplicable()
{
   return m_IsApplicable;
}

void pgsDeckReinforcementCheckArtifact::IsApplicable(bool isApp)
{
   m_IsApplicable = isApp;
}

bool pgsDeckReinforcementCheckArtifact::Passed() const
{

   for (const auto& artifact : m_DeckReinforcementCheckAtPoisArtifacts)
   {
      if (!artifact.Passed())
      {
         return false;
      }
   }

   return true;
}

void pgsDeckReinforcementCheckArtifact::MakeCopy(const pgsDeckReinforcementCheckArtifact& rOther)
{
   m_DeckReinforcementCheckAtPoisArtifacts   = rOther.m_DeckReinforcementCheckAtPoisArtifacts;
}

void pgsDeckReinforcementCheckArtifact::MakeAssignment(const pgsDeckReinforcementCheckArtifact& rOther)
{
   MakeCopy( rOther );
}


