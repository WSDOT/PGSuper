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
#include <PgsExt\HaulingAnalysisArtifact.h>

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
pgsHaulingStressAnalysisArtifact::pgsHaulingStressAnalysisArtifact()
{
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
//======================== ACCESS     =======================================
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

void pgsHaulingStressAnalysisArtifact::GetIncludedGirderStresses(Float64* pftu,Float64* pftd,Float64* pfbu,Float64* pfbd) const
{
   *pftu = m_ftu;
   *pftd = m_ftd;
   *pfbu = m_fbu;
   *pfbd = m_fbd;
}

void pgsHaulingStressAnalysisArtifact::SetIncludedGirderStresses(Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd)
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

void pgsHaulingStressAnalysisArtifact::SetAlternativeTensileStressParameters(Float64 YnaUp,Float64 YnaDown,Float64 YnaInclined,ImpactDir dir, GirderOrientation orientation,Float64 At, Float64 T,Float64 As)
{
   m_Yna[0] = YnaUp;
   m_Yna[1] = YnaDown;
   m_Yna[2] = YnaInclined;
   m_ImpactDir = dir;
   m_GirderOrientation = orientation,
   m_At = At;
   m_T = T;
   m_As = As;
}

void pgsHaulingStressAnalysisArtifact::GetAlternativeTensileStressParameters(Float64* YnaUp,Float64* YnaDown,Float64* YnaInclined,ImpactDir *dir, GirderOrientation* orientation,Float64* At, Float64* T,Float64* As) const
{
   *YnaUp       = m_Yna[0];
   *YnaDown     = m_Yna[1];
   *YnaInclined = m_Yna[2];
   *dir = m_ImpactDir;
   *orientation = m_GirderOrientation,
   *At = m_At;
   *T = m_T;
   *As = m_As;
}

Float64 pgsHaulingStressAnalysisArtifact::GetNeutralAxis(ImpactDir dir)
{
   Float64 Yna;
   switch ( dir )
   {
   case Up:
      Yna = m_Yna[0];
      break;

   case Down:
      Yna = m_Yna[1];
      break;

   case None:
      Yna = m_Yna[2];
      break;

   }

   return Yna;
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


   m_Yna[0] = rOther.m_Yna[0];
   m_Yna[1] = rOther.m_Yna[1];
   m_Yna[2] = rOther.m_Yna[2];
   m_ImpactDir = rOther.m_ImpactDir;
   m_GirderOrientation = rOther.m_GirderOrientation;
   m_At = rOther.m_At;
   m_T = rOther.m_T;
   m_As = rOther.m_As;
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
//======================== ACCESS     =======================================

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
pgsHaulingAnalysisArtifact::pgsHaulingAnalysisArtifact()
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
//======================== ACCESS     =======================================


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

   Int32 size = m_HaulingStressAnalysisArtifacts.size();
   Int32 psiz = m_HaulingPois.size();
   PRECONDITION(m_HaulingStressAnalysisArtifacts.size()==psiz);

   rMaxStresses.clear();
   rMaxStresses.reserve(size);

   Int16 idx=0;
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
           double ftu,ftd,fbu,fbd;
           is->second.GetIncludedGirderStresses(&ftu,&ftd,&fbu,&fbd);

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

void pgsHaulingAnalysisArtifact::SetAlterantiveTensileStressAsMax(Float64 AsMax)
{
    m_AsMax = AsMax;
}

Float64 pgsHaulingAnalysisArtifact::GetAlterantiveTensileStressAsMax() const
{
    return m_AsMax;
}

void pgsHaulingAnalysisArtifact::GetRequiredConcreteStrength(double *pfcComp,double *pfcTens,bool* pMinRebarReqd,double fcMax,bool bDesign) const
{
   // required concrete strength based on plumb girder
   // compression limited by xf'ci, tension limited by xSqrt(f'ci) <= max
   Float64 min_stress, max_stress;
   GetMinMaxStresses(&min_stress, &max_stress);

   double fc_compression = -1;
   if ( min_stress < 0 )
   {
      fc_compression = min_stress/m_C;
   }
   else
   {
      fc_compression = 0; // compression isn't an issue
   }

   *pMinRebarReqd = false;
   double fc_tension = -1;
   if ( bDesign )
   {
      if ( 0 < max_stress )
         fc_tension = pow(max_stress/m_Talt,2);
      else
         fc_tension = 0; // tension isn't an issue
   }
   else
   {
      if ( 0 < max_stress )
      {
         fc_tension = pow(max_stress/m_T,2);
         if ( (m_bfmax &&                  // allowable stress is limited -AND-
              (0 < fc_tension) &&              // there is a concrete strength that might work -AND-
              (pow(m_fmax/m_T,2) < fc_tension) )   // that strength will exceed the max limit on allowable
              || // OR
              (fcMax < fc_tension) // required strength w/o rebar is greater than max
            )
         {
            // then that concrete strength won't really work afterall

            // use the alternative limit with required rebar
            fc_tension = pow(max_stress/m_Talt,2);
            *pMinRebarReqd = true;
         }
      }
      else
      {
         fc_tension = 0;
      }
   }

   // required concrete strength based on inclined girder
   // compression limited by xf'ci, tension limited by modulus of rupture
   double ftuMin,ftdMin,fbuMin,fbdMin;
   double ftuMax,ftdMax,fbuMax,fbdMax;

   GetMinMaxInclinedStresses(&ftuMin,&ftdMin,&fbuMin,&fbdMin,&ftuMax,&ftdMax,&fbuMax,&fbdMax);

   double fmin = Min4(ftuMin,ftdMin,fbuMin,fbdMin);
   double fmax = Max4(ftuMax,ftdMax,fbuMax,fbdMax);

   double fc_compression_inclined = -1;
   if ( fmin < 0 )
   {
      fc_compression_inclined = fmin/m_C;
   }
   else
   {
      fc_compression_inclined = 0;
   }

   double fc_tension_inclined = -1;
   if ( 0 < fmax )
   {
      fc_tension_inclined = pow(fmax/GetModRuptureCoefficient(),2);
   }
   else
   {
      fc_tension_inclined = 0;
   }

   // get max f'c for plumb and inclined conditions
   double fc_comp = max(fc_compression_inclined, fc_compression);
   double fc_tens = max(fc_tension_inclined, fc_tension);

   *pfcComp = fc_comp;
   *pfcTens = fc_tens;
}

void pgsHaulingAnalysisArtifact::SetAllowableTensileConcreteStressParameters(double f,bool bMax,double fmax)
{
   m_T = f;
   m_bfmax = bMax;
   m_fmax = fmax;
}

void pgsHaulingAnalysisArtifact::SetAllowableCompressionFactor(double c)
{
   m_C = c;
}

void pgsHaulingAnalysisArtifact::SetAlternativeTensileConcreteStressFactor(double f)
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

   m_AsMax = rOther.m_AsMax;

   m_T     = rOther.m_T;
   m_bfmax = rOther.m_bfmax;
   m_fmax  = rOther.m_fmax;;
   m_C     = rOther.m_C;
   m_Talt  = rOther.m_Talt;
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
   os << "Dump for pgsHaulingAnalysisArtifact" << endl;
   os <<" Stress Artifacts - Plumb Girder: "<<endl;
   os << "=================================" <<endl;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for (iter=m_HaulingPois.begin(); iter!=m_HaulingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      double loc = rpoi.GetDistFromStart();
      os <<"At " << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << " ft: ";
      std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( loc );

      os<<endl;
      double fps, fup, fno, fdown;
      found->second.GetTopFiberStress(&fps, &fup, &fno, &fdown);
      os<<"TopStress fps="<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<"ksi, fup="<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<"ksi, fno="<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<"ksi, fdown="<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<"ksi"<<endl;

      found->second.GetBottomFiberStress(&fps, &fup, &fno, &fdown);
      os<<"BotStress fps="<<::ConvertFromSysUnits(fps,unitMeasure::KSI)<<"ksi, fup="<<::ConvertFromSysUnits(fup,unitMeasure::KSI)<<"ksi, fno="<<::ConvertFromSysUnits(fno,unitMeasure::KSI)<<"ksi, fdown="<<::ConvertFromSysUnits(fdown,unitMeasure::KSI)<<"ksi"<<endl;

      Float64 min_stress = found->second.GetMaximumConcreteCompressiveStress();
      Float64 max_stress = found->second.GetMaximumConcreteTensileStress();
      os<<"Total Stress: Min ="<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<"ksi, Max="<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<"ksi"<<endl;
   }

   os <<" Stress Artifacts - Inclined Girder: "<<endl;
   os << "=================================" <<endl;
   for (iter=m_HaulingPois.begin(); iter!=m_HaulingPois.end(); iter++)
   {
      const pgsPointOfInterest& rpoi = *iter;
      double loc = rpoi.GetDistFromStart();
      os <<"At " << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << " ft: ";
      std::map<Float64,pgsHaulingStressAnalysisArtifact,Float64_less>::const_iterator found;
      found = m_HaulingStressAnalysisArtifacts.find( loc );

      os<<endl;
      Float64 min_stress = found->second.GetMaximumInclinedConcreteCompressiveStress();
      Float64 max_stress = found->second.GetMaximumInclinedConcreteTensileStress();
      os<<"Stress: Tensile ="<<::ConvertFromSysUnits(min_stress,unitMeasure::KSI)<<"ksi, Compressive ="<<::ConvertFromSysUnits(max_stress,unitMeasure::KSI)<<"ksi"<<endl;
   }
   os <<" Dump Complete"<<endl;
   os << "=============" <<endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsHaulingAnalysisArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsHaulingAnalysisArtifact");


   TESTME_EPILOG("HaulingAnalysisArtifact");
}
#endif // _UNITTEST
