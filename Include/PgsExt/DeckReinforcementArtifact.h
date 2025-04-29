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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\PointOfInterest.h>
#include <map>

/*****************************************************************************
CLASS
   pgsDeckReinforcementCheckArtifact

DESCRIPTION
   Code check artifact for minimum deck reinforcement

LOG
   rdp : 04.16.2025 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDeckReinforcementCheckAtPoisArtifact
{
   public:
      pgsDeckReinforcementCheckAtPoisArtifact() = default;

      pgsDeckReinforcementCheckAtPoisArtifact(const pgsPointOfInterest& POI, Float64 DeckTensileStress, Float64 AreaCIPDeck, Float64 AreaReinforcement, Float64 deckModulusRupture, bool Passed, bool IsApplicable);

      pgsDeckReinforcementCheckAtPoisArtifact(const pgsDeckReinforcementCheckAtPoisArtifact& rOther) = default;
      ~pgsDeckReinforcementCheckAtPoisArtifact() = default;

      pgsDeckReinforcementCheckAtPoisArtifact& operator = (const pgsDeckReinforcementCheckAtPoisArtifact& rOther) = default;

      const pgsPointOfInterest& GetPointOfInterest() const;
      void SetPointOfInterest(const pgsPointOfInterest& poi);

      Float64 GetDeckTensileStress() const;
      void SetDeckTensileStress(Float64 stress);
      Float64 GetAreaCIPDeck() const;
      void SetAreaCIPDeck(Float64 area);
      Float64 GetAreaReinforcement() const;
      void SetAreaReinforcement(Float64 area);
      Float64 GetDeckModulusRupture() const;
      void SetDeckModulusRupture(Float64 mod);

      bool IsApplicable() const;
      void SetApplicability(bool isApplicable);

      bool Passed() const;
      void SetPassed(bool passed);

   private:
      pgsPointOfInterest m_POI;
      Float64 m_DeckTensileStress;
      Float64 m_AreaCIPDeck;
      Float64 m_AreaReinforcement;
      Float64 m_DeckModulusRupture;
      bool m_Passed;
      bool m_IsApplicable;
   };


/*****************************************************************************
CLASS 
   pgsDeckReinforcementCheckArtifact

DESCRIPTION
   Code check artifact for minimum deck reinforcement

LOG
   rdp : 04.16.2025 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDeckReinforcementCheckArtifact
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsDeckReinforcementCheckArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsDeckReinforcementCheckArtifact(const pgsDeckReinforcementCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsDeckReinforcementCheckArtifact();

   //------------------------------------------------------------------------
   // Assignment operator
   pgsDeckReinforcementCheckArtifact& operator = (const pgsDeckReinforcementCheckArtifact& rOther);

   void AddDeckReinforcementCheckAtPoisArtifact(const pgsDeckReinforcementCheckAtPoisArtifact& artifact);
   IndexType GetDeckReinforcementCheckAtPoisArtifactCount() const;
   const pgsDeckReinforcementCheckAtPoisArtifact* GetDeckReinforcementCheckAtPoisArtifact(IndexType Idx) const;

   // Clear out all data
   void Clear();

   // factor is always 0.9 unless we want to change it later
   void SetPhiFactor(Float64);
   Float64 GetPhiFactor() const;

   bool IsApplicable();
   void IsApplicable(bool isApp);

   bool Passed() const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const pgsDeckReinforcementCheckArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsDeckReinforcementCheckArtifact& rOther);

private:

   mutable std::vector<pgsDeckReinforcementCheckAtPoisArtifact> m_DeckReinforcementCheckAtPoisArtifacts;
   bool m_IsApplicable = false;
   Float64 m_PhiFactor = 0.9;
};
