///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
m_bIsSlabOffsetApplicable(false)
{
   m_Provided = 0;
   m_Required = 0;
   m_SlabOffsetWarningTolerance = ::ConvertToSysUnits(0.25,unitMeasure::Inch); // WSDOT standard
   m_MinimumRequiredFillet = 0;
   m_ProvidedFillet = 0;

   m_ProvidedAtBearingCLs  = 0;
   m_RequiredAtBearingCLs  = 0;
   m_bIsHaunchAtBearingCLsApplicable = false;
  
   m_bIsBottomFlangeClearanceApplicable = false;
   m_C = 0;
   m_Cmin = 0;
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

void pgsConstructabilityArtifact::SetSlabOffsetWarningTolerance(Float64 val)
{
   m_SlabOffsetWarningTolerance = val;
}

Float64 pgsConstructabilityArtifact::GetSlabOffsetWarningTolerance() const
{
   return m_SlabOffsetWarningTolerance;
}

void pgsConstructabilityArtifact::SetRequiredMinimumFillet(Float64 reqd)
{
   m_MinimumRequiredFillet = reqd;
}

Float64 pgsConstructabilityArtifact::GetRequiredMinimumFillet() const
{
   return m_MinimumRequiredFillet;
}

void pgsConstructabilityArtifact::SetProvidedFillet(Float64 provided)
{
   m_ProvidedFillet = provided;
}

Float64 pgsConstructabilityArtifact::GetProvidedFillet() const
{
   return m_ProvidedFillet;
}

bool pgsConstructabilityArtifact::MinimumFilletPassed() const
{
   return m_ProvidedFillet + TOLERANCE > m_MinimumRequiredFillet;
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
   {
      return NA;
   }

   if ( IsEqual(m_Provided,m_Required) )
   {
      return Pass;
   }

   if ( m_Provided < m_Required )
   {
      return Fail;
   }

   if ( (m_Required + m_SlabOffsetWarningTolerance) < m_Provided )
   {
      return Excessive;
   }

   return Pass;
}

bool pgsConstructabilityArtifact::SlabOffsetPassed() const
{
   return ( SlabOffsetStatus()==Fail ) ? false : true;
}

void pgsConstructabilityArtifact::CheckStirrupLength(bool bCheck)
{
   m_bCheckStirrupLength = bCheck;
}

bool pgsConstructabilityArtifact::CheckStirrupLength() const
{
   return m_bIsSlabOffsetApplicable && m_bCheckStirrupLength;
}

void pgsConstructabilityArtifact::SetProvidedHaunchAtBearingCLs(Float64 provided)
{
   m_ProvidedAtBearingCLs = provided;
}

Float64 pgsConstructabilityArtifact::GetProvidedHaunchAtBearingCLs() const
{
   ATLASSERT(m_bIsHaunchAtBearingCLsApplicable);
   return m_ProvidedAtBearingCLs;
}

void pgsConstructabilityArtifact::SetRequiredHaunchAtBearingCLs(Float64 reqd)
{
   m_RequiredAtBearingCLs = reqd;
}

Float64 pgsConstructabilityArtifact::GetRequiredHaunchAtBearingCLs() const
{
   ATLASSERT(m_bIsHaunchAtBearingCLsApplicable);
   return m_RequiredAtBearingCLs;
}

void pgsConstructabilityArtifact::SetHaunchAtBearingCLsApplicability(bool bSet)
{
   m_bIsHaunchAtBearingCLsApplicable = bSet;
}

bool pgsConstructabilityArtifact::IsHaunchAtBearingCLsApplicable() const
{
   return m_bIsHaunchAtBearingCLsApplicable;
}

bool pgsConstructabilityArtifact::HaunchAtBearingCLsPassed() const
{
   return  m_ProvidedAtBearingCLs+TOLERANCE >= m_RequiredAtBearingCLs;
}

void pgsConstructabilityArtifact::SetBottomFlangeClearanceApplicability(bool bSet)
{
   m_bIsBottomFlangeClearanceApplicable = bSet;
}

bool pgsConstructabilityArtifact::IsBottomFlangeClearanceApplicable() const
{
   return m_bIsBottomFlangeClearanceApplicable;
}

void pgsConstructabilityArtifact::SetBottomFlangeClearanceParameters(Float64 C,Float64 Cmin)
{
   m_C = C;
   m_Cmin = Cmin;
}

void pgsConstructabilityArtifact::GetBottomFlangeClearanceParameters(Float64* pC,Float64* pCmin) const
{
   *pC = m_C;
   *pCmin = m_Cmin;
}

bool pgsConstructabilityArtifact::BottomFlangeClearancePassed() const
{
   if ( !m_bIsBottomFlangeClearanceApplicable )
   {
      return true;
   }

   return ::IsGE(m_Cmin,m_C) ? true : false;
}

bool pgsConstructabilityArtifact::Passed() const
{
   if ( !SlabOffsetPassed() )
   {
      return false;
   }

   if (IsHaunchAtBearingCLsApplicable() && !HaunchAtBearingCLsPassed())
   {
      return false;
   }

   if (!MinimumFilletPassed())
   {
      return false;
   }

   if ( !BottomFlangeClearancePassed() )
   {
      return false;
   }

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
   m_SlabOffsetWarningTolerance = rOther.m_SlabOffsetWarningTolerance;
   m_bCheckStirrupLength = rOther.m_bCheckStirrupLength;
   m_bIsSlabOffsetApplicable = rOther.m_bIsSlabOffsetApplicable;

   m_MinimumRequiredFillet = rOther.m_MinimumRequiredFillet;
   m_ProvidedFillet = rOther.m_ProvidedFillet;

   m_ProvidedAtBearingCLs  = rOther.m_ProvidedAtBearingCLs;
   m_RequiredAtBearingCLs  = rOther.m_RequiredAtBearingCLs;
   m_bIsHaunchAtBearingCLsApplicable  = rOther.m_bIsHaunchAtBearingCLsApplicable;

   m_bIsBottomFlangeClearanceApplicable = rOther.m_bIsBottomFlangeClearanceApplicable;
   m_C = rOther.m_C;
   m_Cmin = rOther.m_Cmin;
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
