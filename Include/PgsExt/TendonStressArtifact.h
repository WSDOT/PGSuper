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


/*****************************************************************************
CLASS 
   pgsTendonStressArtifact

   Artifact for post-tension tendon stress checks.


DESCRIPTION
   Artifact for post-tension tendon stress checks. Records tendon stress and
   allowables at each poi along the tendon for the cases of
   1) Prior to seating
   2) Immediately after anchor set
   3) Service Limit State after losses


COPYRIGHT
   Copyright © 1997-2011
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.22.2011 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsTendonStressArtifact
{
public:
   pgsTendonStressArtifact();
   pgsTendonStressArtifact(const pgsTendonStressArtifact& rOther);
   virtual ~pgsTendonStressArtifact();

   pgsTendonStressArtifact& operator = (const pgsTendonStressArtifact& rOther);

   void SetCheckAtJacking(Float64 capacity,Float64 demand);
   void GetCheckAtJacking(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const;

   void SetCheckPriorToSeating(Float64 capacity,Float64 demand);
   void GetCheckPriorToSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const;

   void SetCheckAtAnchoragesAfterSeating(Float64 capacity,Float64 demand);
   void GetCheckAtAnchoragesAfterSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const;

   void SetCheckAfterSeating(Float64 capacity,Float64 demand);
   void GetCheckAfterSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const;

   void SetCheckAfterLosses(Float64 capacity,Float64 demand);
   void GetCheckAfterLosses(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const;

   bool IsAtJackingApplicable() const;
   bool IsPriorToSeatingApplicable() const;

   bool Passed() const;

protected:
   void MakeCopy(const pgsTendonStressArtifact& rOther);
   virtual void MakeAssignment(const pgsTendonStressArtifact& rOther);

   Float64 m_AtJacking[2];
   Float64 m_PriorToSeating[2]; // index 0 = capacity, index 1 = demand
   Float64 m_AtAnchoragesAfterSeating[2];
   Float64 m_AfterSeating[2];
   Float64 m_AfterLosses[2];

   bool m_bAtJacking;
   bool m_bPriorToSeating;
};
