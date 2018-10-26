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

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLongReinfShearArtifact::pgsLongReinfShearArtifact():
m_Fy(0),
m_As(0),
m_Aps(0),
m_Fps(0),
m_Apt(0),
m_Fpt(0),
m_Mu(0),
m_Mr(0),
m_Dv(0),
m_FlexuralPhi(0),
m_Nu(0),
m_AxialPhi(0),
m_Vu(0),
m_ShearPhi(0),
m_Vs(0),
m_Vp(0),
m_Theta(0),
m_DemandForce(0),
m_CapacityForce(0),
m_IsApplicable(false)
{
}

pgsLongReinfShearArtifact::pgsLongReinfShearArtifact(const pgsLongReinfShearArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsLongReinfShearArtifact::~pgsLongReinfShearArtifact()
{
}

//======================== OPERATORS  =======================================
pgsLongReinfShearArtifact& pgsLongReinfShearArtifact::operator= (const pgsLongReinfShearArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================


Float64 pgsLongReinfShearArtifact::GetFy() const
{
   return m_Fy;
}

void pgsLongReinfShearArtifact::SetFy(Float64 fy)
{
   m_Fy = fy;
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

Float64 pgsLongReinfShearArtifact::GetApt() const
{
   return m_Apt;
}

void pgsLongReinfShearArtifact::SetApt(Float64 apt)
{
   m_Apt = apt;
}

Float64 pgsLongReinfShearArtifact::GetFpt() const
{
   return m_Fpt;
}

void pgsLongReinfShearArtifact::SetFpt(Float64 fpt)
{
   m_Fpt = fpt;
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

void pgsLongReinfShearArtifact::SetDemandForce(Float64 area)
{
   m_DemandForce = area;
}

Float64 pgsLongReinfShearArtifact::GetDemandForce() const
{
   return m_DemandForce;
}

void pgsLongReinfShearArtifact::SetCapacityForce(Float64 area)
{
   m_CapacityForce = area;
}

Float64 pgsLongReinfShearArtifact::GetCapacityForce() const
{
   return m_CapacityForce;
}

bool pgsLongReinfShearArtifact::Passed() const
{
   if ( !m_IsApplicable ) 
   {
      return true;
   }

   if ( m_Equation == 1 && fabs(m_Mu) <= fabs(m_Mr) && !IsZero(m_Mr) )
   {
      return true;
   }

   return m_DemandForce <= m_CapacityForce;
}

bool pgsLongReinfShearArtifact::IsApplicable() const
{
   return m_IsApplicable;
}

void pgsLongReinfShearArtifact::SetApplicability(bool isApplicable)
{
   m_IsApplicable = isApplicable;
}


//======================== INQUIRY    =======================================
//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLongReinfShearArtifact::AssertValid() const
{
   return true;
}

void pgsLongReinfShearArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLongReinfShearArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLongReinfShearArtifact::MakeCopy(const pgsLongReinfShearArtifact& rOther)
{
   m_Fy = rOther.m_Fy;
   m_As = rOther.m_As;
   m_Aps = rOther.m_Aps;
   m_Fps = rOther.m_Fps;
   m_Apt = rOther.m_Apt;
   m_Fpt = rOther.m_Fpt;
   m_Mu = rOther.m_Mu;
   m_Mr = rOther.m_Mr;
   m_Dv = rOther.m_Dv;
   m_FlexuralPhi = rOther.m_FlexuralPhi;
   m_Nu = rOther.m_Nu;
   m_AxialPhi = rOther.m_AxialPhi;
   m_Vu = rOther.m_Vu;
   m_ShearPhi = rOther.m_ShearPhi;
   m_Vs = rOther.m_Vs;
   m_Vp = rOther.m_Vp;
   m_Theta = rOther.m_Theta;
   m_Equation = rOther.m_Equation;
   m_DemandForce = rOther.m_DemandForce;
   m_CapacityForce = rOther.m_CapacityForce;
   m_IsApplicable = rOther.m_IsApplicable;
}

void pgsLongReinfShearArtifact::MakeAssignment(const pgsLongReinfShearArtifact& rOther)
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

/****************************************************************************
CLASS
   pgsVerticalShearArtifact
****************************************************************************/

#include <PGSExt\StirrupCheckAtPoisArtifact.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsVerticalShearArtifact::pgsVerticalShearArtifact() :
m_bIsApplicable(true),
m_AvOverSReqd(0.0),
m_Capacity(0.0),
m_Demand(0.0),
m_bIsStrutAndTieRequired(false),
m_bEndSpacingApplicable(false),
m_AvSprovided(0.0),
m_AvSatCS(0.0)
{
}

pgsVerticalShearArtifact::pgsVerticalShearArtifact(const pgsVerticalShearArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsVerticalShearArtifact::~pgsVerticalShearArtifact()
{
}

//======================== OPERATORS  =======================================
pgsVerticalShearArtifact& pgsVerticalShearArtifact::operator= (const pgsVerticalShearArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
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

void pgsVerticalShearArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsVerticalShearArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsVerticalShearArtifact::MakeCopy(const pgsVerticalShearArtifact& rOther)
{
   m_bIsApplicable = rOther.m_bIsApplicable;

   m_bIsStrutAndTieRequired = rOther.m_bIsStrutAndTieRequired;
   m_AvSprovided            = rOther.m_AvSprovided;
   m_AvSatCS                = rOther.m_AvSatCS;
   m_bEndSpacingApplicable  = rOther.m_bEndSpacingApplicable;

   m_AreStirrupsReqd     = rOther.m_AreStirrupsReqd;
   m_AreStirrupsProvided = rOther.m_AreStirrupsProvided;

   m_AvOverSReqd = rOther.m_AvOverSReqd;

   m_Demand   = rOther.m_Demand;
   m_Capacity = rOther.m_Capacity;
}

void pgsVerticalShearArtifact::MakeAssignment(const pgsVerticalShearArtifact& rOther)
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

/****************************************************************************
CLASS
   pgsHorizontalShearArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsHorizontalShearArtifact::pgsHorizontalShearArtifact():
m_AvfAdditional(0),
m_SAdditional(0),
m_AvfGirder(0),
m_SGirder(0),
m_NormalCompressionForce(0),
m_Acv(0),
m_CohesionFactor(0),
m_FrictionFactor(0),
m_K1(0),
m_K2(0),
m_Phi(0),
m_Vn1(0),
m_Vn2(0),
m_Vn3(0),
m_Fc(0),
m_Bv(0),
m_Smax(0),
m_Fy(0),
m_bWasFyLimited(false),
m_AvOverSMin_5_8_4_4_1(0),
m_AvOverSMin_5_8_4_1_3(0),
m_AvOverSMin(0),
m_NumLegs(0),
m_NumLegsReqd(0),
m_VsAvg(0),
m_VsLimit(0),
m_Dv(0),
m_I(0),
m_Q(0),
m_UltimateHorizontalShear(0),
m_AvsReqd(0.0),
m_bDoAllPrimaryStirrupsEngageDeck(false),
m_bIsTopFlangeRoughened(false),
m_IsApplicable(false),
m_bEndSpacingApplicable(false),
m_AvSprovided(0.0),
m_AvSatCS(0.0)
{
}

pgsHorizontalShearArtifact::pgsHorizontalShearArtifact(const pgsHorizontalShearArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsHorizontalShearArtifact::~pgsHorizontalShearArtifact()
{
}

//======================== OPERATORS  =======================================
pgsHorizontalShearArtifact& pgsHorizontalShearArtifact::operator= (const pgsHorizontalShearArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

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
      return Is5_8_4_1_4Applicable() ? (m_AvOverSMin <= GetAvOverS()) : -1; // -1 = not applicable
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

Float64 pgsHorizontalShearArtifact::GetCapacity() const
{
   ASSERTVALID;
   return m_Phi * Min(m_Vn1, m_Vn2, m_Vn3);
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

bool pgsHorizontalShearArtifact::Is5_8_4_1_4Applicable() const
{
   return m_VsLimit <= m_VsAvg;
}

Float64 pgsHorizontalShearArtifact::GetAvOverSMin_5_8_4_4_1() const
{
   return m_AvOverSMin_5_8_4_4_1;
}

void pgsHorizontalShearArtifact::SetAvOverSMin_5_8_4_4_1(Float64 fmin)
{
   m_AvOverSMin_5_8_4_4_1 = fmin;
}

Float64 pgsHorizontalShearArtifact::GetAvOverSMin_5_8_4_1_3() const
{
   return m_AvOverSMin_5_8_4_1_3;
}

void pgsHorizontalShearArtifact::SetAvOverSMin_5_8_4_1_3(Float64 fmin)
{
   m_AvOverSMin_5_8_4_1_3 = fmin;
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

Float64 pgsHorizontalShearArtifact::GetVsLimit() const // max shear strength at which 5.8.4.1-4 is not applicable
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

void pgsHorizontalShearArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsHorizontalShearArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsHorizontalShearArtifact::MakeCopy(const pgsHorizontalShearArtifact& rOther)
{
   m_IsApplicable            = rOther.m_IsApplicable;
   m_UltimateHorizontalShear = rOther.m_UltimateHorizontalShear;
   m_NormalCompressionForce  = rOther.m_NormalCompressionForce;
   m_Acv                     = rOther.m_Acv;
   m_CohesionFactor          = rOther.m_CohesionFactor;
   m_FrictionFactor          = rOther.m_FrictionFactor;
   m_K1                      = rOther.m_K1;
   m_K2                      = rOther.m_K2;
   m_Phi                     = rOther.m_Phi;
   m_Vn1                     = rOther.m_Vn1;
   m_Vn2                     = rOther.m_Vn2;
   m_Vn3                     = rOther.m_Vn3;
   m_Fc                      = rOther.m_Fc;

   m_Bv           = rOther.m_Bv;
   m_AvfAdditional = rOther.m_AvfAdditional;
   m_SAdditional   = rOther.m_SAdditional;
   m_AvfGirder    = rOther.m_AvfGirder;
   m_SGirder      = rOther.m_SGirder;
   m_Smax         = rOther.m_Smax;
   m_Fy           = rOther.m_Fy;
   m_bWasFyLimited = rOther.m_bWasFyLimited;
   m_AvOverSMin_5_8_4_4_1   = rOther.m_AvOverSMin_5_8_4_4_1;
   m_AvOverSMin_5_8_4_1_3   = rOther.m_AvOverSMin_5_8_4_1_3;
   m_AvOverSMin   = rOther.m_AvOverSMin;
   m_NumLegs      = rOther.m_NumLegs;
   m_NumLegsReqd  = rOther.m_NumLegsReqd;
   m_VsAvg        = rOther.m_VsAvg;
   m_VsLimit      = rOther.m_VsLimit;

   m_bDoAllPrimaryStirrupsEngageDeck = rOther.m_bDoAllPrimaryStirrupsEngageDeck;
   m_bIsTopFlangeRoughened = rOther.m_bIsTopFlangeRoughened;

   m_Dv = rOther.m_Dv;
   m_I = rOther.m_I;
   m_Q = rOther.m_Q;
   m_Vu = rOther.m_Vu;

   m_AvsReqd = rOther.m_AvsReqd;

   m_AvSprovided            = rOther.m_AvSprovided;
   m_AvSatCS                = rOther.m_AvSatCS;
   m_bEndSpacingApplicable  = rOther.m_bEndSpacingApplicable;
}

void pgsHorizontalShearArtifact::MakeAssignment(const pgsHorizontalShearArtifact& rOther)
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



/****************************************************************************
CLASS
   pgsStirrupDetailArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsStirrupDetailArtifact::pgsStirrupDetailArtifact()
{
}

pgsStirrupDetailArtifact::pgsStirrupDetailArtifact(const pgsStirrupDetailArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupDetailArtifact::~pgsStirrupDetailArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStirrupDetailArtifact& pgsStirrupDetailArtifact::operator= (const pgsStirrupDetailArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

bool pgsStirrupDetailArtifact::Passed() const
{
   if ( IsApplicable() )
   {
      if (m_Avs < m_AvsMin)
      {
         return false;
      }
   }

   // always check spacing requirements
   if (m_SMax < m_S-TOLERANCE)
   {
      return false;
   }

   if (m_S+TOLERANCE < m_SMin)
   {
      return false;
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

void pgsStirrupDetailArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStirrupDetailArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStirrupDetailArtifact::MakeCopy(const pgsStirrupDetailArtifact& rOther)
{
   m_Fy = rOther.m_Fy;
   m_Fc = rOther.m_Fc;
   m_AvsMin = rOther.m_AvsMin;
   m_Avs = rOther.m_Avs;
   m_SMax = rOther.m_SMax;
   m_SMin = rOther.m_SMin;
   m_BarSize = rOther.m_BarSize;
   m_S = rOther.m_S;
   m_Bv = rOther.m_Bv;
   m_Dv = rOther.m_Dv;
   m_Vu = rOther.m_Vu;
   m_VuLimit = rOther.m_VuLimit;
   m_IsApplicable = rOther.m_IsApplicable;
   m_IsInCritialSectionZone = rOther.m_IsInCritialSectionZone;
}

void pgsStirrupDetailArtifact::MakeAssignment(const pgsStirrupDetailArtifact& rOther)
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



/****************************************************************************
CLASS
   pgsStirrupCheckAtPoisArtifact
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
#pragma Reminder("UPDATE: rename this class to pgsSectionalShearCheckArtifact")
// this class encapulates the results of section based shear checks.... it does more than
// check stirrups
pgsStirrupCheckAtPoisArtifact::pgsStirrupCheckAtPoisArtifact()
{
}

pgsStirrupCheckAtPoisArtifact::pgsStirrupCheckAtPoisArtifact(const pgsStirrupCheckAtPoisArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupCheckAtPoisArtifact::~pgsStirrupCheckAtPoisArtifact()
{
}

//======================== OPERATORS  =======================================
pgsStirrupCheckAtPoisArtifact& pgsStirrupCheckAtPoisArtifact::operator= (const pgsStirrupCheckAtPoisArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

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

void pgsStirrupCheckAtPoisArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsStirrupCheckAtPoisArtifact" << endl;
}
#endif // _DEBUG

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsStirrupCheckAtPoisArtifact::MakeCopy(const pgsStirrupCheckAtPoisArtifact& rOther)
{
   m_Poi                     = rOther.m_Poi;
   m_VerticalShearArtifact   = rOther.m_VerticalShearArtifact;
   m_HorizontalShearArtifact = rOther.m_HorizontalShearArtifact;
   m_StirrupDetailArtifact   = rOther.m_StirrupDetailArtifact;
   m_LongReinfShearArtifact  = rOther.m_LongReinfShearArtifact;
}

void pgsStirrupCheckAtPoisArtifact::MakeAssignment(const pgsStirrupCheckAtPoisArtifact& rOther)
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

