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
   m_BrgPadWidth = 0;
   m_Zo = 0;
   m_Ybottom = 1;
   m_Orientation = 0;
   m_FS = 1.2;
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

void pgsSegmentStabilityArtifact::SetTargetFactorOfSafety(Float64 fs)
{
   m_FS = fs;
}

Float64 pgsSegmentStabilityArtifact::GetTargetFactorOfSafety() const
{
   return m_FS;
}

void pgsSegmentStabilityArtifact::SetGlobalGirderStabilityParameters(Float64 brgPadWidth,Float64 Ybottom,Float64 Orientation,Float64 Zo)
{
   m_BrgPadWidth = brgPadWidth;
   m_Ybottom = Ybottom;
   m_Orientation = Orientation;
   m_Zo = Zo;
}

void pgsSegmentStabilityArtifact::GetGlobalGirderStabilityParameters(Float64* brgPadWidth,Float64 *Ybottom,Float64 *Orientation,Float64* Zo) const
{
   *brgPadWidth = m_BrgPadWidth;
   *Ybottom     = m_Ybottom;
   *Orientation = m_Orientation;
   *Zo          = m_Zo;
}

Float64 pgsSegmentStabilityArtifact::GetFactorOfSafety() const
{
   Float64 theta_max = GetMaxGirderIncline();
   Float64 theta = m_Orientation;
   Float64 FS = theta_max / theta;
   return FS;
}

Float64 pgsSegmentStabilityArtifact::GetMaxGirderIncline() const
{
   return fabs(m_BrgPadWidth /(6*(m_Zo + m_Ybottom))); // resultant at kern point
}

bool pgsSegmentStabilityArtifact::Passed() const
{
   if ( !m_bIsGlobalGirderStabilityApplicable )
   {
      return true;
   }

   Float64 fs = GetFactorOfSafety();
   return (m_FS <= fs) ? true : false;
}

void pgsSegmentStabilityArtifact::MakeCopy(const pgsSegmentStabilityArtifact& rOther)
{
   m_bIsGlobalGirderStabilityApplicable = rOther.m_bIsGlobalGirderStabilityApplicable;
   m_BrgPadWidth = rOther.m_BrgPadWidth;
   m_Ybottom = rOther.m_Ybottom;
   m_Orientation = rOther.m_Orientation;
   m_Zo = rOther.m_Zo;
   m_FS = rOther.m_FS;
}

void pgsSegmentStabilityArtifact::MakeAssignment(const pgsSegmentStabilityArtifact& rOther)
{
   MakeCopy( rOther );
}
