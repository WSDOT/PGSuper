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

#ifndef INCLUDED_PGSEXT_GIRDERARTIFACT_H_
#define INCLUDED_PGSEXT_GIRDERARTIFACT_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_MAP_
#include <map>
#define INCLUDED_MAP_
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_PGSEXT_STRANDSTRESSARTIFACT_H_
#include <PgsExt\StrandStressArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_FLEXURALSTRESSARTIFACT_H_
#include <PgsExt\FlexuralStressArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_FLEXURALCAPACITYARTIFACT_H_
#include <PgsExt\FlexuralCapacityArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_STIRRUPCHECKARTIFACT_H_
#include <PGSExt\StirrupCheckArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_POIARTIFACTKEY_H_
#include <PgsExt\PoiArtifactKey.h>
#endif

#if !defined INCLUDED_PGSEXT_STRANDSLOPEARTIFACT_H_
#include <PgsExt\StrandSlopeArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_HOLDDOWNFORCEARTIFACT_H_
#include <PgsExt\HoldDownForceArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
#include <PgsExt\ConstructabilityArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_PRECASTIGIRDERDETAILINGARTIFACT_H_
#include <PgsExt\PrecastIGirderDetailingArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_HAULINGCHECKARTIFACT_H_
#include <PgsExt\HaulingCheckArtifact.h>
#endif

#if !defined INCLUDED_PGSEXT_LIFTINGCHECKARTIFACT_H_
#include <PgsExt\LiftingCheckArtifact.h>
#endif

#include <PgsExt\DebondArtifact.h>

#include <PgsExt\StirrupCheckAtZonesArtifact.h>

#include <PgsExt\DeflectionCheckArtifact.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsGirderArtifact

   Code check artifact for a prestressed girder.


DESCRIPTION
   Code check artifact for a prestressed girder.  


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 10.28.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsGirderArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsGirderArtifact(SpanIndexType spanIdx,GirderIndexType gdrIdx);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsGirderArtifact(const pgsGirderArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsGirderArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsGirderArtifact& operator = (const pgsGirderArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Sets the strand stress artifact for this artifact
   void SetStrandStressArtifact(const pgsStrandStressArtifact& artifact);

   //------------------------------------------------------------------------
   // Returns a pointer to the strand stress artifact.
   const pgsStrandStressArtifact* GetStrandStressArtifact() const;
   pgsStrandStressArtifact* GetStrandStressArtifact();

   void SetStrandSlopeArtifact(const pgsStrandSlopeArtifact& artifact);
   const pgsStrandSlopeArtifact* GetStrandSlopeArtifact() const;
   pgsStrandSlopeArtifact* GetStrandSlopeArtifact();

   void SetHoldDownForceArtifact(const pgsHoldDownForceArtifact& artifact);
   const pgsHoldDownForceArtifact* GetHoldDownForceArtifact() const;
   pgsHoldDownForceArtifact* GetHoldDownForceArtifact();

   void AddFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key,
                                  const pgsFlexuralStressArtifact& artifact);

   const pgsFlexuralStressArtifact* GetFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key) const;
   pgsFlexuralStressArtifact* GetFlexuralStressArtifact(const pgsFlexuralStressArtifactKey& key);
   std::vector<pgsFlexuralStressArtifactKey> GetFlexuralStressArtifactKeys() const;

   void AddFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key,
                                    const pgsFlexuralCapacityArtifact& pmartifact,
                                    const pgsFlexuralCapacityArtifact& nmartifact);

   std::vector<pgsFlexuralCapacityArtifactKey> GetFlexuralCapacityArtifactKeys() const;
   const pgsFlexuralCapacityArtifact* GetPositiveMomentFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key) const;
   const pgsFlexuralCapacityArtifact* GetNegativeMomentFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifactKey& key) const;

   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();
   const pgsStirrupCheckArtifact* GetStirrupCheckArtifact() const;

   pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact();
   const pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact() const;

   void SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact);
   const pgsConstructabilityArtifact* GetConstructabilityArtifact() const;
   pgsConstructabilityArtifact* GetConstructabilityArtifact();

   void SetLiftingCheckArtifact(pgsLiftingCheckArtifact* artifact);
   const pgsLiftingCheckArtifact* GetLiftingCheckArtifact() const;
   
   void SetHaulingCheckArtifact(pgsHaulingCheckArtifact*  artifact);
   const pgsHaulingCheckArtifact* GetHaulingCheckArtifact() const;

   pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact();
   const pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact() const;

   void SetCastingYardMildRebarRequirement(Float64 As);
   Float64 GetCastingYardMildRebarRequirement() const;

   void SetCastingYardCapacityWithMildRebar(Float64 fAllow);
   Float64 GetCastingYardCapacityWithMildRebar() const;

   pgsDebondArtifact* GetDebondArtifact(pgsTypes::StrandType strandType);
   const pgsDebondArtifact* GetDebondArtifact(pgsTypes::StrandType strandType) const;
   
   bool Passed() const;

   double GetRequiredConcreteStrength(pgsTypes::Stage stage,pgsTypes::LimitState ls) const;
   double GetRequiredConcreteStrength() const;
   double GetRequiredReleaseStrength() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsGirderArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsGirderArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   SpanIndexType m_SpanIdx;
   GirderIndexType m_GirderIdx;

   pgsStrandStressArtifact m_StrandStressArtifact;
   pgsStrandSlopeArtifact m_StrandSlopeArtifact;
   pgsHoldDownForceArtifact m_HoldDownForceArtifact;
   pgsConstructabilityArtifact m_ConstructabilityArtifact;

   std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact> m_FlexuralStressArtifacts;
   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact> m_PositiveMomentFlexuralCapacityArtifacts;
   std::map<pgsFlexuralCapacityArtifactKey,pgsFlexuralCapacityArtifact> m_NegativeMomentFlexuralCapacityArtifacts;

   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   pgsPrecastIGirderDetailingArtifact m_PrecastIGirderDetailingArtifact;

   std::auto_ptr<pgsLiftingCheckArtifact> m_pLiftingCheckArtifact;
   std::auto_ptr<pgsHaulingCheckArtifact> m_pHaulingCheckArtifact;

   pgsDeflectionCheckArtifact m_DeflectionCheckArtifact;

   pgsDebondArtifact m_DebondArtifact[3];

   Float64 m_CastingYardAs; // required area of steel for casting yard tension stresses
   Float64 m_CastingYardAllowable; // allowable tensile stress for casting yard with required mild rebar
};

#endif // INCLUDED_PGSEXT_GIRDERARTIFACT_H_
