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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PlantHandlingWeightArtifact.h>


/****************************************************************************
CLASS
   pgsPlantHandlingWeightArtifact
****************************************************************************/

pgsPlantHandlingWeightArtifact::pgsPlantHandlingWeightArtifact() :
   m_bIsApplicable(false)
{
}

pgsPlantHandlingWeightArtifact::pgsPlantHandlingWeightArtifact(const pgsPlantHandlingWeightArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsPlantHandlingWeightArtifact::~pgsPlantHandlingWeightArtifact()
{
}

pgsPlantHandlingWeightArtifact& pgsPlantHandlingWeightArtifact::operator=(const pgsPlantHandlingWeightArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsPlantHandlingWeightArtifact::SetWeight(Float64 Wg)
{
   m_Wg = Wg;
}

Float64 pgsPlantHandlingWeightArtifact::GetWeight() const
{
   return m_Wg;
}

void pgsPlantHandlingWeightArtifact::SetWeightLimit(Float64 limit)
{
   m_Limit = limit;
}

Float64 pgsPlantHandlingWeightArtifact::GetWeightLimit() const
{
   return m_Limit;
}

void pgsPlantHandlingWeightArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsPlantHandlingWeightArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

bool pgsPlantHandlingWeightArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
   {
      return true;
   }

   if (m_Limit < m_Wg )
   {
      return false;
   }

   return true;
}

void pgsPlantHandlingWeightArtifact::MakeCopy(const pgsPlantHandlingWeightArtifact& rOther)
{
   m_bIsApplicable = rOther.m_bIsApplicable;
   m_Wg = rOther.m_Wg;
   m_Limit = rOther.m_Limit;
}

void pgsPlantHandlingWeightArtifact::MakeAssignment(const pgsPlantHandlingWeightArtifact& rOther)
{
   MakeCopy( rOther );
}
