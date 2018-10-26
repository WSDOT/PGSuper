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

#ifndef INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
#define INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsFlexuralStressArtifact

   Artifact for flexural stress checks


DESCRIPTION
   Artifact for flexural stress checks.  Records the demand, capacity, and
   outcome of a flexural stress check.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.17.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsFlexuralStressArtifact
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsFlexuralStressArtifact();
   pgsFlexuralStressArtifact(const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsFlexuralStressArtifact(const pgsFlexuralStressArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsFlexuralStressArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsFlexuralStressArtifact& operator = (const pgsFlexuralStressArtifact& rOther);
   bool operator<(const pgsFlexuralStressArtifact& rOther) const;

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   //------------------------------------------------------------------------
   // Set/Get the top/bottom girder stresses due to pre-tensioning
   void SetPretensionEffects(Float64 fTop,Float64 fBot);
   void GetPretensionEffects(Float64* pfTop,Float64* pfBot) const;

   // Set/Get the top/bottom girder stresses due to post-tensioning
   void SetPosttensionEffects(Float64 fTop,Float64 fBot);
   void GetPosttensionEffects(Float64* pfTop,Float64* pfBot) const;

   // Set/Get the top/bottom girder stresses due to externally applied loads
   void SetExternalEffects(Float64 fTop,Float64 fBot);
   void GetExternalEffects(Float64* pfTop,Float64* pfBot) const;

   // Set/Get the top/bottom girder stresses demand (external loads + pre-tension + post-tension)
   void SetDemand(Float64 fTop,Float64 fBot);
   void GetDemand(Float64* pfTop,Float64* pfBot) const;

   // Set/Get the allowable stress "capacity"
   void SetCapacity(Float64 fAllowable,pgsTypes::LimitState ls,pgsTypes::StressType stressType);
   Float64 GetCapacity() const;
   pgsTypes::LimitState GetLimitState() const;
   pgsTypes::StressType GetStressType() const;

   void SetRequiredConcreteStrength(Float64 fcReqd);
   Float64 GetRequiredConcreteStrength() const;

   void SetAlternativeTensileStressParameters(Float64 Yna,Float64 At,Float64 T,Float64 AsProvided,Float64 AsRequired,Float64 fHigherAllow);
   void GetAlternativeTensileStressParameters(Float64* Yna,Float64* At,Float64* T,Float64* AsProvided,Float64* AsRequired) const;
   bool WasHigherAllowableStressUsed() const;

   bool TopPassed() const;
   bool BottomPassed() const;
   bool Passed() const;


protected:
   void MakeCopy(const pgsFlexuralStressArtifact& rOther);
   virtual void MakeAssignment(const pgsFlexuralStressArtifact& rOther);

private:
   pgsPointOfInterest m_Poi;

   // Stresses caused by pre-tensioning
   Float64 m_fTopPretension;
   Float64 m_fBotPretension;

   // Stresses caused by post-tensioning
   Float64 m_fTopPosttension;
   Float64 m_fBotPosttension;

   // Stresses caused by externally applied loads
   Float64 m_fTopExternal;
   Float64 m_fBotExternal;

   // Demand
   Float64 m_fTopDemand;
   Float64 m_fBotDemand;

   // Allowable stresses
   Float64 m_fAllowableStress;
   pgsTypes::LimitState m_LimitState;
   pgsTypes::StressType m_StressType;

   // Alternative tensile stress parameters
   bool   m_bIsAltTensileStressApplicable;
   Float64 m_Yna;
   Float64 m_At;
   Float64 m_T;
   Float64 m_AsProvided;
   Float64 m_AsRequired;
   Float64 m_fAltAllowableStress;

   // Other
   Float64 m_FcReqd; // concrete strenght required to satisfy allowable for this section
                    // No concrete strength work if < 0

   bool StressedPassed(Float64 fStress) const;
   bool TensionPassedWithRebar(Float64 fTens) const;
   bool TensionPassedWithoutRebar(Float64 fTens) const;
};

#endif // INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
