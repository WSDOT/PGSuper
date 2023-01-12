///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <PGSExt\StirrupCheckAtPoisArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsLongReinfShearArtifact
****************************************************************************/


Float64 pgsLongReinfShearArtifact::GetFy() const
{
   return m_Fy;
}

void pgsLongReinfShearArtifact::SetFy(Float64 fy)
{
   m_Fy = fy;
}

Float64 pgsLongReinfShearArtifact::GetEs() const
{
   return m_Es;
}

void pgsLongReinfShearArtifact::SetEs(Float64 Es)
{
   m_Es = Es;
}

Float64 pgsLongReinfShearArtifact::GetAs() const
{
   return m_As;
}

void pgsLongReinfShearArtifact::SetAs(Float64 as)
{
   m_As = as;
}

Float64 pgsLongReinfShearArtifact::GetAps() const
{
   return m_Aps;
}

void pgsLongReinfShearArtifact::SetAps(Float64 aps)
{
   m_Aps = aps;
}

Float64 pgsLongReinfShearArtifact::GetFps() const
{
   return m_Fps;
}

void pgsLongReinfShearArtifact::SetFps(Float64 fps)
{
   m_Fps = fps;
}

Float64 pgsLongReinfShearArtifact::GetAptSegment() const
{
   return m_AptSegment;
}

void pgsLongReinfShearArtifact::SetAptSegment(Float64 apt)
{
   m_AptSegment = apt;
}

Float64 pgsLongReinfShearArtifact::GetFptSegment() const
{
   return m_FptSegment;
}

void pgsLongReinfShearArtifact::SetFptSegment(Float64 fpt)
{
   m_FptSegment = fpt;
}

Float64 pgsLongReinfShearArtifact::GetAptGirder() const
{
   return m_AptGirder;
}

void pgsLongReinfShearArtifact::SetAptGirder(Float64 apt)
{
   m_AptGirder = apt;
}

Float64 pgsLongReinfShearArtifact::GetFptGirder() const
{
   return m_FptGirder;
}

void pgsLongReinfShearArtifact::SetFptGirder(Float64 fpt)
{
   m_FptGirder = fpt;
}

Float64 pgsLongReinfShearArtifact::GetMu() const
{
   return m_Mu;
}

void pgsLongReinfShearArtifact::SetMu(Float64 mu)
{
   m_Mu = mu;
}

Float64 pgsLongReinfShearArtifact::GetMr() const
{
   return m_Mr;
}

void pgsLongReinfShearArtifact::SetMr(Float64 mr)
{
   m_Mr = mr;
}

Float64 pgsLongReinfShearArtifact::GetDv() const
{
   return m_Dv;
}

void pgsLongReinfShearArtifact::SetDv(Float64 dv)
{
   m_Dv = dv;
}

Float64 pgsLongReinfShearArtifact::GetFlexuralPhi() const
{
   return m_FlexuralPhi;
}
void pgsLongReinfShearArtifact::SetFlexuralPhi(Float64 phi)
{
   m_FlexuralPhi = phi;
}

Float64 pgsLongReinfShearArtifact::GetNu() const
{
   return m_Nu;
}
void pgsLongReinfShearArtifact::SetNu(Float64 nu)
{
   m_Nu = nu;
}

Float64 pgsLongReinfShearArtifact::GetAxialPhi() const
{
   return m_AxialPhi;
}
void pgsLongReinfShearArtifact::SetAxialPhi(Float64 phi)
{
   m_AxialPhi = phi;
}

Float64 pgsLongReinfShearArtifact::GetVu() const
{
   return m_Vu;
}

void pgsLongReinfShearArtifact::SetVu(Float64 vu)
{
   m_Vu = vu;
}

Float64 pgsLongReinfShearArtifact::GetShearPhi() const
{
   return m_ShearPhi;
}

void pgsLongReinfShearArtifact::SetShearPhi(Float64 phi)
{
   m_ShearPhi = phi;
}

Float64 pgsLongReinfShearArtifact::GetVs() const
{
   return m_Vs;
}

void pgsLongReinfShearArtifact::SetVs(Float64 vs)
{
   m_Vs = vs;
}

Float64 pgsLongReinfShearArtifact::GetVp() const
{
   return m_Vp;
}

void pgsLongReinfShearArtifact::SetVp(Float64 vp)
{
   m_Vp = vp;
}

Float64 pgsLongReinfShearArtifact::GetTheta() const
{
   return m_Theta;
}

void pgsLongReinfShearArtifact::SetTheta(Float64 theta)
{
   m_Theta = theta;
}

Uint16 pgsLongReinfShearArtifact::GetEquation() const
{
    return m_Equation;
}

void pgsLongReinfShearArtifact::SetEquation(Uint16 eqn)
{
    m_Equation = eqn;
}

void pgsLongReinfShearArtifact::SetDemandForce(Float64 demand)
{
   m_DemandForce = demand;
}

Float64 pgsLongReinfShearArtifact::GetDemandForce() const
{
   return m_DemandForce;
}

Float64 pgsLongReinfShearArtifact::GetCapacityForce() const
{
   Float64 Pr = GetRebarForce();
   Float64 Pps = GetPretensionForce();
   Float64 Pc = GetConcreteForce(); // FHWA UHPC has a contribution due to the tensile strength of UHPC
   return Pr + Pps + Pc;
}

Float64 pgsLongReinfShearArtifact::GetConcreteForce() const
{
   if (m_bFHWAUHPC)
   {
      return m_Act * m_ft;
   }
   else
   {
      return 0.0;
   }
}

Float64 pgsLongReinfShearArtifact::GetPretensionForce() const
{
   return m_Aps*m_Fps + m_AptSegment*m_FptSegment + m_AptGirder*m_FptGirder;
}

Float64 pgsLongReinfShearArtifact::GetRebarForce() const
{
   // need to return As*Es*et,loc <= As*Fy for FHWA UHPC
   if (m_bFHWAUHPC)
      return Min(m_As * m_Fy, m_As * m_Es * m_etloc); // FHWA GS 1.7.3.5 (Es*et,loc <= fy, As*Es*et,loc <= As*fy)
   else
      return m_As*m_Fy;
}


bool pgsLongReinfShearArtifact::PassedPretensionForce() const
{
   if (m_bPretensionForceLimit)
   {
      // new requirement in 9th Edition
      Float64 AsFy = GetRebarForce();
      Float64 ApsFps = GetPretensionForce();
      return AsFy < ApsFps; // ApsFps must exceed AsFy
   }
   else
   {
      return true;
   }
}

bool pgsLongReinfShearArtifact::PassedCapacity() const
{
   if (!m_bIsApplicable)
   {
      return true;
   }

   if (m_Equation == 1 && fabs(m_Mu) <= fabs(m_Mr) && !IsZero(m_Mr))
   {
      // Except as may be required by Article 5.7.3.6.3, where the reaction force or the load at the maximum
      // moment location introduces direct compression into the flexural compression face of the member, the area of
      // longitudinal reinforcement on the flexural tension side of the member need not exceed the area required to
      // resist the maximum moment acting alone.
      //
      // Mr > Mu, therefore, check not required
      return true;
   }

   return m_DemandForce <= GetCapacityForce();
}

bool pgsLongReinfShearArtifact::Passed() const
{
   return PassedCapacity() && PassedPretensionForce();
}

bool pgsLongReinfShearArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

void pgsLongReinfShearArtifact::SetApplicability(bool isApplicable)
{
   m_bIsApplicable = isApplicable;
}

void pgsLongReinfShearArtifact::IsFHWAUHPC(bool bIsUHPC)
{
   m_bFHWAUHPC = bIsUHPC;
}

bool pgsLongReinfShearArtifact::IsFHWAUHPC() const
{
   return m_bFHWAUHPC;
}

void pgsLongReinfShearArtifact::SetCrackLocalizationStrain(Float64 etloc)
{
   m_etloc = etloc;
}

Float64 pgsLongReinfShearArtifact::GetCrackLocalizationStrain() const
{
   return m_etloc;
}

void pgsLongReinfShearArtifact::SetDesignEffectiveConcreteStrength(Float64 ft)
{
   m_ft = ft;
}

Float64 pgsLongReinfShearArtifact::GetDesignEffectiveConcreteStrength() const
{
   return m_ft;
}

void pgsLongReinfShearArtifact::SetAct(Float64 Act)
{
   m_Act = Act;
}

Float64 pgsLongReinfShearArtifact::GetAct() const
{
   return m_Act;
}

bool pgsLongReinfShearArtifact::PretensionForceMustExceedBarForce() const
{
   return m_bPretensionForceLimit;
}

void pgsLongReinfShearArtifact::PretensionForceMustExceedBarForce(bool bExceed)
{
   m_bPretensionForceLimit = bExceed;
}


/****************************************************************************
CLASS
   pgsVerticalShearArtifact
****************************************************************************/

#include <PGSExt\StirrupCheckAtPoisArtifact.h>

bool pgsVerticalShearArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

void pgsVerticalShearArtifact::IsApplicable(bool bApplicable)
{
   m_bIsApplicable = bApplicable;
}

bool pgsVerticalShearArtifact::IsStrutAndTieRequired() const
{
   return m_bIsStrutAndTieRequired;
}

void pgsVerticalShearArtifact::IsStrutAndTieRequired(bool bRequired)
{
   m_bIsStrutAndTieRequired = bRequired;
}

void pgsVerticalShearArtifact::SetAreStirrupsReqd(bool reqd)
{
   m_AreStirrupsReqd = reqd;
}

bool pgsVerticalShearArtifact::GetAreStirrupsReqd() const
{
   return m_AreStirrupsReqd;
}

void pgsVerticalShearArtifact::SetAreStirrupsProvided(bool provd)
{
   m_AreStirrupsProvided = provd;
}

bool pgsVerticalShearArtifact::GetAreStirrupsProvided() const
{
   return m_AreStirrupsProvided;
}

void pgsVerticalShearArtifact::SetAvOverSReqd(Float64 reqd)
{
   m_AvOverSReqd = reqd;
}

Float64 pgsVerticalShearArtifact::GetAvOverSReqd() const
{
   return m_AvOverSReqd;
}

void pgsVerticalShearArtifact::SetDemand(Float64 demand)
{
   m_Demand = demand;
}

Float64 pgsVerticalShearArtifact::GetDemand() const
{
   return m_Demand;
}

void pgsVerticalShearArtifact::SetCapacity(Float64 cap)
{
   m_Capacity = cap;
}

Float64 pgsVerticalShearArtifact::GetCapacity() const
{
   return m_Capacity;
}

void pgsVerticalShearArtifact::SetEndSpacing(Float64 AvS_provided,Float64 AvS_at_CS)
{
   m_bEndSpacingApplicable = true;
   m_AvSprovided = AvS_provided;
   m_AvSatCS     = AvS_at_CS;
}

void pgsVerticalShearArtifact::GetEndSpacing(Float64* pAvS_provided,Float64* pAvS_at_CS)
{
   *pAvS_provided = m_AvSprovided;
   *pAvS_at_CS    = m_AvSatCS;
}

bool pgsVerticalShearArtifact::IsInCriticalSectionZone() const
{
   return m_bEndSpacingApplicable;
}

bool pgsVerticalShearArtifact::DidAvsDecreaseAtEnd() const
{
   if ( m_bEndSpacingApplicable && m_AvSprovided < m_AvSatCS )
   {
      return true;
   }

   return false;
}

bool pgsVerticalShearArtifact::Passed() const
{
   if (DidAvsDecreaseAtEnd())
   {
      return false;
   }

   if ( m_bIsApplicable )
   {
      if ( m_bIsStrutAndTieRequired )
      {
         return false;
      }

      if (m_AreStirrupsReqd && !m_AreStirrupsProvided)
      {
         return false;
      }

      if (m_Capacity < m_Demand)
      {
         return false;
      }
   }

   return true;
}

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsVerticalShearArtifact::AssertValid() const
{
   return true;
}

void pgsVerticalShearArtifact::Dump(WBFL::Debug::LogContext& os) const
{
   os << "Dump for pgsVerticalShearArtifact" << WBFL::Debug::endl;
}
#endif // _DEBUG


/****************************************************************************
CLASS
   pgsHorizontalShearArtifact
****************************************************************************/

//======================== OPERATIONS =======================================
bool pgsHorizontalShearArtifact::IsApplicable() const
{
   return m_IsApplicable;
}

void pgsHorizontalShearArtifact::SetApplicability(bool isApplicable)
{
   m_IsApplicable = isApplicable;
}

bool pgsHorizontalShearArtifact::SpacingPassed() const
{
   if (m_IsApplicable && 0.0 < GetAvOverS())
   {
      return GetSpacing() <= m_Smax;
   }
   else
   {
      // No bars to space
      return false;
   }
}

int pgsHorizontalShearArtifact::MinReinforcementPassed() const
{
   if (m_IsApplicable)
   {
      return Is5_7_4_1_4Applicable() ? (m_AvOverSMin <= GetAvOverS()) : -1; // -1 = not applicable
   }
   else
   {
      return true;
   }
}

bool pgsHorizontalShearArtifact::StrengthPassed() const
{
   if (m_IsApplicable)
   {
      Float64 demand = abs(GetDemand());
      bool cap = demand <= GetCapacity();
      return cap;
   }
   else
   {
      return true;
   }
}

bool pgsHorizontalShearArtifact::Passed() const
{
   if (DidAvsDecreaseAtEnd())
   {
      return false;
   }

   if (m_IsApplicable)
   {
      bool cap = StrengthPassed();
      bool spc = SpacingPassed();
      int  avf = MinReinforcementPassed();

      if ( avf < 0 )
      {
         return cap && spc;
      }
      else
      {
         return cap && avf && spc;
      }
   }
   else
   {
      return true;
   }
}

Float64 pgsHorizontalShearArtifact::GetNominalCapacity() const
{
   ASSERTVALID;
   return Min(m_Vn1, m_Vn2, m_Vn3);
}

Float64 pgsHorizontalShearArtifact::GetCapacity() const
{
   ASSERTVALID;
   return m_Phi * GetNominalCapacity();
}

Float64 pgsHorizontalShearArtifact::GetAvOverS() const
{
   Float64 avsg=0.0;
   Float64 avstf=0.0;
   if (0.0 < m_AvfGirder)
   {
      ATLASSERT(0.0 < m_SGirder);
      avsg = m_AvfGirder/m_SGirder;
   }

   if (0.0 < m_AvfAdditional)
   {
      ATLASSERT(0.0 < m_SAdditional);
      avstf = m_AvfAdditional/m_SAdditional;
   }

   return avsg + avstf;
}

Float64 pgsHorizontalShearArtifact::GetSpacing() const
{
   if (0.0 < m_AvfGirder && 0.0 < m_AvfAdditional)
   {
      return Min(m_SGirder, m_SAdditional);
   }
   else
   {
      return Max(m_SGirder, m_SAdditional);
   }
}

//======================== ACCESS     =======================================
Float64 pgsHorizontalShearArtifact::GetFc() const
{
   return m_Fc;
}

void pgsHorizontalShearArtifact::SetFc(Float64 fc)
{
   m_Fc = fc;
}

void pgsHorizontalShearArtifact::GetVn(Float64* pVn1, Float64* pVn2, Float64* pVn3) const
{
   ASSERTVALID;
   *pVn1=m_Vn1; 
   *pVn2=m_Vn2; 
   *pVn3=m_Vn3;
}

void pgsHorizontalShearArtifact::SetVn(Float64 Vn1, Float64 Vn2, Float64 Vn3) 
{
   m_Vn1=Vn1; 
   m_Vn2=Vn2; 
   m_Vn3=Vn3;

   ASSERTVALID;
}

Float64 pgsHorizontalShearArtifact::GetBv() const
{
   return m_Bv;
}

void pgsHorizontalShearArtifact::SetBv(Float64 bv)
{
   m_Bv = bv;
}

Float64 pgsHorizontalShearArtifact::GetSAdditional() const
{
   return m_SAdditional;
}

void pgsHorizontalShearArtifact::SetSAdditional(Float64 s)
{
   m_SAdditional = s;
}

Float64 pgsHorizontalShearArtifact::GetSGirder() const
{
   return m_SGirder;
}

void pgsHorizontalShearArtifact::SetSGirder(Float64 s)
{
   m_SGirder = s;
}

Float64 pgsHorizontalShearArtifact::GetSmax() const
{
   return m_Smax;
}

void pgsHorizontalShearArtifact::SetSmax(Float64 sMax)
{
   m_Smax = sMax;
}

Float64 pgsHorizontalShearArtifact::GetFy() const
{
   return m_Fy;
}

void pgsHorizontalShearArtifact::SetFy(Float64 fy)
{
   m_Fy = fy;
}

bool pgsHorizontalShearArtifact::WasFyLimited() const
{
   return m_bWasFyLimited;
}

void pgsHorizontalShearArtifact::WasFyLimited(bool bWasLimited)
{
   m_bWasFyLimited = bWasLimited;
}

bool pgsHorizontalShearArtifact::Is5_7_4_1_4Applicable() const
{
   return m_VsLimit <= m_VsAvg;
}

Float64 pgsHorizontalShearArtifact::GetAvOverSMin_5_7_4_2_1() const
{
   return m_AvOverSMin_5_7_4_2_1;
}

void pgsHorizontalShearArtifact::SetAvOverSMin_5_7_4_2_1(Float64 fmin)
{
   m_AvOverSMin_5_7_4_2_1 = fmin;
}

Float64 pgsHorizontalShearArtifact::GetAvOverSMin_5_7_4_1_3() const
{
   return m_AvOverSMin_5_7_4_1_3;
}

void pgsHorizontalShearArtifact::SetAvOverSMin_5_7_4_1_3(Float64 fmin)
{
   m_AvOverSMin_5_7_4_1_3 = fmin;
}


Float64 pgsHorizontalShearArtifact::GetAvOverSMin() const
{
   return m_AvOverSMin;
}

void pgsHorizontalShearArtifact::SetAvOverSMin(Float64 fmin)
{
   m_AvOverSMin = fmin;
}

Float64 pgsHorizontalShearArtifact::GetNumLegs() const
{
   return m_NumLegs;
}

void pgsHorizontalShearArtifact::SetNumLegs(Float64 legs)
{
   m_NumLegs = legs;
}

Float64 pgsHorizontalShearArtifact::GetNumLegsReqd() const
{
   return m_NumLegsReqd;
}

void pgsHorizontalShearArtifact::SetNumLegsReqd(Float64 legs)
{
   m_NumLegsReqd = legs;
}

bool pgsHorizontalShearArtifact::DoAllPrimaryStirrupsEngageDeck() const
{
   return m_bDoAllPrimaryStirrupsEngageDeck;
}

void pgsHorizontalShearArtifact::SetDoAllPrimaryStirrupsEngageDeck(bool doEngage)
{
   m_bDoAllPrimaryStirrupsEngageDeck = doEngage;
}

bool pgsHorizontalShearArtifact::IsTopFlangeRoughened() const
{
   return m_bIsTopFlangeRoughened;
}

void pgsHorizontalShearArtifact::SetIsTopFlangeRoughened(bool doIsRough)
{
   m_bIsTopFlangeRoughened = doIsRough;
}

Float64 pgsHorizontalShearArtifact::GetVsAvg() const
{
   return m_VsAvg;
}

void pgsHorizontalShearArtifact::SetVsAvg(Float64 vsavg)
{
   m_VsAvg = vsavg;
}

Float64 pgsHorizontalShearArtifact::GetVsLimit() const // max shear strength at which 5.7.4.3-4 (pre2017: 5.8.4.1-4) is not applicable
{
   return m_VsLimit;
}

void pgsHorizontalShearArtifact::SetVsLimit(Float64 vs)
{
   m_VsLimit = vs;
}

Float64 pgsHorizontalShearArtifact::GetVu() const
{
   return m_Vu;
}

void pgsHorizontalShearArtifact::SetVu(const Float64& vu)
{
   m_Vu = vu;
}

Float64 pgsHorizontalShearArtifact::GetDv() const
{
   return m_Dv;
}

void pgsHorizontalShearArtifact::SetDv(Float64 dv)
{
   m_Dv = dv;
}

Float64 pgsHorizontalShearArtifact::GetI() const
{
   return m_I;
}

void pgsHorizontalShearArtifact::SetI(Float64 i)
{
   m_I = i;
}

Float64 pgsHorizontalShearArtifact::GetQ() const
{
   return m_Q;
}

void pgsHorizontalShearArtifact::SetQ(Float64 q)
{
   m_Q = q;
}

Float64 pgsHorizontalShearArtifact::GetK1() const
{
   return m_K1;
}

void pgsHorizontalShearArtifact::SetK1(Float64 k1)
{
   m_K1 = k1;
}

Float64 pgsHorizontalShearArtifact::GetK2() const
{
   return m_K2;
}

void pgsHorizontalShearArtifact::SetK2(Float64 k2)
{
   m_K2 = k2;
}

Float64 pgsHorizontalShearArtifact::GetAvOverSReqd() const
{
   return m_AvsReqd;
}

void pgsHorizontalShearArtifact::SetAvOverSReqd(const Float64& vu)
{
   m_AvsReqd = vu;
}

void pgsHorizontalShearArtifact::SetEndSpacing(Float64 AvS_provided,Float64 AvS_at_CS)
{
   m_bEndSpacingApplicable = true;
   m_AvSprovided = AvS_provided;
   m_AvSatCS     = AvS_at_CS;
}

void pgsHorizontalShearArtifact::GetEndSpacing(Float64* pAvS_provided,Float64* pAvS_at_CS)
{
   *pAvS_provided = m_AvSprovided;
   *pAvS_at_CS    = m_AvSatCS;
}

bool pgsHorizontalShearArtifact::DidAvsDecreaseAtEnd() const
{
   if ( m_bEndSpacingApplicable && m_AvSprovided < m_AvSatCS )
   {
      return true;
   }

   return false;
}

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsHorizontalShearArtifact::AssertValid() const
{
   if (m_Vn1 < 0)
   {
      return false;
   }

   if (m_Vn2 < 0)
   {
      return false;
   }

   if (m_Vn3 < 0)
   {
      return false;
   }

   return true;
}

void pgsHorizontalShearArtifact::Dump(WBFL::Debug::LogContext& os) const
{
   os << "Dump for pgsHorizontalShearArtifact" << WBFL::Debug::endl;
}
#endif // _DEBUG



/****************************************************************************
CLASS
   pgsStirrupDetailArtifact
****************************************************************************/


bool pgsStirrupDetailArtifact::Passed() const
{
   if ( IsApplicable() )
   {
      if (m_Avs < m_AvsMin)
      {
         return false;
      }
   }

   // only check required stirrups spacing if stirrups are required
   // stirrups are required when Av/S min is > 0
   if (0 < m_AvsMin)
   {
      if (::IsLT(m_SMax,m_S,TOLERANCE))
   {
      return false;
   }

      if (::IsGT(m_S,m_SMin,TOLERANCE))
   {
      return false;
      }
   }

   return true;
}

//======================== ACCESS     =======================================

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsStirrupDetailArtifact::AssertValid() const
{
   return true;
}

void pgsStirrupDetailArtifact::Dump(WBFL::Debug::LogContext& os) const
{
   os << "Dump for pgsStirrupDetailArtifact" << WBFL::Debug::endl;
}
#endif // _DEBUG

/****************************************************************************
CLASS
   pgsStirrupCheckAtPoisArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
#pragma Reminder("UPDATE: rename this class to pgsSectionalShearCheckArtifact")
// this class encapulates the results of section based shear checks.... it does more than
// check stirrups

bool pgsStirrupCheckAtPoisArtifact::operator<(const pgsStirrupCheckAtPoisArtifact& artifact) const
{
   // this operator is used for sorting the artifacts in their container
   // sort by POI
   return m_Poi < artifact.m_Poi;
}

const pgsPointOfInterest& pgsStirrupCheckAtPoisArtifact::GetPointOfInterest() const
{
   return m_Poi;
}

void pgsStirrupCheckAtPoisArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

//======================== OPERATIONS =======================================
void pgsStirrupCheckAtPoisArtifact::SetVerticalShearArtifact(const pgsVerticalShearArtifact& artifact)
{
   m_VerticalShearArtifact = artifact;
}

const pgsVerticalShearArtifact* pgsStirrupCheckAtPoisArtifact::GetVerticalShearArtifact() const
{
   return &m_VerticalShearArtifact;
}

void pgsStirrupCheckAtPoisArtifact::SetHorizontalShearArtifact(const pgsHorizontalShearArtifact& artifact)
{
   m_HorizontalShearArtifact = artifact;
}

const pgsHorizontalShearArtifact* pgsStirrupCheckAtPoisArtifact::GetHorizontalShearArtifact() const
{
   return &m_HorizontalShearArtifact;
}

void pgsStirrupCheckAtPoisArtifact::SetStirrupDetailArtifact(const pgsStirrupDetailArtifact& artifact)
{
   m_StirrupDetailArtifact = artifact;
}

const pgsStirrupDetailArtifact* pgsStirrupCheckAtPoisArtifact::GetStirrupDetailArtifact() const
{
   return &m_StirrupDetailArtifact;
}

void pgsStirrupCheckAtPoisArtifact::SetLongReinfShearArtifact( const pgsLongReinfShearArtifact& artifact)
{
   m_LongReinfShearArtifact = artifact;
}

const pgsLongReinfShearArtifact* pgsStirrupCheckAtPoisArtifact::GetLongReinfShearArtifact() const
{
   return &m_LongReinfShearArtifact;
}

bool pgsStirrupCheckAtPoisArtifact::Passed() const
{
   if (!m_HorizontalShearArtifact.Passed())
   {
      return false;
   }

   if (!m_VerticalShearArtifact.Passed())
   {
      return false;
   }

   if (!m_StirrupDetailArtifact.Passed())
   {
      return false;
   }

   if (/*m_LongReinfShearArtifact.IsApplicable() &&*/ !m_LongReinfShearArtifact.Passed())
   {
      return false;
   }

   return true;
}

//======================== ACCESS     =======================================

//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsStirrupCheckAtPoisArtifact::AssertValid() const
{
   return true;
}

void pgsStirrupCheckAtPoisArtifact::Dump(WBFL::Debug::LogContext& os) const
{
   os << "Dump for pgsStirrupCheckAtPoisArtifact" << WBFL::Debug::endl;
}
#endif // _DEBUG
