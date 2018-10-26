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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentStabilityArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsSegmentStabilityArtifact
****************************************************************************/


pgsSegmentStabilityArtifact::pgsSegmentStabilityArtifact():
m_bIsGlobalGirderStabilityApplicable(false)
{
   m_Wbottom = 0;
   m_Ybottom = 1;
   m_Orientation = 0;
}

pgsSegmentStabilityArtifact::pgsSegmentStabilityArtifact(const pgsSegmentStabilityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSegmentStabilityArtifact::~pgsSegmentStabilityArtifact()
{
}

pgsSegmentStabilityArtifact& pgsSegmentStabilityArtifact::operator=(const pgsSegmentStabilityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsSegmentStabilityArtifact::SetGlobalGirderStabilityApplicability(bool bSet)
{
   m_bIsGlobalGirderStabilityApplicable = bSet;
}

bool pgsSegmentStabilityArtifact::IsGlobalGirderStabilityApplicable() const
{
   return m_bIsGlobalGirderStabilityApplicable;
}

void pgsSegmentStabilityArtifact::SetGlobalGirderStabilityParameters(Float64 Wbottom,Float64 Ybottom,Float64 Orientation)
{
   m_Wbottom = Wbottom;
   m_Ybottom = Ybottom;
   m_Orientation = Orientation;
}

void pgsSegmentStabilityArtifact::GetGlobalGirderStabilityParameters(Float64 *Wbottom,Float64 *Ybottom,Float64 *Orientation) const
{
   *Wbottom     = m_Wbottom;
   *Ybottom     = m_Ybottom;
   *Orientation = m_Orientation;
}

Float64 pgsSegmentStabilityArtifact::GetMaxGirderIncline() const
{
   return fabs(m_Wbottom/(6*m_Ybottom)); // resultant in middle-1/3 of bottom
}

bool pgsSegmentStabilityArtifact::Passed() const
{
   if ( !m_bIsGlobalGirderStabilityApplicable )
   {
      return true;
   }

   // maximum incline to have the result of the girder dead load reaction within
   // the middle third of the girder
   Float64 maxIncline = GetMaxGirderIncline();
   return (maxIncline < m_Orientation) ? false : true;
}

void pgsSegmentStabilityArtifact::MakeCopy(const pgsSegmentStabilityArtifact& rOther)
{
   m_bIsGlobalGirderStabilityApplicable = rOther.m_bIsGlobalGirderStabilityApplicable;
   m_Wbottom = rOther.m_Wbottom;
   m_Ybottom = rOther.m_Ybottom;
   m_Orientation = rOther.m_Orientation;
}

void pgsSegmentStabilityArtifact::MakeAssignment(const pgsSegmentStabilityArtifact& rOther)
{
   MakeCopy( rOther );
}
