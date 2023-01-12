///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
   pgsPlantHandlingWeightArtifact

DESCRIPTION
   Artifact for plant handling weight checks
*****************************************************************************/

class PGSEXTCLASS pgsPlantHandlingWeightArtifact
{
public:
   pgsPlantHandlingWeightArtifact();
   pgsPlantHandlingWeightArtifact(const pgsPlantHandlingWeightArtifact& rOther);
   ~pgsPlantHandlingWeightArtifact();

   pgsPlantHandlingWeightArtifact& operator=(const pgsPlantHandlingWeightArtifact& rOther);

   void SetWeight(Float64 Wg);
   Float64 GetWeight() const;

   void SetWeightLimit(Float64 Limit);
   Float64 GetWeightLimit() const;

   void IsApplicable(bool bIsApplicable);
   bool IsApplicable() const;

   bool Passed() const;

protected:
   void MakeCopy(const pgsPlantHandlingWeightArtifact& rOther);
   void MakeAssignment(const pgsPlantHandlingWeightArtifact& rOther);

private:
   bool m_bIsApplicable;
   Float64 m_Wg;   // girder weight
   Float64 m_Limit; // weight limit
};
