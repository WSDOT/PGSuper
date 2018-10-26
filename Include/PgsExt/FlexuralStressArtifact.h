///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

   pgsFlexuralStressArtifact& operator = (const pgsFlexuralStressArtifact& rOther);
   bool operator<(const pgsFlexuralStressArtifact& rOther) const;

   // Set/Get the point of interest where this stress check occurs
   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   // Set/Get Limit State used in this stress check
   void SetLimitState(pgsTypes::LimitState limitState);
   pgsTypes::LimitState GetLimitState() const;

   // Set/Get the type of stress check that was performed
   void SetStressType(pgsTypes::StressType stressType);
   pgsTypes::StressType GetStressType() const;

   // Set/Get applicability of this stress check
   void IsApplicable(pgsTypes::StressLocation stressLocation,bool bIsApplicable);
   bool IsApplicable(pgsTypes::StressLocation stressLocation) const;

   // Set/Get the top/bottom girder stresses due to pre-tensioning
   void SetPretensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPS);
   Float64 GetPretensionEffects(pgsTypes::StressLocation stressLocation) const;

   // Set/Get the top/bottom girder stresses due to post-tensioning
   void SetPosttensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPT);
   Float64 GetPosttensionEffects(pgsTypes::StressLocation stressLocation) const;

   // Set/Get the top/bottom girder stresses due to externally applied loads
   void SetExternalEffects(pgsTypes::StressLocation stressLocation,Float64 f);
   Float64 GetExternalEffects(pgsTypes::StressLocation stressLocation) const;

   // Set/Get the top/bottom girder stresses demand (external loads + pre-tension + post-tension)
   void SetDemand(pgsTypes::StressLocation stressLocation,Float64 fDemand);
   Float64 GetDemand(pgsTypes::StressLocation stressLocation) const;

   // Indicates if this point is in the precompressed tensile zone
   void IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation,bool bIsInPTZ);
   bool IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation) const;

   // Set/Get the allowable stress "capacity"
   void SetCapacity(pgsTypes::StressLocation stressLocation,Float64 fAllowable);
   Float64 GetCapacity(pgsTypes::StressLocation stressLocation) const;

   void SetRequiredConcreteStrength(pgsTypes::StressLocation stressLocation,Float64 fcReqd);
   Float64 GetRequiredConcreteStrength(pgsTypes::StressLocation stressLocation) const;
   Float64 GetRequiredBeamConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength() const;

   void SetAlternativeTensileStressParameters(pgsTypes::StressLocation stressLocation,Float64 Yna,Float64 At,Float64 T,Float64 AsProvided,Float64 AsRequired,Float64 fAllowableWithRebar);
   void GetAlternativeTensileStressParameters(pgsTypes::StressLocation stressLocation,Float64* Yna,Float64* At,Float64* T,Float64* AsProvided,Float64* AsRequired) const;
   Float64 GetAlternativeAllowableTensileStress(pgsTypes::StressLocation stressLocation) const;
   bool IsWithRebarAllowableStressApplicable(pgsTypes::StressLocation stressLocation) const;
   bool WasWithRebarAllowableStressUsed(pgsTypes::StressLocation stressLocation) const;

   bool Passed(pgsTypes::StressLocation stressLocation) const;
   bool BeamPassed() const;
   bool DeckPassed() const;

   Float64 GetCDRatio(pgsTypes::StressLocation stressLocation) const;
   Float64 GetBeamCDRatio() const;
   Float64 GetDeckCDRatio() const;


protected:
   void MakeCopy(const pgsFlexuralStressArtifact& rOther);
   virtual void MakeAssignment(const pgsFlexuralStressArtifact& rOther);

private:
   pgsPointOfInterest m_Poi;

   // In the following arrays, use the pgsTypes::StressLocation enum to access the values
   bool    m_bIsApplicable[4]; // Applicability of the stress check
   Float64 m_fPretension[4];   // Stresses caused by pre-tensioning
   Float64 m_fPosttension[4];  // Stresses caused by post-tensioning
   Float64 m_fExternal[4];     // Stresses caused by externally applied loads
   bool    m_bIsInPTZ[4];      // Is the location in the Precompressed Tensile Zone
   Float64 m_fDemand[4];       // Total stress demand
   Float64 m_fAllowable[4];    // Allowable stresses

   pgsTypes::LimitState m_LimitState;
   pgsTypes::StressType m_StressType;

   // Alternative tensile stress parameters
   // access array with pgsTypes::StressLocation constant
   bool   m_bIsAltTensileStressApplicable[4];
   Float64 m_Yna[4];
   Float64 m_At[4];
   Float64 m_T[4];
   Float64 m_AsProvided[4];
   Float64 m_AsRequired[4];
   Float64 m_fAltAllowableStress[4];

   // Other
   Float64 m_FcReqd[4]; // concrete strenght required to satisfy allowable for this section
                        // No concrete strength work if < 0

   bool StressedPassed(pgsTypes::StressLocation stressLocation) const;
   bool TensionPassedWithRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const;
   bool TensionPassedWithoutRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const;

   Float64 GetCDRatio(Float64 c,Float64 d) const;
   Float64 GetCDRatio(pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation) const;
};

#endif // INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
