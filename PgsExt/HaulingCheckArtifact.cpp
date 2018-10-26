///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\HaulingCheckArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsHaulingStressCheckArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingStressCheckArtifact::pgsHaulingStressCheckArtifact(const pgsHaulingCheckArtifact& rParent):
m_pParent(&rParent)
{
}

pgsHaulingStressCheckArtifact::pgsHaulingStressCheckArtifact(const pgsHaulingStressCheckArtifact& rOther):
m_pParent(rOther.m_pParent)
{
   MakeCopy(rOther);
}

pgsHaulingStressCheckArtifact::~pgsHaulingStressCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingStressCheckArtifact& pgsHaulingStressCheckArtifact::operator= (const pgsHaulingStressCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsHaulingStressCheckArtifact::SetParent(const pgsHaulingCheckArtifact* pParent)
{
   m_pParent = pParent;
}

bool pgsHaulingStressCheckArtifact::TensionPassed() const
{
   Float64 tens_stress = GetMaximumConcreteTensileStress();
   Float64 max_tens_stress = m_pParent->GetAllowableTensileStress();
   if (tens_stress>max_tens_stress)
      return false;

   return true;
}

bool pgsHaulingStressCheckArtifact::AlternativeTensionPassed() const
{
   Float64 tens_stress = GetMaximumConcreteTensileStress();
   Float64 max_tens_stress = m_pParent->GetAlternativeTensionAllowableStress();
   if (tens_stress>max_tens_stress)
      return false;

   return true;
}

bool pgsHaulingStressCheckArtifact::CompressionPassed() const
{
   Float64 comp_stress = _cpp_min( GetMaximumConcreteCompressiveStress(),
                                        GetMaximumInclinedConcreteCompressiveStress() );

   Float64 max_comp_stress = m_pParent->GetAllowableCompressionStress();

   if (comp_stress < max_comp_stress)
      return false;

   return true;
}

bool pgsHaulingStressCheckArtifact::Passed() const
{
    return (TensionPassed() || AlternativeTensionPassed()) && CompressionPassed();
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHaulingStressCheckArtifact::MakeCopy(const pgsHaulingStressCheckArtifact& rOther)
{
   pgsHaulingStressAnalysisArtifact::MakeCopy(rOther);
}

void pgsHaulingStressCheckArtifact::MakeAssignment(const pgsHaulingStressCheckArtifact& rOther)
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
bool pgsHaulingStressCheckArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingStressCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHaulingStressCheckArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsHaulingCrackingCheckArtifact
****************************************************************************/

#include <PgsExt\HaulingCheckArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingCrackingCheckArtifact::pgsHaulingCrackingCheckArtifact(const pgsHaulingCheckArtifact& rParent):
m_pParent(&rParent)
{
}

pgsHaulingCrackingCheckArtifact::pgsHaulingCrackingCheckArtifact(const pgsHaulingCrackingCheckArtifact& rOther):
m_pParent(rOther.m_pParent)
{
   MakeCopy(rOther);
}

pgsHaulingCrackingCheckArtifact::~pgsHaulingCrackingCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingCrackingCheckArtifact& pgsHaulingCrackingCheckArtifact::operator= (const pgsHaulingCrackingCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
void pgsHaulingCrackingCheckArtifact::SetParent(const pgsHaulingCheckArtifact* pParent)
{
   m_pParent = pParent;
}

bool  pgsHaulingCrackingCheckArtifact::Passed() const
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
void pgsHaulingCrackingCheckArtifact::MakeCopy(const pgsHaulingCrackingCheckArtifact& rOther)
{
   pgsHaulingCrackingAnalysisArtifact::MakeCopy(rOther);
}

void pgsHaulingCrackingCheckArtifact::MakeAssignment(const pgsHaulingCrackingCheckArtifact& rOther)
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
bool pgsHaulingCrackingCheckArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingCrackingCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHaulingCrackingCheckArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsHaulingCheckArtifact
****************************************************************************/

#include <PgsExt\HaulingCheckArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingCheckArtifact::pgsHaulingCheckArtifact()
{
}

pgsHaulingCheckArtifact::pgsHaulingCheckArtifact(const pgsHaulingCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHaulingCheckArtifact::~pgsHaulingCheckArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingCheckArtifact& pgsHaulingCheckArtifact::operator= (const pgsHaulingCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================

bool pgsHaulingCheckArtifact::Passed() const
{
   // cracking
   Float64 fs_crack = this->GetMinFsForCracking();
   Float64 all_crack = this->GetAllowableFsForCracking();
   if (fs_crack<all_crack)
      return false;

   // Rollover
   Float64 fs_roll = this->GetFsRollover();
   Float64 all_roll = this->GetAllowableFsForRollover();
   if (fs_roll<all_roll)
      return false;

   // stresses
   Float64 min_stress, max_stress;
   this->GetMinMaxStresses(&min_stress, &max_stress);
   
   Float64 min_all_stress = this->GetAllowableCompressionStress();
   if (min_stress<min_all_stress)
      return false;

   if (max_stress > _cpp_max(GetAllowableTensileStress(),GetAlternativeTensionAllowableStress()))
      return false;

   // tolerance for distances
   Float64 TOL_DIST = ::ConvertToSysUnits(1.0,unitMeasure::Millimeter); 

   Float64 all_span = this->GetAllowableSpanBetweenSupportLocations();
   Float64 span = this->GetClearSpanBetweenSupportLocations();
   if (span>all_span+TOL_DIST)
      return false;

   Float64 allow_overhang = this->GetAllowableLeadingOverhang();
   Float64 overhang = this->GetLeadingOverhang();
   if ( overhang > allow_overhang+TOL_DIST )
      return false;


   if ( GetGirderWeight() > GetMaxGirderWgt() )
      return false;

   return true;
}

Float64 pgsHaulingCheckArtifact::GetAllowableSpanBetweenSupportLocations() const
{
   return m_AllowableSpanBetweenSupportLocations;
}

void pgsHaulingCheckArtifact::SetAllowableSpanBetweenSupportLocations(Float64 val)
{
   m_AllowableSpanBetweenSupportLocations = val;
}

Float64 pgsHaulingCheckArtifact::GetAllowableLeadingOverhang() const
{
   return m_AllowableLeadingOverhang;
}

void pgsHaulingCheckArtifact::SetAllowableLeadingOverhang(Float64 val)
{
   m_AllowableLeadingOverhang = val;
}

Float64 pgsHaulingCheckArtifact::GetAllowableTensileStress() const
{
   return m_AllowableTensileStress;
}

void pgsHaulingCheckArtifact::SetAllowableTensileStress(Float64 val)
{
   m_AllowableTensileStress = val;
}

Float64 pgsHaulingCheckArtifact::GetAllowableCompressionStress() const
{
   return m_AllowableCompressionStress;
}

void pgsHaulingCheckArtifact::SetAllowableCompressionStress(Float64 val)
{
   m_AllowableCompressionStress = val;
}

Float64 pgsHaulingCheckArtifact::GetAlternativeTensionAllowableStress() const
{
    return m_AllowableAlternativeTensileStress;
}

void pgsHaulingCheckArtifact::SetAlternativeTensionAllowableStress(Float64 val)
{
    m_AllowableAlternativeTensileStress = val;
}

Float64 pgsHaulingCheckArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsHaulingCheckArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsHaulingCheckArtifact::GetAllowableFsForRollover() const
{
   return m_AllowableFsForRollover;
}

void pgsHaulingCheckArtifact::SetAllowableFsForRollover(Float64 val)
{
   m_AllowableFsForRollover = val;
}

pgsHaulingStressCheckArtifact pgsHaulingCheckArtifact::GetHaulingStressCheckArtifact(Float64 distFromStart) const
{
   pgsHaulingStressCheckArtifact stressArtifact(*this);
   const pgsHaulingStressAnalysisArtifact* pArtifact = GetHaulingStressAnalysisArtifact(distFromStart);
   if ( pArtifact == NULL )
      return stressArtifact;
 
   pgsHaulingStressCheckArtifact* pStressArtifact = &stressArtifact;
   (*(pgsHaulingStressAnalysisArtifact*)pStressArtifact) = *pArtifact;
   return stressArtifact;
}

pgsHaulingCrackingCheckArtifact pgsHaulingCheckArtifact::GetHaulingCrackingCheckArtifact(Float64 distFromStart) const
{
   pgsHaulingCrackingCheckArtifact crackingArtifact(*this);
   const pgsHaulingCrackingAnalysisArtifact* pArtifact = GetHaulingCrackingAnalysisArtifact(distFromStart);
   if ( pArtifact == NULL )
      return crackingArtifact;

   pgsHaulingCrackingCheckArtifact* pCrackingArtifact = &crackingArtifact;
   (*(pgsHaulingCrackingAnalysisArtifact*)pCrackingArtifact) = *pArtifact;
   return crackingArtifact;
}

Float64 pgsHaulingCheckArtifact::GetMaxGirderWgt() const
{
   return m_MaxGirderWgt;
}

void pgsHaulingCheckArtifact::SetMaxGirderWgt(Float64 wgt)
{
   m_MaxGirderWgt = wgt;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHaulingCheckArtifact::MakeCopy(const pgsHaulingCheckArtifact& rOther)
{
   pgsHaulingAnalysisArtifact::MakeCopy(rOther);

   m_AllowableSpanBetweenSupportLocations = rOther.m_AllowableSpanBetweenSupportLocations;
   m_AllowableLeadingOverhang = rOther.m_AllowableLeadingOverhang;
   m_AllowableTensileStress = rOther.m_AllowableTensileStress;
   m_AllowableCompressionStress = rOther.m_AllowableCompressionStress;
   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_AllowableFsForRollover = rOther.m_AllowableFsForRollover;
   m_MaxGirderWgt           = rOther.m_MaxGirderWgt;
   m_AllowableAlternativeTensileStress = rOther.m_AllowableAlternativeTensileStress;
}

void pgsHaulingCheckArtifact::MakeAssignment(const pgsHaulingCheckArtifact& rOther)
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
bool pgsHaulingCheckArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingCheckArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHaulingCheckArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsHaulingCheckArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsHaulingCheckArtifact");


   TESTME_EPILOG("HaulingCheckArtifact");
}
#endif // _UNITTEST
