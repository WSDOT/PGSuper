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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ConstructabilityArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsConstructabilityArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsConstructabilityArtifact::pgsConstructabilityArtifact():
m_bIsSlabOffsetApplicable(false),m_bIsGlobalGirderStabilityApplicable(false)
{
   m_Wbottom = 0;
   m_Ybottom = 1;
   m_Orientation = 0;

   m_Provided = 0;
   m_Required =0;
}

pgsConstructabilityArtifact::pgsConstructabilityArtifact(const pgsConstructabilityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsConstructabilityArtifact::~pgsConstructabilityArtifact()
{
}

//======================== OPERATORS  =======================================
pgsConstructabilityArtifact& pgsConstructabilityArtifact::operator=(const pgsConstructabilityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsConstructabilityArtifact::SetProvidedSlabOffset(Float64 provided)
{
   m_Provided = provided;
}

Float64 pgsConstructabilityArtifact::GetProvidedSlabOffset() const
{
   ATLASSERT(m_bIsSlabOffsetApplicable);
   return m_Provided;
}

void pgsConstructabilityArtifact::SetRequiredSlabOffset(Float64 reqd)
{
   m_Required = reqd;
}

Float64 pgsConstructabilityArtifact::GetRequiredSlabOffset() const
{
   ATLASSERT(m_bIsSlabOffsetApplicable);
   return m_Required;
}

void pgsConstructabilityArtifact::SetSlabOffsetApplicability(bool bSet)
{
   m_bIsSlabOffsetApplicable = bSet;
}

bool pgsConstructabilityArtifact::IsSlabOffsetApplicable() const
{
   return m_bIsSlabOffsetApplicable;
}

pgsConstructabilityArtifact::SlabOffsetStatusType pgsConstructabilityArtifact::SlabOffsetStatus() const
{
   if (!m_bIsSlabOffsetApplicable)
      return NA;

   if ( IsEqual(m_Provided,m_Required) )
      return Passed;

   if ( m_Provided < m_Required )
      return Failed;

   if ( (m_Required + ::ConvertToSysUnits(0.25,unitMeasure::Inch)) < m_Provided )
      return Excessive;

   return Passed;
}

bool pgsConstructabilityArtifact::SlabOffsetPassed() const
{
   return ( SlabOffsetStatus() == Failed ) ? false : true;
}

void pgsConstructabilityArtifact::CheckStirrupLength(bool bCheck)
{
   m_bCheckStirrupLength = bCheck;
}

bool pgsConstructabilityArtifact::CheckStirrupLength() const
{
   return m_bIsSlabOffsetApplicable && m_bCheckStirrupLength;
}

void pgsConstructabilityArtifact::SetGlobalGirderStabilityApplicability(bool bSet)
{
   m_bIsGlobalGirderStabilityApplicable = bSet;
}

bool pgsConstructabilityArtifact::IsGlobalGirderStabilityApplicable() const
{
   return m_bIsGlobalGirderStabilityApplicable;
}

void pgsConstructabilityArtifact::SetGlobalGirderStabilityParameters(Float64 Wbottom,Float64 Ybottom,Float64 Orientation)
{
   m_Wbottom = Wbottom;
   m_Ybottom = Ybottom;
   m_Orientation = Orientation;
}

void pgsConstructabilityArtifact::GetGlobalGirderStabilityParameters(Float64 *Wbottom,Float64 *Ybottom,Float64 *Orientation) const
{
   *Wbottom     = m_Wbottom;
   *Ybottom     = m_Ybottom;
   *Orientation = m_Orientation;
}

Float64 pgsConstructabilityArtifact::GetMaxGirderIncline() const
{
   return fabs(m_Wbottom/(6*m_Ybottom)); // resultant in middle-1/3 of bottom
}

bool pgsConstructabilityArtifact::GlobalGirderStabilityPassed() const
{
   if ( !m_bIsGlobalGirderStabilityApplicable )
      return true;

   // maximum incline to have the result of the girder dead load reaction within
   // the middle third of the girder
   Float64 maxIncline = GetMaxGirderIncline();
   return (maxIncline < m_Orientation) ? false : true;
}

void pgsConstructabilityArtifact::SetRebarRowsOutsideOfSection(const std::vector<RowIndexType>& rows)
{
   m_RebarRowsOutsideSection = rows;
}

std::vector<RowIndexType> pgsConstructabilityArtifact::GetRebarRowsOutsideOfSection() const
{
   return m_RebarRowsOutsideSection;
}

bool pgsConstructabilityArtifact::RebarGeometryCheckPassed() const
{
   return m_RebarRowsOutsideSection.empty();
}

bool pgsConstructabilityArtifact::Pass() const
{
   if ( !SlabOffsetPassed() )
      return false;

   if ( !GlobalGirderStabilityPassed() )
      return false;

   if (! RebarGeometryCheckPassed() )
      return false;

   return true;
}

 //======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsConstructabilityArtifact::MakeCopy(const pgsConstructabilityArtifact& rOther)
{
   m_Provided = rOther.m_Provided;
   m_Required = rOther.m_Required;
   m_bCheckStirrupLength = rOther.m_bCheckStirrupLength;
   m_bIsSlabOffsetApplicable = rOther.m_bIsSlabOffsetApplicable;

   m_bIsGlobalGirderStabilityApplicable = rOther.m_bIsGlobalGirderStabilityApplicable;
   m_Wbottom = rOther.m_Wbottom;
   m_Ybottom = rOther.m_Ybottom;
   m_Orientation = rOther.m_Orientation;

   m_RebarRowsOutsideSection = rOther.m_RebarRowsOutsideSection;
}

void pgsConstructabilityArtifact::MakeAssignment(const pgsConstructabilityArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
