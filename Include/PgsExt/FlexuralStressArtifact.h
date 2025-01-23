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

#include <WBFLGenericBridgeTools\AlternativeTensileStressCalculator.h>


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
   pgsFlexuralStressArtifact(const pgsPointOfInterest& poi,const StressCheckTask& task);

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

   void SetTask(const StressCheckTask& task);
   const StressCheckTask& GetTask() const;

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
   void SetCapacity(pgsTypes::StressLocation stressLocation,Float64 fLimit);
   Float64 GetCapacity(pgsTypes::StressLocation stressLocation) const;

   void SetRequiredConcreteStrength(pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation,Float64 fcReqd);
   Float64 GetRequiredConcreteStrength(pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation) const;
   Float64 GetRequiredBeamConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength() const;

   void SetAlternativeTensileStressRequirements(pgsTypes::StressLocation stressLocation, const gbtAlternativeTensileStressRequirements& requirements,Float64 fHigherAllowable,bool bBiaxialStresses);
   const gbtAlternativeTensileStressRequirements& GetAlternativeTensileStressRequirements(pgsTypes::StressLocation stressLocation) const;
   bool BiaxialStresses(pgsTypes::StressLocation stressLocation) const;
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
   void MakeAssignment(const pgsFlexuralStressArtifact& rOther);

private:
   pgsPointOfInterest m_Poi;
   StressCheckTask m_Task;

   // In the following arrays, use the pgsTypes::StressLocation enum to access the values
   std::array<bool,4>    m_bIsApplicable; // Applicability of the stress check
   std::array<Float64,4> m_fPretension;   // Stresses caused by pre-tensioning
   std::array<Float64,4> m_fPosttension;  // Stresses caused by post-tensioning
   std::array<Float64,4> m_fExternal;     // Stresses caused by externally applied loads
   std::array<bool,4>    m_bIsInPTZ;      // Is the location in the Precompressed Tensile Zone
   std::array<Float64,4> m_fDemand;       // Total stress demand
   std::array<Float64,4> m_fLimit;    // Allowable stresses

   // Alternative tensile stress parameters
   // access array with pgsTypes::StressLocation constant
   std::array<bool, 4> m_bIsAltTensileStressApplicable;
   std::array<Float64, 4> m_fAltAllowableStress;
   std::array<gbtAlternativeTensileStressRequirements, 4> m_AltTensileStressRequirements;
   std::array<bool, 4> m_bBiaxialStresses; // if true, the alternative tensile stress requirements are reported for biaxial stresses

   // Other
   std::array<std::array<Float64,4>, 2> m_FcReqd; // array index is m_FcReqd[pgsTypes::StressType][pgsTypes::StressLocation]
   // concrete strength required to satisfy allowable at this section
   // If no concrete strength work store a negative value
   // Store a value of zero when stress limits are not a function of concrete strength

   bool StressedPassed(pgsTypes::StressLocation stressLocation) const;
   bool TensionPassedWithRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const;
   bool TensionPassedWithoutRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const;

   Float64 GetCDRatio(Float64 c,Float64 d) const;
   Float64 GetCDRatio(pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation) const;
};

#endif // INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
