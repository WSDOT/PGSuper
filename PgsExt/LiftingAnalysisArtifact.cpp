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
#include <PgsExt\LiftingAnalysisArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsLiftingStressAnalysisArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingStressAnalysisArtifact::pgsLiftingStressAnalysisArtifact():
m_EffectiveHorizPsForce(0.0),
m_EccentricityPsForce(0.0),
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
m_AllowableCompression(0.0),
m_ReqdCompConcreteStrength(0.0),
m_ReqdTensConcreteStrengthNoRebar(0.0),
m_ReqdTensConcreteStrengthWithRebar(0.0),
m_WasRebarReqd(0.0)
{
   memset(m_Yna,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_At,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_T,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_AsReqd,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_AsProvd,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
   memset(m_fAllow,0,SIZE_OF_IMPACTDIR*sizeof(Float64));
}

pgsLiftingStressAnalysisArtifact::pgsLiftingStressAnalysisArtifact(const pgsLiftingStressAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsLiftingStressAnalysisArtifact::~pgsLiftingStressAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLiftingStressAnalysisArtifact& pgsLiftingStressAnalysisArtifact::operator= (const pgsLiftingStressAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

bool pgsLiftingStressAnalysisArtifact::TensionPassed() const
{
   Float64 fTop, fBottom, CapacityTop, CapacityBottom;
   GetMaxTensileStress(&fTop, &fBottom, &CapacityTop, &CapacityBottom);
   if ( IsGT(CapacityTop,fTop) )
   {
      return false;
   }
   else if ( IsGT(CapacityBottom,fBottom) )
   {
      return false;
   }
   else
   {
      return true;
   }
}

bool pgsLiftingStressAnalysisArtifact::CompressionPassed() const
{
   Float64 fTop, fBottom, Capacity;
   GetMaxCompressiveStress(&fTop, &fBottom, &Capacity);
   if ( IsLT(fTop, Capacity))
   {
      return false;
   }
   else if ( IsLT(fBottom, Capacity) )
   {
      return false;
   }
   else
   {
      return true;
   }
}

bool pgsLiftingStressAnalysisArtifact::Passed() const
{
    return TensionPassed() && CompressionPassed();
}

void pgsLiftingStressAnalysisArtifact::GetMaxCompressiveStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const
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

void pgsLiftingStressAnalysisArtifact::GetMaxTensileStress(Float64* pfTop, Float64* pfBottom, Float64* pCapacityTop, Float64* pCapacityBottom) const
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

//======================== ACCESS     =======================================
Float64 pgsLiftingStressAnalysisArtifact::GetEffectiveHorizPsForce() const
{
   return m_EffectiveHorizPsForce;
}

void pgsLiftingStressAnalysisArtifact::SetEffectiveHorizPsForce(Float64 f)
{
   m_EffectiveHorizPsForce = f;
}

Float64 pgsLiftingStressAnalysisArtifact::GetEccentricityPsForce() const
{
   return m_EccentricityPsForce;
}

void pgsLiftingStressAnalysisArtifact::SetEccentricityPsForce(Float64 f)
{
   m_EccentricityPsForce = f;
}

void pgsLiftingStressAnalysisArtifact::GetMomentImpact(Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward = m_MomentUpward;
   *pNoImpact = m_MomentNoImpact;
   *pDownward = m_MomentDownward;
}

void pgsLiftingStressAnalysisArtifact::SetMomentImpact(Float64 upward, Float64 noImpact, Float64 downward)
{
   m_MomentUpward   = upward;
   m_MomentNoImpact = noImpact;
   m_MomentDownward = downward;
}

void pgsLiftingStressAnalysisArtifact::GetTopFiberStress(Float64* pPS, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPS       = m_TopFiberStressPrestress;
    *pUpward   = m_TopFiberStressUpward;
    *pNoImpact = m_TopFiberStressNoImpact;
    *pDownward = m_TopFiberStressDownward;
}

void pgsLiftingStressAnalysisArtifact::SetTopFiberStress(Float64 PS, Float64 upward, Float64 noImpact, Float64 downward)
{
   m_TopFiberStressPrestress= PS;
   m_TopFiberStressUpward   = upward;
   m_TopFiberStressNoImpact = noImpact;
   m_TopFiberStressDownward = downward;
}

void pgsLiftingStressAnalysisArtifact::GetBottomFiberStress(Float64* pPS, Float64* pUpward, Float64* pNoImpact, Float64* pDownward) const
{
    *pPS       = m_BottomFiberStressPrestress;
    *pUpward   = m_BottomFiberStressUpward;
    *pNoImpact = m_BottomFiberStressNoImpact;
    *pDownward = m_BottomFiberStressDownward;
}

void pgsLiftingStressAnalysisArtifact::SetBottomFiberStress(Float64 PS, Float64 upward, Float64 noImpact, Float64 downward)
{
   m_BottomFiberStressPrestress = PS;
   m_BottomFiberStressUpward   = upward;
   m_BottomFiberStressNoImpact = noImpact;
   m_BottomFiberStressDownward = downward;
}

Float64 pgsLiftingStressAnalysisArtifact::GetMaximumConcreteCompressiveStress() const
{
   return Min4(m_TopFiberStressUpward,m_TopFiberStressDownward,m_BottomFiberStressUpward,m_BottomFiberStressDownward);
}

Float64 pgsLiftingStressAnalysisArtifact::GetMaximumConcreteTensileStress() const
{
   return Max4(m_TopFiberStressUpward,m_TopFiberStressDownward,m_BottomFiberStressUpward,m_BottomFiberStressDownward);
}

void pgsLiftingStressAnalysisArtifact::SetAlternativeTensileStressParameters(ImpactDir impact, Float64 Yna,   Float64 At,   Float64 T,
                                                                             Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow)
{
   m_Yna[impact] = Yna;
   m_At[impact] = At;
   m_T[impact] = T;
   m_AsReqd[impact] = AsReqd;
   m_AsProvd[impact] = AsProvd;
   m_fAllow[impact] = fAllow;
}

void pgsLiftingStressAnalysisArtifact::GetAlternativeTensileStressParameters(ImpactDir impact, Float64* Yna,   Float64* At,   Float64* T,  
                                                                             Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const
{
   *Yna   = m_Yna[impact];
   *At    = m_At[impact];
   *T     = m_T[impact];
   *AsReqd   = m_AsReqd[impact];
   *AsProvd  = m_AsProvd[impact];
   *fAllow   = m_fAllow[impact];
}

void pgsLiftingStressAnalysisArtifact::SetCompressiveCapacity(Float64 fAllowable)
{
   m_AllowableCompression = fAllowable;
}

void pgsLiftingStressAnalysisArtifact::GetCompressiveCapacity(Float64* fAllowable) const
{
   *fAllowable     = m_AllowableCompression;
}

void pgsLiftingStressAnalysisArtifact::GetTensileCapacities(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const
{
   *pUpward   = m_fAllow[idUp];
   *pNoImpact = m_fAllow[idNone];
   *pDownward = m_fAllow[idDown];
}

void pgsLiftingStressAnalysisArtifact::SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar)
{
   m_ReqdCompConcreteStrength = fciComp;
   m_ReqdTensConcreteStrengthNoRebar = fciTensNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = fciTensWithRebar;
}

void pgsLiftingStressAnalysisArtifact::GetRequiredConcreteStrength(Float64* pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const
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
void pgsLiftingStressAnalysisArtifact::MakeCopy(const pgsLiftingStressAnalysisArtifact& rOther)
{
   m_EffectiveHorizPsForce     = rOther.m_EffectiveHorizPsForce;
   m_EccentricityPsForce       = rOther.m_EccentricityPsForce;
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

   std::copy(rOther.m_Yna, rOther.m_Yna+SIZE_OF_IMPACTDIR, m_Yna);
   std::copy(rOther.m_At, rOther.m_At+SIZE_OF_IMPACTDIR, m_At);
   std::copy(rOther.m_T, rOther.m_T+SIZE_OF_IMPACTDIR, m_T);
   std::copy(rOther.m_AsReqd, rOther.m_AsReqd+SIZE_OF_IMPACTDIR, m_AsReqd);
   std::copy(rOther.m_AsProvd, rOther.m_AsProvd+SIZE_OF_IMPACTDIR, m_AsProvd);
   std::copy(rOther.m_fAllow, rOther.m_fAllow+SIZE_OF_IMPACTDIR, m_fAllow);

   m_AllowableCompression = rOther.m_AllowableCompression;
   m_ReqdCompConcreteStrength = rOther.m_ReqdCompConcreteStrength;
   m_ReqdTensConcreteStrengthNoRebar   = rOther.m_ReqdTensConcreteStrengthNoRebar;
   m_ReqdTensConcreteStrengthWithRebar = rOther.m_ReqdTensConcreteStrengthWithRebar;
}

void pgsLiftingStressAnalysisArtifact::MakeAssignment(const pgsLiftingStressAnalysisArtifact& rOther)
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
bool pgsLiftingStressAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingStressAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsLiftingStressAnalysisArtifact") << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsLiftingCrackingAnalysisArtifact
****************************************************************************/

#include <PgsExt\LiftingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingCrackingAnalysisArtifact::pgsLiftingCrackingAnalysisArtifact():
m_VerticalMoment(0.0),
m_LateralMoment(0.0),
m_ThetaCrackingMax(0.0),
m_FsCracking(0.0),
m_AllowableFsForCracking(0.0),
m_CrackedFlange(BottomFlange),
m_LateralMomentStress(0.0)
{
}

pgsLiftingCrackingAnalysisArtifact::pgsLiftingCrackingAnalysisArtifact(const pgsLiftingCrackingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsLiftingCrackingAnalysisArtifact::~pgsLiftingCrackingAnalysisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLiftingCrackingAnalysisArtifact& pgsLiftingCrackingAnalysisArtifact::operator= (const pgsLiftingCrackingAnalysisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsLiftingCrackingAnalysisArtifact::Passed() const
{
   return m_AllowableFsForCracking <= m_FsCracking;
}

//======================== ACCESS     =======================================
Float64 pgsLiftingCrackingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsLiftingCrackingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsLiftingCrackingAnalysisArtifact::GetVerticalMoment() const
{
   return m_VerticalMoment;
}

void pgsLiftingCrackingAnalysisArtifact::SetVerticalMoment(Float64 m)
{
   m_VerticalMoment = m;
}

Float64 pgsLiftingCrackingAnalysisArtifact::GetLateralMoment() const
{
   return m_LateralMoment;
}

void pgsLiftingCrackingAnalysisArtifact::SetLateralMoment(Float64 m)
{
   m_LateralMoment = m;
}

Float64 pgsLiftingCrackingAnalysisArtifact::GetThetaCrackingMax() const
{
   return m_ThetaCrackingMax;
}

void pgsLiftingCrackingAnalysisArtifact::SetThetaCrackingMax(Float64 t)
{
   m_ThetaCrackingMax = t;
}

CrackedFlange pgsLiftingCrackingAnalysisArtifact::GetCrackedFlange() const
{
   return m_CrackedFlange;
}

void pgsLiftingCrackingAnalysisArtifact::SetCrackedFlange(CrackedFlange flange)
{
   m_CrackedFlange = flange;
}

Float64 pgsLiftingCrackingAnalysisArtifact::GetLateralMomentStress() const
{
   return m_LateralMomentStress;
}

void pgsLiftingCrackingAnalysisArtifact::SetLateralMomentStress(Float64 m)
{
   m_LateralMomentStress = m;
}

Float64 pgsLiftingCrackingAnalysisArtifact::GetFsCracking() const
{
   return m_FsCracking;
//   if (m_pParent->IsGirderStable())
//      return m_FsCracking;
//   else
//      return 0.0; // no safety for unstable girders
}

void pgsLiftingCrackingAnalysisArtifact::SetFsCracking(Float64 fs)
{
   m_FsCracking = fs;
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLiftingCrackingAnalysisArtifact::MakeCopy(const pgsLiftingCrackingAnalysisArtifact& rOther)
{
   m_VerticalMoment      = rOther.m_VerticalMoment;
   m_LateralMoment       = rOther.m_LateralMoment;
   m_ThetaCrackingMax    = rOther.m_ThetaCrackingMax;
   m_FsCracking          = rOther.m_FsCracking;
   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_CrackedFlange       = rOther.m_CrackedFlange;
   m_LateralMomentStress = rOther.m_LateralMomentStress;
}

void pgsLiftingCrackingAnalysisArtifact::MakeAssignment(const pgsLiftingCrackingAnalysisArtifact& rOther)
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
bool pgsLiftingCrackingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingCrackingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsLiftingCrackingAnalysisArtifact") << endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsLiftingAnalysisArtifact
****************************************************************************/

#include <PgsExt\LiftingAnalysisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLiftingAnalysisArtifact::pgsLiftingAnalysisArtifact()
{
}

pgsLiftingAnalysisArtifact::pgsLiftingAnalysisArtifact(const pgsLiftingAnalysisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsLiftingAnalysisArtifact::~pgsLiftingAnalysisArtifact()
{
   m_LiftingStressAnalysisArtifacts.clear();
   m_LiftingCrackingAnalysisArtifacts.clear();
}

//======================== OPERATORS  =======================================
pgsLiftingAnalysisArtifact& pgsLiftingAnalysisArtifact::operator= (const pgsLiftingAnalysisArtifact& rOther)
{
   if( this != &rOther ) 
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool pgsLiftingAnalysisArtifact::Passed() const
{
   // cracking
   Float64 fs_crack = this->GetMinFsForCracking();
   Float64 all_crack = this->GetAllowableFsForCracking();
   if (fs_crack<all_crack)
      return false;

   // Failure
   if ( !PassedFailureCheck() )
      return false;

   // Stresses
   if (! PassedStressCheck() )
      return false;

   return true;
}

bool pgsLiftingAnalysisArtifact::PassedFailureCheck() const
{
   Float64 fsfail = GetFsFailure();
   Float64 alfail = GetAllowableFsForFailure();
   return alfail < fsfail;
}

bool pgsLiftingAnalysisArtifact::PassedStressCheck() const
{
   for (std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin(); 
        is!=m_LiftingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsLiftingStressAnalysisArtifact& rart = is->second;

      if (!rart.Passed())
         return false;
   }

   return true;
}

//======================== ACCESS     =======================================

Float64 pgsLiftingAnalysisArtifact::GetAllowableFsForCracking() const
{
   return m_AllowableFsForCracking;
}

void pgsLiftingAnalysisArtifact::SetAllowableFsForCracking(Float64 val)
{
   m_AllowableFsForCracking = val;
}

Float64 pgsLiftingAnalysisArtifact::GetAllowableFsForFailure() const
{
   return m_AllowableFsForFailure;
}

void pgsLiftingAnalysisArtifact::SetAllowableFsForFailure(Float64 val)
{
   m_AllowableFsForFailure = val;
}

Float64 pgsLiftingAnalysisArtifact::GetGirderLength() const
{
   return m_GirderLength;
}

void pgsLiftingAnalysisArtifact::SetGirderLength(Float64 val)
{
   m_GirderLength = val;
}

Float64 pgsLiftingAnalysisArtifact::GetXFerLength() const
{
   return m_XFerLength;
}

void pgsLiftingAnalysisArtifact::SetXFerLength(Float64 val)
{
   m_XFerLength = val;
}

Float64 pgsLiftingAnalysisArtifact::GetAvgGirderWeightPerLength() const
{
   return m_GirderWeight/m_GirderLength;
}

void pgsLiftingAnalysisArtifact::SetGirderWeight(Float64 gdrWgt)
{
   m_GirderWeight = gdrWgt;
}

Float64 pgsLiftingAnalysisArtifact::GetGirderWeight() const
{
   return m_GirderWeight;
}

Float64 pgsLiftingAnalysisArtifact::GetClearSpanBetweenPickPoints() const
{
   return m_ClearSpanBetweenPickPoints;
}

void pgsLiftingAnalysisArtifact::SetClearSpanBetweenPickPoints(Float64 val)
{
   m_ClearSpanBetweenPickPoints = val;
}

Float64 pgsLiftingAnalysisArtifact::GetLeftOverhang() const
{
   return m_LeftOverhang;
}

Float64 pgsLiftingAnalysisArtifact::GetRightOverhang() const
{
   return m_RightOverhang;
}

void pgsLiftingAnalysisArtifact::SetOverhangs(Float64 left,Float64 right)
{
   m_LeftOverhang  = left;
   m_RightOverhang = right;
}

Float64 pgsLiftingAnalysisArtifact::GetVerticalDistanceFromPickPointToGirderCg() const
{
   return m_VerticalDistanceFromPickPointToGirderCg;
}

void pgsLiftingAnalysisArtifact::SetVerticalDistanceFromPickPointToGirderCg(Float64 val)
{
   m_VerticalDistanceFromPickPointToGirderCg = val;
}

Float64 pgsLiftingAnalysisArtifact::GetUpwardImpact() const
{
   return m_UpwardImpact;
}

void pgsLiftingAnalysisArtifact::SetUpwardImpact(Float64 val)
{
   m_UpwardImpact = val;
}

Float64 pgsLiftingAnalysisArtifact::GetDownwardImpact() const
{
   return m_DownwardImpact;
}

void pgsLiftingAnalysisArtifact::SetDownwardImpact(Float64 val)
{
   m_DownwardImpact = val;
}

Float64 pgsLiftingAnalysisArtifact::GetSweepTolerance() const
{
   return m_SweepTolerance;
}

void pgsLiftingAnalysisArtifact::SetSweepTolerance(Float64 val)
{
   m_SweepTolerance = val;
}

Float64 pgsLiftingAnalysisArtifact::GetLiftingDeviceTolerance() const
{
   return m_LiftingDeviceTolerance;
}

void pgsLiftingAnalysisArtifact::SetLiftingDeviceTolerance(Float64 val)
{
   m_LiftingDeviceTolerance = val;
}

Float64 pgsLiftingAnalysisArtifact::GetConcreteStrength() const
{
   return m_Fci;
}

void pgsLiftingAnalysisArtifact::SetConcreteStrength(Float64 val)
{
   m_Fci = val;
}

Float64 pgsLiftingAnalysisArtifact::GetModRupture() const
{
   return m_ModulusOfRupture;
}

void pgsLiftingAnalysisArtifact::SetModRupture(Float64 val)
{
   m_ModulusOfRupture = val;
}

Float64 pgsLiftingAnalysisArtifact::GetModRuptureCoefficient() const
{
   return m_Krupture;
}

void pgsLiftingAnalysisArtifact::SetModRuptureCoefficient(Float64 val)
{
   m_Krupture = val;
}

Float64 pgsLiftingAnalysisArtifact::GetElasticModulusOfGirderConcrete() const
{
   return m_ElasticModulusOfGirderConcrete;
}

void pgsLiftingAnalysisArtifact::SetElasticModulusOfGirderConcrete(Float64 val)
{
   m_ElasticModulusOfGirderConcrete = val;
}

Float64 pgsLiftingAnalysisArtifact::GetAxialCompressiveForceDueToInclinationOfLiftingCables() const
{
   return m_AxialCompressiveForceDueToInclinationOfLiftingCables;
}

void pgsLiftingAnalysisArtifact::SetAxialCompressiveForceDueToInclinationOfLiftingCables(Float64 val)
{
   m_AxialCompressiveForceDueToInclinationOfLiftingCables = val;
}

Float64 pgsLiftingAnalysisArtifact::GetMomentInGirderDueToInclinationOfLiftingCables() const
{
   return m_MomentInGirderDueToInclinationOfLiftingCables;
}

void pgsLiftingAnalysisArtifact::SetMomentInGirderDueToInclinationOfLiftingCables(Float64 val)
{
   m_MomentInGirderDueToInclinationOfLiftingCables = val;
}

Float64 pgsLiftingAnalysisArtifact::GetInclinationOfLiftingCables() const
{
   return m_InclinationOfLiftingCables;
}

void pgsLiftingAnalysisArtifact::SetInclinationOfLiftingCables(Float64 val)
{
   m_InclinationOfLiftingCables = val;
}

Float64 pgsLiftingAnalysisArtifact::GetEccentricityDueToSweep() const
{
   return m_EccentricityDueToSweep;
}

void pgsLiftingAnalysisArtifact::SetEccentricityDueToSweep(Float64 val)
{
   m_EccentricityDueToSweep = val;
}

Float64 pgsLiftingAnalysisArtifact::GetEccentricityDueToPlacementTolerance() const
{
   return m_EccentricityDueToPlacementTolerance;
}

void pgsLiftingAnalysisArtifact::SetEccentricityDueToPlacementTolerance(Float64 val)
{
   m_EccentricityDueToPlacementTolerance = val;
}

Float64 pgsLiftingAnalysisArtifact::GetOffsetFactor() const
{
   return m_OffsetFactor;
}

void pgsLiftingAnalysisArtifact::SetOffsetFactor(Float64 val)
{
   m_OffsetFactor = val;
}

Float64 pgsLiftingAnalysisArtifact::GetTotalInitialEccentricity() const
{
   return m_TotalInitialEccentricity;
}

void pgsLiftingAnalysisArtifact::SetTotalInitialEccentricity(Float64 val)
{
   m_TotalInitialEccentricity = val;
}


Float64 pgsLiftingAnalysisArtifact::GetCamberDueToSelfWeight() const
{
   return m_CamberDueToSelfWeight;
}

void pgsLiftingAnalysisArtifact::SetCamberDueToSelfWeight(Float64 val)
{
   m_CamberDueToSelfWeight = val;
}

Float64 pgsLiftingAnalysisArtifact::GetCamberDueToPrestress() const
{
   return m_CamberDueToPrestress;
}

void pgsLiftingAnalysisArtifact::SetCamberDueToPrestress(Float64 val)
{
   m_CamberDueToPrestress = val;
}

Float64 pgsLiftingAnalysisArtifact::GetTotalCamberAtLifting() const
{
   return m_AdjustedTotalCamberAtLifting;
}

void pgsLiftingAnalysisArtifact::SetTotalCamberAtLifting(Float64 val)
{
   m_AdjustedTotalCamberAtLifting = val;
}

Float64 pgsLiftingAnalysisArtifact::GetAdjustedYr() const
{
   return m_AdjustedYr;
}

void pgsLiftingAnalysisArtifact::SetAdjustedYr(Float64 val)
{
   m_AdjustedYr = val;
}

Float64 pgsLiftingAnalysisArtifact::GetIx() const
{
   return m_Ix;
}

void pgsLiftingAnalysisArtifact::SetIx(Float64 ix)
{
   m_Ix = ix;
}

Float64 pgsLiftingAnalysisArtifact::GetIy() const
{
   return m_Iy;
}

void pgsLiftingAnalysisArtifact::SetIy(Float64 iy)
{
   m_Iy = iy;
}

Float64 pgsLiftingAnalysisArtifact::GetZo() const
{
   return m_Zo;
}
void pgsLiftingAnalysisArtifact::SetZo(Float64 zo)
{
   m_Zo = zo;
}

Float64 pgsLiftingAnalysisArtifact::GetZoPrime() const
{
   return m_ZoPrime;
}

void pgsLiftingAnalysisArtifact::SetZoPrime(Float64 zo)
{
   m_ZoPrime = zo;
}

Float64 pgsLiftingAnalysisArtifact::GetInitialTiltAngle() const
{
   return m_InitialTiltAngle;
}

void pgsLiftingAnalysisArtifact::SetInitialTiltAngle(Float64 val)
{
   m_InitialTiltAngle = val;
}

Float64 pgsLiftingAnalysisArtifact::GetMinFsForCracking() const
{
   // cycle through all fs's at all pois and return min
   Float64 min_fs=DBL_MAX;
   PRECONDITION(m_LiftingCrackingAnalysisArtifacts.size()>0);
   for (std::map<Float64,pgsLiftingCrackingAnalysisArtifact,Float64_less>::const_iterator i = m_LiftingCrackingAnalysisArtifacts.begin(); 
        i!=m_LiftingCrackingAnalysisArtifacts.end(); i++)
   {
      Float64 fs = i->second.GetFsCracking();
      min_fs = min(min_fs,fs);
   }
   return min_fs;
}

Float64 pgsLiftingAnalysisArtifact::GetBasicFsFailure() const
{
   return m_BasicFsFailure;
}

void pgsLiftingAnalysisArtifact::SetBasicFsFailure(Float64 val)
{
   m_BasicFsFailure = val;
}

Float64 pgsLiftingAnalysisArtifact::GetFsFailure() const
{
   return m_FsFailure;
}

void pgsLiftingAnalysisArtifact::SetFsFailure(Float64 val)
{
   m_FsFailure = val;
}

Float64 pgsLiftingAnalysisArtifact::GetThetaFailureMax() const
{
   return m_ThetaFailureMax;
}

void pgsLiftingAnalysisArtifact::SetThetaFailureMax(Float64 val)
{
   m_ThetaFailureMax = val;
}

bool pgsLiftingAnalysisArtifact::IsGirderStable() const
{
   return m_IsStable;
}

void pgsLiftingAnalysisArtifact::SetIsGirderStable(bool isStable)
{
   m_IsStable = isStable;
}

void pgsLiftingAnalysisArtifact::SetLiftingPointsOfInterest(const std::vector<pgsPointOfInterest>& rPois)
{
   m_LiftingPois = rPois;
}

std::vector<pgsPointOfInterest> pgsLiftingAnalysisArtifact::GetLiftingPointsOfInterest() const
{
   return m_LiftingPois;
}

void pgsLiftingAnalysisArtifact::AddLiftingStressAnalysisArtifact(Float64 distFromStart,
                                   const pgsLiftingStressAnalysisArtifact& artifact)
{
   m_LiftingStressAnalysisArtifacts.insert(std::make_pair(distFromStart,artifact));
}

const pgsLiftingStressAnalysisArtifact* pgsLiftingAnalysisArtifact::GetLiftingStressAnalysisArtifact(Float64 distFromStart) const
{
   std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator found;
   found = m_LiftingStressAnalysisArtifacts.find( distFromStart );
   if ( found == m_LiftingStressAnalysisArtifacts.end() )
      return 0;

   return &(*found).second;
}


void pgsLiftingAnalysisArtifact::AddLiftingCrackingAnalysisArtifact(Float64 distFromStart,
                                   const pgsLiftingCrackingAnalysisArtifact& artifact)
{
   m_LiftingCrackingAnalysisArtifacts.insert(std::make_pair(distFromStart,artifact));

}

const pgsLiftingCrackingAnalysisArtifact* pgsLiftingAnalysisArtifact::GetLiftingCrackingAnalysisArtifact(Float64 distFromStart) const
{
   std::map<Float64,pgsLiftingCrackingAnalysisArtifact,Float64_less>::const_iterator found;
   found = m_LiftingCrackingAnalysisArtifacts.find( distFromStart );
   if ( found == m_LiftingCrackingAnalysisArtifacts.end() )
      return 0;

   return &(*found).second;
}

void pgsLiftingAnalysisArtifact::GetMinMaxStresses(Float64* minStress, Float64* maxStress,Float64* minDistFromStart,Float64* maxDistFromStart) const
{
   PRECONDITION(m_LiftingStressAnalysisArtifacts.size()>0);

   Float64 min_stress = Float64_Max;
   Float64 max_stress = -Float64_Max;
   for (std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin(); 
        is!=m_LiftingStressAnalysisArtifacts.end(); is++)
   {
      Float64 stress = is->second.GetMaximumConcreteCompressiveStress();
      if (  stress < min_stress )
      {
         min_stress = stress;
         *minDistFromStart = is->first;
      }

      stress = is->second.GetMaximumConcreteTensileStress();
      if ( max_stress < stress )
      {
         max_stress = stress;
         *maxDistFromStart = is->first;
      }

   }

   *minStress = min_stress;
   *maxStress = max_stress;
}

void pgsLiftingAnalysisArtifact::GetGirderStress(
   std::vector<Float64> locs, // locations were stresses are to be retreived
   bool bMin,                // if true, minimum (compression) stresses are returned
   bool bIncludePrestress,   // if true, stresses contain the effect of prestressing
   std::vector<Float64>& fTop,// vector of resulting stresses at top of girder
   std::vector<Float64>& fBot // vector of resulting stresses at bottom of girder
) const
{
   // get the girder stress duing lift
   ATLASSERT(0 < m_LiftingStressAnalysisArtifacts.size());

   std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator iter;
   for (iter = m_LiftingStressAnalysisArtifacts.begin(); iter != m_LiftingStressAnalysisArtifacts.end(); iter++)
   {
      Float64 distFromStart = iter->first;
      const pgsLiftingStressAnalysisArtifact& liftStressArtifact = iter->second;

      std::vector<Float64>::iterator locIter;
      for ( locIter = locs.begin(); locIter != locs.end(); locIter++ )
      {
         Float64 location = *locIter;
         if ( IsEqual(location,distFromStart) )
         {
            Float64 fTopPS,fTopImpactUp,fTopNoImpact,fTopImpactDown;
            liftStressArtifact.GetTopFiberStress(&fTopPS,&fTopImpactUp,&fTopNoImpact,&fTopImpactDown);

            Float64 ft = ( bMin ? Min3(fTopImpactUp,fTopNoImpact,fTopImpactDown) : Max3(fTopImpactUp,fTopNoImpact,fTopImpactDown) );
            if ( !bIncludePrestress )
               ft -= fTopPS;

            Float64 fBottomPS,fBottomImpactUp,fBottomNoImpact,fBottomImpactDown;
            liftStressArtifact.GetBottomFiberStress(&fBottomPS,&fBottomImpactUp,&fBottomNoImpact,&fBottomImpactDown);

            Float64 fb = ( bMin ? Min3(fBottomImpactUp,fBottomNoImpact,fBottomImpactDown) : Max3(fBottomImpactUp,fBottomNoImpact,fBottomImpactDown) );
            if ( !bIncludePrestress )
               fb -= fBottomPS;


            fTop.push_back(ft);
            fBot.push_back(fb);
         }
      }
   }
}

void pgsLiftingAnalysisArtifact::GetEndZoneMinMaxRawStresses(Float64* topStress, Float64* botStress,Float64* topDistFromStart,Float64* botDistFromStart) const
{
   ATLASSERT(0 < m_LiftingStressAnalysisArtifacts.size());

   Float64 top_stress = -Float64_Max;
   Float64 bot_stress =  Float64_Max;
   Float64 top_loc, bot_loc;

   // look at lifting locations and transfer lengths
   // Largest of overhang or transfer will control. (from sensitivity study and until proven wrong)
   Float64 left_loc, right_loc;
   if (m_LeftOverhang > m_XFerLength)
   {
       left_loc = m_LeftOverhang;
   }
   else
   {
      left_loc = m_XFerLength;
   }

   if(m_RightOverhang > m_XFerLength)
   {
      right_loc = m_GirderLength - m_RightOverhang;
   }
   else
   {
      right_loc = m_GirderLength - m_XFerLength;
   }

   Int16 found=0;
   for (std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin(); 
        is!=m_LiftingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsLiftingStressAnalysisArtifact& rart = is->second;

      if ( IsEqual(distFromStart,left_loc) || IsEqual(distFromStart,right_loc))
      {
         found++;

         // top fiber
         // subtract out stress due to prestressing
         Float64 stps;
         Float64 up, no, dn;
         rart.GetTopFiberStress(&stps,&up, &no, &dn);

         Float64 optim = Max3(up,no,dn);

         optim -= stps;

         if (optim > top_stress)
         {
            top_stress = optim;
            top_loc    = distFromStart;
         }

         // bottom fiber
         // subtract out stress due to prestressing
         Float64 sbps;
         rart.GetBottomFiberStress(&sbps, &up, &no, &dn);

         optim = Min3(up,no,dn);

         optim -= sbps;

         if (optim < bot_stress)
         {
            bot_stress = optim;
            bot_loc    = distFromStart;
         }
      }
   }

   ATLASSERT(found>=2);

   *topStress = top_stress;
   *topDistFromStart = top_loc;

   *botStress = bot_stress;
   *botDistFromStart = bot_loc;
}


void pgsLiftingAnalysisArtifact::GetMidZoneMinMaxRawStresses(Float64 leftHp, Float64 rightHp, Float64* topStress, Float64* botStress,Float64* topDistFromStart,Float64* botDistFromStart) const
{
   PRECONDITION(m_LiftingStressAnalysisArtifacts.size()>0);

   Float64 top_stress = -Float64_Max;
   Float64 bot_stress =  Float64_Max;
   Float64 top_loc, bot_loc;

   for (std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin(); 
        is!=m_LiftingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsLiftingStressAnalysisArtifact& rart = is->second;

      // location must be outside of HP's
      if (distFromStart>=leftHp && distFromStart<=rightHp)
      {
         Float64 psf = rart.GetEffectiveHorizPsForce();
         Float64 ecc = rart.GetEccentricityPsForce();

         // top fiber
         // subtract out stress due to prestressing
         Float64 stps;
         Float64 up, no, dn;
         rart.GetTopFiberStress(&stps, &up, &no, &dn);

         Float64 optim = Max3(up, no, dn);

         optim -= stps;

         if (optim > top_stress)
         {
            top_stress = optim;
            top_loc    = distFromStart;
         }

         // bottom fiber
         // subtract out stress due to prestressing
         Float64 sbps;
         rart.GetBottomFiberStress(&sbps, &up, &no, &dn);

         optim = Min3(up, no, dn);

         optim -= sbps;

         if (optim < bot_stress)
         {
            bot_stress = optim;
            bot_loc    = distFromStart;
         }
      }
   }

   *topStress = top_stress;
   *topDistFromStart = top_loc;

   *botStress = bot_stress;
   *botDistFromStart = bot_loc;
}

void pgsLiftingAnalysisArtifact::GetMinMaxLiftingStresses(MaxLiftingStressCollection& rMaxStresses) const
{
   PRECONDITION(m_LiftingStressAnalysisArtifacts.size()>0);

   IndexType size = m_LiftingStressAnalysisArtifacts.size();
   IndexType psiz = m_LiftingPois.size();
   PRECONDITION(m_LiftingStressAnalysisArtifacts.size()==psiz);

   rMaxStresses.clear();
   rMaxStresses.reserve(size);

   IndexType idx=0;
   for (std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin(); 
        is!=m_LiftingStressAnalysisArtifacts.end(); is++)
   {
      Float64 distFromStart = is->first;
      const pgsLiftingStressAnalysisArtifact& rart = is->second;

      // top fiber
      Float64 stps;
      Float64 up, no, dn;
      // upward impact will always cause max top tension
      rart.GetTopFiberStress(&stps, &up, &no, &dn);

      Float64 top_stress = Max3(up,no,dn);

      // bottom fiber
      Float64 sbps;
      rart.GetBottomFiberStress(&sbps, &up, &no, &dn);

      Float64 bot_stress = Min3(up,no,dn);

      // prestress force
      Float64 psf = rart.GetEffectiveHorizPsForce();

      rMaxStresses.push_back( MaxdLiftingStresses(m_LiftingPois[idx], psf, top_stress,bot_stress) );
      idx++;
   }
}

void pgsLiftingAnalysisArtifact::GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const
{
   Float64 maxFciComp = -Float64_Max;
   Float64 maxFciTensnobar = -Float64_Max;
   Float64 maxFciTenswithbar = -Float64_Max;

   std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator is = m_LiftingStressAnalysisArtifacts.begin();
   std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator iend = m_LiftingStressAnalysisArtifacts.end();
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

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLiftingAnalysisArtifact::MakeCopy(const pgsLiftingAnalysisArtifact& rOther)
{
   m_GirderLength = rOther.m_GirderLength;
   m_XFerLength = rOther.m_XFerLength;
   m_GirderWeight = rOther.m_GirderWeight;
   m_ClearSpanBetweenPickPoints = rOther.m_ClearSpanBetweenPickPoints;
   m_VerticalDistanceFromPickPointToGirderCg = rOther.m_VerticalDistanceFromPickPointToGirderCg;
   m_UpwardImpact = rOther.m_UpwardImpact;
   m_DownwardImpact = rOther.m_DownwardImpact;
   m_SweepTolerance = rOther.m_SweepTolerance;
   m_LiftingDeviceTolerance = rOther.m_LiftingDeviceTolerance;
   m_Fci = rOther.m_Fci;
   m_ModulusOfRupture = rOther.m_ModulusOfRupture;
   m_Krupture = rOther.m_Krupture;
   m_ElasticModulusOfGirderConcrete = rOther.m_ElasticModulusOfGirderConcrete;
   m_AxialCompressiveForceDueToInclinationOfLiftingCables = rOther.m_AxialCompressiveForceDueToInclinationOfLiftingCables;
   m_MomentInGirderDueToInclinationOfLiftingCables = rOther.m_MomentInGirderDueToInclinationOfLiftingCables;
   m_InclinationOfLiftingCables = rOther.m_InclinationOfLiftingCables;
   m_EccentricityDueToSweep = rOther.m_EccentricityDueToSweep;
   m_EccentricityDueToPlacementTolerance = rOther.m_EccentricityDueToPlacementTolerance;
   m_OffsetFactor = rOther.m_OffsetFactor;
   m_TotalInitialEccentricity = rOther.m_TotalInitialEccentricity;
   m_CamberDueToSelfWeight = rOther.m_CamberDueToSelfWeight;
   m_CamberDueToPrestress = rOther.m_CamberDueToPrestress;
   m_AdjustedTotalCamberAtLifting = rOther.m_AdjustedTotalCamberAtLifting;
   m_AdjustedYr = rOther.m_AdjustedYr;
   m_Ix = rOther.m_Ix;
   m_Iy = rOther.m_Iy;
   m_Zo = rOther.m_Zo;
   m_ZoPrime = rOther.m_ZoPrime;
   m_InitialTiltAngle = rOther.m_InitialTiltAngle;
   m_BasicFsFailure = rOther.m_BasicFsFailure;
   m_FsFailure = rOther.m_FsFailure;
   m_ThetaFailureMax = rOther.m_ThetaFailureMax;
   m_IsStable = rOther.m_IsStable;
   m_LeftOverhang = rOther.m_LeftOverhang;
   m_RightOverhang = rOther.m_RightOverhang;

   m_AllowableFsForCracking = rOther.m_AllowableFsForCracking;
   m_AllowableFsForFailure  = rOther.m_AllowableFsForFailure;

   m_LiftingStressAnalysisArtifacts   = rOther.m_LiftingStressAnalysisArtifacts;
   m_LiftingCrackingAnalysisArtifacts = rOther.m_LiftingCrackingAnalysisArtifacts;
   m_LiftingPois = rOther.m_LiftingPois;
}

void pgsLiftingAnalysisArtifact::MakeAssignment(const pgsLiftingAnalysisArtifact& rOther)
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
bool pgsLiftingAnalysisArtifact::AssertValid() const
{
   return true;
}

void pgsLiftingAnalysisArtifact::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsLiftingAnalysisArtifact") << endl;
   os << _T("===================================") <<endl;

   os <<_T(" Stress Artifacts")<<endl;
   os << _T("================") <<endl;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for (iter=m_LiftingPois.begin(); iter!=m_LiftingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_LiftingStressAnalysisArtifacts.find( loc );
/*
      os<<endl;
      Float64 fps, fup, fno, fdown;
      found->second.GetTopFiberStress(&fps, &fup, &fno, &fdown);
      os<<_T("TopStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;

      found->second.GetBottomFiberStress(&fps, &fup, &fno, &fdown);
      os<<_T("BotStress fps=")<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<_T("ksi, fup=")<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<_T("ksi, fno=")<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<_T("ksi, fdown=")<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<_T("ksi")<<endl;
*/
      Float64 max_stress = found->second.GetMaximumConcreteCompressiveStress();
      Float64 min_stress = found->second.GetMaximumConcreteTensileStress();
      os<<_T("Total Stress: Min =")<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<_T("ksi, Max=")<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<_T("ksi")<<endl;
   }

   os <<_T(" Cracking Artifacts")<<endl;
   os << _T("==================") <<endl;
   for (iter=m_LiftingPois.begin(); iter!=m_LiftingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      Float64 loc = rpoi.GetDistFromStart();
      os <<_T("At ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft: ");
      std::map<Float64,pgsLiftingCrackingAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_LiftingCrackingAnalysisArtifacts.find( loc );

      CrackedFlange flange = found->second.GetCrackedFlange();
      os<<_T("Flange=")<<(flange==TopFlange?_T("TopFlange"):_T("BottomFlange"));

      Float64 stress = found->second.GetLateralMomentStress();
      Float64 fs = found->second.GetFsCracking();
      os<<_T(" Lateral Stress = ")<<::ConvertFromSysUnits(stress,unitMeasure::KSI)<<_T("ksi, FS =")<<fs<<endl;
   }
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsLiftingAnalysisArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsLiftingAnalysisArtifact");


   TESTME_EPILOG("LiftingAnalysisArtifact");
}
#endif // _UNITTEST
