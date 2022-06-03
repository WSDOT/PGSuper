///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
   pgsSegmentStabilityArtifact

   Artifact for segment stability checks
*****************************************************************************/

class PGSEXTCLASS pgsSegmentStabilityArtifact
{
public:
   pgsSegmentStabilityArtifact();

   pgsSegmentStabilityArtifact(const pgsSegmentStabilityArtifact& rOther);

   virtual ~pgsSegmentStabilityArtifact();

   pgsSegmentStabilityArtifact& operator = (const pgsSegmentStabilityArtifact& rOther);

   void SetGlobalGirderStabilityApplicability(bool bSet);
   bool IsGlobalGirderStabilityApplicable() const;
   void SetTargetFactorOfSafety(Float64 fs);
   Float64 GetTargetFactorOfSafety() const;
   void SetGlobalGirderStabilityParameters(Float64 brgPadWidth,Float64 Ybottom,Float64 Orientation,Float64 Zo);
   void GetGlobalGirderStabilityParameters(Float64* brgPadWidth,Float64 *Ybottom,Float64 *Orientation,Float64* Zo) const;
   Float64 GetMaxGirderIncline() const;
   Float64 GetFactorOfSafety() const;

   bool Passed() const;

protected:
   void MakeCopy(const pgsSegmentStabilityArtifact& rOther);
   void MakeAssignment(const pgsSegmentStabilityArtifact& rOther);

private:
   bool m_bIsGlobalGirderStabilityApplicable;
   Float64 m_BrgPadWidth;
   Float64 m_Ybottom;
   Float64 m_Orientation;
   Float64 m_Zo;
   Float64 m_FS;
};
