///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
   pgsTendonStressArtifact
****************************************************************************/

#include <PgsExt\TendonStressArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CAPACITY 0
#define DEMAND   1

pgsTendonStressArtifact::pgsTendonStressArtifact()
{
   for ( int i = 0; i < 2; i++ )
   {
      m_AtJacking[i]                = 0;
      m_PriorToSeating[i]           = 0;
      m_AtAnchoragesAfterSeating[i] = 0;
      m_AfterSeating[i]             = 0;
      m_AfterLosses[i]              = 0;
   }

   m_bAtJacking      = false;
   m_bPriorToSeating = false;
}

pgsTendonStressArtifact::pgsTendonStressArtifact(const pgsTendonStressArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsTendonStressArtifact::~pgsTendonStressArtifact()
{
}

pgsTendonStressArtifact& pgsTendonStressArtifact::operator= (const pgsTendonStressArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsTendonStressArtifact::SetCheckAtJacking(Float64 capacity,Float64 demand)
{
   m_bAtJacking = true;
   m_AtJacking[CAPACITY] = capacity;
   m_AtJacking[DEMAND]   = demand;
}

void pgsTendonStressArtifact::GetCheckAtJacking(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const
{
   *pCapacity = m_AtJacking[CAPACITY];
   *pDemand   = m_AtJacking[DEMAND];
   *pbPassed  = (*pDemand <= *pCapacity);
}

void pgsTendonStressArtifact::SetCheckPriorToSeating(Float64 capacity,Float64 demand)
{
   m_bPriorToSeating = true;
   m_PriorToSeating[CAPACITY] = capacity;
   m_PriorToSeating[DEMAND]   = demand;
}

void pgsTendonStressArtifact::GetCheckPriorToSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const
{
   *pCapacity = m_PriorToSeating[CAPACITY];
   *pDemand   = m_PriorToSeating[DEMAND];
   *pbPassed  = (*pDemand <= *pCapacity);
}

void pgsTendonStressArtifact::SetCheckAtAnchoragesAfterSeating(Float64 capacity,Float64 demand)
{
   m_AtAnchoragesAfterSeating[CAPACITY] = capacity;
   m_AtAnchoragesAfterSeating[DEMAND]   = demand;
}

void pgsTendonStressArtifact::GetCheckAtAnchoragesAfterSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const
{
   *pCapacity = m_AtAnchoragesAfterSeating[CAPACITY];
   *pDemand   = m_AtAnchoragesAfterSeating[DEMAND];
   *pbPassed  = (*pDemand <= *pCapacity);
}

void pgsTendonStressArtifact::SetCheckAfterSeating(Float64 capacity,Float64 demand)
{
   m_AfterSeating[CAPACITY] = capacity;
   m_AfterSeating[DEMAND]   = demand;
}

void pgsTendonStressArtifact::GetCheckAfterSeating(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const
{
   *pCapacity = m_AfterSeating[CAPACITY];
   *pDemand   = m_AfterSeating[DEMAND];
   *pbPassed  = (*pDemand <= *pCapacity);
}

void pgsTendonStressArtifact::SetCheckAfterLosses(Float64 capacity,Float64 demand)
{
   m_AfterLosses[CAPACITY] = capacity;
   m_AfterLosses[DEMAND]   = demand;
}

void pgsTendonStressArtifact::GetCheckAfterLosses(Float64* pCapacity,Float64* pDemand,bool* pbPassed) const
{
   *pCapacity = m_AfterLosses[CAPACITY];
   *pDemand   = m_AfterLosses[DEMAND];
   *pbPassed  = (*pDemand <= *pCapacity);
}

bool pgsTendonStressArtifact::IsAtJackingApplicable() const
{
   return m_bAtJacking;
}

bool pgsTendonStressArtifact::IsPriorToSeatingApplicable() const
{
   return m_bPriorToSeating;
}

bool pgsTendonStressArtifact::Passed() const
{
   Float64 capacity,demand;
   bool bPassed;
   if (m_bAtJacking)
   {
      GetCheckAtJacking(&capacity,&demand,&bPassed);
      if ( !bPassed )
         return false;
   }

   if ( m_bPriorToSeating )
   {
      GetCheckPriorToSeating(&capacity,&demand,&bPassed);
      if ( !bPassed )
         return false;
   }

   GetCheckAtAnchoragesAfterSeating(&capacity,&demand,&bPassed);
   if ( !bPassed )
      return false;

   GetCheckAfterSeating(&capacity,&demand,&bPassed);
   if ( !bPassed )
      return false;

   GetCheckAfterLosses(&capacity,&demand,&bPassed);
   if ( !bPassed )
      return false;

   return true;
}

void pgsTendonStressArtifact::MakeCopy(const pgsTendonStressArtifact& rOther)
{
   for ( int i = 0; i < 2; i++ )
   {
      m_AtJacking[i]                = rOther.m_AtJacking[i];
      m_PriorToSeating[i]           = rOther.m_PriorToSeating[i];
      m_AtAnchoragesAfterSeating[i] = rOther.m_AtAnchoragesAfterSeating[i];
      m_AfterSeating[i]             = rOther.m_AfterSeating[i];
      m_AfterLosses[i]              = rOther.m_AfterLosses[i];
   }

   m_bAtJacking      = rOther.m_bAtJacking;
   m_bPriorToSeating = rOther.m_bPriorToSeating;
}

void pgsTendonStressArtifact::MakeAssignment(const pgsTendonStressArtifact& rOther)
{
   MakeCopy( rOther );
}
