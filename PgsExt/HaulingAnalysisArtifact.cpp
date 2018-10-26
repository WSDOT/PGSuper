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
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\GirderLabel.h>

#include "PGSuperUnits.h"

#include <Reporting\ReportNotes.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\GirderArtifact.h>

#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>

#include <limits>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsWsdotHaulingStressAnalysisArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsWsdotHaulingStressAnalysisArtifact::pgsWsdotHaulingStressAnalysisArtifact():
m_EffectiveHorizPsForce(0.0),
m_EccentricityPsForce(0.0),
m_LateralMoment(0.0),
m_MomentUpward(0.0),
m_MomentNoImpact(0.0),
m_MomentDownward(0.0),
m_TopFiberStressPrestress(0.0),
m_TopFiberStressUpward(0.0),
m_TopFiberStressNoImpact(0.0),
m_TopFiberStressDownward(0.0),
m_BottomFiberStressPrestress(0.0),
m_BottomFiberStressUpward(0.0),
m_BottomFiberStressNoImpact(0.0),
m_BottomFiberStressDownward(0.0),
m_ftu(0.0),
m_ftd(0.0),
m_fbu(0.0),
m_fbd(0.0),
m_AllowableCompression(0.0),
m_AllowableInclinedTension(0.0),
m_ReqdCompConcreteStrength(0.0),
m_ReqdTensConcreteStrengthNoRebar(0.0),
m_ReqdTensConcreteStrengthWithRebar(0.0),
m_WasRebarReqd(false)
{
   memset(m_Yna,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_At,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_T,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_AsReqd,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_AsProvd,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_fAllow,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
}

pgsWsdotHaulingStressAnalysisArtifact::pgsWsdotHaulingStressAnalysisArtifact(const pgsWsdotHaulingStressAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsWsdotHaulingStressAnalysisArtifact::~pgsWsdotHaulingStressAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsWsdotHaulingStressAnalysisArtifact& pgsWsdotHaulingStressAnalysisArtifact::operator= (const pgsWsdotHaulingStressAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsWsdotHaulingStressAnalysisArtifact::TensionPassedPlumbGirder() const
{
   Float64 fTop, fBottom, CapacityTop, CapacityBottom;
   GetMaxPlumbTensileStress(&fTop, &fBottom, &CapacityTop, &CapacityBottom);
   if ( IsGT(CapacityTop,fTop) )
   {
      return false;
   }
   else if ( IsGT(CapacityBottom,fBottom) )
   {
      return false;
   }

   return true;
}

bool pgsWsdotHaulingStressAnalysisArtifact::TensionPassedInclinedGirder() const
{
   Float64 ftu,ftd,fbu,fbd;
   GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);
   Float64 fmax = Max(ftu,ftd,fbu,fbd);
   Float64 fcap = GetInclinedTensileCapacity();
   if (  IsGT(fcap,fmax) )
   {
      return false;
   }

   return true;
}

bool pgsWsdotHaulingStressAnalysisArtifact::TensionPassed() const
{
   return TensionPassedPlumbGirder() && TensionPassedInclinedGirder();
}

bool pgsWsdotHaulingStressAnalysisArtifact::CompressionPassedPlumbGirder() const
{
   Float64 comp_stress = GetMaximumConcreteCompressiveStress();
   Float64 max_comp_stress = GetCompressiveCapacity();

   if (comp_stress < max_comp_stress)
      return false;

   return true;
}

bool pgsWsdotHaulingStressAnalysisArtifact::CompressionPassedInclinedGirder() const
{
   Float64 comp_stress = GetMaximumInclinedConcreteCompressiveStress();

   Float64 max_comp_stress = GetCompressiveCapacity();

   if (comp_stress < max_comp_stress)
   {
      return false;
   }

   return true;
}

bool pgsWsdotHaulingStressAnalysisArtifact::CompressionPassed() const
{
   return CompressionPassedPlumbGirder() && CompressionPassedInclinedGirder();
}

bool pgsWsdotHaulingStressAnalysisArtifact::Passed() const
{
    return TensionPassed() && CompressionPassed();
}

//======================== ACCESS     =======================================
void pgsWsdotHaulingStressAnalysisArtifact::SetCompressiveCapacity(Float64 fAllowable)
{
   m_AllowableCompression = fAllowable;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetCompressiveCapacity() const
{
   return m_AllowableCompression;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetInclinedTensileCapacity(Float64 fAllowable)
{
   m_AllowableInclinedTension = fAllowable;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetInclinedTensileCapacity() const
{
   return m_AllowableInclinedTension;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetEffectiveHorizPsForce() const
{
   return m_EffectiveHorizPsForce;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetEffectiveHorizPsForce(Float64 f)
{
   m_EffectiveHorizPsForce = f;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetEccentricityPsForce() const
{
   return m_EccentricityPsForce;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetEccentricityPsForce(Float64 f)
{
   m_EccentricityPsForce = f;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetLateralMoment() const
{
   return m_LateralMoment;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetLateralMoment(Float64 mom)
{
   m_LateralMoment = mom;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetMomentImpact(Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward = m_MomentUpward;
   *pNoImpact = m_MomentNoImpact;
   *pDownward = m_MomentDownward;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetMomentImpact(Float64 upward, Float64 noImpact, Float64 downward)
{
   m_MomentUpward   = upward;
   m_MomentNoImpact = noImpact;
   m_MomentDownward = downward;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetTopFiberStress(Float64* pPs, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPs       = m_TopFiberStressPrestress;
    *pUpward   = m_TopFiberStressUpward;
    *pNoImpact = m_TopFiberStressNoImpact;
    *pDownward = m_TopFiberStressDownward;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetTopFiberStress(Float64 Ps, Float64 upward, Float64 noImpact, Float64 downward)
{
   m_TopFiberStressPrestress= Ps;
   m_TopFiberStressUpward   = upward;
   m_TopFiberStressNoImpact = noImpact;
   m_TopFiberStressDownward = downward;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetBottomFiberStress(Float64* pPs, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPs       = m_BottomFiberStressPrestress;
    *pUpward   = m_BottomFiberStressUpward;
    *pNoImpact = m_BottomFiberStressNoImpact;
    *pDownward = m_BottomFiberStressDownward;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetBottomFiberStress(Float64 Ps,Float64 upward, Float64 noImpact, Float64 downward)
{
   m_BottomFiberStressPrestress= Ps;
   m_BottomFiberStressUpward   = upward;
   m_BottomFiberStressNoImpact = noImpact;
   m_BottomFiberStressDownward = downward;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetMaxPlumbCompressiveStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const
{
   // Compression is easy: Allowable cannot change
   Float64 fps, fNone;
   Float64 fTopUp, fTopDown;
   this->GetTopFiberStress(&fps, &fTopUp, &fNone, &fTopDown);
   *fTop    = Max(fTopUp, fTopDown);

   Float64 fBotUp, fBotDown;
   this->GetBottomFiberStress(&fps, &fBotUp, &fNone, &fBotDown);
   *fBottom = Max(fBotUp, fBotDown);
   *Capacity = m_AllowableCompression;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetMaxPlumbTensileStress(Float64* pfTop, Float64* pfBottom, Float64* pCapacityTop, Float64* pCapacityBottom) const
{
   // Tensile allowable can change based on location. Most find max based on C/D
   Float64 capUp, capNone, capDown;
   GetTensileCapacities(&capUp, &capNone, &capDown);
   ATLASSERT(0.0 < capUp && 0.0 < capNone && 0.0 < capDown);

   // Do top of girder first
   Float64 fps;
   Float64 fTopUp, fTopNone, fTopDown;
   this->GetTopFiberStress(&fps, &fTopUp, &fTopNone, &fTopDown);

   // Use inline function from above
   DetermineControllingTensileStress(fTopUp, fTopNone, fTopDown, capUp, capNone, capDown,
                                     pfTop, pCapacityTop);

   // Bottom
   Float64 fBottomUp, fBottomNone, fBottomDown;
   this->GetBottomFiberStress(&fps, &fBottomUp, &fBottomNone, &fBottomDown);

   DetermineControllingTensileStress(fBottomUp, fBottomNone, fBottomDown, capUp, capNone, capDown,
                                     pfBottom, pCapacityBottom);
}

void pgsWsdotHaulingStressAnalysisArtifact::GetInclinedGirderStresses(Float64* pftu,Float64* pftd,Float64* pfbu,Float64* pfbd) const
{
   *pftu = m_ftu;
   *pftd = m_ftd;
   *pfbu = m_fbu;
   *pfbd = m_fbd;
}

void pgsWsdotHaulingStressAnalysisArtifact::SetInclinedGirderStresses(Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd)
{
   m_ftu = ftu;
   m_ftd = ftd;
   m_fbu = fbu;
   m_fbd = fbd;
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetMaximumInclinedConcreteCompressiveStress() const
{
   return Min(m_ftu,m_ftd,m_fbu,m_fbd);
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetMaximumInclinedConcreteTensileStress() const
{
   return Max(m_ftu,m_ftd,m_fbu,m_fbd);
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetMaximumConcreteCompressiveStress() const
{
   return Min(m_TopFiberStressUpward, m_TopFiberStressDownward, m_BottomFiberStressUpward, m_BottomFiberStressDownward);
}

Float64 pgsWsdotHaulingStressAnalysisArtifact::GetMaximumConcreteTensileStress() const
{
   return Max(m_TopFiberStressUpward, m_TopFiberStressDownward, m_BottomFiberStressUpward, m_BottomFiberStressDownward);
}

void pgsWsdotHaulingStressAnalysisArtifact::SetAlternativeTensileStressParameters(ImpactDir impactdir, Float64 Yna,   Float64 At,   Float64 T,
                                                                             Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow)
{
   m_Yna[impactdir] = Yna;
   m_At[impactdir] = At;
   m_T[impactdir] = T;
   m_AsReqd[impactdir] = AsReqd;
   m_AsProvd[impactdir] = AsProvd;
   m_fAllow[impactdir] = fAllow;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetAlternativeTensileStressParameters(enum ImpactDir impactdir, Float64* Yna,   Float64* At,   Float64* T,  
                                                                             Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const
{
   *Yna   = m_Yna[impactdir];
   *At    = m_At[impactdir];
   *T     = m_T[impactdir];
   *AsReqd   = m_AsReqd[impactdir];
   *AsProvd  = m_AsProvd[impactdir];
   *fAllow   = m_fAllow[impactdir];
}

void pgsWsdotHaulingStressAnalysisArtifact::GetTensileCapacities(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward   = m_fAllow[idUp];
   *pNoImpact = m_fAllow[idNone];
   *pDownward = m_fAllow[idDown];
}

void pgsWsdotHaulingStressAnalysisArtifact::SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar)
{
   m_ReqdCompConcreteStrength = fciComp;
   m_ReqdTensConcreteStrengthNoRebar = fciTensNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = fciTensWithRebar;
}

void pgsWsdotHaulingStressAnalysisArtifact::GetRequiredConcreteStrength(Float64* pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const
{
   *pfciComp = m_ReqdCompConcreteStrength;
   *pfciTensNoRebar = m_ReqdTensConcreteStrengthNoRebar;
   *pfciTensWithRebar = m_ReqdTensConcreteStrengthWithRebar;   
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsWsdotHaulingStressAnalysisArtifact::MakeCopy(const pgsWsdotHaulingStressAnalysisArtifact& rOther)
{
   m_EffectiveHorizPsForce     = rOther.m_EffectiveHorizPsForce;
   m_EccentricityPsForce       = rOther.m_EccentricityPsForce;
   m_LateralMoment             = rOther.m_LateralMoment;

   m_MomentUpward              = rOther.m_MomentUpward;
   m_MomentNoImpact            = rOther.m_MomentNoImpact;
   m_MomentDownward            = rOther.m_MomentDownward;
   m_TopFiberStressPrestress   = rOther.m_TopFiberStressPrestress;
   m_TopFiberStressUpward      = rOther.m_TopFiberStressUpward;
   m_TopFiberStressNoImpact    = rOther.m_TopFiberStressNoImpact;
   m_TopFiberStressDownward    = rOther.m_TopFiberStressDownward;
   m_BottomFiberStressPrestress= rOther.m_BottomFiberStressPrestress;
   m_BottomFiberStressUpward   = rOther.m_BottomFiberStressUpward;
   m_BottomFiberStressNoImpact = rOther.m_BottomFiberStressNoImpact;
   m_BottomFiberStressDownward = rOther.m_BottomFiberStressDownward;

   m_ftu = rOther.m_ftu;
   m_ftd = rOther.m_ftd;
   m_fbu = rOther.m_fbu;
   m_fbd = rOther.m_fbd;

   std::copy(rOther.m_Yna, rOther.m_Yna+SIZE_OF_IMPACTDIR, m_Yna);
   std::copy(rOther.m_At, rOther.m_At+SIZE_OF_IMPACTDIR, m_At);
   std::copy(rOther.m_T, rOther.m_T+SIZE_OF_IMPACTDIR, m_T);
   std::copy(rOther.m_AsReqd, rOther.m_AsReqd+SIZE_OF_IMPACTDIR, m_AsReqd);
   std::copy(rOther.m_AsProvd, rOther.m_AsProvd+SIZE_OF_IMPACTDIR, m_AsProvd);
   std::copy(rOther.m_fAllow, rOther.m_fAllow+SIZE_OF_IMPACTDIR, m_fAllow);

   m_AllowableCompression = rOther.m_AllowableCompression;
   m_AllowableInclinedTension = rOther.m_AllowableInclinedTension;
   m_ReqdCompConcreteStrength = rOther.m_ReqdCompConcreteStrength;
   m_ReqdTensConcreteStrengthNoRebar   = rOther.m_ReqdTensConcreteStrengthNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = rOther.m_ReqdTensConcreteStrengthWithRebar;
}

void pgsWsdotHaulingStressAnalysisArtifact::MakeAssignment(const pgsWsdotHaulingStressAnalysisArtifact& rOther)
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
bool pgsWsdotHaulingStressAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsWsdotHaulingStressAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsWsdotHaulingStressAnalysisArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsWsdotHaulingCrackingAnalysisArtifact
****************************************************************************/

#include <PgsExt\HaulingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsWsdotHaulingCrackingAnalysisArtifact::pgsWsdotHaulingCrackingAnalysisArtifact() :
m_VerticalMoment(0.0),
m_LateralMoment(0.0),
m_ThetaCrackingMax(0.0),
m_FsCracking(0.0),
m_AllowableFsForCracking(0.0),
m_CrackedFlange(BottomFlange),
m_LateralMomentStress(0.0)
{
}

pgsWsdotHaulingCrackingAnalysisArtifact::pgsWsdotHaulingCrackingAnalysisArtifact(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsWsdotHaulingCrackingAnalysisArtifact::~pgsWsdotHaulingCrackingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsWsdotHaulingCrackingAnalysisArtifact& pgsWsdotHaulingCrackingAnalysisArtifact::operator= (const pgsWsdotHaulingCrackingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool  pgsWsdotHaulingCrackingAnalysisArtifact::Passed() const
{
   Float64 all_fs = GetAllowableFsForCracking();
   Float64 fs = GetFsCracking();

   return all_fs <= fs;
}
//======================== ACCESS     =======================================
Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetVerticalMoment() const
{
   return m_VerticalMoment;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetVerticalMoment(Float64 m)
{
   m_VerticalMoment = m;
}

Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetLateralMoment() const
{
   return m_LateralMoment;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetLateralMoment(Float64 m)
{
   m_LateralMoment = m;
}

Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetLateralMomentStress() const
{
   return m_LateralMomentStress;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetLateralMomentStress(Float64 m)
{
   m_LateralMomentStress = m;
}

CrackedFlange pgsWsdotHaulingCrackingAnalysisArtifact::GetCrackedFlange() const
{
   return m_CrackedFlange;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetCrackedFlange(CrackedFlange flange)
{
   m_CrackedFlange = flange;
}

Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetThetaCrackingMax() const
{
   return m_ThetaCrackingMax;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetThetaCrackingMax(Float64 t)
{
   m_ThetaCrackingMax = t;
}

Float64 pgsWsdotHaulingCrackingAnalysisArtifact::GetFsCracking() const
{
   return m_FsCracking;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::SetFsCracking(Float64 fs)
{
   m_FsCracking = fs;
}



//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsWsdotHaulingCrackingAnalysisArtifact::MakeCopy(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther)
{
   m_VerticalMoment   = rOther.m_VerticalMoment;
   m_LateralMoment    = rOther.m_LateralMoment;
   m_LateralMomentStress= rOther.m_LateralMomentStress;
   m_ThetaCrackingMax = rOther.m_ThetaCrackingMax;
   m_FsCracking       = rOther.m_FsCracking;
   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_CrackedFlange       = rOther.m_CrackedFlange;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::MakeAssignment(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther)
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
bool pgsWsdotHaulingCrackingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsWsdotHaulingCrackingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsWsdotHaulingCrackingAnalysisArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsWsdotHaulingAnalysisArtifact
****************************************************************************/

#include <PgsExt\HaulingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsWsdotHaulingAnalysisArtifact::pgsWsdotHaulingAnalysisArtifact():
m_GirderLength(0.0),
m_GirderWeightPerLength(0.0),
m_ClearSpanBetweenSupportLocations(0.0),
m_HeightOfRollCenterAboveRoadway(0.0),
m_HeightOfCgOfGirderAboveRollCenter(0.0),
m_RollStiffnessOfvehicle(0.0),
m_AxleWidth(0.0),
m_RoadwaySuperelevation(0.0),
m_UpwardImpact(0.0),
m_DownwardImpact(0.0),
m_SweepTolerance(0.0),
m_SupportPlacementTolerance(0.0),
m_Fc(0.0),
m_ModulusOfRupture(0.0),
m_Krupture(0.0),
m_ElasticModulusOfGirderConcrete(0.0),
m_EccentricityDueToSweep(0.0),
m_EccentricityDueToPlacementTolerance(0.0),
m_OffsetFactor(0.0),
m_TotalInitialEccentricity(0.0),
m_RadiusOfStability(0.0),
m_Ix(0.0),
m_Iy(0.0),
m_Zo(0.0),
m_ZoPrime(0.0),
m_ThetaRolloverMax(0.0),
m_EqualibriumAngle(0.0),
m_FsRollover(0.0),
m_LeadingOverhang(0.0),
m_TrailingOverhang(0.0),
m_GirderWeight(0.0), // total girder weight
m_C(0.0),
m_T(0.0),
m_bfmax(false),
m_fmax(0.0),
m_Talt(0.0),
m_AllowableSpanBetweenSupportLocations(0.0),
m_AllowableLeadingOverhang(0.0),
m_AllowableFsForCracking(0.0),
m_AllowableFsForRollover(0.0),
m_MaxGirderWgt(0.0)
{
}

pgsWsdotHaulingAnalysisArtifact::pgsWsdotHaulingAnalysisArtifact(const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsWsdotHaulingAnalysisArtifact::~pgsWsdotHaulingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsWsdotHaulingAnalysisArtifact& pgsWsdotHaulingAnalysisArtifact::operator= (const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsWsdotHaulingAnalysisArtifact::Passed() const
{
   // cracking
   Float64 fs_crack = this->GetMinFsForCracking();
   Float64 all_crack = this->GetAllowableFsForCracking();
   if (fs_crack < all_crack)
   {
      return false;
   }

   // Rollover
   Float64 fs_roll = this->GetFsRollover();
   Float64 all_roll = this->GetAllowableFsForRollover();
   if (fs_roll<all_roll)
   {
      return false;
   }

   // stresses
   if (!PassedStressCheck())
   {
      return false;
   }

   // tolerance for distances
   Float64 TOL_DIST = ::ConvertToSysUnits(1.0,unitMeasure::Millimeter); 

   Float64 all_span = this->GetAllowableSpanBetweenSupportLocations();
   Float64 span = this->GetClearSpanBetweenSupportLocations();
   if (all_span+TOL_DIST < span)
   {
      return false;
   }

   Float64 allow_overhang = this->GetAllowableLeadingOverhang();
   Float64 overhang = this->GetLeadingOverhang();
   if ( allow_overhang+TOL_DIST < overhang)
   {
      return false;
   }

   if ( GetMaxGirderWgt() < GetGirderWeight() )
   {
      return false;
   }

   return true;
}

bool pgsWsdotHaulingAnalysisArtifact::PassedStressCheck() const
{
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iter(m_HaulingStressAnalysisArtifacts.begin()); 
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iterEnd(m_HaulingStressAnalysisArtifacts.end()); 
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsWsdotHaulingStressAnalysisArtifact& artifact = iter->second;

      if (!artifact.Passed())
      {
         return false;
      }
   }

   return true;
}

pgsHaulingAnalysisArtifact* pgsWsdotHaulingAnalysisArtifact::Clone() const
{
   std::auto_ptr<pgsWsdotHaulingAnalysisArtifact> clone(new pgsWsdotHaulingAnalysisArtifact());
   *clone = *this;

   return clone.release();
}

void pgsWsdotHaulingAnalysisArtifact::BuildHaulingCheckReport(const CSegmentKey& segmentKey,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   if( BuildImpactedStressTable(segmentKey, pChapter,pBroker,pDisplayUnits) )
   {
      BuildInclinedStressTable(segmentKey, pChapter,pBroker,pDisplayUnits);

      BuildOtherTables(pChapter,pBroker,pDisplayUnits);
   }
}

void  pgsWsdotHaulingAnalysisArtifact::BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
   INIT_UV_PROTOTYPE( rptPointOfInterest,         location, pDisplayUnits->GetSpanLengthUnit(),          false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         loc,      pDisplayUnits->GetSpanLengthUnit(),          false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,    pDisplayUnits->GetShearUnit(),               false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,      pDisplayUnits->GetComponentDimUnit(),        false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         stress,   pDisplayUnits->GetStressUnit(),              false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,         mod_e,    pDisplayUnits->GetModEUnit(),                false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,         moment,   pDisplayUnits->GetMomentUnit(),              false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,          angle,    pDisplayUnits->GetRadAngleUnit(),            false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, wt_len,   pDisplayUnits->GetForcePerLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptMomentPerAngleUnitValue, spring,   pDisplayUnits->GetMomentPerAngleUnit(),      false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,           area,     pDisplayUnits->GetAreaUnit(),                false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,        mom_I,    pDisplayUnits->GetMomentOfInertiaUnit(),     true  );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   SegmentIndexType nSegments = pBridgeDesc->GetGirder(segmentKey)->GetSegmentCount();
   if ( 1 < nSegments )
   {
      rptParagraph* p = new rptParagraph(rptStyleManager::GetSubheadingStyle() );
      *pChapter << p;
      *p << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << rptNewLine;
   }

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Details for Check for Hauling to Bridge Site [5.5.4.3][5.9.4.1]")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;


   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (!pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
   }

   *p << Sub2(_T("l"),_T("g")) << _T(" = Overall Length of girder = ")<<loc.SetValue(this->GetGirderLength())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;

   Float64 leadingOH  = this->GetLeadingOverhang();
   Float64 trailingOH = this->GetTrailingOverhang();

   FormatDimension(leadingOH,pDisplayUnits->GetSpanLengthUnit());
   *p << _T("Leading Overhang = ")<<loc.SetValue(leadingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("Trailing Overhang = ")<<loc.SetValue(trailingOH)<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("l"),_T("l"))<<_T(" = Clear span length between supports = ")<<loc.SetValue(this->GetClearSpanBetweenSupportLocations())<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << _T("w = average girder weight/length = ")<<wt_len.SetValue(this->GetAvgGirderWeightPerLength())<<_T(" ")<<_T(" ")<<wt_len.GetUnitTag()<<rptNewLine;
   *p << _T("W = girder weight = ")<<force.SetValue(this->GetGirderWeight())<<_T(" ")<<_T(" ")<<force.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("I"),_T("y")) << _T(" = ") << mom_I.SetValue(this->GetIy())<< rptNewLine;
   *p << Sub2(_T("h"),_T("r"))<<_T(" = Height of roll center above roadway = ")<<dim.SetValue(this->GetHeightOfRollCenterAboveRoadway())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("y = height of C.G. of Girder above roll center (Increased for camber by 2 percent) =")<<dim.SetValue(this->GetHeightOfCgOfGirderAboveRollCenter())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("K"),symbol(theta))<<_T(" = roll stiffness of vehicle = ")<<spring.SetValue(this->GetRollStiffnessOfvehicle())<<_T(" ")<<spring.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("z"),_T("max"))<<_T(" = distance from center of vehicle to center of dual tires = ")<<dim.SetValue(this->GetAxleWidth()/2.0)<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << symbol(alpha)<<_T(" = Roadway superelevation = ")<<scalar.SetValue(this->GetRoadwaySuperelevation())<<rptNewLine;
   *p << _T("Upward Impact during Hauling = ")<<this->GetUpwardImpact()<<rptNewLine;
   *p << _T("Downward Impact during Hauling = ")<<this->GetDownwardImpact()<<rptNewLine;
   *p << _T("Sweep tolerance = ")<<this->GetSweepTolerance()<<loc.GetUnitTag()<<_T("/")<<_T(" ")<<loc.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("e"),_T("truck"))<<_T(" = tolerance in placement of supports =")<<dim.SetValue(this->GetSupportPlacementTolerance())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << RPT_STRESS(_T("c")) << _T(" = concrete strength = ") << stress.SetValue(this->GetConcreteStrength()) << _T(" ") << stress.GetUnitTag() << rptNewLine;
   *p << RPT_STRESS(_T("r"))<<_T(" = modulus of rupture at Hauling = ")<<stress.SetValue(this->GetModRupture())<<_T(" ")<<stress.GetUnitTag()<<rptNewLine;
   *p << _T("Elastic modulus of girder concrete at Hauling = ")<<mod_e.SetValue(this->GetElasticModulusOfGirderConcrete())<<_T(" ")<<mod_e.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("e"),_T("s"))<<_T(" = eccentricity due to sweep = ")<<dim.SetValue(this->GetEccentricityDueToSweep())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("F"),_T("o"))<<_T(" = offset factor = ") << _T("(") << Sub2(_T("l"),_T("l")) << _T("/") << Sub2(_T("l"),_T("g")) << _T(")") << Super(_T("2")) << _T(" - 1/3 = ") << this->GetOffsetFactor()<<rptNewLine;
   *p << Sub2(_T("e"),_T("i"))<<_T(" = total initial eccentricity = ") << Sub2(_T("e"),_T("s"))<<_T("*") << Sub2(_T("F"),_T("o"))<<_T(" + ") << Sub2(_T("e"),_T("truck"))<<_T(" = ")<<dim.SetValue(this->GetTotalInitialEccentricity())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(std::_tstring(std::_tstring(rptStyleManager::GetImagePath())) + _T("zo.png") )<<_T(" = ")<<dim.SetValue(this->GetZo())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << Sub2(_T("z"),_T("o")) << _T(" is based on average girder unit weight and mid-span section properties") << rptNewLine;
   *p << _T("r = Radius of stability = ") << Sub2(_T("K"),symbol(theta))<<_T("/W = ")<<dim.SetValue(this->GetRadiusOfStability())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << _T("Equilibrium angle = ")<<rptRcImage(std::_tstring(std::_tstring(rptStyleManager::GetImagePath())) + _T("ThetaHauling.png") )<<_T(" = ")<<angle.SetValue(this->GetEqualibriumAngle())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;
   *p << _T("Lateral Moment = (Vertical Moment)(Equilibrium Angle)") << rptNewLine;

   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;

   *pTitle << _T("Girder Forces and Stresses At Hauling");
   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(6,_T("Hauling Forces"));
   *p << p_table<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("Effective") << rptNewLine << _T("Prestress") << rptNewLine << _T("Force"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*p_table)(0,2) << COLHDR(_T("Eccentricity"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(0,3) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Up"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(_T("Moment") << rptNewLine << _T("No Impact"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(_T("Moment") << rptNewLine << _T("Impact Down"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(m_HaulingPois.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(m_HaulingPois.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingStressAnalysisArtifact* pStressArtifact = GetHaulingStressAnalysisArtifact(poi);
 
      (*p_table)(row,0) << location.SetValue( POI_HAUL_SEGMENT, poi );
      (*p_table)(row,1) << force.SetValue( pStressArtifact->GetEffectiveHorizPsForce());
      (*p_table)(row,2) << dim.SetValue( pStressArtifact->GetEccentricityPsForce());

      Float64 M1,M2,M3;
      pStressArtifact->GetMomentImpact(&M1,&M2,&M3);
      (*p_table)(row,3) << moment.SetValue(M1);
      (*p_table)(row,4) << moment.SetValue(M2);
      (*p_table)(row,5) << moment.SetValue(M3);
      row++;
   }

   p_table = rptStyleManager::CreateDefaultTable(9,_T("Hauling Stresses - Plumb Girder"));
   *p << p_table;

   p_table->SetNumberOfHeaderRows(2);
   p_table->SetRowSpan(0,0,2);
   p_table->SetRowSpan(1,0,SKIP_CELL);
   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,1,4);
   (*p_table)(0,1) << _T("Top Stress, ") << RPT_FTOP;

   p_table->SetColumnSpan(0,2,4);
   (*p_table)(0,2) << _T("Bottom Stress, ") << RPT_FBOT;

   p_table->SetColumnSpan(0,3,SKIP_CELL);
   p_table->SetColumnSpan(0,4,SKIP_CELL);
   p_table->SetColumnSpan(0,5,SKIP_CELL);
   p_table->SetColumnSpan(0,6,SKIP_CELL);
   p_table->SetColumnSpan(0,7,SKIP_CELL);
   p_table->SetColumnSpan(0,8,SKIP_CELL);

   (*p_table)(1,1) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,2) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,3) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,4) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,5) << COLHDR(_T("Prestress"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,6) << COLHDR(_T("Impact") << rptNewLine << _T("Up"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,7) << COLHDR(_T("No") << rptNewLine << _T("Impact"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,8) << COLHDR(_T("Impact") << rptNewLine << _T("Down"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   rptRcTable* p_table2 = rptStyleManager::CreateDefaultTable(7,_T("Hauling Stresses - Tilted Girder"));
   *p << p_table2;
   *p << RPT_STRESS(_T("tu")) << _T(" = top fiber stress, uphill side") << rptNewLine;
   *p << RPT_STRESS(_T("td")) << _T(" = top fiber stress, downhill side") << rptNewLine;
   *p << RPT_STRESS(_T("bu")) << _T(" = bottom fiber stress, uphill side") << rptNewLine;
   *p << RPT_STRESS(_T("bd")) << _T(" = bottom fiber stress, downhill side") << rptNewLine;
   (*p_table2)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table2)(0,1) << COLHDR(Sub2(_T("M"),_T("vert")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,2) << COLHDR(Sub2(_T("M"),_T("lat")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table2)(0,3) << COLHDR(RPT_STRESS(_T("tu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,4) << COLHDR(RPT_STRESS(_T("td")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,5) << COLHDR(RPT_STRESS(_T("bu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table2)(0,6) << COLHDR(RPT_STRESS(_T("bd")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   RowIndexType row1 = 2;
   RowIndexType row2 = 1;
   poiIter = m_HaulingPois.begin();
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingStressAnalysisArtifact* pStressArtifact = GetHaulingStressAnalysisArtifact(poi);
 
      (*p_table)(row1,0) << location.SetValue( POI_HAUL_SEGMENT, poi );
      
      Float64 ps, up, down, no;
      pStressArtifact->GetTopFiberStress(&ps, &up,&no,&down);
      (*p_table)(row1,1) << stress.SetValue( ps );
      (*p_table)(row1,2) << stress.SetValue( up );
      (*p_table)(row1,3) << stress.SetValue( no );
      (*p_table)(row1,4) << stress.SetValue( down );
      
      pStressArtifact->GetBottomFiberStress(&ps,&up,&no,&down);
      (*p_table)(row1,5) << stress.SetValue( ps );
      (*p_table)(row1,6) << stress.SetValue( up );
      (*p_table)(row1,7) << stress.SetValue( no );
      (*p_table)(row1,8) << stress.SetValue( down );

      (*p_table2)(row2,0) << location.SetValue(POI_HAUL_SEGMENT, poi);

      Float64 Mu,Mn,Md;
      pStressArtifact->GetMomentImpact(&Mu,&Mn,&Md);

      Float64 lat_moment;
      lat_moment  = pStressArtifact->GetLateralMoment();
      (*p_table2)(row2,1) << moment.SetValue(Mn);
      (*p_table2)(row2,2) << moment.SetValue(lat_moment);

      Float64 ftu, ftd, fbu, fbd;
      pStressArtifact->GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);
      (*p_table2)(row2,3) << stress.SetValue(ftu);
      (*p_table2)(row2,4) << stress.SetValue(ftd);
      (*p_table2)(row2,5) << stress.SetValue(fbu);
      (*p_table2)(row2,6) << stress.SetValue(fbd);

      row1++;
      row2++;
   }

   // Rebar requirements tables
   BuildRebarTable(pBroker, pChapter, segmentKey, idDown);
   BuildRebarTable(pBroker, pChapter, segmentKey, idNone);
   BuildRebarTable(pBroker, pChapter, segmentKey, idUp);

   p = new rptParagraph;
   *pChapter << p;

   // FS Cracking
   p_table = rptStyleManager::CreateDefaultTable(7,_T("Factor of Safety Against Cracking"));
   *p << p_table << rptNewLine;
   *p << RPT_STRESS(_T("t")) << _T(" = governing tension stress")<<rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_STRESS(_T("t")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << _T("Governing") << rptNewLine << _T("Flange");
   (*p_table)(0,3) << COLHDR(Sub2(_T("M"),_T("lat")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,4) << COLHDR(Sub2(_T("M"),_T("vert")),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*p_table)(0,5) << COLHDR(Sub2(symbol(theta),_T("max")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit() );
   (*p_table)(0,6) << Sub2(_T("FS"),_T("cr"));

   row=1;
   poiIter = m_HaulingPois.begin();
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingCrackingAnalysisArtifact* pCrackArtifact = GetHaulingCrackingAnalysisArtifact(poi);
 
      (*p_table)(row,0) << location.SetValue( POI_HAUL_SEGMENT, poi);
      (*p_table)(row,1) << stress.SetValue( pCrackArtifact->GetLateralMomentStress() );

      if (pCrackArtifact->GetCrackedFlange()==BottomFlange)
      {
         (*p_table)(row,2) << _T("Bottom");
      }
      else
      {
         (*p_table)(row,2) << _T("Top");
      }

      (*p_table)(row,3) << moment.SetValue( pCrackArtifact->GetLateralMoment() );
      (*p_table)(row,4) << moment.SetValue( pCrackArtifact->GetVerticalMoment() );
      (*p_table)(row,5) << angle.SetValue(  pCrackArtifact->GetThetaCrackingMax() );

      Float64 FScr = pCrackArtifact->GetFsCracking();
      if ( FScr == Float64_Inf )
      {
         (*p_table)(row,6) << symbol(INFINITY);
      }
      else
      {
         (*p_table)(row,6) << scalar.SetValue( pCrackArtifact->GetFsCracking());
      }
      row++;
   }

   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Mlat.png") )<<rptNewLine;
   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ThetaMax.png") )<<rptNewLine;
   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("FScrHauling.png") )<<rptNewLine;

   // FS Rollover
   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Factor of Safety Against Rollover")<<rptNewLine;
   p = new rptParagraph;
   *pChapter << p;

   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ThetaPrimeMaxHauling.png") )<<_T(" = ")<<angle.SetValue(this->GetThetaRolloverMax())<<_T(" ")<<angle.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("zo_prime_hauling.png") )<<_T(" = ")<<dim.SetValue(this->GetZoPrime())<<_T(" ")<<dim.GetUnitTag()<<rptNewLine;
   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("FSrHauling.png") )<<_T(" = ")<<scalar.SetValue(this->GetFsRollover())<<rptNewLine;
}

void pgsWsdotHaulingAnalysisArtifact::Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile, IBroker* pBroker,
                                                    const std::_tstring& pid, const std::_tstring& bridgeId) const
{
   GET_IFACE2(pBroker,ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);

   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pSegmentHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,POI_5L);
   ATLASSERT(vPoi.size()==1);
   pgsPointOfInterest& poi = vPoi[0];
   Float64 loc = poi.GetDistFromStart();

   const pgsWsdotHaulingStressAnalysisArtifact* pStress = GetHaulingStressAnalysisArtifact(poi);

   GirderIndexType gdr = segmentKey.girderIndex;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100005, ")<<loc<<_T(", ")<< ::ConvertFromSysUnits(pStress->GetMaximumConcreteTensileStress() , unitMeasure::MPa) <<_T(", 50, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100006, ")<<loc<<_T(", ")<< ::ConvertFromSysUnits(pStress->GetMaximumConcreteCompressiveStress() , unitMeasure::MPa) <<_T(", 50, ")<<gdr<<std::endl;

   const pgsWsdotHaulingCrackingAnalysisArtifact* pCrack = GetHaulingCrackingAnalysisArtifact(poi);

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100007, ")<<loc<<_T(", ")<< pCrack->GetFsCracking()<<_T(", 50, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100008, ")<<loc<<_T(", ")<< GetFsRollover()<<_T(", 50, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100010, ")<<loc<<_T(", ")<<(int)(Passed()?1:0)<<_T(", 50, ")<<gdr<<std::endl;
}


//======================== ACCESS     =======================================

Float64 pgsWsdotHaulingAnalysisArtifact::GetAllowableSpanBetweenSupportLocations() const
{
   return m_AllowableSpanBetweenSupportLocations;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableSpanBetweenSupportLocations(Float64 val)
{
   m_AllowableSpanBetweenSupportLocations = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetAllowableLeadingOverhang() const
{
   return m_AllowableLeadingOverhang;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableLeadingOverhang(Float64 val)
{
   m_AllowableLeadingOverhang = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetAllowableFsForRollover() const
{
   return m_AllowableFsForRollover;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableFsForRollover(Float64 val)
{
   m_AllowableFsForRollover = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetMaxGirderWgt() const
{
   return m_MaxGirderWgt;
}

void pgsWsdotHaulingAnalysisArtifact::SetMaxGirderWgt(Float64 wgt)
{
   m_MaxGirderWgt = wgt;
}


Float64 pgsWsdotHaulingAnalysisArtifact::GetGirderLength() const
{
   return m_GirderLength;
}

void pgsWsdotHaulingAnalysisArtifact::SetGirderLength(Float64 val)
{
   m_GirderLength = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetLeadingOverhang() const
{
   return m_LeadingOverhang;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetTrailingOverhang() const
{
   return m_TrailingOverhang;
}

void pgsWsdotHaulingAnalysisArtifact::SetOverhangs(Float64 trailing,Float64 leading)
{
   m_LeadingOverhang  = leading;
   m_TrailingOverhang = trailing;
}

void pgsWsdotHaulingAnalysisArtifact::SetGirderWeight(Float64 val)
{
   m_GirderWeight = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetGirderWeight() const
{
   return m_GirderWeight;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetAvgGirderWeightPerLength() const
{
   return m_GirderWeight/m_GirderLength;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetClearSpanBetweenSupportLocations() const
{
   return m_ClearSpanBetweenSupportLocations;
}

void pgsWsdotHaulingAnalysisArtifact::SetClearSpanBetweenSupportLocations(Float64 val)
{
   m_ClearSpanBetweenSupportLocations = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetHeightOfRollCenterAboveRoadway() const
{
   return m_HeightOfRollCenterAboveRoadway;
}

void pgsWsdotHaulingAnalysisArtifact::SetHeightOfRollCenterAboveRoadway(Float64 val)
{
   m_HeightOfRollCenterAboveRoadway = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetHeightOfCgOfGirderAboveRollCenter() const
{
   return m_HeightOfCgOfGirderAboveRollCenter;
}

void pgsWsdotHaulingAnalysisArtifact::SetHeightOfCgOfGirderAboveRollCenter(Float64 val)
{
   m_HeightOfCgOfGirderAboveRollCenter = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetRollStiffnessOfvehicle() const
{
   return m_RollStiffnessOfvehicle;
}

void pgsWsdotHaulingAnalysisArtifact::SetRollStiffnessOfvehicle(Float64 val)
{
   m_RollStiffnessOfvehicle = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetAxleWidth() const
{
   return m_AxleWidth;
}

void pgsWsdotHaulingAnalysisArtifact::SetAxleWidth(Float64 val)
{
   m_AxleWidth = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetRoadwaySuperelevation() const
{
   return m_RoadwaySuperelevation;
}

void pgsWsdotHaulingAnalysisArtifact::SetRoadwaySuperelevation(Float64 val)
{
   m_RoadwaySuperelevation = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetUpwardImpact() const
{
   return m_UpwardImpact;
}

void pgsWsdotHaulingAnalysisArtifact::SetUpwardImpact(Float64 val)
{
   m_UpwardImpact = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetDownwardImpact() const
{
   return m_DownwardImpact;
}

void pgsWsdotHaulingAnalysisArtifact::SetDownwardImpact(Float64 val)
{
   m_DownwardImpact = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetSweepTolerance() const
{
   return m_SweepTolerance;
}

void pgsWsdotHaulingAnalysisArtifact::SetSweepTolerance(Float64 val)
{
   m_SweepTolerance = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetSupportPlacementTolerance() const
{
   return m_SupportPlacementTolerance;
}

void pgsWsdotHaulingAnalysisArtifact::SetSupportPlacementTolerance(Float64 val)
{
   m_SupportPlacementTolerance = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetConcreteStrength() const
{
   return m_Fc;
}

void pgsWsdotHaulingAnalysisArtifact::SetConcreteStrength(Float64 val)
{
   m_Fc = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetModRupture() const
{
   return m_ModulusOfRupture;
}

void pgsWsdotHaulingAnalysisArtifact::SetModRupture(Float64 val)
{
   m_ModulusOfRupture = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetModRuptureCoefficient() const
{
   return m_Krupture;
}

void pgsWsdotHaulingAnalysisArtifact::SetModRuptureCoefficient(Float64 val)
{
   m_Krupture = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetElasticModulusOfGirderConcrete() const
{
   return m_ElasticModulusOfGirderConcrete;
}

void pgsWsdotHaulingAnalysisArtifact::SetElasticModulusOfGirderConcrete(Float64 val)
{
   m_ElasticModulusOfGirderConcrete = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetEccentricityDueToSweep() const
{
   return m_EccentricityDueToSweep;
}

void pgsWsdotHaulingAnalysisArtifact::SetEccentricityDueToSweep(Float64 val)
{
   m_EccentricityDueToSweep = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetEccentricityDueToPlacementTolerance() const
{
   return m_EccentricityDueToPlacementTolerance;
}

void pgsWsdotHaulingAnalysisArtifact::SetEccentricityDueToPlacementTolerance(Float64 val)
{
   m_EccentricityDueToPlacementTolerance = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetOffsetFactor() const
{
   return m_OffsetFactor;
}

void pgsWsdotHaulingAnalysisArtifact::SetOffsetFactor(Float64 val)
{
   m_OffsetFactor = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetTotalInitialEccentricity() const
{
   return m_TotalInitialEccentricity;
}

void pgsWsdotHaulingAnalysisArtifact::SetTotalInitialEccentricity(Float64 val)
{
   m_TotalInitialEccentricity = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetIx() const
{
   return m_Ix;
}

void pgsWsdotHaulingAnalysisArtifact::SetIx(Float64 ix)
{
   m_Ix = ix;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetIy() const
{
   return m_Iy;
}

void pgsWsdotHaulingAnalysisArtifact::SetIy(Float64 iy)
{
   m_Iy = iy;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetZo() const
{
   return m_Zo;
}
void pgsWsdotHaulingAnalysisArtifact::SetZo(Float64 zo)
{
   m_Zo = zo;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetZoPrime() const
{
   return m_ZoPrime;
}

void pgsWsdotHaulingAnalysisArtifact::SetZoPrime(Float64 zo)
{
   m_ZoPrime = zo;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetEqualibriumAngle() const
{
   return m_EqualibriumAngle;
}

void pgsWsdotHaulingAnalysisArtifact::SetEqualibriumAngle(Float64 val)
{
   m_EqualibriumAngle = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetMinFsForCracking() const
{
   // cycle through all fs's at all pois and return min
   Float64 min_fs=DBL_MAX;
   ATLASSERT(0 < m_HaulingCrackingAnalysisArtifacts.size());
   std::map<pgsPointOfInterest,pgsWsdotHaulingCrackingAnalysisArtifact>::const_iterator iter(m_HaulingCrackingAnalysisArtifacts.begin()); 
   std::map<pgsPointOfInterest,pgsWsdotHaulingCrackingAnalysisArtifact>::const_iterator iterEnd(m_HaulingCrackingAnalysisArtifacts.end()); 
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsWsdotHaulingCrackingAnalysisArtifact& artifact = iter->second;
      Float64 fs = artifact.GetFsCracking();
      min_fs = Min(min_fs,fs);
   }
   return min_fs;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetFsRollover() const
{
   return m_FsRollover;
}

void pgsWsdotHaulingAnalysisArtifact::SetFsRollover(Float64 val)
{
   m_FsRollover = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetRadiusOfStability() const
{
   return m_RadiusOfStability;
}

void pgsWsdotHaulingAnalysisArtifact::SetRadiusOfStability(Float64 val)
{
   m_RadiusOfStability = val;
}

Float64 pgsWsdotHaulingAnalysisArtifact::GetThetaRolloverMax() const
{
   return m_ThetaRolloverMax;
}

void pgsWsdotHaulingAnalysisArtifact::SetThetaRolloverMax(Float64 t)
{
   m_ThetaRolloverMax = t;
}

void pgsWsdotHaulingAnalysisArtifact::AddHaulingStressAnalysisArtifact(const pgsPointOfInterest& poi,const pgsWsdotHaulingStressAnalysisArtifact& artifact)
{
   ATLASSERT(poi.GetID() != INVALID_ID);
   std::vector<pgsPointOfInterest>::iterator found = std::find(m_HaulingPois.begin(),m_HaulingPois.end(),poi);
   if ( found == m_HaulingPois.end() )
   {
      m_HaulingPois.push_back(poi);
      std::sort(m_HaulingPois.begin(),m_HaulingPois.end());
   }
   m_HaulingStressAnalysisArtifacts.insert(std::make_pair(poi,artifact));
}

const pgsWsdotHaulingStressAnalysisArtifact* pgsWsdotHaulingAnalysisArtifact::GetHaulingStressAnalysisArtifact(const pgsPointOfInterest& poi) const
{
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator found;
   found = m_HaulingStressAnalysisArtifacts.find( poi );
   if ( found == m_HaulingStressAnalysisArtifacts.end() )
   {
      return NULL;
   }

   return &(*found).second;
}


void pgsWsdotHaulingAnalysisArtifact::AddHaulingCrackingAnalysisArtifact(const pgsPointOfInterest& poi,const pgsWsdotHaulingCrackingAnalysisArtifact& artifact)
{
   ATLASSERT(poi.GetID() != INVALID_ID);
   std::vector<pgsPointOfInterest>::iterator found = std::find(m_HaulingPois.begin(),m_HaulingPois.end(),poi);
   if ( found == m_HaulingPois.end() )
   {
      m_HaulingPois.push_back(poi);
      std::sort(m_HaulingPois.begin(),m_HaulingPois.end());
   }
   m_HaulingCrackingAnalysisArtifacts.insert(std::make_pair(poi,artifact));

}

const pgsWsdotHaulingCrackingAnalysisArtifact* pgsWsdotHaulingAnalysisArtifact::GetHaulingCrackingAnalysisArtifact(const pgsPointOfInterest& poi) const
{
   std::map<pgsPointOfInterest,pgsWsdotHaulingCrackingAnalysisArtifact>::const_iterator found;
   found = m_HaulingCrackingAnalysisArtifacts.find( poi );
   if ( found == m_HaulingCrackingAnalysisArtifacts.end() )
   {
      return NULL;
   }

   return &(*found).second;
}

void pgsWsdotHaulingAnalysisArtifact::GetMinMaxStresses(Float64* minStress, Float64* maxStress) const
{
   ATLASSERT(0 < m_HaulingStressAnalysisArtifacts.size());

   Float64 min_stress = Float64_Max;
   Float64 max_stress = -Float64_Max;
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iter(m_HaulingStressAnalysisArtifacts.begin());
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iterEnd(m_HaulingStressAnalysisArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsWsdotHaulingStressAnalysisArtifact& artifact = iter->second;
      min_stress = Min(min_stress, artifact.GetMaximumConcreteCompressiveStress());
      max_stress = Max(max_stress, artifact.GetMaximumConcreteTensileStress());
   }

   *minStress = min_stress;
   *maxStress = max_stress;
}

const std::vector<pgsPointOfInterest>& pgsWsdotHaulingAnalysisArtifact::GetHaulingPointsOfInterest() const
{
   return m_HaulingPois;
}

void pgsWsdotHaulingAnalysisArtifact::GetMinMaxHaulingStresses(MaxHaulingStressCollection& rMaxStresses) const
{
   ATLASSERT(0 < m_HaulingStressAnalysisArtifacts.size());

   rMaxStresses.clear();
   rMaxStresses.reserve(m_HaulingStressAnalysisArtifacts.size());

   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iter( m_HaulingStressAnalysisArtifacts.begin() );
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iterEnd( m_HaulingStressAnalysisArtifacts.end() );
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      const pgsWsdotHaulingStressAnalysisArtifact& artifact(iter->second);

      // top fiber
      // subtract out stress due to prestressing
      Float64 stps;
      Float64 top_stress, up, no, dn;
      artifact.GetTopFiberStress(&stps, &up, &no, &dn);

      top_stress = Max(up, no, dn);

      // bottom fiber
      // subtract out stress due to prestressing
      Float64 sbps;
      artifact.GetBottomFiberStress(&sbps, &up, &no, &dn);

      Float64 bot_stress = Min(up, no, dn);

      // prestress force
      Float64 ps_force = artifact.GetEffectiveHorizPsForce();

      rMaxStresses.push_back( MaxdHaulingStresses(poi, ps_force, top_stress,bot_stress) );
   }
}


void pgsWsdotHaulingAnalysisArtifact::GetMinMaxInclinedStresses(Float64* pftuMin,Float64* pftdMin,Float64* pfbuMin,Float64* pfbdMin,
                                                           Float64* pftuMax,Float64* pftdMax,Float64* pfbuMax,Float64* pfbdMax) const
{
   ATLASSERT(0 < m_HaulingStressAnalysisArtifacts.size());

   *pftuMin = Float64_Max;
   *pftdMin = Float64_Max;
   *pfbuMin = Float64_Max;
   *pfbdMin = Float64_Max;

   *pftuMax = -Float64_Max;
   *pftdMax = -Float64_Max;
   *pfbuMax = -Float64_Max;
   *pfbdMax = -Float64_Max;

   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iter(m_HaulingStressAnalysisArtifacts.begin());
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iterEnd(m_HaulingStressAnalysisArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsWsdotHaulingStressAnalysisArtifact& artifact = iter->second;
      Float64 ftu,ftd,fbu,fbd;
      artifact.GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);

      *pftuMin = Min(*pftuMin,ftu);
      *pftdMin = Min(*pftdMin,ftd);
      *pfbuMin = Min(*pfbuMin,fbu);
      *pfbdMin = Min(*pfbdMin,fbd);

      *pftuMax = Max(*pftuMax,ftu);
      *pftdMax = Max(*pftdMax,ftd);
      *pfbuMax = Max(*pfbuMax,fbu);
      *pfbdMax = Max(*pfbdMax,fbd);
   }
}

void pgsWsdotHaulingAnalysisArtifact::GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const
{
   Float64 maxFciComp = -Float64_Max;
   Float64 maxFciTensnobar = -Float64_Max;
   Float64 maxFciTenswithbar = -Float64_Max;

   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iter( m_HaulingStressAnalysisArtifacts.begin() );
   std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator iterEnd( m_HaulingStressAnalysisArtifacts.end() );
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsWsdotHaulingStressAnalysisArtifact& artifact = iter->second;

      Float64 fciComp, fciTensNoRebar, fciTensWithRebar;
      artifact.GetRequiredConcreteStrength(&fciComp, &fciTensNoRebar, &fciTensWithRebar);

      // Use inline function for comparison
      maxFciComp        = CompareConcreteStrength(maxFciComp, fciComp);
      maxFciTensnobar   = CompareConcreteStrength(maxFciTensnobar, fciTensNoRebar);
      maxFciTenswithbar = CompareConcreteStrength(maxFciTenswithbar, fciTensWithRebar);
   }

   *pfciComp            = maxFciComp;
   *pfcTensionNoRebar   = maxFciTensnobar;
   *pfcTensionWithRebar = maxFciTenswithbar;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableTensileConcreteStressParameters(Float64 f,bool bMax,Float64 fmax)
{
   m_T = f;
   m_bfmax = bMax;
   m_fmax = fmax;
}

void pgsWsdotHaulingAnalysisArtifact::SetAllowableCompressionFactor(Float64 c)
{
   m_C = c;
}

void pgsWsdotHaulingAnalysisArtifact::SetAlternativeTensileConcreteStressFactor(Float64 f)
{
   m_Talt = f;
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
bool pgsWsdotHaulingAnalysisArtifact::BuildImpactedStressTable(const CSegmentKey& segmentKey,
                              rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Check for Hauling to Bridge Site [5.5.4.3][5.9.4.1]")<<rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   SegmentIndexType nSegments = pBridgeDesc->GetGirder(segmentKey)->GetSegmentCount();
   if ( 1 < nSegments )
   {
      rptParagraph* p = new rptParagraph(rptStyleManager::GetSubheadingStyle() );
      *pChapter << p;
      *p << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << rptNewLine;
   }

   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Hauling Stresses for Plumb Girder With Impact, and Factor of Safety Against Cracking")<<rptNewLine;

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   INIT_UV_PROTOTYPE( rptPointOfInterest,   location,      pDisplayUnits->GetSpanLengthUnit(),         false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,    force,         pDisplayUnits->GetShearUnit(),              false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   dim,           pDisplayUnits->GetComponentDimUnit(),       false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress,        pDisplayUnits->GetStressUnit(),             false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress_u,      pDisplayUnits->GetStressUnit(),             true  );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,     area,          pDisplayUnits->GetAreaUnit(),               true  );

   rptCapacityToDemand cap_demand;

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (!pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
      return false;
   }

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   Float64 c; // compression coefficient
   Float64 t; // tension coefficient
   Float64 t_max; // maximum allowable tension
   bool b_t_max; // true if max allowable tension is applicable

   c = pSpecEntry->GetHaulingCompressionStressFactor();
   t = pSpecEntry->GetHaulingTensionStressFactor();
   pSpecEntry->GetHaulingMaximumTensionStress(&b_t_max,&t_max);

   Float64 t2 = pSpecEntry->GetHaulingTensionStressFactorWithRebar();

   Float64 capCompression = pSegmentHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);

   *p <<_T("Maximum allowable concrete compressive stress = -") << c;
   *p << RPT_FC << _T(" = ") << stress.SetValue(capCompression)<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t);
   if ( bLambda )
   {
      *p << symbol(lambda);
   }
   *p << symbol(ROOT) << RPT_FC;

   if ( b_t_max )
   {
      *p << _T(" but not more than: ") << stress.SetValue(t_max);
   }
   *p << _T(" = ") << stress.SetValue(pSegmentHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(segmentKey))<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress = ") << tension_coeff.SetValue(t2);
   if ( bLambda )
   {
      *p << symbol(lambda);
   }
   *p << symbol(ROOT) << RPT_FC
      << _T(" = ") << stress.SetValue(pSegmentHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(segmentKey)) << _T(" ") << stress.GetUnitTag()
      << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

   *p <<_T("Allowable factor of safety against cracking = ")<<this->GetAllowableFsForCracking()<<rptNewLine; 

   Float64 fc_reqd_comp,fc_reqd_tens, fc_reqd_tens_wrebar;
   this->GetRequiredConcreteStrength(&fc_reqd_comp,&fc_reqd_tens,&fc_reqd_tens_wrebar);

   *p << RPT_FC << _T(" required for Compressive stress = ");
   if ( 0 < fc_reqd_comp )
   {
      *p << stress_u.SetValue( fc_reqd_comp ) << rptNewLine;
   }
   else
   {
      *p << symbol(INFINITY) << rptNewLine;
   }

   *p << RPT_FC << _T(" required for Tensile stress without sufficient reinforcement = ");
   if ( 0 < fc_reqd_tens )
   {
      *p << stress_u.SetValue( fc_reqd_tens ) << rptNewLine;
   }
   else
   {
      *p << symbol(INFINITY) << rptNewLine;
   }

   *p << RPT_FC << _T(" required for Tensile stress with sufficient reinforcement to resist the tensile force in the concrete = ");
   if ( 0 < fc_reqd_tens_wrebar )
   {
      *p << stress_u.SetValue( fc_reqd_tens_wrebar ) << rptNewLine;
   }
   else
   {
      *p << symbol(INFINITY) << rptNewLine;
   }

   GET_IFACE2(pBroker,ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pSegmentHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(11,_T(""));
   *p << p_table;

   int col1=0;
   int col2=0;
   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   (*p_table)(0,col1++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Pick Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Max") << rptNewLine << _T("Demand");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Min") << rptNewLine << _T("Demand");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Tensile") << rptNewLine << _T("Capacity");
   (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << Sub2(_T("FS"),_T("cr"));

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   (*p_table)(0,col1++) << _T("FS") << rptNewLine << _T("Status");

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
   {
      p_table->SetColumnSpan(0,i,SKIP_CELL);
   }

   Float64 overhang = this->GetTrailingOverhang();

   RowIndexType row=2;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingStressAnalysisArtifact* pStressArtifact = GetHaulingStressAnalysisArtifact(poi);
      const pgsWsdotHaulingCrackingAnalysisArtifact* pCrackArtifact = GetHaulingCrackingAnalysisArtifact(poi);

      if (pStressArtifact == NULL || pCrackArtifact == NULL)
      {
         ATLASSERT(false); // this should not happen
         continue;
      }
      (*p_table)(row,0) << location.SetValue( POI_HAUL_SEGMENT, poi );

      // Tension
      Float64 fTensTop, fTensBottom, tensCapacityTop, tensCapacityBottom;
      pStressArtifact->GetMaxPlumbTensileStress(&fTensTop, &fTensBottom, &tensCapacityTop, &tensCapacityBottom);

      // Compression
      Float64 fPs, fTopUpward, fTopNoImpact, fTopDownward;
      pStressArtifact->GetTopFiberStress(&fPs, &fTopUpward, &fTopNoImpact, &fTopDownward);

      Float64 fBotUpward, fBotNoImpact, fBotDownward;
      pStressArtifact->GetBottomFiberStress(&fPs, &fBotUpward, &fBotNoImpact, &fBotDownward);

      Float64 fTopMin = Min(fTopUpward, fTopNoImpact, fTopDownward);
      Float64 fBotMin = Min(fBotUpward, fBotNoImpact, fBotDownward);

      ColumnIndexType col = 1;
      (*p_table)(row,col++) << stress.SetValue(fTensTop);
      (*p_table)(row,col++) << stress.SetValue(fTensBottom);
      (*p_table)(row,col++) << stress.SetValue(fTopMin);
      (*p_table)(row,col++) << stress.SetValue(fBotMin);

      if (0 < fTensTop)
      {
         (*p_table)(row,col++) << stress.SetValue(tensCapacityTop);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      if (0 < fTensBottom)
      {
         (*p_table)(row,col++) << stress.SetValue(tensCapacityBottom);
      }
      else
      {
         (*p_table)(row,col++) << _T("-");
      }

      // Determine which c/d controls. top or bottom
      Float64 fTens, capTens;
      if( IsCDLess(cdPositive, tensCapacityTop, fTensTop, tensCapacityBottom, fTensBottom))
      {
         fTens = fTensTop;
         capTens = tensCapacityTop;
      }
      else
      {
         fTens = fTensBottom;
         capTens = tensCapacityBottom;
      }

      if ( pStressArtifact->TensionPassedPlumbGirder() )
      {
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,true)<<_T(")");
      }
      else
      {
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capTens,fTens,false)<<_T(")");
      }

      Float64 fComp = Min(fTopMin, fBotMin);
      
      if ( pStressArtifact->CompressionPassedPlumbGirder() )
      {
          (*p_table)(row,col++) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,true)<<_T(")");
      }
      else
      {
          (*p_table)(row,col++) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(capCompression,fComp,false)<<_T(")");
      }

      Float64 FScr = pCrackArtifact->GetFsCracking();
      if ( FScr == Float64_Inf )
      {
         (*p_table)(row,col++) << symbol(INFINITY);
      }
      else
      {
         (*p_table)(row,col++) << scalar.SetValue(FScr);
      }
      
      if (pCrackArtifact->Passed() )
      {
         (*p_table)(row,col++) << RPT_PASS;
      }
      else
      {
         (*p_table)(row,col++) << RPT_FAIL;
      }

      row++;
   }

   return true;
}

void pgsWsdotHaulingAnalysisArtifact::BuildInclinedStressTable(const CSegmentKey& segmentKey,rptChapter* pChapter,
                              IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   rptCapacityToDemand cap_demand;

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Hauling Stresses for Inclined Girder Without Impact")<<rptNewLine;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker, ISpecification, pSpec );
   GET_IFACE2(pBroker, ILibrary,       pLib );
   std::_tstring specName = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( specName.c_str() );

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   Float64 c = pSpecEntry->GetHaulingCompressionStressFactor(); // compression coefficient

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   Float64 all_comp = pSegmentHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);
   Float64 mod_rupture = this->GetModRupture();
   Float64 mod_rupture_coeff = this->GetModRuptureCoefficient();

   *p <<_T("Maximum allowable concrete compressive stress = -") << c;
   *p << RPT_FC << _T(" = ") << stress.SetValue(all_comp)<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   *p <<_T("Maximum allowable concrete tensile stress, inclined girder without impact = ") << RPT_STRESS(_T("r")) << _T(" = ") << tension_coeff.SetValue(mod_rupture_coeff);
   if ( bLambda )
   {
      *p << symbol(lambda);
   }
   *p << symbol(ROOT) << RPT_FC;
   *p << _T(" = ") << stress.SetValue(mod_rupture)<< _T(" ") << stress.GetUnitTag()<< rptNewLine;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(7,_T(""));
   *p << p_table;
   *p << RPT_STRESS(_T("tu")) << _T(" = top fiber stress, uphill side; ") 
      << RPT_STRESS(_T("td")) << _T(" = top fiber stress, downhill side") << rptNewLine;
   *p << RPT_STRESS(_T("bu")) << _T(" = bottom fiber stress, uphill side; ")
      << RPT_STRESS(_T("bd")) << _T(" = bottom fiber stress, downhill side") << rptNewLine;

   (*p_table)(0,0) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(RPT_STRESS(_T("tu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_STRESS(_T("td")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << COLHDR(RPT_STRESS(_T("bu")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,4) << COLHDR(RPT_STRESS(_T("bd")),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,5) << _T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   (*p_table)(0,6) << _T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   Float64 overhang = this->GetTrailingOverhang();

   GET_IFACE2(pBroker,ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pSegmentHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0);

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingStressAnalysisArtifact* pStressArtifact = GetHaulingStressAnalysisArtifact(poi);

      if ( pStressArtifact == NULL )
      {
         ATLASSERT(false); // this should not happen
         continue;
      }
 
      (*p_table)(row,0) << location.SetValue(POI_HAUL_SEGMENT, poi);

      Float64 ftu, ftd, fbu, fbd;
      pStressArtifact->GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);
      (*p_table)(row,1) << stress.SetValue(ftu);
      (*p_table)(row,2) << stress.SetValue(ftd);
      (*p_table)(row,3) << stress.SetValue(fbu);
      (*p_table)(row,4) << stress.SetValue(fbd);

      Float64 fTens = Max(ftu, ftd, fbu, fbd);
      Float64 fComp = Min(ftu, ftd, fbu, fbd);
      
      if ( pStressArtifact->TensionPassedInclinedGirder() )
      {
          (*p_table)(row,5) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(mod_rupture,fTens,true)<<_T(")");
      }
      else
      {
          (*p_table)(row,5) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(mod_rupture,fTens,false)<<_T(")");
      }

      if ( pStressArtifact->CompressionPassedInclinedGirder() )
      {
          (*p_table)(row,6) << RPT_PASS << rptNewLine <<_T("(")<< cap_demand.SetValue(all_comp,fComp,true)<<_T(")");
      }
      else
      {
          (*p_table)(row,6) << RPT_FAIL << rptNewLine <<_T("(")<< cap_demand.SetValue(all_comp,fComp,false)<<_T(")");
      }

      row++;
   }
}

void pgsWsdotHaulingAnalysisArtifact::BuildOtherTables(rptChapter* pChapter,
                              IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits) const
{
   // FS for failure
   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Factor of Safety Against Rollover");

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,      pDisplayUnits->GetSpanLengthUnit(), true  );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2);
   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << _T("Factor of Safety Against Rollover (FS") << Sub(_T("r")) << _T(")");
   (*p_table)(1,0) << _T("Allowable Factor of Safety Against Rollover");
   (*p_table)(2,0) << _T("Status");

   Float64 fs_fail  = this->GetFsRollover();
   Float64 all_fail = this->GetAllowableFsForRollover();
   (*p_table)(0,1) << scalar.SetValue(this->GetFsRollover());
   (*p_table)(1,1) << scalar.SetValue(this->GetAllowableFsForRollover());
   if (IsLE(all_fail,fs_fail))
   {
      (*p_table)(2,1) << RPT_PASS;
   }
   else
   {
      (*p_table)(2,1) << RPT_FAIL;
   }

   // Truck support spacing
   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Spacing Between Truck Supports for Hauling");

   p = new rptParagraph;
   *pChapter << p;

   p_table = rptStyleManager::CreateTableNoHeading(2);
   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << _T("Distance Between Supports");
   (*p_table)(1,0) << _T("Max. Allowable Distance Between Supports");
   (*p_table)(2,0) << _T("Status");

   Float64 span_length  = this->GetClearSpanBetweenSupportLocations();
   Float64 allowable_span_length = this->GetAllowableSpanBetweenSupportLocations();

   (*p_table)(0,1) << loc.SetValue(span_length);
   (*p_table)(1,1) << loc.SetValue(allowable_span_length);

   if ( IsLE(span_length,allowable_span_length) )
   {
      (*p_table)(2,1) << RPT_PASS;
   }
   else
   {
      (*p_table)(2,1) << RPT_FAIL;
   }



   // Truck support spacing
   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Girder Support Configuration");

   p = new rptParagraph;
   *pChapter << p;

   p_table = rptStyleManager::CreateTableNoHeading(2);
   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;

   (*p_table)(0,0) << _T("Leading Overhang (closest to cab of truck)");
   (*p_table)(1,0) << _T("Max. Allowable Leading Overhang");
   (*p_table)(2,0) << _T("Status");
   Float64 oh  = this->GetLeadingOverhang();
   Float64 all_oh = this->GetAllowableLeadingOverhang();
   (*p_table)(0,1) << loc.SetValue(oh);
   (*p_table)(1,1) << loc.SetValue(all_oh);
   if ( IsLE(oh,all_oh) )
   {
      (*p_table)(2,1) << RPT_PASS;
   }
   else
   {
      (*p_table)(2,1) << RPT_FAIL;
   }

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

   // Max Girder Weight
   pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Maximum Girder Weight");

   p = new rptParagraph;
   *pChapter << p;

   p_table = rptStyleManager::CreateTableNoHeading(2);
   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
   *p << p_table;
   (*p_table)(0,0) << _T("Girder Weight");
   (*p_table)(1,0) << _T("Maximum Allowable Weight");
   (*p_table)(2,0) << _T("Status");
   Float64 wgt  = this->GetGirderWeight();
   Float64 maxwgt = this->GetMaxGirderWgt();
   force.ShowUnitTag(true);
   (*p_table)(0,1) << force.SetValue(wgt);
   (*p_table)(1,1) << force.SetValue(maxwgt);
   if ( IsLE(wgt,maxwgt) )
   {
      (*p_table)(2,1) << RPT_PASS;
   }
   else
   {
      (*p_table)(2,1) << RPT_FAIL;
   }
}

void pgsWsdotHaulingAnalysisArtifact::BuildRebarTable(IBroker* pBroker,rptChapter* pChapter, const CSegmentKey& segmentKey,
                                                      ImpactDir dir) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,     pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   rptCapacityToDemand cap_demand;

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   Float64 overhang = this->GetTrailingOverhang();

   GET_IFACE2(pBroker,ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);
   std::vector<pgsPointOfInterest> vPoi = pSegmentHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0);
   ATLASSERT(0 < vPoi.size());

   std::_tstring tablename;
   if (dir==idDown)
   {
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Hauling, Downward Impact");
   }
   else if (dir==idNone)
   {
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Hauling, Without Impact");
   }
   else
   {
      tablename=_T("Rebar Requirements for Tensile Stress Limit [C5.9.4.1.2] - Hauling, Upward Impact");
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10,tablename);
   *p << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Bunk Point"),    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << _T("Tension") << rptNewLine << _T("Face");
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("na")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("f"),_T("ci"))<<rptNewLine<<_T("Demand"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("t")),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("T"),rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("* A"),_T("s"))<< rptNewLine << _T("Provided"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("s"))<< rptNewLine << _T("Required"),rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Tensile")<<rptNewLine<<_T("Capacity"),rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) <<_T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");

   Int16 row=1;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const pgsWsdotHaulingStressAnalysisArtifact* pStressArtifact = GetHaulingStressAnalysisArtifact(poi);
      if(pStressArtifact == NULL)
      {
         ATLASSERT(false);
         continue;
      }

      Float64 Yna, At, T, AsProvd, AsReqd, fAllow;
      pStressArtifact->GetAlternativeTensileStressParameters(dir, &Yna, &At, &T, &AsProvd, &AsReqd, &fAllow);

      (*pTable)(row,0) << location.SetValue( POI_HAUL_SEGMENT, poi );

      if (Yna < 0 )
      {
         // Entire section is in compression - blank out row
         (*pTable)(row,1) << _T("Neither");
         for ( ColumnIndexType ic = 2; ic < pTable->GetNumberOfColumns(); ic++ )
         {
           (*pTable)(row,ic) << _T("-");
         }
      }
      else
      {
         // Stress demand
         Float64 fps;
         Float64 fTopUp, fTopNone, fTopDown;
         Float64 fBottomUp, fBottomNone, fBottomDown;
         pStressArtifact->GetTopFiberStress(&fps, &fTopUp, &fTopNone, &fTopDown);
         pStressArtifact->GetBottomFiberStress(&fps, &fBottomUp, &fBottomNone, &fBottomDown);

         Float64 fTop, fBot;
         if(dir == idDown)
         {
            fTop = fTopDown;
            fBot = fBottomDown;
         }
         else if(dir == idNone)
         {
            fTop = fTopNone;
            fBot = fBottomNone;
         }
         else
         {
            fTop = fTopUp;
            fBot = fBottomUp;
         }

         Float64 fTens;
         if (0.0 < fTop)
         {
            fTens = fTop;
            (*pTable)(row,1) << _T("Top");
         }
         else
         {
            fTens = fBot;
            (*pTable)(row,1) << _T("Bottom");
         }

         (*pTable)(row,2) << dim.SetValue(Yna);
         (*pTable)(row,3) << stress.SetValue(fTens);
         (*pTable)(row,4) << area.SetValue(At);
         (*pTable)(row,5) << force.SetValue(T);
         (*pTable)(row,6) << area.SetValue(AsProvd);
         (*pTable)(row,7) << area.SetValue(AsReqd);
         (*pTable)(row,8) << stress.SetValue(fAllow);
         (*pTable)(row,9) <<_T("(")<< cap_demand.SetValue(fAllow,fTens,true)<<_T(")");
      }

      row++;
   }

   *p << _T("* Bars must be fully developed and lie within tension area of section before they are considered.");
}


//======================== OPERATIONS =======================================
void pgsWsdotHaulingAnalysisArtifact::MakeCopy(const pgsWsdotHaulingAnalysisArtifact& rOther)
{
   m_GirderLength                        = rOther.m_GirderLength;
   m_GirderWeightPerLength               = rOther.m_GirderWeightPerLength;
   m_ClearSpanBetweenSupportLocations    = rOther.m_ClearSpanBetweenSupportLocations;
   m_HeightOfRollCenterAboveRoadway      = rOther.m_HeightOfRollCenterAboveRoadway;
   m_HeightOfCgOfGirderAboveRollCenter   = rOther.m_HeightOfCgOfGirderAboveRollCenter;
   m_RollStiffnessOfvehicle              = rOther.m_RollStiffnessOfvehicle;
   m_AxleWidth                           = rOther.m_AxleWidth;
   m_RoadwaySuperelevation               = rOther.m_RoadwaySuperelevation;
   m_UpwardImpact                        = rOther.m_UpwardImpact;
   m_DownwardImpact                      = rOther.m_DownwardImpact;
   m_SweepTolerance                      = rOther.m_SweepTolerance;
   m_SupportPlacementTolerance           = rOther.m_SupportPlacementTolerance;
   m_Fc                                  = rOther.m_Fc;
   m_ModulusOfRupture                    = rOther.m_ModulusOfRupture;
   m_Krupture                            = rOther.m_Krupture;
   m_ElasticModulusOfGirderConcrete      = rOther.m_ElasticModulusOfGirderConcrete;
   m_EccentricityDueToSweep              = rOther.m_EccentricityDueToSweep;
   m_EccentricityDueToPlacementTolerance = rOther.m_EccentricityDueToPlacementTolerance;
   m_OffsetFactor                        = rOther.m_OffsetFactor;
   m_TotalInitialEccentricity            = rOther.m_TotalInitialEccentricity;
   m_Ix                                  = rOther.m_Ix;
   m_Iy                                  = rOther.m_Iy;
   m_Zo                                  = rOther.m_Zo;
   m_ZoPrime                             = rOther.m_ZoPrime;
   m_EqualibriumAngle                    = rOther.m_EqualibriumAngle;
   m_FsRollover                          = rOther.m_FsRollover;
   m_RadiusOfStability                   = rOther.m_RadiusOfStability;
   m_ThetaRolloverMax                    = rOther.m_ThetaRolloverMax;

   // make sure artifacts have me as a parent
   m_HaulingStressAnalysisArtifacts   = rOther.m_HaulingStressAnalysisArtifacts;
   m_HaulingCrackingAnalysisArtifacts = rOther.m_HaulingCrackingAnalysisArtifacts;

   m_HaulingPois  = rOther.m_HaulingPois;

   m_GirderWeight = rOther.m_GirderWeight;

   m_LeadingOverhang  = rOther.m_LeadingOverhang;
   m_TrailingOverhang = rOther.m_TrailingOverhang;

   m_T     = rOther.m_T;
   m_bfmax = rOther.m_bfmax;
   m_fmax  = rOther.m_fmax;;
   m_C     = rOther.m_C;
   m_Talt  = rOther.m_Talt;

   m_AllowableSpanBetweenSupportLocations = rOther.m_AllowableSpanBetweenSupportLocations;
   m_AllowableLeadingOverhang             = rOther.m_AllowableLeadingOverhang;
   m_AllowableFsForCracking               = rOther.m_AllowableFsForCracking;
   m_AllowableFsForRollover               = rOther.m_AllowableFsForRollover;
   m_MaxGirderWgt                         = rOther.m_MaxGirderWgt;
}

void pgsWsdotHaulingAnalysisArtifact::MakeAssignment(const pgsWsdotHaulingAnalysisArtifact& rOther)
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
bool pgsWsdotHaulingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsWsdotHaulingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsWsdotHaulingAnalysisArtifact") << endl;
   os <<_T(" Stress Artifacts - Plumb Girder: ")<<endl;
   os << _T("=================================") <<endl;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for (iter=m_HaulingPois.begin(); iter!=m_HaulingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( rpoi );

      os<<endl;
      Float64 fps, fup, fno, fdown;
      found->second.GetTopFiberStress(&fps, &fup, &fno, &fdown);
      os<<_T("TopStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;

      found->second.GetBottomFiberStress(&fps, &fup, &fno, &fdown);
      os<<_T("BotStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;

      Float64 min_stress = found->second.GetMaximumConcreteCompressiveStress();
      Float64 max_stress = found->second.GetMaximumConcreteTensileStress();
      os<<_T("Total Stress: Min =")<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<_T("ksi, Max=")<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<_T("ksi")<<endl;
   }

   os <<_T(" Stress Artifacts - Inclined Girder: ")<<endl;
   os << _T("=================================") <<endl;
   for (iter=m_HaulingPois.begin(); iter!=m_HaulingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<pgsPointOfInterest,pgsWsdotHaulingStressAnalysisArtifact>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( rpoi );

      os<<endl;
      Float64 min_stress = found->second.GetMaximumInclinedConcreteCompressiveStress();
      Float64 max_stress = found->second.GetMaximumInclinedConcreteTensileStress();
      os<<_T("Stress: Tensile =")<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<_T("ksi, Compressive =")<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<_T("ksi")<<endl;
   }
   os <<_T(" Dump Complete")<<endl;
   os << _T("=============") <<endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsWsdotHaulingAnalysisArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsWsdotHaulingAnalysisArtifact");


   TESTME_EPILOG("HaulingAnalysisArtifact");
}
#endif // _UNITTEST


