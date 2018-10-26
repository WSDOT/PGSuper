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
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsHaulingStressAnalysisArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingStressAnalysisArtifact::pgsHaulingStressAnalysisArtifact():
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

pgsHaulingStressAnalysisArtifact::pgsHaulingStressAnalysisArtifact(const pgsHaulingStressAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHaulingStressAnalysisArtifact::~pgsHaulingStressAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingStressAnalysisArtifact& pgsHaulingStressAnalysisArtifact::operator= (const pgsHaulingStressAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsHaulingStressAnalysisArtifact::TensionPassed() const
{
   // First plumb girder
   Float64 fTop, fBottom, CapacityTop, CapacityBottom;
   GetMaxPlumbTensileStress(&fTop, &fBottom, &CapacityTop, &CapacityBottom);
   if ( IsGT(fTop, CapacityTop) )
   {
      return false;
   }
   else if ( IsGT(fBottom, CapacityBottom) )
   {
      return false;
   }

   // Inclined girder
   Float64 ftu,ftd,fbu,fbd;
   GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);
   Float64 fmax = Max4(ftu,ftd,fbu,fbd);
   Float64 fcap = GetInclinedTensileCapacity();
   if (  IsGT(fmax, fcap) )
   {
      return false;
   }

   return true;
}

bool pgsHaulingStressAnalysisArtifact::CompressionPassed() const
{
   Float64 comp_stress = _cpp_min( GetMaximumConcreteCompressiveStress(),
                                        GetMaximumInclinedConcreteCompressiveStress() );

   Float64 max_comp_stress = GetCompressiveCapacity();

   if (comp_stress < max_comp_stress)
      return false;

   return true;
}

bool pgsHaulingStressAnalysisArtifact::Passed() const
{
    return TensionPassed() && CompressionPassed();
}

//======================== ACCESS     =======================================
void pgsHaulingStressAnalysisArtifact::SetCompressiveCapacity(Float64 fAllowable)
{
   m_AllowableCompression = fAllowable;
}

Float64 pgsHaulingStressAnalysisArtifact::GetCompressiveCapacity() const
{
   return m_AllowableCompression;
}

void pgsHaulingStressAnalysisArtifact::SetInclinedTensileCapacity(Float64 fAllowable)
{
   m_AllowableInclinedTension = fAllowable;
}

Float64 pgsHaulingStressAnalysisArtifact::GetInclinedTensileCapacity() const
{
   return m_AllowableInclinedTension;
}

Float64 pgsHaulingStressAnalysisArtifact::GetEffectiveHorizPsForce() const
{
   return m_EffectiveHorizPsForce;
}

void pgsHaulingStressAnalysisArtifact::SetEffectiveHorizPsForce(Float64 f)
{
   m_EffectiveHorizPsForce = f;
}

Float64 pgsHaulingStressAnalysisArtifact::GetEccentricityPsForce() const
{
   return m_EccentricityPsForce;
}

void pgsHaulingStressAnalysisArtifact::SetEccentricityPsForce(Float64 f)
{
   m_EccentricityPsForce = f;
}

Float64 pgsHaulingStressAnalysisArtifact::GetLateralMoment() const
{
   return m_LateralMoment;
}

void pgsHaulingStressAnalysisArtifact::SetLateralMoment(Float64 mom)
{
   m_LateralMoment = mom;
}

void pgsHaulingStressAnalysisArtifact::GetMomentImpact(Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward = m_MomentUpward;
   *pNoImpact = m_MomentNoImpact;
   *pDownward = m_MomentDownward;
}

void pgsHaulingStressAnalysisArtifact::SetMomentImpact(Float64 upward, Float64 noImpact, Float64 downward)
{
   m_MomentUpward   = upward;
   m_MomentNoImpact = noImpact;
   m_MomentDownward = downward;
}

void pgsHaulingStressAnalysisArtifact::GetTopFiberStress(Float64* pPs, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPs       = m_TopFiberStressPrestress;
    *pUpward   = m_TopFiberStressUpward;
    *pNoImpact = m_TopFiberStressNoImpact;
    *pDownward = m_TopFiberStressDownward;
}

void pgsHaulingStressAnalysisArtifact::SetTopFiberStress(Float64 Ps, Float64 upward, Float64 noImpact, Float64 downward)
{
   m_TopFiberStressPrestress= Ps;
   m_TopFiberStressUpward   = upward;
   m_TopFiberStressNoImpact = noImpact;
   m_TopFiberStressDownward = downward;
}

void pgsHaulingStressAnalysisArtifact::GetBottomFiberStress(Float64* pPs, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPs       = m_BottomFiberStressPrestress;
    *pUpward   = m_BottomFiberStressUpward;
    *pNoImpact = m_BottomFiberStressNoImpact;
    *pDownward = m_BottomFiberStressDownward;
}

void pgsHaulingStressAnalysisArtifact::SetBottomFiberStress(Float64 Ps,Float64 upward, Float64 noImpact, Float64 downward)
{
   m_BottomFiberStressPrestress= Ps;
   m_BottomFiberStressUpward   = upward;
   m_BottomFiberStressNoImpact = noImpact;
   m_BottomFiberStressDownward = downward;
}

void pgsHaulingStressAnalysisArtifact::GetMaxPlumbCompressiveStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const
{
   // Compression is easy: Allowable cannot change
   Float64 fps, fNone;
   Float64 fTopUp, fTopDown;
   this->GetTopFiberStress(&fps, &fTopUp, &fNone, &fTopDown);
   *fTop    = max(fTopUp, fTopDown);

   Float64 fBotUp, fBotDown;
   this->GetBottomFiberStress(&fps, &fBotUp, &fNone, &fBotDown);
   *fBottom = max(fBotUp, fBotDown);
   *Capacity = m_AllowableCompression;
}

void pgsHaulingStressAnalysisArtifact::GetMaxPlumbTensileStress(Float64* pfTop, Float64* pfBottom, Float64* pCapacityTop, Float64* pCapacityBottom) const
{
   // Tensile allowable can change based on location. Most find max based on C/D
   Float64 capUp, capNone, capDown;
   GetTensileCapacities(&capUp, &capNone, &capDown);
   ATLASSERT(capUp>0.0 && capNone>0.0 && capDown>0.0);

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

void pgsHaulingStressAnalysisArtifact::GetInclinedGirderStresses(Float64* pftu,Float64* pftd,Float64* pfbu,Float64* pfbd) const
{
   *pftu = m_ftu;
   *pftd = m_ftd;
   *pfbu = m_fbu;
   *pfbd = m_fbd;
}

void pgsHaulingStressAnalysisArtifact::SetInclinedGirderStresses(Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd)
{
   m_ftu = ftu;
   m_ftd = ftd;
   m_fbu = fbu;
   m_fbd = fbd;
}

Float64 pgsHaulingStressAnalysisArtifact::GetMaximumInclinedConcreteCompressiveStress() const
{
   return Min4(m_ftu,m_ftd,m_fbu,m_fbd);
}

Float64 pgsHaulingStressAnalysisArtifact::GetMaximumInclinedConcreteTensileStress() const
{
   return Max4(m_ftu,m_ftd,m_fbu,m_fbd);
}

Float64 pgsHaulingStressAnalysisArtifact::GetMaximumConcreteCompressiveStress() const
{
   return Min4(m_TopFiberStressUpward, m_TopFiberStressDownward, m_BottomFiberStressUpward, m_BottomFiberStressDownward);
}

Float64 pgsHaulingStressAnalysisArtifact::GetMaximumConcreteTensileStress() const
{
   return Max4(m_TopFiberStressUpward, m_TopFiberStressDownward, m_BottomFiberStressUpward, m_BottomFiberStressDownward);
}

void pgsHaulingStressAnalysisArtifact::SetAlternativeTensileStressParameters(ImpactDir impactdir, Float64 Yna,   Float64 At,   Float64 T,
                                                                             Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow)
{
   m_Yna[impactdir] = Yna;
   m_At[impactdir] = At;
   m_T[impactdir] = T;
   m_AsReqd[impactdir] = AsReqd;
   m_AsProvd[impactdir] = AsProvd;
   m_fAllow[impactdir] = fAllow;
}

void pgsHaulingStressAnalysisArtifact::GetAlternativeTensileStressParameters(enum ImpactDir impactdir, Float64* Yna,   Float64* At,   Float64* T,  
                                                                             Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const
{
   *Yna   = m_Yna[impactdir];
   *At    = m_At[impactdir];
   *T     = m_T[impactdir];
   *AsReqd   = m_AsReqd[impactdir];
   *AsProvd  = m_AsProvd[impactdir];
   *fAllow   = m_fAllow[impactdir];
}

void pgsHaulingStressAnalysisArtifact::GetTensileCapacities(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward   = m_fAllow[idUp];
   *pNoImpact = m_fAllow[idNone];
   *pDownward = m_fAllow[idDown];
}

void pgsHaulingStressAnalysisArtifact::SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar)
{
   m_ReqdCompConcreteStrength = fciComp;
   m_ReqdTensConcreteStrengthNoRebar = fciTensNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = fciTensWithRebar;
}

void pgsHaulingStressAnalysisArtifact::GetRequiredConcreteStrength(Float64* pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const
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
void pgsHaulingStressAnalysisArtifact::MakeCopy(const pgsHaulingStressAnalysisArtifact& rOther)
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

void pgsHaulingStressAnalysisArtifact::MakeAssignment(const pgsHaulingStressAnalysisArtifact& rOther)
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
bool pgsHaulingStressAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingStressAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHaulingStressAnalysisArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsHaulingCrackingAnalysisArtifact
****************************************************************************/

#include <PgsExt\HaulingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingCrackingAnalysisArtifact::pgsHaulingCrackingAnalysisArtifact() :
m_VerticalMoment(0.0),
m_LateralMoment(0.0),
m_ThetaCrackingMax(0.0),
m_FsCracking(0.0),
m_AllowableFsForCracking(0.0),
m_CrackedFlange(BottomFlange),
m_LateralMomentStress(0.0)
{
}

pgsHaulingCrackingAnalysisArtifact::pgsHaulingCrackingAnalysisArtifact(const pgsHaulingCrackingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHaulingCrackingAnalysisArtifact::~pgsHaulingCrackingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingCrackingAnalysisArtifact& pgsHaulingCrackingAnalysisArtifact::operator= (const pgsHaulingCrackingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool  pgsHaulingCrackingAnalysisArtifact::Passed() const
{
   Float64 all_fs = GetAllowableFsForCracking();
   Float64 fs = GetFsCracking();

   return all_fs <= fs;
}
//======================== ACCESS     =======================================
Float64 pgsHaulingCrackingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsHaulingCrackingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsHaulingCrackingAnalysisArtifact::GetVerticalMoment() const
{
   return m_VerticalMoment;
}

void pgsHaulingCrackingAnalysisArtifact::SetVerticalMoment(Float64 m)
{
   m_VerticalMoment = m;
}

Float64 pgsHaulingCrackingAnalysisArtifact::GetLateralMoment() const
{
   return m_LateralMoment;
}

void pgsHaulingCrackingAnalysisArtifact::SetLateralMoment(Float64 m)
{
   m_LateralMoment = m;
}

Float64 pgsHaulingCrackingAnalysisArtifact::GetLateralMomentStress() const
{
   return m_LateralMomentStress;
}

void pgsHaulingCrackingAnalysisArtifact::SetLateralMomentStress(Float64 m)
{
   m_LateralMomentStress = m;
}

CrackedFlange pgsHaulingCrackingAnalysisArtifact::GetCrackedFlange() const
{
   return m_CrackedFlange;
}

void pgsHaulingCrackingAnalysisArtifact::SetCrackedFlange(CrackedFlange flange)
{
   m_CrackedFlange = flange;
}

Float64 pgsHaulingCrackingAnalysisArtifact::GetThetaCrackingMax() const
{
   return m_ThetaCrackingMax;
}

void pgsHaulingCrackingAnalysisArtifact::SetThetaCrackingMax(Float64 t)
{
   m_ThetaCrackingMax = t;
}

Float64 pgsHaulingCrackingAnalysisArtifact::GetFsCracking() const
{
   return m_FsCracking;
}

void pgsHaulingCrackingAnalysisArtifact::SetFsCracking(Float64 fs)
{
   m_FsCracking = fs;
}



//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHaulingCrackingAnalysisArtifact::MakeCopy(const pgsHaulingCrackingAnalysisArtifact& rOther)
{
   m_VerticalMoment   = rOther.m_VerticalMoment;
   m_LateralMoment    = rOther.m_LateralMoment;
   m_LateralMomentStress= rOther.m_LateralMomentStress;
   m_ThetaCrackingMax = rOther.m_ThetaCrackingMax;
   m_FsCracking       = rOther.m_FsCracking;
   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_CrackedFlange       = rOther.m_CrackedFlange;
}

void pgsHaulingCrackingAnalysisArtifact::MakeAssignment(const pgsHaulingCrackingAnalysisArtifact& rOther)
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
bool pgsHaulingCrackingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingCrackingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHaulingCrackingAnalysisArtifact" << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsHaulingAnalysisArtifact
****************************************************************************/

#include <PgsExt\HaulingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHaulingAnalysisArtifact::pgsHaulingAnalysisArtifact():
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

pgsHaulingAnalysisArtifact::pgsHaulingAnalysisArtifact(const pgsHaulingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHaulingAnalysisArtifact::~pgsHaulingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHaulingAnalysisArtifact& pgsHaulingAnalysisArtifact::operator= (const pgsHaulingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsHaulingAnalysisArtifact::Passed() const
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
   if (!PassedStressCheck())
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

bool pgsHaulingAnalysisArtifact::PassedStressCheck() const
{
   for (std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin(); 
        is!=m_HaulingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsHaulingStressAnalysisArtifact& rart = is->second;

      if (!rart.Passed())
         return false;
   }

   return true;
}
//======================== ACCESS     =======================================

Float64 pgsHaulingAnalysisArtifact::GetAllowableSpanBetweenSupportLocations() const
{
   return m_AllowableSpanBetweenSupportLocations;
}

void pgsHaulingAnalysisArtifact::SetAllowableSpanBetweenSupportLocations(Float64 val)
{
   m_AllowableSpanBetweenSupportLocations = val;
}

Float64 pgsHaulingAnalysisArtifact::GetAllowableLeadingOverhang() const
{
   return m_AllowableLeadingOverhang;
}

void pgsHaulingAnalysisArtifact::SetAllowableLeadingOverhang(Float64 val)
{
   m_AllowableLeadingOverhang = val;
}

Float64 pgsHaulingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsHaulingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsHaulingAnalysisArtifact::GetAllowableFsForRollover() const
{
   return m_AllowableFsForRollover;
}

void pgsHaulingAnalysisArtifact::SetAllowableFsForRollover(Float64 val)
{
   m_AllowableFsForRollover = val;
}

Float64 pgsHaulingAnalysisArtifact::GetMaxGirderWgt() const
{
   return m_MaxGirderWgt;
}

void pgsHaulingAnalysisArtifact::SetMaxGirderWgt(Float64 wgt)
{
   m_MaxGirderWgt = wgt;
}


Float64 pgsHaulingAnalysisArtifact::GetGirderLength() const
{
   return m_GirderLength;
}

void pgsHaulingAnalysisArtifact::SetGirderLength(Float64 val)
{
   m_GirderLength = val;
}

Float64 pgsHaulingAnalysisArtifact::GetLeadingOverhang() const
{
   return m_LeadingOverhang;
}

Float64 pgsHaulingAnalysisArtifact::GetTrailingOverhang() const
{
   return m_TrailingOverhang;
}

void pgsHaulingAnalysisArtifact::SetOverhangs(Float64 trailing,Float64 leading)
{
   m_LeadingOverhang  = leading;
   m_TrailingOverhang = trailing;
}

void pgsHaulingAnalysisArtifact::SetGirderWeight(Float64 val)
{
   m_GirderWeight = val;
}

Float64 pgsHaulingAnalysisArtifact::GetGirderWeight() const
{
   return m_GirderWeight;
}

Float64 pgsHaulingAnalysisArtifact::GetAvgGirderWeightPerLength() const
{
   return m_GirderWeight/m_GirderLength;
}

Float64 pgsHaulingAnalysisArtifact::GetClearSpanBetweenSupportLocations() const
{
   return m_ClearSpanBetweenSupportLocations;
}

void pgsHaulingAnalysisArtifact::SetClearSpanBetweenSupportLocations(Float64 val)
{
   m_ClearSpanBetweenSupportLocations = val;
}

Float64 pgsHaulingAnalysisArtifact::GetHeightOfRollCenterAboveRoadway() const
{
   return m_HeightOfRollCenterAboveRoadway;
}

void pgsHaulingAnalysisArtifact::SetHeightOfRollCenterAboveRoadway(Float64 val)
{
   m_HeightOfRollCenterAboveRoadway = val;
}

Float64 pgsHaulingAnalysisArtifact::GetHeightOfCgOfGirderAboveRollCenter() const
{
   return m_HeightOfCgOfGirderAboveRollCenter;
}

void pgsHaulingAnalysisArtifact::SetHeightOfCgOfGirderAboveRollCenter(Float64 val)
{
   m_HeightOfCgOfGirderAboveRollCenter = val;
}

Float64 pgsHaulingAnalysisArtifact::GetRollStiffnessOfvehicle() const
{
   return m_RollStiffnessOfvehicle;
}

void pgsHaulingAnalysisArtifact::SetRollStiffnessOfvehicle(Float64 val)
{
   m_RollStiffnessOfvehicle = val;
}

Float64 pgsHaulingAnalysisArtifact::GetAxleWidth() const
{
   return m_AxleWidth;
}

void pgsHaulingAnalysisArtifact::SetAxleWidth(Float64 val)
{
   m_AxleWidth = val;
}

Float64 pgsHaulingAnalysisArtifact::GetRoadwaySuperelevation() const
{
   return m_RoadwaySuperelevation;
}

void pgsHaulingAnalysisArtifact::SetRoadwaySuperelevation(Float64 val)
{
   m_RoadwaySuperelevation = val;
}

Float64 pgsHaulingAnalysisArtifact::GetUpwardImpact() const
{
   return m_UpwardImpact;
}

void pgsHaulingAnalysisArtifact::SetUpwardImpact(Float64 val)
{
   m_UpwardImpact = val;
}

Float64 pgsHaulingAnalysisArtifact::GetDownwardImpact() const
{
   return m_DownwardImpact;
}

void pgsHaulingAnalysisArtifact::SetDownwardImpact(Float64 val)
{
   m_DownwardImpact = val;
}

Float64 pgsHaulingAnalysisArtifact::GetSweepTolerance() const
{
   return m_SweepTolerance;
}

void pgsHaulingAnalysisArtifact::SetSweepTolerance(Float64 val)
{
   m_SweepTolerance = val;
}

Float64 pgsHaulingAnalysisArtifact::GetSupportPlacementTolerance() const
{
   return m_SupportPlacementTolerance;
}

void pgsHaulingAnalysisArtifact::SetSupportPlacementTolerance(Float64 val)
{
   m_SupportPlacementTolerance = val;
}

Float64 pgsHaulingAnalysisArtifact::GetConcreteStrength() const
{
   return m_Fc;
}

void pgsHaulingAnalysisArtifact::SetConcreteStrength(Float64 val)
{
   m_Fc = val;
}

Float64 pgsHaulingAnalysisArtifact::GetModRupture() const
{
   return m_ModulusOfRupture;
}

void pgsHaulingAnalysisArtifact::SetModRupture(Float64 val)
{
   m_ModulusOfRupture = val;
}

Float64 pgsHaulingAnalysisArtifact::GetModRuptureCoefficient() const
{
   return m_Krupture;
}

void pgsHaulingAnalysisArtifact::SetModRuptureCoefficient(Float64 val)
{
   m_Krupture = val;
}

Float64 pgsHaulingAnalysisArtifact::GetElasticModulusOfGirderConcrete() const
{
   return m_ElasticModulusOfGirderConcrete;
}

void pgsHaulingAnalysisArtifact::SetElasticModulusOfGirderConcrete(Float64 val)
{
   m_ElasticModulusOfGirderConcrete = val;
}

Float64 pgsHaulingAnalysisArtifact::GetEccentricityDueToSweep() const
{
   return m_EccentricityDueToSweep;
}

void pgsHaulingAnalysisArtifact::SetEccentricityDueToSweep(Float64 val)
{
   m_EccentricityDueToSweep = val;
}

Float64 pgsHaulingAnalysisArtifact::GetEccentricityDueToPlacementTolerance() const
{
   return m_EccentricityDueToPlacementTolerance;
}

void pgsHaulingAnalysisArtifact::SetEccentricityDueToPlacementTolerance(Float64 val)
{
   m_EccentricityDueToPlacementTolerance = val;
}

Float64 pgsHaulingAnalysisArtifact::GetOffsetFactor() const
{
   return m_OffsetFactor;
}

void pgsHaulingAnalysisArtifact::SetOffsetFactor(Float64 val)
{
   m_OffsetFactor = val;
}

Float64 pgsHaulingAnalysisArtifact::GetTotalInitialEccentricity() const
{
   return m_TotalInitialEccentricity;
}

void pgsHaulingAnalysisArtifact::SetTotalInitialEccentricity(Float64 val)
{
   m_TotalInitialEccentricity = val;
}

Float64 pgsHaulingAnalysisArtifact::GetIx() const
{
   return m_Ix;
}

void pgsHaulingAnalysisArtifact::SetIx(Float64 ix)
{
   m_Ix = ix;
}

Float64 pgsHaulingAnalysisArtifact::GetIy() const
{
   return m_Iy;
}

void pgsHaulingAnalysisArtifact::SetIy(Float64 iy)
{
   m_Iy = iy;
}

Float64 pgsHaulingAnalysisArtifact::GetZo() const
{
   return m_Zo;
}
void pgsHaulingAnalysisArtifact::SetZo(Float64 zo)
{
   m_Zo = zo;
}

Float64 pgsHaulingAnalysisArtifact::GetZoPrime() const
{
   return m_ZoPrime;
}

void pgsHaulingAnalysisArtifact::SetZoPrime(Float64 zo)
{
   m_ZoPrime = zo;
}

Float64 pgsHaulingAnalysisArtifact::GetEqualibriumAngle() const
{
   return m_EqualibriumAngle;
}

void pgsHaulingAnalysisArtifact::SetEqualibriumAngle(Float64 val)
{
   m_EqualibriumAngle = val;
}

Float64 pgsHaulingAnalysisArtifact::GetMinFsForCracking() const
{
   // cycle through all fs's at all pois and return min
   Float64 min_fs=DBL_MAX;
   PRECONDITION(m_HaulingCrackingAnalysisArtifacts.size()>0);
   for (std::map<Float64,pgsHaulingCrackingAnalysisArtifact,Float64_less>::const_iterator i = m_HaulingCrackingAnalysisArtifacts.begin(); 
        i!=m_HaulingCrackingAnalysisArtifacts.end(); i++)
   {
      Float64 fs = i->second.GetFsCracking();
      min_fs = min(min_fs,fs);
   }
   return min_fs;
}

Float64 pgsHaulingAnalysisArtifact::GetFsRollover() const
{
   return m_FsRollover;
}

void pgsHaulingAnalysisArtifact::SetFsRollover(Float64 val)
{
   m_FsRollover = val;
}

Float64 pgsHaulingAnalysisArtifact::GetRadiusOfStability() const
{
   return m_RadiusOfStability;
}

void pgsHaulingAnalysisArtifact::SetRadiusOfStability(Float64 val)
{
   m_RadiusOfStability = val;
}

Float64 pgsHaulingAnalysisArtifact::GetThetaRolloverMax() const
{
   return m_ThetaRolloverMax;
}

void pgsHaulingAnalysisArtifact::SetThetaRolloverMax(Float64 t)
{
   m_ThetaRolloverMax = t;
}

void pgsHaulingAnalysisArtifact::AddHaulingStressAnalysisArtifact(Float64 distFromStart,
                                   const pgsHaulingStressAnalysisArtifact& artifact)
{
   m_HaulingStressAnalysisArtifacts.insert(std::make_pair(distFromStart,artifact));
}

const pgsHaulingStressAnalysisArtifact* pgsHaulingAnalysisArtifact::GetHaulingStressAnalysisArtifact(Float64 distFromStart) const
{
   std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
   found = m_HaulingStressAnalysisArtifacts.find( distFromStart );
   if ( found == m_HaulingStressAnalysisArtifacts.end() )
      return 0;

   return &(*found).second;
}


void pgsHaulingAnalysisArtifact::AddHaulingCrackingAnalysisArtifact(Float64 distFromStart,
                                   const pgsHaulingCrackingAnalysisArtifact& artifact)
{
   m_HaulingCrackingAnalysisArtifacts.insert(std::make_pair(distFromStart,artifact));

}

const pgsHaulingCrackingAnalysisArtifact* pgsHaulingAnalysisArtifact::GetHaulingCrackingAnalysisArtifact(Float64 distFromStart) const
{
   std::map<Float64,pgsHaulingCrackingAnalysisArtifact,Float64_less>::const_iterator found;
   found = m_HaulingCrackingAnalysisArtifacts.find( distFromStart );
   if ( found == m_HaulingCrackingAnalysisArtifacts.end() )
      return 0;

   return &(*found).second;
}

void pgsHaulingAnalysisArtifact::GetMinMaxStresses(Float64* minStress, Float64* maxStress) const
{
   PRECONDITION(m_HaulingStressAnalysisArtifacts.size()>0);

   Float64 min_stress = Float64_Max;
   Float64 max_stress = -Float64_Max;
   for (std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin(); 
        is!=m_HaulingStressAnalysisArtifacts.end(); is++)
   {
      min_stress = min(min_stress, is->second.GetMaximumConcreteCompressiveStress());
      max_stress = max(max_stress, is->second.GetMaximumConcreteTensileStress());
   }

   *minStress = min_stress;
   *maxStress = max_stress;
}


void pgsHaulingAnalysisArtifact::SetHaulingPointsOfInterest(const std::vector<pgsPointOfInterest>& rPois)
{
   m_HaulingPois = rPois;
}

std::vector<pgsPointOfInterest> pgsHaulingAnalysisArtifact::GetHaulingPointsOfInterest() const
{
   return m_HaulingPois;
}

void pgsHaulingAnalysisArtifact::GetMinMaxHaulingStresses(MaxHaulingStressCollection& rMaxStresses) const
{
   PRECONDITION(m_HaulingStressAnalysisArtifacts.size()>0);

   IndexType size = m_HaulingStressAnalysisArtifacts.size();
   IndexType psiz = m_HaulingPois.size();
   PRECONDITION(m_HaulingStressAnalysisArtifacts.size()==psiz);

   rMaxStresses.clear();
   rMaxStresses.reserve(size);

   IndexType idx=0;
   for (std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin(); 
        is!=m_HaulingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsHaulingStressAnalysisArtifact& rart = is->second;

      // top fiber
      // subtract out stress due to prestressing
      Float64 stps;
      Float64 top_stress, up, no, dn;
      rart.GetTopFiberStress(&stps, &up, &no, &dn);

      top_stress = Max3(up, no, dn);

      // bottom fiber
      // subtract out stress due to prestressing
      Float64 sbps;
      rart.GetBottomFiberStress(&sbps, &up, &no, &dn);

      Float64 bot_stress = Min3(up, no, dn);

      // prestress force
      Float64 ps_force = rart.GetEffectiveHorizPsForce();

      rMaxStresses.push_back( MaxdHaulingStresses(m_HaulingPois[idx], ps_force, top_stress,bot_stress) );
      idx++;
   }
}


void pgsHaulingAnalysisArtifact::GetMinMaxInclinedStresses(Float64* pftuMin,Float64* pftdMin,Float64* pfbuMin,Float64* pfbdMin,
                                                           Float64* pftuMax,Float64* pftdMax,Float64* pfbuMax,Float64* pfbdMax) const
{
   PRECONDITION(m_HaulingStressAnalysisArtifacts.size()>0);

   *pftuMin = Float64_Max;
   *pftdMin = Float64_Max;
   *pfbuMin = Float64_Max;
   *pfbdMin = Float64_Max;

   *pftuMax = -Float64_Max;
   *pftdMax = -Float64_Max;
   *pfbuMax = -Float64_Max;
   *pfbdMax = -Float64_Max;

   for (std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin(); 
        is!=m_HaulingStressAnalysisArtifacts.end(); is++)
   {
           Float64 ftu,ftd,fbu,fbd;
           is->second.GetInclinedGirderStresses(&ftu,&ftd,&fbu,&fbd);

           *pftuMin = min(*pftuMin,ftu);
           *pftdMin = min(*pftdMin,ftd);
           *pfbuMin = min(*pfbuMin,fbu);
           *pfbdMin = min(*pfbdMin,fbd);

           *pftuMax = max(*pftuMax,ftu);
           *pftdMax = max(*pftdMax,ftd);
           *pfbuMax = max(*pfbuMax,fbu);
           *pfbdMax = max(*pfbdMax,fbd);
   }
}

void pgsHaulingAnalysisArtifact::GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const
{
   Float64 maxFciComp = -Float64_Max;
   Float64 maxFciTensnobar = -Float64_Max;
   Float64 maxFciTenswithbar = -Float64_Max;

   std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator is = m_HaulingStressAnalysisArtifacts.begin();
   std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator iend = m_HaulingStressAnalysisArtifacts.end();
   for ( ; is!=iend; is++)
   {
      Float64 fciComp, fciTensNoRebar, fciTensWithRebar;
      is->second.GetRequiredConcreteStrength(&fciComp, &fciTensNoRebar, &fciTensWithRebar);

      // Use inline function for comparison
      maxFciComp        = CompareConcreteStrength(maxFciComp, fciComp);
      maxFciTensnobar   = CompareConcreteStrength(maxFciTensnobar, fciTensNoRebar);
      maxFciTenswithbar = CompareConcreteStrength(maxFciTenswithbar, fciTensWithRebar);
   }

   *pfciComp            = maxFciComp;
   *pfcTensionNoRebar   = maxFciTensnobar;
   *pfcTensionWithRebar = maxFciTenswithbar;
}

void pgsHaulingAnalysisArtifact::SetAllowableTensileConcreteStressParameters(Float64 f,bool bMax,Float64 fmax)
{
   m_T = f;
   m_bfmax = bMax;
   m_fmax = fmax;
}

void pgsHaulingAnalysisArtifact::SetAllowableCompressionFactor(Float64 c)
{
   m_C = c;
}

void pgsHaulingAnalysisArtifact::SetAlternativeTensileConcreteStressFactor(Float64 f)
{
   m_Talt = f;
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHaulingAnalysisArtifact::MakeCopy(const pgsHaulingAnalysisArtifact& rOther)
{
   m_GirderLength = rOther.m_GirderLength;
   m_GirderWeightPerLength = rOther.m_GirderWeightPerLength;
   m_ClearSpanBetweenSupportLocations = rOther.m_ClearSpanBetweenSupportLocations;
   m_HeightOfRollCenterAboveRoadway = rOther.m_HeightOfRollCenterAboveRoadway;
   m_HeightOfCgOfGirderAboveRollCenter = rOther.m_HeightOfCgOfGirderAboveRollCenter;
   m_RollStiffnessOfvehicle = rOther.m_RollStiffnessOfvehicle;
   m_AxleWidth = rOther.m_AxleWidth;
   m_RoadwaySuperelevation = rOther.m_RoadwaySuperelevation;
   m_UpwardImpact = rOther.m_UpwardImpact;
   m_DownwardImpact = rOther.m_DownwardImpact;
   m_SweepTolerance = rOther.m_SweepTolerance;
   m_SupportPlacementTolerance = rOther.m_SupportPlacementTolerance;
   m_Fc = rOther.m_Fc;
   m_ModulusOfRupture = rOther.m_ModulusOfRupture;
   m_Krupture = rOther.m_Krupture;
   m_ElasticModulusOfGirderConcrete = rOther.m_ElasticModulusOfGirderConcrete;
   m_EccentricityDueToSweep = rOther.m_EccentricityDueToSweep;
   m_EccentricityDueToPlacementTolerance = rOther.m_EccentricityDueToPlacementTolerance;
   m_OffsetFactor = rOther.m_OffsetFactor;
   m_TotalInitialEccentricity = rOther.m_TotalInitialEccentricity;
   m_Ix = rOther.m_Ix;
   m_Iy = rOther.m_Iy;
   m_Zo = rOther.m_Zo;
   m_ZoPrime = rOther.m_ZoPrime;
   m_EqualibriumAngle = rOther.m_EqualibriumAngle;
   m_FsRollover = rOther.m_FsRollover;
   m_RadiusOfStability = rOther.m_RadiusOfStability;
   m_ThetaRolloverMax = rOther.m_ThetaRolloverMax;

   // make sure artifacts have me as a parent
   m_HaulingStressAnalysisArtifacts = rOther.m_HaulingStressAnalysisArtifacts;
   m_HaulingCrackingAnalysisArtifacts = rOther.m_HaulingCrackingAnalysisArtifacts;

   m_HaulingPois = rOther.m_HaulingPois;

   m_GirderWeight = rOther.m_GirderWeight;

   m_LeadingOverhang  = rOther.m_LeadingOverhang;
   m_TrailingOverhang = rOther.m_TrailingOverhang;

   m_T     = rOther.m_T;
   m_bfmax = rOther.m_bfmax;
   m_fmax  = rOther.m_fmax;;
   m_C     = rOther.m_C;
   m_Talt  = rOther.m_Talt;

   m_AllowableSpanBetweenSupportLocations = rOther.m_AllowableSpanBetweenSupportLocations;
   m_AllowableLeadingOverhang = rOther.m_AllowableLeadingOverhang;
   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_AllowableFsForRollover = rOther.m_AllowableFsForRollover;
   m_MaxGirderWgt           = rOther.m_MaxGirderWgt;
}

void pgsHaulingAnalysisArtifact::MakeAssignment(const pgsHaulingAnalysisArtifact& rOther)
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
bool pgsHaulingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsHaulingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsHaulingAnalysisArtifact") << endl;
   os <<_T(" Stress Artifacts - Plumb Girder: ")<<endl;
   os << _T("=================================") <<endl;
   std::vector<pgsPointOfInterest>::const_iterator iter(m_HaulingPois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(m_HaulingPois.end());
   for ( ; iter != end; iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( loc );

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
   iter = m_HaulingPois.begin();
   end  = m_HaulingPois.end();
   for ( ; iter != end; iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( loc );

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
bool pgsHaulingAnalysisArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsHaulingAnalysisArtifact");


   TESTME_EPILOG("HaulingAnalysisArtifact");
}
#endif // _UNITTEST
