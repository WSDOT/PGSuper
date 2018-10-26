///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

/****************************************************************************
CLASS
   pgsStrandStressArtifact
****************************************************************************/

#include <PgsExt\StrandStressArtifact.h>
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define JACKING       0
#define BEFORE_XFER   1
#define AFTER_XFER    2
#define AFTER_LOSSES  3

#define DEMAND        0
#define CAPACITY      1

#define STRAND(_x_) (_x_ == pgsTypes::Permanent ? 0 : 1)

static Float64 STRESS_TOLERANCE = ::ConvertToSysUnits(1.0,unitMeasure::PSI);
////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStrandStressArtifact::pgsStrandStressArtifact()
{
   for ( Int16 i = 0; i < 4; i++ ) // strand type
   {
      for ( Int16 j = 0; j < 4; j++ ) // stress stage (at jacking, befer xfer, etc)
      {
         for ( Int16 k = 0; k < 2; k++ ) //
         {
            m_Stress[i][j][k] = 0.00;
         }

         m_bIsApplicable[i][j] = false;
      }
   }
}

pgsStrandStressArtifact::pgsStrandStressArtifact(const pgsStrandStressArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStrandStressArtifact::~pgsStrandStressArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStrandStressArtifact& pgsStrandStressArtifact::operator= (const pgsStrandStressArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsStrandStressArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

pgsPointOfInterest pgsStrandStressArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsStrandStressArtifact::SetCheckAtJacking(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity)
{
   m_Stress[strandType][JACKING][DEMAND]   = demand;
   m_Stress[strandType][JACKING][CAPACITY] = capacity;
   m_bIsApplicable[strandType][JACKING]    = true;
}

void pgsStrandStressArtifact::SetCheckBeforeXfer(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity)
{
   m_Stress[strandType][BEFORE_XFER][DEMAND]   = demand;
   m_Stress[strandType][BEFORE_XFER][CAPACITY] = capacity;
   m_bIsApplicable[strandType][BEFORE_XFER]    = true;
}

void pgsStrandStressArtifact::SetCheckAfterXfer(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity)
{
   m_Stress[strandType][AFTER_XFER][DEMAND]   = demand;
   m_Stress[strandType][AFTER_XFER][CAPACITY] = capacity;
   m_bIsApplicable[strandType][AFTER_XFER]    = true;
}

void pgsStrandStressArtifact::SetCheckAfterLosses(pgsTypes::StrandType strandType,Float64 demand,Float64 capacity)
{
   m_Stress[strandType][AFTER_LOSSES][DEMAND]   = demand;
   m_Stress[strandType][AFTER_LOSSES][CAPACITY] = capacity;
   m_bIsApplicable[strandType][AFTER_LOSSES]    = true;
}

void pgsStrandStressArtifact::GetCheckAtJacking(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const
{
   *pDemand   = m_Stress[strandType][JACKING][DEMAND];
   *pCapacity = m_Stress[strandType][JACKING][CAPACITY];
   *pbPassed = IsGE(*pDemand,*pCapacity,STRESS_TOLERANCE);
}

void pgsStrandStressArtifact::GetCheckBeforeXfer(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const
{
   *pDemand   = m_Stress[strandType][BEFORE_XFER][DEMAND];
   *pCapacity = m_Stress[strandType][BEFORE_XFER][CAPACITY];
   *pbPassed = IsGE(*pDemand,*pCapacity,STRESS_TOLERANCE);
}

void pgsStrandStressArtifact::GetCheckAfterXfer(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const
{
   *pDemand   = m_Stress[strandType][AFTER_XFER][DEMAND];
   *pCapacity = m_Stress[strandType][AFTER_XFER][CAPACITY];
   *pbPassed = IsGE(*pDemand,*pCapacity,STRESS_TOLERANCE);
}

void pgsStrandStressArtifact::GetCheckAfterLosses(pgsTypes::StrandType strandType,Float64* pDemand,Float64* pCapacity,bool* pbPassed) const
{
   *pDemand   = m_Stress[strandType][AFTER_LOSSES][DEMAND];
   *pCapacity = m_Stress[strandType][AFTER_LOSSES][CAPACITY];
   *pbPassed = IsGE(*pDemand,*pCapacity,STRESS_TOLERANCE);
}

//======================== INQUIRY    =======================================

bool pgsStrandStressArtifact::IsCheckAtJackingApplicable(pgsTypes::StrandType strandType) const
{
   return m_bIsApplicable[strandType][JACKING];
}

bool pgsStrandStressArtifact::IsCheckBeforeXferApplicable(pgsTypes::StrandType strandType) const
{
   return m_bIsApplicable[strandType][BEFORE_XFER];
}

bool pgsStrandStressArtifact::IsCheckAfterXferApplicable(pgsTypes::StrandType strandType) const
{
   return m_bIsApplicable[strandType][AFTER_XFER];
}

bool pgsStrandStressArtifact::IsCheckAfterLossesApplicable(pgsTypes::StrandType strandType) const
{
   return m_bIsApplicable[strandType][AFTER_LOSSES];
}

bool pgsStrandStressArtifact::Passed(pgsTypes::StrandType strandType) const
{
   Float64 cap,demand;
   bool bPassed;

   if ( IsCheckAtJackingApplicable(strandType) )
   {
      GetCheckAtJacking(strandType,&demand,&cap,&bPassed);
      if ( !bPassed )
      {
         return false;
      }
   }

   if ( IsCheckBeforeXferApplicable(strandType) )
   {
      GetCheckBeforeXfer(strandType,&demand,&cap,&bPassed);
      if ( !bPassed )
      {
         return false;
      }
   }

   if ( IsCheckAfterXferApplicable(strandType) )
   {
      GetCheckAfterXfer(strandType,&demand,&cap,&bPassed);
      if ( !bPassed )
      {
         return false;
      }
   }

   if ( IsCheckAfterLossesApplicable(strandType) )
   {
      GetCheckAfterLosses(strandType,&demand,&cap,&bPassed);
      if ( !bPassed )
      {
         return false;
      }
   }

   return true;
}

bool pgsStrandStressArtifact::Passed() const
{
   for ( Int16 i = 0; i < 4; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if (!Passed(strandType))
      {
         return false;
      }
   }

   return true;
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStrandStressArtifact::MakeCopy(const pgsStrandStressArtifact& rOther)
{
   for ( Int16 i = 0; i < 4; i++ ) // strand type
   {
      for ( Int16 j = 0; j < 4; j++ ) // stress stage (at jacking, befer xfer, etc)
      {
         for ( Int16 k = 0; k < 2; k++ ) //
         {
            m_Stress[i][j][k] = rOther.m_Stress[i][j][k];
         }

         m_bIsApplicable[i][j] = rOther.m_bIsApplicable[i][j];
      }
   }

   m_POI = rOther.m_POI;
}

void pgsStrandStressArtifact::MakeAssignment(const pgsStrandStressArtifact& rOther)
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
