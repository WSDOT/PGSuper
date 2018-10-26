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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SegmentKey.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\FlexuralCapacityArtifact.h>
#include <PGSExt\StirrupCheckArtifact.h>
#include <PgsExt\PoiArtifactKey.h>
#include <PgsExt\ConstructabilityArtifact.h>
#include <PgsExt\PrecastIGirderDetailingArtifact.h>
#include <PgsExt\StirrupCheckAtZonesArtifact.h>
#include <PgsExt\DeflectionCheckArtifact.h>

#include <map>


/*****************************************************************************
   pgsClosurePourArtifact

   Code check artifact for a cast-in-place closure (or match cast joint).
*****************************************************************************/
class PGSEXTCLASS pgsClosurePourArtifact
{
public:
   pgsClosurePourArtifact(const CSegmentKey& closurePourKey);
   pgsClosurePourArtifact(const pgsClosurePourArtifact& rOther);
   virtual ~pgsClosurePourArtifact();

   pgsClosurePourArtifact& operator = (const pgsClosurePourArtifact& rOther);
   bool operator<(const pgsClosurePourArtifact& rOther) const;


   // artifact for stress check at CL of closure
   void SetFlexuralStressArtifact(pgsTypes::StressType stressType,const pgsFlexuralStressArtifact& artifact);
   const pgsFlexuralStressArtifact* GetFlexuralStressArtifact(pgsTypes::StressType stressType) const;
   pgsFlexuralStressArtifact* GetFlexuralStressArtifact(pgsTypes::StressType stressType);

   // moment capacity
   void SetFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifact& pmartifact,const pgsFlexuralCapacityArtifact& nmartifact);
   const pgsFlexuralCapacityArtifact* GetPositiveMomentFlexuralCapacityArtifact() const;
   const pgsFlexuralCapacityArtifact* GetNegativeMomentFlexuralCapacityArtifact() const;

   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();
   const pgsStirrupCheckArtifact* GetStirrupCheckArtifact() const;

   //pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact();
   //const pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact() const;

   //void SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact);
   //const pgsConstructabilityArtifact* GetConstructabilityArtifact() const;
   //pgsConstructabilityArtifact* GetConstructabilityArtifact();


   pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact();
   const pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact() const;
   
   bool Passed() const;

   bool DidFlexuralStressesPass() const;

   Float64 GetRequiredConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   Float64 GetRequiredConcreteStrength() const;
   Float64 GetRequiredReleaseStrength() const;

   const CSegmentKey& GetClosurePourKey() const;

protected:
   void MakeCopy(const pgsClosurePourArtifact& rOther);
   virtual void MakeAssignment(const pgsClosurePourArtifact& rOther);


private:
   CSegmentKey m_ClosurePourKey;

   //pgsConstructabilityArtifact m_ConstructabilityArtifact;

   pgsFlexuralStressArtifact m_FlexuralStressArtifact[2]; // index is pgsTypes::StressType enum value
   pgsFlexuralCapacityArtifact m_PositiveMomentFlexuralCapacityArtifact;
   pgsFlexuralCapacityArtifact m_NegativeMomentFlexuralCapacityArtifact;

   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   //pgsPrecastIGirderDetailingArtifact m_PrecastIGirderDetailingArtifact;

   //pgsDeflectionCheckArtifact m_DeflectionCheckArtifact;
};
