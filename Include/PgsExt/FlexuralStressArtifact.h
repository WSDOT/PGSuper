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

#include <PgsExt\PoiArtifactKey.h>

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
   // GROUP: LIFECYCLE
   enum TensionReinforcement { WithRebar, WithoutRebar };

   //------------------------------------------------------------------------
   // Default constructor
   pgsFlexuralStressArtifact();

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

   // GROUP: OPERATIONS

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   void SetPrestressEffects(Float64 fTop,Float64 fBot);
   void GetPrestressEffects(Float64* pfTop,Float64* pfBot) const;
   void SetExternalEffects(Float64 fTop,Float64 fBot);
   void GetExternalEffects(Float64* pfTop,Float64* pfBot) const;
   void SetDemand(Float64 fTop,Float64 fBot);
   void GetDemand(Float64* pfTop,Float64* pfBot) const;
   void SetCapacity(Float64 fAllowable,pgsTypes::StressType stressType);
   Float64 GetCapacity() const;
   pgsTypes::StressType GetStressType() const;
   void SetRequiredConcreteStrength(double fcReqd);
   double GetRequiredConcreteStrength() const;

   void IsAlternativeTensileStressApplicable(bool bApplicable);
   bool IsAlternativeTensileStressApplicable() const;
   void SetAlternativeTensileStressParameters(double Yna,double At,double T,double As,double fAllow);
   void GetAlternativeTensileStressParameters(double* Yna,double* At,double* T,double* As) const;

   bool TopPassed(TensionReinforcement reinfType) const;
   bool BottomPassed(TensionReinforcement reinfType) const;
   bool Passed(TensionReinforcement reinfType) const;

   void SetKey(const pgsFlexuralStressArtifactKey& key){m_Key = key;}

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsFlexuralStressArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsFlexuralStressArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   // Stresses caused by prestressing
   Float64 m_fTopPrestress;
   Float64 m_fBotPrestress;

   // Stresses caused by externally applied loads
   Float64 m_fTopExternal;
   Float64 m_fBotExternal;

   // Demand
   Float64 m_fTopDemand;
   Float64 m_fBotDemand;

   // Allowable stresses
   Float64 m_fAllowableStress;
   pgsTypes::StressType m_StressType;

   // Alternative tensile stress parameters
   bool   m_bIsAltTensileStressApplicable;
   double m_Yna;
   double m_At;
   double m_T;
   double m_As;
   double m_fAltAllowableStress;

   // Other
   double m_FcReqd; // concrete strenght required to satisfy allowable for this section
                    // No concrete strength work if < 0
   pgsFlexuralStressArtifactKey m_Key;

   bool TensionPassedWithRebar(double fTens) const;
   bool TensionPassedWithoutRebar(double fTens) const;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
