///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <map>
#include <PGSExt\StirrupCheckAtPoisArtifact.h>
#include <PGSExt\StirrupCheckAtZonesArtifact.h>

/*****************************************************************************
CLASS 
   pgsStirrupCheckArtifact

   Code check artifact for stirrups


DESCRIPTION
   Code check artifact for stirrups

LOG
   rab : 10.28.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStirrupCheckArtifact
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsStirrupCheckArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStirrupCheckArtifact(const pgsStirrupCheckArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStirrupCheckArtifact();

   //------------------------------------------------------------------------
   // Assignment operator
   pgsStirrupCheckArtifact& operator = (const pgsStirrupCheckArtifact& rOther);


   void AddStirrupCheckAtPoisArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsStirrupCheckAtPoisArtifact& artifact);
   CollectionIndexType GetStirrupCheckAtPoisArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   const pgsStirrupCheckAtPoisArtifact* GetStirrupCheckAtPoisArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType index) const;
   const pgsStirrupCheckAtPoisArtifact* GetStirrupCheckAtPoisArtifactAtPOI(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PoiIDType poiID) const;

   // confinement check
   void SetConfinementArtifact(const pgsConfinementArtifact& artifact);
   const pgsConfinementArtifact& GetConfinementArtifact() const;

   // splitting check
   pgsSplittingZoneArtifact* GetSplittingZoneArtifact();
   const pgsSplittingZoneArtifact* GetSplittingZoneArtifact() const;

   // Clear out all data
   void Clear();

   bool Passed() const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStirrupCheckArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStirrupCheckArtifact& rOther);

private:
   struct Key
   {
      IntervalIndexType intervalIdx;
      pgsTypes::LimitState ls;
      bool operator<(const Key& other) const
      {
         if ( intervalIdx < other.intervalIdx )
            return true;

         if ( other.intervalIdx < intervalIdx )
            return false;

         if (ls < other.ls)
            return true;

         if ( other.ls < ls)
            return false;

         return false;
      }
   };

   mutable std::map<Key,std::vector<pgsStirrupCheckAtPoisArtifact>> m_StirrupCheckAtPoisArtifacts;
   std::vector<pgsStirrupCheckAtPoisArtifact>& GetStirrupCheckArtifacts(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;

   pgsConfinementArtifact m_ConfinementArtifact;

   pgsSplittingZoneArtifact m_SplittingZoneArtifact;
};
