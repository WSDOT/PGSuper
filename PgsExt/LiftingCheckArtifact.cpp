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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\LiftingCheckArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsLiftingStressCheckArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingStressCheckArtifact::pgsLiftingStressCheckArtifact(const pgsLiftingCheckArtifact& rParent):
m_pParent(&rParent)
{
}

pgsLiftingStressCheckArtifact::pgsLiftingStressCheckArtifact(const pgsLiftingStressCheckArtifact& rOther):
m_pParent(rOther.m_pParent)
{
   MakeCopy(rOther);
}

pgsLiftingStressCheckArtifact::~pgsLiftingStressCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLiftingStressCheckArtifact& pgsLiftingStressCheckArtifact::operator= (const pgsLiftingStressCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsLiftingStressCheckArtifact::SetParent(const pgsLiftingCheckArtifact* pParent)
{
   m_pParent = pParent;
}

bool pgsLiftingStressCheckArtifact::TensionPassed() const
{
   Float64 tens_stress = GetMaximumConcreteTensileStress();
   Float64 max_tens_stress = m_pParent->GetAllowableTensileStress();
   if ( IsGT(max_tens_stress,tens_stress) )
      return true;

   return false;
}

bool pgsLiftingStressCheckArtifact::AlternativeTensionPassed() const
{
   Float64 tens_stress = GetMaximumConcreteTensileStress();
   Float64 max_tens_stress = m_pParent->GetAlternativeTensionAllowableStress();
   if ( IsGT(max_tens_stress,tens_stress) )
      return true;

   return false;
}

bool pgsLiftingStressCheckArtifact::CompressionPassed() const
{
   Float64 comp_stress = GetMaximumConcreteCompressiveStress();
   Float64 max_comp_stress = m_pParent->GetAllowableCompressionStress();
   if ( IsLT(max_comp_stress,comp_stress) )
      return true;

   return false;
}

bool pgsLiftingStressCheckArtifact::Passed() const
{
    return (TensionPassed() || AlternativeTensionPassed()) && CompressionPassed();
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLiftingStressCheckArtifact::MakeCopy(const pgsLiftingStressCheckArtifact& rOther)
{
   pgsLiftingStressAnalysisArtifact::MakeCopy(rOther);
}

void pgsLiftingStressCheckArtifact::MakeAssignment(const pgsLiftingStressCheckArtifact& rOther)
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLiftingStressCheckArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingStressCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLiftingStressCheckArtifact" << endl;
}
#endif // _DEBUG




/****************************************************************************
CLASS
   pgsLiftingCrackingCheckArtifact
****************************************************************************/

#include <PgsExt\LiftingCheckArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingCrackingCheckArtifact::pgsLiftingCrackingCheckArtifact(const pgsLiftingCheckArtifact& rParent):
m_pParent(&rParent)
{
}

pgsLiftingCrackingCheckArtifact::pgsLiftingCrackingCheckArtifact(const pgsLiftingCrackingCheckArtifact& rOther):
m_pParent(rOther.m_pParent)
{
   MakeCopy(rOther);
}

pgsLiftingCrackingCheckArtifact::~pgsLiftingCrackingCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLiftingCrackingCheckArtifact& pgsLiftingCrackingCheckArtifact::operator= (const pgsLiftingCrackingCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsLiftingCrackingCheckArtifact::SetParent(const pgsLiftingCheckArtifact* pParent)
{
   m_pParent = pParent;
}

bool  pgsLiftingCrackingCheckArtifact::Passed() const
{
   Float64 min_fs = m_pParent->GetAllowableFsForCracking();
   if (min_fs > GetFsCracking())
      return false;

   return true;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLiftingCrackingCheckArtifact::MakeCopy(const pgsLiftingCrackingCheckArtifact& rOther)
{
   pgsLiftingCrackingAnalysisArtifact::MakeCopy(rOther);
}

void pgsLiftingCrackingCheckArtifact::MakeAssignment(const pgsLiftingCrackingCheckArtifact& rOther)
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLiftingCrackingCheckArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingCrackingCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLiftingCrackingCheckArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsLiftingCheckArtifact
****************************************************************************/

#include <PgsExt\LiftingCheckArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingCheckArtifact::pgsLiftingCheckArtifact()
{
}

pgsLiftingCheckArtifact::pgsLiftingCheckArtifact(const pgsLiftingCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsLiftingCheckArtifact::~pgsLiftingCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLiftingCheckArtifact& pgsLiftingCheckArtifact::operator= (const pgsLiftingCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

bool pgsLiftingCheckArtifact::Passed() const
{
   // cracking
   Float64 fs_crack = this->GetMinFsForCracking();
   Float64 all_crack = this->GetAllowableFsForCracking();
   if (fs_crack<all_crack)
      return false;

   // Failure
   if ( !PassedFailureCheck() )
      return false;

   // stresses
   Float64 min_stress, max_stress;
   Float64 minDistFromStart, maxDistFromStart; // Distance from start for min/max stresses
   this->GetMinMaxStresses(&min_stress, &max_stress,&minDistFromStart,&maxDistFromStart);
   
   Float64 min_all_stress = this->GetAllowableCompressionStress();
   if (min_stress<min_all_stress)
      return false;

   if (max_stress > _cpp_max(GetAllowableTensileStress(),GetAlternativeTensionAllowableStress()))
      return false;

   return true;
}

Float64 pgsLiftingCheckArtifact::GetAllowableTensileStress() const
{
   return m_AllowableTensileStress;
}

void pgsLiftingCheckArtifact::SetAllowableTensileStress(Float64 val)
{
   m_AllowableTensileStress = val;
}

Float64 pgsLiftingCheckArtifact::GetAllowableCompressionStress() const
{
   return m_AllowableCompressionStress;
}

void pgsLiftingCheckArtifact::SetAllowableCompressionStress(Float64 val)
{
   m_AllowableCompressionStress = val;
}

Float64 pgsLiftingCheckArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsLiftingCheckArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

bool pgsLiftingCheckArtifact::PassedFailureCheck() const
{
   Float64 fsfail = GetFsFailure();
   Float64 alfail = GetAllowableFsForFailure();
   return alfail < fsfail;
}

Float64 pgsLiftingCheckArtifact::GetAllowableFsForFailure() const
{
   return m_AllowableFsForFailure;
}

void pgsLiftingCheckArtifact::SetAllowableFsForFailure(Float64 val)
{
   m_AllowableFsForFailure = val;
}

pgsLiftingStressCheckArtifact pgsLiftingCheckArtifact::GetLiftingStressCheckArtifact(Float64 distFromStart) const
{
   pgsLiftingStressCheckArtifact checkArtifact(*this);

   const pgsLiftingStressAnalysisArtifact* pArtifact = GetLiftingStressAnalysisArtifact(distFromStart);
   if ( pArtifact == NULL )
      return checkArtifact;

   pgsLiftingStressCheckArtifact* pCheckArtifact = &checkArtifact;
   (*(pgsLiftingStressAnalysisArtifact*)pCheckArtifact) = *pArtifact;
   return checkArtifact;
}

pgsLiftingCrackingCheckArtifact pgsLiftingCheckArtifact::GetLiftingCrackingCheckArtifact(Float64 distFromStart) const
{
   pgsLiftingCrackingCheckArtifact checkArtifact(*this);
   const pgsLiftingCrackingAnalysisArtifact* pArtifact = GetLiftingCrackingAnalysisArtifact(distFromStart);
   if ( pArtifact == NULL )
      return checkArtifact;

   pgsLiftingCrackingCheckArtifact* pCheckArtifact = &checkArtifact;
   (*(pgsLiftingCrackingAnalysisArtifact*)pCheckArtifact) = *pArtifact;
   return checkArtifact;
}
   
void pgsLiftingCheckArtifact::SetAlternativeTensionAllowableStress(Float64 val)
{
    m_AllowableAlternativeTensileStress = val;
}

Float64 pgsLiftingCheckArtifact::GetAlternativeTensionAllowableStress() const
{
    return m_AllowableAlternativeTensileStress;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLiftingCheckArtifact::MakeCopy(const pgsLiftingCheckArtifact& rOther)
{
   pgsLiftingAnalysisArtifact::MakeCopy(rOther);

   m_AllowableTensileStress     = rOther.m_AllowableTensileStress;
   m_AllowableCompressionStress = rOther.m_AllowableCompressionStress;
   m_AllowableFsForCracking     = rOther.m_AllowableFsForCracking;
   m_AllowableFsForFailure      = rOther.m_AllowableFsForFailure;
   m_AllowableAlternativeTensileStress = rOther.m_AllowableAlternativeTensileStress;
}

void pgsLiftingCheckArtifact::MakeAssignment(const pgsLiftingCheckArtifact& rOther)
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLiftingCheckArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLiftingCheckArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsLiftingCheckArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsLiftingCheckArtifact");


   TESTME_EPILOG("LiftingCheckArtifact");
}
#endif // _UNITTEST
